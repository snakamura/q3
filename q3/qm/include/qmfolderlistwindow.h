/*
 * $Id$
 *
 * Copyright(C) 1998-2004 Satoshi Nakamura
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

class FolderListModel;
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
	FolderListWindow(qs::WindowBase* pParentWindow,
					 FolderListModel* pFolderListModel,
					 FolderModel* pFolderModel,
					 qs::Profile* pProfile);
	virtual ~FolderListWindow();

public:
	bool save();
	bool isSizeShown() const;
	void showSize();

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
	LRESULT onKeyDown(UINT nKey,
					  UINT nRepeat,
					  UINT nFlags);
	LRESULT onLButtonDblClk(UINT nFlags,
							const POINT& pt);
	LRESULT onLButtonDown(UINT nFlags,
						  const POINT& pt);

public:
	virtual bool isShow() const;
	virtual bool isActive() const;
	virtual void setActive();

private:
	FolderListWindow(const FolderListWindow&);
	FolderListWindow& operator=(const FolderListWindow&);

private:
	class FolderListWindowImpl* pImpl_;
};

}

#endif // __QMFOLDERLISTWINDOW_H__
