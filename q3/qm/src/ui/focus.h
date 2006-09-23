/*
 * $Id$
 *
 * Copyright(C) 1998-2006 Satoshi Nakamura
 * All rights reserved.
 *
 */

#ifndef __FOCUS_H__
#define __FOCUS_H__

#include <qm.h>


namespace qm {

class FocusControllerBase;
template<class Item> class FocusController;


/****************************************************************************
 *
 * FocusControllerBase
 *
 */

class FocusControllerBase
{
public:
	enum Focus {
		FOCUS_NEXT,
		FOCUS_PREV
	};

public:
	virtual ~FocusControllerBase();

public:
	virtual void setFocus(Focus focus) = 0;
	virtual void setFocus(unsigned int nItem) = 0;
};


/****************************************************************************
 *
 * FocusController
 *
 */

template<class Item>
class FocusController : public FocusControllerBase
{
public:
	virtual ~FocusController();

public:
	virtual Item* getFocusedItem() = 0;
};

}

#include "focus.inl"

#endif // __FOCUS_H__
