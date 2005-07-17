/*
 * $Id$
 *
 * Copyright(C) 1998-2005 Satoshi Nakamura
 * All rights reserved.
 *
 */

#pragma warning(disable:4786)

#include <qmaccount.h>
#include <qmsecurity.h>
#include <qmsession.h>
#include <qmsyncfilter.h>

#include <qsras.h>

#include <tchar.h>

#include "accountdialog.h"
#include "optiondialog.h"
#include "resourceinc.h"

using namespace qm;
using namespace qs;


/****************************************************************************
 *
 * AccountDialog
 *
 */

qm::AccountDialog::AccountDialog(AccountManager* pAccountManager,
								 Account* pAccount,
								 PasswordManager* pPasswordManager,
								 SyncFilterManager* pSyncFilterManager,
								 const Security* pSecurity,
								 JunkFilter* pJunkFilter,
								 OptionDialogManager* pOptionDialogManager,
								 Profile* pProfile) :
	DefaultDialog(IDD_ACCOUNT),
	pAccountManager_(pAccountManager),
	pPasswordManager_(pPasswordManager),
	pSubAccount_(pAccount ? pAccount->getCurrentSubAccount() : 0),
	pSyncFilterManager_(pSyncFilterManager),
	pSecurity_(pSecurity),
	pJunkFilter_(pJunkFilter),
	pOptionDialogManager_(pOptionDialogManager),
	pProfile_(pProfile),
	bAccountAdded_(false)
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
		if (pAccountManager_->hasAccount(pwszName)) {
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
			pSecurity_, pPasswordManager_, pJunkFilter_));
		Account* p = pAccount.get();
		pAccountManager_->addAccount(pAccount);
		pSubAccount_ = p->getCurrentSubAccount();
		
		update();
		
		bAccountAdded_ = true;
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
		
		CreateSubAccountDialog dialog(pAccountManager_);
		if (dialog.doModal(getHandle()) == IDOK) {
			const WCHAR* pwszName = dialog.getName();
			
			if (pAccount->getSubAccount(pwszName)) {
				// TODO MSG
				return 0;
			}
			
			if (!pAccount->save(false)) {
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
				pAccountManager_->removeAccount(pAccount);
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
				if (!pAccountManager_->renameAccount(pAccount, dialog.getName())) {
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
		AccountAdvancedPage advancedPage(pSubAccount, pJunkFilter_,
			pSyncFilterManager_, pOptionDialogManager_);
		PropertySheetBase sheet(hInst, wstrTitle.get(), false);
		sheet.add(&generalPage);
		sheet.add(&userPage);
		sheet.add(&detailPage);
		sheet.add(pReceivePage.get());
		sheet.add(pSendPage.get());
		sheet.add(&dialupPage);
		sheet.add(&advancedPage);
		
		sheet.doModal(getHandle());
		
		if (bAccountAdded_ && pAccount->isSupport(Account::SUPPORT_REMOTEFOLDER)) {
			HINSTANCE hInst = Application::getApplication().getResourceHandle();
			if (messageBox(hInst, IDS_UPDATEFOLDER, MB_YESNO | MB_ICONQUESTION, getHandle()) == IDYES) {
				if (!pAccount->updateFolders())
					messageBox(hInst, IDS_ERROR_UPDATEFOLDER, MB_OK | MB_ICONERROR, getHandle());
			}
		}
	}
	
	bAccountAdded_ = false;
	
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
		
		const AccountManager::AccountList& listAccount = pAccountManager_->getAccounts();
		for (AccountManager::AccountList::const_iterator itA = listAccount.begin(); itA != listAccount.end(); ++itA) {
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
 * CreateSubAccountDialog
 *
 */

qm::CreateSubAccountDialog::CreateSubAccountDialog(AccountManager* pAccountManager) :
	DefaultDialog(IDD_CREATESUBACCOUNT),
	pAccountManager_(pAccountManager)
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
	
	const AccountManager::AccountList& listAccount = pAccountManager_->getAccounts();
	for (AccountManager::AccountList::const_iterator itA = listAccount.begin(); itA != listAccount.end(); ++itA) {
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
 * AccountAdvancedPage
 *
 */

qm::AccountAdvancedPage::AccountAdvancedPage(SubAccount* pSubAccount,
											 JunkFilter* pJunkFilter,
											 SyncFilterManager* pSyncFilterManager,
											 OptionDialogManager* pOptionDialogManager) :
	DefaultPropertyPage(IDD_ACCOUNTADVANCED),
	pSubAccount_(pSubAccount),
	pJunkFilter_(pJunkFilter),
	pSyncFilterManager_(pSyncFilterManager),
	pOptionDialogManager_(pOptionDialogManager)
{
}

qm::AccountAdvancedPage::~AccountAdvancedPage()
{
}

LRESULT qm::AccountAdvancedPage::onCommand(WORD nCode,
										   WORD nId)
{
	BEGIN_COMMAND_HANDLER()
		HANDLE_COMMAND_ID(IDC_EDIT, onEdit)
	END_COMMAND_HANDLER()
	return DefaultPropertyPage::onCommand(nCode, nId);
}

LRESULT qm::AccountAdvancedPage::onInitDialog(HWND hwndFocus,
											  LPARAM lParam)
{
	setDlgItemText(IDC_IDENTITY, pSubAccount_->getIdentity());
	wstring_ptr wstrMyAddress = pSubAccount_->getMyAddress();
	setDlgItemText(IDC_MYADDRESS, wstrMyAddress.get());
	
	setDlgItemText(IDC_SYNCFILTER, pSubAccount_->getSyncFilterName());
	sendDlgItemMessage(IDC_SYNCFILTER, CB_SETDROPPEDWIDTH, 150);
	updateFilter();
	
	setDlgItemInt(IDC_TIMEOUT, pSubAccount_->getTimeout());
	sendDlgItemMessage(IDC_CONNECTRECEIVEHOSTBEFORESEND, BM_SETCHECK,
		pSubAccount_->isConnectReceiveBeforeSend() ? BST_CHECKED : BST_UNCHECKED);
	sendDlgItemMessage(IDC_TREATASSENT, BM_SETCHECK,
		pSubAccount_->isTreatAsSent() ? BST_CHECKED : BST_UNCHECKED);
	sendDlgItemMessage(IDC_ADDMESSAGEID, BM_SETCHECK,
		pSubAccount_->isAddMessageId() ? BST_CHECKED : BST_UNCHECKED);
	sendDlgItemMessage(IDC_AUTOAPPLYRULES, BM_SETCHECK,
		pSubAccount_->isAutoApplyRules() ? BST_CHECKED : BST_UNCHECKED);
#ifndef _WIN32_WCE
	sendDlgItemMessage(IDC_JUNKFILTER, BM_SETCHECK,
		pSubAccount_->isJunkFilterEnabled() ? BST_CHECKED : BST_UNCHECKED);
	if (!pJunkFilter_ || !pSubAccount_->getAccount()->isSupport(Account::SUPPORT_JUNKFILTER))
		Window(getDlgItem(IDC_JUNKFILTER)).enableWindow(false);
	sendDlgItemMessage(IDC_STOREDECODED, BM_SETCHECK,
		pSubAccount_->getAccount()->isStoreDecodedMessage() ? BST_CHECKED : BST_UNCHECKED);
#endif
	
	return TRUE;
}

LRESULT qm::AccountAdvancedPage::onOk()
{
	wstring_ptr wstrIdentity(getDlgItemText(IDC_IDENTITY));
	if (wstrIdentity.get())
		pSubAccount_->setIdentity(wstrIdentity.get());
	
	wstring_ptr wstrMyAddress(getDlgItemText(IDC_MYADDRESS));
	if (wstrMyAddress.get())
		pSubAccount_->setMyAddress(wstrMyAddress.get());
	
	wstring_ptr wstrSyncFilter(getDlgItemText(IDC_SYNCFILTER));
	if (wstrSyncFilter.get())
		pSubAccount_->setSyncFilterName(wstrSyncFilter.get());
	
	pSubAccount_->setTimeout(getDlgItemInt(IDC_TIMEOUT));
	pSubAccount_->setConnectReceiveBeforeSend(
		sendDlgItemMessage(IDC_CONNECTRECEIVEHOSTBEFORESEND, BM_GETCHECK) == BST_CHECKED);
	pSubAccount_->setTreatAsSent(
		sendDlgItemMessage(IDC_TREATASSENT, BM_GETCHECK) == BST_CHECKED);
	pSubAccount_->setAddMessageId(
		sendDlgItemMessage(IDC_ADDMESSAGEID, BM_GETCHECK) == BST_CHECKED);
	pSubAccount_->setAutoApplyRules(
		sendDlgItemMessage(IDC_AUTOAPPLYRULES, BM_GETCHECK) == BST_CHECKED);
#ifndef _WIN32_WCE
	pSubAccount_->setJunkFilterEnabled(
		sendDlgItemMessage(IDC_JUNKFILTER, BM_GETCHECK) == BST_CHECKED);
	pSubAccount_->getAccount()->setStoreDecodedMessage(
		sendDlgItemMessage(IDC_STOREDECODED, BM_GETCHECK) == BST_CHECKED);
#endif
	
	return DefaultPropertyPage::onOk();
}

LRESULT qm::AccountAdvancedPage::onEdit()
{
	HWND hwnd = getHandle();
	while (hwnd) {
		if (Window(hwnd).getStyle() & WS_POPUP)
			break;
		hwnd = Window(hwnd).getParent();
	}
	
	if (pOptionDialogManager_->showDialog(hwnd, OptionDialog::PANEL_SYNCFILTERS) == IDOK)
		updateFilter();
	
	return 0;
}

void qm::AccountAdvancedPage::updateFilter()
{
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
 * AccountDetailPage
 *
 */

qm::AccountDetailPage::AccountDetailPage(SubAccount* pSubAccount,
										 ReceiveSessionUI* pReceiveUI,
										 SendSessionUI* pSendUI) :
	DefaultPropertyPage(IDD_ACCOUNTDETAIL),
	pSubAccount_(pSubAccount),
	pReceiveUI_(pReceiveUI),
	pSendUI_(pSendUI)
{
}

qm::AccountDetailPage::~AccountDetailPage()
{
}

LRESULT qm::AccountDetailPage::onCommand(WORD nCode,
										 WORD nId)
{
	BEGIN_COMMAND_HANDLER()
		HANDLE_COMMAND_ID_RANGE(IDC_RECEIVENOSECURE, IDC_SENDSTARTTLS, onSecure)
	END_COMMAND_HANDLER()
	return DefaultPropertyPage::onCommand(nCode, nId);
}

LRESULT qm::AccountDetailPage::onInitDialog(HWND hwndFocus,
											LPARAM lParam)
{
	setDlgItemInt(IDC_RECEIVEPORT, pSubAccount_->getPort(Account::HOST_RECEIVE));
	switch (pSubAccount_->getSecure(Account::HOST_RECEIVE)) {
	case SubAccount::SECURE_SSL:
		sendDlgItemMessage(IDC_RECEIVESSL, BM_SETCHECK, BST_CHECKED);
		break;
	case SubAccount::SECURE_STARTTLS:
		sendDlgItemMessage(IDC_RECEIVESTARTTLS, BM_SETCHECK, BST_CHECKED);
		break;
	default:
		sendDlgItemMessage(IDC_RECEIVENOSECURE, BM_SETCHECK, BST_CHECKED);
		break;
	}
	sendDlgItemMessage(IDC_RECEIVELOG, BM_SETCHECK,
		pSubAccount_->isLog(Account::HOST_RECEIVE) ? BST_CHECKED : BST_UNCHECKED);
	
	setDlgItemInt(IDC_SENDPORT, pSubAccount_->getPort(Account::HOST_SEND));
	switch (pSubAccount_->getSecure(Account::HOST_SEND)) {
	case SubAccount::SECURE_SSL:
		sendDlgItemMessage(IDC_SENDSSL, BM_SETCHECK, BST_CHECKED);
		break;
	case SubAccount::SECURE_STARTTLS:
		sendDlgItemMessage(IDC_SENDSTARTTLS, BM_SETCHECK, BST_CHECKED);
		break;
	default:
		sendDlgItemMessage(IDC_SENDNOSECURE, BM_SETCHECK, BST_CHECKED);
		break;
	}
	sendDlgItemMessage(IDC_SENDLOG, BM_SETCHECK,
		pSubAccount_->isLog(Account::HOST_SEND) ? BST_CHECKED : BST_UNCHECKED);
	
	if (!pReceiveUI_->isSupported(ReceiveSessionUI::SUPPORT_HOST)) {
		Window(getDlgItem(IDC_RECEIVEPORT)).enableWindow(false);
		Window(getDlgItem(IDC_RECEIVESSL)).enableWindow(false);
		Window(getDlgItem(IDC_RECEIVESTARTTLS)).enableWindow(false);
		Window(getDlgItem(IDC_RECEIVENOSECURE)).enableWindow(false);
	}
	if (!pReceiveUI_->isSupported(ReceiveSessionUI::SUPPORT_SSL))
		Window(getDlgItem(IDC_RECEIVESSL)).enableWindow(false);
	if (!pReceiveUI_->isSupported(ReceiveSessionUI::SUPPORT_STARTTLS))
		Window(getDlgItem(IDC_RECEIVESTARTTLS)).enableWindow(false);
	if (!pSendUI_->isSupported(SendSessionUI::SUPPORT_HOST)) {
		Window(getDlgItem(IDC_SENDPORT)).enableWindow(false);
		Window(getDlgItem(IDC_SENDSSL)).enableWindow(false);
		Window(getDlgItem(IDC_SENDSTARTTLS)).enableWindow(false);
		Window(getDlgItem(IDC_SENDNOSECURE)).enableWindow(false);
	}
	if (!pSendUI_->isSupported(SendSessionUI::SUPPORT_SSL))
		Window(getDlgItem(IDC_SENDSSL)).enableWindow(false);
	if (!pSendUI_->isSupported(SendSessionUI::SUPPORT_STARTTLS))
		Window(getDlgItem(IDC_SENDSTARTTLS)).enableWindow(false);
	
	if (!Security::isSSLEnabled()) {
		UINT nIds[] = {
			IDC_RECEIVENOSECURE,
			IDC_RECEIVESSL,
			IDC_RECEIVESTARTTLS,
			IDC_SENDNOSECURE,
			IDC_SENDSSL,
			IDC_SENDSTARTTLS
		};
		for (int n = 0; n < countof(nIds); ++n)
			Window(getDlgItem(nIds[n])).enableWindow(false);
	}
	
	return TRUE;
}

LRESULT qm::AccountDetailPage::onOk()
{
	pSubAccount_->setPort(Account::HOST_RECEIVE, getDlgItemInt(IDC_RECEIVEPORT));
	if (sendDlgItemMessage(IDC_RECEIVESSL, BM_GETCHECK) == BST_CHECKED)
		pSubAccount_->setSecure(Account::HOST_RECEIVE, SubAccount::SECURE_SSL);
	else if (sendDlgItemMessage(IDC_RECEIVESTARTTLS, BM_GETCHECK) == BST_CHECKED)
		pSubAccount_->setSecure(Account::HOST_RECEIVE, SubAccount::SECURE_STARTTLS);
	else
		pSubAccount_->setSecure(Account::HOST_RECEIVE, SubAccount::SECURE_NONE);
	pSubAccount_->setLog(Account::HOST_RECEIVE,
		sendDlgItemMessage(IDC_RECEIVELOG, BM_GETCHECK) == BST_CHECKED);
	
	pSubAccount_->setPort(Account::HOST_SEND, getDlgItemInt(IDC_SENDPORT));
	if (sendDlgItemMessage(IDC_SENDSSL, BM_GETCHECK) == BST_CHECKED)
		pSubAccount_->setSecure(Account::HOST_SEND, SubAccount::SECURE_SSL);
	else if (sendDlgItemMessage(IDC_SENDSTARTTLS, BM_GETCHECK) == BST_CHECKED)
		pSubAccount_->setSecure(Account::HOST_SEND, SubAccount::SECURE_STARTTLS);
	else
		pSubAccount_->setSecure(Account::HOST_SEND, SubAccount::SECURE_NONE);
	pSubAccount_->setLog(Account::HOST_SEND,
		sendDlgItemMessage(IDC_SENDLOG, BM_GETCHECK) == BST_CHECKED);
	
	return 0;
}

LRESULT qm::AccountDetailPage::onSecure(UINT nId)
{
	bool bReceive = IDC_RECEIVENOSECURE <= nId && nId <= IDC_RECEIVESTARTTLS;
	bool bSecure = nId == IDC_RECEIVESSL || nId == IDC_SENDSSL;
	if (bReceive)
		setDlgItemInt(IDC_RECEIVEPORT, pReceiveUI_->getDefaultPort(bSecure));
	else
		setDlgItemInt(IDC_SENDPORT, pSendUI_->getDefaultPort(bSecure));
	
	return true;
}


/****************************************************************************
 *
 * AccountDialupPage
 *
 */

namespace {
struct {
	SubAccount::DialupType type_;
	UINT nId_;
} types[] = {
	{ SubAccount::DIALUPTYPE_NEVER,					IDC_NEVER					},
	{ SubAccount::DIALUPTYPE_WHENEVERNOTCONNECTED,	IDC_WHENEVERNOTCONNECTED	},
	{ SubAccount::DIALUPTYPE_CONNECT,				IDC_CONNECT					}
};
}

qm::AccountDialupPage::AccountDialupPage(SubAccount* pSubAccount) :
	DefaultPropertyPage(IDD_ACCOUNTDIALUP),
	pSubAccount_(pSubAccount)
{
}

qm::AccountDialupPage::~AccountDialupPage()
{
}

LRESULT qm::AccountDialupPage::onCommand(WORD nCode,
										 WORD nId)
{
	BEGIN_COMMAND_HANDLER()
		HANDLE_COMMAND_ID(IDC_DIALPROPERTY, onDialProperty)
		HANDLE_COMMAND_ID_RANGE(IDC_NEVER, IDC_CONNECT, onTypeSelect)
	END_COMMAND_HANDLER()
	return DefaultPropertyPage::onCommand(nCode, nId);
}

LRESULT qm::AccountDialupPage::onInitDialog(HWND hwndFocus,
											LPARAM lParam)
{
	for (int n = 0; n < countof(types); ++n) {
		if (pSubAccount_->getDialupType() == types[n].type_) {
			sendDlgItemMessage(types[n].nId_, BM_SETCHECK, BST_CHECKED);
			break;
		}
	}
	
	RasConnection::EntryList listEntry;
	StringListFree<RasConnection::EntryList> free(listEntry);
	RasConnection::getEntries(&listEntry);
	
	if (listEntry.empty()) {
		UINT nIds[] = {
			IDC_NEVER,
			IDC_WHENEVERNOTCONNECTED,
			IDC_CONNECT,
			IDC_ENTRY,
			IDC_SHOWDIALOG,
			IDC_DIALPROPERTY,
			IDC_WAITBEFOREDISCONNECT
		};
		for (int n = 0; n < countof(nIds); ++n)
			Window(getDlgItem(nIds[n])).enableWindow(false);
	}
	else {
		RasConnection::EntryList::const_iterator it = listEntry.begin();
		while (it != listEntry.end()) {
			W2T(*it, ptszName);
			sendDlgItemMessage(IDC_ENTRY, CB_ADDSTRING,
				0, reinterpret_cast<LPARAM>(ptszName));
			++it;
		}
		
		W2T(pSubAccount_->getDialupEntry(), ptszEntry);
		if (sendDlgItemMessage(IDC_ENTRY, CB_SELECTSTRING,
			-1, reinterpret_cast<LPARAM>(ptszEntry)) == CB_ERR)
			sendDlgItemMessage(IDC_ENTRY, CB_SETCURSEL, 0);
	}
	
	sendDlgItemMessage(IDC_SHOWDIALOG, BM_SETCHECK,
		pSubAccount_->isDialupShowDialog() ? BST_CHECKED : BST_UNCHECKED);
	setDlgItemInt(IDC_WAITBEFOREDISCONNECT, pSubAccount_->getDialupDisconnectWait());
	
	updateState();
	
	return TRUE;
}

LRESULT qm::AccountDialupPage::onOk()
{
	for (int n = 0; n < countof(types); ++n) {
		if (sendDlgItemMessage(types[n].nId_, BM_GETCHECK) == BST_CHECKED) {
			pSubAccount_->setDialupType(types[n].type_);
			break;
		}
	}
	
	int nIndex = sendDlgItemMessage(IDC_ENTRY, CB_GETCURSEL);
	if (nIndex != CB_ERR) {
		int nLen = sendDlgItemMessage(IDC_ENTRY, CB_GETLBTEXTLEN, nIndex);
		if (nLen != CB_ERR) {
			tstring_ptr tstrEntry(allocTString(nLen + 1));
			if (tstrEntry.get()) {
				sendDlgItemMessage(IDC_ENTRY, CB_GETLBTEXT,
					nIndex, reinterpret_cast<LPARAM>(tstrEntry.get()));
				T2W(tstrEntry.get(), ptszEntry);
				pSubAccount_->setDialupEntry(ptszEntry);
			}
		}
	}
	
	pSubAccount_->setDialupShowDialog(
		sendDlgItemMessage(IDC_SHOWDIALOG, BM_GETCHECK) == BST_CHECKED);
	pSubAccount_->setDialupDisconnectWait(
		getDlgItemInt(IDC_WAITBEFOREDISCONNECT));
	
	return DefaultPropertyPage::onOk();
}

LRESULT qm::AccountDialupPage::onDialProperty()
{
	RasConnection::selectLocation(getHandle());
	return 0;
}

LRESULT qm::AccountDialupPage::onTypeSelect(UINT nId)
{
	updateState();
	return 0;
}

void qm::AccountDialupPage::updateState()
{
	bool bEnable = sendDlgItemMessage(IDC_NEVER, BM_GETCHECK) != BST_CHECKED;
	
	UINT nIds[] = {
		IDC_ENTRY,
		IDC_SHOWDIALOG,
		IDC_DIALPROPERTY,
		IDC_WAITBEFOREDISCONNECT
	};
	for (int n = 0; n < countof(nIds); ++n)
		Window(getDlgItem(nIds[n])).enableWindow(bEnable);
}


/****************************************************************************
 *
 * AccountGeneralPage
 *
 */

qm::AccountGeneralPage::AccountGeneralPage(SubAccount* pSubAccount,
										   ReceiveSessionUI* pReceiveUI,
										   SendSessionUI* pSendUI) :
	DefaultPropertyPage(IDD_ACCOUNTGENERAL),
	pSubAccount_(pSubAccount),
	pReceiveUI_(pReceiveUI),
	pSendUI_(pSendUI)
{
}

qm::AccountGeneralPage::~AccountGeneralPage()
{
}

LRESULT qm::AccountGeneralPage::onInitDialog(HWND hwndFocus,
											 LPARAM lParam)
{
	setDlgItemText(IDC_RECEIVEHOST, pSubAccount_->getHost(Account::HOST_RECEIVE));
	setDlgItemText(IDC_SENDHOST, pSubAccount_->getHost(Account::HOST_SEND));
	setDlgItemText(IDC_NAME, pSubAccount_->getSenderName());
	setDlgItemText(IDC_ADDRESS, pSubAccount_->getSenderAddress());
	
	if (!pReceiveUI_->isSupported(ReceiveSessionUI::SUPPORT_HOST))
		Window(getDlgItem(IDC_RECEIVEHOST)).enableWindow(false);
	if (!pSendUI_->isSupported(SendSessionUI::SUPPORT_HOST))
		Window(getDlgItem(IDC_SENDHOST)).enableWindow(false);
	if (!pSendUI_->isSupported(SendSessionUI::SUPPORT_USER)) {
		Window(getDlgItem(IDC_NAME)).enableWindow(false);
		Window(getDlgItem(IDC_ADDRESS)).enableWindow(false);
	}
	
	return TRUE;
}

LRESULT qm::AccountGeneralPage::onOk()
{
	wstring_ptr wstrReceiveHost(getDlgItemText(IDC_RECEIVEHOST));
	if (wstrReceiveHost.get())
		pSubAccount_->setHost(Account::HOST_RECEIVE, wstrReceiveHost.get());
	
	wstring_ptr wstrSendHost(getDlgItemText(IDC_SENDHOST));
	if (wstrSendHost.get())
		pSubAccount_->setHost(Account::HOST_SEND, wstrSendHost.get());
	
	wstring_ptr wstrName(getDlgItemText(IDC_NAME));
	if (wstrName.get())
		pSubAccount_->setSenderName(wstrName.get());
	
	wstring_ptr wstrAddress(getDlgItemText(IDC_ADDRESS));
	if (wstrAddress.get())
		pSubAccount_->setSenderAddress(wstrAddress.get());
	
	return DefaultPropertyPage::onOk();
}


/****************************************************************************
 *
 * AccountUserPage
 *
 */

qm::AccountUserPage::AccountUserPage(SubAccount* pSubAccount,
									 PasswordManager* pPasswordManager,
									 ReceiveSessionUI* pReceiveUI,
									 SendSessionUI* pSendUI) :
	DefaultPropertyPage(IDD_ACCOUNTUSER),
	pPasswordManager_(pPasswordManager),
	pSubAccount_(pSubAccount),
	pReceiveUI_(pReceiveUI),
	pSendUI_(pSendUI)
{
}

qm::AccountUserPage::~AccountUserPage()
{
}

LRESULT qm::AccountUserPage::onCommand(WORD nCode,
									   WORD nId)
{
	BEGIN_COMMAND_HANDLER()
		HANDLE_COMMAND_ID(IDC_SENDAUTHENTICATE, onSendAuthenticate)
	END_COMMAND_HANDLER()
	return DefaultPropertyPage::onCommand(nCode, nId);
}

LRESULT qm::AccountUserPage::onInitDialog(HWND hwndFocus,
										  LPARAM lParam)
{
	setDlgItemText(IDC_RECEIVEUSERNAME, pSubAccount_->getUserName(Account::HOST_RECEIVE));
	setPassword(IDC_RECEIVEPASSWORD, Account::HOST_RECEIVE);
	sendDlgItemMessage(IDC_SENDAUTHENTICATE, BM_SETCHECK,
		*pSubAccount_->getUserName(Account::HOST_SEND) ? BST_CHECKED : BST_UNCHECKED);
	setDlgItemText(IDC_SENDUSERNAME, pSubAccount_->getUserName(Account::HOST_SEND));
	setPassword(IDC_SENDPASSWORD, Account::HOST_SEND);
	
	if (!pReceiveUI_->isSupported(ReceiveSessionUI::SUPPORT_USER)) {
		Window(getDlgItem(IDC_RECEIVEUSERNAME)).enableWindow(false);
		Window(getDlgItem(IDC_RECEIVEPASSWORD)).enableWindow(false);
	}
	if (!pSendUI_->isSupported(SendSessionUI::SUPPORT_USER)) {
		Window(getDlgItem(IDC_SENDAUTHENTICATE)).enableWindow(false);
		Window(getDlgItem(IDC_SENDUSERNAME)).enableWindow(false);
		Window(getDlgItem(IDC_SENDPASSWORD)).enableWindow(false);
	}
	
	updateState();
	
	return TRUE;
}

LRESULT qm::AccountUserPage::onOk()
{
	wstring_ptr wstrReceiveUserName(getDlgItemText(IDC_RECEIVEUSERNAME));
	pSubAccount_->setUserName(Account::HOST_RECEIVE, wstrReceiveUserName.get());
	getPassword(IDC_RECEIVEPASSWORD, Account::HOST_RECEIVE, false);
	
	if (sendDlgItemMessage(IDC_SENDAUTHENTICATE, BM_GETCHECK) == BST_CHECKED) {
		wstring_ptr wstrSendUserName(getDlgItemText(IDC_SENDUSERNAME));
		pSubAccount_->setUserName(Account::HOST_SEND, wstrSendUserName.get());
		getPassword(IDC_SENDPASSWORD, Account::HOST_SEND, false);
	}
	else {
		pSubAccount_->setUserName(Account::HOST_SEND, L"");
		getPassword(IDC_SENDPASSWORD, Account::HOST_SEND, true);
	}
	
	return DefaultPropertyPage::onOk();
}

LRESULT qm::AccountUserPage::onSendAuthenticate()
{
	updateState();
	return 0;
}

void qm::AccountUserPage::setPassword(UINT nId,
									  Account::Host host)
{
	Account* pAccount = pSubAccount_->getAccount();
	AccountPasswordCondition condition(pAccount, pSubAccount_, host);
	wstring_ptr wstrPassword(pPasswordManager_->getPassword(condition, true, 0));
	if (wstrPassword.get())
		setDlgItemText(nId, wstrPassword.get());
}

void qm::AccountUserPage::getPassword(UINT nId,
									  Account::Host host,
									  bool bForceRemove)
{
	Account* pAccount = pSubAccount_->getAccount();
	AccountPasswordCondition condition(pAccount, pSubAccount_, host);
	
	wstring_ptr wstrPassword;
	if (!bForceRemove)
		wstrPassword = getDlgItemText(nId);
	
	if (wstrPassword.get() && *wstrPassword.get())
		pPasswordManager_->setPassword(condition, wstrPassword.get(), true);
	else
		pPasswordManager_->removePassword(condition);
}

void qm::AccountUserPage::updateState()
{
	bool bEnable = sendDlgItemMessage(IDC_SENDAUTHENTICATE, BM_GETCHECK) == BST_CHECKED;
	Window(getDlgItem(IDC_SENDUSERNAME)).enableWindow(bEnable);
	Window(getDlgItem(IDC_SENDPASSWORD)).enableWindow(bEnable);
}
