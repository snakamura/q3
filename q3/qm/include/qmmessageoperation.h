/*
 * $Id$
 *
 * Copyright(C) 1998-2003 Satoshi Nakamura
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
	virtual qs::QSTATUS setCount(unsigned int nCount) = 0;
	virtual qs::QSTATUS step(unsigned int nStep) = 0;
	virtual qs::QSTATUS show() = 0;
};

}

#endif // __QMMESSAGEOPERATION_H__
