/*
 * $Id: qmfolderlistwindow.h,v 1.1.1.1 2003/04/29 08:07:31 snakamura Exp $
 *
 * Copyright(C) 1998-2003 Satoshi Nakamura
 * All rights reserved.
 *
 */

#ifndef __QMFOLDERLISTWINDOW_H__
#define __QMFOLDERLISTWINDOW_H__

#include <qm.h>
#include <qmview.h>

#include <qskeymap.h>
#include <qsprofile.h>
#include <qswindow.h>


namespace qm {

class FolderListWindow;

class FolderModel;


/****************************************************************************
 *
 * FolderListWindow
 *
 */

class FolderListWindow :
	public qs::WindowBase,
	public qs::DefaultWindowHandler,
	public View
{
public:
	FolderListWindow(qs::WindowBase* pParentWindow, FolderModel* pFolderModel,
		qs::Profile* pProfile, qs::QSTATUS* pstatus);
	virtual ~FolderListWindow();

public:
	virtual qs::QSTATUS getSuperClass(qs::WSTRING* pwstrSuperClass);
	virtual qs::QSTATUS preCreateWindow(CREATESTRUCT* pCreateStruct);
	virtual qs::QSTATUS getAccelerator(qs::Accelerator** ppAccelerator);
	virtual LRESULT windowProc(UINT uMsg, WPARAM wParam, LPARAM lParam);

protected:
	LRESULT onContextMenu(HWND hwnd, const POINT& pt);
	LRESULT onCreate(CREATESTRUCT* pCreateStruct);
	LRESULT onDestroy();
	LRESULT onKeyDown(UINT nKey, UINT nRepeat, UINT nFlags);
	LRESULT onLButtonDblClk(UINT nFlags, const POINT& pt);
	LRESULT onLButtonDown(UINT nFlags, const POINT& pt);

public:
	virtual bool isShow() const;
	virtual bool isActive() const;
	virtual qs::QSTATUS setActive();

private:
	FolderListWindow(const FolderListWindow&);
	FolderListWindow& operator=(const FolderListWindow&);

private:
	class FolderListWindowImpl* pImpl_;
};

}

#endif // __QMFOLDERLISTWINDOW_H__
