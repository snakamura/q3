/*
 * $Id$
 *
 * Copyright(C) 1998-2003 Satoshi Nakamura
 * All rights reserved.
 *
 */

#include <qm.h>
#include <qmapplication.h>
#include <qmmain.h>

#include <qserror.h>
#include <qsnew.h>
#include <qsosutil.h>

#include <memory>

#include <windows.h>

#include "commandline.h"
#include "../ui/dialogs.h"

using namespace qm;
using namespace qs;


namespace qm{

QSTATUS main(const WCHAR* pwszCommandLine);

}


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

extern "C" QMEXPORTPROC int qmMain(const WCHAR* pwszCommandLine)
{
	DECLARE_QSTATUS();
	
	status = main(pwszCommandLine);
	CHECK_QSTATUS_VALUE(1);
	
	return 0;
}

QSTATUS qm::main(const WCHAR* pwszCommandLine)
{
	DECLARE_QSTATUS();
	
	MainCommandLineHandler handler;
	CommandLine commandLine(&handler, &status);
	CHECK_QSTATUS();
	status = commandLine.parse(pwszCommandLine);
	CHECK_QSTATUS();
	
	string_ptr<WSTRING> wstrMailFolder;
	const WCHAR* pwszMailFolder = handler.getMailFolder();
	if (pwszMailFolder) {
		wstrMailFolder.reset(allocWString(pwszMailFolder));
		if (!wstrMailFolder.get())
			return QSTATUS_OUTOFMEMORY;
	}
	
	string_ptr<WSTRING> wstrProfile;
	const WCHAR* pwszProfile = handler.getProfile();
	if (pwszProfile) {
		wstrProfile.reset(allocWString(pwszProfile));
		if (!wstrProfile.get())
			return QSTATUS_OUTOFMEMORY;
	}
	
	if (!wstrMailFolder.get() || !wstrProfile.get()) {
		Registry reg(HKEY_CURRENT_USER,
			L"Software\\sn\\q3\\Setting", &status);
		CHECK_QSTATUS();
		
		if (!wstrMailFolder.get()) {
			status = reg.getValue(L"MailFolder", &wstrMailFolder);
			CHECK_QSTATUS();
			if (!wstrMailFolder.get()) {
				// TODO
				// Use resource handle
				MailFolderDialog dialog(g_hInstDll, &status);
				CHECK_QSTATUS();
				int nRet = 0;
				status = dialog.doModal(0, 0, &nRet);
				CHECK_QSTATUS();
				if (nRet != IDOK)
					return QSTATUS_FAIL;
				wstrMailFolder.reset(allocWString(dialog.getMailFolder()));
				if (!wstrMailFolder.get())
					return QSTATUS_OUTOFMEMORY;
				
				status = reg.setValue(L"MailFolder", wstrMailFolder.get());
				CHECK_QSTATUS();
			}
			int nLen = wcslen(wstrMailFolder.get());
			if (*(wstrMailFolder.get() + nLen - 1) == L'\\')
				*(wstrMailFolder.get() + nLen - 1) = L'\0';
		}
		
		if (!wstrProfile.get()) {
			status = reg.getValue(L"Profile", &wstrProfile);
			CHECK_QSTATUS();
			if (!wstrProfile.get()) {
				wstrProfile.reset(allocWString(L""));
				if (!wstrProfile.get())
					return QSTATUS_OUTOFMEMORY;
			}
		}
	}
	
	std::auto_ptr<Application> pApplication;
	status = newQsObject(g_hInstDll, wstrMailFolder.get(),
		wstrProfile.get(), &pApplication);
	CHECK_QSTATUS();
	wstrMailFolder.release();
	wstrProfile.release();
	status = pApplication->initialize();
	CHECK_QSTATUS();
	status = pApplication->run();
	CHECK_QSTATUS();
	status = pApplication->uninitialize();
	CHECK_QSTATUS();
	
	return QSTATUS_SUCCESS;
}
