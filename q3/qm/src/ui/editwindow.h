/*
 * $Id: editwindow.h,v 1.1.1.1 2003/04/29 08:07:32 snakamura Exp $
 *
 * Copyright(C) 1998-2003 Satoshi Nakamura
 * All rights reserved.
 *
 */

#ifndef __EDITWINDOW_H__
#define __EDITWINDOW_H__

#include <qs.h>
#include <qskeymap.h>
#include <qsmenu.h>
#include <qsprofile.h>
#include <qstextwindow.h>


namespace qm {

class EditWindowItem;
class EditWindowItemWindow;
class EditWindowFocusController;
struct EditWindowCreateContext;
class EditTextWindow;
struct EditTextWindowCreateContext;
class EditTextWindowCallback;


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
	virtual qs::QSTATUS copy() = 0;
	virtual qs::QSTATUS canCopy(bool* pbCan) = 0;
	virtual qs::QSTATUS cut() = 0;
	virtual qs::QSTATUS canCut(bool* pbCan) = 0;
	virtual qs::QSTATUS paste() = 0;
	virtual qs::QSTATUS canPaste(bool* pbCan) = 0;
	virtual qs::QSTATUS selectAll() = 0;
	virtual qs::QSTATUS canSelectAll(bool* pbCan) = 0;
	virtual qs::QSTATUS undo() = 0;
	virtual qs::QSTATUS canUndo(bool* pbCan) = 0;
	virtual qs::QSTATUS redo() = 0;
	virtual qs::QSTATUS canRedo(bool* pbCan) = 0;
	virtual qs::QSTATUS setFocus() = 0;
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
		EditWindowItem* pItem, HWND hwnd, qs::QSTATUS* pstatus);
	EditWindowItemWindow(EditWindowFocusController* pController,
		EditWindowItem* pItem, HWND hwnd, bool bPrevOnly, qs::QSTATUS* pstatus);
	virtual ~EditWindowItemWindow();

public:
	virtual LRESULT windowProc(UINT uMsg, WPARAM wParam, LPARAM lParam);

protected:
	LRESULT onChar(UINT nChar, UINT nRepeat, UINT nFlags);

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
	virtual qs::QSTATUS setFocus(EditWindowItem* pItem, Focus focus) = 0;
};


/****************************************************************************
 *
 * EditWindowCreateContext
 *
 */

struct EditWindowCreateContext
{
	qs::MenuManager* pMenuManager_;
	qs::KeyMap* pKeyMap_;
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
	EditTextWindow(qs::Profile* pProfile,
		const WCHAR* pwszSection, qs::QSTATUS* pstatus);
	virtual ~EditTextWindow();

public:
	qs::EditableTextModel* getEditableTextModel() const;

public:
	virtual LRESULT windowProc(UINT uMsg, WPARAM wParam, LPARAM lParam);

protected:
	LRESULT onContextMenu(HWND hwnd, const POINT& pt);
	LRESULT onCreate(CREATESTRUCT* pCreateStruct);
	LRESULT onKillFocus(HWND hwnd);
	LRESULT onLButtonDown(UINT nFlags, const POINT& pt);
	LRESULT onSetFocus(HWND hwnd);

public:
	virtual qs::QSTATUS openLink(const WCHAR* pwszURL);

private:
	EditTextWindow(const EditTextWindow&);
	EditTextWindow& operator=(const EditTextWindow&);

private:
	qs::MenuManager* pMenuManager_;
	EditTextWindowCallback* pCallback_;
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
	virtual qs::QSTATUS layout() = 0;
};

}

#endif // __EDITWINDOW_H__
