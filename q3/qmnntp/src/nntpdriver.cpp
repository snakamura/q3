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

qmnntp::NntpDriver::NntpDriver(Account* pAccount,
							   const Security* pSecurity) :
	pAccount_(pAccount),
	pSecurity_(pSecurity),
	pSubAccount_(0),
	pNntp_(0),
	pCallback_(0),
	pLogger_(0),
	bOffline_(true)
{
}

qmnntp::NntpDriver::~NntpDriver()
{
	clearSession();
}

bool qmnntp::NntpDriver::isSupport(Account::Support support)
{
	switch (support) {
	case Account::SUPPORT_REMOTEFOLDER:
		return true;
	case Account::SUPPORT_LOCALFOLDERDOWNLOAD:
		return false;
	default:
		assert(false);
		return false;
	}
}

void qmnntp::NntpDriver::setOffline(bool bOffline)
{
	if (!bOffline_ && bOffline)
		clearSession();
	bOffline_ = bOffline;
}

bool qmnntp::NntpDriver::save()
{
	return true;
}

std::auto_ptr<NormalFolder> qmnntp::NntpDriver::createFolder(SubAccount* pSubAccount,
															 const WCHAR* pwszName,
															 Folder* pParent)
{
	return new NormalFolder(pAccount_->generateFolderId(), pwszName,
		L'/', Folder::FLAG_SYNCABLE, 0, 0, 0, 0, 0, pParent, pAccount_);
}

bool qmnntp::NntpDriver::removeFolder(SubAccount* pSubAccount,
									  NormalFolder* pFolder)
{
	return true;
}

bool qmnntp::NntpDriver::renameFolder(SubAccount* pSubAccount,
									  NormalFolder* pFolder,
									  const WCHAR* pwszName)
{
	return true;
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
	if (!pNntp_->getMessage(pmh->getId(),
		Nntp::GETMESSAGEFLAG_ARTICLE, &strMessage))
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
	assert(pSubAccount);
	assert(pFolder);
	assert(!l.empty());
	assert(std::find_if(l.begin(), l.end(),
		std::not1(
			std::bind2nd(
				binary_compose_f_gx_hy(
					std::equal_to<Folder*>(),
					std::mem_fun(&MessageHolder::getFolder),
					std::identity<Folder*>()),
				pFolder))) == l.end());
	
	for (MessageHolderList::const_iterator it = l.begin(); it != l.end(); ++it)
		(*it)->setFlags(nFlags, nMask);
	
	return true;
}

bool qmnntp::NntpDriver::appendMessage(SubAccount* pSubAccount,
									   NormalFolder* pFolder,
									   const CHAR* pszMessage,
									   unsigned int nFlags)
{
	assert(pSubAccount);
	assert(pFolder);
	assert(pszMessage);
	
	return false;
}

bool qmnntp::NntpDriver::removeMessages(SubAccount* pSubAccount,
										NormalFolder* pFolder,
										const MessageHolderList& l)
{
	assert(pSubAccount);
	assert(pFolder);
	assert(!l.empty());
	assert(pAccount_->isLocked());
	assert(std::find_if(l.begin(), l.end(),
		std::not1(
			std::bind2nd(
				binary_compose_f_gx_hy(
					std::equal_to<Folder*>(),
					std::mem_fun(&MessageHolder::getFolder),
					std::identity<Folder*>()),
				pFolder))) == l.end());
	
	for (MessageHolderList::const_iterator it = l.begin(); it != l.end(); ++it) {
		if (!pAccount_->unstoreMessage(*it))
			return false;
	}
	
	return true;
}

bool qmnntp::NntpDriver::copyMessages(SubAccount* pSubAccount,
									  const MessageHolderList& l,
									  NormalFolder* pFolderFrom,
									  NormalFolder* pFolderTo,
									  bool bMove)
{
	assert(!l.empty());
	assert(pFolderFrom);
	assert(pFolderTo);
	assert(std::find_if(l.begin(), l.end(),
		std::not1(
			std::bind2nd(
				binary_compose_f_gx_hy(
					std::equal_to<Folder*>(),
					std::mem_fun(&MessageHolder::getFolder),
					std::identity<Folder*>()),
				pFolderFrom))) == l.end());
	
	return false;
}

bool qmnntp::NntpDriver::clearDeletedMessages(SubAccount* pSubAccount,
											  NormalFolder* pFolder)
{
	return true;
}

bool qmnntp::NntpDriver::prepareSession(SubAccount* pSubAccount,
										NormalFolder* pFolder)
{
	assert(pSubAccount);
	
	if (pSubAccount_ != pSubAccount) {
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
			pSubAccount->isSsl(Account::HOST_RECEIVE)))
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
	
	return true;
}

void qmnntp::NntpDriver::clearSession()
{
	if (pNntp_.get()) {
		pNntp_->disconnect();
		pNntp_.reset(0);
		pCallback_.reset(0);
		pLogger_.reset(0);
		pSubAccount_ = 0;
	}
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
	return new NntpDriver(pAccount, pSecurity);
}
