/*
 * $Id$
 *
 * Copyright(C) 1998-2005 Satoshi Nakamura
 * All rights reserved.
 *
 */

#include <qmdocument.h>

#include <tchar.h>

#include "groups.h"
#include "main.h"
#include "resourceinc.h"
#include "ui.h"
#include "util.h"

using namespace qmnntp;
using namespace qm;
using namespace qs;


/****************************************************************************
 *
 * ReceivePage
 *
 */

qmnntp::ReceivePage::ReceivePage(SubAccount* pSubAccount) :
	DefaultPropertyPage(getResourceHandle(), IDD_RECEIVE),
	pSubAccount_(pSubAccount)
{
}

qmnntp::ReceivePage::~ReceivePage()
{
}

LRESULT qmnntp::ReceivePage::onInitDialog(HWND hwndFocus,
										  LPARAM lParam)
{
	int nInitialFetchCount = pSubAccount_->getProperty(L"Nntp", L"InitialFetchCount", 300);
	bool bUseXOver = pSubAccount_->getProperty(L"Nntp", L"UseXOVER", 1) != 0;
	int nXOverStep = pSubAccount_->getProperty(L"Nntp", L"XOVERStep", 100);
	
	setDlgItemInt(IDC_INITIALFETCHCOUNT, nInitialFetchCount);
	sendDlgItemMessage(IDC_XOVER, BM_SETCHECK, bUseXOver ? BST_CHECKED : BST_UNCHECKED);
	setDlgItemInt(IDC_FETCHCOUNT, nXOverStep);
	
	return TRUE;
}

LRESULT qmnntp::ReceivePage::onOk()
{
	pSubAccount_->setProperty(L"Nntp", L"InitialFetchCount",
		getDlgItemInt(IDC_INITIALFETCHCOUNT));
	pSubAccount_->setProperty(L"Nntp", L"UseXOVER",
		sendDlgItemMessage(IDC_XOVER, BM_GETCHECK) == BST_CHECKED ? 1 : 0);
	pSubAccount_->setProperty(L"Nntp", L"XOVERStep",
		getDlgItemInt(IDC_FETCHCOUNT));
	
	return DefaultPropertyPage::onOk();
}


/****************************************************************************
 *
 * SendPage
 *
 */

qmnntp::SendPage::SendPage(SubAccount* pSubAccount) :
	DefaultPropertyPage(getResourceHandle(), IDD_SEND),
	pSubAccount_(pSubAccount)
{
}

qmnntp::SendPage::~SendPage()
{
}

LRESULT qmnntp::SendPage::onInitDialog(HWND hwndFocus,
									   LPARAM lParam)
{
	return TRUE;
}

LRESULT qmnntp::SendPage::onOk()
{
	return DefaultPropertyPage::onOk();
}


/****************************************************************************
 *
 * SubscribeDialog
 *
 */

qmnntp::SubscribeDialog::SubscribeDialog(Document* pDocument,
										 Account* pAccount,
										 PasswordCallback* pPasswordCallback) :
	DefaultDialog(getResourceHandle(), IDD_SUBSCRIBE),
	pDocument_(pDocument),
	pAccount_(pAccount),
	pPasswordCallback_(pPasswordCallback),
	nTimerId_(-1)
{
	wstring_ptr wstrPath(concat(pAccount->getPath(), L"\\groups.xml"));
	pGroups_.reset(new Groups(wstrPath.get()));
}

qmnntp::SubscribeDialog::~SubscribeDialog()
{
}

const WCHAR* qmnntp::SubscribeDialog::getGroup() const
{
	return wstrGroup_.get();
}

LRESULT qmnntp::SubscribeDialog::onDestroy()
{
	if (nTimerId_ != -1)
		killTimer(nTimerId_);
	
	removeNotifyHandler(this);
	
	pGroups_->save();
	
	return DefaultDialog::onDestroy();
}

LRESULT qmnntp::SubscribeDialog::onInitDialog(HWND hwndFocus,
											  LPARAM lParam)
{
	HWND hwndGroup = getDlgItem(IDC_GROUP);
	wstring_ptr wstrName(loadString(getResourceHandle(), IDS_GROUP));
	W2T(wstrName.get(), ptszName);
	RECT rect;
	Window(hwndGroup).getWindowRect(&rect);
	LVCOLUMN column = {
		LVCF_FMT | LVCF_WIDTH | LVCF_TEXT,
		LVCFMT_LEFT,
		rect.right - rect.left - ::GetSystemMetrics(SM_CXVSCROLL) - 4,
		const_cast<LPTSTR>(ptszName)
	};
	ListView_InsertColumn(hwndGroup, 0, &column);
	
	addNotifyHandler(this);
	
	if (pGroups_->getGroupList().empty()) {
		int nMsg = messageBox(getResourceHandle(), IDS_REFRESH,
			MB_YESNO | MB_ICONQUESTION, getHandle());
		if (nMsg == IDYES) {
			if (!refreshGroup())
				messageBox(getResourceHandle(), IDS_ERROR_REFRESH,
					MB_OK | MB_ICONERROR, getHandle());
		}
	}
	if (listGroup_.empty())
		refresh();
	
	init(false);
	updateState();
	
	return TRUE;
}

LRESULT qmnntp::SubscribeDialog::onOk()
{
	HWND hwndGroup = getDlgItem(IDC_GROUP);
	int nItem = ListView_GetNextItem(hwndGroup, -1, LVNI_SELECTED);
	if (nItem == -1)
		return 0;
	
	wstrGroup_ = allocWString(listGroup_[nItem]->getName());
	
	return DefaultDialog::onOk();
}

INT_PTR qmnntp::SubscribeDialog::dialogProc(UINT uMsg,
											WPARAM wParam,
											LPARAM lParam)
{
	BEGIN_MESSAGE_HANDLER()
		HANDLE_TIMER()
	END_MESSAGE_HANDLER()
	return DefaultDialog::dialogProc(uMsg, wParam, lParam);
}

LRESULT qmnntp::SubscribeDialog::onTimer(UINT_PTR nId)
{
	if (nId == nTimerId_) {
		killTimer(nTimerId_);
		nTimerId_ = -1;
		
		refresh();
	}
	
	return DefaultDialog::onTimer(nId);
}

LRESULT qmnntp::SubscribeDialog::onCommand(WORD nCode,
										   WORD nId)
{
	BEGIN_COMMAND_HANDLER()
		HANDLE_COMMAND_ID_CODE(IDC_FILTER, EN_CHANGE, onFilterChange)
		HANDLE_COMMAND_ID(IDC_REFRESH, onRefresh)
	END_COMMAND_HANDLER()
	return DefaultDialog::onCommand(nCode, nId);
}

LRESULT qmnntp::SubscribeDialog::onFilterChange()
{
	nTimerId_ = setTimer(TIMERID, TIMEOUT);
	return 0;
}

LRESULT qmnntp::SubscribeDialog::onRefresh()
{
	if (!refreshGroup()) {
		messageBox(getResourceHandle(), IDS_ERROR_REFRESH,
			MB_OK | MB_ICONERROR, getHandle());
		refresh();
	}
	return 0;
}

LRESULT qmnntp::SubscribeDialog::onNotify(NMHDR* pnmhdr,
										  bool* pbHandled)
{
	BEGIN_NOTIFY_HANDLER()
		HANDLE_NOTIFY(LVN_GETDISPINFO, IDC_GROUP, onGetDispInfo)
		HANDLE_NOTIFY(LVN_ITEMCHANGED, IDC_GROUP, onItemChanged)
	END_NOTIFY_HANDLER()
	return NotifyHandler::onNotify(pnmhdr, pbHandled);
}

LRESULT qmnntp::SubscribeDialog::onGetDispInfo(NMHDR* pnmhdr,
											   bool* pbHandled)
{
	NMLVDISPINFO* pDispInfo = reinterpret_cast<NMLVDISPINFO*>(pnmhdr);
	if (pDispInfo->item.mask & LVIF_TEXT) {
		const Group* pGroup = listGroup_[pDispInfo->item.iItem];
		W2T(pGroup->getName(), ptszGroup);
		_tcsncpy(pDispInfo->item.pszText, ptszGroup, pDispInfo->item.cchTextMax);
	}
	
	*pbHandled = true;
	
	return 0;
}

LRESULT qmnntp::SubscribeDialog::onItemChanged(NMHDR* pnmhdr,
											   bool* pbHandled)
{
	updateState();
	*pbHandled = true;
	return 0;
}

void qmnntp::SubscribeDialog::refresh()
{
	const Groups::GroupList& l = pGroups_->getGroupList();
	
	wstring_ptr wstrFilter(getDlgItemText(IDC_FILTER));
	if (*wstrFilter.get()) {
		listGroup_.clear();
		listGroup_.reserve(l.size());
		for (Groups::GroupList::const_iterator it = l.begin(); it != l.end(); ++it) {
			const Group* pGroup = *it;
			const WCHAR* pwszName = pGroup->getName();
			if (!*wstrFilter.get() || wcsstr(pwszName, wstrFilter.get()))
				listGroup_.push_back(pGroup);
		}
	}
	else {
		listGroup_.resize(l.size());
		std::copy(l.begin(), l.end(), listGroup_.begin());
	}
	
	ListView_SetItemCount(getDlgItem(IDC_GROUP), listGroup_.size());
}

bool qmnntp::SubscribeDialog::refreshGroup()
{
	// TODO
	// Use NEWGROUPS if groups is not empty.
	
	SubAccount* pSubAccount = pAccount_->getCurrentSubAccount();
	
	std::auto_ptr<Logger> pLogger;
	if (pSubAccount->isLog(Account::HOST_RECEIVE))
		pLogger = pAccount_->openLogger(Account::HOST_RECEIVE);
	
	DefaultCallback callback(pSubAccount, pPasswordCallback_, pDocument_->getSecurity());
	Nntp nntp(pSubAccount->getTimeout(), &callback, &callback, &callback, pLogger.get());
	if (!nntp.connect(pSubAccount->getHost(Account::HOST_RECEIVE),
		pSubAccount->getPort(Account::HOST_RECEIVE),
		pSubAccount->getSecure(Account::HOST_RECEIVE) == SubAccount::SECURE_SSL))
		return false;
	
	std::auto_ptr<GroupsData> pGroupsData;
	if (!nntp.list(&pGroupsData))
		return false;
	
	nntp.disconnect();
	
	pGroups_->add(pGroupsData.get());
	refresh();
	
	return true;
}

void qmnntp::SubscribeDialog::updateState()
{
	int nItem = ListView_GetNextItem(getDlgItem(IDC_GROUP), -1, LVNI_SELECTED);
	Window(getDlgItem(IDOK)).enableWindow(nItem != -1);
}
