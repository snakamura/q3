/*
 * $Id$
 *
 * Copyright(C) 1998-2006 Satoshi Nakamura
 * All rights reserved.
 *
 */

#ifndef __QMVIEW_H__
#define __QMVIEW_H__

#include <qm.h>

#include <qs.h>


namespace qm {

class View;

class FocusControllerBase;


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
	virtual FocusControllerBase* getViewFocusController() const;
};

}

#endif // __QMVIEW_H__
