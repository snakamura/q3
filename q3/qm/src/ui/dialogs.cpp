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
#include <qmfilenames.h>
#include <qmgoround.h>
#include <qmsession.h>
#include <qmuiutil.h>

#include <qsconv.h>
#include <qsmime.h>
#include <qsras.h>
#include <qsuiutil.h>

#include <algorithm>

#include <commdlg.h>
#include <tchar.h>

#include "dialogs.h"
#include "propertypages.h"
#include "resourceinc.h"
#include "uimanager.h"
#include "uiutil.h"
#include "../model/addressbook.h"
#include "../model/fixedformtext.h"
#include "../model/templatemanager.h"

using namespace qm;
using namespace qs;


/****************************************************************************
 *
 * DefaultDialog
 *
 */

qm::DefaultDialog::DefaultDialog(UINT nId) :
	qs::DefaultDialog(Application::getApplication().getResourceHandle(), nId)
{
}

qm::DefaultDialog::~DefaultDialog()
{
}


/****************************************************************************
 *
 * AccountDialog
 *
 */

qm::AccountDialog::AccountDialog(Document* pDocument,
								 Account* pAccount,
								 SyncFilterManager* pSyncFilterManager,
								 Profile* pProfile) :
	DefaultDialog(IDD_ACCOUNT),
	pDocument_(pDocument),
	pSubAccount_(pAccount ? pAccount->getCurrentSubAccount() : 0),
	pSyncFilterManager_(pSyncFilterManager),
	pProfile_(pProfile)
{
}

qm::AccountDialog::~AccountDialog()
{
}

LRESULT qm::AccountDialog::onCommand(WORD nCode,
									 WORD nId)
{
	BEGIN_COMMAND_HANDLER()
		HANDLE_COMMAND_ID(IDC_ADDACCOUNT, onAddAccount)
		HANDLE_COMMAND_ID(IDC_ADDSUBACCOUNT, onAddSubAccount)
		HANDLE_COMMAND_ID(IDC_PROPERTY, onProperty)
		HANDLE_COMMAND_ID(IDC_REMOVE, onRemove)
		HANDLE_COMMAND_ID(IDC_RENAME, onRename)
	END_COMMAND_HANDLER()
	return DefaultDialog::onCommand(nCode, nId);
}

LRESULT qm::AccountDialog::onDestroy()
{
	HIMAGELIST hImageList = TreeView_GetImageList(
		getDlgItem(IDC_ACCOUNT), TVSIL_NORMAL);
	ImageList_Destroy(hImageList);
	
	removeNotifyHandler(this);
	return DefaultDialog::onDestroy();
}

LRESULT qm::AccountDialog::onInitDialog(HWND hwndFocus,
										LPARAM lParam)
{
	HIMAGELIST hImageList = ImageList_LoadImage(
		Application::getApplication().getResourceHandle(),
		MAKEINTRESOURCE(IDB_ACCOUNT), 16, 0, CLR_DEFAULT, IMAGE_BITMAP, 0);
	TreeView_SetImageList(getDlgItem(IDC_ACCOUNT), hImageList, TVSIL_NORMAL);
	
	init(true);
	update();
	updateState();
	addNotifyHandler(this);
	
	return TRUE;
}

LRESULT qm::AccountDialog::onNotify(NMHDR* pnmhdr,
									bool* pbHandled)
{
	BEGIN_NOTIFY_HANDLER()
		HANDLE_NOTIFY(TVN_SELCHANGED, IDC_ACCOUNT, onAccountSelChanged);
	END_NOTIFY_HANDLER()
	return 1;
}

LRESULT qm::AccountDialog::onAddAccount()
{
	HINSTANCE hInst = Application::getApplication().getResourceHandle();
	
	CreateAccountDialog dialog(pProfile_);
	if (dialog.doModal(getHandle()) == IDOK) {
		const WCHAR* pwszName = dialog.getName();
		if (pDocument_->hasAccount(pwszName)) {
			messageBox(hInst, IDS_ERROR_CREATEACCOUNT, MB_OK | MB_ICONERROR, getHandle());
			return 0;
		}
		
		wstring_ptr wstrDir(concat(Application::getApplication().getMailFolder(),
			L"\\accounts\\", pwszName));
		W2T(wstrDir.get(), ptszDir);
		if (!::CreateDirectory(ptszDir, 0)) {
			messageBox(hInst, IDS_ERROR_CREATEACCOUNT, MB_OK | MB_ICONERROR, getHandle());
			return 0;
		}
		
		wstring_ptr wstrPath(concat(wstrDir.get(), L"\\", FileNames::ACCOUNT_XML));
		XMLProfile profile(wstrPath.get());
		profile.setString(L"Global", L"Class", dialog.getClass());
		profile.setInt(L"Global", L"BlockSize", dialog.getBlockSize());
		profile.setInt(L"Global", L"CacheBlockSize", dialog.getCacheBlockSize());
		profile.setString(L"Receive", L"Type", dialog.getReceiveProtocol());
		profile.setInt(L"Receive", L"Port", dialog.getReceivePort());
		profile.setString(L"Send", L"Type", dialog.getSendProtocol());
		profile.setInt(L"Send", L"Port", dialog.getSendPort());
		if (!profile.save()) {
			messageBox(hInst, IDS_ERROR_CREATEACCOUNT, MB_OK | MB_ICONERROR, getHandle());
			return 0;
		}
		
		std::auto_ptr<Account> pAccount(new Account(
			wstrDir.get(), pDocument_->getSecurity()));
		Account* p = pAccount.get();
		pDocument_->addAccount(pAccount);
		pSubAccount_ = p->getCurrentSubAccount();
		
		update();
		
		postMessage(WM_COMMAND, MAKEWPARAM(IDC_PROPERTY, BN_CLICKED),
			reinterpret_cast<LPARAM>(getDlgItem(IDC_PROPERTY)));
	}
	
	return 0;
}

LRESULT qm::AccountDialog::onAddSubAccount()
{
	HWND hwnd = getDlgItem(IDC_ACCOUNT);
	
	HTREEITEM hItem = TreeView_GetSelection(hwnd);
	if (hItem) {
		HTREEITEM hParent = TreeView_GetParent(hwnd, hItem);
		TVITEM item = {
			TVIF_HANDLE | TVIF_PARAM,
			hParent ? hParent : hItem
		};
		TreeView_GetItem(hwnd, &item);
		
		Account* pAccount = reinterpret_cast<Account*>(item.lParam);
		assert(pAccount);
		
		CreateSubAccountDialog dialog(pDocument_);
		if (dialog.doModal(getHandle()) == IDOK) {
			const WCHAR* pwszName = dialog.getName();
			
			if (pAccount->getSubAccount(pwszName)) {
				// TODO MSG
				return 0;
			}
			
			if (!pAccount->save()) {
				// TODO MSG
				return 0;
			}
			
			wstring_ptr wstrAccountPath(concat(
				pAccount->getPath(), L"\\", FileNames::ACCOUNT_XML));
			
			ConcatW c[] = {
				{ pAccount->getPath(),		-1	},
				{ L"\\",					1	},
				{ FileNames::ACCOUNT,		-1	},
				{ L"_",						1	},
				{ pwszName,					-1	},
				{ FileNames::XML_EXT,		-1	}
			};
			wstring_ptr wstrPath(concat(c, countof(c)));
			
			W2T(wstrAccountPath.get(), ptszAccountPath);
			W2T(wstrPath.get(), ptszPath);
			if (!::CopyFile(ptszAccountPath, ptszPath, FALSE)) {
				// TODO MSG
				return 0;
			}
			
			std::auto_ptr<XMLProfile> pProfile(new XMLProfile(wstrPath.get()));
			if (!pProfile->load()) {
				// TODO MSG
				return 0;
			}
			
			std::auto_ptr<SubAccount> pSubAccount(
				new SubAccount(pAccount, pProfile, pwszName));
			pAccount->addSubAccount(pSubAccount);
			
			update();
			
			postMessage(WM_COMMAND, MAKEWPARAM(IDC_PROPERTY, BN_CLICKED),
				reinterpret_cast<LPARAM>(getDlgItem(IDC_PROPERTY)));
		}
	}
	
	return 0;
}

LRESULT qm::AccountDialog::onRemove()
{
	HWND hwnd = getDlgItem(IDC_ACCOUNT);
	
	HTREEITEM hItem = TreeView_GetSelection(hwnd);
	if (hItem) {
		TVITEM item = {
			TVIF_HANDLE | TVIF_PARAM,
			hItem
		};
		TreeView_GetItem(hwnd, &item);
		
		HINSTANCE hInst = Application::getApplication().getResourceHandle();
		
		if (TreeView_GetParent(hwnd, hItem)) {
			SubAccount* pSubAccount = reinterpret_cast<SubAccount*>(item.lParam);
			
			int nRet = messageBox(hInst, IDS_CONFIRMREMOVESUBACCOUNT,
				MB_YESNO | MB_DEFBUTTON2, getHandle());
			if (nRet == IDYES) {
				Account* pAccount = pSubAccount->getAccount();
				pAccount->removeSubAccount(pSubAccount);
				pSubAccount_ = pAccount->getCurrentSubAccount();
				
				update();
			}
		}
		else {
			Account* pAccount = reinterpret_cast<Account*>(item.lParam);
			
			int nRet = messageBox(hInst, IDS_CONFIRMREMOVEACCOUNT,
				MB_YESNO | MB_DEFBUTTON2, getHandle());
			if (nRet == IDYES) {
				pDocument_->removeAccount(pAccount);
				update();
			}
		}
	}
	
	updateState();
	
	return 0;
}

LRESULT qm::AccountDialog::onRename()
{
	HWND hwnd = getDlgItem(IDC_ACCOUNT);
	
	HTREEITEM hItem = TreeView_GetSelection(hwnd);
	if (hItem) {
		TVITEM item = {
			TVIF_HANDLE | TVIF_PARAM,
			hItem
		};
		TreeView_GetItem(hwnd, &item);
		
		if (TreeView_GetParent(hwnd, hItem)) {
			SubAccount* pSubAccount = reinterpret_cast<SubAccount*>(item.lParam);
			
			RenameDialog dialog(pSubAccount->getName());
			if (dialog.doModal(getHandle()) == IDOK) {
				Account* pAccount = pSubAccount->getAccount();
				if (!pAccount->renameSubAccount(pSubAccount, dialog.getName())) {
					// TODO MSG
					return 0;
				}
				update();
			}
		}
		else {
			Account* pAccount = reinterpret_cast<Account*>(item.lParam);
			
			RenameDialog dialog(pAccount->getName());
			if (dialog.doModal(getHandle()) == IDOK) {
				if (!pDocument_->renameAccount(pAccount, dialog.getName())) {
					// TODO MSG
					return 0;
				}
				update();
			}
		}
	}
	
	return 0;
}

LRESULT qm::AccountDialog::onProperty()
{
	HWND hwnd = getDlgItem(IDC_ACCOUNT);
	
	HTREEITEM hItem = TreeView_GetSelection(hwnd);
	if (hItem) {
		TVITEM item = {
			TVIF_HANDLE | TVIF_PARAM,
			hItem
		};
		TreeView_GetItem(hwnd, &item);
		
		SubAccount* pSubAccount = 0;
		if (TreeView_GetParent(hwnd, hItem)) {
			pSubAccount = reinterpret_cast<SubAccount*>(item.lParam);
		}
		else {
			Account* pAccount = reinterpret_cast<Account*>(item.lParam);
			pSubAccount = pAccount->getSubAccount(L"");
		}
		assert(pSubAccount);
		
		HINSTANCE hInst = Application::getApplication().getResourceHandle();
		wstring_ptr wstrTitle(loadString(hInst, IDS_ACCOUNT));
		
		Account* pAccount = pSubAccount->getAccount();
		PropertyPage* pPage = 0;
		
		std::auto_ptr<ReceiveSessionUI> pReceiveUI(
			ReceiveSessionFactory::getUI(pAccount->getType(Account::HOST_RECEIVE)));
		std::auto_ptr<PropertyPage> pReceivePage(pReceiveUI->createPropertyPage(pSubAccount));
		
		std::auto_ptr<SendSessionUI> pSendUI(
			SendSessionFactory::getUI(pAccount->getType(Account::HOST_SEND)));
		std::auto_ptr<PropertyPage> pSendPage(pSendUI->createPropertyPage(pSubAccount));
		
		AccountGeneralPage generalPage(pSubAccount, pReceiveUI.get(), pSendUI.get());
		AccountUserPage userPage(pSubAccount, pReceiveUI.get(), pSendUI.get());
		AccountDetailPage detailPage(pSubAccount, pReceiveUI.get(), pSendUI.get());
		AccountDialupPage dialupPage(pSubAccount);
		AccountAdvancedPage advancedPage(pSubAccount, pSyncFilterManager_);
		PropertySheetBase sheet(hInst, wstrTitle.get(), false);
		sheet.add(&generalPage);
		sheet.add(&userPage);
		sheet.add(&detailPage);
		sheet.add(pReceivePage.get());
		sheet.add(pSendPage.get());
		sheet.add(&dialupPage);
		sheet.add(&advancedPage);
		
		sheet.doModal(getHandle());
	}
	
	return 0;
}

LRESULT qm::AccountDialog::onAccountSelChanged(NMHDR* pnmhdr,
											   bool* pbHandled)
{
	updateState();
	*pbHandled = true;
	return 0;
}

void qm::AccountDialog::update()
{
	SubAccount* pCurrentSubAccount = pSubAccount_;
	
	HWND hwnd = getDlgItem(IDC_ACCOUNT);
	
	DisableRedraw disable(hwnd);
	
	TreeView_DeleteAllItems(hwnd);
	
	HINSTANCE hInst = Application::getApplication().getResourceHandle();
	wstring_ptr wstrDefault(loadString(hInst, IDS_DEFAULTSUBACCOUNT));
	
	HTREEITEM hItemSelect = 0;
	
	const Document::AccountList& listAccount = pDocument_->getAccounts();
	for (Document::AccountList::const_iterator itA = listAccount.begin(); itA != listAccount.end(); ++itA) {
		Account* pAccount = *itA;
		
		W2T(pAccount->getName(), ptszName);
		TVINSERTSTRUCT tis = {
			TVI_ROOT,
			TVI_SORT,
			{
				TVIF_TEXT | TVIF_IMAGE | TVIF_SELECTEDIMAGE | TVIF_PARAM,
				0,
				0,
				0,
				const_cast<LPTSTR>(ptszName),
				0,
				0,
				0,
				0,
				reinterpret_cast<LPARAM>(pAccount)
			}
		};
		HTREEITEM hItem = TreeView_InsertItem(hwnd, &tis);
		
		const Account::SubAccountList& listSubAccount = pAccount->getSubAccounts();
		for (Account::SubAccountList::const_iterator itS = listSubAccount.begin(); itS != listSubAccount.end(); ++itS) {
			SubAccount* pSubAccount = *itS;
			
			const WCHAR* pwszName = pSubAccount->getName();
			if (!*pwszName)
				pwszName = wstrDefault.get();
			W2T(pwszName, ptszName);
			TVINSERTSTRUCT tis = {
				hItem,
				TVI_SORT,
				{
					TVIF_TEXT | TVIF_IMAGE | TVIF_SELECTEDIMAGE | TVIF_PARAM,
					0,
					0,
					0,
					const_cast<LPTSTR>(ptszName),
					0,
					1,
					1,
					0,
					reinterpret_cast<LPARAM>(pSubAccount)
				}
			};
			HTREEITEM hSubItem = TreeView_InsertItem(hwnd, &tis);
			if (pSubAccount == pCurrentSubAccount) {
				if (*pSubAccount->getName())
					hItemSelect = hSubItem;
				else
					hItemSelect = hItem;
			}
		}
	}
	
	if (hItemSelect) {
		TreeView_SelectItem(hwnd, hItemSelect);
		TreeView_EnsureVisible(hwnd, hItemSelect);
	}
}

void qm::AccountDialog::updateState()
{
	HWND hwnd = getDlgItem(IDC_ACCOUNT);
	
	UINT nIds[] = {
		IDC_ADDSUBACCOUNT,
		IDC_PROPERTY,
		IDC_REMOVE,
		IDC_RENAME
	};
	
	int nEnable = 0;
	
	HTREEITEM hItem = TreeView_GetSelection(hwnd);
	if (hItem) {
		nEnable = countof(nIds);
		if (TreeView_GetParent(hwnd, hItem)) {
			TVITEM item = {
				TVIF_HANDLE | TVIF_PARAM,
				hItem
			};
			TreeView_GetItem(hwnd, &item);
			
			SubAccount* pSubAccount = reinterpret_cast<SubAccount*>(item.lParam);
			if (!*pSubAccount->getName())
				nEnable = 2;
		}
	}
	
	int nDisabledDefaultId = 0;
	
	int n = 0;
	for (n = 0; n < nEnable; ++n)
		Window(getDlgItem(nIds[n])).enableWindow(true);
	for (; n < countof(nIds); ++n) {
		Window button(getDlgItem(nIds[n]));
		button.enableWindow(false);
		if (button.getWindowLong(GWL_STYLE) & BS_DEFPUSHBUTTON)
			nDisabledDefaultId = nIds[n];
	}
	
	if (nDisabledDefaultId) {
		Window(getDlgItem(IDOK)).setFocus();
		sendDlgItemMessage(nDisabledDefaultId, BM_SETSTYLE, BS_PUSHBUTTON, TRUE);
		sendMessage(DM_SETDEFID, IDOK);
		sendDlgItemMessage(IDOK, BM_SETSTYLE, BS_DEFPUSHBUTTON, TRUE);
	}
}


/****************************************************************************
 *
 * AddressBookDialog
 *
 */

namespace {
int CALLBACK itemComp(LPARAM lParam1,
					  LPARAM lParam2,
					  LPARAM lParamSort)
{
	AddressBookAddress* pAddress1 = reinterpret_cast<AddressBookAddress*>(lParam1);
	AddressBookAddress* pAddress2 = reinterpret_cast<AddressBookAddress*>(lParam2);
	
	const WCHAR* p1 = 0;
	const WCHAR* p2 = 0;
	switch (lParamSort & AddressBookDialog::SORT_TYPE_MASK) {
	case AddressBookDialog::SORT_NAME:
		p1 = pAddress1->getEntry()->getSortKey();
		p2 = pAddress2->getEntry()->getSortKey();
		break;
	case AddressBookDialog::SORT_ADDRESS:
		p1 = pAddress1->getAddress();
		p2 = pAddress2->getAddress();
		break;
	case AddressBookDialog::SORT_COMMENT:
		p1 = pAddress1->getComment();
		p2 = pAddress2->getComment();
		break;
	default:
		assert(false);
		break;
	}
	
	int nComp = p1 == p2 ? 0 : !p1 ? -1 : !p2 ? 1 : _wcsicmp(p1, p2);
	return (lParamSort & AddressBookDialog::SORT_DIRECTION_MASK)
		== AddressBookDialog::SORT_ASCENDING ? nComp : -nComp;
}

int CALLBACK selectedItemComp(LPARAM lParam1,
							  LPARAM lParam2,
							  LPARAM lParamSort)
{
	AddressBookDialog::Item* pItem1 = reinterpret_cast<AddressBookDialog::Item*>(lParam1);
	AddressBookDialog::Item* pItem2 = reinterpret_cast<AddressBookDialog::Item*>(lParam2);
	if (pItem1->getType() < pItem2->getType())
		return -1;
	else if (pItem1->getType() > pItem2->getType())
		return 1;
	else
		return wcscmp(pItem1->getValue(), pItem2->getValue());
}
}

#pragma warning(push)
#pragma warning(disable:4355)

qm::AddressBookDialog::AddressBookDialog(AddressBook* pAddressBook,
										 Profile* pProfile,
										 const WCHAR* pwszAddress[]) :
	DefaultDialog(IDD_ADDRESSBOOK),
	pAddressBook_(pAddressBook),
	pProfile_(pProfile),
	nSort_(SORT_NAME | SORT_ASCENDING),
	wndAddressList_(this)
{
	Type types[] = {
		TYPE_TO,
		TYPE_CC,
		TYPE_BCC
	};
	for (int n = 0; n < countof(listAddress_); ++n) {
		UTF8Parser field(pwszAddress[n]);
		Part part;
		if (part.setField(L"Dummy", field)) {
			AddressListParser addressList(AddressListParser::FLAG_ALLOWUTF8);
			if (part.getField(L"Dummy", &addressList) == Part::FIELD_EXIST) {
				const AddressListParser::AddressList& l = addressList.getAddressList();
				for (AddressListParser::AddressList::const_iterator it = l.begin(); it != l.end(); ++it) {
					wstring_ptr wstrValue((*it)->getValue());
					listAddress_[n].push_back(wstrValue.get());
					wstrValue.release();
				}
			}
		}
	}
}

#pragma warning(pop)

qm::AddressBookDialog::~AddressBookDialog()
{
	for (int n = 0; n < countof(listAddress_); ++n)
		std::for_each(listAddress_[n].begin(),
			listAddress_[n].end(), string_free<WSTRING>());
}

const AddressBookDialog::AddressList& qm::AddressBookDialog::getAddresses(Type type) const
{
	return listAddress_[type];
}

INT_PTR qm::AddressBookDialog::dialogProc(UINT uMsg,
										  WPARAM wParam,
										  LPARAM lParam)
{
	BEGIN_DIALOG_HANDLER()
		HANDLE_SIZE()
	END_DIALOG_HANDLER()
	return DefaultDialog::dialogProc(uMsg, wParam, lParam);
}

LRESULT qm::AddressBookDialog::onCommand(WORD nCode,
										 WORD nId)
{
	BEGIN_COMMAND_HANDLER()
		HANDLE_COMMAND_ID(IDC_CATEGORY, onCategory)
		HANDLE_COMMAND_ID_RANGE(IDC_TO, IDC_BCC, onSelect)
		HANDLE_COMMAND_ID(IDC_REMOVE, onRemove)
#if !defined _WIN32_WCE || _WIN32_WCE < 300 || !defined _WIN32_WCE_PSPC
		HANDLE_COMMAND_ID_CODE(IDC_FILTER, EN_CHANGE, onFilterChange)
#endif
	END_COMMAND_HANDLER()
	return DefaultDialog::onCommand(nCode, nId);
}

LRESULT qm::AddressBookDialog::onDestroy()
{
	int n = 0;
	
	HWND hwndSelected = getDlgItem(IDC_SELECTEDADDRESS);
	int nCount = ListView_GetItemCount(hwndSelected);
	for (n = 0; n < nCount; ++n) {
		LVITEM item = {
			LVIF_PARAM,
			n
		};
		ListView_GetItem(hwndSelected, &item);
		delete reinterpret_cast<Item*>(item.lParam);
	}
	
	const WCHAR* pwszKeys[] = {
		L"NameWidth",
		L"AddressWidth",
		L"CommentWidth"
	};
	HWND hwndAddress = getDlgItem(IDC_ADDRESS);
	for (n = 0; n < countof(pwszKeys); ++n) {
		int nColumnWidth = ListView_GetColumnWidth(hwndAddress, n);
		pProfile_->setInt(L"AddressBook", pwszKeys[n], nColumnWidth);
	}
	
	int nColumnWidth = ListView_GetColumnWidth(hwndSelected, 0);
	pProfile_->setInt(L"AddressBook", L"SelectedAddressWidth", nColumnWidth);
	
#ifndef _WIN32_WCE
	RECT rect;
	getWindowRect(&rect);
	pProfile_->setInt(L"AddressBook", L"Width", rect.right - rect.left);
	pProfile_->setInt(L"AddressBook", L"Height", rect.bottom - rect.top);
#endif
	
	const WCHAR* pwszCategory = L"";
	if (wstrCategory_.get())
		pwszCategory = wstrCategory_.get();
	pProfile_->setString(L"AddressBook", L"Category", pwszCategory);
	
	removeNotifyHandler(this);
	
	pAddressBook_->setEnableReload(true);
	
	return DefaultDialog::onDestroy();
}

LRESULT qm::AddressBookDialog::onInitDialog(HWND hwndFocus,
											LPARAM lParam)
{
	HINSTANCE hInst = Application::getApplication().getResourceHandle();
	HWND hwndList = getDlgItem(IDC_ADDRESS);
	
	ListView_SetExtendedListViewStyle(hwndList, LVS_EX_FULLROWSELECT);
	
	struct {
		UINT nId_;
		const WCHAR* pwszKey_;
		int nWidth_;
	} columns[] = {
		{ IDS_ADDRESSBOOK_NAME,		L"NameWidth",		120	},
		{ IDS_ADDRESSBOOK_ADDRESS,	L"AddressWidth",	130	},
		{ IDS_ADDRESSBOOK_COMMENT,	L"CommentWidth",	60	}
	};
	for (int n = 0; n < countof(columns); ++n) {
		wstring_ptr wstrName(loadString(hInst, columns[n].nId_));
		W2T(wstrName.get(), ptszName);
		
		int nWidth = pProfile_->getInt(L"AddressBook",
			columns[n].pwszKey_, columns[n].nWidth_);
		LVCOLUMN column = {
			LVCF_FMT | LVCF_SUBITEM | LVCF_TEXT | LVCF_WIDTH,
			LVCFMT_LEFT,
			nWidth,
			const_cast<LPTSTR>(ptszName),
			0,
			n,
		};
		ListView_InsertColumn(hwndList, n, &column);
	}
	
	HWND hwndSelected = getDlgItem(IDC_SELECTEDADDRESS);
	
	HIMAGELIST hImageList = ImageList_LoadBitmap(hInst,
		MAKEINTRESOURCE(IDB_ADDRESSBOOK), 16, 0, RGB(255, 255, 255));
	ListView_SetImageList(hwndSelected, hImageList, LVSIL_SMALL);
	
	int nColumnWidth = pProfile_->getInt(L"AddressBook", L"SelectedAddressWidth", 150);
	LVCOLUMN column = {
		LVCF_FMT | LVCF_SUBITEM | LVCF_TEXT | LVCF_WIDTH,
		LVCFMT_LEFT,
		nColumnWidth,
		_T(""),
		0,
		0
	};
	ListView_InsertColumn(hwndSelected, 0, &column);
	
	Type types[] = {
		TYPE_TO,
		TYPE_CC,
		TYPE_BCC
	};
	for (int n = 0; n < countof(listAddress_); ++n) {
		for (AddressList::iterator it = listAddress_[n].begin(); it != listAddress_[n].end(); ++it) {
			wstring_ptr wstrValue(*it);
			W2T(wstrValue.get(), ptszValue);
			*it = 0;
			
			std::auto_ptr<Item> pItem(new Item(wstrValue, types[n]));
			LVITEM newItem = {
				LVIF_TEXT | LVIF_IMAGE | LVIF_PARAM,
				ListView_GetItemCount(hwndSelected),
				0,
				0,
				0,
				const_cast<LPTSTR>(ptszValue),
				0,
				types[n],
				reinterpret_cast<LPARAM>(pItem.get())
			};
			ListView_InsertItem(hwndSelected, &newItem);
			pItem.release();
		}
		listAddress_[n].clear();
	}
	ListView_SortItems(hwndSelected, &selectedItemComp, 0);
	
	wstring_ptr wstrCategory(pProfile_->getString(L"AddressBook", L"Category", L""));
	setCurrentCategory(*wstrCategory.get() ? wstrCategory.get() : 0);
	
#ifdef _WIN32_WCE
	RECT rectWorkArea;
	::SystemParametersInfo(SPI_GETWORKAREA, 0, &rectWorkArea, 0);
	int nWidth = rectWorkArea.right - rectWorkArea.left;
	int nHeight = rectWorkArea.bottom - rectWorkArea.top;
	setWindowPos(0, 0, 0, nWidth, nHeight, SWP_NOZORDER | SWP_NOACTIVATE);
#else
	int nWidth = pProfile_->getInt(L"AddressBook", L"Width", 620);
	int nHeight = pProfile_->getInt(L"AddressBook", L"Height", 450);
	setWindowPos(0, 0, 0, nWidth, nHeight,
		SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE);
#endif
	
	addNotifyHandler(this);
	init(false);
	wndAddressList_.subclassWindow(hwndList);
	Window(hwndList).setFocus();
	
	pAddressBook_->setEnableReload(false);
	
	return FALSE;
}

LRESULT qm::AddressBookDialog::onOk()
{
	HWND hwndSelected = getDlgItem(IDC_SELECTEDADDRESS);
	
	int nCount = ListView_GetItemCount(hwndSelected);
	for (int n = 0; n < nCount; ++n) {
		LVITEM item = {
			LVIF_PARAM,
			n
		};
		ListView_GetItem(hwndSelected, &item);
		
		Item* pItem = reinterpret_cast<Item*>(item.lParam);
		
		wstring_ptr wstrValue(pItem->releaseValue());
		listAddress_[pItem->getType()].push_back(wstrValue.get());
		wstrValue.release();
	}
	
	return DefaultDialog::onOk();
}

LRESULT qm::AddressBookDialog::onNotify(NMHDR* pnmhdr,
										bool* pbHandled)
{
	BEGIN_NOTIFY_HANDLER()
		HANDLE_NOTIFY(LVN_COLUMNCLICK, IDC_ADDRESS, onAddressColumnClick)
		HANDLE_NOTIFY(NM_DBLCLK, IDC_ADDRESS, onAddressDblClk)
	END_NOTIFY_HANDLER()
	return 1;
}

LRESULT qm::AddressBookDialog::onSize(UINT nFlags,
									  int cx,
									  int cy)
{
	layout();
	return 0;
}

LRESULT qm::AddressBookDialog::onCategory()
{
	RECT rect;
	Window(getDlgItem(IDC_CATEGORY)).getWindowRect(&rect);
	
	AddressBook::CategoryList listCategory;
	pAddressBook_->getCategories(&listCategory);
	std::sort(listCategory.begin(), listCategory.end(), CategoryLess());
	
	CategoryNameList listName;
	StringListFree<CategoryNameList> free(listName);
	AutoMenuHandle hmenu(createCategoryMenu(listCategory, &listName));
	if (hmenu.get()) {
		unsigned int nFlags = TPM_LEFTALIGN |
			TPM_TOPALIGN | TPM_NONOTIFY | TPM_RETURNCMD;
#ifndef _WIN32_WCE
		nFlags |= TPM_LEFTBUTTON | TPM_RIGHTBUTTON;
#endif
		UINT nCommand = ::TrackPopupMenu(hmenu.get(), nFlags,
			rect.left, rect.bottom, 0, getHandle(), 0);
		if (nCommand == 0)
			;
		else if (nCommand == IDM_ADDRESSBOOK_ALLCATEGORY)
			setCurrentCategory(0);
		else if (nCommand - IDM_ADDRESSBOOK_CATEGORY < listName.size())
			setCurrentCategory(listName[nCommand - IDM_ADDRESSBOOK_CATEGORY]);
	}
	
	return 0;
}

LRESULT qm::AddressBookDialog::onSelect(UINT nId)
{
	Type types[] = { TYPE_TO, TYPE_CC, TYPE_BCC };
	select(types[nId - IDC_TO]);
	return 0;
}

LRESULT qm::AddressBookDialog::onRemove()
{
	remove();
	return 0;
}

#if !defined _WIN32_WCE || _WIN32_WCE < 300 || !defined _WIN32_WCE_PSPC
LRESULT qm::AddressBookDialog::onFilterChange()
{
	wstring_ptr wstrFilter(getDlgItemText(IDC_FILTER));
	wstrFilter_ = tolower(wstrFilter.get());
	update();
	return 0;
}
#endif

LRESULT qm::AddressBookDialog::onAddressColumnClick(NMHDR* pnmhdr,
													bool* pbHandled)
{
	NMLISTVIEW* pnm = reinterpret_cast<NMLISTVIEW*>(pnmhdr);
	
	Sort sorts[] = {
		SORT_NAME,
		SORT_ADDRESS,
		SORT_COMMENT
	};
	unsigned int nSort = sorts[pnm->iSubItem];
	if ((nSort_ & SORT_TYPE_MASK) == nSort)
		nSort |= (nSort_ & SORT_DIRECTION_MASK) == SORT_ASCENDING ?
			SORT_DESCENDING : SORT_ASCENDING;
	else
		nSort |= SORT_ASCENDING;
	nSort_ = nSort;
	ListView_SortItems(getDlgItem(IDC_ADDRESS), &itemComp, nSort);
	
	return 0;
}

LRESULT qm::AddressBookDialog::onAddressDblClk(NMHDR* pnmhdr,
											   bool* pbHandled)
{
	select(TYPE_TO);
	return 0;
}

void qm::AddressBookDialog::update()
{
	HWND hwndList = getDlgItem(IDC_ADDRESS);
	
	ListView_DeleteAllItems(hwndList);
	
	const AddressBook::EntryList& listEntry = pAddressBook_->getEntries();
	
	size_t nCategoryLen = 0;
	if (wstrCategory_.get())
		nCategoryLen = wcslen(wstrCategory_.get());
	
	int n = 0;
	for (AddressBook::EntryList::const_iterator itE = listEntry.begin(); itE != listEntry.end(); ++itE) {
		AddressBookEntry* pEntry = *itE;
		bool bMatchEntry = isMatchFilter(pEntry);
		W2T(pEntry->getName(), ptszName);
		const AddressBookEntry::AddressList& l = pEntry->getAddresses();
		for (AddressBookEntry::AddressList::const_iterator itA = l.begin(); itA != l.end(); ++itA) {
			AddressBookAddress* pAddress = *itA;
			
			if (isCategory(pAddress->getCategories()) &&
				(bMatchEntry || isMatchFilter(pAddress))) {
				LVITEM item = {
					LVIF_TEXT | LVIF_PARAM,
					n,
					0,
					0,
					0,
					const_cast<LPTSTR>(ptszName),
					0,
					0,
					reinterpret_cast<LPARAM>(pAddress)
				};
				ListView_InsertItem(hwndList, &item);
				
				W2T(pAddress->getAddress(), ptszAddress);
				ListView_SetItemText(hwndList, n, 1, const_cast<LPTSTR>(ptszAddress));
				if (pAddress->getComment()) {
					W2T(pAddress->getComment(), ptszComment);
					ListView_SetItemText(hwndList, n, 2, const_cast<LPTSTR>(ptszComment));
				}
				++n;
			}
		}
	}
	
	ListView_SortItems(hwndList, &itemComp, nSort_);
}

void qm::AddressBookDialog::select(Type type)
{
	HWND hwndList = getDlgItem(IDC_ADDRESS);
	HWND hwndSelected = getDlgItem(IDC_SELECTEDADDRESS);
	
	bool bFilter = getFocus() == getDlgItem(IDC_FILTER);
	
	int nItem = -1;
	while (true) {
		nItem = ListView_GetNextItem(hwndList, nItem,
			bFilter ? LVNI_ALL : LVNI_SELECTED);
		if (nItem == -1)
			break;
		
		LVITEM item = {
			LVIF_STATE | LVIF_PARAM,
			nItem,
			0,
			0,
			LVIS_SELECTED
		};
		ListView_GetItem(hwndList, &item);
		
		AddressBookAddress* pAddress =
			reinterpret_cast<AddressBookAddress*>(item.lParam);
		wstring_ptr wstrValue(pAddress->getValue());
		W2T(wstrValue.get(), ptszValue);
		
		std::auto_ptr<Item> pItem(new Item(wstrValue, type));
		LVITEM newItem = {
			LVIF_TEXT | LVIF_IMAGE | LVIF_PARAM,
			ListView_GetItemCount(hwndSelected),
			0,
			0,
			0,
			const_cast<LPTSTR>(ptszValue),
			0,
			type,
			reinterpret_cast<LPARAM>(pItem.get())
		};
		ListView_InsertItem(hwndSelected, &newItem);
		pItem.release();
	}
	
	ListView_SortItems(hwndSelected, &selectedItemComp, 0);
}

void qm::AddressBookDialog::remove()
{
	HWND hwndSelected = getDlgItem(IDC_SELECTEDADDRESS);
	
	for (int n = ListView_GetItemCount(hwndSelected) - 1; n >= 0; --n) {
		LVITEM item = {
			LVIF_STATE | LVIF_PARAM,
			n,
			0,
			0,
			LVIS_SELECTED
		};
		ListView_GetItem(hwndSelected, &item);
		if (item.state & LVIS_SELECTED) {
			delete reinterpret_cast<Item*>(item.lParam);
			ListView_DeleteItem(hwndSelected, n);
		}
	}
}

void qm::AddressBookDialog::layout()
{
#if !defined _WIN32_WCE || _WIN32_WCE < 300 || !defined _WIN32_WCE_PSPC
	RECT rect;
	getClientRect(&rect);
	
	HWND hwnds[] = {
		getDlgItem(IDOK),
		getDlgItem(IDCANCEL),
		getDlgItem(IDC_TO),
		getDlgItem(IDC_CC),
		getDlgItem(IDC_BCC),
		getDlgItem(IDC_REMOVE)
	};
	
	RECT rectButton;
	Window(hwnds[0]).getWindowRect(&rectButton);
	
	int nWidth = (rect.right - rect.left) -
		(rectButton.right - rectButton.left) - 5*3;
	int nLeftWidth = (nWidth - 5)*2/3;
	int nRightWidth = nWidth - 5 - nLeftWidth;
	int nHeight = rect.bottom - rect.top;
	int nButtonHeight = rectButton.bottom - rectButton.top;
	
	Window(getDlgItem(IDC_CATEGORY)).setWindowPos(0, 5, 5, nLeftWidth,
		nButtonHeight, SWP_NOZORDER | SWP_NOACTIVATE);
	Window(getDlgItem(IDC_FILTER)).setWindowPos(0, nLeftWidth + 5*2, 5,
		nRightWidth, nButtonHeight, SWP_NOZORDER | SWP_NOACTIVATE);
	Window(getDlgItem(IDC_ADDRESS)).setWindowPos(0, 5, nButtonHeight + 5*2,
		nLeftWidth, nHeight - nButtonHeight - 5*3, SWP_NOZORDER | SWP_NOACTIVATE);
	Window(getDlgItem(IDC_SELECTEDADDRESS)).setWindowPos(0,
		nLeftWidth + 5*2, nButtonHeight + 5*2, nRightWidth,
		nHeight - nButtonHeight - 5*3, SWP_NOZORDER | SWP_NOACTIVATE);
	
	int nDx[] = { 1, 5, 1, 1, 5, 0 };
	int nTop = 5;
	for (int n = 0; n < countof(hwnds); ++n) {
		Window(hwnds[n]).setWindowPos(0, nLeftWidth + nRightWidth + 5*3,
			nTop, 0, 0, SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE);
		nTop += nButtonHeight + nDx[n];
	}
	
	Window(getDlgItem(IDC_FILTERLABEL)).setWindowPos(
		0, 0, 0, 0, 0, SWP_NOZORDER | SWP_NOACTIVATE);
	
#ifndef _WIN32_WCE
	Window(getDlgItem(IDC_SIZEGRIP)).setWindowPos(0,
		rect.right - rect.left - 13, rect.bottom - rect.top - 12,
		0, 0, SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE);
#endif
#endif
}

HMENU qm::AddressBookDialog::createCategoryMenu(const AddressBook::CategoryList& l,
												CategoryNameList* pList)
{
	assert(pList);
	
	AutoMenuHandle hmenu(::CreatePopupMenu());
	
	typedef std::vector<std::pair<HMENU, WSTRING> > MenuStack;
	MenuStack stackMenu;
	stackMenu.push_back(MenuStack::value_type(hmenu.get(), 0));
	
	struct Deleter
	{
		typedef std::vector<std::pair<HMENU, WSTRING> > MenuStack;
		
		Deleter(MenuStack& s) :
			s_(s)
		{
		}
		
		~Deleter()
		{
			std::for_each(s_.begin(), s_.end(),
				unary_compose_f_gx(
					string_free<WSTRING>(),
					std::select2nd<MenuStack::value_type>()));
		}
		
		MenuStack& s_;
	} deleter(stackMenu);
	
	HINSTANCE hInst = Application::getApplication().getResourceHandle();
	wstring_ptr wstrThisCategory(loadString(hInst, IDS_THISCATEGORY));
	W2T(wstrThisCategory.get(), ptszThisCategory);
	
	UINT nId = IDM_ADDRESSBOOK_CATEGORY;
	for (AddressBook::CategoryList::const_iterator it = l.begin(); it != l.end(); ++it) {
		const AddressBookCategory* pCategory = *it;
		
		size_t nLevel = getCategoryLevel(pCategory->getName());
		
		while (true) {
			bool bPop = false;
			if (nLevel < stackMenu.size()) {
				bPop = true;
			}
			else if (stackMenu.size() > 1) {
				wstring_ptr wstrName(getCategoryName(
					pCategory->getName(), stackMenu.size() - 2, false));
				if (wcscmp(wstrName.get(), stackMenu.back().second) != 0)
					bPop = true;
			}
			if (!bPop)
				break;
			freeWString(stackMenu.back().second);
			stackMenu.pop_back();
		}
		
		while (nLevel >= stackMenu.size()) {
			wstring_ptr wstrName(getCategoryName(
				pCategory->getName(), stackMenu.size() - 1, false));
			
			wstring_ptr wstrText(UIUtil::formatMenu(wstrName.get()));
			W2T(wstrText.get(), ptszText);
			
			bool bSubMenu = false;
			if (nLevel > stackMenu.size()) {
				bSubMenu = true;
			}
			else {
				if (it + 1 != l.end()) {
					const WCHAR* pwszNext = (*(it + 1))->getName();
					size_t nLen = wcslen(pCategory->getName());
					bSubMenu = wcsncmp(pCategory->getName(), pwszNext, nLen) == 0 &&
						*(pwszNext + nLen) == L'/';
				}
			}
			
			wstring_ptr wstrFullName(getCategoryName(
				pCategory->getName(), stackMenu.size() - 1, true));
			bool bCheck = wstrCategory_.get() &&
				wcscmp(wstrFullName.get(), wstrCategory_.get()) == 0;
			pList->push_back(wstrFullName.get());
			wstrFullName.release();
			
			unsigned int nFlags = MF_STRING | (bCheck ? MF_CHECKED : 0);
			if (bSubMenu) {
				HMENU hSubMenu = ::CreatePopupMenu();
				::AppendMenu(stackMenu.back().first, MF_POPUP,
					reinterpret_cast<UINT_PTR>(hSubMenu), ptszText);
				stackMenu.push_back(std::make_pair(hSubMenu, wstrName.get()));
				wstrName.release();
				::AppendMenu(hSubMenu, nFlags, nId++, ptszThisCategory);
			}
			else {
				::AppendMenu(stackMenu.back().first, nFlags, nId++, ptszText);
				break;
			}
		}
	}
	
	::AppendMenu(hmenu.get(), MF_SEPARATOR, -1, 0);
	
	wstring_ptr wstrAll(loadString(hInst, IDS_ALLCATEGORY));
	W2T(wstrAll.get(), ptszAll);
	::AppendMenu(hmenu.get(), MF_STRING | (!wstrCategory_.get() ? MF_CHECKED : 0),
		IDM_ADDRESSBOOK_ALLCATEGORY, ptszAll);
	
	return hmenu.release();
}

void qm::AddressBookDialog::setCurrentCategory(const WCHAR* pwszCategory)
{
	if (pwszCategory)
		wstrCategory_ = allocWString(pwszCategory);
	else
		wstrCategory_.reset(0);
	
	HINSTANCE hInst = Application::getApplication().getResourceHandle();
	wstring_ptr wstrTitle(loadString(hInst, IDS_CATEGORY));
	wstring_ptr wstrAll(loadString(hInst, IDS_CATEGORYALL));
	
	ConcatW c[] = {
		{ wstrTitle.get(),												-1	},
		{ L" (",														2	},
		{ wstrCategory_.get() ? wstrCategory_.get() : wstrAll.get(),	-1	},
		{ L")",															1	}
	};
	wstring_ptr wstrButton(concat(c, countof(c)));
	setDlgItemText(IDC_CATEGORY, wstrButton.get());
	
	update();
}

bool qm::AddressBookDialog::isCategory(const AddressBookAddress::CategoryList& listCategory) const
{
	if (!wstrCategory_.get())
		return true;
	
	size_t nLen = wcslen(wstrCategory_.get());
	
	for (AddressBookAddress::CategoryList::const_iterator it = listCategory.begin(); it != listCategory.end(); ++it) {
		const AddressBookCategory* pCategory = *it;
		const WCHAR* pwszCategory = pCategory->getName();
		
		if (wcscmp(pwszCategory, wstrCategory_.get()) == 0)
			return true;
		else if (wcslen(pwszCategory) > nLen &&
			wcsncmp(pwszCategory, wstrCategory_.get(), nLen) == 0 &&
			*(pwszCategory + nLen) == L'/')
			return true;
	}
	
	return false;
}

bool qm::AddressBookDialog::isMatchFilter(const AddressBookEntry* pEntry) const
{
	if (!wstrFilter_.get())
		return true;
	
	wstring_ptr wstrName(tolower(pEntry->getName()));
	return wcsstr(wstrName.get(), wstrFilter_.get()) != 0;
}

bool qm::AddressBookDialog::isMatchFilter(const AddressBookAddress* pAddress) const
{
	if (!wstrFilter_.get())
		return true;
	return wcsstr(pAddress->getAddress(), wstrFilter_.get()) != 0;
}

size_t qm::AddressBookDialog::getCategoryLevel(const WCHAR* pwszCategory)
{
	assert(pwszCategory);
	return std::count(pwszCategory, pwszCategory + wcslen(pwszCategory), L'/') + 1;
}

wstring_ptr qm::AddressBookDialog::getCategoryName(const WCHAR* pwszCategory,
												   size_t nLevel,
												   bool bFull)
{
	assert(pwszCategory);
	
	const WCHAR* p = pwszCategory;
	while (nLevel != 0) {
		while (*p++ != L'/')
			;
		--nLevel;
	}
	
	const WCHAR* pEnd = wcschr(p, L'/');
	
	wstring_ptr wstrName;
	if (bFull) {
		size_t nLen = pEnd ? pEnd - pwszCategory : wcslen(pwszCategory);
		wstrName = allocWString(pwszCategory, nLen);
	}
	else {
		size_t nLen = pEnd ? pEnd - p : wcslen(p);
		wstrName = allocWString(p, nLen);
	}
	return wstrName;
}


/****************************************************************************
 *
 * AddressBookDialog::Item
 *
 */

qm::AddressBookDialog::Item::Item(wstring_ptr wstrValue,
								  Type type) :
	wstrValue_(wstrValue),
	type_(type)
{
}

qm::AddressBookDialog::Item::~Item()
{
}

const WCHAR* qm::AddressBookDialog::Item::getValue() const
{
	return wstrValue_.get();
}

wstring_ptr qm::AddressBookDialog::Item::releaseValue()
{
	return wstrValue_;
}

AddressBookDialog::Type qm::AddressBookDialog::Item::getType() const
{
	return type_;
}


/****************************************************************************
 *
 * AddressBookDialog::AddressListWindow
 *
 */

qm::AddressBookDialog::AddressListWindow::AddressListWindow(AddressBookDialog* pDialog) :
	WindowBase(false),
	pDialog_(pDialog)
{
	setWindowHandler(this, false);
}

qm::AddressBookDialog::AddressListWindow::~AddressListWindow()
{
}

LRESULT qm::AddressBookDialog::AddressListWindow::windowProc(UINT uMsg,
															 WPARAM wParam,
															 LPARAM lParam)
{
	BEGIN_MESSAGE_HANDLER()
		HANDLE_CHAR()
	END_MESSAGE_HANDLER()
	return DefaultWindowHandler::windowProc(uMsg, wParam, lParam);
}

LRESULT qm::AddressBookDialog::AddressListWindow::onChar(UINT nChar,
														 UINT nRepeat,
														 UINT nFlags)
{
	if (nChar == L' ') {
		pDialog_->select(AddressBookDialog::TYPE_TO);
		return 0;
	}
	return DefaultWindowHandler::onChar(nChar, nRepeat, nFlags);
}


/****************************************************************************
 *
 * AddressBookDialog::CategoryLess
 *
 */

bool qm::AddressBookDialog::CategoryLess::operator()(const AddressBookCategory* pLhs,
													 const AddressBookCategory* pRhs)
{
	const WCHAR* pwszLhs = pLhs->getName();
	const WCHAR* pwszRhs = pRhs->getName();
	
	while (*pwszLhs && *pwszRhs) {
		if (*pwszLhs == *pwszRhs)
			;
		else if (*pwszLhs == L'/')
			return true;
		else if (*pwszRhs == L'/')
			return false;
		else if (*pwszLhs < *pwszRhs)
			return true;
		else if (*pwszLhs > *pwszRhs)
			return false;
		
		++pwszLhs;
		++pwszRhs;
	}
	
	return *pwszRhs != L'\0';
}


/****************************************************************************
 *
 * AttachmentDialog
 *
 */

qm::AttachmentDialog::AttachmentDialog(EditMessage::AttachmentList& listAttachment) :
	DefaultDialog(IDD_ATTACHMENT),
	listAttachment_(listAttachment)
{
	std::sort(listAttachment_.begin(), listAttachment_.end(),
		EditMessage::AttachmentComp());
}

qm::AttachmentDialog::~AttachmentDialog()
{
}

LRESULT qm::AttachmentDialog::onCommand(WORD nCode,
										WORD nId)
{
	BEGIN_COMMAND_HANDLER()
		HANDLE_COMMAND_ID(IDC_ADD, onAdd)
		HANDLE_COMMAND_ID(IDC_REMOVE, onRemove)
	END_COMMAND_HANDLER()
	return DefaultDialog::onCommand(nCode, nId);
}

LRESULT qm::AttachmentDialog::onDestroy()
{
	removeNotifyHandler(this);
	return DefaultDialog::onDestroy();
}

LRESULT qm::AttachmentDialog::onInitDialog(HWND hwndFocus,
										   LPARAM lParam)
{
	init(false);
	
	HWND hwndList = getDlgItem(IDC_ATTACHMENT);
	
	SHFILEINFO info = { 0 };
	HIMAGELIST hImageList = reinterpret_cast<HIMAGELIST>(::SHGetFileInfo(
		_T("dummy.txt"), FILE_ATTRIBUTE_NORMAL, &info, sizeof(info),
		SHGFI_USEFILEATTRIBUTES | SHGFI_SYSICONINDEX | SHGFI_SMALLICON));
	ListView_SetImageList(hwndList, hImageList, LVSIL_SMALL);
	
	update();
	updateState();
	
	addNotifyHandler(this);
	
	return TRUE;
}

LRESULT qm::AttachmentDialog::onNotify(NMHDR* pnmhdr,
									   bool* pbHandled)
{
	BEGIN_NOTIFY_HANDLER()
		HANDLE_NOTIFY(LVN_ITEMCHANGED, IDC_ATTACHMENT, onAttachmentItemChanged);
	END_NOTIFY_HANDLER()
	return 1;
}

LRESULT qm::AttachmentDialog::onAdd()
{
	HINSTANCE hInst = Application::getApplication().getResourceHandle();
	wstring_ptr wstrFilter(loadString(hInst, IDS_FILTER_ATTACHMENT));
	
	FileDialog dialog(true, wstrFilter.get(), 0, 0, 0,
		OFN_EXPLORER | OFN_HIDEREADONLY | OFN_LONGNAMES | OFN_ALLOWMULTISELECT);
	if (dialog.doModal(getHandle()) == IDOK) {
		const WCHAR* pwszPath = dialog.getPath();
		const WCHAR* p = pwszPath;
		while (*p) {
			wstring_ptr wstrName(allocWString(p));
			EditMessage::Attachment attachment = {
				wstrName.get(),
				true
			};
			listAttachment_.push_back(attachment);
			wstrName.release();
			
			p += wcslen(p) + 1;
		}
		
		std::sort(listAttachment_.begin(), listAttachment_.end(),
			EditMessage::AttachmentComp());
		
		update();
	}
	
	updateState();
	
	return 0;
}

LRESULT qm::AttachmentDialog::onRemove()
{
	HWND hwndList = getDlgItem(IDC_ATTACHMENT);
	
	int nDeleted = 0;
	int nItem = -1;
	while (true) {
		nItem = ListView_GetNextItem(hwndList, nItem, LVNI_SELECTED);
		if (nItem == -1)
			break;
		EditMessage::AttachmentList::iterator it = listAttachment_.begin() + nItem - nDeleted;
		freeWString((*it).wstrName_);
		listAttachment_.erase(it);
		++nDeleted;
	}
	
	update();
	updateState();
	
	return 0;
}

LRESULT qm::AttachmentDialog::onAttachmentItemChanged(NMHDR* pnmhdr,
													  bool* pbHandled)
{
	updateState();
	*pbHandled = true;
	return 0;
}

void qm::AttachmentDialog::update()
{
	HWND hwndList = getDlgItem(IDC_ATTACHMENT);
	
	ListView_DeleteAllItems(hwndList);
	
	for (EditMessage::AttachmentList::size_type n = 0; n < listAttachment_.size(); ++n) {
		const EditMessage::Attachment& attachment = listAttachment_[n];
		
		wstring_ptr wstrName;
		int nIcon = 0;
		UIUtil::getAttachmentInfo(attachment, &wstrName, &nIcon);
		
		W2T(wstrName.get(), ptszName);
		LVITEM item = {
			LVIF_TEXT | LVIF_IMAGE,
			n,
			0,
			0,
			0,
			const_cast<LPTSTR>(ptszName),
			0,
			nIcon
		};
		ListView_InsertItem(hwndList, &item);
	}
}

void qm::AttachmentDialog::updateState()
{
	Window(getDlgItem(IDC_REMOVE)).enableWindow(
		ListView_GetSelectedCount(getDlgItem(IDC_ATTACHMENT)) != 0);
}


/****************************************************************************
 *
 * CreateAccountDialog
 *
 */

qm::CreateAccountDialog::CreateAccountDialog(Profile* pProfile) :
	DefaultDialog(IDD_CREATEACCOUNT),
	pProfile_(pProfile),
	nReceiveProtocol_(0),
	nSendProtocol_(0),
	nBlockSize_(-1),
	nCacheBlockSize_(-1)
{
	wstrClass_ = allocWString(L"mail");
}

qm::CreateAccountDialog::~CreateAccountDialog()
{
	clearProtocols();
}

const WCHAR* qm::CreateAccountDialog::getName() const
{
	return wstrName_.get();
}

const WCHAR* qm::CreateAccountDialog::getClass() const
{
	return wstrClass_.get();
}

const WCHAR* qm::CreateAccountDialog::getReceiveProtocol() const
{
	return listReceiveProtocol_[nReceiveProtocol_].wstrName_;
}

short qm::CreateAccountDialog::getReceivePort() const
{
	return listReceiveProtocol_[nReceiveProtocol_].nPort_;
}

const WCHAR* qm::CreateAccountDialog::getSendProtocol() const
{
	return listSendProtocol_[nSendProtocol_].wstrName_;
}

short qm::CreateAccountDialog::getSendPort() const
{
	return listSendProtocol_[nSendProtocol_].nPort_;
}

unsigned int qm::CreateAccountDialog::getBlockSize() const
{
	return nBlockSize_;
}

unsigned int qm::CreateAccountDialog::getCacheBlockSize() const
{
	return nCacheBlockSize_;
}

LRESULT qm::CreateAccountDialog::onCommand(WORD nCode,
										   WORD nId)
{
	BEGIN_COMMAND_HANDLER()
		HANDLE_COMMAND_ID_CODE(IDC_NAME, EN_CHANGE, onNameChange)
		HANDLE_COMMAND_ID_CODE(IDC_CLASS, CBN_SELENDOK, onClassChange)
		HANDLE_COMMAND_ID_CODE(IDC_INCOMINGPROTOCOL, CBN_SELENDOK, onProtocolChange)
		HANDLE_COMMAND_ID_CODE(IDC_OUTGOINGPROTOCOL, CBN_SELENDOK, onProtocolChange)
		HANDLE_COMMAND_ID(IDC_SINGLEFILE, onTypeChange)
		HANDLE_COMMAND_ID(IDC_MULTIPLEFILE, onTypeChange)
	END_COMMAND_HANDLER()
	return DefaultDialog::onCommand(nCode, nId);
}

LRESULT qm::CreateAccountDialog::onInitDialog(HWND hwndFocus,
											  LPARAM lParam)
{
	init(false);
	
	typedef ReceiveSessionFactory::NameList ClassList;
	ClassList listClasses;
	StringListFree<ClassList> free(listClasses);
	ReceiveSessionFactory::getClasses(&listClasses);
	for (ClassList::iterator it = listClasses.begin(); it != listClasses.end(); ++it) {
		W2T(*it, ptsz);
		sendDlgItemMessage(IDC_CLASS, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(ptsz));
	}
	sendDlgItemMessage(IDC_CLASS, CB_SETCURSEL, 0);
	
	updateProtocols();
	
	Window(getDlgItem(IDC_MULTIPLEFILE)).sendMessage(BM_SETCHECK, BST_CHECKED);
	setDlgItemInt(IDC_BLOCKSIZE, 0);
	setDlgItemInt(IDC_CACHEBLOCKSIZE, 0);
	
	updateState();
	
	return TRUE;
}

LRESULT qm::CreateAccountDialog::onOk()
{
	wstrName_ = getDlgItemText(IDC_NAME);
	nReceiveProtocol_ = sendDlgItemMessage(IDC_INCOMINGPROTOCOL, CB_GETCURSEL);
	nSendProtocol_ = sendDlgItemMessage(IDC_OUTGOINGPROTOCOL, CB_GETCURSEL);
	
	if (sendDlgItemMessage(IDC_SINGLEFILE, BM_GETCHECK) == BST_CHECKED) {
		nBlockSize_ = getDlgItemInt(IDC_BLOCKSIZE);
		if (nBlockSize_ == 0)
			nBlockSize_ = -1;
	}
	else {
		nBlockSize_ = 0;
	}
	
	nCacheBlockSize_ = getDlgItemInt(IDC_CACHEBLOCKSIZE);
	if (nCacheBlockSize_ == 0)
		nCacheBlockSize_ = -1;
	
	return DefaultDialog::onOk();
}

LRESULT qm::CreateAccountDialog::onNameChange()
{
	updateState();
	return 0;
}

LRESULT qm::CreateAccountDialog::onClassChange()
{
	int nItem = sendDlgItemMessage(IDC_CLASS, CB_GETCURSEL);
	int nLen = sendDlgItemMessage(IDC_CLASS, CB_GETLBTEXTLEN, nItem);
	tstring_ptr tstrClass(allocTString(nLen + 1));
	sendDlgItemMessage(IDC_CLASS, CB_GETLBTEXT,
		nItem, reinterpret_cast<LPARAM>(tstrClass.get()));
	
	wstring_ptr wstrClass(tcs2wcs(tstrClass.get()));
	if (wcscmp(wstrClass_.get(), wstrClass.get()) != 0) {
		wstrClass_ = wstrClass;
		updateProtocols();
	}
	
	return 0;
}

LRESULT qm::CreateAccountDialog::onProtocolChange()
{
	updateState();
	return 0;
}

LRESULT qm::CreateAccountDialog::onTypeChange()
{
	updateState();
	return 0;
}

void qm::CreateAccountDialog::updateProtocols()
{
	clearProtocols();
	sendDlgItemMessage(IDC_INCOMINGPROTOCOL, CB_RESETCONTENT);
	sendDlgItemMessage(IDC_OUTGOINGPROTOCOL, CB_RESETCONTENT);
	
	ReceiveSessionFactory::NameList listReceiveName;
	StringListFree<ReceiveSessionFactory::NameList> freeReceive(listReceiveName);
	ReceiveSessionFactory::getNames(&listReceiveName);
	listReceiveProtocol_.reserve(listReceiveName.size());
	for (ReceiveSessionFactory::NameList::iterator itR = listReceiveName.begin(); itR != listReceiveName.end(); ++itR) {
		wstring_ptr wstrName(*itR);
		*itR = 0;
		std::auto_ptr<ReceiveSessionUI> pUI(ReceiveSessionFactory::getUI(wstrName.get()));
		if (wcscmp(pUI->getClass(), wstrClass_.get()) == 0) {
			Protocol p = {
				wstrName.get(),
				pUI->getDefaultPort(false)
			};
			listReceiveProtocol_.push_back(p);
			wstrName.release();
			
			wstring_ptr wstrDisplayName(pUI->getDisplayName());
			W2T(wstrDisplayName.get(), ptszDisplayName);
			sendDlgItemMessage(IDC_INCOMINGPROTOCOL, CB_ADDSTRING,
				0, reinterpret_cast<LPARAM>(ptszDisplayName));
		}
	}
	sendDlgItemMessage(IDC_INCOMINGPROTOCOL, CB_SETCURSEL, 0);
	
	SendSessionFactory::NameList listSendName;
	StringListFree<SendSessionFactory::NameList> freeSend(listSendName);
	SendSessionFactory::getNames(&listSendName);
	listSendProtocol_.reserve(listSendName.size());
	for (SendSessionFactory::NameList::iterator itS = listSendName.begin(); itS != listSendName.end(); ++itS) {
		wstring_ptr wstrName(*itS);
		*itS = 0;
		std::auto_ptr<SendSessionUI> pUI(SendSessionFactory::getUI(wstrName.get()));
		if (wcscmp(pUI->getClass(), wstrClass_.get()) == 0) {
			Protocol p = {
				wstrName.get(),
				pUI->getDefaultPort(false)
			};
			listSendProtocol_.push_back(p);
			wstrName.release();
			
			wstring_ptr wstrDisplayName(pUI->getDisplayName());
			W2T(wstrDisplayName.get(), ptszDisplayName);
			sendDlgItemMessage(IDC_OUTGOINGPROTOCOL, CB_ADDSTRING,
				0, reinterpret_cast<LPARAM>(ptszDisplayName));
		}
	}
	sendDlgItemMessage(IDC_OUTGOINGPROTOCOL, CB_SETCURSEL, 0);
	
	updateState();
}

void qm::CreateAccountDialog::clearProtocols()
{
	std::for_each(listReceiveProtocol_.begin(), listReceiveProtocol_.end(),
		unary_compose_f_gx(
			string_free<WSTRING>(),
			mem_data_ref(&Protocol::wstrName_)));
	listReceiveProtocol_.clear();
	std::for_each(listSendProtocol_.begin(), listSendProtocol_.end(),
		unary_compose_f_gx(
			string_free<WSTRING>(),
			mem_data_ref(&Protocol::wstrName_)));
	listSendProtocol_.clear();
}

void qm::CreateAccountDialog::updateState()
{
	bool bEnableOk = Window(getDlgItem(IDC_NAME)).getWindowTextLength() != 0 &&
		sendDlgItemMessage(IDC_INCOMINGPROTOCOL, CB_GETCURSEL) != CB_ERR &&
		sendDlgItemMessage(IDC_OUTGOINGPROTOCOL, CB_GETCURSEL) != CB_ERR;
	Window(getDlgItem(IDOK)).enableWindow(bEnableOk);
	
	bool bEnableBlockSize = sendDlgItemMessage(
		IDC_SINGLEFILE, BM_GETCHECK) == BST_CHECKED;
	Window(getDlgItem(IDC_BLOCKSIZE)).enableWindow(bEnableBlockSize);
}


/****************************************************************************
 *
 * CreateFolderDialog
 *
 */

qm::CreateFolderDialog::CreateFolderDialog(Type type,
										   unsigned int nFlags) :
	DefaultDialog(IDD_CREATEFOLDER),
	type_(type),
	nFlags_(nFlags),
	bSyncable_(false)
{
}

qm::CreateFolderDialog::~CreateFolderDialog()
{
}

CreateFolderDialog::Type qm::CreateFolderDialog::getType() const
{
	return type_;
}

const WCHAR* qm::CreateFolderDialog::getName() const
{
	return wstrName_.get();
}

bool qm::CreateFolderDialog::isSyncable() const
{
	return bSyncable_;
}

LRESULT qm::CreateFolderDialog::onCommand(WORD nCode,
										  WORD nId)
{
	BEGIN_COMMAND_HANDLER()
		HANDLE_COMMAND_ID_CODE(IDC_NAME, EN_CHANGE, onNameChange)
		HANDLE_COMMAND_ID_RANGE_CODE(IDC_LOCALFOLDER,
			IDC_QUERYFOLDER, BN_CLICKED, onTypeChange)
	END_COMMAND_HANDLER()
	return DefaultDialog::onCommand(nCode, nId);
}

LRESULT qm::CreateFolderDialog::onInitDialog(HWND hwndFocus,
											 LPARAM lParam)
{
	init(false);
	
	Window(getDlgItem(IDC_REMOTEFOLDER)).enableWindow(
		(nFlags_ & FLAG_ALLOWREMOTE) != 0);
	if (nFlags_ & FLAG_ALLOWLOCALSYNC)
		sendDlgItemMessage(IDC_SYNCABLE, BM_SETCHECK, BST_CHECKED);
	
	UINT nIds[] = {
		IDC_LOCALFOLDER,
		IDC_REMOTEFOLDER,
		IDC_QUERYFOLDER
	};
	sendDlgItemMessage(nIds[type_], BM_SETCHECK, BST_CHECKED);
	
	updateState();
	
	return TRUE;
}

LRESULT qm::CreateFolderDialog::onOk()
{
	UINT nIds[] = {
		IDC_LOCALFOLDER,
		IDC_REMOTEFOLDER,
		IDC_QUERYFOLDER
	};
	for (int n = 0; n < countof(nIds); ++n) {
		if (sendDlgItemMessage(nIds[n], BM_GETCHECK) == BST_CHECKED) {
			type_ = static_cast<Type>(n);
			break;
		}
	}
	
	wstrName_ = getDlgItemText(IDC_NAME);
	bSyncable_ = sendDlgItemMessage(IDC_SYNCABLE, BM_GETCHECK) == BST_CHECKED;
	
	return DefaultDialog::onOk();
}

LRESULT qm::CreateFolderDialog::onNameChange()
{
	updateState();
	return 0;
}

LRESULT qm::CreateFolderDialog::onTypeChange(UINT nId)
{
	updateState();
	return 0;
}

void qm::CreateFolderDialog::updateState()
{
	Window(getDlgItem(IDOK)).enableWindow(
		Window(getDlgItem(IDC_NAME)).getWindowTextLength() != 0);
	Window(getDlgItem(IDC_SYNCABLE)).enableWindow(
		nFlags_ & FLAG_ALLOWLOCALSYNC &&
		sendDlgItemMessage(IDC_LOCALFOLDER, BM_GETCHECK) == BST_CHECKED);
}


/****************************************************************************
 *
 * CreateSubAccountDialog
 *
 */

qm::CreateSubAccountDialog::CreateSubAccountDialog(Document* pDocument) :
	DefaultDialog(IDD_CREATESUBACCOUNT),
	pDocument_(pDocument)
{
}

qm::CreateSubAccountDialog::~CreateSubAccountDialog()
{
}

const WCHAR* qm::CreateSubAccountDialog::getName() const
{
	return wstrName_.get();
}

LRESULT qm::CreateSubAccountDialog::onCommand(WORD nCode,
											  WORD nId)
{
	BEGIN_COMMAND_HANDLER()
		HANDLE_COMMAND_ID_CODE(IDC_NAME, CBN_EDITCHANGE, onNameChanged)
	END_COMMAND_HANDLER()
	return DefaultDialog::onCommand(nCode, nId);
}

LRESULT qm::CreateSubAccountDialog::onInitDialog(HWND hwndFocus,
												 LPARAM lParam)
{
	init(false);
	
	const Document::AccountList& listAccount = pDocument_->getAccounts();
	for (Document::AccountList::const_iterator itA = listAccount.begin(); itA != listAccount.end(); ++itA) {
		Account* pAccount = *itA;
		
		const Account::SubAccountList& listSubAccount = pAccount->getSubAccounts();
		for (Account::SubAccountList::const_iterator itS = listSubAccount.begin(); itS != listSubAccount.end(); ++itS) {
			SubAccount* pSubAccount = *itS;
			
			const WCHAR* pwszName = pSubAccount->getName();
			if (*pwszName) {
				W2T(pwszName, ptszName);
				sendDlgItemMessage(IDC_NAME, CB_ADDSTRING, 0,
					reinterpret_cast<LPARAM>(ptszName));
			}
		}
	}
	
	updateState();
	
	return TRUE;
}

LRESULT qm::CreateSubAccountDialog::onOk()
{
	wstrName_ = Window(getDlgItem(IDC_NAME)).getWindowText();
	return DefaultDialog::onOk();
}

LRESULT qm::CreateSubAccountDialog::onNameChanged()
{
	updateState();
	return 0;
}

void qm::CreateSubAccountDialog::updateState()
{
	Window(getDlgItem(IDOK)).enableWindow(
		Window(getDlgItem(IDC_NAME)).getWindowTextLength() != 0);
}


/****************************************************************************
 *
 * CustomFilterDialog
 *
 */

qm::CustomFilterDialog::CustomFilterDialog(const WCHAR* pwszMacro) :
	DefaultDialog(IDD_CUSTOMFILTER)
{
	if (pwszMacro)
		wstrMacro_ = allocWString(pwszMacro);
}

qm::CustomFilterDialog::~CustomFilterDialog()
{
}

const WCHAR* qm::CustomFilterDialog::getMacro() const
{
	return wstrMacro_.get();
}

LRESULT qm::CustomFilterDialog::onInitDialog(HWND hwndFocus,
											 LPARAM lParam)
{
	init(false);
	
	if (wstrMacro_.get())
		setDlgItemText(IDC_MACRO, wstrMacro_.get());
	
	return TRUE;
}

LRESULT qm::CustomFilterDialog::onOk()
{
	wstrMacro_ = getDlgItemText(IDC_MACRO);
	return DefaultDialog::onOk();
}


/****************************************************************************
 *
 * DetachDialog
 *
 */

qm::DetachDialog::DetachDialog(Profile* pProfile,
							   List& list) :
	DefaultDialog(IDD_DETACH),
	pProfile_(pProfile),
	list_(list)
{
	wstrFolder_ = pProfile_->getString(L"Global", L"DetachFolder", L"");
}

qm::DetachDialog::~DetachDialog()
{
}

const WCHAR* qm::DetachDialog::getFolder() const
{
	return wstrFolder_.get();
}

LRESULT qm::DetachDialog::onCommand(WORD nCode,
									WORD nId)
{
	BEGIN_COMMAND_HANDLER()
		HANDLE_COMMAND_ID(IDC_BROWSE, onBrowse)
		HANDLE_COMMAND_ID(IDC_RENAME, onRename)
	END_COMMAND_HANDLER()
	return DefaultDialog::onCommand(nCode, nId);
}

LRESULT qm::DetachDialog::onDestroy()
{
	removeNotifyHandler(this);
	return DefaultDialog::onDestroy();
}

LRESULT qm::DetachDialog::onInitDialog(HWND hwndFocus,
									   LPARAM lParam)
{
	init(false);
	
	HWND hwndList = getDlgItem(IDC_ATTACHMENT);
	ListView_SetExtendedListViewStyle(hwndList, LVS_EX_CHECKBOXES);
	for (List::size_type n = 0; n < list_.size(); ++n) {
		W2T(list_[n].wstrName_, ptszName);
		LVITEM item = {
			LVIF_TEXT,
			n,
			0,
			0,
			0,
			const_cast<LPTSTR>(ptszName),
			0
		};
		ListView_InsertItem(hwndList, &item);
		if (list_[n].bSelected_)
			ListView_SetCheckState(hwndList, n, TRUE);
	}
	
	setDlgItemText(IDC_FOLDER, wstrFolder_.get());
	updateState();
	
	addNotifyHandler(this);
	
	return TRUE;
}

LRESULT qm::DetachDialog::onOk()
{
	HWND hwndList = getDlgItem(IDC_ATTACHMENT);
	for (List::size_type n = 0; n < list_.size(); ++n) {
		if (ListView_GetCheckState(hwndList, n)) {
			TCHAR tszName[MAX_PATH];
			ListView_GetItemText(hwndList, n, 0, tszName, countof(tszName));
			wstring_ptr wstrName(tcs2wcs(tszName));
			if (wstrName.get() && wcscmp(wstrName.get(), list_[n].wstrName_) != 0) {
				freeWString(list_[n].wstrName_);
				list_[n].wstrName_ = wstrName.release();
			}
		}
		else {
			freeWString(list_[n].wstrName_);
			list_[n].wstrName_ = 0;
		}
	}
	
	wstrFolder_ = getDlgItemText(IDC_FOLDER);
	pProfile_->setString(L"Global", L"DetachFolder", wstrFolder_.get());
	
	return DefaultDialog::onOk();
}

LRESULT qm::DetachDialog::onNotify(NMHDR* pnmhdr,
								   bool* pbHandled)
{
	BEGIN_NOTIFY_HANDLER()
		HANDLE_NOTIFY(LVN_ENDLABELEDIT, IDC_ATTACHMENT, onAttachmentEndLabelEdit)
		HANDLE_NOTIFY(LVN_ITEMCHANGED, IDC_ATTACHMENT, onAttachmentItemChanged)
	END_NOTIFY_HANDLER()
	return 1;
}

LRESULT qm::DetachDialog::onBrowse()
{
	wstring_ptr wstrFolder(getDlgItemText(IDC_FOLDER));
	
	wstring_ptr wstrPath(qs::UIUtil::browseFolder(getHandle(), 0, wstrFolder.get()));
	if (wstrPath.get())
		setDlgItemText(IDC_FOLDER, wstrPath.get());
	
	return 0;
}

LRESULT qm::DetachDialog::onRename()
{
	HWND hwndList = getDlgItem(IDC_ATTACHMENT);
	Window(hwndList).setFocus();
	for (int n = 0; n < ListView_GetItemCount(hwndList); ++n) {
		if (ListView_GetItemState(hwndList, n, LVIS_FOCUSED)) {
			ListView_EditLabel(hwndList, n);
			break;
		}
	}
	
	return 0;
}

LRESULT qm::DetachDialog::onAttachmentEndLabelEdit(NMHDR* pnmhdr,
												   bool* pbHandled)
{
	*pbHandled = true;
	
	NMLVDISPINFO* pDispInfo = reinterpret_cast<NMLVDISPINFO*>(pnmhdr);
	if (pDispInfo->item.iItem == -1 || !pDispInfo->item.pszText)
		return 0;
	
	ListView_SetItemText(pDispInfo->hdr.hwndFrom,
		pDispInfo->item.iItem, 0, pDispInfo->item.pszText);
	
	return 1;
}

LRESULT qm::DetachDialog::onAttachmentItemChanged(NMHDR* pnmhdr,
												  bool* pbHandled)
{
	updateState();
	*pbHandled = true;
	return 0;
}

void qm::DetachDialog::updateState()
{
	HWND hwndList = getDlgItem(IDC_ATTACHMENT);
	Window(getDlgItem(IDC_RENAME)).enableWindow(
		ListView_GetSelectedCount(hwndList) != 0);
}


/****************************************************************************
 *
 * DialupDialog
 *
 */

qm::DialupDialog::DialupDialog(const WCHAR* pwszEntry,
							   const WCHAR* pwszUserName,
							   const WCHAR* pwszPassword,
							   const WCHAR* pwszDomain) :
	DefaultDialog(IDD_DIALUP)
{
	wstrEntry_ = allocWString(pwszEntry);
	wstrUserName_ = allocWString(pwszUserName);
	wstrPassword_ = allocWString(pwszPassword);
	wstrDomain_ = allocWString(pwszDomain);
}

qm::DialupDialog::~DialupDialog()
{
}

const WCHAR* qm::DialupDialog::getUserName() const
{
	return wstrUserName_.get();
}

const WCHAR* qm::DialupDialog::getPassword() const
{
	return wstrPassword_.get();
}

const WCHAR* qm::DialupDialog::getDomain() const
{
	return wstrDomain_.get();
}

LRESULT qm::DialupDialog::onCommand(WORD nCode,
									WORD nId)
{
	BEGIN_COMMAND_HANDLER()
		HANDLE_COMMAND_ID(IDC_DIALPROPERTY, onDialProperty)
	END_COMMAND_HANDLER()
	return DefaultDialog::onCommand(nCode, nId);
}

LRESULT qm::DialupDialog::onInitDialog(HWND hwndFocus,
									   LPARAM lParam)
{
	init(false);
	
	struct {
		UINT nId_;
		const WCHAR* pwsz_;
	} items[] = {
		{ IDC_ENTRY,	wstrEntry_.get()	},
		{ IDC_USERNAME,	wstrUserName_.get()	},
		{ IDC_PASSWORD,	wstrPassword_.get()	},
		{ IDC_DOMAIN,	wstrDomain_.get()	}
	};
	for (int n = 0; n < countof(items); ++n)
		setDlgItemText(items[n].nId_, items[n].pwsz_);
	
	updateLocation();
	
	if (!*wstrPassword_.get()) {
		Window(getDlgItem(IDC_PASSWORD)).setFocus();
		return FALSE;
	}
	
	return TRUE;
}

LRESULT qm::DialupDialog::onOk()
{
	struct {
		UINT nId_;
		wstring_ptr* pwstr_;
	} items[] = {
		{ IDC_ENTRY,	&wstrEntry_		},
		{ IDC_USERNAME,	&wstrUserName_	},
		{ IDC_PASSWORD,	&wstrPassword_	},
		{ IDC_DOMAIN,	&wstrDomain_	}
	};
	for (int n = 0; n < countof(items); ++n) {
		wstring_ptr wstr(getDlgItemText(items[n].nId_));
		if (wstr.get())
			*items[n].pwstr_ = wstr;
	}
	
	return DefaultDialog::onOk();
}

LRESULT qm::DialupDialog::onDialProperty()
{
	RasConnection::selectLocation(getHandle());
	updateLocation();
	return 0;
}

void qm::DialupDialog::updateLocation()
{
	wstring_ptr wstrLocation(RasConnection::getLocation());
	if (wstrLocation.get())
		setDlgItemText(IDC_DIALFROM, wstrLocation.get());
}


/****************************************************************************
 *
 * ExportDialog
 *
 */

qm::ExportDialog::ExportDialog(Account* pAccount,
							   const TemplateManager* pTemplateManager,
							   Profile* pProfile,
							   bool bSingleMessage) :
	DefaultDialog(IDD_EXPORT),
	pAccount_(pAccount),
	pTemplateManager_(pTemplateManager),
	pProfile_(pProfile),
	bSingleMessage_(bSingleMessage),
	nFlags_(0)
{
}

qm::ExportDialog::~ExportDialog()
{
}

const WCHAR* qm::ExportDialog::getPath() const
{
	return wstrPath_.get();
}

bool qm::ExportDialog::isFilePerMessage() const
{
	return (nFlags_ & FLAG_FILEPERMESSAGE) != 0;
}

bool qm::ExportDialog::isExportFlags() const
{
	return (nFlags_ & FLAG_EXPORTFLAGS) != 0;
}

const WCHAR* qm::ExportDialog::getTemplate() const
{
	return wstrTemplate_.get();
}

const WCHAR* qm::ExportDialog::getEncoding() const
{
	return wstrEncoding_.get();
}

LRESULT qm::ExportDialog::onCommand(WORD nCode,
									WORD nId)
{
	BEGIN_COMMAND_HANDLER()
		HANDLE_COMMAND_ID(IDC_BROWSE, onBrowse)
		HANDLE_COMMAND_ID_CODE(IDC_PATH, EN_CHANGE, onPathChange)
		HANDLE_COMMAND_ID_CODE(IDC_TEMPLATE, CBN_SELCHANGE, onTemplateSelChange)
		HANDLE_COMMAND_ID_CODE(IDC_ENCODING, CBN_EDITCHANGE, onEncodingEditChange)
		HANDLE_COMMAND_ID_CODE(IDC_ENCODING, CBN_SELCHANGE, onEncodingSelChange)
	END_COMMAND_HANDLER()
	return DefaultDialog::onCommand(nCode, nId);
}

LRESULT qm::ExportDialog::onInitDialog(HWND hwndFocus,
									   LPARAM lParam)
{
	init(false);
	
	if (bSingleMessage_)
		Window(getDlgItem(IDC_FILEPERMESSAGE)).enableWindow(false);
	
	HINSTANCE hInst = Application::getApplication().getResourceHandle();
	wstring_ptr wstrNone(loadString(hInst, IDS_NONE));
	W2T(wstrNone.get(), ptszNone);
	sendDlgItemMessage(IDC_TEMPLATE, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(ptszNone));
	
	TemplateManager::NameList listTemplate;
	StringListFree<TemplateManager::NameList> freeTemplate(listTemplate);
	pTemplateManager_->getTemplateNames(pAccount_, L"export", &listTemplate);
	for (TemplateManager::NameList::const_iterator it = listTemplate.begin(); it != listTemplate.end(); ++it) {
		W2T(*it + 7, ptszTemplate);
		sendDlgItemMessage(IDC_TEMPLATE, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(ptszTemplate));
	}
	sendDlgItemMessage(IDC_TEMPLATE, CB_SETCURSEL, 0);
	
	UIUtil::EncodingList listEncoding;
	StringListFree<UIUtil::EncodingList> freeEncoding(listEncoding);
	UIUtil::loadEncodings(pProfile_, &listEncoding);
	for (UIUtil::EncodingList::const_iterator it = listEncoding.begin(); it != listEncoding.end(); ++it) {
		W2T(*it, ptszEncoding);
		sendDlgItemMessage(IDC_ENCODING, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(ptszEncoding));
	}
	sendDlgItemMessage(IDC_ENCODING, CB_SETCURSEL, 0);
	
	updateState();
	
	return TRUE;
}

LRESULT qm::ExportDialog::onOk()
{
	wstrPath_ = getDlgItemText(IDC_PATH);
	
	nFlags_ = 0;
	struct {
		UINT nId_;
		Flag flag_;
	} flags[] = {
		{ IDC_FILEPERMESSAGE,		FLAG_FILEPERMESSAGE	},
		{ IDC_EXPORTFLAGS,			FLAG_EXPORTFLAGS	}
	};
	for (int n = 0; n < countof(flags); ++n) {
		if (sendDlgItemMessage(flags[n].nId_, BM_GETCHECK) == BST_CHECKED)
			nFlags_ |= flags[n].flag_;
	}
	
	int nIndex = sendDlgItemMessage(IDC_TEMPLATE, CB_GETCURSEL);
	if (nIndex != CB_ERR && nIndex != 0) {
		int nLen = sendDlgItemMessage(IDC_TEMPLATE, CB_GETLBTEXTLEN, nIndex);
		tstring_ptr tstrTemplate(allocTString(nLen + 1));
		sendDlgItemMessage(IDC_TEMPLATE, CB_GETLBTEXT,
			nIndex, reinterpret_cast<LPARAM>(tstrTemplate.get()));
		wstring_ptr wstrTemplate(tcs2wcs(tstrTemplate.get()));
		wstrTemplate_ = concat(L"export_", wstrTemplate.get());
		
		wstrEncoding_ = getDlgItemText(IDC_ENCODING);
	}
	
	return DefaultDialog::onOk();
}

LRESULT qm::ExportDialog::onBrowse()
{
	HINSTANCE hInst = Application::getApplication().getResourceHandle();
	wstring_ptr wstrFilter(loadString(hInst, IDS_FILTER_EXPORT));
	
	FileDialog dialog(false, wstrFilter.get(), 0, 0, 0,
		OFN_EXPLORER | OFN_HIDEREADONLY | OFN_LONGNAMES | OFN_OVERWRITEPROMPT);
	
	if (dialog.doModal(getHandle()) == IDOK) {
		setDlgItemText(IDC_PATH, dialog.getPath());
		updateState();
	}
	
	return 0;
}

LRESULT qm::ExportDialog::onPathChange()
{
	updateState();
	return 0;
}

LRESULT qm::ExportDialog::onTemplateSelChange()
{
	updateState();
	return 0;
}

LRESULT qm::ExportDialog::onEncodingEditChange()
{
	updateState();
	return 0;
}

LRESULT qm::ExportDialog::onEncodingSelChange()
{
	postMessage(WM_COMMAND, MAKEWPARAM(IDC_ENCODING, CBN_EDITCHANGE));
	return 0;
}

void qm::ExportDialog::updateState()
{
	int nIndex = sendDlgItemMessage(IDC_TEMPLATE, CB_GETCURSEL);
	Window(getDlgItem(IDC_ENCODING)).enableWindow(nIndex != 0 && nIndex != CB_ERR);
	
	bool bEnable = sendDlgItemMessage(IDC_PATH, WM_GETTEXTLENGTH) != 0 &&
		(nIndex == 0 || sendDlgItemMessage(IDC_ENCODING, WM_GETTEXTLENGTH) != 0);
	Window(getDlgItem(IDOK)).enableWindow(bEnable);
}


/****************************************************************************
 *
 * FindDialog
 *
 */

qm::FindDialog::FindDialog(Profile* pProfile,
						   bool bSupportRegex) :
	DefaultDialog(IDD_FIND),
	pProfile_(pProfile),
	bSupportRegex_(bSupportRegex),
	bMatchCase_(false),
	bRegex_(false),
	bPrev_(false)
{
}

qm::FindDialog::~FindDialog()
{
}

const WCHAR* qm::FindDialog::getFind() const
{
	return wstrFind_.get();
}

bool qm::FindDialog::isMatchCase() const
{
	return bMatchCase_;
}

bool qm::FindDialog::isRegex() const
{
	return bRegex_;
}

bool qm::FindDialog::isPrev() const
{
	return bPrev_;
}

LRESULT qm::FindDialog::onCommand(WORD nCode,
								  WORD nId)
{
	BEGIN_COMMAND_HANDLER()
		HANDLE_COMMAND_ID_RANGE(IDC_FINDNEXT, IDC_FINDPREV, onFind)
		HANDLE_COMMAND_ID_CODE(IDC_FIND, CBN_EDITCHANGE, onFindChange)
		HANDLE_COMMAND_ID_CODE(IDC_FIND, CBN_SELCHANGE, onFindSelChange)
		HANDLE_COMMAND_ID(IDC_REGEX, onRegexChange)
	END_COMMAND_HANDLER()
	return DefaultDialog::onCommand(nCode, nId);
}

LRESULT qm::FindDialog::onInitDialog(HWND hwndFocus,
									 LPARAM lParam)
{
	init(false);
	
	History history(pProfile_, L"Find");
	for (unsigned int n = 0; n < history.getSize(); ++n) {
		wstring_ptr wstr(history.getValue(n));
		if (*wstr.get()) {
			W2T(wstr.get(), ptsz);
			sendDlgItemMessage(IDC_FIND, CB_ADDSTRING,
				0, reinterpret_cast<LPARAM>(ptsz));
		}
	}
	if (sendDlgItemMessage(IDC_FIND, CB_GETCOUNT) != 0)
		sendDlgItemMessage(IDC_FIND, CB_SETCURSEL, 0);
	
	int nMatchCase = pProfile_->getInt(L"Find", L"MatchCase", 0);
	sendDlgItemMessage(IDC_MATCHCASE, BM_SETCHECK,
		nMatchCase ? BST_CHECKED : BST_UNCHECKED);
	
	if (bSupportRegex_) {
		int nRegex = pProfile_->getInt(L"Find", L"Regex", 0);
		sendDlgItemMessage(IDC_REGEX, BM_SETCHECK,
			nRegex ? BST_CHECKED : BST_UNCHECKED);
	}
	
	updateState();
	
	return TRUE;
}

LRESULT qm::FindDialog::onFind(UINT nId)
{
	Window edit(Window(getDlgItem(IDC_FIND)).getWindow(GW_CHILD));
	wstrFind_ = edit.getWindowText();
	if (wstrFind_.get())
		History(pProfile_, L"Find").addValue(wstrFind_.get());
	
	bMatchCase_ = sendDlgItemMessage(IDC_MATCHCASE, BM_GETCHECK) == BST_CHECKED;
	pProfile_->setInt(L"Find", L"MatchCase", bMatchCase_ ? 1 : 0);
	
	bRegex_ = sendDlgItemMessage(IDC_REGEX, BM_GETCHECK) == BST_CHECKED;
	pProfile_->setInt(L"Find", L"Regex", bRegex_ ? 1 : 0);
	
	bPrev_ = nId == IDC_FINDPREV;
	
	endDialog(IDOK);
	
	return 0;
}

LRESULT qm::FindDialog::onFindChange()
{
	updateState();
	return 0;
}

LRESULT qm::FindDialog::onFindSelChange()
{
	postMessage(WM_COMMAND, MAKEWPARAM(IDC_FIND, CBN_EDITCHANGE));
	return 0;
}

LRESULT qm::FindDialog::onRegexChange()
{
	updateState();
	return 0;
}

void qm::FindDialog::updateState()
{
	Window(getDlgItem(IDC_REGEX)).enableWindow(bSupportRegex_);
	bool bRegex = bSupportRegex_ &&
		sendDlgItemMessage(IDC_REGEX, BM_GETCHECK) == BST_CHECKED;
	Window(getDlgItem(IDC_MATCHCASE)).enableWindow(!bRegex);
	
	Window edit(Window(getDlgItem(IDC_FIND)).getWindow(GW_CHILD));
	bool bEnable = edit.getWindowTextLength() != 0;
	Window(getDlgItem(IDC_FINDNEXT)).enableWindow(bEnable);
	Window(getDlgItem(IDC_FINDPREV)).enableWindow(bEnable);
}


/****************************************************************************
 *
 * GoRoundDialog
 *
 */

qm::GoRoundDialog::GoRoundDialog(GoRound* pGoRound,
								 Document* pDocument,
								 SyncFilterManager* pSyncFilterManager) :
	DefaultDialog(IDD_GOROUND),
	pGoRound_(pGoRound),
	pDocument_(pDocument),
	pSyncFilterManager_(pSyncFilterManager)
{
	const GoRound::CourseList& listCourse = pGoRound_->getCourses();
	listCourse_.reserve(listCourse.size());
	for (GoRound::CourseList::const_iterator it = listCourse.begin(); it != listCourse.end(); ++it)
		listCourse_.push_back(new GoRoundCourse(**it));
}

qm::GoRoundDialog::~GoRoundDialog()
{
	std::for_each(listCourse_.begin(), listCourse_.end(), qs::deleter<GoRoundCourse>());
}

LRESULT qm::GoRoundDialog::onCommand(WORD nCode,
									 WORD nId)
{
	BEGIN_COMMAND_HANDLER()
		HANDLE_COMMAND_ID(IDC_ADD, onAdd)
		HANDLE_COMMAND_ID(IDC_DOWN, onDown)
		HANDLE_COMMAND_ID(IDC_EDIT, onEdit)
		HANDLE_COMMAND_ID(IDC_REMOVE, onRemove)
		HANDLE_COMMAND_ID(IDC_UP, onUp)
		HANDLE_COMMAND_ID_CODE(IDC_COURSE, LBN_SELCHANGE, onCourseSelChange)
	END_COMMAND_HANDLER()
	return DefaultDialog::onCommand(nCode, nId);
}

LRESULT qm::GoRoundDialog::onInitDialog(HWND hwndFocus,
										LPARAM lParam)
{
	init(false);
	
	for (GoRound::CourseList::const_iterator it = listCourse_.begin(); it != listCourse_.end(); ++it) {
		GoRoundCourse* pCourse = *it;
		
		W2T(pCourse->getName(), ptszName);
		sendDlgItemMessage(IDC_COURSE, LB_ADDSTRING,
			0, reinterpret_cast<LPARAM>(ptszName));
	}
	sendDlgItemMessage(IDC_COURSE, LB_SETCURSEL);
	
	updateState();
	
	return TRUE;
}

LRESULT qm::GoRoundDialog::onOk()
{
	pGoRound_->setCourses(listCourse_);
	if (!pGoRound_->save()) {
		// TODO MSG
	}
	
	return DefaultDialog::onOk();
}

LRESULT qm::GoRoundDialog::onAdd()
{
	std::auto_ptr<GoRoundCourse> pCourse(new GoRoundCourse());
	GoRoundCourseDialog dialog(pCourse.get(), pDocument_, pSyncFilterManager_);
	if (dialog.doModal(getHandle()) == IDOK) {
		listCourse_.push_back(pCourse.get());
		GoRoundCourse* p = pCourse.release();
		
		W2T(p->getName(), ptszName);
		sendDlgItemMessage(IDC_COURSE, LB_ADDSTRING,
			0, reinterpret_cast<LPARAM>(ptszName));
	}
	return 0;
}

LRESULT qm::GoRoundDialog::onRemove()
{
	int n = sendDlgItemMessage(IDC_COURSE, LB_GETCURSEL);
	if (n == LB_ERR)
		return 0;
	
	delete listCourse_[n];
	listCourse_.erase(listCourse_.begin() + n);
	
	sendDlgItemMessage(IDC_COURSE, LB_DELETESTRING, n);
	
	return 0;
}

LRESULT qm::GoRoundDialog::onEdit()
{
	int n = sendDlgItemMessage(IDC_COURSE, LB_GETCURSEL);
	if (n == LB_ERR)
		return 0;
	
	GoRoundCourse* pCourse = listCourse_[n];
	GoRoundCourseDialog dialog(pCourse, pDocument_, pSyncFilterManager_);
	if (dialog.doModal(getHandle()) == IDOK) {
		sendDlgItemMessage(IDC_COURSE, LB_DELETESTRING, n);
		W2T(pCourse->getName(), ptszName);
		sendDlgItemMessage(IDC_COURSE, LB_INSERTSTRING,
			n, reinterpret_cast<LPARAM>(ptszName));
		sendDlgItemMessage(IDC_COURSE, LB_SETCURSEL, n);
	}
	
	return 0;
}

LRESULT qm::GoRoundDialog::onUp()
{
	int n = sendDlgItemMessage(IDC_COURSE, LB_GETCURSEL);
	if (n == LB_ERR || n == 0)
		return 0;
	
	GoRoundCourse* pCourse = listCourse_[n];
	std::swap(listCourse_[n], listCourse_[n - 1]);
	
	sendDlgItemMessage(IDC_COURSE, LB_DELETESTRING, n);
	W2T(pCourse->getName(), ptszName);
	sendDlgItemMessage(IDC_COURSE, LB_INSERTSTRING,
		n - 1, reinterpret_cast<LPARAM>(ptszName));
	sendDlgItemMessage(IDC_COURSE, LB_SETCURSEL, n - 1);
	
	return 0;
}

LRESULT qm::GoRoundDialog::onDown()
{
	int n = sendDlgItemMessage(IDC_COURSE, LB_GETCURSEL);
	if (n == LB_ERR || n == sendDlgItemMessage(IDC_COURSE, LB_GETCOUNT) - 1)
		return 0;
	
	GoRoundCourse* pCourse = listCourse_[n];
	std::swap(listCourse_[n], listCourse_[n + 1]);
	
	sendDlgItemMessage(IDC_COURSE, LB_DELETESTRING, n);
	W2T(pCourse->getName(), ptszName);
	sendDlgItemMessage(IDC_COURSE, LB_INSERTSTRING,
		n + 1, reinterpret_cast<LPARAM>(ptszName));
	sendDlgItemMessage(IDC_COURSE, LB_SETCURSEL, n + 1);
	
	return 0;
}

LRESULT qm::GoRoundDialog::onCourseSelChange()
{
	updateState();
	return 0;
}

void qm::GoRoundDialog::updateState()
{
	int n = sendDlgItemMessage(IDC_COURSE, LB_GETCURSEL);
	Window(getDlgItem(IDC_REMOVE)).enableWindow(n != LB_ERR);
	Window(getDlgItem(IDC_EDIT)).enableWindow(n != LB_ERR);
	Window(getDlgItem(IDC_UP)).enableWindow(n != LB_ERR && n != 0);
	Window(getDlgItem(IDC_DOWN)).enableWindow(n != LB_ERR &&
		n != sendDlgItemMessage(IDC_COURSE, LB_GETCOUNT) - 1);
}


/****************************************************************************
 *
 * GoRoundCourseDialog
 *
 */

qm::GoRoundCourseDialog::GoRoundCourseDialog(GoRoundCourse* pCourse,
											 Document* pDocument,
											 SyncFilterManager* pSyncFilterManager) :
	DefaultDialog(IDD_GOROUNDCOURSE),
	pCourse_(pCourse),
	pDocument_(pDocument),
	pSyncFilterManager_(pSyncFilterManager)
{
	const GoRoundCourse::EntryList& listEntry = pCourse_->getEntries();
	listEntry_.reserve(listEntry.size());
	for (GoRoundCourse::EntryList::const_iterator it = listEntry.begin(); it != listEntry.end(); ++it)
		listEntry_.push_back(new GoRoundEntry(**it));
}

qm::GoRoundCourseDialog::~GoRoundCourseDialog()
{
	std::for_each(listEntry_.begin(), listEntry_.end(), qs::deleter<GoRoundEntry>());
}

LRESULT qm::GoRoundCourseDialog::onCommand(WORD nCode,
										   WORD nId)
{
	BEGIN_COMMAND_HANDLER()
		HANDLE_COMMAND_ID(IDC_ADD, onAdd)
		HANDLE_COMMAND_ID(IDC_DIALUP, onDialup)
		HANDLE_COMMAND_ID(IDC_DOWN, onDown)
		HANDLE_COMMAND_ID(IDC_EDIT, onEdit)
		HANDLE_COMMAND_ID(IDC_REMOVE, onRemove)
		HANDLE_COMMAND_ID(IDC_UP, onUp)
		HANDLE_COMMAND_ID_CODE(IDC_ENTRY, LBN_SELCHANGE, onEntrySelChange)
		HANDLE_COMMAND_ID_CODE(IDC_NAME, EN_CHANGE, onNameChange)
	END_COMMAND_HANDLER()
	return DefaultDialog::onCommand(nCode, nId);
}

LRESULT qm::GoRoundCourseDialog::onInitDialog(HWND hwndFocus,
											  LPARAM lParam)
{
	init(false);
	
	setDlgItemText(IDC_NAME, pCourse_->getName());
	
	for (GoRoundCourse::EntryList::const_iterator it = listEntry_.begin(); it != listEntry_.end(); ++it) {
		GoRoundEntry* pEntry = *it;
		wstring_ptr wstrName(getEntryName(pEntry));
		W2T(wstrName.get(), ptszName);
		sendDlgItemMessage(IDC_ENTRY, LB_ADDSTRING,
			0, reinterpret_cast<LPARAM>(ptszName));
	}
	sendDlgItemMessage(IDC_ENTRY, LB_SETCURSEL);
	
	switch (pCourse_->getType()) {
	case GoRoundCourse::TYPE_SEQUENTIAL:
		sendDlgItemMessage(IDC_SEQUENTIAL, BM_SETCHECK, BST_CHECKED);
		break;
	case GoRoundCourse::TYPE_PARALLEL:
		sendDlgItemMessage(IDC_PARALLEL, BM_SETCHECK, BST_CHECKED);
		break;
	default:
		assert(false);
		break;
	}
	
	sendDlgItemMessage(IDC_CONFIRM, BM_SETCHECK,
		pCourse_->isFlag(GoRoundCourse::FLAG_CONFIRM) ? BST_CHECKED : BST_UNCHECKED);
	
	updateState();
	
	return TRUE;
}

LRESULT qm::GoRoundCourseDialog::onOk()
{
	wstring_ptr wstrName(getDlgItemText(IDC_NAME));
	if (!wstrName.get())
		return 0;
	pCourse_->setName(wstrName.get());
	
	pCourse_->setEntries(listEntry_);
	
	pCourse_->setType(sendDlgItemMessage(IDC_SEQUENTIAL, BM_GETCHECK) == BST_CHECKED ?
		GoRoundCourse::TYPE_SEQUENTIAL : GoRoundCourse::TYPE_PARALLEL);
	pCourse_->setFlags(sendDlgItemMessage(IDC_CONFIRM, BM_GETCHECK) == BST_CHECKED ?
		GoRoundCourse::FLAG_CONFIRM : 0);
	
	return DefaultDialog::onOk();
}

LRESULT qm::GoRoundCourseDialog::onAdd()
{
	std::auto_ptr<GoRoundEntry> pEntry(new GoRoundEntry());
	GoRoundEntryDialog dialog(pEntry.get(), pDocument_, pSyncFilterManager_);
	if (dialog.doModal(getHandle()) == IDOK) {
		listEntry_.push_back(pEntry.get());
		GoRoundEntry* p = pEntry.release();
		
		wstring_ptr wstrName(getEntryName(p));
		W2T(wstrName.get(), ptszName);
		sendDlgItemMessage(IDC_ENTRY, LB_ADDSTRING,
			0, reinterpret_cast<LPARAM>(ptszName));
	}
	return 0;
}

LRESULT qm::GoRoundCourseDialog::onRemove()
{
	int n = sendDlgItemMessage(IDC_ENTRY, LB_GETCURSEL);
	if (n == LB_ERR)
		return 0;
	
	delete listEntry_[n];
	listEntry_.erase(listEntry_.begin() + n);
	
	sendDlgItemMessage(IDC_ENTRY, LB_DELETESTRING, n);
	
	return 0;
}

LRESULT qm::GoRoundCourseDialog::onEdit()
{
	int n = sendDlgItemMessage(IDC_ENTRY, LB_GETCURSEL);
	if (n == LB_ERR)
		return 0;
	
	GoRoundEntry* pEntry = listEntry_[n];
	GoRoundEntryDialog dialog(pEntry, pDocument_, pSyncFilterManager_);
	if (dialog.doModal(getHandle()) == IDOK) {
		sendDlgItemMessage(IDC_ENTRY, LB_DELETESTRING, n);
		wstring_ptr wstrName(getEntryName(pEntry));
		W2T(wstrName.get(), ptszName);
		sendDlgItemMessage(IDC_ENTRY, LB_INSERTSTRING,
			n, reinterpret_cast<LPARAM>(ptszName));
		sendDlgItemMessage(IDC_ENTRY, LB_SETCURSEL, n);
	}
	
	return 0;
}

LRESULT qm::GoRoundCourseDialog::onUp()
{
	int n = sendDlgItemMessage(IDC_ENTRY, LB_GETCURSEL);
	if (n == LB_ERR || n == 0)
		return 0;
	
	GoRoundEntry* pEntry = listEntry_[n];
	std::swap(listEntry_[n], listEntry_[n - 1]);
	
	sendDlgItemMessage(IDC_ENTRY, LB_DELETESTRING, n);
	wstring_ptr wstrName(getEntryName(pEntry));
	W2T(wstrName.get(), ptszName);
	sendDlgItemMessage(IDC_ENTRY, LB_INSERTSTRING,
		n - 1, reinterpret_cast<LPARAM>(ptszName));
	sendDlgItemMessage(IDC_ENTRY, LB_SETCURSEL, n - 1);
	
	return 0;
}

LRESULT qm::GoRoundCourseDialog::onDown()
{
	int n = sendDlgItemMessage(IDC_ENTRY, LB_GETCURSEL);
	if (n == LB_ERR || n == sendDlgItemMessage(IDC_ENTRY, LB_GETCOUNT) - 1)
		return 0;
	
	GoRoundEntry* pEntry = listEntry_[n];
	std::swap(listEntry_[n], listEntry_[n + 1]);
	
	sendDlgItemMessage(IDC_ENTRY, LB_DELETESTRING, n);
	wstring_ptr wstrName(getEntryName(pEntry));
	W2T(wstrName.get(), ptszName);
	sendDlgItemMessage(IDC_ENTRY, LB_INSERTSTRING,
		n + 1, reinterpret_cast<LPARAM>(ptszName));
	sendDlgItemMessage(IDC_ENTRY, LB_SETCURSEL, n + 1);
	
	return 0;
}

LRESULT qm::GoRoundCourseDialog::onDialup()
{
	if (pCourse_->getDialup()) {
		GoRoundDialupDialog dialog(pCourse_->getDialup(), false);
		if (dialog.doModal(getHandle()) == IDOK) {
			if (dialog.isNoDialup()) {
				std::auto_ptr<GoRoundDialup> pDialup;
				pCourse_->setDialup(pDialup);
			}
		}
	}
	else {
		std::auto_ptr<GoRoundDialup> pDialup(new GoRoundDialup());
		GoRoundDialupDialog dialog(pDialup.get(), true);
		if (dialog.doModal(getHandle()) == IDOK) {
			if (!dialog.isNoDialup())
				pCourse_->setDialup(pDialup);
		}
	}
	
	return 0;
}

LRESULT qm::GoRoundCourseDialog::onNameChange()
{
	updateState();
	return 0;
}

LRESULT qm::GoRoundCourseDialog::onEntrySelChange()
{
	updateState();
	return 0;
}

void qm::GoRoundCourseDialog::updateState()
{
	int n = sendDlgItemMessage(IDC_ENTRY, LB_GETCURSEL);
	Window(getDlgItem(IDC_REMOVE)).enableWindow(n != LB_ERR);
	Window(getDlgItem(IDC_EDIT)).enableWindow(n != LB_ERR);
	Window(getDlgItem(IDC_UP)).enableWindow(n != LB_ERR && n != 0);
	Window(getDlgItem(IDC_DOWN)).enableWindow(n != LB_ERR &&
		n != sendDlgItemMessage(IDC_ENTRY, LB_GETCOUNT) - 1);
	
	wstring_ptr wstrName(getDlgItemText(IDC_NAME));
	Window(getDlgItem(IDOK)).enableWindow(*wstrName.get() != L'\0');
}

wstring_ptr qm::GoRoundCourseDialog::getEntryName(const GoRoundEntry* pEntry)
{
	StringBuffer<WSTRING> buf(pEntry->getAccount());
	if (pEntry->getSubAccount()) {
		buf.append(L'/');
		buf.append(pEntry->getSubAccount());
	}
	if (pEntry->getFolder()) {
		buf.append(L" [");
		buf.append(pEntry->getFolder());
		buf.append(L']');
	}
	return buf.getString();
}


/****************************************************************************
 *
 * GoRoundDialupDialog
 *
 */

qm::GoRoundDialupDialog::GoRoundDialupDialog(GoRoundDialup* pDialup,
											 bool bNoDialup) :
	DefaultDialog(IDD_GOROUNDDIALUP),
	pDialup_(pDialup),
	bNoDialup_(bNoDialup)
{
}

qm::GoRoundDialupDialog::~GoRoundDialupDialog()
{
}

bool qm::GoRoundDialupDialog::isNoDialup() const
{
	return bNoDialup_;
}

LRESULT qm::GoRoundDialupDialog::onCommand(WORD nCode,
										   WORD nId)
{
	BEGIN_COMMAND_HANDLER()
		HANDLE_COMMAND_ID_RANGE(IDC_NEVER, IDC_CONNECT, onTypeSelect)
	END_COMMAND_HANDLER()
	return DefaultDialog::onCommand(nCode, nId);
}

LRESULT qm::GoRoundDialupDialog::onInitDialog(HWND hwndFocus,
											  LPARAM lParam)
{
	init(false);
	
	if (bNoDialup_)
		sendDlgItemMessage(IDC_NEVER, BM_SETCHECK, BST_CHECKED);
	else if (pDialup_->isFlag(GoRoundDialup::FLAG_WHENEVERNOTCONNECTED))
		sendDlgItemMessage(IDC_WHENEVERNOTCONNECTED, BM_SETCHECK, BST_CHECKED);
	else
		sendDlgItemMessage(IDC_CONNECT, BM_SETCHECK, BST_CHECKED);
	
	if (pDialup_->getName())
		setDlgItemText(IDC_ENTRY, pDialup_->getName());
	if (pDialup_->getDialFrom())
		setDlgItemText(IDC_DIALFROM, pDialup_->getDialFrom());
	sendDlgItemMessage(IDC_SHOWDIALOG, BM_SETCHECK,
		pDialup_->isFlag(GoRoundDialup::FLAG_SHOWDIALOG) ? BST_CHECKED : BST_UNCHECKED);
	setDlgItemInt(IDC_WAITBEFOREDISCONNECT, pDialup_->getDisconnectWait());
	
	RasConnection::EntryList listEntry;
	StringListFree<RasConnection::EntryList> free(listEntry);
	RasConnection::getEntries(&listEntry);
	for (RasConnection::EntryList::const_iterator it = listEntry.begin(); it != listEntry.end(); ++it) {
		W2T(*it, ptszEntry);
		sendDlgItemMessage(IDC_ENTRY, CB_ADDSTRING,
			0, reinterpret_cast<LPARAM>(ptszEntry));
	}
	
	updateState();
	
	return TRUE;
}

LRESULT qm::GoRoundDialupDialog::onOk()
{
	bNoDialup_ = sendDlgItemMessage(IDC_NEVER, BM_GETCHECK) == BST_CHECKED;
	if (!bNoDialup_) {
		wstring_ptr wstrEntry(getDlgItemText(IDC_ENTRY));
		pDialup_->setName(*wstrEntry.get() ? wstrEntry.get() : 0);
		
		wstring_ptr wstrDialFrom(getDlgItemText(IDC_DIALFROM));
		pDialup_->setDialFrom(*wstrDialFrom.get() ? wstrDialFrom.get() : 0);
		
		unsigned int nFlags = 0;
		if (sendDlgItemMessage(IDC_WHENEVERNOTCONNECTED, BM_GETCHECK) == BST_CHECKED)
			nFlags |= GoRoundDialup::FLAG_WHENEVERNOTCONNECTED;
		if (sendDlgItemMessage(IDC_SHOWDIALOG, BM_GETCHECK) == BST_CHECKED)
			nFlags |= GoRoundDialup::FLAG_SHOWDIALOG;
		pDialup_->setFlags(nFlags);
		
		pDialup_->setDisconnectWait(getDlgItemInt(IDC_WAITBEFOREDISCONNECT));
	}
	return DefaultDialog::onOk();
}

LRESULT qm::GoRoundDialupDialog::onTypeSelect(UINT nId)
{
	updateState();
	return 0;
}

void qm::GoRoundDialupDialog::updateState()
{
	bool bEnable = sendDlgItemMessage(IDC_NEVER, BM_GETCHECK) != BST_CHECKED;
	
	UINT nIds[] = {
		IDC_ENTRY,
		IDC_SHOWDIALOG,
		IDC_DIALFROM,
		IDC_WAITBEFOREDISCONNECT
	};
	for (int n = 0; n < countof(nIds); ++n)
		Window(getDlgItem(nIds[n])).enableWindow(bEnable);
}


/****************************************************************************
 *
 * GoRoundEntryDialog
 *
 */

qm::GoRoundEntryDialog::GoRoundEntryDialog(GoRoundEntry* pEntry,
										   Document* pDocument,
										   SyncFilterManager* pSyncFilterManager) :
	DefaultDialog(IDD_GOROUNDENTRY),
	pEntry_(pEntry),
	pDocument_(pDocument),
	pSyncFilterManager_(pSyncFilterManager)
{
}

qm::GoRoundEntryDialog::~GoRoundEntryDialog()
{
}

LRESULT qm::GoRoundEntryDialog::onCommand(WORD nCode,
										  WORD nId)
{
	BEGIN_COMMAND_HANDLER()
		HANDLE_COMMAND_ID_CODE(IDC_ACCOUNT, CBN_EDITCHANGE, onAccountEditChange)
		HANDLE_COMMAND_ID_CODE(IDC_ACCOUNT, CBN_SELCHANGE, onAccountSelChange)
		HANDLE_COMMAND_ID_CODE(IDC_SELECTFOLDER, BN_CLICKED, onSelectFolderClicked)
	END_COMMAND_HANDLER()
	return DefaultDialog::onCommand(nCode, nId);
}

LRESULT qm::GoRoundEntryDialog::onInitDialog(HWND hwndFocus,
											 LPARAM lParam)
{
	HINSTANCE hInst = Application::getApplication().getResourceHandle();
	
	init(false);
	
	const Document::AccountList& listAccount = pDocument_->getAccounts();
	for (Document::AccountList::const_iterator it = listAccount.begin(); it != listAccount.end(); ++it) {
		Account* pAccount = *it;
		W2T(pAccount->getName(), ptszName);
		sendDlgItemMessage(IDC_ACCOUNT, CB_ADDSTRING,
			0, reinterpret_cast<LPARAM>(ptszName));
	}
	W2T(pEntry_->getAccount(), ptszAccount);
	setDlgItemText(IDC_ACCOUNT, pEntry_->getAccount());
	
	const WCHAR* pwszSubAccount = pEntry_->getSubAccount();
	if (pwszSubAccount) {
		setDlgItemText(IDC_SUBACCOUNT, pwszSubAccount);
	}
	else {
		wstring_ptr wstrUnspecified(loadString(hInst, IDS_UNSPECIFIED));
		setDlgItemText(IDC_SUBACCOUNT, wstrUnspecified.get());
	}
	
	const WCHAR* pwszFolder = pEntry_->getFolder();
	if (pwszFolder) {
		setDlgItemText(IDC_FOLDER, pwszFolder);
	}
	else {
		wstring_ptr wstrAll(loadString(hInst, IDS_ALLFOLDER));
		setDlgItemText(IDC_FOLDER, wstrAll.get());
	}
	
	if (pEntry_->isFlag(GoRoundEntry::FLAG_SELECTFOLDER))
		sendDlgItemMessage(IDC_SELECTFOLDER, BM_SETCHECK, BST_CHECKED);
	
	const WCHAR* pwszFilter = pEntry_->getFilter();
	if (pwszFilter)
		setDlgItemText(IDC_SYNCFILTER, pwszFilter);
	
	if (pEntry_->isFlag(GoRoundEntry::FLAG_SEND) &&
		pEntry_->isFlag(GoRoundEntry::FLAG_RECEIVE))
		sendDlgItemMessage(IDC_SENDRECEIVE, BM_SETCHECK, BST_CHECKED);
	else if (pEntry_->isFlag(GoRoundEntry::FLAG_SEND))
		sendDlgItemMessage(IDC_SEND, BM_SETCHECK, BST_CHECKED);
	else if (pEntry_->isFlag(GoRoundEntry::FLAG_RECEIVE))
		sendDlgItemMessage(IDC_RECEIVE, BM_SETCHECK, BST_CHECKED);
	
	if (pEntry_->getConnectReceiveBeforeSend() == GoRoundEntry::CRBS_TRUE)
		sendDlgItemMessage(IDC_CONNECTRECEIVEBEFORESEND, BM_SETCHECK, BST_CHECKED);
	
	updateState();
	
	return TRUE;
}

LRESULT qm::GoRoundEntryDialog::onOk()
{
	HINSTANCE hInst = Application::getApplication().getResourceHandle();
	
	wstring_ptr wstrAccount(getDlgItemText(IDC_ACCOUNT));
	if (!wstrAccount.get())
		return 0;
	
	wstring_ptr wstrSubAccount(getDlgItemText(IDC_SUBACCOUNT));
	const WCHAR* pwszSubAccount = wstrSubAccount.get();
	wstring_ptr wstrUnspecified(loadString(hInst, IDS_UNSPECIFIED));
	wstring_ptr wstrDefault(loadString(hInst, IDS_DEFAULTSUBACCOUNT));
	if (wcscmp(pwszSubAccount, wstrUnspecified.get()) == 0)
		pwszSubAccount = 0;
	else if (wcscmp(pwszSubAccount, wstrDefault.get()) == 0)
		pwszSubAccount = L"";
	
	wstring_ptr wstrFolder(getDlgItemText(IDC_FOLDER));
	const WCHAR* pwszFolder = wstrFolder.get();
	wstring_ptr wstrAll(loadString(hInst, IDS_ALLFOLDER));
	if (wcscmp(pwszFolder, wstrAll.get()) == 0)
		pwszFolder = 0;
	std::auto_ptr<RegexPattern> pFolderPattern;
	if (pwszFolder) {
		pFolderPattern = RegexCompiler().compile(pwszFolder);
		if (!pFolderPattern.get())
			return 0;
	}
	
	unsigned int nFlags = 0;
	if (sendDlgItemMessage(IDC_SELECTFOLDER, BM_GETCHECK) == BST_CHECKED)
		nFlags |= GoRoundEntry::FLAG_SELECTFOLDER;
	if (sendDlgItemMessage(IDC_SENDRECEIVE, BM_GETCHECK) == BST_CHECKED)
		nFlags |= GoRoundEntry::FLAG_SEND | GoRoundEntry::FLAG_RECEIVE;
	else if (sendDlgItemMessage(IDC_RECEIVE, BM_GETCHECK) == BST_CHECKED)
		nFlags |= GoRoundEntry::FLAG_RECEIVE;
	else if (sendDlgItemMessage(IDC_SEND, BM_GETCHECK) == BST_CHECKED)
		nFlags |= GoRoundEntry::FLAG_SEND;
	
	wstring_ptr wstrFilter(getDlgItemText(IDC_SYNCFILTER));
	const WCHAR* pwszFilter = wstrFilter.get();
	if (!*pwszFilter)
		pwszFilter = 0;
	
	GoRoundEntry::ConnectReceiveBeforeSend crbs =
		sendDlgItemMessage(IDC_CONNECTRECEIVEBEFORESEND, BM_GETCHECK) == BST_CHECKED ?
		GoRoundEntry::CRBS_TRUE : GoRoundEntry::CRBS_NONE;
	
	pEntry_->setAccount(wstrAccount.get());
	pEntry_->setSubAccount(pwszSubAccount);
	pEntry_->setFolder(pwszFolder, pFolderPattern);
	pEntry_->setFlags(nFlags);
	pEntry_->setFilter(pwszFilter);
	pEntry_->setConnectReceiveBeforeSend(crbs);
	
	return DefaultDialog::onOk();
}

LRESULT qm::GoRoundEntryDialog::onAccountEditChange()
{
	updateState();
	return 0;
}

LRESULT qm::GoRoundEntryDialog::onAccountSelChange()
{
	postMessage(WM_COMMAND, MAKEWPARAM(IDC_ACCOUNT, CBN_EDITCHANGE));
	return 0;
}

LRESULT qm::GoRoundEntryDialog::onSelectFolderClicked()
{
	updateState();
	return 0;
}

void qm::GoRoundEntryDialog::updateState()
{
	Account* pAccount = 0;
	
	wstring_ptr wstrAccount(getDlgItemText(IDC_ACCOUNT));
	if (wstrAccount.get())
		pAccount = pDocument_->getAccount(wstrAccount.get());
	
	updateSubAccount(pAccount);
	updateFolder(pAccount);
	updateFilter(pAccount);
	
	Window(getDlgItem(IDC_FOLDER)).enableWindow(
		sendDlgItemMessage(IDC_SELECTFOLDER, BM_GETCHECK) != BST_CHECKED);
	Window(getDlgItem(IDOK)).enableWindow(*wstrAccount.get() != L'\0');
}

void qm::GoRoundEntryDialog::updateSubAccount(Account* pAccount)
{
	HINSTANCE hInst = Application::getApplication().getResourceHandle();
	
	wstring_ptr wstrSubAccount(getDlgItemText(IDC_SUBACCOUNT));
	
	sendDlgItemMessage(IDC_SUBACCOUNT, CB_RESETCONTENT);
	
	wstring_ptr wstrUnspecified(loadString(hInst, IDS_UNSPECIFIED));
	W2T(wstrUnspecified.get(), ptszUnspecified);
	sendDlgItemMessage(IDC_SUBACCOUNT, CB_ADDSTRING,
		0, reinterpret_cast<LPARAM>(ptszUnspecified));
	
	if (pAccount) {
		Account::SubAccountList l(pAccount->getSubAccounts());
		std::sort(l.begin(), l.end(),
			binary_compose_f_gx_hy(
				string_equal<WCHAR>(),
				std::mem_fun(&SubAccount::getName),
				std::mem_fun(&SubAccount::getName)));
		for (Account::SubAccountList::const_iterator it = l.begin(); it != l.end(); ++it) {
			SubAccount* pSubAccount = *it;
			if (*pSubAccount->getName()) {
				W2T(pSubAccount->getName(), ptszName);
				sendDlgItemMessage(IDC_SUBACCOUNT, CB_ADDSTRING,
					0, reinterpret_cast<LPARAM>(ptszName));
			}
			else {
				wstring_ptr wstrDefault(loadString(hInst, IDS_DEFAULTSUBACCOUNT));
				W2T(wstrDefault.get(), ptszDefault);
				sendDlgItemMessage(IDC_SUBACCOUNT, CB_ADDSTRING,
					0, reinterpret_cast<LPARAM>(ptszDefault));
			}
		}
	}
	
	setDlgItemText(IDC_SUBACCOUNT, wstrSubAccount.get());
}

void qm::GoRoundEntryDialog::updateFolder(Account* pAccount)
{
	HINSTANCE hInst = Application::getApplication().getResourceHandle();
	
	wstring_ptr wstrFolder(getDlgItemText(IDC_FOLDER));
	
	sendDlgItemMessage(IDC_FOLDER, CB_RESETCONTENT);
	
	wstring_ptr wstrAll(loadString(hInst, IDS_ALLFOLDER));
	W2T(wstrAll.get(), ptszAll);
	sendDlgItemMessage(IDC_FOLDER, CB_ADDSTRING,
		0, reinterpret_cast<LPARAM>(ptszAll));
	
	if (pAccount) {
		Account::FolderList l(pAccount->getFolders());
		std::sort(l.begin(), l.end(), FolderLess());
		for (Account::FolderList::const_iterator it = l.begin(); it != l.end(); ++it) {
			Folder* pFolder = *it;
			
			wstring_ptr wstrName(pFolder->getFullName());
			W2T(wstrName.get(), ptszName);
			sendDlgItemMessage(IDC_FOLDER, CB_ADDSTRING,
				0, reinterpret_cast<LPARAM>(ptszName));
		}
	}
	
	setDlgItemText(IDC_FOLDER, wstrFolder.get());
}

void qm::GoRoundEntryDialog::updateFilter(Account* pAccount)
{
	HINSTANCE hInst = Application::getApplication().getResourceHandle();
	
	wstring_ptr wstrFilter(getDlgItemText(IDC_SYNCFILTER));
	
	sendDlgItemMessage(IDC_SYNCFILTER, CB_RESETCONTENT);
	
	if (pAccount) {
		SyncFilterManager::FilterSetList l;
		pSyncFilterManager_->getFilterSets(pAccount, &l);
		for (SyncFilterManager::FilterSetList::const_iterator it = l.begin(); it != l.end(); ++it) {
			SyncFilterSet* pSet = *it;
			W2T(pSet->getName(), ptszName);
			sendDlgItemMessage(IDC_SYNCFILTER, CB_ADDSTRING,
				0, reinterpret_cast<LPARAM>(ptszName));
		}
	}
	
	setDlgItemText(IDC_SYNCFILTER, wstrFilter.get());
}


/****************************************************************************
 *
 * ImportDialog
 *
 */

qm::ImportDialog::ImportDialog(Profile* pProfile) :
	DefaultDialog(IDD_IMPORT),
	pProfile_(pProfile),
	bMultiple_(false),
	nFlags_(0)
{
}

qm::ImportDialog::~ImportDialog()
{
}

const WCHAR* qm::ImportDialog::getPath() const
{
	return wstrPath_.get();
}

bool qm::ImportDialog::isMultiple() const
{
	return bMultiple_;
}

const WCHAR* qm::ImportDialog::getEncoding() const
{
	return *wstrEncoding_.get() ? wstrEncoding_.get() : 0;
}

unsigned int qm::ImportDialog::getFlags() const
{
	return nFlags_;
}

LRESULT qm::ImportDialog::onCommand(WORD nCode,
									WORD nId)
{
	BEGIN_COMMAND_HANDLER()
		HANDLE_COMMAND_ID(IDC_BROWSE, onBrowse)
		HANDLE_COMMAND_ID_CODE(IDC_PATH, EN_CHANGE, onPathChange)
		HANDLE_COMMAND_ID_CODE(IDC_ENCODING, CBN_EDITCHANGE, onEncodingEditChange)
		HANDLE_COMMAND_ID_CODE(IDC_ENCODING, CBN_SELCHANGE, onEncodingSelChange)
	END_COMMAND_HANDLER()
	return DefaultDialog::onCommand(nCode, nId);
}

LRESULT qm::ImportDialog::onInitDialog(HWND hwndFocus,
									   LPARAM lParam)
{
	init(false);
	
	sendDlgItemMessage(IDC_ENCODING, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(_T("")));
	UIUtil::EncodingList listEncoding;
	StringListFree<UIUtil::EncodingList> freeEncoding(listEncoding);
	UIUtil::loadEncodings(pProfile_, &listEncoding);
	for (UIUtil::EncodingList::const_iterator it = listEncoding.begin(); it != listEncoding.end(); ++it) {
		W2T(*it, ptszEncoding);
		sendDlgItemMessage(IDC_ENCODING, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(ptszEncoding));
	}
	sendDlgItemMessage(IDC_ENCODING, CB_SETCURSEL, 0);
	
	sendDlgItemMessage(IDC_NORMAL, BM_SETCHECK, BST_CHECKED);
	
	updateState();
	
	return TRUE;
}

LRESULT qm::ImportDialog::onOk()
{
	wstrPath_ = getDlgItemText(IDC_PATH);
	
	bMultiple_ = sendDlgItemMessage(IDC_MULTIMESSAGES, BM_GETCHECK) == BST_CHECKED;
	
	wstrEncoding_ = getDlgItemText(IDC_ENCODING);
	
	nFlags_ = 0;
	struct {
		UINT nId_;
		Account::ImportFlag flag_;
	} flags[] = {
		{ IDC_NORMAL,				Account::IMPORTFLAG_NORMALFLAGS		},
		{ IDC_QMAIL20COMPATIBLE,	Account::IMPORTFLAG_QMAIL20FLAGS	},
		{ IDC_IGNORE,				Account::IMPORTFLAG_IGNOREFLAGS		}
	};
	for (int n = 0; n < countof(flags); ++n) {
		if (sendDlgItemMessage(flags[n].nId_, BM_GETCHECK) == BST_CHECKED)
			nFlags_ |= flags[n].flag_;
	}
	
	return DefaultDialog::onOk();
}

LRESULT qm::ImportDialog::onBrowse()
{
	HINSTANCE hInst = Application::getApplication().getResourceHandle();
	wstring_ptr wstrFilter(loadString(hInst, IDS_FILTER_IMPORT));
	
	FileDialog dialog(true, wstrFilter.get(), 0, 0, 0,
		OFN_EXPLORER | OFN_HIDEREADONLY | OFN_LONGNAMES | OFN_ALLOWMULTISELECT);
	
	if (dialog.doModal(getHandle()) == IDOK) {
		const WCHAR* pwszPath = dialog.getPath();
		if (*(pwszPath + wcslen(pwszPath) + 1)) {
			StringBuffer<WSTRING> buf;
			const WCHAR* p = pwszPath;
			while (true) {
				buf.append(p);
				p += wcslen(p) + 1;
				if (!*p)
					break;
				buf.append(L';');
			}
			setDlgItemText(IDC_PATH, buf.getCharArray());
		}
		else {
			setDlgItemText(IDC_PATH, pwszPath);
		}
		updateState();
	}
	
	return 0;
}

LRESULT qm::ImportDialog::onPathChange()
{
	updateState();
	return 0;
}

LRESULT qm::ImportDialog::onEncodingEditChange()
{
	updateState();
	return 0;
}

LRESULT qm::ImportDialog::onEncodingSelChange()
{
	postMessage(WM_COMMAND, MAKEWPARAM(IDC_ENCODING, CBN_EDITCHANGE));
	return 0;
}

void qm::ImportDialog::updateState()
{
	Window(getDlgItem(IDC_MULTIMESSAGES)).enableWindow(
		sendDlgItemMessage(IDC_ENCODING, WM_GETTEXTLENGTH) == 0);
	Window(getDlgItem(IDOK)).enableWindow(
		sendDlgItemMessage(IDC_PATH, WM_GETTEXTLENGTH) != 0);
}


/****************************************************************************
 *
 * InputBoxDialog
 *
 */

qm::InputBoxDialog::InputBoxDialog(bool bMultiLine,
								   const WCHAR* pwszMessage,
								   const WCHAR* pwszValue) :
	DefaultDialog(bMultiLine ? IDD_MULTIINPUTBOX : IDD_SINGLEINPUTBOX),
	bMultiLine_(bMultiLine)
{
	if (pwszMessage)
		wstrMessage_ = allocWString(pwszMessage);
	if (pwszValue)
		wstrValue_ = allocWString(pwszValue);
}

qm::InputBoxDialog::~InputBoxDialog()
{
}

const WCHAR* qm::InputBoxDialog::getMessage() const
{
	return wstrMessage_.get();
}

const WCHAR* qm::InputBoxDialog::getValue() const
{
	return wstrValue_.get();
}

LRESULT qm::InputBoxDialog::onInitDialog(HWND hwndFocus,
										 LPARAM lParam)
{
	init(false);
	
	if (wstrMessage_.get())
		setDlgItemText(IDC_MESSAGE, wstrMessage_.get());
	
	if (wstrValue_.get()) {
		if (bMultiLine_ && wcschr(wstrValue_.get(), L'\n')) {
			StringBuffer<WSTRING> buf;
			
			const WCHAR* p = wstrValue_.get();
			while (*p) {
				if (*p == L'\n')
					buf.append(L'\r');
				buf.append(*p);
				++p;
			}
			
			setDlgItemText(IDC_VALUE, buf.getCharArray());
		}
		else {
			setDlgItemText(IDC_VALUE, wstrValue_.get());
		}
	}
	
	return TRUE;
}

LRESULT qm::InputBoxDialog::onOk()
{
	wstrValue_ = getDlgItemText(IDC_VALUE);
	if (bMultiLine_ && wstrValue_.get()) {
		WCHAR* pSrc = wstrValue_.get();
		WCHAR* pDst = wstrValue_.get();
		while (*pSrc) {
			if (*pSrc != L'\r')
				*pDst++ = *pSrc;
			++pSrc;
		}
		*pDst = L'\0';
	}
	return DefaultDialog::onOk();
}


/****************************************************************************
 *
 * InsertTextDialog
 *
 */

qm::InsertTextDialog::InsertTextDialog() :
	DefaultDialog(IDD_INSERTTEXT),
	pText_(0)
{
	pManager_.reset(new FixedFormTextManager());
}

qm::InsertTextDialog::~InsertTextDialog()
{
}

const FixedFormText* qm::InsertTextDialog::getText() const
{
	return pText_;
}

LRESULT qm::InsertTextDialog::onInitDialog(HWND hwndFocus,
										   LPARAM lParam)
{
	init(false);
	
	const FixedFormTextManager::TextList& l = pManager_->getTextList();
	for (FixedFormTextManager::TextList::const_iterator it = l.begin(); it != l.end(); ++it) {
		tstring_ptr tstrName(wcs2tcs((*it)->getName()));
		sendDlgItemMessage(IDC_LIST, LB_ADDSTRING, 0,
			reinterpret_cast<LPARAM>(tstrName.get()));
	}
	
	sendDlgItemMessage(IDC_LIST, LB_SETCURSEL);
	
	return TRUE;
}

LRESULT qm::InsertTextDialog::onOk()
{
	const FixedFormTextManager::TextList& l = pManager_->getTextList();
	unsigned int nItem = sendDlgItemMessage(IDC_LIST, LB_GETCURSEL);
	assert(nItem < l.size());
	pText_ = l[nItem];
	return DefaultDialog::onOk();
}


/****************************************************************************
 *
 * MailFolderDialog
 *
 */

qm::MailFolderDialog::MailFolderDialog(HINSTANCE hInstResource) :
	DefaultDialog(hInstResource, IDD_MAILFOLDER)
{
}

qm::MailFolderDialog::~MailFolderDialog()
{
}

const WCHAR* qm::MailFolderDialog::getMailFolder() const
{
	return wstrMailFolder_.get();
}

LRESULT qm::MailFolderDialog::onCommand(WORD nCode,
										WORD nId)
{
	BEGIN_COMMAND_HANDLER()
		HANDLE_COMMAND_ID(IDC_BROWSE, onBrowse)
		HANDLE_COMMAND_ID_CODE(IDC_MAILFOLDER, EN_CHANGE, onMailFolderChange)
	END_COMMAND_HANDLER()
	return DefaultDialog::onCommand(nCode, nId);
}

LRESULT qm::MailFolderDialog::onInitDialog(HWND hwndFocus,
										   LPARAM lParam)
{
	init(false);
	updateState();
	return TRUE;
}

LRESULT qm::MailFolderDialog::onOk()
{
	wstrMailFolder_ = getDlgItemText(IDC_MAILFOLDER);
	return DefaultDialog::onOk();
}

LRESULT qm::MailFolderDialog::onMailFolderChange()
{
	updateState();
	return 0;
}

LRESULT qm::MailFolderDialog::onBrowse()
{
	wstring_ptr wstrPath(qs::UIUtil::browseFolder(getHandle(), 0, 0));
	if (wstrPath.get())
		setDlgItemText(IDC_MAILFOLDER, wstrPath.get());
	
	return 0;
}

void qm::MailFolderDialog::updateState()
{
	Window(getDlgItem(IDOK)).enableWindow(
		Window(getDlgItem(IDC_MAILFOLDER)).getWindowTextLength() != 0);
}


/****************************************************************************
 *
 * MoveMessageDialog
 *
 */

qm::MoveMessageDialog::MoveMessageDialog(Document* pDocument,
										 Profile* pProfile) :
	DefaultDialog(IDD_MOVEMESSAGE),
	pDocument_(pDocument),
	pProfile_(pProfile),
	pFolder_(0),
	bCopy_(false),
	bShowHidden_(false)
{
	bShowHidden_ = pProfile->getInt(L"MoveMessageDialog", L"ShowHidden", 0) != 0;
}

qm::MoveMessageDialog::~MoveMessageDialog()
{
}

NormalFolder* qm::MoveMessageDialog::getFolder() const
{
	return pFolder_;
}

bool qm::MoveMessageDialog::isCopy() const
{
	return bCopy_;
}

LRESULT qm::MoveMessageDialog::onCommand(WORD nCode,
										 WORD nId)
{
	BEGIN_COMMAND_HANDLER()
		HANDLE_COMMAND_ID(IDC_SHOWHIDDEN, onShowHidden)
	END_COMMAND_HANDLER()
	return DefaultDialog::onCommand(nCode, nId);
}

LRESULT qm::MoveMessageDialog::onNotify(NMHDR* pnmhdr,
										bool* pbHandled)
{
	BEGIN_NOTIFY_HANDLER()
		HANDLE_NOTIFY(TVN_SELCHANGED, IDC_FOLDER, onFolderSelChanged);
	END_NOTIFY_HANDLER()
	return 1;
}

LRESULT qm::MoveMessageDialog::onInitDialog(HWND hwndFocus,
											LPARAM lParam)
{
	init(false);
	
	HIMAGELIST hImageList = ImageList_LoadImage(
		Application::getApplication().getResourceHandle(),
		MAKEINTRESOURCE(IDB_FOLDER), 16, 0, CLR_DEFAULT, IMAGE_BITMAP, 0);
	TreeView_SetImageList(getDlgItem(IDC_FOLDER), hImageList, TVSIL_NORMAL);
	
	if (bShowHidden_)
		sendDlgItemMessage(IDC_SHOWHIDDEN, BM_SETCHECK, BST_CHECKED);
	
	update();
	updateState();
	addNotifyHandler(this);
	
	return TRUE;
}

LRESULT qm::MoveMessageDialog::onDestroy()
{
	pProfile_->setInt(L"MoveMessageDialog", L"ShowHidden", bShowHidden_);
	
	HIMAGELIST hImageList = TreeView_SetImageList(getHandle(), 0, TVSIL_NORMAL);
	ImageList_Destroy(hImageList);
	
	removeNotifyHandler(this);
	
	return DefaultDialog::onDestroy();
}

LRESULT qm::MoveMessageDialog::onOk()
{
	HWND hwnd = getDlgItem(IDC_FOLDER);
	HTREEITEM hItem = TreeView_GetSelection(hwnd);
	if (!TreeView_GetParent(hwnd, hItem))
		return 0;
	
	TVITEM item = {
		TVIF_HANDLE | TVIF_PARAM,
		hItem
	};
	TreeView_GetItem(hwnd, &item);
	
	Folder* pFolder = reinterpret_cast<Folder*>(item.lParam);
	if (pFolder->getType() != Folder::TYPE_NORMAL)
		return 0;
	pFolder_ = static_cast<NormalFolder*>(pFolder);
	
	bCopy_ = sendDlgItemMessage(IDC_COPY, BM_GETCHECK) == BST_CHECKED;
	
	return DefaultDialog::onOk();
}

LRESULT qm::MoveMessageDialog::onShowHidden()
{
	bool bShowHidden = sendDlgItemMessage(IDC_SHOWHIDDEN, BM_GETCHECK) == BST_CHECKED;
	if (bShowHidden != bShowHidden_) {
		bShowHidden_ = bShowHidden;
		update();
	}
	return 0;
}

LRESULT qm::MoveMessageDialog::onFolderSelChanged(NMHDR* pnmhdr, bool* pbHandled)
{
	updateState();
	*pbHandled = true;
	return 0;
}

bool qm::MoveMessageDialog::update()
{
	HWND hwnd = getDlgItem(IDC_FOLDER);
	DisableRedraw disable(hwnd);
	
	TreeView_DeleteAllItems(hwnd);
	
	const Document::AccountList& listAccount = pDocument_->getAccounts();
	for (Document::AccountList::const_iterator it = listAccount.begin(); it != listAccount.end(); ++it) {
		if (!insertAccount(*it))
			return false;
	}
	
	return true;
}

bool qm::MoveMessageDialog::insertAccount(Account* pAccount)
{
	assert(pAccount);
	
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
			0,
			0,
			0,
			reinterpret_cast<LPARAM>(pAccount)
		}
	};
	HTREEITEM hItemAccount = TreeView_InsertItem(
		getDlgItem(IDC_FOLDER), &tvisAccount);
	if (!hItemAccount)
		return false;
	
	if (!insertFolders(hItemAccount, pAccount))
		return false;
	
	return true;
}

bool qm::MoveMessageDialog::insertFolders(HTREEITEM hItem,
										  Account* pAccount)
{
	assert(hItem);
	assert(pAccount);
	
	const Account::FolderList& l = pAccount->getFolders();
	Account::FolderList listFolder;
	listFolder.reserve(l.size());
	if (bShowHidden_)
		std::copy(l.begin(), l.end(), std::back_inserter(listFolder));
	else
		std::remove_copy_if(l.begin(), l.end(),
			std::back_inserter(listFolder), std::mem_fun(&Folder::isHidden));
	std::sort(listFolder.begin(), listFolder.end(), FolderLess());
	
	typedef std::vector<std::pair<Folder*, HTREEITEM> > Stack;
	Stack stack;
	stack.push_back(Stack::value_type(0, hItem));
	
	for (Account::FolderList::const_iterator it = listFolder.begin(); it != listFolder.end(); ++it) {
		Folder* pFolder = *it;
		
		W2T(pFolder->getName(), ptszName);
		
		TVINSERTSTRUCT tvisFolder = {
			hItem,
			TVI_LAST,
			{
				TVIF_TEXT | TVIF_PARAM | TVIF_IMAGE | TVIF_SELECTEDIMAGE,
				0,
				0,
				0,
				const_cast<LPTSTR>(ptszName),
				0,
				UIUtil::getFolderImage(pFolder, false),
				UIUtil::getFolderImage(pFolder, true),
				0,
				reinterpret_cast<LPARAM>(pFolder)
			}
		};
		Folder* pParentFolder = pFolder->getParentFolder();
		while (stack.back().first != pParentFolder)
			stack.pop_back();
		assert(!stack.empty());
		tvisFolder.hParent = stack.back().second;
		
		HTREEITEM hItemFolder = TreeView_InsertItem(
			getDlgItem(IDC_FOLDER), &tvisFolder);
		if (!hItemFolder)
			return false;
		
		stack.push_back(Stack::value_type(pFolder, hItemFolder));
	}
	
	return true;
}

void qm::MoveMessageDialog::updateState()
{
	bool bEnable = false;
	
	HWND hwnd = getDlgItem(IDC_FOLDER);
	HTREEITEM hItem = TreeView_GetSelection(hwnd);
	if (TreeView_GetParent(hwnd, hItem)) {
		TVITEM item = {
			TVIF_HANDLE | TVIF_PARAM,
			hItem
		};
		TreeView_GetItem(hwnd, &item);
		
		Folder* pFolder = reinterpret_cast<Folder*>(item.lParam);
		if (pFolder->getType() == Folder::TYPE_NORMAL)
			bEnable = true;
	}
	
	Window(getDlgItem(IDOK)).enableWindow(bEnable);
}


/****************************************************************************
 *
 * ParameterDialog
 *
 */

qm::ParameterDialog::ParameterDialog(const WCHAR* pwszName,
									 const WCHAR* pwszValue) :
	DefaultDialog(IDD_PARAMETER)
{
	wstrName_ = allocWString(pwszName);
	wstrValue_ = allocWString(pwszValue);
}

qm::ParameterDialog::~ParameterDialog()
{
}

const WCHAR* qm::ParameterDialog::getValue() const
{
	return wstrValue_.get();
}

LRESULT qm::ParameterDialog::onInitDialog(HWND hwndFocus,
										  LPARAM lParam)
{
	init(false);
	
	setDlgItemText(IDC_NAME, wstrName_.get());
	setDlgItemText(IDC_VALUE, wstrValue_.get());
	
	return TRUE;
}

LRESULT qm::ParameterDialog::onOk()
{
	wstrValue_ = getDlgItemText(IDC_VALUE);
	return DefaultDialog::onOk();
}


/****************************************************************************
 *
 * ProgressDialog
 *
 */

qm::ProgressDialog::ProgressDialog(UINT nTitleId) :
	DefaultDialog(IDD_PROGRESS),
	nTitleId_(nTitleId),
	bCanceled_(false)
{
}

qm::ProgressDialog::~ProgressDialog()
{
}

bool qm::ProgressDialog::init(HWND hwnd)
{
	if (!create(hwnd))
		return false;
	showWindow(SW_SHOW);
	getModalHandler()->preModalDialog(0);
	
	return true;
}

void qm::ProgressDialog::term()
{
	getModalHandler()->postModalDialog(0);
	destroyWindow();
}

bool qm::ProgressDialog::isCanceled() const
{
	MSG msg;
	while (::PeekMessage(&msg, getHandle(), 0, 0, PM_REMOVE)) {
		::TranslateMessage(&msg);
		::DispatchMessage(&msg);
	}
	return bCanceled_;
}

void qm::ProgressDialog::setTitle(UINT nId)
{
	HINSTANCE hInst = Application::getApplication().getResourceHandle();
	wstring_ptr wstrMessage(loadString(hInst, nId));
	setWindowText(wstrMessage.get());
}

void qm::ProgressDialog::setMessage(UINT nId)
{
	HINSTANCE hInst = Application::getApplication().getResourceHandle();
	wstring_ptr wstrMessage(loadString(hInst, nId));
	setDlgItemText(IDC_MESSAGE, wstrMessage.get());
}

void qm::ProgressDialog::setMessage(const WCHAR* pwszMessage)
{
	setDlgItemText(IDC_MESSAGE, pwszMessage);
}

void qm::ProgressDialog::setRange(unsigned int nMin,
								  unsigned int nMax)
{
	sendDlgItemMessage(IDC_PROGRESS, PBM_SETRANGE32, nMin, nMax);
}

void qm::ProgressDialog::setPos(unsigned int n)
{
	sendDlgItemMessage(IDC_PROGRESS, PBM_SETPOS, n);
}

void qm::ProgressDialog::setStep(unsigned int n)
{
	sendDlgItemMessage(IDC_PROGRESS, PBM_SETSTEP, n);
}

void qm::ProgressDialog::step()
{
	sendDlgItemMessage(IDC_PROGRESS, PBM_STEPIT);
}

LRESULT qm::ProgressDialog::onDestroy()
{
	return DefaultDialog::onDestroy();
}

LRESULT qm::ProgressDialog::onInitDialog(HWND hwndFocus,
										 LPARAM lParam)
{
	DefaultDialog::init(false);
	setTitle(nTitleId_);
	return DefaultDialog::onInitDialog(hwndFocus, lParam);
}

LRESULT qm::ProgressDialog::onCancel()
{
	bCanceled_ = true;
	return 0;
}


/****************************************************************************
 *
 * RenameDialog
 *
 */

qm::RenameDialog::RenameDialog(const WCHAR* pwszName) :
	DefaultDialog(IDD_RENAME)
{
	wstrName_ = allocWString(pwszName);
}

qm::RenameDialog::~RenameDialog()
{
}

const WCHAR* qm::RenameDialog::getName() const
{
	return wstrName_.get();
}

LRESULT qm::RenameDialog::onCommand(WORD nCode,
									WORD nId)
{
	BEGIN_COMMAND_HANDLER()
		HANDLE_COMMAND_ID_CODE(IDC_NAME, EN_CHANGE, onNameChange)
	END_COMMAND_HANDLER()
	return DefaultDialog::onCommand(nCode, nId);
}

LRESULT qm::RenameDialog::onInitDialog(HWND hwndFocus,
									   LPARAM lParam)
{
	init(false);
	setDlgItemText(IDC_NAME, wstrName_.get());
	return TRUE;
}

LRESULT qm::RenameDialog::onOk()
{
	wstrName_ = getDlgItemText(IDC_NAME);
	return DefaultDialog::onOk();
}

void qm::RenameDialog::updateState()
{
	bool bEnableOk = Window(getDlgItem(IDC_NAME)).getWindowTextLength() != 0;
	Window(getDlgItem(IDOK)).enableWindow(bEnableOk);
}

LRESULT qm::RenameDialog::onNameChange()
{
	updateState();
	return 0;
}


/****************************************************************************
 *
 * ReplaceDialog
 *
 */

qm::ReplaceDialog::ReplaceDialog(Profile* pProfile) :
	DefaultDialog(IDD_REPLACE),
	pProfile_(pProfile),
	bMatchCase_(false),
	bRegex_(false),
	type_(TYPE_NEXT)
{
}

qm::ReplaceDialog::~ReplaceDialog()
{
}

const WCHAR* qm::ReplaceDialog::getFind() const
{
	return wstrFind_.get();
}

const WCHAR* qm::ReplaceDialog::getReplace() const
{
	return wstrReplace_.get();
}

bool qm::ReplaceDialog::isMatchCase() const
{
	return bMatchCase_;
}

bool qm::ReplaceDialog::isRegex() const
{
	return bRegex_;
}

ReplaceDialog::Type qm::ReplaceDialog::getType() const
{
	return type_;
}

LRESULT qm::ReplaceDialog::onCommand(WORD nCode,
									 WORD nId)
{
	BEGIN_COMMAND_HANDLER()
		HANDLE_COMMAND_ID_RANGE(IDC_REPLACENEXT, IDC_REPLACEALL, onReplace)
		HANDLE_COMMAND_ID_CODE(IDC_FIND, CBN_EDITCHANGE, onFindChange)
		HANDLE_COMMAND_ID_CODE(IDC_FIND, CBN_SELCHANGE, onFindSelChange)
		HANDLE_COMMAND_ID(IDC_REGEX, onRegexChange)
	END_COMMAND_HANDLER()
	return DefaultDialog::onCommand(nCode, nId);
}

LRESULT qm::ReplaceDialog::onInitDialog(HWND hwndFocus,
										LPARAM lParam)
{
	init(false);
	
	for (int n = 0; n < HISTORY_SIZE; ++n) {
		WCHAR wszKey[32];
		swprintf(wszKey, L"History%d", n);
		
		struct {
			const WCHAR* pwszSection_;
			UINT nId_;
		} items[] = {
			{ L"Find",		IDC_FIND	},
			{ L"Replace",	IDC_REPLACE	}
		};
		for (int m = 0; m < countof(items); ++m) {
			wstring_ptr wstr(pProfile_->getString(items[m].pwszSection_, wszKey, L""));
			if (*wstr.get()) {
				W2T(wstr.get(), ptsz);
				sendDlgItemMessage(items[m].nId_, CB_ADDSTRING, 0,
					reinterpret_cast<LPARAM>(ptsz));
			}
		}
	}
	
	int nMatchCase = pProfile_->getInt(L"Find", L"MatchCase", 0);
	sendDlgItemMessage(IDC_MATCHCASE, BM_SETCHECK,
		nMatchCase ? BST_CHECKED : BST_UNCHECKED);
	
	int nRegex = pProfile_->getInt(L"Find", L"Regex", 0);
	sendDlgItemMessage(IDC_REGEX, BM_SETCHECK,
		nRegex ? BST_CHECKED : BST_UNCHECKED);
	
	updateState();
	
	return TRUE;
}

LRESULT qm::ReplaceDialog::onReplace(UINT nId)
{
	struct {
		UINT nId_;
		wstring_ptr* pwstr_;
		const WCHAR* pwszSection_;
	} items[] = {
		{ IDC_FIND,		&wstrFind_,		L"Find"		},
		{ IDC_REPLACE,	&wstrReplace_,	L"Replace"	}
	};
	for (int m = 0; m < countof(items); ++m) {
		Window edit(Window(getDlgItem(items[m].nId_)).getWindow(GW_CHILD));
		wstring_ptr wstrText(edit.getWindowText());
		if (wstrText.get()) {
			for (int n = HISTORY_SIZE - 1; n > 0; --n) {
				WCHAR wszFromKey[32];
				swprintf(wszFromKey, L"History%d", n - 1);
				wstring_ptr wstr(pProfile_->getString(items[m].pwszSection_, wszFromKey, L""));
				
				WCHAR wszToKey[32];
				swprintf(wszToKey, L"History%d", n);
				pProfile_->setString(items[m].pwszSection_, wszToKey, wstr.get());
			}
			
			pProfile_->setString(items[m].pwszSection_, L"History0", wstrText.get());
		}
		*items[m].pwstr_ = wstrText;
	}
	
	bMatchCase_ = sendDlgItemMessage(IDC_MATCHCASE, BM_GETCHECK) == BST_CHECKED;
	pProfile_->setInt(L"Find", L"MatchCase", bMatchCase_ ? 1 : 0);
	
	bRegex_ = sendDlgItemMessage(IDC_REGEX, BM_GETCHECK) == BST_CHECKED;
	pProfile_->setInt(L"Find", L"Regex", bRegex_ ? 1 : 0);
	
	type_ = nId == IDC_REPLACEPREV ? TYPE_PREV :
		nId == IDC_REPLACEALL ? TYPE_ALL : TYPE_NEXT;
	
	endDialog(IDOK);
	
	return 0;
}

LRESULT qm::ReplaceDialog::onFindChange()
{
	updateState();
	return 0;
}

LRESULT qm::ReplaceDialog::onFindSelChange()
{
	postMessage(WM_COMMAND, MAKEWPARAM(IDC_FIND, CBN_EDITCHANGE));
	return 0;
}

LRESULT qm::ReplaceDialog::onRegexChange()
{
	updateState();
	return 0;
}

void qm::ReplaceDialog::updateState()
{
	bool bRegex = sendDlgItemMessage(IDC_REGEX, BM_GETCHECK) == BST_CHECKED;
	Window(getDlgItem(IDC_MATCHCASE)).enableWindow(!bRegex);
	
	Window edit(Window(getDlgItem(IDC_FIND)).getWindow(GW_CHILD));
	bool bEnable = edit.getWindowTextLength() != 0;
	Window(getDlgItem(IDC_REPLACENEXT)).enableWindow(bEnable);
	Window(getDlgItem(IDC_REPLACEPREV)).enableWindow(bEnable);
	Window(getDlgItem(IDC_REPLACEALL)).enableWindow(bEnable);
}


/****************************************************************************
 *
 * ResourceDialog
 *
 */

qm::ResourceDialog::ResourceDialog(ResourceList& listResource) :
	DefaultDialog(IDD_RESOURCE),
	listResource_(listResource),
	bBackup_(false)
{
}

qm::ResourceDialog::~ResourceDialog()
{
}

bool qm::ResourceDialog::isBackup() const
{
	return bBackup_;
}

LRESULT qm::ResourceDialog::onInitDialog(HWND hwndFocus,
										 LPARAM lParam)
{
	init(false);
	
	HINSTANCE hInst = Application::getApplication().getResourceHandle();
	HWND hwnd = getDlgItem(IDC_RESOURCE);
	
	ListView_SetExtendedListViewStyle(hwnd, LVS_EX_CHECKBOXES | LVS_EX_FULLROWSELECT);
	
	RECT rect;
	Window(hwnd).getClientRect(&rect);
	int nWidth = rect.right - rect.left - ::GetSystemMetrics(SM_CXVSCROLL);
	wstring_ptr wstrPath(loadString(hInst, IDS_RESOURCEPATH));
	W2T(wstrPath.get(), ptszPath);
	LVCOLUMN column = {
		LVCF_FMT | LVCF_TEXT | LVCF_WIDTH,
		LVCFMT_LEFT,
		nWidth,
		const_cast<LPTSTR>(ptszPath),
		0,
	};
	ListView_InsertColumn(hwnd, 0, &column);
	
	for (ResourceList::size_type n = 0; n < listResource_.size(); ++n) {
		W2T(listResource_[n].first, ptszName);
		
		LVITEM item = {
			LVIF_TEXT,
			n,
			0,
			0,
			0,
			const_cast<LPTSTR>(ptszName),
		};
		ListView_InsertItem(hwnd, &item);
		ListView_SetCheckState(hwnd, n, TRUE);
	}
	
	sendDlgItemMessage(IDC_BACKUP, BM_SETCHECK, BST_CHECKED);
	
	return TRUE;
}

LRESULT qm::ResourceDialog::onOk()
{
	HWND hwnd = getDlgItem(IDC_RESOURCE);
	
	for (ResourceList::size_type n = 0; n < listResource_.size(); ++n)
		listResource_[n].second = ListView_GetCheckState(hwnd, n) != 0;
	
	bBackup_ = sendDlgItemMessage(IDC_BACKUP, BM_GETCHECK) == BST_CHECKED;
	
	return DefaultDialog::onOk();
}


/****************************************************************************
 *
 * SelectDialupEntryDialog
 *
 */

qm::SelectDialupEntryDialog::SelectDialupEntryDialog(Profile* pProfile) :
	DefaultDialog(IDD_SELECTDIALUPENTRY),
	pProfile_(pProfile)
{
}

qm::SelectDialupEntryDialog::~SelectDialupEntryDialog()
{
}

const WCHAR* qm::SelectDialupEntryDialog::getEntry() const
{
	return wstrEntry_.get();
}

LRESULT qm::SelectDialupEntryDialog::onCommand(WORD nCode,
											   WORD nId)
{
	BEGIN_COMMAND_HANDLER()
		HANDLE_COMMAND_ID_CODE(IDC_ENTRY, LBN_SELCHANGE, onSelChange)
	END_COMMAND_HANDLER()
	return DefaultDialog::onCommand(nCode, nId);
}

LRESULT qm::SelectDialupEntryDialog::onInitDialog(HWND hwndFocus,
												  LPARAM lParam)
{
	init(false);
	
	typedef RasConnection::EntryList List;
	List listEntry;
	StringListFree<List> free(listEntry);
	RasConnection::getEntries(&listEntry);
	
	for (List::const_iterator it = listEntry.begin(); it != listEntry.end(); ++it) {
		W2T(*it, ptszEntry);
		sendDlgItemMessage(IDC_ENTRY, LB_ADDSTRING, 0,
			reinterpret_cast<LPARAM>(ptszEntry));
	}
	
	wstring_ptr wstrEntry(pProfile_->getString(L"Dialup", L"Entry", 0));
	W2T(wstrEntry.get(), ptszEntry);
	sendDlgItemMessage(IDC_ENTRY, LB_SELECTSTRING, -1,
		reinterpret_cast<LPARAM>(ptszEntry));
	
	updateState();
	
	return TRUE;
}

LRESULT qm::SelectDialupEntryDialog::onOk()
{
	int nIndex = sendDlgItemMessage(IDC_ENTRY, LB_GETCURSEL);
	if (nIndex == LB_ERR)
		return 0;
	
	int nLen = sendDlgItemMessage(IDC_ENTRY, LB_GETTEXTLEN, nIndex);
	if (nLen == LB_ERR)
		return 0;
	
	tstring_ptr tstrEntry(allocTString(nLen + 1));
	sendDlgItemMessage(IDC_ENTRY, LB_GETTEXT, nIndex,
		reinterpret_cast<LPARAM>(tstrEntry.get()));
	
	wstrEntry_ = tcs2wcs(tstrEntry.get());
	pProfile_->setString(L"Dialup", L"Entry", wstrEntry_.get());
	
	return DefaultDialog::onOk();
}

LRESULT qm::SelectDialupEntryDialog::onSelChange()
{
	updateState();
	return 0;
}

void qm::SelectDialupEntryDialog::updateState()
{
	bool bEnable = sendDlgItemMessage(IDC_ENTRY, LB_GETCURSEL) != LB_ERR;
	Window(getDlgItem(IDOK)).enableWindow(bEnable);
}


/****************************************************************************
 *
 * SelectSyncFilterDialog
 *
 */

qm::SelectSyncFilterDialog::SelectSyncFilterDialog(SyncFilterManager* pManager,
												   Account* pAccount,
												   const WCHAR* pwszDefaultName) :
	DefaultDialog(IDD_SELECTSYNCFILTER),
	pwszName_(pwszDefaultName)
{
	pManager->getFilterSets(pAccount, &list_);
}

qm::SelectSyncFilterDialog::~SelectSyncFilterDialog()
{
}

const WCHAR* qm::SelectSyncFilterDialog::getName() const
{
	return pwszName_;
}

LRESULT qm::SelectSyncFilterDialog::onInitDialog(HWND hwndFocus,
												 LPARAM lParam)
{
	init(false);
	
	if (list_.empty()) {
		endDialog(IDOK);
	}
	else {
		typedef SyncFilterManager::FilterSetList List;
		for (List::const_iterator it = list_.begin(); it != list_.end(); ++it) {
			W2T((*it)->getName(), ptszName);
			sendDlgItemMessage(IDC_FILTERSETLIST, LB_ADDSTRING,
				0, reinterpret_cast<LPARAM>(ptszName));
		}
		W2T(pwszName_, ptszName);
		sendDlgItemMessage(IDC_FILTERSETLIST, LB_SELECTSTRING,
			-1, reinterpret_cast<LPARAM>(ptszName));
	}
	
	return TRUE;
}

LRESULT qm::SelectSyncFilterDialog::onOk()
{
	unsigned int nItem = sendDlgItemMessage(IDC_FILTERSETLIST, LB_GETCURSEL);
	if (nItem == LB_ERR)
		return onCancel();
	
	assert(nItem < list_.size());
	pwszName_ = list_[nItem]->getName();
	
	return DefaultDialog::onOk();
}


/****************************************************************************
 *
 * ViewsColumnDialog
 *
 */

qm::ViewsColumnDialog::ViewsColumnDialog(ViewColumn* pColumn) :
	DefaultDialog(IDD_VIEWSCOLUMN),
	pColumn_(pColumn)
{
}

qm::ViewsColumnDialog::~ViewsColumnDialog()
{
}

LRESULT qm::ViewsColumnDialog::onCommand(WORD nCode,
										 WORD nId)
{
	BEGIN_COMMAND_HANDLER()
		HANDLE_COMMAND_ID_CODE(IDC_TYPE, CBN_SELCHANGE, onTypeSelChange)
	END_COMMAND_HANDLER()
	return DefaultDialog::onCommand(nCode, nId);
}

LRESULT qm::ViewsColumnDialog::onInitDialog(HWND hwndFocus,
											LPARAM lParam)
{
	init(false);
	
	HINSTANCE hInst = Application::getApplication().getResourceHandle();
	
	setDlgItemText(IDC_TITLE, pColumn_->getTitle());
	
	UINT nTypes[] = {
		IDS_COLUMNTYPE_ID,
		IDS_COLUMNTYPE_DATE,
		IDS_COLUMNTYPE_FROM,
		IDS_COLUMNTYPE_TO,
		IDS_COLUMNTYPE_FROMTO,
		IDS_COLUMNTYPE_SUBJECT,
		IDS_COLUMNTYPE_SIZE,
		IDS_COLUMNTYPE_FLAGS,
		IDS_COLUMNTYPE_OTHER
	};
	for (int n = 0; n < countof(nTypes); ++n) {
		wstring_ptr wstrType(loadString(hInst, nTypes[n]));
		W2T(wstrType.get(), ptszType);
		sendDlgItemMessage(IDC_TYPE, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(ptszType));
	}
	sendDlgItemMessage(IDC_TYPE, CB_SETCURSEL, pColumn_->getType() - 1);
	
	setDlgItemInt(IDC_WIDTH, pColumn_->getWidth());
	
	if (pColumn_->getType() == ViewColumn::TYPE_OTHER) {
		wstring_ptr wstrMacro(pColumn_->getMacro()->getString());
		setDlgItemText(IDC_MACRO, wstrMacro.get());
	}
	
	unsigned int nFlags = pColumn_->getFlags();
	if (nFlags & ViewColumn::FLAG_INDENT)
		sendDlgItemMessage(IDC_INDENT, BM_SETCHECK, BST_CHECKED);
	if (nFlags & ViewColumn::FLAG_LINE)
		sendDlgItemMessage(IDC_LINE, BM_SETCHECK, BST_CHECKED);
	if (nFlags & ViewColumn::FLAG_ICON)
		sendDlgItemMessage(IDC_ASICON, BM_SETCHECK, BST_CHECKED);
	
	if (nFlags & ViewColumn::FLAG_RIGHTALIGN)
		sendDlgItemMessage(IDC_RIGHTALIGN, BM_SETCHECK, BST_CHECKED);
	else
		sendDlgItemMessage(IDC_LEFTALIGN, BM_SETCHECK, BST_CHECKED);
	
	switch (nFlags & ViewColumn::FLAG_SORT_MASK) {
	case ViewColumn::FLAG_SORT_TEXT:
		sendDlgItemMessage(IDC_SORTTEXT, BM_SETCHECK, BST_CHECKED);
		break;
	case ViewColumn::FLAG_SORT_NUMBER:
		sendDlgItemMessage(IDC_SORTNUMBER, BM_SETCHECK, BST_CHECKED);
		break;
	case ViewColumn::FLAG_SORT_DATE:
		sendDlgItemMessage(IDC_SORTDATE, BM_SETCHECK, BST_CHECKED);
		break;
	default:
		sendDlgItemMessage(IDC_SORTTEXT, BM_SETCHECK, BST_CHECKED);
		break;
	}
	
	updateState();
	
	return TRUE;
}

LRESULT qm::ViewsColumnDialog::onOk()
{
	wstring_ptr wstrTitle(getDlgItemText(IDC_TITLE));
	ViewColumn::Type type = static_cast<ViewColumn::Type>(
		sendDlgItemMessage(IDC_TYPE, CB_GETCURSEL) + 1);
	std::auto_ptr<Macro> pMacro;
	if (type == ViewColumn::TYPE_OTHER) {
		wstring_ptr wstrMacro(getDlgItemText(IDC_MACRO));
		pMacro = MacroParser(MacroParser::TYPE_COLUMN).parse(wstrMacro.get());
		if (!pMacro.get()) {
			HINSTANCE hInst = Application::getApplication().getResourceHandle();
			messageBox(hInst, IDS_ERROR_INVALIDMACRO, MB_OK | MB_ICONERROR, getHandle());
			return 0;
		}
	}
	
	unsigned int nWidth = getDlgItemInt(IDC_WIDTH);
	
	unsigned int nFlags = 0;
	if (sendDlgItemMessage(IDC_INDENT, BM_GETCHECK) == BST_CHECKED)
		nFlags |= ViewColumn::FLAG_INDENT;
	if (sendDlgItemMessage(IDC_LINE, BM_GETCHECK) == BST_CHECKED)
		nFlags |= ViewColumn::FLAG_LINE;
	if (sendDlgItemMessage(IDC_ASICON, BM_GETCHECK) == BST_CHECKED)
		nFlags |= ViewColumn::FLAG_ICON;
	if (sendDlgItemMessage(IDC_RIGHTALIGN, BM_GETCHECK) == BST_CHECKED)
		nFlags |= ViewColumn::FLAG_RIGHTALIGN;
	if (sendDlgItemMessage(IDC_SORTNUMBER, BM_GETCHECK) == BST_CHECKED)
		nFlags |= ViewColumn::FLAG_SORT_NUMBER;
	else if (sendDlgItemMessage(IDC_SORTDATE, BM_GETCHECK) == BST_CHECKED)
		nFlags |= ViewColumn::FLAG_SORT_DATE;
	else
		nFlags |= ViewColumn::FLAG_SORT_TEXT;
	
	pColumn_->set(wstrTitle.get(), type, pMacro, nFlags, nWidth);
	
	return DefaultDialog::onOk();
}

LRESULT qm::ViewsColumnDialog::onTypeSelChange()
{
	updateState();
	return 0;
}

void qm::ViewsColumnDialog::updateState()
{
	bool bEnableMacro = sendDlgItemMessage(IDC_TYPE, CB_GETCURSEL) ==
		sendDlgItemMessage(IDC_TYPE, CB_GETCOUNT) - 1;
	Window(getDlgItem(IDC_MACRO)).enableWindow(bEnableMacro);
}


/****************************************************************************
 *
 * ViewsDialog
 *
 */

qm::ViewsDialog::ViewsDialog(UIManager* pUIManager,
							 ViewModelManager* pViewModelManager,
							 ViewModel* pViewModel) :
	DefaultDialog(IDD_VIEWS),
	pUIManager_(pUIManager),
	pViewModelManager_(pViewModelManager),
	pViewModel_(pViewModel)
{
	const ViewColumnList& listColumn = pViewModel->getColumns();
	listColumn_.reserve(listColumn.size());
	for (ViewColumnList::const_iterator it = listColumn.begin(); it != listColumn.end(); ++it) {
		std::auto_ptr<ViewColumn> pColumn((*it)->clone());
		listColumn_.push_back(pColumn.release());
	}
}

qm::ViewsDialog::~ViewsDialog()
{
	std::for_each(listColumn_.begin(), listColumn_.end(), deleter<ViewColumn>());
}

LRESULT qm::ViewsDialog::onCommand(WORD nCode,
								   WORD nId)
{
	BEGIN_COMMAND_HANDLER()
		HANDLE_COMMAND_ID(IDC_ADD, onAdd)
		HANDLE_COMMAND_ID(IDC_REMOVE, onRemove)
		HANDLE_COMMAND_ID(IDC_EDIT, onEdit)
		HANDLE_COMMAND_ID(IDC_UP, onUp)
		HANDLE_COMMAND_ID(IDC_DOWN, onDown)
		HANDLE_COMMAND_ID(IDC_ASDEFAULT, onAsDefault)
		HANDLE_COMMAND_ID(IDC_APPLYDEFAULT, onApplyDefault)
		HANDLE_COMMAND_ID(IDC_INHERIT, onInherit)
		HANDLE_COMMAND_ID(IDC_APPLYTOALL, onApplyToAll)
		HANDLE_COMMAND_ID(IDC_APPLYTOCHILDREN, onApplyToChildren)
	END_COMMAND_HANDLER()
	return DefaultDialog::onCommand(nCode, nId);
}

LRESULT qm::ViewsDialog::onNotify(NMHDR* pnmhdr,
								  bool* pbHandled)
{
	BEGIN_NOTIFY_HANDLER()
		HANDLE_NOTIFY(NM_DBLCLK, IDC_COLUMNS, onColumnsDblClk)
		HANDLE_NOTIFY(LVN_ITEMCHANGED, IDC_COLUMNS, onColumnsItemChanged)
	END_NOTIFY_HANDLER()
	return 1;
}

LRESULT qm::ViewsDialog::onDestroy()
{
	removeNotifyHandler(this);
	return DefaultDialog::onDestroy();
}

LRESULT qm::ViewsDialog::onInitDialog(HWND hwndFocus,
									  LPARAM lParam)
{
	init(false);
	
	HINSTANCE hInst = Application::getApplication().getResourceHandle();
	
	HWND hwndList = getDlgItem(IDC_COLUMNS);
	ListView_SetExtendedListViewStyle(hwndList, LVS_EX_FULLROWSELECT);
	
	struct {
		UINT nId_;
		int nWidth_;
	} columns[] = {
#ifndef _WIN32_WCE_PSPC
		{ IDS_TITLE,	150	},
		{ IDS_TYPE,		150	},
		{ IDS_WIDTH,	50	}
#else
		{ IDS_TITLE,	80	},
		{ IDS_TYPE,		80	},
		{ IDS_WIDTH,	30	}
#endif
	};
	for (int n = 0; n < countof(columns); ++n) {
		wstring_ptr wstrColumn(loadString(hInst, columns[n].nId_));
		W2T(wstrColumn.get(), ptszColumn);
		
		LVCOLUMN column = {
			LVCF_FMT | LVCF_TEXT | LVCF_WIDTH | LVCF_SUBITEM,
			LVCFMT_LEFT,
			columns[n].nWidth_,
			const_cast<LPTSTR>(ptszColumn),
			0,
			n,
		};
		ListView_InsertColumn(hwndList, n, &column);
	}
	
	update();
	updateState();
	addNotifyHandler(this);
	
	return TRUE;
}

LRESULT qm::ViewsDialog::onOk()
{
	pViewModel_->setColumns(listColumn_);
	listColumn_.clear();
	return DefaultDialog::onOk();
}

LRESULT qm::ViewsDialog::onAdd()
{
	HWND hwndList = getDlgItem(IDC_COLUMNS);
	
	std::auto_ptr<Macro> pMacro;
	std::auto_ptr<ViewColumn> pColumn(new ViewColumn(L"New Column",
		ViewColumn::TYPE_ID, pMacro, ViewColumn::FLAG_SORT_TEXT, 100));
	ViewsColumnDialog dialog(pColumn.get());
	if (dialog.doModal(getHandle()) == IDOK) {
		listColumn_.push_back(pColumn.get());
		pColumn.release();
		update();
		ListView_SetItemState(hwndList, ListView_GetItemCount(hwndList) - 1,
			LVIS_FOCUSED | LVIS_SELECTED, LVIS_FOCUSED | LVIS_SELECTED);
	}
	return 0;
}

LRESULT qm::ViewsDialog::onRemove()
{
	HWND hwndList = getDlgItem(IDC_COLUMNS);
	if (ListView_GetItemCount(hwndList) > 1) {
		int nItem = ListView_GetNextItem(hwndList, -1, LVNI_ALL | LVNI_SELECTED);
		if (nItem != -1) {
			ViewColumnList::iterator it = listColumn_.begin() + nItem;
			delete *it;
			listColumn_.erase(it);
			update();
			ListView_SetItemState(hwndList, nItem,
				LVIS_FOCUSED | LVIS_SELECTED, LVIS_FOCUSED | LVIS_SELECTED);
		}
	}
	return 0;
}

LRESULT qm::ViewsDialog::onEdit()
{
	HWND hwndList = getDlgItem(IDC_COLUMNS);
	int nItem = ListView_GetNextItem(hwndList, -1, LVNI_ALL | LVNI_SELECTED);
	if (nItem != -1) {
		LVITEM item = {
			LVIF_PARAM,
			nItem
		};
		ListView_GetItem(hwndList, &item);
		ViewColumn* pColumn = reinterpret_cast<ViewColumn*>(item.lParam);
		
		ViewsColumnDialog dialog(pColumn);
		if (dialog.doModal(getHandle()) == IDOK) {
			update();
			ListView_SetItemState(hwndList, nItem,
				LVIS_FOCUSED | LVIS_SELECTED, LVIS_FOCUSED | LVIS_SELECTED);
		}
	}
	
	return 0;
}

LRESULT qm::ViewsDialog::onUp()
{
	HWND hwndList = getDlgItem(IDC_COLUMNS);
	int nItem = ListView_GetNextItem(hwndList, -1, LVNI_ALL | LVNI_SELECTED);
	if (nItem != -1 && nItem != 0) {
		std::swap(listColumn_[nItem - 1], listColumn_[nItem]);
		update();
		ListView_SetItemState(hwndList, nItem - 1,
			LVIS_FOCUSED | LVIS_SELECTED, LVIS_FOCUSED | LVIS_SELECTED);
	}
	return 0;
}

LRESULT qm::ViewsDialog::onDown()
{
	HWND hwndList = getDlgItem(IDC_COLUMNS);
	int nItem = ListView_GetNextItem(hwndList, -1, LVNI_ALL | LVNI_SELECTED);
	if (nItem != -1 && nItem != ListView_GetItemCount(hwndList) - 1) {
		std::swap(listColumn_[nItem + 1], listColumn_[nItem]);
		update();
		ListView_SetItemState(hwndList, nItem + 1,
			LVIS_FOCUSED | LVIS_SELECTED, LVIS_FOCUSED | LVIS_SELECTED);
	}
	return 0;
}

LRESULT qm::ViewsDialog::onAsDefault()
{
	ViewDataItem* pItem = getDefaultItem();
	ViewColumnList listColumn;
	cloneColumns(listColumn_, &listColumn);
	pItem->setColumns(listColumn);
	return 0;
}

LRESULT qm::ViewsDialog::onApplyDefault()
{
	ViewDataItem* pItem = getDefaultItem();
	setColumns(pItem->getColumns());
	update();
	return 0;
}

LRESULT qm::ViewsDialog::onInherit()
{
	Folder* pFolder = pViewModel_->getFolder()->getParentFolder();
	if (pFolder) {
		ViewModel* pViewModel = pViewModelManager_->getViewModel(pFolder);
		setColumns(pViewModel->getColumns());
		update();
	}
	return 0;
}

LRESULT qm::ViewsDialog::onApplyToAll()
{
	Account* pAccount = pViewModel_->getFolder()->getAccount();
	const Account::FolderList& listFolder = pAccount->getFolders();
	for (Account::FolderList::const_iterator it = listFolder.begin(); it != listFolder.end(); ++it) {
		Folder* pFolder = *it;
		ViewModel* pViewModel = pViewModelManager_->getViewModel(pFolder);
		ViewColumnList listColumn;
		cloneColumns(listColumn_, &listColumn);
		pViewModel->setColumns(listColumn);
	}
	
	Window(getDlgItem(IDCANCEL)).enableWindow(false);
	
	return 0;
}

LRESULT qm::ViewsDialog::onApplyToChildren()
{
	Folder* pCurrentFolder = pViewModel_->getFolder();
	Account* pAccount = pCurrentFolder->getAccount();
	const Account::FolderList& listFolder = pAccount->getFolders();
	for (Account::FolderList::const_iterator it = listFolder.begin(); it != listFolder.end(); ++it) {
		Folder* pFolder = *it;
		if (pCurrentFolder->isAncestorOf(pFolder)) {
			ViewModel* pViewModel = pViewModelManager_->getViewModel(pFolder);
			ViewColumnList listColumn;
			cloneColumns(listColumn_, &listColumn);
			pViewModel->setColumns(listColumn);
		}
	}
	return 0;
}

LRESULT qm::ViewsDialog::onColumnsDblClk(NMHDR* pnmhdr,
										 bool* pbHandled)
{
	onEdit();
	*pbHandled = true;
	return 0;
}

LRESULT qm::ViewsDialog::onColumnsItemChanged(NMHDR* pnmhdr,
											  bool* pbHandled)
{
	updateState();
	*pbHandled = true;
	return 0;
}

void qm::ViewsDialog::update()
{
	HINSTANCE hInst = Application::getApplication().getResourceHandle();
	HWND hwndList = getDlgItem(IDC_COLUMNS);
	
	DisableRedraw disable(hwndList);
	
	ListView_DeleteAllItems(hwndList);
	
	UINT nTypes[] = {
		0,
		IDS_COLUMNTYPE_ID,
		IDS_COLUMNTYPE_DATE,
		IDS_COLUMNTYPE_FROM,
		IDS_COLUMNTYPE_TO,
		IDS_COLUMNTYPE_FROMTO,
		IDS_COLUMNTYPE_SUBJECT,
		IDS_COLUMNTYPE_SIZE,
		IDS_COLUMNTYPE_FLAGS,
		IDS_COLUMNTYPE_OTHER
	};
	
	for (ViewColumnList::size_type n = 0; n < listColumn_.size(); ++n) {
		ViewColumn* pColumn = listColumn_[n];
		
		W2T(pColumn->getTitle(), ptszTitle);
		
		LVITEM item = {
			LVIF_TEXT | LVIF_PARAM,
			n,
			0,
			0,
			0,
			const_cast<LPTSTR>(ptszTitle),
			0,
			0,
			reinterpret_cast<LPARAM>(pColumn)
		};
		ListView_InsertItem(hwndList, &item);
		
		wstring_ptr wstrType(loadString(hInst, nTypes[pColumn->getType()]));
		W2T(wstrType.get(), ptszType);
		ListView_SetItemText(hwndList, n, 1, const_cast<LPTSTR>(ptszType));
		
		WCHAR wszWidth[32];
		swprintf(wszWidth, L"%u", pColumn->getWidth());
		W2T(wszWidth, ptszWidth);
		ListView_SetItemText(hwndList, n, 2, const_cast<LPTSTR>(ptszWidth));
	}
}

void qm::ViewsDialog::updateState()
{
	struct {
		UINT nId_;
		bool bEnable_;
	} items[] = {
		{ IDC_REMOVE,	true	},
		{ IDC_EDIT,		true	},
		{ IDC_UP,		true	},
		{ IDC_DOWN,		true	}
	};
	
	HWND hwndList = getDlgItem(IDC_COLUMNS);
	int nItem = ListView_GetNextItem(hwndList, -1, LVNI_ALL | LVNI_SELECTED);
	if (nItem == -1) {
		for (int n = 0; n < countof(items); ++n)
			items[n].bEnable_ = false;
	}
	else if (nItem == 0) {
		items[2].bEnable_ = false;
	}
	else if (nItem == ListView_GetItemCount(hwndList) - 1) {
		items[3].bEnable_ = false;
	}
	
	for (int n = 0; n < countof(items); ++n)
		Window(getDlgItem(items[n].nId_)).enableWindow(items[n].bEnable_);
	
	Window(getDlgItem(IDC_INHERIT)).enableWindow(pViewModel_->getFolder()->getParentFolder() != 0);
}

void qm::ViewsDialog::setColumns(const ViewColumnList& listColumn)
{
	std::for_each(listColumn_.begin(), listColumn_.end(), deleter<ViewColumn>());
	listColumn_.clear();
	cloneColumns(listColumn, &listColumn_);
}

void qm::ViewsDialog::cloneColumns(const ViewColumnList& listColumn,
								   ViewColumnList* pListColumn)
{
	assert(pListColumn);
	assert(pListColumn->empty());
	
	pListColumn->reserve(listColumn.size());
	for (ViewColumnList::const_iterator it = listColumn.begin(); it != listColumn.end(); ++it) {
		std::auto_ptr<ViewColumn> pColumn((*it)->clone());
		pListColumn->push_back(pColumn.release());
	}
}

ViewDataItem* qm::ViewsDialog::getDefaultItem()
{
	Folder* pFolder = pViewModel_->getFolder();
	Account* pAccount = pFolder->getAccount();
	DefaultViewData* pDefaultViewData = pUIManager_->getDefaultViewData();
	return pDefaultViewData->getItem(pAccount->getClass());
}
