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
	return TRUE;
}

LRESULT qmrss::ReceivePage::onOk()
{
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
