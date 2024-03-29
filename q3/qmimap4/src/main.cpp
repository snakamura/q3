/*
 * $Id$
 *
 * Copyright(C) 1998-2008 Satoshi Nakamura
 *
 */

#include <qs.h>

#include "main.h"

using namespace qs;


/****************************************************************************
 *
 * Global Variables
 *
 */

namespace {

HINSTANCE g_hInst = 0;
HINSTANCE g_hInstResource = 0;

}


/****************************************************************************
 *
 * Global Functions
 *
 */

HINSTANCE qmimap4::getInstanceHandle()
{
	return g_hInst;
}

HINSTANCE qmimap4::getResourceHandle()
{
	return g_hInstResource;
}


/****************************************************************************
 *
 * DllMain
 *
 */

BOOL WINAPI DllMain(HANDLE hInst,
					DWORD dwReason,
					LPVOID lpReserved)
{
	switch (dwReason) {
	case DLL_PROCESS_ATTACH:
#ifndef NDEBUG
		{
			TCHAR tsz[32];
			wsprintf(tsz, TEXT("qmimap4: %p\n"), hInst);
			::OutputDebugString(tsz);
		}
#endif
		g_hInst = static_cast<HINSTANCE>(hInst);
		g_hInstResource = loadResourceDll(g_hInst);
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
