/*
 * $Id$
 *
 * Copyright(C) 1998-2005 Satoshi Nakamura
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

#include "actionid.h"
#include "dialogs.h"
#include "propertypages.h"
#include "uimanager.h"
#include "uiutil.h"
#include "../model/addressbook.h"
#include "../model/templatemanager.h"
#include "../util/util.h"

#pragma warning(disable:4786)

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
								 PasswordManager* pPasswordManager,
								 SyncFilterManager* pSyncFilterManager,
								 Profile* pProfile) :
	DefaultDialog(IDD_ACCOUNT),
	pDocument_(pDocument),
	pPasswordManager_(pPasswordManager),
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
	return NotifyHandler::onNotify(pnmhdr, pbHandled);
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
		profile.setInt(L"Global", L"IndexBlockSize", dialog.getIndexBlockSize());
		profile.setString(L"Receive", L"Type", dialog.getReceiveProtocol());
		profile.setInt(L"Receive", L"Port", dialog.getReceivePort());
		profile.setString(L"Send", L"Type", dialog.getSendProtocol());
		profile.setInt(L"Send", L"Port", dialog.getSendPort());
		initProfileForClass(dialog.getClass(), &profile);
		if (!profile.save()) {
			messageBox(hInst, IDS_ERROR_CREATEACCOUNT, MB_OK | MB_ICONERROR, getHandle());
			return 0;
		}
		
		std::auto_ptr<Account> pAccount(new Account(wstrDir.get(),
			pDocument_->getSecurity(), pPasswordManager_, pDocument_->getJunkFilter()));
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
			pSubAccount_ = pSubAccount.get();
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
		Account::Host hosts[] = {
			Account::HOST_RECEIVE,
			Account::HOST_SEND
		};
		
		if (TreeView_GetParent(hwnd, hItem)) {
			SubAccount* pSubAccount = reinterpret_cast<SubAccount*>(item.lParam);
			Account* pAccount = pSubAccount->getAccount();
			
			wstring_ptr wstrConfirm(loadString(hInst, IDS_CONFIRMREMOVESUBACCOUNT));
			wstring_ptr wstrName(concat(pAccount->getName(), L"/", pSubAccount->getName()));
			wstring_ptr wstrMessage(allocWString(wcslen(wstrConfirm.get()) + wcslen(wstrName.get()) + 64));
			swprintf(wstrMessage.get(), wstrConfirm.get(), wstrName.get());
			int nRet = messageBox(wstrMessage.get(), MB_YESNO | MB_DEFBUTTON2, getHandle());
			if (nRet == IDYES) {
				for (int n = 0; n < countof(hosts); ++n) {
					AccountPasswordCondition condition(pAccount, pSubAccount, hosts[n]);
					pPasswordManager_->removePassword(condition);
				}
				
				pAccount->removeSubAccount(pSubAccount);
				pSubAccount_ = pAccount->getCurrentSubAccount();
				
				update();
			}
		}
		else {
			Account* pAccount = reinterpret_cast<Account*>(item.lParam);
			
			const Account::SubAccountList& l = pAccount->getSubAccounts();
			for (Account::SubAccountList::const_iterator it = l.begin(); it != l.end(); ++it) {
				SubAccount* pSubAccount = *it;
				for (int n = 0; n < countof(hosts); ++n) {
					AccountPasswordCondition condition(pAccount, pSubAccount, hosts[n]);
					pPasswordManager_->removePassword(condition);
				}
			}
			
			wstring_ptr wstrConfirm(loadString(hInst, IDS_CONFIRMREMOVEACCOUNT));
			wstring_ptr wstrMessage(allocWString(wcslen(wstrConfirm.get()) + wcslen(pAccount->getName()) + 64));
			swprintf(wstrMessage.get(), wstrConfirm.get(), pAccount->getName());
			int nRet = messageBox(wstrMessage.get(), MB_YESNO | MB_DEFBUTTON2, getHandle());
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
		AccountUserPage userPage(pSubAccount, pPasswordManager_, pReceiveUI.get(), pSendUI.get());
		AccountDetailPage detailPage(pSubAccount, pReceiveUI.get(), pSendUI.get());
		AccountDialupPage dialupPage(pSubAccount);
		AccountAdvancedPage advancedPage(pSubAccount, pDocument_, pSyncFilterManager_);
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
	
	HTREEITEM hItemSelect = 0;
	{
		DisableRedraw disable(hwnd);
		
		TreeView_DeleteAllItems(hwnd);
		
		HINSTANCE hInst = Application::getApplication().getResourceHandle();
		wstring_ptr wstrDefault(loadString(hInst, IDS_DEFAULTSUBACCOUNT));
		
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

void qm::AccountDialog::initProfileForClass(const WCHAR* pwszClass,
											qs::Profile* pProfile)
{
	// TODO
	// Use skelton or something?
	
	enum Class {
		MAIL,
		NEWS,
		RSS
	};
	struct {
		const WCHAR* pwszClass_;
		Class class_;
	} classes[] = {
		{ L"mail",	MAIL	},
		{ L"news",	NEWS	},
		{ L"rss",	RSS		}
	};
	Class c = MAIL;
	for (int n = 0; n < countof(classes); ++n) {
		if (wcscmp(pwszClass, classes[n].pwszClass_) == 0) {
			c = classes[n].class_;
			break;
		}
	}
	
	struct {
		const WCHAR* pwszSection_;
		const WCHAR* pwszKey_;
		int nValue_[3];
	} numbers[] = {
		{ L"Global",	L"AddMessageId",	{	1,	0,	0	}	},
		{ L"Global",	L"TreatAsSent",		{	1,	0,	0	}	}
	};
	for (int n = 0; n < countof(numbers); ++n)
		pProfile->setInt(numbers[n].pwszSection_,
			numbers[n].pwszKey_, numbers[n].nValue_[c]);
}


/****************************************************************************
 *
 * AddAddressDialog
 *
 */

qm::AddAddressDialog::AddAddressDialog(AddressBook* pAddressBook) :
	DefaultDialog(IDD_ADDADDRESS),
	pAddressBook_(pAddressBook),
	type_(TYPE_NEWENTRY),
	pEntry_(0)
{
	pAddressBook_->reload();
}

qm::AddAddressDialog::~AddAddressDialog()
{
}

AddAddressDialog::Type qm::AddAddressDialog::getType() const
{
	return type_;
}

AddressBookEntry* qm::AddAddressDialog::getEntry() const
{
	return pEntry_;
}

LRESULT qm::AddAddressDialog::onCommand(WORD nCode,
										WORD nId)
{
	BEGIN_COMMAND_HANDLER()
		HANDLE_COMMAND_ID(IDC_NEWADDRESS, onNewAddress)
		HANDLE_COMMAND_ID(IDC_NEWENTRY, onNewEntry)
		HANDLE_COMMAND_ID_CODE(IDC_ENTRIES, LBN_SELCHANGE, onEntriesSelChange)
	END_COMMAND_HANDLER()
	return DefaultDialog::onCommand(nCode, nId);
}

LRESULT qm::AddAddressDialog::onInitDialog(HWND hwndFocus,
										   LPARAM lParam)
{
	init(false);
	
	switch (type_) {
	case TYPE_NEWENTRY:
		sendDlgItemMessage(IDC_NEWENTRY, BM_SETCHECK, BST_CHECKED);
		break;
	case TYPE_NEWADDRESS:
		sendDlgItemMessage(IDC_NEWADDRESS, BM_SETCHECK, BST_CHECKED);
		break;
	}
	
	const AddressBook::EntryList& l = pAddressBook_->getEntries();
	AddressBook::EntryList listEntry;
	listEntry.reserve(l.size());
	std::remove_copy_if(l.begin(), l.end(),
		std::back_inserter(listEntry),
		std::mem_fun(&AddressBookEntry::isExternal));
	std::sort(listEntry.begin(), listEntry.end(),
		binary_compose_f_gx_hy(
			string_less_i<WCHAR>(),
			std::mem_fun(&AddressBookEntry::getActualSortKey),
			std::mem_fun(&AddressBookEntry::getActualSortKey)));
	for (AddressBook::EntryList::const_iterator it = listEntry.begin(); it != listEntry.end(); ++it) {
		const AddressBookEntry* pEntry = *it;
		W2T(pEntry->getName(), ptszName);
		int nItem = sendDlgItemMessage(IDC_ENTRIES, LB_ADDSTRING,
			0, reinterpret_cast<LPARAM>(ptszName));
		sendDlgItemMessage(IDC_ENTRIES, LB_SETITEMDATA,
			nItem, reinterpret_cast<LPARAM>(pEntry));
	}
	sendDlgItemMessage(IDC_ENTRIES, LB_SETCURSEL, 0);
	
	updateState();
	
	return TRUE;
}

LRESULT qm::AddAddressDialog::onOk()
{
	if (sendDlgItemMessage(IDC_NEWADDRESS, BM_GETCHECK) == BST_CHECKED) {
		type_ = TYPE_NEWADDRESS;
		
		int nItem = sendDlgItemMessage(IDC_ENTRIES, LB_GETCURSEL);
		if (nItem == LB_ERR)
			return 0;
		pEntry_ = reinterpret_cast<AddressBookEntry*>(
			sendDlgItemMessage(IDC_ENTRIES, LB_GETITEMDATA, nItem));
	}
	else {
		type_ = TYPE_NEWENTRY;
	}
	
	return DefaultDialog::onOk();
}

LRESULT qm::AddAddressDialog::onNewEntry()
{
	updateState();
	return 0;
}

LRESULT qm::AddAddressDialog::onNewAddress()
{
	updateState();
	return 0;
}

LRESULT qm::AddAddressDialog::onEntriesSelChange()
{
	updateState();
	return 0;
}

void qm::AddAddressDialog::updateState()
{
	bool bNewAddress = sendDlgItemMessage(IDC_NEWADDRESS, BM_GETCHECK) == BST_CHECKED;
	Window(getDlgItem(IDC_ENTRIES)).enableWindow(bNewAddress);
	Window(getDlgItem(IDOK)).enableWindow(!bNewAddress ||
		sendDlgItemMessage(IDC_ENTRIES, LB_GETCURSEL) != LB_ERR);
}


/****************************************************************************
 *
 * AddressBookAddressDialog
 *
 */

qm::AddressBookAddressDialog::AddressBookAddressDialog(AddressBook* pAddressBook,
													   AddressBookAddress* pAddress) :
	DefaultDialog(IDD_ADDRESSBOOKADDRESS),
	pAddressBook_(pAddressBook),
	pAddress_(pAddress)
{
}

qm::AddressBookAddressDialog::~AddressBookAddressDialog()
{
}

LRESULT qm::AddressBookAddressDialog::onCommand(WORD nCode,
												WORD nId)
{
	BEGIN_COMMAND_HANDLER()
		HANDLE_COMMAND_ID_CODE(IDC_ADDRESS, EN_CHANGE, onAddressChange)
	END_COMMAND_HANDLER()
	return DefaultDialog::onCommand(nCode, nId);
}

LRESULT qm::AddressBookAddressDialog::onInitDialog(HWND hwndFocus,
												   LPARAM lParam)
{
	init(false);
	
	setDlgItemText(IDC_ADDRESS, pAddress_->getAddress());
	sendDlgItemMessage(IDC_RFC2822, BM_SETCHECK,
		pAddress_->isRFC2822() ? BST_CHECKED : BST_UNCHECKED);
	if (pAddress_->getAlias())
		setDlgItemText(IDC_ALIAS, pAddress_->getAlias());
	
	AddressBook::CategoryList listAllCategory(pAddressBook_->getCategories());
	std::sort(listAllCategory.begin(), listAllCategory.end(), AddressBookCategoryLess());
	for (AddressBook::CategoryList::const_iterator it = listAllCategory.begin(); it != listAllCategory.end(); ++it) {
		W2T((*it)->getName(), ptszName);
		sendDlgItemMessage(IDC_CATEGORY, CB_ADDSTRING,
			0, reinterpret_cast<LPARAM>(ptszName));
	}
	
	StringBuffer<WSTRING> bufCategory;
	const AddressBookAddress::CategoryList& listCategory = pAddress_->getCategories();
	for (AddressBookAddress::CategoryList::const_iterator it = listCategory.begin(); it != listCategory.end(); ++it) {
		if (bufCategory.getLength() != 0)
			bufCategory.append(L',');
		bufCategory.append((*it)->getName());
	}
	setDlgItemText(IDC_CATEGORY, bufCategory.getCharArray());
	
	if (pAddress_->getComment())
		setDlgItemText(IDC_COMMENT, pAddress_->getComment());
	if (pAddress_->getCertificate())
		setDlgItemText(IDC_CERTIFICATE, pAddress_->getCertificate());
	
	updateState();
	
	return TRUE;
}

LRESULT qm::AddressBookAddressDialog::onOk()
{
	wstring_ptr wstrAddress(getDlgItemText(IDC_ADDRESS));
	if (!*wstrAddress.get())
		return 0;
	bool bRFC2822 = sendDlgItemMessage(IDC_RFC2822, BM_GETCHECK) == BST_CHECKED;
	wstring_ptr wstrAlias(getDlgItemText(IDC_ALIAS));
	
	wstring_ptr wstrCategory(getDlgItemText(IDC_CATEGORY));
	AddressBookAddress::CategoryList listCategory;
	const WCHAR* p = wcstok(wstrCategory.get(), L", ");
	while (p) {
		listCategory.push_back(pAddressBook_->getCategory(p));
		p = wcstok(0, L", ");
	}
	
	wstring_ptr wstrComment(getDlgItemText(IDC_COMMENT));
	wstring_ptr wstrCertificate(getDlgItemText(IDC_CERTIFICATE));
	
	pAddress_->setAddress(wstrAddress.get());
	pAddress_->setAlias(*wstrAlias.get() ? wstrAlias.get() : 0);
	pAddress_->setCategories(listCategory);
	pAddress_->setComment(*wstrComment.get() ? wstrComment.get() : 0);
	pAddress_->setCertificate(*wstrCertificate.get() ? wstrCertificate.get() : 0);
	pAddress_->setRFC2822(bRFC2822);
	
	return DefaultDialog::onOk();
}

LRESULT qm::AddressBookAddressDialog::onAddressChange()
{
	updateState();
	return 0;
}

void qm::AddressBookAddressDialog::updateState()
{
	Window(getDlgItem(IDOK)).enableWindow(
		Window(getDlgItem(IDC_ADDRESS)).getWindowTextLength() != 0);
}


/****************************************************************************
 *
 * AddressBookEntryDialog
 *
 */

qm::AddressBookEntryDialog::AddressBookEntryDialog(AddressBook* pAddressBook,
												   AddressBookEntry* pEntry) :
	AbstractListDialog<AddressBookAddress, AddressBookEntry::AddressList>(IDD_ADDRESSBOOKENTRY, IDC_ADDRESSES),
	pAddressBook_(pAddressBook),
	pEntry_(pEntry)
{
	const AddressBookEntry::AddressList& l = pEntry->getAddresses();
	AddressBookEntry::AddressList& list = getList();
	list.reserve(l.size());
	for (AddressBookEntry::AddressList::const_iterator it = l.begin(); it != l.end(); ++it)
		list.push_back(new AddressBookAddress(**it));
}

qm::AddressBookEntryDialog::~AddressBookEntryDialog()
{
}

LRESULT qm::AddressBookEntryDialog::onCommand(WORD nCode,
											  WORD nId)
{
	BEGIN_COMMAND_HANDLER()
		HANDLE_COMMAND_ID_CODE(IDC_NAME, EN_CHANGE, onNameChange)
	END_COMMAND_HANDLER()
	return AbstractListDialog<AddressBookAddress, AddressBookEntry::AddressList>::onCommand(nCode, nId);
}

LRESULT qm::AddressBookEntryDialog::onInitDialog(HWND hwndFocus,
												 LPARAM lParam)
{
	init(false);
	
	setDlgItemText(IDC_NAME, pEntry_->getName());
	if (pEntry_->getSortKey())
		setDlgItemText(IDC_SORTKEY, pEntry_->getSortKey());
	
	return AbstractListDialog<AddressBookAddress, AddressBookEntry::AddressList>::onInitDialog(hwndFocus, lParam);
}

LRESULT qm::AddressBookEntryDialog::onOk()
{
	wstring_ptr wstrName(getDlgItemText(IDC_NAME));
	pEntry_->setName(wstrName.get());
	
	wstring_ptr wstrSortKey(getDlgItemText(IDC_SORTKEY));
	pEntry_->setSortKey(*wstrSortKey.get() ? wstrSortKey.get() : 0);
	
	pEntry_->setAddresses(getList());
	
	return AbstractListDialog<AddressBookAddress, AddressBookEntry::AddressList>::onOk();
}

wstring_ptr qm::AddressBookEntryDialog::getLabel(const AddressBookAddress* p) const
{
	return allocWString(p->getAddress());
}

std::auto_ptr<AddressBookAddress> qm::AddressBookEntryDialog::create() const
{
	std::auto_ptr<AddressBookAddress> pAddress(new AddressBookAddress(pEntry_));
	AddressBookAddressDialog dialog(pAddressBook_, pAddress.get());
	if (dialog.doModal(getHandle()) != IDOK)
		return std::auto_ptr<AddressBookAddress>();
	return pAddress;
}

bool qm::AddressBookEntryDialog::edit(AddressBookAddress* p) const
{
	AddressBookAddressDialog dialog(pAddressBook_, p);
	return dialog.doModal(getHandle()) == IDOK;
}

void qm::AddressBookEntryDialog::updateState()
{
	AbstractListDialog<AddressBookAddress, AddressBookEntry::AddressList>::updateState();
	
	bool bEnable = Window(getDlgItem(IDC_NAME)).getWindowTextLength() != 0 &&
		sendDlgItemMessage(IDC_ADDRESSES, LB_GETCOUNT) != 0;
	Window(getDlgItem(IDOK)).enableWindow(bEnable);
}

LRESULT qm::AddressBookEntryDialog::onNameChange()
{
	updateState();
	return 0;
}


/****************************************************************************
 *
 * ArgumentDialog
 *
 */

qm::ArgumentDialog::ArgumentDialog(const WCHAR* pwszName,
								   const WCHAR* pwszValue) :
	DefaultDialog(IDD_ARGUMENT)
{
	if (pwszName)
		wstrName_ = allocWString(pwszName);
	if (pwszValue)
		wstrValue_ = allocWString(pwszValue);
}

qm::ArgumentDialog::~ArgumentDialog()
{
}

const WCHAR* qm::ArgumentDialog::getName() const
{
	return wstrName_.get();
}

const WCHAR* qm::ArgumentDialog::getValue() const
{
	return wstrValue_.get();
}

LRESULT qm::ArgumentDialog::onCommand(WORD nCode,
									  WORD nId)
{
	BEGIN_COMMAND_HANDLER()
		HANDLE_COMMAND_ID_CODE(IDC_NAME, EN_CHANGE, onNameChange)
	END_COMMAND_HANDLER()
	return DefaultDialog::onCommand(nCode, nId);
}

LRESULT qm::ArgumentDialog::onInitDialog(HWND hwndFocus,
										 LPARAM lParam)
{
	init(false);
	
	if (wstrName_.get())
		setDlgItemText(IDC_NAME, wstrName_.get());
	if (wstrValue_.get())
		setDlgItemText(IDC_VALUE, wstrValue_.get());
	
	updateState();
	
	return TRUE;
}

LRESULT qm::ArgumentDialog::onOk()
{
	wstrName_ = getDlgItemText(IDC_NAME);
	wstrValue_ = getDlgItemText(IDC_VALUE);
	
	return DefaultDialog::onOk();
}

LRESULT qm::ArgumentDialog::onNameChange()
{
	updateState();
	return 0;
}

void qm::ArgumentDialog::updateState()
{
	Window(getDlgItem(IDOK)).enableWindow(
		Window(getDlgItem(IDC_NAME)).getWindowTextLength() != 0);
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
	return NotifyHandler::onNotify(pnmhdr, pbHandled);
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
 * AutoPilotDialog
 *
 */

qm::AutoPilotDialog::AutoPilotDialog(AutoPilotManager* pManager,
									 GoRound* pGoRound) :
	AbstractListDialog<AutoPilotEntry, AutoPilotManager::EntryList>(IDD_AUTOPILOT, IDC_ENTRIES),
	pManager_(pManager),
	pGoRound_(pGoRound)
{
	const AutoPilotManager::EntryList& l = pManager->getEntries();
	AutoPilotManager::EntryList& list = getList();
	list.reserve(l.size());
	for (AutoPilotManager::EntryList::const_iterator it = l.begin(); it != l.end(); ++it)
		list.push_back(new AutoPilotEntry(**it));
}

qm::AutoPilotDialog::~AutoPilotDialog()
{
}

LRESULT qm::AutoPilotDialog::onOk()
{
	pManager_->setEntries(getList());
	if (!pManager_->save()) {
		// TODO
	}
	return AbstractListDialog<AutoPilotEntry, AutoPilotManager::EntryList>::onOk();
}

wstring_ptr qm::AutoPilotDialog::getLabel(const AutoPilotEntry* p) const
{
	return allocWString(p->getCourse());
}

std::auto_ptr<AutoPilotEntry> qm::AutoPilotDialog::create() const
{
	std::auto_ptr<AutoPilotEntry> pEntry(new AutoPilotEntry());
	AutoPilotEntryDialog dialog(pEntry.get(), pGoRound_);
	if (dialog.doModal(getHandle()) != IDOK)
		return std::auto_ptr<AutoPilotEntry>();
	return pEntry;
}

bool qm::AutoPilotDialog::edit(AutoPilotEntry* p) const
{
	AutoPilotEntryDialog dialog(p, pGoRound_);
	return dialog.doModal(getHandle()) == IDOK;
}


/****************************************************************************
 *
 * AutoPilotEntryDialog
 *
 */

qm::AutoPilotEntryDialog::AutoPilotEntryDialog(AutoPilotEntry* pEntry,
											   GoRound* pGoRound) :
	DefaultDialog(IDD_AUTOPILOTENTRY),
	pEntry_(pEntry),
	pGoRound_(pGoRound)
{
}

qm::AutoPilotEntryDialog::~AutoPilotEntryDialog()
{
}

LRESULT qm::AutoPilotEntryDialog::onCommand(WORD nCode,
											WORD nId)
{
	BEGIN_COMMAND_HANDLER()
		HANDLE_COMMAND_ID_CODE(IDC_COURSE, CBN_EDITCHANGE, onCourseEditChange)
		HANDLE_COMMAND_ID_CODE(IDC_COURSE, CBN_SELCHANGE, onCourseSelChange)
		HANDLE_COMMAND_ID_CODE(IDC_INTERVAL, EN_CHANGE, onIntervalChange)
	END_COMMAND_HANDLER()
	return DefaultDialog::onCommand(nCode, nId);
}

LRESULT qm::AutoPilotEntryDialog::onInitDialog(HWND hwndFocus,
											   LPARAM lParam)
{
	init(false);
	
	const GoRound::CourseList& listCourse = pGoRound_->getCourses();
	for (GoRound::CourseList::const_iterator it = listCourse.begin(); it != listCourse.end(); ++it) {
		W2T((*it)->getName(), ptszCourse);
		sendDlgItemMessage(IDC_COURSE, CB_ADDSTRING,
			0, reinterpret_cast<LPARAM>(ptszCourse));
	}
	setDlgItemText(IDC_COURSE, pEntry_->getCourse());
	setDlgItemInt(IDC_INTERVAL, pEntry_->getInterval());
	
	updateState();
	
	return TRUE;
}

LRESULT qm::AutoPilotEntryDialog::onOk()
{
	wstring_ptr wstrCourse(getDlgItemText(IDC_COURSE));
	if (!*wstrCourse.get())
		return 0;
	
	int nInterval = getDlgItemInt(IDC_INTERVAL);
	if (nInterval == 0)
		return 0;
	
	pEntry_->setCourse(wstrCourse.get());
	pEntry_->setInterval(nInterval);
	
	return DefaultDialog::onOk();
}

LRESULT qm::AutoPilotEntryDialog::onCourseEditChange()
{
	updateState();
	return 0;
}

LRESULT qm::AutoPilotEntryDialog::onCourseSelChange()
{
	postMessage(WM_COMMAND, MAKEWPARAM(IDC_COURSE, CBN_EDITCHANGE));
	return 0;
}

LRESULT qm::AutoPilotEntryDialog::onIntervalChange()
{
	updateState();
	return 0;
}

void qm::AutoPilotEntryDialog::updateState()
{
	Window(getDlgItem(IDOK)).enableWindow(
		Window(getDlgItem(IDC_COURSE)).getWindowTextLength() != 0 &&
		getDlgItemInt(IDC_INTERVAL) != 0);
}


/****************************************************************************
 *
 * ColorDialog
 *
 */

qm::ColorDialog::ColorDialog(ColorEntry* pColor,
							 Document* pDocument) :
	DefaultDialog(IDD_COLOR),
	pColor_(pColor)
{
}

qm::ColorDialog::~ColorDialog()
{
}

LRESULT qm::ColorDialog::onCommand(WORD nCode,
								   WORD nId)
{
	BEGIN_COMMAND_HANDLER()
		HANDLE_COMMAND_ID(IDC_EDIT, onEdit)
		HANDLE_COMMAND_ID(IDC_CHOOSE, onChoose)
		HANDLE_COMMAND_ID_CODE(IDC_COLOR, EN_CHANGE, onColorChange)
		HANDLE_COMMAND_ID_CODE(IDC_CONDITION, EN_CHANGE, onConditionChange)
	END_COMMAND_HANDLER()
	return DefaultDialog::onCommand(nCode, nId);
}

LRESULT qm::ColorDialog::onInitDialog(HWND hwndFocus,
									  LPARAM lParam)
{
	init(false);
	
	const Macro* pCondition = pColor_->getCondition();
	if (pCondition) {
		wstring_ptr wstrCondition(pCondition->getString());
		setDlgItemText(IDC_CONDITION, wstrCondition.get());
	}
	
	Color color(pColor_->getColor());
	wstring_ptr wstrColor(color.getString());
	setDlgItemText(IDC_COLOR, wstrColor.get());
	
	updateState();
	
	return TRUE;
}

LRESULT qm::ColorDialog::onOk()
{
	wstring_ptr wstrCondition(getDlgItemText(IDC_CONDITION));
	std::auto_ptr<Macro> pCondition(MacroParser(MacroParser::TYPE_COLOR).parse(wstrCondition.get()));
	if (!pCondition.get()) {
		// TODO MSG
		return 0;
	}
	
	wstring_ptr wstrColor(getDlgItemText(IDC_COLOR));
	Color color(wstrColor.get());
	if (color.getColor() == 0xffffffff) {
		// TODO MSG
		return 0;
	}
	
	pColor_->setCondition(pCondition);
	pColor_->setColor(color.getColor());
	
	return DefaultDialog::onOk();
}

LRESULT qm::ColorDialog::onEdit()
{
	wstring_ptr wstrCondition(getDlgItemText(IDC_CONDITION));
	ConditionDialog dialog(wstrCondition.get());
	if (dialog.doModal(getHandle()) == IDOK)
		setDlgItemText(IDC_CONDITION, dialog.getCondition());
	return 0;
}

LRESULT qm::ColorDialog::onChoose()
{
	wstring_ptr wstrColor(getDlgItemText(IDC_COLOR));
	Color color(wstrColor.get());
	COLORREF cr = color.getColor();
	if (cr == 0xffffffff)
		cr = RGB(0, 0, 0);
	
	COLORREF crCustom[16];
	CHOOSECOLOR cc = {
		sizeof(cc),
		getHandle(),
		0,
		cr,
		crCustom,
		CC_ANYCOLOR | CC_RGBINIT,
		0,
		0,
		0
	};
	if (::ChooseColor(&cc)) {
		Color color(cc.rgbResult);
		wstring_ptr wstrColor(color.getString());
		setDlgItemText(IDC_COLOR, wstrColor.get());
	}
	return 0;
}

LRESULT qm::ColorDialog::onConditionChange()
{
	updateState();
	return 0;
}

LRESULT qm::ColorDialog::onColorChange()
{
	updateState();
	return 0;
}

void qm::ColorDialog::updateState()
{
	Window(getDlgItem(IDOK)).enableWindow(
		Window(getDlgItem(IDC_CONDITION)).getWindowTextLength() != 0 &&
		Window(getDlgItem(IDC_COLOR)).getWindowTextLength() != 0);
}


/****************************************************************************
 *
 * ConditionDialog
 *
 */

qm::ConditionDialog::ConditionDialog(const WCHAR* pwszCondition) :
	DefaultDialog(IDD_CONDITION)
{
	wstrCondition_ = allocWString(pwszCondition);
}

qm::ConditionDialog::~ConditionDialog()
{
}

const WCHAR* qm::ConditionDialog::getCondition() const
{
	return wstrCondition_.get();
}

LRESULT qm::ConditionDialog::onCommand(WORD nCode,
									   WORD nId)
{
	// TODO
	return DefaultDialog::onCommand(nCode, nId);
}

LRESULT qm::ConditionDialog::onInitDialog(HWND hwndFocus,
										  LPARAM lParam)
{
	init(false);
	
	// TODO
	
	return TRUE;
}

LRESULT qm::ConditionDialog::onOk()
{
	// TODO
	return DefaultDialog::onOk();
}


/****************************************************************************
 *
 * ConfirmSendDialog
 *
 */

qm::ConfirmSendDialog::ConfirmSendDialog() :
	DefaultDialog(IDD_CONFIRMSEND)
{
}

qm::ConfirmSendDialog::~ConfirmSendDialog()
{
}

LRESULT qm::ConfirmSendDialog::onCommand(WORD nCode,
										 WORD nId)
{
	BEGIN_COMMAND_HANDLER()
		HANDLE_COMMAND_ID(IDC_SEND, onSend)
		HANDLE_COMMAND_ID(IDC_SAVE, onSave)
		HANDLE_COMMAND_ID(IDC_DISCARD, onDiscard)
	END_COMMAND_HANDLER()
	return DefaultDialog::onCommand(nCode, nId);
}

LRESULT qm::ConfirmSendDialog::onInitDialog(HWND hwndFocus,
											LPARAM lParam)
{
	centerWindow(0);
	
#ifndef _WIN32_WCE
	HICON hIcon = ::LoadIcon(0, IDI_QUESTION);
	sendDlgItemMessage(IDC_QUESTION, STM_SETICON, reinterpret_cast<LPARAM>(hIcon));
#endif
	
	return TRUE;
}

LRESULT qm::ConfirmSendDialog::onSend()
{
	endDialog(ID_SEND);
	return 0;
}

LRESULT qm::ConfirmSendDialog::onSave()
{
	endDialog(ID_SAVE);
	return 0;
}

LRESULT qm::ConfirmSendDialog::onDiscard()
{
	endDialog(ID_DISCARD);
	return 0;
}


/****************************************************************************
 *
 * CopyRuleTemplateDialog
 *
 */

qm::CopyRuleTemplateDialog::CopyRuleTemplateDialog(const WCHAR* pwszName,
												   CopyRuleAction::ArgumentList* pListArgument) :
	DefaultDialog(IDD_COPYRULETEMPLATE),
	pListArgument_(pListArgument)
{
	if (pwszName)
		wstrName_ = allocWString(pwszName);
}

qm::CopyRuleTemplateDialog::~CopyRuleTemplateDialog()
{
}

const WCHAR* qm::CopyRuleTemplateDialog::getName() const
{
	return wstrName_.get();
}

LRESULT qm::CopyRuleTemplateDialog::onCommand(WORD nCode,
											  WORD nId)
{
	BEGIN_COMMAND_HANDLER()
		HANDLE_COMMAND_ID(IDC_ADD, onAdd)
		HANDLE_COMMAND_ID(IDC_EDIT, onEdit)
		HANDLE_COMMAND_ID(IDC_REMOVE, onRemove)
	END_COMMAND_HANDLER()
	return DefaultDialog::onCommand(nCode, nId);
}

LRESULT qm::CopyRuleTemplateDialog::onDestroy()
{
	removeNotifyHandler(this);
	return DefaultDialog::onDestroy();
}

LRESULT qm::CopyRuleTemplateDialog::onInitDialog(HWND hwndFocus,
												 LPARAM lParam)
{
	init(false);
	
	if (wstrName_.get())
		setDlgItemText(IDC_NAME, wstrName_.get());
	
	HINSTANCE hInst = Application::getApplication().getResourceHandle();
	HWND hwndList = getDlgItem(IDC_ARGUMENT);
	
	ListView_SetExtendedListViewStyle(hwndList, LVS_EX_FULLROWSELECT);
	
	struct {
		UINT nId_;
		int nWidth_;
	} columns[] = {
		{ IDS_NAME,		100	},
		{ IDS_VALUE,	100	},
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
	
	for (CopyRuleAction::ArgumentList::size_type n = 0; n < pListArgument_->size(); ++n) {
		W2T((*pListArgument_)[n].first, ptszName);
		
		LVITEM item = {
			LVIF_TEXT,
			n,
			0,
			0,
			0,
			const_cast<LPTSTR>(ptszName),
			0,
			0,
			0
		};
		int nItem = ListView_InsertItem(hwndList, &item);
		
		W2T((*pListArgument_)[n].second, ptszValue);
		ListView_SetItemText(hwndList, nItem, 1, const_cast<LPTSTR>(ptszValue));
	}
	
	updateState();
	
	addNotifyHandler(this);
	
	return TRUE;
}

LRESULT qm::CopyRuleTemplateDialog::onOk()
{
	wstring_ptr wstrName(getDlgItemText(IDC_NAME));
	if (*wstrName.get())
		wstrName_ = wstrName;
	else
		wstrName_.reset(0);
	
	HWND hwndList = getDlgItem(IDC_ARGUMENT);
	CopyRuleAction::ArgumentList listArgument;
	int nCount = ListView_GetItemCount(hwndList);
	listArgument.reserve(nCount);
	for (int n = 0; n < nCount; ++n) {
		TCHAR tsz[256];
		ListView_GetItemText(hwndList, n, 0, tsz, countof(tsz) - 1);
		wstring_ptr wstrName(tcs2wcs(tsz));
		ListView_GetItemText(hwndList, n, 1, tsz, countof(tsz) - 1);
		wstring_ptr wstrValue(tcs2wcs(tsz));
		listArgument.push_back(CopyRuleAction::ArgumentList::value_type(
			wstrName.release(), wstrValue.release()));
	}
	pListArgument_->swap(listArgument);
	for (CopyRuleAction::ArgumentList::iterator it = listArgument.begin(); it != listArgument.end(); ++it) {
		freeWString((*it).first);
		freeWString((*it).second);
	}
	
	return DefaultDialog::onOk();
}

LRESULT qm::CopyRuleTemplateDialog::onNotify(NMHDR* pnmhdr,
											 bool* pbHandled)
{
	BEGIN_NOTIFY_HANDLER()
		HANDLE_NOTIFY(LVN_ITEMCHANGED, IDC_ARGUMENT, onArgumentItemChanged);
	END_NOTIFY_HANDLER()
	return NotifyHandler::onNotify(pnmhdr, pbHandled);
}

LRESULT qm::CopyRuleTemplateDialog::onAdd()
{
	ArgumentDialog dialog(0, 0);
	if (dialog.doModal(getHandle()) == IDOK) {
		HWND hwndList = getDlgItem(IDC_ARGUMENT);
		int nCount = ListView_GetItemCount(hwndList);
		
		W2T(dialog.getName(), ptszName);
		LVITEM item = {
			LVIF_TEXT | LVIF_STATE,
			nCount,
			0,
			LVIS_SELECTED | LVIS_FOCUSED,
			LVIS_SELECTED | LVIS_FOCUSED,
			const_cast<LPTSTR>(ptszName),
			0,
			0,
			0
		};
		int nItem = ListView_InsertItem(hwndList, &item);
		
		W2T(dialog.getValue(), ptszValue);
		ListView_SetItemText(hwndList, nItem, 1, const_cast<LPTSTR>(ptszValue));
	}
	return 0;
}

LRESULT qm::CopyRuleTemplateDialog::onRemove()
{
	HWND hwndList = getDlgItem(IDC_ARGUMENT);
	int nItem = ListView_GetNextItem(hwndList, -1, LVNI_SELECTED);
	if (nItem != -1) {
		ListView_DeleteItem(hwndList, nItem);
		
		int nCount = ListView_GetItemCount(hwndList);
		if (nCount != 0) {
			if (nItem < nCount) {
				ListView_SetItemState(hwndList, nItem,
					LVIS_SELECTED | LVIS_FOCUSED, LVIS_SELECTED | LVIS_FOCUSED);
			}
			else {
				ListView_SetItemState(hwndList, nCount - 1,
					LVIS_SELECTED | LVIS_FOCUSED, LVIS_SELECTED | LVIS_FOCUSED);
			}
		}
	}
	return 0;
}

LRESULT qm::CopyRuleTemplateDialog::onEdit()
{
	HWND hwndList = getDlgItem(IDC_ARGUMENT);
	int nItem = ListView_GetNextItem(hwndList, -1, LVNI_SELECTED);
	if (nItem != -1) {
		TCHAR tsz[256];
		ListView_GetItemText(hwndList, nItem, 0, tsz, countof(tsz) - 1);
		wstring_ptr wstrName(tcs2wcs(tsz));
		ListView_GetItemText(hwndList, nItem, 1, tsz, countof(tsz) - 1);
		wstring_ptr wstrValue(tcs2wcs(tsz));
		
		ArgumentDialog dialog(wstrName.get(), wstrValue.get());
		if (dialog.doModal(getHandle()) == IDOK) {
			W2T(dialog.getName(), ptszName);
			ListView_SetItemText(hwndList, nItem, 0, const_cast<LPTSTR>(ptszName));
			W2T(dialog.getValue(), ptszValue);
			ListView_SetItemText(hwndList, nItem, 1, const_cast<LPTSTR>(ptszValue));
		}
	}
	return 0;
}

LRESULT qm::CopyRuleTemplateDialog::onArgumentItemChanged(NMHDR* pnmhdr,
														  bool* pbHandled)
{
	updateState();
	*pbHandled = true;
	return 0;
}

void qm::CopyRuleTemplateDialog::updateState()
{
	HWND hwndList = getDlgItem(IDC_ARGUMENT);
	int nItem = ListView_GetNextItem(hwndList, -1, LVNI_SELECTED);
	Window(getDlgItem(IDC_EDIT)).enableWindow(nItem != -1);
	Window(getDlgItem(IDC_REMOVE)).enableWindow(nItem != -1);
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
	nIndexBlockSize_(-1)
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

unsigned int qm::CreateAccountDialog::getIndexBlockSize() const
{
	return nIndexBlockSize_;
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
	setDlgItemInt(IDC_INDEXBLOCKSIZE, 0);
	
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
	
	nIndexBlockSize_ = getDlgItemInt(IDC_INDEXBLOCKSIZE);
	if (nIndexBlockSize_ == 0)
		nIndexBlockSize_ = -1;
	
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

qm::CustomFilterDialog::CustomFilterDialog(const WCHAR* pwszCondition) :
	DefaultDialog(IDD_CUSTOMFILTER)
{
	if (pwszCondition)
		wstrCondition_ = allocWString(pwszCondition);
}

qm::CustomFilterDialog::~CustomFilterDialog()
{
}

const WCHAR* qm::CustomFilterDialog::getCondition() const
{
	return wstrCondition_.get();
}

LRESULT qm::CustomFilterDialog::onCommand(WORD nCode,
										  WORD nId)
{
	BEGIN_COMMAND_HANDLER()
		HANDLE_COMMAND_ID(IDC_EDIT, onEdit)
	END_COMMAND_HANDLER()
	return DefaultDialog::onCommand(nCode, nId);
}

LRESULT qm::CustomFilterDialog::onInitDialog(HWND hwndFocus,
											 LPARAM lParam)
{
	init(false);
	
	if (wstrCondition_.get())
		setDlgItemText(IDC_CONDITION, wstrCondition_.get());
	
	return TRUE;
}

LRESULT qm::CustomFilterDialog::onOk()
{
	wstrCondition_ = getDlgItemText(IDC_CONDITION);
	return DefaultDialog::onOk();
}

LRESULT qm::CustomFilterDialog::onEdit()
{
	wstring_ptr wstrCondition(getDlgItemText(IDC_CONDITION));
	ConditionDialog dialog(wstrCondition.get());
	if (dialog.doModal(getHandle()) == IDOK)
		setDlgItemText(IDC_CONDITION, dialog.getCondition());
	return 0;
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
	return NotifyHandler::onNotify(pnmhdr, pbHandled);
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
 * FilterDialog
 *
 */

qm::FilterDialog::FilterDialog(Filter* pFilter) :
	DefaultDialog(IDD_FILTER),
	pFilter_(pFilter)
{
}

qm::FilterDialog::~FilterDialog()
{
}

LRESULT qm::FilterDialog::onCommand(WORD nCode,
										   WORD nId)
{
	BEGIN_COMMAND_HANDLER()
		HANDLE_COMMAND_ID(IDC_EDIT, onEdit)
		HANDLE_COMMAND_ID_CODE(IDC_CONDITION, EN_CHANGE, onConditionChange)
		HANDLE_COMMAND_ID_CODE(IDC_NAME, EN_CHANGE, onNameChange)
	END_COMMAND_HANDLER()
	return DefaultDialog::onCommand(nCode, nId);
}

LRESULT qm::FilterDialog::onInitDialog(HWND hwndFocus,
											  LPARAM lParam)
{
	init(false);
	
	setDlgItemText(IDC_NAME, pFilter_->getName());
	
	const Macro* pCondition = pFilter_->getCondition();
	wstring_ptr wstrCondition(pCondition->getString());
	setDlgItemText(IDC_CONDITION, wstrCondition.get());
	
	return TRUE;
}

LRESULT qm::FilterDialog::onOk()
{
	wstring_ptr wstrCondition(getDlgItemText(IDC_CONDITION));
	std::auto_ptr<Macro> pCondition(MacroParser(MacroParser::TYPE_FILTER).parse(wstrCondition.get()));
	if (!pCondition.get()) {
		// TODO MSG
		return 0;
	}
	pFilter_->setCondition(pCondition);
	
	wstring_ptr wstrName(getDlgItemText(IDC_NAME));
	pFilter_->setName(wstrName.get());
	
	return DefaultDialog::onOk();
}

LRESULT qm::FilterDialog::onEdit()
{
	wstring_ptr wstrCondition(getDlgItemText(IDC_CONDITION));
	ConditionDialog dialog(wstrCondition.get());
	if (dialog.doModal(getHandle()) == IDOK)
		setDlgItemText(IDC_CONDITION, dialog.getCondition());
	return 0;
}

LRESULT qm::FilterDialog::onConditionChange()
{
	updateState();
	return 0;
}

LRESULT qm::FilterDialog::onNameChange()
{
	updateState();
	return 0;
}

void qm::FilterDialog::updateState()
{
	Window(getDlgItem(IDOK)).enableWindow(
		Window(getDlgItem(IDC_NAME)).getWindowTextLength() != 0 &&
		Window(getDlgItem(IDC_CONDITION)).getWindowTextLength() != 0);
}


/****************************************************************************
 *
 * FiltersDialog
 *
 */

qm::FiltersDialog::FiltersDialog(FilterManager* pManager) :
	AbstractListDialog<Filter, FilterManager::FilterList>(IDD_FILTERS, IDC_FILTERS),
	pManager_(pManager)
{
	const FilterManager::FilterList& l = pManager->getFilters();
	FilterManager::FilterList& list = getList();
	list.reserve(l.size());
	for (FilterManager::FilterList::const_iterator it = l.begin(); it != l.end(); ++it)
		list.push_back(new Filter(**it));
}

qm::FiltersDialog::~FiltersDialog()
{
}

LRESULT qm::FiltersDialog::onOk()
{
	pManager_->setFilters(getList());
	if (!pManager_->save()) {
		// TODO
	}
	return AbstractListDialog<Filter, FilterManager::FilterList>::onOk();
}

wstring_ptr qm::FiltersDialog::getLabel(const Filter* p) const
{
	return allocWString(p->getName());
}

std::auto_ptr<Filter> qm::FiltersDialog::create() const
{
	std::auto_ptr<Filter> pFilter(new Filter());
	FilterDialog dialog(pFilter.get());
	if (dialog.doModal(getHandle()) != IDOK)
		return std::auto_ptr<Filter>();
	return pFilter;
}

bool qm::FiltersDialog::edit(Filter* p) const
{
	FilterDialog dialog(p);
	return dialog.doModal(getHandle()) == IDOK;
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
 * FixedFormTextDialog
 *
 */

qm::FixedFormTextDialog::FixedFormTextDialog(FixedFormText* pText) :
	DefaultDialog(IDD_FIXEDFORMTEXT),
	pText_(pText)
{
}

qm::FixedFormTextDialog::~FixedFormTextDialog()
{
}

LRESULT qm::FixedFormTextDialog::onCommand(WORD nCode,
										   WORD nId)
{
	BEGIN_COMMAND_HANDLER()
		HANDLE_COMMAND_ID_CODE(IDC_NAME, EN_CHANGE, onNameChange)
	END_COMMAND_HANDLER()
	return DefaultDialog::onCommand(nCode, nId);
}

LRESULT qm::FixedFormTextDialog::onInitDialog(HWND hwndFocus,
											  LPARAM lParam)
{
	init(false);
	
	setDlgItemText(IDC_NAME, pText_->getName());
	
	wstring_ptr wstrText(Util::convertLFtoCRLF(pText_->getText()));
	setDlgItemText(IDC_TEXT, wstrText.get());
	
	return TRUE;
}

LRESULT qm::FixedFormTextDialog::onOk()
{
	wstring_ptr wstrName(getDlgItemText(IDC_NAME));
	
	wstring_ptr wstrText(getDlgItemText(IDC_TEXT));
	wstrText = Util::convertCRLFtoLF(wstrText.get());
	
	pText_->setName(wstrName.get());
	pText_->setText(wstrText.get());
	
	return DefaultDialog::onOk();
}

LRESULT qm::FixedFormTextDialog::onNameChange()
{
	updateState();
	return 0;
}

void qm::FixedFormTextDialog::updateState()
{
	Window(getDlgItem(IDOK)).enableWindow(
		Window(getDlgItem(IDC_NAME)).getWindowTextLength() != 0);
}


/****************************************************************************
*
* FixedFormTextsDialog
*
*/

qm::FixedFormTextsDialog::FixedFormTextsDialog(FixedFormTextManager* pManager) :
	AbstractListDialog<FixedFormText, FixedFormTextManager::TextList>(IDD_FIXEDFORMTEXTS, IDC_TEXTS),
	pManager_(pManager)
{
	const FixedFormTextManager::TextList& l = pManager->getTexts();
	FixedFormTextManager::TextList& list = getList();
	list.reserve(l.size());
	for (FixedFormTextManager::TextList::const_iterator it = l.begin(); it != l.end(); ++it)
		list.push_back(new FixedFormText(**it));
}

qm::FixedFormTextsDialog::~FixedFormTextsDialog()
{
}

LRESULT qm::FixedFormTextsDialog::onOk()
{
	pManager_->setTexts(getList());
	if (!pManager_->save()) {
		// TODO
	}
	return AbstractListDialog<FixedFormText, FixedFormTextManager::TextList>::onOk();
}

wstring_ptr qm::FixedFormTextsDialog::getLabel(const FixedFormText* p) const
{
	return allocWString(p->getName());
}

std::auto_ptr<FixedFormText> qm::FixedFormTextsDialog::create() const
{
	std::auto_ptr<FixedFormText> pText(new FixedFormText());
	FixedFormTextDialog dialog(pText.get());
	if (dialog.doModal(getHandle()) != IDOK)
		return std::auto_ptr<FixedFormText>();
	return pText;
}

bool qm::FixedFormTextsDialog::edit(FixedFormText* p) const
{
	FixedFormTextDialog dialog(p);
	return dialog.doModal(getHandle()) == IDOK;
}


/****************************************************************************
 *
 * GoRoundDialog
 *
 */

qm::GoRoundDialog::GoRoundDialog(GoRound* pGoRound,
								 Document* pDocument,
								 SyncFilterManager* pSyncFilterManager) :
	AbstractListDialog<GoRoundCourse, GoRound::CourseList>(IDD_GOROUND, IDC_COURSE),
	pGoRound_(pGoRound),
	pDocument_(pDocument),
	pSyncFilterManager_(pSyncFilterManager)
{
	const GoRound::CourseList& listCourse = pGoRound_->getCourses();
	GoRound::CourseList& list = getList();
	list.reserve(listCourse.size());
	for (GoRound::CourseList::const_iterator it = listCourse.begin(); it != listCourse.end(); ++it)
		list.push_back(new GoRoundCourse(**it));
}

qm::GoRoundDialog::~GoRoundDialog()
{
}

LRESULT qm::GoRoundDialog::onOk()
{
	pGoRound_->setCourses(getList());
	if (!pGoRound_->save()) {
		// TODO MSG
	}
	return AbstractListDialog<GoRoundCourse, GoRound::CourseList>::onOk();
}

wstring_ptr qm::GoRoundDialog::getLabel(const GoRoundCourse* p) const
{
	return allocWString(p->getName());
}

std::auto_ptr<GoRoundCourse> qm::GoRoundDialog::create() const
{
	std::auto_ptr<GoRoundCourse> pCourse(new GoRoundCourse());
	GoRoundCourseDialog dialog(pCourse.get(), pDocument_, pSyncFilterManager_);
	if (dialog.doModal(getHandle()) != IDOK)
		return std::auto_ptr<GoRoundCourse>();
	return pCourse;
}

bool qm::GoRoundDialog::edit(GoRoundCourse* p) const
{
	GoRoundCourseDialog dialog(p, pDocument_, pSyncFilterManager_);
	return dialog.doModal(getHandle()) == IDOK;
}


/****************************************************************************
 *
 * GoRoundCourseDialog
 *
 */

qm::GoRoundCourseDialog::GoRoundCourseDialog(GoRoundCourse* pCourse,
											 Document* pDocument,
											 SyncFilterManager* pSyncFilterManager) :
	AbstractListDialog<GoRoundEntry, GoRoundCourse::EntryList>(IDD_GOROUNDCOURSE, IDC_ENTRY),
	pCourse_(pCourse),
	pDocument_(pDocument),
	pSyncFilterManager_(pSyncFilterManager)
{
	const GoRoundCourse::EntryList& listEntry = pCourse_->getEntries();
	GoRoundCourse::EntryList& list = getList();
	list.reserve(listEntry.size());
	for (GoRoundCourse::EntryList::const_iterator it = listEntry.begin(); it != listEntry.end(); ++it)
		list.push_back(new GoRoundEntry(**it));
}

qm::GoRoundCourseDialog::~GoRoundCourseDialog()
{
}

LRESULT qm::GoRoundCourseDialog::onCommand(WORD nCode,
										   WORD nId)
{
	BEGIN_COMMAND_HANDLER()
		HANDLE_COMMAND_ID(IDC_DIALUP, onDialup)
		HANDLE_COMMAND_ID_CODE(IDC_NAME, EN_CHANGE, onNameChange)
	END_COMMAND_HANDLER()
	return AbstractListDialog<GoRoundEntry, GoRoundCourse::EntryList>::onCommand(nCode, nId);
}

LRESULT qm::GoRoundCourseDialog::onInitDialog(HWND hwndFocus,
											  LPARAM lParam)
{
	setDlgItemText(IDC_NAME, pCourse_->getName());
	
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
	
	return AbstractListDialog<GoRoundEntry, GoRoundCourse::EntryList>::onInitDialog(hwndFocus, lParam);
}

LRESULT qm::GoRoundCourseDialog::onOk()
{
	wstring_ptr wstrName(getDlgItemText(IDC_NAME));
	if (!wstrName.get())
		return 0;
	pCourse_->setName(wstrName.get());
	
	pCourse_->setEntries(getList());
	
	pCourse_->setType(sendDlgItemMessage(IDC_SEQUENTIAL, BM_GETCHECK) == BST_CHECKED ?
		GoRoundCourse::TYPE_SEQUENTIAL : GoRoundCourse::TYPE_PARALLEL);
	pCourse_->setFlags(sendDlgItemMessage(IDC_CONFIRM, BM_GETCHECK) == BST_CHECKED ?
		GoRoundCourse::FLAG_CONFIRM : 0);
	
	return AbstractListDialog<GoRoundEntry, GoRoundCourse::EntryList>::onOk();
}

wstring_ptr qm::GoRoundCourseDialog::getLabel(const GoRoundEntry* p) const
{
	StringBuffer<WSTRING> buf(p->getAccount());
	if (p->getSubAccount()) {
		buf.append(L'/');
		buf.append(p->getSubAccount());
	}
	if (p->getFolder()) {
		buf.append(L" [");
		buf.append(p->getFolder());
		buf.append(L']');
	}
	return buf.getString();
}

std::auto_ptr<GoRoundEntry> qm::GoRoundCourseDialog::create() const
{
	std::auto_ptr<GoRoundEntry> pEntry(new GoRoundEntry());
	GoRoundEntryDialog dialog(pEntry.get(), pDocument_, pSyncFilterManager_);
	if (dialog.doModal(getHandle()) != IDOK)
		return std::auto_ptr<GoRoundEntry>();
	return pEntry;
}

bool qm::GoRoundCourseDialog::edit(GoRoundEntry* p) const
{
	GoRoundEntryDialog dialog(p, pDocument_, pSyncFilterManager_);
	return dialog.doModal(getHandle()) == IDOK;
}

void qm::GoRoundCourseDialog::updateState()
{
	AbstractListDialog<GoRoundEntry, GoRoundCourse::EntryList>::updateState();
	
	wstring_ptr wstrName(getDlgItemText(IDC_NAME));
	Window(getDlgItem(IDOK)).enableWindow(*wstrName.get() != L'\0');
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
		HANDLE_COMMAND_ID(IDC_EDIT, onEdit)
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
	sendDlgItemMessage(IDC_SYNCFILTER, CB_SETDROPPEDWIDTH, 150);
	
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

LRESULT qm::GoRoundEntryDialog::onEdit()
{
	SyncFilterSetsDialog dialog(pSyncFilterManager_);
	if (dialog.doModal(getHandle()) == IDOK)
		updateFilter();
	return 0;
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
	updateFilter();
	
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

void qm::GoRoundEntryDialog::updateFilter()
{
	HINSTANCE hInst = Application::getApplication().getResourceHandle();
	
	wstring_ptr wstrFilter(getDlgItemText(IDC_SYNCFILTER));
	
	sendDlgItemMessage(IDC_SYNCFILTER, CB_RESETCONTENT);
	
	sendDlgItemMessage(IDC_SYNCFILTER, CB_ADDSTRING,
		0, reinterpret_cast<LPARAM>(_T("")));
	const SyncFilterManager::FilterSetList& l = pSyncFilterManager_->getFilterSets();
	for (SyncFilterManager::FilterSetList::const_iterator it = l.begin(); it != l.end(); ++it) {
		SyncFilterSet* pSet = *it;
		W2T(pSet->getName(), ptszName);
		sendDlgItemMessage(IDC_SYNCFILTER, CB_ADDSTRING,
			0, reinterpret_cast<LPARAM>(ptszName));
	}
	
	setDlgItemText(IDC_SYNCFILTER, wstrFilter.get());
}


/****************************************************************************
 *
 * ImportDialog
 *
 */

qm::ImportDialog::ImportDialog(const WCHAR* pwszPath,
							   Profile* pProfile) :
	DefaultDialog(IDD_IMPORT),
	pProfile_(pProfile),
	bMultiple_(false),
	nFlags_(0)
{
	if (pwszPath)
		wstrPath_ = allocWString(pwszPath);
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
	END_COMMAND_HANDLER()
	return DefaultDialog::onCommand(nCode, nId);
}

LRESULT qm::ImportDialog::onInitDialog(HWND hwndFocus,
									   LPARAM lParam)
{
	init(false);
	
	if (wstrPath_.get())
		setDlgItemText(IDC_PATH, wstrPath_.get());
	
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
	
	if (dialog.doModal(getHandle()) != IDOK)
		return 0;
	
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
	
	return 0;
}

LRESULT qm::ImportDialog::onPathChange()
{
	updateState();
	return 0;
}

void qm::ImportDialog::updateState()
{
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
 * MailFolderDialog
 *
 */

qm::MailFolderDialog::MailFolderDialog(HINSTANCE hInstResource,
									   const WCHAR* pwszMailFolder) :
	DefaultDialog(hInstResource, IDD_MAILFOLDER)
{
	if (pwszMailFolder)
		wstrMailFolder_ = allocWString(pwszMailFolder);
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
	
	if (wstrMailFolder_.get())
		setDlgItemText(IDC_MAILFOLDER, wstrMailFolder_.get());
	
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
										 Account* pAccount,
										 Profile* pProfile) :
	DefaultDialog(IDD_MOVEMESSAGE),
	pDocument_(pDocument),
	pAccount_(pAccount),
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
	return NotifyHandler::onNotify(pnmhdr, pbHandled);
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
	
	Folder* pFolderSelected = 0;
	wstring_ptr wstrFolder(pAccount_->getProperty(L"UI", L"FolderTo", L""));
	if (*wstrFolder.get())
		pFolderSelected = pDocument_->getFolder(0, wstrFolder.get());
	
	update(pFolderSelected);
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
	
	wstring_ptr wstrFolderName(pFolder_->getFullName());
	ConcatW c[] = {
		{ L"//",								2	},
		{ pFolder_->getAccount()->getName(),	-1	},
		{ L"/",									1	},
		{ wstrFolderName.get(),					-1	}
	};
	wstring_ptr wstrName(concat(c, countof(c)));
	pAccount_->setProperty(L"UI", L"FolderTo", wstrName.get());
	
	return DefaultDialog::onOk();
}

LRESULT qm::MoveMessageDialog::onShowHidden()
{
	bool bShowHidden = sendDlgItemMessage(IDC_SHOWHIDDEN, BM_GETCHECK) == BST_CHECKED;
	if (bShowHidden != bShowHidden_) {
		bShowHidden_ = bShowHidden;
		
		Folder* pFolderSelected = 0;
		
		HWND hwndFolder = getDlgItem(IDC_FOLDER);
		HTREEITEM hItem = TreeView_GetSelection(hwndFolder);
		if (hItem && TreeView_GetParent(hwndFolder, hItem)) {
			TVITEM item = {
				TVIF_HANDLE | TVIF_PARAM,
				hItem
			};
			TreeView_GetItem(hwndFolder, &item);
			pFolderSelected = reinterpret_cast<Folder*>(item.lParam);
		}
		
		update(pFolderSelected);
	}
	return 0;
}

LRESULT qm::MoveMessageDialog::onFolderSelChanged(NMHDR* pnmhdr, bool* pbHandled)
{
	updateState();
	*pbHandled = true;
	return 0;
}

bool qm::MoveMessageDialog::update(Folder* pFolderSelected)
{
	HWND hwndFolder = getDlgItem(IDC_FOLDER);
	{
		DisableRedraw disable(hwndFolder);
		
		TreeView_DeleteAllItems(hwndFolder);
		
		const Document::AccountList& listAccount = pDocument_->getAccounts();
		for (Document::AccountList::const_iterator it = listAccount.begin(); it != listAccount.end(); ++it) {
			if (!insertAccount(hwndFolder, *it, pFolderSelected))
				return false;
		}
	}
	
	HTREEITEM hItem = TreeView_GetSelection(hwndFolder);
	if (hItem)
		TreeView_EnsureVisible(hwndFolder, hItem);
	
	return true;
}

bool qm::MoveMessageDialog::insertAccount(HWND hwnd,
										  Account* pAccount,
										  Folder* pFolderSelected)
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
	HTREEITEM hItemAccount = TreeView_InsertItem(hwnd, &tvisAccount);
	if (!hItemAccount)
		return false;
	
	if (!insertFolders(hwnd, hItemAccount, pAccount, pFolderSelected))
		return false;
	
	return true;
}

bool qm::MoveMessageDialog::insertFolders(HWND hwnd,
										  HTREEITEM hItem,
										  Account* pAccount,
										  Folder* pFolderSelected)
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
		
		HTREEITEM hItemFolder = TreeView_InsertItem(hwnd, &tvisFolder);
		if (!hItemFolder)
			return false;
		
		if (pFolder == pFolderSelected)
			TreeView_SelectItem(hwnd, hItemFolder);
		
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
 * PasswordDialog
 *
 */

qm::PasswordDialog::PasswordDialog(const WCHAR* pwszHint) :
	DefaultDialog(IDD_PASSWORD),
	state_(PASSWORDSTATE_ONETIME)
{
	wstrHint_ = allocWString(pwszHint);
}

qm::PasswordDialog::~PasswordDialog()
{
}

const WCHAR* qm::PasswordDialog::getPassword() const
{
	return wstrPassword_.get();
}

PasswordState qm::PasswordDialog::getState() const
{
	return state_;
}

LRESULT qm::PasswordDialog::onCommand(WORD nCode,
									  WORD nId)
{
	BEGIN_COMMAND_HANDLER()
		HANDLE_COMMAND_ID_CODE(IDC_PASSWORD, EN_CHANGE, onPasswordChange)
	END_COMMAND_HANDLER()
	return DefaultDialog::onCommand(nCode, nId);
}

LRESULT qm::PasswordDialog::onInitDialog(HWND hwndFocus,
										 LPARAM lParam)
{
	init(false);
	
	setDlgItemText(IDC_HINT, wstrHint_.get());
	sendDlgItemMessage(IDC_SESSION, BM_SETCHECK, BST_CHECKED);
	
	return TRUE;
}

LRESULT qm::PasswordDialog::onOk()
{
	wstrPassword_ = getDlgItemText(IDC_PASSWORD);
	
	if (sendDlgItemMessage(IDC_DONTSAVE, BM_GETCHECK) == BST_CHECKED)
		state_ = PASSWORDSTATE_ONETIME;
	else if (sendDlgItemMessage(IDC_SESSION, BM_GETCHECK) == BST_CHECKED)
		state_ = PASSWORDSTATE_SESSION;
	else if (sendDlgItemMessage(IDC_SAVE, BM_GETCHECK) == BST_CHECKED)
		state_ = PASSWORDSTATE_SAVE;
	
	return DefaultDialog::onOk();
}

LRESULT qm::PasswordDialog::onPasswordChange()
{
	updateState();
	return 0;
}

void qm::PasswordDialog::updateState()
{
	Window(getDlgItem(IDOK)).enableWindow(
		Window(getDlgItem(IDC_PASSWORD)).getWindowTextLength() != 0);
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

bool qm::ProgressDialog::isCanceled()
{
	HWND hwnd = getHandle();
	if (sendDlgItemMessage(IDC_PROGRESS, PBM_GETPOS) % 10 == 0)
		hwnd = 0;
	
	MSG msg;
	while (::PeekMessage(&msg, hwnd, 0, 0, PM_REMOVE)) {
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
	Window(getDlgItem(IDOK)).enableWindow(
		Window(getDlgItem(IDC_NAME)).getWindowTextLength() != 0);
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

LRESULT qm::ResourceDialog::onCommand(WORD nCode,
									  WORD nId)
{
	BEGIN_COMMAND_HANDLER()
		HANDLE_COMMAND_ID(IDC_CHECKALL, onCheckAll)
		HANDLE_COMMAND_ID(IDC_CLEARALL, onClearAll)
	END_COMMAND_HANDLER()
	return DefaultDialog::onCommand(nCode, nId);
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

LRESULT qm::ResourceDialog::onCheckAll()
{
	HWND hwnd = getDlgItem(IDC_RESOURCE);
	
	for (int n = 0; n < ListView_GetItemCount(hwnd); ++n)
		ListView_SetCheckState(hwnd, n, TRUE);
	
	return 0;
}

LRESULT qm::ResourceDialog::onClearAll()
{
	HWND hwnd = getDlgItem(IDC_RESOURCE);
	
	for (int n = 0; n < ListView_GetItemCount(hwnd); ++n)
		ListView_SetCheckState(hwnd, n, FALSE);
	
	return 0;
}


/****************************************************************************
 *
 * RuleDialog
 *
 */

qm::RuleDialog::RuleDialog(Rule* pRule,
						   Document* pDocument) :
	DefaultDialog(IDD_RULE),
	pRule_(pRule),
	pDocument_(pDocument),
	bInit_(false)
{
	RuleAction* pAction = pRule_->getAction();
	if (pAction &&
		(pAction->getType() == RuleAction::TYPE_MOVE ||
		pAction->getType() == RuleAction::TYPE_COPY)) {
		CopyRuleAction* pCopyAction = static_cast<CopyRuleAction*>(pAction);
		
		if (pCopyAction->getTemplate())
			wstrTemplate_ = allocWString(pCopyAction->getTemplate());
		
		const CopyRuleAction::ArgumentList& l = pCopyAction->getArguments();
		listArgument_.reserve(l.size());
		for (CopyRuleAction::ArgumentList::const_iterator it = l.begin(); it != l.end(); ++it)
			listArgument_.push_back(CopyRuleAction::ArgumentList::value_type(
				allocWString((*it).first).release(), allocWString((*it).second).release()));
	}
}

qm::RuleDialog::~RuleDialog()
{
	for (CopyRuleAction::ArgumentList::iterator it = listArgument_.begin(); it != listArgument_.end(); ++it) {
		freeWString((*it).first);
		freeWString((*it).second);
	}
}

LRESULT qm::RuleDialog::onCommand(WORD nCode,
								  WORD nId)
{
	BEGIN_COMMAND_HANDLER()
		HANDLE_COMMAND_ID(IDC_EDIT, onEdit)
		HANDLE_COMMAND_ID(IDC_TEMPLATE, onTemplate)
		HANDLE_COMMAND_ID_CODE(IDC_ACCOUNT, CBN_EDITCHANGE, onAccountEditChange)
		HANDLE_COMMAND_ID_CODE(IDC_ACCOUNT, CBN_SELCHANGE, onAccountSelChange)
		HANDLE_COMMAND_ID_CODE(IDC_ACTION, CBN_SELCHANGE, onActionSelChange)
		HANDLE_COMMAND_ID_CODE(IDC_CONDITION, EN_CHANGE, onConditionChange)
		HANDLE_COMMAND_ID_CODE(IDC_FOLDER, CBN_EDITCHANGE, onFolderEditChange)
		HANDLE_COMMAND_ID_CODE(IDC_FOLDER, CBN_SELCHANGE, onFolderSelChange)
		HANDLE_COMMAND_ID_CODE(IDC_MACRO, EN_CHANGE, onMacroChange)
	END_COMMAND_HANDLER()
	return DefaultDialog::onCommand(nCode, nId);
}

LRESULT qm::RuleDialog::onInitDialog(HWND hwndFocus,
									 LPARAM lParam)
{
	init(false);
	
	const Macro* pCondition = pRule_->getCondition();
	if (pCondition) {
		wstring_ptr wstrCondition(pCondition->getString());
		setDlgItemText(IDC_CONDITION, wstrCondition.get());
	}
	
	const WCHAR* pwszTypes[] = {
		L"None",
		L"Move",
		L"Copy",
		L"Delete",
		L"DeleteCache",
		L"Apply"
	};
	for (int n = 0; n < countof(pwszTypes); ++n) {
		W2T(pwszTypes[n], ptszType);
		sendDlgItemMessage(IDC_ACTION, CB_ADDSTRING,
			0, reinterpret_cast<LPARAM>(ptszType));
	}
	
	const Document::AccountList& listAccount = pDocument_->getAccounts();
	for (Document::AccountList::const_iterator it = listAccount.begin(); it != listAccount.end(); ++it) {
		Account* pAccount = *it;
		W2T(pAccount->getName(), ptszName);
		sendDlgItemMessage(IDC_ACCOUNT, CB_ADDSTRING,
			0, reinterpret_cast<LPARAM>(ptszName));
	}
	
	int nItem = 0;
	RuleAction* pAction = pRule_->getAction();
	if (pAction) {
		RuleAction::Type type = pAction->getType();
		switch (type) {
		case RuleAction::TYPE_MOVE:
		case RuleAction::TYPE_COPY:
			{
				nItem = type == RuleAction::TYPE_MOVE ? 1 : 2;
				
				CopyRuleAction* pCopy = static_cast<CopyRuleAction*>(pAction);
				const WCHAR* pwszAccount = pCopy->getAccount();
				if (pwszAccount)
					setDlgItemText(IDC_ACCOUNT, pwszAccount);
				setDlgItemText(IDC_FOLDER, pCopy->getFolder());
			}
			break;
		case RuleAction::TYPE_DELETE:
			{
				nItem = 3;
				
				bool bDirect = static_cast<DeleteRuleAction*>(pAction)->isDirect();
				sendDlgItemMessage(IDC_DIRECT, BM_SETCHECK,
					bDirect ? BST_CHECKED : BST_UNCHECKED);
			}
			break;
		case RuleAction::TYPE_DELETECACHE:
			nItem = 4;
			break;
		case RuleAction::TYPE_APPLY:
			{
				nItem = 5;
				
				wstring_ptr wstrMacro(static_cast<ApplyRuleAction*>(pAction)->getMacro()->getString());
				setDlgItemText(IDC_MACRO, wstrMacro.get());
			}
			break;
		default:
			assert(false);
			break;
		}
	}
	sendDlgItemMessage(IDC_ACTION, CB_SETCURSEL, nItem);
	
	bInit_ = true;
	
	updateState(true);
	
	return TRUE;
}

LRESULT qm::RuleDialog::onOk()
{
	wstring_ptr wstrCondition(getDlgItemText(IDC_CONDITION));
	std::auto_ptr<Macro> pCondition(MacroParser(MacroParser::TYPE_RULE).parse(wstrCondition.get()));
	if (!pCondition.get()) {
		// TODO MSG
		return 0;
	}
	pRule_->setCondition(pCondition);
	
	std::auto_ptr<RuleAction> pAction;
	int nItem = sendDlgItemMessage(IDC_ACTION, CB_GETCURSEL);
	switch (nItem) {
	case 0:
		break;
	case 1:
	case 2:
		{
			wstring_ptr wstrAccount(getDlgItemText(IDC_ACCOUNT));
			if (!*wstrAccount.get())
				wstrAccount.reset(0);
			wstring_ptr wstrFolder(getDlgItemText(IDC_FOLDER));
			if (!*wstrFolder.get())
				return 0;
			
			std::auto_ptr<CopyRuleAction> pCopyAction(new CopyRuleAction(
				wstrAccount.get(), wstrFolder.get(), nItem == 1));
			if (wstrTemplate_.get()) {
				pCopyAction->setTemplate(wstrTemplate_.get());
				pCopyAction->setTemplateArguments(listArgument_);
			}
			
			pAction.reset(pCopyAction.release());
		}
		break;
	case 3:
		{
			bool bDirect = sendDlgItemMessage(IDC_DIRECT, BM_GETCHECK) == BST_CHECKED;
			pAction.reset(new DeleteRuleAction(bDirect));
		}
		break;
	case 4:
		pAction.reset(new DeleteCacheRuleAction());
		break;
	case 5:
		{
			wstring_ptr wstrMacro(getDlgItemText(IDC_MACRO));
			std::auto_ptr<Macro> pMacro(MacroParser(MacroParser::TYPE_RULE).parse(wstrMacro.get()));
			if (!pMacro.get()) {
				// TODO MSG
				return 0;
			}
			pAction.reset(new ApplyRuleAction(pMacro));
		}
		break;
	default:
		assert(false);
		break;
	}
	pRule_->setAction(pAction);
	
	return DefaultDialog::onOk();
}

LRESULT qm::RuleDialog::onEdit()
{
	wstring_ptr wstrCondition(getDlgItemText(IDC_CONDITION));
	ConditionDialog dialog(wstrCondition.get());
	if (dialog.doModal(getHandle()) == IDOK)
		setDlgItemText(IDC_CONDITION, dialog.getCondition());
	return 0;
}

LRESULT qm::RuleDialog::onTemplate()
{
	CopyRuleTemplateDialog dialog(wstrTemplate_.get(), &listArgument_);
	if (dialog.doModal(getHandle()) == IDOK) {
		const WCHAR* pwszTemplate = dialog.getName();
		if (pwszTemplate)
			wstrTemplate_ = allocWString(pwszTemplate);
		else
			wstrTemplate_.reset(0);
	}
	return 0;
}

LRESULT qm::RuleDialog::onActionSelChange()
{
	updateState(true);
	return 0;
}

LRESULT qm::RuleDialog::onAccountEditChange()
{
	updateState(true);
	return 0;
}

LRESULT qm::RuleDialog::onAccountSelChange()
{
	postMessage(WM_COMMAND, MAKEWPARAM(IDC_ACCOUNT, CBN_EDITCHANGE));
	return 0;
}

LRESULT qm::RuleDialog::onConditionChange()
{
	updateState(false);
	return 0;
}

LRESULT qm::RuleDialog::onFolderEditChange()
{
	updateState(false);
	return 0;
}

LRESULT qm::RuleDialog::onFolderSelChange()
{
	postMessage(WM_COMMAND, MAKEWPARAM(IDC_FOLDER, CBN_EDITCHANGE));
	return 0;
}

LRESULT qm::RuleDialog::onMacroChange()
{
	updateState(false);
	return 0;
}

void qm::RuleDialog::updateState(bool bUpdateFolder)
{
	if (!bInit_)
		return;
	
	struct {
		UINT nId_;
		bool b_;
	} items[] = {
		{ IDC_ACCOUNTLABEL,	false	},
		{ IDC_ACCOUNT,		false	},
		{ IDC_FOLDERLABEL,	false	},
		{ IDC_FOLDER,		false	},
		{ IDC_TEMPLATE,		false	},
		{ IDC_DIRECT,		false	},
		{ IDC_MACROLABEL,	false	},
		{ IDC_MACRO,		false	}
	};
	
	int nStart = 0;
	int nEnd = 0;
	bool bEnable = true;
	switch (sendDlgItemMessage(IDC_ACTION, CB_GETCURSEL)) {
	case 0:
		break;
	case 1:
	case 2:
		nStart = 0;
		nEnd = 5;
		bEnable = Window(getDlgItem(IDC_FOLDER)).getWindowTextLength() != 0;
		break;
	case 3:
		nStart = 5;
		nEnd = 6;
		break;
	case 4:
		break;
	case 5:
		nStart = 6;
		nEnd = 8;
		bEnable = Window(getDlgItem(IDC_MACRO)).getWindowTextLength() != 0;
		break;
	default:
		assert(false);
		break;
	}
	for (int n = nStart; n < nEnd; ++n)
		items[n].b_ = true;
	for (int n = 0; n < countof(items); ++n)
		Window(getDlgItem(items[n].nId_)).showWindow(items[n].b_ ? SW_SHOW : SW_HIDE);
	
	Window(getDlgItem(IDOK)).enableWindow(
		bEnable && Window(getDlgItem(IDC_CONDITION)).getWindowTextLength() != 0);
		
	
	if (bUpdateFolder) {
		Account* pAccount = 0;
		wstring_ptr wstrAccount(getDlgItemText(IDC_ACCOUNT));
		if (wstrAccount.get())
			pAccount = pDocument_->getAccount(wstrAccount.get());
		updateFolder(pAccount);
	}
}

void qm::RuleDialog::updateFolder(Account* pAccount)
{
	wstring_ptr wstrFolder(getDlgItemText(IDC_FOLDER));
	
	sendDlgItemMessage(IDC_FOLDER, CB_RESETCONTENT);
	
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


/****************************************************************************
 *
 * ColorSetsDialog
 *
 */

qm::ColorSetsDialog::ColorSetsDialog(ColorManager* pColorManager,
									 Document* pDocument) :
	RuleColorSetsDialog<ColorSet, ColorManager::ColorSetList, ColorManager, ColorsDialog>(
		pColorManager, pDocument, IDS_COLORSETS, &ColorManager::getColorSets, &ColorManager::setColorSets)
{
}


/****************************************************************************
 *
 * RuleSetsDialog
 *
 */

qm::RuleSetsDialog::RuleSetsDialog(RuleManager* pRuleManager,
								   Document* pDocument) :
	RuleColorSetsDialog<RuleSet, RuleManager::RuleSetList, RuleManager, RulesDialog>(
		pRuleManager, pDocument, IDS_RULESETS, &RuleManager::getRuleSets, &RuleManager::setRuleSets)
{
}


/****************************************************************************
 *
 * ColorsDialog
 *
 */

qm::ColorsDialog::ColorsDialog(ColorSet* pColorSet,
							   Document* pDocument) :
	RulesColorsDialog<ColorEntry, ColorSet::ColorList, ColorSet, ColorDialog>(
		pColorSet, pDocument, IDS_COLORS, &ColorSet::getColors, &ColorSet::setColors)
{
}


/****************************************************************************
 *
 * RulesDialog
 *
 */

qm::RulesDialog::RulesDialog(RuleSet* pRuleSet,
							 Document* pDocument) :
	RulesColorsDialog<Rule, RuleSet::RuleList, RuleSet, RuleDialog>(
		pRuleSet, pDocument, IDS_RULES, &RuleSet::getRules, &RuleSet::setRules)
{
}


/****************************************************************************
 *
 * SelectAddressDialog
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
	switch (lParamSort & SelectAddressDialog::SORT_TYPE_MASK) {
	case SelectAddressDialog::SORT_NAME:
		p1 = pAddress1->getEntry()->getActualSortKey();
		p2 = pAddress2->getEntry()->getActualSortKey();
		break;
	case SelectAddressDialog::SORT_ADDRESS:
		p1 = pAddress1->getAddress();
		p2 = pAddress2->getAddress();
		break;
	case SelectAddressDialog::SORT_COMMENT:
		p1 = pAddress1->getComment();
		p2 = pAddress2->getComment();
		break;
	default:
		assert(false);
		break;
	}
	
	int nComp = p1 == p2 ? 0 : !p1 ? -1 : !p2 ? 1 : _wcsicmp(p1, p2);
	return (lParamSort & SelectAddressDialog::SORT_DIRECTION_MASK)
		== SelectAddressDialog::SORT_ASCENDING ? nComp : -nComp;
}

int CALLBACK selectedItemComp(LPARAM lParam1,
							  LPARAM lParam2,
							  LPARAM lParamSort)
{
	SelectAddressDialog::Item* pItem1 = reinterpret_cast<SelectAddressDialog::Item*>(lParam1);
	SelectAddressDialog::Item* pItem2 = reinterpret_cast<SelectAddressDialog::Item*>(lParam2);
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

qm::SelectAddressDialog::SelectAddressDialog(AddressBook* pAddressBook,
											 Profile* pProfile,
											 const WCHAR* pwszAddress[]) :
	DefaultDialog(IDD_SELECTADDRESS),
	pAddressBook_(pAddressBook),
	pProfile_(pProfile),
	nSort_(SORT_NAME | SORT_ASCENDING),
	wndAddressList_(this),
	wndSelectedAddressList_(this)
{
	pAddressBook_->reload();
	
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

qm::SelectAddressDialog::~SelectAddressDialog()
{
	for (int n = 0; n < countof(listAddress_); ++n)
		std::for_each(listAddress_[n].begin(),
			listAddress_[n].end(), string_free<WSTRING>());
}

const SelectAddressDialog::AddressList& qm::SelectAddressDialog::getAddresses(Type type) const
{
	return listAddress_[type];
}

INT_PTR qm::SelectAddressDialog::dialogProc(UINT uMsg,
											WPARAM wParam,
											LPARAM lParam)
{
	BEGIN_DIALOG_HANDLER()
		HANDLE_SIZE()
	END_DIALOG_HANDLER()
	return DefaultDialog::dialogProc(uMsg, wParam, lParam);
}

LRESULT qm::SelectAddressDialog::onCommand(WORD nCode,
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

LRESULT qm::SelectAddressDialog::onDestroy()
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
	
	return DefaultDialog::onDestroy();
}

LRESULT qm::SelectAddressDialog::onInitDialog(HWND hwndFocus,
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
	wndSelectedAddressList_.subclassWindow(hwndSelected);
	Window(hwndList).setFocus();
	
	return FALSE;
}

LRESULT qm::SelectAddressDialog::onOk()
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

LRESULT qm::SelectAddressDialog::onNotify(NMHDR* pnmhdr,
										  bool* pbHandled)
{
	BEGIN_NOTIFY_HANDLER()
		HANDLE_NOTIFY(LVN_COLUMNCLICK, IDC_ADDRESS, onAddressColumnClick)
		HANDLE_NOTIFY(NM_DBLCLK, IDC_ADDRESS, onAddressDblClk)
	END_NOTIFY_HANDLER()
	return NotifyHandler::onNotify(pnmhdr, pbHandled);
}

LRESULT qm::SelectAddressDialog::onSize(UINT nFlags,
										int cx,
										int cy)
{
	layout();
	return 0;
}

LRESULT qm::SelectAddressDialog::onCategory()
{
	RECT rect;
	Window(getDlgItem(IDC_CATEGORY)).getWindowRect(&rect);
	
	AddressBook::CategoryList listCategory(pAddressBook_->getCategories());
	std::sort(listCategory.begin(), listCategory.end(), AddressBookCategoryLess());
	
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

LRESULT qm::SelectAddressDialog::onSelect(UINT nId)
{
	Type types[] = { TYPE_TO, TYPE_CC, TYPE_BCC };
	select(types[nId - IDC_TO]);
	return 0;
}

LRESULT qm::SelectAddressDialog::onRemove()
{
	remove();
	return 0;
}

#if !defined _WIN32_WCE || _WIN32_WCE < 300 || !defined _WIN32_WCE_PSPC
LRESULT qm::SelectAddressDialog::onFilterChange()
{
	wstring_ptr wstrFilter(getDlgItemText(IDC_FILTER));
	wstrFilter_ = tolower(wstrFilter.get());
	update();
	return 0;
}
#endif

LRESULT qm::SelectAddressDialog::onAddressColumnClick(NMHDR* pnmhdr,
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

LRESULT qm::SelectAddressDialog::onAddressDblClk(NMHDR* pnmhdr,
												 bool* pbHandled)
{
	select(TYPE_TO);
	return 0;
}

void qm::SelectAddressDialog::update()
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

void qm::SelectAddressDialog::select(Type type)
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

void qm::SelectAddressDialog::remove()
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

void qm::SelectAddressDialog::layout()
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
	
	HDWP hdwp = beginDeferWindowPos(12);
	hdwp = Window(getDlgItem(IDC_CATEGORY)).deferWindowPos(hdwp, 0, 5, 5,
		nLeftWidth, nButtonHeight, SWP_NOZORDER | SWP_NOACTIVATE);
	hdwp = Window(getDlgItem(IDC_FILTER)).deferWindowPos(hdwp, 0, nLeftWidth + 5*2,
		5, nRightWidth, nButtonHeight, SWP_NOZORDER | SWP_NOACTIVATE);
	hdwp = Window(getDlgItem(IDC_ADDRESS)).deferWindowPos(hdwp, 0, 5, nButtonHeight + 5*2,
		nLeftWidth, nHeight - nButtonHeight - 5*3, SWP_NOZORDER | SWP_NOACTIVATE);
	hdwp = Window(getDlgItem(IDC_SELECTEDADDRESS)).deferWindowPos(
		hdwp, 0, nLeftWidth + 5*2, nButtonHeight + 5*2, nRightWidth,
		nHeight - nButtonHeight - 5*3, SWP_NOZORDER | SWP_NOACTIVATE);
	
	int nDx[] = { 1, 5, 1, 1, 5, 0 };
	int nTop = 5;
	for (int n = 0; n < countof(hwnds); ++n) {
		hdwp = Window(hwnds[n]).deferWindowPos(hdwp, 0, nLeftWidth + nRightWidth + 5*3,
			nTop, 0, 0, SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE);
		nTop += nButtonHeight + nDx[n];
	}
	
	hdwp = Window(getDlgItem(IDC_FILTERLABEL)).deferWindowPos(
		hdwp, 0, 0, 0, 0, 0, SWP_NOZORDER | SWP_NOACTIVATE);
	
#ifndef _WIN32_WCE
	hdwp = Window(getDlgItem(IDC_SIZEGRIP)).deferWindowPos(hdwp, 0,
		rect.right - rect.left - 13, rect.bottom - rect.top - 12,
		0, 0, SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE);
#endif
	endDeferWindowPos(hdwp);
#endif
}

HMENU qm::SelectAddressDialog::createCategoryMenu(const AddressBook::CategoryList& l,
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

void qm::SelectAddressDialog::setCurrentCategory(const WCHAR* pwszCategory)
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

bool qm::SelectAddressDialog::isCategory(const AddressBookAddress::CategoryList& listCategory) const
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

bool qm::SelectAddressDialog::isMatchFilter(const AddressBookEntry* pEntry) const
{
	if (!wstrFilter_.get())
		return true;
	
	wstring_ptr wstrName(tolower(pEntry->getName()));
	return wcsstr(wstrName.get(), wstrFilter_.get()) != 0;
}

bool qm::SelectAddressDialog::isMatchFilter(const AddressBookAddress* pAddress) const
{
	if (!wstrFilter_.get())
		return true;
	return wcsstr(pAddress->getAddress(), wstrFilter_.get()) != 0;
}

size_t qm::SelectAddressDialog::getCategoryLevel(const WCHAR* pwszCategory)
{
	assert(pwszCategory);
	return std::count(pwszCategory, pwszCategory + wcslen(pwszCategory), L'/') + 1;
}

wstring_ptr qm::SelectAddressDialog::getCategoryName(const WCHAR* pwszCategory,
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
 * SelectAddressDialog::Item
 *
 */

qm::SelectAddressDialog::Item::Item(wstring_ptr wstrValue,
									Type type) :
	wstrValue_(wstrValue),
	type_(type)
{
}

qm::SelectAddressDialog::Item::~Item()
{
}

const WCHAR* qm::SelectAddressDialog::Item::getValue() const
{
	return wstrValue_.get();
}

wstring_ptr qm::SelectAddressDialog::Item::releaseValue()
{
	return wstrValue_;
}

SelectAddressDialog::Type qm::SelectAddressDialog::Item::getType() const
{
	return type_;
}

void qm::SelectAddressDialog::Item::setType(Type type)
{
	type_ = type;
}


/****************************************************************************
 *
 * SelectAddressDialog::AddressListWindow
 *
 */

qm::SelectAddressDialog::AddressListWindow::AddressListWindow(SelectAddressDialog* pDialog) :
	WindowBase(false),
	pDialog_(pDialog)
{
	setWindowHandler(this, false);
}

qm::SelectAddressDialog::AddressListWindow::~AddressListWindow()
{
}

LRESULT qm::SelectAddressDialog::AddressListWindow::windowProc(UINT uMsg,
															   WPARAM wParam,
															   LPARAM lParam)
{
	BEGIN_MESSAGE_HANDLER()
		HANDLE_CHAR()
	END_MESSAGE_HANDLER()
	return DefaultWindowHandler::windowProc(uMsg, wParam, lParam);
}

LRESULT qm::SelectAddressDialog::AddressListWindow::onChar(UINT nChar,
														   UINT nRepeat,
														   UINT nFlags)
{
	if (nChar == L' ') {
		pDialog_->select(SelectAddressDialog::TYPE_TO);
		return 0;
	}
	return DefaultWindowHandler::onChar(nChar, nRepeat, nFlags);
}


/****************************************************************************
 *
 * SelectAddressDialog::SelectedAddressListWindow
 *
 */

qm::SelectAddressDialog::SelectedAddressListWindow::SelectedAddressListWindow(SelectAddressDialog* pDialog) :
	WindowBase(false),
	pDialog_(pDialog)
{
	setWindowHandler(this, false);
}

qm::SelectAddressDialog::SelectedAddressListWindow::~SelectedAddressListWindow()
{
}

bool qm::SelectAddressDialog::SelectedAddressListWindow::preSubclassWindow()
{
#if defined _WIN32_WCE && _WIN32_WCE >= 400 && defined _WIN32_WCE_PSPC
	pDialog_->addNotifyHandler(this);
#endif
	return true;
}

LRESULT qm::SelectAddressDialog::SelectedAddressListWindow::windowProc(UINT uMsg,
																	   WPARAM wParam,
																	   LPARAM lParam)
{
	BEGIN_MESSAGE_HANDLER()
		HANDLE_CONTEXTMENU()
		HANDLE_DESTROY()
		HANDLE_LBUTTONDOWN()
	END_MESSAGE_HANDLER()
	return DefaultWindowHandler::windowProc(uMsg, wParam, lParam);
}

LRESULT qm::SelectAddressDialog::SelectedAddressListWindow::onContextMenu(HWND hwnd,
																		  const POINT& pt)
{
	HINSTANCE hInst = Application::getApplication().getResourceHandle();
	AutoMenuHandle hmenu(::LoadMenu(hInst, MAKEINTRESOURCE(IDR_ADDRESSBOOK)));
	HMENU hmenuSub = ::GetSubMenu(hmenu.get(), 0);
	UINT nFlags = TPM_LEFTALIGN | TPM_TOPALIGN | TPM_RETURNCMD;
#ifndef _WIN32_WCE
	nFlags |= TPM_LEFTBUTTON | TPM_RIGHTBUTTON;
#endif
	UINT nId = ::TrackPopupMenu(hmenuSub, nFlags, pt.x, pt.y, 0, getHandle(), 0);
	if (nId == IDM_ADDRESSBOOK_REMOVE) {
		pDialog_->remove();
	}
	else {
		struct {
			UINT nId_;
			Type type_;
		} types[] = {
			{ IDM_ADDRESSBOOK_CHANGETO,		TYPE_TO		},
			{ IDM_ADDRESSBOOK_CHANGECC,		TYPE_CC		},
			{ IDM_ADDRESSBOOK_CHANGEBCC,	TYPE_BCC	}
		};
		Type type = static_cast<Type>(-1);
		for (int n = 0; n < countof(types) && type == -1; ++n) {
			if (types[n].nId_ == nId)
				type = types[n].type_;
		}
		if (type == -1)
			return 0;
		
		int nItem = -1;
		while (true) {
			nItem = ListView_GetNextItem(getHandle(), nItem, LVNI_ALL | LVNI_SELECTED);
			if (nItem == -1)
				break;
			
			LVITEM item = {
				LVIF_PARAM,
				nItem
			};
			ListView_GetItem(getHandle(), &item);
			
			Item* pItem = reinterpret_cast<Item*>(item.lParam);
			pItem->setType(type);
			
			item.mask = LVIF_IMAGE;
			item.iImage = type;
			ListView_SetItem(getHandle(), &item);
		}
		ListView_SortItems(getHandle(), &selectedItemComp, 0);
	}
	
	return 0;
}

LRESULT qm::SelectAddressDialog::SelectedAddressListWindow::onDestroy()
{
#if defined _WIN32_WCE && _WIN32_WCE >= 400 && defined _WIN32_WCE_PSPC
	pDialog_->removeNotifyHandler(this);
#endif
	return DefaultWindowHandler::onDestroy();
}

LRESULT qm::SelectAddressDialog::SelectedAddressListWindow::onLButtonDown(UINT nFlags,
																		  const POINT& pt)
{
#if defined _WIN32_WCE && _WIN32_WCE >= 300 && _WIN32_WCE < 400 && defined _WIN32_WCE_PSPC
	if (tapAndHold(pt))
		return 0;
#endif
	return DefaultWindowHandler::onLButtonDown(nFlags, pt);
}

LRESULT qm::SelectAddressDialog::SelectedAddressListWindow::onNotify(NMHDR* pnmhdr,
																	 bool* pbHandled)
{
	BEGIN_NOTIFY_HANDLER()
#if defined _WIN32_WCE && _WIN32_WCE >= 400 && defined _WIN32_WCE_PSPC
		HANDLE_NOTIFY(NM_RECOGNIZEGESTURE, IDC_SELECTEDADDRESS, onRecognizeGesture)
#endif
	END_NOTIFY_HANDLER()
	return NotifyHandler::onNotify(pnmhdr, pbHandled);
}

#if defined _WIN32_WCE && _WIN32_WCE >= 400 && defined _WIN32_WCE_PSPC
LRESULT qm::SelectAddressDialog::SelectedAddressListWindow::onRecognizeGesture(NMHDR* pnmhdr,
																			   bool* pbHandled)
{
	*pbHandled = true;
	return TRUE;
}
#endif


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
												   const WCHAR* pwszDefaultName) :
	DefaultDialog(IDD_SELECTSYNCFILTER),
	pManager_(pManager)
{
	if (pwszDefaultName)
		wstrName_ = allocWString(pwszDefaultName);
}

qm::SelectSyncFilterDialog::~SelectSyncFilterDialog()
{
}

const WCHAR* qm::SelectSyncFilterDialog::getName() const
{
	return wstrName_.get();
}

LRESULT qm::SelectSyncFilterDialog::onInitDialog(HWND hwndFocus,
												 LPARAM lParam)
{
	init(false);
	
	const SyncFilterManager::FilterSetList& l = pManager_->getFilterSets();
	
	if (l.empty()) {
		endDialog(IDOK);
	}
	else {
		typedef SyncFilterManager::FilterSetList List;
		for (List::const_iterator it = l.begin(); it != l.end(); ++it) {
			W2T((*it)->getName(), ptszName);
			sendDlgItemMessage(IDC_FILTERSETLIST, LB_ADDSTRING,
				0, reinterpret_cast<LPARAM>(ptszName));
		}
		if (wstrName_.get()) {
			W2T(wstrName_.get(), ptszName);
			sendDlgItemMessage(IDC_FILTERSETLIST, LB_SELECTSTRING,
				-1, reinterpret_cast<LPARAM>(ptszName));
		}
		else {
			sendDlgItemMessage(IDC_FILTERSETLIST, LB_SETCURSEL, 0);
		}
	}
	
	return TRUE;
}

LRESULT qm::SelectSyncFilterDialog::onOk()
{
	unsigned int nItem = sendDlgItemMessage(IDC_FILTERSETLIST, LB_GETCURSEL);
	if (nItem == LB_ERR)
		return onCancel();
	
	int nLen = sendDlgItemMessage(IDC_FILTERSETLIST, LB_GETTEXTLEN, nItem);
	tstring_ptr tstrName(allocTString(nLen + 10));
	sendDlgItemMessage(IDC_FILTERSETLIST, LB_GETTEXT,
		nItem, reinterpret_cast<LPARAM>(tstrName.get()));
	
	wstrName_ = tcs2wcs(tstrName.get());
	
	return DefaultDialog::onOk();
}


/****************************************************************************
 *
 * SignatureDialog
 *
 */

qm::SignatureDialog::SignatureDialog(Signature* pSignature,
									 Document* pDocument) :
	DefaultDialog(IDD_SIGNATURE),
	pSignature_(pSignature),
	pDocument_(pDocument)
{
}

qm::SignatureDialog::~SignatureDialog()
{
}

LRESULT qm::SignatureDialog::onCommand(WORD nCode,
									   WORD nId)
{
	BEGIN_COMMAND_HANDLER()
		HANDLE_COMMAND_ID_CODE(IDC_NAME, EN_CHANGE, onNameChange)
	END_COMMAND_HANDLER()
	return DefaultDialog::onCommand(nCode, nId);
}

LRESULT qm::SignatureDialog::onInitDialog(HWND hwndFocus,
										  LPARAM lParam)
{
	init(false);
	
	setDlgItemText(IDC_NAME, pSignature_->getName());
	
	const Document::AccountList& l = pDocument_->getAccounts();
	for (Document::AccountList::const_iterator it = l.begin(); it != l.end(); ++it) {
		W2T((*it)->getName(), ptszName);
		sendDlgItemMessage(IDC_ACCOUNT, CB_ADDSTRING,
			0, reinterpret_cast<LPARAM>(ptszName));
	}
	if (pSignature_->getAccount())
		setDlgItemText(IDC_ACCOUNT, pSignature_->getAccount());
	
	if (pSignature_->isDefault())
		sendDlgItemMessage(IDC_DEFAULT, BM_SETCHECK, BST_CHECKED);
	
	wstring_ptr wstrSignature(Util::convertLFtoCRLF(pSignature_->getSignature()));
	setDlgItemText(IDC_SIGNATURE, wstrSignature.get());
	
	updateState();
	
	return TRUE;
}

LRESULT qm::SignatureDialog::onOk()
{
	wstring_ptr wstrName(getDlgItemText(IDC_NAME));
	
	wstring_ptr wstrAccount(getDlgItemText(IDC_ACCOUNT));
	const WCHAR* pwszAccount = 0;
	std::auto_ptr<RegexPattern> pAccount;
	if (*wstrAccount.get()) {
		pAccount = RegexCompiler().compile(wstrAccount.get());
		if (!pAccount.get())
			return 0;
		pwszAccount = wstrAccount.get();
	}
	
	bool bDefault = sendDlgItemMessage(IDC_DEFAULT, BM_GETCHECK) == BST_CHECKED;
	
	wstring_ptr wstrSignature(getDlgItemText(IDC_SIGNATURE));
	wstrSignature = Util::convertCRLFtoLF(wstrSignature.get());
	
	pSignature_->setName(wstrName.get());
	pSignature_->setAccount(pwszAccount, pAccount);
	pSignature_->setDefault(bDefault);
	pSignature_->setSignature(wstrSignature.get());
	
	return DefaultDialog::onOk();
}

LRESULT qm::SignatureDialog::onNameChange()
{
	updateState();
	return 0;
}

void qm::SignatureDialog::updateState()
{
	Window(getDlgItem(IDOK)).enableWindow(
		Window(getDlgItem(IDC_NAME)).getWindowTextLength() != 0);
}


/****************************************************************************
*
* SignaturesDialog
*
*/

qm::SignaturesDialog::SignaturesDialog(SignatureManager* pSignatureManager,
									   Document* pDocument) :
	AbstractListDialog<Signature, SignatureManager::SignatureList>(IDD_SIGNATURES, IDC_SIGNATURES),
	pSignatureManager_(pSignatureManager),
	pDocument_(pDocument)
{
	const SignatureManager::SignatureList& l = pSignatureManager_->getSignatures();
	SignatureManager::SignatureList& list = getList();
	list.reserve(l.size());
	for (SignatureManager::SignatureList::const_iterator it = l.begin(); it != l.end(); ++it)
		list.push_back(new Signature(**it));
}

qm::SignaturesDialog::~SignaturesDialog()
{
}

LRESULT qm::SignaturesDialog::onOk()
{
	pSignatureManager_->setSignatures(getList());
	if (!pSignatureManager_->save()) {
		// TODO
	}
	
	return DefaultDialog::onOk();
}

wstring_ptr qm::SignaturesDialog::getLabel(const Signature* p) const
{
	StringBuffer<WSTRING> buf(p->getName());
	if (p->getAccount()) {
		buf.append(L" [");
		buf.append(p->getAccount());
		buf.append(L"]");
	}
	if (p->isDefault())
		buf.append(L" *");
	return buf.getString();
}

std::auto_ptr<Signature> qm::SignaturesDialog::create() const
{
	std::auto_ptr<Signature> pSignature(new Signature());
	SignatureDialog dialog(pSignature.get(), pDocument_);
	if (dialog.doModal(getHandle()) != IDOK)
		return std::auto_ptr<Signature>();
	return pSignature;
}

bool qm::SignaturesDialog::edit(Signature* p) const
{
	SignatureDialog dialog(p, pDocument_);
	return dialog.doModal(getHandle()) == IDOK;
}


/****************************************************************************
 *
 * SyncFilterDialog
 *
 */

qm::SyncFilterDialog::SyncFilterDialog(SyncFilter* pSyncFilter) :
	DefaultDialog(IDD_SYNCFILTER),
	pSyncFilter_(pSyncFilter)
{
}

qm::SyncFilterDialog::~SyncFilterDialog()
{
}

LRESULT qm::SyncFilterDialog::onCommand(WORD nCode,
										WORD nId)
{
	BEGIN_COMMAND_HANDLER()
		HANDLE_COMMAND_ID(IDC_EDIT, onEdit)
		HANDLE_COMMAND_ID_CODE(IDC_ACTION, CBN_SELCHANGE, onActionSelChange)
	END_COMMAND_HANDLER()
	return DefaultDialog::onCommand(nCode, nId);
}

LRESULT qm::SyncFilterDialog::onInitDialog(HWND hwndFocus,
										   LPARAM lParam)
{
	init(false);
	
	wstring_ptr wstrCondition(pSyncFilter_->getCondition()->getString());
	setDlgItemText(IDC_CONDITION, wstrCondition.get());
	
	setDlgItemText(IDC_FOLDER, pSyncFilter_->getFolder());
	
	const TCHAR* ptszActions[] = {
		_T("Download (POP3)"),
		_T("Download (IMAP4)"),
		_T("Download (NNTP)"),
		_T("Delete (POP3, IMAP4)"),
		_T("Ignore (POP3, NNTP)")
	};
	for (int n = 0; n < countof(ptszActions); ++n)
		sendDlgItemMessage(IDC_ACTION, CB_ADDSTRING,
			0, reinterpret_cast<LPARAM>(ptszActions[n]));
	
	const TCHAR* ptszTypes[] = {
		_T("All"),
		_T("Text"),
		_T("Html"),
		_T("Header")
	};
	for (int n = 0; n < countof(ptszTypes); ++n)
		sendDlgItemMessage(IDC_TYPE, CB_ADDSTRING,
			0, reinterpret_cast<LPARAM>(ptszTypes[n]));
	
	int nAction = 0;
	int nMaxLine = 0;
	int nType = 0;
	
	const SyncFilter::ActionList& listAction = pSyncFilter_->getActions();
	if (!listAction.empty()) {
		const SyncFilterAction* pAction = listAction.front();
		const WCHAR* pwszAction = pAction->getName();
		if (wcscmp(pwszAction, L"download") == 0) {
			const WCHAR* pwszProtocol = L"pop3";
			if (pAction->getParam(L"line"))
				pwszProtocol = L"pop3";
			else if (pAction->getParam(L"type"))
				pwszProtocol = L"imap4";
			else
				pwszProtocol = L"nntp";
			
			if (wcscmp(pwszProtocol, L"imap4") == 0) {
				nAction = 1;
				const WCHAR* pwszType = pAction->getParam(L"type");
				if (pwszType) {
					const WCHAR* pwszTypes[] = {
						L"all",
						L"text",
						L"html",
						L"header"
					};
					for (int n = 0; n < countof(pwszTypes); ++n) {
						if (wcscmp(pwszType, pwszTypes[n]) == 0) {
							nType = n;
							break;
						}
					}
				}
			}
			else if (wcscmp(pwszProtocol, L"nntp") == 0) {
				nAction = 2;
			}
			else {
				const WCHAR* pwszLine = pAction->getParam(L"line");
				if (pwszLine) {
					WCHAR* pEnd = 0;
					long nLine = wcstol(pwszLine, &pEnd, 10);
					if (!*pEnd)
						nMaxLine = nLine;
				}
			}
		}
		else if (wcscmp(pwszAction, L"delete") == 0) {
			nAction = 3;
		}
		else if (wcscmp(pwszAction, L"ignore") == 0) {
			nAction = 4;
		}
	}
	
	sendDlgItemMessage(IDC_ACTION, CB_SETCURSEL, nAction);
	setDlgItemInt(IDC_MAXLINE, nMaxLine);
	sendDlgItemMessage(IDC_TYPE, CB_SETCURSEL, nType);
	
	updateState();
	
	return TRUE;
}

LRESULT qm::SyncFilterDialog::onOk()
{
	wstring_ptr wstrCondition(getDlgItemText(IDC_CONDITION));
	std::auto_ptr<Macro> pCondition(MacroParser(MacroParser::TYPE_SYNCFILTER).parse(wstrCondition.get()));
	if (!pCondition.get()) {
		// TODO MSG
		return 0;
	}
	
	wstring_ptr wstrFolder(getDlgItemText(IDC_FOLDER));
	const WCHAR* pwszFolder = 0;
	std::auto_ptr<RegexPattern> pFolder;
	if (*wstrFolder.get()) {
		pFolder = RegexCompiler().compile(wstrFolder.get());
		if (!pFolder.get()) {
			// TODO MSG
			return 0;
		}
		pwszFolder = wstrFolder.get();
	}
	
	int nAction = sendDlgItemMessage(IDC_ACTION, CB_GETCURSEL);
	if (nAction == CB_ERR) {
		// TODO MSG
		return 0;
	}
	
	const WCHAR* pwszName = 0;
	switch (nAction) {
	case 0:
	case 1:
	case 2:
		pwszName = L"download";
		break;
	case 3:
		pwszName = L"delete";
		break;
	case 4:
		pwszName = L"ignore";
		break;
	default:
		assert(false);
		return 0;
	}
	
	std::auto_ptr<SyncFilterAction> pAction(new SyncFilterAction(pwszName));
	
	switch (nAction) {
	case 0:
		{
			int nLine = getDlgItemInt(IDC_MAXLINE);
			wstring_ptr wstrLine(allocWString(32));
			swprintf(wstrLine.get(), L"%d", nLine);
			pAction->addParam(allocWString(L"line"), wstrLine);
		}
		break;
	case 1:
		{
			int nType = sendDlgItemMessage(IDC_TYPE, CB_GETCURSEL);
			if (nType == CB_ERR) {
				// TODO MSG
				return 0;
			}
			const WCHAR* pwszTypes[] = {
				L"all",
				L"text",
				L"html",
				L"header"
			};
			pAction->addParam(allocWString(L"type"), allocWString(pwszTypes[nType]));
		}
		break;
	default:
		break;
	}
	
	SyncFilter::ActionList listAction(1, pAction.get());
	pAction.release();
	
	const SyncFilter::ActionList& l = pSyncFilter_->getActions();
	listAction.reserve(l.size());
	for (SyncFilter::ActionList::size_type n = 1; n < l.size(); ++n)
		listAction.push_back(new SyncFilterAction(*l[n]));
	
	pSyncFilter_->setFolder(pwszFolder, pFolder);
	pSyncFilter_->setCondition(pCondition);
	pSyncFilter_->setActions(listAction);
	
	return DefaultDialog::onOk();
}

LRESULT qm::SyncFilterDialog::onEdit()
{
	wstring_ptr wstrCondition(getDlgItemText(IDC_CONDITION));
	ConditionDialog dialog(wstrCondition.get());
	if (dialog.doModal(getHandle()) == IDOK)
		setDlgItemText(IDC_CONDITION, dialog.getCondition());
	return 0;
}

LRESULT qm::SyncFilterDialog::onActionSelChange()
{
	updateState();
	return 0;
}

void qm::SyncFilterDialog::updateState()
{
	int nItem = sendDlgItemMessage(IDC_ACTION, CB_GETCURSEL);
	Window(getDlgItem(IDC_MAXLINELABEL)).showWindow(nItem == 0 ? SW_SHOW : SW_HIDE);
	Window(getDlgItem(IDC_MAXLINE)).showWindow(nItem == 0 ? SW_SHOW : SW_HIDE);
	Window(getDlgItem(IDC_TYPELABEL)).showWindow(nItem == 1 ? SW_SHOW : SW_HIDE);
	Window(getDlgItem(IDC_TYPE)).showWindow(nItem == 1 ? SW_SHOW : SW_HIDE);
}


/****************************************************************************
*
* SyncFiltersDialog
*
*/

qm::SyncFiltersDialog::SyncFiltersDialog(SyncFilterSet* pSyncFilterSet) :
	AbstractListDialog<SyncFilter, SyncFilterSet::FilterList>(IDD_SYNCFILTERS, IDC_FILTERS),
	pSyncFilterSet_(pSyncFilterSet)
{
	const SyncFilterSet::FilterList& l = pSyncFilterSet->getFilters();
	SyncFilterSet::FilterList& list = getList();
	list.reserve(l.size());
	for (SyncFilterSet::FilterList::const_iterator it = l.begin(); it != l.end(); ++it)
		list.push_back(new SyncFilter(**it));
}

qm::SyncFiltersDialog::~SyncFiltersDialog()
{
}

LRESULT qm::SyncFiltersDialog::onCommand(WORD nCode,
										 WORD nId)
{
	BEGIN_COMMAND_HANDLER()
		HANDLE_COMMAND_ID_CODE(IDC_NAME, EN_CHANGE, onNameChange)
	END_COMMAND_HANDLER()
	return AbstractListDialog<SyncFilter, SyncFilterSet::FilterList>::onCommand(nCode, nId);
}

LRESULT qm::SyncFiltersDialog::onInitDialog(HWND hwndFocus,
											LPARAM lParam)
{
	setDlgItemText(IDC_NAME, pSyncFilterSet_->getName());
	
	return AbstractListDialog<SyncFilter, SyncFilterSet::FilterList>::onInitDialog(hwndFocus, lParam);
}

LRESULT qm::SyncFiltersDialog::onOk()
{
	wstring_ptr wstrName(getDlgItemText(IDC_NAME));
	
	pSyncFilterSet_->setName(wstrName.get());
	pSyncFilterSet_->setFilters(getList());
	
	return AbstractListDialog<SyncFilter, SyncFilterSet::FilterList>::onOk();
}

wstring_ptr qm::SyncFiltersDialog::getLabel(const SyncFilter* p) const
{
	StringBuffer<WSTRING> buf;
	
	const WCHAR* pwszFolder = p->getFolder();
	if (pwszFolder) {
		buf.append(L'[');
		buf.append(pwszFolder);
		buf.append(L"] ");
	}
	
	wstring_ptr wstrCondition(p->getCondition()->getString());
	buf.append(wstrCondition.get());
	
	return buf.getString();
}

std::auto_ptr<SyncFilter> qm::SyncFiltersDialog::create() const
{
	std::auto_ptr<SyncFilter> pFilter(new SyncFilter());
	SyncFilterDialog dialog(pFilter.get());
	if (dialog.doModal(getHandle()) != IDOK)
		return std::auto_ptr<SyncFilter>();
	return pFilter;
}

bool qm::SyncFiltersDialog::edit(SyncFilter* p) const
{
	SyncFilterDialog dialog(p);
	return dialog.doModal(getHandle()) == IDOK;
}

void qm::SyncFiltersDialog::updateState()
{
	AbstractListDialog<SyncFilter, SyncFilterSet::FilterList>::updateState();
	
	Window(getDlgItem(IDOK)).enableWindow(
		Window(getDlgItem(IDC_NAME)).getWindowTextLength() != 0);
}


LRESULT qm::SyncFiltersDialog::onNameChange()
{
	updateState();
	return 0;
}


/****************************************************************************
*
* SyncFilterSetsDialog
*
*/

qm::SyncFilterSetsDialog::SyncFilterSetsDialog(SyncFilterManager* pSyncFilterManager) :
	AbstractListDialog<SyncFilterSet, SyncFilterManager::FilterSetList>(IDD_SYNCFILTERSETS, IDC_FILTERSETS),
	pSyncFilterManager_(pSyncFilterManager)
{
	const SyncFilterManager::FilterSetList& l = pSyncFilterManager->getFilterSets();
	SyncFilterManager::FilterSetList& list = getList();
	list.reserve(l.size());
	for (SyncFilterManager::FilterSetList::const_iterator it = l.begin(); it != l.end(); ++it)
		list.push_back(new SyncFilterSet(**it));
}

qm::SyncFilterSetsDialog::~SyncFilterSetsDialog()
{
}

LRESULT qm::SyncFilterSetsDialog::onOk()
{
	pSyncFilterManager_->setFilterSets(getList());
	if (!pSyncFilterManager_->save()) {
		// TODO
	}
	return AbstractListDialog<SyncFilterSet, SyncFilterManager::FilterSetList>::onOk();
}

wstring_ptr qm::SyncFilterSetsDialog::getLabel(const SyncFilterSet* p) const
{
	return allocWString(p->getName());
}

std::auto_ptr<SyncFilterSet> qm::SyncFilterSetsDialog::create() const
{
	std::auto_ptr<SyncFilterSet> pFilterSet(new SyncFilterSet());
	SyncFiltersDialog dialog(pFilterSet.get());
	if (dialog.doModal(getHandle()) != IDOK)
		return std::auto_ptr<SyncFilterSet>();
	return pFilterSet;
}

bool qm::SyncFilterSetsDialog::edit(SyncFilterSet* p) const
{
	SyncFiltersDialog dialog(p);
	return dialog.doModal(getHandle()) == IDOK;
}


#ifdef QMTABWINDOW
/****************************************************************************
 *
 * TabTitleDialog
 *
 */

qm::TabTitleDialog::TabTitleDialog(const WCHAR* pwszTitle) :
	DefaultDialog(IDD_TABTITLE)
{
	if (pwszTitle)
		wstrTitle_ = allocWString(pwszTitle);
}

qm::TabTitleDialog::~TabTitleDialog()
{
}

const WCHAR* qm::TabTitleDialog::getTitle() const
{
	return wstrTitle_.get();
}

LRESULT qm::TabTitleDialog::onInitDialog(HWND hwndFocus,
										 LPARAM lParam)
{
	init(false);
	
	if (wstrTitle_.get())
		setDlgItemText(IDC_TITLE, wstrTitle_.get());
	
	return TRUE;
}

LRESULT qm::TabTitleDialog::onOk()
{
	wstrTitle_ = getDlgItemText(IDC_TITLE);
	return DefaultDialog::onOk();
}
#endif // TABWINDOW


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
	if (nFlags & ViewColumn::FLAG_CACHE)
		sendDlgItemMessage(IDC_CACHE, BM_SETCHECK, BST_CHECKED);
	
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
	if (sendDlgItemMessage(IDC_CACHE, BM_GETCHECK) == BST_CHECKED)
		nFlags |= ViewColumn::FLAG_CACHE;
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
	bool bEnable = sendDlgItemMessage(IDC_TYPE, CB_GETCURSEL) ==
		sendDlgItemMessage(IDC_TYPE, CB_GETCOUNT) - 1;
	Window(getDlgItem(IDC_MACRO)).enableWindow(bEnable);
	Window(getDlgItem(IDC_CACHE)).enableWindow(bEnable);
}


/****************************************************************************
 *
 * ViewsDialog
 *
 */

qm::ViewsDialog::ViewsDialog(ViewModelManager* pViewModelManager,
							 ViewModel* pViewModel) :
	DefaultDialog(IDD_VIEWS),
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
	return NotifyHandler::onNotify(pnmhdr, pbHandled);
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
	DefaultViewData* pDefaultViewData = pViewModelManager_->getDefaultViewData();
	return pDefaultViewData->getItem(pAccount->getClass());
}
