/*
 * $Id$
 *
 * Copyright(C) 1998-2004 Satoshi Nakamura
 * All rights reserved.
 *
 */

#include <qmaccount.h>
#include <qmapplication.h>
#include <qmmessageholder.h>
#include <qmsearch.h>
#include <qmsyncfilter.h>

#include <qsconv.h>
#include <qsras.h>
#include <qsstl.h>

#include "dialogs.h"
#include "propertypages.h"
#include "resourceinc.h"

using namespace qm;
using namespace qs;


/****************************************************************************
 *
 * DefaultPropertyPage
 *
 */

qm::DefaultPropertyPage::DefaultPropertyPage(UINT nId) :
	qs::DefaultPropertyPage(Application::getApplication().getResourceHandle(), nId)
{
}

qm::DefaultPropertyPage::~DefaultPropertyPage()
{
}


/****************************************************************************
 *
 * AccountAdvancedPage
 *
 */

qm::AccountAdvancedPage::AccountAdvancedPage(SubAccount* pSubAccount,
											 SyncFilterManager* pSyncFilterManager) :
	DefaultPropertyPage(IDD_ACCOUNTADVANCED),
	pSubAccount_(pSubAccount),
	pSyncFilterManager_(pSyncFilterManager)
{
}

qm::AccountAdvancedPage::~AccountAdvancedPage()
{
}

LRESULT qm::AccountAdvancedPage::onInitDialog(HWND hwndFocus,
											  LPARAM lParam)
{
	setDlgItemText(IDC_IDENTITY, pSubAccount_->getIdentity());
	wstring_ptr wstrMyAddress = pSubAccount_->getMyAddress();
	setDlgItemText(IDC_MYADDRESS, wstrMyAddress.get());
	
	SyncFilterManager::FilterSetList l;
	pSyncFilterManager_->getFilterSets(pSubAccount_->getAccount(), &l);
	for (SyncFilterManager::FilterSetList::const_iterator it = l.begin(); it != l.end(); ++it) {
		SyncFilterSet* pSet = *it;
		W2T(pSet->getName(), ptszName);
		sendDlgItemMessage(IDC_SYNCFILTER, CB_ADDSTRING,
			0, reinterpret_cast<LPARAM>(ptszName));
	}
	W2T(pSubAccount_->getSyncFilterName(), ptszName);
	if (sendDlgItemMessage(IDC_SYNCFILTER, CB_SELECTSTRING,
		-1, reinterpret_cast<LPARAM>(ptszName)) == CB_ERR)
		sendDlgItemMessage(IDC_SYNCFILTER, CB_SETCURSEL, 0);
	
	setDlgItemInt(IDC_TIMEOUT, pSubAccount_->getTimeout());
	sendDlgItemMessage(IDC_CONNECTRECEIVEHOSTBEFORESEND, BM_SETCHECK,
		pSubAccount_->isConnectReceiveBeforeSend() ? BST_CHECKED : BST_UNCHECKED);
	sendDlgItemMessage(IDC_TREATASSENT, BM_SETCHECK,
		pSubAccount_->isTreatAsSent() ? BST_CHECKED : BST_UNCHECKED);
	sendDlgItemMessage(IDC_ADDMESSAGEID, BM_SETCHECK,
		pSubAccount_->isAddMessageId() ? BST_CHECKED : BST_UNCHECKED);
	
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
	
	return DefaultPropertyPage::onOk();
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

qm::AccountGeneralPage::AccountGeneralPage(SubAccount* pSubAccount) :
	DefaultPropertyPage(IDD_ACCOUNTGENERAL),
	pSubAccount_(pSubAccount)
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

qm::AccountUserPage::AccountUserPage(SubAccount* pSubAccount) :
	DefaultPropertyPage(IDD_ACCOUNTUSER),
	pSubAccount_(pSubAccount)
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
	setDlgItemText(IDC_RECEIVEPASSWORD, pSubAccount_->getPassword(Account::HOST_RECEIVE));
	sendDlgItemMessage(IDC_SENDAUTHENTICATE, BM_SETCHECK,
		*pSubAccount_->getUserName(Account::HOST_SEND) ? BST_CHECKED : BST_UNCHECKED);
	setDlgItemText(IDC_SENDUSERNAME, pSubAccount_->getUserName(Account::HOST_SEND));
	setDlgItemText(IDC_SENDPASSWORD, pSubAccount_->getPassword(Account::HOST_SEND));
	
	updateState();
	
	return TRUE;
}

LRESULT qm::AccountUserPage::onOk()
{
	wstring_ptr wstrReceiveUserName(getDlgItemText(IDC_RECEIVEUSERNAME));
	if (wstrReceiveUserName.get())
		pSubAccount_->setUserName(Account::HOST_RECEIVE, wstrReceiveUserName.get());
	
	wstring_ptr wstrReceivePassword(getDlgItemText(IDC_RECEIVEPASSWORD));
	if (wstrReceivePassword.get())
		pSubAccount_->setPassword(Account::HOST_RECEIVE, wstrReceivePassword.get());
	
	if (sendDlgItemMessage(IDC_SENDAUTHENTICATE, BM_GETCHECK) == BST_CHECKED) {
		wstring_ptr wstrSendUserName(getDlgItemText(IDC_SENDUSERNAME));
		if (wstrSendUserName.get())
			pSubAccount_->setUserName(Account::HOST_SEND, wstrSendUserName.get());
		
		wstring_ptr wstrSendPassword(getDlgItemText(IDC_SENDPASSWORD));
		if (wstrSendPassword.get())
			pSubAccount_->setPassword(Account::HOST_SEND, wstrSendPassword.get());
	}
	else {
		pSubAccount_->setUserName(Account::HOST_SEND, L"");
		pSubAccount_->setPassword(Account::HOST_SEND, L"");
	}
	
	return DefaultPropertyPage::onOk();
}

LRESULT qm::AccountUserPage::onSendAuthenticate()
{
	updateState();
	return 0;
}

void qm::AccountUserPage::updateState()
{
	bool bEnable = sendDlgItemMessage(IDC_SENDAUTHENTICATE, BM_GETCHECK) == BST_CHECKED;
	Window(getDlgItem(IDC_SENDUSERNAME)).enableWindow(bEnable);
	Window(getDlgItem(IDC_SENDPASSWORD)).enableWindow(bEnable);
}


/****************************************************************************
 *
 * FolderConditionPage
 *
 */

qm::FolderConditionPage::FolderConditionPage(QueryFolder* pFolder,
											 Profile* pProfile) :
	DefaultPropertyPage(IDD_FOLDERCONDITION),
	pFolder_(pFolder),
	pProfile_(pProfile)
{
}

qm::FolderConditionPage::~FolderConditionPage()
{
	std::for_each(listUI_.begin(), listUI_.end(), deleter<SearchUI>());
}

LRESULT qm::FolderConditionPage::onInitDialog(HWND hwndFocus,
											  LPARAM lParam)
{
	initDriver();
	initFolder();
	
	setDlgItemText(IDC_CONDITION, pFolder_->getCondition());
	sendDlgItemMessage(IDC_RECURSIVE, BM_SETCHECK,
		pFolder_->isRecursive() ? BST_CHECKED : BST_UNCHECKED);
	
	return TRUE;
}

LRESULT qm::FolderConditionPage::onOk()
{
	int nDriver = sendDlgItemMessage(IDC_DRIVER, CB_GETCURSEL);
	const WCHAR* pwszDriver = listUI_[nDriver]->getName();
	
	wstring_ptr wstrCondition = getDlgItemText(IDC_CONDITION);
	
	wstring_ptr wstrTargetFolder;
	int nFolder = sendDlgItemMessage(IDC_FOLDER, CB_GETCURSEL);
	if (nFolder != 0)
		wstrTargetFolder = listFolder_[nFolder - 1]->getFullName();
	
	bool bRecursive = sendDlgItemMessage(IDC_RECURSIVE, BM_GETCHECK) == BST_CHECKED;
	
	pFolder_->set(pwszDriver, wstrCondition.get(),
		wstrTargetFolder.get(), bRecursive);
	
	return DefaultPropertyPage::onOk();
}

void qm::FolderConditionPage::initDriver()
{
	SearchDriverFactory::NameList listName;
	SearchDriverFactory::getNames(&listName);
	for (SearchDriverFactory::NameList::const_iterator itN = listName.begin(); itN != listName.end(); ++itN) {
		std::auto_ptr<SearchUI> pUI(SearchDriverFactory::getUI(
			*itN, pFolder_->getAccount(), pProfile_));
		if (pUI.get()) {
			listUI_.push_back(pUI.get());
			pUI.release();
		}
	}
	std::sort(listUI_.begin(), listUI_.end(),
		binary_compose_f_gx_hy(
			std::less<int>(),
			std::mem_fun(&SearchUI::getIndex),
			std::mem_fun(&SearchUI::getIndex)));
	int nIndex = 0;
	for (UIList::size_type n = 0; n < listUI_.size(); ++n) {
		SearchUI* pUI = listUI_[n];
		wstring_ptr wstrName(pUI->getDisplayName());
		W2T(wstrName.get(), ptszName);
		sendDlgItemMessage(IDC_DRIVER, CB_ADDSTRING, 0,
			reinterpret_cast<LPARAM>(ptszName));
		if (wcscmp(pUI->getName(), pFolder_->getDriver()) == 0)
			nIndex = n;
	}
	sendDlgItemMessage(IDC_DRIVER, CB_SETCURSEL, nIndex);
}

void qm::FolderConditionPage::initFolder()
{
	HINSTANCE hInst = Application::getApplication().getResourceHandle();
	wstring_ptr wstrAllFolder(loadString(hInst, IDS_ALLFOLDER));
	W2T(wstrAllFolder.get(), ptszAllFolder);
	sendDlgItemMessage(IDC_FOLDER, CB_ADDSTRING, 0,
		reinterpret_cast<LPARAM>(ptszAllFolder));
	
	Account* pAccount = pFolder_->getAccount();
	const Account::FolderList& l = pAccount->getFolders();
	listFolder_.assign(l.begin(), l.end());
	std::sort(listFolder_.begin(), listFolder_.end(), FolderLess());
	
	Folder* pTargetFolder = 0;
	const WCHAR* pwszTargetFolder = pFolder_->getTargetFolder();
	if (pwszTargetFolder)
		pTargetFolder = pAccount->getFolder(pwszTargetFolder);
	
	int nIndex = 0;
	for (Account::FolderList::size_type n = 0; n < listFolder_.size(); ++n) {
		Folder* pFolder = listFolder_[n];
		
		unsigned int nLevel = pFolder->getLevel();
		StringBuffer<WSTRING> buf;
		while (nLevel != 0) {
			buf.append(L"  ");
			--nLevel;
		}
		buf.append(pFolder->getName());
		
		W2T(buf.getCharArray(), ptszName);
		sendDlgItemMessage(IDC_FOLDER, CB_ADDSTRING, 0,
			reinterpret_cast<LPARAM>(ptszName));
		
		if (pTargetFolder == pFolder)
			nIndex = n + 1;
	}
	sendDlgItemMessage(IDC_FOLDER, CB_SETCURSEL, nIndex);
}


/****************************************************************************
 *
 * FolderParameterPage
 *
 */

qm::FolderParameterPage::FolderParameterPage(Folder* pFolder,
											 const WCHAR** ppwszParams,
											 size_t nParamCount) :
	DefaultPropertyPage(IDD_FOLDERPARAMETER),
	pFolder_(pFolder),
	ppwszParams_(ppwszParams),
	nParamCount_(nParamCount)
{
	assert(ppwszParams_);
	assert(nParamCount_ != 0);
}

qm::FolderParameterPage::~FolderParameterPage()
{
}

LRESULT qm::FolderParameterPage::onCommand(WORD nCode,
										   WORD nId)
{
	BEGIN_COMMAND_HANDLER()
		HANDLE_COMMAND_ID(IDC_EDIT, onEdit)
	END_COMMAND_HANDLER()
	return DefaultPropertyPage::onCommand(nCode, nId);
}

LRESULT qm::FolderParameterPage::onInitDialog(HWND hwndFocus,
											  LPARAM lParam)
{
	HINSTANCE hInst = Application::getApplication().getResourceHandle();
	HWND hwndList = getDlgItem(IDC_PARAMETER);
	
	ListView_SetExtendedListViewStyle(hwndList, LVS_EX_FULLROWSELECT);
	
	struct {
		UINT nId_;
		int nWidth_;
	} columns[] = {
		{ IDS_PROPERTY_NAME,	120	},
		{ IDS_PROPERTY_VALUE,	200	}
	};
	for (int n = 0; n < countof(columns); ++n) {
		wstring_ptr wstrName(loadString(hInst, columns[n].nId_));
		W2T(wstrName.get(), ptszName);
		
		LVCOLUMN column = {
			LVCF_FMT | LVCF_SUBITEM | LVCF_TEXT | LVCF_WIDTH,
			LVCFMT_LEFT,
			columns[n].nWidth_,
			const_cast<LPTSTR>(ptszName),
			0,
			n,
		};
		ListView_InsertColumn(hwndList, n, &column);
	}
	
	for (size_t n = 0; n < nParamCount_; ++n) {
		const WCHAR* pwszName = ppwszParams_[n];
		W2T(pwszName, ptszName);
		LVITEM item = {
			LVIF_TEXT,
			n,
			0,
			0,
			0,
			const_cast<LPTSTR>(ptszName),
		};
		ListView_InsertItem(hwndList, &item);
		
		const WCHAR* pwszValue = pFolder_->getParam(pwszName);
		if (!pwszValue)
			pwszValue = L"";
		W2T(pwszValue, ptszValue);
		ListView_SetItemText(hwndList, n, 1, const_cast<LPTSTR>(ptszValue));
	}
	
	updateState();
	
	return TRUE;
}

LRESULT qm::FolderParameterPage::onOk()
{
	HWND hwndList = getDlgItem(IDC_PARAMETER);
	
	for (size_t n = 0; n < nParamCount_; ++n) {
		const WCHAR* pwszName = ppwszParams_[n];
		
		TCHAR tszValue[1024];
		ListView_GetItemText(hwndList, n, 1, tszValue, countof(tszValue) - 1);
		T2W(tszValue, pwszValue);
		
		pFolder_->setParam(pwszName, pwszValue);
	}
	
	return DefaultPropertyPage::onOk();
}

LRESULT qm::FolderParameterPage::onNotify(NMHDR* pnmhdr,
										  bool* pbHandled)
{
	BEGIN_NOTIFY_HANDLER()
		HANDLE_NOTIFY(NM_DBLCLK, IDC_PARAMETER, onParameterDblClk)
		HANDLE_NOTIFY(LVN_ITEMCHANGED, IDC_PARAMETER, onParameterItemChanged)
	END_NOTIFY_HANDLER()
	return DefaultPropertyPage::onNotify(pnmhdr, pbHandled);
}

LRESULT qm::FolderParameterPage::onEdit()
{
	edit();
	return 0;
}

LRESULT qm::FolderParameterPage::onParameterDblClk(NMHDR* pnmhdr,
												   bool* pbHandled)
{
	edit();
	*pbHandled = true;
	return 0;
}

LRESULT qm::FolderParameterPage::onParameterItemChanged(NMHDR* pnmhdr,
														bool* pbHandled)
{
	updateState();
	*pbHandled = true;
	return 0;
}

void qm::FolderParameterPage::edit()
{
	HWND hwndList = getDlgItem(IDC_PARAMETER);
	
	int nItem = ListView_GetNextItem(hwndList, -1, LVNI_SELECTED);
	if (nItem != -1) {
		const WCHAR* pwszName = ppwszParams_[nItem];
		
		TCHAR tszValue[1024];
		ListView_GetItemText(hwndList, nItem, 1, tszValue, countof(tszValue) - 1);
		T2W(tszValue, pwszValue);
		
		ParameterDialog dialog(pwszName, pwszValue);
		if (dialog.doModal(getHandle()) == IDOK) {
			W2T(dialog.getValue(), ptszValue);
			ListView_SetItemText(hwndList, nItem, 1, const_cast<LPTSTR>(ptszValue));
		}
	}
}

void qm::FolderParameterPage::updateState()
{
	HWND hwndList = getDlgItem(IDC_PARAMETER);
	int nItem = ListView_GetNextItem(hwndList, -1, LVNI_SELECTED);
	Window(getDlgItem(IDC_EDIT)).enableWindow(nItem != -1);
}


/****************************************************************************
 *
 * FolderPropertyPage
 *
 */

namespace {
struct
{
	Folder::Flag flag_;
	UINT nId_;
	Folder::Flag enableFlag_;
	bool bEnableQuery_;
	bool bReverse_;
} folderFlags[] = {
	{ Folder::FLAG_SYNCWHENOPEN,	IDC_SYNCWHENOPEN,	Folder::FLAG_SYNCABLE,	true,	false	},
	{ Folder::FLAG_CACHEWHENREAD,	IDC_CACHEWHENREAD,	Folder::FLAG_SYNCABLE,	false,	false	},
	{ Folder::FLAG_IGNOREUNSEEN,	IDC_IGNOREUNSEEN,	Folder::FLAG_NOSELECT,	true,	true	},
	{ Folder::FLAG_INBOX,			IDC_INBOX,			Folder::FLAG_NOSELECT,	false,	true	},
	{ Folder::FLAG_OUTBOX,			IDC_OUTBOX,			Folder::FLAG_NOSELECT,	false,	true	},
	{ Folder::FLAG_SENTBOX,			IDC_SENTBOX,		Folder::FLAG_NOSELECT,	false,	true	},
	{ Folder::FLAG_DRAFTBOX,		IDC_DRAFTBOX,		Folder::FLAG_NOSELECT,	false,	true	},
	{ Folder::FLAG_TRASHBOX,		IDC_TRASHBOX,		Folder::FLAG_NOSELECT,	false,	true	}
};
};

qm::FolderPropertyPage::FolderPropertyPage(const Account::FolderList& l) :
	DefaultPropertyPage(IDD_FOLDERPROPERTY),
	listFolder_(l)
{
	assert(!l.empty());
}

qm::FolderPropertyPage::~FolderPropertyPage()
{
}

LRESULT qm::FolderPropertyPage::onInitDialog(HWND hwndFocus,
											 LPARAM lParam)
{
	if (listFolder_.size() == 1) {
		Folder* pFolder = listFolder_.front();
		
		wstring_ptr wstrName(pFolder->getFullName());
		setDlgItemText(IDC_NAME, wstrName.get());
		
		setDlgItemInt(IDC_ID, pFolder->getId());
		
		HINSTANCE hInst = Application::getApplication().getResourceHandle();
		wstring_ptr wstrTemplate(loadString(hInst, IDS_FOLDERTYPETEMPLATE));
		UINT nTypeId = pFolder->getType() == Folder::TYPE_NORMAL ?
			IDS_NORMALFOLDER : IDS_QUERYFOLDER;
		UINT nLocalId = pFolder->isFlag(Folder::FLAG_LOCAL) ?
			IDS_LOCALFOLDER : IDS_REMOTEFOLDER;
		wstring_ptr wstrType(loadString(hInst, nTypeId));
		wstring_ptr wstrLocal(loadString(hInst, nLocalId));
		WCHAR wszType[128];
		swprintf(wszType, wstrTemplate.get(), wstrType.get(), wstrLocal.get());
		setDlgItemText(IDC_TYPE, wszType);
		
		bool bQuery = pFolder->getType() == Folder::TYPE_QUERY;
		unsigned int nFlags = pFolder->getFlags();
		for (int n = 0; n < countof(folderFlags); ++n) {
			sendDlgItemMessage(folderFlags[n].nId_, BM_SETCHECK,
				nFlags & folderFlags[n].flag_ ? BST_CHECKED : BST_UNCHECKED);
			Window(getDlgItem(folderFlags[n].nId_)).setStyle(
				BS_AUTOCHECKBOX, BS_AUTOCHECKBOX | BS_AUTO3STATE);
			
			bool bEnable = false;
			if (bQuery) {
				bEnable = folderFlags[n].bEnableQuery_;
			}
			else {
				bEnable = pFolder->isFlag(folderFlags[n].enableFlag_);
				if (folderFlags[n].bReverse_)
					bEnable = !bEnable;
			}
			if (!bEnable)
				Window(getDlgItem(folderFlags[n].nId_)).enableWindow(false);
		}
	}
	else {
		for (int n = 0; n < countof(folderFlags); ++n) {
			unsigned int nCount = 0;
			bool bEnable = false;
			for (Account::FolderList::const_iterator it = listFolder_.begin(); it != listFolder_.end(); ++it) {
				Folder* pFolder = *it;
				if (pFolder->getFlags() & folderFlags[n].flag_)
					++nCount;
				
				bool b = pFolder->getType() != Folder::TYPE_QUERY || folderFlags[n].bEnableQuery_;
				if (b) {
					b = pFolder->isFlag(folderFlags[n].enableFlag_);
					if (folderFlags[n].bReverse_)
						b = !b;
				}
				if (b)
					bEnable = true;
			}
			sendDlgItemMessage(folderFlags[n].nId_, BM_SETCHECK,
				nCount == 0 ? BST_UNCHECKED :
				nCount == listFolder_.size() ? BST_CHECKED : BST_INDETERMINATE);
			
			if (!bEnable)
				Window(getDlgItem(folderFlags[n].nId_)).enableWindow(false);
		}
	}
	
	return TRUE;
}

LRESULT qm::FolderPropertyPage::onOk()
{
	unsigned int nFlags = 0;
	unsigned int nMask = 0;
	
	for (int n = 0; n < countof(folderFlags); ++n) {
		int nCheck = sendDlgItemMessage(folderFlags[n].nId_, BM_GETCHECK);
		switch (nCheck) {
		case BST_CHECKED:
			nFlags |= folderFlags[n].flag_;
			nMask |= folderFlags[n].flag_;
			break;
		case BST_UNCHECKED:
			nMask |= folderFlags[n].flag_;
			break;
		case BST_INDETERMINATE:
			break;
		default:
			assert(false);
			break;
		}
	}
	
	for (Account::FolderList::const_iterator it = listFolder_.begin(); it != listFolder_.end(); ++it) {
		Folder* pFolder = *it;
		
		unsigned int nFolderMask = nMask;
		
		switch (pFolder->getType()) {
		case Folder::TYPE_NORMAL:
			if (!pFolder->isFlag(Folder::FLAG_SYNCABLE))
				nFolderMask &= ~(Folder::FLAG_SYNCWHENOPEN | Folder::FLAG_CACHEWHENREAD);
			if (pFolder->isFlag(Folder::FLAG_NOSELECT))
				nFolderMask &= ~(Folder::FLAG_INBOX | Folder::FLAG_OUTBOX |
					Folder::FLAG_SENTBOX | Folder::FLAG_DRAFTBOX | Folder::FLAG_TRASHBOX);
			break;
		case Folder::TYPE_QUERY:
			nFolderMask &= ~(Folder::FLAG_CACHEWHENREAD | Folder::FLAG_INBOX | Folder::FLAG_OUTBOX |
				Folder::FLAG_SENTBOX | Folder::FLAG_DRAFTBOX | Folder::FLAG_TRASHBOX);
			break;
		default:
			assert(false);
			break;
		}
		
		pFolder->getAccount()->setFolderFlags(pFolder, nFlags, nFolderMask);
	}
	
	return DefaultPropertyPage::onOk();
}


/****************************************************************************
 *
 * MessagePropertyPage
 *
 */

namespace {
struct
{
	MessageHolderBase::Flag flag_;
	UINT nId_;
} flags[] = {
	{ MessageHolderBase::FLAG_SEEN,			IDC_SEEN			},
	{ MessageHolderBase::FLAG_REPLIED,		IDC_REPLIED			},
	{ MessageHolderBase::FLAG_FORWARDED,	IDC_FORWARDED		},
	{ MessageHolderBase::FLAG_SENT,			IDC_SENT			},
	{ MessageHolderBase::FLAG_DRAFT,		IDC_DRAFT			},
	{ MessageHolderBase::FLAG_MARKED,		IDC_MARKED			},
	{ MessageHolderBase::FLAG_DELETED,		IDC_DELETED			},
	{ MessageHolderBase::FLAG_DOWNLOAD,		IDC_DOWNLOAD		},
	{ MessageHolderBase::FLAG_DOWNLOADTEXT,	IDC_DOWNLOADTEXT	},
	{ MessageHolderBase::FLAG_TOME,			IDC_TOME			},
	{ MessageHolderBase::FLAG_CCME,			IDC_CCME			},
	{ MessageHolderBase::FLAG_USER1,		IDC_USER1			},
	{ MessageHolderBase::FLAG_USER2,		IDC_USER2			},
	{ MessageHolderBase::FLAG_USER3,		IDC_USER3			},
	{ MessageHolderBase::FLAG_USER4,		IDC_USER4			},
};
}

qm::MessagePropertyPage::MessagePropertyPage(const MessageHolderList& l) :
	DefaultPropertyPage(IDD_MESSAGEPROPERTY),
	listMessage_(l),
	nFlags_(0),
	nMask_(0)
{
	assert(!l.empty());
}

qm::MessagePropertyPage::~MessagePropertyPage()
{
}

unsigned int qm::MessagePropertyPage::getFlags() const
{
	return nFlags_;
}

unsigned int qm::MessagePropertyPage::getMask() const
{
	return nMask_;
}

LRESULT qm::MessagePropertyPage::onInitDialog(HWND hwndFocus,
											  LPARAM lParam)
{
	if (listMessage_.size() == 1) {
		MessageHolder* pmh = listMessage_.front();
		
		int n = 0;
		struct
		{
			wstring_ptr (MessageHolder::*pfn_)() const;
			UINT nId_;
		} texts[] = {
			{ &MessageHolder::getFrom,		IDC_FROM	},
			{ &MessageHolder::getTo,		IDC_TO		},
			{ &MessageHolder::getSubject,	IDC_SUBJECT	}
		};
		for (n = 0; n < countof(texts); ++n) {
			wstring_ptr wstr((pmh->*texts[n].pfn_)());
			setDlgItemText(texts[n].nId_, wstr.get());
		}
		
		struct
		{
			unsigned int (MessageHolder::*pfn_)() const;
			UINT nId_;
		} numbers[] = {
			{ &MessageHolder::getId,		IDC_ID			},
			{ &MessageHolder::getSize,		IDC_MESSAGESIZE	}
		};
		for (n = 0; n < countof(numbers); ++n)
			setDlgItemInt(numbers[n].nId_, (pmh->*numbers[n].pfn_)());
		
		wstring_ptr wstrFolder(pmh->getFolder()->getFullName());
		setDlgItemText(IDC_FOLDER, wstrFolder.get());
		
		Time time;
		pmh->getDate(&time);
		wstring_ptr wstrTime(time.format(L"%Y4/%M0/%D %h:%m:%s", Time::FORMAT_LOCAL));
		setDlgItemText(IDC_DATE, wstrTime.get());
		
		unsigned int nFlags = pmh->getFlags();
		for (n = 0; n < countof(flags); ++n) {
			sendDlgItemMessage(flags[n].nId_, BM_SETCHECK,
				nFlags & flags[n].flag_ ? BST_CHECKED : BST_UNCHECKED);
			Window(getDlgItem(flags[n].nId_)).setStyle(
				BS_AUTOCHECKBOX, BS_AUTOCHECKBOX | BS_AUTO3STATE);
		}
	}
	else {
		for (int n = 0; n < countof(flags); ++n) {
			unsigned int nCount = 0;
			for (MessageHolderList::const_iterator it = listMessage_.begin(); it != listMessage_.end(); ++it) {
				if ((*it)->getFlags() & flags[n].flag_)
					++nCount;
			}
			
			sendDlgItemMessage(flags[n].nId_, BM_SETCHECK,
				nCount == 0 ? BST_UNCHECKED :
				nCount == listMessage_.size() ? BST_CHECKED : BST_INDETERMINATE);
			if (nCount == 0 || nCount == listMessage_.size())
				Window(getDlgItem(flags[n].nId_)).setStyle(
					BS_AUTOCHECKBOX, BS_AUTOCHECKBOX | BS_AUTO3STATE);
		}
	}
	
	return TRUE;
}

LRESULT qm::MessagePropertyPage::onOk()
{
	for (int n = 0; n < countof(flags); ++n) {
		int nCheck = sendDlgItemMessage(flags[n].nId_, BM_GETCHECK);
		switch (nCheck) {
		case BST_CHECKED:
			nFlags_ |= flags[n].flag_;
			nMask_ |= flags[n].flag_;
			break;
		case BST_UNCHECKED:
			nMask_ |= flags[n].flag_;
			break;
		case BST_INDETERMINATE:
			break;
		default:
			assert(false);
			break;
		}
	}
	
	return DefaultPropertyPage::onOk();
}
