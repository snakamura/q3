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

using namespace qmsmtp;
using namespace qm;
using namespace qs;


/****************************************************************************
 *
 * SendPage
 *
 */

qmsmtp::SendPage::SendPage(SubAccount* pSubAccount, QSTATUS* pstatus) :
	DefaultPropertyPage(getResourceHandle(), IDD_SEND, pstatus),
	pSubAccount_(pSubAccount)
{
}

qmsmtp::SendPage::~SendPage()
{
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
	CHECK_QSTATUS_VALUE(TRUE);
	
	setDlgItemInt(IDC_PORT, pSubAccount_->getPort(Account::HOST_SEND));
	sendDlgItemMessage(IDC_SSL, BM_SETCHECK,
		pSubAccount_->isSsl(Account::HOST_SEND) ? BST_CHECKED : BST_UNCHECKED);
	setDlgItemText(IDC_LOCALHOST, wstrLocalHost.get());
	sendDlgItemMessage(IDC_STARTTLS, BM_SETCHECK,
		nStartTls ? BST_CHECKED : BST_UNCHECKED);
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

LRESULT qmsmtp::SendPage::onOk()
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
	
	return DefaultPropertyPage::onOk();
}
