/*
 * $Id: qmview.h,v 1.1.1.1 2003/04/29 08:07:31 snakamura Exp $
 *
 * Copyright(C) 1998-2003 Satoshi Nakamura
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
	virtual qs::QSTATUS setActive() = 0;
};

}

#endif // __QMVIEW_H__
