/*
 * $Id$
 *
 * Copyright(C) 1998-2005 Satoshi Nakamura
 * All rights reserved.
 *
 */

#ifndef __QMMESSAGEHOLDERLIST_H__
#define __QMMESSAGEHOLDERLIST_H__

#include <vector>

namespace qm {

class MessageHolder;
class MessagePtr;

typedef std::vector<MessageHolder*> MessageHolderList;
typedef std::vector<MessagePtr> MessagePtrList;

}

#endif // __QMMESSAGEHOLDERLIST_H__
