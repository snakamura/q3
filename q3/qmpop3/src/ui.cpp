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

using namespace qmpop3;
using namespace qm;
using namespace qs;


/****************************************************************************
 *
 * ReceivePage
 *
 */

qmpop3::ReceivePage::ReceivePage(SubAccount* pSubAccount, QSTATUS* pstatus) :
	PropertyPage(getResourceHandle(), IDD_RECEIVE, false, pstatus),
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

qmpop3::ReceivePage::~ReceivePage()
{
}

INT_PTR qmpop3::ReceivePage::dialogProc(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	BEGIN_DIALOG_HANDLER()
		HANDLE_DESTROY()
		HANDLE_INITDIALOG()
	END_DIALOG_HANDLER()
	return DefaultDialogHandler::dialogProc(uMsg, wParam, lParam);
}

LRESULT qmpop3::ReceivePage::onCommand(WORD nCode, WORD nId)
{
	return 1;
}

LRESULT qmpop3::ReceivePage::onNotify(NMHDR* pnmhdr, bool* pbHandled)
{
	BEGIN_NOTIFY_HANDLER()
		HANDLE_NOTIFY_CODE(PSN_APPLY, onApply)
	END_NOTIFY_HANDLER()
	return 1;
}

LRESULT qmpop3::ReceivePage::onDestroy()
{
	removeCommandHandler(this);
	removeNotifyHandler(this);
	return 0;
}

LRESULT qmpop3::ReceivePage::onInitDialog(HWND hwndFocus, LPARAM lParam)
{
	DECLARE_QSTATUS();
	
	int nGetAll = 0;
	status = pSubAccount_->getProperty(L"Pop3", L"GetAll", 20, &nGetAll);
	CHECK_QSTATUS_VALUE(TRUE);
	int nDeleteOnServer = 0;
	status = pSubAccount_->getProperty(
		L"Pop3", L"DeleteOnServer", 0, &nDeleteOnServer);
	CHECK_QSTATUS_VALUE(TRUE);
	int nApop = 0;
	status = pSubAccount_->getProperty(L"Pop3", L"Apop", 0, &nApop);
	CHECK_QSTATUS_VALUE(TRUE);
	int nStartTls = 0;
	status = pSubAccount_->getProperty(L"Pop3", L"STARTTLS", 0, &nStartTls);
	CHECK_QSTATUS_VALUE(TRUE);
	unsigned int nNoopInterval = 0;
	status = pSubAccount_->getProperty(L"Pop3", L"NoopInterval",
		100, reinterpret_cast<int*>(&nNoopInterval));
	CHECK_QSTATUS_VALUE(TRUE);
	int nDeleteBefore = 0;
	status = pSubAccount_->getProperty(L"Pop3", L"DeleteBefore", 0, &nDeleteBefore);
	CHECK_QSTATUS_VALUE(TRUE);
	
	setDlgItemInt(IDC_PORT, pSubAccount_->getPort(Account::HOST_RECEIVE));
	sendDlgItemMessage(IDC_SSL, BM_SETCHECK,
		pSubAccount_->isSsl(Account::HOST_RECEIVE) ? BST_CHECKED : BST_UNCHECKED);
	setDlgItemInt(IDC_GETALL, nGetAll);
	setDlgItemInt(IDC_NOOPINTERVAL, nNoopInterval);
	sendDlgItemMessage(IDC_DELETEONSERVER, BM_SETCHECK,
		nDeleteOnServer ? BST_CHECKED : BST_UNCHECKED);
	setDlgItemInt(IDC_DELETEBEFORE, nDeleteBefore);
	sendDlgItemMessage(IDC_APOP, BM_SETCHECK, nApop ? BST_CHECKED : BST_UNCHECKED);
	sendDlgItemMessage(IDC_STARTTLS, BM_SETCHECK, nStartTls ? BST_CHECKED : BST_UNCHECKED);
	sendDlgItemMessage(IDC_LOG, BM_SETCHECK,
		pSubAccount_->isLog(Account::HOST_RECEIVE) ? BST_CHECKED : BST_UNCHECKED);
	
	return TRUE;
}

LRESULT qmpop3::ReceivePage::onApply(NMHDR* pnmhdr, bool* pbHandled)
{
	pSubAccount_->setPort(Account::HOST_RECEIVE, getDlgItemInt(IDC_PORT));
	pSubAccount_->setSsl(Account::HOST_RECEIVE,
		sendDlgItemMessage(IDC_SSL, BM_GETCHECK) == BST_CHECKED);
	pSubAccount_->setProperty(L"Pop3", L"GetAll", getDlgItemInt(IDC_GETALL));
	pSubAccount_->setProperty(L"Pop3", L"NoopInterval",
		getDlgItemInt(IDC_NOOPINTERVAL));
	pSubAccount_->setProperty(L"Pop3", L"DeleteOnServer",
		sendDlgItemMessage(IDC_DELETEONSERVER, BM_GETCHECK) == BST_CHECKED ? 1 : 0);
	pSubAccount_->setProperty(L"Pop3", L"DeleteBefore", getDlgItemInt(IDC_DELETEBEFORE));
	pSubAccount_->setProperty(L"Pop3", L"Apop",
		sendDlgItemMessage(IDC_APOP, BM_GETCHECK) == BST_CHECKED ? 1 : 0);
	pSubAccount_->setProperty(L"Pop3", L"STARTTLS",
		sendDlgItemMessage(IDC_STARTTLS, BM_GETCHECK) == BST_CHECKED ? 1 : 0);
	pSubAccount_->setLog(Account::HOST_RECEIVE,
		sendDlgItemMessage(IDC_LOG, BM_GETCHECK) == BST_CHECKED);
	
	return 0;
}


/****************************************************************************
 *
 * SendPage
 *
 */

qmpop3::SendPage::SendPage(SubAccount* pSubAccount, QSTATUS* pstatus) :
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

qmpop3::SendPage::~SendPage()
{
}

INT_PTR qmpop3::SendPage::dialogProc(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	BEGIN_DIALOG_HANDLER()
		HANDLE_DESTROY()
		HANDLE_INITDIALOG()
	END_DIALOG_HANDLER()
	return DefaultDialogHandler::dialogProc(uMsg, wParam, lParam);
}

LRESULT qmpop3::SendPage::onCommand(WORD nCode, WORD nId)
{
	return 1;
}

LRESULT qmpop3::SendPage::onNotify(NMHDR* pnmhdr, bool* pbHandled)
{
	BEGIN_NOTIFY_HANDLER()
		HANDLE_NOTIFY_CODE(PSN_APPLY, onApply)
	END_NOTIFY_HANDLER()
	return 1;
}

LRESULT qmpop3::SendPage::onDestroy()
{
	removeCommandHandler(this);
	removeNotifyHandler(this);
	return 0;
}

LRESULT qmpop3::SendPage::onInitDialog(HWND hwndFocus, LPARAM lParam)
{
	DECLARE_QSTATUS();
	
	int nApop = 0;
	status = pSubAccount_->getProperty(L"Pop3Send", L"Apop", 0, &nApop);
	CHECK_QSTATUS_VALUE(TRUE);
	int nStartTls = 0;
	status = pSubAccount_->getProperty(L"Pop3Send", L"STARTTLS", 0, &nStartTls);
	CHECK_QSTATUS_VALUE(TRUE);
	
	setDlgItemInt(IDC_PORT, pSubAccount_->getPort(Account::HOST_SEND));
	sendDlgItemMessage(IDC_SSL, BM_SETCHECK,
		pSubAccount_->isSsl(Account::HOST_SEND) ? BST_CHECKED : BST_UNCHECKED);
	sendDlgItemMessage(IDC_APOP, BM_SETCHECK, nApop ? BST_CHECKED : BST_UNCHECKED);
	sendDlgItemMessage(IDC_STARTTLS, BM_SETCHECK, nStartTls ? BST_CHECKED : BST_UNCHECKED);
	sendDlgItemMessage(IDC_LOG, BM_SETCHECK,
		pSubAccount_->isLog(Account::HOST_SEND) ? BST_CHECKED : BST_UNCHECKED);
	
	return TRUE;
}

LRESULT qmpop3::SendPage::onApply(NMHDR* pnmhdr, bool* pbHandled)
{
	pSubAccount_->setPort(Account::HOST_SEND, getDlgItemInt(IDC_PORT));
	pSubAccount_->setSsl(Account::HOST_SEND,
		sendDlgItemMessage(IDC_SSL, BM_GETCHECK) == BST_CHECKED);
	pSubAccount_->setProperty(L"Pop3Send", L"Apop",
		sendDlgItemMessage(IDC_APOP, BM_GETCHECK) == BST_CHECKED ? 1 : 0);
	pSubAccount_->setProperty(L"Pop3Send", L"STARTTLS",
		sendDlgItemMessage(IDC_STARTTLS, BM_GETCHECK) == BST_CHECKED ? 1 : 0);
	pSubAccount_->setLog(Account::HOST_SEND,
		sendDlgItemMessage(IDC_LOG, BM_GETCHECK) == BST_CHECKED);
	
	return 0;
}
