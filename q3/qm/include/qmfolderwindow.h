/*
 * $Id$
 *
 * Copyright(C) 1998-2004 Satoshi Nakamura
 * All rights reserved.
 *
 */

#ifndef __QMFOLDERWINDOW_H__
#define __QMFOLDERWINDOW_H__

#include <qm.h>
#include <qmview.h>

#include <qsprofile.h>
#include <qsstring.h>
#include <qsutil.h>
#include <qswindow.h>


namespace qm {

class FolderWindow;

class FolderModel;


/****************************************************************************
 *
 * FolderWindow
 *
 */

class FolderWindow :
	public qs::WindowBase,
	public qs::DefaultWindowHandler,
	public View
{
public:
	FolderWindow(qs::WindowBase* pParentWindow,
				 FolderModel* pFolderModel,
				 qs::Profile* pProfile);
	virtual ~FolderWindow();

public:
	bool save();
	void expand(bool bExpand);

public:
	virtual qs::wstring_ptr getSuperClass();
	virtual bool preCreateWindow(CREATESTRUCT* pCreateStruct);
	virtual qs::Accelerator* getAccelerator();
	virtual LRESULT windowProc(UINT uMsg,
							   WPARAM wParam,
							   LPARAM lParam);

protected:
	LRESULT onContextMenu(HWND hwnd,
						  const POINT& pt);
	LRESULT onCreate(CREATESTRUCT* pCreateStruct);
	LRESULT onDestroy();
	LRESULT onLButtonDown(UINT nFlags,
						  const POINT& pt);
	LRESULT onMessageAdded(WPARAM wParam,
						   LPARAM lParam);
	LRESULT onMessageRemoved(WPARAM wParam,
							 LPARAM lParam);
	LRESULT onMessageRefreshed(WPARAM wParam,
							   LPARAM lParam);
	LRESULT onMessageChanged(WPARAM wParam,
							 LPARAM lParam);
	LRESULT onDeselectTemporary(WPARAM wParam,
								LPARAM lParam);

public:
	virtual bool isShow() const;
	virtual bool isActive() const;
	virtual void setActive();

private:
	FolderWindow(const FolderWindow&);
	FolderWindow& operator=(const FolderWindow&);

private:
	class FolderWindowImpl* pImpl_;
};

}

#endif // __QFOLDERWINDOW_H__
