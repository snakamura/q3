/*
 * $Id$
 *
 * Copyright(C) 1998-2003 Satoshi Nakamura
 * All rights reserved.
 *
 */

#include "main.h"
#include "option.h"
#include "resourceinc.h"
#include "ui.h"

using namespace qmimap4;
using namespace qm;
using namespace qs;


/****************************************************************************
 *
 * ReceivePage
 *
 */

qmimap4::ReceivePage::ReceivePage(SubAccount* pSubAccount, QSTATUS* pstatus) :
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

qmimap4::ReceivePage::~ReceivePage()
{
}

INT_PTR qmimap4::ReceivePage::dialogProc(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	BEGIN_DIALOG_HANDLER()
		HANDLE_DESTROY()
		HANDLE_INITDIALOG()
	END_DIALOG_HANDLER()
	return DefaultDialogHandler::dialogProc(uMsg, wParam, lParam);
}

LRESULT qmimap4::ReceivePage::onCommand(WORD nCode, WORD nId)
{
	return 1;
}

LRESULT qmimap4::ReceivePage::onNotify(NMHDR* pnmhdr, bool* pbHandled)
{
	BEGIN_NOTIFY_HANDLER()
		HANDLE_NOTIFY_CODE(PSN_APPLY, onApply)
	END_NOTIFY_HANDLER()
	return 1;
}

LRESULT qmimap4::ReceivePage::onDestroy()
{
	removeCommandHandler(this);
	removeNotifyHandler(this);
	return 0;
}

LRESULT qmimap4::ReceivePage::onInitDialog(HWND hwndFocus, LPARAM lParam)
{
	DECLARE_QSTATUS();
	
	string_ptr<WSTRING> wstrRootFolder;
	Account* pAccount = pSubAccount_->getAccount();
	status = pAccount->getProperty(
		L"Imap4", L"RootFolder", 0, &wstrRootFolder);
	CHECK_QSTATUS_VALUE(TRUE);
	int nFetchCount = 0;
	status = pSubAccount_->getProperty(L"Imap4",
		L"FetchCount", 100, &nFetchCount);
	CHECK_QSTATUS_VALUE(TRUE);
	int nMaxSession = 0;
	status = pSubAccount_->getProperty(L"Imap4",
		L"MaxSession", 5, &nMaxSession);
	CHECK_QSTATUS_VALUE(TRUE);
	int nOption = 0;
	status = pSubAccount_->getProperty(L"Imap4", L"Option", 0xff, &nOption);
	CHECK_QSTATUS_VALUE(TRUE);
	int nUseNamespace = 0;
	status = pSubAccount_->getProperty(L"Imap4", L"UseNamespace", 0, &nUseNamespace);
	CHECK_QSTATUS_VALUE(TRUE);
	int nCloseFolder = 0;
	status = pSubAccount_->getProperty(
		L"Imap4", L"CloseFolder", 0, &nCloseFolder);
	CHECK_QSTATUS_VALUE(TRUE);
	int nStartTls = 0;
	status = pSubAccount_->getProperty(L"Imap4", L"STARTTLS", 0, &nStartTls);
	CHECK_QSTATUS_VALUE(TRUE);
	
	setDlgItemInt(IDC_PORT, pSubAccount_->getPort(Account::HOST_RECEIVE));
	sendDlgItemMessage(IDC_SSL, BM_SETCHECK,
		pSubAccount_->isSsl(Account::HOST_RECEIVE) ? BST_CHECKED : BST_UNCHECKED);
	setDlgItemText(IDC_ROOTFOLDER, wstrRootFolder.get());
	setDlgItemInt(IDC_FETCHCOUNT, nFetchCount);
	setDlgItemInt(IDC_MAXSESSION, nMaxSession);
	sendDlgItemMessage(IDC_USEENVELOPE, BM_SETCHECK,
		nOption & OPTION_USEENVELOPE ? BST_CHECKED : BST_UNCHECKED);
	sendDlgItemMessage(IDC_USEBODYSTRUCTUREALWAYS, BM_SETCHECK,
		nOption & OPTION_USEBODYSTRUCTUREALWAYS ? BST_CHECKED : BST_UNCHECKED);
	sendDlgItemMessage(IDC_TRUSTBODYSTRUCTURE, BM_SETCHECK,
		nOption & OPTION_TRUSTBODYSTRUCTURE ? BST_CHECKED : BST_UNCHECKED);
	sendDlgItemMessage(IDC_NAMESPACE, BM_SETCHECK,
		nUseNamespace ? BST_CHECKED : BST_UNCHECKED);
	sendDlgItemMessage(IDC_CLOSEFOLDER, BM_SETCHECK,
		nCloseFolder ? BST_CHECKED : BST_UNCHECKED);
	sendDlgItemMessage(IDC_STARTTLS, BM_SETCHECK,
		nStartTls ? BST_CHECKED : BST_UNCHECKED);
	sendDlgItemMessage(IDC_LOG, BM_SETCHECK,
		pSubAccount_->isLog(Account::HOST_RECEIVE) ? BST_CHECKED : BST_UNCHECKED);
	
	
	return TRUE;
}

LRESULT qmimap4::ReceivePage::onApply(NMHDR* pnmhdr, bool* pbHandled)
{
	pSubAccount_->setPort(Account::HOST_RECEIVE, getDlgItemInt(IDC_PORT));
	pSubAccount_->setSsl(Account::HOST_RECEIVE,
		sendDlgItemMessage(IDC_SSL, BM_GETCHECK) == BST_CHECKED);
	string_ptr<WSTRING> wstrRootFolder(getDlgItemText(IDC_ROOTFOLDER));
	if (wstrRootFolder.get())
		pSubAccount_->setProperty(L"Imap4", L"RootFolder", wstrRootFolder.get());
	pSubAccount_->setProperty(L"Imap4", L"FetchCount",
		getDlgItemInt(IDC_FETCHCOUNT));
	pSubAccount_->setProperty(L"Imap4", L"MaxSession",
		getDlgItemInt(IDC_MAXSESSION));
	int nOption = 0;
	if (sendDlgItemMessage(IDC_USEENVELOPE, BM_GETCHECK) == BST_CHECKED)
		nOption |= OPTION_USEENVELOPE;
	if (sendDlgItemMessage(IDC_USEBODYSTRUCTUREALWAYS, BM_GETCHECK) == BST_CHECKED)
		nOption |= OPTION_USEBODYSTRUCTUREALWAYS;
	if (sendDlgItemMessage(IDC_TRUSTBODYSTRUCTURE, BM_GETCHECK) == BST_CHECKED)
		nOption |= OPTION_TRUSTBODYSTRUCTURE;
	pSubAccount_->setProperty(L"Imap4", L"Option", nOption);
	pSubAccount_->setProperty(L"Imap4", L"UseNamespace",
		sendDlgItemMessage(IDC_NAMESPACE, BM_GETCHECK) == BST_CHECKED ? 1 : 0);
	pSubAccount_->setProperty(L"Imap4", L"CloseFolder",
		sendDlgItemMessage(IDC_CLOSEFOLDER, BM_GETCHECK) == BST_CHECKED ? 1 : 0);
	pSubAccount_->setProperty(L"Imap4", L"STARTTLS",
		sendDlgItemMessage(IDC_STARTTLS, BM_GETCHECK) == BST_CHECKED);
	pSubAccount_->setLog(Account::HOST_RECEIVE,
		sendDlgItemMessage(IDC_LOG, BM_GETCHECK) == BST_CHECKED);
	
	return 0;
}
