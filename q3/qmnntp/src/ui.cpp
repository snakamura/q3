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

using namespace qmnntp;
using namespace qm;
using namespace qs;


/****************************************************************************
 *
 * ReceivePage
 *
 */

qmnntp::ReceivePage::ReceivePage(SubAccount* pSubAccount, QSTATUS* pstatus) :
	DefaultPropertyPage(getResourceHandle(), IDD_RECEIVE, pstatus),
	pSubAccount_(pSubAccount)
{
}

qmnntp::ReceivePage::~ReceivePage()
{
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
	
	if (!Security::isEnabled()) {
		UINT nIds[] = {
			IDC_SSL
		};
		for (int n = 0; n < countof(nIds); ++n)
			Window(getDlgItem(nIds[n])).enableWindow(false);
	}
	
	return TRUE;
}

LRESULT qmnntp::ReceivePage::onOk()
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
	
	return DefaultPropertyPage::onOk();
}


/****************************************************************************
 *
 * SendPage
 *
 */

qmnntp::SendPage::SendPage(SubAccount* pSubAccount, QSTATUS* pstatus) :
	DefaultPropertyPage(getResourceHandle(), IDD_SEND, pstatus),
	pSubAccount_(pSubAccount)
{
}

qmnntp::SendPage::~SendPage()
{
}

LRESULT qmnntp::SendPage::onInitDialog(HWND hwndFocus, LPARAM lParam)
{
	DECLARE_QSTATUS();
	
	setDlgItemInt(IDC_PORT, pSubAccount_->getPort(Account::HOST_SEND));
	sendDlgItemMessage(IDC_SSL, BM_SETCHECK,
		pSubAccount_->isSsl(Account::HOST_SEND) ? BST_CHECKED : BST_UNCHECKED);
	sendDlgItemMessage(IDC_LOG, BM_SETCHECK,
		pSubAccount_->isLog(Account::HOST_SEND) ? BST_CHECKED : BST_UNCHECKED);
	
	if (!Security::isEnabled()) {
		UINT nIds[] = {
			IDC_SSL
		};
		for (int n = 0; n < countof(nIds); ++n)
			Window(getDlgItem(nIds[n])).enableWindow(false);
	}
	
	return TRUE;
}

LRESULT qmnntp::SendPage::onOk()
{
	pSubAccount_->setPort(Account::HOST_SEND, getDlgItemInt(IDC_PORT));
	pSubAccount_->setSsl(Account::HOST_SEND,
		sendDlgItemMessage(IDC_SSL, BM_GETCHECK) == BST_CHECKED);
	pSubAccount_->setLog(Account::HOST_SEND,
		sendDlgItemMessage(IDC_LOG, BM_GETCHECK) == BST_CHECKED);
	
	return DefaultPropertyPage::onOk();
}
