/*
 * $Id$
 *
 * Copyright(C) 1998-2003 Satoshi Nakamura
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
	static qs::QSTATUS reportError(Nntp* pNntp,
		qm::SessionCallback* pSessionCallback,
		qm::Account* pAccount, qm::SubAccount* pSubAccount);
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
		const qm::Security* pSecurity, qs::QSTATUS* pstatus);
	virtual ~AbstractCallback();

public:
	virtual qs::QSTATUS getUserInfo(qs::WSTRING* pwstrUserName,
		qs::WSTRING* pwstrPassword);
	virtual qs::QSTATUS setPassword(const WCHAR* pwszPassword);

private:
	AbstractCallback(const AbstractCallback&);
	AbstractCallback& operator=(const AbstractCallback&);

private:
	qm::SubAccount* pSubAccount_;
};

}

#endif // __UTIL_H__
