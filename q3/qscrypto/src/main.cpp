/*
 * $Id: main.cpp,v 1.2 2003/05/30 06:22:57 snakamura Exp $
 *
 * Copyright(C) 1998-2003 Satoshi Nakamura
 * All rights reserved.
 *
 */

#include <qsnew.h>

#include <openssl/evp.h>
#ifdef _WIN32_WCE
#	include <openssl/rand.h>
#endif

#include "main.h"
#include "smime.h"


/****************************************************************************
 *
 * Global Variables
 *
 */

HINSTANCE g_hInst = 0;
HINSTANCE g_hInstResource = 0;


/****************************************************************************
 *
 * Global Functions
 *
 */

HINSTANCE qscrypto::getInstanceHandle()
{
	return g_hInst;
}

HINSTANCE qscrypto::getResourceHandle()
{
	return g_hInstResource;
}


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
			wsprintf(tsz, TEXT("qscrypto: %p\n"), hInst);
			::OutputDebugString(tsz);
		}
#endif
		g_hInst = static_cast<HINSTANCE>(hInst);
		g_hInstResource = g_hInst;
		OpenSSL_add_all_algorithms();
#ifdef _WIN32_WCE
		RAND_load_file("\\.qmail", -1);
#endif
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
