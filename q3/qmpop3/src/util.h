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
	static qs::QSTATUS reportError(Pop3* pPop3,
		qm::SessionCallback* pSessionCallback,
		qm::Account* pAccount, qm::SubAccount* pSubAccount);
	static qs::QSTATUS getSsl(qm::SubAccount* pSubAccount, Pop3::Ssl* pSsl);
};

}

#endif // __UTIL_H__
