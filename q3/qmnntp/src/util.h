/*
 * $Id$
 *
 * Copyright(C) 1998-2005 Satoshi Nakamura
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
							qm::SubAccount* pSubAccount,
							qm::NormalFolder* pFolder,
							unsigned int nNntpError);
	static qm::PasswordState getUserInfo(qm::SubAccount* pSubAccount,
										 qm::Account::Host host,
										 qm::PasswordCallback* pPasswordCallback,
										 qs::wstring_ptr* pwstrUserName,
										 qs::wstring_ptr* pwstrPassword);
	static void setPassword(qm::SubAccount* pSubAccount,
							qm::Account::Host host,
							qm::PasswordState state,
							qm::PasswordCallback* pPasswordCallback,
							const WCHAR* pwszPassword);
};


/****************************************************************************
 *
 * DefaultCallback
 *
 */

class DefaultCallback :
	public qs::SocketCallback,
	public qm::DefaultSSLSocketCallback,
	public NntpCallback
{
public:
	DefaultCallback(qm::SubAccount* pSubAccount,
					qm::PasswordCallback* pPasswordCallback,
					const qm::Security* pSecurity);
	virtual ~DefaultCallback();

public:
	virtual bool isCanceled(bool bForce) const;
	virtual void initialize();
	virtual void lookup();
	virtual void connecting();
	virtual void connected();
	
public:
	virtual bool getUserInfo(qs::wstring_ptr* pwstrUserName,
							 qs::wstring_ptr* pwstrPassword);
	virtual void setPassword(const WCHAR* pwszPassword);
	
	virtual void authenticating();
	virtual void setRange(size_t nMin,
						  size_t nMax);
	virtual void setPos(size_t nPos);

private:
	DefaultCallback(const DefaultCallback&);
	DefaultCallback& operator=(const DefaultCallback&);

private:
	qm::SubAccount* pSubAccount_;
	qm::PasswordCallback* pPasswordCallback_;
	qm::PasswordState state_;
};

}

#endif // __UTIL_H__
