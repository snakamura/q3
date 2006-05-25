/*
 * $Id$
 *
 * Copyright(C) 1998-2006 Satoshi Nakamura.
 * All rights reserved.
 *
 */

#ifndef __QMMESSAGEINDEX_H__
#define __QMMESSAGEINDEX_H__

#include <qm.h>


namespace qm {

enum MessageIndexName {
	NAME_FROM,
	NAME_TO,
	NAME_SUBJECT,
	NAME_MESSAGEID,
	NAME_REFERENCE,
	NAME_LABEL,
	
	NAME_MAX
};

}

#endif // __QMMESSAGEINDEX_H__
