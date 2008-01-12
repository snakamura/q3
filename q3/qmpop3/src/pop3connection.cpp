/*
 * $Id$
 *
 * Copyright(C) 1998-2008 Satoshi Nakamura
 *
 */

#include "main.h"
#include "pop3connection.h"
#include "resourceinc.h"
#include "util.h"

using namespace qmpop3;
using namespace qm;
using namespace qs;


/****************************************************************************
 *
 * Pop3Connection
 *
 */

qmpop3::Pop3Connection::Pop3Connection(long nTimeout,
									   SocketCallback* pSocketCallback,
									   SSLSocketCallback* pSSLSocketCallback,
									   ConnectionCallback* pConnectionCallback,
									   Logger* pLogger) :
	bApop_(false)
{
	pCallback_.reset(new CallbackImpl(pConnectionCallback));
	pPop3_.reset(new Pop3(nTimeout, pSocketCallback,
		pSSLSocketCallback, pCallback_.get(), pLogger));
}

qmpop3::Pop3Connection::~Pop3Connection()
{
}

bool qmpop3::Pop3Connection::connect(const WCHAR* pwszHost,
									 short nPort,
									 SubAccount::Secure secure)
{
	if (!pPop3_->connect(pwszHost, nPort, bApop_, Util::getSecure(secure))) {
		Util::reportError(pPop3_.get(), pCallback_->getConnectionCallback(), 0, 0, 0, 0);
		return false;
	}
	return true;
}

void qmpop3::Pop3Connection::disconnect()
{
	pPop3_->disconnect();
}

bool qmpop3::Pop3Connection::setProperty(const WCHAR* pwszName,
										 const WCHAR* pwszValue)
{
	if (wcscmp(pwszName, L"Apop") == 0) {
		bApop_ = pwszValue && wcscmp(pwszValue, L"true") == 0;
		return true;
	}
	return Connection::setProperty(pwszName, pwszValue);
}


/****************************************************************************
 *
 * Pop3Connection::CallbackImpl
 *
 */

qmpop3::Pop3Connection::CallbackImpl::CallbackImpl(ConnectionCallback* pConnectionCallback) :
	pConnectionCallback_(pConnectionCallback)
{
}

qmpop3::Pop3Connection::CallbackImpl::~CallbackImpl()
{
}

ConnectionCallback* qmpop3::Pop3Connection::CallbackImpl::getConnectionCallback() const
{
	return pConnectionCallback_;
}

bool qmpop3::Pop3Connection::CallbackImpl::getUserInfo(wstring_ptr* pwstrUserName,
													   wstring_ptr* pwstrPassword)
{
	return pConnectionCallback_->getUserInfo(pwstrUserName, pwstrPassword);
}

void qmpop3::Pop3Connection::CallbackImpl::setPassword(const WCHAR* pwszPassword)
{
	pConnectionCallback_->setPassword(pwszPassword);
}

void qmpop3::Pop3Connection::CallbackImpl::authenticating()
{
	pConnectionCallback_->authenticating();
}

void qmpop3::Pop3Connection::CallbackImpl::setRange(size_t nMin,
													size_t nMax)
{
}

void qmpop3::Pop3Connection::CallbackImpl::setPos(size_t nPos)
{
}


/****************************************************************************
 *
 * Pop3ConnectionUI
 *
 */

qmpop3::Pop3ConnectionUI::Pop3ConnectionUI()
{
}

qmpop3::Pop3ConnectionUI::~Pop3ConnectionUI()
{
}

wstring_ptr qmpop3::Pop3ConnectionUI::getDisplayName()
{
	return loadString(getResourceHandle(), IDS_POP3);
}

short qmpop3::Pop3ConnectionUI::getDefaultPort(bool bSecure)
{
	return bSecure ? 995 : 110;
}


/****************************************************************************
 *
 * Pop3ConnectionFactory
 *
 */

Pop3ConnectionFactory qmpop3::Pop3ConnectionFactory::factory__;

qmpop3::Pop3ConnectionFactory::Pop3ConnectionFactory()
{
	registerFactory(L"pop3", this);
}

qmpop3::Pop3ConnectionFactory::~Pop3ConnectionFactory()
{
	unregisterFactory(L"pop3");
}

std::auto_ptr<Connection> qmpop3::Pop3ConnectionFactory::createConnection(long nTimeout,
																		  SocketCallback* pSocketCallback,
																		  SSLSocketCallback* pSSLSocketCallback,
																		  ConnectionCallback* pConnectionCallback,
																		  Logger* pLogger)
{
	return std::auto_ptr<Connection>(new Pop3Connection(nTimeout,
		pSocketCallback, pSSLSocketCallback, pConnectionCallback, pLogger));
}

std::auto_ptr<ConnectionUI> qmpop3::Pop3ConnectionFactory::createUI()
{
	return std::auto_ptr<ConnectionUI>(new Pop3ConnectionUI());
}
