/*
 * $Id$
 *
 * Copyright(C) 1998-2008 Satoshi Nakamura
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
class FolderListWindow;
class GoRound;
class ListWindow;
class PasswordManager;
class Recents;
class SyncDialogManager;
class SyncManager;
class SyncQueue;
class TempFileCleaner;
class UIManager;
class UpdateChecker;


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
	LRESULT onLButtonUp(UINT nFlags,
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
						qs::MenuManager* pMenuManager);
	virtual ~MainWindowStatusBar();

public:
	void updateListParts(const WCHAR* pwszText);

public:
	virtual LRESULT windowProc(UINT uMsg,
							   WPARAM wParam,
							   LPARAM lParam);

protected:
	LRESULT onLButtonUp(UINT nFlags,
						const POINT& pt);

protected:
	virtual const WCHAR* getMenuName(int nPart);

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
	private RecentsHandler,
	private SyncManagerHandler
{
public:
	enum {
		WM_SHELLICON_NOTIFYICON			= WM_APP + 1001,
		WM_SHELLICON_RECENTSCHANGED		= WM_APP + 1002,
		WM_SHELLICON_SYNCSTATUSCHANGED	= WM_APP + 1003
	};
	
	enum {
		ID_NOTIFYICON	= 1500
	};
	
	enum {
		HOTKEY_RECENTS	= 0xC000
	};

public:
	ShellIcon(Recents* pRecents,
			  SyncManager* pSyncManager,
			  qs::Profile* pProfile,
			  HWND hwnd,
			  ShellIconCallback* pCallback);
	virtual ~ShellIcon();

public:
	void showHiddenIcon();
	void hideHiddenIcon();

public:
	virtual LRESULT windowProc(UINT uMsg,
							   WPARAM wParam,
							   LPARAM lParam);

protected:
	LRESULT onHotKey(UINT nId,
					 UINT nModifier,
					 UINT nKey);
	LRESULT onTimer(UINT_PTR nId);
	LRESULT onNotifyIcon(WPARAM wParam,
						 LPARAM lParam);
	LRESULT onRecentsChanged(WPARAM wParam,
							 LPARAM lParam);
	LRESULT onSyncStatusChanged(WPARAM wParam,
								LPARAM lParam);

public:
	virtual void recentsChanged(const RecentsEvent& event);

public:
	virtual void statusChanged(const SyncManagerEvent& event);

private:
	void updateIcon();

private:
	ShellIcon(const ShellIcon&);
	ShellIcon& operator=(const ShellIcon&);

private:
	enum State {
		STATE_NONE		= 0x00,
		STATE_HIDDEN	= 0x01,
		STATE_RECENT	= 0x02
	};
	enum {
		TIMER_ID		= 1001,
		TIMER_INTERVAL	= 100
	};

private:
	Recents* pRecents_;
	SyncManager* pSyncManager_;
	qs::Profile* pProfile_;
	ShellIconCallback* pCallback_;
	HIMAGELIST hImageList_;
	NOTIFYICONDATA notifyIcon_;
	unsigned int nState_;
	unsigned int nSync_;
	unsigned int nIconIndex_;
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
	virtual void showRecentsMenu(bool bHotKey) = 0;
	virtual void show() = 0;
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
	SyncQueue* pSyncQueue_;
	GoRound* pGoRound_;
	TempFileCleaner* pTempFileCleaner_;
	AutoPilot* pAutoPilot_;
	UpdateChecker* pUpdateChecker_;
	const FolderImage* pFolderImage_;
};


/****************************************************************************
 *
 * SplitterHelper
 *
 */

class SplitterHelper
{
public:
	enum Type {
		TYPE_VERTICAL,
		TYPE_HORIZONTAL,
		
		MAX_TYPE
	};
	
	enum Placement {
		PLACEMENT_PRIMARY		= 0x01,
		PLACEMENT_SECONDARY		= 0x02,
		
		PLACEMENT_0				= 0x10,
		PLACEMENT_1				= 0x20,
		
		PLACEMENT_PRIMARY0		= PLACEMENT_PRIMARY | PLACEMENT_0,
		PLACEMENT_PRIMARY1		= PLACEMENT_PRIMARY | PLACEMENT_1,
		PLACEMENT_SECONDARY0	= PLACEMENT_SECONDARY | PLACEMENT_0,
		PLACEMENT_SECONDARY1	= PLACEMENT_SECONDARY | PLACEMENT_1,
		
		PLACEMENT_SPLITTER_MASK	= 0x0f,
		PLACEMENT_PANE_MASK		= 0xf0
	};
	
	enum Splitter {
		SPLITTER_PRIMARY,
		SPLITTER_SECONDARY,
			
		MAX_SPLITTER
	};
	
	enum Component {
		COMPONENT_SECONDARY,
		COMPONENT_FOLDER,
		COMPONENT_LIST,
		COMPONENT_PREVIEW,
		
		MAX_COMPONENT
	};

public:
	explicit SplitterHelper(qs::Profile* pProfile);
	~SplitterHelper();

public:
	Type getType(Splitter splitter) const;
	Placement getPlacement(Component component) const;
	int getLocation(Splitter splitter) const;

public:
	void setWindows(qs::SplitterWindow* pPrimarySplitterWindow,
					qs::SplitterWindow* pSecondarySplitterWindow);
	qs::SplitterWindow* getSplitterWindow(Component component) const;
	void addComponents(qs::Window* pFolderWindow,
					   qs::Window* pListWindow,
					   qs::Window* pPreviewWindow);
	void applyLocation(Splitter splitter) const;
	void saveLocation(Splitter splitter);
	bool isVisible(Component component) const;
	bool setVisible(Component component,
					bool bVisible);
	void applyVisibility(Component component) const;
	void save() const;

private:
	Splitter getSplitter(Component component) const;
	std::pair<int, int> getPane(Component component) const;
	void addComponent(Component component,
					  qs::Window* pWindow);
	Component getOppositeComponent(Component component) const;

private:
	static bool checkType(WCHAR c);
	static bool checkComponents(WCHAR c0,
								WCHAR c1,
								WCHAR c2);
	static Type getType(WCHAR c);
	static Component getComponent(WCHAR c);

private:
	SplitterHelper(const SplitterHelper&);
	SplitterHelper& operator=(const SplitterHelper&);

private:
	qs::Profile* pProfile_;
	
	Type types_[MAX_SPLITTER];
	Placement placements_[MAX_COMPONENT];
	
	qs::SplitterWindow* pSplitterWindow_[MAX_SPLITTER];
	int nLocations_[MAX_SPLITTER];
	bool bVisible_[MAX_COMPONENT];
};

}

#endif // __MAINWINDOW_H__
