/*
 * $Id$
 *
 * Copyright(C) 1998-2005 Satoshi Nakamura
 * All rights reserved.
 *
 */

#include <qmaccount.h>
#include <qmapplication.h>
#include <qmfolder.h>
#include <qmuiutil.h>

#include <qsfile.h>
#include <qsosutil.h>
#include <qsstl.h>
#include <qsuiutil.h>

#include "fulltextsearch.h"
#include "../ui/resourceinc.h"

using namespace qm;
using namespace qs;


#ifndef _WIN32_WCE

/****************************************************************************
 *
 * FullTextSearchDriver
 *
 */

qm::FullTextSearchDriver::FullTextSearchDriver(Account* pAccount,
											   Profile* pProfile) :
	pAccount_(pAccount),
	pProfile_(pProfile)
{
}

qm::FullTextSearchDriver::~FullTextSearchDriver()
{
}

bool qm::FullTextSearchDriver::search(const SearchContext& context,
									  MessageHolderList* pList)
{
	wstring_ptr wstrCommand(pProfile_->getString(L"FullTextSearch",
		L"Command", L"namazu -l -a \"$condition\" \"$index\""));
	
	wstrCommand = FullTextSearchUtil::replace(
		wstrCommand.get(), L"$condition", context.getCondition());
	if (!wstrCommand.get())
		return true;
	
	wstring_ptr wstrIndex(pAccount_->getProperty(L"FullTextSearch", L"Index", L""));
	if (!*wstrIndex.get())
		wstrIndex = concat(pAccount_->getPath(), L"\\index");
	wstrCommand = FullTextSearchUtil::replace(wstrCommand.get(), L"$index", wstrIndex.get());
	if (!wstrCommand.get())
		return true;
	
	wstring_ptr wstrOutput(Process::exec(wstrCommand.get(), 0));
	if (!wstrOutput.get())
		return false;
	
	typedef std::vector<unsigned int> OffsetList;
	OffsetList listOffset;
	
	const WCHAR* p = wstrOutput.get();
	while (*p) {
		WCHAR* pEnd = wcschr(p, L'\n');
		if (!pEnd)
			break;
		if (*(pEnd - 1) == L'\r')
			--pEnd;
		*pEnd = L'\0';
		
		p = wcsrchr(p, L'/');
		if (p) {
			WCHAR* pp = 0;
			unsigned int n = static_cast<unsigned int>(wcstol(p + 1, &pp, 10));
			if (*pp == L'.')
				listOffset.push_back(n);
		}
		
		p = pEnd + 1;
		if (*p == L'\n')
			++p;
	}
	if (listOffset.empty())
		return true;
	std::sort(listOffset.begin(), listOffset.end());
	
	SearchContext::FolderList listFolder;
	context.getTargetFolders(pAccount_, &listFolder);
	
	MessageHolderList listMessageHolder;
	for (SearchContext::FolderList::const_iterator itF = listFolder.begin(); itF != listFolder.end(); ++itF) {
		NormalFolder* pFolder = *itF;
		
		if (!pFolder->loadMessageHolders())
			return false;
		
		const MessageHolderList& l = pFolder->getMessages();
		listMessageHolder.reserve(listMessageHolder.size() + l.size());
		std::copy(l.begin(), l.end(), std::back_inserter(listMessageHolder));
	}
	
	std::sort(listMessageHolder.begin(), listMessageHolder.end(),
		binary_compose_f_gx_hy(
			std::less<unsigned int>(),
			unary_compose_f_gx(
				mem_data_ref(&MessageHolder::MessageBoxKey::nOffset_),
				std::mem_fun(&MessageHolder::getMessageBoxKey)),
			unary_compose_f_gx(
				mem_data_ref(&MessageHolder::MessageBoxKey::nOffset_),
				std::mem_fun(&MessageHolder::getMessageBoxKey))));
	
	pList->reserve(listOffset.size());
	
	unsigned int nPrevOffset = -1;
	for (OffsetList::const_iterator itO = listOffset.begin(); itO != listOffset.end(); ++itO) {
		unsigned int nOffset = *itO;
		if (nOffset == nPrevOffset)
			continue;
		
		MessageHolder::Init init = { 0 };
		init.nOffset_ = nOffset;
		
		MessageHolder mh(0, init);
		
		MessageHolderList::const_iterator itM = std::lower_bound(
			listMessageHolder.begin(), listMessageHolder.end(), &mh,
			binary_compose_f_gx_hy(
				std::less<unsigned int>(),
				unary_compose_f_gx(
					mem_data_ref(&MessageHolder::MessageBoxKey::nOffset_),
					std::mem_fun(&MessageHolder::getMessageBoxKey)),
				unary_compose_f_gx(
					mem_data_ref(&MessageHolder::MessageBoxKey::nOffset_),
					std::mem_fun(&MessageHolder::getMessageBoxKey))));
		if (itM != listMessageHolder.end() &&
			(*itM)->getMessageBoxKey().nOffset_ == nOffset)
			pList->push_back(*itM);
		
		nPrevOffset = nOffset;
	}
	
	return true;
}


/****************************************************************************
 *
 * FullTextSearchUI
 *
 */

qm::FullTextSearchUI::FullTextSearchUI(Account* pAccount,
									   Profile* pProfile) :
	pAccount_(pAccount),
	pProfile_(pProfile)
{
}

qm::FullTextSearchUI::~FullTextSearchUI()
{
}

int qm::FullTextSearchUI::getIndex()
{
	return 20;
}

const WCHAR* qm::FullTextSearchUI::getName()
{
	return L"fulltext";
}

wstring_ptr qm::FullTextSearchUI::getDisplayName()
{
	return loadString(Application::getApplication().getResourceHandle(), IDS_FULLTEXTSEARCH);
}

std::auto_ptr<SearchPropertyPage> qm::FullTextSearchUI::createPropertyPage(bool bAllFolder,
																		   SearchPropertyData* pData)
{
	return std::auto_ptr<SearchPropertyPage>(new FullTextSearchPage(
		pAccount_, pProfile_, bAllFolder, pData));
}


/****************************************************************************
 *
 * FullTextSearchPage
 *
 */

qm::FullTextSearchPage::FullTextSearchPage(Account* pAccount,
										   Profile* pProfile,
										   bool bAllFolder,
										   SearchPropertyData* pData) :
	SearchPropertyPage(Application::getApplication().getResourceHandle(), IDD_FULLTEXTSEARCH, pData),
	pAccount_(pAccount),
	pProfile_(pProfile),
	bAllFolder_(bAllFolder),
	bRecursive_(false)
{
}

qm::FullTextSearchPage::~FullTextSearchPage()
{
}

const WCHAR* qm::FullTextSearchPage::getDriver() const
{
	return L"fulltext";
}

const WCHAR* qm::FullTextSearchPage::getCondition() const
{
	return wstrCondition_.get();
}

bool qm::FullTextSearchPage::isAllFolder() const
{
	return bAllFolder_;
}

bool qm::FullTextSearchPage::isRecursive() const
{
	return bRecursive_;
}

void qm::FullTextSearchPage::updateData(SearchPropertyData* pData)
{
	wstring_ptr wstrCondition = getDlgItemText(IDC_CONDITION);
	pData->set(wstrCondition.get(),
		sendDlgItemMessage(IDC_ALLFOLDER, BM_GETCHECK) == BST_CHECKED,
		sendDlgItemMessage(IDC_RECURSIVE, BM_GETCHECK) == BST_CHECKED,
		UIUtil::isImeEnabled(getHandle()));
}

void qm::FullTextSearchPage::updateUI(const SearchPropertyData* pData)
{
	if (pData->getCondition()) {
		setDlgItemText(IDC_CONDITION, pData->getCondition());
		UINT nId = pData->isAllFolder() ? IDC_ALLFOLDER :
			pData->isRecursive() ? IDC_RECURSIVE : IDC_CURRENT;
		for (UINT n = IDC_CURRENT; n < IDC_CURRENT + 3; ++n)
			sendDlgItemMessage(n, BM_SETCHECK, n == nId ? BST_CHECKED : BST_UNCHECKED);
	}
	UIUtil::setImeEnabled(getHandle(), pData->isIme());
}

LRESULT qm::FullTextSearchPage::onCommand(WORD nCode,
										  WORD nId)
{
	BEGIN_COMMAND_HANDLER()
		HANDLE_COMMAND_ID(IDC_UPDATEINDEX, onUpdateIndex)
	END_COMMAND_HANDLER()
	return SearchPropertyPage::onCommand(nCode, nId);
}

LRESULT qm::FullTextSearchPage::onInitDialog(HWND hwndFocus,
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
	
	int nFolder = 0;
	if (bAllFolder_) {
		Window(getDlgItem(IDC_CURRENT)).enableWindow(false);
		Window(getDlgItem(IDC_RECURSIVE)).enableWindow(false);
		nFolder = 2;
	}
	else {
		nFolder = pProfile_->getInt(L"Search", L"Folder", 0);
		if (nFolder < 0 || 3 < nFolder)
			nFolder = 0;
	}
	sendDlgItemMessage(IDC_CURRENT + nFolder, BM_SETCHECK, BST_CHECKED);
	
	return TRUE;
}

LRESULT qm::FullTextSearchPage::onOk()
{
	if (PropSheet_GetCurrentPageHwnd(getSheet()->getHandle()) == getHandle()) {
		Window edit(Window(getDlgItem(IDC_CONDITION)).getWindow(GW_CHILD));
		wstrCondition_ = edit.getWindowText();
		if (wstrCondition_.get())
			History(pProfile_, L"Search").addValue(wstrCondition_.get());
		bAllFolder_ = sendDlgItemMessage(IDC_ALLFOLDER, BM_GETCHECK) == BST_CHECKED;
		bRecursive_ = sendDlgItemMessage(IDC_RECURSIVE, BM_GETCHECK) == BST_CHECKED;
		
		pProfile_->setString(L"Search", L"Condition", wstrCondition_.get());
		pProfile_->setInt(L"Search", L"Folder", bAllFolder_ ? 2 : bRecursive_ ? 1 : 0);
	}
	return SearchPropertyPage::onOk();
}

LRESULT qm::FullTextSearchPage::onUpdateIndex()
{
	updateIndex();
	return 0;
}

bool qm::FullTextSearchPage::updateIndex()
{
	wstring_ptr wstrCommand(pProfile_->getString(L"FullTextSearch",
		L"IndexCommand", L"mknmz.bat -a -h -O \"$index\" \"$msg\""));
	
	wstring_ptr wstrIndex(pAccount_->getProperty(L"FullTextSearch", L"Index", L""));
	if (!*wstrIndex.get())
		wstrIndex = concat(pAccount_->getPath(), L"\\index");
	
	if (!File::createDirectory(wstrIndex.get()))
		return false;
	
	// Because mknmz cannot handle a path with whitespace correctly,
	// if the path contains whitespace, use its short file name.
	if (wcschr(wstrIndex.get(), L' ')) {
		W2T(wstrIndex.get(), ptszIndex);
		TCHAR tszShort[MAX_PATH];
		if (::GetShortPathName(ptszIndex, tszShort, countof(tszShort)))
			wstrIndex = tcs2wcs(tszShort);
	}
	wstrCommand = FullTextSearchUtil::replace(wstrCommand.get(), L"$index", wstrIndex.get());
	
	wstring_ptr wstrMsg(concat(pAccount_->getMessageStorePath(), L"\\msg"));
	wstrCommand = FullTextSearchUtil::replace(wstrCommand.get(), L"$msg", wstrMsg.get());
	
	W2T(wstrCommand.get(), ptszCommand);
	STARTUPINFO si = { sizeof(si) };
	PROCESS_INFORMATION pi;
	if (!::CreateProcess(0, const_cast<LPTSTR>(ptszCommand),
		0, 0, TRUE, 0, 0, 0, &si, &pi))
		return false;
	::CloseHandle(pi.hThread);
//	::WaitForSingleObject(pi.hProcess, INFINITE);
	::CloseHandle(pi.hProcess);
	
	return true;
}


/****************************************************************************
 *
 * FullTextSearchDriverFactory
 *
 */

FullTextSearchDriverFactory* qm::FullTextSearchDriverFactory::pFactory__;
FullTextSearchDriverFactory::InitializerImpl qm::FullTextSearchDriverFactory::init__;

qm::FullTextSearchDriverFactory::FullTextSearchDriverFactory()
{
	registerFactory(L"fulltext", this);
}

qm::FullTextSearchDriverFactory::~FullTextSearchDriverFactory()
{
	unregisterFactory(L"fulltext");
}

std::auto_ptr<SearchDriver> qm::FullTextSearchDriverFactory::createDriver(Document* pDocument,
																		  Account* pAccount,
																		  HWND hwnd,
																		  Profile* pProfile)
{
	std::auto_ptr<SearchDriver> pDriver;
	if (pAccount->isMultiMessageStore())
		pDriver.reset(new FullTextSearchDriver(pAccount, pProfile));
	return pDriver;
}

std::auto_ptr<SearchUI> qm::FullTextSearchDriverFactory::createUI(Account* pAccount,
																  Profile* pProfile)
{
	std::auto_ptr<SearchUI> pUI;
	if (pAccount->isMultiMessageStore())
		pUI.reset(new FullTextSearchUI(pAccount, pProfile));
	return pUI;
}

qm::FullTextSearchDriverFactory::InitializerImpl::InitializerImpl()
{
}

qm::FullTextSearchDriverFactory::InitializerImpl::~InitializerImpl()
{
}

bool qm::FullTextSearchDriverFactory::InitializerImpl::init()
{
	pFactory__ = new FullTextSearchDriverFactory();
	return true;
}

void qm::FullTextSearchDriverFactory::InitializerImpl::term()
{
	delete pFactory__;
	pFactory__ = 0;
}


/****************************************************************************
 *
 * FullTextSearchUtil
 *
 */

wstring_ptr qm::FullTextSearchUtil::replace(const WCHAR* pwsz,
											const WCHAR* pwszFind,
											const WCHAR* pwszReplace)
{
	WCHAR* p = wcsstr(pwsz, pwszFind);
	if (!p)
		return 0;
	
	ConcatW c[] = {
		{ pwsz,					p - pwsz	},
		{ pwszReplace,			-1			},
		{ p + wcslen(pwszFind),	-1			}
	};
	return concat(c, countof(c));
}

#endif // _WIN32_WCE
