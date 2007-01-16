/*
 * $Id$
 *
 * Copyright(C) 1998-2007 Satoshi Nakamura
 * All rights reserved.
 *
 */

#ifndef __JUNK_H__
#define __JUNK_H__

#include <qm.h>

namespace qm {

class JunkFilterUtil;

class JunkFilter;
class MessageHolder;


/****************************************************************************
 *
 * JunkFilterUtil
 *
 */

class JunkFilterUtil
{
public:
	static bool manage(JunkFilter* pJunkFilter,
					   MessageHolder* pmh,
					   unsigned int nOperation);
};

}

#endif // __JUNK_H__
