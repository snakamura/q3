/*
 * $Id$
 *
 * Copyright(C) 1998-2004 Satoshi Nakamura
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

qmpop3::ReceivePage::ReceivePage(SubAccount* pSubAccount) :
	DefaultPropertyPage(getResourceHandle(), IDD_RECEIVE),
	pSubAccount_(pSubAccount)
{
}

qmpop3::ReceivePage::~ReceivePage()
{
}

LRESULT qmpop3::ReceivePage::onInitDialog(HWND hwndFocus,
										  LPARAM lParam)
{
	int nGetAll = pSubAccount_->getProperty(L"Pop3", L"GetAll", 20);
	int nDeleteOnServer = pSubAccount_->getProperty(L"Pop3", L"DeleteOnServer", 0);
	int nHandleStatus = pSubAccount_->getProperty(L"Pop3", L"HandleStatus", 0);
	int nApop = pSubAccount_->getProperty(L"Pop3", L"Apop", 0);
	int nNoopInterval = pSubAccount_->getProperty(L"Pop3", L"NoopInterval", 100);
	int nDeleteBefore = pSubAccount_->getProperty(L"Pop3", L"DeleteBefore", 0);
	
	setDlgItemInt(IDC_GETALL, nGetAll);
	setDlgItemInt(IDC_NOOPINTERVAL, nNoopInterval);
	sendDlgItemMessage(IDC_DELETEONSERVER, BM_SETCHECK,
		nDeleteOnServer ? BST_CHECKED : BST_UNCHECKED);
	setDlgItemInt(IDC_DELETEBEFORE, nDeleteBefore);
	sendDlgItemMessage(IDC_HANDLESTATUS, BM_SETCHECK,
		nHandleStatus ? BST_CHECKED : BST_UNCHECKED);
	sendDlgItemMessage(IDC_APOP, BM_SETCHECK, nApop ? BST_CHECKED : BST_UNCHECKED);
	
	return TRUE;
}

LRESULT qmpop3::ReceivePage::onOk()
{
	pSubAccount_->setProperty(L"Pop3", L"GetAll", getDlgItemInt(IDC_GETALL));
	pSubAccount_->setProperty(L"Pop3", L"NoopInterval",
		getDlgItemInt(IDC_NOOPINTERVAL));
	pSubAccount_->setProperty(L"Pop3", L"DeleteOnServer",
		sendDlgItemMessage(IDC_DELETEONSERVER, BM_GETCHECK) == BST_CHECKED ? 1 : 0);
	pSubAccount_->setProperty(L"Pop3", L"DeleteBefore", getDlgItemInt(IDC_DELETEBEFORE));
	pSubAccount_->setProperty(L"Pop3", L"HandleStatus",
		sendDlgItemMessage(IDC_HANDLESTATUS, BM_GETCHECK) == BST_CHECKED ? 1 : 0);
	pSubAccount_->setProperty(L"Pop3", L"Apop",
		sendDlgItemMessage(IDC_APOP, BM_GETCHECK) == BST_CHECKED ? 1 : 0);
	
	return DefaultPropertyPage::onOk();
}


/****************************************************************************
 *
 * SendPage
 *
 */

qmpop3::SendPage::SendPage(SubAccount* pSubAccount) :
	DefaultPropertyPage(getResourceHandle(), IDD_SEND),
	pSubAccount_(pSubAccount)
{
}

qmpop3::SendPage::~SendPage()
{
}

LRESULT qmpop3::SendPage::onInitDialog(HWND hwndFocus,
									   LPARAM lParam)
{
	int nApop = pSubAccount_->getProperty(L"Pop3Send", L"Apop", 0);
	
	sendDlgItemMessage(IDC_APOP, BM_SETCHECK, nApop ? BST_CHECKED : BST_UNCHECKED);
	
	return TRUE;
}

LRESULT qmpop3::SendPage::onOk()
{
	pSubAccount_->setProperty(L"Pop3Send", L"Apop",
		sendDlgItemMessage(IDC_APOP, BM_GETCHECK) == BST_CHECKED ? 1 : 0);
	
	return DefaultPropertyPage::onOk();
}
