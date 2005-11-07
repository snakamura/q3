/*
 * $Id$
 *
 * Copyright(C) 1998-2005 Satoshi Nakamura
 * All rights reserved.
 *
 */

#include <qmaccount.h>
#include <qmfolder.h>
#include <qmmessage.h>
#include <qmmessageholder.h>

#include <qsassert.h>

#include "main.h"
#include "nntp.h"
#include "nntpdriver.h"
#include "resourceinc.h"

using namespace qmnntp;
using namespace qm;
using namespace qs;


/****************************************************************************
 *
 * NntpDriver
 *
 */

const unsigned int qmnntp::NntpDriver::nSupport__ =
	Account::SUPPORT_LOCALFOLDERSYNC |
	Account::SUPPORT_LOCALFOLDERGETMESSAGE |
	Account::SUPPORT_JUNKFILTER;

qmnntp::NntpDriver::NntpDriver(Account* pAccount,
							   PasswordCallback* pPasswordCallback,
							   const Security* pSecurity) :
	pAccount_(pAccount),
	pPasswordCallback_(pPasswordCallback),
	pSecurity_(pSecurity),
	pNntp_(0),
	pCallback_(0),
	pLogger_(0),
	pSubAccount_(0),
	bOffline_(true),
	nForceDisconnect_(0),
	nLastUsedTime_(0)
{
}

qmnntp::NntpDriver::~NntpDriver()
{
	clearSession();
}

bool qmnntp::NntpDriver::init()
{
	return true;
}

bool qmnntp::NntpDriver::save(bool bForce)
{
	return true;
}

bool qmnntp::NntpDriver::isSupport(Account::Support support)
{
	return (nSupport__ & support) != 0;
}

void qmnntp::NntpDriver::setOffline(bool bOffline)
{
	if (!bOffline_ && bOffline)
		clearSession();
	bOffline_ = bOffline;
}

void qmnntp::NntpDriver::setSubAccount(qm::SubAccount* pSubAccount)
{
	if (pSubAccount != pSubAccount_) {
		clearSession();
		pSubAccount_ = pSubAccount;
		nForceDisconnect_ = pSubAccount_->getProperty(L"Nntp", L"ForceDisconnect", 0);
	}
}

std::auto_ptr<NormalFolder> qmnntp::NntpDriver::createFolder(const WCHAR* pwszName,
															 Folder* pParent)
{
	assert(false);
	return std::auto_ptr<NormalFolder>(0);
}

bool qmnntp::NntpDriver::removeFolder(NormalFolder* pFolder)
{
	assert(false);
	return false;
}

bool qmnntp::NntpDriver::renameFolder(NormalFolder* pFolder,
									  const WCHAR* pwszName)
{
	assert(false);
	return false;
}

bool qmnntp::NntpDriver::moveFolder(NormalFolder* pFolder,
									NormalFolder* pParent,
									const WCHAR* pwszName)
{
	assert(false);
	return false;
}

bool qmnntp::NntpDriver::createDefaultFolders(Account::FolderList* pList)
{
	assert(pList);
	
	struct {
		UINT nId_;
		unsigned int nFlags_;
	} folders[] = {
		{ IDS_FOLDER_OUTBOX,	Folder::FLAG_LOCAL | Folder::FLAG_OUTBOX | Folder::FLAG_DRAFTBOX	},
		{ IDS_FOLDER_POSTED,	Folder::FLAG_LOCAL | Folder::FLAG_SENTBOX							},
		{ IDS_FOLDER_TRASH,		Folder::FLAG_LOCAL | Folder::FLAG_TRASHBOX							},
#ifndef _WIN32_WCE
		{ IDS_FOLDER_JUNK,		Folder::FLAG_LOCAL | Folder::FLAG_JUNKBOX							}
#endif
	};
	
	pList->reserve(countof(folders));
	
	for (int n = 0; n < countof(folders); ++n) {
		wstring_ptr wstrName(loadString(getResourceHandle(), folders[n].nId_));
		NormalFolder* pFolder = new NormalFolder(n + 1, wstrName.get(),
			L'/', folders[n].nFlags_, 0, 0, 0, 0, 0, 0, pAccount_);
		pList->push_back(pFolder);
	}
	
	return true;
}

bool qmnntp::NntpDriver::getRemoteFolders(RemoteFolderList* pList)
{
	// TODO
	return true;
}

std::pair<const WCHAR**, size_t> qmnntp::NntpDriver::getFolderParamNames()
{
	return std::pair<const WCHAR**, size_t>(0, 0);
}

void qmnntp::NntpDriver::setDefaultFolderParams(NormalFolder* pFolder)
{
}

bool qmnntp::NntpDriver::getMessage(MessageHolder* pmh,
									unsigned int nFlags,
									GetMessageCallback* pCallback)
{
	assert(pmh);
	assert(pCallback);
	
	if (bOffline_)
		return true;
	
	if (!prepareSession(pmh->getFolder()))
		return false;
	
	xstring_size_ptr strMessage;
	if (!pNntp_->getMessage(pmh->getId(),
		Nntp::GETMESSAGEFLAG_ARTICLE, &strMessage, pmh->getSize())) {
		clearSession();
		return false;
	}
	if (!strMessage.get())
		return false;
	
	if (!pCallback->message(strMessage.get(), strMessage.size(), Message::FLAG_NONE, false))
		return false;
	
	return true;
}

bool qmnntp::NntpDriver::setMessagesFlags(NormalFolder* pFolder,
										  const MessageHolderList& l,
										  unsigned int nFlags,
										  unsigned int nMask)
{
	assert(false);
	return false;
}

bool qmnntp::NntpDriver::setMessagesLabel(NormalFolder* pFolder,
										  const MessageHolderList& l,
										  const WCHAR* pwszLabel)
{
	assert(false);
	return false;
}

bool qmnntp::NntpDriver::appendMessage(NormalFolder* pFolder,
									   const CHAR* pszMessage,
									   size_t nLen,
									   unsigned int nFlags)
{
	assert(false);
	return false;
}

bool qmnntp::NntpDriver::removeMessages(NormalFolder* pFolder,
										const MessageHolderList& l)
{
	assert(false);
	return false;
}

bool qmnntp::NntpDriver::copyMessages(const MessageHolderList& l,
									  NormalFolder* pFolderFrom,
									  NormalFolder* pFolderTo,
									  bool bMove)
{
	assert(false);
	return false;
}

bool qmnntp::NntpDriver::prepareSession(NormalFolder* pFolder)
{
	bool bConnect = !pNntp_.get();
	if (!bConnect)
		bConnect = isForceDisconnect();
	
	if (bConnect) {
		clearSession();
		
		std::auto_ptr<Logger> pLogger;
		std::auto_ptr<CallbackImpl> pCallback;
		std::auto_ptr<Nntp> pNntp;
		
		if (pSubAccount_->isLog(Account::HOST_RECEIVE))
			pLogger = pAccount_->openLogger(Account::HOST_RECEIVE);
		
		pCallback.reset(new CallbackImpl(pSubAccount_, pPasswordCallback_, pSecurity_));
		
		pNntp.reset(new Nntp(pSubAccount_->getTimeout(), pCallback.get(),
			pCallback.get(), pCallback.get(), pLogger.get()));
		if (!pNntp->connect(
			pSubAccount_->getHost(Account::HOST_RECEIVE),
			pSubAccount_->getPort(Account::HOST_RECEIVE),
			pSubAccount_->getSecure(Account::HOST_RECEIVE) == SubAccount::SECURE_SSL))
			return false;
		
		pNntp_ = pNntp;
		pCallback_ = pCallback;
		pLogger_ = pLogger;
	}
	
	assert(pNntp_.get());
	
	wstring_ptr wstrGroup(pFolder->getFullName());
	
	const WCHAR* pwszGroup = pNntp_->getGroup();
	if (!pwszGroup || wcscmp(wstrGroup.get(), pwszGroup) != 0) {
		if (!pNntp_->group(wstrGroup.get()))
			return false;
	}
	
	nLastUsedTime_ = ::GetTickCount();
	
	return true;
}

void qmnntp::NntpDriver::clearSession()
{
	if (pNntp_.get() && !isForceDisconnect())
		pNntp_->disconnect();
	pNntp_.reset(0);
	pCallback_.reset(0);
	pLogger_.reset(0);
}

bool qmnntp::NntpDriver::isForceDisconnect() const
{
	return nForceDisconnect_ != 0 && nLastUsedTime_ + nForceDisconnect_*1000 < ::GetTickCount();
}


/****************************************************************************
 *
 * NntpDriver::CallbackImpl
 *
 */

qmnntp::NntpDriver::CallbackImpl::CallbackImpl(SubAccount* pSubAccount,
											   PasswordCallback* pPasswordCallback,
											   const Security* pSecurity) :
	AbstractCallback(pSubAccount, pPasswordCallback, pSecurity)
{
}

qmnntp::NntpDriver::CallbackImpl::~CallbackImpl()
{
}

bool qmnntp::NntpDriver::CallbackImpl::isCanceled(bool bForce) const
{
	return false;
}

void qmnntp::NntpDriver::CallbackImpl::initialize()
{
}

void qmnntp::NntpDriver::CallbackImpl::lookup()
{
}

void qmnntp::NntpDriver::CallbackImpl::connecting()
{
}

void qmnntp::NntpDriver::CallbackImpl::connected()
{
}

void qmnntp::NntpDriver::CallbackImpl::authenticating()
{
}

void qmnntp::NntpDriver::CallbackImpl::setRange(size_t nMin,
												size_t nMax)
{
}

void qmnntp::NntpDriver::CallbackImpl::setPos(size_t nPos)
{
}


/****************************************************************************
 *
 * NntpFactory
 *
 */

NntpFactory qmnntp::NntpFactory::factory__;

qmnntp::NntpFactory::NntpFactory()
{
	registerFactory(L"nntp", this);
}

qmnntp::NntpFactory::~NntpFactory()
{
	unregisterFactory(L"nntp");
}

std::auto_ptr<ProtocolDriver> qmnntp::NntpFactory::createDriver(Account* pAccount,
																PasswordCallback* pPasswordCallback,
																const Security* pSecurity)
{
	return std::auto_ptr<ProtocolDriver>(new NntpDriver(
		pAccount, pPasswordCallback, pSecurity));
}
