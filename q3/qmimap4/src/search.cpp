/*
 * $Id$
 *
 * Copyright(C) 1998-2003 Satoshi Nakamura
 * All rights reserved.
 *
 */

#include <qmaccount.h>

#include <qsnew.h>

#include "imap4driver.h"
#include "main.h"
#include "resourceinc.h"
#include "search.h"

using namespace qmimap4;
using namespace qm;
using namespace qs;


/****************************************************************************
 *
 * Imap4SearchDriver
 *
 */

qmimap4::Imap4SearchDriver::Imap4SearchDriver(
	Account* pAccount, Profile* pProfile, QSTATUS* pstatus) :
	pAccount_(pAccount),
	pProfile_(pProfile)
{
}

qmimap4::Imap4SearchDriver::~Imap4SearchDriver()
{
}

QSTATUS qmimap4::Imap4SearchDriver::search(
	const SearchContext& context, MessageHolderList* pList)
{
	DECLARE_QSTATUS();
	
	Imap4Driver* pDriver = static_cast<Imap4Driver*>(
		pAccount_->getProtocolDriver());
	SubAccount* pSubAccount = pAccount_->getCurrentSubAccount();
	
	string_ptr<WSTRING> wstrCharset;
	status = pAccount_->getProperty(L"Imap4", L"SearchCharset",
		Part::getDefaultCharset(), &wstrCharset);
	CHECK_QSTATUS();
	int nUseCharset = 1;
	status = pAccount_->getProperty(L"Imap4", L"SearchUseCharset", 1, &nUseCharset);
	CHECK_QSTATUS();
	
	SearchContext::FolderList listFolder;
	status = context.getTargetFolders(pAccount_, &listFolder);
	CHECK_QSTATUS();
	SearchContext::FolderList::const_iterator it = listFolder.begin();
	while (it != listFolder.end()) {
		NormalFolder* pFolder = *it;
		if (pFolder->isFlag(Folder::FLAG_SYNCABLE) &&
			!pFolder->isFlag(Folder::FLAG_NOSELECT) &&
			!pFolder->isFlag(Folder::FLAG_LOCAL)) {
			status = pDriver->search(pSubAccount, pFolder, context.getCondition(),
				wstrCharset.get(), nUseCharset != 0, pList);
			CHECK_QSTATUS();
		}
		++it;
	}
	
	return QSTATUS_SUCCESS;
}


/****************************************************************************
 *
 * Imap4SearchUI
 *
 */

qmimap4::Imap4SearchUI::Imap4SearchUI(Account* pAccount,
	Profile* pProfile, QSTATUS* pstatus) :
	pAccount_(pAccount),
	pProfile_(pProfile)
{
}

qmimap4::Imap4SearchUI::~Imap4SearchUI()
{
}

int qmimap4::Imap4SearchUI::getIndex()
{
	return 15;
}

const WCHAR* qmimap4::Imap4SearchUI::getName()
{
	return L"imap4";
}

QSTATUS qmimap4::Imap4SearchUI::getDisplayName(qs::WSTRING* pwstrName)
{
	return loadString(getResourceHandle(), IDS_IMAP4SEARCH, pwstrName);
}

QSTATUS qmimap4::Imap4SearchUI::createPropertyPage(
	bool bAllFolder, SearchPropertyPage** ppPage)
{
	DECLARE_QSTATUS();
	
	std::auto_ptr<Imap4SearchPage> pPage;
	status = newQsObject(pAccount_, pProfile_, bAllFolder, &pPage);
	CHECK_QSTATUS();
	*ppPage = pPage.release();
	
	return QSTATUS_SUCCESS;
}


/****************************************************************************
 *
 * Imap4SearchPage
 *
 */

qmimap4::Imap4SearchPage::Imap4SearchPage(Account* pAccount,
	Profile* pProfile, bool bAllFolder, QSTATUS* pstatus) :
	SearchPropertyPage(getResourceHandle(), IDD_SEARCH, pstatus),
	pAccount_(pAccount),
	pProfile_(pProfile),
	wstrCondition_(0),
	bAllFolder_(bAllFolder),
	bRecursive_(false)
{
}

qmimap4::Imap4SearchPage::~Imap4SearchPage()
{
	freeWString(wstrCondition_);
}

const WCHAR* qmimap4::Imap4SearchPage::getDriver() const
{
	return L"imap4";
}

const WCHAR* qmimap4::Imap4SearchPage::getCondition() const
{
	return wstrCondition_;
}

bool qmimap4::Imap4SearchPage::isAllFolder() const
{
	return bAllFolder_;
}

bool qmimap4::Imap4SearchPage::isRecursive() const
{
	return bRecursive_;
}

LRESULT qmimap4::Imap4SearchPage::onCommand(WORD nCode, WORD nId)
{
	BEGIN_COMMAND_HANDLER()
		HANDLE_COMMAND_ID(IDC_IMAP4COMMAND, onImap4Command)
	END_COMMAND_HANDLER()
	return SearchPropertyPage::onCommand(nCode, nId);
}

LRESULT qmimap4::Imap4SearchPage::onInitDialog(HWND hwndFocus, LPARAM lParam)
{
	DECLARE_QSTATUS();
	
	string_ptr<WSTRING> wstrCondition;
	status = pProfile_->getString(L"Search", L"Condition", L"", &wstrCondition);
	CHECK_QSTATUS();
	setDlgItemText(IDC_CONDITION, wstrCondition.get());
	
	struct {
		UINT nId_;
		const WCHAR* pwszKey_;
	} items[] = {
		{ IDC_IMAP4COMMAND,	L"Command"		},
		{ IDC_SEARCHBODY,	L"SearchBody",	}
	};
	for (int n = 0; n < countof(items); ++n) {
		int nValue = 0;
		status = pProfile_->getInt(L"Imap4Search", items[n].pwszKey_, 0, &nValue);
		CHECK_QSTATUS();
		if (nValue != 0)
			sendDlgItemMessage(items[n].nId_, BM_SETCHECK, BST_CHECKED);
	}
	
	int nFolder = 0;
	if (bAllFolder_) {
		Window(getDlgItem(IDC_CURRENT)).enableWindow(false);
		Window(getDlgItem(IDC_RECURSIVE)).enableWindow(false);
		nFolder = 2;
	}
	else {
		status = pProfile_->getInt(L"Search", L"Folder", 0, &nFolder);
		CHECK_QSTATUS();
		if (nFolder < 0 || 3 < nFolder)
			nFolder = 0;
	}
	sendDlgItemMessage(IDC_CURRENT + nFolder, BM_SETCHECK, BST_CHECKED);
	
	updateState();
	
	return TRUE;
}

LRESULT qmimap4::Imap4SearchPage::onOk()
{
	DECLARE_QSTATUS();
	
	if (PropSheet_GetCurrentPageHwnd(getSheet()->getHandle()) == getHandle()) {
		string_ptr<WSTRING> wstrSearch(getDlgItemText(IDC_CONDITION));
		bool bCommand = sendDlgItemMessage(IDC_IMAP4COMMAND, BM_GETCHECK) == BST_CHECKED;
		bool bSearchBody = sendDlgItemMessage(IDC_SEARCHBODY, BM_GETCHECK) == BST_CHECKED;
		if (bCommand) {
			wstrCondition_ = wstrSearch.release();
		}
		else {
			string_ptr<WSTRING> wstrLiteral;
			status = getLiteral(wstrSearch.get(), &wstrLiteral);
			CHECK_QSTATUS();
			
			StringBuffer<WSTRING> buf(&status);
			CHECK_QSTATUS();
			const WCHAR* pwszFields[] = {
				L"SUBJECT ",
				L"FROM ",
				L"TO "
			};
			for (int n = 0; n < countof(pwszFields); ++n) {
				if (n != 0) {
					status = buf.append(L" ");
					CHECK_QSTATUS();
				}
				if (n != countof(pwszFields) - 1 || bSearchBody) {
					status = buf.append(L"OR ");
					CHECK_QSTATUS();
				}
				status = buf.append(pwszFields[n]);
				CHECK_QSTATUS();
				status = buf.append(wstrLiteral.get());
				CHECK_QSTATUS();
			}
			if (bSearchBody) {
				status = buf.append(L" TEXT ");
				CHECK_QSTATUS();
				status = buf.append(wstrLiteral.get());
				CHECK_QSTATUS();
			}
			
			wstrCondition_ = buf.getString();
		}
		
		bAllFolder_ = sendDlgItemMessage(IDC_ALLFOLDER, BM_GETCHECK) == BST_CHECKED;
		bRecursive_ = sendDlgItemMessage(IDC_RECURSIVE, BM_GETCHECK) == BST_CHECKED;
		
		pProfile_->setString(L"Search", L"Condition",
			bCommand ? wstrCondition_ : wstrSearch.get());
		pProfile_->setInt(L"Imap4Search", L"Command", bCommand);
		pProfile_->setInt(L"Imap4Search", L"SearchBody", bSearchBody);
		pProfile_->setInt(L"Search", L"Folder",
			bAllFolder_ ? 2 : bRecursive_ ? 1 : 0);
	}
	return SearchPropertyPage::onOk();
}

LRESULT qmimap4::Imap4SearchPage::onImap4Command()
{
	updateState();
	return 0;
}

void qmimap4::Imap4SearchPage::updateState()
{
	bool bEnable = sendDlgItemMessage(IDC_IMAP4COMMAND, BM_GETCHECK) != BST_CHECKED;
	Window(getDlgItem(IDC_SEARCHBODY)).enableWindow(bEnable);
}

QSTATUS qmimap4::Imap4SearchPage::getLiteral(const WCHAR* pwsz, WSTRING* pwstr)
{
	DECLARE_QSTATUS();
	
	StringBuffer<WSTRING> buf(&status);
	CHECK_QSTATUS();
	status = buf.append(L'\"');
	CHECK_QSTATUS();
	for (const WCHAR* p = pwsz; *p; ++p) {
		if (*p == L'\"') {
			status = buf.append(L'\\');
			CHECK_QSTATUS();
		}
		status = buf.append(*p);
		CHECK_QSTATUS();
	}
	status = buf.append(L'\"');
	CHECK_QSTATUS();
	
	*pwstr = buf.getString();
	
	return QSTATUS_SUCCESS;
}


/****************************************************************************
 *
 * Imap4SearchDriverFactory
 *
 */

Imap4SearchDriverFactory qmimap4::Imap4SearchDriverFactory::factory__;

qmimap4::Imap4SearchDriverFactory::Imap4SearchDriverFactory()
{
	regist(L"imap4", this);
}

qmimap4::Imap4SearchDriverFactory::~Imap4SearchDriverFactory()
{
	unregist(L"imap4");
}

QSTATUS qmimap4::Imap4SearchDriverFactory::createDriver(Document* pDocument,
	Account* pAccount, HWND hwnd, Profile* pProfile, SearchDriver** ppDriver)
{
	DECLARE_QSTATUS();
	
	if (wcscmp(pAccount->getType(Account::HOST_RECEIVE), L"imap4") == 0) {
		std::auto_ptr<Imap4SearchDriver> pDriver;
		status = newQsObject(pAccount, pProfile, &pDriver);
		CHECK_QSTATUS();
		*ppDriver = pDriver.release();
	}
	
	return QSTATUS_SUCCESS;
}

QSTATUS qmimap4::Imap4SearchDriverFactory::createUI(
	Account* pAccount, Profile* pProfile, SearchUI** ppUI)
{
	DECLARE_QSTATUS();
	
	if (wcscmp(pAccount->getType(Account::HOST_RECEIVE), L"imap4") == 0) {
		std::auto_ptr<Imap4SearchUI> pUI;
		status = newQsObject(pAccount, pProfile, &pUI);
		CHECK_QSTATUS();
		*ppUI = pUI.release();
	}
	
	return QSTATUS_SUCCESS;
}
