/*
 * $Id$
 *
 * Copyright(C) 1998-2004 Satoshi Nakamura
 * All rights reserved.
 *
 */

#include <qmaccount.h>
#include <qmapplication.h>
#include <qmfilenames.h>
#include <qmfolder.h>
#include <qmsyncfilter.h>

#include <qsconv.h>
#include <qsosutil.h>
#include <qsregex.h>
#include <qsstring.h>

#include <algorithm>
#include <vector>

#include "syncfilter.h"

#pragma warning(disable:4786)

using namespace qm;
using namespace qs;


/****************************************************************************
 *
 * SyncFilterManagerImpl
 *
 */

struct qm::SyncFilterManagerImpl
{
public:
	bool load();
	void clear();

public:
	SyncFilterManager* pThis_;
	SyncFilterManager::FilterSetList listFilterSet_;
	FILETIME ft_;
};

bool qm::SyncFilterManagerImpl::load()
{
	wstring_ptr wstrPath(Application::getApplication().getProfilePath(FileNames::SYNCFILTERS_XML));
	
	W2T(wstrPath.get(), ptszPath);
	AutoHandle hFile(::CreateFile(ptszPath, GENERIC_READ, 0, 0,
		OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0));
	if (hFile.get()) {
		FILETIME ft;
		::GetFileTime(hFile.get(), 0, 0, &ft);
		hFile.close();
		
		if (::CompareFileTime(&ft, &ft_) != 0) {
			clear();
			
			XMLReader reader;
			SyncFilterContentHandler handler(pThis_);
			reader.setContentHandler(&handler);
			if (!reader.parse(wstrPath.get()))
				return false;
			
			ft_ = ft;
		}
	}
	else {
		clear();
	}
	
	return true;
}

void qm::SyncFilterManagerImpl::clear()
{
	std::for_each(listFilterSet_.begin(),
		listFilterSet_.end(), deleter<SyncFilterSet>());
	listFilterSet_.clear();
}


/****************************************************************************
 *
 * SyncFilterManager
 *
 */

qm::SyncFilterManager::SyncFilterManager() :
	pImpl_(0)
{
	pImpl_ = new SyncFilterManagerImpl();
	pImpl_->pThis_ = this;
	
	SYSTEMTIME st;
	::GetSystemTime(&st);
	::SystemTimeToFileTime(&st, &pImpl_->ft_);
}

qm::SyncFilterManager::~SyncFilterManager()
{
	if (pImpl_) {
		pImpl_->clear();
		delete pImpl_;
	}
}

const SyncFilterSet* qm::SyncFilterManager::getFilterSet(const Account* pAccount,
														 const WCHAR* pwszName) const
{
	if (pwszName) {
		if (!pImpl_->load())
			return 0;
		
		for (FilterSetList::const_iterator it = pImpl_->listFilterSet_.begin(); it != pImpl_->listFilterSet_.end(); ++it) {
			if ((*it)->match(pAccount, pwszName))
				return *it;
		}
	}
	
	return 0;
}

void qm::SyncFilterManager::getFilterSets(const Account* pAccount,
										  FilterSetList* pList) const
{
	assert(pList);
	
	if (!pImpl_->load())
		return;
	
	for (FilterSetList::const_iterator it = pImpl_->listFilterSet_.begin(); it != pImpl_->listFilterSet_.end(); ++it) {
		if ((*it)->match(pAccount, 0))
			pList->push_back(*it);
	}
}

void qm::SyncFilterManager::addFilterSet(std::auto_ptr<SyncFilterSet> pFilterSet)
{
	pImpl_->listFilterSet_.push_back(pFilterSet.get());
	pFilterSet.release();
}


/****************************************************************************
 *
 * SyncFilterSetImpl
 *
 */

struct qm::SyncFilterSetImpl
{
public:
	typedef std::vector<SyncFilter*> FilterList;

public:
	std::auto_ptr<RegexPattern> pAccountName_;
	wstring_ptr wstrName_;
	FilterList listFilter_;
};


/****************************************************************************
 *
 * SyncFilterSet
 *
 */

qm::SyncFilterSet::SyncFilterSet(std::auto_ptr<RegexPattern> pAccountName,
								 const WCHAR* pwszName) :
	pImpl_(0)
{
	assert(pwszName);
	
	pImpl_ = new SyncFilterSetImpl();
	pImpl_->pAccountName_ = pAccountName;
	pImpl_->wstrName_ = allocWString(pwszName);
}

qm::SyncFilterSet::~SyncFilterSet()
{
	if (pImpl_) {
		std::for_each(pImpl_->listFilter_.begin(),
			pImpl_->listFilter_.end(), deleter<SyncFilter>());
		delete pImpl_;
	}
}

const WCHAR* qm::SyncFilterSet::getName() const
{
	return pImpl_->wstrName_.get();
}

const SyncFilter* qm::SyncFilterSet::getFilter(SyncFilterCallback* pCallback) const
{
	assert(pCallback);
	
	bool bMatch = false;
	for (SyncFilterSetImpl::FilterList::const_iterator it = pImpl_->listFilter_.begin(); it != pImpl_->listFilter_.end(); ++it) {
		if ((*it)->match(pCallback))
			return *it;
	}
	return 0;
}

bool qm::SyncFilterSet::match(const Account* pAccount,
							  const WCHAR* pwszName) const
{
	assert(pAccount);
	
	if (pImpl_->pAccountName_.get()) {
		if (!pImpl_->pAccountName_->match(pAccount->getName()))
			return false;
	}
	
	if (pwszName)
		return wcscmp(pImpl_->wstrName_.get(), pwszName) == 0;
	else
		return true;
}

void qm::SyncFilterSet::addFilter(std::auto_ptr<SyncFilter> pFilter)
{
	pImpl_->listFilter_.push_back(pFilter.get());
	pFilter.release();
}


/****************************************************************************
 *
 * SyncFilterImpl
 *
 */

struct qm::SyncFilterImpl
{
	std::auto_ptr<RegexPattern> pFolderName_;
	std::auto_ptr<Macro> pMacro_;
	SyncFilter::ActionList listAction_;
};


/****************************************************************************
 *
 * SyncFilter
 *
 */

qm::SyncFilter::SyncFilter(std::auto_ptr<RegexPattern> pFolderName,
						   std::auto_ptr<Macro> pMacro) :
	pImpl_(0)
{
	pImpl_ = new SyncFilterImpl();
	pImpl_->pFolderName_ = pFolderName;
	pImpl_->pMacro_ = pMacro;
}

qm::SyncFilter::~SyncFilter()
{
	if (pImpl_) {
		std::for_each(pImpl_->listAction_.begin(),
			pImpl_->listAction_.end(), deleter<SyncFilterAction>());
		delete pImpl_;
	}
}

bool qm::SyncFilter::match(SyncFilterCallback* pCallback) const
{
	assert(pCallback);
	
	const NormalFolder* pFolder = pCallback->getFolder();
	if (pImpl_->pFolderName_.get()) {
		wstring_ptr wstrName(pFolder->getFullName());
		if (!pImpl_->pFolderName_->match(wstrName.get()))
			return false;
	}
	
	if (pImpl_->pMacro_.get()) {
		Lock<Account> lock(*pFolder->getAccount());
		std::auto_ptr<MacroContext> pContext = pCallback->getMacroContext();
		if (!pContext.get())
			return false;
		MacroValuePtr pValue(pImpl_->pMacro_->value(pContext.get()));
		if (!pValue.get())
			return false;
		if (!pValue->boolean())
			return false;
	}
	
	return true;
}

const SyncFilter::ActionList& qm::SyncFilter::getActions() const
{
	return pImpl_->listAction_;
}

void qm::SyncFilter::addAction(std::auto_ptr<SyncFilterAction> pAction)
{
	pImpl_->listAction_.push_back(pAction.get());
	pAction.release();
}


/****************************************************************************
 *
 * SyncFilterCallback
 *
 */

qm::SyncFilterCallback::~SyncFilterCallback()
{
}


/****************************************************************************
 *
 * SyncFilterActionImpl
 *
 */

struct qm::SyncFilterActionImpl
{
public:
	typedef std::vector<std::pair<WSTRING, WSTRING> > ParamList;

public:
	wstring_ptr wstrName_;
	ParamList listParam_;
};


/****************************************************************************
 *
 * SyncFilterAction
 *
 */

qm::SyncFilterAction::SyncFilterAction(const WCHAR* pwszName) :
	pImpl_(0)
{
	pImpl_ = new SyncFilterActionImpl();
	pImpl_->wstrName_ = allocWString(pwszName);
}

qm::SyncFilterAction::~SyncFilterAction()
{
	if (pImpl_) {
		std::for_each(pImpl_->listParam_.begin(), pImpl_->listParam_.end(),
			unary_compose_fx_gx(string_free<WSTRING>(), string_free<WSTRING>()));
		delete pImpl_;
	}
}

const WCHAR* qm::SyncFilterAction::getName() const
{
	return pImpl_->wstrName_.get();
}

const WCHAR* qm::SyncFilterAction::getParam(const WCHAR* pwszName) const
{
	SyncFilterActionImpl::ParamList::const_iterator it = std::find_if(
		pImpl_->listParam_.begin(), pImpl_->listParam_.end(),
		std::bind2nd(
			binary_compose_f_gx_hy(
				string_equal<WCHAR>(),
				std::select1st<SyncFilterActionImpl::ParamList::value_type>(),
				std::identity<const WCHAR*>()),
			pwszName));
	return it != pImpl_->listParam_.end() ? (*it).second : 0;
}

void qm::SyncFilterAction::addParam(wstring_ptr wstrName,
									wstring_ptr wstrValue)
{
	pImpl_->listParam_.push_back(std::make_pair(wstrName.get(), wstrValue.get()));
	wstrName.release();
	wstrValue.release();
}


/****************************************************************************
 *
 * SyncFilterContentHandler
 *
 */

qm::SyncFilterContentHandler::SyncFilterContentHandler(SyncFilterManager* pManager) :
	pManager_(pManager),
	state_(STATE_ROOT),
	pCurrentFilterSet_(0),
	pCurrentFilter_(0),
	pCurrentAction_(0)
{
	pParser_.reset(new MacroParser(MacroParser::TYPE_SYNCFILTER));
}

qm::SyncFilterContentHandler::~SyncFilterContentHandler()
{
}

bool qm::SyncFilterContentHandler::startElement(const WCHAR* pwszNamespaceURI,
												const WCHAR* pwszLocalName,
												const WCHAR* pwszQName,
												const Attributes& attributes)
{
	if (wcscmp(pwszLocalName, L"filters") == 0) {
		if (state_ != STATE_ROOT)
			return false;
		if (attributes.getLength() != 0)
			return false;
		state_ = STATE_FILTERS;
	}
	else if (wcscmp(pwszLocalName, L"filterSet") == 0) {
		if (state_ != STATE_FILTERS)
			return false;
		
		const WCHAR* pwszAccount = 0;
		const WCHAR* pwszName = 0;
		for (int n = 0; n < attributes.getLength(); ++n) {
			const WCHAR* pwszAttrName = attributes.getLocalName(n);
			if (wcscmp(pwszAttrName, L"account") == 0)
				pwszAccount = attributes.getValue(n);
			else if (wcscmp(pwszAttrName, L"name") == 0)
				pwszName = attributes.getValue(n);
			else
				return false;
		}
		if (!pwszName)
			return false;
		
		std::auto_ptr<RegexPattern> pAccountName(RegexCompiler().compile(pwszAccount));
		if (!pAccountName.get())
			return false;
		
		std::auto_ptr<SyncFilterSet> pSet(new SyncFilterSet(pAccountName, pwszName));
		pCurrentFilterSet_ = pSet.get();
		pManager_->addFilterSet(pSet);
		
		state_ = STATE_FILTERSET;
	}
	else if (wcscmp(pwszLocalName, L"filter") == 0) {
		if (state_ != STATE_FILTERSET)
			return false;
		
		const WCHAR* pwszFolder = 0;
		const WCHAR* pwszMatch = 0;
		for (int n = 0; n < attributes.getLength(); ++n) {
			const WCHAR* pwszAttrName = attributes.getLocalName(n);
			if (wcscmp(pwszAttrName, L"folder") == 0)
				pwszFolder = attributes.getValue(n);
			else if (wcscmp(pwszAttrName, L"match") == 0)
				pwszMatch = attributes.getValue(n);
			else
				return false;
		}
		
		
		std::auto_ptr<RegexPattern> pFolderName;
		if (pwszFolder) {
			pFolderName = RegexCompiler().compile(pwszFolder);
			if (!pFolderName.get())
				return false;
		}
		
		std::auto_ptr<Macro> pMacro;
		if (pwszMatch) {
			pMacro = pParser_->parse(pwszMatch);
			if (!pMacro.get())
				return false;
		}
		
		std::auto_ptr<SyncFilter> pFilter(new SyncFilter(pFolderName, pMacro));
		assert(pCurrentFilterSet_);
		pCurrentFilter_ = pFilter.get();
		pCurrentFilterSet_->addFilter(pFilter);
		
		state_ = STATE_FILTER;
	}
	else if (wcscmp(pwszLocalName, L"action") == 0) {
		if (state_ != STATE_FILTER)
			return false;
		
		const WCHAR* pwszName = 0;
		for (int n = 0; n < attributes.getLength(); ++n) {
			const WCHAR* pwszAttrName = attributes.getLocalName(n);
			if (wcscmp(pwszAttrName, L"name") == 0)
				pwszName = attributes.getValue(n);
			else
				return false;
		}
		if (!pwszName)
			return false;
		
		std::auto_ptr<SyncFilterAction> pAction(new SyncFilterAction(pwszName));
		pCurrentAction_ = pAction.get();
		pCurrentFilter_->addAction(pAction);
		
		state_ = STATE_ACTION;
	}
	else if (wcscmp(pwszLocalName, L"param") == 0) {
		if (state_ != STATE_ACTION)
			return false;
		
		const WCHAR* pwszName = 0;
		for (int n = 0; n < attributes.getLength(); ++n) {
			const WCHAR* pwszAttrName = attributes.getLocalName(n);
			if (wcscmp(pwszAttrName, L"name") == 0)
				pwszName = attributes.getValue(n);
			else
				return false;
		}
		if (!pwszName)
			return false;
		
		assert(!wstrCurrentParamName_.get());
		wstrCurrentParamName_ = allocWString(pwszName);
		
		state_ = STATE_PARAM;
	}
	else {
		return false;
	}
	
	return true;
}

bool qm::SyncFilterContentHandler::endElement(const WCHAR* pwszNamespaceURI,
											  const WCHAR* pwszLocalName,
											  const WCHAR* pwszQName)
{
	if (wcscmp(pwszLocalName, L"filters") == 0) {
		assert(state_ == STATE_FILTERS);
		state_ = STATE_ROOT;
	}
	else if (wcscmp(pwszLocalName, L"filterSet") == 0) {
		assert(state_ == STATE_FILTERSET);
		assert(pCurrentFilterSet_);
		pCurrentFilterSet_ = 0;
		state_ = STATE_FILTERS;
	}
	else if (wcscmp(pwszLocalName, L"filter") == 0) {
		assert(state_ == STATE_FILTER);
		assert(pCurrentFilter_);
		
		if (pCurrentFilter_->getActions().empty())
			return false;
		pCurrentFilter_ = 0;
		
		state_ = STATE_FILTERSET;
	}
	else if (wcscmp(pwszLocalName, L"action") == 0) {
		assert(state_ == STATE_ACTION);
		assert(pCurrentAction_);
		pCurrentAction_ = 0;
		state_ = STATE_FILTER;
	}
	else if (wcscmp(pwszLocalName, L"param") == 0) {
		assert(state_ == STATE_PARAM);
		assert(pCurrentAction_);
		assert(wstrCurrentParamName_.get());
		pCurrentAction_->addParam(wstrCurrentParamName_, buffer_.getString());
		state_ = STATE_ACTION;
	}
	else {
		return false;
	}
	
	return true;
}

bool qm::SyncFilterContentHandler::characters(const WCHAR* pwsz,
											  size_t nStart,
											  size_t nLength)
{
	if (state_ == STATE_PARAM) {
		buffer_.append(pwsz + nStart, nLength);
	}
	else {
		const WCHAR* p = pwsz + nStart;
		for (size_t n = 0; n < nLength; ++n, ++p) {
			if (*p != L' ' && *p != L'\t' && *p != '\n')
				return false;
		}
	}
	
	return true;
}
