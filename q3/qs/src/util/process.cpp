/*
 * $Id$
 *
 * Copyright(C) 1998-2004 Satoshi Nakamura
 * All rights reserved.
 *
 */

#include <qsosutil.h>

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
	std::pair<const CHAR*, HANDLE> p =
		*static_cast<std::pair<const CHAR*, HANDLE>*>(pParam);
	const CHAR* psz = p.first;
	HANDLE hInput = p.second;
	size_t nLen = strlen(psz);
	DWORD dwWrite = 0;
	BOOL b = ::WriteFile(hInput, psz, nLen, &dwWrite, 0);
	::CloseHandle(hInput);
	return (b && dwWrite == nLen) ? 0 : 1;
}
}

wstring_ptr qs::Process::exec(const WCHAR* pwszCommand,
							  const WCHAR* pwszInput)
{
	assert(pwszCommand);
	
	SECURITY_ATTRIBUTES sa = { sizeof(sa), 0, TRUE };
	AutoHandle hInputRead;
	AutoHandle hInputWrite;
	if (!::CreatePipe(&hInputRead, &hInputWrite, &sa, 0))
		return 0;
	AutoHandle hInput;
	if (!::DuplicateHandle(::GetCurrentProcess(), hInputWrite.get(),
		::GetCurrentProcess(), &hInput, 0, FALSE, DUPLICATE_SAME_ACCESS))
		return 0;
	hInputWrite.close();
	
	AutoHandle hOutputRead;
	AutoHandle hOutputWrite;
	if (!::CreatePipe(&hOutputRead, &hOutputWrite, &sa, 0))
		return 0;
	AutoHandle hOutput;
	if (!::DuplicateHandle(::GetCurrentProcess(), hOutputRead.get(),
		::GetCurrentProcess(), &hOutput, 0, FALSE, DUPLICATE_SAME_ACCESS))
		return 0;
	hOutputRead.close();
	
	STARTUPINFO si = { sizeof(si) };
	si.dwFlags = STARTF_USESTDHANDLES | STARTF_USESHOWWINDOW;
	si.hStdInput = hInputRead.get();
	si.hStdOutput = hOutputWrite.get();
	si.wShowWindow = SW_HIDE;
	PROCESS_INFORMATION pi;
	W2T(pwszCommand, ptszCommand);
	if (!::CreateProcess(0, const_cast<LPTSTR>(ptszCommand),
		0, 0, TRUE, 0, 0, 0, &si, &pi))
		return 0;
	AutoHandle hProcess(pi.hProcess);
	::CloseHandle(pi.hThread);
	hInputRead.close();
	hOutputWrite.close();
	
	HANDLE hThread = 0;
	if (pwszInput) {
		string_ptr strInput(wcs2mbs(pwszInput));
		std::pair<const CHAR*, HANDLE> p(strInput.get(), hInput.get());
		DWORD dwThreadId = 0;
		hThread = ::CreateThread(0, 0, writeProc, &p, 0, &dwThreadId);
		if (!hThread)
			return 0;
		hInput.release();
	}
	AutoHandle ahThread(hThread);
	
	StringBuffer<STRING> bufRead;
	char buf[1024];
	DWORD dwRead = 0;
	while (::ReadFile(hOutput.get(), buf, sizeof(buf), &dwRead, 0) && dwRead != 0)
		bufRead.append(buf, dwRead);
	wstring_ptr wstrOutput(mbs2wcs(bufRead.getCharArray()));
	
	HANDLE hWaits[] = { hProcess.get(), hThread };
	::WaitForMultipleObjects(hThread ? 2 : 1, hWaits, TRUE, INFINITE);
	
	DWORD dwExitCode = 0;
	if (!::GetExitCodeProcess(hProcess.get(), &dwExitCode) || dwExitCode != 0)
		return 0;
	
	return wstrOutput;
}

#endif
