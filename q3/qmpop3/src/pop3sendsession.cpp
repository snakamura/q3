/*
 * $Id$
 *
 * Copyright(C) 1998-2003 Satoshi Nakamura
 * All rights reserved.
 *
 */

#include <qmaccount.h>
#include <qmmessage.h>

#include <qsnew.h>

#include "main.h"
#include "pop3sendsession.h"
#include "resourceinc.h"
#include "ui.h"
#include "util.h"

using namespace qmpop3;
using namespace qm;
using namespace qs;


#define CHECK_QSTATUS_ERROR() \
	if (status != QSTATUS_SUCCESS) { \
		Util::reportError(pPop3_, pSessionCallback_, pAccount_, pSubAccount_); \
		return status; \
	} \


/****************************************************************************
 *
 * Pop3SendSession
 *
 */

qmpop3::Pop3SendSession::Pop3SendSession(QSTATUS* pstatus) :
	pPop3_(0),
	pCallback_(0),
	pAccount_(0),
	pSubAccount_(0),
	pLogger_(0),
	pSessionCallback_(0)
{
}

qmpop3::Pop3SendSession::~Pop3SendSession()
{
	delete pPop3_;
	delete pCallback_;
}

QSTATUS qmpop3::Pop3SendSession::init(Account* pAccount,
	SubAccount* pSubAccount, Profile* pProfile,
	Logger* pLogger, SendSessionCallback* pCallback)
{
	assert(pAccount);
	assert(pSubAccount);
	assert(pCallback);
	
	DECLARE_QSTATUS();
	
	pAccount_ = pAccount;
	pSubAccount_ = pSubAccount;
	pLogger_ = pLogger;
	pSessionCallback_ = pCallback;
	
	status = newQsObject(pSubAccount_, pSessionCallback_, &pCallback_);
	CHECK_QSTATUS();
	
	return QSTATUS_SUCCESS;
}

QSTATUS qmpop3::Pop3SendSession::connect()
{
	assert(!pPop3_);
	
	DECLARE_QSTATUS();
	
	Log log(pLogger_, L"qmpop3::Pop3SendSession");
	status = log.debug(L"Connecting to the server...");
	CHECK_QSTATUS();
	
	Pop3::Option option = {
		pSubAccount_->getTimeout(),
		pCallback_,
		pCallback_,
		pLogger_
	};
	status = newQsObject(option, &pPop3_);
	CHECK_QSTATUS();
	
	int nApop = 0;
	status = pSubAccount_->getProperty(L"Pop3Send", L"Apop", 0, &nApop);
	CHECK_QSTATUS();
	status = pPop3_->connect(pSubAccount_->getHost(Account::HOST_SEND),
		pSubAccount_->getPort(Account::HOST_SEND),
		nApop != 0, pSubAccount_->isSsl(Account::HOST_SEND));
	CHECK_QSTATUS_ERROR();
	
	status = log.debug(L"Connected to the server.");
	CHECK_QSTATUS();
	
	return QSTATUS_SUCCESS;
}

QSTATUS qmpop3::Pop3SendSession::disconnect()
{
	assert(pPop3_);
	
	DECLARE_QSTATUS();
	
	Log log(pLogger_, L"qmpop3::Pop3SendSession");
	status = log.debug(L"Disconnecting from the server...");
	CHECK_QSTATUS();
	
	status = pPop3_->disconnect();
	CHECK_QSTATUS_ERROR();
	
	status = log.debug(L"Disconnected from the server.");
	CHECK_QSTATUS();
	
	return QSTATUS_SUCCESS;
}

QSTATUS qmpop3::Pop3SendSession::sendMessage(Message* pMessage)
{
	DECLARE_QSTATUS();
	
	string_ptr<STRING> strContent;
	status = pMessage->getContent(&strContent);
	CHECK_QSTATUS();
	
	status = pPop3_->sendMessage(strContent.get(), -1);
	CHECK_QSTATUS_ERROR();
	
	return QSTATUS_SUCCESS;
}


/****************************************************************************
 *
 * Pop3SendSession::CallbackImpl
 *
 */

qmpop3::Pop3SendSession::CallbackImpl::CallbackImpl(SubAccount* pSubAccount,
	SendSessionCallback* pSessionCallback, QSTATUS* pstatus) :
	pSubAccount_(pSubAccount),
	pSessionCallback_(pSessionCallback)
{
}

qmpop3::Pop3SendSession::CallbackImpl::~CallbackImpl()
{
}

QSTATUS qmpop3::Pop3SendSession::CallbackImpl::setMessage(UINT nId)
{
	DECLARE_QSTATUS();
	
	string_ptr<WSTRING> wstrMessage;
	status = loadString(getResourceHandle(), nId, &wstrMessage);
	CHECK_QSTATUS();
	
	return pSessionCallback_->setMessage(wstrMessage.get());
}

bool qmpop3::Pop3SendSession::CallbackImpl::isCanceled(bool bForce) const
{
	return pSessionCallback_->isCanceled(bForce);
}

QSTATUS qmpop3::Pop3SendSession::CallbackImpl::initialize()
{
	return setMessage(IDS_INITIALIZE);
}

QSTATUS qmpop3::Pop3SendSession::CallbackImpl::lookup()
{
	return setMessage(IDS_LOOKUP);
}

QSTATUS qmpop3::Pop3SendSession::CallbackImpl::connecting()
{
	return setMessage(IDS_CONNECTING);
}

QSTATUS qmpop3::Pop3SendSession::CallbackImpl::connected()
{
	return setMessage(IDS_CONNECTED);
}

QSTATUS qmpop3::Pop3SendSession::CallbackImpl::getUserInfo(
	WSTRING* pwstrUserName, WSTRING* pwstrPassword)
{
	assert(pwstrUserName);
	assert(pwstrPassword);
	
	DECLARE_QSTATUS();
	
	string_ptr<WSTRING> wstrUserName(
		allocWString(pSubAccount_->getUserName(Account::HOST_SEND)));
	if (!wstrUserName.get())
		return QSTATUS_OUTOFMEMORY;
	string_ptr<WSTRING> wstrPassword(
		allocWString(pSubAccount_->getPassword(Account::HOST_SEND)));
	if (!wstrPassword.get())
		return QSTATUS_OUTOFMEMORY;
	
	*pwstrUserName = wstrUserName.release();
	*pwstrPassword = wstrPassword.release();
	
	return QSTATUS_SUCCESS;
}

QSTATUS qmpop3::Pop3SendSession::CallbackImpl::setPassword(
	const WCHAR* pwszPassword)
{
	// TODO
	return QSTATUS_SUCCESS;
}


QSTATUS qmpop3::Pop3SendSession::CallbackImpl::authenticating()
{
	return setMessage(IDS_AUTHENTICATING);
}

QSTATUS qmpop3::Pop3SendSession::CallbackImpl::setRange(
	unsigned int nMin, unsigned int nMax)
{
	return pSessionCallback_->setSubRange(nMin, nMax);
}

QSTATUS qmpop3::Pop3SendSession::CallbackImpl::setPos(unsigned int nPos)
{
	return pSessionCallback_->setSubPos(nPos);
}


/****************************************************************************
 *
 * Pop3SendSessionUI
 *
 */

qmpop3::Pop3SendSessionUI::Pop3SendSessionUI(QSTATUS* pstatus)
{
}

qmpop3::Pop3SendSessionUI::~Pop3SendSessionUI()
{
}

const WCHAR* qmpop3::Pop3SendSessionUI::getClass()
{
	return L"mail";
}

QSTATUS qmpop3::Pop3SendSessionUI::getDisplayName(WSTRING* pwstrName)
{
	assert(pwstrName);
	return loadString(getResourceHandle(), IDS_POP3SEND, pwstrName);
}

short qmpop3::Pop3SendSessionUI::getDefaultPort()
{
	return 110;
}

QSTATUS qmpop3::Pop3SendSessionUI::createPropertyPage(
	SubAccount* pSubAccount, PropertyPage** ppPage)
{
	assert(ppPage);
	
	DECLARE_QSTATUS();
	
	*ppPage = 0;
	
	std::auto_ptr<SendPage> pPage;
	status = newQsObject(pSubAccount, &pPage);
	CHECK_QSTATUS();
	
	*ppPage = pPage.release();
	
	return QSTATUS_SUCCESS;
}


/****************************************************************************
 *
 * Pop3SendSessionFactory
 *
 */

Pop3SendSessionFactory qmpop3::Pop3SendSessionFactory::factory__;

qmpop3::Pop3SendSessionFactory::Pop3SendSessionFactory()
{
	regist(L"pop3", this);
}

qmpop3::Pop3SendSessionFactory::~Pop3SendSessionFactory()
{
	unregist(L"pop3");
}

QSTATUS qmpop3::Pop3SendSessionFactory::createSession(SendSession** ppSendSession)
{
	assert(ppSendSession);
	
	DECLARE_QSTATUS();
	
	Pop3SendSession* pSession = 0;
	status = newQsObject(&pSession);
	CHECK_QSTATUS();
	
	*ppSendSession = pSession;
	
	return QSTATUS_SUCCESS;
}

QSTATUS qmpop3::Pop3SendSessionFactory::createUI(SendSessionUI** ppUI)
{
	assert(ppUI);
	
	DECLARE_QSTATUS();
	
	Pop3SendSessionUI* pUI = 0;
	status = newQsObject(&pUI);
	CHECK_QSTATUS();
	
	*ppUI = pUI;
	
	return QSTATUS_SUCCESS;
}
