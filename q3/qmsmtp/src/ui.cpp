/*
 * $Id$
 *
 * Copyright(C) 1998-2003 Satoshi Nakamura
 * All rights reserved.
 *
 */

#include "main.h"
#include "resourceinc.h"
#include "ui.h"

using namespace qmsmtp;
using namespace qm;
using namespace qs;


/****************************************************************************
 *
 * SendPage
 *
 */

qmsmtp::SendPage::SendPage(SubAccount* pSubAccount, QSTATUS* pstatus) :
	PropertyPage(getResourceHandle(), IDD_SEND, false, pstatus),
	DefaultDialogHandler(pstatus),
	DefaultCommandHandler(pstatus),
	pSubAccount_(pSubAccount)
{
	DECLARE_QSTATUS();
	
	status = addCommandHandler(this);
	CHECK_QSTATUS_SET(pstatus);
	status = addNotifyHandler(this);
	CHECK_QSTATUS_SET(pstatus);
	
	setDialogHandler(this, false);
}

qmsmtp::SendPage::~SendPage()
{
}

INT_PTR qmsmtp::SendPage::dialogProc(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	BEGIN_DIALOG_HANDLER()
		HANDLE_DESTROY()
		HANDLE_INITDIALOG()
	END_DIALOG_HANDLER()
	return DefaultDialogHandler::dialogProc(uMsg, wParam, lParam);
}

LRESULT qmsmtp::SendPage::onCommand(WORD nCode, WORD nId)
{
	return 1;
}

LRESULT qmsmtp::SendPage::onNotify(NMHDR* pnmhdr, bool* pbHandled)
{
	BEGIN_NOTIFY_HANDLER()
		HANDLE_NOTIFY_CODE(PSN_APPLY, onApply)
	END_NOTIFY_HANDLER()
	return 1;
}

LRESULT qmsmtp::SendPage::onDestroy()
{
	removeCommandHandler(this);
	removeNotifyHandler(this);
	return 0;
}

LRESULT qmsmtp::SendPage::onInitDialog(HWND hwndFocus, LPARAM lParam)
{
	DECLARE_QSTATUS();
	
	string_ptr<WSTRING> wstrLocalHost;
	status = pSubAccount_->getProperty(L"Smtp",
		L"LocalHost", L"", &wstrLocalHost);
	CHECK_QSTATUS_VALUE(TRUE);
	int nStartTls = 0;
	status = pSubAccount_->getProperty(L"Smtp", L"STARTTLS", 0, &nStartTls);
	CHECK_QSTATUS();
	
	setDlgItemInt(IDC_PORT, pSubAccount_->getPort(Account::HOST_SEND));
	sendDlgItemMessage(IDC_SSL, BM_SETCHECK,
		pSubAccount_->isSsl(Account::HOST_SEND) ? BST_CHECKED : BST_UNCHECKED);
	setDlgItemText(IDC_LOCALHOST, wstrLocalHost.get());
	sendDlgItemMessage(IDC_STARTTLS, BM_SETCHECK,
		nStartTls ? BST_CHECKED : BST_UNCHECKED);
	sendDlgItemMessage(IDC_LOG, BM_SETCHECK,
		pSubAccount_->isLog(Account::HOST_SEND) ? BST_CHECKED : BST_UNCHECKED);
	
	return TRUE;
}

LRESULT qmsmtp::SendPage::onApply(NMHDR* pnmhdr, bool* pbHandled)
{
	pSubAccount_->setPort(Account::HOST_SEND, getDlgItemInt(IDC_PORT));
	pSubAccount_->setSsl(Account::HOST_SEND,
		sendDlgItemMessage(IDC_SSL, BM_GETCHECK) == BST_CHECKED);
	string_ptr<WSTRING> wstrLocalHost(getDlgItemText(IDC_LOCALHOST));
	if (wstrLocalHost.get())
		pSubAccount_->setProperty(L"Smtp", L"LocalHost", wstrLocalHost.get());
	pSubAccount_->setProperty(L"Smtp", L"STARTTLS",
		sendDlgItemMessage(IDC_STARTTLS, BM_GETCHECK) == BST_CHECKED);
	pSubAccount_->setLog(Account::HOST_SEND,
		sendDlgItemMessage(IDC_LOG, BM_GETCHECK) == BST_CHECKED);
	
	return 0;
}
