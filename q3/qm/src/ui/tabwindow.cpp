/*
 * $Id$
 *
 * Copyright(C) 1998-2004 Satoshi Nakamura
 * All rights reserved.
 *
 */

#ifdef QMTABWINDOW

#include <qmaccount.h>
#include <qmapplication.h>
#include <qmdocument.h>
#include <qmfolder.h>

#include <qsmenu.h>
#include <qsuiutil.h>

#include "resourceinc.h"
#include "tabmodel.h"
#include "tabwindow.h"
#include "uimanager.h"
#include "uiutil.h"

using namespace qm;
using namespace qs;


/****************************************************************************
 *
 * TabWindowImpl
 *
 */

class qm::TabWindowImpl :
	public NotifyHandler,
	public TabModelHandler,
	public FolderHandler
{
public:
	enum {
		ID_TABCTRL = 1100
	};
	
	enum {
		WM_TABWINDOW_MESSAGEADDED		= WM_APP + 1501,
		WM_TABWINDOW_MESSAGEREMOVED		= WM_APP + 1502,
		WM_TABWINDOW_MESSAGEREFRESHED	= WM_APP + 1503,
		WM_TABWINDOW_MESSAGECHANGED		= WM_APP + 1504,
	};

public:
	typedef std::vector<std::pair<Folder*, int> > FolderList;

public:
	void layoutChildren();
	void layoutChildren(int cx,
						int cy);
	void handleUpdateMessage(LPARAM lParam);

public:
	virtual LRESULT onNotify(NMHDR* pnmhdr,
							 bool* pbHandled);

public:
	virtual void itemAdded(const TabModelEvent& event);
	virtual void itemRemoved(const TabModelEvent& event);
	virtual void itemChanged(const TabModelEvent& event);
	virtual void itemMoved(const TabModelEvent& event);
	virtual void currentChanged(const TabModelEvent& event);

public:
	virtual void messageAdded(const FolderMessageEvent& event);
	virtual void messageRemoved(const FolderMessageEvent& event);
	virtual void messageRefreshed(const FolderEvent& event);
	virtual void unseenCountChanged(const FolderEvent& event);
	virtual void folderDestroyed(const FolderEvent& event);

private:
	LRESULT onSelChange(NMHDR* pnmhdr,
						bool* pbHandled);

private:
	void getChildRect(RECT* pRect);
	void update(Folder* pFolder);
	void update(int nItem);
	void resetHandlers(Folder* pOldFolder,
					   Folder* pNewFolder);
	wstring_ptr getTitle(Account* pAccount);
	wstring_ptr getTitle(Folder* pFolder);

private:
	static int getFolderImage(Folder* pFolder);

public:
	TabWindow* pThis_;
	TabCtrlWindow* pTabCtrl_;
	HWND hwnd_;
	TabModel* pTabModel_;
	Profile* pProfile_;
	
	bool bShowTab_;
	bool bShowAllCount_;
	bool bShowUnseenCount_;
	bool bCreated_;
	bool bLayouting_;
	
	FolderList listHandledFolder_;
};

void qm::TabWindowImpl::layoutChildren()
{
	RECT rect;
	pThis_->getClientRect(&rect);
	layoutChildren(rect.right, rect.bottom);
}

void qm::TabWindowImpl::layoutChildren(int cx,
									   int cy)
{
	bLayouting_ = true;
	
	RECT rect;
	getChildRect(&rect);
	
	pTabCtrl_->setWindowPos(0, 0, 0, cx, rect.top, SWP_NOMOVE | SWP_NOZORDER);
	Window(hwnd_).setWindowPos(0, rect.left, rect.top,
		rect.right - rect.left, rect.bottom - rect.top, SWP_NOZORDER);
	
	bLayouting_ = false;
}

void qm::TabWindowImpl::handleUpdateMessage(LPARAM lParam)
{
	MSG msg;
	while (true) {
		if (!::PeekMessage(&msg, pThis_->getHandle(),
			WM_TABWINDOW_MESSAGEADDED,
			WM_TABWINDOW_MESSAGECHANGED, PM_NOREMOVE))
			break;
		else if (msg.lParam != lParam)
			break;
		::PeekMessage(&msg, pThis_->getHandle(),
			WM_TABWINDOW_MESSAGEADDED,
			WM_TABWINDOW_MESSAGECHANGED, PM_REMOVE);
	}
	
	update(reinterpret_cast<Folder*>(lParam));
}

LRESULT qm::TabWindowImpl::onNotify(NMHDR* pnmhdr,
									bool* pbHandled)
{
	BEGIN_NOTIFY_HANDLER()
		HANDLE_NOTIFY(TCN_SELCHANGE, ID_TABCTRL, onSelChange)
	END_NOTIFY_HANDLER()
	return 1;
}

void qm::TabWindowImpl::itemAdded(const TabModelEvent& event)
{
	int nItem = event.getItem();
	TabItem* pItem = event.getNewItem();
	std::pair<Account*, Folder*> p(pItem->get());
	
	wstring_ptr wstrName(p.first ? getTitle(p.first) : getTitle(p.second));
	W2T(wstrName.get(), ptszName);
	
	TCITEM item = {
		TCIF_TEXT | TCIF_IMAGE | TCIF_PARAM,
		0,
		0,
		const_cast<LPTSTR>(ptszName),
		0,
		p.second ? getFolderImage(p.second) : 0,
		reinterpret_cast<LPARAM>(pItem)
	};
	TabCtrl_InsertItem(pTabCtrl_->getHandle(), nItem, &item);
	
	resetHandlers(0, p.second);
	
	if (pTabModel_->getCount() == 1 || pTabCtrl_->isMultiline())
		layoutChildren();
}

void qm::TabWindowImpl::itemRemoved(const TabModelEvent& event)
{
	int nItem = event.getItem();
	TabItem* pItem = event.getOldItem();
	std::pair<Account*, Folder*> p(pItem->get());
	
	TabCtrl_DeleteItem(pTabCtrl_->getHandle(), nItem);
	
	resetHandlers(p.second, 0);
	
	if (pTabModel_->getCount() == 0 || pTabCtrl_->isMultiline())
		layoutChildren();
}

void qm::TabWindowImpl::itemChanged(const TabModelEvent& event)
{
	int nItem = event.getItem();
	TabItem* pOldItem = event.getOldItem();
	std::pair<Account*, Folder*> pOld(pOldItem->get());
	TabItem* pNewItem = event.getNewItem();
	std::pair<Account*, Folder*> pNew(pNewItem->get());
	
	update(nItem);
	
	resetHandlers(pOld.second, pNew.second);
	
	if (pTabCtrl_->isMultiline())
		layoutChildren();
}

void qm::TabWindowImpl::itemMoved(const TabModelEvent& event)
{
	int nItem = event.getItem();
	TabCtrl_DeleteItem(pTabCtrl_->getHandle(), nItem);
	
	int nAmount = event.getAmount();
	TabItem* pItem = pTabModel_->getItem(nItem + nAmount);
	std::pair<Account*, Folder*> p(pItem->get());
	
	wstring_ptr wstrName(p.first ? getTitle(p.first) : getTitle(p.second));
	W2T(wstrName.get(), ptszName);
	
	TCITEM item = {
		TCIF_TEXT | TCIF_IMAGE | TCIF_PARAM,
		0,
		0,
		const_cast<LPTSTR>(ptszName),
		0,
		p.second ? getFolderImage(p.second) : 0,
		reinterpret_cast<LPARAM>(pItem)
	};
	TabCtrl_InsertItem(pTabCtrl_->getHandle(), nItem + nAmount, &item);
	TabCtrl_SetCurSel(pTabCtrl_->getHandle(), pTabModel_->getCurrent());
	
	if (pTabCtrl_->isMultiline())
		layoutChildren();
}

void qm::TabWindowImpl::currentChanged(const TabModelEvent& event)
{
	int nItem = pTabModel_->getCurrent();
	if (nItem != -1 && TabCtrl_GetCurSel(pTabCtrl_->getHandle()) != nItem)
		TabCtrl_SetCurSel(pTabCtrl_->getHandle(), nItem);
}

void qm::TabWindowImpl::messageAdded(const FolderMessageEvent& event)
{
	pThis_->postMessage(WM_TABWINDOW_MESSAGEADDED,
		0, reinterpret_cast<LPARAM>(event.getFolder()));
}

void qm::TabWindowImpl::messageRemoved(const FolderMessageEvent& event)
{
	pThis_->postMessage(WM_TABWINDOW_MESSAGEREMOVED,
		0, reinterpret_cast<LPARAM>(event.getFolder()));
}

void qm::TabWindowImpl::messageRefreshed(const FolderEvent& event)
{
	pThis_->postMessage(WM_TABWINDOW_MESSAGEREFRESHED,
		0, reinterpret_cast<LPARAM>(event.getFolder()));
}

void qm::TabWindowImpl::unseenCountChanged(const FolderEvent& event)
{
	pThis_->postMessage(WM_TABWINDOW_MESSAGECHANGED,
		0, reinterpret_cast<LPARAM>(event.getFolder()));
}

void qm::TabWindowImpl::folderDestroyed(const FolderEvent& event)
{
}

LRESULT qm::TabWindowImpl::onSelChange(NMHDR* pnmhdr,
									   bool* pbHandled)
{
	int nItem = TabCtrl_GetCurSel(pTabCtrl_->getHandle());
	pTabModel_->setCurrent(nItem);
	return 0;
}

void qm::TabWindowImpl::getChildRect(RECT* pRect)
{
	RECT rect;
	pThis_->getClientRect(&rect);
	*pRect = rect;
	if (bShowTab_) {
		TabCtrl_AdjustRect(pTabCtrl_->getHandle(), FALSE, &rect);
		pRect->top = rect.top - 4;
	}
}

void qm::TabWindowImpl::update(Folder* pFolder)
{
	assert(pFolder);
	
	int nCount = pTabModel_->getCount();
	for (int n = 0; n < nCount; ++n) {
		TabItem* pItem = pTabModel_->getItem(n);
		if (pItem->get().second == pFolder)
			update(n);
	}
}

void qm::TabWindowImpl::update(int nItem)
{
	TabItem* pItem = pTabModel_->getItem(nItem);
	std::pair<Account*, Folder*> p(pItem->get());
	
	wstring_ptr wstrName(p.first ? getTitle(p.first) : getTitle(p.second));
	W2T(wstrName.get(), ptszName);
	
	TCITEM item = {
		TCIF_TEXT | TCIF_IMAGE | TCIF_PARAM,
		0,
		0,
		const_cast<LPTSTR>(ptszName),
		0,
		p.second ? getFolderImage(p.second) : 0,
		reinterpret_cast<LPARAM>(pItem)
	};
	TabCtrl_SetItem(pTabCtrl_->getHandle(), nItem, &item);
}

void qm::TabWindowImpl::resetHandlers(Folder* pOldFolder,
									  Folder* pNewFolder)
{
	if (pOldFolder) {
		FolderList::iterator it = std::find_if(
			listHandledFolder_.begin(), listHandledFolder_.end(),
			std::bind2nd(
				binary_compose_f_gx_hy(
					std::equal_to<Folder*>(),
					std::select1st<FolderList::value_type>(),
					std::identity<Folder*>()),
				pOldFolder));
		assert(it != listHandledFolder_.end());
		if (--(*it).second == 0) {
			pOldFolder->removeFolderHandler(this);
			listHandledFolder_.erase(it);
		}
	}
	
	if (pNewFolder) {
		FolderList::iterator it = std::find_if(
			listHandledFolder_.begin(), listHandledFolder_.end(),
			std::bind2nd(
				binary_compose_f_gx_hy(
					std::equal_to<Folder*>(),
					std::select1st<FolderList::value_type>(),
					std::identity<Folder*>()),
				pNewFolder));
		if (it != listHandledFolder_.end()) {
			++(*it).second;
		}
		else {
			pNewFolder->addFolderHandler(this);
			listHandledFolder_.push_back(std::make_pair(pNewFolder, 1));
		}
	}
}

wstring_ptr qm::TabWindowImpl::getTitle(Account* pAccount)
{
	return concat(L"[", pAccount->getName(), L"]");
}

wstring_ptr qm::TabWindowImpl::getTitle(Folder* pFolder)
{
	WCHAR wsz[64] = L"";
	if (bShowAllCount_ && bShowUnseenCount_)
		swprintf(wsz, L" (%d/%d)", pFolder->getUnseenCount(), pFolder->getCount());
	else if (bShowAllCount_)
		swprintf(wsz, L" (%d)", pFolder->getCount());
	else if (bShowUnseenCount_)
		swprintf(wsz, L" (%d)", pFolder->getUnseenCount());
	
	ConcatW c[] = {
		{ L"[",								1	},
		{ pFolder->getAccount()->getName(),	-1	},
		{ L"] ",							2	},
		{ pFolder->getName(),				-1	},
		{ wsz,								-1	}
	};
	return concat(c, countof(c));
}

int qm::TabWindowImpl::getFolderImage(Folder* pFolder)
{
	int nImage = UIUtil::getFolderImage(pFolder, false);
	
	const unsigned int nIgnore =
		(Folder::FLAG_BOX_MASK & ~Folder::FLAG_INBOX) |
		Folder::FLAG_IGNOREUNSEEN;
	
	bool bUnseen = false;
	const Account::FolderList& l = pFolder->getAccount()->getFolders();
	for (Account::FolderList::const_iterator it = l.begin(); it != l.end() && !bUnseen; ++it) {
		Folder* p = *it;
		if (p == pFolder ||
			((p->getFlags() & nIgnore) == 0 && pFolder->isAncestorOf(p)))
			bUnseen = p->getUnseenCount() != 0;
	}
	
	return bUnseen ? nImage + 1 : nImage;
}


/****************************************************************************
 *
 * TabWindow
 *
 */

qm::TabWindow::TabWindow(TabModel* pTabModel,
						 Profile* pProfile) :
	WindowBase(true),
	pImpl_(0)
{
	pImpl_ = new TabWindowImpl();
	pImpl_->pThis_ = this;
	pImpl_->pTabCtrl_ = 0;
	pImpl_->hwnd_ = 0;
	pImpl_->pTabModel_ = pTabModel;
	pImpl_->pProfile_ = pProfile;
	pImpl_->bShowTab_ = pProfile->getInt(L"TabWindow", L"Show", 1) != 0;
	pImpl_->bShowAllCount_ = pProfile->getInt(L"TabWindow", L"ShowAllCount", 1) != 0;
	pImpl_->bShowUnseenCount_ = pProfile->getInt(L"TabWindow", L"ShowUnseenCount", 1) != 0;
	
	setWindowHandler(this, false);
}

qm::TabWindow::~TabWindow()
{
	delete pImpl_;
}

bool qm::TabWindow::isShowTab() const
{
	return pImpl_->bShowTab_;
}

void qm::TabWindow::setShowTab(bool bShow)
{
	if (bShow != pImpl_->bShowTab_) {
		pImpl_->bShowTab_ = bShow;
		pImpl_->layoutChildren();
	}
}

bool qm::TabWindow::save() const
{
	pImpl_->pProfile_->setInt(L"TabWindow", L"Show", pImpl_->bShowTab_);
	return true;
}

void qm::TabWindow::setControl(HWND hwnd)
{
	pImpl_->hwnd_ = hwnd;
}

LRESULT qm::TabWindow::windowProc(UINT uMsg,
								  WPARAM wParam,
								  LPARAM lParam)
{
	BEGIN_MESSAGE_HANDLER()
		HANDLE_CREATE()
		HANDLE_DESTROY()
		HANDLE_SIZE()
		HANDLE_MESSAGE(TabWindowImpl::WM_TABWINDOW_MESSAGEADDED, onMessageAdded)
		HANDLE_MESSAGE(TabWindowImpl::WM_TABWINDOW_MESSAGEREMOVED, onMessageRemoved)
		HANDLE_MESSAGE(TabWindowImpl::WM_TABWINDOW_MESSAGEREFRESHED, onMessageRefreshed)
		HANDLE_MESSAGE(TabWindowImpl::WM_TABWINDOW_MESSAGECHANGED, onMessageChanged)
	END_MESSAGE_HANDLER()
	return DefaultWindowHandler::windowProc(uMsg, wParam, lParam);
}

LRESULT qm::TabWindow::onCreate(CREATESTRUCT* pCreateStruct)
{
	if (DefaultWindowHandler::onCreate(pCreateStruct) == -1)
		return -1;
	
	TabWindowCreateContext* pContext =
		reinterpret_cast<TabWindowCreateContext*>(pCreateStruct->lpCreateParams);
	
	std::auto_ptr<TabCtrlWindow> pTabCtrl(new TabCtrlWindow(
		pImpl_->pTabModel_, pImpl_->pProfile_,
		pContext->pUIManager_->getMenuManager()));
	if (!pTabCtrl->create(L"QmTabCtrlWindow", 0,
		WS_CHILD | WS_VISIBLE | WS_CLIPCHILDREN | WS_CLIPSIBLINGS,
		CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
		getHandle(), 0, 0, TabWindowImpl::ID_TABCTRL, 0))
		return -1;
	pImpl_->pTabCtrl_ = pTabCtrl.release();
	
	pImpl_->pTabModel_->addTabModelHandler(pImpl_);
	addNotifyHandler(pImpl_);
	
	pImpl_->bCreated_ = true;
	
	return 0;
}

LRESULT qm::TabWindow::onDestroy()
{
	removeNotifyHandler(pImpl_);
	pImpl_->pTabModel_->removeTabModelHandler(pImpl_);
	
	return DefaultWindowHandler::onDestroy();
}

LRESULT qm::TabWindow::onSize(UINT nFlags,
							  int cx,
							  int cy)
{
	if (pImpl_->bCreated_ && !pImpl_->bLayouting_ && pImpl_->hwnd_)
		pImpl_->layoutChildren(cx, cy);
	return DefaultWindowHandler::onSize(nFlags, cx, cy);
}

LRESULT qm::TabWindow::onMessageAdded(WPARAM wParam,
									  LPARAM lParam)
{
	pImpl_->handleUpdateMessage(lParam);
	return 0;
}

LRESULT qm::TabWindow::onMessageRemoved(WPARAM wParam,
										LPARAM lParam)
{
	pImpl_->handleUpdateMessage(lParam);
	return 0;
}

LRESULT qm::TabWindow::onMessageRefreshed(WPARAM wParam,
										  LPARAM lParam)
{
	pImpl_->handleUpdateMessage(lParam);
	return 0;
}

LRESULT qm::TabWindow::onMessageChanged(WPARAM wParam,
										LPARAM lParam)
{
	pImpl_->handleUpdateMessage(lParam);
	return 0;
}


/****************************************************************************
 *
 * TabCtrlWindow
 *
 */

qm::TabCtrlWindow::TabCtrlWindow(TabModel* pTabModel,
								 Profile* pProfile,
								 MenuManager* pMenuManager) :
	WindowBase(true),
	pTabModel_(pTabModel),
	pProfile_(pProfile),
	pMenuManager_(pMenuManager),
	bMultiline_(false),
	hfont_(0)
{
	bMultiline_ = pProfile->getInt(L"TabWindow", L"Multiline", 0) != 0;
	
	setWindowHandler(this, false);
}

qm::TabCtrlWindow::~TabCtrlWindow()
{
}

bool qm::TabCtrlWindow::isMultiline() const
{
	return bMultiline_;
}

wstring_ptr qm::TabCtrlWindow::getSuperClass()
{
	return allocWString(WC_TABCONTROLW);
}

bool qm::TabCtrlWindow::preCreateWindow(CREATESTRUCT* pCreateStruct)
{
	if (!DefaultWindowHandler::preCreateWindow(pCreateStruct))
		return false;
	pCreateStruct->style |= TCS_TABS | TCS_FOCUSNEVER |
		(bMultiline_ ? TCS_MULTILINE : TCS_SINGLELINE);
	return true;
}

LRESULT qm::TabCtrlWindow::windowProc(UINT uMsg,
									  WPARAM wParam,
									  LPARAM lParam)
{
	BEGIN_MESSAGE_HANDLER()
		HANDLE_CONTEXTMENU()
		HANDLE_CREATE()
		HANDLE_DESTROY()
		HANDLE_MESSAGE(WM_TABCTRLWINDOW_DESELECTTEMPORARY, onDeselectTemporary)
	END_MESSAGE_HANDLER()
	return DefaultWindowHandler::windowProc(uMsg, wParam, lParam);
}

LRESULT qm::TabCtrlWindow::onContextMenu(HWND hwnd,
										 const POINT& pt)
{
	HMENU hmenu = pMenuManager_->getMenu(L"tab", false, false);
	if (hmenu) {
		TCHITTESTINFO info = {
			{ pt.x, pt.y }
		};
		screenToClient(&info.pt);
		int nItem = TabCtrl_HitTest(getHandle(), &info);
		pTabModel_->setTemporary(nItem);
		
		UINT nFlags = TPM_LEFTALIGN | TPM_TOPALIGN;
#ifndef _WIN32_WCE
		nFlags |= TPM_LEFTBUTTON | TPM_RIGHTBUTTON;
#endif
		::TrackPopupMenu(hmenu, nFlags, pt.x, pt.y, 0, getParentFrame(), 0);
		
		postMessage(TabCtrlWindow::WM_TABCTRLWINDOW_DESELECTTEMPORARY);
	}
	
	return 0;
}

LRESULT qm::TabCtrlWindow::onCreate(CREATESTRUCT* pCreateStruct)
{
	if (DefaultWindowHandler::onCreate(pCreateStruct) == -1)
		return -1;
	
	hfont_ = qs::UIUtil::createFontFromProfile(pProfile_, L"TabWindow", false);
	setFont(hfont_);
	
	HIMAGELIST hImageList = ImageList_LoadImage(
		Application::getApplication().getResourceHandle(),
		MAKEINTRESOURCE(IDB_FOLDER), 16, 0, CLR_DEFAULT, IMAGE_BITMAP, 0);
	TabCtrl_SetImageList(getHandle(), hImageList);
	
	return 0;
}

LRESULT qm::TabCtrlWindow::onDestroy()
{
	if (hfont_) {
		::DeleteObject(hfont_);
		hfont_ = 0;
	}
	
	HIMAGELIST hImageList = TabCtrl_SetImageList(getHandle(), 0);
	ImageList_Destroy(hImageList);
	
	return DefaultWindowHandler::onDestroy();
}

LRESULT qm::TabCtrlWindow::onDeselectTemporary(WPARAM wParam,
											   LPARAM lParam)
{
	pTabModel_->setTemporary(-1);
	return 0;
}

#endif // QMTABWINDOW
