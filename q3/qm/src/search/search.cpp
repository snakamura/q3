/*
 * $Id$
 *
 * Copyright(C) 1998-2005 Satoshi Nakamura
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
 * SearchPropertyDataImpl
 *
 */

struct qm::SearchPropertyDataImpl
{
	wstring_ptr wstrCondition_;
	bool bAllFolder_;
	bool bRecursive_;
};


/****************************************************************************
 *
 * SearchPropertyData
 *
 */

qm::SearchPropertyData::SearchPropertyData() :
	pImpl_(0)
{
	pImpl_ = new SearchPropertyDataImpl();
}

qm::SearchPropertyData::~SearchPropertyData()
{
	delete pImpl_;
}

const WCHAR* qm::SearchPropertyData::getCondition() const
{
	return pImpl_->wstrCondition_.get();
}

bool qm::SearchPropertyData::isAllFolder() const
{
	return pImpl_->bAllFolder_;
}

bool qm::SearchPropertyData::isRecursive() const
{
	return pImpl_->bRecursive_;
}

void qm::SearchPropertyData::set(const WCHAR* pwszCondition,
								 bool bAllFolder,
								 bool bRecursive)
{
	pImpl_->wstrCondition_ = allocWString(pwszCondition);
	pImpl_->bAllFolder_ = bAllFolder;
	pImpl_->bRecursive_ = bRecursive;
}


/****************************************************************************
 *
 * SearchPropertyPage
 *
 */

qm::SearchPropertyPage::SearchPropertyPage(HINSTANCE hInst,
										   UINT nId,
										   SearchPropertyData* pData) :
	DefaultPropertyPage(hInst, nId),
	pData_(pData)
{
}

qm::SearchPropertyPage::~SearchPropertyPage()
{
}

LRESULT qm::SearchPropertyPage::onNotify(NMHDR* pnmhdr,
										 bool* pbHandled)
{
	BEGIN_NOTIFY_HANDLER()
		HANDLE_NOTIFY_CODE(PSN_KILLACTIVE, onKillActive)
		HANDLE_NOTIFY_CODE(PSN_SETACTIVE, onSetActive)
	END_NOTIFY_HANDLER()
	return DefaultPropertyPage::onNotify(pnmhdr, pbHandled);
}

LRESULT qm::SearchPropertyPage::onKillActive(NMHDR* pnmhdr,
											 bool* pbHandled)
{
	updateData(pData_);
	return 0;
}

LRESULT qm::SearchPropertyPage::onSetActive(NMHDR* pnmhdr,
											bool* pbHandled)
{
	updateUI(pData_);
	return 0;
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

qm::SearchDriverFactoryImpl::FactoryList::iterator qm::SearchDriverFactoryImpl::getIterator(const WCHAR* pwszName)
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

std::auto_ptr<SearchDriver> qm::SearchDriverFactory::getDriver(const WCHAR* pwszName,
															   Document* pDocument,
															   Account* pAccount,
															   HWND hwnd,
															   Profile* pProfile)
{
	SearchDriverFactoryImpl::FactoryList::iterator it =
		SearchDriverFactoryImpl::getIterator(pwszName);
	if (it == SearchDriverFactoryImpl::listFactory__.end())
		return std::auto_ptr<SearchDriver>(0);
	else
		return (*it).second->createDriver(pDocument, pAccount, hwnd, pProfile);
}

std::auto_ptr<SearchUI> qm::SearchDriverFactory::getUI(const WCHAR* pwszName,
													   Account* pAccount,
													   Profile* pProfile)
{
	SearchDriverFactoryImpl::FactoryList::iterator it =
		SearchDriverFactoryImpl::getIterator(pwszName);
	if (it == SearchDriverFactoryImpl::listFactory__.end())
		return std::auto_ptr<SearchUI>(0);
	else
		return (*it).second->createUI(pAccount, pProfile);
}

void qm::SearchDriverFactory::getNames(NameList* pList)
{
	assert(pList);
	
	pList->resize(SearchDriverFactoryImpl::listFactory__.size());
	std::transform(SearchDriverFactoryImpl::listFactory__.begin(),
		SearchDriverFactoryImpl::listFactory__.end(),
		pList->begin(),
		std::select1st<SearchDriverFactoryImpl::FactoryList::value_type>());
}

void qm::SearchDriverFactory::registerFactory(const WCHAR* pwszName,
											  SearchDriverFactory* pFactory)
{
	assert(pwszName);
	assert(pFactory);
	
	SearchDriverFactoryImpl::listFactory__.push_back(std::make_pair(pwszName, pFactory));
}

void qm::SearchDriverFactory::unregisterFactory(const WCHAR* pwszName)
{
	assert(pwszName);
	
	SearchDriverFactoryImpl::FactoryList::iterator it =
		SearchDriverFactoryImpl::getIterator(pwszName);
	if (it != SearchDriverFactoryImpl::listFactory__.end())
		SearchDriverFactoryImpl::listFactory__.erase(it);
}


/****************************************************************************
 *
 * SearchContext
 *
 */

qm::SearchContext::SearchContext(const WCHAR* pwszCondition,
								 const WCHAR* pwszTargetFolder,
								 bool bRecursive,
								 unsigned int nSecurityMode) :
	bRecursive_(bRecursive),
	nSecurityMode_(nSecurityMode)
{
	wstrCondition_ = (allocWString(pwszCondition));
	if (pwszTargetFolder)
		wstrTargetFolder_ = allocWString(pwszTargetFolder);
}

qm::SearchContext::~SearchContext()
{
}

const WCHAR* qm::SearchContext::getCondition() const
{
	return wstrCondition_.get();
}

const WCHAR* qm::SearchContext::getTargetFolder() const
{
	return wstrTargetFolder_.get();
}

bool qm::SearchContext::isRecursive() const
{
	return bRecursive_;
}

unsigned int qm::SearchContext::getSecurityMode() const
{
	return nSecurityMode_;
}

void qm::SearchContext::getTargetFolders(Account* pAccount,
										 FolderList* pList) const
{
	pAccount->getNormalFolders(wstrTargetFolder_.get(), bRecursive_, pList);
}
