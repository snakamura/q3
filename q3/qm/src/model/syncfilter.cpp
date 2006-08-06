/*
 * $Id$
 *
 * Copyright(C) 1998-2006 Satoshi Nakamura
 * All rights reserved.
 *
 */

#pragma warning(disable:4786)

#include <qmaccount.h>
#include <qmfolder.h>
#include <qmsyncfilter.h>

#include <qsconv.h>
#include <qsosutil.h>
#include <qsregex.h>
#include <qsstring.h>

#include <algorithm>
#include <vector>

#include "syncfilter.h"
#include "term.h"
#include "../util/confighelper.h"

using namespace qm;
using namespace qs;


/****************************************************************************
 *
 * SyncFilterManagerImpl
 *
 */

struct qm::SyncFilterManagerImpl
{
	SyncFilterManagerImpl(const WCHAR* pwszPath);
	
	bool load();
	
	SyncFilterManager* pThis_;
	SyncFilterManager::FilterSetList listFilterSet_;
	ConfigHelper<SyncFilterManager, SyncFilterContentHandler, SyncFilterWriter> helper_;
};

qm::SyncFilterManagerImpl::SyncFilterManagerImpl(const WCHAR* pwszPath) :
	helper_(pwszPath)
{
}

bool qm::SyncFilterManagerImpl::load()
{
	SyncFilterContentHandler handler(pThis_);
	return helper_.load(pThis_, &handler);
}


/****************************************************************************
 *
 * SyncFilterManager
 *
 */

qm::SyncFilterManager::SyncFilterManager(const WCHAR* pwszPath) :
	pImpl_(0)
{
	pImpl_ = new SyncFilterManagerImpl(pwszPath);
	pImpl_->pThis_ = this;
}

qm::SyncFilterManager::~SyncFilterManager()
{
	clear();
	delete pImpl_;
}

const SyncFilterManager::FilterSetList& qm::SyncFilterManager::getFilterSets()
{
	return getFilterSets(true);
}

const SyncFilterManager::FilterSetList& qm::SyncFilterManager::getFilterSets(bool bReload)
{
	if (bReload)
		pImpl_->load();
	return pImpl_->listFilterSet_;
}

std::auto_ptr<SyncFilterSet> qm::SyncFilterManager::getFilterSet(const WCHAR* pwszName) const
{
	assert(pwszName);
	
	if (!pImpl_->load())
		return std::auto_ptr<SyncFilterSet>();
	
	FilterSetList::const_iterator it = std::find_if(
		pImpl_->listFilterSet_.begin(), pImpl_->listFilterSet_.end(),
		std::bind2nd(
			binary_compose_f_gx_hy(
				string_equal<WCHAR>(),
				std::mem_fun(&SyncFilterSet::getName),
				std::identity<const WCHAR*>()),
			pwszName));
	if (it == pImpl_->listFilterSet_.end())
		return std::auto_ptr<SyncFilterSet>();
	
	return std::auto_ptr<SyncFilterSet>(new SyncFilterSet(**it));
}

void qm::SyncFilterManager::setFilterSets(FilterSetList& listFilterSet)
{
	clear();
	pImpl_->listFilterSet_.swap(listFilterSet);
}

bool qm::SyncFilterManager::save() const
{
	return pImpl_->helper_.save(this);
}

void qm::SyncFilterManager::clear()
{
	std::for_each(pImpl_->listFilterSet_.begin(),
		pImpl_->listFilterSet_.end(), deleter<SyncFilterSet>());
	pImpl_->listFilterSet_.clear();
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
	void clear();
	
	wstring_ptr wstrName_;
	SyncFilterSet::FilterList listFilter_;
};

void qm::SyncFilterSetImpl::clear()
{
	std::for_each(listFilter_.begin(), listFilter_.end(), deleter<SyncFilter>());
	listFilter_.clear();
}


/****************************************************************************
 *
 * SyncFilterSet
 *
 */

qm::SyncFilterSet::SyncFilterSet() :
	pImpl_(0)
{
	pImpl_ = new SyncFilterSetImpl();
	pImpl_->wstrName_ = allocWString(L"");
}

qm::SyncFilterSet::SyncFilterSet(const WCHAR* pwszName) :
	pImpl_(0)
{
	assert(pwszName);
	
	pImpl_ = new SyncFilterSetImpl();
	pImpl_->wstrName_ = allocWString(pwszName);
}

qm::SyncFilterSet::SyncFilterSet(const SyncFilterSet& filterSet) :
	pImpl_(0)
{
	pImpl_ = new SyncFilterSetImpl();
	
	pImpl_->wstrName_ = allocWString(filterSet.pImpl_->wstrName_.get());
	
	const SyncFilterSet::FilterList& l = filterSet.pImpl_->listFilter_;
	pImpl_->listFilter_.resize(l.size());
	for (SyncFilterSet::FilterList::size_type n = 0; n < l.size(); ++n)
		pImpl_->listFilter_[n] = new SyncFilter(*l[n]);
}

qm::SyncFilterSet::~SyncFilterSet()
{
	pImpl_->clear();
	delete pImpl_;
}

const WCHAR* qm::SyncFilterSet::getName() const
{
	return pImpl_->wstrName_.get();
}

void qm::SyncFilterSet::setName(const WCHAR* pwszName)
{
	assert(pwszName);
	pImpl_->wstrName_ = allocWString(pwszName);
}

const SyncFilterSet::FilterList& qm::SyncFilterSet::getFilters() const
{
	return pImpl_->listFilter_;
}

const SyncFilter* qm::SyncFilterSet::getFilter(SyncFilterCallback* pCallback) const
{
	assert(pCallback);
	
	for (SyncFilterSet::FilterList::const_iterator it = pImpl_->listFilter_.begin(); it != pImpl_->listFilter_.end(); ++it) {
		if ((*it)->match(pCallback))
			return *it;
	}
	return 0;
}

void qm::SyncFilterSet::setFilters(FilterList& listFilter)
{
	pImpl_->clear();
	pImpl_->listFilter_.swap(listFilter);
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
	void clear();
	
	Term folder_;
	std::auto_ptr<Macro> pCondition_;
	wstring_ptr wstrDescription_;
	SyncFilter::ActionList listAction_;
};

void qm::SyncFilterImpl::clear()
{
	std::for_each(listAction_.begin(), listAction_.end(), deleter<SyncFilterAction>());
	listAction_.clear();
}


/****************************************************************************
 *
 * SyncFilter
 *
 */

qm::SyncFilter::SyncFilter() :
	pImpl_(0)
{
	pImpl_ = new SyncFilterImpl();
	pImpl_->pCondition_ = MacroParser().parse(L"@True()");
}

qm::SyncFilter::SyncFilter(Term& folder,
						   std::auto_ptr<Macro> pCondition,
						   const WCHAR* pwszDescription) :
	pImpl_(0)
{
	pImpl_ = new SyncFilterImpl();
	pImpl_->folder_.assign(folder);
	pImpl_->pCondition_ = pCondition;
	if (pwszDescription)
		pImpl_->wstrDescription_ = allocWString(pwszDescription);
}

qm::SyncFilter::SyncFilter(const SyncFilter& filter) :
	pImpl_(0)
{
	pImpl_ = new SyncFilterImpl();
	
	pImpl_->folder_ = filter.pImpl_->folder_;
	
	wstring_ptr wstrCondition(filter.pImpl_->pCondition_->getString());
	pImpl_->pCondition_ = MacroParser().parse(wstrCondition.get());
	
	if (filter.pImpl_->wstrDescription_.get())
		pImpl_->wstrDescription_ = allocWString(filter.pImpl_->wstrDescription_.get());
	
	const SyncFilter::ActionList& l = filter.pImpl_->listAction_;
	pImpl_->listAction_.resize(l.size());
	for (SyncFilter::ActionList::size_type n = 0; n < l.size(); ++n)
		pImpl_->listAction_[n] = new SyncFilterAction(*l[n]);
}

qm::SyncFilter::~SyncFilter()
{
	pImpl_->clear();
	delete pImpl_;
}

const WCHAR* qm::SyncFilter::getFolder() const
{
	return pImpl_->folder_.getValue();
}

void qm::SyncFilter::setFolder(Term& folder)
{
	pImpl_->folder_.assign(folder);
}

const Macro* qm::SyncFilter::getCondition() const
{
	return pImpl_->pCondition_.get();
}

void qm::SyncFilter::setCondition(std::auto_ptr<Macro> pCondition)
{
	pImpl_->pCondition_ = pCondition;
}

const WCHAR* qm::SyncFilter::getDescription() const
{
	return pImpl_->wstrDescription_.get();
}

void qm::SyncFilter::setDescription(const WCHAR* pwszDescription)
{
	if (pwszDescription && *pwszDescription)
		pImpl_->wstrDescription_ = allocWString(pwszDescription);
	else
		pImpl_->wstrDescription_.reset(0);
}

const SyncFilter::ActionList& qm::SyncFilter::getActions() const
{
	return pImpl_->listAction_;
}

void qm::SyncFilter::setActions(ActionList& listAction)
{
	pImpl_->clear();
	pImpl_->listAction_.swap(listAction);
}

bool qm::SyncFilter::match(SyncFilterCallback* pCallback) const
{
	assert(pCallback);
	
	const NormalFolder* pFolder = pCallback->getFolder();
	if (pImpl_->folder_.isSpecified()) {
		wstring_ptr wstrName(pFolder->getFullName());
		if (!pImpl_->folder_.match(wstrName.get()))
			return false;
	}
	
	if (pImpl_->pCondition_.get()) {
		Lock<Account> lock(*pFolder->getAccount());
		std::auto_ptr<MacroContext> pContext = pCallback->getMacroContext();
		if (!pContext.get())
			return false;
		MacroValuePtr pValue(pImpl_->pCondition_->value(pContext.get()));
		if (!pValue.get())
			return false;
		if (!pValue->boolean())
			return false;
	}
	
	return true;
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
	wstring_ptr wstrName_;
	SyncFilterAction::ParamList listParam_;
};


/****************************************************************************
 *
 * SyncFilterAction
 *
 */

qm::SyncFilterAction::SyncFilterAction(const WCHAR* pwszName) :
	pImpl_(0)
{
	assert(pwszName);
	
	pImpl_ = new SyncFilterActionImpl();
	pImpl_->wstrName_ = allocWString(pwszName);
}

qm::SyncFilterAction::SyncFilterAction(const SyncFilterAction& action)
{
	pImpl_ = new SyncFilterActionImpl();
	pImpl_->wstrName_ = allocWString(action.getName());
	
	const ParamList& l = action.pImpl_->listParam_;
	pImpl_->listParam_.resize(l.size());
	for (ParamList::size_type n = 0; n < l.size(); ++n) {
		pImpl_->listParam_[n].first = allocWString(l[n].first).release();
		pImpl_->listParam_[n].second = allocWString(l[n].second).release();
	}
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
	ParamList::const_iterator it = std::find_if(
		pImpl_->listParam_.begin(), pImpl_->listParam_.end(),
		std::bind2nd(
			binary_compose_f_gx_hy(
				string_equal<WCHAR>(),
				std::select1st<ParamList::value_type>(),
				std::identity<const WCHAR*>()),
			pwszName));
	return it != pImpl_->listParam_.end() ? (*it).second : 0;
}

const SyncFilterAction::ParamList& qm::SyncFilterAction::getParams() const
{
	return pImpl_->listParam_;
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
		
		const WCHAR* pwszName = 0;
		for (int n = 0; n < attributes.getLength(); ++n) {
			const WCHAR* pwszAttrName = attributes.getLocalName(n);
			if (wcscmp(pwszAttrName, L"account") == 0)
				; // Just for compatibility
			else if (wcscmp(pwszAttrName, L"name") == 0)
				pwszName = attributes.getValue(n);
			else
				return false;
		}
		if (!pwszName)
			return false;
		
		std::auto_ptr<SyncFilterSet> pSet(new SyncFilterSet(pwszName));
		pCurrentFilterSet_ = pSet.get();
		pManager_->addFilterSet(pSet);
		
		state_ = STATE_FILTERSET;
	}
	else if (wcscmp(pwszLocalName, L"filter") == 0) {
		if (state_ != STATE_FILTERSET)
			return false;
		
		const WCHAR* pwszFolder = 0;
		const WCHAR* pwszMatch = 0;
		const WCHAR* pwszDescription = 0;
		for (int n = 0; n < attributes.getLength(); ++n) {
			const WCHAR* pwszAttrName = attributes.getLocalName(n);
			if (wcscmp(pwszAttrName, L"folder") == 0)
				pwszFolder = attributes.getValue(n);
			else if (wcscmp(pwszAttrName, L"match") == 0)
				pwszMatch = attributes.getValue(n);
			else if (wcscmp(pwszAttrName, L"description") == 0)
				pwszDescription = attributes.getValue(n);
			else
				return false;
		}
		
		Term folder;
		if (pwszFolder && !folder.setValue(pwszFolder))
			return false;
		
		std::auto_ptr<Macro> pCondition;
		if (pwszMatch) {
			pCondition = MacroParser().parse(pwszMatch);
			if (!pCondition.get())
				return false;
		}
		
		std::auto_ptr<SyncFilter> pFilter(new SyncFilter(
			folder, pCondition, pwszDescription));
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


/****************************************************************************
 *
 * SyncFilterWriter
 *
 */

qm::SyncFilterWriter::SyncFilterWriter(Writer* pWriter) :
	handler_(pWriter)
{
}

qm::SyncFilterWriter::~SyncFilterWriter()
{
}

bool qm::SyncFilterWriter::write(const SyncFilterManager* pManager)
{
	if (!handler_.startDocument())
		return false;
	if (!handler_.startElement(0, 0, L"filters", DefaultAttributes()))
		return false;
	
	const SyncFilterManager::FilterSetList& l = const_cast<SyncFilterManager*>(pManager)->getFilterSets(false);
	for (SyncFilterManager::FilterSetList::const_iterator it = l.begin(); it != l.end(); ++it) {
		const SyncFilterSet* pFilterSet = *it;
		if (!write(pFilterSet))
			return false;
	}
	
	if (!handler_.endElement(0, 0, L"filters"))
		return false;
	if (!handler_.endDocument())
		return false;
	
	return true;
}

bool qm::SyncFilterWriter::write(const SyncFilterSet* pFilterSet)
{
	const WCHAR* pwszName = pFilterSet->getName();
	const SimpleAttributes::Item items[] = {
		{ L"name",		pwszName							},
	};
	SimpleAttributes attrs(items, countof(items));
	if (!handler_.startElement(0, 0, L"filterSet", attrs))
		return false;
	
	const SyncFilterSet::FilterList& listFilter = pFilterSet->getFilters();
	for (SyncFilterSet::FilterList::const_iterator it = listFilter.begin(); it != listFilter.end(); ++it) {
		const SyncFilter* pFilter = *it;
		if (!write(pFilter))
			return false;
	}
	
	if (!handler_.endElement(0, 0, L"filterSet"))
		return false;
	
	return true;
}

bool qm::SyncFilterWriter::write(const SyncFilter* pFilter)
{
	const WCHAR* pwszFolder = pFilter->getFolder();
	wstring_ptr wstrCondition(pFilter->getCondition()->getString());
	const WCHAR* pwszDescription = pFilter->getDescription();
	const SimpleAttributes::Item items[] = {
		{ L"folder",		pwszFolder,			!pwszFolder			},
		{ L"match",			wstrCondition.get()						},
		{ L"description",	pwszDescription,	!pwszDescription	}
	};
	SimpleAttributes attrs(items, countof(items));
	if (!handler_.startElement(0, 0, L"filter", attrs))
		return false;
	
	const SyncFilter::ActionList& listAction = pFilter->getActions();
	for (SyncFilter::ActionList::const_iterator it = listAction.begin(); it != listAction.end(); ++it) {
		const SyncFilterAction* pAction = *it;
		if (!write(pAction))
			return false;
	}
	
	if (!handler_.endElement(0, 0, L"filter"))
		return false;
	
	return true;
}

bool qm::SyncFilterWriter::write(const SyncFilterAction* pAction)
{
	SimpleAttributes attrs(L"name", pAction->getName());
	if (!handler_.startElement(0, 0, L"action", attrs))
		return false;
	
	const SyncFilterAction::ParamList& listParam = pAction->getParams();
	for (SyncFilterAction::ParamList::const_iterator it = listParam.begin(); it != listParam.end(); ++it) {
		SimpleAttributes attrs(L"name", (*it).first);
		if (!handler_.startElement(0, 0, L"param", attrs) ||
			!handler_.characters((*it).second, 0, wcslen((*it).second)) ||
			!handler_.endElement(0, 0, L"param"))
			return false;
	}
	
	if (!handler_.endElement(0, 0, L"action"))
		return false;
	
	return true;
}
