/*
 * $Id$
 *
 * Copyright(C) 1998-2004 Satoshi Nakamura
 * All rights reserved.
 *
 */

#ifndef __UTIL_H__
#define __UTIL_H__

#include <qmaccount.h>
#include <qmsession.h>

#include <qs.h>
#include <qssocket.h>

#include "nntp.h"


namespace qmnntp {

class Util;
class AbstractCallback;


/****************************************************************************
 *
 * Util
 *
 */

class Util
{
public:
	static void reportError(Nntp* pNntp,
							qm::SessionCallback* pSessionCallback,
							qm::Account* pAccount,
							qm::SubAccount* pSubAccount);
	static qm::PasswordCallback::Result getUserInfo(qm::SubAccount* pSubAccount,
													qm::Account::Host host,
													qm::PasswordCallback* pPasswordCallback,
													qs::wstring_ptr* pwstrUserName,
													qs::wstring_ptr* pwstrPassword);
	static void setPassword(qm::SubAccount* pSubAccount,
							qm::Account::Host host,
							qm::PasswordCallback::Result result,
							qm::PasswordCallback* pPasswordCallback,
							const WCHAR* pwszPassword);
};


/****************************************************************************
 *
 * AbstractCallback
 *
 */

class AbstractCallback :
	public qs::SocketCallback,
	public qm::DefaultSSLSocketCallback,
	public NntpCallback
{
public:
	AbstractCallback(qm::SubAccount* pSubAccount,
					 qm::PasswordCallback* pPasswordCallback,
					 const qm::Security* pSecurity);
	virtual ~AbstractCallback();

public:
	virtual bool getUserInfo(qs::wstring_ptr* pwstrUserName,
							 qs::wstring_ptr* pwstrPassword);
	virtual void setPassword(const WCHAR* pwszPassword);

private:
	AbstractCallback(const AbstractCallback&);
	AbstractCallback& operator=(const AbstractCallback&);

private:
	qm::SubAccount* pSubAccount_;
	qm::PasswordCallback* pPasswordCallback_;
	qm::PasswordCallback::Result result_;
};

}

#endif // __UTIL_H__
