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

#include <qsdragdrop.h>
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
	public qs::DefaultWindowHandler,
	public qs::DropTargetHandler
{
public:
	TabCtrlWindow(Document* pDocument,
				  TabModel* pTabModel,
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

public:
	virtual void dragEnter(const qs::DropTargetDragEvent& event);
	virtual void dragOver(const qs::DropTargetDragEvent& event);
	virtual void dragExit(const qs::DropTargetEvent& event);
	virtual void drop(const qs::DropTargetDropEvent& event);

private:
	void processDragEvent(const qs::DropTargetDragEvent& event);

private:
	TabCtrlWindow(const TabCtrlWindow&);
	TabCtrlWindow& operator=(const TabCtrlWindow&);

private:
	enum {
		WM_TABCTRLWINDOW_DESELECTTEMPORARY	= WM_APP + 1601
	};

private:
	Document* pDocument_;
	TabModel* pTabModel_;
	qs::Profile* pProfile_;
	qs::MenuManager* pMenuManager_;
	
	bool bMultiline_;
	
	HFONT hfont_;
	std::auto_ptr<qs::DropTarget> pDropTarget_;
};


/****************************************************************************
 *
 * TabWindowCreateContext
 *
 */

struct TabWindowCreateContext
{
	Document* pDocument_;
	UIManager* pUIManager_;
};

}

#endif // QMTABWINDOW

#endif // __TABWINDOW_H__
