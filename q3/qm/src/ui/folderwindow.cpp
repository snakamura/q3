/*
 * $Id$
 *
 * Copyright(C) 1998-2003 Satoshi Nakamura
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
#include <qserror.h>
#include <qsinit.h>
#include <qskeymap.h>
#include <qsnew.h>
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
	public DropTargetHandler
{
public:
	enum {
		WM_FOLDERWINDOW_MESSAGEADDED	= WM_APP + 1101,
		WM_FOLDERWINDOW_MESSAGEREMOVED	= WM_APP + 1102,
		WM_FOLDERWINDOW_MESSAGECHANGED	= WM_APP + 1103
	};

public:
	typedef std::vector<std::pair<Folder*, HTREEITEM> > FolderMap;

public:
	Account* getAccount(HTREEITEM hItem) const;
	Account* getSelectedAccount() const;
	Folder* getFolder(HTREEITEM hItem) const;
	Folder* getSelectedFolder() const;
	HTREEITEM getHandleFromAccount(Account* pAccount) const;
	HTREEITEM getHandleFromFolder(Folder* pFolder) const;
	QSTATUS update(Folder* pFolder);

public:
	virtual LRESULT onNotify(NMHDR* pnmhdr, bool* pbHandled);

public:
	virtual QSTATUS accountListChanged(
		const AccountListChangedEvent& event);

public:
	virtual QSTATUS folderListChanged(const FolderListChangedEvent& event);

public:
	virtual QSTATUS messageAdded(const FolderEvent& event);
	virtual QSTATUS messageRemoved(const FolderEvent& event);
	virtual QSTATUS messageChanged(const MessageEvent& event);
	virtual QSTATUS folderDestroyed(const FolderEvent& event);

public:
	virtual QSTATUS accountSelected(const FolderModelEvent& event);
	virtual QSTATUS folderSelected(const FolderModelEvent& event);

public:
	virtual QSTATUS dragEnter(const DropTargetDragEvent& event);
	virtual QSTATUS dragOver(const DropTargetDragEvent& event);
	virtual QSTATUS dragExit(const DropTargetEvent& event);
	virtual QSTATUS drop(const DropTargetDropEvent& event);

private:
	LRESULT onRClick(NMHDR* pnmhdr, bool* pbHandled);
	LRESULT onGetDispInfo(NMHDR* pnmhdr, bool* pbHandled);
	LRESULT onItemExpanded(NMHDR* pnmhdr, bool* pbHandled);
	LRESULT onSelChanged(NMHDR* pnmhdr, bool* pbHandled);

private:
	QSTATUS clearAccountList();
	QSTATUS updateAccountList();
	QSTATUS refreshFolderList(Account* pAccount);
	QSTATUS addAccount(Account* pAccount);
	QSTATUS insertFolders(HTREEITEM hItem, Account* pAccount);
	int getFolderImage(Folder* pFolder, bool bSelected, bool bExpanded) const;
	int getAccountImage(Account* pAccount, bool bSelected, bool bExpanded) const;
	
public:
	FolderWindow* pThis_;
	WindowBase* pParentWindow_;
	FolderModel* pFolderModel_;
	MenuManager* pMenuManager_;
	Profile* pProfile_;
	Accelerator* pAccelerator_;
	Document* pDocument_;
	
	UINT nId_;
	HFONT hfont_;
	bool bShowAllCount_;
	bool bShowUnseenCount_;
	DropTarget* pDropTarget_;
	
	FolderMap mapFolder_;
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

QSTATUS qm::FolderWindowImpl::update(Folder* pFolder)
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
	
	return QSTATUS_SUCCESS;
}

LRESULT qm::FolderWindowImpl::onNotify(NMHDR* pnmhdr, bool* pbHandled)
{
	BEGIN_NOTIFY_HANDLER()
		HANDLE_NOTIFY(NM_RCLICK, nId_, onRClick)
		HANDLE_NOTIFY(TVN_GETDISPINFO, nId_, onGetDispInfo)
		HANDLE_NOTIFY(TVN_ITEMEXPANDED, nId_, onItemExpanded)
		HANDLE_NOTIFY(TVN_SELCHANGED, nId_, onSelChanged)
	END_NOTIFY_HANDLER()
	return 1;
}

QSTATUS qm::FolderWindowImpl::accountListChanged(
	const AccountListChangedEvent& event)
{
	DECLARE_QSTATUS();
	
	switch (event.getType()) {
	case AccountListChangedEvent::TYPE_ALL:
		status = clearAccountList();
		CHECK_QSTATUS();
		status = updateAccountList();
		CHECK_QSTATUS();
		break;
	case AccountListChangedEvent::TYPE_ADD:
		status = addAccount(event.getAccount());
		CHECK_QSTATUS();
		break;
	case AccountListChangedEvent::TYPE_REMOVE:
		// TODO
		break;
	case AccountListChangedEvent::TYPE_RENAME:
		// TODO
		break;
	default:
		assert(false);
		return QSTATUS_FAIL;
	}
	
	return status;
}

QSTATUS qm::FolderWindowImpl::folderListChanged(
	const FolderListChangedEvent& event)
{
	DECLARE_QSTATUS();
	
	switch (event.getType()) {
	case FolderListChangedEvent::TYPE_ALL:
		status = refreshFolderList(event.getAccount());
		CHECK_QSTATUS();
		break;
	case FolderListChangedEvent::TYPE_ADD:
		// TODO
		status = refreshFolderList(event.getAccount());
		CHECK_QSTATUS();
		break;
	case FolderListChangedEvent::TYPE_REMOVE:
		// TODO
		status = refreshFolderList(event.getAccount());
		CHECK_QSTATUS();
		break;
	case FolderListChangedEvent::TYPE_RENAME:
		// TODO
		status = refreshFolderList(event.getAccount());
		CHECK_QSTATUS();
		break;
	case FolderListChangedEvent::TYPE_SHOW:
	case FolderListChangedEvent::TYPE_HIDE:
		// TODO
		status = refreshFolderList(event.getAccount());
		CHECK_QSTATUS();
		break;
	default:
		assert(false);
		return QSTATUS_FAIL;
	}
	
	return status;
}

QSTATUS qm::FolderWindowImpl::messageAdded(const FolderEvent& event)
{
	pThis_->postMessage(WM_FOLDERWINDOW_MESSAGEADDED,
		0, reinterpret_cast<LPARAM>(event.getFolder()));
	return QSTATUS_SUCCESS;
}

QSTATUS qm::FolderWindowImpl::messageRemoved(const FolderEvent& event)
{
	pThis_->postMessage(WM_FOLDERWINDOW_MESSAGEREMOVED,
		0, reinterpret_cast<LPARAM>(event.getFolder()));
	return QSTATUS_SUCCESS;
}

QSTATUS qm::FolderWindowImpl::messageChanged(const MessageEvent& event)
{
	if ((event.getOldFlags() & MessageHolder::FLAG_SEEN) !=
		(event.getNewFlags() & MessageHolder::FLAG_SEEN))
		pThis_->postMessage(WM_FOLDERWINDOW_MESSAGECHANGED,
			0, reinterpret_cast<LPARAM>(event.getFolder()));
	return QSTATUS_SUCCESS;
}

QSTATUS qm::FolderWindowImpl::folderDestroyed(const FolderEvent& event)
{
	return QSTATUS_SUCCESS;
}

QSTATUS qm::FolderWindowImpl::accountSelected(const FolderModelEvent& event)
{
	HTREEITEM hItem = getHandleFromAccount(event.getAccount());
	if (hItem != TreeView_GetSelection(pThis_->getHandle())) {
		TreeView_SelectItem(pThis_->getHandle(), hItem);
		TreeView_EnsureVisible(pThis_->getHandle(), hItem);
	}
	return QSTATUS_SUCCESS;
}

QSTATUS qm::FolderWindowImpl::folderSelected(const FolderModelEvent& event)
{
	HTREEITEM hItem = getHandleFromFolder(event.getFolder());
	if (hItem != TreeView_GetSelection(pThis_->getHandle())) {
		TreeView_SelectItem(pThis_->getHandle(), hItem);
		TreeView_EnsureVisible(pThis_->getHandle(), hItem);
	}
	return QSTATUS_SUCCESS;
}

QSTATUS qm::FolderWindowImpl::dragEnter(const DropTargetDragEvent& event)
{
	return dragOver(event);
}

QSTATUS qm::FolderWindowImpl::dragOver(const DropTargetDragEvent& event)
{
	POINT pt = event.getPoint();
	pThis_->screenToClient(&pt);
	TVHITTESTINFO info = {
		{ pt.x, pt.y },
	};
	HTREEITEM hItem = TreeView_HitTest(pThis_->getHandle(), &info);
	HTREEITEM hSelectItem = 0;
	if (hItem && TreeView_GetParent(pThis_->getHandle(), hItem)) {
		Folder* pFolder = getFolder(hItem);
		if (pFolder->getType() == Folder::TYPE_NORMAL &&
			!pFolder->isFlag(Folder::FLAG_NOSELECT)) {
			IDataObject* pDataObject = event.getDataObject();
			DWORD dwEffect = DROPEFFECT_NONE;
			if (MessageDataObject::canPasteMessage(pDataObject)) {
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
			}
			event.setEffect(dwEffect);
			
			hSelectItem = hItem;
		}
	}
	
	TreeView_SelectDropTarget(pThis_->getHandle(), hSelectItem);
	
	return QSTATUS_SUCCESS;
}

QSTATUS qm::FolderWindowImpl::dragExit(const DropTargetEvent& event)
{
	TreeView_SelectDropTarget(pThis_->getHandle(), 0);
	
	return QSTATUS_SUCCESS;
}

QSTATUS qm::FolderWindowImpl::drop(const DropTargetDropEvent& event)
{
	DECLARE_QSTATUS();
	
	TreeView_SelectDropTarget(pThis_->getHandle(), 0);
	
	POINT pt = event.getPoint();
	pThis_->screenToClient(&pt);
	TVHITTESTINFO info = {
		{ pt.x, pt.y },
	};
	HTREEITEM hItem = TreeView_HitTest(pThis_->getHandle(), &info);
	if (hItem && TreeView_GetParent(pThis_->getHandle(), hItem)) {
		Folder* pFolder = getFolder(hItem);
		if (pFolder->getType() == Folder::TYPE_NORMAL &&
			!pFolder->isFlag(Folder::FLAG_NOSELECT)) {
			NormalFolder* pNormalFolder = static_cast<NormalFolder*>(pFolder);
			
			IDataObject* pDataObject = event.getDataObject();
			
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
			status = MessageDataObject::pasteMessages(pDataObject,
				pDocument_, pNormalFolder, flag, &callback);
			CHECK_QSTATUS();
			
			event.setEffect(bMove ? DROPEFFECT_MOVE : DROPEFFECT_COPY);
		}
	}
	return QSTATUS_SUCCESS;
}

LRESULT qm::FolderWindowImpl::onRClick(NMHDR* pnmhdr, bool* pbHandled)
{
	return pThis_->sendMessage(WM_CONTEXTMENU,
		reinterpret_cast<WPARAM>(pnmhdr->hwndFrom), ::GetMessagePos());
}

LRESULT qm::FolderWindowImpl::onGetDispInfo(NMHDR* pnmhdr, bool* pbHandled)
{
	DECLARE_QSTATUS();
	
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
			
			string_ptr<WSTRING> wstrText(concat(pFolder->getName(), wsz));
			if (!wstrText.get())
				return 1;
			
			W2T_STATUS(wstrText.get(), ptszText);
			CHECK_QSTATUS_VALUE(1);
			_tcsncpy(item.pszText,
				ptszText, item.cchTextMax);
		}
	}
	else {
		Account* pAccount = reinterpret_cast<Account*>(
			item.lParam);
		if (item.mask & TVIF_IMAGE)
			item.iImage = getAccountImage(pAccount,
				false, (item.state & TVIS_EXPANDED) != 0);
		if (item.mask & TVIF_SELECTEDIMAGE)
			item.iSelectedImage = getAccountImage(pAccount,
				true, (item.state & TVIS_EXPANDED) != 0);
	}
	
	return 0;
}

LRESULT qm::FolderWindowImpl::onItemExpanded(NMHDR* pnmhdr, bool* pbHandled)
{
	NMTREEVIEW* pnmtv = reinterpret_cast<NMTREEVIEW*>(pnmhdr);
	
	RECT rect;
	if (TreeView_GetItemRect(pThis_->getHandle(),
		pnmtv->itemNew.hItem, &rect, FALSE))
		pThis_->invalidateRect(rect);
	
	return 0;
}

LRESULT qm::FolderWindowImpl::onSelChanged(NMHDR* pnmhdr, bool* pbHandled)
{
	DECLARE_QSTATUS();
	
	NMTREEVIEW* pnmtv = reinterpret_cast<NMTREEVIEW*>(pnmhdr);
	bool bDelay = (pnmtv->action & TVC_BYKEYBOARD) != 0;
	
	Folder* pFolder = getSelectedFolder();
	if (pFolder)
		status = pFolderModel_->setCurrentFolder(pFolder, bDelay);
	else
		status = pFolderModel_->setCurrentAccount(getSelectedAccount(), bDelay);
	
	return 0;
}

QSTATUS qm::FolderWindowImpl::clearAccountList()
{
	DECLARE_QSTATUS();
	
	FolderMap::iterator it = mapFolder_.begin();
	while (it != mapFolder_.end()) {
		status = (*it).first->removeFolderHandler(this);
		CHECK_QSTATUS();
		++it;
	}
	
	HTREEITEM hItem = TreeView_GetRoot(pThis_->getHandle());
	while (hItem) {
		Account* pAccount = getAccount(hItem);
		if (!pAccount)
			return QSTATUS_FAIL;
		status = pAccount->removeAccountHandler(this);
		CHECK_QSTATUS();
		
		hItem = TreeView_GetNextSibling(pThis_->getHandle(), hItem);
	}
	
	mapFolder_.clear();
	TreeView_DeleteAllItems(pThis_->getHandle());
	
	return QSTATUS_SUCCESS;
}

QSTATUS qm::FolderWindowImpl::updateAccountList()
{
	DECLARE_QSTATUS();
	
	const Document::AccountList& l = pDocument_->getAccounts();
	Document::AccountList::const_iterator it = l.begin();
	while (it != l.end()) {
		status = addAccount(*it++);
		CHECK_QSTATUS();
	}
	
	return status;
}

QSTATUS qm::FolderWindowImpl::refreshFolderList(Account* pAccount)
{
	DECLARE_QSTATUS();
	
	FolderMap::iterator it = mapFolder_.begin();
	while (it != mapFolder_.end()) {
		if ((*it).first->getAccount() == pAccount) {
			status = (*it).first->removeFolderHandler(this);
			CHECK_QSTATUS();
			it = mapFolder_.erase(it);
		}
		else {
			++it;
		}
	}
	
	HTREEITEM hItem = getHandleFromAccount(pAccount);
	assert(hItem);
	
	TreeView_Expand(pThis_->getHandle(), hItem,
		TVE_COLLAPSE | TVE_COLLAPSERESET);
	
	status = insertFolders(hItem, pAccount);
	CHECK_QSTATUS();
	
	TreeView_Expand(pThis_->getHandle(), hItem, TVE_EXPAND);
	
	return QSTATUS_SUCCESS;
}

QSTATUS qm::FolderWindowImpl::addAccount(Account* pAccount)
{
	DECLARE_QSTATUS();
	
	W2T(pAccount->getName(), ptszName);
	
	TVINSERTSTRUCT tvisAccount = {
		TVI_ROOT,
		TVI_SORT,
		{
			TVIF_TEXT | TVIF_PARAM | TVIF_IMAGE | TVIF_SELECTEDIMAGE,
			0,
			0,
			0,
			const_cast<LPTSTR>(ptszName),
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
		return QSTATUS_FAIL;
	
	status = insertFolders(hItemAccount, pAccount);
	CHECK_QSTATUS();
	
	status = pAccount->addAccountHandler(this);
	CHECK_QSTATUS();
	
	return QSTATUS_SUCCESS;
}

QSTATUS qm::FolderWindowImpl::insertFolders(HTREEITEM hItem, Account* pAccount)
{
	assert(hItem);
	assert(pAccount);
	
	DECLARE_QSTATUS();
	
	STLWrapper<FolderMap> wrapper(mapFolder_);
	
	const Account::FolderList& l = pAccount->getFolders();
	Account::FolderList listFolder;
	status = STLWrapper<Account::FolderList>(listFolder).reserve(l.size());
	CHECK_QSTATUS();
	std::remove_copy_if(l.begin(), l.end(),
		std::back_inserter(listFolder), std::mem_fun(&Folder::isHidden));
	std::sort(listFolder.begin(), listFolder.end(), FolderLess());
	
	Account::FolderList::const_iterator it = listFolder.begin();
	while (it != listFolder.end()) {
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
			return QSTATUS_FAIL;
		status = wrapper.push_back(std::make_pair(pFolder, hItemFolder));
		CHECK_QSTATUS();
		
		status = pFolder->addFolderHandler(this);
		CHECK_QSTATUS();
		
		++it;
	}
	
	return QSTATUS_SUCCESS;
}

int qm::FolderWindowImpl::getFolderImage(Folder* pFolder,
	bool bSelected, bool bExpanded) const
{
	int nImage = UIUtil::getFolderImage(pFolder, bSelected);
	bool bUnseen = false;
	if (bExpanded) {
		bUnseen = pFolder->getUnseenCount() != 0;
	}
	else {
		const Account::FolderList& l = pFolder->getAccount()->getFolders();
		Account::FolderList::const_iterator it = l.begin();
		while (it != l.end() && !bUnseen) {
			if (pFolder == *it || pFolder->isAncestorOf(*it))
				bUnseen = (*it)->getUnseenCount() != 0;
			++it;
		}
	}
	
	return bUnseen ? nImage + 1 : nImage;
}

int qm::FolderWindowImpl::getAccountImage(Account* pAccount,
	bool bSelected, bool bExpanded) const
{
	bool bUnseen = false;
	
	const unsigned int nIgnore = Folder::FLAG_BOX_MASK & ~Folder::FLAG_INBOX;
	
	const Account::FolderList& l = pAccount->getFolders();
	Account::FolderList::const_iterator it = l.begin();
	while (it != l.end() && !bUnseen) {
		Folder* pFolder = *it;
		if (!(pFolder->getFlags() & nIgnore))
			bUnseen = pFolder->getUnseenCount() != 0;
		++it;
	}
	
	return bUnseen ? 1 : 0;
}


/****************************************************************************
 *
 * FolderWindow
 *
 */

qm::FolderWindow::FolderWindow(WindowBase* pParentWindow,
	FolderModel* pFolderModel, Profile* pProfile, QSTATUS* pstatus) :
	WindowBase(true, pstatus),
	DefaultWindowHandler(pstatus),
	pImpl_(0)
{
	if (*pstatus != QSTATUS_SUCCESS)
		return;
	
	DECLARE_QSTATUS();
	
	int nShowAllCount = 0;
	status = pProfile->getInt(L"FolderWindow",
		L"ShowAllCount", 1, &nShowAllCount);
	CHECK_QSTATUS_SET(pstatus);
	int nShowUnseenCount = 0;
	status = pProfile->getInt(L"FolderWindow",
		L"ShowUnseenCount", 1, &nShowUnseenCount);
	CHECK_QSTATUS_SET(pstatus);
	
	status = newObject(&pImpl_);
	CHECK_QSTATUS_SET(pstatus);
	pImpl_->pThis_ = this;
	pImpl_->pParentWindow_ = pParentWindow;
	pImpl_->pFolderModel_ = pFolderModel;
	pImpl_->pMenuManager_ = 0;
	pImpl_->pProfile_ = pProfile;
	pImpl_->pAccelerator_ = 0;
	pImpl_->pDocument_ = 0;
	pImpl_->nId_ = 0;
	pImpl_->hfont_ = 0;
	pImpl_->bShowAllCount_ = nShowAllCount != 0;
	pImpl_->bShowUnseenCount_ = nShowUnseenCount != 0;
	pImpl_->pDropTarget_ = 0;
	
	setWindowHandler(this, false);
	
	status = pParentWindow->addNotifyHandler(pImpl_);
	CHECK_QSTATUS_SET(pstatus);
	status = pFolderModel->addFolderModelHandler(pImpl_);
	CHECK_QSTATUS_SET(pstatus);
}

qm::FolderWindow::~FolderWindow()
{
	if (pImpl_) {
		delete pImpl_->pAccelerator_;
		delete pImpl_;
		pImpl_ = 0;
	}
}

QSTATUS qm::FolderWindow::getSuperClass(WSTRING* pwstrSuperClass)
{
	assert(pwstrSuperClass);
	
	*pwstrSuperClass = allocWString(WC_TREEVIEWW);
	if (!*pwstrSuperClass)
		return QSTATUS_OUTOFMEMORY;
	
	return QSTATUS_SUCCESS;
}

QSTATUS qm::FolderWindow::preCreateWindow(CREATESTRUCT* pCreateStruct)
{
	DECLARE_QSTATUS();
	
	status = DefaultWindowHandler::preCreateWindow(pCreateStruct);
	CHECK_QSTATUS();
	
	pCreateStruct->style |= TVS_HASBUTTONS | TVS_HASLINES | TVS_SHOWSELALWAYS;
	
	return QSTATUS_SUCCESS;
}

QSTATUS qm::FolderWindow::getAccelerator(Accelerator** ppAccelerator)
{
	assert(ppAccelerator);
	*ppAccelerator = pImpl_->pAccelerator_;
	return QSTATUS_SUCCESS;
}

LRESULT qm::FolderWindow::windowProc(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	BEGIN_MESSAGE_HANDLER()
		HANDLE_CONTEXTMENU()
		HANDLE_CREATE()
		HANDLE_DESTROY()
		HANDLE_LBUTTONDOWN()
		HANDLE_MESSAGE(FolderWindowImpl::WM_FOLDERWINDOW_MESSAGEADDED, onMessageAdded)
		HANDLE_MESSAGE(FolderWindowImpl::WM_FOLDERWINDOW_MESSAGEREMOVED, onMessageRemoved)
		HANDLE_MESSAGE(FolderWindowImpl::WM_FOLDERWINDOW_MESSAGECHANGED, onMessageChanged)
	END_MESSAGE_HANDLER()
	return DefaultWindowHandler::windowProc(uMsg, wParam, lParam);
}

LRESULT qm::FolderWindow::onContextMenu(HWND hwnd, const POINT& pt)
{
	DECLARE_QSTATUS();
	
	HMENU hmenu = 0;
	status = pImpl_->pMenuManager_->getMenu(L"folder", false, false, &hmenu);
	if (status == QSTATUS_SUCCESS) {
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
	
	DECLARE_QSTATUS();
	
	FolderWindowCreateContext* pContext =
		static_cast<FolderWindowCreateContext*>(pCreateStruct->lpCreateParams);
	pImpl_->pDocument_ = pContext->pDocument_;
	pImpl_->pMenuManager_ = pContext->pMenuManager_;
	pImpl_->pDocument_->addDocumentHandler(pImpl_);
	
	status = pContext->pKeyMap_->createAccelerator(
		CustomAcceleratorFactory(), L"FolderWindow",
		mapKeyNameToId, countof(mapKeyNameToId), &pImpl_->pAccelerator_);
	CHECK_QSTATUS_VALUE(-1);
	
	pImpl_->nId_ = getWindowLong(GWL_ID);
	
	status = qs::UIUtil::createFontFromProfile(pImpl_->pProfile_,
		L"FolderWindow", false, &pImpl_->hfont_);
	CHECK_QSTATUS_VALUE(-1);
	setFont(pImpl_->hfont_);
	
	HIMAGELIST hImageList = ImageList_LoadImage(
		Application::getApplication().getResourceHandle(),
		MAKEINTRESOURCE(IDB_FOLDER), 16, 0, CLR_DEFAULT, IMAGE_BITMAP, 0);
	TreeView_SetImageList(getHandle(), hImageList, TVSIL_NORMAL);
	
	status = newQsObject(getHandle(), &pImpl_->pDropTarget_);
	CHECK_QSTATUS_VALUE(-1);
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
	
	delete pImpl_->pDropTarget_;
	
	return DefaultWindowHandler::onDestroy();
}

LRESULT qm::FolderWindow::onLButtonDown(UINT nFlags, const POINT& pt)
{
#if defined _WIN32_WCE && _WIN32_WCE >= 300 && _WIN32_WCE_PSPC
	if (tapAndHold(pt))
		return 0;
#endif
	return DefaultWindowHandler::onLButtonDown(nFlags, pt);
}

LRESULT qm::FolderWindow::onMessageAdded(WPARAM wParam, LPARAM lParam)
{
	pImpl_->update(reinterpret_cast<Folder*>(lParam));
	return 0;
}

LRESULT qm::FolderWindow::onMessageRemoved(WPARAM wParam, LPARAM lParam)
{
	pImpl_->update(reinterpret_cast<Folder*>(lParam));
	return 0;
}

LRESULT qm::FolderWindow::onMessageChanged(WPARAM wParam, LPARAM lParam)
{
	pImpl_->update(reinterpret_cast<NormalFolder*>(lParam));
	return 0;
}

bool qm::FolderWindow::isShow() const
{
	return isVisible();
}

bool qm::FolderWindow::isActive() const
{
	return hasFocus();
}

QSTATUS qm::FolderWindow::setActive()
{
	setFocus();
	return QSTATUS_SUCCESS;
}
