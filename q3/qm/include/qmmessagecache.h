/*
 * $Id$
 *
 * Copyright(C) 1998-2003 Satoshi Nakamura
 * All rights reserved.
 *
 */

#ifndef __QMMESSAGECACHE_H__
#define __QMMESSAGECACHE_H__

#include <qm.h>

namespace qm {

typedef unsigned int MessageCacheKey;

enum MessageCacheItem {
	ITEM_FROM,
	ITEM_TO,
	ITEM_SUBJECT,
	ITEM_MESSAGEID,
	ITEM_REFERENCE,
	ITEM_MAX
};

}

#endif // __QMMESSAGECACHE_H__
