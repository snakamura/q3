/*
 * $Id$
 *
 * Copyright(C) 1998-2003 Satoshi Nakamura
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

QSTATUS qs::Process::exec(const WCHAR* pwszCommand,
	const WCHAR* pwszInput, WSTRING* pwstrOutput)
{
	assert(pwszCommand);
	assert(pwstrOutput);
	
	DECLARE_QSTATUS();
	
	SECURITY_ATTRIBUTES sa = { sizeof(sa), 0, TRUE };
	AutoHandle hInputRead;
	AutoHandle hInputWrite;
	if (!::CreatePipe(&hInputRead, &hInputWrite, &sa, 0))
		return QSTATUS_FAIL;
	AutoHandle hInput;
	if (!::DuplicateHandle(::GetCurrentProcess(), hInputWrite.get(),
		::GetCurrentProcess(), &hInput, 0, FALSE, DUPLICATE_SAME_ACCESS))
		return QSTATUS_FAIL;
	hInputWrite.close();
	
	AutoHandle hOutputRead;
	AutoHandle hOutputWrite;
	if (!::CreatePipe(&hOutputRead, &hOutputWrite, &sa, 0))
		return QSTATUS_FAIL;
	AutoHandle hOutput;
	if (!::DuplicateHandle(::GetCurrentProcess(), hOutputRead.get(),
		::GetCurrentProcess(), &hOutput, 0, FALSE, DUPLICATE_SAME_ACCESS))
		return QSTATUS_FAIL;
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
		return QSTATUS_FAIL;
	AutoHandle hProcess(pi.hProcess);
	::CloseHandle(pi.hThread);
	hInputRead.close();
	hOutputWrite.close();
	
	HANDLE hThread = 0;
	if (pwszInput) {
		string_ptr<STRING> strInput(wcs2mbs(pwszInput));
		if (!strInput.get())
			return QSTATUS_OUTOFMEMORY;
		std::pair<const CHAR*, HANDLE> p(strInput.get(), hInput.get());
		DWORD dwThreadId = 0;
		hThread = ::CreateThread(0, 0, writeProc, &p, 0, &dwThreadId);
		if (!hThread)
			return QSTATUS_FAIL;
		hInput.release();
	}
	AutoHandle ahThread(hThread);
	
	StringBuffer<STRING> bufRead(&status);
	CHECK_QSTATUS();
	char buf[1024];
	DWORD dwRead = 0;
	while (::ReadFile(hOutput.get(), buf, sizeof(buf), &dwRead, 0) && dwRead != 0) {
		status = bufRead.append(buf, dwRead);
		CHECK_QSTATUS();
	}
	string_ptr<WSTRING> wstrOutput(mbs2wcs(bufRead.getCharArray()));
	if (!wstrOutput.get())
		return QSTATUS_OUTOFMEMORY;
	
	HANDLE hWaits[] = { hProcess.get(), hThread };
	::WaitForMultipleObjects(hThread ? 2 : 1, hWaits, TRUE, INFINITE);
	
	DWORD dwExitCode = 0;
	if (!::GetExitCodeProcess(hProcess.get(), &dwExitCode) || dwExitCode != 0)
		return QSTATUS_FAIL;
	
	*pwstrOutput = wstrOutput.release();
	
	return QSTATUS_SUCCESS;
}

#endif
