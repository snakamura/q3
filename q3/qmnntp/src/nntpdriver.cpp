/*
 * $Id$
 *
 * Copyright(C) 1998-2003 Satoshi Nakamura
 * All rights reserved.
 *
 */

#include <qmaccount.h>
#include <qmfolder.h>
#include <qmmessage.h>
#include <qmmessageholder.h>

#include <qsassert.h>
#include <qserror.h>
#include <qsnew.h>

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
	const Security* pSecurity, QSTATUS* pstatus) :
	pAccount_(pAccount),
	pSecurity_(pSecurity),
	pSubAccount_(0),
	pNntp_(0),
	pCallback_(0),
	pLogger_(0),
	bOffline_(true)
{
	assert(pstatus);
	
	*pstatus = QSTATUS_SUCCESS;
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

QSTATUS qmnntp::NntpDriver::setOffline(bool bOffline)
{
	DECLARE_QSTATUS();
	
	if (!bOffline_ && bOffline) {
		status = clearSession();
		CHECK_QSTATUS();
	}
	bOffline_ = bOffline;
	
	return QSTATUS_SUCCESS;
}

QSTATUS qmnntp::NntpDriver::setForceOnline(bool bOnline)
{
	return QSTATUS_SUCCESS;
}

QSTATUS qmnntp::NntpDriver::save()
{
	return QSTATUS_SUCCESS;
}

QSTATUS qmnntp::NntpDriver::createFolder(SubAccount* pSubAccount,
	const WCHAR* pwszName, Folder* pParent, NormalFolder** ppFolder)
{
	DECLARE_QSTATUS();
	
	NormalFolder::Init init;
	init.nId_ = pAccount_->generateFolderId();
	init.pwszName_ = pwszName;
	init.cSeparator_ = L'/';
	init.nFlags_ = Folder::FLAG_SYNCABLE;
	init.nCount_ = 0;
	init.nUnseenCount_ = 0;
	init.pParentFolder_ = pParent;
	init.pAccount_ = pAccount_;
	status = newQsObject(init, ppFolder);
	CHECK_QSTATUS();
	
	return QSTATUS_SUCCESS;
}

QSTATUS qmnntp::NntpDriver::createDefaultFolders(
	Folder*** pppFolder, size_t* pnCount)
{
	// TODO
	return QSTATUS_SUCCESS;
}

QSTATUS qmnntp::NntpDriver::getRemoteFolders(SubAccount* pSubAccount,
	std::pair<Folder*, bool>** ppFolder, size_t* pnCount)
{
	// TODO
	return QSTATUS_SUCCESS;
}

QSTATUS qmnntp::NntpDriver::getMessage(SubAccount* pSubAccount,
	MessageHolder* pmh, unsigned int nFlags, Message* pMessage,
	bool* pbGet, bool* pbMadeSeen)
{
	assert(pSubAccount);
	assert(pmh);
	assert(pMessage);
	assert(pbGet);
	assert(pbMadeSeen);
	
	DECLARE_QSTATUS();
	
	*pbGet = false;
	*pbMadeSeen = false;
	
	if (!bOffline_) {
		status = prepareSession(pSubAccount, pmh->getFolder());
		CHECK_QSTATUS();
		
		string_ptr<STRING> strMessage;
		status = pNntp_->getMessage(pmh->getId(),
			Nntp::GETMESSAGEFLAG_ARTICLE, &strMessage);
		CHECK_QSTATUS();
		if (strMessage.get()) {
			status = pMessage->create(strMessage.get(), -1, Message::FLAG_NONE);
			CHECK_QSTATUS();
			*pbGet = true;
		}
	}
	
	return QSTATUS_SUCCESS;
}

QSTATUS qmnntp::NntpDriver::setMessagesFlags(SubAccount* pSubAccount,
	NormalFolder* pFolder, const Folder::MessageHolderList& l,
	unsigned int nFlags, unsigned int nMask)
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
	
	Folder::MessageHolderList::const_iterator it = l.begin();
	while (it != l.end()) {
		(*it)->setFlags(nFlags, nMask);
		++it;
	}
	
	return QSTATUS_SUCCESS;
}

QSTATUS qmnntp::NntpDriver::appendMessage(SubAccount* pSubAccount,
	NormalFolder* pFolder, const CHAR* pszMessage, unsigned int nFlags)
{
	assert(pSubAccount);
	assert(pFolder);
	assert(pszMessage);
	
	return QSTATUS_FAIL;
}

QSTATUS qmnntp::NntpDriver::removeMessages(SubAccount* pSubAccount,
	NormalFolder* pFolder, const Folder::MessageHolderList& l)
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
	
	return pFolder->deleteMessages(l, 0);
}

QSTATUS qmnntp::NntpDriver::copyMessages(SubAccount* pSubAccount,
	const Folder::MessageHolderList& l, NormalFolder* pFolderFrom,
	NormalFolder* pFolderTo, bool bMove)
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
	
	return QSTATUS_FAIL;
}

QSTATUS qmnntp::NntpDriver::clearDeletedMessages(
	SubAccount* pSubAccount, NormalFolder* pFolder)
{
	return QSTATUS_SUCCESS;
}

QSTATUS qmnntp::NntpDriver::prepareSession(
	SubAccount* pSubAccount, NormalFolder* pFolder)
{
	assert(pSubAccount);
	
	DECLARE_QSTATUS();
	
	if (pSubAccount_ != pSubAccount) {
		clearSession();
		
		std::auto_ptr<Logger> pLogger;
		std::auto_ptr<CallbackImpl> pCallback;
		std::auto_ptr<Nntp> pNntp;
		
		if (pSubAccount->isLog(Account::HOST_RECEIVE)) {
			Logger* p = 0;
			status = pAccount_->openLogger(Account::HOST_RECEIVE, &p);
			CHECK_QSTATUS();
			pLogger.reset(p);
		}
		
		status = newQsObject(pSubAccount, pSecurity_, &pCallback);
		CHECK_QSTATUS();
		
		Nntp::Option option = {
			pSubAccount->getTimeout(),
			pCallback.get(),
			pCallback.get(),
			pCallback.get(),
			pLogger.get()
		};
		status = newQsObject(option, &pNntp);
		CHECK_QSTATUS();
		status = pNntp->connect(
			pSubAccount->getHost(Account::HOST_RECEIVE),
			pSubAccount->getPort(Account::HOST_RECEIVE),
			pSubAccount->isSsl(Account::HOST_RECEIVE));
		CHECK_QSTATUS();
		
		pNntp_ = pNntp.release();
		pCallback_ = pCallback.release();
		pLogger_ = pLogger.release();
		pSubAccount_ = pSubAccount;
	}
	
	assert(pNntp_);
	
	string_ptr<WSTRING> wstrGroup;
	status = pFolder->getFullName(&wstrGroup);
	CHECK_QSTATUS();
	
	const WCHAR* pwszGroup = pNntp_->getGroup();
	if (!pwszGroup || wcscmp(wstrGroup.get(), pwszGroup) != 0) {
		status = pNntp_->group(wstrGroup.get());
		CHECK_QSTATUS();
	}
	
	return QSTATUS_SUCCESS;
}

QSTATUS qmnntp::NntpDriver::clearSession()
{
	if (pNntp_) {
		pNntp_->disconnect();
		delete pNntp_;
		pNntp_ = 0;
		delete pCallback_;
		pCallback_ = 0;
		delete pLogger_;
		pLogger_ = 0;
		pSubAccount_ = 0;
	}
	return QSTATUS_SUCCESS;
}


/****************************************************************************
 *
 * NntpDriver::CallbackImpl
 *
 */

qmnntp::NntpDriver::CallbackImpl::CallbackImpl(SubAccount* pSubAccount,
	const Security* pSecurity, QSTATUS* pstatus) :
	AbstractCallback(pSubAccount, pSecurity, pstatus)
{
}

qmnntp::NntpDriver::CallbackImpl::~CallbackImpl()
{
}

bool qmnntp::NntpDriver::CallbackImpl::isCanceled(bool bForce) const
{
	return false;
}

QSTATUS qmnntp::NntpDriver::CallbackImpl::initialize()
{
	return QSTATUS_SUCCESS;
}

QSTATUS qmnntp::NntpDriver::CallbackImpl::lookup()
{
	return QSTATUS_SUCCESS;
}

QSTATUS qmnntp::NntpDriver::CallbackImpl::connecting()
{
	return QSTATUS_SUCCESS;
}

QSTATUS qmnntp::NntpDriver::CallbackImpl::connected()
{
	return QSTATUS_SUCCESS;
}

QSTATUS qmnntp::NntpDriver::CallbackImpl::authenticating()
{
	return QSTATUS_SUCCESS;
}

QSTATUS qmnntp::NntpDriver::CallbackImpl::setRange(
	unsigned int nMin, unsigned int nMax)
{
	return QSTATUS_SUCCESS;
}

QSTATUS qmnntp::NntpDriver::CallbackImpl::setPos(unsigned int nPos)
{
	return QSTATUS_SUCCESS;
}


/****************************************************************************
 *
 * NntpFactory
 *
 */

NntpFactory qmnntp::NntpFactory::factory__;

qmnntp::NntpFactory::NntpFactory()
{
	regist(L"nntp", this);
}

qmnntp::NntpFactory::~NntpFactory()
{
	unregist(L"nntp");
}

QSTATUS qmnntp::NntpFactory::createDriver(Account* pAccount,
	const Security* pSecurity, ProtocolDriver** ppProtocolDriver)
{
	assert(pAccount);
	assert(ppProtocolDriver);
	
	DECLARE_QSTATUS();
	
	NntpDriver* pDriver = 0;
	status = newQsObject(pAccount, pSecurity, &pDriver);
	CHECK_QSTATUS();
	
	*ppProtocolDriver = pDriver;
	
	return QSTATUS_SUCCESS;
}
