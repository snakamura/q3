/*
 * $Id$
 *
 * Copyright(C) 1998-2003 Satoshi Nakamura
 * All rights reserved.
 *
 */

#include <qmaccount.h>
#include <qmapplication.h>
#include <qmmessageholder.h>
#include <qmsyncfilter.h>

#include <qsconv.h>
#include <qsras.h>

#include "propertypages.h"
#include "resourceinc.h"

using namespace qm;
using namespace qs;


/****************************************************************************
 *
 * DefaultPropertyPage
 *
 */

qm::DefaultPropertyPage::DefaultPropertyPage(UINT nId, QSTATUS* pstatus) :
	qs::DefaultPropertyPage(Application::getApplication().getResourceHandle(), nId, pstatus)
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
	SyncFilterManager* pSyncFilterManager, QSTATUS* pstatus) :
	DefaultPropertyPage(IDD_ACCOUNTADVANCED, pstatus),
	pSubAccount_(pSubAccount),
	pSyncFilterManager_(pSyncFilterManager)
{
}

qm::AccountAdvancedPage::~AccountAdvancedPage()
{
}

LRESULT qm::AccountAdvancedPage::onInitDialog(HWND hwndFocus, LPARAM lParam)
{
	DECLARE_QSTATUS();
	
	setDlgItemText(IDC_IDENTITY, pSubAccount_->getIdentity());
	string_ptr<WSTRING> wstrMyAddress;
	status = pSubAccount_->getMyAddress(&wstrMyAddress);
	CHECK_QSTATUS_VALUE(TRUE);
	setDlgItemText(IDC_MYADDRESS, wstrMyAddress.get());
	
	SyncFilterManager::FilterSetList l;
	status = pSyncFilterManager_->getFilterSets(
		pSubAccount_->getAccount(), &l);
	CHECK_QSTATUS_VALUE(TRUE);
	SyncFilterManager::FilterSetList::const_iterator it = l.begin();
	while (it != l.end()) {
		SyncFilterSet* pSet = *it;
		
		W2T_STATUS(pSet->getName(), ptszName);
		CHECK_QSTATUS_VALUE(TRUE);
		sendDlgItemMessage(IDC_SYNCFILTER, CB_ADDSTRING,
			0, reinterpret_cast<LPARAM>(ptszName));
		
		++it;
	}
	W2T_STATUS(pSubAccount_->getSyncFilterName(), ptszName);
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
	string_ptr<WSTRING> wstr;
	wstr.reset(getDlgItemText(IDC_IDENTITY));
	if (wstr.get())
		pSubAccount_->setIdentity(wstr.get());
	wstr.reset(getDlgItemText(IDC_MYADDRESS));
	if (wstr.get())
		pSubAccount_->setMyAddress(wstr.get());
	wstr.reset(getDlgItemText(IDC_SYNCFILTER));
	if (wstr.get())
		pSubAccount_->setSyncFilterName(wstr.get());
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

qm::AccountDialupPage::AccountDialupPage(
	SubAccount* pSubAccount, QSTATUS* pstatus) :
	DefaultPropertyPage(IDD_ACCOUNTDIALUP, pstatus),
	pSubAccount_(pSubAccount)
{
}

qm::AccountDialupPage::~AccountDialupPage()
{
}

LRESULT qm::AccountDialupPage::onCommand(WORD nCode, WORD nId)
{
	BEGIN_COMMAND_HANDLER()
		HANDLE_COMMAND_ID(IDC_DIALPROPERTY, onDialProperty)
		HANDLE_COMMAND_ID_RANGE(IDC_NEVER, IDC_CONNECT, onTypeSelect)
	END_COMMAND_HANDLER()
	return DefaultPropertyPage::onCommand(nCode, nId);
}

LRESULT qm::AccountDialupPage::onInitDialog(HWND hwndFocus, LPARAM lParam)
{
	DECLARE_QSTATUS();
	
	for (int n = 0; n < countof(types); ++n) {
		if (pSubAccount_->getDialupType() == types[n].type_) {
			sendDlgItemMessage(types[n].nId_, BM_SETCHECK, BST_CHECKED);
			break;
		}
	}
	
	RasConnection::EntryList listEntry;
	StringListFree<RasConnection::EntryList> free(listEntry);
	status = RasConnection::getEntries(&listEntry);
	CHECK_QSTATUS_VALUE(TRUE);
	
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
			W2T_STATUS(*it, ptszName);
			CHECK_QSTATUS_VALUE(TRUE);
			sendDlgItemMessage(IDC_ENTRY, CB_ADDSTRING,
				0, reinterpret_cast<LPARAM>(ptszName));
			++it;
		}
		
		W2T_STATUS(pSubAccount_->getDialupEntry(), ptszEntry);
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
	DECLARE_QSTATUS();
	
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
			string_ptr<TSTRING> tstrEntry(allocTString(nLen + 1));
			if (tstrEntry.get()) {
				sendDlgItemMessage(IDC_ENTRY, CB_GETLBTEXT,
					nIndex, reinterpret_cast<LPARAM>(tstrEntry.get()));
				T2W_STATUS(tstrEntry.get(), ptszEntry);
				if (status == QSTATUS_SUCCESS)
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

qm::AccountGeneralPage::AccountGeneralPage(
	SubAccount* pSubAccount, QSTATUS* pstatus) :
	DefaultPropertyPage(IDD_ACCOUNTGENERAL, pstatus),
	pSubAccount_(pSubAccount)
{
}

qm::AccountGeneralPage::~AccountGeneralPage()
{
}

LRESULT qm::AccountGeneralPage::onInitDialog(HWND hwndFocus, LPARAM lParam)
{
	setDlgItemText(IDC_RECEIVEHOST, pSubAccount_->getHost(Account::HOST_RECEIVE));
	setDlgItemText(IDC_SENDHOST, pSubAccount_->getHost(Account::HOST_SEND));
	setDlgItemText(IDC_NAME, pSubAccount_->getSenderName());
	setDlgItemText(IDC_ADDRESS, pSubAccount_->getSenderAddress());
	
	return TRUE;
}

LRESULT qm::AccountGeneralPage::onOk()
{
	string_ptr<WSTRING> wstr;
	wstr.reset(getDlgItemText(IDC_RECEIVEHOST));
	if (wstr.get())
		pSubAccount_->setHost(Account::HOST_RECEIVE, wstr.get());
	wstr.reset(getDlgItemText(IDC_SENDHOST));
	if (wstr.get())
		pSubAccount_->setHost(Account::HOST_SEND, wstr.get());
	wstr.reset(getDlgItemText(IDC_NAME));
	if (wstr.get())
		pSubAccount_->setSenderName(wstr.get());
	wstr.reset(getDlgItemText(IDC_ADDRESS));
	if (wstr.get())
		pSubAccount_->setSenderAddress(wstr.get());
	
	return DefaultPropertyPage::onOk();
}


/****************************************************************************
 *
 * AccountUserPage
 *
 */

qm::AccountUserPage::AccountUserPage(SubAccount* pSubAccount, QSTATUS* pstatus) :
	DefaultPropertyPage(IDD_ACCOUNTUSER, pstatus),
	pSubAccount_(pSubAccount)
{
}

qm::AccountUserPage::~AccountUserPage()
{
}

LRESULT qm::AccountUserPage::onCommand(WORD nCode, WORD nId)
{
	BEGIN_COMMAND_HANDLER()
		HANDLE_COMMAND_ID(IDC_SENDAUTHENTICATE, onSendAuthenticate)
	END_COMMAND_HANDLER()
	return DefaultPropertyPage::onCommand(nCode, nId);
}

LRESULT qm::AccountUserPage::onInitDialog(HWND hwndFocus, LPARAM lParam)
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
	string_ptr<WSTRING> wstr;
	wstr.reset(getDlgItemText(IDC_RECEIVEUSERNAME));
	if (wstr.get())
		pSubAccount_->setUserName(Account::HOST_RECEIVE, wstr.get());
	wstr.reset(getDlgItemText(IDC_RECEIVEPASSWORD));
	if (wstr.get())
		pSubAccount_->setPassword(Account::HOST_RECEIVE, wstr.get());
	
	if (sendDlgItemMessage(IDC_SENDAUTHENTICATE, BM_GETCHECK) == BST_CHECKED) {
		wstr.reset(getDlgItemText(IDC_SENDUSERNAME));
		if (wstr.get())
			pSubAccount_->setUserName(Account::HOST_SEND, wstr.get());
		wstr.reset(getDlgItemText(IDC_SENDPASSWORD));
		if (wstr.get())
			pSubAccount_->setPassword(Account::HOST_SEND, wstr.get());
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
 * FolderPropertyPage
 *
 */

namespace {
struct
{
	Folder::Flag flag_;
	UINT nId_;
	Folder::Flag enableFlag_;
	bool bReverse_;
} folderFlags[] = {
	{ Folder::FLAG_SYNCWHENOPEN,	IDC_SYNCWHENOPEN,	Folder::FLAG_SYNCABLE,	false	},
	{ Folder::FLAG_CACHEWHENREAD,	IDC_CACHEWHENREAD,	Folder::FLAG_SYNCABLE,	false	},
	{ Folder::FLAG_INBOX,			IDC_INBOX,			Folder::FLAG_NOSELECT,	true	},
	{ Folder::FLAG_OUTBOX,			IDC_OUTBOX,			Folder::FLAG_NOSELECT,	true	},
	{ Folder::FLAG_SENTBOX,			IDC_SENTBOX,		Folder::FLAG_NOSELECT,	true	},
	{ Folder::FLAG_DRAFTBOX,		IDC_DRAFTBOX,		Folder::FLAG_NOSELECT,	true	},
	{ Folder::FLAG_TRASHBOX,		IDC_TRASHBOX,		Folder::FLAG_NOSELECT,	true	}
};
};

qm::FolderPropertyPage::FolderPropertyPage(
	const Account::FolderList& l, QSTATUS* pstatus) :
	DefaultPropertyPage(IDD_FOLDERPROPERTY, pstatus),
	listFolder_(l),
	nFlags_(0),
	nMask_(0)
{
	assert(!l.empty());
}

qm::FolderPropertyPage::~FolderPropertyPage()
{
}

unsigned int qm::FolderPropertyPage::getFlags() const
{
	return nFlags_;
}

unsigned int qm::FolderPropertyPage::getMask() const
{
	return nMask_;
}

LRESULT qm::FolderPropertyPage::onInitDialog(HWND hwndFocus, LPARAM lParam)
{
	DECLARE_QSTATUS();
	
	if (listFolder_.size() == 1) {
		Folder* pFolder = listFolder_.front();
		
		string_ptr<WSTRING> wstrName;
		status = pFolder->getFullName(&wstrName);
		CHECK_QSTATUS_VALUE(TRUE);
		setDlgItemText(IDC_NAME, wstrName.get());
		
		setDlgItemInt(IDC_ID, pFolder->getId());
		
		HINSTANCE hInst = Application::getApplication().getResourceHandle();
		string_ptr<WSTRING> wstrTemplate;
		status = loadString(hInst, IDS_FOLDERTYPETEMPLATE, &wstrTemplate);
		CHECK_QSTATUS_VALUE(TRUE);
		UINT nTypeId = pFolder->getType() == Folder::TYPE_NORMAL ?
			IDS_NORMALFOLDER : IDS_QUERYFOLDER;
		UINT nLocalId = pFolder->isFlag(Folder::FLAG_LOCAL) ?
			IDS_LOCALFOLDER : IDS_REMOTEFOLDER;
		string_ptr<WSTRING> wstrType;
		status = loadString(hInst, nTypeId, &wstrType);
		CHECK_QSTATUS_VALUE(TRUE);
		string_ptr<WSTRING> wstrLocal;
		status = loadString(hInst, nLocalId, &wstrLocal);
		CHECK_QSTATUS_VALUE(TRUE);
		WCHAR wszType[128];
		swprintf(wszType, wstrTemplate.get(), wstrType.get(), wstrLocal.get());
		setDlgItemText(IDC_TYPE, wszType);
		
		unsigned int nFlags = pFolder->getFlags();
		for (int n = 0; n < countof(folderFlags); ++n) {
			sendDlgItemMessage(folderFlags[n].nId_, BM_SETCHECK,
				nFlags & folderFlags[n].flag_ ? BST_CHECKED : BST_UNCHECKED);
			Window(getDlgItem(folderFlags[n].nId_)).setStyle(
				BS_AUTOCHECKBOX, BS_AUTOCHECKBOX | BS_AUTO3STATE);
			
			bool bEnable = pFolder->isFlag(folderFlags[n].enableFlag_);
			bEnable = folderFlags[n].bReverse_ ? !bEnable : bEnable;
			if (!bEnable)
				Window(getDlgItem(folderFlags[n].nId_)).enableWindow(false);
		}
	}
	else {
		for (int n = 0; n < countof(folderFlags); ++n) {
			unsigned int nCount = 0;
			bool bEnable = false;
			Account::FolderList::const_iterator it = listFolder_.begin();
			while (it != listFolder_.end()) {
				Folder* pFolder = *it;
				if (pFolder->getFlags() & folderFlags[n].flag_)
					++nCount;
				
				bool b = pFolder->isFlag(folderFlags[n].enableFlag_);
				b = folderFlags[n].bReverse_ ? !b : b;
				if (b)
					bEnable = true;
				
				++it;
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
	for (int n = 0; n < countof(folderFlags); ++n) {
		int nCheck = sendDlgItemMessage(folderFlags[n].nId_, BM_GETCHECK);
		switch (nCheck) {
		case BST_CHECKED:
			nFlags_ |= folderFlags[n].flag_;
			nMask_ |= folderFlags[n].flag_;
			break;
		case BST_UNCHECKED:
			nMask_ |= folderFlags[n].flag_;
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

qm::MessagePropertyPage::MessagePropertyPage(
	const Folder::MessageHolderList& l, QSTATUS* pstatus) :
	DefaultPropertyPage(IDD_MESSAGEPROPERTY, pstatus),
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

LRESULT qm::MessagePropertyPage::onInitDialog(HWND hwndFocus, LPARAM lParam)
{
	DECLARE_QSTATUS();
	
	if (listMessage_.size() == 1) {
		MessageHolder* pmh = listMessage_.front();
		
		int n = 0;
		struct
		{
			QSTATUS (MessageHolder::*pfn_)(WSTRING* pwstr) const;
			UINT nId_;
		} texts[] = {
			{ &MessageHolder::getFrom,		IDC_FROM	},
			{ &MessageHolder::getTo,		IDC_TO		},
			{ &MessageHolder::getSubject,	IDC_SUBJECT	}
		};
		for (n = 0; n < countof(texts); ++n) {
			string_ptr<WSTRING> wstr;
			status = (pmh->*texts[n].pfn_)(&wstr);
			CHECK_QSTATUS_VALUE(TRUE);
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
		
		string_ptr<WSTRING> wstrFolder;
		status = pmh->getFolder()->getFullName(&wstrFolder);
		CHECK_QSTATUS_VALUE(TRUE);
		setDlgItemText(IDC_FOLDER, wstrFolder.get());
		
		Time time;
		status = pmh->getDate(&time);
		CHECK_QSTATUS_VALUE(TRUE);
		string_ptr<WSTRING> wstrTime;
		status = time.format(L"%Y4/%M0/%D %h:%m:%s", Time::FORMAT_LOCAL, &wstrTime);
		CHECK_QSTATUS_VALUE(TRUE);
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
			Folder::MessageHolderList::const_iterator it = listMessage_.begin();
			while (it != listMessage_.end()) {
				if ((*it)->getFlags() & flags[n].flag_)
					++nCount;
				++it;
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
