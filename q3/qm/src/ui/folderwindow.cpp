/*
 * $Id$
 *
 * Copyright(C) 1998-2004 Satoshi Nakamura
 * All rights reserved.
 *
 */

#include <qmaccount.h>
#include <qmapplication.h>
#include <qmdocument.h>
#include <qmfolder.h>
#include <qmfolderwindow.h>
#include <qmmessageholder.h>

#include <qsaccelerator.h>
#include <qsconv.h>
#include <qsdevicecontext.h>
#include <qsdragdrop.h>
#include <qsinit.h>
#include <qskeymap.h>
#include <qsmenu.h>
#include <qsprofile.h>
#include <qsstl.h>
#include <qsuiutil.h>

#include <algorithm>

#include <tchar.h>

#include "dialogs.h"
#include "foldermodel.h"
#include "folderwindow.h"
#include "keymap.h"
#include "resourceinc.h"
#include "uimanager.h"
#include "uiutil.h"
#include "../model/dataobject.h"

#pragma warning(disable:4786)

using namespace qm;
using namespace qs;


/****************************************************************************
 *
 * FolderWindowImpl
 *
 */

class qm::FolderWindowImpl :
	public NotifyHandler,
	public DefaultDocumentHandler,
	public DefaultAccountHandler,
	public FolderHandler,
	public FolderModelHandler,
	public DragSourceHandler,
	public DropTargetHandler
{
public:
	enum {
		WM_FOLDERWINDOW_MESSAGEADDED		= WM_APP + 1301,
		WM_FOLDERWINDOW_MESSAGEREMOVED		= WM_APP + 1302,
		WM_FOLDERWINDOW_MESSAGEREFRESHED	= WM_APP + 1303,
		WM_FOLDERWINDOW_MESSAGECHANGED		= WM_APP + 1304,
		
		WM_FOLDERWINDOW_DESELECTTEMPORARY	= WM_APP + 1310
	};

public:
	typedef std::vector<std::pair<Folder*, HTREEITEM> > FolderMap;
	typedef std::vector<HTREEITEM> ItemList;

public:
	Account* getAccount(HTREEITEM hItem) const;
	Account* getSelectedAccount() const;
	Folder* getFolder(HTREEITEM hItem) const;
	Folder* getSelectedFolder() const;
	HTREEITEM getHandleFromAccount(Account* pAccount) const;
	HTREEITEM getHandleFromFolder(Folder* pFolder) const;
	void getItems(ItemList* pList) const;
	void getDescendantItems(HTREEITEM hItem,
							ItemList* pList) const;
	void update(Folder* pFolder);
	void expand(HTREEITEM hItem,
				bool bExpand);
	void handleUpdateMessage(LPARAM lParam);

public:
	virtual LRESULT onNotify(NMHDR* pnmhdr,
							 bool* pbHandled);

public:
	virtual void offlineStatusChanged(const DocumentEvent& event);
	virtual void accountListChanged(const AccountListChangedEvent& event);
	virtual void documentInitialized(const DocumentEvent& event);

public:
	virtual void currentSubAccountChanged(const AccountEvent& event);
	virtual void folderListChanged(const FolderListChangedEvent& event);

public:
	virtual void messageAdded(const FolderMessageEvent& event);
	virtual void messageRemoved(const FolderMessageEvent& event);
	virtual void messageRefreshed(const FolderEvent& event);
	virtual void unseenCountChanged(const FolderEvent& event);
	virtual void folderDestroyed(const FolderEvent& event);

public:
	virtual void accountSelected(const FolderModelEvent& event);
	virtual void folderSelected(const FolderModelEvent& event);

public:
	virtual void dragDropEnd(const DragSourceDropEvent& event);

public:
	virtual void dragEnter(const DropTargetDragEvent& event);
	virtual void dragOver(const DropTargetDragEvent& event);
	virtual void dragExit(const DropTargetEvent& event);
	virtual void drop(const DropTargetDropEvent& event);

private:
	LRESULT onRClick(NMHDR* pnmhdr,
					 bool* pbHandled);
#if defined _WIN32_WCE && _WIN32_WCE >= 400 && defined _WIN32_WCE_PSPC
	LRESULT onRecognizeGesture(NMHDR* pnmhdr,
							   bool* pbHandled);
#endif
	LRESULT onBeginDrag(NMHDR* pnmhdr,
						bool* pbHandled);
	LRESULT onGetDispInfo(NMHDR* pnmhdr,
						  bool* pbHandled);
	LRESULT onItemExpanded(NMHDR* pnmhdr,
						   bool* pbHandled);
	LRESULT onSelChanged(NMHDR* pnmhdr,
						 bool* pbHandled);

private:
	void clearAccountList();
	void updateAccountList();
	void refreshFolderList(Account* pAccount);
	void addAccount(Account* pAccount);
	void removeAccount(Account* pAccount);
	void insertFolders(HTREEITEM hItem,
					   Account* pAccount);
	void processDragEvent(const DropTargetDragEvent& event);

private:
	static int getFolderImage(Folder* pFolder,
							  bool bSelected,
							  bool bExpanded);
	static int getAccountImage(Account* pAccount,
							   bool bSelected,
							   bool bExpanded);
	
public:
	FolderWindow* pThis_;
	WindowBase* pParentWindow_;
	FolderModel* pFolderModel_;
	MenuManager* pMenuManager_;
	Profile* pProfile_;
	std::auto_ptr<Accelerator> pAccelerator_;
	Document* pDocument_;
	
	UINT nId_;
	HFONT hfont_;
	bool bShowAllCount_;
	bool bShowUnseenCount_;
	unsigned int nDragOpenWait_;
	std::auto_ptr<DropTarget> pDropTarget_;
	
	FolderMap mapFolder_;
	
	HTREEITEM hItemDragTarget_;
	HTREEITEM hItemDragOver_;
	DWORD dwDragOverLastChangedTime_;
};

Account* qm::FolderWindowImpl::getAccount(HTREEITEM hItem) const
{
	assert(!TreeView_GetParent(pThis_->getHandle(), hItem));
	
	TVITEM item = {
		TVIF_HANDLE | TVIF_PARAM,
		hItem
	};
	if (!TreeView_GetItem(pThis_->getHandle(), &item))
		return 0;
	
	return reinterpret_cast<Account*>(item.lParam);
}

Account* qm::FolderWindowImpl::getSelectedAccount() const
{
	HTREEITEM hItem = TreeView_GetSelection(pThis_->getHandle());
	if (!hItem)
		return 0;
	
	HTREEITEM hItemParent = TreeView_GetParent(pThis_->getHandle(), hItem);
	while (hItemParent) {
		hItem = hItemParent;
		hItemParent = TreeView_GetParent(pThis_->getHandle(), hItem);
	}
	
	return getAccount(hItem);
}

Folder* qm::FolderWindowImpl::getFolder(HTREEITEM hItem) const
{
	assert(TreeView_GetParent(pThis_->getHandle(), hItem));
	
	TVITEM item = {
		TVIF_HANDLE | TVIF_PARAM,
		hItem
	};
	if (!TreeView_GetItem(pThis_->getHandle(), &item))
		return 0;
	
	return reinterpret_cast<Folder*>(item.lParam);
}

Folder* qm::FolderWindowImpl::getSelectedFolder() const
{
	HTREEITEM hItem = TreeView_GetSelection(pThis_->getHandle());
	if (!hItem)
		return 0;
	
	if (!TreeView_GetParent(pThis_->getHandle(), hItem))
		return 0;
	
	return getFolder(hItem);
}

HTREEITEM qm::FolderWindowImpl::getHandleFromAccount(Account* pAccount) const
{
	HTREEITEM hItem = TreeView_GetRoot(pThis_->getHandle());
	while (hItem) {
		if (getAccount(hItem) == pAccount)
			return hItem;
		hItem = TreeView_GetNextSibling(pThis_->getHandle(), hItem);
	}
	
	assert(false);
	
	return 0;
}

HTREEITEM qm::FolderWindowImpl::getHandleFromFolder(Folder* pFolder) const
{
	FolderMap::const_iterator it = std::find_if(
		mapFolder_.begin(), mapFolder_.end(),
		std::bind2nd(binary_compose_f_gx_hy(std::equal_to<Folder*>(),
			std::select1st<FolderMap::value_type>(),
			std::identity<Folder*>()), pFolder));
	return it != mapFolder_.end() ? (*it).second : 0;
}

void qm::FolderWindowImpl::getItems(ItemList* pList) const
{
	HWND hwnd = pThis_->getHandle();
	
	HTREEITEM hItem = TreeView_GetRoot(hwnd);
	while (hItem) {
		pList->push_back(hItem);
		getDescendantItems(hItem, pList);
		hItem = TreeView_GetNextSibling(hwnd, hItem);
	}
}

void qm::FolderWindowImpl::getDescendantItems(HTREEITEM hItem,
											  ItemList* pList) const
{
	HWND hwnd = pThis_->getHandle();
	
	hItem = TreeView_GetChild(hwnd, hItem);
	while (hItem) {
		pList->push_back(hItem);
		getDescendantItems(hItem, pList);
		hItem = TreeView_GetNextSibling(hwnd, hItem);
	}
}

void qm::FolderWindowImpl::update(Folder* pFolder)
{
	assert(pFolder);
	
	HTREEITEM hItem = getHandleFromFolder(pFolder);
	assert(hItem);
	
	HWND hwnd = pThis_->getHandle();
	
	HTREEITEM hParent = TreeView_GetParent(hwnd, hItem);
	HTREEITEM hRoot = hParent;
	while (hParent) {
		TVITEM item = {
			TVIF_HANDLE | TVIF_STATE,
			hParent,
			0,
			TVIS_EXPANDED
		};
		TreeView_GetItem(hwnd, &item);
		if (!(item.state & TVIS_EXPANDED))
			hItem = hParent;
		hParent = TreeView_GetParent(hwnd, hParent);
		if (hParent)
			hRoot = hParent;
	}
	
	HTREEITEM hItems[] = { hItem, hRoot };
	for (int n = 0; n < countof(hItems); ++n) {
		RECT rect;
		if (TreeView_GetItemRect(hwnd, hItems[n], &rect, FALSE))
			pThis_->invalidateRect(rect);
	}
}

void qm::FolderWindowImpl::expand(HTREEITEM hItem,
								  bool bExpand)
{
	if (!hItem)
		return;
	
	HWND hwnd = pThis_->getHandle();
	
	bool b = true;
	if (!bExpand) {
		HTREEITEM hSelection = TreeView_GetSelection(hwnd);
		if (hSelection) {
			HTREEITEM hParent = TreeView_GetParent(hwnd, hSelection);
			while (hParent && b) {
				if (hParent == hItem)
					b = false;
				hParent = TreeView_GetParent(hwnd, hParent);
			}
		}
	}
	if (b)
		TreeView_Expand(hwnd, hItem, bExpand ? TVE_EXPAND : TVE_COLLAPSE);
	
	expand(TreeView_GetChild(hwnd, hItem), bExpand);
	expand(TreeView_GetNextSibling(hwnd, hItem), bExpand);
}

void qm::FolderWindowImpl::handleUpdateMessage(LPARAM lParam)
{
	MSG msg;
	while (true) {
		if (!::PeekMessage(&msg, pThis_->getHandle(),
			FolderWindowImpl::WM_FOLDERWINDOW_MESSAGEADDED,
			FolderWindowImpl::WM_FOLDERWINDOW_MESSAGECHANGED, PM_NOREMOVE))
			break;
		else if (msg.lParam != lParam)
			break;
		::PeekMessage(&msg, pThis_->getHandle(),
			FolderWindowImpl::WM_FOLDERWINDOW_MESSAGEADDED,
			FolderWindowImpl::WM_FOLDERWINDOW_MESSAGECHANGED, PM_REMOVE);
	}
	
	update(reinterpret_cast<Folder*>(lParam));
}

LRESULT qm::FolderWindowImpl::onNotify(NMHDR* pnmhdr,
									   bool* pbHandled)
{
	BEGIN_NOTIFY_HANDLER()
		HANDLE_NOTIFY(NM_RCLICK, nId_, onRClick)
#if defined _WIN32_WCE && _WIN32_WCE >= 400 && defined _WIN32_WCE_PSPC
		HANDLE_NOTIFY(NM_RECOGNIZEGESTURE, nId_, onRecognizeGesture)
#endif
		HANDLE_NOTIFY(TVN_BEGINDRAG, nId_, onBeginDrag)
		HANDLE_NOTIFY(TVN_GETDISPINFO, nId_, onGetDispInfo)
		HANDLE_NOTIFY(TVN_ITEMEXPANDED, nId_, onItemExpanded)
		HANDLE_NOTIFY(TVN_SELCHANGED, nId_, onSelChanged)
	END_NOTIFY_HANDLER()
	return 1;
}

void qm::FolderWindowImpl::offlineStatusChanged(const DocumentEvent& event)
{
	UINT nState = pDocument_->isOffline() ? 0 : TVIS_BOLD;
	
	HWND hwnd = pThis_->getHandle();
	HTREEITEM hItem = TreeView_GetRoot(hwnd);
	while (hItem) {
		TVITEM item = {
			TVIF_STATE,
			hItem,
			nState,
			TVIS_BOLD
		};
		TreeView_SetItem(hwnd, &item);
		hItem = TreeView_GetNextSibling(hwnd, hItem);
	}
}

void qm::FolderWindowImpl::accountListChanged(const AccountListChangedEvent& event)
{
	switch (event.getType()) {
	case AccountListChangedEvent::TYPE_ALL:
		clearAccountList();
		updateAccountList();
		break;
	case AccountListChangedEvent::TYPE_ADD:
		addAccount(event.getAccount());
		break;
	case AccountListChangedEvent::TYPE_REMOVE:
		removeAccount(event.getAccount());
		break;
	default:
		assert(false);
		break;
	}
}

void qm::FolderWindowImpl::documentInitialized(const DocumentEvent& event)
{
	DisableRedraw disable(pThis_->getHandle());
	
	Profile::StringList listFolders;
	StringListFree<Profile::StringList> free(listFolders);
	pProfile_->getStringList(L"FolderWindow", L"ExpandedFolders", &listFolders);
	for (Profile::StringList::const_iterator it = listFolders.begin(); it != listFolders.end(); ++it) {
		std::pair<Account*, Folder*> p(UIUtil::getAccountOrFolder(pDocument_, *it));
		HTREEITEM hItem = 0;
		if (p.first)
			hItem = getHandleFromAccount(p.first);
		else if (p.second)
			hItem = getHandleFromFolder(p.second);
		if (hItem)
			TreeView_Expand(pThis_->getHandle(), hItem, TVE_EXPAND);
	}
}

void qm::FolderWindowImpl::currentSubAccountChanged(const AccountEvent& event)
{
	HTREEITEM hItem = getHandleFromAccount(event.getAccount());
	assert(hItem);
	
	RECT rect;
	if (TreeView_GetItemRect(pThis_->getHandle(), hItem, &rect, FALSE))
		pThis_->invalidateRect(rect);
}

void qm::FolderWindowImpl::folderListChanged(const FolderListChangedEvent& event)
{
	switch (event.getType()) {
	case FolderListChangedEvent::TYPE_ALL:
		refreshFolderList(event.getAccount());
		break;
	case FolderListChangedEvent::TYPE_ADD:
		// TODO
		refreshFolderList(event.getAccount());
		break;
	case FolderListChangedEvent::TYPE_REMOVE:
		// TODO
		refreshFolderList(event.getAccount());
		break;
	case FolderListChangedEvent::TYPE_RENAME:
		// TODO
		refreshFolderList(event.getAccount());
		break;
	case FolderListChangedEvent::TYPE_FLAGS:
		if ((event.getOldFlags() & (Folder::FLAG_HIDE | Folder::FLAG_BOX_MASK)) !=
			(event.getNewFlags() & (Folder::FLAG_HIDE | Folder::FLAG_BOX_MASK))) {
			// TODO
			refreshFolderList(event.getAccount());
		}
		break;
	default:
		assert(false);
		break;
	}
}

void qm::FolderWindowImpl::messageAdded(const FolderMessageEvent& event)
{
	pThis_->postMessage(WM_FOLDERWINDOW_MESSAGEADDED,
		0, reinterpret_cast<LPARAM>(event.getFolder()));
}

void qm::FolderWindowImpl::messageRemoved(const FolderMessageEvent& event)
{
	pThis_->postMessage(WM_FOLDERWINDOW_MESSAGEREMOVED,
		0, reinterpret_cast<LPARAM>(event.getFolder()));
}

void qm::FolderWindowImpl::messageRefreshed(const FolderEvent& event)
{
	pThis_->postMessage(WM_FOLDERWINDOW_MESSAGEREFRESHED,
		0, reinterpret_cast<LPARAM>(event.getFolder()));
}

void qm::FolderWindowImpl::unseenCountChanged(const FolderEvent& event)
{
	pThis_->postMessage(WM_FOLDERWINDOW_MESSAGECHANGED,
		0, reinterpret_cast<LPARAM>(event.getFolder()));
}

void qm::FolderWindowImpl::folderDestroyed(const FolderEvent& event)
{
}

void qm::FolderWindowImpl::accountSelected(const FolderModelEvent& event)
{
	Account* pAccount = event.getAccount();
	if (pAccount) {
		HTREEITEM hItem = getHandleFromAccount(pAccount);
		if (hItem != TreeView_GetSelection(pThis_->getHandle()))
			TreeView_SelectItem(pThis_->getHandle(), hItem);
	}
}

void qm::FolderWindowImpl::folderSelected(const FolderModelEvent& event)
{
	HTREEITEM hItem = getHandleFromFolder(event.getFolder());
	if (hItem != TreeView_GetSelection(pThis_->getHandle()))
		TreeView_SelectItem(pThis_->getHandle(), hItem);
}

void qm::FolderWindowImpl::dragDropEnd(const DragSourceDropEvent& event)
{
}

void qm::FolderWindowImpl::dragEnter(const DropTargetDragEvent& event)
{
	hItemDragTarget_ = 0;
	hItemDragOver_ = 0;
	dwDragOverLastChangedTime_ = 0;
	
	POINT pt = event.getPoint();
	pThis_->screenToClient(&pt);
	ImageList_DragEnter(pThis_->getHandle(), pt.x, pt.y);
	
	processDragEvent(event);
}

void qm::FolderWindowImpl::dragOver(const DropTargetDragEvent& event)
{
	processDragEvent(event);
}

void qm::FolderWindowImpl::dragExit(const DropTargetEvent& event)
{
	ImageList_DragShowNolock(FALSE);
	TreeView_SelectDropTarget(pThis_->getHandle(), 0);
	ImageList_DragShowNolock(TRUE);
	
	hItemDragTarget_ = 0;
	hItemDragOver_ = 0;
	dwDragOverLastChangedTime_ = 0;
	
	ImageList_DragLeave(pThis_->getHandle());
}

void qm::FolderWindowImpl::drop(const DropTargetDropEvent& event)
{
	ImageList_DragShowNolock(FALSE);
	TreeView_SelectDropTarget(pThis_->getHandle(), 0);
	ImageList_DragShowNolock(TRUE);
	
	hItemDragTarget_ = 0;
	hItemDragOver_ = 0;
	dwDragOverLastChangedTime_ = 0;
	
	POINT pt = event.getPoint();
	pThis_->screenToClient(&pt);
	TVHITTESTINFO info = {
		{ pt.x, pt.y },
	};
	HTREEITEM hItem = TreeView_HitTest(pThis_->getHandle(), &info);
	
	IDataObject* pDataObject = event.getDataObject();
	if (MessageDataObject::canPasteMessage(pDataObject)) {
		if (hItem && TreeView_GetParent(pThis_->getHandle(), hItem)) {
			Folder* pFolder = getFolder(hItem);
			if (pFolder->getType() == Folder::TYPE_NORMAL &&
				!pFolder->isFlag(Folder::FLAG_NOSELECT)) {
				NormalFolder* pNormalFolder = static_cast<NormalFolder*>(pFolder);
				
				MessageDataObject::Flag flag = MessageDataObject::FLAG_NONE;
				DWORD dwKeyState = event.getKeyState();
				if (dwKeyState & MK_CONTROL)
					flag = MessageDataObject::FLAG_COPY;
				else if (dwKeyState & MK_SHIFT)
					flag = MessageDataObject::FLAG_MOVE;
				else
					flag = MessageDataObject::getPasteFlag(
						pDataObject, pDocument_, pNormalFolder);
				bool bMove = flag == MessageDataObject::FLAG_MOVE;
				
				UINT nId = bMove ? IDS_MOVEMESSAGE : IDS_COPYMESSAGE;
				ProgressDialogMessageOperationCallback callback(
					pThis_->getParentFrame(), nId, nId);
				if (!MessageDataObject::pasteMessages(pDataObject,
					pDocument_, pNormalFolder, flag, &callback))
					messageBox(Application::getApplication().getResourceHandle(),
						IDS_ERROR_COPYMESSAGES, MB_OK | MB_ICONERROR, pThis_->getParentFrame());
				
				event.setEffect(bMove ? DROPEFFECT_MOVE : DROPEFFECT_COPY);
			}
		}
	}
	else if (FolderDataObject::canPasteFolder(pDataObject)) {
		if (hItem) {
			Account* pAccount = 0;
			Folder* pTarget = 0;
			if (TreeView_GetParent(pThis_->getHandle(), hItem)) {
				pTarget = getFolder(hItem);
				pAccount = pTarget->getAccount();
			}
			else {
				pAccount = getAccount(hItem);
			}
			
			Folder* pFolder = FolderDataObject::getFolder(pDataObject, pDocument_);
			if (pFolder && pFolder->getAccount() == pAccount &&
				(!pTarget || !pFolder->isAncestorOf(pTarget))) {
				if (!pAccount->moveFolder(pFolder, pTarget))
					messageBox(Application::getApplication().getResourceHandle(),
						IDS_ERROR_MOVEFOLDER, MB_OK | MB_ICONERROR, pThis_->getParentFrame());
				
				event.setEffect(DROPEFFECT_MOVE);
			}
		}
	}
	
	ImageList_DragLeave(pThis_->getHandle());
}

LRESULT qm::FolderWindowImpl::onRClick(NMHDR* pnmhdr,
									   bool* pbHandled)
{
	return pThis_->sendMessage(WM_CONTEXTMENU,
		reinterpret_cast<WPARAM>(pnmhdr->hwndFrom), ::GetMessagePos());
}

#if defined _WIN32_WCE && _WIN32_WCE >= 400 && defined _WIN32_WCE_PSPC
LRESULT qm::FolderWindowImpl::onRecognizeGesture(NMHDR* pnmhdr,
												 bool* pbHandled)
{
	*pbHandled = true;
	return TRUE;
}
#endif

LRESULT qm::FolderWindowImpl::onBeginDrag(NMHDR* pnmhdr,
										  bool* pbHandled)
{
	NMTREEVIEW* pnmtv = reinterpret_cast<NMTREEVIEW*>(pnmhdr);
	
	HTREEITEM hItem = pnmtv->itemNew.hItem;
	if (!TreeView_GetParent(pThis_->getHandle(), hItem))
		return 0;
	
	Folder* pFolder = getFolder(hItem);
	std::auto_ptr<FolderDataObject> p(new FolderDataObject(pFolder));
	p->AddRef();
	ComPtr<IDataObject> pDataObject(p.release());
	
	HIMAGELIST hImageList = TreeView_CreateDragImage(pThis_->getHandle(), hItem);
	ImageList_BeginDrag(hImageList, 0, 0, 0);
	
	DragSource source;
	source.setDragSourceHandler(this);
	DragGestureEvent event(pThis_->getHandle(), pnmtv->ptDrag);
	source.startDrag(event, pDataObject.get(), DROPEFFECT_MOVE);
	
	ImageList_EndDrag();
	ImageList_Destroy(hImageList);
	
	return 0;
}

LRESULT qm::FolderWindowImpl::onGetDispInfo(NMHDR* pnmhdr,
											bool* pbHandled)
{
	NMTVDISPINFO* pnmtvDispInfo = reinterpret_cast<NMTVDISPINFO*>(pnmhdr);
	TVITEM& item = pnmtvDispInfo->item;
	if (TreeView_GetParent(pThis_->getHandle(), item.hItem)) {
		Folder* pFolder = reinterpret_cast<Folder*>(item.lParam);
		if (item.mask & TVIF_IMAGE)
			item.iImage = getFolderImage(pFolder,
				false, (item.state & TVIS_EXPANDED) != 0);
		if (item.mask & TVIF_SELECTEDIMAGE)
			item.iSelectedImage = getFolderImage(pFolder,
				true, (item.state & TVIS_EXPANDED) != 0);
		if (item.mask & TVIF_TEXT) {
			WCHAR wsz[64] = L"";
			if (bShowAllCount_ && bShowUnseenCount_)
				swprintf(wsz, L" (%d/%d)",
					pFolder->getUnseenCount(), pFolder->getCount());
			else if (bShowAllCount_)
				swprintf(wsz, L" (%d)", pFolder->getCount());
			else if (bShowUnseenCount_)
				swprintf(wsz, L" (%d)", pFolder->getUnseenCount());
			
			wstring_ptr wstrText(concat(pFolder->getName(), wsz));
			W2T(wstrText.get(), ptszText);
			_tcsncpy(item.pszText, ptszText, item.cchTextMax);
		}
	}
	else {
		Account* pAccount = reinterpret_cast<Account*>(item.lParam);
		if (item.mask & TVIF_IMAGE)
			item.iImage = getAccountImage(pAccount,
				false, (item.state & TVIS_EXPANDED) != 0);
		if (item.mask & TVIF_SELECTEDIMAGE)
			item.iSelectedImage = getAccountImage(pAccount,
				true, (item.state & TVIS_EXPANDED) != 0);
		if (item.mask & TVIF_TEXT) {
			StringBuffer<WSTRING> buf;
			buf.append(pAccount->getName());
			
			SubAccount* pSubAccount = pAccount->getCurrentSubAccount();
			if (*pSubAccount->getName()) {
				buf.append(L" [");
				buf.append(pSubAccount->getName());
				buf.append(L"]");
			}
			W2T(buf.getCharArray(), ptszText);
			_tcsncpy(item.pszText, ptszText, item.cchTextMax);
		}
	}
	
	return 0;
}

LRESULT qm::FolderWindowImpl::onItemExpanded(NMHDR* pnmhdr,
											 bool* pbHandled)
{
	NMTREEVIEW* pnmtv = reinterpret_cast<NMTREEVIEW*>(pnmhdr);
	
	RECT rect;
	if (TreeView_GetItemRect(pThis_->getHandle(),
		pnmtv->itemNew.hItem, &rect, FALSE))
		pThis_->invalidateRect(rect);
	
	return 0;
}

LRESULT qm::FolderWindowImpl::onSelChanged(NMHDR* pnmhdr,
										   bool* pbHandled)
{
	NMTREEVIEW* pnmtv = reinterpret_cast<NMTREEVIEW*>(pnmhdr);
	bool bDelay = (pnmtv->action & TVC_BYKEYBOARD) != 0;
	
	Folder* pFolder = getSelectedFolder();
	if (pFolder)
		pFolderModel_->setCurrent(0, pFolder, bDelay);
	else
		pFolderModel_->setCurrent(getSelectedAccount(), 0, bDelay);
	
	return 0;
}

void qm::FolderWindowImpl::clearAccountList()
{
	for (FolderMap::iterator it = mapFolder_.begin(); it != mapFolder_.end(); ++it)
		(*it).first->removeFolderHandler(this);
	
	HTREEITEM hItem = TreeView_GetRoot(pThis_->getHandle());
	while (hItem) {
		Account* pAccount = getAccount(hItem);
		if (pAccount)
			pAccount->removeAccountHandler(this);
		
		hItem = TreeView_GetNextSibling(pThis_->getHandle(), hItem);
	}
	
	mapFolder_.clear();
	TreeView_DeleteAllItems(pThis_->getHandle());
}

void qm::FolderWindowImpl::updateAccountList()
{
	const Document::AccountList& l = pDocument_->getAccounts();
	for (Document::AccountList::const_iterator it = l.begin(); it != l.end(); ++it)
		addAccount(*it);
}

void qm::FolderWindowImpl::refreshFolderList(Account* pAccount)
{
	for (FolderMap::iterator it = mapFolder_.begin(); it != mapFolder_.end(); ) {
		if ((*it).first->getAccount() == pAccount) {
			(*it).first->removeFolderHandler(this);
			it = mapFolder_.erase(it);
		}
		else {
			++it;
		}
	}
	
	HTREEITEM hItem = getHandleFromAccount(pAccount);
	assert(hItem);
	
	TreeView_Expand(pThis_->getHandle(), hItem, TVE_COLLAPSE | TVE_COLLAPSERESET);
	
	insertFolders(hItem, pAccount);
	
	TreeView_Expand(pThis_->getHandle(), hItem, TVE_EXPAND);
	
	std::pair<Account*, Folder*> p(pFolderModel_->getCurrent());
	if (p.first)
		TreeView_SelectItem(pThis_->getHandle(), getHandleFromAccount(p.first));
	else if (p.second)
		TreeView_SelectItem(pThis_->getHandle(), getHandleFromFolder(p.second));
}

void qm::FolderWindowImpl::addAccount(Account* pAccount)
{
	TVINSERTSTRUCT tvisAccount = {
		TVI_ROOT,
		TVI_SORT,
		{
			TVIF_TEXT | TVIF_PARAM | TVIF_IMAGE | TVIF_SELECTEDIMAGE | TVIF_STATE,
			0,
			pDocument_->isOffline() ? 0 : TVIS_BOLD,
			TVIS_BOLD,
			LPSTR_TEXTCALLBACK,
			0,
			I_IMAGECALLBACK,
			I_IMAGECALLBACK,
			0,
			reinterpret_cast<LPARAM>(pAccount)
		}
	};
	HTREEITEM hItemAccount = TreeView_InsertItem(
		pThis_->getHandle(), &tvisAccount);
	if (!hItemAccount)
		return;
	
	insertFolders(hItemAccount, pAccount);
	
	pAccount->addAccountHandler(this);
}

void qm::FolderWindowImpl::removeAccount(Account* pAccount)
{
	HTREEITEM hItem = getHandleFromAccount(pAccount);
	assert(hItem);
	TreeView_DeleteItem(pThis_->getHandle(), hItem);
	TreeView_SelectItem(pThis_->getHandle(), 0);
}

void qm::FolderWindowImpl::insertFolders(HTREEITEM hItem,
										 Account* pAccount)
{
	assert(hItem);
	assert(pAccount);
	
	DisableRedraw disable(pThis_->getHandle());
	
	const Account::FolderList& l = pAccount->getFolders();
	Account::FolderList listFolder;
	listFolder.reserve(l.size());
	std::remove_copy_if(l.begin(), l.end(),
		std::back_inserter(listFolder), std::mem_fun(&Folder::isHidden));
	std::sort(listFolder.begin(), listFolder.end(), FolderLess());
	
	for (Account::FolderList::const_iterator it = listFolder.begin(); it != listFolder.end(); ++it) {
		Folder* pFolder = *it;
		
		TVINSERTSTRUCT tvisFolder = {
			hItem,
			TVI_LAST,
			{
				TVIF_TEXT | TVIF_PARAM | TVIF_IMAGE | TVIF_SELECTEDIMAGE,
				0,
				0,
				0,
				LPSTR_TEXTCALLBACK,
				0,
				I_IMAGECALLBACK,
				I_IMAGECALLBACK,
				0,
				reinterpret_cast<LPARAM>(pFolder)
			}
		};
		Folder* pParentFolder = pFolder->getParentFolder();
		if (pParentFolder) {
			tvisFolder.hParent = getHandleFromFolder(pParentFolder);
			assert(tvisFolder.hParent);
		}
		
		HTREEITEM hItemFolder = TreeView_InsertItem(
			pThis_->getHandle(), &tvisFolder);
		if (!hItemFolder)
			return;
		mapFolder_.push_back(std::make_pair(pFolder, hItemFolder));
		pFolder->addFolderHandler(this);
	}
}

void qm::FolderWindowImpl::processDragEvent(const DropTargetDragEvent& event)
{
	POINT pt = event.getPoint();
	pThis_->screenToClient(&pt);
	TVHITTESTINFO info = {
		{ pt.x, pt.y },
	};
	HTREEITEM hItem = TreeView_HitTest(pThis_->getHandle(), &info);
	HTREEITEM hSelectItem = 0;
	
	DWORD dwEffect = DROPEFFECT_NONE;
	
	IDataObject* pDataObject = event.getDataObject();
	if (MessageDataObject::canPasteMessage(pDataObject)) {
		if (hItem && TreeView_GetParent(pThis_->getHandle(), hItem)) {
			Folder* pFolder = getFolder(hItem);
			if (pFolder->getType() == Folder::TYPE_NORMAL &&
				!pFolder->isFlag(Folder::FLAG_NOSELECT)) {
				DWORD dwKeyState = event.getKeyState();
				if (dwKeyState & MK_CONTROL) {
					dwEffect = DROPEFFECT_COPY;
				}
				else if (dwKeyState & MK_SHIFT) {
					dwEffect = DROPEFFECT_MOVE;
				}
				else {
					MessageDataObject::Flag flag =
						MessageDataObject::getPasteFlag(pDataObject,
							pDocument_, static_cast<NormalFolder*>(pFolder));
					dwEffect = flag == MessageDataObject::FLAG_COPY ?
						DROPEFFECT_COPY : DROPEFFECT_MOVE;
				}
				
				hSelectItem = hItem;
			}
		}
	}
	else if (FolderDataObject::canPasteFolder(pDataObject)) {
		if (hItem) {
			Account* pAccount = 0;
			Folder* pTarget = 0;
			if (TreeView_GetParent(pThis_->getHandle(), hItem)) {
				pTarget = getFolder(hItem);
				pAccount = pTarget->getAccount();
			}
			else {
				pAccount = getAccount(hItem);
			}
			
			Folder* pFolder = FolderDataObject::getFolder(pDataObject, pDocument_);
			if (pFolder && pFolder->getAccount() == pAccount &&
				(!pTarget || !pFolder->isAncestorOf(pTarget)))
				dwEffect = DROPEFFECT_MOVE;
		}
		
		hSelectItem = hItem;
	}
	event.setEffect(dwEffect);
	
	{
		struct ShowLock
		{
			ShowLock() :
				bLock_(false)
			{
			}
			
			~ShowLock()
			{
				if (bLock_)
					ImageList_DragShowNolock(TRUE);
			}
			
			void lock() {
				if (!bLock_)
					ImageList_DragShowNolock(FALSE);
				bLock_ = true;
			}
			
			bool bLock_;
		} lock;
		
		if (hSelectItem != hItemDragTarget_) {
			lock.lock();
			TreeView_SelectDropTarget(pThis_->getHandle(), hSelectItem);
			hItemDragTarget_ = hSelectItem;
		}
		
		RECT rect;
		pThis_->getClientRect(&rect);
		
		int nVScroll = -1;
		if (pt.y < rect.top + 30)
			nVScroll = SB_LINEUP;
		else if (pt.y > rect.bottom - 30)
			nVScroll = SB_LINEDOWN;
		if (nVScroll != -1) {
			lock.lock();
			pThis_->sendMessage(WM_VSCROLL, MAKEWPARAM(nVScroll, 0), 0);
		}
		
		int nHScroll = -1;
		if (pt.x < rect.left + 30)
			nHScroll = SB_LEFT;
		else if (pt.x > rect.right - 30)
			nHScroll = SB_RIGHT;
		if (nHScroll != -1) {
			lock.lock();
			pThis_->sendMessage(WM_HSCROLL, MAKEWPARAM(nHScroll, 0), 0);
		}
		
		if (hItem && (info.flags & TVHT_ONITEMBUTTON || info.flags & TVHT_ONITEMICON)) {
			if (hItemDragOver_ != hItem) {
				hItemDragOver_ = hItem;
				dwDragOverLastChangedTime_ = ::GetTickCount();
			}
			else if (dwDragOverLastChangedTime_ != -1 &&
				::GetTickCount() - dwDragOverLastChangedTime_ > nDragOpenWait_) {
				lock.lock();
				TreeView_Expand(pThis_->getHandle(), hItem, TVE_TOGGLE);
				dwDragOverLastChangedTime_ = -1;
			}
		}
		else {
			hItemDragOver_ = 0;
			dwDragOverLastChangedTime_ = 0;
		}
	}
	
	ImageList_DragMove(pt.x, pt.y);
}

int qm::FolderWindowImpl::getFolderImage(Folder* pFolder,
										 bool bSelected,
										 bool bExpanded)
{
	int nImage = UIUtil::getFolderImage(pFolder, bSelected);
	bool bUnseen = false;
	if (bExpanded) {
		bUnseen = pFolder->getUnseenCount() != 0;
	}
	else {
		const Account::FolderList& l = pFolder->getAccount()->getFolders();
		for (Account::FolderList::const_iterator it = l.begin(); it != l.end() && !bUnseen; ++it) {
			if (pFolder == *it || pFolder->isAncestorOf(*it))
				bUnseen = (*it)->getUnseenCount() != 0;
		}
	}
	
	return bUnseen ? nImage + 1 : nImage;
}

int qm::FolderWindowImpl::getAccountImage(Account* pAccount,
										  bool bSelected,
										  bool bExpanded)
{
	bool bUnseen = false;
	
	const unsigned int nIgnore =
		(Folder::FLAG_BOX_MASK & ~Folder::FLAG_INBOX) |
		Folder::FLAG_IGNOREUNSEEN;
	
	const Account::FolderList& l = pAccount->getFolders();
	for (Account::FolderList::const_iterator it = l.begin(); it != l.end() && !bUnseen; ++it) {
		Folder* pFolder = *it;
		if (!(pFolder->getFlags() & nIgnore))
			bUnseen = pFolder->getUnseenCount() != 0;
	}
	
	return bUnseen ? 1 : 0;
}


/****************************************************************************
 *
 * FolderWindow
 *
 */

qm::FolderWindow::FolderWindow(WindowBase* pParentWindow,
							   FolderModel* pFolderModel,
							   Profile* pProfile) :
	WindowBase(true),
	pImpl_(0)
{
	pImpl_ = new FolderWindowImpl();
	pImpl_->pThis_ = this;
	pImpl_->pParentWindow_ = pParentWindow;
	pImpl_->pFolderModel_ = pFolderModel;
	pImpl_->pMenuManager_ = 0;
	pImpl_->pProfile_ = pProfile;
	pImpl_->pDocument_ = 0;
	pImpl_->nId_ = 0;
	pImpl_->hfont_ = 0;
	pImpl_->bShowAllCount_ = pProfile->getInt(L"FolderWindow", L"ShowAllCount", 1) != 0;
	pImpl_->bShowUnseenCount_ = pProfile->getInt(L"FolderWindow", L"ShowUnseenCount", 1) != 0;
	pImpl_->nDragOpenWait_ = pProfile->getInt(L"FolderWindow", L"DragOpenWait", 500);
	pImpl_->hItemDragTarget_ = 0;
	pImpl_->hItemDragOver_ = 0;
	pImpl_->dwDragOverLastChangedTime_ = -1;
	
	setWindowHandler(this, false);
	
	pParentWindow->addNotifyHandler(pImpl_);
	pFolderModel->addFolderModelHandler(pImpl_);
}

qm::FolderWindow::~FolderWindow()
{
	delete pImpl_;
}

bool qm::FolderWindow::save()
{
	HWND hwnd = getHandle();
	
	FolderWindowImpl::ItemList listItem;
	pImpl_->getItems(&listItem);
	
	Profile::StringList listValue;
	StringListFree<Profile::StringList> free(listValue);
	for (FolderWindowImpl::ItemList::const_iterator it = listItem.begin(); it != listItem.end(); ++it) {
		HTREEITEM hItem = *it;
		TVITEM item = {
			TVIF_HANDLE | TVIF_STATE,
			hItem,
			0,
			TVIS_EXPANDED
		};
		TreeView_GetItem(hwnd, &item);
		if (item.state & TVIS_EXPANDED) {
			wstring_ptr wstr;
			if (TreeView_GetParent(hwnd, hItem))
				wstr = UIUtil::formatFolder(pImpl_->getFolder(hItem));
			else
				wstr = UIUtil::formatAccount(pImpl_->getAccount(hItem));
			listValue.push_back(wstr.release());
		}
	}
	
	pImpl_->pProfile_->setStringList(L"FolderWindow", L"ExpandedFolders", listValue);
	
	return true;
}

void qm::FolderWindow::expand(bool bExpand)
{
	pImpl_->expand(TreeView_GetRoot(getHandle()), bExpand);
}

wstring_ptr qm::FolderWindow::getSuperClass()
{
	return allocWString(WC_TREEVIEWW);
}

bool qm::FolderWindow::preCreateWindow(CREATESTRUCT* pCreateStruct)
{
	if (!DefaultWindowHandler::preCreateWindow(pCreateStruct))
		return false;
	pCreateStruct->style |= TVS_HASBUTTONS | TVS_HASLINES | TVS_SHOWSELALWAYS;
	return true;
}

Accelerator* qm::FolderWindow::getAccelerator()
{
	return pImpl_->pAccelerator_.get();
}

LRESULT qm::FolderWindow::windowProc(UINT uMsg,
									 WPARAM wParam,
									 LPARAM lParam)
{
	BEGIN_MESSAGE_HANDLER()
		HANDLE_CONTEXTMENU()
		HANDLE_CREATE()
		HANDLE_DESTROY()
		HANDLE_LBUTTONDOWN()
		HANDLE_MESSAGE(FolderWindowImpl::WM_FOLDERWINDOW_MESSAGEADDED, onMessageAdded)
		HANDLE_MESSAGE(FolderWindowImpl::WM_FOLDERWINDOW_MESSAGEREMOVED, onMessageRemoved)
		HANDLE_MESSAGE(FolderWindowImpl::WM_FOLDERWINDOW_MESSAGEREFRESHED, onMessageRefreshed)
		HANDLE_MESSAGE(FolderWindowImpl::WM_FOLDERWINDOW_MESSAGECHANGED, onMessageChanged)
		HANDLE_MESSAGE(FolderWindowImpl::WM_FOLDERWINDOW_DESELECTTEMPORARY, onDeselectTemporary)
	END_MESSAGE_HANDLER()
	return DefaultWindowHandler::windowProc(uMsg, wParam, lParam);
}

LRESULT qm::FolderWindow::onContextMenu(HWND hwnd,
										const POINT& pt)
{
	HMENU hmenu = pImpl_->pMenuManager_->getMenu(L"folder", false, false);
	if (hmenu) {
		TVHITTESTINFO info = {
			{ pt.x, pt.y },
		};
		screenToClient(&info.pt);
		HTREEITEM hItem = TreeView_HitTest(getHandle(), &info);
		if (hItem) {
			if (TreeView_GetParent(getHandle(), hItem))
				pImpl_->pFolderModel_->setTemporary(0, pImpl_->getFolder(hItem));
			else
				pImpl_->pFolderModel_->setTemporary(pImpl_->getAccount(hItem), 0);
		}
		
		struct TemporaryDeselector
		{
			TemporaryDeselector(HWND hwnd) : hwnd_(hwnd) {}
			~TemporaryDeselector() { Window(hwnd_).postMessage(FolderWindowImpl::WM_FOLDERWINDOW_DESELECTTEMPORARY); }
			HWND hwnd_;
		} deselector(getHandle());
		
		UINT nFlags = TPM_LEFTALIGN | TPM_TOPALIGN;
#ifndef _WIN32_WCE
		nFlags |= TPM_LEFTBUTTON | TPM_RIGHTBUTTON;
#endif
		::TrackPopupMenu(hmenu, nFlags, pt.x, pt.y, 0, getParentFrame(), 0);
	}
	
	return DefaultWindowHandler::onContextMenu(hwnd, pt);
}

LRESULT qm::FolderWindow::onCreate(CREATESTRUCT* pCreateStruct)
{
	if (DefaultWindowHandler::onCreate(pCreateStruct) == -1)
		return -1;
	
	FolderWindowCreateContext* pContext =
		static_cast<FolderWindowCreateContext*>(pCreateStruct->lpCreateParams);
	pImpl_->pDocument_ = pContext->pDocument_;
	pImpl_->pMenuManager_ = pContext->pUIManager_->getMenuManager();
	pImpl_->pDocument_->addDocumentHandler(pImpl_);
	
	CustomAcceleratorFactory acceleratorFactory;
	pImpl_->pAccelerator_ = pContext->pUIManager_->getKeyMap()->createAccelerator(
		&acceleratorFactory, L"FolderWindow", mapKeyNameToId, countof(mapKeyNameToId));
	if (!pImpl_->pAccelerator_.get())
		return -1;
	
	pImpl_->nId_ = getWindowLong(GWL_ID);
	
	pImpl_->hfont_ = qs::UIUtil::createFontFromProfile(
		pImpl_->pProfile_, L"FolderWindow", false);
	setFont(pImpl_->hfont_);
	
	HIMAGELIST hImageList = ImageList_LoadImage(
		Application::getApplication().getResourceHandle(),
		MAKEINTRESOURCE(IDB_FOLDER), 16, 0, CLR_DEFAULT, IMAGE_BITMAP, 0);
	TreeView_SetImageList(getHandle(), hImageList, TVSIL_NORMAL);
	
	pImpl_->pDropTarget_.reset(new DropTarget(getHandle()));
	pImpl_->pDropTarget_->setDropTargetHandler(pImpl_);
	
	return 0;
}

LRESULT qm::FolderWindow::onDestroy()
{
	if (pImpl_->hfont_) {
		::DeleteObject(pImpl_->hfont_);
		pImpl_->hfont_ = 0;
	}
	
	HIMAGELIST hImageList = TreeView_SetImageList(getHandle(), 0, TVSIL_NORMAL);
	ImageList_Destroy(hImageList);
	
	pImpl_->pParentWindow_->removeNotifyHandler(pImpl_);
	pImpl_->pFolderModel_->removeFolderModelHandler(pImpl_);
	
	pImpl_->pDropTarget_.reset(0);
	
	return DefaultWindowHandler::onDestroy();
}

LRESULT qm::FolderWindow::onLButtonDown(UINT nFlags,
										const POINT& pt)
{
#if defined _WIN32_WCE && _WIN32_WCE >= 300 && defined _WIN32_WCE_PSPC
	if (tapAndHold(pt))
		return 0;
#endif
	return DefaultWindowHandler::onLButtonDown(nFlags, pt);
}

LRESULT qm::FolderWindow::onMessageAdded(WPARAM wParam,
										 LPARAM lParam)
{
	pImpl_->handleUpdateMessage(lParam);
	return 0;
}

LRESULT qm::FolderWindow::onMessageRemoved(WPARAM wParam,
										   LPARAM lParam)
{
	pImpl_->handleUpdateMessage(lParam);
	return 0;
}

LRESULT qm::FolderWindow::onMessageRefreshed(WPARAM wParam,
											 LPARAM lParam)
{
	pImpl_->handleUpdateMessage(lParam);
	return 0;
}

LRESULT qm::FolderWindow::onMessageChanged(WPARAM wParam,
										   LPARAM lParam)
{
	pImpl_->handleUpdateMessage(lParam);
	return 0;
}

LRESULT qm::FolderWindow::onDeselectTemporary(WPARAM wParam,
											  LPARAM lParam)
{
	pImpl_->pFolderModel_->setTemporary(0, 0);
	return 0;
}

bool qm::FolderWindow::isShow() const
{
	return (getStyle() & WS_VISIBLE) != 0;
}

bool qm::FolderWindow::isActive() const
{
	return hasFocus();
}

void qm::FolderWindow::setActive()
{
	setFocus();
}
