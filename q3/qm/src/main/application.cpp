/*
 * $Id$
 *
 * Copyright(C) 1998-2004 Satoshi Nakamura
 * All rights reserved.
 *
 */

#include <qm.h>
#include <qmaccount.h>
#include <qmapplication.h>
#include <qmdocument.h>
#include <qmfilenames.h>
#include <qmgoround.h>
#include <qmmainwindow.h>
#include <qmsecurity.h>

#include <qsconv.h>
#include <qsdialog.h>
#include <qsfile.h>
#include <qsinit.h>
#include <qsmime.h>
#include <qsosutil.h>
#include <qsprofile.h>
#include <qssocket.h>
#include <qsstream.h>
#include <qswindow.h>

#include <algorithm>

#include <windows.h>
#include <tchar.h>

#include "main.h"
#ifndef DEPENDCHECK
#	include "version.h"
#endif
#include "../model/dataobject.h"
#include "../model/tempfilecleaner.h"
#include "../sync/syncmanager.h"
#include "../ui/dialogs.h"
#include "../ui/foldermodel.h"
#include "../ui/mainwindow.h"
#include "../ui/newmailchecker.h"
#include "../ui/syncdialog.h"
#include "../ui/uimanager.h"

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
	enum ResourceState {
		RS_EXIST,
		RS_EXISTPROFILE,
		RS_NOTEXIST,
		RS_SAMEREVISION,
		RS_OVERWRITE,
		RS_OVERWRITEPROFILE
	};
	
	struct Resource
	{
		const WCHAR* pwszDir_;
		const WCHAR* pwszSubDir_;
		const WCHAR* pwszProfile_;
		const WCHAR* pwszFileName_;
		const WCHAR* pwszResourceType_;
		const WCHAR* pwszResourceName_;
		unsigned int nRevision_;
		ResourceState state_;
	};

public:
	bool ensureDirectory(const WCHAR* pwszPath,
						 const WCHAR* pwszName);
	bool ensureFile(const WCHAR* pwszPath,
					const WCHAR* pwszDir,
					const WCHAR* pwszFileName,
					const WCHAR* pwszExtension,
					const WCHAR* pwszType,
					const WCHAR* pwszName);
	bool ensureResources(Resource* pResource,
						 size_t nCount);
	bool detachResource(const WCHAR* pwszPath,
						const WCHAR* pwszType,
						const WCHAR* pwszName);
	void restoreCurrentFolder();
	void saveCurrentFolder();

public:
	virtual bool canCheck();

public:
	static void loadLibrary(const WCHAR* pwszName);

public:
	HINSTANCE hInst_;
	HINSTANCE hInstResource_;
	std::auto_ptr<MailFolderLock> pLock_;
	std::auto_ptr<Winsock> pWinSock_;
	wstring_ptr wstrMailFolder_;
	wstring_ptr wstrTemporaryFolder_;
	wstring_ptr wstrProfileName_;
	std::auto_ptr<XMLProfile> pProfile_;
	std::auto_ptr<Document> pDocument_;
	std::auto_ptr<SyncManager> pSyncManager_;
	std::auto_ptr<SyncDialogManager> pSyncDialogManager_;
	std::auto_ptr<GoRound> pGoRound_;
	std::auto_ptr<TempFileCleaner> pTempFileCleaner_;
	std::auto_ptr<UIManager> pUIManager_;
	MainWindow* pMainWindow_;
	std::auto_ptr<NewMailChecker> pNewMailChecker_;
	HINSTANCE hInstAtl_;
	
	static Application* pApplication__;
};

Application* qm::ApplicationImpl::pApplication__ = 0;

bool qm::ApplicationImpl::ensureDirectory(const WCHAR* pwszPath,
										  const WCHAR* pwszName)
{
	assert(pwszPath);
	assert(!pwszName || *pwszName);
	
	wstring_ptr wstrPath;
	if (pwszName) {
		wstrPath = concat(pwszPath, L"\\", pwszName);
		pwszPath = wstrPath.get();
	}
	
	return File::createDirectory(pwszPath);
}

bool qm::ApplicationImpl::ensureFile(const WCHAR* pwszPath,
									 const WCHAR* pwszDir,
									 const WCHAR* pwszFileName,
									 const WCHAR* pwszExtension,
									 const WCHAR* pwszType,
									 const WCHAR* pwszName)
{
	assert(pwszPath);
	assert(pwszFileName);
	assert(pwszType);
	assert(pwszName);
	
	if (!ensureDirectory(pwszPath, pwszDir))
		return false;
	
	StringBuffer<WSTRING> buf;
	buf.append(pwszPath);
	if (pwszDir) {
		buf.append(L'\\');
		buf.append(pwszDir);
	}
	buf.append(L'\\');
	buf.append(pwszFileName);
	if (pwszExtension) {
		buf.append(L'.');
		buf.append(pwszExtension);
	}
	
	W2T(buf.getCharArray(), ptszPath);
	if (::GetFileAttributes(ptszPath) == 0xffffffff) {
		if (!detachResource(buf.getCharArray(), pwszType, pwszName))
			return false;
	}
	
	return true;
}

bool qm::ApplicationImpl::ensureResources(Resource* pResource,
										  size_t nCount)
{
	ResourceDialog::ResourceList listResource;
	struct Deleter
	{
		Deleter(ResourceDialog::ResourceList& l) :
			l_(l)
		{
		}
		
		~Deleter()
		{
			std::for_each(l_.begin(), l_.end(),
				unary_compose_f_gx(
					string_free<WSTRING>(),
					std::select1st<ResourceDialog::ResourceList::value_type>()));
		}
		ResourceDialog::ResourceList& l_;
	} deleter(listResource);
	
	size_t nMailFolderLen = wcslen(wstrMailFolder_.get()) + 1;
	
	for (size_t n = 0; n < nCount; ++n) {
		Resource* p = pResource + n;
		
		StringBuffer<WSTRING> baseDir;
		baseDir.append(p->pwszDir_);
		baseDir.append(L'\\');
		if (p->pwszSubDir_) {
			baseDir.append(p->pwszSubDir_);
			baseDir.append(L'\\');
		}
		
		p->state_ = RS_NOTEXIST;
		unsigned int nRevision = pProfile_->getInt(
			L"Resource", p->pwszResourceName_, 0);
		if (nRevision != p->nRevision_) {
			if (p->pwszProfile_) {
				ConcatW c[] = {
					{ baseDir.getCharArray(),		baseDir.getLength()	},
					{ p->pwszProfile_,				-1					},
					{ L"\\",						1					},
					{ p->pwszFileName_,				-1					}
				};
				wstring_ptr wstrPath(concat(c, countof(c)));
				W2T(wstrPath.get(), ptszPath);
				if (::GetFileAttributes(ptszPath) != 0xffffffff) {
					p->state_ = RS_EXISTPROFILE;
					
					wstring_ptr wstr(allocWString(wstrPath.get() + nMailFolderLen));
					listResource.push_back(std::make_pair(wstr.get(), true));
					wstr.release();
				}
			}
			if (p->state_ == RS_NOTEXIST) {
				ConcatW c[] = {
					{ baseDir.getCharArray(),		baseDir.getLength()	},
					{ p->pwszFileName_,				-1					}
				};
				wstring_ptr wstrPath(concat(c, countof(c)));
				W2T(wstrPath.get(), ptszPath);
				if (::GetFileAttributes(ptszPath) != 0xffffffff) {
					p->state_ = RS_EXIST;
					
					wstring_ptr wstr(allocWString(wstrPath.get() + nMailFolderLen));
					listResource.push_back(std::make_pair(wstr.get(), true));
					wstr.release();
				}
			}
		}
		else {
			p->state_ = RS_SAMEREVISION;
		}
	}
	
	bool bBackup = true;
	if (!listResource.empty()) {
		ResourceDialog dialog(listResource);
		if (dialog.doModal(0) != IDOK)
			return false;
		
		int nResource = 0;
		for (size_t n = 0; n < nCount; ++n) {
			Resource* p = pResource + n;
			
			if (p->state_ == RS_EXIST ||
				p->state_ == RS_EXISTPROFILE) {
				if (listResource[nResource].second)
					p->state_ = p->state_ == RS_EXIST ?
						RS_OVERWRITE : RS_OVERWRITEPROFILE;
				++nResource;
			}
		}
		
		bBackup = dialog.isBackup();
	}
	
	for (size_t n = 0; n < nCount; ++n) {
		const Resource* p = pResource + n;
		
		if (p->state_ == RS_NOTEXIST ||
			p->state_ == RS_OVERWRITE ||
			p->state_ == RS_OVERWRITEPROFILE) {
			StringBuffer<WSTRING> buf;
			buf.append(p->pwszDir_);
			if (p->pwszSubDir_) {
				buf.append(L'\\');
				buf.append(p->pwszSubDir_);
			}
			
			if (!ensureDirectory(buf.getCharArray(), 0))
				return false;
			
			buf.append(L'\\');
			if (p->state_ == RS_OVERWRITEPROFILE) {
				buf.append(p->pwszProfile_);
				buf.append(L'\\');
			}
			buf.append(p->pwszFileName_);
			
			if (bBackup && (p->state_ == RS_OVERWRITE || p->state_ == RS_OVERWRITEPROFILE)) {
				wstring_ptr wstrBackupPath(concat(buf.getCharArray(), L".bak"));
				W2T(buf.getCharArray(), ptszOld);
				W2T(wstrBackupPath.get(), ptszNew);
				if (::GetFileAttributes(ptszNew) != 0xffffffff) {
					if (!::DeleteFile(ptszNew))
						return false;
				}
				if (!::MoveFile(ptszOld, ptszNew))
					return false;
			}
			
			if (!detachResource(buf.getCharArray(),
				p->pwszResourceType_, p->pwszResourceName_))
				return false;
		}
		
		pProfile_->setInt(L"Resource", p->pwszResourceName_, p->nRevision_);
	}
	
	return true;
}

bool qm::ApplicationImpl::detachResource(const WCHAR* pwszPath,
										 const WCHAR* pwszType,
										 const WCHAR* pwszName)
{
	assert(pwszPath);
	assert(pwszType);
	assert(pwszName);
	
	W2T(pwszName, ptszName);
	W2T(pwszType, ptszType);
	HRSRC hrsrc = ::FindResource(hInstResource_, ptszName, ptszType);
	if (!hrsrc)
		return false;
	HGLOBAL hResource = ::LoadResource(hInstResource_, hrsrc);
	if (!hResource)
		return false;
	void* pResource = ::LockResource(hResource);
	if (!pResource)
		return false;
	int nLen = ::SizeofResource(hInstResource_, hrsrc);
	
	FileOutputStream stream(pwszPath);
	if (!stream ||
		stream.write(static_cast<unsigned char*>(pResource), nLen) == -1 ||
		!stream.close())
		return false;
	
	return true;
}

void qm::ApplicationImpl::restoreCurrentFolder()
{
	wstring_ptr wstrFolder(pProfile_->getString(
		L"Global", L"CurrentFolder", L""));
	
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
				Folder* pFolder = pAccount->getFolder(pwszFolder);
				if (pFolder)
					pFolderModel->setCurrent(0, pFolder, false);
			}
			else {
				pFolderModel->setCurrent(pAccount, 0, false);
			}
		}
	}
}

void qm::ApplicationImpl::saveCurrentFolder()
{
	StringBuffer<WSTRING> buf;
	
	FolderModel* pFolderModel = pMainWindow_->getFolderModel();
	Account* pAccount = pFolderModel->getCurrentAccount();
	Folder* pFolder = pFolderModel->getCurrentFolder();
	if (!pAccount && pFolder)
		pAccount = pFolder->getAccount();
	if (pAccount) {
		buf.append(L"//");
		buf.append(pAccount->getName());
		if (pFolder) {
			buf.append(L'/');
			wstring_ptr wstrFolder(pFolder->getFullName());
			buf.append(wstrFolder.get());
		}
	}
	
	pProfile_->setString(L"Global", L"CurrentFolder", buf.getCharArray());
}

bool qm::ApplicationImpl::canCheck()
{
	return !pMainWindow_->isShowingModalDialog() &&
		!pSyncManager_->isSyncing();
}

void qm::ApplicationImpl::loadLibrary(const WCHAR* pwszName)
{
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
	wstring_ptr wstrLib(concat(L"qm", pwszName, SUFFIX L".dll"));
	W2T(wstrLib.get(), ptszLib);
	::LoadLibrary(ptszLib);
}


/****************************************************************************
 *
 * Application
 *
 */

qm::Application::Application(HINSTANCE hInst,
							 wstring_ptr wstrMailFolder,
							 wstring_ptr wstrProfile,
							 std::auto_ptr<MailFolderLock> pLock)
{
	assert(wstrMailFolder.get());
	assert(wstrProfile.get());
	assert(pLock.get());
	
	pImpl_ = new ApplicationImpl();
	pImpl_->hInst_ = hInst;
	pImpl_->hInstResource_ = hInst;
	pImpl_->pLock_ = pLock;
	pImpl_->wstrMailFolder_ = wstrMailFolder;
	pImpl_->wstrProfileName_ = wstrProfile;
	pImpl_->pMainWindow_ = 0;
	pImpl_->hInstAtl_ = 0;
	
	assert(!ApplicationImpl::pApplication__);
	ApplicationImpl::pApplication__ = this;
}

qm::Application::~Application()
{
	assert(ApplicationImpl::pApplication__ == this);
	ApplicationImpl::pApplication__ = 0;
	delete pImpl_;
}

bool qm::Application::initialize()
{
	pImpl_->pWinSock_.reset(new Winsock());
	
	Part::setDefaultCharset(L"iso-2022-jp");
	Part::setGlobalOptions(Part::O_USE_COMMENT_AS_PHRASE |
		Part::O_ALLOW_ENCODED_QSTRING |
		Part::O_ALLOW_ENCODED_PARAMETER |
		Part::O_ALLOW_PARAMETER_INVALID_SEMICOLON |
		Part::O_ALLOW_ADDRESS_WITHOUT_DOMAIN |
		Part::O_ALLOW_INCOMPLETE_MULTIPART |
		Part::O_ALLOW_RAW_FIELD |
		Part::O_ALLOW_SPECIALS_IN_REFERENCES |
		Part::O_ALLOW_INVALID_PERIOD_IN_LOCALPART |
		Part::O_ALLOW_SINGLE_DIGIT_TIME);
	
	if (!pImpl_->ensureDirectory(pImpl_->wstrMailFolder_.get(), 0))
		return false;
	const WCHAR* pwszDirs[] = {
		L"accounts",
		L"templates",
		L"scripts",
		L"security",
		L"profiles",
		L"logs"
	};
	for (int n = 0; n < countof(pwszDirs); ++n) {
		if (!pImpl_->ensureDirectory(pImpl_->wstrMailFolder_.get(), pwszDirs[n]))
			return false;
	}
	
	wstring_ptr wstrProfileDir(concat(pImpl_->wstrMailFolder_.get(), L"\\profiles"));
	if (!pImpl_->ensureFile(wstrProfileDir.get(),
		*pImpl_->wstrProfileName_.get() ? pImpl_->wstrProfileName_.get() : 0,
		FileNames::QMAIL_XML, 0, L"PROFILE", FileNames::QMAIL_XML))
		return false;
	
	wstring_ptr wstrProfilePath(getProfilePath(FileNames::QMAIL_XML));
	pImpl_->pProfile_.reset(new XMLProfile(wstrProfilePath.get()));
	if (!pImpl_->pProfile_->load())
		return false;
	
	wstring_ptr wstrTemplateDir(concat(
		pImpl_->wstrMailFolder_.get(), L"\\templates"));
	
#define DECLARE_PROFILE(name, revision) \
	{ \
		wstrProfileDir.get(), \
		0, \
		pImpl_->wstrProfileName_.get(), \
		name, \
		L"PROFILE", \
		name, \
		revision, \
	} \

#define DECLARE_TEMPLATE(classname, name, revision) \
	{ \
		wstrTemplateDir.get(), \
		classname, \
		0, \
		name L".template", \
		L"TEMPLATE", \
		classname L"." name, \
		revision, \
	} \

	ApplicationImpl::Resource resources[] = {
		DECLARE_PROFILE(FileNames::COLORS_XML,		1000),
		DECLARE_PROFILE(FileNames::HEADER_XML,		1000),
		DECLARE_PROFILE(FileNames::HEADEREDIT_XML,	1000),
		DECLARE_PROFILE(FileNames::KEYMAP_XML,		1000),
		DECLARE_PROFILE(FileNames::MENUS_XML,		1000),
		DECLARE_PROFILE(FileNames::TOOLBAR_BMP,		1000),
		DECLARE_PROFILE(FileNames::TOOLBARS_XML,	1000),
		DECLARE_PROFILE(FileNames::VIEWS_XML,		1000),
		DECLARE_TEMPLATE(L"mail",	L"new",			1000),
		DECLARE_TEMPLATE(L"mail",	L"reply",		1000),
		DECLARE_TEMPLATE(L"mail",	L"reply_all",	1000),
		DECLARE_TEMPLATE(L"mail",	L"forward",		1000),
		DECLARE_TEMPLATE(L"mail",	L"edit",		1000),
		DECLARE_TEMPLATE(L"mail",	L"url",			1000),
		DECLARE_TEMPLATE(L"mail",	L"print",		1000),
		DECLARE_TEMPLATE(L"news",	L"new",			1000),
		DECLARE_TEMPLATE(L"news",	L"reply",		1000),
		DECLARE_TEMPLATE(L"news",	L"reply_all",	1000),
		DECLARE_TEMPLATE(L"news",	L"forward",		1000),
		DECLARE_TEMPLATE(L"news",	L"edit",		1000),
		DECLARE_TEMPLATE(L"news",	L"url",			1000),
		DECLARE_TEMPLATE(L"news",	L"print",		1000),
	};
	if (!pImpl_->ensureResources(resources, countof(resources)))
		return false;
	
	int nLog = pImpl_->pProfile_->getInt(L"Global", L"Log", -1);
	if (nLog >= 0) {
		wstring_ptr wstrLogDir(concat(
			pImpl_->wstrMailFolder_.get(), L"\\logs"));
		if (nLog > Logger::LEVEL_DEBUG)
			nLog = Logger::LEVEL_DEBUG;
		Init::getInit().setLogInfo(true, wstrLogDir.get(),
			static_cast<Logger::Level>(nLog));
	}
	
	wstring_ptr wstrTempFolder(
		pImpl_->pProfile_->getString(L"Global", L"TemporaryFolder", L""));
	if (!*wstrTempFolder.get()) {
		TCHAR tszPath[MAX_PATH];
		::GetTempPath(countof(tszPath), tszPath);
		wstrTempFolder = tcs2wcs(tszPath);
	}
	pImpl_->wstrTemporaryFolder_ = wstrTempFolder;
	
	pImpl_->pUIManager_.reset(new UIManager());
	
	const WCHAR* pwszLibraries[] = {
		L"smtp",
		L"pop3",
		L"imap4",
		L"nntp",
		L"rss"
	};
	for (int n = 0; n < countof(pwszLibraries); ++n)
		ApplicationImpl::loadLibrary(pwszLibraries[n]);
	wstring_ptr wstrLibraries(pImpl_->pProfile_->getString(L"Global", L"Libraries", L""));
	WCHAR* p = wcstok(wstrLibraries.get(), L" ,");
	while (p) {
		ApplicationImpl::loadLibrary(p);
		p = wcstok(0, L" ,");
	}
	
	Security::init();
	
	pImpl_->pDocument_.reset(new Document(pImpl_->pProfile_.get()));
	pImpl_->pSyncManager_.reset(new SyncManager(pImpl_->pProfile_.get()));
	pImpl_->pSyncDialogManager_.reset(new SyncDialogManager(pImpl_->pProfile_.get()));
	pImpl_->pGoRound_.reset(new GoRound());
	pImpl_->pTempFileCleaner_.reset(new TempFileCleaner());
	
	std::auto_ptr<MainWindow> pMainWindow(new MainWindow(pImpl_->pProfile_.get()));
#ifdef _WIN32_WCE
	DWORD dwStyle = WS_CLIPCHILDREN;
	DWORD dwExStyle = 0;
#else
	DWORD dwStyle = WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN;
	DWORD dwExStyle = WS_EX_WINDOWEDGE;
#endif
	MainWindowCreateContext context = {
		pImpl_->pDocument_.get(),
		pImpl_->pUIManager_.get(),
		pImpl_->pSyncManager_.get(),
		pImpl_->pSyncDialogManager_.get(),
		pImpl_->pGoRound_.get(),
		pImpl_->pTempFileCleaner_.get(),
	};
	wstring_ptr wstrTitle(getVersion(L' ', false));
	if (!pMainWindow->create(L"QmMainWindow", wstrTitle.get(), dwStyle,
		CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
		0, dwExStyle, 0, 0, &context))
		return false;
	pImpl_->pMainWindow_ = pMainWindow.release();
	setMainWindow(pImpl_->pMainWindow_);
	pImpl_->pMainWindow_->initialShow();
	
	wstring_ptr wstrAccountFolder(concat(
		pImpl_->wstrMailFolder_.get(), L"\\accounts"));
	if (!pImpl_->pDocument_->loadAccounts(wstrAccountFolder.get()))
		return false;
	
	pImpl_->pMainWindow_->updateWindow();
	pImpl_->pMainWindow_->setForegroundWindow();
	
	if (!pImpl_->pProfile_->getInt(L"Global", L"Offline", 1))
		pImpl_->pDocument_->setOffline(false);
	
	pImpl_->restoreCurrentFolder();
	
	pImpl_->pNewMailChecker_.reset(new NewMailChecker(pImpl_->pProfile_.get(),
		pImpl_->pDocument_.get(), pImpl_->pGoRound_.get(),
		pImpl_->pSyncManager_.get(), pImpl_->pSyncDialogManager_.get(),
		pImpl_->pMainWindow_->getHandle(), pImpl_));
	
	return true;
}

void qm::Application::uninitialize()
{
	assert(pImpl_);
	
	pImpl_->pLock_->unsetWindow();
	
	pImpl_->pNewMailChecker_.reset(0);
	
#ifndef _WIN32_WCE
	ComPtr<IDataObject> pDataObject;
	HRESULT hr = ::OleGetClipboard(&pDataObject);
	if (hr == S_OK) {
		hr = pDataObject->QueryGetData(
			&MessageDataObject::formats__[MessageDataObject::FORMAT_FOLDER]);
		if (hr == S_OK)
			::OleFlushClipboard();
	}
#endif
	
	pImpl_->pUIManager_.reset(0);
	pImpl_->pTempFileCleaner_.reset(0);
	pImpl_->pGoRound_.reset(0);
	pImpl_->pSyncDialogManager_.reset(0);
	pImpl_->pSyncManager_.reset(0);
	pImpl_->pDocument_.reset(0);
	pImpl_->pProfile_.reset(0);
	
	Part::setDefaultCharset(0);
	
	Security::term();
	
	if (pImpl_->hInstAtl_) {
		::FreeLibrary(pImpl_->hInstAtl_);
		pImpl_->hInstAtl_ = 0;
	}
	
	pImpl_->pWinSock_.reset(0);
	pImpl_->pLock_.reset(0);
}

void qm::Application::run()
{
	MessageLoop::getMessageLoop().run();
}

bool qm::Application::save()
{
	if (!pImpl_->pDocument_->save())
		return false;
	if (!pImpl_->pMainWindow_->save())
		return false;
	if (!pImpl_->pSyncDialogManager_->save())
		return false;
	pImpl_->saveCurrentFolder();
	if (!pImpl_->pProfile_->save())
		return false;
	if (!pImpl_->pUIManager_->save())
		return false;
	return true;
}

HINSTANCE qm::Application::getResourceHandle() const
{
	return pImpl_->hInstResource_;
}

HINSTANCE qm::Application::getAtlHandle() const
{
	if (!pImpl_->hInstAtl_) {
#ifdef _WIN32_WCE
		pImpl_->hInstAtl_ = ::LoadLibrary(_T("atlce400.dll"));
#else
		pImpl_->hInstAtl_ = ::LoadLibrary(_T("atl.dll"));
#endif
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
	return pImpl_->wstrMailFolder_.get();
}

const WCHAR* qm::Application::getTemporaryFolder() const
{
	return pImpl_->wstrTemporaryFolder_.get();
}

const WCHAR* qm::Application::getProfileName() const
{
	return pImpl_->wstrProfileName_.get();
}

wstring_ptr qm::Application::getProfilePath(const WCHAR* pwszName) const
{
	assert(pwszName);
	
	wstring_ptr wstrPath;
	if (*pImpl_->wstrProfileName_.get()) {
		ConcatW c[] = {
			{ pImpl_->wstrMailFolder_.get(),	-1	},
			{ L"\\profiles\\",					-1	},
			{ pImpl_->wstrProfileName_.get(),	-1	},
			{ L"\\",							1	},
			{ pwszName,							-1	}
		};
		wstrPath = concat(c, countof(c));
		
		W2T(wstrPath.get(), ptszPath);
		if (::GetFileAttributes(ptszPath) == 0xffffffff)
			wstrPath.reset(0);
	}
	
	if (!wstrPath.get())
		wstrPath = concat(pImpl_->wstrMailFolder_.get(),
			L"\\profiles\\", pwszName);

	return wstrPath;
}

wstring_ptr qm::Application::getVersion(WCHAR cSeparator,
										bool bWithOSVersion) const
{
	wstring_ptr wstrVersion(allocWString(256));
	
	if (bWithOSVersion) {
		wstring_ptr wstrOSVersion(getOSVersion());
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
		swprintf(wstrVersion.get(), L"QMAIL%c%d.%d.%d.%d / %s / %s",
			cSeparator, QMAIL_VERSION/100000, (QMAIL_VERSION%100000)/1000,
			QMAIL_VERSION%1000, QMAIL_REVISION, wstrOSVersion.get(), pwszCPU);
	}
	else {
		swprintf(wstrVersion.get(), L"QMAIL%c%d.%d.%d.%d", cSeparator,
			QMAIL_VERSION/100000, (QMAIL_VERSION%100000)/1000,
			QMAIL_VERSION%1000, QMAIL_REVISION);
	}
	
	return wstrVersion;
}

wstring_ptr qm::Application::getOSVersion() const
{
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
	
	wstring_ptr wstrOSVersion(allocWString(256));
	T2W(ovi.szCSDVersion, pwszAdditional);
	if (bAddVersion)
		swprintf(wstrOSVersion.get(), L"%s %u.%02u%s%s",
			pwszPlatform, ovi.dwMajorVersion, ovi.dwMinorVersion,
			*pwszAdditional ? L" " : L"", pwszAdditional);
	else
		swprintf(wstrOSVersion.get(), L"%s%s%s",
			pwszPlatform, *pwszAdditional ? L" " : L"", pwszAdditional);
	
	return wstrOSVersion;
}

Application& qm::Application::getApplication()
{
	return *ApplicationImpl::pApplication__;
}
