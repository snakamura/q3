/*
 * $Id$
 *
 * Copyright(C) 1998-2005 Satoshi Nakamura
 * All rights reserved.
 *
 */

#include <qs.h>

#include "macro.h"
#include "main.h"
#include "obj.h"

using namespace qmscript;
using namespace qs;


/****************************************************************************
 *
 * Global Variables
 *
 */

HINSTANCE g_hInst = 0;
HINSTANCE g_hInstResource = 0;
ITypeLib* g_pTypeLib  = 0;


/****************************************************************************
 *
 * Global Functions
 *
 */

HINSTANCE qmscript::getInstanceHandle()
{
	return g_hInst;
}

HINSTANCE qmscript::getResourceHandle()
{
	return g_hInstResource;
}

ITypeLib* qmscript::getTypeLib()
{
	return g_pTypeLib;
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
			wsprintf(tsz, TEXT("qmscript: %p\n"), hInst);
			::OutputDebugString(tsz);
		}
#endif
		{
			g_hInst = static_cast<HINSTANCE>(hInst);
			g_hInstResource = g_hInst;
			
			TCHAR tszPath[MAX_PATH];
			::GetModuleFileName(g_hInst, tszPath, countof(tszPath));
#ifdef UNICODE
			const WCHAR* pwszPath = tszPath;
#else
			WCHAR wszPath[MAX_PATH];
			::MultiByteToWideChar(CP_ACP, 0, tszPath, -1, wszPath, countof(wszPath));
			const WCHAR* pwszPath = wszPath;
#endif
			HRESULT hr = ::LoadTypeLib(pwszPath, &g_pTypeLib);
			if (FAILED(hr))
				return FALSE;
		}
		break;
	case DLL_PROCESS_DETACH:
		if (g_pTypeLib)
			g_pTypeLib->Release();
		break;
	case DLL_THREAD_ATTACH:
		break;
	case DLL_THREAD_DETACH:
		break;
	}
	return TRUE;
}

STDAPI DllGetClassObject(REFCLSID rclsid, REFIID riid, void** ppv)
{
	BEGIN_COCLASS_MAP()
	END_COCLASS_MAP()
}

STDAPI DllCanUnloadNow()
{
	return S_FALSE;
}

STDAPI DllRegisterServer()
{
	return S_OK;
}

STDAPI DllUnregisterServer()
{
	return S_OK;
}
