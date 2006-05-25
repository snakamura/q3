/*
 * $Id$
 *
 * Copyright(C) 1998-2006 Satoshi Nakamura
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
	ListWindow(ViewModelManager* pViewModelManager,
			   qs::Profile* pProfile,
			   MessageFrameWindowManager* pMessageFrameWindowManager);
	virtual ~ListWindow();

public:
	void refresh();
	void moveSelection(MoveSelection m,
					   bool bShift,
					   bool bCtrl);

public:
	bool isShowHeaderColumn() const;
	void setShowHeaderColumn(bool bShow);
	void reloadProfiles();
	void save() const;

public:
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
	LRESULT onEraseBkgnd(HDC hdc);
	LRESULT onHScroll(UINT nCode,
					  UINT nPos,
					  HWND hwnd);
	LRESULT onKeyDown(UINT nKey,
					  UINT nRepeat,
					  UINT nFlags);
	LRESULT onKillFocus(HWND hwnd);
	LRESULT onLButtonDblClk(UINT nFlags,
							const POINT& pt);
	LRESULT onLButtonDown(UINT nFlags,
						  const POINT& pt);
	LRESULT onLButtonUp(UINT nFlags,
						const POINT& pt);
#if !defined _WIN32_WCE || _WIN32_WCE >= 211
	LRESULT onMouseWheel(UINT nFlags,
						 short nDelta,
						 const POINT& pt);
#endif
	LRESULT onPaint();
	LRESULT onRButtonDown(UINT nFlags,
						  const POINT& pt);
	LRESULT onRButtonUp(UINT nFlags,
						const POINT& pt);
	LRESULT onSetFocus(HWND hwnd);
	LRESULT onSize(UINT nFlags,
				   int cx,
				   int cy);
	LRESULT onVScroll(UINT nCode,
					  UINT nPos,
					  HWND hwnd);
	LRESULT onViewModelItemAdded(WPARAM wParam,
								 LPARAM lParam);
	LRESULT onViewModelItemRemoved(WPARAM wParam,
								   LPARAM lParam);

public:
	virtual bool isShow() const;
	virtual bool isActive() const;
	virtual void setActive();

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
					 qs::Profile* pProfile);
	virtual ~ListHeaderColumn();

public:
	int getWidth() const;
	int getHeight() const;
	int getPreferredHeight(int nWidth,
						   int nHeight) const;

// These methods are intended to be called from ListWindow class
public:
	void setViewModel(ViewModel* pViewModel);
	bool isShow() const;
	void setShow(bool bShow);
	void save() const;

public:
	virtual qs::wstring_ptr getSuperClass();
	virtual bool preCreateWindow(CREATESTRUCT* pCreateStruct);
	virtual LRESULT windowProc(UINT uMsg,
							   WPARAM wParam,
							   LPARAM lParam);

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
