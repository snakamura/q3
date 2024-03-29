/*
 * $Id$
 *
 * Copyright(C) 1998-2008 Satoshi Nakamura
 *
 */

#pragma warning(disable:4786)

#include <qmaccount.h>
#include <qmfolder.h>
#include <qmuiutil.h>

#include <qsfile.h>
#include <qsinit.h>
#include <qsosutil.h>
#include <qsstl.h>
#include <qstextutil.h>
#include <qsuiutil.h>

#include "fulltextsearch.h"
#include "../main/main.h"
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
	wstring_ptr wstrCommand(pProfile_->getString(L"FullTextSearch", L"Command"));
	
	wstring_ptr wstrIndex(pAccount_->getPropertyString(L"FullTextSearch", L"Index"));
	if (!*wstrIndex.get())
		wstrIndex = concat(pAccount_->getPath(), L"\\index");
	wstrCommand = TextUtil::replaceAll(wstrCommand.get(), L"$index", wstrIndex.get());
	wstrCommand = TextUtil::replaceAll(wstrCommand.get(),
		L"$encoding", Init::getInit().getSystemEncoding());
	wstrCommand = TextUtil::replaceAll(wstrCommand.get(),
		L"$condition", escapeQuote(context.getCondition()).get());
	
	wstring_ptr wstrOutput(Process::exec(wstrCommand.get(), 0));
	if (!wstrOutput.get())
		return false;
	
	typedef std::vector<unsigned int> OffsetList;
	OffsetList listOffset;
	
	WCHAR* p = wstrOutput.get();
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
		boost::bind(&MessageHolder::MessageBoxKey::nOffset_,
			boost::bind(&MessageHolder::getMessageBoxKey, _1)) <
		boost::bind(&MessageHolder::MessageBoxKey::nOffset_,
			boost::bind(&MessageHolder::getMessageBoxKey, _2)));
	
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
			boost::bind(&MessageHolder::MessageBoxKey::nOffset_,
				boost::bind(&MessageHolder::getMessageBoxKey, _1)) <
			boost::bind(&MessageHolder::MessageBoxKey::nOffset_,
				boost::bind(&MessageHolder::getMessageBoxKey, _2)));
		if (itM != listMessageHolder.end() &&
			(*itM)->getMessageBoxKey().nOffset_ == nOffset)
			pList->push_back(*itM);
		
		nPrevOffset = nOffset;
	}
	
	return true;
}

wstring_ptr qm::FullTextSearchDriver::escapeQuote(const WCHAR* pwsz)
{
	StringBuffer<WSTRING> buf;
	while (*pwsz) {
		if (*pwsz == L'\"')
			buf.append(L'\\');
		buf.append(*pwsz);
		++pwsz;
	}
	return buf.getString();
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
	return loadString(getResourceHandle(), IDS_TITLE_FULLTEXTSEARCH);
}

std::auto_ptr<SearchPropertyPage> qm::FullTextSearchUI::createPropertyPage(SearchPropertyData* pData)
{
	return std::auto_ptr<SearchPropertyPage>(
		new FullTextSearchPage(pAccount_, pProfile_, pData));
}


/****************************************************************************
 *
 * FullTextSearchPage
 *
 */

qm::FullTextSearchPage::FullTextSearchPage(Account* pAccount,
										   Profile* pProfile,
										   SearchPropertyData* pData) :
	SearchPropertyPage(getResourceHandle(), IDD_FULLTEXTSEARCH, IDD_FULLTEXTSEARCH,
		IDC_CONDITION, IDC_FOLDER, IDC_RECURSIVE, IDC_NEWFOLDER, pData),
	pAccount_(pAccount),
	pProfile_(pProfile)
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
	SearchPropertyPage::onInitDialog(hwndFocus, lParam);
	
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
	
	return TRUE;
}

LRESULT qm::FullTextSearchPage::onOk()
{
	if (PropSheet_GetCurrentPageHwnd(getSheet()->getHandle()) == getHandle()) {
		wstring_ptr wstrSearch = getDlgItemText(IDC_CONDITION);
		if (*wstrSearch.get()) {
			History(pProfile_, L"Search").addValue(wstrSearch.get());
			wstrCondition_ = wstrSearch;
		}
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
	wstring_ptr wstrCommand(pProfile_->getString(L"FullTextSearch", L"IndexCommand"));
	
	wstring_ptr wstrIndex(pAccount_->getPropertyString(L"FullTextSearch", L"Index"));
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
	wstrCommand = TextUtil::replaceAll(wstrCommand.get(), L"$index", wstrIndex.get());
	wstring_ptr wstrMsg(concat(pAccount_->getMessageStorePath(), L"\\msg"));
	wstrCommand = TextUtil::replaceAll(wstrCommand.get(), L"$msg", wstrMsg.get());
	wstrCommand = TextUtil::replaceAll(wstrCommand.get(), L"$encoding", Init::getInit().getSystemEncoding());
	
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
																		  ActionInvoker* pActionInvoker,
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

#endif // _WIN32_WCE
