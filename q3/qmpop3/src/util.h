/*
 * $Id: util.h,v 1.1.1.1 2003/04/29 08:07:34 snakamura Exp $
 *
 * Copyright(C) 1998-2003 Satoshi Nakamura
 * All rights reserved.
 *
 */

#ifndef __UTIL_H__
#define __UTIL_H__

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
};

}

#endif // __UTIL_H__
