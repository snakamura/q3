/*
 * $Id: qmail.cpp,v 1.1.1.1 2003/04/29 08:07:31 snakamura Exp $
 *
 * Copyright(C) 1998-2003 Satoshi Nakamura
 * All rights reserved.
 *
 */

#include <qsconv.h>
#include <qserror.h>
#include <qsinit.h>
#include <qsosutil.h>
#include <qsutil.h>

#include <windows.h>
#include <tchar.h>
#ifndef _WIN32_WCE
#	include <crtdbg.h>
#endif

#include "resource.h"

using namespace qs;

#ifndef NDEBUG
extern "C" int qmMain(int);
#endif


namespace qmail {

/****************************************************************************
 *
 * MainCommandLineHandler
 *
 */

class MainCommandLineHandler : public qs::CommandLineHandler
{
public:
	MainCommandLineHandler();
	virtual ~MainCommandLineHandler();

public:
	virtual qs::QSTATUS process(const WCHAR* pwszOption);

private:
	MainCommandLineHandler(const MainCommandLineHandler&);
	MainCommandLineHandler& operator=(const MainCommandLineHandler&);

private:
};

}

qmail::MainCommandLineHandler::MainCommandLineHandler()
{
}

qmail::MainCommandLineHandler::~MainCommandLineHandler()
{
}

qs::QSTATUS qmail::MainCommandLineHandler::process(const WCHAR* pwszOption)
{
	
	
	return qs::QSTATUS_SUCCESS;
}


void showError(HINSTANCE hInst, UINT nId)
{
	TCHAR szTitle[32];
	::LoadString(hInst, IDS_TITLE, szTitle,
		sizeof(szTitle)/sizeof(szTitle[0]));
	
	TCHAR szMessage[128];
	::LoadString(hInst, nId,
		szMessage, sizeof(szMessage)/sizeof(szMessage[0]));
	
	::MessageBox(0, szMessage, szTitle, MB_OK | MB_ICONSTOP);
}

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
//	_CrtSetBreakAlloc(30086);
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
	
	qs::Init init(hInst, L"QMAIL", &status);
	CHECK_QSTATUS_VALUE(1);
	
#ifdef NDEBUG
#	ifdef UNICODE
#		define SUFFIX L"u"
#	else
#		define SUFFIX ""
#	endif
#else
#	ifdef UNICODE
#		define SUFFIX L"ud"
#	else
#		define SUFFIX "d"
#	endif
#endif
	HINSTANCE hInstDll = ::LoadLibrary(_T("qm") SUFFIX _T(".dll"));
	if (!hInstDll) {
		showError(hInst, IDS_ERROR_LOADLIBRARY);
		return 1;
	}
	
	typedef int (*ENTRYPROC)(int);
	ENTRYPROC proc = reinterpret_cast<ENTRYPROC>(
		::GetProcAddress(hInstDll, WCE_T("qmMain")));
	if (!proc) {
		showError(hInst, IDS_ERROR_GETPROCADDRESS);
		return 1;
	}
	
	
#ifdef _WIN32_WCE
	const WCHAR* pwszCommandLine = pCommandLine;
#else
	qs::string_ptr<qs::WSTRING> wstrCommandLine(qs::mbs2wcs(pCommandLine));
	if (!wstrCommandLine.get())
		return 1;
	const WCHAR* pwszCommandLine = wstrCommandLine.get();
#endif
	
	qmail::MainCommandLineHandler handler;
	qs::CommandLine commandLine(&handler, &status);
	CHECK_QSTATUS_VALUE(1);
	status = commandLine.parse(pwszCommandLine);
	CHECK_QSTATUS_VALUE(1);
	
#ifdef NDEBUG
	return (*proc)(nCmdShow);
#else
	return qmMain(nCmdShow);
#endif
}
