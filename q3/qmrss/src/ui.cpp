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


using namespace qmrss;
using namespace qm;
using namespace qs;


/****************************************************************************
 *
 * ReceivePage
 *
 */

qmrss::ReceivePage::ReceivePage(SubAccount* pSubAccount) :
	DefaultPropertyPage(getResourceHandle(), IDD_RECEIVE),
	pSubAccount_(pSubAccount)
{
}

qmrss::ReceivePage::~ReceivePage()
{
}

LRESULT qmrss::ReceivePage::onInitDialog(HWND hwndFocus,
										 LPARAM lParam)
{
	wstring_ptr wstrHost(pSubAccount_->getProperty(L"Http", L"ProxyHost", L""));
	setDlgItemText(IDC_HOST, wstrHost.get());
	setDlgItemInt(IDC_PORT, pSubAccount_->getProperty(L"Http", L"ProxyPort", 8080));
	sendDlgItemMessage(IDC_LOG, BM_SETCHECK,
		pSubAccount_->isLog(Account::HOST_RECEIVE) ? BST_CHECKED : BST_UNCHECKED);
	
	return TRUE;
}

LRESULT qmrss::ReceivePage::onOk()
{
	wstring_ptr wstrHost(getDlgItemText(IDC_HOST));
	if (wstrHost.get())
		pSubAccount_->setProperty(L"Http", L"ProxyHost", wstrHost.get());
	pSubAccount_->setProperty(L"Http", L"ProxyPort", getDlgItemInt(IDC_PORT));
	pSubAccount_->setLog(Account::HOST_RECEIVE,
		sendDlgItemMessage(IDC_LOG, BM_GETCHECK) == BST_CHECKED);
	
	return DefaultPropertyPage::onOk();
}


/****************************************************************************
 *
 * SendPage
 *
 */

qmrss::SendPage::SendPage(SubAccount* pSubAccount) :
	DefaultPropertyPage(getResourceHandle(), IDD_SEND),
	pSubAccount_(pSubAccount)
{
}

qmrss::SendPage::~SendPage()
{
}

LRESULT qmrss::SendPage::onInitDialog(HWND hwndFocus,
									  LPARAM lParam)
{
	return TRUE;
}

LRESULT qmrss::SendPage::onOk()
{
	return DefaultPropertyPage::onOk();
}
