/*
 * $Id$
 *
 * Copyright(C) 1998-2003 Satoshi Nakamura
 * All rights reserved.
 *
 */

#include <qmsecurity.h>

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
	DefaultPropertyPage(getResourceHandle(), IDD_RECEIVE, pstatus),
	pSubAccount_(pSubAccount)
{
}

qmpop3::ReceivePage::~ReceivePage()
{
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
	int nHandleStatus = 0;
	status = pSubAccount_->getProperty(
		L"Pop3", L"HandleStatus", 0, &nHandleStatus);
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
	sendDlgItemMessage(IDC_HANDLESTATUS, BM_SETCHECK,
		nHandleStatus ? BST_CHECKED : BST_UNCHECKED);
	sendDlgItemMessage(IDC_APOP, BM_SETCHECK, nApop ? BST_CHECKED : BST_UNCHECKED);
	sendDlgItemMessage(IDC_STARTTLS, BM_SETCHECK,
		nStartTls ? BST_CHECKED : BST_UNCHECKED);
	sendDlgItemMessage(IDC_LOG, BM_SETCHECK,
		pSubAccount_->isLog(Account::HOST_RECEIVE) ? BST_CHECKED : BST_UNCHECKED);
	
	if (!Security::isEnabled()) {
		UINT nIds[] = {
			IDC_SSL,
			IDC_STARTTLS
		};
		for (int n = 0; n < countof(nIds); ++n)
			Window(getDlgItem(nIds[n])).enableWindow(false);
	}
	
	return TRUE;
}

LRESULT qmpop3::ReceivePage::onOk()
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
	pSubAccount_->setProperty(L"Pop3", L"HandleStatus",
		sendDlgItemMessage(IDC_HANDLESTATUS, BM_GETCHECK) == BST_CHECKED ? 1 : 0);
	pSubAccount_->setProperty(L"Pop3", L"Apop",
		sendDlgItemMessage(IDC_APOP, BM_GETCHECK) == BST_CHECKED ? 1 : 0);
	pSubAccount_->setProperty(L"Pop3", L"STARTTLS",
		sendDlgItemMessage(IDC_STARTTLS, BM_GETCHECK) == BST_CHECKED ? 1 : 0);
	pSubAccount_->setLog(Account::HOST_RECEIVE,
		sendDlgItemMessage(IDC_LOG, BM_GETCHECK) == BST_CHECKED);
	
	return DefaultPropertyPage::onOk();
}


/****************************************************************************
 *
 * SendPage
 *
 */

qmpop3::SendPage::SendPage(SubAccount* pSubAccount, QSTATUS* pstatus) :
	DefaultPropertyPage(getResourceHandle(), IDD_SEND, pstatus),
	pSubAccount_(pSubAccount)
{
}

qmpop3::SendPage::~SendPage()
{
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
	
	if (!Security::isEnabled()) {
		UINT nIds[] = {
			IDC_SSL,
			IDC_STARTTLS
		};
		for (int n = 0; n < countof(nIds); ++n)
			Window(getDlgItem(nIds[n])).enableWindow(false);
	}
	
	return TRUE;
}

LRESULT qmpop3::SendPage::onOk()
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
	
	return DefaultPropertyPage::onOk();
}
