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

using namespace qmsmtp;
using namespace qm;
using namespace qs;


/****************************************************************************
 *
 * SendPage
 *
 */

qmsmtp::SendPage::SendPage(SubAccount* pSubAccount) :
	DefaultPropertyPage(getResourceHandle(), IDD_SEND),
	pSubAccount_(pSubAccount)
{
}

qmsmtp::SendPage::~SendPage()
{
}

LRESULT qmsmtp::SendPage::onInitDialog(HWND hwndFocus,
									   LPARAM lParam)
{
	wstring_ptr wstrLocalHost(pSubAccount_->getPropertyString(L"Smtp", L"LocalHost"));
	
	setDlgItemText(IDC_LOCALHOST, wstrLocalHost.get());
	
	return TRUE;
}

LRESULT qmsmtp::SendPage::onOk()
{
	wstring_ptr wstrLocalHost(getDlgItemText(IDC_LOCALHOST));
	if (wstrLocalHost.get())
		pSubAccount_->setPropertyString(L"Smtp", L"LocalHost", wstrLocalHost.get());
	
	return DefaultPropertyPage::onOk();
}
