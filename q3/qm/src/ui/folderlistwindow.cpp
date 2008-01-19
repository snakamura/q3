/*
 * $Id$
 *
 * Copyright(C) 1998-2008 Satoshi Nakamura
 *
 */

#pragma warning(disable:4786)

#include <qmaccount.h>
#include <qmdocument.h>
#include <qmfilenames.h>
#include <qmfolder.h>
#include <qmfolderlistwindow.h>

#include <qsaccelerator.h>
#include <qsconv.h>
#include <qsmenu.h>
#include <qsuiutil.h>

#include <algorithm>

#include <tchar.h>

#include "folderimage.h"
#include "folderlistwindow.h"
#include "resourceinc.h"
#include "uimanager.h"
#include "uiutil.h"
#include "../main/main.h"
#include "../uimodel/folderlistmodel.h"
#include "../uimodel/foldermodel.h"


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
	int getItem(const POINT& pt) const;
	void selectItem(int nItem);
	void open(int nItem);
	Folder* getFolder(int nItem) const;
	void reloadProfiles(bool bInitialize);

public:
	virtual LRESULT onNotify(NMHDR* pnmhdr,
							 bool* pbHandled);

public:
	virtual void accountChanged(const FolderListModelEvent& event);
	virtual void folderListChanged(const FolderListModelEvent& event);

private:
#if defined _WIN32_WCE && _WIN32_WCE >= 0x400 && defined _WIN32_WCE_PSPC
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
	const FolderImage* pFolderImage_;
	
	UINT nId_;
	HFONT hfont_;
	bool bUseSystemColor_;
	COLORREF crForeground_;
	COLORREF crBackground_;
	bool bSizeShown_;
	bool bInserting_;
};

void qm::FolderListWindowImpl::loadColumns()
{
	struct {
		UINT nId_;
		const WCHAR* pwszWidthKey_;
		bool bLeft_;
	} columns[] = {
		{ IDS_FOLDERLIST_NAME,			L"NameWidth",			true	},
		{ IDS_FOLDERLIST_ID,			L"IdWidth",				false	},
		{ IDS_FOLDERLIST_COUNT,			L"CountWidth",			false	},
		{ IDS_FOLDERLIST_UNSEENCOUNT,	L"UnseenCountWidth",	false	},
		{ IDS_FOLDERLIST_SIZE,			L"SizeWidth",			false	}
	};
	for (int n = 0; n < countof(columns); ++n) {
		wstring_ptr wstrTitle(loadString(getResourceHandle(), columns[n].nId_));
		W2T(wstrTitle.get(), ptszTitle);
		
		int nWidth = pProfile_->getInt(L"FolderListWindow", columns[n].pwszWidthKey_);
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
			boost::bind(FolderLess(),
				boost::bind(&FolderList::value_type::first, _1),
				boost::bind(&FolderList::value_type::first, _2)));
		
		bInserting_ = true;
		
		for (FolderList::size_type n = 0; n < listFolder.size(); ++n) {
			Folder* pFolder = listFolder[n].first;
			
			W2T(pFolder->getName(), ptszName);
			
			LVITEM item = {
				LVIF_IMAGE | LVIF_INDENT | LVIF_TEXT | LVIF_PARAM,
				static_cast<int>(n),
				0,
				0,
				0,
				const_cast<LPTSTR>(ptszName),
				0,
				pFolderImage_->getFolderImage(pFolder, false, false, false),
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

int qm::FolderListWindowImpl::getItem(const POINT& pt) const
{
	LVHITTESTINFO info = {
		{ pt.x, pt.y }
	};
	return ListView_HitTest(pThis_->getHandle(), &info);
}

void qm::FolderListWindowImpl::selectItem(int nItem)
{
	HWND hwnd = pThis_->getHandle();
	
	int nCount = ListView_GetItemCount(hwnd);
	for (int n = 0; n < nCount; ++n)
		ListView_SetItemState(hwnd, n,
			n == nItem ? LVIS_SELECTED | LVIS_FOCUSED : 0,
			LVIS_SELECTED | LVIS_FOCUSED);
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

void qm::FolderListWindowImpl::reloadProfiles(bool bInitialize)
{
	HFONT hfont = qs::UIUtil::createFontFromProfile(pProfile_,
		L"FolderListWindow", qs::UIUtil::DEFAULTFONT_UI);
	if (!bInitialize) {
		assert(hfont_);
		Window(ListView_GetHeader(pThis_->getHandle())).setFont(hfont);
		pThis_->setFont(hfont);
		::DeleteObject(hfont_);
	}
	hfont_ = hfont;
	
	bool bUseSystemColor = pProfile_->getInt(L"FolderListWindow", L"UseSystemColor") != 0;
	if (!bUseSystemColor) {
		struct {
			const WCHAR* pwszKey_;
			int nIndex_;
			COLORREF* pcr_;
		} colors[] = {
			{ L"ForegroundColor",	COLOR_WINDOWTEXT,	&crForeground_	},
			{ L"BackgroundColor",	COLOR_WINDOW,		&crBackground_	}
		};
		for (int n = 0; n < countof(colors); ++n) {
			wstring_ptr wstr(pProfile_->getString(L"FolderListWindow", colors[n].pwszKey_));
			Color color(wstr.get());
			if (color.getColor() != 0xffffffff)
				*colors[n].pcr_ = color.getColor();
			else
				*colors[n].pcr_ = ::GetSysColor(colors[n].nIndex_);
		}
	}
	if (!bInitialize) {
		if (bUseSystemColor) {
			if (!bUseSystemColor_) {
				ListView_SetTextColor(pThis_->getHandle(), ::GetSysColor(COLOR_WINDOWTEXT));
				ListView_SetTextBkColor(pThis_->getHandle(), ::GetSysColor(COLOR_WINDOW));
				ListView_SetBkColor(pThis_->getHandle(), ::GetSysColor(COLOR_WINDOW));
			}
		}
		else {
			ListView_SetTextColor(pThis_->getHandle(), crForeground_);
			ListView_SetTextBkColor(pThis_->getHandle(), crBackground_);
			ListView_SetBkColor(pThis_->getHandle(), crBackground_);
		}
	}
	bUseSystemColor_ = bUseSystemColor;
}

LRESULT qm::FolderListWindowImpl::onNotify(NMHDR* pnmhdr,
										   bool* pbHandled)
{
	BEGIN_NOTIFY_HANDLER()
#if defined _WIN32_WCE && _WIN32_WCE >= 0x400 && defined _WIN32_WCE_PSPC
		HANDLE_NOTIFY(NM_RECOGNIZEGESTURE, nId_, onRecognizeGesture)
#endif
		HANDLE_NOTIFY(LVN_ITEMCHANGED, nId_, onItemChanged)
	END_NOTIFY_HANDLER()
	return NotifyHandler::onNotify(pnmhdr, pbHandled);
}

void qm::FolderListWindowImpl::accountChanged(const FolderListModelEvent& event)
{
	setCurrentAccount(pFolderListModel_->getAccount(), false);
}

void qm::FolderListWindowImpl::folderListChanged(const FolderListModelEvent& event)
{
	setCurrentAccount(pFolderListModel_->getAccount(), false);
}

#if defined _WIN32_WCE && _WIN32_WCE >= 0x400 && defined _WIN32_WCE_PSPC
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
	pImpl_->nId_ = 0;
	pImpl_->hfont_ = 0;
	pImpl_->bUseSystemColor_ = true;
	pImpl_->crForeground_ = RGB(0, 0, 0);
	pImpl_->crBackground_ = RGB(255, 255, 255);
	pImpl_->bSizeShown_ = false;
	pImpl_->bInserting_ = false;
	
	pImpl_->reloadProfiles(true);
	
	setWindowHandler(this, false);
	
	pParentWindow->addNotifyHandler(pImpl_);
	pFolderListModel->addFolderListModelHandler(pImpl_);
}

qm::FolderListWindow::~FolderListWindow()
{
	delete pImpl_;
}

void qm::FolderListWindow::reloadProfiles()
{
	pImpl_->reloadProfiles(false);
}

void qm::FolderListWindow::save() const
{
	pImpl_->saveColumns();
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
	POINT ptMenu = UIUtil::getListViewContextMenuPosition(getHandle(), pt);
	HMENU hmenu = pImpl_->pMenuManager_->getMenu(L"folderlist", false, false);
	if (hmenu) {
		UINT nFlags = TPM_LEFTALIGN | TPM_TOPALIGN;
#ifndef _WIN32_WCE
		nFlags |= TPM_LEFTBUTTON | TPM_RIGHTBUTTON;
#endif
		::TrackPopupMenu(hmenu, nFlags, ptMenu.x, ptMenu.y, 0, getParentFrame(), 0);
	}
	
	return 0;
}

LRESULT qm::FolderListWindow::onCreate(CREATESTRUCT* pCreateStruct)
{
	if (DefaultWindowHandler::onCreate(pCreateStruct) == -1)
		return -1;
	
	FolderListWindowCreateContext* pContext =
		static_cast<FolderListWindowCreateContext*>(pCreateStruct->lpCreateParams);
	pImpl_->pMenuManager_ = pContext->pUIManager_->getMenuManager();
	pImpl_->pFolderImage_ = pContext->pFolderImage_;
	
	CustomAcceleratorFactory acceleratorFactory;
	pImpl_->pAccelerator_ = pContext->pUIManager_->getKeyMap()->createAccelerator(
		&acceleratorFactory, L"FolderListWindow");
	if (!pImpl_->pAccelerator_.get())
		return -1;
	
	pImpl_->nId_ = getId();
	
	setFont(pImpl_->hfont_, false);
	
	if (!pImpl_->bUseSystemColor_) {
		ListView_SetTextColor(getHandle(), pImpl_->crForeground_);
		ListView_SetTextBkColor(getHandle(), pImpl_->crBackground_);
		ListView_SetBkColor(getHandle(), pImpl_->crBackground_);
	}
	
	HIMAGELIST hImageList = ImageList_Duplicate(pImpl_->pFolderImage_->getImageList());
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
				getHandle(), -1, LVNI_ALL | LVNI_FOCUSED);
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
	int nItem = pImpl_->getItem(pt);
	if (nItem != -1)
		pImpl_->open(nItem);
	
	return DefaultWindowHandler::onLButtonDblClk(nFlags, pt);
}

LRESULT qm::FolderListWindow::onLButtonDown(UINT nFlags,
											const POINT& pt)
{
#if defined _WIN32_WCE && _WIN32_WCE >= 0x300 && defined _WIN32_WCE_PSPC
	pImpl_->selectItem(pImpl_->getItem(pt));
	
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
