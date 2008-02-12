/*
 * $Id$
 *
 * Copyright(C) 1998-2008 Satoshi Nakamura
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


#define HANDLE_ERROR() HANDLE_ERROR_(0)
#define HANDLE_ERROR_SSL() HANDLE_ERROR_(pCallback_->getSSLErrorMessage().get())
#define HANDLE_ERROR_(s) \
	do { \
		Util::reportError(pPop3_.get(), pSessionCallback_, pAccount_, \
			pSubAccount_, 0, 0, pCallback_->getErrorMessage(), s); \
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
	pCallback_.reset(new DefaultCallback(pSubAccount_, Account::HOST_SEND,
		pDocument->getSecurity(), pSessionCallback_));
	
	return true;
}

void qmpop3::Pop3SendSession::term()
{
}

bool qmpop3::Pop3SendSession::connect()
{
	assert(!pPop3_.get());
	
	Log log(pLogger_, L"qmpop3::Pop3SendSession");
	log.debug(L"Connecting to the server...");
	
	pPop3_.reset(new Pop3(pSubAccount_->getTimeout(), pCallback_.get(),
		pCallback_.get(), pCallback_.get(), pLogger_));
	
	bool bApop = pSubAccount_->getPropertyInt(L"Pop3Send", L"Apop") != 0;
	Pop3::Secure secure = Util::getSecure(pSubAccount_, Account::HOST_SEND);
	if (!pPop3_->connect(pSubAccount_->getHost(Account::HOST_SEND),
		pSubAccount_->getPort(Account::HOST_SEND), bApop, secure))
		HANDLE_ERROR_SSL();
	
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
	if (!pMessage->removePrivateFields())
		return false;
	
	xstring_size_ptr strContent(pMessage->getContent());
	if (!strContent.get())
		return false;
	
	if (!pPop3_->sendMessage(strContent.get(), strContent.size()))
		HANDLE_ERROR();
	
	return true;
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

short qmpop3::Pop3SendSessionUI::getDefaultPort(bool bSecure)
{
	return bSecure ? 995 : 110;
}

bool qmpop3::Pop3SendSessionUI::isSupported(Support support)
{
	return true;
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
