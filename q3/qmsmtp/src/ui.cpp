/*
 * $Id$
 *
 * Copyright(C) 1998-2006 Satoshi Nakamura
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
 * PopBeforeSmtpData
 *
 */

qmsmtp::PopBeforeSmtpData::PopBeforeSmtpData() :
	bCustom_(false),
	nPort_(110),
	secure_(SubAccount::SECURE_NONE),
	bApop_(false)
{
}

qmsmtp::PopBeforeSmtpData::~PopBeforeSmtpData()
{
}

void qmsmtp::PopBeforeSmtpData::load(const SubAccount* pSubAccount)
{
	bCustom_ = pSubAccount->getPropertyInt(L"Smtp", L"PopBeforeSmtpCustom") != 0;
	wstrProtocol_ = pSubAccount->getPropertyString(L"Smtp", L"PopBeforeSmtpProtocol");
	wstrHost_ = pSubAccount->getPropertyString(L"Smtp", L"PopBeforeSmtpHost");
	nPort_ = pSubAccount->getPropertyInt(L"Smtp", L"PopBeforeSmtpPort");
	int nSecure = pSubAccount->getPropertyInt(L"Smtp", L"PopBeforeSmtpSecure");
	if (SubAccount::SECURE_NONE <= nSecure && nSecure <= SubAccount::SECURE_STARTTLS)
		secure_ = static_cast<SubAccount::Secure>(nSecure);
	bApop_ = pSubAccount->getPropertyInt(L"Smtp", L"PopBeforeSmtpApop") != 0;
}

void qmsmtp::PopBeforeSmtpData::save(SubAccount* pSubAccount) const
{
	pSubAccount->setPropertyInt(L"Smtp", L"PopBeforeSmtpCustom", bCustom_);
	pSubAccount->setPropertyString(L"Smtp", L"PopBeforeSmtpProtocol", wstrProtocol_.get());
	pSubAccount->setPropertyString(L"Smtp", L"PopBeforeSmtpHost", wstrHost_.get());
	pSubAccount->setPropertyInt(L"Smtp", L"PopBeforeSmtpPort", nPort_);
	pSubAccount->setPropertyInt(L"Smtp", L"PopBeforeSmtpSecure", secure_);
	pSubAccount->getPropertyInt(L"Smtp", L"PopBeforeSmtpApop", bApop_);
}


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

LRESULT qmsmtp::SendPage::onCommand(WORD nCode,
									WORD nId)
{
	BEGIN_COMMAND_HANDLER()
		HANDLE_COMMAND_ID(IDC_CUSTOM, onCustom)
		HANDLE_COMMAND_ID(IDC_POPBEFORESMTP, onPopBeforeSmtp)
	END_COMMAND_HANDLER()
	return DefaultPropertyPage::onCommand(nCode, nId);
}

LRESULT qmsmtp::SendPage::onInitDialog(HWND hwndFocus,
									   LPARAM lParam)
{
	wstring_ptr wstrLocalHost(pSubAccount_->getPropertyString(L"Smtp", L"LocalHost"));
	setDlgItemText(IDC_LOCALHOST, wstrLocalHost.get());
	
	bool bPopBeforeSmtp = pSubAccount_->getPropertyInt(L"Smtp", L"PopBeforeSmtp") != 0;
	Button_SetCheck(getDlgItem(IDC_POPBEFORESMTP), bPopBeforeSmtp ? BST_CHECKED : BST_UNCHECKED);
	
	int nWait = pSubAccount_->getPropertyInt(L"Smtp", L"PopBeforeSmtpWait");
	setDlgItemInt(IDC_POPBEFORESMTPWAIT, nWait);
	
	popBeforeSmtpData_.load(pSubAccount_);
	
	updateState();
	
	return TRUE;
}

LRESULT qmsmtp::SendPage::onOk()
{
	wstring_ptr wstrLocalHost(getDlgItemText(IDC_LOCALHOST));
	if (wstrLocalHost.get())
		pSubAccount_->setPropertyString(L"Smtp", L"LocalHost", wstrLocalHost.get());
	
	pSubAccount_->setPropertyInt(L"Smtp", L"PopBeforeSmtp",
		Button_GetCheck(getDlgItem(IDC_POPBEFORESMTP)) == BST_CHECKED);
	pSubAccount_->setPropertyInt(L"Smtp", L"PopBeforeSmtpWait",
		getDlgItemInt(IDC_POPBEFORESMTPWAIT));
	
	popBeforeSmtpData_.save(pSubAccount_);
	
	return DefaultPropertyPage::onOk();
}

LRESULT qmsmtp::SendPage::onCustom()
{
	PopBeforeSmtpDialog dialog(&popBeforeSmtpData_);
	dialog.doModal(getHandle());
	return 0;
}

LRESULT qmsmtp::SendPage::onPopBeforeSmtp()
{
	updateState();
	return 0;
}

void qmsmtp::SendPage::updateState()
{
	bool bEnable = Button_GetCheck(getDlgItem(IDC_POPBEFORESMTP)) == BST_CHECKED;
	UINT nIds[] = {
		IDC_POPBEFORESMTPWAIT,
		IDC_CUSTOM
	};
	for (int n = 0; n < countof(nIds); ++n)
		Window(getDlgItem(nIds[n])).enableWindow(bEnable);
}


/****************************************************************************
 *
 * PopBeforeSmtpDialog
 *
 */

qmsmtp::PopBeforeSmtpDialog::PopBeforeSmtpDialog(PopBeforeSmtpData* pData) :
	DefaultDialog(getResourceHandle(), IDD_POPBEFORESMTP),
	pData_(pData)
{
	ConnectionFactory::getNames(&listName_);
}

qmsmtp::PopBeforeSmtpDialog::~PopBeforeSmtpDialog()
{
	std::for_each(listName_.begin(), listName_.end(), string_free<WSTRING>());
}

LRESULT qmsmtp::PopBeforeSmtpDialog::onCommand(WORD nCode,
											   WORD nId)
{
	BEGIN_COMMAND_HANDLER()
		HANDLE_COMMAND_ID(IDC_CUSTOM, onType)
		HANDLE_COMMAND_ID(IDC_DEFAULT, onType)
		HANDLE_COMMAND_ID_CODE(IDC_PROTOCOL, CBN_SELCHANGE, onProtocolSelChange)
		HANDLE_COMMAND_ID_RANGE(IDC_NONE, IDC_STARTTLS, onSecure)
	END_COMMAND_HANDLER()
	return DefaultDialog::onCommand(nCode, nId);
}

LRESULT qmsmtp::PopBeforeSmtpDialog::onInitDialog(HWND hwndFocus,
												  LPARAM lParam)
{
	init(false);
	
	UINT nTypeId = pData_->bCustom_ ? IDC_CUSTOM : IDC_DEFAULT;
	Button_SetCheck(getDlgItem(nTypeId),  BST_CHECKED);
	
	HWND hwndProtocol = getDlgItem(IDC_PROTOCOL);
	int nSelection = 0;
	for (ConnectionFactory::NameList::size_type n = 0; n < listName_.size(); ++n) {
		const WCHAR* pwszName = listName_[n];
		std::auto_ptr<ConnectionUI> pUI(ConnectionFactory::getUI(pwszName));
		wstring_ptr wstrDisplayName(pUI->getDisplayName());
		W2T(wstrDisplayName.get(), ptszName);
		ComboBox_AddString(hwndProtocol, ptszName);
		
		if (wcscmp(pData_->wstrProtocol_.get(), pwszName) == 0)
			nSelection = static_cast<int>(n);
	}
	ComboBox_SetCurSel(hwndProtocol, nSelection);
	
	setDlgItemText(IDC_HOST, pData_->wstrHost_.get());
	setDlgItemInt(IDC_PORT, pData_->nPort_);
	
	UINT nSecureId = IDC_NONE;
	switch (pData_->secure_) {
	case SubAccount::SECURE_SSL:
		nSecureId = IDC_SSL;
		break;
	case SubAccount::SECURE_STARTTLS:
		nSecureId = IDC_STARTTLS;
		break;
	}
	Button_SetCheck(getDlgItem(nSecureId), BST_CHECKED);
	
	Button_SetCheck(getDlgItem(IDC_APOP), pData_->bApop_);
	
	updateState();
	
	return TRUE;
}

LRESULT qmsmtp::PopBeforeSmtpDialog::onOk()
{
	pData_->bCustom_ = Button_GetCheck(getDlgItem(IDC_CUSTOM)) == BST_CHECKED;
	
	int nProtocol = ComboBox_GetCurSel(getDlgItem(IDC_PROTOCOL));
	pData_->wstrProtocol_ = allocWString(listName_[nProtocol]);
	
	pData_->wstrHost_ = getDlgItemText(IDC_HOST);
	pData_->nPort_ = getDlgItemInt(IDC_PORT);
	
	if (Button_GetCheck(getDlgItem(IDC_SSL)) == BST_CHECKED)
		pData_->secure_ = SubAccount::SECURE_SSL;
	else if (Button_GetCheck(getDlgItem(IDC_STARTTLS)) == BST_CHECKED)
		pData_->secure_ = SubAccount::SECURE_STARTTLS;
	else
		pData_->secure_ = SubAccount::SECURE_NONE;
	
	pData_->bApop_ = Button_GetCheck(getDlgItem(IDC_APOP)) == BST_CHECKED;
	
	return DefaultDialog::onOk();
}

LRESULT qmsmtp::PopBeforeSmtpDialog::onProtocolSelChange()
{
	updatePort();
	updateState();
	return 0;
}

LRESULT qmsmtp::PopBeforeSmtpDialog::onSecure(UINT nId)
{
	updatePort();
	return 0;
}

LRESULT qmsmtp::PopBeforeSmtpDialog::onType()
{
	updateState();
	return 0;
}

void qmsmtp::PopBeforeSmtpDialog::updateState()
{
	bool bEnable = Button_GetCheck(getDlgItem(IDC_CUSTOM)) == BST_CHECKED;
	UINT nIds[] = {
		IDC_PROTOCOL,
		IDC_HOST,
		IDC_PORT,
		IDC_NONE,
		IDC_SSL,
		IDC_STARTTLS,
		IDC_APOP
	};
	for (int n = 0; n < countof(nIds); ++n)
		Window(getDlgItem(nIds[n])).enableWindow(bEnable);
	
	int nProtocol = ComboBox_GetCurSel(getDlgItem(IDC_PROTOCOL));
	const WCHAR* pwszName = listName_[nProtocol];
	Window(getDlgItem(IDC_APOP)).enableWindow(wcscmp(pwszName, L"pop3") == 0);
}

void qmsmtp::PopBeforeSmtpDialog::updatePort()
{
	int nProtocol = ComboBox_GetCurSel(getDlgItem(IDC_PROTOCOL));
	const WCHAR* pwszName = listName_[nProtocol];
	std::auto_ptr<ConnectionUI> pUI(ConnectionFactory::getUI(pwszName));
	setDlgItemInt(IDC_PORT, pUI->getDefaultPort(Button_GetCheck(getDlgItem(IDC_SSL)) == BST_CHECKED));
}
