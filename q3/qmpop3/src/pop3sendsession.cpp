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

#include "main.h"
#include "pop3sendsession.h"
#include "resourceinc.h"
#include "ui.h"
#include "util.h"

using namespace qmpop3;
using namespace qm;
using namespace qs;


#define HANDLE_ERROR() \
	do { \
		Util::reportError(pPop3_.get(), pSessionCallback_, pAccount_, pSubAccount_); \
		return false; \
	} while (false) \


/****************************************************************************
 *
 * Pop3SendSession
 *
 */

qmpop3::Pop3SendSession::Pop3SendSession() :
	pAccount_(0),
	pSubAccount_(0),
	pLogger_(0),
	pSessionCallback_(0)
{
}

qmpop3::Pop3SendSession::~Pop3SendSession()
{
}

bool qmpop3::Pop3SendSession::init(Document* pDocument,
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

bool qmpop3::Pop3SendSession::connect()
{
	assert(!pPop3_.get());
	
	Log log(pLogger_, L"qmpop3::Pop3SendSession");
	log.debug(L"Connecting to the server...");
	
	pPop3_.reset(new Pop3(pSubAccount_->getTimeout(), pCallback_.get(),
		pCallback_.get(), pCallback_.get(), pLogger_));
	
	bool bApop = pSubAccount_->getProperty(L"Pop3Send", L"Apop", 0) != 0;
	Pop3::Ssl ssl = Util::getSsl(pSubAccount_);
	if (!pPop3_->connect(pSubAccount_->getHost(Account::HOST_SEND),
		pSubAccount_->getPort(Account::HOST_SEND), bApop, ssl))
		HANDLE_ERROR();
	
	log.debug(L"Connected to the server.");
	
	return true;
}

void qmpop3::Pop3SendSession::disconnect()
{
	assert(pPop3_.get());
	
	Log log(pLogger_, L"qmpop3::Pop3SendSession");
	log.debug(L"Disconnecting from the server...");
	
	pPop3_->disconnect();
	
	log.debug(L"Disconnected from the server.");
}

bool qmpop3::Pop3SendSession::sendMessage(Message* pMessage)
{
	xstring_ptr strContent(pMessage->getContent());
	if (!strContent.get())
		return false;
	
	if (!pPop3_->sendMessage(strContent.get(), -1))
		HANDLE_ERROR();
	
	return true;
}


/****************************************************************************
 *
 * Pop3SendSession::CallbackImpl
 *
 */

qmpop3::Pop3SendSession::CallbackImpl::CallbackImpl(SubAccount* pSubAccount,
													const Security* pSecurity,
													SendSessionCallback* pSessionCallback) :
	DefaultSSLSocketCallback(pSubAccount, Account::HOST_SEND, pSecurity),
	pSubAccount_(pSubAccount),
	pSessionCallback_(pSessionCallback)
{
}

qmpop3::Pop3SendSession::CallbackImpl::~CallbackImpl()
{
}

void qmpop3::Pop3SendSession::CallbackImpl::setMessage(UINT nId)
{
	wstring_ptr wstrMessage(loadString(getResourceHandle(), nId));
	pSessionCallback_->setMessage(wstrMessage.get());
}

bool qmpop3::Pop3SendSession::CallbackImpl::isCanceled(bool bForce) const
{
	return pSessionCallback_->isCanceled(bForce);
}

void qmpop3::Pop3SendSession::CallbackImpl::initialize()
{
	setMessage(IDS_INITIALIZE);
}

void qmpop3::Pop3SendSession::CallbackImpl::lookup()
{
	setMessage(IDS_LOOKUP);
}

void qmpop3::Pop3SendSession::CallbackImpl::connecting()
{
	setMessage(IDS_CONNECTING);
}

void qmpop3::Pop3SendSession::CallbackImpl::connected()
{
	setMessage(IDS_CONNECTED);
}

bool qmpop3::Pop3SendSession::CallbackImpl::getUserInfo(wstring_ptr* pwstrUserName,
														wstring_ptr* pwstrPassword)
{
	assert(pwstrUserName);
	assert(pwstrPassword);
	
	*pwstrUserName = allocWString(pSubAccount_->getUserName(Account::HOST_SEND));
	*pwstrPassword = allocWString(pSubAccount_->getPassword(Account::HOST_SEND));
	
	return true;
}

void qmpop3::Pop3SendSession::CallbackImpl::setPassword(const WCHAR* pwszPassword)
{
	// TODO
}


void qmpop3::Pop3SendSession::CallbackImpl::authenticating()
{
	setMessage(IDS_AUTHENTICATING);
}

void qmpop3::Pop3SendSession::CallbackImpl::setRange(unsigned int nMin,
													 unsigned int nMax)
{
	pSessionCallback_->setSubRange(nMin, nMax);
}

void qmpop3::Pop3SendSession::CallbackImpl::setPos(unsigned int nPos)
{
	pSessionCallback_->setSubPos(nPos);
}


/****************************************************************************
 *
 * Pop3SendSessionUI
 *
 */

qmpop3::Pop3SendSessionUI::Pop3SendSessionUI()
{
}

qmpop3::Pop3SendSessionUI::~Pop3SendSessionUI()
{
}

const WCHAR* qmpop3::Pop3SendSessionUI::getClass()
{
	return L"mail";
}

wstring_ptr qmpop3::Pop3SendSessionUI::getDisplayName()
{
	return loadString(getResourceHandle(), IDS_POP3SEND);
}

short qmpop3::Pop3SendSessionUI::getDefaultPort()
{
	return 110;
}

std::auto_ptr<PropertyPage> qmpop3::Pop3SendSessionUI::createPropertyPage(SubAccount* pSubAccount)
{
	return std::auto_ptr<PropertyPage>(new SendPage(pSubAccount));
}


/****************************************************************************
 *
 * Pop3SendSessionFactory
 *
 */

Pop3SendSessionFactory qmpop3::Pop3SendSessionFactory::factory__;

qmpop3::Pop3SendSessionFactory::Pop3SendSessionFactory()
{
	registerFactory(L"pop3", this);
}

qmpop3::Pop3SendSessionFactory::~Pop3SendSessionFactory()
{
	unregisterFactory(L"pop3");
}

std::auto_ptr<SendSession> qmpop3::Pop3SendSessionFactory::createSession()
{
	return std::auto_ptr<SendSession>(new Pop3SendSession());
}

std::auto_ptr<SendSessionUI> qmpop3::Pop3SendSessionFactory::createUI()
{
	return std::auto_ptr<SendSessionUI>(new Pop3SendSessionUI());
}
