/*
 * $Id$
 *
 * Copyright(C) 1998-2004 Satoshi Nakamura
 * All rights reserved.
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
		Util::reportError(pNntp_.get(), pSessionCallback_, pAccount_, pSubAccount_); \
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

bool qmnntp::NntpSendSession::connect()
{
	assert(!pNntp_.get());
	
	Log log(pLogger_, L"qmnntp::NntpSendSession");
	log.debug(L"Connecting to the server...");
	
	pNntp_.reset(new Nntp(pSubAccount_->getTimeout(), pCallback_.get(),
		pCallback_.get(), pCallback_.get(), pLogger_));
	
	if (!pNntp_->connect(pSubAccount_->getHost(Account::HOST_SEND),
		pSubAccount_->getPort(Account::HOST_SEND),
		pSubAccount_->isSsl(Account::HOST_SEND)))
		HANDLE_ERROR();
	
	log.debug(L"Connected to the server.");
	
	return true;
}

bool qmnntp::NntpSendSession::disconnect()
{
	assert(pNntp_.get());
	
	Log log(pLogger_, L"qmnntp::NntpSendSession");
	log.debug(L"Disconnecting from the server...");
	
	pNntp_->disconnect();
	
	log.debug(L"Disconnected from the server.");
	
	return true;
}

bool qmnntp::NntpSendSession::sendMessage(Message* pMessage)
{
	xstring_ptr strContent(pMessage->getContent());
	if (!strContent.get())
		return false;
	
	if (!pNntp_->postMessage(strContent.get(), -1))
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
	pSessionCallback_(pSessionCallback)
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
	assert(pwstrUserName);
	assert(pwstrPassword);
	
	*pwstrUserName = allocWString(pSubAccount_->getUserName(Account::HOST_SEND));
	*pwstrPassword = allocWString(pSubAccount_->getPassword(Account::HOST_SEND));
	
	return true;
}

void qmnntp::NntpSendSession::CallbackImpl::setPassword(const WCHAR* pwszPassword)
{
	// TODO
}


void qmnntp::NntpSendSession::CallbackImpl::authenticating()
{
	setMessage(IDS_AUTHENTICATING);
}

void qmnntp::NntpSendSession::CallbackImpl::setRange(unsigned int nMin,
													 unsigned int nMax)
{
	pSessionCallback_->setSubRange(nMin, nMax);
}

void qmnntp::NntpSendSession::CallbackImpl::setPos(unsigned int nPos)
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

short qmnntp::NntpSendSessionUI::getDefaultPort()
{
	return 119;
}

std::auto_ptr<PropertyPage> qmnntp::NntpSendSessionUI::createPropertyPage(SubAccount* pSubAccount)
{
	return new SendPage(pSubAccount);
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
	return new NntpSendSession();
}

std::auto_ptr<SendSessionUI> qmnntp::NntpSendSessionFactory::createUI()
{
	return new NntpSendSessionUI();
}
