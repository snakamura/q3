/*
 * $Id$
 *
 * Copyright(C) 1998-2004 Satoshi Nakamura
 * All rights reserved.
 *
 */

#ifndef __TABWINDOW_H__
#define __TABWINDOW_H__

#ifdef QMTABWINDOW

#include <qmtabwindow.h>

#include <qsmenu.h>


namespace qm {

class TabCtrlWindow;
struct TabWindowCreateContext;

class TabItem;
class TabModel;
class UIManager;


/****************************************************************************
 *
 * TabCtrlWindow
 *
 */

class TabCtrlWindow :
	public qs::WindowBase,
	public qs::DefaultWindowHandler
{
public:
	TabCtrlWindow(TabModel* pTabModel,
				  qs::Profile* pProfile,
				  qs::MenuManager* pMenuManager);
	virtual ~TabCtrlWindow();

public:
	bool isMultiline() const;

public:
	virtual qs::wstring_ptr getSuperClass();
	virtual bool preCreateWindow(CREATESTRUCT* pCreateStruct);
	virtual LRESULT windowProc(UINT uMsg,
							   WPARAM wParam,
							   LPARAM lParam);

protected:
	LRESULT onContextMenu(HWND hwnd,
						  const POINT& pt);
	LRESULT onCreate(CREATESTRUCT* pCreateStruct);
	LRESULT onDestroy();
#if !defined _WIN32_WCE || _WIN32_WCE >= 400
	LRESULT onMButtonDown(UINT nFlags,
						  const POINT& pt);
	LRESULT onMButtonUp(UINT nFlags,
						const POINT& pt);
#endif
#if !defined _WIN32_WCE || _WIN32_WCE >= 211
	LRESULT onMouseWheel(UINT nFlags,
						 short nDelta,
						 const POINT& pt);
#endif
	LRESULT onDeselectTemporary(WPARAM wParam,
								LPARAM lParam);

private:
	TabCtrlWindow(const TabCtrlWindow&);
	TabCtrlWindow& operator=(const TabCtrlWindow&);

private:
	enum {
		WM_TABCTRLWINDOW_DESELECTTEMPORARY	= WM_APP + 1601
	};

private:
	TabModel* pTabModel_;
	qs::Profile* pProfile_;
	qs::MenuManager* pMenuManager_;
	
	bool bMultiline_;
	
	HFONT hfont_;
};


/****************************************************************************
 *
 * TabWindowCreateContext
 *
 */

struct TabWindowCreateContext
{
	UIManager* pUIManager_;
};

}

#endif // QMTABWINDOW

#endif // __TABWINDOW_H__
