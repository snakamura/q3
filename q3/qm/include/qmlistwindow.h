/*
 * $Id$
 *
 * Copyright(C) 1998-2003 Satoshi Nakamura
 * All rights reserved.
 *
 */

#ifndef __QMLISTWINDOW_H__
#define __QMLISTWINDOW_H__

#include <qm.h>
#include <qmview.h>

#include <qskeymap.h>
#include <qsprofile.h>
#include <qswindow.h>


namespace qm {

class ListWindow;
class ListHeaderColumn;

class ViewColumn;
class ViewModel;
class ViewModelManager;
class FolderSelectionHandler;
class MessageFrameWindowManager;


/****************************************************************************
 *
 * ListWindow
 *
 */

class ListWindow :
	public qs::WindowBase,
	public qs::DefaultWindowHandler,
	public View
{
public:
	enum MoveSelection {
		MOVESELECTION_LINEUP,
		MOVESELECTION_LINEDOWN,
		MOVESELECTION_PAGEUP,
		MOVESELECTION_PAGEDOWN,
		MOVESELECTION_TOP,
		MOVESELECTION_BOTTOM,
		MOVESELECTION_CURRENT
	};

public:
	ListWindow(ViewModelManager* pViewModelManager, qs::Profile* pProfile,
		MessageFrameWindowManager* pMessageFrameWindowManager, qs::QSTATUS* pstatus);
	virtual ~ListWindow();

public:
	qs::QSTATUS refresh();
	qs::QSTATUS moveSelection(MoveSelection m, bool bShift, bool bCtrl);

public:
	HFONT getFont() const;
	bool isShowHeaderColumn() const;
	qs::QSTATUS setShowHeaderColumn(bool bShow);
	qs::QSTATUS save();

public:
	virtual qs::QSTATUS preCreateWindow(CREATESTRUCT* pCreateStruct);
	virtual qs::QSTATUS getAccelerator(qs::Accelerator** ppAccelerator);
	virtual LRESULT windowProc(UINT uMsg, WPARAM wParam, LPARAM lParam);

protected:
	LRESULT onContextMenu(HWND hwnd, const POINT& pt);
	LRESULT onCreate(CREATESTRUCT* pCreateStruct);
	LRESULT onDestroy();
	LRESULT onEraseBkgnd(HDC hdc);
	LRESULT onHScroll(UINT nCode, UINT nPos, HWND hwnd);
	LRESULT onKeyDown(UINT nKey, UINT nRepeat, UINT nFlags);
	LRESULT onKillFocus(HWND hwnd);
	LRESULT onLButtonDblClk(UINT nFlags, const POINT& pt);
	LRESULT onLButtonDown(UINT nFlags, const POINT& pt);
	LRESULT onLButtonUp(UINT nFlags, const POINT& pt);
#if !defined _WIN32_WCE || _WIN32_WCE >= 211
	LRESULT onMouseWheel(UINT nFlags, short nDelta, const POINT& pt);
#endif
	LRESULT onPaint();
	LRESULT onSetFocus(HWND hwnd);
	LRESULT onSize(UINT nFlags, int cx, int cy);
	LRESULT onVScroll(UINT nCode, UINT nPos, HWND hwnd);
	
	LRESULT onViewModelItemAdded(WPARAM wParam, LPARAM lParam);
	LRESULT onViewModelItemRemoved(WPARAM wParam, LPARAM lParam);

public:
	virtual bool isShow() const;
	virtual bool isActive() const;
	virtual qs::QSTATUS setActive();

private:
	ListWindow(const ListWindow&);
	ListWindow& operator=(const ListWindow&);

private:
	struct ListWindowImpl* pImpl_;
};


/****************************************************************************
 *
 * ListHeaderColumn
 *
 */

class ListHeaderColumn :
	public qs::WindowBase,
	public qs::DefaultWindowHandler
{
public:
	ListHeaderColumn(ListWindow* pListWindow,
		qs::Profile* pProfile, qs::QSTATUS* pstatus);
	virtual ~ListHeaderColumn();

public:
	int getWidth() const;
	int getHeight() const;

// These methods are intended to be called from ListWindow class
public:
	qs::QSTATUS setViewModel(ViewModel* pViewModel);
	bool isShow() const;
	qs::QSTATUS setShow(bool bShow);
	qs::QSTATUS save();

public:
	virtual qs::QSTATUS getSuperClass(qs::WSTRING* pwstrSuperClass);
	virtual qs::QSTATUS preCreateWindow(CREATESTRUCT* pCreateStruct);
	virtual LRESULT windowProc(UINT uMsg, WPARAM wParam, LPARAM lParam);

protected:
	LRESULT onCreate(CREATESTRUCT* pCreateStruct);
	LRESULT onDestroy();

private:
	ListHeaderColumn(const ListHeaderColumn&);
	ListHeaderColumn& operator=(const ListHeaderColumn&);

private:
	struct ListHeaderColumnImpl* pImpl_;
};

}

#endif // __QMLISTWINDOW_H__
