/*
 * $Id$
 *
 * Copyright(C) 1998-2003 Satoshi Nakamura
 * All rights reserved.
 *
 */

#include <qmaccount.h>
#include <qmapplication.h>
#include <qmfilenames.h>
#include <qmfolder.h>
#include <qmsyncfilter.h>

#include <qsconv.h>
#include <qserror.h>
#include <qsnew.h>
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
	QSTATUS load();
	void clear();

public:
	SyncFilterManager* pThis_;
	SyncFilterManager::FilterSetList listFilterSet_;
	FILETIME ft_;
};

QSTATUS qm::SyncFilterManagerImpl::load()
{
	DECLARE_QSTATUS();
	
	string_ptr<WSTRING> wstrPath;
	status = Application::getApplication().getProfilePath(
		FileNames::SYNCFILTERS_XML, &wstrPath);
	CHECK_QSTATUS();
	
	W2T(wstrPath.get(), ptszPath);
	AutoHandle hFile(::CreateFile(ptszPath, GENERIC_READ, 0, 0,
		OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0));
	if (hFile.get()) {
		FILETIME ft;
		::GetFileTime(hFile.get(), 0, 0, &ft);
		hFile.close();
		
		if (::CompareFileTime(&ft, &ft_) != 0) {
			clear();
			
			XMLReader reader(&status);
			CHECK_QSTATUS();
			SyncFilterContentHandler handler(pThis_, &status);
			CHECK_QSTATUS();
			reader.setContentHandler(&handler);
			status = reader.parse(wstrPath.get());
			CHECK_QSTATUS();
			
			ft_ = ft;
		}
	}
	else {
		clear();
	}
	
	return QSTATUS_SUCCESS;
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

qm::SyncFilterManager::SyncFilterManager(QSTATUS* pstatus) :
	pImpl_(0)
{
	assert(pstatus);
	
	DECLARE_QSTATUS();
	
	status = newObject(&pImpl_);
	CHECK_QSTATUS_SET(pstatus);
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

QSTATUS qm::SyncFilterManager::getFilterSet(const Account* pAccount,
	const WCHAR* pwszName, const SyncFilterSet** ppSyncFilterSet) const
{
	assert(ppSyncFilterSet);
	
	DECLARE_QSTATUS();
	
	*ppSyncFilterSet = 0;
	
	if (pwszName) {
		status = pImpl_->load();
		CHECK_QSTATUS();
		
		FilterSetList::const_iterator it = pImpl_->listFilterSet_.begin();
		while (it != pImpl_->listFilterSet_.end() && !*ppSyncFilterSet) {
			bool bMatch = false;
			status = (*it)->match(pAccount, pwszName, &bMatch);
			CHECK_QSTATUS();
			if (bMatch)
				*ppSyncFilterSet = *it;
			++it;
		}
	}
	
	return QSTATUS_SUCCESS;
}

QSTATUS qm::SyncFilterManager::getFilterSets(
	const Account* pAccount, FilterSetList* pList) const
{
	assert(pList);
	
	DECLARE_QSTATUS();
	
	status = pImpl_->load();
	CHECK_QSTATUS();
	
	FilterSetList::const_iterator it = pImpl_->listFilterSet_.begin();
	while (it != pImpl_->listFilterSet_.end()) {
		bool bMatch = false;
		status = (*it)->match(pAccount, 0, &bMatch);
		CHECK_QSTATUS();
		if (bMatch) {
			status = STLWrapper<FilterSetList>(*pList).push_back(*it);
			CHECK_QSTATUS();
		}
		++it;
	}
	
	return QSTATUS_SUCCESS;
}

QSTATUS qm::SyncFilterManager::addFilterSet(SyncFilterSet* pFilterSet)
{
	return STLWrapper<FilterSetList>(
		pImpl_->listFilterSet_).push_back(pFilterSet);
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
	RegexPattern* pAccountName_;
	WSTRING wstrName_;
	FilterList listFilter_;
};


/****************************************************************************
 *
 * SyncFilterSet
 *
 */

qm::SyncFilterSet::SyncFilterSet(const WCHAR* pwszAccount,
	const WCHAR* pwszName, qs::QSTATUS* pstatus) :
	pImpl_(0)
{
	assert(pwszName);
	assert(pstatus);
	
	DECLARE_QSTATUS();
	
	std::auto_ptr<RegexPattern> pAccountName;
	if (pwszAccount) {
		RegexPattern* p = 0;
		RegexCompiler compiler;
		status = compiler.compile(pwszAccount, &p);
		CHECK_QSTATUS_SET(pstatus);
		pAccountName.reset(p);
	}
	
	string_ptr<WSTRING> wstrName(allocWString(pwszName));
	if (!wstrName.get()) {
		*pstatus = QSTATUS_OUTOFMEMORY;
		return;
	}
	
	status = newObject(&pImpl_);
	CHECK_QSTATUS_SET(pstatus);
	pImpl_->pAccountName_ = pAccountName.release();
	pImpl_->wstrName_ = wstrName.release();
}

qm::SyncFilterSet::~SyncFilterSet()
{
	if (pImpl_) {
		delete pImpl_->pAccountName_;
		freeWString(pImpl_->wstrName_);
		std::for_each(pImpl_->listFilter_.begin(),
			pImpl_->listFilter_.end(), deleter<SyncFilter>());
		delete pImpl_;
	}
}

const WCHAR* qm::SyncFilterSet::getName() const
{
	return pImpl_->wstrName_;
}

QSTATUS qm::SyncFilterSet::getFilter(SyncFilterCallback* pCallback,
	const SyncFilter** ppFilter) const
{
	assert(pCallback);
	assert(ppFilter);
	
	DECLARE_QSTATUS();
	
	*ppFilter = 0;
	
	bool bMatch = false;
	SyncFilterSetImpl::FilterList::const_iterator it =
		pImpl_->listFilter_.begin();
	while (it != pImpl_->listFilter_.end()) {
		status = (*it)->match(pCallback, &bMatch);
		CHECK_QSTATUS();
		if (bMatch)
			break;
		++it;
	}
	if (it != pImpl_->listFilter_.end())
		*ppFilter = *it;
	
	return QSTATUS_SUCCESS;
}

QSTATUS qm::SyncFilterSet::match(const Account* pAccount,
	const WCHAR* pwszName, bool* pbMatch) const
{
	assert(pAccount);
	assert(pbMatch);
	
	DECLARE_QSTATUS();
	
	if (pImpl_->pAccountName_) {
		status = pImpl_->pAccountName_->match(pAccount->getName(), pbMatch);
		CHECK_QSTATUS();
		if (!*pbMatch)
			return QSTATUS_SUCCESS;
	}
	
	if (pwszName)
		*pbMatch = wcscmp(pImpl_->wstrName_, pwszName) == 0;
	else
		*pbMatch = true;
	
	return QSTATUS_SUCCESS;
}

QSTATUS qm::SyncFilterSet::addFilter(SyncFilter* pFilter)
{
	return STLWrapper<SyncFilterSetImpl::FilterList>(
		pImpl_->listFilter_).push_back(pFilter);
}


/****************************************************************************
 *
 * SyncFilterImpl
 *
 */

struct qm::SyncFilterImpl
{
	RegexPattern* pFolderName_;
	Macro* pMacro_;
	SyncFilter::ActionList listAction_;
};


/****************************************************************************
 *
 * SyncFilter
 *
 */

qm::SyncFilter::SyncFilter(const WCHAR* pwszFolder,
	Macro* pMacro, QSTATUS* pstatus) :
	pImpl_(0)
{
	assert(pstatus);
	
	DECLARE_QSTATUS();
	
	std::auto_ptr<RegexPattern> pFolderName;
	if (pwszFolder) {
		RegexCompiler compiler;
		RegexPattern* p = 0;
		status = compiler.compile(pwszFolder, &p);
		CHECK_QSTATUS_SET(pstatus);
		pFolderName.reset(p);
	}
	
	status = newObject(&pImpl_);
	CHECK_QSTATUS_SET(pstatus);
	pImpl_->pFolderName_ = pFolderName.release();
	pImpl_->pMacro_ = pMacro;
}

qm::SyncFilter::~SyncFilter()
{
	if (pImpl_) {
		delete pImpl_->pFolderName_;
		delete pImpl_->pMacro_;
		std::for_each(pImpl_->listAction_.begin(),
			pImpl_->listAction_.end(), deleter<SyncFilterAction>());
		delete pImpl_;
	}
}

QSTATUS qm::SyncFilter::match(SyncFilterCallback* pCallback, bool* pbMatch) const
{
	assert(pCallback);
	assert(pbMatch);
	
	DECLARE_QSTATUS();
	
	*pbMatch = true;
	
	const NormalFolder* pFolder = pCallback->getFolder();
	if (pImpl_->pFolderName_) {
		string_ptr<WSTRING> wstrName;
		status = pFolder->getFullName(&wstrName);
		CHECK_QSTATUS();
		status = pImpl_->pFolderName_->match(wstrName.get(), pbMatch);
		CHECK_QSTATUS();
		if (!*pbMatch)
			return QSTATUS_SUCCESS;
	}
	
	if (pImpl_->pMacro_) {
		Lock<Account> lock(*pFolder->getAccount());
		MacroContext* pContext = 0;
		status = pCallback->getMacroContext(&pContext);
		CHECK_QSTATUS();
		std::auto_ptr<MacroContext> apContext(pContext);
		MacroValuePtr pValue;
		status = pImpl_->pMacro_->value(apContext.get(), &pValue);
		CHECK_QSTATUS();
		
		*pbMatch = pValue->boolean();
	}
	
	return QSTATUS_SUCCESS;
}

const SyncFilter::ActionList& qm::SyncFilter::getActions() const
{
	return pImpl_->listAction_;
}

QSTATUS qm::SyncFilter::addAction(SyncFilterAction* pAction)
{
	return STLWrapper<ActionList>(pImpl_->listAction_).push_back(pAction);
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
	WSTRING wstrName_;
	ParamList listParam_;
};


/****************************************************************************
 *
 * SyncFilterAction
 *
 */

qm::SyncFilterAction::SyncFilterAction(
	const WCHAR* pwszName, QSTATUS* pstatus) :
	pImpl_(0)
{
	assert(pstatus);
	
	DECLARE_QSTATUS();
	
	string_ptr<WSTRING> wstrName(allocWString(pwszName));
	if (!wstrName.get()) {
		*pstatus = QSTATUS_FAIL;
		return;
	}
	
	status = newObject(&pImpl_);
	CHECK_QSTATUS_SET(pstatus);
	pImpl_->wstrName_ = wstrName.release();
}

qm::SyncFilterAction::~SyncFilterAction()
{
	if (pImpl_) {
		freeWString(pImpl_->wstrName_);
		std::for_each(pImpl_->listParam_.begin(), pImpl_->listParam_.end(),
			unary_compose_fx_gx(string_free<WSTRING>(), string_free<WSTRING>()));
		delete pImpl_;
	}
}

const WCHAR* qm::SyncFilterAction::getName() const
{
	return pImpl_->wstrName_;
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

QSTATUS qm::SyncFilterAction::addParam(
	const WCHAR* pwszName, const WCHAR* pwszValue)
{
	DECLARE_QSTATUS();
	
	string_ptr<WSTRING> wstrName(allocWString(pwszName));
	if (!wstrName.get())
		return QSTATUS_OUTOFMEMORY;
	string_ptr<WSTRING> wstrValue(allocWString(pwszValue));
	if (!wstrValue.get())
		return QSTATUS_OUTOFMEMORY;
	
	status = STLWrapper<SyncFilterActionImpl::ParamList>(
		pImpl_->listParam_).push_back(
			std::make_pair(wstrName.get(), wstrValue.get()));
	CHECK_QSTATUS();
	wstrName.release();
	wstrValue.release();
	
	return QSTATUS_SUCCESS;
}


/****************************************************************************
 *
 * SyncFilterContentHandler
 *
 */

qm::SyncFilterContentHandler::SyncFilterContentHandler(
	SyncFilterManager* pManager, QSTATUS* pstatus) :
	DefaultHandler(pstatus),
	pManager_(pManager),
	state_(STATE_ROOT),
	pCurrentFilterSet_(0),
	pCurrentFilter_(0),
	pCurrentAction_(0),
	wstrCurrentParamName_(0),
	pBuffer_(0),
	pParser_(0)
{
	DECLARE_QSTATUS();
	
	status = newQsObject(&pBuffer_);
	CHECK_QSTATUS_SET(pstatus);
	
	status = newQsObject(MacroParser::TYPE_SYNCFILTER, &pParser_);
	CHECK_QSTATUS_SET(pstatus);
}

qm::SyncFilterContentHandler::~SyncFilterContentHandler()
{
	freeWString(wstrCurrentParamName_);
	delete pBuffer_;
	delete pParser_;
}

QSTATUS qm::SyncFilterContentHandler::startElement(
	const WCHAR* pwszNamespaceURI, const WCHAR* pwszLocalName,
	const WCHAR* pwszQName, const Attributes& attributes)
{
	DECLARE_QSTATUS();
	
	if (wcscmp(pwszLocalName, L"filters") == 0) {
		if (state_ != STATE_ROOT)
			return QSTATUS_FAIL;
		if (attributes.getLength() != 0)
			return QSTATUS_FAIL;
		state_ = STATE_FILTERS;
	}
	else if (wcscmp(pwszLocalName, L"filterSet") == 0) {
		if (state_ != STATE_FILTERS)
			return QSTATUS_FAIL;
		
		const WCHAR* pwszAccount = 0;
		const WCHAR* pwszName = 0;
		for (int n = 0; n < attributes.getLength(); ++n) {
			const WCHAR* pwszAttrName = attributes.getLocalName(n);
			if (wcscmp(pwszAttrName, L"account") == 0)
				pwszAccount = attributes.getValue(n);
			else if (wcscmp(pwszAttrName, L"name") == 0)
				pwszName = attributes.getValue(n);
			else
				return QSTATUS_FAIL;
		}
		if (!pwszName)
			return QSTATUS_FAIL;
		
		std::auto_ptr<SyncFilterSet> pSet;
		status = newQsObject(pwszAccount, pwszName, &pSet);
		CHECK_QSTATUS();
		status = pManager_->addFilterSet(pSet.get());
		CHECK_QSTATUS();
		pCurrentFilterSet_ = pSet.release();
		
		state_ = STATE_FILTERSET;
	}
	else if (wcscmp(pwszLocalName, L"filter") == 0) {
		if (state_ != STATE_FILTERSET)
			return QSTATUS_FAIL;
		
		const WCHAR* pwszFolder = 0;
		const WCHAR* pwszMatch = 0;
		for (int n = 0; n < attributes.getLength(); ++n) {
			const WCHAR* pwszAttrName = attributes.getLocalName(n);
			if (wcscmp(pwszAttrName, L"folder") == 0)
				pwszFolder = attributes.getValue(n);
			else if (wcscmp(pwszAttrName, L"match") == 0)
				pwszMatch = attributes.getValue(n);
			else
				return QSTATUS_FAIL;
		}
		
		std::auto_ptr<Macro> pMacro;
		if (pwszMatch) {
			Macro* p = 0;
			status = pParser_->parse(pwszMatch, &p);
			CHECK_QSTATUS();
			pMacro.reset(p);
		}
		
		std::auto_ptr<SyncFilter> pFilter;
		status = newQsObject(pwszFolder, pMacro.get(), &pFilter);
		CHECK_QSTATUS();
		pMacro.release();
		assert(pCurrentFilterSet_);
		status = pCurrentFilterSet_->addFilter(pFilter.get());
		CHECK_QSTATUS();
		pCurrentFilter_ = pFilter.release();
		
		state_ = STATE_FILTER;
	}
	else if (wcscmp(pwszLocalName, L"action") == 0) {
		if (state_ != STATE_FILTER)
			return QSTATUS_FAIL;
		
		const WCHAR* pwszName = 0;
		for (int n = 0; n < attributes.getLength(); ++n) {
			const WCHAR* pwszAttrName = attributes.getLocalName(n);
			if (wcscmp(pwszAttrName, L"name") == 0)
				pwszName = attributes.getValue(n);
			else
				return QSTATUS_FAIL;
		}
		if (!pwszName)
			return QSTATUS_FAIL;
		
		std::auto_ptr<SyncFilterAction> pAction;
		status = newQsObject(pwszName, &pAction);
		CHECK_QSTATUS();
		status = pCurrentFilter_->addAction(pAction.get());
		CHECK_QSTATUS();
		pCurrentAction_ = pAction.release();
		
		state_ = STATE_ACTION;
	}
	else if (wcscmp(pwszLocalName, L"param") == 0) {
		if (state_ != STATE_ACTION)
			return QSTATUS_FAIL;
		
		const WCHAR* pwszName = 0;
		for (int n = 0; n < attributes.getLength(); ++n) {
			const WCHAR* pwszAttrName = attributes.getLocalName(n);
			if (wcscmp(pwszAttrName, L"name") == 0)
				pwszName = attributes.getValue(n);
			else
				return QSTATUS_FAIL;
		}
		if (!pwszName)
			return QSTATUS_FAIL;
		
		assert(!wstrCurrentParamName_);
		wstrCurrentParamName_ = allocWString(pwszName);
		if (!wstrCurrentParamName_)
			return QSTATUS_OUTOFMEMORY;
		
		state_ = STATE_PARAM;
	}
	else {
		return QSTATUS_FAIL;
	}
	
	return QSTATUS_SUCCESS;
}

QSTATUS qm::SyncFilterContentHandler::endElement(
	const WCHAR* pwszNamespaceURI, const WCHAR* pwszLocalName,
	const WCHAR* pwszQName)
{
	DECLARE_QSTATUS();
	
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
			return QSTATUS_FAIL;
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
		assert(wstrCurrentParamName_);
		status = pCurrentAction_->addParam(
			wstrCurrentParamName_, pBuffer_->getCharArray());
		CHECK_QSTATUS();
		freeWString(wstrCurrentParamName_);
		wstrCurrentParamName_ = 0;
		pBuffer_->remove();
		state_ = STATE_ACTION;
	}
	else {
		return QSTATUS_FAIL;
	}
	
	return QSTATUS_SUCCESS;
}

QSTATUS qm::SyncFilterContentHandler::characters(
	const WCHAR* pwsz, size_t nStart, size_t nLength)
{
	DECLARE_QSTATUS();
	
	if (state_ == STATE_PARAM) {
		status = pBuffer_->append(pwsz + nStart, nLength);
		CHECK_QSTATUS();
	}
	else {
		const WCHAR* p = pwsz + nStart;
		for (size_t n = 0; n < nLength; ++n, ++p) {
			if (*p != L' ' && *p != L'\t' && *p != '\n')
				return QSTATUS_FAIL;
		}
	}
	
	return QSTATUS_SUCCESS;
}
