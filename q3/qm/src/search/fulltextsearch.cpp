/*
 * $Id$
 *
 * Copyright(C) 1998-2003 Satoshi Nakamura
 * All rights reserved.
 *
 */

#include <qmaccount.h>
#include <qmapplication.h>
#include <qmfolder.h>

#include <qsnew.h>
#include <qsosutil.h>
#include <qsstl.h>

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

qm::FullTextSearchDriver::FullTextSearchDriver(
	Account* pAccount, Profile* pProfile, QSTATUS* pstatus) :
	pAccount_(pAccount),
	pProfile_(pProfile)
{
}

qm::FullTextSearchDriver::~FullTextSearchDriver()
{
}

QSTATUS qm::FullTextSearchDriver::search(
	const SearchContext& context, MessageHolderList* pList)
{
	DECLARE_QSTATUS();
	
	string_ptr<WSTRING> wstrCommand;
	status = pProfile_->getString(L"FullTextSearch", L"Command",
		L"namazu -l \"$condition\" \"$index\"", &wstrCommand);
	CHECK_QSTATUS();
	
	string_ptr<WSTRING> wstr;
	status = FullTextSearchUtil::replace(wstrCommand.get(),
		L"$condition", context.getCondition(), &wstr);
	CHECK_QSTATUS();
	if (!wstr.get())
		return QSTATUS_SUCCESS;
	wstrCommand.reset(wstr.release());
	
	string_ptr<WSTRING> wstrIndex;
	status = pAccount_->getProperty(L"FullTextSearch", L"Index", L"", &wstrIndex);
	CHECK_QSTATUS();
	if (!*wstrIndex.get()) {
		wstrIndex.reset(concat(pAccount_->getPath(), L"\\index"));
		if (!wstrIndex.get())
			return QSTATUS_OUTOFMEMORY;
	}
	status = FullTextSearchUtil::replace(wstrCommand.get(),
		L"$index", wstrIndex.get(), &wstr);
	CHECK_QSTATUS();
	if (!wstr.get())
		return QSTATUS_SUCCESS;
	wstrCommand.reset(wstr.release());
	
	string_ptr<WSTRING> wstrOutput;
	status = Process::exec(wstrCommand.get(), 0, &wstrOutput);
	CHECK_QSTATUS();
	
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
			if (*pp == L'.') {
				status = STLWrapper<OffsetList>(listOffset).push_back(n);
				CHECK_QSTATUS();
			}
		}
		
		p = pEnd + 1;
		if (*p == L'\n')
			++p;
	}
	if (listOffset.empty())
		return QSTATUS_SUCCESS;
	
	SearchContext::FolderList listFolder;
	status = context.getTargetFolders(pAccount_, &listFolder);
	CHECK_QSTATUS();
	
	MessageHolderList listMessageHolder;
	SearchContext::FolderList::const_iterator itF = listFolder.begin();
	while (itF != listFolder.end()) {
		NormalFolder* pFolder = *itF;
		
		status = pFolder->loadMessageHolders();
		CHECK_QSTATUS();
		
		const MessageHolderList* p;
		status = pFolder->getMessages(&p);
		CHECK_QSTATUS();
		status = STLWrapper<MessageHolderList>(listMessageHolder).reserve(
			listMessageHolder.size() + p->size());
		CHECK_QSTATUS();
		std::copy(p->begin(), p->end(), std::back_inserter(listMessageHolder));
		
		++itF;
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
	
	status = STLWrapper<MessageHolderList>(*pList).reserve(listOffset.size());
	CHECK_QSTATUS();
	
	OffsetList::const_iterator itO = listOffset.begin();
	while (itO != listOffset.end()) {
		unsigned int nOffset = *itO;
		
		MessageHolder::Init init = { 0 };
		init.nOffset_ = nOffset;
		
		MessageHolder mh(0, init, &status);
		CHECK_QSTATUS();
		
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
		
		++itO;
	}
	
	return QSTATUS_SUCCESS;
}


/****************************************************************************
 *
 * FullTextSearchUI
 *
 */

qm::FullTextSearchUI::FullTextSearchUI(Account* pAccount,
	Profile* pProfile, QSTATUS* pstatus) :
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

QSTATUS qm::FullTextSearchUI::getDisplayName(qs::WSTRING* pwstrName)
{
	return loadString(Application::getApplication().getResourceHandle(),
		IDS_FULLTEXTSEARCH, pwstrName);
}

QSTATUS qm::FullTextSearchUI::createPropertyPage(
	bool bAllFolder, SearchPropertyPage** ppPage)
{
	DECLARE_QSTATUS();
	
	std::auto_ptr<FullTextSearchPage> pPage;
	status = newQsObject(pAccount_, pProfile_, bAllFolder, &pPage);
	CHECK_QSTATUS();
	*ppPage = pPage.release();
	
	return QSTATUS_SUCCESS;
}


/****************************************************************************
 *
 * FullTextSearchPage
 *
 */

qm::FullTextSearchPage::FullTextSearchPage(Account* pAccount,
	Profile* pProfile, bool bAllFolder, QSTATUS* pstatus) :
	SearchPropertyPage(Application::getApplication().getResourceHandle(),
		IDD_FULLTEXTSEARCH, pstatus),
	pAccount_(pAccount),
	pProfile_(pProfile),
	wstrCondition_(0),
	bAllFolder_(bAllFolder),
	bRecursive_(false)
{
}

qm::FullTextSearchPage::~FullTextSearchPage()
{
	freeWString(wstrCondition_);
}

const WCHAR* qm::FullTextSearchPage::getDriver() const
{
	return L"fulltext";
}

const WCHAR* qm::FullTextSearchPage::getCondition() const
{
	return wstrCondition_;
}

bool qm::FullTextSearchPage::isAllFolder() const
{
	return bAllFolder_;
}

bool qm::FullTextSearchPage::isRecursive() const
{
	return bRecursive_;
}

LRESULT qm::FullTextSearchPage::onCommand(WORD nCode, WORD nId)
{
	BEGIN_COMMAND_HANDLER()
		HANDLE_COMMAND_ID(IDC_UPDATEINDEX, onUpdateIndex)
	END_COMMAND_HANDLER()
	return SearchPropertyPage::onCommand(nCode, nId);
}

LRESULT qm::FullTextSearchPage::onInitDialog(HWND hwndFocus, LPARAM lParam)
{
	DECLARE_QSTATUS();
	
	string_ptr<WSTRING> wstrCondition;
	status = pProfile_->getString(L"Search", L"Condition", L"", &wstrCondition);
	CHECK_QSTATUS();
	setDlgItemText(IDC_CONDITION, wstrCondition.get());
	
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
	
	return TRUE;
}

LRESULT qm::FullTextSearchPage::onOk()
{
	if (PropSheet_GetCurrentPageHwnd(getSheet()->getHandle()) == getHandle()) {
		wstrCondition_ = getDlgItemText(IDC_CONDITION);
		bAllFolder_ = sendDlgItemMessage(IDC_ALLFOLDER, BM_GETCHECK) == BST_CHECKED;
		bRecursive_ = sendDlgItemMessage(IDC_RECURSIVE, BM_GETCHECK) == BST_CHECKED;
		
		pProfile_->setString(L"Search", L"Condition", wstrCondition_);
		pProfile_->setInt(L"Search", L"Folder",
			bAllFolder_ ? 2 : bRecursive_ ? 1 : 0);
	}
	return SearchPropertyPage::onOk();
}

LRESULT qm::FullTextSearchPage::onUpdateIndex()
{
	updateIndex();
	return 0;
}

QSTATUS qm::FullTextSearchPage::updateIndex()
{
	DECLARE_QSTATUS();
	
	string_ptr<WSTRING> wstrCommand;
	status = pProfile_->getString(L"FullTextSearch", L"IndexCommand",
		L"mknmz.bat -a -h -O \"$index\" \"$msg\"", &wstrCommand);
	CHECK_QSTATUS();
	
	string_ptr<WSTRING> wstr;
	string_ptr<WSTRING> wstrIndex;
	status = pAccount_->getProperty(L"FullTextSearch", L"Index", L"", &wstrIndex);
	CHECK_QSTATUS();
	if (!*wstrIndex.get()) {
		wstrIndex.reset(concat(pAccount_->getPath(), L"\\index"));
		if (!wstrIndex.get())
			return QSTATUS_OUTOFMEMORY;
	}
	status = FullTextSearchUtil::replace(wstrCommand.get(),
		L"$index", wstrIndex.get(), &wstr);
	CHECK_QSTATUS();
	if (!wstr.get())
		return QSTATUS_SUCCESS;
	wstrCommand.reset(wstr.release());
	
	string_ptr<WSTRING> wstrMsg(concat(pAccount_->getMessageStorePath(), L"\\msg"));
	if (!*wstrMsg.get())
		return QSTATUS_OUTOFMEMORY;
	status = FullTextSearchUtil::replace(
		wstrCommand.get(), L"$msg", wstrMsg.get(), &wstr);
	CHECK_QSTATUS();
	if (!wstr.get())
		return QSTATUS_SUCCESS;
	wstrCommand.reset(wstr.release());
	
	W2T(wstrIndex.get(), ptszIndex);
	::CreateDirectory(ptszIndex, 0);
	
	W2T(wstrCommand.get(), ptszCommand);
	STARTUPINFO si = { sizeof(si) };
	PROCESS_INFORMATION pi;
	if (!::CreateProcess(0, const_cast<LPTSTR>(ptszCommand),
		0, 0, TRUE, 0, 0, 0, &si, &pi))
		return QSTATUS_FAIL;
	::CloseHandle(pi.hThread);
	::WaitForSingleObject(pi.hProcess, INFINITE);
	::CloseHandle(pi.hProcess);
	
	return QSTATUS_SUCCESS;
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
	regist(L"fulltext", this);
}

qm::FullTextSearchDriverFactory::~FullTextSearchDriverFactory()
{
	unregist(L"fulltext");
}

QSTATUS qm::FullTextSearchDriverFactory::createDriver(Document* pDocument,
	Account* pAccount, HWND hwnd, Profile* pProfile, SearchDriver** ppDriver)
{
	DECLARE_QSTATUS();
	
	if (pAccount->isMultiMessageStore()) {
		std::auto_ptr<FullTextSearchDriver> pDriver;
		status = newQsObject(pAccount, pProfile, &pDriver);
		CHECK_QSTATUS();
		*ppDriver = pDriver.release();
	}
	
	return QSTATUS_SUCCESS;
}

QSTATUS qm::FullTextSearchDriverFactory::createUI(
	Account* pAccount, Profile* pProfile, SearchUI** ppUI)
{
	DECLARE_QSTATUS();
	
	if (pAccount->isMultiMessageStore()) {
		std::auto_ptr<FullTextSearchUI> pUI;
		status = newQsObject(pAccount, pProfile, &pUI);
		CHECK_QSTATUS();
		*ppUI = pUI.release();
	}
	
	return QSTATUS_SUCCESS;
}

qm::FullTextSearchDriverFactory::InitializerImpl::InitializerImpl()
{
}

qm::FullTextSearchDriverFactory::InitializerImpl::~InitializerImpl()
{
}

QSTATUS qm::FullTextSearchDriverFactory::InitializerImpl::init()
{
	DECLARE_QSTATUS();
	
	status = newObject(&pFactory__);
	CHECK_QSTATUS();
	
	return QSTATUS_SUCCESS;
}

QSTATUS qm::FullTextSearchDriverFactory::InitializerImpl::term()
{
	delete pFactory__;
	pFactory__ = 0;
	
	return QSTATUS_SUCCESS;
}


/****************************************************************************
 *
 * FullTextSearchUtil
 *
 */

QSTATUS qm::FullTextSearchUtil::replace(const WCHAR* pwsz,
	const WCHAR* pwszFind, const WCHAR* pwszReplace, qs::WSTRING* pwstr)
{
	assert(pwstr);
	
	*pwstr = 0;
	
	WCHAR* p = wcsstr(pwsz, pwszFind);
	if (!p)
		return QSTATUS_SUCCESS;
	
	ConcatW c[] = {
		{ pwsz,					p - pwsz	},
		{ pwszReplace,			-1			},
		{ p + wcslen(pwszFind),	-1			}
	};
	*pwstr = concat(c, countof(c));
	if (!*pwstr)
		return QSTATUS_OUTOFMEMORY;
	
	return QSTATUS_SUCCESS;
}

#endif // _WIN32_WCE
