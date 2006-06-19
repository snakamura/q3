/*
 * $Id$
 *
 * Copyright(C) 1998-2006 Satoshi Nakamura
 * All rights reserved.
 *
 */

#pragma warning(disable:4786)

#include <qmaccount.h>
#include <qmuiutil.h>

#include <qsuiutil.h>

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

qmimap4::Imap4SearchDriver::Imap4SearchDriver(Account* pAccount,
											  Profile* pProfile) :
	pAccount_(pAccount),
	pProfile_(pProfile)
{
}

qmimap4::Imap4SearchDriver::~Imap4SearchDriver()
{
}

bool qmimap4::Imap4SearchDriver::search(const SearchContext& context,
										MessageHolderList* pList)
{
	Imap4Driver* pDriver = static_cast<Imap4Driver*>(pAccount_->getProtocolDriver());
	SubAccount* pSubAccount = pAccount_->getCurrentSubAccount();
	
	wstring_ptr wstrCharset(pAccount_->getPropertyString(L"Imap4",
		L"SearchCharset", Part::getDefaultCharset()));
	int nUseCharset = pAccount_->getPropertyInt(L"Imap4", L"SearchUseCharset");
	
	SearchContext::FolderList listFolder;
	context.getTargetFolders(pAccount_, &listFolder);
	for (SearchContext::FolderList::const_iterator it = listFolder.begin(); it != listFolder.end(); ++it) {
		NormalFolder* pFolder = *it;
		if (pFolder->isFlag(Folder::FLAG_SYNCABLE) &&
			!pFolder->isFlag(Folder::FLAG_NOSELECT) &&
			!pFolder->isFlag(Folder::FLAG_LOCAL)) {
			if (!pDriver->search(pFolder, context.getCondition(),
				wstrCharset.get(), nUseCharset != 0, pList))
				return false;
		}
	}
	
	return true;
}


/****************************************************************************
 *
 * Imap4SearchUI
 *
 */

qmimap4::Imap4SearchUI::Imap4SearchUI(Account* pAccount,
									  Profile* pProfile) :
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

wstring_ptr qmimap4::Imap4SearchUI::getDisplayName()
{
	return loadString(getResourceHandle(), IDS_IMAP4SEARCH);
}

std::auto_ptr<SearchPropertyPage> qmimap4::Imap4SearchUI::createPropertyPage(bool bAllFolder,
																			 SearchPropertyData* pData)
{
	return std::auto_ptr<SearchPropertyPage>(new Imap4SearchPage(
		pAccount_, pProfile_, bAllFolder, pData));
}


/****************************************************************************
 *
 * Imap4SearchPage
 *
 */

qmimap4::Imap4SearchPage::Imap4SearchPage(Account* pAccount,
										  Profile* pProfile,
										  bool bAllFolderOnly,
										  SearchPropertyData* pData) :
	SearchPropertyPage(getResourceHandle(), IDD_SEARCH, pData),
	pAccount_(pAccount),
	pProfile_(pProfile),
	bAllFolderOnly_(bAllFolderOnly)
{
}

qmimap4::Imap4SearchPage::~Imap4SearchPage()
{
}

const WCHAR* qmimap4::Imap4SearchPage::getDriver() const
{
	return L"imap4";
}

const WCHAR* qmimap4::Imap4SearchPage::getCondition() const
{
	return wstrCondition_.get();
}

void qmimap4::Imap4SearchPage::updateData(SearchPropertyData* pData)
{
	wstring_ptr wstrCondition = getDlgItemText(IDC_CONDITION);
	pData->set(wstrCondition.get(),
		sendDlgItemMessage(IDC_ALLFOLDER, BM_GETCHECK) == BST_CHECKED,
		sendDlgItemMessage(IDC_RECURSIVE, BM_GETCHECK) == BST_CHECKED,
		sendDlgItemMessage(IDC_NEWFOLDER, BM_GETCHECK) == BST_CHECKED, getImeFlags());
}

void qmimap4::Imap4SearchPage::updateUI(const SearchPropertyData* pData)
{
	if (pData->getCondition()) {
		setDlgItemText(IDC_CONDITION, pData->getCondition());
		
		UINT nId = pData->isAllFolder() ? IDC_ALLFOLDER :
			pData->isRecursive() ? IDC_RECURSIVE : IDC_CURRENT;
		for (UINT n = IDC_CURRENT; n < IDC_CURRENT + 3; ++n)
			sendDlgItemMessage(n, BM_SETCHECK, n == nId ? BST_CHECKED : BST_UNCHECKED);
		
		sendDlgItemMessage(IDC_NEWFOLDER, BM_SETCHECK, pData->isNewFolder() ? BST_CHECKED : BST_UNCHECKED);
	}
	setImeFlags(pData->getImeFlags());
}

LRESULT qmimap4::Imap4SearchPage::onCommand(WORD nCode,
											WORD nId)
{
	BEGIN_COMMAND_HANDLER()
		HANDLE_COMMAND_ID(IDC_IMAP4COMMAND, onImap4Command)
	END_COMMAND_HANDLER()
	return SearchPropertyPage::onCommand(nCode, nId);
}

LRESULT qmimap4::Imap4SearchPage::onInitDialog(HWND hwndFocus,
											   LPARAM lParam)
{
	History history(pProfile_, L"Search");
	for (unsigned int n = 0; n < history.getSize(); ++n) {
		wstring_ptr wstr(history.getValue(n));
		if (*wstr.get()) {
			W2T(wstr.get(), ptsz);
			sendDlgItemMessage(IDC_CONDITION, CB_ADDSTRING,
				0, reinterpret_cast<LPARAM>(ptsz));
		}
	}
	if (sendDlgItemMessage(IDC_CONDITION, CB_GETCOUNT) != 0)
		sendDlgItemMessage(IDC_CONDITION, CB_SETCURSEL, 0);
	
	struct {
		UINT nId_;
		const WCHAR* pwszKey_;
	} items[] = {
		{ IDC_IMAP4COMMAND,	L"Command"		},
		{ IDC_SEARCHBODY,	L"SearchBody",	}
	};
	for (int n = 0; n < countof(items); ++n) {
		if (pProfile_->getInt(L"Imap4Search", items[n].pwszKey_) != 0)
			sendDlgItemMessage(items[n].nId_, BM_SETCHECK, BST_CHECKED);
	}
	
	if (bAllFolderOnly_) {
		Window(getDlgItem(IDC_CURRENT)).enableWindow(false);
		Window(getDlgItem(IDC_RECURSIVE)).enableWindow(false);
	}
	
	updateState();
	
	return TRUE;
}

LRESULT qmimap4::Imap4SearchPage::onOk()
{
	if (PropSheet_GetCurrentPageHwnd(getSheet()->getHandle()) == getHandle()) {
		wstring_ptr wstrSearch = getDlgItemText(IDC_CONDITION);
		if (*wstrSearch.get()) {
			History(pProfile_, L"Search").addValue(wstrSearch.get());
			bool bCommand = sendDlgItemMessage(IDC_IMAP4COMMAND, BM_GETCHECK) == BST_CHECKED;
			bool bSearchBody = sendDlgItemMessage(IDC_SEARCHBODY, BM_GETCHECK) == BST_CHECKED;
			if (bCommand) {
				wstrCondition_ = wstrSearch;
			}
			else {
				wstring_ptr wstrLiteral(getLiteral(wstrSearch.get()));
				
				StringBuffer<WSTRING> buf;
				const WCHAR* pwszFields[] = {
					L"SUBJECT ",
					L"FROM ",
					L"TO "
				};
				for (int n = 0; n < countof(pwszFields); ++n) {
					if (n != 0)
						buf.append(L" ");
					if (n != countof(pwszFields) - 1 || bSearchBody)
						buf.append(L"OR ");
					buf.append(pwszFields[n]);
					buf.append(wstrLiteral.get());
				}
				if (bSearchBody) {
					buf.append(L" TEXT ");
					buf.append(wstrLiteral.get());
				}
				
				wstrCondition_ = buf.getString();
			}
			
			pProfile_->setInt(L"Imap4Search", L"Command", bCommand);
			pProfile_->setInt(L"Imap4Search", L"SearchBody", bSearchBody);
		}
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

wstring_ptr qmimap4::Imap4SearchPage::getLiteral(const WCHAR* pwsz)
{
	StringBuffer<WSTRING> buf;
	buf.append(L'\"');
	for (const WCHAR* p = pwsz; *p; ++p) {
		if (*p == L'\"')
			buf.append(L'\\');
		buf.append(*p);
	}
	buf.append(L'\"');
	
	return buf.getString();
}


/****************************************************************************
 *
 * Imap4SearchDriverFactory
 *
 */

Imap4SearchDriverFactory qmimap4::Imap4SearchDriverFactory::factory__;

qmimap4::Imap4SearchDriverFactory::Imap4SearchDriverFactory()
{
	registerFactory(L"imap4", this);
}

qmimap4::Imap4SearchDriverFactory::~Imap4SearchDriverFactory()
{
	unregisterFactory(L"imap4");
}

std::auto_ptr<SearchDriver> qmimap4::Imap4SearchDriverFactory::createDriver(Document* pDocument,
																			Account* pAccount,
																			HWND hwnd,
																			Profile* pProfile)
{
	std::auto_ptr<SearchDriver> pDriver;
	if (wcscmp(pAccount->getType(Account::HOST_RECEIVE), L"imap4") == 0)
		pDriver.reset(new Imap4SearchDriver(pAccount, pProfile));
	return pDriver;
}

std::auto_ptr<SearchUI> qmimap4::Imap4SearchDriverFactory::createUI(Account* pAccount,
																	Profile* pProfile)
{
	std::auto_ptr<SearchUI> pUI;
	if (wcscmp(pAccount->getType(Account::HOST_RECEIVE), L"imap4") == 0)
		pUI.reset(new Imap4SearchUI(pAccount, pProfile));
	return pUI;
}
