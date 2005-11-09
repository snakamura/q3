/*
 * $Id$
 *
 * Copyright(C) 1998-2005 Satoshi Nakamura
 * All rights reserved.
 *
 */

#include <qs.h>
#include <qsassert.h>
#include <qsinit.h>
#include <qsstl.h>
#include <qsstring.h>
#include <qsthread.h>

#include <cstdio>
#include <memory>
#include <vector>

#include <mlang.h>
#include <tchar.h>
#include <windows.h>

using namespace qs;


/****************************************************************************
 *
 * Global Variables
 *
 */

HINSTANCE g_hInstDll = 0;
HINSTANCE g_hInstResourceDll = 0;
Window* g_pMainWindow = 0;
ModalHandler* g_pModalHandler = 0;


/****************************************************************************
 *
 * DllMain
 *
 */

void initProcess(HINSTANCE hInst);
void termProcess();
void initThread();
void termThread();

BOOL WINAPI DllMain(HANDLE hInst,
					DWORD dwReason,
					LPVOID lpReserved)
{
	switch (dwReason) {
	case DLL_PROCESS_ATTACH:
#ifndef NDEBUG
		{
			TCHAR tsz[32];
			wsprintf(tsz, TEXT("qs: %p\n"), hInst);
			::OutputDebugString(tsz);
		}
#endif
		initProcess(static_cast<HINSTANCE>(hInst));
		break;
	case DLL_PROCESS_DETACH:
		termProcess();
		break;
	case DLL_THREAD_ATTACH:
		initThread();
		break;
	case DLL_THREAD_DETACH:
		termThread();
		break;
	}
	return TRUE;
}

void initProcess(HINSTANCE hInst)
{
	g_hInstDll = hInst;
	g_hInstResourceDll = loadResourceDll(hInst);
}

void termProcess()
{
	g_hInstDll = 0;
	g_hInstResourceDll = 0;
}

void initThread()
{
}

void termThread()
{
}


/****************************************************************************
 *
 * Instance
 *
 */

QSEXPORTPROC HINSTANCE qs::getInstanceHandle()
{
	return Init::getInit().getInstanceHandle();
}

QSEXPORTPROC HINSTANCE qs::getDllInstanceHandle()
{
	return g_hInstDll;
}

QSEXPORTPROC HINSTANCE qs::getResourceDllInstanceHandle()
{
	return g_hInstResourceDll;
}

QSEXPORTPROC HINSTANCE qs::loadResourceDll(HINSTANCE hInst)
{
	TCHAR tszPath[MAX_PATH + 10];
	DWORD dwLen = ::GetModuleFileName(hInst, tszPath, MAX_PATH);
	if (dwLen == 0)
		return hInst;
	
	_sntprintf(tszPath + dwLen, countof(tszPath) - dwLen, _T(".%04x.mui"),
		static_cast<unsigned int>(::GetUserDefaultLangID()));
#if !defined _WIN32_WCE && _WIN32_WINNT>=0x500
	HINSTANCE hInstResource = ::LoadLibraryEx(tszPath, 0, LOAD_LIBRARY_AS_DATAFILE);
#else
	HINSTANCE hInstResource = ::LoadLibrary(tszPath);
#endif
	return hInstResource ? hInstResource : hInst;
}


/****************************************************************************
 *
 * Window
 *
 */

QSEXPORTPROC Window* qs::getMainWindow()
{
	return g_pMainWindow;
}

QSEXPORTPROC void qs::setMainWindow(Window* pWindow)
{
	g_pMainWindow = pWindow;
}

QSEXPORTPROC const WCHAR* qs::getTitle()
{
	return Init::getInit().getTitle();
}

QSEXPORTPROC ModalHandler* qs::getModalHandler()
{
	return g_pModalHandler;
}

QSEXPORTPROC void qs::setModalHandler(ModalHandler* pModalHandler)
{
	g_pModalHandler = pModalHandler;
}


/****************************************************************************
 *
 * misc
 *
 */

QSEXPORTPROC const WCHAR* qs::getSystemEncoding()
{
	return Init::getInit().getSystemEncoding();
}
