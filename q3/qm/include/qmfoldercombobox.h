/*
 * $Id$
 *
 * Copyright(C) 1998-2005 Satoshi Nakamura
 * All rights reserved.
 *
 */

#ifndef __QMFOLDERCOMBOBOX_H__
#define __QMFOLDERCOMBOBOX_H__

#include <qm.h>
#include <qmview.h>

#include <qsprofile.h>
#include <qswindow.h>


namespace qm {

class FolderComboBox;

class FolderModel;


/****************************************************************************
 *
 * FolderComboBox
 *
 */

class FolderComboBox :
	public qs::WindowBase,
	public qs::DefaultWindowHandler,
	public View
{
public:
	FolderComboBox(qs::WindowBase* pParentWindow,
				   FolderModel* pFolderModel,
				   qs::Profile* pProfile);
	virtual ~FolderComboBox();

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
#ifdef _WIN32_WCE
	LRESULT onWindowPosChanged(WINDOWPOS* pWindowPos);
#endif
	LRESULT onMessageAdded(WPARAM wParam,
						   LPARAM lParam);
	LRESULT onMessageRemoved(WPARAM wParam,
							 LPARAM lParam);
	LRESULT onMessageRefreshed(WPARAM wParam,
							   LPARAM lParam);
	LRESULT onMessageChanged(WPARAM wParam,
							 LPARAM lParam);

public:
	virtual bool isShow() const;
	virtual bool isActive() const;
	virtual void setActive();

private:
	FolderComboBox(const FolderComboBox&);
	FolderComboBox& operator=(const FolderComboBox&);

private:
	class FolderComboBoxImpl* pImpl_;
};

}

#endif // __QMFOLDERCOMBOBOX_H__
