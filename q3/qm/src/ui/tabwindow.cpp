/*
 * $Id$
 *
 * Copyright(C) 1998-2005 Satoshi Nakamura
 * All rights reserved.
 *
 */

#ifdef QMTABWINDOW

#pragma warning(disable:4786)

#include <qmaccount.h>
#include <qmapplication.h>
#include <qmdocument.h>
#include <qmfolder.h>

#include <qsmenu.h>
#include <qsuiutil.h>

#include "resourceinc.h"
#include "tabwindow.h"
#include "uimanager.h"
#include "uiutil.h"
#include "../model/dataobject.h"
#include "../uimodel/tabmodel.h"

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
	public DefaultFolderHandler
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
	void reloadProfiles(bool bInitialize);

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
	virtual void folderRenamed(const FolderEvent& event);

private:
	LRESULT onSelChange(NMHDR* pnmhdr,
						bool* pbHandled);

private:
	void getChildRect(RECT* pRect);
	void update(Folder* pFolder);
	void update(int nItem);
	void resetHandlers(Folder* pOldFolder,
					   Folder* pNewFolder);
	wstring_ptr getTitle(const TabItem* pItem) const;

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

void qm::TabWindowImpl::reloadProfiles(bool bInitialize)
{
	bool bShowTab = pProfile_->getInt(L"TabWindow", L"Show", 1) != 0;
	
	bShowAllCount_ = pProfile_->getInt(L"TabWindow", L"ShowAllCount", 1) != 0;
	bShowUnseenCount_ = pProfile_->getInt(L"TabWindow", L"ShowUnseenCount", 1) != 0;
	
	if (!bInitialize) {
		if (bShowTab != pThis_->isShowTab())
			pThis_->setShowTab(bShowTab);
	}
	else {
		bShowTab_ = bShowTab;
	}
	
	
	if (!bInitialize) {
		pTabCtrl_->reloadProfiles();
		for (int n = 0; n < TabCtrl_GetItemCount(pTabCtrl_->getHandle()); ++n)
			update(n);
		layoutChildren();
	}
}

LRESULT qm::TabWindowImpl::onNotify(NMHDR* pnmhdr,
									bool* pbHandled)
{
	BEGIN_NOTIFY_HANDLER()
		HANDLE_NOTIFY(TCN_SELCHANGE, ID_TABCTRL, onSelChange)
	END_NOTIFY_HANDLER()
	return NotifyHandler::onNotify(pnmhdr, pbHandled);
}

void qm::TabWindowImpl::itemAdded(const TabModelEvent& event)
{
	int nItem = event.getItem();
	const TabItem* pItem = event.getNewItem();
	std::pair<Account*, Folder*> p(pItem->get());
	
	wstring_ptr wstrName(getTitle(pItem));
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
	const TabItem* pItem = event.getOldItem();
	std::pair<Account*, Folder*> p(pItem->get());
	
	TabCtrl_DeleteItem(pTabCtrl_->getHandle(), nItem);
	
	resetHandlers(p.second, 0);
	
	if (pTabModel_->getCount() == 0 || pTabCtrl_->isMultiline())
		layoutChildren();
}

void qm::TabWindowImpl::itemChanged(const TabModelEvent& event)
{
	int nItem = event.getItem();
	const TabItem* pOldItem = event.getOldItem();
	std::pair<Account*, Folder*> pOld(pOldItem->get());
	const TabItem* pNewItem = event.getNewItem();
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
	const TabItem* pItem = pTabModel_->getItem(nItem + nAmount);
	std::pair<Account*, Folder*> p(pItem->get());
	
	wstring_ptr wstrName(getTitle(pItem));
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

void qm::TabWindowImpl::folderRenamed(const FolderEvent& event)
{
	update(event.getFolder());
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
	if (bShowTab_ && TabCtrl_GetItemCount(pTabCtrl_->getHandle()) != 0) {
		pTabCtrl_->setWindowPos(HWND_BOTTOM, 0, 0, rect.right, rect.bottom, SWP_NOMOVE);
		TabCtrl_AdjustRect(pTabCtrl_->getHandle(), FALSE, &rect);
		pRect->top = rect.top - 4;
	}
}

void qm::TabWindowImpl::update(Folder* pFolder)
{
	assert(pFolder);
	
	int nCount = pTabModel_->getCount();
	for (int n = 0; n < nCount; ++n) {
		const TabItem* pItem = pTabModel_->getItem(n);
		if (pItem->get().second == pFolder)
			update(n);
	}
}

void qm::TabWindowImpl::update(int nItem)
{
	const TabItem* pItem = pTabModel_->getItem(nItem);
	std::pair<Account*, Folder*> p(pItem->get());
	
	wstring_ptr wstrName(getTitle(pItem));
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

wstring_ptr qm::TabWindowImpl::getTitle(const TabItem* pItem) const
{
	const WCHAR* pwszLock = pItem->isLocked() ? L"*" : L"";
	const WCHAR* pwszTitle = pItem->getTitle();
	
	std::pair<Account*, Folder*> p(pItem->get());
	if (p.first) {
		if (pwszTitle) {
			return concat(pwszTitle, pwszLock);
		}
		else {
			Account* pAccount = p.first;
			ConcatW c[] = {
				{ L"[",					1	},
				{ pAccount->getName(),	-1	},
				{ L"]",					1	},
				{ pwszLock,				-1	}
			};
			return concat(c, countof(c));
		}
	}
	else {
		Folder* pFolder = p.second;
		
		WCHAR wsz[64] = L"";
		if (bShowAllCount_ && bShowUnseenCount_)
			swprintf(wsz, L" (%d/%d)", pFolder->getUnseenCount(), pFolder->getCount());
		else if (bShowAllCount_)
			swprintf(wsz, L" (%d)", pFolder->getCount());
		else if (bShowUnseenCount_)
			swprintf(wsz, L" (%d)", pFolder->getUnseenCount());
		
		if (pwszTitle) {
			return concat(pwszTitle, wsz, pwszLock);
		}
		else {
			ConcatW c[] = {
				{ L"[",								1	},
				{ pFolder->getAccount()->getName(),	-1	},
				{ L"] ",							2	},
				{ pFolder->getName(),				-1	},
				{ wsz,								-1	},
				{ pwszLock,							-1	}
			};
			return concat(c, countof(c));
		}
	}
}

int qm::TabWindowImpl::getFolderImage(Folder* pFolder)
{
	int nImage = UIUtil::getFolderImage(pFolder, false);
	if (pFolder->getUnseenCount() != 0)
		nImage += 2;
	else if (pFolder->getCount() != 0)
		nImage += 1;
	return nImage;
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
	pImpl_->bShowTab_ = true;
	pImpl_->bShowAllCount_ = true;
	pImpl_->bShowUnseenCount_ = true;
	
	pImpl_->reloadProfiles(true);
	
	setWindowHandler(this, false);
}

qm::TabWindow::~TabWindow()
{
	delete pImpl_;
}

TabModel* qm::TabWindow::getTabModel() const
{
	return pImpl_->pTabModel_;
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

void qm::TabWindow::reloadProfiles()
{
	pImpl_->reloadProfiles(false);
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
		pContext->pDocument_, pImpl_->pTabModel_, pImpl_->pProfile_,
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
	pImpl_->bLayouting_ = false;
	
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

qm::TabCtrlWindow::TabCtrlWindow(Document* pDocument,
								 TabModel* pTabModel,
								 Profile* pProfile,
								 MenuManager* pMenuManager) :
	WindowBase(true),
	pDocument_(pDocument),
	pTabModel_(pTabModel),
	pProfile_(pProfile),
	pMenuManager_(pMenuManager),
	hfont_(0)
{
	reloadProfiles(true);
	
	setWindowHandler(this, false);
}

qm::TabCtrlWindow::~TabCtrlWindow()
{
}

bool qm::TabCtrlWindow::isMultiline() const
{
	return (getStyle() & TCS_MULTILINE) != 0;
}

void qm::TabCtrlWindow::reloadProfiles()
{
	reloadProfiles(false);
}

wstring_ptr qm::TabCtrlWindow::getSuperClass()
{
	return allocWString(WC_TABCONTROLW);
}

bool qm::TabCtrlWindow::preCreateWindow(CREATESTRUCT* pCreateStruct)
{
	if (!DefaultWindowHandler::preCreateWindow(pCreateStruct))
		return false;
	
	bool bMultiline = pProfile_->getInt(L"TabWindow", L"Multiline", 0) != 0;
	pCreateStruct->style |= TCS_TABS | TCS_FOCUSNEVER |
		(bMultiline ? TCS_MULTILINE : TCS_SINGLELINE);
	
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
#if !defined _WIN32_WCE || _WIN32_WCE >= 400
		HANDLE_MBUTTONDOWN()
		HANDLE_MBUTTONUP()
#endif
#if !defined _WIN32_WCE || _WIN32_WCE >= 211
		HANDLE_MOUSEWHEEL()
#endif
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
	
	setFont(hfont_);
	
	HIMAGELIST hImageList = ImageList_LoadImage(
		Application::getApplication().getResourceHandle(),
		MAKEINTRESOURCE(IDB_FOLDER), 16, 0, CLR_DEFAULT, IMAGE_BITMAP, 0);
	TabCtrl_SetImageList(getHandle(), hImageList);
	
	pDropTarget_.reset(new DropTarget(getHandle()));
	pDropTarget_->setDropTargetHandler(this);
	
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
	
	pDropTarget_.reset(0);
	
	return DefaultWindowHandler::onDestroy();
}

#if !defined _WIN32_WCE || _WIN32_WCE >= 400
LRESULT qm::TabCtrlWindow::onMButtonDown(UINT nFlags,
										 const POINT& pt)
{
	return 0;
}

LRESULT qm::TabCtrlWindow::onMButtonUp(UINT nFlags,
									   const POINT& pt)
{
	if (pTabModel_->getCount() > 1) {
		TCHITTESTINFO info = {
			{ pt.x, pt.y }
		};
		int nItem = TabCtrl_HitTest(getHandle(), &info);
		if (nItem != -1)
			pTabModel_->close(nItem);
	}
	
	return 0;
}
#endif

#if !defined _WIN32_WCE || _WIN32_WCE >= 211
LRESULT qm::TabCtrlWindow::onMouseWheel(UINT nFlags,
										short nDelta,
										const POINT& pt)
{
#ifdef _WIN32_WCE
#	define WHEEL_DELTA 120
#endif
	
	if (pTabModel_->getCount() != 0) {
		int nTab = nDelta/WHEEL_DELTA;
		int nItem = (pTabModel_->getCurrent() - nTab)%pTabModel_->getCount();
		if (nItem < 0)
			nItem = pTabModel_->getCount() + nItem;
		pTabModel_->setCurrent(nItem);
	}
	
	return DefaultWindowHandler::onMouseWheel(nFlags, nDelta, pt);
}
#endif

LRESULT qm::TabCtrlWindow::onDeselectTemporary(WPARAM wParam,
											   LPARAM lParam)
{
	pTabModel_->setTemporary(-1);
	return 0;
}

void qm::TabCtrlWindow::dragEnter(const DropTargetDragEvent& event)
{
	POINT pt = event.getPoint();
	screenToClient(&pt);
	ImageList_DragEnter(getHandle(), pt.x, pt.y);
	
	processDragEvent(event);
}

void qm::TabCtrlWindow::dragOver(const DropTargetDragEvent& event)
{
	processDragEvent(event);
}

void qm::TabCtrlWindow::dragExit(const DropTargetEvent& event)
{
	ImageList_DragLeave(getHandle());
}

void qm::TabCtrlWindow::drop(const DropTargetDropEvent& event)
{
	DWORD dwEffect = DROPEFFECT_NONE;
	IDataObject* pDataObject = event.getDataObject();
	if (FolderDataObject::canPasteFolder(pDataObject)) {
		std::pair<Account*, Folder*> p(FolderDataObject::get(pDataObject, pDocument_));
		if (p.first) {
			pTabModel_->open(p.first);
			dwEffect = DROPEFFECT_MOVE;
		}
		else if (p.second) {
			pTabModel_->open(p.second);
			dwEffect = DROPEFFECT_MOVE;
		}
	}
	event.setEffect(dwEffect);
	
	ImageList_DragLeave(getHandle());
}

void qm::TabCtrlWindow::processDragEvent(const DropTargetDragEvent& event)
{
	POINT pt = event.getPoint();
	screenToClient(&pt);
	
	DWORD dwEffect = DROPEFFECT_NONE;
	IDataObject* pDataObject = event.getDataObject();
	if (FolderDataObject::canPasteFolder(pDataObject))
		dwEffect = DROPEFFECT_MOVE;
	event.setEffect(dwEffect);
	
	ImageList_DragMove(pt.x, pt.y);
}

void qm::TabCtrlWindow::reloadProfiles(bool bInitialize)
{
	HFONT hfont = qs::UIUtil::createFontFromProfile(pProfile_, L"TabWindow", false);
	if (!bInitialize) {
		assert(hfont_);
		setFont(hfont);
		::DeleteObject(hfont_);
	}
	hfont_ = hfont;
	
	if (!bInitialize) {
		bool bMultiline = pProfile_->getInt(L"TabWindow", L"Multiline", 0) != 0;
		if (bMultiline != isMultiline())
			setStyle(bMultiline ? TCS_MULTILINE : TCS_SINGLELINE,
				TCS_MULTILINE | TCS_SINGLELINE);
		
		invalidate();
	}
}

#endif // QMTABWINDOW
