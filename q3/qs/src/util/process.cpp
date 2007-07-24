/*
 * $Id$
 *
 * Copyright(C) 1998-2007 Satoshi Nakamura
 * All rights reserved.
 *
 */

#include <qsosutil.h>
#include <qsstream.h>

#include <boost/tuple/tuple.hpp>

using namespace qs;


/****************************************************************************
 *
 * Process
 *
 */

#ifndef _WIN32_WCE

namespace {

DWORD WINAPI readProc(void* pParam)
{
	std::auto_ptr<boost::tuple<Process::PFN_WRITE, void*, HANDLE> > p(
		static_cast<boost::tuple<Process::PFN_WRITE, void*, HANDLE>*>(pParam));
	Process::PFN_WRITE pfnWrite = p->get<0>();
	void* pWriteParam = p->get<1>();
	AutoHandle hOutput(p->get<2>());
	
	unsigned char buf[1024];
	while (true) {
		DWORD dwRead = 0;
		BOOL b = ::ReadFile(hOutput.get(), buf, sizeof(buf), &dwRead, 0);
		if (!b && ::GetLastError() != ERROR_BROKEN_PIPE)
			return 1;
		else if (!b || dwRead == 0)
			break;
		
		if ((*pfnWrite)(buf, dwRead, pWriteParam) == -1)
			return 1;
	}
	
	return 0;
}

DWORD WINAPI writeProc(void* pParam)
{
	std::auto_ptr<boost::tuple<Process::PFN_READ, void*, HANDLE> > p(
		static_cast<boost::tuple<Process::PFN_READ, void*, HANDLE>*>(pParam));
	Process::PFN_READ pfnRead = p->get<0>();
	void* pReadParam = p->get<1>();
	AutoHandle hInput(p->get<2>());
	
	unsigned char buf[1024];
	while (true) {
		size_t nLen = (*pfnRead)(buf, sizeof(buf), pReadParam);
		if (nLen == 0)
			break;
		else if (nLen == -1)
			return 1;
		
		DWORD dwWritten = 0;
		if (!::WriteFile(hInput.get(), buf, static_cast<DWORD>(nLen), &dwWritten, 0) || dwWritten != nLen)
			return 1;
	}
	
	return 0;
}

size_t readStreamProc(unsigned char* p,
					  size_t n,
					  void* pParam)
{
	OutputStream* pOutputStream = static_cast<OutputStream*>(pParam);
	return pOutputStream->write(p, n);
}

size_t writeStreamProc(unsigned char* p,
					   size_t n,
					   void* pParam)
{
	InputStream* pInputStream = static_cast<InputStream*>(pParam);
	return pInputStream->read(p, n);
}

}

wstring_ptr qs::Process::exec(const WCHAR* pwszCommand,
							  const WCHAR* pwszInput)
{
	assert(pwszCommand);
	
	const WCHAR* p = pwszInput ? pwszInput : L"";
	string_ptr strInput(wcs2mbs(p));
	ByteInputStream is(reinterpret_cast<unsigned char*>(strInput.get()), strlen(strInput.get()), false);
	ByteOutputStream os;
	
	if (exec(pwszCommand, pwszInput ? &is : 0, &os, 0) != 0)
		return 0;
	
	return mbs2wcs(reinterpret_cast<const CHAR*>(os.getBuffer()), os.getLength());
}

int qs::Process::exec(const WCHAR* pwszCommand,
					  InputStream* pStdInput,
					  OutputStream* pStdOutput,
					  OutputStream* pStdError)
{
	return exec(pwszCommand,
		pStdInput ? writeStreamProc : 0, pStdInput,
		pStdOutput ? readStreamProc : 0, pStdOutput,
		pStdError ? readStreamProc : 0, pStdError, 0, 0);
}

int qs::Process::exec(const WCHAR* pwszCommand,
					  PFN_READ pfnReadStdInput,
					  void* pParamStdInput,
					  PFN_WRITE pfnWriteStdOutput,
					  void* pParamStdOutput,
					  PFN_WRITE pfnWriteStdError,
					  void* pParamStdError,
					  PFN_WAIT pfnWait,
					  void* pParamWait)
{
	assert(pwszCommand);
	
	SECURITY_ATTRIBUTES sa = { sizeof(sa), 0, TRUE };
	
	AutoHandle hStdinRead;
	AutoHandle hStdin;
	if (pfnReadStdInput) {
		AutoHandle hStdinWrite;
		if (!::CreatePipe(&hStdinRead, &hStdinWrite, &sa, 0))
			return -1;
		if (!::DuplicateHandle(::GetCurrentProcess(), hStdinWrite.get(),
			::GetCurrentProcess(), &hStdin, 0, FALSE, DUPLICATE_SAME_ACCESS))
			return -1;
	}
	
	AutoHandle hStdoutWrite;
	AutoHandle hStdout;
	if (pfnWriteStdOutput) {
		AutoHandle hStdoutRead;
		if (!::CreatePipe(&hStdoutRead, &hStdoutWrite, &sa, 0))
			return -1;
		if (!::DuplicateHandle(::GetCurrentProcess(), hStdoutRead.get(),
			::GetCurrentProcess(), &hStdout, 0, FALSE, DUPLICATE_SAME_ACCESS))
			return -1;
	}
	
	AutoHandle hStderrWrite;
	AutoHandle hStderr;
	if (pfnWriteStdError) {
		AutoHandle hStderrRead;
		if (!::CreatePipe(&hStderrRead, &hStderrWrite, &sa, 0))
			return -1;
		if (!::DuplicateHandle(::GetCurrentProcess(), hStderrRead.get(),
			::GetCurrentProcess(), &hStderr, 0, FALSE, DUPLICATE_SAME_ACCESS))
			return -1;
	}
	
	STARTUPINFO si = { sizeof(si) };
	si.dwFlags = STARTF_USESTDHANDLES | STARTF_USESHOWWINDOW;
	si.hStdInput = hStdinRead.get();
	si.hStdOutput = hStdoutWrite.get();
	si.hStdError = hStderrWrite.get();
	si.wShowWindow = SW_HIDE;
	PROCESS_INFORMATION pi;
	tstring_ptr tstrCommand(wcs2tcs(pwszCommand));
	if (!::CreateProcess(0, tstrCommand.get(), 0, 0, TRUE, 0, 0, 0, &si, &pi))
		return -1;
	AutoHandle hProcess(pi.hProcess);
	::CloseHandle(pi.hThread);
	hStdinRead.close();
	hStdoutWrite.close();
	hStderrWrite.close();
	
	HANDLE hThreadStdout = 0;
	if (pfnWriteStdOutput) {
		std::auto_ptr<boost::tuple<PFN_WRITE, void*, HANDLE> > p(
			new boost::tuple<PFN_WRITE, void*, HANDLE>(
				pfnWriteStdOutput, pParamStdOutput, hStdout.get()));
		DWORD dwThreadId = 0;
		hThreadStdout = ::CreateThread(0, 0, &readProc, p.get(), 0, &dwThreadId);
		if (!hThreadStdout)
			return -1;
		p.release();
		hStdout.release();
	}
	AutoHandle ahThreadStdout(hThreadStdout);
	
	HANDLE hThreadStderr = 0;
	if (pfnWriteStdError) {
		std::auto_ptr<boost::tuple<PFN_WRITE, void*, HANDLE> > p(
			new boost::tuple<PFN_WRITE, void*, HANDLE>(
				pfnWriteStdError, pParamStdError, hStderr.get()));
		DWORD dwThreadId = 0;
		hThreadStderr = ::CreateThread(0, 0, &readProc, p.get(), 0, &dwThreadId);
		if (!hThreadStderr)
			return -1;
		p.release();
		hStderr.release();
	}
	AutoHandle ahThreadStderr(hThreadStderr);
	
	HANDLE hThreadStdin = 0;
	if (pfnReadStdInput) {
		std::auto_ptr<boost::tuple<PFN_READ, void*, HANDLE> > p(
			new boost::tuple<PFN_READ, void*, HANDLE>(
				pfnReadStdInput, pParamStdInput, hStdin.get()));
		DWORD dwThreadId = 0;
		hThreadStdin = ::CreateThread(0, 0, &writeProc, p.get(), 0, &dwThreadId);
		if (!hThreadStdin)
			return -1;
		p.release();
		hStdin.release();
	}
	AutoHandle ahThreadStdin(hThreadStdin);
	
	typedef std::vector<HANDLE> HandleList;
	HandleList listHandle;
	listHandle.push_back(hProcess.get());
	if (hThreadStdout)
		listHandle.push_back(hThreadStdout);
	if (hThreadStderr)
		listHandle.push_back(hThreadStderr);
	if (hThreadStdin)
		listHandle.push_back(hThreadStdin);
	
	bool b = true;
	if (pfnWait)
		b = (*pfnWait)(&listHandle[0], listHandle.size(), pParamWait);
	
	::WaitForMultipleObjects(static_cast<DWORD>(listHandle.size()), &listHandle[0], TRUE, INFINITE);
	
	if (!b)
		return -1;
	
	for (HandleList::const_iterator it = listHandle.begin() + 1; it != listHandle.end(); ++it) {
		DWORD dwExitCode = 0;
		if (!::GetExitCodeThread(*it, &dwExitCode) || dwExitCode != 0)
			return -1;
	}
	
	DWORD dwExitCode = 0;
	if (!::GetExitCodeProcess(hProcess.get(), &dwExitCode))
		return -1;
	
	return dwExitCode;
}

#endif

bool qs::Process::shellExecute(const WCHAR* pwszCommand,
							   HWND hwnd)
{
	assert(pwszCommand);
	
	wstring_ptr wstr(allocWString(pwszCommand));
	const WCHAR* pCommand = wstr.get();
	WCHAR* pParam = 0;
	if (*pCommand == L'\"') {
		++pCommand;
		
		pParam = wstr.get() + 1;
		while (*pParam) {
			if (*pParam == L'\"') {
				if (*(pParam + 1) == L' ' || *(pParam + 1) == L'\0')
					break;
			}
			++pParam;
		}
		if (pParam) {
			*pParam = L'\0';
			++pParam;
		}
	}
	else {
		pParam = wstr.get();
		while (*pParam && *pParam != L' ')
			++pParam;
		if (*pParam) {
			*pParam = L'\0';
			++pParam;
		}
	}
	while (*pParam == L' ')
		++pParam;
	
	W2T(pCommand, ptszCommand);
	W2T(pParam, ptszParam);
	
	SHELLEXECUTEINFO sei = {
		sizeof(sei),
		0,
		hwnd,
#ifdef _WIN32_WCE
		_T("open"),
#else
		0,
#endif
		ptszCommand,
		ptszParam,
		0,
		SW_SHOW
	};
	return ::ShellExecuteEx(&sei) != 0;
}
