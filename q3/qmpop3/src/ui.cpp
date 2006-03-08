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
	bool bApop = pSubAccount_->getProperty(L"Pop3", L"Apop", 0) != 0;
	bool bDeleteOnServer = pSubAccount_->getProperty(L"Pop3", L"DeleteOnServer", 0) != 0;
	int nDeleteBefore = pSubAccount_->getProperty(L"Pop3", L"DeleteBefore", 0);
	bool bDeleteLocal = pSubAccount_->getProperty(L"Pop3", L"DeleteLocal", 0) != 0;
	bool bHandleStatus = pSubAccount_->getProperty(L"Pop3", L"HandleStatus", 0) != 0;
	int nNoopInterval = pSubAccount_->getProperty(L"Pop3", L"NoopInterval", 100);
	int nGetAll = pSubAccount_->getProperty(L"Pop3", L"GetAll", 20);
	bool bSkipDuplicatedUID = pSubAccount_->getProperty(L"Pop3", L"SkipDuplicatedUID", 0) != 0;
	
	sendDlgItemMessage(IDC_APOP, BM_SETCHECK, bApop ? BST_CHECKED : BST_UNCHECKED);
	sendDlgItemMessage(IDC_DELETEONSERVER, BM_SETCHECK,
		bDeleteOnServer ? BST_CHECKED : BST_UNCHECKED);
	setDlgItemInt(IDC_DELETEBEFORE, nDeleteBefore);
	sendDlgItemMessage(IDC_DELETELOCAL, BM_SETCHECK,
		bDeleteLocal ? BST_CHECKED : BST_UNCHECKED);
	sendDlgItemMessage(IDC_HANDLESTATUS, BM_SETCHECK,
		bHandleStatus ? BST_CHECKED : BST_UNCHECKED);
	setDlgItemInt(IDC_NOOPINTERVAL, nNoopInterval);
	setDlgItemInt(IDC_GETALL, nGetAll);
	sendDlgItemMessage(IDC_SKIPDUPLICATEDUID, BM_SETCHECK,
		bSkipDuplicatedUID ? BST_CHECKED : BST_UNCHECKED);
	
	updateState();
	
	return TRUE;
}

LRESULT qmpop3::ReceivePage::onOk()
{
	pSubAccount_->setProperty(L"Pop3", L"Apop",
		sendDlgItemMessage(IDC_APOP, BM_GETCHECK) == BST_CHECKED ? 1 : 0);
	pSubAccount_->setProperty(L"Pop3", L"DeleteOnServer",
		sendDlgItemMessage(IDC_DELETEONSERVER, BM_GETCHECK) == BST_CHECKED ? 1 : 0);
	pSubAccount_->setProperty(L"Pop3", L"DeleteBefore", getDlgItemInt(IDC_DELETEBEFORE));
	pSubAccount_->setProperty(L"Pop3", L"DeleteLocal",
		sendDlgItemMessage(IDC_DELETELOCAL, BM_GETCHECK) == BST_CHECKED ? 1 : 0);
	pSubAccount_->setProperty(L"Pop3", L"HandleStatus",
		sendDlgItemMessage(IDC_HANDLESTATUS, BM_GETCHECK) == BST_CHECKED ? 1 : 0);
	pSubAccount_->setProperty(L"Pop3", L"NoopInterval",
		getDlgItemInt(IDC_NOOPINTERVAL));
	pSubAccount_->setProperty(L"Pop3", L"GetAll", getDlgItemInt(IDC_GETALL));
	pSubAccount_->setProperty(L"Pop3", L"SkipDuplicatedUID",
		sendDlgItemMessage(IDC_SKIPDUPLICATEDUID, BM_GETCHECK) == BST_CHECKED ? 1 : 0);
	
	return DefaultPropertyPage::onOk();
}

LRESULT qmpop3::ReceivePage::onCommand(WORD nCode,
									   WORD nId)
{
	BEGIN_COMMAND_HANDLER()
		HANDLE_COMMAND_ID(IDC_DELETEONSERVER, onDeleteOnServer)
	END_COMMAND_HANDLER()
	return DefaultPropertyPage::onCommand(nCode, nId);
}

LRESULT qmpop3::ReceivePage::onDeleteOnServer()
{
	updateState();
	return 0;
}

void qmpop3::ReceivePage::updateState()
{
	Window(getDlgItem(IDC_DELETEBEFORE)).enableWindow(
		sendDlgItemMessage(IDC_DELETEONSERVER, BM_GETCHECK) != BST_CHECKED);
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
