/*
 * $Id$
 *
 * Copyright(C) 1998-2003 Satoshi Nakamura
 * All rights reserved.
 *
 */

#include <qmmain.h>

#include <qsconv.h>
#include <qserror.h>
#include <qsinit.h>
#include <qsosutil.h>

#include <windows.h>
#include <tchar.h>
#ifndef _WIN32_WCE
#	include <crtdbg.h>
#endif

#include "resource.h"

using namespace qs;


#ifdef _WIN32_WCE
typedef LPWSTR CommandArg;
#else
typedef LPSTR CommandArg;
#endif

int WINAPI WinMain(HINSTANCE hInst, HINSTANCE hInstPrev,
	CommandArg pCommandLine, int nCmdShow)
{
#if !defined NDEBUG && !defined _WIN32_WCE
	int nFlag = _CrtSetDbgFlag(_CRTDBG_REPORT_FLAG);
	nFlag |= _CRTDBG_LEAK_CHECK_DF;
	nFlag &= ~_CRTDBG_CHECK_CRT_DF;
	_CrtSetDbgFlag(nFlag);
//	_CrtSetBreakAlloc(30863);
#endif // !NDEBUG && !_WIN32_WCE
	
	DECLARE_QSTATUS();
	
	AutoHandle hMutex(::CreateMutex(0, TRUE, _T("QMAIL3Mutex")));
	if (!hMutex.get())
		return 1;
	if (::GetLastError() == ERROR_ALREADY_EXISTS) {
		HWND hwnd = ::FindWindow(_T("QmMainWindow"), 0);
		if (hwnd) {
			// TODO
			::SetForegroundWindow(hwnd);
		}
		return 0;
	}
	
	Init init(hInst, L"QMAIL", 0, 0, &status);
	CHECK_QSTATUS_VALUE(1);
	
#ifdef _WIN32_WCE
	const WCHAR* pwszCommandLine = pCommandLine;
#else
	string_ptr<WSTRING> wstrCommandLine(mbs2wcs(pCommandLine));
	if (!wstrCommandLine.get())
		return 1;
	const WCHAR* pwszCommandLine = wstrCommandLine.get();
#endif
	
	return qmMain(pwszCommandLine);
}
