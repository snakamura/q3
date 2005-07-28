/*
 * $Id$
 *
 * Copyright(C) 1998-2005 Satoshi Nakamura
 * All rights reserved.
 *
 */

#ifndef __QMMESSAGEOPERATION_H__
#define __QMMESSAGEOPERATION_H__

#include <qm.h>

#include <qs.h>


namespace qm {

/****************************************************************************
 *
 * MessageOperationCallback
 *
 */

class QMEXPORTCLASS MessageOperationCallback
{
public:
	virtual ~MessageOperationCallback();

public:
	virtual bool isCanceled() = 0;
	virtual void setCount(size_t nCount) = 0;
	virtual void step(size_t nStep) = 0;
	virtual void show() = 0;
};

}

#endif // __QMMESSAGEOPERATION_H__
