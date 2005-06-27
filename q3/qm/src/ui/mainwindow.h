/*
 * $Id$
 *
 * Copyright(C) 1998-2005 Satoshi Nakamura
 * All rights reserved.
 *
 */

#ifndef __MAINWINDOW_H__
#define __MAINWINDOW_H__

#include <qmrecents.h>

#include <qskeymap.h>
#include <qsmenu.h>
#include <qstheme.h>
#include <qstoolbar.h>
#include <qswindow.h>

#include "statusbar.h"
#include "../sync/syncmanager.h"
#include "../uimodel/foldermodel.h"

namespace qm {

class ListContainerWindow;
class SyncNotificationWindow;
class MainWindowStatusBar;
class ShellIcon;
class ShellIconCallback;
struct MainWindowCreateContext;

class AutoPilot;
class Document;
class FilterMenu;
class FolderListWindow;
class GoRound;
class ListWindow;
class PasswordManager;
class Recents;
class SyncDialogManager;
class SyncManager;
class TempFileCleaner;
class UIManager;


/****************************************************************************
 *
 * ListContainerWindow
 *
 */

class ListContainerWindow :
	public qs::WindowBase,
	public qs::DefaultWindowHandler,
	public FolderModelHandler
{
public:
	explicit ListContainerWindow(FolderModel* pFolderModel);
	virtual ~ListContainerWindow();

public:
	void setFolderListWindow(FolderListWindow* pFolderListWindow);
	void setListWindow(ListWindow* pListWindow);

public:
	virtual LRESULT windowProc(UINT uMsg,
							   WPARAM wParam,
							   LPARAM lParam);

protected:
	LRESULT onCreate(CREATESTRUCT* pCreateStruct);
	LRESULT onDestroy();
#ifndef _WIN32_WCE
	LRESULT onNcPaint(HRGN hrgn);
#endif
	LRESULT onSize(UINT nFlags,
				   int cx,
				   int cy);
#ifndef _WIN32_WCE
	LRESULT onThemeChanged();
#endif

public:
	virtual void accountSelected(const FolderModelEvent& event);
	virtual void folderSelected(const FolderModelEvent& event);

private:
	ListContainerWindow(const ListContainerWindow&);
	ListContainerWindow& operator=(const ListContainerWindow&);

private:
	FolderModel* pFolderModel_;
	FolderListWindow* pFolderListWindow_;
	ListWindow* pListWindow_;
	std::auto_ptr<DelayedFolderModelHandler> pDelayedFolderModelHandler_;
#ifndef _WIN32_WCE
	std::auto_ptr<qs::Theme> pTheme_;
#endif
};


/****************************************************************************
 *
 * SyncNotificationWindow
 *
 */

class SyncNotificationWindow :
	public qs::WindowBase,
	public qs::DefaultWindowHandler,
	public SyncManagerHandler
{
public:
	enum {
		WM_SYNCNOTIFICATION_STATUSCHANGED
	};
	
	enum {
		WIDTH	= 16,
		HEIGHT	= 16
	};

public:
	SyncNotificationWindow(SyncManager* pSyncManager,
						   SyncDialogManager* pSyncDialogManager);
	virtual ~SyncNotificationWindow();

public:
	virtual void getWindowClass(WNDCLASS* pwc);
	virtual LRESULT windowProc(UINT uMsg,
							   WPARAM wParam,
							   LPARAM lParam);

protected:
	LRESULT onCreate(CREATESTRUCT* pCreateStruct);
	LRESULT onDestroy();
	LRESULT onLButtonDown(UINT nFlags,
						  const POINT& pt);
	LRESULT onPaint();
	LRESULT onStatusChanged(WPARAM wParam,
							LPARAM lParam);

public:
	virtual void statusChanged(const SyncManagerEvent& event);

private:
	SyncNotificationWindow(const SyncNotificationWindow&);
	SyncNotificationWindow& operator=(const SyncNotificationWindow&);

private:
	SyncManager* pSyncManager_;
	SyncDialogManager* pSyncDialogManager_;
	HBITMAP hbm_;
};


/****************************************************************************
 *
 * MainWindowStatusBar
 *
 */

class MainWindowStatusBar : public MessageStatusBar
{
public:
	MainWindowStatusBar(Document* pDocument,
						ViewModelManager* pViewModelManager,
						FolderModel* pFolderModel,
						SyncManager* pSyncManager,
						MessageWindow* pMessageWindow,
						EncodingModel* pEncodingModel,
						int nOffset,
						EncodingMenu* pEncodingMenu,
						ViewTemplateMenu* pViewTemplateMenu,
						FilterMenu* pFilterMenu);
	virtual ~MainWindowStatusBar();

public:
	void updateListParts(const WCHAR* pwszText);

public:
	virtual LRESULT windowProc(UINT uMsg,
							   WPARAM wParam,
							   LPARAM lParam);

protected:
	LRESULT onLButtonDown(UINT nFlags,
						  const POINT& pt);

protected:
	virtual HMENU getMenu(int nPart);

private:
	virtual Account* getAccount();

private:
	MainWindowStatusBar(const MainWindowStatusBar&);
	MainWindowStatusBar& operator=(const MainWindowStatusBar&);

private:
	enum Offline {
		OFFLINE_NONE,
		OFFLINE_OFFLINE,
		OFFLINE_ONLINE
	};

private:
	Document* pDocument_;
	ViewModelManager* pViewModelManager_;
	FolderModel* pFolderModel_;
	SyncManager* pSyncManager_;
	FilterMenu* pFilterMenu_;
	unsigned int nCount_;
	unsigned int nUnseenCount_;
	unsigned int nSelectedCount_;
	Offline offline_;
	qs::wstring_ptr wstrFilter_;
	qs::wstring_ptr wstrText_;
};


#ifndef _WIN32_WCE_PSPC

/****************************************************************************
 *
 * ShellIcon
 *
 */

class ShellIcon :
	public qs::WindowBase,
	public qs::DefaultWindowHandler,
	public RecentsHandler
{
public:
	enum {
		WM_SHELLICON_NOTIFYICON		= WM_APP + 1001,
		WM_SHELLICON_RECENTSCHANGED	= WM_APP + 1002
	};
	
	enum {
		ID_NOTIFYICON	= 1500
	};
	
	enum {
		HOTKEY_RECENTS	= 0xC000
	};

public:
	ShellIcon(MainWindow* pMainWindow,
			  Recents* pRecents,
			  qs::Profile* pProfile,
			  ShellIconCallback* pCallback);
	virtual ~ShellIcon();

public:
	virtual LRESULT windowProc(UINT uMsg,
							   WPARAM wParam,
							   LPARAM lParam);

#ifndef _WIN32_WCE_PSPC
protected:
	LRESULT onHotKey(UINT nId,
					 UINT nModifier,
					 UINT nKey);
	LRESULT onNotifyIcon(WPARAM wParam,
						 LPARAM lParam);
	LRESULT onRecentsChanged(WPARAM wParam,
							 LPARAM lParam);
#endif

public:
	virtual void recentsChanged(const RecentsEvent& event);

private:
	ShellIcon(const ShellIcon&);
	ShellIcon& operator=(const ShellIcon&);

private:
	Recents* pRecents_;
	qs::Profile* pProfile_;
	ShellIconCallback* pCallback_;
	NOTIFYICONDATA notifyIcon_;
	bool bNotifyIcon_;
};


/****************************************************************************
 *
 * ShellIconCallback
 *
 */

class ShellIconCallback
{
public:
	virtual ~ShellIconCallback();

public:
	virtual void showRecentsMenu() = 0;
};

#endif // _WIN32_WCE_PSPC


/****************************************************************************
 *
 * MainWindowCreateContext
 *
 */

struct MainWindowCreateContext
{
	Document* pDocument_;
	UIManager* pUIManager_;
	PasswordManager* pPasswordManager_;
	SyncManager* pSyncManager_;
	SyncDialogManager* pSyncDialogManager_;
	GoRound* pGoRound_;
	TempFileCleaner* pTempFileCleaner_;
	AutoPilot* pAutoPilot_;
};


}

#endif // __MAINWINDOW_H__
