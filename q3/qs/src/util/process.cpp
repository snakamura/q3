/*
 * $Id$
 *
 * Copyright(C) 1998-2004 Satoshi Nakamura
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
DWORD WINAPI writeProc(void* pParam)
{
	std::auto_ptr<std::pair<InputStream*, HANDLE> > p(
		static_cast<std::pair<InputStream*, HANDLE>*>(pParam));
	InputStream* pInputStream = p->first;
	HANDLE hInput = p->second;
	
	unsigned char buf[1024];
	while (true) {
		size_t nLen = pInputStream->read(buf, sizeof(buf));
		if (nLen == 0)
			break;
		else if (nLen == -1)
			return 1;
		
		DWORD dwWritten = 0;
		if (!::WriteFile(hInput, buf, nLen, &dwWritten, 0) || dwWritten != nLen)
			return 1;
	}
	::CloseHandle(hInput);
	
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
	
	HANDLE hThread = 0;
	if (pStdInput) {
		std::auto_ptr<std::pair<InputStream*, HANDLE> > p(
			new std::pair<InputStream*, HANDLE>(pStdInput, hStdin.get()));
		DWORD dwThreadId = 0;
		hThread = ::CreateThread(0, 0, writeProc, p.get(), 0, &dwThreadId);
		if (!hThread)
			return -1;
		p.release();
		hStdin.release();
	}
	AutoHandle ahThread(hThread);
	
	if (hStdout.get() || hStderr.get()) {
		HANDLE hPipes[2];
		int nPipeCount = 0;
		if (hStdout.get())
			hPipes[nPipeCount++] = hStdout.get();
		if (hStderr.get())
			hPipes[nPipeCount++] = hStderr.get();
		
		unsigned char buf[1024];
		DWORD dwRead = 0;
		while (nPipeCount != 0) {
			HANDLE* p = hPipes;
			if (!*p)
				++p;
			assert(*p);
			
			DWORD dwWait = ::WaitForMultipleObjects(nPipeCount, p, FALSE, INFINITE);
			if (dwWait < WAIT_OBJECT_0 || WAIT_OBJECT_0 + nPipeCount <= dwWait)
				return -1;
			
			int nPipe = dwWait - WAIT_OBJECT_0;
			HANDLE hPipe = p[nPipe];
			BOOL b = ::ReadFile(hPipe, buf, sizeof(buf), &dwRead, 0);
			if (!b && ::GetLastError() != ERROR_BROKEN_PIPE) {
				return -1;
			}
			else if (!b || dwRead == 0) {
				p[nPipe] = 0;
				--nPipeCount;
			}
			OutputStream* pOutputStream = hPipe == hStdout.get() ? pStdOutput : pStdError;
			if (pOutputStream->write(buf, dwRead) == -1)
				return -1;
		}
	}
	
	HANDLE hWaits[] = { hProcess.get(), hThread };
	::WaitForMultipleObjects(hThread ? 2 : 1, hWaits, TRUE, INFINITE);
	
	DWORD dwExitCode = 0;
	if (!::GetExitCodeThread(hThread, &dwExitCode) || dwExitCode != 0)
		return -1;
	if (!::GetExitCodeProcess(hProcess.get(), &dwExitCode))
		return -1;
	
	return dwExitCode;
}

#endif
