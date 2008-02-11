/*
 * $Id$
 *
 * Copyright(C) 1998-2008 Satoshi Nakamura
 *
 */

#include <qmaccount.h>
#include <qmdocument.h>
#include <qmmessage.h>
#include <qmsecurity.h>

#include "main.h"
#include "nntpsendsession.h"
#include "resourceinc.h"
#include "ui.h"
#include "util.h"

using namespace qmnntp;
using namespace qm;
using namespace qs;

#define HANDLE_ERROR() \
	do { \
		Util::reportError(pNntp_.get(), pSessionCallback_, pAccount_, \
			pSubAccount_, 0, 0, pCallback_->getErrorMessage()); \
		return false; \
	} while (false) \


/****************************************************************************
 *
 * NntpSendSession
 *
 */

qmnntp::NntpSendSession::NntpSendSession() :
	pAccount_(0),
	pSubAccount_(0),
	pLogger_(0),
	pSessionCallback_(0)
{
}

qmnntp::NntpSendSession::~NntpSendSession()
{
}

bool qmnntp::NntpSendSession::init(Document* pDocument,
								   Account* pAccount,
								   SubAccount* pSubAccount,
								   Profile* pProfile,
								   Logger* pLogger,
								   SendSessionCallback* pCallback)
{
	assert(pAccount);
	assert(pSubAccount);
	assert(pCallback);
	
	pAccount_ = pAccount;
	pSubAccount_ = pSubAccount;
	pLogger_ = pLogger;
	pSessionCallback_ = pCallback;
	
	pCallback_.reset(new CallbackImpl(pSubAccount_,
		pDocument->getSecurity(), pSessionCallback_));
	
	return true;
}

void qmnntp::NntpSendSession::term()
{
}

bool qmnntp::NntpSendSession::connect()
{
	assert(!pNntp_.get());
	
	Log log(pLogger_, L"qmnntp::NntpSendSession");
	log.debug(L"Connecting to the server...");
	
	pNntp_.reset(new Nntp(pSubAccount_->getTimeout(), pCallback_.get(),
		pCallback_.get(), pCallback_.get(), pLogger_));
	
	if (!pNntp_->connect(pSubAccount_->getHost(Account::HOST_SEND),
		pSubAccount_->getPort(Account::HOST_SEND),
		pSubAccount_->getSecure(Account::HOST_SEND) == SubAccount::SECURE_SSL))
		HANDLE_ERROR();
	
	log.debug(L"Connected to the server.");
	
	return true;
}

void qmnntp::NntpSendSession::disconnect()
{
	assert(pNntp_.get());
	
	Log log(pLogger_, L"qmnntp::NntpSendSession");
	log.debug(L"Disconnecting from the server...");
	
	pNntp_->disconnect();
	
	log.debug(L"Disconnected from the server.");
}

bool qmnntp::NntpSendSession::sendMessage(Message* pMessage)
{
	if (!pMessage->removePrivateFields())
		return false;
	
	xstring_size_ptr strContent(pMessage->getContent());
	if (!strContent.get())
		return false;
	
	if (!pNntp_->postMessage(strContent.get(), strContent.size()))
		HANDLE_ERROR();
	
	return true;
}


/****************************************************************************
 *
 * NntpSendSession::CallbackImpl
 *
 */

qmnntp::NntpSendSession::CallbackImpl::CallbackImpl(SubAccount* pSubAccount,
													const Security* pSecurity,
													SendSessionCallback* pSessionCallback) :
	DefaultSSLSocketCallback(pSubAccount, Account::HOST_SEND, pSecurity),
	pSubAccount_(pSubAccount),
	pSessionCallback_(pSessionCallback),
	state_(PASSWORDSTATE_ONETIME)
{
}

qmnntp::NntpSendSession::CallbackImpl::~CallbackImpl()
{
}

void qmnntp::NntpSendSession::CallbackImpl::setMessage(UINT nId)
{
	wstring_ptr wstrMessage(loadString(getResourceHandle(), nId));
	pSessionCallback_->setMessage(wstrMessage.get());
}

bool qmnntp::NntpSendSession::CallbackImpl::isCanceled(bool bForce) const
{
	return pSessionCallback_->isCanceled(bForce);
}

void qmnntp::NntpSendSession::CallbackImpl::initialize()
{
	setMessage(IDS_INITIALIZE);
}

void qmnntp::NntpSendSession::CallbackImpl::lookup()
{
	setMessage(IDS_LOOKUP);
}

void qmnntp::NntpSendSession::CallbackImpl::connecting()
{
	setMessage(IDS_CONNECTING);
}

void qmnntp::NntpSendSession::CallbackImpl::connected()
{
	setMessage(IDS_CONNECTED);
}

bool qmnntp::NntpSendSession::CallbackImpl::getUserInfo(wstring_ptr* pwstrUserName,
														wstring_ptr* pwstrPassword)
{
	state_ = Util::getUserInfo(pSubAccount_, Account::HOST_SEND,
		pSessionCallback_, pwstrUserName, pwstrPassword);
	return state_ != PASSWORDSTATE_NONE;
}

void qmnntp::NntpSendSession::CallbackImpl::setPassword(const WCHAR* pwszPassword)
{
	Util::setPassword(pSubAccount_, Account::HOST_SEND,
		state_, pSessionCallback_, pwszPassword);
}


void qmnntp::NntpSendSession::CallbackImpl::authenticating()
{
	setMessage(IDS_AUTHENTICATING);
}

void qmnntp::NntpSendSession::CallbackImpl::setRange(size_t nMin,
													 size_t nMax)
{
	pSessionCallback_->setSubRange(nMin, nMax);
}

void qmnntp::NntpSendSession::CallbackImpl::setPos(size_t nPos)
{
	pSessionCallback_->setSubPos(nPos);
}


/****************************************************************************
 *
 * NntpSendSessionUI
 *
 */

qmnntp::NntpSendSessionUI::NntpSendSessionUI()
{
}

qmnntp::NntpSendSessionUI::~NntpSendSessionUI()
{
}

const WCHAR* qmnntp::NntpSendSessionUI::getClass()
{
	return L"news";
}

wstring_ptr qmnntp::NntpSendSessionUI::getDisplayName()
{
	return loadString(getResourceHandle(), IDS_NNTP);
}

short qmnntp::NntpSendSessionUI::getDefaultPort(bool bSecure)
{
	return bSecure ? 563 : 119;
}

bool qmnntp::NntpSendSessionUI::isSupported(Support support)
{
	return support != SUPPORT_STARTTLS;
}

std::auto_ptr<PropertyPage> qmnntp::NntpSendSessionUI::createPropertyPage(SubAccount* pSubAccount)
{
	return std::auto_ptr<PropertyPage>(new SendPage(pSubAccount));
}


/****************************************************************************
 *
 * NntpSendSessionFactory
 *
 */

NntpSendSessionFactory qmnntp::NntpSendSessionFactory::factory__;

qmnntp::NntpSendSessionFactory::NntpSendSessionFactory()
{
	registerFactory(L"nntp", this);
}

qmnntp::NntpSendSessionFactory::~NntpSendSessionFactory()
{
	unregisterFactory(L"nntp");
}

std::auto_ptr<SendSession> qmnntp::NntpSendSessionFactory::createSession()
{
	return std::auto_ptr<SendSession>(new NntpSendSession());
}

std::auto_ptr<SendSessionUI> qmnntp::NntpSendSessionFactory::createUI()
{
	return std::auto_ptr<SendSessionUI>(new NntpSendSessionUI());
}
