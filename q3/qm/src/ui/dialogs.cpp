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
#include <qmfilenames.h>
#include <qmsession.h>

#include <qsconv.h>
#include <qsmime.h>
#include <qsnew.h>
#include <qsras.h>
#include <qsuiutil.h>

#include <algorithm>

#include <commdlg.h>
#include <tchar.h>

#include "dialogs.h"
#include "propertypages.h"
#include "resourceinc.h"
#include "uiutil.h"
#include "../model/addressbook.h"
#include "../model/fixedformtext.h"

using namespace qm;
using namespace qs;


/****************************************************************************
 *
 * DefaultDialog
 *
 */

qm::DefaultDialog::DefaultDialog(UINT nId, QSTATUS* pstatus) :
	qs::DefaultDialog(Application::getApplication().getResourceHandle(), nId, pstatus)
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

qm::AccountDialog::AccountDialog(Document* pDocument, Account* pAccount,
	SyncFilterManager* pSyncFilterManager, Profile* pProfile, QSTATUS* pstatus) :
	DefaultDialog(IDD_ACCOUNT, pstatus),
	pDocument_(pDocument),
	pSubAccount_(pAccount ? pAccount->getCurrentSubAccount() : 0),
	pSyncFilterManager_(pSyncFilterManager),
	pProfile_(pProfile)
{
}

qm::AccountDialog::~AccountDialog()
{
}

LRESULT qm::AccountDialog::onCommand(WORD nCode, WORD nId)
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

LRESULT qm::AccountDialog::onInitDialog(HWND hwndFocus, LPARAM lParam)
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

LRESULT qm::AccountDialog::onNotify(NMHDR* pnmhdr, bool* pbHandled)
{
	BEGIN_NOTIFY_HANDLER()
		HANDLE_NOTIFY(TVN_SELCHANGED, IDC_ACCOUNT, onAccountSelChanged);
	END_NOTIFY_HANDLER()
	return 1;
}

LRESULT qm::AccountDialog::onAddAccount()
{
	DECLARE_QSTATUS();
	
	CreateAccountDialog dialog(pProfile_, &status);
	CHECK_QSTATUS_VALUE(0);
	int nRet = 0;
	status = dialog.doModal(getHandle(), 0, &nRet);
	CHECK_QSTATUS_VALUE(0);
	if (nRet == IDOK) {
		string_ptr<WSTRING> wstrDir(concat(
			Application::getApplication().getMailFolder(),
			L"\\accounts\\", dialog.getName()));
		if (!wstrDir.get())
			return 0;
		W2T(wstrDir.get(), ptszDir);
		if (!::CreateDirectory(ptszDir, 0))
			return 0;
		
		string_ptr<WSTRING> wstrPath(concat(
			wstrDir.get(), L"\\", FileNames::ACCOUNT_XML));
		if (!wstrPath.get())
			return 0;
		XMLProfile profile(wstrPath.get(), &status);
		CHECK_QSTATUS_VALUE(0);
		status = profile.setString(L"Global", L"Class", dialog.getClass());
		CHECK_QSTATUS_VALUE(0);
		status = profile.setInt(L"Global", L"BlockSize", dialog.getBlockSize());
		CHECK_QSTATUS_VALUE(0);
		status = profile.setInt(L"Global", L"CacheBlockSize", dialog.getCacheBlockSize());
		CHECK_QSTATUS_VALUE(0);
		status = profile.setString(L"Receive", L"Type", dialog.getReceiveProtocol());
		CHECK_QSTATUS_VALUE(0);
		status = profile.setInt(L"Receive", L"Port", dialog.getReceivePort());
		CHECK_QSTATUS_VALUE(0);
		status = profile.setString(L"Send", L"Type", dialog.getSendProtocol());
		CHECK_QSTATUS_VALUE(0);
		status = profile.setInt(L"Send", L"Port", dialog.getSendPort());
		CHECK_QSTATUS_VALUE(0);
		status = profile.save();
		CHECK_QSTATUS_VALUE(0);
		
		std::auto_ptr<Account> pAccount;
		status = newQsObject(wstrDir.get(), pDocument_->getSecurity(), &pAccount);
		CHECK_QSTATUS_VALUE(0);
		status = pDocument_->addAccount(pAccount.get());
		CHECK_QSTATUS_VALUE(0);
		pSubAccount_ = pAccount->getCurrentSubAccount();
		pAccount.release();
		
		status = update();
		CHECK_QSTATUS_VALUE(0);
		
		postMessage(WM_COMMAND, MAKEWPARAM(IDC_PROPERTY, BN_CLICKED),
			reinterpret_cast<LPARAM>(getDlgItem(IDC_PROPERTY)));
	}
	
	return 0;
}

LRESULT qm::AccountDialog::onAddSubAccount()
{
	DECLARE_QSTATUS();
	
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
		
		CreateSubAccountDialog dialog(pDocument_, &status);
		CHECK_QSTATUS_VALUE(0);
		int nRet = 0;
		status = dialog.doModal(getHandle(), 0, &nRet);
		CHECK_QSTATUS_VALUE(0);
		if (nRet == IDOK) {
			const WCHAR* pwszName = dialog.getName();
			
			if (pAccount->getSubAccount(pwszName)) {
				// TODO
				// Message box
				return 0;
			}
			
			status = pAccount->save();
			CHECK_QSTATUS();
			
			string_ptr<WSTRING> wstrAccountPath(concat(
				pAccount->getPath(), L"\\", FileNames::ACCOUNT_XML));
			if (!wstrAccountPath.get())
				return 0;
			
			ConcatW c[] = {
				{ pAccount->getPath(),		-1	},
				{ L"\\",					1	},
				{ FileNames::ACCOUNT,		-1	},
				{ L"_",						1	},
				{ pwszName,					-1	},
				{ FileNames::XML_EXT,		-1	}
			};
			string_ptr<WSTRING> wstrPath(concat(c, countof(c)));
			if (!wstrPath.get())
				return 0;
			
			W2T_STATUS(wstrAccountPath.get(), ptszAccountPath);
			CHECK_QSTATUS_VALUE(0);
			W2T_STATUS(wstrPath.get(), ptszPath);
			CHECK_QSTATUS_VALUE(0);
			if (!::CopyFile(ptszAccountPath, ptszPath, FALSE))
				return 0;
			
			std::auto_ptr<XMLProfile> pProfile;
			status = newQsObject(wstrPath.get(), &pProfile);
			CHECK_QSTATUS_VALUE(0);
			status = pProfile->load();
			CHECK_QSTATUS();
			
			std::auto_ptr<SubAccount> pSubAccount;
			status = newQsObject(pAccount, pProfile.get(),
				pwszName, &pSubAccount);
			CHECK_QSTATUS();
			pProfile.release();
			status = pAccount->addSubAccount(pSubAccount.get());
			CHECK_QSTATUS_VALUE(0);
			pSubAccount_ = pSubAccount.release();
			
			status = update();
			CHECK_QSTATUS_VALUE(0);
			
			postMessage(WM_COMMAND, MAKEWPARAM(IDC_PROPERTY, BN_CLICKED),
				reinterpret_cast<LPARAM>(getDlgItem(IDC_PROPERTY)));
		}
	}
	
	return 0;
}

LRESULT qm::AccountDialog::onRemove()
{
	DECLARE_QSTATUS();
	
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
			
			int nRet = 0;
			status = messageBox(hInst, IDS_CONFIRMREMOVESUBACCOUNT,
				MB_YESNO | MB_DEFBUTTON2, getHandle(), 0, 0, &nRet);
			CHECK_QSTATUS();
			if (nRet == IDYES) {
				Account* pAccount = pSubAccount->getAccount();
				status = pAccount->removeSubAccount(pSubAccount);
				CHECK_QSTATUS_VALUE(0);
				pSubAccount_ = pAccount->getCurrentSubAccount();
				status = update();
				CHECK_QSTATUS_VALUE(0);
			}
		}
		else {
			Account* pAccount = reinterpret_cast<Account*>(item.lParam);
			
			int nRet = 0;
			status = messageBox(hInst, IDS_CONFIRMREMOVEACCOUNT,
				MB_YESNO | MB_DEFBUTTON2, getHandle(), 0, 0, &nRet);
			CHECK_QSTATUS();
			if (nRet == IDYES) {
				status = pDocument_->removeAccount(pAccount);
				CHECK_QSTATUS();
				status = update();
				CHECK_QSTATUS_VALUE(0);
			}
		}
	}
	
	updateState();
	
	return 0;
}

LRESULT qm::AccountDialog::onRename()
{
	DECLARE_QSTATUS();
	
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
			
			RenameDialog dialog(pSubAccount->getName(), &status);
			CHECK_QSTATUS_VALUE(0);
			int nRet = 0;
			status = dialog.doModal(getHandle(), 0, &nRet);
			CHECK_QSTATUS();
			if (nRet == IDOK) {
				Account* pAccount = pSubAccount->getAccount();
				status = pAccount->renameSubAccount(pSubAccount, dialog.getName());
				CHECK_QSTATUS_VALUE(0);
				status = update();
				CHECK_QSTATUS_VALUE(0);
			}
		}
		else {
			Account* pAccount = reinterpret_cast<Account*>(item.lParam);
			
			RenameDialog dialog(pAccount->getName(), &status);
			CHECK_QSTATUS_VALUE(0);
			int nRet = 0;
			status = dialog.doModal(getHandle(), 0, &nRet);
			CHECK_QSTATUS();
			if (nRet == IDOK) {
				status = pDocument_->renameAccount(pAccount, dialog.getName());
				CHECK_QSTATUS();
				status = update();
				CHECK_QSTATUS_VALUE(0);
			}
		}
	}
	
	return 0;
}

LRESULT qm::AccountDialog::onProperty()
{
	DECLARE_QSTATUS();
	
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
		string_ptr<WSTRING> wstrTitle;
		status = loadString(hInst, IDS_ACCOUNT, &wstrTitle);
		CHECK_QSTATUS();
		
		Account* pAccount = pSubAccount->getAccount();
		PropertyPage* pPage = 0;
		
		std::auto_ptr<ReceiveSessionUI> pReceiveUI;
		status = ReceiveSessionFactory::getUI(
			pAccount->getType(Account::HOST_RECEIVE), &pReceiveUI);
		CHECK_QSTATUS_VALUE(0);
		status = pReceiveUI->createPropertyPage(pSubAccount, &pPage);
		CHECK_QSTATUS_VALUE(0);
		std::auto_ptr<PropertyPage> pReceivePage(pPage);
		
		std::auto_ptr<SendSessionUI> pSendUI;
		status = SendSessionFactory::getUI(
			pAccount->getType(Account::HOST_SEND), &pSendUI);
		CHECK_QSTATUS_VALUE(0);
		status = pSendUI->createPropertyPage(pSubAccount, &pPage);
		CHECK_QSTATUS_VALUE(0);
		std::auto_ptr<PropertyPage> pSendPage(pPage);
		
		AccountGeneralPage generalPage(pSubAccount, &status);
		CHECK_QSTATUS_VALUE(0);
		AccountUserPage userPage(pSubAccount, &status);
		CHECK_QSTATUS_VALUE(0);
		AccountDialupPage dialupPage(pSubAccount, &status);
		CHECK_QSTATUS_VALUE(0);
		AccountAdvancedPage advancedPage(pSubAccount,
			pSyncFilterManager_, &status);
		CHECK_QSTATUS_VALUE(0);
		PropertySheetBase sheet(hInst, wstrTitle.get(), false, &status);
		CHECK_QSTATUS_VALUE(0);
		status = sheet.add(&generalPage);
		CHECK_QSTATUS_VALUE(0);
		status = sheet.add(&userPage);
		CHECK_QSTATUS_VALUE(0);
		status = sheet.add(pReceivePage.get());
		CHECK_QSTATUS_VALUE(0);
		status = sheet.add(pSendPage.get());
		CHECK_QSTATUS_VALUE(0);
		status = sheet.add(&dialupPage);
		CHECK_QSTATUS_VALUE(0);
		status = sheet.add(&advancedPage);
		CHECK_QSTATUS_VALUE(0);
		
		int nRet = 0;
		status = sheet.doModal(getHandle(), 0, &nRet);
		CHECK_QSTATUS_VALUE(0);
	}
	
	return 0;
}

LRESULT qm::AccountDialog::onAccountSelChanged(NMHDR* pnmhdr, bool* pbHandled)
{
	updateState();
	*pbHandled = true;
	return 0;
}

QSTATUS qm::AccountDialog::update()
{
	DECLARE_QSTATUS();
	
	SubAccount* pCurrentSubAccount = pSubAccount_;
	
	HWND hwnd = getDlgItem(IDC_ACCOUNT);
	
	DisableRedraw disable(hwnd);
	
	TreeView_DeleteAllItems(hwnd);
	
	string_ptr<WSTRING> wstrDefault;
	status = loadString(Application::getApplication().getResourceHandle(),
		IDS_DEFAULTSUBACCOUNT, &wstrDefault);
	CHECK_QSTATUS();
	
	HTREEITEM hItemSelect = 0;
	
	const Document::AccountList& listAccount = pDocument_->getAccounts();
	Document::AccountList::const_iterator itA = listAccount.begin();
	while (itA != listAccount.end()) {
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
		Account::SubAccountList::const_iterator itS = listSubAccount.begin();
		while (itS != listSubAccount.end()) {
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
			
			++itS;
		}
		++itA;
	}
	
	if (hItemSelect) {
		TreeView_SelectItem(hwnd, hItemSelect);
		TreeView_EnsureVisible(hwnd, hItemSelect);
	}
	
	return QSTATUS_SUCCESS;
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
int CALLBACK itemComp(LPARAM lParam1, LPARAM lParam2, LPARAM lParamSort)
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

int CALLBACK selectedItemComp(LPARAM lParam1, LPARAM lParam2, LPARAM lParamSort)
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

qm::AddressBookDialog::AddressBookDialog(AddressBook* pAddressBook,
	Profile* pProfile, const WCHAR* pwszAddress[], QSTATUS* pstatus) :
	DefaultDialog(IDD_ADDRESSBOOK, pstatus),
	pAddressBook_(pAddressBook),
	pProfile_(pProfile),
	nSort_(SORT_NAME | SORT_ASCENDING),
	wstrCategory_(0),
	wstrFilter_(0),
	wndAddressList_(this, pstatus)
{
	DECLARE_QSTATUS();
	
	Type types[] = {
		TYPE_TO,
		TYPE_CC,
		TYPE_BCC
	};
	for (int n = 0; n < countof(listAddress_); ++n) {
		DummyParser field(pwszAddress[n], 0, &status);
		CHECK_QSTATUS_SET(pstatus);
		Part part(&status);
		CHECK_QSTATUS_SET(pstatus);
		status = part.setField(L"Dummy", field);
		CHECK_QSTATUS_SET(pstatus);
		AddressListParser addressList(0, &status);
		CHECK_QSTATUS_SET(pstatus);
		Part::Field f;
		status = part.getField(L"Dummy", &addressList, &f);
		CHECK_QSTATUS_SET(pstatus);
		if (f == Part::FIELD_EXIST) {
			const AddressListParser::AddressList& l = addressList.getAddressList();
			AddressListParser::AddressList::const_iterator it = l.begin();
			while (it != l.end()) {
				string_ptr<WSTRING> wstrValue;
				status = (*it)->getValue(&wstrValue);
				CHECK_QSTATUS_SET(pstatus);
				status = STLWrapper<AddressList>(
					listAddress_[n]).push_back(wstrValue.get());
				CHECK_QSTATUS_SET(pstatus);
				wstrValue.release();
				++it;
			}
		}
	}
}

qm::AddressBookDialog::~AddressBookDialog()
{
	freeWString(wstrCategory_);
	freeWString(wstrFilter_);
	for (int n = 0; n < countof(listAddress_); ++n)
		std::for_each(listAddress_[n].begin(),
			listAddress_[n].end(), string_free<WSTRING>());
}

const AddressBookDialog::AddressList& qm::AddressBookDialog::getAddresses(Type type) const
{
	return listAddress_[type];
}

INT_PTR qm::AddressBookDialog::dialogProc(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	BEGIN_DIALOG_HANDLER()
		HANDLE_SIZE()
	END_DIALOG_HANDLER()
	return DefaultDialog::dialogProc(uMsg, wParam, lParam);
}

LRESULT qm::AddressBookDialog::onCommand(WORD nCode, WORD nId)
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
	if (wstrCategory_)
		pwszCategory = wstrCategory_;
	pProfile_->setString(L"AddressBook", L"Category", pwszCategory);
	
	removeNotifyHandler(this);
	
	pAddressBook_->setEnableReload(true);
	
	return DefaultDialog::onDestroy();
}

LRESULT qm::AddressBookDialog::onInitDialog(HWND hwndFocus, LPARAM lParam)
{
	DECLARE_QSTATUS();
	
	int n = 0;
	
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
	for (n = 0; n < countof(columns); ++n) {
		string_ptr<WSTRING> wstrName;
		status = loadString(hInst, columns[n].nId_, &wstrName);
		CHECK_QSTATUS_VALUE(TRUE);
		W2T(wstrName.get(), ptszName);
		
		int nWidth = 100;
		status = pProfile_->getInt(L"AddressBook",
			columns[n].pwszKey_, columns[n].nWidth_, &nWidth);
		CHECK_QSTATUS_VALUE(TRUE);
		
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
	
	int nColumnWidth = 100;
	status = pProfile_->getInt(L"AddressBook",
		L"SelectedAddressWidth", 150, &nColumnWidth);
	CHECK_QSTATUS_VALUE(TRUE);
	
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
	for (n = 0; n < countof(listAddress_); ++n) {
		AddressList::iterator it = listAddress_[n].begin();
		while (it != listAddress_[n].end()) {
			string_ptr<WSTRING> wstrValue(*it);
			W2T(wstrValue.get(), ptszValue);
			*it = 0;
			
			Item* pItem = 0;
			status = newQsObject(wstrValue.get(), types[n], &pItem);
			CHECK_QSTATUS_VALUE(TRUE);
			
			LVITEM newItem = {
				LVIF_TEXT | LVIF_IMAGE | LVIF_PARAM,
				ListView_GetItemCount(hwndSelected),
				0,
				0,
				0,
				const_cast<LPTSTR>(ptszValue),
				0,
				types[n],
				reinterpret_cast<LPARAM>(pItem)
			};
			ListView_InsertItem(hwndSelected, &newItem);
			
			wstrValue.release();
			
			++it;
		}
		listAddress_[n].clear();
	}
	ListView_SortItems(hwndSelected, &selectedItemComp, 0);
	
	string_ptr<WSTRING> wstrCategory;
	status = pProfile_->getString(L"AddressBook", L"Category", L"", &wstrCategory);
	CHECK_QSTATUS_VALUE(TRUE);
	status = setCurrentCategory(*wstrCategory.get() ? wstrCategory.get() : 0);
	CHECK_QSTATUS_VALUE(TRUE);
	
#ifdef _WIN32_WCE
	RECT rectWorkArea;
	::SystemParametersInfo(SPI_GETWORKAREA, 0, &rectWorkArea, 0);
	int nWidth = rectWorkArea.right - rectWorkArea.left;
	int nHeight = rectWorkArea.bottom - rectWorkArea.top;
	setWindowPos(0, 0, 0, nWidth, nHeight, SWP_NOZORDER | SWP_NOACTIVATE);
#else
	int nWidth = 0;
	status = pProfile_->getInt(L"AddressBook", L"Width", 620, &nWidth);
	CHECK_QSTATUS_VALUE(TRUE);
	int nHeight = 0;
	status = pProfile_->getInt(L"AddressBook", L"Height", 450, &nHeight);
	CHECK_QSTATUS_VALUE(TRUE);
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
	DECLARE_QSTATUS();
	
	HWND hwndSelected = getDlgItem(IDC_SELECTEDADDRESS);
	
	int nCount = ListView_GetItemCount(hwndSelected);
	for (int n = 0; n < nCount; ++n) {
		LVITEM item = {
			LVIF_PARAM,
			n
		};
		ListView_GetItem(hwndSelected, &item);
		
		Item* pItem = reinterpret_cast<Item*>(item.lParam);
		
		string_ptr<WSTRING> wstrValue(pItem->releaseValue());
		status = STLWrapper<AddressList>(
			listAddress_[pItem->getType()]).push_back(wstrValue.get());
		if (status != QSTATUS_SUCCESS)
			break;
		wstrValue.release();
	}
	
	return DefaultDialog::onOk();
}

LRESULT qm::AddressBookDialog::onNotify(NMHDR* pnmhdr, bool* pbHandled)
{
	BEGIN_NOTIFY_HANDLER()
		HANDLE_NOTIFY(LVN_COLUMNCLICK, IDC_ADDRESS, onAddressColumnClick)
		HANDLE_NOTIFY(NM_DBLCLK, IDC_ADDRESS, onAddressDblClk)
	END_NOTIFY_HANDLER()
	return 1;
}

LRESULT qm::AddressBookDialog::onSize(UINT nFlags, int cx, int cy)
{
	layout();
	return 0;
}

LRESULT qm::AddressBookDialog::onCategory()
{
	DECLARE_QSTATUS();
	
	RECT rect;
	Window(getDlgItem(IDC_CATEGORY)).getWindowRect(&rect);
	
	AddressBook::CategoryList listCategory;
	status = pAddressBook_->getCategories(&listCategory);
	if (status == QSTATUS_SUCCESS) {
		std::sort(listCategory.begin(), listCategory.end(), CategoryLess());
		
		AutoMenuHandle hmenu;
		CategoryNameList listName;
		StringListFree<CategoryNameList> free(listName);
		status = createCategoryMenu(listCategory, &hmenu, &listName);
		if (status == QSTATUS_SUCCESS) {
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
	freeWString(wstrFilter_);
	string_ptr<WSTRING> wstrFilter(getDlgItemText(IDC_FILTER));
	wstrFilter_ = tolower(wstrFilter.get());
	update();
	return 0;
}
#endif

LRESULT qm::AddressBookDialog::onAddressColumnClick(NMHDR* pnmhdr, bool* pbHandled)
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

LRESULT qm::AddressBookDialog::onAddressDblClk(NMHDR* pnmhdr, bool* pbHandled)
{
	select(TYPE_TO);
	return 0;
}

QSTATUS qm::AddressBookDialog::update()
{
	DECLARE_QSTATUS();
	
	HWND hwndList = getDlgItem(IDC_ADDRESS);
	
	ListView_DeleteAllItems(hwndList);
	
	const AddressBook::EntryList* pList = 0;
	status = pAddressBook_->getEntries(&pList);
	CHECK_QSTATUS();
	
	size_t nCategoryLen = 0;
	if (wstrCategory_)
		nCategoryLen = wcslen(wstrCategory_);
	
	int n = 0;
	AddressBook::EntryList::const_iterator itE = pList->begin();
	while (itE != pList->end()) {
		AddressBookEntry* pEntry = *itE;
		bool bMatchEntry = isMatchFilter(pEntry);
		W2T(pEntry->getName(), ptszName);
		const AddressBookEntry::AddressList& l = pEntry->getAddresses();
		AddressBookEntry::AddressList::const_iterator itA = l.begin();
		while (itA != l.end()) {
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
			++itA;
		}
		++itE;
	}
	
	ListView_SortItems(hwndList, &itemComp, nSort_);
	
	return QSTATUS_SUCCESS;
}

QSTATUS qm::AddressBookDialog::select(Type type)
{
	DECLARE_QSTATUS();
	
	HWND hwndList = getDlgItem(IDC_ADDRESS);
	HWND hwndSelected = getDlgItem(IDC_SELECTEDADDRESS);
	
	int nItem = -1;
	while (true) {
		nItem = ListView_GetNextItem(hwndList, nItem, LVNI_SELECTED);
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
		
		if (item.state & LVIS_SELECTED) {
			AddressBookAddress* pAddress =
				reinterpret_cast<AddressBookAddress*>(item.lParam);
			string_ptr<WSTRING> wstrValue;
			status = pAddress->getValue(&wstrValue);
			CHECK_QSTATUS();
			W2T(wstrValue.get(), ptszValue);
			
			Item* pItem = 0;
			status = newQsObject(wstrValue.get(), type, &pItem);
			CHECK_QSTATUS();
			
			LVITEM newItem = {
				LVIF_TEXT | LVIF_IMAGE | LVIF_PARAM,
				ListView_GetItemCount(hwndSelected),
				0,
				0,
				0,
				const_cast<LPTSTR>(ptszValue),
				0,
				type,
				reinterpret_cast<LPARAM>(pItem)
			};
			ListView_InsertItem(hwndSelected, &newItem);
			
			wstrValue.release();
		}
	}
	
	ListView_SortItems(hwndSelected, &selectedItemComp, 0);
	
	return QSTATUS_SUCCESS;
}

QSTATUS qm::AddressBookDialog::remove()
{
	DECLARE_QSTATUS();
	
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
	
	return QSTATUS_SUCCESS;
}

QSTATUS qm::AddressBookDialog::layout()
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
	return QSTATUS_SUCCESS;
}

QSTATUS qm::AddressBookDialog::createCategoryMenu(
	const AddressBook::CategoryList& l, HMENU* phmenu, CategoryNameList* pList)
{
	assert(phmenu);
	assert(pList);
	
	DECLARE_QSTATUS();
	
	AutoMenuHandle hmenu(::CreatePopupMenu());
	
	typedef std::vector<std::pair<HMENU, WSTRING> > MenuStack;
	MenuStack stackMenu;
	status = STLWrapper<MenuStack>(stackMenu).push_back(
		MenuStack::value_type(hmenu.get(), 0));
	CHECK_QSTATUS();
	
	struct Deleter
	{
		typedef std::vector<std::pair<HMENU, WSTRING> > MenuStack;
		
		Deleter(MenuStack& s) :
			s_(s)
		{
		}
		
		~Deleter()
		{
			MenuStack::iterator it = s_.begin();
			while (it != s_.end()) {
				freeWString((*it).second);
				++it;
			}
		}
		
		MenuStack& s_;
	} deleter(stackMenu);
	
	string_ptr<WSTRING> wstrThisCategory;
	status = loadString(Application::getApplication().getResourceHandle(),
		IDS_THISCATEGORY, &wstrThisCategory);
	CHECK_QSTATUS();
	W2T(wstrThisCategory.get(), ptszThisCategory);
	
	UINT nId = IDM_ADDRESSBOOK_CATEGORY;
	AddressBook::CategoryList::const_iterator it = l.begin();
	while (it != l.end()) {
		const AddressBookCategory* pCategory = *it;
		
		size_t nLevel = getCategoryLevel(pCategory->getName());
		
		while (true) {
			bool bPop = false;
			if (nLevel < stackMenu.size()) {
				bPop = true;
			}
			else if (stackMenu.size() > 1) {
				string_ptr<WSTRING> wstrName;
				status = getCategoryName(pCategory->getName(),
					stackMenu.size() - 2, false, &wstrName);
				CHECK_QSTATUS();
				if (wcscmp(wstrName.get(), stackMenu.back().second) != 0)
					bPop = true;
			}
			if (!bPop)
				break;
			freeWString(stackMenu.back().second);
			stackMenu.pop_back();
		}
		
		while (nLevel >= stackMenu.size()) {
			string_ptr<WSTRING> wstrName;
			status = getCategoryName(pCategory->getName(),
				stackMenu.size() - 1, false, &wstrName);
			CHECK_QSTATUS();
			
			string_ptr<WSTRING> wstrText;
			status = UIUtil::formatMenu(wstrName.get(), &wstrText);
			CHECK_QSTATUS();
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
			
			string_ptr<WSTRING> wstrFullName;
			status = getCategoryName(pCategory->getName(),
				stackMenu.size() - 1, true, &wstrFullName);
			CHECK_QSTATUS();
			bool bCheck = wstrCategory_ && wcscmp(wstrFullName.get(), wstrCategory_) == 0;
			status = STLWrapper<CategoryNameList>(*pList).push_back(wstrFullName.get());
			CHECK_QSTATUS();
			wstrFullName.release();
			
			unsigned int nFlags = MF_STRING | (bCheck ? MF_CHECKED : 0);
			
			if (bSubMenu) {
				HMENU hSubMenu = ::CreatePopupMenu();
				::AppendMenu(stackMenu.back().first, MF_POPUP,
					reinterpret_cast<UINT_PTR>(hSubMenu), ptszText);
				status = STLWrapper<MenuStack>(stackMenu).push_back(
					std::make_pair(hSubMenu, wstrName.get()));
				CHECK_QSTATUS();
				wstrName.release();
				
				::AppendMenu(hSubMenu, nFlags, nId++, ptszThisCategory);
			}
			else {
				::AppendMenu(stackMenu.back().first, nFlags, nId++, ptszText);
				break;
			}
		}
		
		++it;
	}
	
	::AppendMenu(hmenu.get(), MF_SEPARATOR, -1, 0);
	
	string_ptr<WSTRING> wstrAll;
	status = loadString(Application::getApplication().getResourceHandle(),
		IDS_ALLCATEGORY, &wstrAll);
	CHECK_QSTATUS();
	W2T(wstrAll.get(), ptszAll);
	::AppendMenu(hmenu.get(), MF_STRING | (!wstrCategory_ ? MF_CHECKED : 0),
		IDM_ADDRESSBOOK_ALLCATEGORY, ptszAll);
	
	*phmenu = hmenu.release();
	
	return QSTATUS_SUCCESS;
}

QSTATUS qm::AddressBookDialog::setCurrentCategory(const WCHAR* pwszCategory)
{
	DECLARE_QSTATUS();
	
	string_ptr<WSTRING> wstrCategory;
	if (pwszCategory) {
		wstrCategory.reset(allocWString(pwszCategory));
		if (!wstrCategory.get())
			return QSTATUS_OUTOFMEMORY;
	}
	
	freeWString(wstrCategory_);
	wstrCategory_ = wstrCategory.release();
	
	HINSTANCE hInst = Application::getApplication().getResourceHandle();
	string_ptr<WSTRING> wstrTitle;
	status = loadString(hInst, IDS_CATEGORY, &wstrTitle);
	CHECK_QSTATUS();
	string_ptr<WSTRING> wstrAll;
	status = loadString(hInst, IDS_CATEGORYALL, &wstrAll);
	CHECK_QSTATUS();
	
	ConcatW c[] = {
		{ wstrTitle.get(),									-1	},
		{ L" (",											2	},
		{ wstrCategory_ ? wstrCategory_ : wstrAll.get(),	-1	},
		{ L")",												1	}
	};
	string_ptr<WSTRING> wstrButton(concat(c, countof(c)));
	if (!wstrButton.get())
		return QSTATUS_OUTOFMEMORY;
	setDlgItemText(IDC_CATEGORY, wstrButton.get());
	
	status = update();
	CHECK_QSTATUS();
	
	return QSTATUS_SUCCESS;
}

bool qm::AddressBookDialog::isCategory(
	const AddressBookAddress::CategoryList& listCategory) const
{
	if (!wstrCategory_)
		return true;
	
	size_t nLen = wcslen(wstrCategory_);
	
	AddressBookAddress::CategoryList::const_iterator it = listCategory.begin();
	while (it != listCategory.end()) {
		const AddressBookCategory* pCategory = *it;
		const WCHAR* pwszCategory = pCategory->getName();
		
		if (wcscmp(pwszCategory, wstrCategory_) == 0)
			return true;
		else if (wcslen(pwszCategory) > nLen &&
			wcsncmp(pwszCategory, wstrCategory_, nLen) == 0 &&
			*(pwszCategory + nLen) == L'/')
			return true;
		
		++it;
	}
	
	return false;
}

bool qm::AddressBookDialog::isMatchFilter(const AddressBookEntry* pEntry) const
{
	if (!wstrFilter_)
		return true;
	
	string_ptr<WSTRING> wstrName(tolower(pEntry->getName()));
	return wcsstr(wstrName.get(), wstrFilter_) != 0;
}

bool qm::AddressBookDialog::isMatchFilter(const AddressBookAddress* pAddress) const
{
	if (!wstrFilter_)
		return true;
	return wcsstr(pAddress->getAddress(), wstrFilter_) != 0;
}

size_t qm::AddressBookDialog::getCategoryLevel(const WCHAR* pwszCategory)
{
	assert(pwszCategory);
	return std::count(pwszCategory, pwszCategory + wcslen(pwszCategory), L'/') + 1;
}

QSTATUS qm::AddressBookDialog::getCategoryName(const WCHAR* pwszCategory,
	size_t nLevel, bool bFull, WSTRING* pwstrName)
{
	assert(pwszCategory);
	assert(pwstrName);
	
	DECLARE_QSTATUS();
	
	const WCHAR* p = pwszCategory;
	while (nLevel != 0) {
		while (*p++ != L'/')
			;
		--nLevel;
	}
	
	const WCHAR* pEnd = wcschr(p, L'/');
	
	string_ptr<WSTRING> wstrName;
	if (bFull) {
		size_t nLen = pEnd ? pEnd - pwszCategory : wcslen(pwszCategory);
		wstrName.reset(allocWString(pwszCategory, nLen));
	}
	else {
		size_t nLen = pEnd ? pEnd - p : wcslen(p);
		wstrName.reset(allocWString(p, nLen));
	}
	if (!wstrName.get())
		return QSTATUS_OUTOFMEMORY;
	
	*pwstrName = wstrName.release();
	
	return QSTATUS_SUCCESS;
}


/****************************************************************************
 *
 * AddressBookDialog::Item
 *
 */

qm::AddressBookDialog::Item::Item(qs::WSTRING wstrValue,
	Type type, QSTATUS* pstatus) :
	wstrValue_(wstrValue),
	type_(type)
{
}

qm::AddressBookDialog::Item::~Item()
{
	freeWString(wstrValue_);
}

const WCHAR* qm::AddressBookDialog::Item::getValue() const
{
	return wstrValue_;
}

WSTRING qm::AddressBookDialog::Item::releaseValue()
{
	WSTRING wstr = wstrValue_;
	wstrValue_ = 0;
	return wstr;
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

qm::AddressBookDialog::AddressListWindow::AddressListWindow(
	AddressBookDialog* pDialog, QSTATUS* pstatus) :
	WindowBase(false, pstatus),
	DefaultWindowHandler(pstatus),
	pDialog_(pDialog)
{
	setWindowHandler(this, false);
}

qm::AddressBookDialog::AddressListWindow::~AddressListWindow()
{
}

LRESULT qm::AddressBookDialog::AddressListWindow::windowProc(
	UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	BEGIN_MESSAGE_HANDLER()
		HANDLE_CHAR()
	END_MESSAGE_HANDLER()
	return DefaultWindowHandler::windowProc(uMsg, wParam, lParam);
}

LRESULT qm::AddressBookDialog::AddressListWindow::onChar(
	UINT nChar, UINT nRepeat, UINT nFlags)
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

bool qm::AddressBookDialog::CategoryLess::operator()(
	const AddressBookCategory* pLhs, const AddressBookCategory* pRhs)
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

qm::AttachmentDialog::AttachmentDialog(
	EditMessage::AttachmentList& listAttachment, QSTATUS* pstatus) :
	DefaultDialog(IDD_ATTACHMENT, pstatus),
	listAttachment_(listAttachment)
{
	std::sort(listAttachment_.begin(), listAttachment_.end(),
		EditMessage::AttachmentComp());
}

qm::AttachmentDialog::~AttachmentDialog()
{
}

LRESULT qm::AttachmentDialog::onCommand(WORD nCode, WORD nId)
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

LRESULT qm::AttachmentDialog::onInitDialog(HWND hwndFocus, LPARAM lParam)
{
	DECLARE_QSTATUS();
	
	init(false);
	
	HWND hwndList = getDlgItem(IDC_ATTACHMENT);
	
	SHFILEINFO info = { 0 };
	HIMAGELIST hImageList = reinterpret_cast<HIMAGELIST>(::SHGetFileInfo(
		_T("dummy.txt"), FILE_ATTRIBUTE_NORMAL, &info, sizeof(info),
		SHGFI_USEFILEATTRIBUTES | SHGFI_SYSICONINDEX | SHGFI_SMALLICON));
	ListView_SetImageList(hwndList, hImageList, LVSIL_SMALL);
	
	status = update();
	CHECK_QSTATUS_VALUE(TRUE);
	
	updateState();
	
	addNotifyHandler(this);
	
	return TRUE;
}

LRESULT qm::AttachmentDialog::onNotify(NMHDR* pnmhdr, bool* pbHandled)
{
	BEGIN_NOTIFY_HANDLER()
		HANDLE_NOTIFY(LVN_ITEMCHANGED, IDC_ATTACHMENT, onAttachmentItemChanged);
	END_NOTIFY_HANDLER()
	return 1;
}

LRESULT qm::AttachmentDialog::onAdd()
{
	DECLARE_QSTATUS();
	
	string_ptr<WSTRING> wstrFilter;
	status = loadString(Application::getApplication().getResourceHandle(),
		IDS_FILTER_ATTACHMENT, &wstrFilter);
	CHECK_QSTATUS();
	
	FileDialog dialog(true, wstrFilter.get(), 0, 0, 0,
		OFN_EXPLORER | OFN_HIDEREADONLY | OFN_LONGNAMES | OFN_ALLOWMULTISELECT,
		&status);
	CHECK_QSTATUS_VALUE(0);
	
	int nRet = IDCANCEL;
	status = dialog.doModal(getHandle(), 0, &nRet);
	CHECK_QSTATUS_VALUE(0);
	if (nRet == IDOK) {
		const WCHAR* pwszPath = dialog.getPath();
		const WCHAR* p = pwszPath;
		while (*p) {
			string_ptr<WSTRING> wstrName(allocWString(p));
			if (!wstrName.get())
				return 0;
			EditMessage::Attachment attachment = {
				wstrName.get(),
				true
			};
			status = STLWrapper<EditMessage::AttachmentList>(
				listAttachment_).push_back(attachment);
			CHECK_QSTATUS_VALUE(0);
			wstrName.release();
			
			p += wcslen(p) + 1;
		}
		
		std::sort(listAttachment_.begin(), listAttachment_.end(),
			EditMessage::AttachmentComp());
		
		status = update();
		CHECK_QSTATUS_VALUE(0);
	}
	
	updateState();
	
	return 0;
}

LRESULT qm::AttachmentDialog::onRemove()
{
	DECLARE_QSTATUS();
	
	HWND hwndList = getDlgItem(IDC_ATTACHMENT);
	
	int nDeleted = 0;
	int nItem = -1;
	while (true) {
		nItem = ListView_GetNextItem(hwndList, nItem, LVNI_SELECTED);
		if (nItem == -1)
			break;
		EditMessage::AttachmentList::iterator it =
			listAttachment_.begin() + nItem - nDeleted;
		freeWString((*it).wstrName_);
		listAttachment_.erase(it);
		++nDeleted;
	}
	
	status = update();
	CHECK_QSTATUS_VALUE(0);
	
	updateState();
	
	return 0;
}

LRESULT qm::AttachmentDialog::onAttachmentItemChanged(NMHDR* pnmhdr, bool* pbHandled)
{
	updateState();
	*pbHandled = true;
	return 0;
}

QSTATUS qm::AttachmentDialog::update()
{
	DECLARE_QSTATUS();
	
	HWND hwndList = getDlgItem(IDC_ATTACHMENT);
	
	ListView_DeleteAllItems(hwndList);
	
	EditMessage::AttachmentList::size_type n = 0;
	while (n < listAttachment_.size()) {
		EditMessage::Attachment& attachment = listAttachment_[n];
		const WCHAR* pwszName = wcsrchr(attachment.wstrName_, L'\\');
		pwszName = pwszName ? pwszName + 1 : attachment.wstrName_;
		W2T(pwszName, ptszName);
		
		SHFILEINFO info = { 0 };
		::SHGetFileInfo(ptszName, FILE_ATTRIBUTE_NORMAL, &info, sizeof(info),
			SHGFI_USEFILEATTRIBUTES | SHGFI_SYSICONINDEX | SHGFI_SMALLICON);
		
		string_ptr<TSTRING> tstrName;
		if (!attachment.bNew_) {
			tstrName.reset(concat(_T("<"), ptszName, _T(">")));
			if (!tstrName.get())
				return QSTATUS_OUTOFMEMORY;
			ptszName = tstrName.get();
		}
		
		LVITEM item = {
			LVIF_TEXT | LVIF_IMAGE,
			n,
			0,
			0,
			0,
			const_cast<LPTSTR>(ptszName),
			0,
			info.iIcon
		};
		ListView_InsertItem(hwndList, &item);
		
		++n;
	}
	
	return QSTATUS_SUCCESS;
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

qm::CreateAccountDialog::CreateAccountDialog(
	Profile* pProfile, QSTATUS* pstatus) :
	DefaultDialog(IDD_CREATEACCOUNT, pstatus),
	pProfile_(pProfile),
	wstrName_(0),
	wstrClass_(0),
	nReceiveProtocol_(0),
	nSendProtocol_(0),
	nBlockSize_(-1),
	nCacheBlockSize_(-1)
{
	wstrClass_ = allocWString(L"mail");
	if (!wstrClass_) {
		*pstatus = QSTATUS_OUTOFMEMORY;
		return;
	}
}

qm::CreateAccountDialog::~CreateAccountDialog()
{
	freeWString(wstrName_);
	freeWString(wstrClass_);
	clearProtocols();
}

const WCHAR* qm::CreateAccountDialog::getName() const
{
	return wstrName_;
}

const WCHAR* qm::CreateAccountDialog::getClass() const
{
	return wstrClass_;
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

LRESULT qm::CreateAccountDialog::onCommand(WORD nCode, WORD nId)
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

LRESULT qm::CreateAccountDialog::onInitDialog(HWND hwndFocus, LPARAM lParam)
{
	DECLARE_QSTATUS();
	
	init(false);
	
	string_ptr<WSTRING> wstrClasses;
	status = pProfile_->getString(L"Global", L"Classes", L"mail news", &wstrClasses);
	CHECK_QSTATUS_VALUE(TRUE);
	const WCHAR* p = wcstok(wstrClasses.get(), L" ");
	while (p) {
		W2T(p, ptsz);
		sendDlgItemMessage(IDC_CLASS, CB_ADDSTRING,
			0, reinterpret_cast<LPARAM>(ptsz));
		p = wcstok(0, L" ");
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
	DECLARE_QSTATUS();
	
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
	string_ptr<TSTRING> tstrClass(allocTString(nLen + 1));
	if (!tstrClass.get())
		return 0;
	sendDlgItemMessage(IDC_CLASS, CB_GETLBTEXT,
		nItem, reinterpret_cast<LPARAM>(tstrClass.get()));
	
	string_ptr<WSTRING> wstrClass(tcs2wcs(tstrClass.get()));
	if (!wstrClass.get())
		return 0;
	if (wcscmp(wstrClass_, wstrClass.get()) != 0) {
		freeWString(wstrClass_);
		wstrClass_ = wstrClass.release();
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

QSTATUS qm::CreateAccountDialog::updateProtocols()
{
	DECLARE_QSTATUS();
	
	clearProtocols();
	sendDlgItemMessage(IDC_INCOMINGPROTOCOL, CB_RESETCONTENT);
	sendDlgItemMessage(IDC_OUTGOINGPROTOCOL, CB_RESETCONTENT);
	
	ReceiveSessionFactory::NameList listReceiveName;
	StringListFree<ReceiveSessionFactory::NameList> freeReceive(listReceiveName);
	status = ReceiveSessionFactory::getNames(&listReceiveName);
	CHECK_QSTATUS();
	status = STLWrapper<ProtocolList>(listReceiveProtocol_).reserve(listReceiveName.size());
	CHECK_QSTATUS();
	ReceiveSessionFactory::NameList::iterator itR = listReceiveName.begin();
	while (itR != listReceiveName.end()) {
		string_ptr<WSTRING> wstrName(*itR);
		*itR = 0;
		std::auto_ptr<ReceiveSessionUI> pUI;
		status = ReceiveSessionFactory::getUI(wstrName.get(), &pUI);
		CHECK_QSTATUS();
		if (wcscmp(pUI->getClass(), wstrClass_) == 0) {
			Protocol p = {
				wstrName.get(),
				pUI->getDefaultPort()
			};
			listReceiveProtocol_.push_back(p);
			wstrName.release();
			
			string_ptr<WSTRING> wstrDisplayName;
			status = pUI->getDisplayName(&wstrDisplayName);
			CHECK_QSTATUS();
			W2T(wstrDisplayName.get(), ptszDisplayName);
			sendDlgItemMessage(IDC_INCOMINGPROTOCOL, CB_ADDSTRING,
				0, reinterpret_cast<LPARAM>(ptszDisplayName));
		}
		++itR;
	}
	sendDlgItemMessage(IDC_INCOMINGPROTOCOL, CB_SETCURSEL, 0);
	
	SendSessionFactory::NameList listSendName;
	StringListFree<SendSessionFactory::NameList> freeSend(listSendName);
	status = SendSessionFactory::getNames(&listSendName);
	CHECK_QSTATUS();
	status = STLWrapper<ProtocolList>(listSendProtocol_).reserve(listSendName.size());
	CHECK_QSTATUS();
	SendSessionFactory::NameList::iterator itS = listSendName.begin();
	while (itS != listSendName.end()) {
		string_ptr<WSTRING> wstrName(*itS);
		*itS = 0;
		std::auto_ptr<SendSessionUI> pUI;
		status = SendSessionFactory::getUI(wstrName.get(), &pUI);
		CHECK_QSTATUS();
		if (wcscmp(pUI->getClass(), wstrClass_) == 0) {
			Protocol p = {
				wstrName.get(),
				pUI->getDefaultPort()
			};
			listSendProtocol_.push_back(p);
			wstrName.release();
			
			string_ptr<WSTRING> wstrDisplayName;
			status = pUI->getDisplayName(&wstrDisplayName);
			CHECK_QSTATUS();
			W2T(wstrDisplayName.get(), ptszDisplayName);
			sendDlgItemMessage(IDC_OUTGOINGPROTOCOL, CB_ADDSTRING,
				0, reinterpret_cast<LPARAM>(ptszDisplayName));
		}
		++itS;
	}
	sendDlgItemMessage(IDC_OUTGOINGPROTOCOL, CB_SETCURSEL, 0);
	
	updateState();
	
	return QSTATUS_SUCCESS;
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

qm::CreateFolderDialog::CreateFolderDialog(
	Type type, bool bAllowRemote, QSTATUS* pstatus) :
	DefaultDialog(IDD_CREATEFOLDER, pstatus),
	type_(type),
	bAllowRemote_(bAllowRemote),
	wstrName_(0)
{
}

qm::CreateFolderDialog::~CreateFolderDialog()
{
	freeWString(wstrName_);
}

CreateFolderDialog::Type qm::CreateFolderDialog::getType() const
{
	return type_;
}

const WCHAR* qm::CreateFolderDialog::getName() const
{
	return wstrName_;
}

LRESULT qm::CreateFolderDialog::onCommand(WORD nCode, WORD nId)
{
	BEGIN_COMMAND_HANDLER()
		HANDLE_COMMAND_ID_CODE(IDC_NAME, EN_CHANGE, onNameChange)
		HANDLE_COMMAND_ID_RANGE_CODE(IDC_LOCALFOLDER,
			IDC_QUERYFOLDER, BN_CLICKED, onTypeChange)
	END_COMMAND_HANDLER()
	return DefaultDialog::onCommand(nCode, nId);
}

LRESULT qm::CreateFolderDialog::onInitDialog(HWND hwndFocus, LPARAM lParam)
{
	init(false);
	
	Window(getDlgItem(IDC_REMOTEFOLDER)).enableWindow(bAllowRemote_);
	
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
}


/****************************************************************************
 *
 * CreateSubAccountDialog
 *
 */

qm::CreateSubAccountDialog::CreateSubAccountDialog(
	Document* pDocument, QSTATUS* pstatus) :
	DefaultDialog(IDD_CREATESUBACCOUNT, pstatus),
	pDocument_(pDocument),
	wstrName_(0)
{
}

qm::CreateSubAccountDialog::~CreateSubAccountDialog()
{
	freeWString(wstrName_);
}

const WCHAR* qm::CreateSubAccountDialog::getName() const
{
	return wstrName_;
}

LRESULT qm::CreateSubAccountDialog::onCommand(WORD nCode, WORD nId)
{
	BEGIN_COMMAND_HANDLER()
		HANDLE_COMMAND_ID_CODE(IDC_NAME, CBN_EDITCHANGE, onNameChanged)
	END_COMMAND_HANDLER()
	return DefaultDialog::onCommand(nCode, nId);
}

LRESULT qm::CreateSubAccountDialog::onInitDialog(HWND hwndFocus, LPARAM lParam)
{
	DECLARE_QSTATUS();
	
	init(false);
	
	const Document::AccountList& listAccount = pDocument_->getAccounts();
	Document::AccountList::const_iterator itA = listAccount.begin();
	while (itA != listAccount.end()) {
		Account* pAccount = *itA;
		
		const Account::SubAccountList& listSubAccount = pAccount->getSubAccounts();
		Account::SubAccountList::const_iterator itS = listSubAccount.begin();
		while (itS != listSubAccount.end()) {
			SubAccount* pSubAccount = *itS;
			
			const WCHAR* pwszName = pSubAccount->getName();
			if (*pwszName) {
				W2T_STATUS(pwszName, ptszName);
				CHECK_QSTATUS_VALUE(0);
				sendDlgItemMessage(IDC_NAME, CB_ADDSTRING, 0,
					reinterpret_cast<LPARAM>(ptszName));
			}
			
			++itS;
		}
		
		++itA;
	}
	
	updateState();
	
	return TRUE;
}

LRESULT qm::CreateSubAccountDialog::onOk()
{
	wstrName_ = Window(getDlgItem(IDC_NAME)).getWindowText();
	if (!wstrName_)
		return 0;
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

qm::CustomFilterDialog::CustomFilterDialog(
	const WCHAR* pwszMacro, QSTATUS* pstatus) :
	DefaultDialog(IDD_CUSTOMFILTER, pstatus),
	wstrMacro_(0)
{
	if (pwszMacro) {
		wstrMacro_ = allocWString(pwszMacro);
		if (!wstrMacro_) {
			*pstatus = QSTATUS_OUTOFMEMORY;
			return;
		}
	}
}

qm::CustomFilterDialog::~CustomFilterDialog()
{
	freeWString(wstrMacro_);
}

const WCHAR* qm::CustomFilterDialog::getMacro() const
{
	return wstrMacro_;
}

LRESULT qm::CustomFilterDialog::onInitDialog(HWND hwndFocus, LPARAM lParam)
{
	init(false);
	
	if (wstrMacro_) {
		setDlgItemText(IDC_MACRO, wstrMacro_);
		freeWString(wstrMacro_);
		wstrMacro_ = 0;
	}
	
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
	List& list, QSTATUS* pstatus) :
	DefaultDialog(IDD_DETACH, pstatus),
	pProfile_(pProfile),
	list_(list),
	wstrFolder_(0)
{
	DECLARE_QSTATUS();
	
	status = pProfile_->getString(L"Global",
		L"DetachFolder", L"", &wstrFolder_);
	CHECK_QSTATUS_SET(pstatus);
}

qm::DetachDialog::~DetachDialog()
{
	freeWString(wstrFolder_);
}

const WCHAR* qm::DetachDialog::getFolder() const
{
	return wstrFolder_;
}

LRESULT qm::DetachDialog::onCommand(WORD nCode, WORD nId)
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

LRESULT qm::DetachDialog::onInitDialog(HWND hwndFocus, LPARAM lParam)
{
	DECLARE_QSTATUS();
	
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
	
	setDlgItemText(IDC_FOLDER, wstrFolder_);
	updateState();
	
	addNotifyHandler(this);
	
	return TRUE;
}

LRESULT qm::DetachDialog::onOk()
{
	DECLARE_QSTATUS();
	
	HWND hwndList = getDlgItem(IDC_ATTACHMENT);
	for (List::size_type n = 0; n < list_.size(); ++n) {
		if (ListView_GetCheckState(hwndList, n)) {
			TCHAR tszName[MAX_PATH];
			ListView_GetItemText(hwndList, n, 0, tszName, countof(tszName));
			string_ptr<WSTRING> wstrName(tcs2wcs(tszName));
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
	
	string_ptr<WSTRING> wstrFolder(getDlgItemText(IDC_FOLDER));
	if (wstrFolder.get()) {
		pProfile_->setString(L"Global", L"DetachFolder", wstrFolder.get());
		freeWString(wstrFolder_);
		wstrFolder_ = wstrFolder.release();
	}
	
	return DefaultDialog::onOk();
}

LRESULT qm::DetachDialog::onNotify(NMHDR* pnmhdr, bool* pbHandled)
{
	BEGIN_NOTIFY_HANDLER()
		HANDLE_NOTIFY(LVN_ITEMCHANGED, IDC_ATTACHMENT, onAttachmentItemChanged)
	END_NOTIFY_HANDLER()
	return 1;
}

LRESULT qm::DetachDialog::onBrowse()
{
	DECLARE_QSTATUS();
	
	string_ptr<WSTRING> wstrFolder(getDlgItemText(IDC_FOLDER));
	
	string_ptr<WSTRING> wstrPath;
	status = qs::UIUtil::browseFolder(getHandle(),
		0, wstrFolder.get(), &wstrPath);
	CHECK_QSTATUS_VALUE(0);
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

LRESULT qm::DetachDialog::onAttachmentItemChanged(NMHDR* pnmhdr, bool* pbHandled)
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

qm::DialupDialog::DialupDialog(const WCHAR* pwszEntry, const WCHAR* pwszUserName,
	const WCHAR* pwszPassword, const WCHAR* pwszDomain,QSTATUS* pstatus) :
	DefaultDialog(IDD_DIALUP, pstatus),
	wstrEntry_(0),
	wstrUserName_(0),
	wstrPassword_(0),
	wstrDomain_(0)
{
	const WCHAR* pwsz[] = {
		pwszEntry,
		pwszUserName,
		pwszPassword,
		pwszDomain
	};
	string_ptr<WSTRING> wstr[4];
	for (int n = 0; n < countof(pwsz); ++n) {
		wstr[n].reset(allocWString(pwsz[n]));
		if (!wstr[n].get()) {
			*pstatus = QSTATUS_OUTOFMEMORY;
			return;
		}
	}
	
	wstrEntry_ = wstr[0].release();
	wstrUserName_ = wstr[1].release();
	wstrPassword_ = wstr[2].release();
	wstrDomain_ = wstr[3].release();
}

qm::DialupDialog::~DialupDialog()
{
	freeWString(wstrEntry_);
	freeWString(wstrUserName_);
	freeWString(wstrPassword_);
	freeWString(wstrDomain_);
}

const WCHAR* qm::DialupDialog::getUserName() const
{
	return wstrUserName_;
}

const WCHAR* qm::DialupDialog::getPassword() const
{
	return wstrPassword_;
}

const WCHAR* qm::DialupDialog::getDomain() const
{
	return wstrDomain_;
}

LRESULT qm::DialupDialog::onCommand(WORD nCode, WORD nId)
{
	BEGIN_COMMAND_HANDLER()
		HANDLE_COMMAND_ID(IDC_DIALPROPERTY, onDialProperty)
	END_COMMAND_HANDLER()
	return DefaultDialog::onCommand(nCode, nId);
}

LRESULT qm::DialupDialog::onInitDialog(HWND hwndFocus, LPARAM lParam)
{
	init(false);
	
	struct {
		UINT nId_;
		const WCHAR* pwsz_;
	} items[] = {
		{ IDC_ENTRY,	wstrEntry_		},
		{ IDC_USERNAME,	wstrUserName_	},
		{ IDC_PASSWORD,	wstrPassword_	},
		{ IDC_DOMAIN,	wstrDomain_		}
	};
	for (int n = 0; n < countof(items); ++n)
		setDlgItemText(items[n].nId_, items[n].pwsz_);
	
	updateLocation();
	
	if (!*wstrPassword_) {
		Window(getDlgItem(IDC_PASSWORD)).setFocus();
		return FALSE;
	}
	
	return TRUE;
}

LRESULT qm::DialupDialog::onOk()
{
	struct {
		UINT nId_;
		WSTRING* pwstr_;
	} items[] = {
		{ IDC_ENTRY,	&wstrEntry_		},
		{ IDC_USERNAME,	&wstrUserName_	},
		{ IDC_PASSWORD,	&wstrPassword_	},
		{ IDC_DOMAIN,	&wstrDomain_	}
	};
	for (int n = 0; n < countof(items); ++n) {
		string_ptr<WSTRING> wstr(getDlgItemText(items[n].nId_));
		if (wstr.get()) {
			freeWString(*items[n].pwstr_);
			*items[n].pwstr_ = wstr.release();
		}
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
	DECLARE_QSTATUS();
	
	string_ptr<WSTRING> wstrLocation;
	status = RasConnection::getLocation(&wstrLocation);
	if (status == QSTATUS_SUCCESS)
		setDlgItemText(IDC_DIALFROM, wstrLocation.get());
}


/****************************************************************************
 *
 * ExportDialog
 *
 */

qm::ExportDialog::ExportDialog(bool bSingleMessage, QSTATUS* pstatus) :
	DefaultDialog(IDD_EXPORT, pstatus),
	bSingleMessage_(bSingleMessage),
	wstrPath_(0),
	nFlags_(0)
{
}

qm::ExportDialog::~ExportDialog()
{
	freeWString(wstrPath_);
}

const WCHAR* qm::ExportDialog::getPath() const
{
	return wstrPath_;
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
	// TODO
	return 0;
}

const WCHAR* qm::ExportDialog::getEncoding() const
{
	// TODO
	return 0;
}

LRESULT qm::ExportDialog::onCommand(WORD nCode, WORD nId)
{
	BEGIN_COMMAND_HANDLER()
		HANDLE_COMMAND_ID(IDC_BROWSE, onBrowse)
		HANDLE_COMMAND_ID_CODE(IDC_PATH, EN_CHANGE, onPathChange)
	END_COMMAND_HANDLER()
	return DefaultDialog::onCommand(nCode, nId);
}

LRESULT qm::ExportDialog::onInitDialog(HWND hwndFocus, LPARAM lParam)
{
	init(false);
	
	if (bSingleMessage_)
		Window(getDlgItem(IDC_FILEPERMESSAGE)).enableWindow(false);
	
	updateState();
	
	return TRUE;
}

LRESULT qm::ExportDialog::onOk()
{
	string_ptr<WSTRING> wstrPath(getDlgItemText(IDC_PATH));
	if (!wstrPath.get())
		return 0;
	
	freeWString(wstrPath_);
	wstrPath_ = wstrPath.release();
	
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
	
	return DefaultDialog::onOk();
}

LRESULT qm::ExportDialog::onBrowse()
{
	DECLARE_QSTATUS();
	
	string_ptr<WSTRING> wstrFilter;
	status = loadString(Application::getApplication().getResourceHandle(),
		IDS_FILTER_EXPORT, &wstrFilter);
	CHECK_QSTATUS_VALUE(0);
	
	FileDialog dialog(false, wstrFilter.get(), 0, 0, 0,
		OFN_EXPLORER | OFN_HIDEREADONLY | OFN_LONGNAMES | OFN_OVERWRITEPROMPT,
		&status);
	CHECK_QSTATUS_VALUE(0);
	
	int nRet = IDCANCEL;
	status = dialog.doModal(getHandle(), 0, &nRet);
	CHECK_QSTATUS_VALUE(0);
	if (nRet == IDOK) {
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

void qm::ExportDialog::updateState()
{
	bool bEnable = sendDlgItemMessage(IDC_PATH, WM_GETTEXTLENGTH) != 0;
	Window(getDlgItem(IDOK)).enableWindow(bEnable);
}


/****************************************************************************
 *
 * FindDialog
 *
 */

qm::FindDialog::FindDialog(Profile* pProfile, QSTATUS* pstatus) :
	DefaultDialog(IDD_FIND, pstatus),
	pProfile_(pProfile),
	wstrFind_(0),
	bMatchCase_(0),
	bPrev_(false)
{
}

qm::FindDialog::~FindDialog()
{
	freeWString(wstrFind_);
}

const WCHAR* qm::FindDialog::getFind() const
{
	return wstrFind_;
}

bool qm::FindDialog::isMatchCase() const
{
	return bMatchCase_;
}

bool qm::FindDialog::isPrev() const
{
	return bPrev_;
}

LRESULT qm::FindDialog::onCommand(WORD nCode, WORD nId)
{
	BEGIN_COMMAND_HANDLER()
		HANDLE_COMMAND_ID_RANGE(IDC_FINDNEXT, IDC_FINDPREV, onFind)
	END_COMMAND_HANDLER()
	return DefaultDialog::onCommand(nCode, nId);
}

LRESULT qm::FindDialog::onInitDialog(HWND hwndFocus, LPARAM lParam)
{
	DECLARE_QSTATUS();
	
	init(false);
	
	for (int n = 0; n < HISTORY_SIZE; ++n) {
		WCHAR wszKey[32];
		swprintf(wszKey, L"History%d", n);
		string_ptr<WSTRING> wstr;
		status = pProfile_->getString(L"Find", wszKey, L"", &wstr);
		CHECK_QSTATUS();
		if (*wstr.get()) {
			W2T(wstr.get(), ptsz);
			sendDlgItemMessage(IDC_FIND, CB_ADDSTRING, 0,
				reinterpret_cast<LPARAM>(ptsz));
		}
	}
	
	int nMatchCase = 0;
	status = pProfile_->getInt(L"Find", L"MatchCase", 0, &nMatchCase);
	CHECK_QSTATUS();
	sendDlgItemMessage(IDC_MATCHCASE, BM_SETCHECK,
		nMatchCase ? BST_CHECKED : BST_UNCHECKED);
	
	return TRUE;
}

LRESULT qm::FindDialog::onFind(UINT nId)
{
	DECLARE_QSTATUS();
	
	Window edit(Window(getDlgItem(IDC_FIND)).getWindow(GW_CHILD));
	wstrFind_ = edit.getWindowText();
	if (wstrFind_) {
		for (int n = HISTORY_SIZE - 1; n > 0; --n) {
			WCHAR wszFromKey[32];
			swprintf(wszFromKey, L"History%d", n - 1);
			string_ptr<WSTRING> wstr;
			status = pProfile_->getString(L"Find", wszFromKey, L"", &wstr);
			CHECK_QSTATUS();
			
			WCHAR wszToKey[32];
			swprintf(wszToKey, L"History%d", n);
			status = pProfile_->setString(L"Find", wszToKey, wstr.get());
			CHECK_QSTATUS();
		}
		
		status = pProfile_->setString(L"Find", L"History0", wstrFind_);
		CHECK_QSTATUS();
	}
	
	
	bMatchCase_ = sendDlgItemMessage(IDC_MATCHCASE, BM_GETCHECK) == BST_CHECKED;
	status = pProfile_->setInt(L"Find", L"MatchCase", bMatchCase_ ? 1 : 0);
	CHECK_QSTATUS();
	
	bPrev_ = nId == IDC_FINDPREV;
	
	endDialog(IDOK);
	
	return 0;
}


/****************************************************************************
 *
 * ImportDialog
 *
 */

qm::ImportDialog::ImportDialog(QSTATUS* pstatus) :
	DefaultDialog(IDD_IMPORT, pstatus),
	wstrPath_(0),
	bMultiple_(false),
	nFlags_(0)
{
}

qm::ImportDialog::~ImportDialog()
{
	freeWString(wstrPath_);
}

const WCHAR* qm::ImportDialog::getPath() const
{
	return wstrPath_;
}

bool qm::ImportDialog::isMultiple() const
{
	return bMultiple_;
}

unsigned int qm::ImportDialog::getFlags() const
{
	return nFlags_;
}

LRESULT qm::ImportDialog::onCommand(WORD nCode, WORD nId)
{
	BEGIN_COMMAND_HANDLER()
		HANDLE_COMMAND_ID(IDC_BROWSE, onBrowse)
		HANDLE_COMMAND_ID_CODE(IDC_PATH, EN_CHANGE, onPathChange)
	END_COMMAND_HANDLER()
	return DefaultDialog::onCommand(nCode, nId);
}

LRESULT qm::ImportDialog::onInitDialog(HWND hwndFocus, LPARAM lParam)
{
	init(false);
	sendDlgItemMessage(IDC_NORMAL, BM_SETCHECK, BST_CHECKED);
	updateState();
	
	return TRUE;
}

LRESULT qm::ImportDialog::onOk()
{
	string_ptr<WSTRING> wstrPath(getDlgItemText(IDC_PATH));
	if (!wstrPath.get())
		return 0;
	
	freeWString(wstrPath_);
	wstrPath_ = wstrPath.release();
	
	bMultiple_ = sendDlgItemMessage(IDC_MULTIMESSAGES, BM_GETCHECK) == BST_CHECKED;
	
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
	DECLARE_QSTATUS();
	
	string_ptr<WSTRING> wstrFilter;
	status = loadString(Application::getApplication().getResourceHandle(),
		IDS_FILTER_IMPORT, &wstrFilter);
	CHECK_QSTATUS_VALUE(0);
	
	FileDialog dialog(true, wstrFilter.get(), 0, 0, 0,
		OFN_EXPLORER | OFN_HIDEREADONLY | OFN_LONGNAMES | OFN_ALLOWMULTISELECT,
		&status);
	CHECK_QSTATUS_VALUE(0);
	
	int nRet = IDCANCEL;
	status = dialog.doModal(getHandle(), 0, &nRet);
	CHECK_QSTATUS_VALUE(0);
	if (nRet == IDOK) {
		const WCHAR* pwszPath = dialog.getPath();
		if (*(pwszPath + wcslen(pwszPath) + 1)) {
			StringBuffer<WSTRING> buf(&status);
			CHECK_QSTATUS();
			const WCHAR* p = pwszPath;
			while (true) {
				status = buf.append(p);
				CHECK_QSTATUS();
				p += wcslen(p) + 1;
				if (!*p)
					break;
				status = buf.append(L';');
				CHECK_QSTATUS();
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

void qm::ImportDialog::updateState()
{
	bool bEnable = sendDlgItemMessage(IDC_PATH, WM_GETTEXTLENGTH) != 0;
	Window(getDlgItem(IDOK)).enableWindow(bEnable);
}


/****************************************************************************
 *
 * InputBoxDialog
 *
 */

qm::InputBoxDialog::InputBoxDialog(bool bMultiLine,
	const WCHAR* pwszMessage, const WCHAR* pwszValue, QSTATUS* pstatus) :
	DefaultDialog(bMultiLine ? IDD_MULTIINPUTBOX : IDD_SINGLEINPUTBOX, pstatus),
	bMultiLine_(bMultiLine),
	wstrMessage_(0),
	wstrValue_(0)
{
	string_ptr<WSTRING> wstrMessage;
	if (pwszMessage) {
		wstrMessage.reset(allocWString(pwszMessage));
		if (!wstrMessage.get()) {
			*pstatus = QSTATUS_OUTOFMEMORY;
			return;
		}
	}
	string_ptr<WSTRING> wstrValue;
	if (pwszValue) {
		wstrValue.reset(allocWString(pwszValue));
		if (!wstrValue.get()) {
			*pstatus = QSTATUS_OUTOFMEMORY;
			return;
		}
	}
	
	wstrMessage_ = wstrMessage.release();
	wstrValue_ = wstrValue.release();
}

qm::InputBoxDialog::~InputBoxDialog()
{
	freeWString(wstrMessage_);
	freeWString(wstrValue_);
}

const WCHAR* qm::InputBoxDialog::getMessage() const
{
	return wstrMessage_;
}

const WCHAR* qm::InputBoxDialog::getValue() const
{
	return wstrValue_;
}

LRESULT qm::InputBoxDialog::onInitDialog(HWND hwndFocus, LPARAM lParam)
{
	DECLARE_QSTATUS();
	
	init(false);
	
	if (wstrMessage_)
		setDlgItemText(IDC_MESSAGE, wstrMessage_);
	
	if (wstrValue_) {
		if (bMultiLine_ && wcschr(wstrValue_, L'\n')) {
			StringBuffer<WSTRING> buf(&status);
			CHECK_QSTATUS();
			
			const WCHAR* p = wstrValue_;
			while (*p) {
				if (*p == L'\n') {
					status = buf.append(L'\r');
					CHECK_QSTATUS_VALUE(TRUE);
				}
				status = buf.append(*p);
				CHECK_QSTATUS();
				++p;
			}
			
			setDlgItemText(IDC_VALUE, buf.getCharArray());
		}
		else {
			setDlgItemText(IDC_VALUE, wstrValue_);
		}
	}
	
	return TRUE;
}

LRESULT qm::InputBoxDialog::onOk()
{
	freeWString(wstrValue_);
	wstrValue_ = getDlgItemText(IDC_VALUE);
	if (bMultiLine_ && wstrValue_) {
		WCHAR* pSrc = wstrValue_;
		WCHAR* pDst = wstrValue_;
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

qm::InsertTextDialog::InsertTextDialog(QSTATUS* pstatus) :
	DefaultDialog(IDD_INSERTTEXT, pstatus),
	pManager_(0),
	pText_(0)
{
	DECLARE_QSTATUS();
	
	status = newQsObject(&pManager_);
	CHECK_QSTATUS_SET(pstatus);
}

qm::InsertTextDialog::~InsertTextDialog()
{
	delete pManager_;
}

const FixedFormText* qm::InsertTextDialog::getText() const
{
	return pText_;
}

LRESULT qm::InsertTextDialog::onInitDialog(HWND hwndFocus, LPARAM lParam)
{
	init(false);
	
	const FixedFormTextManager::TextList& l = pManager_->getTextList();
	FixedFormTextManager::TextList::const_iterator it = l.begin();
	while (it != l.end()) {
		string_ptr<TSTRING> tstrName(wcs2tcs((*it)->getName()));
		if (tstrName.get())
			sendDlgItemMessage(IDC_LIST, LB_ADDSTRING, 0,
				reinterpret_cast<LPARAM>(tstrName.get()));
		++it;
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

qm::MailFolderDialog::MailFolderDialog(HINSTANCE hInstResource, QSTATUS* pstatus) :
	DefaultDialog(hInstResource, IDD_MAILFOLDER, pstatus),
	wstrMailFolder_(0)
{
}

qm::MailFolderDialog::~MailFolderDialog()
{
	freeWString(wstrMailFolder_);
}

const WCHAR* qm::MailFolderDialog::getMailFolder() const
{
	return wstrMailFolder_;
}

LRESULT qm::MailFolderDialog::onCommand(WORD nCode, WORD nId)
{
	BEGIN_COMMAND_HANDLER()
		HANDLE_COMMAND_ID(IDC_BROWSE, onBrowse)
		HANDLE_COMMAND_ID_CODE(IDC_MAILFOLDER, EN_CHANGE, onMailFolderChange)
	END_COMMAND_HANDLER()
	return DefaultDialog::onCommand(nCode, nId);
}

LRESULT qm::MailFolderDialog::onInitDialog(HWND hwndFocus, LPARAM lParam)
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
	DECLARE_QSTATUS();
	
	string_ptr<WSTRING> wstrPath;
	status = qs::UIUtil::browseFolder(getHandle(), 0, 0, &wstrPath);
	CHECK_QSTATUS_VALUE(0);
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

qm::MoveMessageDialog::MoveMessageDialog(
	Document* pDocument, Profile* pProfile, QSTATUS* pstatus) :
	DefaultDialog(IDD_MOVEMESSAGE, pstatus),
	pDocument_(pDocument),
	pProfile_(pProfile),
	pFolder_(0),
	bCopy_(false),
	bShowHidden_(false)
{
	DECLARE_QSTATUS();
	
	int nShowHidden = 0;
	status = pProfile->getInt(L"MoveMessageDialog", L"ShowHidden", 0, &nShowHidden);
	CHECK_QSTATUS_SET(pstatus);
	bShowHidden_ = nShowHidden != 0;
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

LRESULT qm::MoveMessageDialog::onCommand(WORD nCode, WORD nId)
{
	BEGIN_COMMAND_HANDLER()
		HANDLE_COMMAND_ID(IDC_SHOWHIDDEN, onShowHidden)
	END_COMMAND_HANDLER()
	return DefaultDialog::onCommand(nCode, nId);
}

LRESULT qm::MoveMessageDialog::onNotify(NMHDR* pnmhdr, bool* pbHandled)
{
	BEGIN_NOTIFY_HANDLER()
		HANDLE_NOTIFY(TVN_SELCHANGED, IDC_FOLDER, onFolderSelChanged);
	END_NOTIFY_HANDLER()
	return 1;
}

LRESULT qm::MoveMessageDialog::onInitDialog(HWND hwndFocus, LPARAM lParam)
{
	DECLARE_QSTATUS();
	
	init(false);
	
	HIMAGELIST hImageList = ImageList_LoadImage(
		Application::getApplication().getResourceHandle(),
		MAKEINTRESOURCE(IDB_FOLDER), 16, 0, CLR_DEFAULT, IMAGE_BITMAP, 0);
	TreeView_SetImageList(getDlgItem(IDC_FOLDER), hImageList, TVSIL_NORMAL);
	
	if (bShowHidden_)
		sendDlgItemMessage(IDC_SHOWHIDDEN, BM_SETCHECK, BST_CHECKED);
	
	status = update();
	CHECK_QSTATUS_VALUE(TRUE);
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

QSTATUS qm::MoveMessageDialog::update()
{
	DECLARE_QSTATUS();
	
	HWND hwnd = getDlgItem(IDC_FOLDER);
	DisableRedraw disable(hwnd);
	
	TreeView_DeleteAllItems(hwnd);
	
	const Document::AccountList& listAccount = pDocument_->getAccounts();
	Document::AccountList::const_iterator it = listAccount.begin();
	while (it != listAccount.end()) {
		status = insertAccount(*it);
		CHECK_QSTATUS();
		++it;
	}
	
	return QSTATUS_SUCCESS;
}

QSTATUS qm::MoveMessageDialog::insertAccount(Account* pAccount)
{
	assert(pAccount);
	
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
			0,
			0,
			0,
			reinterpret_cast<LPARAM>(pAccount)
		}
	};
	HTREEITEM hItemAccount = TreeView_InsertItem(
		getDlgItem(IDC_FOLDER), &tvisAccount);
	if (!hItemAccount)
		return QSTATUS_FAIL;
	
	status = insertFolders(hItemAccount, pAccount);
	CHECK_QSTATUS();
	
	return QSTATUS_SUCCESS;
}

QSTATUS qm::MoveMessageDialog::insertFolders(HTREEITEM hItem, Account* pAccount)
{
	assert(hItem);
	assert(pAccount);
	
	DECLARE_QSTATUS();
	
	const Account::FolderList& l = pAccount->getFolders();
	Account::FolderList listFolder;
	status = STLWrapper<Account::FolderList>(listFolder).reserve(l.size());
	CHECK_QSTATUS();
	if (bShowHidden_)
		std::copy(l.begin(), l.end(), std::back_inserter(listFolder));
	else
		std::remove_copy_if(l.begin(), l.end(),
			std::back_inserter(listFolder), std::mem_fun(&Folder::isHidden));
	std::sort(listFolder.begin(), listFolder.end(), FolderLess());
	
	typedef std::vector<std::pair<Folder*, HTREEITEM> > Stack;
	Stack stack;
	status = STLWrapper<Stack>(stack).push_back(Stack::value_type(0, hItem));
	CHECK_QSTATUS();
	
	Account::FolderList::const_iterator it = listFolder.begin();
	while (it != listFolder.end()) {
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
			return QSTATUS_FAIL;
		
		status = STLWrapper<Stack>(stack).push_back(
			Stack::value_type(pFolder, hItemFolder));
		CHECK_QSTATUS();
		
		++it;
	}
	
	return QSTATUS_SUCCESS;
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
 * ProgressDialog
 *
 */

qm::ProgressDialog::ProgressDialog(UINT nTitleId, QSTATUS* pstatus) :
	DefaultDialog(IDD_PROGRESS, pstatus),
	nTitleId_(nTitleId),
	bCanceled_(false)
{
}

qm::ProgressDialog::~ProgressDialog()
{
}

QSTATUS qm::ProgressDialog::init(HWND hwnd)
{
	DECLARE_QSTATUS();
	
	status = create(hwnd);
	CHECK_QSTATUS();
	showWindow(SW_SHOW);
	getModalHandler()->preModalDialog(0);
	
	return QSTATUS_SUCCESS;
}

void qm::ProgressDialog::term()
{
	getModalHandler()->postModalDialog(0);
	destroyWindow();
}

bool qm::ProgressDialog::isCanceled() const
{
	MSG msg;
	while (::PeekMessage(&msg, 0, 0, 0, PM_REMOVE)) {
		::TranslateMessage(&msg);
		::DispatchMessage(&msg);
	}
	return bCanceled_;
}

QSTATUS qm::ProgressDialog::setTitle(UINT nId)
{
	DECLARE_QSTATUS();
	
	string_ptr<WSTRING> wstrMessage;
	status = loadString(Application::getApplication().getResourceHandle(),
		nId, &wstrMessage);
	CHECK_QSTATUS();
	setWindowText(wstrMessage.get());
	
	return QSTATUS_SUCCESS;
}

QSTATUS qm::ProgressDialog::setMessage(UINT nId)
{
	DECLARE_QSTATUS();
	
	string_ptr<WSTRING> wstrMessage;
	status = loadString(Application::getApplication().getResourceHandle(),
		nId, &wstrMessage);
	CHECK_QSTATUS();
	setDlgItemText(IDC_MESSAGE, wstrMessage.get());
	
	return QSTATUS_SUCCESS;
}

QSTATUS qm::ProgressDialog::setRange(unsigned int nMin, unsigned int nMax)
{
	sendDlgItemMessage(IDC_PROGRESS, PBM_SETRANGE32, nMin, nMax);
	return QSTATUS_SUCCESS;
}

QSTATUS qm::ProgressDialog::setPos(unsigned int n)
{
	sendDlgItemMessage(IDC_PROGRESS, PBM_SETPOS, n);
	return QSTATUS_SUCCESS;
}

LRESULT qm::ProgressDialog::onDestroy()
{
	return DefaultDialog::onDestroy();
}

LRESULT qm::ProgressDialog::onInitDialog(HWND hwndFocus, LPARAM lParam)
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

qm::RenameDialog::RenameDialog(const WCHAR* pwszName, QSTATUS* pstatus) :
	DefaultDialog(IDD_RENAME, pstatus),
	wstrName_(0)
{
	wstrName_ = allocWString(pwszName);
	if (!wstrName_) {
		*pstatus = QSTATUS_OUTOFMEMORY;
		return;
	}
}

qm::RenameDialog::~RenameDialog()
{
	freeWString(wstrName_);
}

const WCHAR* qm::RenameDialog::getName() const
{
	return wstrName_;
}

LRESULT qm::RenameDialog::onCommand(WORD nCode, WORD nId)
{
	BEGIN_COMMAND_HANDLER()
		HANDLE_COMMAND_ID_CODE(IDC_NAME, EN_CHANGE, onNameChange)
	END_COMMAND_HANDLER()
	return DefaultDialog::onCommand(nCode, nId);
}

LRESULT qm::RenameDialog::onInitDialog(HWND hwndFocus, LPARAM lParam)
{
	init(false);
	
	setDlgItemText(IDC_NAME, wstrName_);
	
	return TRUE;
}

LRESULT qm::RenameDialog::onOk()
{
	freeWString(wstrName_);
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

qm::ReplaceDialog::ReplaceDialog(Profile* pProfile, QSTATUS* pstatus) :
	DefaultDialog(IDD_REPLACE, pstatus),
	pProfile_(pProfile),
	wstrFind_(0),
	wstrReplace_(0),
	bMatchCase_(0),
	type_(TYPE_NEXT)
{
}

qm::ReplaceDialog::~ReplaceDialog()
{
	freeWString(wstrFind_);
	freeWString(wstrReplace_);
}

const WCHAR* qm::ReplaceDialog::getFind() const
{
	return wstrFind_;
}

const WCHAR* qm::ReplaceDialog::getReplace() const
{
	return wstrReplace_;
}

bool qm::ReplaceDialog::isMatchCase() const
{
	return bMatchCase_;
}

ReplaceDialog::Type qm::ReplaceDialog::getType() const
{
	return type_;
}

LRESULT qm::ReplaceDialog::onCommand(WORD nCode, WORD nId)
{
	BEGIN_COMMAND_HANDLER()
		HANDLE_COMMAND_ID_RANGE(IDC_REPLACENEXT, IDC_REPLACEALL, onReplace)
	END_COMMAND_HANDLER()
	return DefaultDialog::onCommand(nCode, nId);
}

LRESULT qm::ReplaceDialog::onInitDialog(HWND hwndFocus, LPARAM lParam)
{
	DECLARE_QSTATUS();
	
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
			string_ptr<WSTRING> wstr;
			status = pProfile_->getString(
				items[m].pwszSection_, wszKey, L"", &wstr);
			CHECK_QSTATUS();
			if (*wstr.get()) {
				W2T(wstr.get(), ptsz);
				sendDlgItemMessage(items[m].nId_, CB_ADDSTRING, 0,
					reinterpret_cast<LPARAM>(ptsz));
			}
		}
	}
	
	int nMatchCase = 0;
	status = pProfile_->getInt(L"Find", L"MatchCase", 0, &nMatchCase);
	CHECK_QSTATUS();
	sendDlgItemMessage(IDC_MATCHCASE, BM_SETCHECK,
		nMatchCase ? BST_CHECKED : BST_UNCHECKED);
	
	return TRUE;
}

LRESULT qm::ReplaceDialog::onReplace(UINT nId)
{
	DECLARE_QSTATUS();
	
	struct {
		UINT nId_;
		WSTRING* pwstr_;
		const WCHAR* pwszSection_;
	} items[] = {
		{ IDC_FIND,		&wstrFind_,		L"Find"		},
		{ IDC_REPLACE,	&wstrReplace_,	L"Replace"	}
	};
	for (int m = 0; m < countof(items); ++m) {
		Window edit(Window(getDlgItem(items[m].nId_)).getWindow(GW_CHILD));
		string_ptr<WSTRING> wstrText(edit.getWindowText());
		if (wstrText.get()) {
			for (int n = HISTORY_SIZE - 1; n > 0; --n) {
				WCHAR wszFromKey[32];
				swprintf(wszFromKey, L"History%d", n - 1);
				string_ptr<WSTRING> wstr;
				status = pProfile_->getString(
					items[m].pwszSection_, wszFromKey, L"", &wstr);
				CHECK_QSTATUS();
				
				WCHAR wszToKey[32];
				swprintf(wszToKey, L"History%d", n);
				status = pProfile_->setString(
					items[m].pwszSection_, wszToKey, wstr.get());
				CHECK_QSTATUS();
			}
			
			status = pProfile_->setString(
				items[m].pwszSection_, L"History0", wstrText.get());
			CHECK_QSTATUS();
		}
		*items[m].pwstr_ = wstrText.release();
	}
	
	bMatchCase_ = sendDlgItemMessage(IDC_MATCHCASE, BM_GETCHECK) == BST_CHECKED;
	status = pProfile_->setInt(L"Find", L"MatchCase", bMatchCase_ ? 1 : 0);
	CHECK_QSTATUS();
	
	type_ = nId == IDC_REPLACEPREV ? TYPE_PREV :
		nId == IDC_REPLACEALL ? TYPE_ALL : TYPE_NEXT;
	
	endDialog(IDOK);
	
	return 0;
}


/****************************************************************************
 *
 * SelectDialupEntryDialog
 *
 */

qm::SelectDialupEntryDialog::SelectDialupEntryDialog(
	Profile* pProfile, QSTATUS* pstatus) :
	DefaultDialog(IDD_SELECTDIALUPENTRY, pstatus),
	pProfile_(pProfile),
	wstrEntry_(0)
{
}

qm::SelectDialupEntryDialog::~SelectDialupEntryDialog()
{
	freeWString(wstrEntry_);
}

const WCHAR* qm::SelectDialupEntryDialog::getEntry() const
{
	return wstrEntry_;
}

LRESULT qm::SelectDialupEntryDialog::onCommand(WORD nCode, WORD nId)
{
	BEGIN_COMMAND_HANDLER()
		HANDLE_COMMAND_ID_CODE(IDC_ENTRY, LBN_SELCHANGE, onSelChange)
	END_COMMAND_HANDLER()
	return DefaultDialog::onCommand(nCode, nId);
}

LRESULT qm::SelectDialupEntryDialog::onInitDialog(HWND hwndFocus, LPARAM lParam)
{
	DECLARE_QSTATUS();
	
	init(false);
	
	typedef RasConnection::EntryList List;
	List listEntry;
	StringListFree<List> free(listEntry);
	status = RasConnection::getEntries(&listEntry);
	CHECK_QSTATUS_VALUE(TRUE);
	
	List::const_iterator it = listEntry.begin();
	while (it != listEntry.end()) {
		W2T_STATUS(*it, ptszEntry);
		CHECK_QSTATUS_VALUE(TRUE);
		sendDlgItemMessage(IDC_ENTRY, LB_ADDSTRING, 0,
			reinterpret_cast<LPARAM>(ptszEntry));
		++it;
	}
	
	string_ptr<WSTRING> wstrEntry;
	status = pProfile_->getString(L"Dialup", L"Entry", 0, &wstrEntry);
	CHECK_QSTATUS_VALUE(TRUE);
	W2T_STATUS(wstrEntry.get(), ptszEntry);
	CHECK_QSTATUS_VALUE(TRUE);
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
	
	string_ptr<TSTRING> tstrEntry(allocTString(nLen + 1));
	if (!tstrEntry.get())
		return 0;
	
	sendDlgItemMessage(IDC_ENTRY, LB_GETTEXT, nIndex,
		reinterpret_cast<LPARAM>(tstrEntry.get()));
	
	wstrEntry_ = tcs2wcs(tstrEntry.get());
	if (!wstrEntry_)
		return 0;
	
	pProfile_->setString(L"Dialup", L"Entry", wstrEntry_);
	
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
	Account* pAccount, const WCHAR* pwszDefaultName, QSTATUS* pstatus) :
	DefaultDialog(IDD_SELECTSYNCFILTER, pstatus),
	pwszName_(pwszDefaultName)
{
	DECLARE_QSTATUS();
	
	status = pManager->getFilterSets(pAccount, &list_);
	CHECK_QSTATUS_SET(pstatus);
}

qm::SelectSyncFilterDialog::~SelectSyncFilterDialog()
{
}

const WCHAR* qm::SelectSyncFilterDialog::getName() const
{
	return pwszName_;
}

LRESULT qm::SelectSyncFilterDialog::onInitDialog(HWND hwndFocus, LPARAM lParam)
{
	// TODO
	// Error handling while W2T
	
	init(false);
	
	if (list_.empty()) {
		endDialog(IDOK);
	}
	else {
		SyncFilterManager::FilterSetList::const_iterator it = list_.begin();
		while (it != list_.end()) {
			W2T((*it)->getName(), ptszName);
			sendDlgItemMessage(IDC_FILTERSETLIST, LB_ADDSTRING,
				0, reinterpret_cast<LPARAM>(ptszName));
			++it;
		}
		W2T(pwszName_, ptszName);
		sendDlgItemMessage(IDC_FILTERSETLIST, LB_SELECTSTRING,
			-1, reinterpret_cast<LPARAM>(ptszName));
	}
	
	return TRUE;
}

LRESULT qm::SelectSyncFilterDialog::onOk()
{
	DECLARE_QSTATUS();
	
	unsigned int nItem = sendDlgItemMessage(IDC_FILTERSETLIST, LB_GETCURSEL);
	if (nItem == LB_ERR)
		return onCancel();
	
	assert(nItem < list_.size());
	pwszName_ = list_[nItem]->getName();
	
	return DefaultDialog::onOk();
}
