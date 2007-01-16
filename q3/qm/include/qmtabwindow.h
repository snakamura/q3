/*
 * $Id$
 *
 * Copyright(C) 1998-2007 Satoshi Nakamura
 * All rights reserved.
 *
 */

#ifndef __QMTABWINDOW_H__
#define __QMTABWINDOW_H__

#ifdef QMTABWINDOW

#include <qm.h>

#include <qsprofile.h>
#include <qsstring.h>
#include <qswindow.h>


namespace qm {

class TabWindow;

class TabModel;


/****************************************************************************
 *
 * TabWindow
 *
 */

class QMEXPORTCLASS TabWindow :
	public qs::WindowBase,
	public qs::DefaultWindowHandler
{
public:
	TabWindow(TabModel* pTabModel,
			  qs::Profile* pProfile);
	virtual ~TabWindow();

public:
	TabModel* getTabModel() const;
	bool isShowTab() const;
	void setShowTab(bool bShow);
	void reloadProfiles();
	void save() const;
	void setControl(HWND hwnd);

public:
	virtual LRESULT windowProc(UINT uMsg,
							   WPARAM wParam,
							   LPARAM lParam);

protected:
	LRESULT onCreate(CREATESTRUCT* pCreateStruct);
	LRESULT onDestroy();
	LRESULT onSize(UINT nFlags,
				   int cx,
				   int cy);
	LRESULT onMessageAdded(WPARAM wParam,
						   LPARAM lParam);
	LRESULT onMessageRemoved(WPARAM wParam,
							 LPARAM lParam);
	LRESULT onMessageRefreshed(WPARAM wParam,
							   LPARAM lParam);
	LRESULT onMessageChanged(WPARAM wParam,
							 LPARAM lParam);

private:
	TabWindow(const TabWindow&);
	TabWindow& operator=(const TabWindow&);

private:
	class TabWindowImpl* pImpl_;
};

}

#endif // QMTABWINDOW

#endif // __QMTABWINDOW_H__
