/*
 * $Id: main.cpp,v 1.2 2003/05/30 06:22:56 snakamura Exp $
 *
 * Copyright(C) 1998-2003 Satoshi Nakamura
 * All rights reserved.
 *
 */

#include <qm.h>
#include <qmapplication.h>

#include <qsnew.h>
#include <qserror.h>

#include <memory>

#include <windows.h>

using namespace qm;
using namespace qs;


/****************************************************************************
 *
 * Global Variables
 *
 */

HINSTANCE g_hInstDll = 0;


/****************************************************************************
 *
 * DllMain
 *
 */

BOOL WINAPI DllMain(HANDLE hInst, DWORD dwReason, LPVOID lpReserved)
{
	switch (dwReason) {
	case DLL_PROCESS_ATTACH:
#ifndef NDEBUG
		{
			TCHAR tsz[32];
			wsprintf(tsz, TEXT("qm: %p\n"), hInst);
			::OutputDebugString(tsz);
		}
#endif
		g_hInstDll = static_cast<HINSTANCE>(hInst);
		break;
	case DLL_PROCESS_DETACH:
		break;
	case DLL_THREAD_ATTACH:
		break;
	case DLL_THREAD_DETACH:
		break;
	}
	return TRUE;
}


/****************************************************************************
 *
 * qmMain
 *
 */

#if 1//def NDEBUG
#	define QMMAINPROC
#else
#	define QMMAINPROC QMEXPORTPROC
#endif

extern "C" QMMAINPROC int qmMain(int nCmdShow)
{
	DECLARE_QSTATUS();
	
	std::auto_ptr<Application> pApplication;
	status = newQsObject(g_hInstDll, &pApplication);
	CHECK_QSTATUS_VALUE(1);
	
	status = pApplication->initialize();
	CHECK_QSTATUS_VALUE(1);
	
	status = pApplication->run();
	CHECK_QSTATUS_VALUE(1);
	
	status = pApplication->uninitialize();
	CHECK_QSTATUS_VALUE(1);
	
	return 0;
}
