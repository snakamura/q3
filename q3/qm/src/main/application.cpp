/*
 * $Id$
 *
 * Copyright(C) 1998-2003 Satoshi Nakamura
 * All rights reserved.
 *
 */

#include <qm.h>
#include <qmaccount.h>
#include <qmapplication.h>
#include <qmdocument.h>
#include <qmextensions.h>
#include <qmgoround.h>
#include <qmmainwindow.h>

#include <qsconv.h>
#include <qsdialog.h>
#include <qserror.h>
#include <qskeymap.h>
#include <qsmime.h>
#include <qsnew.h>
#include <qsosutil.h>
#include <qsprofile.h>
#include <qssocket.h>
#include <qsstream.h>
#include <qswindow.h>

#include <algorithm>

#include <windows.h>
#include <tchar.h>

#ifndef DEPENDCHECK
#	include "version.h"
#endif
#include "../model/dataobject.h"
#include "../model/security.h"
#include "../model/tempfilecleaner.h"
#include "../sync/syncmanager.h"
#include "../ui/dialogs.h"
#include "../ui/foldermodel.h"
#include "../ui/mainwindow.h"
#include "../ui/menu.h"
#include "../ui/newmailchecker.h"
#include "../ui/syncdialog.h"

using namespace qm;
using namespace qs;


/****************************************************************************
 *
 * ApplicationImpl
 *
 */

struct qm::ApplicationImpl : public NewMailCheckerCallback
{
public:
	QSTATUS ensureDirectory(const WCHAR* pwszPath, const WCHAR* pwszName);
	QSTATUS ensureFile(const WCHAR* pwszPath, const WCHAR* pwszDir,
		const WCHAR* pwszType, const WCHAR* pwszName, const WCHAR* pwszExtension);
	QSTATUS restoreCurrentFolder();
	QSTATUS saveCurrentFolder();

public:
	virtual bool canCheck();

public:
	HINSTANCE hInst_;
	HINSTANCE hInstResource_;
	Winsock* pWinSock_;
	WSTRING wstrMailFolder_;
	WSTRING wstrTemporaryFolder_;
	WSTRING wstrProfileName_;
	XMLProfile* pProfile_;
	Document* pDocument_;
	KeyMap* pKeyMap_;
	SyncManager* pSyncManager_;
	SyncDialogManager* pSyncDialogManager_;
	GoRound* pGoRound_;
	TempFileCleaner* pTempFileCleaner_;
	MainWindow* pMainWindow_;
	MenuManager* pMenuManager_;
	NewMailChecker* pNewMailChecker_;
	HINSTANCE hInstAtl_;
	
	static Application* pApplication__;
};

Application* qm::ApplicationImpl::pApplication__ = 0;

QSTATUS qm::ApplicationImpl::ensureDirectory(
	const WCHAR* pwszPath, const WCHAR* pwszName)
{
	assert(pwszPath);
	
	string_ptr<WSTRING> wstrPath;
	if (pwszName) {
		wstrPath.reset(concat(pwszPath, L"\\", pwszName));
		if (!wstrPath.get())
			return QSTATUS_OUTOFMEMORY;
		pwszPath = wstrPath.get();
	}
	
	W2T(pwszPath, ptszPath);
	DWORD dwAttributes = ::GetFileAttributes(ptszPath);
	if (dwAttributes == 0xffffffff) {
		if (!::CreateDirectory(ptszPath, 0))
			return QSTATUS_FAIL;
	}
	else if (!(dwAttributes & FILE_ATTRIBUTE_DIRECTORY)) {
		return QSTATUS_FAIL;
	}
	
	return QSTATUS_SUCCESS;
}

QSTATUS ApplicationImpl::ensureFile(const WCHAR* pwszPath, const WCHAR* pwszDir,
	const WCHAR* pwszType, const WCHAR* pwszName, const WCHAR* pwszExtension)
{
	assert(pwszPath);
	assert(pwszType);
	assert(pwszName);
	
	DECLARE_QSTATUS();
	
	StringBuffer<WSTRING> buf(&status);
	CHECK_QSTATUS();
	status = buf.append(pwszPath);
	CHECK_QSTATUS();
	if (pwszDir) {
		status = buf.append(L'\\');
		CHECK_QSTATUS();
		status = buf.append(pwszDir);
		CHECK_QSTATUS();
	}
	status = buf.append(L'\\');
	CHECK_QSTATUS();
	status = buf.append(pwszName);
	CHECK_QSTATUS();
	if (pwszExtension) {
		status = buf.append(L'.');
		CHECK_QSTATUS();
		status = buf.append(pwszExtension);
		CHECK_QSTATUS();
	}
	
	W2T(buf.getCharArray(), ptszPath);
	if (::GetFileAttributes(ptszPath) == 0xffffffff) {
		W2T(pwszName, ptszName);
		W2T(pwszType, ptszType);
		HRSRC hrsrc = ::FindResource(hInstResource_, ptszName, ptszType);
		if (!hrsrc)
			return QSTATUS_FAIL;
		HGLOBAL hResource = ::LoadResource(hInstResource_, hrsrc);
		if (!hResource)
			return QSTATUS_FAIL;
		void* pResource = ::LockResource(hResource);
		if (!pResource)
			return QSTATUS_FAIL;
		int nLen = ::SizeofResource(hInstResource_, hrsrc);
		
		FileOutputStream stream(buf.getCharArray(), &status);
		CHECK_QSTATUS();
		status = stream.write(static_cast<unsigned char*>(pResource), nLen);
		CHECK_QSTATUS();
		status = stream.close();
		CHECK_QSTATUS();
	}
	
	return QSTATUS_SUCCESS;
}

QSTATUS qm::ApplicationImpl::restoreCurrentFolder()
{
	DECLARE_QSTATUS();
	
	string_ptr<WSTRING> wstrFolder;
	status = pProfile_->getString(L"Global",
		L"CurrentFolder", L"", &wstrFolder);
	CHECK_QSTATUS();
	
	if (wcsncmp(wstrFolder.get(), L"//", 2) == 0) {
		WCHAR* pwszFolder = wcschr(wstrFolder.get() + 2, L'/');
		if (pwszFolder) {
			*pwszFolder = L'\0';
			++pwszFolder;
		}
		
		Account* pAccount = pDocument_->getAccount(wstrFolder.get() + 2);
		if (pAccount) {
			FolderModel* pFolderModel = pMainWindow_->getFolderModel();
			if (pwszFolder) {
				Folder* pFolder = 0;
				status = pAccount->getFolder(pwszFolder, &pFolder);
				CHECK_QSTATUS();
				if (pFolder) {
					status = pFolderModel->setCurrentFolder(pFolder);
					CHECK_QSTATUS();
				}
			}
			else {
				status = pFolderModel->setCurrentAccount(pAccount);
				CHECK_QSTATUS();
			}
		}
	}
	
	return QSTATUS_SUCCESS;
}

QSTATUS qm::ApplicationImpl::saveCurrentFolder()
{
	DECLARE_QSTATUS();
	
	StringBuffer<WSTRING> buf(&status);
	CHECK_QSTATUS();
	
	FolderModel* pFolderModel = pMainWindow_->getFolderModel();
	Account* pAccount = pFolderModel->getCurrentAccount();
	Folder* pFolder = pFolderModel->getCurrentFolder();
	if (!pAccount && pFolder)
		pAccount = pFolder->getAccount();
	if (pAccount) {
		status = buf.append(L"//");
		CHECK_QSTATUS();
		status = buf.append(pAccount->getName());
		CHECK_QSTATUS();
		if (pFolder) {
			status = buf.append(L'/');
			CHECK_QSTATUS();
			string_ptr<WSTRING> wstrFolder;
			status = pFolder->getFullName(&wstrFolder);
			CHECK_QSTATUS();
			status = buf.append(wstrFolder.get());
			CHECK_QSTATUS();
		}
	}
	
	status = pProfile_->setString(L"Global",
		L"CurrentFolder", buf.getCharArray());
	CHECK_QSTATUS();
	
	return QSTATUS_SUCCESS;
}

bool qm::ApplicationImpl::canCheck()
{
	return !pMainWindow_->isShowingModalDialog();
}


/****************************************************************************
 *
 * Application
 *
 */

qm::Application::Application(HINSTANCE hInst, QSTATUS* pstatus)
{
	assert(pstatus);
	
	*pstatus = QSTATUS_SUCCESS;
	
	DECLARE_QSTATUS();
	
	status = newObject(&pImpl_);
	CHECK_QSTATUS_SET(pstatus);
	pImpl_->hInst_ = hInst;
	pImpl_->hInstResource_ = hInst;
	pImpl_->pWinSock_ = 0;
	pImpl_->wstrMailFolder_ = 0;
	pImpl_->wstrTemporaryFolder_ = 0;
	pImpl_->wstrProfileName_ = 0;
	pImpl_->pProfile_ = 0;
	pImpl_->pDocument_ = 0;
	pImpl_->pKeyMap_ = 0;
	pImpl_->pSyncManager_ = 0;
	pImpl_->pSyncDialogManager_ = 0;
	pImpl_->pGoRound_ = 0;
	pImpl_->pTempFileCleaner_ = 0;
	pImpl_->pMainWindow_ = 0;
	pImpl_->pMenuManager_ = 0;
	pImpl_->pNewMailChecker_ = 0;
	pImpl_->hInstAtl_ = 0;
	
	assert(!ApplicationImpl::pApplication__);
	ApplicationImpl::pApplication__ = this;
}

qm::Application::~Application()
{
	assert(ApplicationImpl::pApplication__ == this);
	ApplicationImpl::pApplication__ = 0;
	
	if (pImpl_) {
		delete pImpl_;
		pImpl_ = 0;
	}
}

QSTATUS qm::Application::initialize()
{
	DECLARE_QSTATUS();
	
	status = newQsObject(&pImpl_->pWinSock_);
	CHECK_QSTATUS();
	
	Part::setDefaultCharset(L"iso-2022-jp");
	Part::setGlobalOptions(Part::O_USE_COMMENT_AS_PHRASE |
		Part::O_ALLOW_ENCODED_QSTRING |
		Part::O_ALLOW_ENCODED_PARAMETER |
		Part::O_ALLOW_ROUTEADDR_WITHOUT_PHRASE |
		Part::O_ALLOW_PARAMETER_INVALID_SEMICOLON |
		Part::O_ALLOW_ADDRESS_WITHOUT_DOMAIN |
		Part::O_ALLOW_INCOMPLETE_MULTIPART |
		Part::O_ALLOW_RAW_FIELD |
		Part::O_ALLOW_SPECIALS_IN_REFERENCES);
	
	bool bShowDialog = false;
	Registry reg(HKEY_CURRENT_USER,
		L"Software\\sn\\q3\\Setting", &status);
	CHECK_QSTATUS();
	string_ptr<WSTRING> wstrMailFolder;
	status = reg.getValue(L"MailFolder", &wstrMailFolder);
	CHECK_QSTATUS();
	if (!wstrMailFolder.get()) {
		MailFolderDialog dialog(&status);
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
		
		bShowDialog = true;
	}
	int nLen = wcslen(wstrMailFolder.get());
	if (*(wstrMailFolder.get() + nLen - 1) == L'\\')
		*(wstrMailFolder.get() + nLen - 1) = L'\0';
	
	string_ptr<WSTRING> wstrProfileName;
	status = reg.getValue(L"Profile", &wstrProfileName);
	CHECK_QSTATUS();
	if (!wstrProfileName.get()) {
		wstrProfileName.reset(allocWString(L""));
		if (!wstrProfileName.get())
			return QSTATUS_OUTOFMEMORY;
	}
	
	status = pImpl_->ensureDirectory(wstrMailFolder.get(), 0);
	CHECK_QSTATUS();
	const WCHAR* pwszDirs[] = {
		L"accounts",
		L"templates",
		L"scripts",
		L"security",
		L"profiles"
	};
	for (int n = 0; n < countof(pwszDirs); ++n) {
		status = pImpl_->ensureDirectory(
			wstrMailFolder.get(), pwszDirs[n]);
		CHECK_QSTATUS();
	}
	
	const WCHAR* pwszTemplates[] = {
		L"new",
		L"reply",
		L"reply_all",
		L"forward",
		L"edit"
	};
	for (n = 0; n < countof(pwszTemplates); ++n) {
		status = pImpl_->ensureFile(wstrMailFolder.get(),
			L"templates", L"TEMPLATE", pwszTemplates[n], L"template");
		CHECK_QSTATUS();
	}
	
	string_ptr<WSTRING> wstrProfileDir(
		concat(wstrMailFolder.get(), L"\\profiles"));
	if (!wstrProfileDir.get())
		return QSTATUS_OUTOFMEMORY;
	
	const WCHAR* pwszProfiles[] = {
		L".qmail",
		L".header",
		L".headeredit",
		L".keymap",
		L".menus"
	};
	for (n = 0; n < countof(pwszProfiles); ++n) {
		status = pImpl_->ensureFile(wstrProfileDir.get(),
			0, L"PROFILE", pwszProfiles[n], 0);
		CHECK_QSTATUS();
	}
	
	pImpl_->wstrMailFolder_ = wstrMailFolder.release();
	pImpl_->wstrProfileName_ = wstrProfileName.release();
	
	string_ptr<WSTRING> wstrProfilePath;
	status = getProfilePath(Extensions::QMAIL, &wstrProfilePath);
	CHECK_QSTATUS();
	status = newQsObject(wstrProfilePath.get(), &pImpl_->pProfile_);
	CHECK_QSTATUS();
	status = pImpl_->pProfile_->load();
	CHECK_QSTATUS();
	
	string_ptr<WSTRING> wstrTempFolder;
	status = pImpl_->pProfile_->getString(L"Global",
		L"TemporaryFolder", L"", &wstrTempFolder);
	CHECK_QSTATUS();
	if (!*wstrTempFolder.get()) {
		TCHAR tszPath[MAX_PATH];
		::GetTempPath(countof(tszPath), tszPath);
		wstrTempFolder.reset(tcs2wcs(tszPath));
		if (!wstrTempFolder.get())
			return QSTATUS_OUTOFMEMORY;
	}
	pImpl_->wstrTemporaryFolder_ = wstrTempFolder.release();
	
	string_ptr<WSTRING> wstrMenuPath;
	status = getProfilePath(Extensions::MENUS, &wstrMenuPath);
	CHECK_QSTATUS();
	PopupMenuManager popupMenuManager(&status);
	CHECK_QSTATUS();
	LoadMenuPopupMenu* pPopupMenus[countof(popupMenuItems)];
	for (n = 0; n < countof(popupMenuItems); ++n) {
		status = newQsObject(getResourceHandle(),
			popupMenuItems[n].nId_, &pPopupMenus[n]);
		CHECK_QSTATUS();
		status = popupMenuManager.addPopupMenu(
			popupMenuItems[n].pwszName_, pPopupMenus[n]);
		CHECK_QSTATUS();
	}
	status = newQsObject(wstrMenuPath.get(), menuItems,
		countof(menuItems), popupMenuManager, &pImpl_->pMenuManager_);
	CHECK_QSTATUS();
	std::for_each(pPopupMenus, pPopupMenus + countof(pPopupMenus),
		deleter<LoadMenuPopupMenu>());
	
	string_ptr<WSTRING> wstrKeyMapPath;
	status = getProfilePath(Extensions::KEYMAP, &wstrKeyMapPath);
	CHECK_QSTATUS();
	status = newQsObject(wstrKeyMapPath.get(), &pImpl_->pKeyMap_);
	CHECK_QSTATUS();
	
	string_ptr<WSTRING> wstrLibraries;
	status = pImpl_->pProfile_->getString(L"Global", L"Libraries",
		L"smtp,pop3,imap4,nntp", &wstrLibraries);
	CHECK_QSTATUS();
	WCHAR* p = wcstok(wstrLibraries.get(), L" ,");
	while (p) {
#ifdef NDEBUG
#	ifdef UNICODE
#		define SUFFIX L"u"
#	else
#		define SUFFIX L""
#	endif
#else
#	ifdef UNICODE
#		define SUFFIX L"ud"
#	else
#		define SUFFIX L"d"
#	endif
#endif
		string_ptr<WSTRING> wstrLib(concat(L"qm", p, SUFFIX L".dll"));
		if (!wstrLib.get())
			return QSTATUS_OUTOFMEMORY;
		W2T(wstrLib.get(), ptszLib);
		HINSTANCE hInst = ::LoadLibrary(ptszLib);
		p = wcstok(0, L" ,");
	}
	
	Security::init();
	
	status = newQsObject(pImpl_->pProfile_, &pImpl_->pDocument_);
	CHECK_QSTATUS();
	
	status = newQsObject(pImpl_->pProfile_, &pImpl_->pSyncManager_);
	CHECK_QSTATUS();
	
	status = newQsObject(pImpl_->pProfile_, &pImpl_->pSyncDialogManager_);
	CHECK_QSTATUS();
	
	status = newQsObject(&pImpl_->pGoRound_);
	CHECK_QSTATUS();
	
	status = newQsObject(&pImpl_->pTempFileCleaner_);
	CHECK_QSTATUS();
	
	std::auto_ptr<MainWindow> pMainWindow;
	status = newQsObject(pImpl_->pProfile_, &pMainWindow);
	CHECK_QSTATUS();
#ifdef _WIN32_WCE
	DWORD dwStyle = WS_VISIBLE | WS_CLIPCHILDREN;
	DWORD dwExStyle = 0;
#else
	DWORD dwStyle = WS_VISIBLE | WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN;
	DWORD dwExStyle = WS_EX_WINDOWEDGE;
#endif
	MainWindowCreateContext context = {
		pImpl_->pDocument_,
		pImpl_->pSyncManager_,
		pImpl_->pSyncDialogManager_,
		pImpl_->pGoRound_,
		pImpl_->pTempFileCleaner_,
		pImpl_->pMenuManager_,
		pImpl_->pKeyMap_
	};
	status = pMainWindow->create(L"QmMainWindow", L"QMAIL", dwStyle,
		CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
		0, dwExStyle, 0, 0, &context);
	CHECK_QSTATUS();
	pImpl_->pMainWindow_ = pMainWindow.release();
	setMainWindow(pImpl_->pMainWindow_);
	
	string_ptr<WSTRING> wstrAccountFolder(
		concat(pImpl_->wstrMailFolder_, L"\\accounts"));
	if (!wstrAccountFolder.get())
		return QSTATUS_OUTOFMEMORY;
	status = pImpl_->pDocument_->loadAccounts(wstrAccountFolder.get());
	CHECK_QSTATUS();
	
	pImpl_->pMainWindow_->updateWindow();
	if (bShowDialog)
		pImpl_->pMainWindow_->setForegroundWindow();
	
	int nOffline = 0;
	status = pImpl_->pProfile_->getInt(L"Global", L"Offline", 1, &nOffline);
	CHECK_QSTATUS();
	if (!nOffline) {
		status = pImpl_->pDocument_->setOffline(false);
		CHECK_QSTATUS();
	}
	
	status = pImpl_->restoreCurrentFolder();
	CHECK_QSTATUS();
	
	status = newQsObject(pImpl_->pProfile_, pImpl_->pDocument_,
		pImpl_->pGoRound_, pImpl_->pSyncManager_, pImpl_->pSyncDialogManager_,
		pImpl_->pMainWindow_->getHandle(), pImpl_, &pImpl_->pNewMailChecker_);
	CHECK_QSTATUS();
	
	return QSTATUS_SUCCESS;
}

QSTATUS qm::Application::uninitialize()
{
	assert(pImpl_);
	
	DECLARE_QSTATUS();
	
	delete pImpl_->pNewMailChecker_;
	pImpl_->pNewMailChecker_ = 0;
	
#ifndef _WIN32_WCE
	ComPtr<IDataObject> pDataObject;
	HRESULT hr = ::OleGetClipboard(&pDataObject);
	if (hr == S_OK) {
		hr = pDataObject->QueryGetData(
			&MessageDataObject::formats__[MessageDataObject::FORMAT_ACCOUNT]);
		if (hr == S_OK)
			::OleFlushClipboard();
	}
#endif
	
	delete pImpl_->pMenuManager_;
	pImpl_->pMenuManager_ = 0;
	
	delete pImpl_->pTempFileCleaner_;
	pImpl_->pTempFileCleaner_ = 0;
	
	delete pImpl_->pGoRound_;
	pImpl_->pGoRound_ = 0;
	
	delete pImpl_->pSyncDialogManager_;
	pImpl_->pSyncDialogManager_ = 0;
	
	delete pImpl_->pSyncManager_;
	pImpl_->pSyncManager_ = 0;
	
	delete pImpl_->pDocument_;
	pImpl_->pDocument_ = 0;
	
	delete pImpl_->pKeyMap_;
	pImpl_->pKeyMap_ = 0;
	
	delete pImpl_->pProfile_;
	pImpl_->pProfile_ = 0;
	
	Part::setDefaultCharset(0);
	
	Security::term();
	
	if (pImpl_->hInstAtl_) {
		::FreeLibrary(pImpl_->hInstAtl_);
		pImpl_->hInstAtl_ = 0;
	}
	
	freeWString(pImpl_->wstrMailFolder_);
	freeWString(pImpl_->wstrTemporaryFolder_);
	freeWString(pImpl_->wstrProfileName_);
	
	delete pImpl_->pWinSock_;
	pImpl_->pWinSock_ = 0;
	
	return QSTATUS_SUCCESS;
}

QSTATUS qm::Application::run()
{
	DECLARE_QSTATUS();
	
	DWORD dwThreadId = ::GetCurrentThreadId();
	MSG msg;
	while (::GetMessage(&msg, 0, 0, 0)) {
		switch (msg.message) {
		case WM_KEYDOWN:
		case WM_SYSKEYDOWN:
		case WM_LBUTTONDOWN:
		case WM_RBUTTONDOWN:
		case WM_MBUTTONDOWN:
		case WM_VSCROLL:
		case WM_HSCROLL:
		case WM_COMMAND:
			break;
#if !defined _WIN32_WCE || _WIN32_WCE >= 211
		case WM_MOUSEWHEEL:
			msg.hwnd = ::WindowFromPoint(msg.pt);
			assert(msg.hwnd);
			if (::GetWindowThreadProcessId(msg.hwnd, 0) != dwThreadId)
				continue;
			break;
#endif
		}
		
		bool bProcessed = false;
		status = DialogBase::processDialogMessage(&msg, &bProcessed);
		CHECK_QSTATUS();
		if (bProcessed)
			continue;
		status = PropertySheetBase::processDialogMessage(&msg, &bProcessed);
		CHECK_QSTATUS();
		if (bProcessed)
			continue;
		status = WindowBase::translateAccelerator(msg, &bProcessed);
		CHECK_QSTATUS();
		if (bProcessed)
			continue;
		
		::TranslateMessage(&msg);
		::DispatchMessage(&msg);
	}
	
	return QSTATUS_SUCCESS;
}

QSTATUS qm::Application::save()
{
	DECLARE_QSTATUS();
	
	status = pImpl_->pDocument_->save();
	CHECK_QSTATUS();
	status = pImpl_->pMainWindow_->save();
	CHECK_QSTATUS();
	status = pImpl_->pSyncDialogManager_->save();
	CHECK_QSTATUS();
	status = pImpl_->saveCurrentFolder();
	CHECK_QSTATUS();
	status = pImpl_->pProfile_->save();
	CHECK_QSTATUS();
	
	return QSTATUS_SUCCESS;
}

HINSTANCE qm::Application::getResourceHandle() const
{
	return pImpl_->hInstResource_;
}

HINSTANCE qm::Application::getAtlHandle() const
{
	if (!pImpl_->hInstAtl_) {
		pImpl_->hInstAtl_ = ::LoadLibrary(_T("atl.dll"));
		if (pImpl_->hInstAtl_) {
			typedef BOOL (__stdcall *PFN_ATLAXWININIT)();
			PFN_ATLAXWININIT pfnAtlAxWinInit = reinterpret_cast<PFN_ATLAXWININIT>(
				::GetProcAddress(pImpl_->hInstAtl_, WCE_T("AtlAxWinInit")));
			if (pfnAtlAxWinInit)
				(*pfnAtlAxWinInit)();
		}
	}
	return pImpl_->hInstAtl_;
}

const WCHAR* qm::Application::getMailFolder() const
{
	return pImpl_->wstrMailFolder_;
}

const WCHAR* qm::Application::getTemporaryFolder() const
{
	return pImpl_->wstrTemporaryFolder_;
}

const WCHAR* qm::Application::getProfileName() const
{
	return pImpl_->wstrProfileName_;
}

QSTATUS qm::Application::getProfilePath(
	const WCHAR* pwszName, WSTRING* pwstrPath) const
{
	assert(pwszName);
	assert(pwstrPath);
	
	DECLARE_QSTATUS();
	
	string_ptr<WSTRING> wstrPath;
	if (*pImpl_->wstrProfileName_) {
		ConcatW c[] = {
			{ pImpl_->wstrMailFolder_,	-1	},
			{ L"\\profiles\\",			-1	},
			{ pImpl_->wstrProfileName_,	-1	},
			{ L"\\",					1	},
			{ pwszName,					-1	}
		};
		wstrPath.reset(concat(c, countof(c)));
		if (!wstrPath.get())
			return QSTATUS_OUTOFMEMORY;
		
		W2T(wstrPath.get(), ptszPath);
		if (::GetFileAttributes(ptszPath) == 0xffffffff)
			wstrPath.reset(0);
	}
	
	if (!wstrPath.get()) {
		wstrPath.reset(concat(pImpl_->wstrMailFolder_,
			L"\\profiles\\", pwszName));
		if (!wstrPath.get())
			return QSTATUS_OUTOFMEMORY;
	}
	
	*pwstrPath = wstrPath.release();
	
	return QSTATUS_SUCCESS;
}

QSTATUS qm::Application::getVersion(
	bool bWithOSVersion, WSTRING* pwstrVersion) const
{
	assert(pwstrVersion);
	
	DECLARE_QSTATUS();
	
	string_ptr<WSTRING> wstrVersion(allocWString(256));
	if (!wstrVersion.get())
		return QSTATUS_OUTOFMEMORY;
	
	if (bWithOSVersion) {
		string_ptr<WSTRING> wstrOSVersion;
		status = getOSVersion(&wstrOSVersion);
		CHECK_QSTATUS();
#if defined SH3
		const WCHAR* pwszCPU = L"SH3";
#elif defined SH4
		const WCHAR* pwszCPU = L"SH4";
#elif defined MIPS
		const WCHAR* pwszCPU = L"MIPS";
#elif defined ARM
		const WCHAR* pwszCPU = L"ARM";
#else
		const WCHAR* pwszCPU = L"x86";
#endif
		swprintf(wstrVersion.get(), L"QMAIL %d.%d.%d / %s / %s",
			QMAIL_VERSION/100000, (QMAIL_VERSION%100000)/1000,
			QMAIL_VERSION%1000, wstrOSVersion.get(), pwszCPU);
	}
	else {
		swprintf(wstrVersion.get(), L"QMAIL %d.%d.%d", QMAIL_VERSION/100000,
			(QMAIL_VERSION%100000)/1000, QMAIL_VERSION%1000);
	}
	
	*pwstrVersion = wstrVersion.release();
	
	return QSTATUS_SUCCESS;
}

QSTATUS qm::Application::getOSVersion(WSTRING* pwstrOSVersion) const
{
	assert(pwstrOSVersion);
	
	DECLARE_QSTATUS();
	
	bool bAddVersion = true;
	const WCHAR* pwszPlatform = 0;
	OSVERSIONINFO ovi = { sizeof(ovi) };
	::GetVersionEx(&ovi);
	switch (ovi.dwPlatformId) {
	case VER_PLATFORM_WIN32_WINDOWS:
		if (ovi.dwMinorVersion >= 90)
			pwszPlatform = L"Windows Me";
		else if (ovi.dwMinorVersion > 0)
			pwszPlatform = L"Windows 98";
		else
			pwszPlatform = L"Windows 95";
		bAddVersion = false;
		break;
	case VER_PLATFORM_WIN32_NT:
		if (ovi.dwMajorVersion == 5) {
			if (ovi.dwMinorVersion == 0) {
				pwszPlatform = L"Windows 2000";
				bAddVersion = false;
			}
			else if (ovi.dwMinorVersion == 1) {
				pwszPlatform = L"Windows XP";
				bAddVersion = false;
			}
			else {
				pwszPlatform = L"Windows NT";
			}
		}
		else {
			pwszPlatform = L"Windows NT";
		}
		break;
#ifdef _WIN32_WCE
	case VER_PLATFORM_WIN32_CE:
		pwszPlatform = L"Windows CE";
		break;
#endif
	}
	
	string_ptr<WSTRING> wstrOSVersion(allocWString(256));
	if (!wstrOSVersion.get())
		return QSTATUS_OUTOFMEMORY;
	T2W(ovi.szCSDVersion, pwszAdditional);
	if (bAddVersion)
		swprintf(wstrOSVersion.get(), L"%s %u.%02u%s%s",
			pwszPlatform, ovi.dwMajorVersion, ovi.dwMinorVersion,
			*pwszAdditional ? L" " : L"", pwszAdditional);
	else
		swprintf(wstrOSVersion.get(), L"%s%s%s",
			pwszPlatform, *pwszAdditional ? L" " : L"", pwszAdditional);
	
	*pwstrOSVersion = wstrOSVersion.release();
	
	return QSTATUS_SUCCESS;
}

Application& qm::Application::getApplication()
{
	return *ApplicationImpl::pApplication__;
}
