/*
 * $Id$
 *
 * Copyright(C) 1998-2008 Satoshi Nakamura
 *
 */

#ifndef __JUNK_H__
#define __JUNK_H__

#include <qm.h>

namespace qm {

class JunkFilterUtil;

class JunkFilter;
class MessageEnumerator;


/****************************************************************************
 *
 * JunkFilterUtil
 *
 */

class JunkFilterUtil
{
public:
	static bool manageMessageHolder(JunkFilter* pJunkFilter,
									MessageHolder* pmh,
									unsigned int nOperation);
	static bool manageMessageEnumerator(JunkFilter* pJunkFilter,
										MessageEnumerator* pEnum,
										unsigned int nOperation);

private:
	static bool manage(JunkFilter* pJunkFilter,
					   MessageHolder* pmh,
					   MessageEnumerator* pEnum,
					   unsigned int nOperation);
};

}

#endif // __JUNK_H__
