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


namespace qmpop3 {

class Util;

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
							qm::SessionCallback* pSessionCallback,
							qm::Account* pAccount,
							qm::SubAccount* pSubAccount,
							qm::NormalFolder* pFolder,
							unsigned int nPop3Error);
	static Pop3::Secure getSecure(qm::SubAccount* pSubAccount);
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

}

#endif // __UTIL_H__
