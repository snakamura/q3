/*
 * $Id$
 *
 * Copyright(C) 1998-2008 Satoshi Nakamura
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

class AccountManager;
class ActionInvoker;
class SyncDialogManager;
class SyncManager;
class TempFileCleaner;
class UIManager;
class URIResolver;


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
	AccountManager* pAccountManager_;
	const URIResolver* pURIResolver_;
	TempFileCleaner* pTempFileCleaner_;
	UIManager* pUIManager_;
	SyncManager* pSyncManager_;
	SyncDialogManager* pSyncDialogManager_;
	ActionInvoker* pActionInvoker_;
};

}

#include "listwindow.inl"

#endif // __LISTWINDOW_H__
