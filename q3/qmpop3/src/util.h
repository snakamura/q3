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
							qm::SubAccount* pSubAccount);
	static Pop3::Ssl getSsl(qm::SubAccount* pSubAccount);
};

}

#endif // __UTIL_H__
