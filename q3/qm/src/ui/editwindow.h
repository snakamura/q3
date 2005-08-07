/*
 * $Id$
 *
 * Copyright(C) 1998-2005 Satoshi Nakamura
 * All rights reserved.
 *
 */

#ifndef __EDITWINDOW_H__
#define __EDITWINDOW_H__

#include <qs.h>
#include <qsmenu.h>
#include <qsprofile.h>


namespace qm {

class EditWindowItem;
class EditWindowItemWindow;
class EditWindowFocusController;
struct EditWindowCreateContext;
class EditTextWindow;
struct EditTextWindowCreateContext;
class EditTextWindowCallback;

class UIManager;


/****************************************************************************
 *
 * EditWindowItem
 *
 */

class EditWindowItem
{
public:
	virtual ~EditWindowItem();

public:
	virtual void copy() = 0;
	virtual bool canCopy() = 0;
	virtual void cut() = 0;
	virtual bool canCut() = 0;
	virtual void paste() = 0;
	virtual bool canPaste() = 0;
	virtual void selectAll() = 0;
	virtual bool canSelectAll() = 0;
	virtual void undo() = 0;
	virtual bool canUndo() = 0;
	virtual void redo() = 0;
	virtual bool canRedo() = 0;
	virtual void setFocus() = 0;
};


/****************************************************************************
 *
 * EditWindowItemWindow
 *
 */

class EditWindowItemWindow :
	public qs::WindowBase,
	public qs::DefaultWindowHandler
{
public:
	EditWindowItemWindow(EditWindowFocusController* pController,
						 EditWindowItem* pItem,
						 HWND hwnd);
	EditWindowItemWindow(EditWindowFocusController* pController,
						 EditWindowItem* pItem,
						 HWND hwnd,
						 bool bPrevOnly);
	virtual ~EditWindowItemWindow();

public:
	virtual LRESULT windowProc(UINT uMsg,
							   WPARAM wParam,
							   LPARAM lParam);

protected:
	LRESULT onChar(UINT nChar,
				   UINT nRepeat,
				   UINT nFlags);
	LRESULT onKeyDown(UINT nKey,
					  UINT nRepeat,
					  UINT nFlags);

private:
	EditWindowItemWindow(const EditWindowItemWindow&);
	EditWindowItemWindow& operator=(const EditWindowItemWindow&);

private:
	EditWindowFocusController* pController_;
	EditWindowItem* pItem_;
	bool bPrevOnly_;
};


/****************************************************************************
 *
 * EditWindowFocusController
 *
 */

class EditWindowFocusController
{
public:
	enum Focus {
		FOCUS_NEXT,
		FOCUS_PREV
	};

public:
	virtual ~EditWindowFocusController();

public:
	virtual void setFocus(EditWindowItem* pItem,
						  Focus focus) = 0;
};


/****************************************************************************
 *
 * EditWindowCreateContext
 *
 */

struct EditWindowCreateContext
{
	UIManager* pUIManager_;
	const WCHAR* pwszClass_;
};


/****************************************************************************
 *
 * EditTextWindow
 *
 */

class EditTextWindow :
	public qs::TextWindow,
	public qs::TextWindowLinkHandler
{
public:
	explicit EditTextWindow(qs::Profile* pProfile);
	virtual ~EditTextWindow();

public:
	qs::EditableTextModel* getEditableTextModel() const;

public:
	virtual LRESULT windowProc(UINT uMsg,
							   WPARAM wParam,
							   LPARAM lParam);

protected:
	LRESULT onContextMenu(HWND hwnd,
						  const POINT& pt);
	LRESULT onCreate(CREATESTRUCT* pCreateStruct);
	LRESULT onKillFocus(HWND hwnd);
	LRESULT onLButtonDown(UINT nFlags,
						  const POINT& pt);
	LRESULT onSetFocus(HWND hwnd);

public:
	virtual bool openLink(const WCHAR* pwszURL);

private:
	EditTextWindow(const EditTextWindow&);
	EditTextWindow& operator=(const EditTextWindow&);

private:
	qs::Profile* pProfile_;
	qs::MenuManager* pMenuManager_;
	EditTextWindowCallback* pCallback_;
	qs::ImeWindow wndIme_;
};


/****************************************************************************
 *
 * EditTextWindowCreateContext
 *
 */

struct EditTextWindowCreateContext
{
	qs::MenuManager* pMenuManager_;
	EditTextWindowCallback* pCallback_;
};


/****************************************************************************
 *
 * EditTextWindowCallback
 *
 */

class EditTextWindowCallback
{
public:
	virtual ~EditTextWindowCallback();

public:
	virtual void focusChanged() = 0;
};

}

#endif // __EDITWINDOW_H__
