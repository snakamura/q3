/*
 * $Id$
 *
 * Copyright(C) 1998-2007 Satoshi Nakamura
 * All rights reserved.
 *
 */

#include <qsosutil.h>
#include <qsstream.h>

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
	std::auto_ptr<std::pair<OutputStream*, HANDLE> > p(
		static_cast<std::pair<OutputStream*, HANDLE>*>(pParam));
	OutputStream* pOutputStream = p->first;
	AutoHandle hOutput(p->second);
	
	while (true) {
		unsigned char buf[1024];
		DWORD dwRead = 0;
		BOOL b = ::ReadFile(hOutput.get(), buf, sizeof(buf), &dwRead, 0);
		if (!b && ::GetLastError() != ERROR_BROKEN_PIPE)
			return 1;
		else if (!b || dwRead == 0)
			break;
		
		if (pOutputStream->write(buf, dwRead) == -1)
			return 1;
	}
	
	return 0;
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
	assert(pwszCommand);
	
	SECURITY_ATTRIBUTES sa = { sizeof(sa), 0, TRUE };
	
	AutoHandle hStdinRead;
	AutoHandle hStdin;
	if (pStdInput) {
		AutoHandle hStdinWrite;
		if (!::CreatePipe(&hStdinRead, &hStdinWrite, &sa, 0))
			return -1;
		if (!::DuplicateHandle(::GetCurrentProcess(), hStdinWrite.get(),
			::GetCurrentProcess(), &hStdin, 0, FALSE, DUPLICATE_SAME_ACCESS))
			return -1;
	}
	
	AutoHandle hStdoutWrite;
	AutoHandle hStdout;
	if (pStdOutput) {
		AutoHandle hStdoutRead;
		if (!::CreatePipe(&hStdoutRead, &hStdoutWrite, &sa, 0))
			return -1;
		if (!::DuplicateHandle(::GetCurrentProcess(), hStdoutRead.get(),
			::GetCurrentProcess(), &hStdout, 0, FALSE, DUPLICATE_SAME_ACCESS))
			return -1;
	}
	
	AutoHandle hStderrWrite;
	AutoHandle hStderr;
	if (pStdError) {
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
	if (pStdOutput) {
		std::auto_ptr<std::pair<OutputStream*, HANDLE> > p(
			new std::pair<OutputStream*, HANDLE>(pStdOutput, hStdout.get()));
		DWORD dwThreadId = 0;
		hThreadStdout = ::CreateThread(0, 0, &readProc, p.get(), 0, &dwThreadId);
		if (!hThreadStdout)
			return -1;
		p.release();
		hStdout.release();
	}
	AutoHandle ahThreadStdout(hThreadStdout);
	
	HANDLE hThreadStderr = 0;
	if (pStdError) {
		std::auto_ptr<std::pair<OutputStream*, HANDLE> > p(
			new std::pair<OutputStream*, HANDLE>(pStdError, hStderr.get()));
		DWORD dwThreadId = 0;
		hThreadStderr = ::CreateThread(0, 0, &readProc, p.get(), 0, &dwThreadId);
		if (!hThreadStderr)
			return -1;
		p.release();
		hStderr.release();
	}
	AutoHandle ahThreadStderr(hThreadStderr);
	
	if (pStdInput) {
		while (true) {
			unsigned char buf[1024];
			size_t nLen = pStdInput->read(buf, sizeof(buf));
			if (nLen == 0)
				break;
			else if (nLen == -1)
				return -1;
			
			DWORD dwWritten = 0;
			if (!::WriteFile(hStdin.get(), buf, static_cast<DWORD>(nLen), &dwWritten, 0) || dwWritten != nLen)
				return -1;
		}
		hStdin.close();
	}
	
	typedef std::vector<HANDLE> HandleList;
	HandleList listHandle;
	listHandle.push_back(hProcess.get());
	if (hThreadStdout)
		listHandle.push_back(hThreadStdout);
	if (hThreadStderr)
		listHandle.push_back(hThreadStderr);
	::WaitForMultipleObjects(static_cast<DWORD>(listHandle.size()), &listHandle[0], TRUE, INFINITE);
	
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
