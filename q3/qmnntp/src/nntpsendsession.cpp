/*
 * $Id$
 *
 * Copyright(C) 1998-2003 Satoshi Nakamura
 * All rights reserved.
 *
 */

#include <qmaccount.h>
#include <qmdocument.h>
#include <qmmessage.h>
#include <qmsecurity.h>

#include <qsnew.h>

#include "main.h"
#include "nntpsendsession.h"
#include "resourceinc.h"
#include "ui.h"
#include "util.h"

using namespace qmnntp;
using namespace qm;
using namespace qs;

#define CHECK_QSTATUS_ERROR() \
	if (status != QSTATUS_SUCCESS) { \
		Util::reportError(pNntp_, pSessionCallback_, pAccount_, pSubAccount_); \
		return status; \
	} \


/****************************************************************************
 *
 * NntpSendSession
 *
 */

qmnntp::NntpSendSession::NntpSendSession(QSTATUS* pstatus) :
	pNntp_(0),
	pCallback_(0),
	pAccount_(0),
	pSubAccount_(0),
	pLogger_(0),
	pSessionCallback_(0)
{
}

qmnntp::NntpSendSession::~NntpSendSession()
{
	delete pNntp_;
	delete pCallback_;
}

QSTATUS qmnntp::NntpSendSession::init(Document* pDocument,
	Account* pAccount, SubAccount* pSubAccount, Profile* pProfile,
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
	
	status = newQsObject(pSubAccount_, pDocument->getSecurity(),
		pSessionCallback_, &pCallback_);
	CHECK_QSTATUS();
	
	return QSTATUS_SUCCESS;
}

QSTATUS qmnntp::NntpSendSession::connect()
{
	assert(!pNntp_);
	
	DECLARE_QSTATUS();
	
	Log log(pLogger_, L"qmnntp::NntpSendSession");
	status = log.debug(L"Connecting to the server...");
	CHECK_QSTATUS();
	
	Nntp::Option option = {
		pSubAccount_->getTimeout(),
		pCallback_,
		pCallback_,
		pCallback_,
		pLogger_
	};
	status = newQsObject(option, &pNntp_);
	CHECK_QSTATUS();
	
	status = pNntp_->connect(pSubAccount_->getHost(Account::HOST_SEND),
		pSubAccount_->getPort(Account::HOST_SEND),
		pSubAccount_->isSsl(Account::HOST_SEND));
	CHECK_QSTATUS_ERROR();
	
	status = log.debug(L"Connected to the server.");
	CHECK_QSTATUS();
	
	return QSTATUS_SUCCESS;
}

QSTATUS qmnntp::NntpSendSession::disconnect()
{
	assert(pNntp_);
	
	DECLARE_QSTATUS();
	
	Log log(pLogger_, L"qmnntp::NntpSendSession");
	status = log.debug(L"Disconnecting from the server...");
	CHECK_QSTATUS();
	
	status = pNntp_->disconnect();
	CHECK_QSTATUS_ERROR();
	
	status = log.debug(L"Disconnected from the server.");
	CHECK_QSTATUS();
	
	return QSTATUS_SUCCESS;
}

QSTATUS qmnntp::NntpSendSession::sendMessage(Message* pMessage)
{
	DECLARE_QSTATUS();
	
	string_ptr<STRING> strContent;
	status = pMessage->getContent(&strContent);
	CHECK_QSTATUS();
	
	status = pNntp_->postMessage(strContent.get(), -1);
	CHECK_QSTATUS_ERROR();
	
	return QSTATUS_SUCCESS;
}


/****************************************************************************
 *
 * NntpSendSession::CallbackImpl
 *
 */

qmnntp::NntpSendSession::CallbackImpl::CallbackImpl(
	SubAccount* pSubAccount, const Security* pSecurity,
	SendSessionCallback* pSessionCallback, QSTATUS* pstatus) :
	pSubAccount_(pSubAccount),
	pSecurity_(pSecurity),
	pSessionCallback_(pSessionCallback)
{
}

qmnntp::NntpSendSession::CallbackImpl::~CallbackImpl()
{
}

QSTATUS qmnntp::NntpSendSession::CallbackImpl::setMessage(UINT nId)
{
	DECLARE_QSTATUS();
	
	string_ptr<WSTRING> wstrMessage;
	status = loadString(getResourceHandle(), nId, &wstrMessage);
	CHECK_QSTATUS();
	
	return pSessionCallback_->setMessage(wstrMessage.get());
}

bool qmnntp::NntpSendSession::CallbackImpl::isCanceled(bool bForce) const
{
	return pSessionCallback_->isCanceled(bForce);
}

QSTATUS qmnntp::NntpSendSession::CallbackImpl::initialize()
{
	return setMessage(IDS_INITIALIZE);
}

QSTATUS qmnntp::NntpSendSession::CallbackImpl::lookup()
{
	return setMessage(IDS_LOOKUP);
}

QSTATUS qmnntp::NntpSendSession::CallbackImpl::connecting()
{
	return setMessage(IDS_CONNECTING);
}

QSTATUS qmnntp::NntpSendSession::CallbackImpl::connected()
{
	return setMessage(IDS_CONNECTED);
}

QSTATUS qmnntp::NntpSendSession::CallbackImpl::getCertStore(const Store** ppStore)
{
	assert(ppStore);
	*ppStore = pSecurity_->getCA();
	return QSTATUS_SUCCESS;
}

QSTATUS qmnntp::NntpSendSession::CallbackImpl::checkCertificate(
	const Certificate& cert, bool bVerified)
{
	DECLARE_QSTATUS();
	
	if (!bVerified && !pSubAccount_->isAllowUnverifiedCertificate())
		return QSTATUS_FAIL;
	
	Name* p = 0;
	status = cert.getSubject(&p);
	CHECK_QSTATUS();
	std::auto_ptr<Name> pName(p);
	
	string_ptr<WSTRING> wstrCommonName;
	status = pName->getCommonName(&wstrCommonName);
	CHECK_QSTATUS();
	
	const WCHAR* pwszHost = pSubAccount_->getHost(Account::HOST_SEND);
	if (_wcsicmp(wstrCommonName.get(), pwszHost) != 0)
		return QSTATUS_FAIL;
	
	return QSTATUS_SUCCESS;
}

QSTATUS qmnntp::NntpSendSession::CallbackImpl::getUserInfo(
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

QSTATUS qmnntp::NntpSendSession::CallbackImpl::setPassword(
	const WCHAR* pwszPassword)
{
	// TODO
	return QSTATUS_SUCCESS;
}


QSTATUS qmnntp::NntpSendSession::CallbackImpl::authenticating()
{
	return setMessage(IDS_AUTHENTICATING);
}

QSTATUS qmnntp::NntpSendSession::CallbackImpl::setRange(
	unsigned int nMin, unsigned int nMax)
{
	return pSessionCallback_->setSubRange(nMin, nMax);
}

QSTATUS qmnntp::NntpSendSession::CallbackImpl::setPos(unsigned int nPos)
{
	return pSessionCallback_->setSubPos(nPos);
}


/****************************************************************************
 *
 * NntpSendSessionUI
 *
 */

qmnntp::NntpSendSessionUI::NntpSendSessionUI(QSTATUS* pstatus)
{
}

qmnntp::NntpSendSessionUI::~NntpSendSessionUI()
{
}

const WCHAR* qmnntp::NntpSendSessionUI::getClass()
{
	return L"news";
}

QSTATUS qmnntp::NntpSendSessionUI::getDisplayName(WSTRING* pwstrName)
{
	assert(pwstrName);
	return loadString(getResourceHandle(), IDS_NNTP, pwstrName);
}

short qmnntp::NntpSendSessionUI::getDefaultPort()
{
	return 119;
}

QSTATUS qmnntp::NntpSendSessionUI::createPropertyPage(
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
 * NntpSendSessionFactory
 *
 */

NntpSendSessionFactory qmnntp::NntpSendSessionFactory::factory__;

qmnntp::NntpSendSessionFactory::NntpSendSessionFactory()
{
	regist(L"nntp", this);
}

qmnntp::NntpSendSessionFactory::~NntpSendSessionFactory()
{
	unregist(L"nntp");
}

QSTATUS qmnntp::NntpSendSessionFactory::createSession(SendSession** ppSendSession)
{
	assert(ppSendSession);
	
	DECLARE_QSTATUS();
	
	NntpSendSession* pSession = 0;
	status = newQsObject(&pSession);
	CHECK_QSTATUS();
	
	*ppSendSession = pSession;
	
	return QSTATUS_SUCCESS;
}

QSTATUS qmnntp::NntpSendSessionFactory::createUI(SendSessionUI** ppUI)
{
	assert(ppUI);
	
	DECLARE_QSTATUS();
	
	NntpSendSessionUI* pUI = 0;
	status = newQsObject(&pUI);
	CHECK_QSTATUS();
	
	*ppUI = pUI;
	
	return QSTATUS_SUCCESS;
}
