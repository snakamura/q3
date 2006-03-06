/*
 * $Id$
 *
 * Copyright(C) 1998-2005 Satoshi Nakamura
 * All rights reserved.
 *
 */

#pragma warning(disable:4786)

#include <qmaction.h>
#include <qmaddressbookwindow.h>

#include <qsaccelerator.h>
#include <qsaction.h>
#include <qsinit.h>
#include <qsuiutil.h>

#ifdef _WIN32_WCE_PSPC
#	include <aygshell.h>
#endif
#include <tchar.h>

#include "actionid.h"
#include "addressbookdialog.h"
#include "addressbookwindow.h"
#include "dialogs.h"
#include "resourceinc.h"
#include "statusbar.h"
#include "uimanager.h"
#include "uiutil.h"
#include "../action/action.h"
#include "../action/actionmacro.h"
#include "../action/addressbookaction.h"
#include "../model/addressbook.h"
#include "../uimodel/addressbookmodel.h"
#include "../uimodel/addressbookselectionmodel.h"

using namespace qm;
using namespace qs;


/****************************************************************************
 *
 * AddressBookFrameWindowImpl
 *
 */

class qm::AddressBookFrameWindowImpl
{
public:
	enum {
		ID_CATEGORYWINDOW		= 1001,
		ID_LISTWINDOW			= 1002,
		ID_TOOLBAR				= 1003,
		ID_STATUSBAR			= 1004,
		ID_COMMANDBARMENU		= 1005,
		ID_COMMANDBARBUTTON		= 1006
	};
	
	enum {
		WM_ADDRESSBOOKFRAMEWINDOW_CLOSE	= WM_APP + 1601
	};

public:
	void initActions();
	void layoutChildren();
	void layoutChildren(int cx,
						int cy);

public:
	AddressBookFrameWindow* pThis_;
	
	bool bShowToolbar_;
	bool bShowStatusBar_;
	
	AddressBookFrameWindowManager* pManager_;
	Profile* pProfile_;
	UIManager* pUIManager_;
	AddressBookListWindow* pListWindow_;
	StatusBar* pStatusBar_;
	std::auto_ptr<Accelerator> pAccelerator_;
	std::auto_ptr<AddressBookModel> pAddressBookModel_;
	std::auto_ptr<ActionMap> pActionMap_;
	std::auto_ptr<ActionInvoker> pActionInvoker_;
	ToolbarCookie* pToolbarCookie_;
	bool bCreated_;
	int nInitialShow_;
	bool bLayouting_;
};

void qm::AddressBookFrameWindowImpl::initActions()
{
	pActionMap_.reset(new ActionMap());
	pActionInvoker_.reset(new ActionInvoker(pActionMap_.get()));
	
	AddressBookSelectionModel* pSelectionModel = pListWindow_->getSelectionModel();
	
	ADD_ACTION2(AddressBookAddressDeleteAction,
		IDM_ADDRESS_DELETE,
		pAddressBookModel_.get(),
		pSelectionModel);
	ADD_ACTION3(AddressBookAddressEditAction,
		IDM_ADDRESS_EDIT,
		pAddressBookModel_.get(),
		pSelectionModel,
		pThis_->getHandle());
	ADD_ACTION2(AddressBookAddressNewAction,
		IDM_ADDRESS_NEW,
		pAddressBookModel_.get(),
		pThis_->getHandle());
	ADD_ACTION1(FileCloseAction,
		IDM_FILE_CLOSE,
		pThis_->getHandle());
	ADD_ACTION2(AddressBookFileSaveAction,
		IDM_FILE_SAVE,
		pAddressBookModel_.get(),
		pThis_->getHandle());
	ADD_ACTION1(AddressBookViewRefreshAction,
		IDM_VIEW_REFRESH,
		pAddressBookModel_.get());
	ADD_ACTION1(ViewShowStatusBarAction<AddressBookFrameWindow>,
		IDM_VIEW_SHOWSTATUSBAR,
		pThis_);
	ADD_ACTION1(ViewShowToolbarAction<AddressBookFrameWindow>,
		IDM_VIEW_SHOWTOOLBAR,
		pThis_);
	ADD_ACTION3(AddressBookViewSortAction,
		IDM_VIEW_SORTADDRESS,
		pAddressBookModel_.get(),
		AddressBookModel::SORT_ADDRESS,
		AddressBookModel::SORT_COLUMN_MASK);
	ADD_ACTION3(AddressBookViewSortAction,
		IDM_VIEW_SORTASCENDING,
		pAddressBookModel_.get(),
		AddressBookModel::SORT_ASCENDING,
		AddressBookModel::SORT_DIRECTION_MASK);
	ADD_ACTION3(AddressBookViewSortAction,
		IDM_VIEW_SORTCOMMENT,
		pAddressBookModel_.get(),
		AddressBookModel::SORT_COMMENT,
		AddressBookModel::SORT_COLUMN_MASK);
	ADD_ACTION3(AddressBookViewSortAction,
		IDM_VIEW_SORTDESCENDING,
		pAddressBookModel_.get(),
		AddressBookModel::SORT_DESCENDING,
		AddressBookModel::SORT_DIRECTION_MASK);
	ADD_ACTION3(AddressBookViewSortAction,
		IDM_VIEW_SORTNAME,
		pAddressBookModel_.get(),
		AddressBookModel::SORT_NAME,
		AddressBookModel::SORT_COLUMN_MASK);
}

void qm::AddressBookFrameWindowImpl::layoutChildren()
{
	RECT rect;
	pThis_->getClientRect(&rect);
	layoutChildren(rect.right - rect.left, rect.bottom - rect.top);
}

void qm::AddressBookFrameWindowImpl::layoutChildren(int cx,
													int cy)
{
	bLayouting_ = true;
	
	HWND hwndToolbar = pThis_->getToolbar();
	RECT rectToolbar;
	Window wndToolbar(hwndToolbar);
	wndToolbar.getWindowRect(&rectToolbar);
	int nToolbarHeight = rectToolbar.bottom - rectToolbar.top;
#if _WIN32_WCE >= 300 && defined _WIN32_WCE_PSPC
	int nTopBarHeight = 0;
	int nBottomBarHeight = bShowToolbar_ ? nToolbarHeight : 0;
#else
	int nTopBarHeight = bShowToolbar_ ? nToolbarHeight : 0;
	int nBottomBarHeight = 0;
#endif
	
	RECT rectStatusBar;
	pStatusBar_->getWindowRect(&rectStatusBar);
	int nStatusBarHeight = bShowStatusBar_ ?
		rectStatusBar.bottom - rectStatusBar.top : 0;
	
	HDWP hdwp = Window::beginDeferWindowPos(3);
	
#if _WIN32_WCE >= 300 && defined _WIN32_WCE_PSPC
	hdwp = wndToolbar.deferWindowPos(hdwp, 0, 0,
		cy - nToolbarHeight, cx, nToolbarHeight, SWP_NOZORDER);
#else
	hdwp = wndToolbar.deferWindowPos(hdwp, 0, 0, 0, cx,
		nToolbarHeight, SWP_NOMOVE | SWP_NOZORDER);
#endif
	wndToolbar.showWindow(bShowToolbar_ ? SW_SHOW : SW_HIDE);
	
	hdwp = pStatusBar_->deferWindowPos(hdwp, 0, 0,
		cy - nStatusBarHeight - nBottomBarHeight, cx,
		rectStatusBar.bottom - rectStatusBar.top, SWP_NOZORDER);
	pStatusBar_->showWindow(bShowStatusBar_ ? SW_SHOW : SW_HIDE);
	
	hdwp = pListWindow_->deferWindowPos(hdwp, 0, 0, nTopBarHeight, cx,
		cy - nStatusBarHeight - nTopBarHeight - nBottomBarHeight, SWP_NOZORDER);
	
	Window::endDeferWindowPos(hdwp);
	
	bLayouting_ = false;
}


/****************************************************************************
 *
 * AddressBookFrameWindow
 *
 */

qm::AddressBookFrameWindow::AddressBookFrameWindow(AddressBookFrameWindowManager* pManager,
												   Profile* pProfile) :
	FrameWindow(Application::getApplication().getResourceHandle(), true),
	pImpl_(0)
{
	pImpl_ = new AddressBookFrameWindowImpl();
	pImpl_->pThis_ = this;
	pImpl_->bShowToolbar_ = pProfile->getInt(L"AddressBookFrameWindow", L"ShowToolbar", 1) != 0;
	pImpl_->bShowStatusBar_ = pProfile->getInt(L"AddressBookFrameWindow", L"ShowStatusBar", 1) != 0;
	pImpl_->pManager_ = pManager;
	pImpl_->pProfile_ = pProfile;
	pImpl_->pUIManager_ = 0;
	pImpl_->pListWindow_ = 0;
	pImpl_->pStatusBar_ = 0;
	pImpl_->pToolbarCookie_ = 0;
	pImpl_->bCreated_ = false;
	pImpl_->nInitialShow_ = SW_SHOWNORMAL;
	pImpl_->bLayouting_ = false;
}

qm::AddressBookFrameWindow::~AddressBookFrameWindow()
{
	delete pImpl_;
}

void qm::AddressBookFrameWindow::initialShow()
{
	showWindow(pImpl_->nInitialShow_);
}

bool qm::AddressBookFrameWindow::tryClose(bool bAsync)
{
	if (pImpl_->pAddressBookModel_->isModified()) {
		HINSTANCE hInst = Application::getApplication().getResourceHandle();
		HWND hwnd = getHandle();
		
		int nMsg = messageBox(hInst, IDS_CONFIRM_SAVEADDRESSBOOK,
			MB_YESNOCANCEL | MB_ICONQUESTION, hwnd);
		switch (nMsg) {
		case IDYES:
			if (!pImpl_->pAddressBookModel_->save()) {
				messageBox(hInst, IDS_ERROR_SAVEADDRESSBOOK, MB_OK | MB_ICONERROR, hwnd);
				return false;
			}
			break;
		case IDNO:
			break;
		case IDCANCEL:
			return false;
		}
	}
	
	if (!bAsync)
		pImpl_->pManager_->close(this);
	else
		postMessage(AddressBookFrameWindowImpl::WM_ADDRESSBOOKFRAMEWINDOW_CLOSE);
	
	return true;
}

bool qm::AddressBookFrameWindow::isShowToolbar() const
{
	return pImpl_->bShowToolbar_;
}

void qm::AddressBookFrameWindow::setShowToolbar(bool bShow)
{
	if (bShow != pImpl_->bShowToolbar_) {
		pImpl_->bShowToolbar_ = bShow;
		pImpl_->layoutChildren();
	}
}

bool qm::AddressBookFrameWindow::isShowStatusBar() const
{
	return pImpl_->bShowStatusBar_;
}

void qm::AddressBookFrameWindow::setShowStatusBar(bool bShow)
{
	if (bShow != pImpl_->bShowStatusBar_) {
		pImpl_->bShowStatusBar_ = bShow;
		pImpl_->layoutChildren();
	}
}

void qm::AddressBookFrameWindow::reloadProfiles()
{
	pImpl_->pListWindow_->reloadProfiles();
}

bool qm::AddressBookFrameWindow::getToolbarButtons(Toolbar* pToolbar)
{
	pToolbar->nId_ = AddressBookFrameWindowImpl::ID_TOOLBAR;
	return true;
}

bool qm::AddressBookFrameWindow::createToolbarButtons(void* pCreateParam,
													  HWND hwndToolbar)
{
	AddressBookFrameWindowCreateContext* pContext =
		static_cast<AddressBookFrameWindowCreateContext*>(pCreateParam);
	UIManager* pUIManager = pContext->pUIManager_;
	pImpl_->pToolbarCookie_ = pUIManager->getToolbarManager()->createButtons(
		L"addressbookframe", hwndToolbar, this);
	return pImpl_->pToolbarCookie_ != 0;
}

#ifdef _WIN32_WCE
UINT qm::AddressBookFrameWindow::getBarId(int n) const
{
	assert(n == 0 || n == 1);
	assert(pnId);
	static UINT nIds[] = {
		AddressBookFrameWindowImpl::ID_COMMANDBARMENU,
		AddressBookFrameWindowImpl::ID_COMMANDBARBUTTON
	};
	return nIds[n];
}

bool qm::AddressBookFrameWindow::getCommandBandsRestoreInfo(int n,
															COMMANDBANDSRESTOREINFO* pcbri) const
{
	WCHAR wszKey[32];
	_snwprintf(wszKey, countof(wszKey), L"CommandBandsRestoreInfo%d", n);
	size_t nSize = pImpl_->pProfile_->getBinary(L"AddressBookFrameWindow", wszKey,
		reinterpret_cast<unsigned char*>(pcbri), sizeof(*pcbri));
	if (nSize != sizeof(*pcbri))
		pcbri->cbSize = 0;
	return true;
}

bool qm::AddressBookFrameWindow::setCommandBandsRestoreInfo(int n,
															const COMMANDBANDSRESTOREINFO& cbri)
{
	WCHAR wszKey[32];
	_snwprintf(wszKey, countof(wszKey), L"CommandBandsRestoreInfo%d", n);
	pImpl_->pProfile_->setBinary(L"AddressBookFrameWindow", wszKey,
		reinterpret_cast<const unsigned char*>(&cbri), sizeof(cbri));
	return true;
}
#endif

HMENU qm::AddressBookFrameWindow::getMenuHandle(void* pCreateParam)
{
	AddressBookFrameWindowCreateContext* pContext =
		static_cast<AddressBookFrameWindowCreateContext*>(pCreateParam);
	UIManager* pUIManager = pContext->pUIManager_;
	return pUIManager->getMenuManager()->getMenu(L"addressbookframe", true, true);
}

UINT qm::AddressBookFrameWindow::getIconId()
{
	return IDI_MAINFRAME;
}

void qm::AddressBookFrameWindow::getWindowClass(WNDCLASS* pwc)
{
	FrameWindow::getWindowClass(pwc);
	pwc->hIcon = ::LoadIcon(Application::getApplication().getResourceHandle(),
		MAKEINTRESOURCE(IDI_MAINFRAME));
	pwc->hbrBackground = reinterpret_cast<HBRUSH>(COLOR_BTNFACE + 1);
}

bool qm::AddressBookFrameWindow::preCreateWindow(CREATESTRUCT* pCreateStruct)
{
#if defined _WIN32_WCE && _WIN32_WCE >= 300 && defined _WIN32_WCE_PSPC
	RECT rect;
	qs::UIUtil::getWorkArea(&rect);
	pCreateStruct->x = rect.left;
	pCreateStruct->y = rect.top;
	pCreateStruct->cx = rect.right - rect.left;
	pCreateStruct->cy = rect.bottom - rect.top;
#elif !defined _WIN32_WCE
	pImpl_->nInitialShow_ = UIUtil::loadWindowPlacement(
		pImpl_->pProfile_, L"AddressBookFrameWindow", pCreateStruct);
#endif
	return true;
}

Action* qm::AddressBookFrameWindow::getAction(UINT nId)
{
	return pImpl_->pActionMap_->getAction(nId);
}

const ActionParam* qm::AddressBookFrameWindow::getActionParam(UINT nId)
{
	return pImpl_->pUIManager_->getActionParamMap()->getActionParam(nId);
}

Accelerator* qm::AddressBookFrameWindow::getAccelerator()
{
	return pImpl_->pAccelerator_.get();
}

LRESULT qm::AddressBookFrameWindow::windowProc(UINT uMsg,
											   WPARAM wParam,
											   LPARAM lParam)
{
	BEGIN_MESSAGE_HANDLER()
		HANDLE_ACTIVATE()
		HANDLE_CLOSE()
		HANDLE_CREATE()
		HANDLE_DESTROY()
		HANDLE_SIZE()
		HANDLE_MESSAGE(AddressBookFrameWindowImpl::WM_ADDRESSBOOKFRAMEWINDOW_CLOSE, onAddressBookFrameWindowClose)
	END_MESSAGE_HANDLER()
	return FrameWindow::windowProc(uMsg, wParam, lParam);
}

LRESULT qm::AddressBookFrameWindow::onActivate(UINT nFlags,
											   HWND hwnd,
											   bool bMinimized)
{
	FrameWindow::onActivate(nFlags, hwnd, bMinimized);
	
	if (nFlags != WA_INACTIVE)
		pImpl_->pListWindow_->setFocus();
	
	return 0;
}

LRESULT qm::AddressBookFrameWindow::onClose()
{
	tryClose(false);
	
	return 0;
}

LRESULT qm::AddressBookFrameWindow::onCreate(CREATESTRUCT* pCreateStruct)
{
	if (FrameWindow::onCreate(pCreateStruct) == -1)
		return -1;
	
	AddressBookFrameWindowCreateContext* pContext =
		static_cast<AddressBookFrameWindowCreateContext*>(pCreateStruct->lpCreateParams);
	pImpl_->pUIManager_ = pContext->pUIManager_;
	
	CustomAcceleratorFactory acceleratorFactory;
	pImpl_->pAccelerator_ = pContext->pUIManager_->getKeyMap()->createAccelerator(
		&acceleratorFactory, L"AddressBookFrameWindow");
	if (!pImpl_->pAccelerator_.get())
		return -1;
	
	pImpl_->pAddressBookModel_.reset(pContext->pAddressBookModel_);
	
	DWORD dwStyle = WS_CHILD | WS_VISIBLE | WS_CLIPCHILDREN | WS_CLIPSIBLINGS;
#if defined _WIN32_WCE && _WIN32_WCE >= 300 && defined _WIN32_WCE_PSPC
	DWORD dwExStyle = 0;
#else
	DWORD dwExStyle = WS_EX_CLIENTEDGE;
#endif
	
	std::auto_ptr<AddressBookListWindow> pListWindow(new AddressBookListWindow(
		this, pImpl_->pAddressBookModel_.get(), pImpl_->pProfile_));
	AddressBookListWindowCreateContext context = {
		pContext->pUIManager_
	};
	if (!pListWindow->create(L"QmAddressBookListWindow", 0, dwStyle,
		CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, getHandle(),
		dwExStyle, 0, AddressBookFrameWindowImpl::ID_LISTWINDOW, &context))
		return -1;
	pImpl_->pListWindow_ = pListWindow.release();
	
	DWORD dwStatusBarStyle = dwStyle;
#if _WIN32_WCE >= 300 && defined _WIN32_WCE_PSPC
	dwStatusBarStyle |= CCS_NOPARENTALIGN;
#endif
	std::auto_ptr<StatusBar> pStatusBar(new StatusBar());
	if (!pStatusBar->create(L"QmStatusBarWindow", 0, dwStatusBarStyle,
		CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, getHandle(),
		0, STATUSCLASSNAMEW, AddressBookFrameWindowImpl::ID_STATUSBAR, 0))
		return -1;
	pImpl_->pStatusBar_ = pStatusBar.release();
	
	pImpl_->layoutChildren();
	pImpl_->initActions();
	pImpl_->bCreated_ = true;
	
	return 0;
}

LRESULT qm::AddressBookFrameWindow::onDestroy()
{
	Profile* pProfile = pImpl_->pProfile_;
	
	pProfile->setInt(L"AddressBookFrameWindow", L"ShowToolbar", pImpl_->bShowToolbar_);
	pProfile->setInt(L"AddressBookFrameWindow", L"ShowStatusBar", pImpl_->bShowStatusBar_);
	
	if (pImpl_->pToolbarCookie_)
		pImpl_->pUIManager_->getToolbarManager()->destroy(pImpl_->pToolbarCookie_);
	
	UIUtil::saveWindowPlacement(getHandle(), pProfile, L"AddressBookFrameWindow");
	
	FrameWindow::save();
	
	::PostQuitMessage(0);
	
	return FrameWindow::onDestroy();
}

LRESULT qm::AddressBookFrameWindow::onSize(UINT nFlags,
										   int cx,
										   int cy)
{
	if (pImpl_->bCreated_ &&
		!pImpl_->bLayouting_ &&
		(nFlags == SIZE_RESTORED || nFlags == SIZE_MAXIMIZED))
		pImpl_->layoutChildren(cx, cy);
	return FrameWindow::onSize(nFlags, cx, cy);
}

LRESULT qm::AddressBookFrameWindow::onAddressBookFrameWindowClose(WPARAM wParam,
																  LPARAM lParam)
{
	pImpl_->pManager_->close(this);
	return 0;
}


/****************************************************************************
 *
 * AddressBookFrameWindowManager
 *
 */

qm::AddressBookFrameWindowManager::AddressBookFrameWindowManager(AddressBook* pAddressBook,
																 UIManager* pUIManager,
																 Profile* pProfile) :
	pAddressBook_(pAddressBook),
	pUIManager_(pUIManager),
	pProfile_(pProfile),
	pSynchronizer_(InitThread::getInitThread().getSynchronizer()),
	pThread_(0),
	pFrameWindow_(0),
	bClosing_(false)
{
}

qm::AddressBookFrameWindowManager::~AddressBookFrameWindowManager()
{
}

void qm::AddressBookFrameWindowManager::open()
{
	Lock<CriticalSection> lock(cs_);
	
	if (bClosing_)
		return;
	
	if (pFrameWindow_) {
		pFrameWindow_->setForegroundWindow();
	}
	else {
		std::auto_ptr<AddressBookModel> pAddressBookModel(new AddressBookModel());
		pAddressBookModel->addAddressBookModelHandler(this);
		std::auto_ptr<AddressBookThread> pThread(new AddressBookThread(
			this, pAddressBookModel, pUIManager_, pProfile_));
		pFrameWindow_ = pThread->create();
		if (!pFrameWindow_)
			return;
		pThread_ = pThread.release();
	}
}

bool qm::AddressBookFrameWindowManager::closeAll()
{
	Thread* pThread = 0;
	{
		Lock<CriticalSection> lock(cs_);
		
		assert(!bClosing_);
		
		if (!pFrameWindow_)
			return true;
		
		pThread = pThread_;
		
		bClosing_ = true;
		
		struct RunnableImpl : public Runnable
		{
			RunnableImpl(AddressBookFrameWindow* pFrameWindow) :
				pFrameWindow_(pFrameWindow)
			{
			}
			
			virtual void run()
			{
				pFrameWindow_->setForegroundWindow();
				b_ = pFrameWindow_->tryClose(true);
			}
			
			AddressBookFrameWindow* pFrameWindow_;
			bool b_;
		} runnable(pFrameWindow_);
		pFrameWindow_->getInitThread()->getSynchronizer()->syncExec(&runnable);
		if (!runnable.b_) {
			bClosing_ = false;
			return false;
		}
	}
	
	pThread->join();
	
	return true;
}

void qm::AddressBookFrameWindowManager::showAll()
{
	struct RunnableImpl : public Runnable
	{
		RunnableImpl(AddressBookFrameWindow* pFrameWindow) :
			pFrameWindow_(pFrameWindow)
		{
		}
		
		virtual void run()
		{
			pFrameWindow_->showWindow();
		}
		
		AddressBookFrameWindow* pFrameWindow_;
	} runnable(pFrameWindow_);
	pFrameWindow_->getInitThread()->getSynchronizer()->syncExec(&runnable);
}

void qm::AddressBookFrameWindowManager::hideAll()
{
	struct RunnableImpl : public Runnable
	{
		RunnableImpl(AddressBookFrameWindow* pFrameWindow) :
			pFrameWindow_(pFrameWindow)
		{
		}
		
		virtual void run()
		{
			pFrameWindow_->showWindow(SW_HIDE);
		}
		
		AddressBookFrameWindow* pFrameWindow_;
	} runnable(pFrameWindow_);
	pFrameWindow_->getInitThread()->getSynchronizer()->syncExec(&runnable);
}

void qm::AddressBookFrameWindowManager::reloadProfiles()
{
	if (!pFrameWindow_)
		return;
	
	struct RunnableImpl : public Runnable
	{
		RunnableImpl(AddressBookFrameWindow* pFrameWindow) :
			pFrameWindow_(pFrameWindow)
		{
		}
		
		virtual void run()
		{
			pFrameWindow_->reloadProfiles();
		}
		
		AddressBookFrameWindow* pFrameWindow_;
	} runnable(pFrameWindow_);
	pFrameWindow_->getInitThread()->getSynchronizer()->syncExec(&runnable);
}

void qm::AddressBookFrameWindowManager::close(AddressBookFrameWindow* pFrameWindow)
{
	Lock<CriticalSection> lock(cs_);
	
	assert(pFrameWindow_ == pFrameWindow);
	
	pFrameWindow->destroyWindow();
	
	pThread_ = 0;
	pFrameWindow_ = 0;
}

void qm::AddressBookFrameWindowManager::saved(const AddressBookModelEvent& event)
{
	struct RunnableImpl : public Runnable
	{
		RunnableImpl(AddressBook* pAddressBook) :
			pAddressBook_(pAddressBook)
		{
		}
		
		virtual void run()
		{
			pAddressBook_->reload();
		}
		
		AddressBook* pAddressBook_;
	} runnable(pAddressBook_);
	pSynchronizer_->syncExec(&runnable);
}


/****************************************************************************
 *
 * AddressBookListWindowImpl
 *
 */

class qm::AddressBookListWindowImpl :
	public AddressBookSelectionModel,
	public NotifyHandler,
	public AddressBookModelHandler
{
public:
	void loadColumns();
	void saveColumns();
	void refresh();
	void open(int nItem);
	void reloadProfiles(bool bInitialize);

public:
	virtual void getSelectedItems(ItemList* pList);
	virtual bool hasSelectedItem();
	virtual unsigned int getFocusedItem();

public:
	virtual LRESULT onNotify(NMHDR* pnmhdr,
							 bool* pbHandled);

public:
	virtual void itemAdded(const AddressBookModelEvent& event);
	virtual void itemRemoved(const AddressBookModelEvent& event);
	virtual void itemEdited(const AddressBookModelEvent& event);
	virtual void refreshed(const AddressBookModelEvent& event);
	virtual void sorting(const AddressBookModelEvent& event);
	virtual void sorted(const AddressBookModelEvent& event);
	virtual void saved(const AddressBookModelEvent& event);

private:
	LRESULT onColumnClick(NMHDR* pnmhdr,
						  bool* pbHandled);
	LRESULT onGetDispInfo(NMHDR* pnmhdr,
						  bool* pbHandled);

private:
	class SelectionRestorer
	{
	public:
		SelectionRestorer(AddressBookModel* pModel,
						  HWND hwnd);
		~SelectionRestorer();
	
	public:
		void restore();
	
	private:
		void save();
	
	private:
		typedef std::vector<const AddressBookEntry*> EntryList;
	
	private:
		AddressBookModel* pModel_;
		HWND hwnd_;
		EntryList listSelected_;
		const AddressBookEntry* pFocused_;
	};

public:
	AddressBookListWindow* pThis_;
	WindowBase* pParentWindow_;
	AddressBookModel* pAddressBookModel_;
	MenuManager* pMenuManager_;
	Profile* pProfile_;
	std::auto_ptr<Accelerator> pAccelerator_;
	
	UINT nId_;
	HFONT hfont_;
	std::auto_ptr<SelectionRestorer> pSelectionRestorer_;
};

void qm::AddressBookListWindowImpl::loadColumns()
{
	HINSTANCE hInst = Application::getApplication().getResourceHandle();
	struct {
		UINT nId_;
		const WCHAR* pwszWidthKey_;
		int nDefaultWidth_;
	} columns[] = {
		{ IDS_ADDRESSBOOK_COLUMN_NAME,		L"NameWidth",		150	},
		{ IDS_ADDRESSBOOK_COLUMN_ADDRESS,	L"AddressWidth",	150	},
		{ IDS_ADDRESSBOOK_COLUMN_COMMENT,	L"CommentWidth",	150	}
	};
	for (int n = 0; n < countof(columns); ++n) {
		wstring_ptr wstrTitle(loadString(hInst, columns[n].nId_));
		W2T(wstrTitle.get(), ptszTitle);
		
		int nWidth = pProfile_->getInt(L"AddressBookListWindow",
			columns[n].pwszWidthKey_, columns[n].nDefaultWidth_);
		LVCOLUMN column = {
			LVCF_FMT | LVCF_SUBITEM | LVCF_TEXT | LVCF_WIDTH,
			LVCFMT_LEFT,
			nWidth,
			const_cast<LPTSTR>(ptszTitle),
			0,
			n
		};
		ListView_InsertColumn(pThis_->getHandle(), n, &column);
	}
}

void qm::AddressBookListWindowImpl::saveColumns()
{
	const WCHAR* pwszWidthKeys[] = {
		L"NameWidth",
		L"AddressWidth",
		L"CommentWidth"
	};
	for (int n = 0; n < countof(pwszWidthKeys); ++n) {
		int nWidth = ListView_GetColumnWidth(pThis_->getHandle(), n);
		pProfile_->setInt(L"AddressBookListWindow", pwszWidthKeys[n], nWidth);
	}
}

void qm::AddressBookListWindowImpl::refresh()
{
	HWND hwnd = pThis_->getHandle();
	DisableRedraw disable(hwnd);
	
	ListView_DeleteAllItems(hwnd);
	
	unsigned int nCount = pAddressBookModel_->getCount();
	for (unsigned int n = 0; n < nCount; ++n) {
		LVITEM item = {
			LVIF_TEXT,
			n,
			0,
			0,
			0,
			LPSTR_TEXTCALLBACK,
			0,
			0,
			0
		};
		item.iItem = ListView_InsertItem(hwnd, &item);
		item.iSubItem = 1;
		ListView_SetItem(hwnd, &item);
		item.iSubItem = 2;
		ListView_SetItem(hwnd, &item);
	}
}

void qm::AddressBookListWindowImpl::open(int nItem)
{
	std::auto_ptr<AddressBookEntry> pEntry(new AddressBookEntry(
		*pAddressBookModel_->getEntry(nItem)));
	AddressBookEntryDialog dialog(pAddressBookModel_->getAddressBook(), pEntry.get());
	if (dialog.doModal(pThis_->getParentFrame()) == IDOK)
		pAddressBookModel_->edit(nItem, pEntry);
}

void qm::AddressBookListWindowImpl::reloadProfiles(bool bInitialize)
{
	HFONT hfont = qs::UIUtil::createFontFromProfile(pProfile_,
		L"AddressBookListWindow", qs::UIUtil::DEFAULTFONT_UI);
	if (!bInitialize) {
		assert(hfont_);
		Window(ListView_GetHeader(pThis_->getHandle())).setFont(hfont);
		pThis_->setFont(hfont);
		::DeleteObject(hfont_);
	}
	hfont_ = hfont;
}

void qm::AddressBookListWindowImpl::getSelectedItems(ItemList* pList)
{
	int nItem = -1;
	while (true) {
		nItem = ListView_GetNextItem(pThis_->getHandle(), nItem, LVNI_ALL | LVNI_SELECTED);
		if (nItem == -1)
			break;
		pList->push_back(nItem);
	}
}

bool qm::AddressBookListWindowImpl::hasSelectedItem()
{
	return ListView_GetNextItem(pThis_->getHandle(), -1, LVNI_ALL | LVNI_SELECTED) != -1;
}

unsigned int qm::AddressBookListWindowImpl::getFocusedItem()
{
	return ListView_GetNextItem(pThis_->getHandle(), -1, LVNI_ALL | LVNI_FOCUSED);
}

LRESULT qm::AddressBookListWindowImpl::onNotify(NMHDR* pnmhdr,
												bool* pbHandled)
{
	BEGIN_NOTIFY_HANDLER()
		HANDLE_NOTIFY(LVN_COLUMNCLICK, nId_, onColumnClick)
		HANDLE_NOTIFY(LVN_GETDISPINFO, nId_, onGetDispInfo)
	END_NOTIFY_HANDLER()
	return NotifyHandler::onNotify(pnmhdr, pbHandled);
}

void qm::AddressBookListWindowImpl::itemAdded(const AddressBookModelEvent& event)
{
	HWND hwnd = pThis_->getHandle();
	
	LVITEM item = {
		LVIF_TEXT,
		event.getItem(),
		0,
		0,
		0,
		LPSTR_TEXTCALLBACK,
		0,
		0,
		0
	};
	item.iItem = ListView_InsertItem(hwnd, &item);
	item.iSubItem = 1;
	ListView_SetItem(hwnd, &item);
	item.iSubItem = 2;
	ListView_SetItem(hwnd, &item);
}

void qm::AddressBookListWindowImpl::itemRemoved(const AddressBookModelEvent& event)
{
	ListView_DeleteItem(pThis_->getHandle(), event.getItem());
}

void qm::AddressBookListWindowImpl::itemEdited(const AddressBookModelEvent& event)
{
	RECT rect;
	ListView_GetItemRect(pThis_->getHandle(), event.getItem(), &rect, LVIR_BOUNDS);
	pThis_->invalidateRect(rect);
}

void qm::AddressBookListWindowImpl::refreshed(const AddressBookModelEvent& event)
{
	refresh();
}

void qm::AddressBookListWindowImpl::sorting(const AddressBookModelEvent& event)
{
	pSelectionRestorer_.reset(new SelectionRestorer(pAddressBookModel_, pThis_->getHandle()));
}

void qm::AddressBookListWindowImpl::sorted(const AddressBookModelEvent& event)
{
	HWND hwnd = pThis_->getHandle();
	
	pSelectionRestorer_->restore();
	pSelectionRestorer_.reset(0);
	
	pThis_->invalidate();
}

void qm::AddressBookListWindowImpl::saved(const AddressBookModelEvent& event)
{
}

LRESULT qm::AddressBookListWindowImpl::onColumnClick(NMHDR* pnmhdr,
													 bool* pbHandled)
{
	NMLISTVIEW* pnmlv = reinterpret_cast<NMLISTVIEW*>(pnmhdr);
	
	AddressBookModel::Sort sort = AddressBookModel::SORT_NAME;
	switch (pnmlv->iSubItem) {
	case 0:
		sort = AddressBookModel::SORT_NAME;
		break;
	case 1:
		sort = AddressBookModel::SORT_ADDRESS;
		break;
	case 2:
		sort = AddressBookModel::SORT_COMMENT;
		break;
	default:
		assert(false);
		break;
	}
	
	unsigned int nSort = pAddressBookModel_->getSort();
	if ((nSort & AddressBookModel::SORT_COLUMN_MASK) == static_cast<unsigned int>(sort)) {
		bool bAscending = (nSort & AddressBookModel::SORT_DIRECTION_MASK) ==
			AddressBookModel::SORT_ASCENDING;
		pAddressBookModel_->setSort(bAscending ? AddressBookModel::SORT_DESCENDING :
			AddressBookModel::SORT_ASCENDING, AddressBookModel::SORT_DIRECTION_MASK);
	}
	else {
		pAddressBookModel_->setSort(sort | AddressBookModel::SORT_ASCENDING,
			AddressBookModel::SORT_COLUMN_MASK | AddressBookModel::SORT_DIRECTION_MASK);
	}
	
	return 0;
}

LRESULT qm::AddressBookListWindowImpl::onGetDispInfo(NMHDR* pnmhdr,
													 bool* pbHandled)
{
	NMLVDISPINFO* pnmlvDispInfo = reinterpret_cast<NMLVDISPINFO*>(pnmhdr);
	const LVITEM& item = pnmlvDispInfo->item;
	
	if (item.mask & LVIF_TEXT) {
		const AddressBookEntry* pEntry = pAddressBookModel_->getEntry(item.iItem);
		const AddressBookAddress* pAddress = 0;
		const AddressBookEntry::AddressList& listAddress = pEntry->getAddresses();
		if (!listAddress.empty())
			pAddress = listAddress.front();
		
		const WCHAR* pwszText = 0;
		switch (item.iSubItem) {
		case 0:
			pwszText = pEntry->getName();
			break;
		case 1:
			if (pAddress)
				pwszText = pAddress->getAddress();
			break;
		case 2:
			if (pAddress)
				pwszText = pAddress->getComment();
			break;
		default:
			assert(false);
			break;
		}
		if (pwszText) {
			W2T(pwszText, ptszText);
			_tcsncpy(item.pszText, ptszText, item.cchTextMax);
		}
	}
	
	return 0;
}


/****************************************************************************
 *
 * AddressBookListWindowImpl::SelectionRestorer
 *
 */

qm::AddressBookListWindowImpl::SelectionRestorer::SelectionRestorer(AddressBookModel* pModel,
																	HWND hwnd) :
	pModel_(pModel),
	hwnd_(hwnd),
	pFocused_(0)
{
	save();
}

qm::AddressBookListWindowImpl::SelectionRestorer::~SelectionRestorer()
{
}

void qm::AddressBookListWindowImpl::SelectionRestorer::restore()
{
	unsigned int nCount = pModel_->getCount();
	for (unsigned int n = 0; n < nCount; ++n) {
		const AddressBookEntry* pEntry = pModel_->getEntry(n);
		
		UINT nState = 0;
		if (std::find(listSelected_.begin(), listSelected_.end(), pEntry) != listSelected_.end())
			nState |= LVIS_SELECTED;
		if (pEntry == pFocused_)
			nState |= LVIS_FOCUSED;
		
		ListView_SetItemState(hwnd_, n, nState, LVIS_SELECTED | LVIS_FOCUSED);
	}
	
	listSelected_.clear();
	pFocused_ = 0;
}

void qm::AddressBookListWindowImpl::SelectionRestorer::save()
{
	listSelected_.clear();
	pFocused_ = 0;
	
	int nItem = -1;
	while (true) {
		nItem = ListView_GetNextItem(hwnd_, nItem, LVNI_ALL | LVNI_SELECTED);
		if (nItem == -1)
			break;
		listSelected_.push_back(pModel_->getEntry(nItem));
	}
	
	nItem = ListView_GetNextItem(hwnd_, nItem, LVNI_ALL | LVNI_FOCUSED);
	if (nItem != -1)
		pFocused_ = pModel_->getEntry(nItem);
}


/****************************************************************************
 *
 * AddressBookListWindow
 *
 */

qm::AddressBookListWindow::AddressBookListWindow(WindowBase* pParentWindow,
												 AddressBookModel* pAddressBookModel,
												 Profile* pProfile) :
	WindowBase(true),
	pImpl_(0)
{
	pImpl_ = new AddressBookListWindowImpl();
	pImpl_->pThis_ = this;
	pImpl_->pParentWindow_ = pParentWindow;
	pImpl_->pAddressBookModel_ = pAddressBookModel;
	pImpl_->pMenuManager_ = 0;
	pImpl_->pProfile_ = pProfile;
	pImpl_->nId_ = 0;
	pImpl_->hfont_ = 0;
	
	pImpl_->reloadProfiles(true);
	
	setWindowHandler(this, false);
	
	pParentWindow->addNotifyHandler(pImpl_);
	pAddressBookModel->addAddressBookModelHandler(pImpl_);
}

qm::AddressBookListWindow::~AddressBookListWindow()
{
	delete pImpl_;
}

AddressBookSelectionModel* qm::AddressBookListWindow::getSelectionModel() const
{
	return pImpl_;
}

void qm::AddressBookListWindow::reloadProfiles()
{
	pImpl_->reloadProfiles(false);
}

wstring_ptr qm::AddressBookListWindow::getSuperClass()
{
	return allocWString(WC_LISTVIEWW);
}

bool qm::AddressBookListWindow::preCreateWindow(CREATESTRUCT* pCreateStruct)
{
	if (!DefaultWindowHandler::preCreateWindow(pCreateStruct))
		return false;
	pCreateStruct->style |= LVS_REPORT;
	return true;
}

Accelerator* qm::AddressBookListWindow::getAccelerator()
{
	return pImpl_->pAccelerator_.get();
}

LRESULT qm::AddressBookListWindow::windowProc(UINT uMsg,
											  WPARAM wParam,
											  LPARAM lParam)
{
	BEGIN_MESSAGE_HANDLER()
		HANDLE_CONTEXTMENU()
		HANDLE_CREATE()
		HANDLE_DESTROY()
		HANDLE_LBUTTONDBLCLK()
		HANDLE_LBUTTONDOWN()
	END_MESSAGE_HANDLER()
	return DefaultWindowHandler::windowProc(uMsg, wParam, lParam);
}

LRESULT qm::AddressBookListWindow::onContextMenu(HWND hwnd,
												 const POINT& pt)
{
	HMENU hmenu = pImpl_->pMenuManager_->getMenu(L"addressbooklist", false, false);
	if (hmenu) {
		UINT nFlags = TPM_LEFTALIGN | TPM_TOPALIGN;
#ifndef _WIN32_WCE
		nFlags |= TPM_LEFTBUTTON | TPM_RIGHTBUTTON;
#endif
		::TrackPopupMenu(hmenu, nFlags, pt.x, pt.y, 0, getParentFrame(), 0);
	}
	
	return 0;
}

LRESULT qm::AddressBookListWindow::onCreate(CREATESTRUCT* pCreateStruct)
{
	if (DefaultWindowHandler::onCreate(pCreateStruct) == -1)
		return -1;
	
	AddressBookListWindowCreateContext* pContext =
		static_cast<AddressBookListWindowCreateContext*>(pCreateStruct->lpCreateParams);
	pImpl_->pMenuManager_ = pContext->pUIManager_->getMenuManager();
	
	CustomAcceleratorFactory acceleratorFactory;
	pImpl_->pAccelerator_ = pContext->pUIManager_->getKeyMap()->createAccelerator(
		&acceleratorFactory, L"AddressBookListWindow");
	if (!pImpl_->pAccelerator_.get())
		return -1;
	
	pImpl_->nId_ = getId();
	
	ListView_SetExtendedListViewStyle(getHandle(), LVS_EX_FULLROWSELECT);
	
	setFont(pImpl_->hfont_, false);
	
	pImpl_->loadColumns();
	pImpl_->refresh();
	
	return 0;
}

LRESULT qm::AddressBookListWindow::onDestroy()
{
	pImpl_->saveColumns();
	
	if (pImpl_->hfont_) {
		::DeleteObject(pImpl_->hfont_);
		pImpl_->hfont_ = 0;
	}
	
	pImpl_->pParentWindow_->removeNotifyHandler(pImpl_);
	pImpl_->pAddressBookModel_->addAddressBookModelHandler(pImpl_);
	
	return DefaultWindowHandler::onDestroy();
}

LRESULT qm::AddressBookListWindow::onLButtonDblClk(UINT nFlags,
												   const POINT& pt)
{
	LVHITTESTINFO info = {
		{ pt.x, pt.y }
	};
	int nItem = ListView_HitTest(getHandle(), &info);
	if (nItem != -1)
		pImpl_->open(nItem);
	
	return DefaultWindowHandler::onLButtonDblClk(nFlags, pt);
}

LRESULT qm::AddressBookListWindow::onLButtonDown(UINT nFlags,
												 const POINT& pt)
{
#if defined _WIN32_WCE && _WIN32_WCE >= 300 && defined _WIN32_WCE_PSPC
	if (tapAndHold(pt))
		return 0;
#endif
	return DefaultWindowHandler::onLButtonDown(nFlags, pt);
}


/****************************************************************************
 *
 * AddressBookThreadImpl
 *
 */

struct qm::AddressBookThreadImpl
{
	AddressBookFrameWindowManager* pManager_;
	std::auto_ptr<AddressBookModel> pAddressBookModel_;
	UIManager* pUIManager_;
	qs::Profile* pProfile_;
	AddressBookFrameWindow* pFrameWindow_;
	std::auto_ptr<qs::Event> pEvent_;
};


/****************************************************************************
 *
 * AddressBookThread
 *
 */

qm::AddressBookThread::AddressBookThread(AddressBookFrameWindowManager* pManager,
										 std::auto_ptr<AddressBookModel> pAddressBookModel,
										 UIManager* pUIManager,
										 Profile* pProfile) :
	pImpl_(0)
{
	pImpl_ = new AddressBookThreadImpl();
	pImpl_->pManager_ = pManager;
	pImpl_->pAddressBookModel_ = pAddressBookModel;
	pImpl_->pUIManager_ = pUIManager;
	pImpl_->pProfile_ = pProfile;
	pImpl_->pFrameWindow_ = 0;
	pImpl_->pEvent_.reset(new Event(true, false));
}

qm::AddressBookThread::~AddressBookThread()
{
	delete pImpl_;
}

AddressBookFrameWindow* qm::AddressBookThread::create()
{
	assert(!pImpl_->pFrameWindow_);
	
	if (!start())
		return 0;
	
	pImpl_->pEvent_->wait();
	
	return pImpl_->pFrameWindow_;
}

void qm::AddressBookThread::run()
{
	InitThread init(InitThread::FLAG_SYNCHRONIZER);
	
	HINSTANCE hInst = Application::getApplication().getResourceHandle();
	wstring_ptr wstrTitle(loadString(hInst, IDS_TITLE_ADDRESSBOOK));
	
	std::auto_ptr<AddressBookFrameWindow> pWindow;
	{
		struct Set
		{
			Set(Event* pEvent) : pEvent_(pEvent) {}
			~Set() { pEvent_->set(); }
			Event* pEvent_;
		} set(pImpl_->pEvent_.get());
		
		pWindow.reset(new AddressBookFrameWindow(pImpl_->pManager_, pImpl_->pProfile_));
#ifdef _WIN32_WCE
		DWORD dwStyle = WS_CLIPCHILDREN;
		DWORD dwExStyle = 0;
#else
		DWORD dwStyle = WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN;
		DWORD dwExStyle = WS_EX_WINDOWEDGE;
#endif
		AddressBookFrameWindowCreateContext context = {
			pImpl_->pAddressBookModel_.get(),
			pImpl_->pUIManager_
		};
		if (!pWindow->create(L"QmAddressBookFrameWindow", wstrTitle.get(), dwStyle,
			CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
			0, dwExStyle, 0, 0, &context))
			return;
		pImpl_->pAddressBookModel_.release();
		pImpl_->pFrameWindow_ = pWindow.release();
		pImpl_->pFrameWindow_->initialShow();
	}
	
	MessageLoop::getMessageLoop().run();
	
	delete this;
}
