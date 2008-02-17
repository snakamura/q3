/*
 * $Id$
 *
 * Copyright(C) 1998-2008 Satoshi Nakamura
 *
 */

#include <qs.h>
#include <qsinit.h>

#include <vector>

#include <windows.h>

#include <kctrl.h>

#include "convja.h"
#include "main.h"

using namespace qs;
using namespace qsconvja;


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

HINSTANCE qsconvja::getInstanceHandle()
{
	return g_hInst;
}

HINSTANCE qsconvja::getResourceHandle()
{
	return g_hInstResource;
}


/****************************************************************************
 *
 * DllMain
 *
 */

namespace {

struct InitializerImpl : public qs::Initializer
{
public:
	virtual bool init()
	{
		assert(listConverterFactory_.empty());
		listConverterFactory_.push_back(new ShiftJISConverterFactory());
		listConverterFactory_.push_back(new ISO2022JPConverterFactory());
		listConverterFactory_.push_back(new EUCJPConverterFactory());
		return true;
	}
	
	virtual void term()
	{
		std::for_each(listConverterFactory_.begin(),
			listConverterFactory_.end(),
			boost::checked_deleter<ConverterFactory>());
		listConverterFactory_.clear();
	}

private:
	typedef std::vector<ConverterFactory*> ConverterFactoryList;

private:
	ConverterFactoryList listConverterFactory_;
} g_initializer;

}

BOOL WINAPI DllMain(HANDLE hInst,
					DWORD dwReason,
					LPVOID lpReserved)
{
	switch (dwReason) {
	case DLL_PROCESS_ATTACH:
#ifndef NDEBUG
		{
			TCHAR tsz[32];
			wsprintf(tsz, TEXT("qsconvja: %p\n"), hInst);
			::OutputDebugString(tsz);
		}
#endif
		g_hInst = static_cast<HINSTANCE>(hInst);
		g_hInstResource = g_hInst;
		
		InitKanjiControls();
		g_initializer.init();
		
		break;
	case DLL_PROCESS_DETACH:
		ReleaseKanjiControls();
		break;
	case DLL_THREAD_ATTACH:
		break;
	case DLL_THREAD_DETACH:
		break;
	}
	return TRUE;
}
