/*
 * $Id$
 *
 * Copyright(C) 1998-2006 Satoshi Nakamura
 * All rights reserved.
 *
 */

#include "imap4connection.h"
#include "main.h"
#include "resourceinc.h"
#include "util.h"

using namespace qmimap4;
using namespace qm;
using namespace qs;


/****************************************************************************
 *
 * Imap4Connection
 *
 */

qmimap4::Imap4Connection::Imap4Connection(long nTimeout,
										  SocketCallback* pSocketCallback,
										  SSLSocketCallback* pSSLSocketCallback,
										  ConnectionCallback* pConnectionCallback,
										  Logger* pLogger)
{
	pCallback_.reset(new CallbackImpl(pConnectionCallback));
	pImap4_.reset(new Imap4(nTimeout, pSocketCallback,
		pSSLSocketCallback, pCallback_.get(), pLogger));
}

qmimap4::Imap4Connection::~Imap4Connection()
{
}

bool qmimap4::Imap4Connection::connect(const WCHAR* pwszHost,
									   short nPort,
									   SubAccount::Secure secure)
{
	if (!pImap4_->connect(pwszHost, nPort, Util::getSecure(secure))) {
		Util::reportError(pImap4_.get(), pCallback_->getConnectionCallback(), 0, 0, 0, 0);
		return false;
	}
	return true;
}

void qmimap4::Imap4Connection::disconnect()
{
	pImap4_->disconnect();
}


/****************************************************************************
 *
 * Imap4Connection::CallbackImpl
 *
 */

qmimap4::Imap4Connection::CallbackImpl::CallbackImpl(ConnectionCallback* pConnectionCallback) :
	pConnectionCallback_(pConnectionCallback)
{
}

qmimap4::Imap4Connection::CallbackImpl::~CallbackImpl()
{
}

ConnectionCallback* qmimap4::Imap4Connection::CallbackImpl::getConnectionCallback() const
{
	return pConnectionCallback_;
}

bool qmimap4::Imap4Connection::CallbackImpl::getUserInfo(wstring_ptr* pwstrUserName,
													   wstring_ptr* pwstrPassword)
{
	return pConnectionCallback_->getUserInfo(pwstrUserName, pwstrPassword);
}

void qmimap4::Imap4Connection::CallbackImpl::setPassword(const WCHAR* pwszPassword)
{
	pConnectionCallback_->setPassword(pwszPassword);
}

wstring_ptr qmimap4::Imap4Connection::CallbackImpl::getAuthMethods()
{
	return 0;
}

void qmimap4::Imap4Connection::CallbackImpl::authenticating()
{
	pConnectionCallback_->authenticating();
}

void qmimap4::Imap4Connection::CallbackImpl::setRange(size_t nMin,
													size_t nMax)
{
}

void qmimap4::Imap4Connection::CallbackImpl::setPos(size_t nPos)
{
}

bool qmimap4::Imap4Connection::CallbackImpl::response(Response* pResponse)
{
	return true;
}


/****************************************************************************
 *
 * Imap4ConnectionUI
 *
 */

qmimap4::Imap4ConnectionUI::Imap4ConnectionUI()
{
}

qmimap4::Imap4ConnectionUI::~Imap4ConnectionUI()
{
}

wstring_ptr qmimap4::Imap4ConnectionUI::getDisplayName()
{
	return loadString(getResourceHandle(), IDS_IMAP4);
}

short qmimap4::Imap4ConnectionUI::getDefaultPort(bool bSecure)
{
	return bSecure ? 993 : 143;
}


/****************************************************************************
 *
 * Imap4ConnectionFactory
 *
 */

Imap4ConnectionFactory qmimap4::Imap4ConnectionFactory::factory__;

qmimap4::Imap4ConnectionFactory::Imap4ConnectionFactory()
{
	registerFactory(L"imap4", this);
}

qmimap4::Imap4ConnectionFactory::~Imap4ConnectionFactory()
{
	unregisterFactory(L"imap4");
}

std::auto_ptr<Connection> qmimap4::Imap4ConnectionFactory::createConnection(long nTimeout,
																			SocketCallback* pSocketCallback,
																			SSLSocketCallback* pSSLSocketCallback,
																			ConnectionCallback* pConnectionCallback,
																			Logger* pLogger)
{
	return std::auto_ptr<Connection>(new Imap4Connection(nTimeout,
		pSocketCallback, pSSLSocketCallback, pConnectionCallback, pLogger));
}

std::auto_ptr<ConnectionUI> qmimap4::Imap4ConnectionFactory::createUI()
{
	return std::auto_ptr<ConnectionUI>(new Imap4ConnectionUI());
}
