/*
 * $Id: main.cpp,v 1.2 2003/05/30 06:22:57 snakamura Exp $
 *
 * Copyright(C) 1998-2003 Satoshi Nakamura
 * All rights reserved.
 *
 */

#include <qs.h>
#include <qsinit.h>
#include <qsthread.h>
#include <qserror.h>
#include <qsstl.h>
#include <qsstring.h>
#include <qsnew.h>
#include <qsassert.h>

#include <memory>
#include <vector>

#include <windows.h>
#include <mlang.h>

using namespace qs;


/****************************************************************************
 *
 * Global Variables
 *
 */

HINSTANCE g_hInstDll = 0;
Window* g_pMainWindow = 0;
ModalHandler* g_pModalHandler = 0;


/****************************************************************************
 *
 * DllMain
 *
 */

QSTATUS initProcess(HINSTANCE hInst);
QSTATUS termProcess();
QSTATUS initThread();
QSTATUS termThread();

BOOL WINAPI DllMain(/*HINSTANCE*/HANDLE hInst, DWORD dwReason, LPVOID lpReserved)
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
		if (initProcess(static_cast<HINSTANCE>(hInst)) != QSTATUS_SUCCESS)
			return FALSE;
		break;
	case DLL_PROCESS_DETACH:
		if (termProcess() != QSTATUS_SUCCESS)
			return FALSE;
		break;
	case DLL_THREAD_ATTACH:
		if (initThread() != QSTATUS_SUCCESS)
			return FALSE;
		break;
	case DLL_THREAD_DETACH:
		if (termThread() != QSTATUS_SUCCESS)
			return FALSE;
		break;
	}
	return TRUE;
}

QSTATUS initProcess(HINSTANCE hInst)
{
	DECLARE_QSTATUS();
	
	g_hInstDll = hInst;
	
	return QSTATUS_SUCCESS;
}

QSTATUS termProcess()
{
	g_hInstDll = 0;
	
	return QSTATUS_SUCCESS;
}

QSTATUS initThread()
{
	return QSTATUS_SUCCESS;
}

QSTATUS termThread()
{
	return QSTATUS_SUCCESS;
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
