/*
 * $Id$
 *
 * Copyright(C) 1998-2004 Satoshi Nakamura
 * All rights reserved.
 *
 */

#ifndef __QMMESSAGEHOLDERLIST_H__
#define __QMMESSAGEHOLDERLIST_H__

#include <vector>

namespace qm {

class MessageHolder;

typedef std::vector<MessageHolder*> MessageHolderList;

}

#endif // __QMMESSAGEHOLDERLIST_H__
