/*
 * $Id$
 *
 * Copyright(C) 1998-2004 Satoshi Nakamura
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

qm::SearchPropertyPage::SearchPropertyPage(HINSTANCE hInst,
										   UINT nId) :
	DefaultPropertyPage(hInst, nId)
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
