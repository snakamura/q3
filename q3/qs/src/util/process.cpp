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

#ifndef _WIN32_WCE
#	include <process.h>
#endif

using namespace qs;


/****************************************************************************
 *
 * Process
 *
 */

#ifndef _WIN32_WCE

namespace {

typedef unsigned int (__stdcall *PFN_THREAD)(void*);

template<typename PFN>
AutoHandle createThread(PFN_THREAD pfnThread,
						PFN pfn,
						void* pParam,
						AutoHandle& h)
{
	std::auto_ptr<boost::tuple<PFN, void*, HANDLE> > p(
		new boost::tuple<PFN, void*, HANDLE>(pfn, pParam, h.get()));
	AutoHandle hThread(reinterpret_cast<HANDLE>(
		_beginthreadex(0, 0, pfnThread, p.get(), 0, 0)));
	if (hThread.get()) {
		p.release();
		h.release();
	}
	return hThread;
}

unsigned int __stdcall readProc(void* pParam)
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

unsigned int __stdcall writeProc(void* pParam)
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
	return exec(pwszCommand, pStdInput, pStdOutput, pStdError, 0, 0);
}

int qs::Process::exec(const WCHAR* pwszCommand,
					  InputStream* pStdInput,
					  OutputStream* pStdOutput,
					  OutputStream* pStdError,
					  PFN_WAIT pfnWait,
					  void* pParamWait)
{
	return exec(pwszCommand,
		pStdInput ? writeStreamProc : 0, pStdInput,
		pStdOutput ? readStreamProc : 0, pStdOutput,
		pStdError ? readStreamProc : 0, pStdError,
		pfnWait, pParamWait);
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
	
	AutoHandle hStdinRead;
	AutoHandle hStdin;
	if (pfnReadStdInput) {
		if (!createInheritablePipe(&hStdinRead, &hStdin, true))
			return -1;
	}
	
	AutoHandle hStdoutWrite;
	AutoHandle hStdout;
	if (pfnWriteStdOutput) {
		if (!createInheritablePipe(&hStdout, &hStdoutWrite, false))
			return -1;
	}
	
	AutoHandle hStderrWrite;
	AutoHandle hStderr;
	if (pfnWriteStdError) {
		if (!createInheritablePipe(&hStderr, &hStderrWrite, false))
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
	
	AutoHandle hThreadStdout;
	if (pfnWriteStdOutput)
		hThreadStdout = createReadThread(pfnWriteStdOutput, pParamStdOutput, hStdout);
	
	AutoHandle hThreadStderr;
	if (pfnWriteStdError)
		hThreadStderr = createReadThread(pfnWriteStdError, pParamStdError, hStderr);
	
	AutoHandle hThreadStdin;
	if (pfnReadStdInput)
		hThreadStdin = createWriteThread(pfnReadStdInput, pParamStdInput, hStdin);
	
	typedef std::vector<HANDLE> HandleList;
	HandleList listHandle;
	listHandle.push_back(hProcess.get());
	if (hThreadStdout.get())
		listHandle.push_back(hThreadStdout.get());
	if (hThreadStderr.get())
		listHandle.push_back(hThreadStderr.get());
	if (hThreadStdin.get())
		listHandle.push_back(hThreadStdin.get());
	
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

bool qs::Process::createInheritablePipe(HANDLE* phRead,
										HANDLE* phWrite,
										bool bRead)
{
	assert(phRead);
	assert(phWrite);
	
	AutoHandle hRead;
	AutoHandle hWrite;
	SECURITY_ATTRIBUTES sa = { sizeof(sa), 0, TRUE };
	if (!::CreatePipe(&hRead, &hWrite, &sa, 0))
		return false;
	
	HANDLE hProcess = ::GetCurrentProcess();
	if (bRead) {
		if (!::DuplicateHandle(hProcess, hWrite.get(),
			hProcess, phWrite, 0, FALSE, DUPLICATE_SAME_ACCESS))
			return false;
		*phRead = hRead.release();
	}
	else {
		if (!::DuplicateHandle(hProcess, hRead.get(),
			hProcess, phRead, 0, FALSE, DUPLICATE_SAME_ACCESS))
			return false;
		*phWrite = hWrite.release();
	}
	return true;
}

AutoHandle qs::Process::createWriteThread(PFN_READ pfnRead,
										  void *pParam,
										  AutoHandle& hWrite)
{
	return createThread(&writeProc, pfnRead, pParam, hWrite);
}

AutoHandle qs::Process::createReadThread(PFN_WRITE pfnWrite,
										 void *pParam,
										 AutoHandle& hRead)
{
	return createThread(&readProc, pfnWrite, pParam, hRead);
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
