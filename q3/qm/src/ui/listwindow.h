/*
 * $Id: listwindow.h,v 1.1.1.1 2003/04/29 08:07:32 snakamura Exp $
 *
 * Copyright(C) 1998-2003 Satoshi Nakamura
 * All rights reserved.
 *
 */

#ifndef __LISTWINDOW_H__
#define __LISTWINDOW_H__

#include <qmlistwindow.h>

#include <qsdevicecontext.h>
#include <qskeymap.h>
#include <qsmenu.h>

#include "viewmodel.h"


namespace qm {

/****************************************************************************
 *
 * PaintInfo
 *
 */

class PaintInfo
{
public:
	PaintInfo(qs::DeviceContext* pdc, ViewModel* pViewModel,
		unsigned int nIndex, const RECT& rect);
	~PaintInfo();

public:
	qs::DeviceContext* getDeviceContext() const;
	ViewModel* getViewModel() const;
	unsigned int getIndex() const;
	const ViewModelItem* getItem() const;
	const RECT& getRect() const;

private:
	PaintInfo(const PaintInfo&);
	PaintInfo& operator=(const PaintInfo&);

private:
	qs::DeviceContext* pdc_;
	ViewModel* pViewModel_;
	unsigned int nIndex_;
	const ViewModelItem* pItem_;
	const RECT& rect_;
};


/****************************************************************************
 *
 * ListWindowCreateContext
 *
 */

struct ListWindowCreateContext
{
	Document* pDocument_;
	qs::MenuManager* pMenuManager_;
	qs::KeyMap* pKeyMap_;
};

}

#endif // __LISTWINDOW_H__
