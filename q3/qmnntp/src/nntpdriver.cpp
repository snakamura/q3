/*
 * $Id$
 *
 * Copyright(C) 1998-2004 Satoshi Nakamura
 * All rights reserved.
 *
 */

#include <qmaccount.h>
#include <qmfolder.h>
#include <qmmessage.h>
#include <qmmessageholder.h>

#include <qsassert.h>

#include "nntp.h"
#include "nntpdriver.h"

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
	Account::SUPPORT_LOCALFOLDERGETMESSAGE;

qmnntp::NntpDriver::NntpDriver(Account* pAccount,
							   const Security* pSecurity) :
	pAccount_(pAccount),
	pSecurity_(pSecurity),
	pSubAccount_(0),
	pNntp_(0),
	pCallback_(0),
	pLogger_(0),
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
	// TODO
	// This should be removed in the future.
	// Just for compatibility.
	const Account::FolderList& l = pAccount_->getFolders();
	for (Account::FolderList::const_iterator it = l.begin(); it != l.end(); ++it) {
		Folder* pFolder = *it;
		if (pFolder->getType() == Folder::TYPE_NORMAL &&
			!pFolder->isFlag(Folder::FLAG_LOCAL))
			pFolder->setFlags(Folder::FLAG_LOCAL, Folder::FLAG_LOCAL);
	}
	return true;
}

bool qmnntp::NntpDriver::save()
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

std::auto_ptr<NormalFolder> qmnntp::NntpDriver::createFolder(SubAccount* pSubAccount,
															 const WCHAR* pwszName,
															 Folder* pParent)
{
	assert(false);
	return std::auto_ptr<NormalFolder>(0);
}

bool qmnntp::NntpDriver::removeFolder(SubAccount* pSubAccount,
									  NormalFolder* pFolder)
{
	assert(false);
	return false;
}

bool qmnntp::NntpDriver::renameFolder(SubAccount* pSubAccount,
									  NormalFolder* pFolder,
									  const WCHAR* pwszName)
{
	assert(false);
	return false;
}

bool qmnntp::NntpDriver::createDefaultFolders(Account::FolderList* pList)
{
	assert(pList);
	
	struct {
		const WCHAR* pwszName_;
		unsigned int nFlags_;
	} folders[] = {
		{ L"Outbox",	Folder::FLAG_LOCAL | Folder::FLAG_OUTBOX | Folder::FLAG_DRAFTBOX	},
		{ L"Posted",	Folder::FLAG_LOCAL | Folder::FLAG_SENTBOX							},
		{ L"Trash",		Folder::FLAG_LOCAL | Folder::FLAG_TRASHBOX							}
	};
	
	pList->reserve(countof(folders));
	
	for (int n = 0; n < countof(folders); ++n) {
		std::auto_ptr<NormalFolder> pFolder(new NormalFolder(n + 1, folders[n].pwszName_,
			L'/', folders[n].nFlags_, 0, 0, 0, 0, 0, 0, pAccount_));
		pList->push_back(pFolder.get());
		pFolder.release();
	}
	
	return true;
}

bool qmnntp::NntpDriver::getRemoteFolders(SubAccount* pSubAccount,
										  RemoteFolderList* pList)
{
	// TODO
	return true;
}

std::pair<const WCHAR**, size_t> qmnntp::NntpDriver::getFolderParamNames()
{
	return std::pair<const WCHAR**, size_t>(0, 0);
}

bool qmnntp::NntpDriver::getMessage(SubAccount* pSubAccount,
									MessageHolder* pmh,
									unsigned int nFlags,
									xstring_ptr* pstrMessage,
									Message::Flag* pFlag,
									bool* pbMadeSeen)
{
	assert(pSubAccount);
	assert(pmh);
	assert(pstrMessage);
	assert(pFlag);
	assert(pbMadeSeen);
	
	pstrMessage->reset(0);
	*pFlag = Message::FLAG_EMPTY;
	*pbMadeSeen = false;
	
	if (bOffline_)
		return true;
	
	if (!prepareSession(pSubAccount, pmh->getFolder()))
		return false;
	
	xstring_ptr strMessage;
	unsigned int nSize = pmh->getSize();
	if (!pNntp_->getMessage(pmh->getId(),
		Nntp::GETMESSAGEFLAG_ARTICLE, &strMessage, &nSize))
		return false;
	if (!strMessage.get())
		return false;
	
	*pstrMessage = strMessage;
	*pFlag = Message::FLAG_NONE;
	
	return true;
}

bool qmnntp::NntpDriver::setMessagesFlags(SubAccount* pSubAccount,
										  NormalFolder* pFolder,
										  const MessageHolderList& l,
										  unsigned int nFlags,
										  unsigned int nMask)
{
	assert(false);
	return false;
}

bool qmnntp::NntpDriver::appendMessage(SubAccount* pSubAccount,
									   NormalFolder* pFolder,
									   const CHAR* pszMessage,
									   unsigned int nFlags)
{
	assert(false);
	return false;
}

bool qmnntp::NntpDriver::removeMessages(SubAccount* pSubAccount,
										NormalFolder* pFolder,
										const MessageHolderList& l)
{
	assert(false);
	return false;
}

bool qmnntp::NntpDriver::copyMessages(SubAccount* pSubAccount,
									  const MessageHolderList& l,
									  NormalFolder* pFolderFrom,
									  NormalFolder* pFolderTo,
									  bool bMove)
{
	assert(false);
	return false;
}

bool qmnntp::NntpDriver::prepareSession(SubAccount* pSubAccount,
										NormalFolder* pFolder)
{
	assert(pSubAccount);
	
	bool bConnect = pSubAccount_ != pSubAccount;
	if (bConnect)
		nForceDisconnect_ = pSubAccount->getProperty(L"Nntp", L"ForceDisconnect", 0);
	else
		bConnect = isForceDisconnect();
	
	if (bConnect) {
		clearSession();
		
		std::auto_ptr<Logger> pLogger;
		std::auto_ptr<CallbackImpl> pCallback;
		std::auto_ptr<Nntp> pNntp;
		
		if (pSubAccount->isLog(Account::HOST_RECEIVE))
			pLogger = pAccount_->openLogger(Account::HOST_RECEIVE);
		
		pCallback.reset(new CallbackImpl(pSubAccount, pSecurity_));
		
		pNntp.reset(new Nntp(pSubAccount->getTimeout(), pCallback.get(),
			pCallback.get(), pCallback.get(), pLogger.get()));
		if (!pNntp->connect(
			pSubAccount->getHost(Account::HOST_RECEIVE),
			pSubAccount->getPort(Account::HOST_RECEIVE),
			pSubAccount->getSecure(Account::HOST_RECEIVE) == SubAccount::SECURE_SSL))
			return false;
		
		pNntp_ = pNntp;
		pCallback_ = pCallback;
		pLogger_ = pLogger;
		pSubAccount_ = pSubAccount;
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
	pSubAccount_ = 0;
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
											   const Security* pSecurity) :
	AbstractCallback(pSubAccount, pSecurity)
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

void qmnntp::NntpDriver::CallbackImpl::setRange(unsigned int nMin,
												unsigned int nMax)
{
}

void qmnntp::NntpDriver::CallbackImpl::setPos(unsigned int nPos)
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
																const Security* pSecurity)
{
	return std::auto_ptr<ProtocolDriver>(new NntpDriver(pAccount, pSecurity));
}
