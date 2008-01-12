/*
 * $Id$
 *
 * Copyright(C) 1998-2008 Satoshi Nakamura
 */

#ifndef __QMRESOURCEVERSION_H__
#define __QMRESOURCEVERSION_H__

#include <qm.h>


namespace qm {

/****************************************************************************
 *
 * ResourceVersion
 *
 */

struct ResourceVersion {
	int nRevision_;
	size_t nSize_;
};

}

#endif // __QMRESOURCEVERSION_H__
