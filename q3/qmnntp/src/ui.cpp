/*
 * $Id: ui.cpp,v 1.1.1.1 2003/04/29 08:07:34 snakamura Exp $
 *
 * Copyright(C) 1998-2003 Satoshi Nakamura
 * All rights reserved.
 *
 */

#include "main.h"
#include "resourceinc.h"
#include "ui.h"

using namespace qmnntp;
using namespace qm;
using namespace qs;


/****************************************************************************
 *
 * ReceivePage
 *
 */

qmnntp::ReceivePage::ReceivePage(SubAccount* pSubAccount, QSTATUS* pstatus) :
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

qmnntp::ReceivePage::~ReceivePage()
{
}

INT_PTR qmnntp::ReceivePage::dialogProc(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	BEGIN_DIALOG_HANDLER()
		HANDLE_DESTROY()
		HANDLE_INITDIALOG()
	END_DIALOG_HANDLER()
	return DefaultDialogHandler::dialogProc(uMsg, wParam, lParam);
}

LRESULT qmnntp::ReceivePage::onCommand(WORD nCode, WORD nId)
{
	return 1;
}

LRESULT qmnntp::ReceivePage::onNotify(NMHDR* pnmhdr, bool* pbHandled)
{
	BEGIN_NOTIFY_HANDLER()
		HANDLE_NOTIFY_CODE(PSN_APPLY, onApply)
	END_NOTIFY_HANDLER()
	return 1;
}

LRESULT qmnntp::ReceivePage::onDestroy()
{
	removeCommandHandler(this);
	removeNotifyHandler(this);
	return 0;
}

LRESULT qmnntp::ReceivePage::onInitDialog(HWND hwndFocus, LPARAM lParam)
{
	DECLARE_QSTATUS();
	
	int nInitialFetchCount = 0;
	status = pSubAccount_->getProperty(L"Nntp",
		L"InitialFetchCount", 300, &nInitialFetchCount);
	CHECK_QSTATUS_VALUE(TRUE);
	int nUseXOver = 0;
	status = pSubAccount_->getProperty(L"Nntp", L"UseXOVER", 1, &nUseXOver);
	CHECK_QSTATUS_VALUE(TRUE);
	int nXOverStep = 0;
	status = pSubAccount_->getProperty(L"Nntp", L"XOVERStep", 100, &nXOverStep);
	CHECK_QSTATUS_VALUE(TRUE);
	
	setDlgItemInt(IDC_PORT, pSubAccount_->getPort(Account::HOST_RECEIVE));
	sendDlgItemMessage(IDC_SSL, BM_SETCHECK,
		pSubAccount_->isSsl(Account::HOST_RECEIVE) ? BST_CHECKED : BST_UNCHECKED);
	setDlgItemInt(IDC_INITIALFETCHCOUNT, nInitialFetchCount);
	sendDlgItemMessage(IDC_XOVER, BM_SETCHECK,
		nUseXOver ? BST_CHECKED : BST_UNCHECKED);
	setDlgItemInt(IDC_FETCHCOUNT, nXOverStep);
	sendDlgItemMessage(IDC_LOG, BM_SETCHECK,
		pSubAccount_->isLog(Account::HOST_RECEIVE) ? BST_CHECKED : BST_UNCHECKED);
	
	return TRUE;
}

LRESULT qmnntp::ReceivePage::onApply(NMHDR* pnmhdr, bool* pbHandled)
{
	pSubAccount_->setPort(Account::HOST_RECEIVE, getDlgItemInt(IDC_PORT));
	pSubAccount_->setSsl(Account::HOST_RECEIVE,
		sendDlgItemMessage(IDC_SSL, BM_GETCHECK) == BST_CHECKED);
	pSubAccount_->setProperty(L"Nntp", L"InitialFetchCount",
		getDlgItemInt(IDC_INITIALFETCHCOUNT));
	pSubAccount_->setProperty(L"Nntp", L"UseXOVER",
		sendDlgItemMessage(IDC_XOVER, BM_GETCHECK) == BST_CHECKED ? 1 : 0);
	pSubAccount_->setProperty(L"Nntp", L"XOVERStep",
		getDlgItemInt(IDC_FETCHCOUNT));
	pSubAccount_->setLog(Account::HOST_RECEIVE,
		sendDlgItemMessage(IDC_LOG, BM_GETCHECK) == BST_CHECKED);
	
	return 0;
}


/****************************************************************************
 *
 * SendPage
 *
 */

qmnntp::SendPage::SendPage(SubAccount* pSubAccount, QSTATUS* pstatus) :
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

qmnntp::SendPage::~SendPage()
{
}

INT_PTR qmnntp::SendPage::dialogProc(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	BEGIN_DIALOG_HANDLER()
		HANDLE_DESTROY()
		HANDLE_INITDIALOG()
	END_DIALOG_HANDLER()
	return DefaultDialogHandler::dialogProc(uMsg, wParam, lParam);
}

LRESULT qmnntp::SendPage::onCommand(WORD nCode, WORD nId)
{
	return 1;
}

LRESULT qmnntp::SendPage::onNotify(NMHDR* pnmhdr, bool* pbHandled)
{
	BEGIN_NOTIFY_HANDLER()
		HANDLE_NOTIFY_CODE(PSN_APPLY, onApply)
	END_NOTIFY_HANDLER()
	return 1;
}

LRESULT qmnntp::SendPage::onDestroy()
{
	removeCommandHandler(this);
	removeNotifyHandler(this);
	return 0;
}

LRESULT qmnntp::SendPage::onInitDialog(HWND hwndFocus, LPARAM lParam)
{
	DECLARE_QSTATUS();
	
	setDlgItemInt(IDC_PORT, pSubAccount_->getPort(Account::HOST_SEND));
	sendDlgItemMessage(IDC_SSL, BM_SETCHECK,
		pSubAccount_->isSsl(Account::HOST_SEND) ? BST_CHECKED : BST_UNCHECKED);
	sendDlgItemMessage(IDC_LOG, BM_SETCHECK,
		pSubAccount_->isLog(Account::HOST_SEND) ? BST_CHECKED : BST_UNCHECKED);
	
	return TRUE;
}

LRESULT qmnntp::SendPage::onApply(NMHDR* pnmhdr, bool* pbHandled)
{
	pSubAccount_->setPort(Account::HOST_SEND, getDlgItemInt(IDC_PORT));
	pSubAccount_->setSsl(Account::HOST_SEND,
		sendDlgItemMessage(IDC_SSL, BM_GETCHECK) == BST_CHECKED);
	pSubAccount_->setLog(Account::HOST_SEND,
		sendDlgItemMessage(IDC_LOG, BM_GETCHECK) == BST_CHECKED);
	
	return 0;
}
