/*
 * $Id$
 *
 * Copyright(C) 1998-2003 Satoshi Nakamura
 * All rights reserved.
 *
 */

#ifndef __QMMAINWINDOW_H__
#define __QMMAINWINDOW_H__

#include <qm.h>
#include <qmaction.h>

#include <qskeymap.h>
#include <qsprofile.h>
#include <qswindow.h>


namespace qm {

/****************************************************************************
 *
 * MainWindow
 *
 */

class QMEXPORTCLASS MainWindow : public qs::FrameWindow
{
public:
	MainWindow(qs::Profile* pProfile, qs::QSTATUS* pstatus);
	virtual ~MainWindow();

public:
	bool isShowToolbar() const;
	qs::QSTATUS setShowToolbar(bool bShow);
	bool isShowStatusBar() const;
	qs::QSTATUS setShowStatusBar(bool bShow);
	bool isShowFolderWindow() const;
	qs::QSTATUS setShowFolderWindow(bool bShow);
	bool isShowPreviewWindow() const;
	qs::QSTATUS setShowPreviewWindow(bool bShow);
	
	const ActionInvoker* getActionInvoker() const;
	
	qs::QSTATUS save() const;

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
	MainWindow(const MainWindow&);
	MainWindow& operator=(const MainWindow&);

private:
	class MainWindowImpl* pImpl_;
};

}

#endif // __QMMAINWINDOW_H__
