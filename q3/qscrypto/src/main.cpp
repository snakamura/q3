/*
 * $Id$
 *
 * Copyright(C) 1998-2008 Satoshi Nakamura
 *
 */

#include <qs.h>

#include <openssl/conf.h>
#include <openssl/engine.h>
#include <openssl/err.h>
#include <openssl/evp.h>
#include <openssl/ssl.h>
#ifdef _WIN32_WCE
#	include <openssl/rand.h>
#endif

#include "lock.h"
#include "main.h"
#include "smime.h"


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

BOOL WINAPI DllMain(HANDLE hInst,
					DWORD dwReason,
					LPVOID lpReserved)
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
		ERR_load_crypto_strings();
		ERR_load_SSL_strings();
		OpenSSL_add_all_algorithms();
		SSL_library_init();
		CRYPTO_set_locking_callback(&qscrypto::lockCallback);
#ifdef _WIN32_WCE
		// TODO
		// Seed correctly.
		{
			for (int n = 0; n < 1000; ++n) {
				RAND_seed("abcdefg", 7);
				RAND_seed(&n, sizeof(n));
			}
		}
#endif
		break;
	case DLL_PROCESS_DETACH:
		ERR_remove_state(0);
		CRYPTO_set_locking_callback(0);
		ENGINE_cleanup();
		CONF_modules_unload(1);
		EVP_cleanup();
		CRYPTO_cleanup_all_ex_data();
		ERR_free_strings();
		break;
	case DLL_THREAD_ATTACH:
		break;
	case DLL_THREAD_DETACH:
		ERR_remove_state(0);
		break;
	}
	return TRUE;
}
