/*
 * $Id$
 *
 * Copyright(C) 1998-2004 Satoshi Nakamura
 * All rights reserved.
 *
 */

#ifndef __LISTWINDOW_H__
#define __LISTWINDOW_H__

#include <qmlistwindow.h>

#include <qsdevicecontext.h>

#include "../uimodel/viewmodel.h"


namespace qm {

class PaintInfo;
struct ListWindowCreateContext;

class Document;
class SyncDialogManager;
class SyncManager;
class UIManager;


/****************************************************************************
 *
 * PaintInfo
 *
 */

class PaintInfo
{
public:
	PaintInfo(qs::DeviceContext* pdc,
			  ViewModel* pViewModel,
			  unsigned int nIndex,
			  const RECT& rect);
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
	UIManager* pUIManager_;
	SyncManager* pSyncManager_;
	SyncDialogManager* pSyncDialogManager_;
};

}

#include "listwindow.inl"

#endif // __LISTWINDOW_H__
