/*
 * $Id$
 *
 * Copyright(C) 1998-2008 Satoshi Nakamura
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
	DefaultPropertyPage(getResourceHandle(), IDD_RECEIVE, LANDSCAPE(IDD_RECEIVE)),
	pSubAccount_(pSubAccount)
{
}

qmpop3::ReceivePage::~ReceivePage()
{
}

LRESULT qmpop3::ReceivePage::onInitDialog(HWND hwndFocus,
										  LPARAM lParam)
{
	bool bApop = pSubAccount_->getPropertyInt(L"Pop3", L"Apop") != 0;
	bool bDeleteOnServer = pSubAccount_->getPropertyInt(L"Pop3", L"DeleteOnServer") != 0;
	int nDeleteBefore = pSubAccount_->getPropertyInt(L"Pop3", L"DeleteBefore");
	bool bDeleteLocal = pSubAccount_->getPropertyInt(L"Pop3", L"DeleteLocal") != 0;
	bool bHandleStatus = pSubAccount_->getPropertyInt(L"Pop3", L"HandleStatus") != 0;
	bool bSkipDuplicatedUID = pSubAccount_->getPropertyInt(L"Pop3", L"SkipDuplicatedUID") != 0;
	
	sendDlgItemMessage(IDC_APOP, BM_SETCHECK, bApop ? BST_CHECKED : BST_UNCHECKED);
	if (bDeleteOnServer) {
		sendDlgItemMessage(IDC_DELETE, BM_SETCHECK, BST_CHECKED);
	}
	else if (nDeleteBefore != 0) {
		sendDlgItemMessage(IDC_DELETEOLD, BM_SETCHECK, BST_CHECKED);
		setDlgItemInt(IDC_DELETEAFTER, nDeleteBefore);
	}
	else {
		sendDlgItemMessage(IDC_DONTDELETE, BM_SETCHECK, BST_CHECKED);
	}
	sendDlgItemMessage(IDC_DELETELOCAL, BM_SETCHECK,
		bDeleteLocal ? BST_CHECKED : BST_UNCHECKED);
	sendDlgItemMessage(IDC_HANDLESTATUS, BM_SETCHECK,
		bHandleStatus ? BST_CHECKED : BST_UNCHECKED);
	sendDlgItemMessage(IDC_SKIPDUPLICATEDUID, BM_SETCHECK,
		bSkipDuplicatedUID ? BST_CHECKED : BST_UNCHECKED);
	
	updateState();
	
	return TRUE;
}

LRESULT qmpop3::ReceivePage::onOk()
{
	pSubAccount_->setPropertyInt(L"Pop3", L"Apop",
		sendDlgItemMessage(IDC_APOP, BM_GETCHECK) == BST_CHECKED ? 1 : 0);
	pSubAccount_->setPropertyInt(L"Pop3", L"DeleteOnServer",
		sendDlgItemMessage(IDC_DELETE, BM_GETCHECK) == BST_CHECKED ? 1 : 0);
	pSubAccount_->setPropertyInt(L"Pop3", L"DeleteBefore",
		sendDlgItemMessage(IDC_DELETEOLD, BM_GETCHECK) == BST_CHECKED ?
			getDlgItemInt(IDC_DELETEAFTER) : 0);
	pSubAccount_->setPropertyInt(L"Pop3", L"DeleteLocal",
		sendDlgItemMessage(IDC_DELETELOCAL, BM_GETCHECK) == BST_CHECKED ? 1 : 0);
	pSubAccount_->setPropertyInt(L"Pop3", L"HandleStatus",
		sendDlgItemMessage(IDC_HANDLESTATUS, BM_GETCHECK) == BST_CHECKED ? 1 : 0);
	pSubAccount_->setPropertyInt(L"Pop3", L"SkipDuplicatedUID",
		sendDlgItemMessage(IDC_SKIPDUPLICATEDUID, BM_GETCHECK) == BST_CHECKED ? 1 : 0);
	
	return DefaultPropertyPage::onOk();
}

LRESULT qmpop3::ReceivePage::onCommand(WORD nCode,
									   WORD nId)
{
	BEGIN_COMMAND_HANDLER()
		HANDLE_COMMAND_ID_RANGE(IDC_DONTDELETE, IDC_DELETEOLD, onDelete)
	END_COMMAND_HANDLER()
	return DefaultPropertyPage::onCommand(nCode, nId);
}

LRESULT qmpop3::ReceivePage::onDelete(UINT nId)
{
	updateState();
	return 0;
}

void qmpop3::ReceivePage::updateState()
{
	Window(getDlgItem(IDC_DELETEAFTER)).enableWindow(
		sendDlgItemMessage(IDC_DELETEOLD, BM_GETCHECK) == BST_CHECKED);
}


/****************************************************************************
 *
 * SendPage
 *
 */

qmpop3::SendPage::SendPage(SubAccount* pSubAccount) :
	DefaultPropertyPage(getResourceHandle(), IDD_SEND, LANDSCAPE(IDD_SEND)),
	pSubAccount_(pSubAccount)
{
}

qmpop3::SendPage::~SendPage()
{
}

LRESULT qmpop3::SendPage::onInitDialog(HWND hwndFocus,
									   LPARAM lParam)
{
	int nApop = pSubAccount_->getPropertyInt(L"Pop3Send", L"Apop");
	
	sendDlgItemMessage(IDC_APOP, BM_SETCHECK, nApop ? BST_CHECKED : BST_UNCHECKED);
	
	return TRUE;
}

LRESULT qmpop3::SendPage::onOk()
{
	pSubAccount_->setPropertyInt(L"Pop3Send", L"Apop",
		sendDlgItemMessage(IDC_APOP, BM_GETCHECK) == BST_CHECKED ? 1 : 0);
	
	return DefaultPropertyPage::onOk();
}
