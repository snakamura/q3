/*
 * $Id$
 *
 * Copyright(C) 1998-2005 Satoshi Nakamura
 * All rights reserved.
 *
 */

#ifndef __QMVIEW_H__
#define __QMVIEW_H__

#include <qm.h>

#include <qs.h>


namespace qm {

/****************************************************************************
 *
 * View
 *
 */

class View
{
public:
	virtual ~View();

public:
	virtual bool isShow() const = 0;
	virtual bool isActive() const = 0;
	virtual void setActive() = 0;
};

}

#endif // __QMVIEW_H__
