/*
 * $Id$
 *
 * Copyright(C) 1998-2005 Satoshi Nakamura
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
