/*
 * $Id$
 *
 * Copyright(C) 1998-2004 Satoshi Nakamura
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

qmimap4::ReceivePage::ReceivePage(SubAccount* pSubAccount) :
	DefaultPropertyPage(getResourceHandle(), IDD_RECEIVE),
	pSubAccount_(pSubAccount)
{
}

qmimap4::ReceivePage::~ReceivePage()
{
}

LRESULT qmimap4::ReceivePage::onInitDialog(HWND hwndFocus,
										   LPARAM lParam)
{
	Account* pAccount = pSubAccount_->getAccount();
	wstring_ptr wstrRootFolder(pAccount->getProperty(L"Imap4", L"RootFolder", L""));
	int nFetchCount = pSubAccount_->getProperty(L"Imap4", L"FetchCount", 100);
	int nMaxSession = pSubAccount_->getProperty(L"Imap4", L"MaxSession", 5);
	int nOption = pSubAccount_->getProperty(L"Imap4", L"Option", 0xff);
	int nUseNamespace = pSubAccount_->getProperty(L"Imap4", L"UseNamespace", 0);
	int nCloseFolder = pSubAccount_->getProperty(L"Imap4", L"CloseFolder", 0);
	
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
	
	return TRUE;
}

LRESULT qmimap4::ReceivePage::onOk()
{
	wstring_ptr wstrRootFolder(getDlgItemText(IDC_ROOTFOLDER));
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
	
	return DefaultPropertyPage::onOk();
}
