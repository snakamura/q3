/*
 * $Id$
 *
 * Copyright(C) 1998-2005 Satoshi Nakamura
 * All rights reserved.
 *
 */

#pragma warning(disable:4786)

#include <qm.h>
#include <qmaccount.h>
#include <qmapplication.h>
#include <qmdocument.h>
#include <qmfilenames.h>
#include <qmgoround.h>
#include <qmmainwindow.h>
#include <qmpassword.h>
#include <qmprotocoldriver.h>
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
#include <qstextutil.h>
#include <qswindow.h>

#include <algorithm>

#include <windows.h>
#include <tchar.h>

#include "main.h"
#include "resourcefile.h"
#ifndef DEPENDCHECK
#	include "version.h"
#endif
#include "../model/dataobject.h"
#include "../model/tempfilecleaner.h"
#include "../sync/autopilot.h"
#include "../sync/syncmanager.h"
#include "../ui/dialogs.h"
#include "../ui/mainwindow.h"
#include "../ui/syncdialog.h"
#include "../ui/uimanager.h"
#include "../ui/uiutil.h"
#include "../uimodel/foldermodel.h"
#include "../util/util.h"

using namespace qm;
using namespace qs;


/****************************************************************************
 *
 * ApplicationImpl
 *
 */

struct qm::ApplicationImpl : public AutoPilotCallback
{
public:
	enum ResourceState {
		RS_NOTEXIST			= 0x00,
		RS_EXIST			= 0x01,
		RS_OVERWRITE		= 0x02,
		RS_SAMEREVISION		= 0x04,
		RS_PROFILE			= 0x10,
		RS_BACKUP			= 0x20
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
		bool bOverwrite_;
		unsigned int nState_;
	};

public:
	bool ensureDirectories();
	bool loadMainProfile();
	void initMime();
	void initLog();
	bool ensureResources();
	void ensureTempDirectory();
	void loadLibraries();

public:
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
	virtual bool canAutoPilot();

public:
	static bool ensureDirectory(const WCHAR* pwszPath,
								const WCHAR* pwszName);
	static bool ensureParentDirectory(const WCHAR* pwszPath);
	static void loadLibrary(const WCHAR* pwszName);

private:
	static qs::wstring_ptr getResourcePath(const Resource& resource,
										   bool bProfile);

public:
	Application* pThis_;
	HINSTANCE hInst_;
	HINSTANCE hInstResource_;
	std::auto_ptr<MailFolderLock> pLock_;
	std::auto_ptr<Winsock> pWinSock_;
	wstring_ptr wstrMailFolder_;
	wstring_ptr wstrTemporaryFolder_;
	wstring_ptr wstrProfileName_;
	std::auto_ptr<XMLProfile> pProfile_;
	std::auto_ptr<Document> pDocument_;
	std::auto_ptr<PasswordManager> pPasswordManager_;
	std::auto_ptr<PasswordManagerCallback> pPasswordManagerCallback_;
	std::auto_ptr<SyncManager> pSyncManager_;
	std::auto_ptr<SyncDialogManager> pSyncDialogManager_;
	std::auto_ptr<GoRound> pGoRound_;
	std::auto_ptr<TempFileCleaner> pTempFileCleaner_;
	std::auto_ptr<AutoPilotManager> pAutoPilotManager_;
	std::auto_ptr<AutoPilot> pAutoPilot_;
	std::auto_ptr<UIManager> pUIManager_;
	MainWindow* pMainWindow_;
	HINSTANCE hInstAtl_;
	bool bShutdown_;
	
	static Application* pApplication__;
};

Application* qm::ApplicationImpl::pApplication__ = 0;

bool qm::ApplicationImpl::ensureDirectories()
{
	if (!ensureDirectory(wstrMailFolder_.get(), 0))
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
		if (!ensureDirectory(wstrMailFolder_.get(), pwszDirs[n]))
			return false;
	}
	
	return true;
}

bool qm::ApplicationImpl::loadMainProfile()
{
	assert(!pProfile_.get());
	
	wstring_ptr wstrProfileDir(concat(wstrMailFolder_.get(), L"\\profiles"));
	if (!ensureFile(wstrProfileDir.get(),
		*wstrProfileName_.get() ? wstrProfileName_.get() : 0,
		FileNames::QMAIL_XML, 0, L"PROFILE", FileNames::QMAIL_XML))
		return false;
	
	wstring_ptr wstrProfilePath(pThis_->getProfilePath(FileNames::QMAIL_XML));
	pProfile_.reset(new XMLProfile(wstrProfilePath.get()));
	if (!pProfile_->load())
		return false;
	
	return true;
}

void qm::ApplicationImpl::initMime()
{
	assert(pProfile_.get());
	
	unsigned int nOptions = Part::O_USE_COMMENT_AS_PHRASE |
		Part::O_INTERPRET_FORMAT_FLOWED |
		Part::O_ALLOW_ALL;
	if (pProfile_->getInt(L"Global", L"RFC2231", 0))
		nOptions |= Part::O_RFC2231;
	Part::setGlobalOptions(nOptions);
	
	wstring_ptr wstrDefaultCharset(pProfile_->getString(L"Global", L"DefaultCharset", 0));
	const WCHAR* pwszCandidates[] = {
		wstrDefaultCharset.get(),
		Init::getInit().getMailEncoding(),
		Init::getInit().getSystemEncoding(),
		L"utf-8"
	};
	for (int n = 0; n < countof(pwszCandidates); ++n) {
		const WCHAR* pwszCharset = pwszCandidates[n];
		if (pwszCharset && *pwszCharset && ConverterFactory::getInstance(pwszCharset).get()) {
			Part::setDefaultCharset(pwszCharset);
			break;
		}
	}
}

void qm::ApplicationImpl::initLog()
{
	assert(pProfile_.get());
	
	Init& init = Init::getInit();
	wstring_ptr wstrLogDir(concat(wstrMailFolder_.get(), L"\\logs"));
	init.setLogDirectory(wstrLogDir.get());
	int nLog = pProfile_->getInt(L"Global", L"Log", -1);
	if (nLog >= 0) {
		if (nLog > Logger::LEVEL_DEBUG)
			nLog = Logger::LEVEL_DEBUG;
		init.setLogLevel(static_cast<Logger::Level>(nLog));
		init.setLogEnabled(true);
	}
	wstring_ptr wstrLogFilter(pProfile_->getString(L"Global", L"LogFilter", L""));
	if (*wstrLogFilter.get())
		init.setLogFilter(wstrLogFilter.get());
}

bool qm::ApplicationImpl::ensureResources()
{
	wstring_ptr wstrProfileDir(concat(wstrMailFolder_.get(), L"\\profiles"));
	wstring_ptr wstrTemplateDir(concat(wstrMailFolder_.get(), L"\\templates"));
	
#define DECLARE_PROFILE(name, revision, overwrite) \
	{ \
		wstrProfileDir.get(), \
		0, \
		wstrProfileName_.get(), \
		name, \
		L"PROFILE", \
		name, \
		revision, \
		overwrite, \
	} \

#define DECLARE_TEMPLATE(classname, name, revision) \
	{ \
		wstrTemplateDir.get(), \
		classname, \
		L"", \
		name L".template", \
		L"TEMPLATE", \
		classname L"." name, \
		revision, \
		true, \
	} \

	ApplicationImpl::Resource resources[] = {
		DECLARE_PROFILE(FileNames::COLORS_XML,		COLORS_XML_REVISION,				false	),
		DECLARE_PROFILE(FileNames::FILTERS_XML,		FILTERS_XML_REVISION,				false	),
		DECLARE_PROFILE(FileNames::HEADER_XML,		HEADER_XML_REVISION,				true	),
		DECLARE_PROFILE(FileNames::HEADEREDIT_XML,	HEADEREDIT_XML_REVISION,			true	),
		DECLARE_PROFILE(FileNames::KEYMAP_XML,		KEYMAP_XML_REVISION,				true	),
		DECLARE_PROFILE(FileNames::MENUS_XML,		MENUS_XML_REVISION,					true	),
		DECLARE_PROFILE(FileNames::SYNCFILTERS_XML,	SYNCFILTERS_XML_REVISION,			false	),
		DECLARE_PROFILE(FileNames::TOOLBAR_BMP,		TOOLBAR_BMP_REVISION,				true	),
		DECLARE_PROFILE(FileNames::TOOLBARS_XML,	TOOLBARS_XML_REVISION,				true	),
		DECLARE_PROFILE(FileNames::VIEWS_XML,		VIEWS_XML_REVISION,					false	),
		DECLARE_TEMPLATE(L"mail",	L"new",			MAIL_NEW_TEMPLATE_REVISION					),
		DECLARE_TEMPLATE(L"mail",	L"reply",		MAIL_REPLY_TEMPLATE_REVISION				),
		DECLARE_TEMPLATE(L"mail",	L"reply_all",	MAIL_REPLY_ALL_TEMPLATE_REVISION			),
		DECLARE_TEMPLATE(L"mail",	L"forward",		MAIL_FORWARD_TEMPLATE_REVISION				),
		DECLARE_TEMPLATE(L"mail",	L"edit",		MAIL_EDIT_TEMPLATE_REVISION					),
		DECLARE_TEMPLATE(L"mail",	L"url",			MAIL_URL_TEMPLATE_REVISION					),
		DECLARE_TEMPLATE(L"mail",	L"print",		MAIL_PRINT_TEMPLATE_REVISION				),
		DECLARE_TEMPLATE(L"mail",	L"quote",		MAIL_QUOTE_TEMPLATE_REVISION				),
		DECLARE_TEMPLATE(L"news",	L"new",			NEWS_NEW_TEMPLATE_REVISION					),
		DECLARE_TEMPLATE(L"news",	L"reply",		NEWS_REPLY_TEMPLATE_REVISION				),
		DECLARE_TEMPLATE(L"news",	L"reply_all",	NEWS_REPLY_ALL_TEMPLATE_REVISION			),
		DECLARE_TEMPLATE(L"news",	L"forward",		NEWS_FORWARD_TEMPLATE_REVISION				),
		DECLARE_TEMPLATE(L"news",	L"edit",		NEWS_EDIT_TEMPLATE_REVISION					),
		DECLARE_TEMPLATE(L"news",	L"print",		NEWS_PRINT_TEMPLATE_REVISION				),
		DECLARE_TEMPLATE(L"news",	L"quote",		NEWS_QUOTE_TEMPLATE_REVISION				),
	};
	if (!ensureResources(resources, countof(resources)))
		return false;
	
	return true;
}

void qm::ApplicationImpl::ensureTempDirectory()
{
	assert(pProfile_.get());
	assert(!wstrTemporaryFolder_.get());
	
	wstring_ptr wstrTempFolder(pProfile_->getString(L"Global", L"TemporaryFolder", L""));
	if (!*wstrTempFolder.get()) {
		wstrTempFolder = concat(wstrMailFolder_.get(), L"\\temp");
		File::createDirectory(wstrTempFolder.get());
	}
	if (wstrTempFolder.get()[wcslen(wstrTempFolder.get()) - 1] != L'\\')
		wstrTempFolder = concat(wstrTempFolder.get(), L"\\");
	
	wstrTemporaryFolder_ = wstrTempFolder;
}

void qm::ApplicationImpl::loadLibraries()
{
	assert(pProfile_.get());
	
	const WCHAR* pwszLibraries[] = {
		L"qmsmtp",
		L"qmpop3",
		L"qmimap4",
		L"qmnntp",
		L"qmrss",
		L"qmscript",
		L"qmjunk"
	};
	for (int n = 0; n < countof(pwszLibraries); ++n)
		loadLibrary(pwszLibraries[n]);
	
	wstring_ptr wstrLibraries(pProfile_->getString(L"Global", L"Libraries", L""));
	WCHAR* p = wcstok(wstrLibraries.get(), L" ,");
	while (p) {
		loadLibrary(p);
		p = wcstok(0, L" ,");
	}
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
	Log log(InitThread::getInitThread().getLogger(), L"qm::ApplicationImpl");
	
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
	
	wstring_ptr wstrResourceFilePath(concat(wstrMailFolder_.get(),
		L"\\profiles\\", FileNames::RESOURCES_XML));
	ResourceFileList files(wstrResourceFilePath.get());
	
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
		
		p->nState_ = RS_NOTEXIST;
		
		struct {
			bool bTry_;
			bool bProfile_;
		} tries[] = {
			{ *p->pwszProfile_ != L'0',	true	},
			{ true,						false	}
		};
		for (int m = 0; m < countof(tries) && p->nState_ == RS_NOTEXIST; ++m) {
			if (tries[m].bTry_) {
				wstring_ptr wstrPath(getResourcePath(*p, tries[m].bProfile_));
				
				const WCHAR* pwszName = wstrPath.get() + nMailFolderLen;
				const ResourceFile* pFile = files.getResourceFile(pwszName);
				unsigned int nRevision = pFile ? pFile->getRevision() : 0;
				
				W2T(wstrPath.get(), ptszPath);
				WIN32_FIND_DATA fd;
				AutoFindHandle hFind(::FindFirstFile(ptszPath, &fd));
				if (hFind.get()) {
					if (p->bOverwrite_ && nRevision != p->nRevision_) {
						if (pFile && ::CompareFileTime(&pFile->getModified(), &fd.ftLastWriteTime) == 0) {
							p->nState_ = RS_OVERWRITE | (tries[m].bProfile_ ? RS_PROFILE : 0);
						}
						else {
							p->nState_ = RS_EXIST | (tries[m].bProfile_ ? RS_PROFILE : 0);
							wstring_ptr wstrName(allocWString(pwszName));
							listResource.push_back(std::make_pair(wstrName.get(), true));
							wstrName.release();
						}
					}
					else {
						p->nState_ = RS_SAMEREVISION;
					}
				}
			}
		}
	}
	
	if (!listResource.empty()) {
		ResourceDialog dialog(listResource);
		if (dialog.doModal(0) != IDOK)
			return false;
		
		bool bBackup = dialog.isBackup();
		
		size_t nResource = 0;
		for (size_t n = 0; n < nCount; ++n) {
			Resource* p = pResource + n;
			
			if (p->nState_ & RS_EXIST) {
				if (listResource[nResource].second) {
					p->nState_ &= ~RS_EXIST;
					p->nState_ |= RS_OVERWRITE | (bBackup ? RS_BACKUP : 0);
				}
				++nResource;
			}
		}
		assert(nResource == listResource.size());
	}
	
	for (size_t n = 0; n < nCount; ++n) {
		const Resource* p = pResource + n;
		
		if (p->nState_ == RS_NOTEXIST || p->nState_ & RS_OVERWRITE) {
			wstring_ptr wstrPath(getResourcePath(*p, (p->nState_ & RS_PROFILE) != 0));
			
			if (!ensureParentDirectory(wstrPath.get()))
				return false;
			
			W2T(wstrPath.get(), ptszPath);
			
			if (p->nState_ & RS_BACKUP) {
				wstring_ptr wstrBackupPath(concat(wstrPath.get(), L".bak"));
				W2T(wstrBackupPath.get(), ptszNew);
				if (::GetFileAttributes(ptszNew) != 0xffffffff) {
					if (!::DeleteFile(ptszNew)) {
						log.errorf(L"Cound not delete the old backup file: %s", wstrBackupPath.get());
						return false;
					}
				}
				if (!::MoveFile(ptszPath, ptszNew)) {
					log.errorf(L"Cound not rename to the backup file: %s, %s", wstrPath.get(), wstrBackupPath.get());
					return false;
				}
			}
			
			if (!detachResource(wstrPath.get(), p->pwszResourceType_, p->pwszResourceName_))
				return false;
			
			WIN32_FIND_DATA fd;
			AutoFindHandle hFind(::FindFirstFile(ptszPath, &fd));
			if (!hFind.get()) {
				log.errorf(L"Cound not find the detached file: %s, %u", wstrPath.get(), ::GetLastError());
				return false;
			}
			files.setResourceFile(wstrPath.get() + nMailFolderLen,
				p->nRevision_, &fd.ftLastWriteTime);
		}
		else if (p->nState_ & RS_EXIST) {
			wstring_ptr wstrPath(getResourcePath(*p, (p->nState_ & RS_PROFILE) != 0));
			files.setResourceFile(wstrPath.get() + nMailFolderLen, p->nRevision_, 0);
		}
	}
	
	if (!files.save())
		return false;
	
	return true;
}

bool qm::ApplicationImpl::detachResource(const WCHAR* pwszPath,
										 const WCHAR* pwszType,
										 const WCHAR* pwszName)
{
	assert(pwszPath);
	assert(pwszType);
	assert(pwszName);
	
	Log log(InitThread::getInitThread().getLogger(), L"qm::ApplicationImpl");
	log.debugf(L"Detaching the resource: %s, %s, %s", pwszName, pwszType, pwszPath);
	
	W2T(pwszName, ptszName);
	W2T(pwszType, ptszType);
	HRSRC hrsrc = ::FindResource(hInstResource_, ptszName, ptszType);
	if (!hrsrc) {
		log.errorf(L"Counld not find a resource: %s, %s", pwszName, pwszType);
		return false;
	}
	HGLOBAL hResource = ::LoadResource(hInstResource_, hrsrc);
	if (!hResource) {
		log.errorf(L"Counld not load the resource: %s", pwszName);
		return false;
	}
	const void* pResource = ::LockResource(hResource);
	if (!pResource) {
		log.errorf(L"Counld not lock the resource: %s", pwszName);
		return false;
	}
	int nLen = ::SizeofResource(hInstResource_, hrsrc);
	
#ifndef _WIN32_WCE
	const unsigned char* p = static_cast<const unsigned char*>(pResource);
#else
	malloc_ptr<unsigned char> pCopy(static_cast<unsigned char*>(malloc(nLen)));
	if (!pCopy.get()) {
		log.errorf(L"Counld not alloc memory for the resource: %s", pwszName);
		return false;
	}
	memcpy(pCopy.get(), pResource, nLen);
	const unsigned char* p = pCopy.get();
#endif
	
	FileOutputStream stream(pwszPath);
	if (!stream || stream.write(p, nLen) == -1 || !stream.close()) {
		log.errorf(L"Counld not write the resource to the file: %s", pwszPath);
		return false;
	}
	
	return true;
}

void qm::ApplicationImpl::restoreCurrentFolder()
{
	wstring_ptr wstrFolder(pProfile_->getString(L"Global", L"CurrentFolder", L""));
	
	FolderModel* pFolderModel = pMainWindow_->getFolderModel();
	
	std::pair<Account*, Folder*> p(Util::getAccountOrFolder(
		pDocument_.get(), wstrFolder.get()));
	if (p.first)
		pFolderModel->setCurrent(p.first, 0, false);
	else if (p.second)
		pFolderModel->setCurrent(0, p.second, false);
}

void qm::ApplicationImpl::saveCurrentFolder()
{
	wstring_ptr wstr;
	
	FolderModel* pFolderModel = pMainWindow_->getFolderModel();
	std::pair<Account*, Folder*> p(pFolderModel->getCurrent());
	if (p.first)
		wstr = Util::formatAccount(p.first);
	else if (p.second)
		wstr = Util::formatFolder(p.second);
	
	pProfile_->setString(L"Global", L"CurrentFolder", wstr.get() ? wstr.get() : L"");
}

bool qm::ApplicationImpl::canAutoPilot()
{
	return !pMainWindow_->isShowingModalDialog() &&
		!pSyncManager_->isSyncing();
}

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

bool qm::ApplicationImpl::ensureParentDirectory(const WCHAR* pwszPath)
{
	const WCHAR* p = wcsrchr(pwszPath, L'\\');
	if (p) {
		wstring_ptr wstrDir(allocWString(pwszPath, p - pwszPath));
		if (!ensureDirectory(wstrDir.get(), 0))
			return false;
	}
	return true;
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
	wstring_ptr wstrLib(concat(pwszName, SUFFIX L".dll"));
	W2T(wstrLib.get(), ptszLib);
	::LoadLibrary(ptszLib);
}

wstring_ptr qm::ApplicationImpl::getResourcePath(const Resource& resource,
												 bool bProfile)
{
	StringBuffer<WSTRING> buf;
	buf.append(resource.pwszDir_);
	buf.append(L'\\');
	if (resource.pwszSubDir_) {
		buf.append(resource.pwszSubDir_);
		buf.append(L'\\');
	}
	
	if (bProfile && *resource.pwszProfile_) {
		buf.append(resource.pwszProfile_);
		buf.append(L'\\');
	}
	
	buf.append(resource.pwszFileName_);
	
	return buf.getString();
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
	pImpl_->pThis_ = this;
	pImpl_->hInst_ = hInst;
	pImpl_->hInstResource_ = loadResourceDll(hInst);
	pImpl_->pLock_ = pLock;
	pImpl_->wstrMailFolder_ = wstrMailFolder;
	pImpl_->wstrProfileName_ = wstrProfile;
	pImpl_->pMainWindow_ = 0;
	pImpl_->hInstAtl_ = 0;
	pImpl_->bShutdown_ = false;
	
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
	if (!pImpl_->ensureDirectories() ||
		!pImpl_->loadMainProfile())
		return false;
	pImpl_->initLog();
	if (!pImpl_->ensureResources())
		return false;
	pImpl_->ensureTempDirectory();
	pImpl_->pUIManager_.reset(new UIManager());
	pImpl_->loadLibraries();
	pImpl_->initMime();
	Security::init();
	
	pImpl_->pPasswordManagerCallback_.reset(new DefaultPasswordManagerCallback());
	pImpl_->pPasswordManager_.reset(new PasswordManager(
		getProfilePath(FileNames::PASSWORDS_XML).get(),
		pImpl_->pPasswordManagerCallback_.get()));
	
	std::auto_ptr<PasswordCallback> pPasswordCallback(
		new DefaultPasswordCallback(pImpl_->pPasswordManager_.get()));
	ProtocolFactory::setPasswordCallback(pPasswordCallback);
	
	pImpl_->pDocument_.reset(new Document(pImpl_->pProfile_.get(),
		pImpl_->pPasswordManager_.get()));
	pImpl_->pSyncManager_.reset(new SyncManager(pImpl_->pProfile_.get()));
	pImpl_->pSyncDialogManager_.reset(new SyncDialogManager(
		pImpl_->pProfile_.get(), pImpl_->pPasswordManager_.get()));
	pImpl_->pGoRound_.reset(new GoRound(getProfilePath(FileNames::GOROUND_XML).get()));
	pImpl_->pTempFileCleaner_.reset(new TempFileCleaner());
	pImpl_->pAutoPilotManager_.reset(new AutoPilotManager(
		getProfilePath(FileNames::AUTOPILOT_XML).get()));
	pImpl_->pAutoPilot_.reset(new AutoPilot(pImpl_->pAutoPilotManager_.get(),
		pImpl_->pProfile_.get(), pImpl_->pDocument_.get(),
		pImpl_->pGoRound_.get(), pImpl_->pSyncManager_.get(),
		pImpl_->pSyncDialogManager_.get(), pImpl_));
	
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
		pImpl_->pPasswordManager_.get(),
		pImpl_->pSyncManager_.get(),
		pImpl_->pSyncDialogManager_.get(),
		pImpl_->pGoRound_.get(),
		pImpl_->pTempFileCleaner_.get(),
		pImpl_->pAutoPilot_.get()
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
	
	pImpl_->restoreCurrentFolder();
	
	pImpl_->pAutoPilot_->start(pImpl_->pMainWindow_->getHandle());
	
	return true;
}

void qm::Application::uninitialize()
{
	assert(pImpl_);
	
	pImpl_->pLock_->unsetWindow();
	
	pImpl_->pAutoPilot_.reset(0);
	
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

bool qm::Application::save(bool bForce)
{
	if (pImpl_->bShutdown_)
		return true;
	
	if (!pImpl_->pDocument_->save(bForce))
		return false;
	if (!pImpl_->pPasswordManager_->save(bForce))
		return false;
	if (!pImpl_->pMainWindow_->save(bForce))
		return false;
	pImpl_->pSyncDialogManager_->save();
	pImpl_->pAutoPilot_->save();
	pImpl_->saveCurrentFolder();
	if (!pImpl_->pProfile_->save() && !bForce)
		return false;
	
	return true;
}

void qm::Application::startShutdown()
{
	pImpl_->bShutdown_ = true;
}

bool qm::Application::isShutdown() const
{
	return pImpl_->bShutdown_;
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
	const size_t nLen = 256;
	wstring_ptr wstrVersion(allocWString(nLen));
	
	if (bWithOSVersion) {
		wstring_ptr wstrOSVersion(getOSVersion());
#if defined _SH3_
		const WCHAR* pwszCPU = L"SH3";
#elif defined _SH4_
		const WCHAR* pwszCPU = L"SH4";
#elif defined _MIPS_
		const WCHAR* pwszCPU = L"MIPS";
#elif defined _ARM_
		const WCHAR* pwszCPU = L"ARM";
#elif defined _AMD64_
		const WCHAR* pwszCPU = L"x64";
#elif defined _X86_
		const WCHAR* pwszCPU = L"x86";
#else
#	error Unknown CPU
#endif
		_snwprintf(wstrVersion.get(), nLen, L"QMAIL%c%d.%d.%d.%d / %s / %s",
			cSeparator, QMAIL_VERSION/100000, (QMAIL_VERSION%100000)/1000,
			QMAIL_VERSION%1000, QMAIL_REVISION, wstrOSVersion.get(), pwszCPU);
	}
	else {
		_snwprintf(wstrVersion.get(), nLen, L"QMAIL%c%d.%d.%d.%d", cSeparator,
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
		switch (ovi.dwMajorVersion) {
		case 5:
			switch (ovi.dwMinorVersion) {
			case 0:
				pwszPlatform = L"Windows 2000";
				bAddVersion = false;
				break;
			case 1:
			case 2:
				pwszPlatform = L"Windows XP";
				bAddVersion = false;
				break;
			default:
				pwszPlatform = L"Windows NT";
				break;
			}
			break;
		case 6:
			pwszPlatform = L"Windows Vista";
			bAddVersion = false;
			break;
		default:
			pwszPlatform = L"Windows NT";
			break;
		}
		break;
#ifdef _WIN32_WCE
	case VER_PLATFORM_WIN32_CE:
		pwszPlatform = L"Windows CE";
		break;
#endif
	}
	
	const size_t nLen = 256;
	wstring_ptr wstrOSVersion(allocWString(nLen));
	T2W(ovi.szCSDVersion, pwszAdditional);
	if (bAddVersion)
		_snwprintf(wstrOSVersion.get(), nLen, L"%s %u.%02u%s%s",
			pwszPlatform, ovi.dwMajorVersion, ovi.dwMinorVersion,
			*pwszAdditional ? L" " : L"", pwszAdditional);
	else
		_snwprintf(wstrOSVersion.get(), nLen, L"%s%s%s",
			pwszPlatform, *pwszAdditional ? L" " : L"", pwszAdditional);
	
	return wstrOSVersion;
}

Application& qm::Application::getApplication()
{
	return *ApplicationImpl::pApplication__;
}
