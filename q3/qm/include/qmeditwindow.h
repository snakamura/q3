/*
 * $Id$
 *
 * Copyright(C) 1998-2003 Satoshi Nakamura
 * All rights reserved.
 *
 */

#ifndef __QMEDITWINDOW_H__
#define __QMEDITWINDOW_H__

#include <qm.h>
#include <qmaction.h>

#include <qsprofile.h>
#include <qstextwindow.h>
#include <qswindow.h>


namespace qm {

class EditFrameWindow;
class EditWindow;

class EditFrameWindowManager;
class EditMessage;
class EditMessageHolder;
class EditWindowItem;


/****************************************************************************
 *
 * EditFrameWindow
 *
 */

class QMEXPORTCLASS EditFrameWindow : public qs::FrameWindow
{
public:
	EditFrameWindow(EditFrameWindowManager* pManager,
		qs::Profile* pProfile, qs::QSTATUS* pstatus);
	virtual ~EditFrameWindow();

public:
	EditWindow* getEditWindow() const;
	void close();
	bool tryClose();
	
	bool isShowToolbar() const;
	qs::QSTATUS setShowToolbar(bool bShow);
	bool isShowStatusBar() const;
	qs::QSTATUS setShowStatusBar(bool bShow);
	
	const ActionInvoker* getActionInvoker() const;

protected:
	virtual qs::QSTATUS getToolbarButtons(Toolbar* pToolbar, bool* pbToolbar);
#if defined _WIN32_WCE && (_WIN32_WCE < 300 || !defined _WIN32_WCE_PSPC)
	virtual qs::QSTATUS getBarId(int n, UINT* pnId) const;
	virtual qs::QSTATUS getCommandBandsRestoreInfo(int n,
		COMMANDBANDSRESTOREINFO* pcbri) const;
	virtual qs::QSTATUS setCommandBandsRestoreInfo(int n,
		const COMMANDBANDSRESTOREINFO& cbri);
#endif
	virtual qs::QSTATUS getMenuHandle(void* pCreateParam, HMENU* phmenu);
	virtual qs::QSTATUS getIconId(UINT* pnId);

public:
	virtual qs::QSTATUS getWindowClass(WNDCLASS* pwc);
	virtual qs::QSTATUS preCreateWindow(CREATESTRUCT* pCreateStruct);
	virtual qs::QSTATUS getAction(UINT nId, qs::Action** ppAction);
	virtual qs::QSTATUS getAccelerator(qs::Accelerator** ppAccelerator);
	virtual LRESULT windowProc(UINT uMsg, WPARAM wParam, LPARAM lParam);

protected:
	LRESULT onActivate(UINT nFlags, HWND hwnd, bool bMinimized);
	LRESULT onClose();
	LRESULT onCreate(CREATESTRUCT* pCreateStruct);
	LRESULT onDestroy();
	LRESULT onInitMenuPopup(HMENU hmenu, UINT nIndex, bool bSysMenu);
	LRESULT onSize(UINT nFlags, int cx, int cy);

private:
	EditFrameWindow(const EditFrameWindow&);
	EditFrameWindow& operator=(const EditFrameWindow&);

private:
	class EditFrameWindowImpl* pImpl_;
};


/****************************************************************************
 *
 * EditWindow
 *
 */

class EditWindow :
	public qs::WindowBase,
	public qs::DefaultWindowHandler
{
public:
	EditWindow(qs::Profile* pProfile, qs::QSTATUS* pstatus);
	virtual ~EditWindow();

public:
	EditMessageHolder* getEditMessageHolder() const;
	qs::TextWindow* getTextWindow() const;
	EditWindowItem* getItemByNumber(unsigned int nNumber) const;
	EditWindowItem* getFocusedItem() const;
	void saveFocusedItem();
	void restoreFocusedItem();

public:
	virtual qs::QSTATUS getAccelerator(qs::Accelerator** ppAccelerator);
	virtual LRESULT windowProc(UINT uMsg, WPARAM wParam, LPARAM lParam);

protected:
	LRESULT onCreate(CREATESTRUCT* pCreateStruct);
	LRESULT onDestroy();
	LRESULT onSize(UINT nFlags, int cx, int cy);

private:
	EditWindow(const EditWindow&);
	EditWindow& operator=(const EditWindow&);

private:
	class EditWindowImpl* pImpl_;
};


/****************************************************************************
 *
 * HeaderEditWindow
 *
 */

class HeaderEditWindow :
	public qs::WindowBase,
	public qs::DefaultWindowHandler
{
public:
	HeaderEditWindow(qs::Profile* pProfile, qs::QSTATUS* pstatus);
	virtual ~HeaderEditWindow();

public:
	qs::QSTATUS setEditMessage(EditMessage* pEditMessage, bool bReset);
	void releaseEditMessage(EditMessage* pEditMessage);
	qs::QSTATUS updateEditMessage(EditMessage* pEditMessage);
	int getHeight() const;
	qs::QSTATUS layout();
	EditWindowItem* getFocusedItem() const;
	EditWindowItem* getInitialFocusedItem() const;
	EditWindowItem* getNextFocusItem(EditWindowItem* pItem) const;
	EditWindowItem* getPrevFocusItem(EditWindowItem* pItem) const;
	EditWindowItem* getItemByNumber(unsigned int nNumber) const;

public:
	virtual qs::QSTATUS getWindowClass(WNDCLASS* pwc);
	virtual LRESULT windowProc(UINT uMsg, WPARAM wParam, LPARAM lParam);

protected:
	LRESULT onCreate(CREATESTRUCT* pCreateStruct);
	LRESULT onCtlColorStatic(HDC hdc, HWND hwnd);
	LRESULT onDestroy();
	LRESULT onSize(UINT nFlags, int cx, int cy);

private:
	HeaderEditWindow(const HeaderEditWindow&);
	HeaderEditWindow& operator=(const HeaderEditWindow&);

private:
	class HeaderEditWindowImpl* pImpl_;
};

}

#endif // __QMEDITWINDOW_H__
