/*
 * $Id$
 *
 * Copyright(C) 1998-2003 Satoshi Nakamura
 * All rights reserved.
 *
 */

#include <qmaccount.h>
#include <qmfolder.h>
#include <qmsearch.h>

#include <qsstl.h>

using namespace qm;
using namespace qs;


namespace qm {
struct SearchDriverFactoryImpl;
}

/****************************************************************************
 *
 * SearchDriver
 *
 */

qm::SearchDriver::~SearchDriver()
{
}


/****************************************************************************
 *
 * SearchUI
 *
 */

qm::SearchUI::~SearchUI()
{
}


/****************************************************************************
 *
 * SearchPropertyPage
 *
 */

qm::SearchPropertyPage::SearchPropertyPage(HINSTANCE hInst, UINT nId, QSTATUS* pstatus) :
	DefaultPropertyPage(hInst, nId, pstatus)
{
}

qm::SearchPropertyPage::~SearchPropertyPage()
{
}


/****************************************************************************
 *
 * SearchDriverFactoryImpl
 *
 */

struct qm::SearchDriverFactoryImpl
{
	typedef std::vector<std::pair<const WCHAR*, SearchDriverFactory*> > FactoryList;
	
	static FactoryList::iterator getIterator(const WCHAR* pwszName);
	
	static FactoryList listFactory__;
};

qm::SearchDriverFactoryImpl::FactoryList qm::SearchDriverFactoryImpl::listFactory__;

qm::SearchDriverFactoryImpl::FactoryList::iterator
	qm::SearchDriverFactoryImpl::getIterator(const WCHAR* pwszName)
{
	return std::find_if(listFactory__.begin(), listFactory__.end(),
		std::bind2nd(
			binary_compose_f_gx_hy(
				string_equal<WCHAR>(),
				std::select1st<FactoryList::value_type>(),
				std::identity<const WCHAR*>()),
			pwszName));
}


/****************************************************************************
 *
 * SearchDriverFactory
 *
 */

qm::SearchDriverFactory::SearchDriverFactory()
{
}

qm::SearchDriverFactory::~SearchDriverFactory()
{
}

QSTATUS qm::SearchDriverFactory::getDriver(const WCHAR* pwszName,
	Document* pDocument, Account* pAccount, HWND hwnd,
	Profile* pProfile, SearchDriver** ppDriver)
{
	assert(ppDriver);
	
	*ppDriver = 0;
	
	SearchDriverFactoryImpl::FactoryList::iterator it =
		SearchDriverFactoryImpl::getIterator(pwszName);
	if (it == SearchDriverFactoryImpl::listFactory__.end())
		return QSTATUS_FAIL;
	
	return (*it).second->createDriver(pDocument, pAccount, hwnd, pProfile, ppDriver);
}

QSTATUS qm::SearchDriverFactory::getDriver(const WCHAR* pwszName,
	Document* pDocument, Account* pAccount, HWND hwnd,
	Profile* pProfile, std::auto_ptr<SearchDriver>* papDriver)
{
	assert(papDriver);
	
	DECLARE_QSTATUS();
	
	SearchDriver* p = 0;
	status = getDriver(pwszName, pDocument, pAccount, hwnd, pProfile, &p);
	CHECK_QSTATUS();
	papDriver->reset(p);
	
	return QSTATUS_SUCCESS;
}

QSTATUS qm::SearchDriverFactory::getUI(const WCHAR* pwszName,
	Account* pAccount, Profile* pProfile, SearchUI** ppUI)
{
	assert(ppUI);
	
	*ppUI = 0;
	
	SearchDriverFactoryImpl::FactoryList::iterator it =
		SearchDriverFactoryImpl::getIterator(pwszName);
	if (it == SearchDriverFactoryImpl::listFactory__.end())
		return QSTATUS_FAIL;
	
	return (*it).second->createUI(pAccount, pProfile, ppUI);
}

QSTATUS qm::SearchDriverFactory::getUI(const WCHAR* pwszName,
	Account* pAccount, Profile* pProfile, std::auto_ptr<SearchUI>* papUI)
{
	assert(papUI);
	
	DECLARE_QSTATUS();
	
	SearchUI* p = 0;
	status = getUI(pwszName, pAccount, pProfile, &p);
	CHECK_QSTATUS();
	papUI->reset(p);
	
	return QSTATUS_SUCCESS;
}

QSTATUS qm::SearchDriverFactory::getNames(NameList* pList)
{
	assert(pList);
	
	DECLARE_QSTATUS();
	
	status = STLWrapper<NameList>(*pList).resize(
		SearchDriverFactoryImpl::listFactory__.size());
	CHECK_QSTATUS();
	std::transform(SearchDriverFactoryImpl::listFactory__.begin(),
		SearchDriverFactoryImpl::listFactory__.end(),
		pList->begin(),
		std::select1st<SearchDriverFactoryImpl::FactoryList::value_type>());
	
	return QSTATUS_SUCCESS;
}

QSTATUS qm::SearchDriverFactory::regist(const WCHAR* pwszName,
	SearchDriverFactory* pFactory)
{
	assert(pwszName);
	assert(pFactory);
	
	return STLWrapper<SearchDriverFactoryImpl::FactoryList>(
		SearchDriverFactoryImpl::listFactory__).push_back(
			std::make_pair(pwszName, pFactory));
}

QSTATUS qm::SearchDriverFactory::unregist(const WCHAR* pwszName)
{
	assert(pwszName);
	
	SearchDriverFactoryImpl::FactoryList::iterator it =
		SearchDriverFactoryImpl::getIterator(pwszName);
	if (it != SearchDriverFactoryImpl::listFactory__.end())
		SearchDriverFactoryImpl::listFactory__.erase(it);
	
	return QSTATUS_SUCCESS;
}


/****************************************************************************
 *
 * SearchContext
 *
 */

qm::SearchContext::SearchContext(const WCHAR* pwszCondition,
	const WCHAR* pwszTargetFolder, bool bRecursive, QSTATUS* pstatus) :
	wstrCondition_(0),
	wstrTargetFolder_(0),
	bRecursive_(bRecursive)
{
	string_ptr<WSTRING> wstrCondition(allocWString(pwszCondition));
	if (!wstrCondition.get()) {
		*pstatus = QSTATUS_OUTOFMEMORY;
		return;
	}
	
	string_ptr<WSTRING> wstrTargetFolder;
	if (pwszTargetFolder) {
		wstrTargetFolder.reset(allocWString(pwszTargetFolder));
		if (!wstrTargetFolder.get()) {
			*pstatus = QSTATUS_OUTOFMEMORY;
			return;
		}
	}
	
	wstrCondition_ = wstrCondition.release();
	wstrTargetFolder_ = wstrTargetFolder.release();
}

qm::SearchContext::~SearchContext()
{
	freeWString(wstrCondition_);
	freeWString(wstrTargetFolder_);
}

const WCHAR* qm::SearchContext::getCondition() const
{
	return wstrCondition_;
}

const WCHAR* qm::SearchContext::getTargetFolder() const
{
	return wstrTargetFolder_;
}

bool qm::SearchContext::isRecursive() const
{
	return bRecursive_;
}

QSTATUS qm::SearchContext::getTargetFolders(
	Account* pAccount, FolderList* pList) const
{
	assert(pAccount);
	assert(pList);
	
	DECLARE_QSTATUS();
	
	Folder* pTargetFolder = 0;
	if (wstrTargetFolder_) {
		status = pAccount->getFolder(wstrTargetFolder_, &pTargetFolder);
		CHECK_QSTATUS();
	}
	
	if (pTargetFolder) {
		if (bRecursive_) {
			const Account::FolderList& l = pAccount->getFolders();
			Account::FolderList::const_iterator it = l.begin();
			while (it != l.end()) {
				Folder* pFolder = *it;
				
				if (pFolder->getType() == Folder::TYPE_NORMAL &&
					!pFolder->isHidden() &&
					(pFolder == pTargetFolder ||
					pTargetFolder->isAncestorOf(pFolder))) {
					status = STLWrapper<FolderList>(*pList).push_back(
						static_cast<NormalFolder*>(pFolder));
					CHECK_QSTATUS();
				}
				
				++it;
			}
		}
		else {
			if (pTargetFolder->getType() == Folder::TYPE_NORMAL) {
				status = STLWrapper<FolderList>(*pList).push_back(
					static_cast<NormalFolder*>(pTargetFolder));
				CHECK_QSTATUS();
			}
		}
	}
	else {
		const Account::FolderList& l = pAccount->getFolders();
		Account::FolderList::const_iterator it = l.begin();
		while (it != l.end()) {
			Folder* pFolder = *it;
			if (pFolder->getType() == Folder::TYPE_NORMAL &&
				!pFolder->isFlag(Folder::FLAG_TRASHBOX)) {
				status = STLWrapper<FolderList>(*pList).push_back(
					static_cast<NormalFolder*>(pFolder));
				CHECK_QSTATUS();
			}
			++it;
		}
	}
	
	return QSTATUS_SUCCESS;
}
