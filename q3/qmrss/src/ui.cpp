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

LRESULT qmrss::ReceivePage::onCommand(WORD nCode,
									  WORD nId)
{
	BEGIN_COMMAND_HANDLER()
		HANDLE_COMMAND_ID_RANGE(IDC_NOPROXY, IDC_CUSTOM, onProxy)
	END_COMMAND_HANDLER()
	return DefaultPropertyPage::onCommand(nCode, nId);
}

LRESULT qmrss::ReceivePage::onInitDialog(HWND hwndFocus,
										 LPARAM lParam)
{
	if (pSubAccount_->getProperty(L"Http", L"UseInternetSetting", 0))
		sendDlgItemMessage(IDC_INTERNETSETTING, BM_SETCHECK, BST_CHECKED);
	else if (pSubAccount_->getProperty(L"Http", L"UseProxy", 0))
		sendDlgItemMessage(IDC_CUSTOM, BM_SETCHECK, BST_CHECKED);
	else
		sendDlgItemMessage(IDC_NOPROXY, BM_SETCHECK, BST_CHECKED);
	
	wstring_ptr wstrHost(pSubAccount_->getProperty(L"Http", L"ProxyHost", L""));
	setDlgItemText(IDC_HOST, wstrHost.get());
	setDlgItemInt(IDC_PORT, pSubAccount_->getProperty(L"Http", L"ProxyPort", 8080));
	
	sendDlgItemMessage(IDC_MAKEMULTIPART, BM_SETCHECK,
		pSubAccount_->getProperty(L"Rss", L"MakeMultipart", 1) ? BST_CHECKED : BST_UNCHECKED);
	
	updateState();
	
	return TRUE;
}

LRESULT qmrss::ReceivePage::onOk()
{
	bool bUseInternetSetting = false;
	bool bUseProxy = false;
	if (sendDlgItemMessage(IDC_INTERNETSETTING, BM_GETCHECK) == BST_CHECKED)
		bUseInternetSetting = true;
	else if (sendDlgItemMessage(IDC_CUSTOM, BM_GETCHECK) == BST_CHECKED)
		bUseProxy = true;
	pSubAccount_->setProperty(L"Http", L"UseInternetSetting", bUseInternetSetting);
	pSubAccount_->setProperty(L"Http", L"UseProxy", bUseProxy);
	
	wstring_ptr wstrHost(getDlgItemText(IDC_HOST));
	if (wstrHost.get())
		pSubAccount_->setProperty(L"Http", L"ProxyHost", wstrHost.get());
	pSubAccount_->setProperty(L"Http", L"ProxyPort", getDlgItemInt(IDC_PORT));
	pSubAccount_->setProperty(L"Rss", L"MakeMultipart",
		sendDlgItemMessage(IDC_MAKEMULTIPART, BM_GETCHECK) == BST_CHECKED ? 1 : 0);
	
	return DefaultPropertyPage::onOk();
}

LRESULT qmrss::ReceivePage::onProxy(UINT nId)
{
	updateState();
	return 0;
}

void qmrss::ReceivePage::updateState()
{
	bool bEnable = sendDlgItemMessage(IDC_CUSTOM, BM_GETCHECK) == BST_CHECKED;
	Window(getDlgItem(IDC_HOST)).enableWindow(bEnable);
	Window(getDlgItem(IDC_PORT)).enableWindow(bEnable);
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
