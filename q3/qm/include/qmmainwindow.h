/*
 * $Id$
 *
 * Copyright(C) 1998-2004 Satoshi Nakamura
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
	explicit MainWindow(qs::Profile* pProfile);
	virtual ~MainWindow();

public:
	FolderModel* getFolderModel() const;
	const ActionInvoker* getActionInvoker() const;
	bool isShowingModalDialog() const;
	void initialShow();
	bool save();
	
	bool isShowToolbar() const;
	void setShowToolbar(bool bShow);
	bool isShowStatusBar() const;
	void setShowStatusBar(bool bShow);
	bool isShowFolderWindow() const;
	void setShowFolderWindow(bool bShow);
	bool isShowPreviewWindow() const;
	void setShowPreviewWindow(bool bShow);

public:
	virtual void processIdle();

protected:
	virtual bool getToolbarButtons(Toolbar* pToolbar);
	virtual bool createToolbarButtons(void* pCreateParam,
									  HWND hwndToolbar);
#if defined _WIN32_WCE && (_WIN32_WCE < 300 || !defined _WIN32_WCE_PSPC)
	virtual UINT getBarId(int n) const;
	virtual bool getCommandBandsRestoreInfo(int n,
											COMMANDBANDSRESTOREINFO* pcbri) const;
	virtual bool setCommandBandsRestoreInfo(int n,
											const COMMANDBANDSRESTOREINFO& cbri);
#endif
	virtual HMENU getMenuHandle(void* pCreateParam);
	virtual UINT getIconId();

public:
	virtual void getWindowClass(WNDCLASS* pwc);
	virtual bool preCreateWindow(CREATESTRUCT* pCreateStruct);
	virtual qs::Action* getAction(UINT nId);
	virtual qs::Accelerator* getAccelerator();
	virtual LRESULT windowProc(UINT uMsg,
							   WPARAM wParam,
							   LPARAM lParam);

protected:
	LRESULT onActivate(UINT nFlags,
					   HWND hwnd,
					   bool bMinimized);
	LRESULT onClose();
	LRESULT onCopyData(HWND hwnd,
					   COPYDATASTRUCT* pData);
	LRESULT onCreate(CREATESTRUCT* pCreateStruct);
	LRESULT onDestroy();
#ifndef _WIN32_WCE
	LRESULT onEndSession(bool bEnd,
						 int nOption);
#endif
	LRESULT onInitMenuPopup(HMENU hmenu,
							UINT nIndex,
							bool bSysMenu);
#ifndef _WIN32_WCE
	LRESULT onQueryEndSession(int nOption);
#endif
	LRESULT onSize(UINT nFlags,
				   int cx,
				   int cy);
#ifndef _WIN32_WCE_PSPC
	LRESULT onNotifyIcon(WPARAM wParam,
						 LPARAM lParam);
	LRESULT onRecentsChanged(WPARAM wParam,
							 LPARAM lParam);
#endif

private:
	MainWindow(const MainWindow&);
	MainWindow& operator=(const MainWindow&);

private:
	class MainWindowImpl* pImpl_;
};

}

#endif // __QMMAINWINDOW_H__
