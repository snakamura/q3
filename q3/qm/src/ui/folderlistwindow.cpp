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
#include <qmfolderlistwindow.h>

#include <qsaccelerator.h>
#include <qsconv.h>
#include <qsmenu.h>
#include <qsuiutil.h>

#include <algorithm>

#include <tchar.h>

#include "folderlistmodel.h"
#include "folderlistwindow.h"
#include "foldermodel.h"
#include "keymap.h"
#include "resourceinc.h"
#include "uimanager.h"
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
	void loadColumns();
	void saveColumns();
	void setCurrentAccount(Account* pAccount,
						   bool bShowSize);
	void open(int nItem);
	Folder* getFolder(int nItem) const;

public:
	virtual LRESULT onNotify(NMHDR* pnmhdr,
							 bool* pbHandled);

public:
	virtual void accountChanged(const FolderListModelEvent& event);
	virtual void folderListChanged(const FolderListModelEvent& event);

private:
#if defined _WIN32_WCE && _WIN32_WCE >= 400 && defined _WIN32_WCE_PSPC
	LRESULT onRecognizeGesture(NMHDR* pnmhdr,
							   bool* pbHandled);
#endif
	LRESULT onItemChanged(NMHDR* pnmhdr,
						  bool* pbHandled);

private:
	void updateFolderListModel();

private:
	static int getIndent(Folder* pFolder);

public:
	FolderListWindow* pThis_;
	WindowBase* pParentWindow_;
	FolderListModel* pFolderListModel_;
	FolderModel* pFolderModel_;
	MenuManager* pMenuManager_;
	Profile* pProfile_;
	std::auto_ptr<Accelerator> pAccelerator_;
	Document* pDocument_;
	
	UINT nId_;
	HFONT hfont_;
	bool bSizeShown_;
	bool bInserting_;
};

void qm::FolderListWindowImpl::loadColumns()
{
	HINSTANCE hInst = Application::getApplication().getResourceHandle();
	struct {
		UINT nId_;
		const WCHAR* pwszWidthKey_;
		bool bLeft_;
		int nDefaultWidth_;
	} columns[] = {
		{ IDS_FOLDERLISTNAME,			L"NameWidth",			true,	150	},
		{ IDS_FOLDERLISTID,				L"IdWidth",				false,	50	},
		{ IDS_FOLDERLISTCOUNT,			L"CountWidth",			false,	50	},
		{ IDS_FOLDERLISTUNSEENCOUNT,	L"UnseenCountWidth",	false,	50	},
		{ IDS_FOLDERLISTSIZE,			L"SizeWidth",			false,	150	},
	};
	for (int n = 0; n < countof(columns); ++n) {
		wstring_ptr wstrTitle(loadString(hInst, columns[n].nId_));
		W2T(wstrTitle.get(), ptszTitle);
		
		int nWidth = pProfile_->getInt(L"FolderListWindow",
			columns[n].pwszWidthKey_, columns[n].nDefaultWidth_);
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
}

void qm::FolderListWindowImpl::saveColumns()
{
	const WCHAR* pwszWidthKeys[] = {
		L"NameWidth",
		L"IdWidth",
		L"CountWidth",
		L"UnseenCountWidth",
		L"SizeWidth"
	};
	for (int n = 0; n < countof(pwszWidthKeys); ++n) {
		int nWidth = ListView_GetColumnWidth(pThis_->getHandle(), n);
		pProfile_->setInt(L"FolderListWindow", pwszWidthKeys[n], nWidth);
	}
}

void qm::FolderListWindowImpl::setCurrentAccount(Account* pAccount,
												 bool bShowSize)
{
	DisableRedraw disable(pThis_->getHandle());
	
	ListView_DeleteAllItems(pThis_->getHandle());
	
	if (pAccount) {
		const Account::FolderList& l = pAccount->getFolders();
		typedef std::vector<std::pair<Folder*, std::pair<unsigned int, unsigned int> > > FolderList;
		FolderList listFolder;
		listFolder.reserve(l.size());
		for (Account::FolderList::const_iterator it = l.begin(); it != l.end(); ++it) {
			Folder* pFolder = *it;
			unsigned int nSize = 0;
			unsigned int nBoxSize = 0;
			if (bShowSize) {
				nSize = pFolder->getSize();
				nBoxSize = pFolder->getBoxSize();
			}
			listFolder.push_back(std::make_pair(pFolder, std::make_pair(nSize, nBoxSize)));
		}
		std::sort(listFolder.begin(), listFolder.end(),
			binary_compose_f_gx_hy(
				FolderLess(),
				std::select1st<FolderList::value_type>(),
				std::select1st<FolderList::value_type>()));
		
		bInserting_ = true;
		
		for (FolderList::size_type n = 0; n < listFolder.size(); ++n) {
			Folder* pFolder = listFolder[n].first;
			
			W2T(pFolder->getName(), ptszName);
			
			LVITEM item = {
				LVIF_IMAGE | LVIF_INDENT | LVIF_TEXT | LVIF_PARAM,
				n,
				0,
				0,
				0,
				const_cast<LPTSTR>(ptszName),
				0,
				UIUtil::getFolderImage(pFolder, false),
				reinterpret_cast<LPARAM>(pFolder),
				getIndent(pFolder)
			};
			int nItem = ListView_InsertItem(pThis_->getHandle(), &item);
			
			TCHAR tsz[64];
			wsprintf(tsz, _T("%d"), pFolder->getId());
			ListView_SetItemText(pThis_->getHandle(), nItem, 1, tsz);
			wsprintf(tsz, _T("%d"), pFolder->getCount());
			ListView_SetItemText(pThis_->getHandle(), nItem, 2, tsz);
			wsprintf(tsz, _T("%d"), pFolder->getUnseenCount());
			ListView_SetItemText(pThis_->getHandle(), nItem, 3, tsz);
			if (bShowSize) {
				unsigned int nSize = listFolder[n].second.first;
				unsigned int nBoxSize = listFolder[n].second.second;
				unsigned int nDescendantSize = nSize;
				unsigned int nDescendantBoxSize = nBoxSize;
				for (FolderList::size_type m = n + 1; m < listFolder.size(); ++m) {
					if (!pFolder->isAncestorOf(listFolder[m].first))
						break;
					nDescendantSize += listFolder[m].second.first;
					nDescendantBoxSize += listFolder[m].second.second;
				}
				wsprintf(tsz, _T("%d/%dKB (%d/%dKB)"), nBoxSize/1024,
					nSize/1024, nDescendantBoxSize/1024, nDescendantSize/1024);
				ListView_SetItemText(pThis_->getHandle(), nItem, 4, tsz);
			}
			
			if (!pFolder->isFlag(Folder::FLAG_HIDE))
				ListView_SetCheckState(pThis_->getHandle(), nItem, TRUE);
		}
		
		bSizeShown_ = bShowSize;
		bInserting_ = false;
	}
	
	updateFolderListModel();
}

void qm::FolderListWindowImpl::open(int nItem)
{
	Folder* pFolder = getFolder(nItem);
	if (!pFolder->isHidden())
		pFolderModel_->setCurrent(0, pFolder, false);
}

Folder* qm::FolderListWindowImpl::getFolder(int nItem) const
{
	assert(nItem != -1);
	
	LVITEM item = {
		LVIF_PARAM,
		nItem
	};
	ListView_GetItem(pThis_->getHandle(), &item);
	
	return reinterpret_cast<Folder*>(item.lParam);
}

LRESULT qm::FolderListWindowImpl::onNotify(NMHDR* pnmhdr,
										   bool* pbHandled)
{
	BEGIN_NOTIFY_HANDLER()
#if defined _WIN32_WCE && _WIN32_WCE >= 400 && defined _WIN32_WCE_PSPC
		HANDLE_NOTIFY(NM_RECOGNIZEGESTURE, nId_, onRecognizeGesture)
#endif
		HANDLE_NOTIFY(LVN_ITEMCHANGED, nId_, onItemChanged)
	END_NOTIFY_HANDLER()
	return 1;
}

void qm::FolderListWindowImpl::accountChanged(const FolderListModelEvent& event)
{
	setCurrentAccount(pFolderListModel_->getAccount(), false);
}

void qm::FolderListWindowImpl::folderListChanged(const FolderListModelEvent& event)
{
	setCurrentAccount(pFolderListModel_->getAccount(), false);
}

#if defined _WIN32_WCE && _WIN32_WCE >= 400 && defined _WIN32_WCE_PSPC
LRESULT qm::FolderListWindowImpl::onRecognizeGesture(NMHDR* pnmhdr,
													 bool* pbHandled)
{
	*pbHandled = true;
	return TRUE;
}
#endif

LRESULT qm::FolderListWindowImpl::onItemChanged(NMHDR* pnmhdr,
												bool* pbHandled)
{
	if (!bInserting_) {
		NMLISTVIEW* pnmlv = reinterpret_cast<NMLISTVIEW*>(pnmhdr);
		if (pnmlv->iItem != -1 &&
			(pnmlv->uOldState & 0x3000) != (pnmlv->uNewState & 0x3000)) {
			Folder* pFolder = getFolder(pnmlv->iItem);
			unsigned int nFlags = (pnmlv->uNewState & 0x1000) == 0 ? 0 : Folder::FLAG_HIDE;
			pFolder->getAccount()->setFolderFlags(pFolder, nFlags, Folder::FLAG_HIDE);
		}
	}
	
	updateFolderListModel();
	
	return 0;
}

void qm::FolderListWindowImpl::updateFolderListModel()
{
	Folder* pFocusedFolder = 0;
	int nItem = ListView_GetNextItem(pThis_->getHandle(),
		-1, LVNI_ALL | LVNI_FOCUSED);
	if (nItem != -1)
		pFocusedFolder = getFolder(nItem);
	pFolderListModel_->setFocusedFolder(pFocusedFolder);
	
	int nCount = ListView_GetItemCount(pThis_->getHandle());
	Account::FolderList l;
	if (nCount != 0) {
		l.reserve(nCount);
		
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
	pFolderListModel_->setSelectedFolders(l);
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
									   FolderListModel* pFolderListModel,
									   FolderModel* pFolderModel,
									   Profile* pProfile) :
	WindowBase(true),
	pImpl_(0)
{
	pImpl_ = new FolderListWindowImpl();
	pImpl_->pThis_ = this;
	pImpl_->pParentWindow_ = pParentWindow;
	pImpl_->pFolderListModel_ = pFolderListModel;
	pImpl_->pFolderModel_ = pFolderModel;
	pImpl_->pMenuManager_ = 0;
	pImpl_->pProfile_ = pProfile;
	pImpl_->pDocument_ = 0;
	pImpl_->nId_ = 0;
	pImpl_->hfont_ = 0;
	pImpl_->bSizeShown_ = false;
	pImpl_->bInserting_ = false;
	
	setWindowHandler(this, false);
	
	pParentWindow->addNotifyHandler(pImpl_);
	pImpl_->pFolderListModel_->addFolderListModelHandler(pImpl_);
}

qm::FolderListWindow::~FolderListWindow()
{
	delete pImpl_;
}

bool qm::FolderListWindow::save()
{
	pImpl_->saveColumns();
	return true;
}

bool qm::FolderListWindow::isSizeShown() const
{
	return pImpl_->bSizeShown_;
}

void qm::FolderListWindow::showSize()
{
	if (!pImpl_->bSizeShown_)
		pImpl_->setCurrentAccount(pImpl_->pFolderListModel_->getAccount(), true);
}

wstring_ptr qm::FolderListWindow::getSuperClass()
{
	return allocWString(WC_LISTVIEWW);
}

bool qm::FolderListWindow::preCreateWindow(CREATESTRUCT* pCreateStruct)
{
	if (!DefaultWindowHandler::preCreateWindow(pCreateStruct))
		return false;
	pCreateStruct->style |= LVS_REPORT | LVS_NOSORTHEADER | LVS_SHOWSELALWAYS;
	return true;
}

Accelerator* qm::FolderListWindow::getAccelerator()
{
	return pImpl_->pAccelerator_.get();
}

LRESULT qm::FolderListWindow::windowProc(UINT uMsg,
										 WPARAM wParam,
										 LPARAM lParam)
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

LRESULT qm::FolderListWindow::onContextMenu(HWND hwnd,
											const POINT& pt)
{
	HMENU hmenu = pImpl_->pMenuManager_->getMenu(L"folderlist", false, false);
	if (hmenu) {
		UINT nFlags = TPM_LEFTALIGN | TPM_TOPALIGN;
#ifndef _WIN32_WCE
		nFlags |= TPM_LEFTBUTTON | TPM_RIGHTBUTTON;
#endif
		::TrackPopupMenu(hmenu, nFlags, pt.x, pt.y, 0, getParentFrame(), 0);
	}
	
	return 0;
}

LRESULT qm::FolderListWindow::onCreate(CREATESTRUCT* pCreateStruct)
{
	if (DefaultWindowHandler::onCreate(pCreateStruct) == -1)
		return -1;
	
	FolderListWindowCreateContext* pContext =
		static_cast<FolderListWindowCreateContext*>(pCreateStruct->lpCreateParams);
	pImpl_->pDocument_ = pContext->pDocument_;
	pImpl_->pMenuManager_ = pContext->pUIManager_->getMenuManager();
	
	CustomAcceleratorFactory acceleratorFactory;
	pImpl_->pAccelerator_ = pContext->pUIManager_->getKeyMap()->createAccelerator(
		&acceleratorFactory, L"FolderListWindow", mapKeyNameToId, countof(mapKeyNameToId));
	if (!pImpl_->pAccelerator_.get())
		return -1;
	
	pImpl_->nId_ = getWindowLong(GWL_ID);
	
	pImpl_->hfont_ = qs::UIUtil::createFontFromProfile(
		pImpl_->pProfile_, L"FolderListWindow", false);
	setFont(pImpl_->hfont_);
	
	HIMAGELIST hImageList = ImageList_LoadImage(
		Application::getApplication().getResourceHandle(),
		MAKEINTRESOURCE(IDB_FOLDER), 16, 0, CLR_DEFAULT, IMAGE_BITMAP, 0);
	ListView_SetImageList(getHandle(), hImageList, LVSIL_SMALL);
	
	ListView_SetExtendedListViewStyle(getHandle(),
		LVS_EX_FULLROWSELECT | LVS_EX_CHECKBOXES);
	
	pImpl_->loadColumns();
	
	return 0;
}

LRESULT qm::FolderListWindow::onDestroy()
{
	if (pImpl_->hfont_) {
		::DeleteObject(pImpl_->hfont_);
		pImpl_->hfont_ = 0;
	}
	
	pImpl_->pParentWindow_->removeNotifyHandler(pImpl_);
	pImpl_->pFolderListModel_->removeFolderListModelHandler(pImpl_);
	
	return DefaultWindowHandler::onDestroy();
}

LRESULT qm::FolderListWindow::onKeyDown(UINT nKey,
										UINT nRepeat,
										UINT nFlags)
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

LRESULT qm::FolderListWindow::onLButtonDblClk(UINT nFlags,
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

LRESULT qm::FolderListWindow::onLButtonDown(UINT nFlags,
											const POINT& pt)
{
#if defined _WIN32_WCE && _WIN32_WCE >= 300 && defined _WIN32_WCE_PSPC
	if (tapAndHold(pt))
		return 0;
#endif
	return DefaultWindowHandler::onLButtonDown(nFlags, pt);
}

bool qm::FolderListWindow::isShow() const
{
	return (getStyle() & WS_VISIBLE) != 0;
}

bool qm::FolderListWindow::isActive() const
{
	return hasFocus();
}

void qm::FolderListWindow::setActive()
{
	setFocus();
}
