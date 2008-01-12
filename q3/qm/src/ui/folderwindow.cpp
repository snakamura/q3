/*
 * $Id$
 *
 * Copyright(C) 1998-2008 Satoshi Nakamura
 *
 */

#pragma warning(disable:4786)

#include <qmaccount.h>
#include <qmapplication.h>
#include <qmfilenames.h>
#include <qmfolder.h>
#include <qmfolderwindow.h>
#include <qmmessageholder.h>

#include <qsaccelerator.h>
#include <qsconv.h>
#include <qsdevicecontext.h>
#include <qsdragdrop.h>
#include <qsinit.h>
#include <qsmenu.h>
#include <qsprofile.h>
#include <qsstl.h>
#include <qsuiutil.h>

#include <algorithm>

#include <tchar.h>

#include "dialogs.h"
#include "folderimage.h"
#include "folderwindow.h"
#include "resourceinc.h"
#include "uimanager.h"
#include "uiutil.h"
#include "../action/action.h"
#include "../model/dataobject.h"
#include "../sync/syncmanager.h"
#include "../uimodel/foldermodel.h"
#include "../util/util.h"

using namespace qm;
using namespace qs;


/****************************************************************************
 *
 * FolderWindowImpl
 *
 */

class qm::FolderWindowImpl :
	public NotifyHandler,
	public DefaultAccountManagerHandler,
	public DefaultAccountHandler,
	public DefaultFolderHandler,
	public FolderModelHandler,
	public DragSourceHandler,
	public DropTargetHandler
{
public:
	enum Flag {
		FLAG_FOLDERSHOWALLCOUNT		= 0x01,
		FLAG_FOLDERSHOWUNSEENCOUNT	= 0x02,
		FLAG_FOLDER_MASK			= 0x0f,
		FLAG_ACCOUNTSHOWALLCOUNT	= 0x10,
		FLAG_ACCOUNTSHOWUNSEENCOUNT	= 0x20,
		FLAG_ACCOUNT_MASK			= 0xf0
	};
	
	enum {
		WM_FOLDERWINDOW_MESSAGEADDED		= WM_APP + 1301,
		WM_FOLDERWINDOW_MESSAGEREMOVED		= WM_APP + 1302,
		WM_FOLDERWINDOW_MESSAGEREFRESHED	= WM_APP + 1303,
		WM_FOLDERWINDOW_MESSAGECHANGED		= WM_APP + 1304,
		
		WM_FOLDERWINDOW_DESELECTTEMPORARY	= WM_APP + 1310
	};
	
	enum {
		DRAGSCROLL_MARGIN		= 30,
		DRAGSCROLL_DELAYTIME	= 300
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
	void clearFolderMap(Account* pAccount);
	void getItems(ItemList* pList) const;
	void getDescendantItems(HTREEITEM hItem,
							ItemList* pList) const;
	void update(Folder* pFolder);
	void expand(HTREEITEM hItem,
				bool bExpand);
	void postUpdateMessage(UINT uMsg,
						   Folder* pFolder);
	void handleUpdateMessage(LPARAM lParam);
	void reloadProfiles(bool bInitialize);

public:
	virtual LRESULT onNotify(NMHDR* pnmhdr,
							 bool* pbHandled);

public:
	virtual void accountListChanged(const AccountManagerEvent& event);
	virtual void accountManagerInitialized(const AccountManagerEvent& event);

public:
	virtual void currentSubAccountChanged(const AccountEvent& event);
	virtual void folderListChanged(const FolderListChangedEvent& event);

public:
	virtual void messageAdded(const FolderMessageEvent& event);
	virtual void messageRemoved(const FolderMessageEvent& event);
	virtual void messageRefreshed(const FolderEvent& event);
	virtual void unseenCountChanged(const FolderEvent& event);

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
#if defined _WIN32_WCE && _WIN32_WCE >= 0x400 && defined _WIN32_WCE_PSPC
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
	void insertFolder(Folder* pFolder,
					  bool bRecursive);
	void removeFolder(Folder* pFolder,
					  bool bRecursive);
	void sortFolders(Folder* pFolder);
	void update(HTREEITEM hItem);
	void processDragEvent(const DropTargetDragEvent& event);
	int getFolderImage(const Folder* pFolder,
					   bool bSelected,
					   bool bExpanded) const;
	int getAccountImage(const Account* pAccount,
						bool bSelected,
						bool bExpanded) const;

private:
	static bool isUnseen(const Folder* pFolder,
						 bool bExpanded);
	static bool isUnseen(const Account* pAccount);

public:
	FolderWindow* pThis_;
	WindowBase* pParentWindow_;
	FolderModel* pFolderModel_;
	MenuManager* pMenuManager_;
	Profile* pProfile_;
	std::auto_ptr<Accelerator> pAccelerator_;
	AccountManager* pAccountManager_;
	const URIResolver* pURIResolver_;
	UndoManager* pUndoManager_;
	const FolderImage* pFolderImage_;
	SyncManager* pSyncManager_;
	
	UINT nId_;
	HFONT hfont_;
#ifndef _WIN32_WCE
	bool bUseSystemColor_;
	COLORREF crForeground_;
	COLORREF crBackground_;
#endif
	unsigned int nFlags_;
	unsigned int nDragOpenWait_;
	std::auto_ptr<DropTarget> pDropTarget_;
	
	FolderMap mapFolder_;
	
	HTREEITEM hItemDragTarget_;
	HTREEITEM hItemDragOver_;
	DWORD dwDragOverLastChangedTime_;
	DWORD dwDragScrollStartTime_;
	
	Folder* volatile pUpdatingFolder_;
};

Account* qm::FolderWindowImpl::getAccount(HTREEITEM hItem) const
{
	assert(hItem);
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
	assert(hItem);
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
	assert(pAccount);
	
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
	assert(pFolder);
	
	FolderMap::const_iterator it = std::find_if(
		mapFolder_.begin(), mapFolder_.end(),
		boost::bind(&FolderMap::value_type::first, _1) == pFolder);
	assert(it != mapFolder_.end());
	return (*it).second;
}

void qm::FolderWindowImpl::clearFolderMap(Account* pAccount)
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
	
	typedef std::vector<HTREEITEM> ItemList;
	ItemList listUpdate(1, hItem);
	
	HTREEITEM hRoot = 0;
	HTREEITEM hParent = TreeView_GetParent(hwnd, hItem);
	while (hParent) {
		hRoot = hParent;
		
		TVITEM item = {
			TVIF_HANDLE | TVIF_STATE,
			hParent,
			0,
			TVIS_EXPANDED
		};
		TreeView_GetItem(hwnd, &item);
		if (!(item.state & TVIS_EXPANDED))
			listUpdate.push_back(hParent);
		hParent = TreeView_GetParent(hwnd, hParent);
	}
	listUpdate.push_back(hRoot);
	
	for (ItemList::const_iterator it = listUpdate.begin(); it != listUpdate.end(); ++it) {
		HTREEITEM hItem = *it;
		
		update(hItem);
		
		RECT rect;
		if (TreeView_GetItemRect(hwnd, hItem, &rect, FALSE))
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

void qm::FolderWindowImpl::postUpdateMessage(UINT uMsg,
											 Folder* pFolder)
{
	if (InterlockedExchangePointer(reinterpret_cast<void* volatile*>(&pUpdatingFolder_), pFolder) == pFolder)
		return;
	if (!pThis_->postMessage(uMsg, 0, reinterpret_cast<LPARAM>(pFolder)))
		InterlockedExchangePointer(reinterpret_cast<void* volatile*>(&pUpdatingFolder_), 0);
}

void qm::FolderWindowImpl::handleUpdateMessage(LPARAM lParam)
{
	InterlockedExchangePointer(reinterpret_cast<void* volatile*>(&pUpdatingFolder_), 0);
	update(reinterpret_cast<Folder*>(lParam));
}

void qm::FolderWindowImpl::reloadProfiles(bool bInitialize)
{
	unsigned int nFlags = 0;
	struct {
		const WCHAR* pwszKey_;
		Flag flag_;
	} flags[] = {
		{ L"FolderShowAllCount",		FLAG_FOLDERSHOWALLCOUNT		},
		{ L"FolderShowUnseenCount",		FLAG_FOLDERSHOWUNSEENCOUNT	},
		{ L"AccountShowAllCount",		FLAG_ACCOUNTSHOWALLCOUNT	},
		{ L"AccountShowUnseenCount",	FLAG_ACCOUNTSHOWUNSEENCOUNT	}
	};
	for (int n = 0; n < countof(flags); ++n) {
		if (pProfile_->getInt(L"FolderWindow", flags[n].pwszKey_))
			nFlags |= flags[n].flag_;
	}
	nFlags_ = nFlags;
	
	nDragOpenWait_ = pProfile_->getInt(L"FolderWindow", L"DragOpenWait");
	
	HFONT hfont = qs::UIUtil::createFontFromProfile(pProfile_,
		L"FolderWindow", qs::UIUtil::DEFAULTFONT_UI);
	if (!bInitialize) {
		assert(hfont_);
		pThis_->setFont(hfont);
		::DeleteObject(hfont_);
	}
	hfont_ = hfont;
	
#ifndef _WIN32_WCE
	bool bUseSystemColor = pProfile_->getInt(L"FolderWindow", L"UseSystemColor") != 0;
	if (!bUseSystemColor) {
		struct {
			const WCHAR* pwszKey_;
			COLORREF* pcr_;
		} colors[] = {
			{ L"ForegroundColor",	&crForeground_	},
			{ L"BackgroundColor",	&crBackground_	}
		};
		for (int n = 0; n < countof(colors); ++n) {
			wstring_ptr wstr(pProfile_->getString(L"FolderWindow", colors[n].pwszKey_));
			Color color(wstr.get());
			if (color.getColor() != 0xffffffff)
				*colors[n].pcr_ = color.getColor();
		}
	}
	if (!bInitialize) {
		if (bUseSystemColor) {
			if (!bUseSystemColor_) {
				TreeView_SetTextColor(pThis_->getHandle(), ::GetSysColor(COLOR_WINDOWTEXT));
				TreeView_SetBkColor(pThis_->getHandle(), ::GetSysColor(COLOR_WINDOW));
			}
		}
		else {
			TreeView_SetTextColor(pThis_->getHandle(), crForeground_);
			TreeView_SetBkColor(pThis_->getHandle(), crBackground_);
		}
	}
	bUseSystemColor_ = bUseSystemColor;
#endif
}

LRESULT qm::FolderWindowImpl::onNotify(NMHDR* pnmhdr,
									   bool* pbHandled)
{
	BEGIN_NOTIFY_HANDLER()
		HANDLE_NOTIFY(NM_RCLICK, nId_, onRClick)
#if defined _WIN32_WCE && _WIN32_WCE >= 0x400 && defined _WIN32_WCE_PSPC
		HANDLE_NOTIFY(NM_RECOGNIZEGESTURE, nId_, onRecognizeGesture)
#endif
		HANDLE_NOTIFY(TVN_BEGINDRAG, nId_, onBeginDrag)
		HANDLE_NOTIFY(TVN_GETDISPINFO, nId_, onGetDispInfo)
		HANDLE_NOTIFY(TVN_ITEMEXPANDED, nId_, onItemExpanded)
		HANDLE_NOTIFY(TVN_SELCHANGED, nId_, onSelChanged)
	END_NOTIFY_HANDLER()
	return NotifyHandler::onNotify(pnmhdr, pbHandled);
}

void qm::FolderWindowImpl::accountListChanged(const AccountManagerEvent& event)
{
	switch (event.getType()) {
	case AccountManagerEvent::TYPE_ALL:
		clearAccountList();
		updateAccountList();
		break;
	case AccountManagerEvent::TYPE_ADD:
		addAccount(event.getAccount());
		break;
	case AccountManagerEvent::TYPE_REMOVE:
		removeAccount(event.getAccount());
		break;
	default:
		assert(false);
		break;
	}
}

void qm::FolderWindowImpl::accountManagerInitialized(const AccountManagerEvent& event)
{
	HWND hwnd = pThis_->getHandle();
	
	DisableRedraw disable(hwnd);
	
	Profile::StringList listFolders;
	StringListFree<Profile::StringList> free(listFolders);
	pProfile_->getStringList(L"FolderWindow", L"ExpandedFolders", &listFolders);
	for (Profile::StringList::const_iterator it = listFolders.begin(); it != listFolders.end(); ++it) {
		std::pair<Account*, Folder*> p(Util::getAccountOrFolder(pAccountManager_, *it));
		HTREEITEM hItem = 0;
		if (p.first)
			hItem = getHandleFromAccount(p.first);
		else if (p.second)
			hItem = getHandleFromFolder(p.second);
		if (hItem)
			TreeView_Expand(hwnd, hItem, TVE_EXPAND);
	}
	
	TreeView_EnsureVisible(hwnd, TreeView_GetSelection(hwnd));
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
	Folder* pFolder = event.getFolder();
	
	switch (event.getType()) {
	case FolderListChangedEvent::TYPE_ALL:
		refreshFolderList(event.getAccount());
		break;
	case FolderListChangedEvent::TYPE_ADD:
		if (!pFolder->isHidden())
			insertFolder(pFolder, false);
		break;
	case FolderListChangedEvent::TYPE_REMOVE:
		if (!pFolder->isHidden())
			removeFolder(pFolder, false);
		break;
	case FolderListChangedEvent::TYPE_RENAME:
		if (!pFolder->isHidden()) {
			sortFolders(pFolder);
			update(pFolder);
		}
		break;
	case FolderListChangedEvent::TYPE_MOVE:
		if (!event.isOldHidden())
			removeFolder(pFolder, true);
		if (!event.isNewHidden())
			insertFolder(pFolder, true);
		break;
	case FolderListChangedEvent::TYPE_FLAGS:
		{
			unsigned int nOldFlags = event.getOldFlags();
			unsigned int nNewFlags = event.getNewFlags();
			if ((nOldFlags & Folder::FLAG_HIDE) && !(nNewFlags & Folder::FLAG_HIDE)) {
				if (!pFolder->isHidden()) {
					insertFolder(pFolder, true);
					TreeView_EnsureVisible(pThis_->getHandle(), getHandleFromFolder(pFolder));
				}
			}
			else if (!(nOldFlags & Folder::FLAG_HIDE) && (nNewFlags & Folder::FLAG_HIDE)) {
				Folder* pParent = pFolder->getParentFolder();
				if (!pParent || !pParent->isHidden()) {
					TreeView_EnsureVisible(pThis_->getHandle(), getHandleFromFolder(pFolder));
					removeFolder(pFolder, true);
				}
			}
			else if ((nOldFlags & Folder::FLAG_BOX_MASK) != (nNewFlags & Folder::FLAG_BOX_MASK)) {
				if (!pFolder->isHidden()) {
					sortFolders(pFolder);
					update(pFolder);
				}
			}
			else {
				if (!pFolder->isHidden())
					update(pFolder);
			}
		}
		break;
	default:
		assert(false);
		break;
	}
}

void qm::FolderWindowImpl::messageAdded(const FolderMessageEvent& event)
{
	postUpdateMessage(WM_FOLDERWINDOW_MESSAGEADDED, event.getFolder());
}

void qm::FolderWindowImpl::messageRemoved(const FolderMessageEvent& event)
{
	postUpdateMessage(WM_FOLDERWINDOW_MESSAGEREMOVED, event.getFolder());
}

void qm::FolderWindowImpl::messageRefreshed(const FolderEvent& event)
{
	postUpdateMessage(WM_FOLDERWINDOW_MESSAGEREFRESHED, event.getFolder());
}

void qm::FolderWindowImpl::unseenCountChanged(const FolderEvent& event)
{
	postUpdateMessage(WM_FOLDERWINDOW_MESSAGECHANGED, event.getFolder());
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
	dwDragOverLastChangedTime_ = -1;
	dwDragScrollStartTime_ = ::GetTickCount() + DRAGSCROLL_DELAYTIME;
	
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
	dwDragOverLastChangedTime_ = -1;
	dwDragScrollStartTime_ = -1;
	
	ImageList_DragLeave(pThis_->getHandle());
}

void qm::FolderWindowImpl::drop(const DropTargetDropEvent& event)
{
	ImageList_DragShowNolock(FALSE);
	TreeView_SelectDropTarget(pThis_->getHandle(), 0);
	ImageList_DragShowNolock(TRUE);
	
	hItemDragTarget_ = 0;
	hItemDragOver_ = 0;
	dwDragOverLastChangedTime_ = -1;
	dwDragScrollStartTime_ = -1;
	
	POINT pt = event.getPoint();
	pThis_->screenToClient(&pt);
	TVHITTESTINFO info = {
		{ pt.x, pt.y },
	};
	HTREEITEM hItem = TreeView_HitTest(pThis_->getHandle(), &info);
	
	DWORD dwEffect = DROPEFFECT_NONE;
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
						pDataObject, pAccountManager_, pNormalFolder);
				bool bMove = flag == MessageDataObject::FLAG_MOVE;
				
				UINT nId = bMove ? IDS_PROGRESS_MOVEMESSAGE : IDS_PROGRESS_COPYMESSAGE;
				ProgressDialogMessageOperationCallback callback(
					pThis_->getParentFrame(), nId, nId);
				if (!MessageDataObject::pasteMessages(pDataObject, pAccountManager_,
					pURIResolver_, pNormalFolder, flag, &callback, pUndoManager_))
					messageBox(Application::getApplication().getResourceHandle(),
						IDS_ERROR_COPYMESSAGES, MB_OK | MB_ICONERROR, pThis_->getParentFrame());
				
				dwEffect = bMove ? DROPEFFECT_MOVE : DROPEFFECT_COPY;
			}
		}
	}
	else if (!pSyncManager_->isSyncing() &&
		FolderDataObject::canPasteFolder(pDataObject)) {
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
			
			Folder* pFolder = FolderDataObject::get(pDataObject, pAccountManager_).second;
			if (pFolder && pFolder->getAccount() == pAccount &&
				(!pTarget || (pTarget != pFolder && !pFolder->isAncestorOf(pTarget) &&
					!pTarget->isFlag(Folder::FLAG_NOINFERIORS)))) {
				if (pTarget && pTarget->isFlag(Folder::FLAG_TRASHBOX)) {
					if (!FolderDeleteAction::deleteFolder(pFolderModel_, pFolder))
						messageBox(Application::getApplication().getResourceHandle(),
							IDS_ERROR_MOVEFOLDER, MB_OK | MB_ICONERROR, pThis_->getParentFrame());
				}
				else {
					if (!pAccount->moveFolder(pFolder, pTarget, 0))
						messageBox(Application::getApplication().getResourceHandle(),
							IDS_ERROR_MOVEFOLDER, MB_OK | MB_ICONERROR, pThis_->getParentFrame());
				}
				
				dwEffect = DROPEFFECT_MOVE;
			}
		}
	}
	event.setEffect(dwEffect);
	
	ImageList_DragLeave(pThis_->getHandle());
}

LRESULT qm::FolderWindowImpl::onRClick(NMHDR* pnmhdr,
									   bool* pbHandled)
{
	return pThis_->sendMessage(WM_CONTEXTMENU,
		reinterpret_cast<WPARAM>(pnmhdr->hwndFrom), ::GetMessagePos());
}

#if defined _WIN32_WCE && _WIN32_WCE >= 0x400 && defined _WIN32_WCE_PSPC
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
	
	std::auto_ptr<FolderDataObject> p;
	
	HTREEITEM hItem = pnmtv->itemNew.hItem;
	if (TreeView_GetParent(pThis_->getHandle(), hItem)) {
		Folder* pFolder = getFolder(hItem);
		p.reset(new FolderDataObject(pFolder));
	}
	else {
		Account* pAccount = getAccount(hItem);
		p.reset(new FolderDataObject(pAccount));
	}
	
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
		if ((item.mask & TVIF_IMAGE) || (item.mask & TVIF_SELECTEDIMAGE)) {
			bool bHasChild = TreeView_GetChild(pThis_->getHandle(), item.hItem) != 0;
			if (item.mask & TVIF_IMAGE)
				item.iImage = getFolderImage(pFolder, false,
					(item.state & TVIS_EXPANDED) != 0 || !bHasChild);
			if (item.mask & TVIF_SELECTEDIMAGE)
				item.iSelectedImage = getFolderImage(pFolder, true,
					(item.state & TVIS_EXPANDED) != 0 || !bHasChild);
		}
		if (item.mask & TVIF_TEXT) {
			WCHAR wsz[64] = L"";
			switch (nFlags_ & FLAG_FOLDER_MASK) {
			case FLAG_FOLDERSHOWALLCOUNT | FLAG_FOLDERSHOWUNSEENCOUNT:
				_snwprintf(wsz, countof(wsz), L" (%d/%d)", pFolder->getUnseenCount(), pFolder->getCount());
				break;
			case FLAG_FOLDERSHOWALLCOUNT:
				_snwprintf(wsz, countof(wsz), L" (%d)", pFolder->getCount());
				break;
			case FLAG_FOLDERSHOWUNSEENCOUNT:
				_snwprintf(wsz, countof(wsz), L" (%d)", pFolder->getUnseenCount());
				break;
			default:
				break;
			}
			
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
			
			WCHAR wsz[64] = L"";
			switch (nFlags_ & FLAG_ACCOUNT_MASK) {
			case FLAG_ACCOUNTSHOWALLCOUNT | FLAG_ACCOUNTSHOWUNSEENCOUNT:
				_snwprintf(wsz, countof(wsz), L" (%d/%d)", Util::getUnseenMessageCount(pAccount),
					Util::getMessageCount(pAccount));
				break;
			case FLAG_ACCOUNTSHOWALLCOUNT:
				_snwprintf(wsz, countof(wsz), L" (%d)", Util::getMessageCount(pAccount));
				break;
			case FLAG_ACCOUNTSHOWUNSEENCOUNT:
				_snwprintf(wsz, countof(wsz), L" (%d)", Util::getUnseenMessageCount(pAccount));
				break;
			default:
				break;
			}
			buf.append(wsz);
			
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
	
	HTREEITEM hItem = pnmtv->itemNew.hItem;
	
	update(hItem);
	
	RECT rect;
	if (TreeView_GetItemRect(pThis_->getHandle(), hItem, &rect, FALSE))
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
	const AccountManager::AccountList& l = pAccountManager_->getAccounts();
	for (AccountManager::AccountList::const_iterator it = l.begin(); it != l.end(); ++it)
		addAccount(*it);
}

void qm::FolderWindowImpl::refreshFolderList(Account* pAccount)
{
	clearFolderMap(pAccount);
	
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
			isUnseen(pAccount) ? TVIS_BOLD : 0,
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
	clearFolderMap(pAccount);
	pAccount->removeAccountHandler(this);
	
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
				TVIF_TEXT | TVIF_PARAM | TVIF_IMAGE | TVIF_SELECTEDIMAGE | TVIF_STATE,
				0,
				isUnseen(pFolder, false) ? TVIS_BOLD : 0,
				TVIS_BOLD,
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
		
		HTREEITEM hItemFolder = TreeView_InsertItem(pThis_->getHandle(), &tvisFolder);
		if (!hItemFolder)
			return;
		mapFolder_.push_back(std::make_pair(pFolder, hItemFolder));
		pFolder->addFolderHandler(this);
	}
}

void qm::FolderWindowImpl::insertFolder(Folder* pFolder,
										bool bRecursive)
{
	assert(pFolder);
	assert(!pFolder->isFlag(Folder::FLAG_HIDE));
	
	HWND hwnd = pThis_->getHandle();
	
	Folder* pParent = pFolder->getParentFolder();
	HTREEITEM hItemParent = pParent ? getHandleFromFolder(pParent) :
		getHandleFromAccount(pFolder->getAccount());
	assert(hItemParent);
	
	HTREEITEM hItemInsertAfter = TVI_FIRST;
	HTREEITEM hItemChild = TreeView_GetChild(hwnd, hItemParent);
	while (hItemChild) {
		Folder* pChild = getFolder(hItemChild);
		if (FolderLess::compare(pChild, pFolder) > 0)
			break;
		hItemInsertAfter = hItemChild;
		hItemChild = TreeView_GetNextSibling(hwnd, hItemChild);
	}
	
	TVINSERTSTRUCT tvisFolder = {
		hItemParent,
		hItemInsertAfter,
		{
			TVIF_TEXT | TVIF_PARAM | TVIF_IMAGE | TVIF_SELECTEDIMAGE | TVIF_STATE,
			0,
			isUnseen(pFolder, false) ? TVIS_BOLD : 0,
			TVIS_BOLD,
			LPSTR_TEXTCALLBACK,
			0,
			I_IMAGECALLBACK,
			I_IMAGECALLBACK,
			0,
			reinterpret_cast<LPARAM>(pFolder)
		}
	};
	HTREEITEM hItem = TreeView_InsertItem(hwnd, &tvisFolder);
	if (!hItem)
		return;
	mapFolder_.push_back(std::make_pair(pFolder, hItem));
	pFolder->addFolderHandler(this);
	
	if (bRecursive) {
		Account::FolderList l;
		pFolder->getAccount()->getChildFolders(pFolder, &l);
		for (Account::FolderList::const_iterator it = l.begin(); it != l.end(); ++it) {
			if (!(*it)->isFlag(Folder::FLAG_HIDE))
				insertFolder(*it, true);
		}
	}
	
	update(pFolder);
}

void qm::FolderWindowImpl::removeFolder(Folder* pFolder,
										bool bRecursive)
{
	assert(pFolder);
	
	if (bRecursive) {
		Account::FolderList l;
		pFolder->getAccount()->getChildFolders(pFolder, &l);
		for (Account::FolderList::const_iterator it = l.begin(); it != l.end(); ++it) {
			if (!(*it)->isFlag(Folder::FLAG_HIDE))
				removeFolder(*it, true);
		}
	}
	
	HWND hwnd = pThis_->getHandle();
	
	FolderMap::iterator it = std::find_if(
		mapFolder_.begin(), mapFolder_.end(),
		boost::bind(&FolderMap::value_type::first, _1) == pFolder);
	assert(it != mapFolder_.end());
	
	HTREEITEM hItem = (*it).second;
	assert(!TreeView_GetChild(hwnd, hItem));
	mapFolder_.erase(it);
	pFolder->removeFolderHandler(this);
	
	TreeView_DeleteItem(hwnd, hItem);
}

void qm::FolderWindowImpl::sortFolders(Folder* pFolder)
{
	assert(pFolder);
	
	HWND hwnd = pThis_->getHandle();
	
	struct Comparator
	{
		static int CALLBACK compare(LPARAM lParam1,
									LPARAM lParam2,
									LPARAM lParamSort)
		{
			return FolderLess::compare(reinterpret_cast<Folder*>(lParam1),
				reinterpret_cast<Folder*>(lParam2));
		}
	};
	TV_SORTCB sort = {
		TreeView_GetParent(hwnd, getHandleFromFolder(pFolder)),
		&Comparator::compare,
		0
	};
	TreeView_SortChildrenCB(hwnd, &sort, FALSE);
}

void qm::FolderWindowImpl::update(HTREEITEM hItem)
{
	assert(hItem);
	
	HWND hwnd = pThis_->getHandle();
	
	bool bUnseen = false;
	if (TreeView_GetParent(hwnd, hItem)) {
		TVITEM item = {
			TVIF_HANDLE | TVIF_STATE | TVIF_CHILDREN,
			hItem,
			0,
			TVIS_EXPANDED
		};
		TreeView_GetItem(hwnd, &item);
		bUnseen = isUnseen(getFolder(hItem),
			(item.state & TVIS_EXPANDED) != 0 || item.cChildren == 0);
	}
	else {
		bUnseen = isUnseen(getAccount(hItem));
	}
	TVITEM item = {
		TVIF_HANDLE | TVIF_STATE,
		hItem,
		bUnseen ? TVIS_BOLD : 0,
		TVIS_BOLD
	};
	TreeView_SetItem(hwnd, &item);
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
							pAccountManager_, static_cast<NormalFolder*>(pFolder));
					dwEffect = flag == MessageDataObject::FLAG_COPY ?
						DROPEFFECT_COPY : DROPEFFECT_MOVE;
				}
				
				hSelectItem = hItem;
			}
		}
	}
	else if (!pSyncManager_->isSyncing() &&
		FolderDataObject::canPasteFolder(pDataObject)) {
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
			
			Folder* pFolder = FolderDataObject::get(pDataObject, pAccountManager_).second;
			if (pFolder && pFolder->getAccount() == pAccount &&
				(!pTarget || (pTarget != pFolder && !pFolder->isAncestorOf(pTarget) &&
					!pTarget->isFlag(Folder::FLAG_NOINFERIORS))))
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
		
		if (dwDragScrollStartTime_ == -1 ||
			::GetTickCount() > dwDragScrollStartTime_) {
			RECT rect;
			pThis_->getClientRect(&rect);
			
			int nVScroll = -1;
			if (pt.y < rect.top + DRAGSCROLL_MARGIN)
				nVScroll = SB_LINEUP;
			else if (pt.y > rect.bottom - DRAGSCROLL_MARGIN)
				nVScroll = SB_LINEDOWN;
			if (nVScroll != -1) {
				lock.lock();
				pThis_->sendMessage(WM_VSCROLL, MAKEWPARAM(nVScroll, 0), 0);
			}
			
			int nHScroll = -1;
			if (pt.x < rect.left + DRAGSCROLL_MARGIN)
				nHScroll = SB_LINELEFT;
			else if (pt.x > rect.right - DRAGSCROLL_MARGIN)
				nHScroll = SB_LINERIGHT;
			if (nHScroll != -1) {
				lock.lock();
				pThis_->sendMessage(WM_HSCROLL, MAKEWPARAM(nHScroll, 0), 0);
			}
			
			dwDragScrollStartTime_ = -1;
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

int qm::FolderWindowImpl::getFolderImage(const Folder* pFolder,
										 bool bSelected,
										 bool bExpanded) const
{
	return pFolderImage_->getFolderImage(pFolder, pFolder->getCount() != 0,
		isUnseen(pFolder, bExpanded), bSelected);
}

int qm::FolderWindowImpl::getAccountImage(const Account* pAccount,
										  bool bSelected,
										  bool bExpanded) const
{
	return pFolderImage_->getAccountImage(pAccount, isUnseen(pAccount), bSelected);
}

bool qm::FolderWindowImpl::isUnseen(const Folder* pFolder,
									bool bExpanded)
{
	if (pFolder->isFlag(Folder::FLAG_TRASHBOX) ||
		pFolder->isFlag(Folder::FLAG_JUNKBOX))
		return false;
	else if (pFolder->getUnseenCount() != 0)
		return true;
	
	if (!bExpanded) {
		const unsigned int nIgnore =
			(Folder::FLAG_BOX_MASK & ~Folder::FLAG_INBOX) |
			Folder::FLAG_IGNOREUNSEEN;
		
		const Account::FolderList& l = pFolder->getAccount()->getFolders();
		for (Account::FolderList::const_iterator it = l.begin(); it != l.end(); ++it) {
			Folder* p = *it;
			if ((p->getFlags() & nIgnore) == 0 &&
				p->getUnseenCount() != 0 &&
				pFolder->isAncestorOf(p))
				return true;
		}
	}
	
	return false;
}

bool qm::FolderWindowImpl::isUnseen(const Account* pAccount)
{
	const unsigned int nIgnore =
		(Folder::FLAG_BOX_MASK & ~Folder::FLAG_INBOX) |
		Folder::FLAG_IGNOREUNSEEN;
	
	const Account::FolderList& l = pAccount->getFolders();
	Account::FolderList::const_iterator it = l.begin();
	while (it != l.end()) {
		const Folder* p = *it;
		if (((p->getFlags() & nIgnore) == 0 && p->getUnseenCount() != 0))
			break;
		++it;
	}
	return it != l.end();
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
	pImpl_->pAccountManager_ = 0;
	pImpl_->pURIResolver_ = 0;
	pImpl_->pUndoManager_ = 0;
	pImpl_->pSyncManager_ = 0;
	pImpl_->nId_ = 0;
	pImpl_->hfont_ = 0;
#ifndef _WIN32_WCE
	pImpl_->bUseSystemColor_ = true;
	pImpl_->crForeground_ = RGB(0, 0, 0);
	pImpl_->crBackground_ = RGB(255, 255, 255);
#endif
	pImpl_->nFlags_ = FolderWindowImpl::FLAG_FOLDERSHOWALLCOUNT |
		FolderWindowImpl::FLAG_FOLDERSHOWUNSEENCOUNT |
		FolderWindowImpl::FLAG_ACCOUNTSHOWALLCOUNT |
		FolderWindowImpl::FLAG_ACCOUNTSHOWUNSEENCOUNT;
	pImpl_->nDragOpenWait_ = 500;
	pImpl_->hItemDragTarget_ = 0;
	pImpl_->hItemDragOver_ = 0;
	pImpl_->dwDragOverLastChangedTime_ = -1;
	pImpl_->dwDragScrollStartTime_ = -1;
	pImpl_->pUpdatingFolder_ = 0;
	
	pImpl_->reloadProfiles(true);
	
	setWindowHandler(this, false);
	
	pParentWindow->addNotifyHandler(pImpl_);
	pFolderModel->addFolderModelHandler(pImpl_);
}

qm::FolderWindow::~FolderWindow()
{
	delete pImpl_;
}

void qm::FolderWindow::reloadProfiles()
{
	pImpl_->reloadProfiles(false);
}

void qm::FolderWindow::save() const
{
	HWND hwnd = getHandle();
	
	FolderWindowImpl::ItemList listItem;
	pImpl_->getItems(&listItem);
	
	Profile::StringList listValue;
	StringListFree<Profile::StringList> free(listValue);
	listValue.reserve(listItem.size());
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
				wstr = Util::formatFolder(pImpl_->getFolder(hItem));
			else
				wstr = Util::formatAccount(pImpl_->getAccount(hItem));
			listValue.push_back(wstr.release());
		}
	}
	
	pImpl_->pProfile_->setStringList(L"FolderWindow", L"ExpandedFolders", listValue);
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
	POINT ptMenu = UIUtil::getTreeViewContextMenuPosition(getHandle(), pt);
	HMENU hmenu = pImpl_->pMenuManager_->getMenu(L"folder", false, false);
	if (hmenu) {
		TVHITTESTINFO info = {
			{ ptMenu.x, ptMenu.y },
		};
		screenToClient(&info.pt);
		HTREEITEM hItem = TreeView_HitTest(getHandle(), &info);
		if (hItem) {
#if defined _WIN32_WCE && (_WIN32_WCE < 0x300 || !defined _WIN32_WCE_PSPC)
			TreeView_SelectDropTarget(getHandle(), hItem);
#endif
			if (TreeView_GetParent(getHandle(), hItem))
				pImpl_->pFolderModel_->setTemporary(0, pImpl_->getFolder(hItem));
			else
				pImpl_->pFolderModel_->setTemporary(pImpl_->getAccount(hItem), 0);
		}
		
		UINT nFlags = TPM_LEFTALIGN | TPM_TOPALIGN;
#ifndef _WIN32_WCE
		nFlags |= TPM_LEFTBUTTON | TPM_RIGHTBUTTON;
#endif
		::TrackPopupMenu(hmenu, nFlags, ptMenu.x, ptMenu.y, 0, getParentFrame(), 0);
		
#if defined _WIN32_WCE && (_WIN32_WCE < 0x300 || !defined _WIN32_WCE_PSPC)
		if (hItem)
			TreeView_SelectDropTarget(getHandle(), 0);
#endif
		postMessage(FolderWindowImpl::WM_FOLDERWINDOW_DESELECTTEMPORARY);
	}
	
	return 0;
}

LRESULT qm::FolderWindow::onCreate(CREATESTRUCT* pCreateStruct)
{
	if (DefaultWindowHandler::onCreate(pCreateStruct) == -1)
		return -1;
	
	FolderWindowCreateContext* pContext =
		static_cast<FolderWindowCreateContext*>(pCreateStruct->lpCreateParams);
	pImpl_->pAccountManager_ = pContext->pAccountManager_;
	pImpl_->pURIResolver_ = pContext->pURIResolver_;
	pImpl_->pUndoManager_ = pContext->pUndoManager_;
	pImpl_->pFolderImage_ = pContext->pFolderImage_;
	pImpl_->pSyncManager_ = pContext->pSyncManager_;
	pImpl_->pMenuManager_ = pContext->pUIManager_->getMenuManager();
	pImpl_->pAccountManager_->addAccountManagerHandler(pImpl_);
	
	CustomAcceleratorFactory acceleratorFactory;
	pImpl_->pAccelerator_ = pContext->pUIManager_->getKeyMap()->createAccelerator(
		&acceleratorFactory, L"FolderWindow");
	if (!pImpl_->pAccelerator_.get())
		return -1;
	
	pImpl_->nId_ = getId();
	
	setFont(pImpl_->hfont_, false);
	
#ifndef _WIN32_WCE
	if (!pImpl_->bUseSystemColor_) {
		TreeView_SetTextColor(getHandle(), pImpl_->crForeground_);
		TreeView_SetBkColor(getHandle(), pImpl_->crBackground_);
	}
#endif
	
	HIMAGELIST hImageList = ImageList_Duplicate(pImpl_->pFolderImage_->getImageList());
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
	pImpl_->pAccountManager_->removeAccountManagerHandler(pImpl_);
	
	pImpl_->pDropTarget_.reset(0);
	
	return DefaultWindowHandler::onDestroy();
}

LRESULT qm::FolderWindow::onLButtonDown(UINT nFlags,
										const POINT& pt)
{
#if defined _WIN32_WCE && (_WIN32_WCE < 0x300 || !defined _WIN32_WCE_PSPC)
	if (::GetKeyState(VK_MENU) < 0)
		return 0;
#elif defined _WIN32_WCE && _WIN32_WCE >= 0x300 && defined _WIN32_WCE_PSPC
	TVHITTESTINFO info = {
		{ pt.x, pt.y },
	};
	HTREEITEM hItem = TreeView_HitTest(getHandle(), &info);
	if (hItem)
		TreeView_SelectDropTarget(getHandle(), hItem);
	
	bool b = tapAndHold(pt);
	
	if (hItem)
		TreeView_SelectDropTarget(getHandle(), 0);
	
	if (b)
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
