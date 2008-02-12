/*
 * $Id$
 *
 * Copyright(C) 1998-2008 Satoshi Nakamura
 *
 */

#ifndef __UTIL_H__
#define __UTIL_H__

#include <qmaccount.h>
#include <qmsession.h>

#include <qs.h>


namespace qmpop3 {

class Util;
class DefaultCallback;

class Pop3;


/****************************************************************************
 *
 * Util
 *
 */

class Util
{
public:
	static void reportError(Pop3* pPop3,
							qm::ErrorCallback* pCallback,
							qm::Account* pAccount,
							qm::SubAccount* pSubAccount,
							qm::NormalFolder* pFolder,
							unsigned int nPop3Error,
							const WCHAR* pwszSocketErrorMessage,
							const WCHAR* pwszSSLErrorMessage);
	static Pop3::Secure getSecure(qm::SubAccount* pSubAccount,
								  qm::Account::Host host);
	static Pop3::Secure getSecure(qm::SubAccount::Secure secure);
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
	public qs::DefaultSocketCallback,
	public qm::DefaultSSLSocketCallback,
	public Pop3Callback
{
public:
	DefaultCallback(qm::SubAccount* pSubAccount,
					qm::Account::Host host,
					const qm::Security* pSecurity,
					qm::SessionCallback* pSessionCallback);
	virtual ~DefaultCallback();

public:
	void setMessage(UINT nId);

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
	qm::Account::Host host_;
	qm::SessionCallback* pSessionCallback_;
	qm::PasswordState state_;
};

}

#endif // __UTIL_H__
