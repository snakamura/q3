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

class MainWindow;

class FolderModel;


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
	FolderModel* getFolderModel() const;
	const ActionInvoker* getActionInvoker() const;
	bool isShowingModalDialog() const;
	void initialShow();
	qs::QSTATUS save();
	
	bool isShowToolbar() const;
	qs::QSTATUS setShowToolbar(bool bShow);
	bool isShowStatusBar() const;
	qs::QSTATUS setShowStatusBar(bool bShow);
	bool isShowFolderWindow() const;
	qs::QSTATUS setShowFolderWindow(bool bShow);
	bool isShowPreviewWindow() const;
	qs::QSTATUS setShowPreviewWindow(bool bShow);

protected:
	virtual qs::QSTATUS getToolbarButtons(Toolbar* pToolbar, bool* pbToolbar);
	virtual qs::QSTATUS createToolbarButtons(void* pCreateParam, HWND hwndToolbar);
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
	LRESULT onCopyData(HWND hwnd, COPYDATASTRUCT* pData);
	LRESULT onCreate(CREATESTRUCT* pCreateStruct);
	LRESULT onDestroy();
#ifndef _WIN32_WCE
	LRESULT onEndSession(bool bEnd, int nOption);
#endif
	LRESULT onInitMenuPopup(HMENU hmenu, UINT nIndex, bool bSysMenu);
#ifndef _WIN32_WCE
	LRESULT onQueryEndSession(int nOption);
#endif
	LRESULT onSize(UINT nFlags, int cx, int cy);
	LRESULT onItemAdded(WPARAM wParam, LPARAM lParam);
	LRESULT onItemRemoved(WPARAM wParam, LPARAM lParam);
	LRESULT onItemChanged(WPARAM wParam, LPARAM lParam);

private:
	MainWindow(const MainWindow&);
	MainWindow& operator=(const MainWindow&);

private:
	class MainWindowImpl* pImpl_;
};

}

#endif // __QMMAINWINDOW_H__
