/*
 * $Id$
 *
 * Copyright(C) 1998-2007 Satoshi Nakamura
 * All rights reserved.
 *
 */

#include "main.h"
#include "option.h"
#include "resourceinc.h"
#include "ui.h"

using namespace qmimap4;
using namespace qm;
using namespace qs;


/****************************************************************************
 *
 * ReceivePage
 *
 */

qmimap4::ReceivePage::ReceivePage(SubAccount* pSubAccount) :
	DefaultPropertyPage(getResourceHandle(), IDD_RECEIVE),
	pSubAccount_(pSubAccount)
{
}

qmimap4::ReceivePage::~ReceivePage()
{
}

LRESULT qmimap4::ReceivePage::onInitDialog(HWND hwndFocus,
										   LPARAM lParam)
{
	Account* pAccount = pSubAccount_->getAccount();
	wstring_ptr wstrRootFolder(pAccount->getPropertyString(L"Imap4", L"RootFolder"));
	int nFetchCount = pSubAccount_->getPropertyInt(L"Imap4", L"FetchCount");
	int nMaxSession = pSubAccount_->getPropertyInt(L"Imap4", L"MaxSession");
	int nOption = pSubAccount_->getPropertyInt(L"Imap4", L"Option");
	struct {
		UINT nId_;
		const WCHAR* pwszKey_;
	} items[] = {
		{ IDC_CLOSEFOLDER,	L"CloseFolder"	},
		{ IDC_NAMESPACE,	L"UseNamespace"	},
		{ IDC_PERSONAL,		L"UsePersonal"	},
		{ IDC_OTHERS,		L"UseOthers"	},
		{ IDC_SHARED,		L"UseShared"	}
	};
	
	setDlgItemText(IDC_ROOTFOLDER, wstrRootFolder.get());
	setDlgItemInt(IDC_FETCHCOUNT, nFetchCount);
	setDlgItemInt(IDC_MAXSESSION, nMaxSession);
	sendDlgItemMessage(IDC_USEENVELOPE, BM_SETCHECK,
		nOption & OPTION_USEENVELOPE ? BST_CHECKED : BST_UNCHECKED);
	sendDlgItemMessage(IDC_USEBODYSTRUCTUREALWAYS, BM_SETCHECK,
		nOption & OPTION_USEBODYSTRUCTUREALWAYS ? BST_CHECKED : BST_UNCHECKED);
	sendDlgItemMessage(IDC_TRUSTBODYSTRUCTURE, BM_SETCHECK,
		nOption & OPTION_TRUSTBODYSTRUCTURE ? BST_CHECKED : BST_UNCHECKED);
	for (int n = 0; n < countof(items); ++n) {
		int nValue = pSubAccount_->getPropertyInt(L"Imap4", items[n].pwszKey_);
		sendDlgItemMessage(items[n].nId_, BM_SETCHECK, nValue ? BST_CHECKED : BST_UNCHECKED);
	}
	
	updateState();
	
	return TRUE;
}

LRESULT qmimap4::ReceivePage::onOk()
{
	wstring_ptr wstrRootFolder(getDlgItemText(IDC_ROOTFOLDER));
	if (wstrRootFolder.get())
		pSubAccount_->setPropertyString(L"Imap4", L"RootFolder", wstrRootFolder.get());
	pSubAccount_->setPropertyInt(L"Imap4", L"FetchCount", getDlgItemInt(IDC_FETCHCOUNT));
	pSubAccount_->setPropertyInt(L"Imap4", L"MaxSession", getDlgItemInt(IDC_MAXSESSION));
	
	int nOption = 0;
	if (sendDlgItemMessage(IDC_USEENVELOPE, BM_GETCHECK) == BST_CHECKED)
		nOption |= OPTION_USEENVELOPE;
	if (sendDlgItemMessage(IDC_USEBODYSTRUCTUREALWAYS, BM_GETCHECK) == BST_CHECKED)
		nOption |= OPTION_USEBODYSTRUCTUREALWAYS;
	if (sendDlgItemMessage(IDC_TRUSTBODYSTRUCTURE, BM_GETCHECK) == BST_CHECKED)
		nOption |= OPTION_TRUSTBODYSTRUCTURE;
	pSubAccount_->setPropertyInt(L"Imap4", L"Option", nOption);
	
	struct {
		UINT nId_;
		const WCHAR* pwszKey_;
	} items[] = {
		{ IDC_CLOSEFOLDER,	L"CloseFolder"	},
		{ IDC_NAMESPACE,	L"UseNamespace"	},
		{ IDC_PERSONAL,		L"UsePersonal"	},
		{ IDC_OTHERS,		L"UseOthers"	},
		{ IDC_SHARED,		L"UseShared"	}
	};
	for (int n = 0; n < countof(items); ++n) {
		int nValue = sendDlgItemMessage(items[n].nId_, BM_GETCHECK) == BST_CHECKED ? 1 : 0;
		pSubAccount_->setPropertyInt(L"Imap4", items[n].pwszKey_, nValue);
	}
	
	return DefaultPropertyPage::onOk();
}

LRESULT qmimap4::ReceivePage::onCommand(WORD nCode,
										WORD nId)
{
	BEGIN_COMMAND_HANDLER()
		HANDLE_COMMAND_ID(IDC_NAMESPACE, onNamespace)
	END_COMMAND_HANDLER()
	return DefaultPropertyPage::onCommand(nCode, nId);
}

LRESULT qmimap4::ReceivePage::onNamespace()
{
	updateState();
	return 0;
}

void qmimap4::ReceivePage::updateState()
{
	bool bEnable = sendDlgItemMessage(IDC_NAMESPACE, BM_GETCHECK) == BST_CHECKED;
	UINT nIds[] = {
		IDC_PERSONAL,
		IDC_OTHERS,
		IDC_SHARED
	};
	for (int n = 0; n < countof(nIds); ++n)
		Window(getDlgItem(nIds[n])).enableWindow(bEnable);
}
