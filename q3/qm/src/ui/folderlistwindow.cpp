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
#include <qmfolderlistwindow.h>

#include <qsaccelerator.h>
#include <qsconv.h>
#include <qsnew.h>
#include <qsuiutil.h>

#include <algorithm>

#include <tchar.h>

#include "folderlistmodel.h"
#include "folderlistwindow.h"
#include "foldermodel.h"
#include "keymap.h"
#include "uiutil.h"


using namespace qm;
using namespace qs;


/****************************************************************************
 *
 * FolderListWindowImpl
 *
 */

class qm::FolderListWindowImpl :
	public NotifyHandler,
	public FolderListModelHandler
{
public:
	QSTATUS loadColumns();
	QSTATUS saveColumns();
	QSTATUS open(int nItem);
	Folder* getFolder(int nItem) const;

public:
	virtual LRESULT onNotify(NMHDR* pnmhdr, bool* pbHandled);

public:
	virtual qs::QSTATUS accountChanged(const FolderListModelEvent& event);
	virtual qs::QSTATUS folderListChanged(const FolderListModelEvent& event);

private:
	LRESULT onItemChanged(NMHDR* pnmhdr, bool* pbHandled);

private:
	QSTATUS setCurrentAccount(Account* pAccount);
	QSTATUS updateFolderListModel();

private:
	static int getIndent(Folder* pFolder);

public:
	FolderListWindow* pThis_;
	WindowBase* pParentWindow_;
	FolderListModel* pFolderListModel_;
	FolderModel* pFolderModel_;
	MenuManager* pMenuManager_;
	Profile* pProfile_;
	Accelerator* pAccelerator_;
	Document* pDocument_;
	
	UINT nId_;
	HFONT hfont_;
	bool bInserting_;
};

QSTATUS qm::FolderListWindowImpl::loadColumns()
{
	DECLARE_QSTATUS();
	
	HINSTANCE hInst = Application::getApplication().getResourceHandle();
	struct {
		UINT nId_;
		const WCHAR* pwszWidthKey_;
		bool bLeft_;
	} columns[] = {
		{ IDS_FOLDERLISTNAME,			L"NameWidth",			true	},
		{ IDS_FOLDERLISTID,				L"IdWidth",				false	},
		{ IDS_FOLDERLISTCOUNT,			L"CountWidth",			false	},
		{ IDS_FOLDERLISTUNSEENCOUNT,	L"UnseenCountWidth",	false	},
		{ IDS_FOLDERLISTINDEXSIZE,		L"IndexSizeWidth",		false	},
		{ IDS_FOLDERLISTBOXSIZE, 		L"BoxSizeWidth",		false	}
	};
	for (int n = 0; n < countof(columns); ++n) {
		string_ptr<WSTRING> wstrTitle;
		status = loadString(hInst, columns[n].nId_, &wstrTitle);
		CHECK_QSTATUS();
		W2T(wstrTitle.get(), ptszTitle);
		
		int nWidth = 0;
		status = pProfile_->getInt(L"FolderListWindow",
			columns[n].pwszWidthKey_, 100, &nWidth);
		CHECK_QSTATUS();
		
		LVCOLUMN column = {
			LVCF_FMT | LVCF_SUBITEM | LVCF_TEXT | LVCF_WIDTH,
			columns[n].bLeft_ ? LVCFMT_LEFT : LVCFMT_RIGHT,
			nWidth,
			const_cast<LPTSTR>(ptszTitle),
			0,
			n
		};
		ListView_InsertColumn(pThis_->getHandle(), n, &column);
	}
	
	return QSTATUS_SUCCESS;
}

QSTATUS qm::FolderListWindowImpl::saveColumns()
{
	DECLARE_QSTATUS();
	
	const WCHAR* pwszWidthKeys[] = {
		L"NameWidth",
		L"IdWidth",
		L"CountWidth",
		L"UnseenCountWidth",
		L"IndexSizeWidth",
		L"BoxSizeWidth"
	};
	for (int n = 0; n < countof(pwszWidthKeys); ++n) {
		int nWidth = ListView_GetColumnWidth(pThis_->getHandle(), n);
		status = pProfile_->setInt(L"FolderListWindow",
			pwszWidthKeys[n], nWidth);
		CHECK_QSTATUS();
	}
	
	return QSTATUS_SUCCESS;
}

QSTATUS qm::FolderListWindowImpl::open(int nItem)
{
	DECLARE_QSTATUS();
	
	Folder* pFolder = getFolder(nItem);
	if (UIUtil::isShowFolder(pFolder)) {
		status = pFolderModel_->setCurrentFolder(pFolder, false);
		CHECK_QSTATUS();
	}
	
	return QSTATUS_SUCCESS;
}

Folder* qm::FolderListWindowImpl::getFolder(int nItem) const
{
	assert(nItem != -1);
	
	DECLARE_QSTATUS();
	
	LVITEM item = {
		LVIF_PARAM,
		nItem
	};
	ListView_GetItem(pThis_->getHandle(), &item);
	
	return reinterpret_cast<Folder*>(item.lParam);
}

LRESULT qm::FolderListWindowImpl::onNotify(NMHDR* pnmhdr, bool* pbHandled)
{
	BEGIN_NOTIFY_HANDLER()
		HANDLE_NOTIFY(LVN_ITEMCHANGED, nId_, onItemChanged)
	END_NOTIFY_HANDLER()
	return 1;
}

QSTATUS qm::FolderListWindowImpl::accountChanged(const FolderListModelEvent& event)
{
	return setCurrentAccount(pFolderListModel_->getAccount());
}

QSTATUS qm::FolderListWindowImpl::folderListChanged(const FolderListModelEvent& event)
{
	return setCurrentAccount(pFolderListModel_->getAccount());
}

LRESULT qm::FolderListWindowImpl::onItemChanged(NMHDR* pnmhdr, bool* pbHandled)
{
	if (!bInserting_) {
		NMLISTVIEW* pnmlv = reinterpret_cast<NMLISTVIEW*>(pnmhdr);
		if (pnmlv->iItem != -1 &&
			(pnmlv->uOldState & 0x3000) != (pnmlv->uNewState & 0x3000)) {
			Folder* pFolder = getFolder(pnmlv->iItem);
			pFolder->getAccount()->showFolder(pFolder,
				(pnmlv->uNewState & 0x1000) == 0);
		}
	}
	
	updateFolderListModel();
	
	return 0;
}

QSTATUS qm::FolderListWindowImpl::setCurrentAccount(Account* pAccount)
{
	DECLARE_QSTATUS();
	
	DisableRedraw disable(pThis_->getHandle());
	
	ListView_DeleteAllItems(pThis_->getHandle());
	
	if (pAccount) {
		const Account::FolderList& l = pAccount->getFolders();
		Account::FolderList listFolder;
		status = STLWrapper<Account::FolderList>(listFolder).resize(l.size());
		CHECK_QSTATUS();
		std::copy(l.begin(), l.end(), listFolder.begin());
		std::sort(listFolder.begin(), listFolder.end(), FolderLess());
		
		bInserting_ = true;
		
		for (Account::FolderList::size_type n = 0; n < listFolder.size(); ++n) {
			Folder* pFolder = listFolder[n];
			
			W2T(pFolder->getName(), ptszName);
			
			unsigned int nFlags = pFolder->getFlags();
			int nImage = nFlags & Folder::FLAG_INBOX ? 6 :
				(nFlags & Folder::FLAG_OUTBOX) || (nFlags & Folder::FLAG_SENTBOX) ? 8 :
				nFlags & Folder::FLAG_TRASHBOX ? 10 : 2;
			
			LVITEM item = {
				LVIF_IMAGE | LVIF_INDENT | LVIF_TEXT | LVIF_PARAM,
				n,
				0,
				0,
				0,
				const_cast<LPTSTR>(ptszName),
				0,
				nImage,
				reinterpret_cast<LPARAM>(pFolder),
				getIndent(pFolder)
			};
			int nItem = ListView_InsertItem(pThis_->getHandle(), &item);
			
			TCHAR tsz[32];
			wsprintf(tsz, _T("%d"), pFolder->getId());
			ListView_SetItemText(pThis_->getHandle(), nItem, 1, tsz);
			wsprintf(tsz, _T("%d"), pFolder->getCount());
			ListView_SetItemText(pThis_->getHandle(), nItem, 2, tsz);
			wsprintf(tsz, _T("%d"), pFolder->getUnseenCount());
			ListView_SetItemText(pThis_->getHandle(), nItem, 3, tsz);
			
			if (!pFolder->isFlag(Folder::FLAG_HIDE))
				ListView_SetCheckState(pThis_->getHandle(), nItem, TRUE);
		}
		
		bInserting_ = false;
	}
	
	updateFolderListModel();
	
	return QSTATUS_SUCCESS;
}

QSTATUS qm::FolderListWindowImpl::updateFolderListModel()
{
	DECLARE_QSTATUS();
	
	Folder* pFocusedFolder = 0;
	int nItem = ListView_GetNextItem(pThis_->getHandle(),
		-1, LVNI_ALL | LVNI_FOCUSED);
	if (nItem != -1)
		pFocusedFolder = getFolder(nItem);
	pFolderListModel_->setFocusedFolder(pFocusedFolder);
	
	int nCount = ListView_GetItemCount(pThis_->getHandle());
	Account::FolderList l;
	if (nCount != 0) {
		status = STLWrapper<Account::FolderList>(l).reserve(nCount);
		CHECK_QSTATUS();
		
		int nItem = -1;
		while (true) {
			nItem = ListView_GetNextItem(pThis_->getHandle(),
				nItem, LVNI_ALL | LVNI_SELECTED);
			if (nItem == -1)
				break;
			Folder* pFolder = getFolder(nItem);
			l.push_back(pFolder);
		}
	}
	status = pFolderListModel_->setSelectedFolders(l);
	CHECK_QSTATUS();
	
	return QSTATUS_SUCCESS;
}

int qm::FolderListWindowImpl::getIndent(Folder* pFolder)
{
	int nIndent = 0;
	
	Folder* pParent = pFolder->getParentFolder();
	while (pParent) {
		++nIndent;
		pParent = pParent->getParentFolder();
	}
	
	return nIndent;
}


/****************************************************************************
 *
 * FolderListWindow
 *
 */

qm::FolderListWindow::FolderListWindow(WindowBase* pParentWindow,
	FolderListModel* pFolderListModel, FolderModel* pFolderModel,
	Profile* pProfile, QSTATUS* pstatus) :
	WindowBase(true, pstatus),
	DefaultWindowHandler(pstatus),
	pImpl_(0)
{
	DECLARE_QSTATUS();
	
	status = newObject(&pImpl_);
	CHECK_QSTATUS_SET(pstatus);
	pImpl_->pThis_ = this;
	pImpl_->pParentWindow_ = pParentWindow;
	pImpl_->pFolderListModel_ = pFolderListModel;
	pImpl_->pFolderModel_ = pFolderModel;
	pImpl_->pMenuManager_ = 0;
	pImpl_->pProfile_ = pProfile;
	pImpl_->pAccelerator_ = 0;
	pImpl_->pDocument_ = 0;
	pImpl_->nId_ = 0;
	pImpl_->hfont_ = 0;
	pImpl_->bInserting_ = false;
	
	setWindowHandler(this, false);
	
	status = pParentWindow->addNotifyHandler(pImpl_);
	CHECK_QSTATUS_SET(pstatus);
	
	status = pImpl_->pFolderListModel_->addFolderListModelHandler(pImpl_);
	CHECK_QSTATUS_SET(pstatus);
}

qm::FolderListWindow::~FolderListWindow()
{
	if (pImpl_) {
		delete pImpl_->pAccelerator_;
		delete pImpl_;
	}
}

QSTATUS qm::FolderListWindow::getSuperClass(WSTRING* pwstrSuperClass)
{
	assert(pwstrSuperClass);
	
	*pwstrSuperClass = allocWString(WC_LISTVIEWW);
	if (!*pwstrSuperClass)
		return QSTATUS_OUTOFMEMORY;
	
	return QSTATUS_SUCCESS;
}

QSTATUS qm::FolderListWindow::preCreateWindow(CREATESTRUCT* pCreateStruct)
{
	DECLARE_QSTATUS();
	
	status = DefaultWindowHandler::preCreateWindow(pCreateStruct);
	CHECK_QSTATUS();
	
	pCreateStruct->style |= LVS_REPORT | LVS_NOSORTHEADER | LVS_SHOWSELALWAYS;
	
	return QSTATUS_SUCCESS;
}

QSTATUS qm::FolderListWindow::getAccelerator(Accelerator** ppAccelerator)
{
	assert(ppAccelerator);
	*ppAccelerator = pImpl_->pAccelerator_;
	return QSTATUS_SUCCESS;
}

LRESULT qm::FolderListWindow::windowProc(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	BEGIN_MESSAGE_HANDLER()
		HANDLE_CONTEXTMENU()
		HANDLE_CREATE()
		HANDLE_DESTROY()
		HANDLE_KEYDOWN()
		HANDLE_LBUTTONDBLCLK()
		HANDLE_LBUTTONDOWN()
	END_MESSAGE_HANDLER()
	return DefaultWindowHandler::windowProc(uMsg, wParam, lParam);
}

LRESULT qm::FolderListWindow::onContextMenu(HWND hwnd, const POINT& pt)
{
	DECLARE_QSTATUS();
	
	HMENU hmenu = 0;
	status = pImpl_->pMenuManager_->getMenu(
		L"folderlist", false, false, &hmenu);
	if (status == QSTATUS_SUCCESS) {
		UINT nFlags = TPM_LEFTALIGN | TPM_TOPALIGN;
#ifndef _WIN32_WCE
		nFlags |= TPM_LEFTBUTTON | TPM_RIGHTBUTTON;
#endif
		::TrackPopupMenu(hmenu, nFlags, pt.x, pt.y, 0, getParentFrame(), 0);
	}
	
	return DefaultWindowHandler::onContextMenu(hwnd, pt);
}

LRESULT qm::FolderListWindow::onCreate(CREATESTRUCT* pCreateStruct)
{
	if (DefaultWindowHandler::onCreate(pCreateStruct) == -1)
		return -1;
	
	DECLARE_QSTATUS();
	
	FolderListWindowCreateContext* pContext =
		static_cast<FolderListWindowCreateContext*>(pCreateStruct->lpCreateParams);
	pImpl_->pDocument_ = pContext->pDocument_;
	pImpl_->pMenuManager_ = pContext->pMenuManager_;
	
	status = pContext->pKeyMap_->createAccelerator(
		CustomAcceleratorFactory(), L"FolderListWindow",
		mapKeyNameToId, countof(mapKeyNameToId), &pImpl_->pAccelerator_);
	CHECK_QSTATUS_VALUE(-1);
	
	pImpl_->nId_ = getWindowLong(GWL_ID);
	
	status = qs::UIUtil::createFontFromProfile(pImpl_->pProfile_,
		L"FolderListWindow", false, &pImpl_->hfont_);
	CHECK_QSTATUS_VALUE(-1);
	setFont(pImpl_->hfont_);
	
	HIMAGELIST hImageList = ImageList_LoadImage(
		Application::getApplication().getResourceHandle(),
		MAKEINTRESOURCE(IDB_FOLDER), 16, 0, CLR_DEFAULT, IMAGE_BITMAP, 0);
	ListView_SetImageList(getHandle(), hImageList, LVSIL_SMALL);
	
	ListView_SetExtendedListViewStyle(getHandle(),
		LVS_EX_FULLROWSELECT | LVS_EX_CHECKBOXES);
	
	status = pImpl_->loadColumns();
	CHECK_QSTATUS_VALUE(-1);
	
	return 0;
}

LRESULT qm::FolderListWindow::onDestroy()
{
	pImpl_->saveColumns();
	
	if (pImpl_->hfont_) {
		::DeleteObject(pImpl_->hfont_);
		pImpl_->hfont_ = 0;
	}
	
	pImpl_->pParentWindow_->removeNotifyHandler(pImpl_);
	pImpl_->pFolderListModel_->removeFolderListModelHandler(pImpl_);
	
	return DefaultWindowHandler::onDestroy();
}

LRESULT qm::FolderListWindow::onKeyDown(UINT nKey, UINT nRepeat, UINT nFlags)
{
	switch (nKey) {
	case VK_RETURN:
		{
			int nItem = ListView_GetNextItem(
				getHandle(), -1, LVNI_ALL | LVNI_SELECTED);
			if (nItem != -1)
				pImpl_->open(nItem);
		}
		break;
	default:
		break;
	}
	
	return DefaultWindowHandler::onKeyDown(nKey, nRepeat, nFlags);
}

LRESULT qm::FolderListWindow::onLButtonDblClk(UINT nFlags, const POINT& pt)
{
	LVHITTESTINFO info = {
		{ pt.x, pt.y }
	};
	int nItem = ListView_HitTest(getHandle(), &info);
	if (nItem != -1)
		pImpl_->open(nItem);
	
	return DefaultWindowHandler::onLButtonDblClk(nFlags, pt);
}

LRESULT qm::FolderListWindow::onLButtonDown(UINT nFlags, const POINT& pt)
{
#if defined _WIN32_WCE && _WIN32_WCE >= 300 && _WIN32_WCE_PSPC
	if (tapAndHold(pt))
		return 0;
#endif
	return DefaultWindowHandler::onLButtonDown(nFlags, pt);
}

bool qm::FolderListWindow::isShow() const
{
	return isVisible();
}

bool qm::FolderListWindow::isActive() const
{
	return hasFocus();
}

QSTATUS qm::FolderListWindow::setActive()
{
	setFocus();
	return QSTATUS_SUCCESS;
}
