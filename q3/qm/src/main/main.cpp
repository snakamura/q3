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
#include <qsthread.h>

#include <memory>

#include <windows.h>

#include "main.h"
#include "../ui/dialogs.h"
#include "../ui/resourceinc.h"

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
 * Global functions
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
	if (pwszMailFolder && *pwszMailFolder) {
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
	
	bool bContinue = false;
	MailFolderLock lock(wstrMailFolder.get(), &bContinue, &status);
	CHECK_QSTATUS();
	if (!bContinue)
		return QSTATUS_SUCCESS;
	
	std::auto_ptr<Application> pApplication;
	status = newQsObject(g_hInstDll, wstrMailFolder.get(),
		wstrProfile.get(), &pApplication);
	CHECK_QSTATUS();
	wstrMailFolder.release();
	wstrProfile.release();
	
	status = pApplication->initialize();
	CHECK_QSTATUS();
	
	assert(getMainWindow());
	status = lock.setWindow(getMainWindow()->getHandle());
	CHECK_QSTATUS();
	
	status = pApplication->run();
	CHECK_QSTATUS();
	
	status = pApplication->uninitialize();
	CHECK_QSTATUS();
	
	return QSTATUS_SUCCESS;
}


/****************************************************************************
 *
 * MainCommandLineHandler
 *
 */

qm::MainCommandLineHandler::MainCommandLineHandler() :
	state_(STATE_NONE),
	wstrGoRound_(0),
	wstrMailFolder_(0),
	wstrProfile_(0),
	wstrURL_(0)
{
}

qm::MainCommandLineHandler::~MainCommandLineHandler()
{
	freeWString(wstrGoRound_);
	freeWString(wstrMailFolder_);
	freeWString(wstrProfile_);
	freeWString(wstrURL_);
}

const WCHAR* qm::MainCommandLineHandler::getGoRound() const
{
	return wstrGoRound_;
}

const WCHAR* qm::MainCommandLineHandler::getMailFolder() const
{
	return wstrMailFolder_;
}

const WCHAR* qm::MainCommandLineHandler::getProfile() const
{
	return wstrProfile_;
}

const WCHAR* qm::MainCommandLineHandler::getURL() const
{
	return wstrURL_;
}

QSTATUS qm::MainCommandLineHandler::process(const WCHAR* pwszOption)
{
	DECLARE_QSTATUS();
	
	struct {
		const WCHAR* pwsz_;
		State state_;
	} options[] = {
		{ L"g",	STATE_GOROUND		},
		{ L"d",	STATE_MAILFOLDER	},
		{ L"p",	STATE_PROFILE		},
		{ L"s",	STATE_URL			}
	};
	
	WSTRING* pwstr[] = {
		&wstrGoRound_,
		&wstrMailFolder_,
		&wstrProfile_,
		&wstrURL_
	};
	
	switch (state_) {
	case STATE_NONE:
		if (*pwszOption == L'-' || *pwszOption == L'/') {
			for (int n = 0; n < countof(options) && state_ == STATE_NONE; ++n) {
				if (wcscmp(pwszOption + 1, options[n].pwsz_) == 0)
					state_ = options[n].state_;
			}
		}
		break;
	case STATE_GOROUND:
	case STATE_MAILFOLDER:
	case STATE_PROFILE:
	case STATE_URL:
		{
			WSTRING& wstr = *pwstr[state_ - STATE_GOROUND];
			wstr = allocWString(pwszOption);
			if (!wstr)
				return QSTATUS_OUTOFMEMORY;
			state_ = STATE_NONE;
		}
		break;
	default:
		break;
	}
	
	return QSTATUS_SUCCESS;
}


/****************************************************************************
 *
 * MailFolderLock
 *
 */

qm::MailFolderLock::MailFolderLock(const WCHAR* pwszMailFolder,
	bool* pbContinue, QSTATUS* pstatus) :
	tstrPath_(0),
	hFile_(0),
	pMutex_(0)
{
	DECLARE_QSTATUS();
	
	status = lock(pwszMailFolder, pbContinue);
	CHECK_QSTATUS_SET(pstatus);
}

qm::MailFolderLock::~MailFolderLock()
{
	if (pMutex_) {
		pMutex_->release();
		delete pMutex_;
	}
	unlock();
	freeTString(tstrPath_);
}

QSTATUS qm::MailFolderLock::setWindow(HWND hwnd)
{
	assert(hwnd);
	assert(hFile_);
	assert(pMutex_);
	
	DECLARE_QSTATUS();
	
	string_ptr<WSTRING> wstrName;
#ifdef _WIN32_WCE
	Registry reg(HKEY_LOCAL_MACHINE, L"Ident", &status);
	CHECK_QSTATUS();
	status = reg.getValue(L"Name", &wstrName);
	CHECK_QSTATUS();
#else
	TCHAR tszComputerName[MAX_COMPUTERNAME_LENGTH + 1];
	DWORD dwSize = countof(tszComputerName);
	::GetComputerName(tszComputerName, &dwSize);
	wstrName.reset(tcs2wcs(tszComputerName));
	if (!wstrName.get())
		return QSTATUS_OUTOFMEMORY;
#endif
	
	WCHAR wszHandle[32];
	swprintf(wszHandle, L"%08x\n", reinterpret_cast<int>(hwnd));
	
	const WCHAR* pwsz[] = {
		wszHandle,
		wstrName.get()
	};
	for (int n = 0; n < countof(pwsz); ++n) {
		DWORD dw = 0;
		if (!::WriteFile(hFile_, pwsz[n], wcslen(pwsz[n])*sizeof(WCHAR), &dw, 0))
			return QSTATUS_FAIL;
	}
	if (!::FlushFileBuffers(hFile_))
		return QSTATUS_FAIL;
	
	status = pMutex_->release();
	CHECK_QSTATUS();
	delete pMutex_;
	pMutex_ = 0;
	
	return QSTATUS_SUCCESS;
}

QSTATUS qm::MailFolderLock::lock(const WCHAR* pwszMailFolder, bool* pbContinue)
{
	assert(pwszMailFolder);
	assert(pbContinue);
	
	DECLARE_QSTATUS();
	
	*pbContinue = false;
	
	string_ptr<WSTRING> wstrPath(concat(pwszMailFolder, L"\\lock"));
	if (!wstrPath.get())
		return QSTATUS_OUTOFMEMORY;
	string_ptr<TSTRING> tstrPath(wcs2tcs(wstrPath.get()));
	if (!tstrPath.get())
		return QSTATUS_OUTOFMEMORY;
	
	std::auto_ptr<Mutex> pMutex;
	status = newQsObject(false, L"QMAIL3Mutex", &pMutex);
	CHECK_QSTATUS();
	status = pMutex->acquire();
	CHECK_QSTATUS();
	
	AutoHandle hFile(::CreateFile(tstrPath.get(),
		GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ, 0,
		OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, 0));
	if (!hFile.get()) {
		AutoHandle hFileRead(::CreateFile(tstrPath.get(),
			GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE,
			0, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0));
		if (!hFileRead.get())
			return QSTATUS_FAIL;
		
		HWND hwnd = 0;
		status = read(hFileRead.get(), &hwnd, 0);
		CHECK_QSTATUS();
		::SetForegroundWindow(hwnd);
	}
	else if (::GetLastError() == ERROR_ALREADY_EXISTS) {
		string_ptr<WSTRING> wstrName;
		status = read(hFile.get(), 0, &wstrName);
		CHECK_QSTATUS();
		
		string_ptr<WSTRING> wstrTemplate;
		status = loadString(g_hInstDll, IDS_CONFIRM_IGNORELOCK, &wstrTemplate);
		CHECK_QSTATUS();
		string_ptr<WSTRING> wstrMessage(allocWString(
			wcslen(wstrTemplate.get()) + wcslen(wstrName.get())));
		swprintf(wstrMessage.get(), wstrTemplate.get(), wstrName.get());
		
		int nRet= 0;
		status = messageBox(wstrMessage.get(),
			MB_YESNO | MB_ICONQUESTION | MB_DEFBUTTON2, &nRet);
		CHECK_QSTATUS();
		*pbContinue = nRet == IDYES;
	}
	else {
		*pbContinue = true;
	}
	
	if (*pbContinue) {
		hFile_ = hFile.release();
		tstrPath_ = tstrPath.release();
		pMutex_ = pMutex.release();
	}
	else {
		pMutex->release();
	}
	
	return QSTATUS_SUCCESS;
}

QSTATUS qm::MailFolderLock::unlock()
{
	DECLARE_QSTATUS();
	
	if (hFile_) {
		assert(tstrPath_);
		
		Mutex mutex(false, L"QMAIL3Mutex", &status);
		CHECK_QSTATUS();
		status = mutex.acquire();
		CHECK_QSTATUS();
		
		::CloseHandle(hFile_);
		hFile_ = 0;
		
		::DeleteFile(tstrPath_);
		
		status = mutex.release();
		CHECK_QSTATUS();
	}
	
	return QSTATUS_SUCCESS;
}

QSTATUS qm::MailFolderLock::read(HANDLE hFile, HWND* phwnd, WSTRING* pwstrName)
{
	assert(hFile);
	
	DECLARE_QSTATUS();
	
	WCHAR wsz[1024];
	DWORD dw = 0;
	if (!::ReadFile(hFile, wsz, sizeof(wsz) - sizeof(WCHAR), &dw, 0))
		return QSTATUS_FAIL;
	wsz[dw/sizeof(WCHAR)] = L'\0';
	::SetFilePointer(hFile, 0, 0, FILE_BEGIN);
	
	WCHAR* p = wcschr(wsz, L'\n');
	if (!p)
		return QSTATUS_FAIL;
	*p = L'\0';
	
	if (phwnd) {
		int hwnd = 0;
		swscanf(wsz, L"%08x", &hwnd);
		*phwnd = reinterpret_cast<HWND>(hwnd);
	}
	
	if (pwstrName) {
		*pwstrName = allocWString(p + 1);
		if (!*pwstrName)
			return QSTATUS_OUTOFMEMORY;
	}
	
	return QSTATUS_SUCCESS;
}
