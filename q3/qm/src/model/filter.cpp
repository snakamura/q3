/*
 * $Id$
 *
 * Copyright(C) 1998-2003 Satoshi Nakamura
 * All rights reserved.
 *
 */

#include <qmapplication.h>
#include <qmextensions.h>
#include <qmmacro.h>

#include <qsconv.h>
#include <qserror.h>
#include <qsnew.h>
#include <qsosutil.h>
#include <qsstl.h>

#include <algorithm>

#include "filter.h"

using namespace qm;
using namespace qs;


/****************************************************************************
 *
 * FilterManager
 *
 */

qm::FilterManager::FilterManager(QSTATUS* pstatus)
{
	assert(pstatus);
	
	DECLARE_QSTATUS();
	
	*pstatus = QSTATUS_SUCCESS;
	
	SYSTEMTIME st;
	::GetSystemTime(&st);
	::SystemTimeToFileTime(&st, &ft_);
	
	status = load();
	CHECK_QSTATUS_SET(pstatus);
}

qm::FilterManager::~FilterManager()
{
	clear();
}

QSTATUS qm::FilterManager::getFilters(const FilterList** ppList)
{
	assert(ppList);
	
	DECLARE_QSTATUS();
	
//	status = load();
//	CHECK_QSTATUS();
	
	*ppList = &listFilter_;
	
	return QSTATUS_SUCCESS;
}

QSTATUS qm::FilterManager::load()
{
	DECLARE_QSTATUS();
	
	string_ptr<WSTRING> wstrPath;
	status = Application::getApplication().getProfilePath(
		Extensions::FILTERS, &wstrPath);
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
			FilterContentHandler handler(&listFilter_, &status);
			CHECK_QSTATUS();
			reader.setContentHandler(&handler);
			status = reader.parse(wstrPath.get());
			CHECK_QSTATUS();
			
			ft_ = ft;
		}
	}
	
	return QSTATUS_SUCCESS;
}

void qm::FilterManager::clear()
{
	std::for_each(listFilter_.begin(), listFilter_.end(), deleter<Filter>());
	listFilter_.clear();
}


/****************************************************************************
 *
 * Filter
 *
 */

qm::Filter::Filter(const WCHAR* pwszName, QSTATUS* pstatus) :
	wstrName_(0),
	pMacro_(0)
{
	assert(pstatus);
	
	*pstatus = QSTATUS_SUCCESS;
	
	wstrName_ = allocWString(pwszName);
	if (!wstrName_) {
		*pstatus = QSTATUS_OUTOFMEMORY;
		return;
	}
}

qm::Filter::~Filter()
{
	freeWString(wstrName_);
	delete pMacro_;
}

const WCHAR* qm::Filter::getName() const
{
	return wstrName_;
}

QSTATUS qm::Filter::match(MacroContext* pContext, bool* pbMatch) const
{
	assert(pContext);
	assert(pbMatch);
	
	DECLARE_QSTATUS();
	
	*pbMatch = false;
	
	MacroValuePtr pValue;
	status = pMacro_->value(pContext, &pValue);
	CHECK_QSTATUS();
	*pbMatch = pValue->boolean();
	
	return QSTATUS_SUCCESS;
}

void qm::Filter::setMacro(Macro* pMacro)
{
	assert(!pMacro_);
	assert(pMacro);
	pMacro_ = pMacro;
}


/****************************************************************************
 *
 * FilterContentHandler
 *
 */

qm::FilterContentHandler::FilterContentHandler(
	FilterList* pListFilter, qs::QSTATUS* pstatus) :
	DefaultHandler(pstatus),
	pListFilter_(pListFilter),
	state_(STATE_ROOT),
	pFilter_(0),
	pBuffer_(0),
	pParser_(0)
{
	assert(pstatus);
	
	DECLARE_QSTATUS();
	
	*pstatus = QSTATUS_SUCCESS;
	
	status = newQsObject(&pBuffer_);
	CHECK_QSTATUS_SET(pstatus);
	
	status = newQsObject(MacroParser::TYPE_FILTER, &pParser_);
	CHECK_QSTATUS_SET(pstatus);
}

qm::FilterContentHandler::~FilterContentHandler()
{
	delete pBuffer_;
	delete pParser_;
}

QSTATUS qm::FilterContentHandler::startElement(const WCHAR* pwszNamespaceURI,
	const WCHAR* pwszLocalName, const WCHAR* pwszQName,
	const Attributes& attributes)
{
	DECLARE_QSTATUS();
	
	if (wcscmp(pwszLocalName, L"filters") == 0) {
		if (state_ != STATE_ROOT)
			return QSTATUS_FAIL;
		if (attributes.getLength() != 0)
			return QSTATUS_FAIL;
		state_ = STATE_FILTERS;
	}
	else if (wcscmp(pwszLocalName, L"filter") == 0) {
		if (state_ != STATE_FILTERS)
			return QSTATUS_FAIL;
		
		const WCHAR* pwszName = 0;
		for (int n = 0; n < attributes.getLength(); ++n) {
			const WCHAR* pwszAttrName = attributes.getLocalName(n);
			if (wcscmp(pwszAttrName, L"name") == 0)
				pwszName = attributes.getValue(n);
		}
		if (!pwszName)
			return QSTATUS_FAIL;
		
		assert(!pFilter_);
		std::auto_ptr<Filter> pFilter;
		status = newQsObject(pwszName, &pFilter);
		CHECK_QSTATUS();
		status = STLWrapper<FilterList>(*pListFilter_).push_back(pFilter.get());
		CHECK_QSTATUS();
		pFilter_ = pFilter.release();
		
		state_ = STATE_FILTER;
	}
	else {
		return QSTATUS_FAIL;
	}
	
	return QSTATUS_SUCCESS;
}

QSTATUS qm::FilterContentHandler::endElement(const WCHAR* pwszNamespaceURI,
	const WCHAR* pwszLocalName, const WCHAR* pwszQName)
{
	DECLARE_QSTATUS();
	
	if (wcscmp(pwszLocalName, L"filters") == 0) {
		assert(state_ == STATE_FILTERS);
		state_ = STATE_ROOT;
	}
	else if (wcscmp(pwszLocalName, L"filter") == 0) {
		assert(state_ == STATE_FILTER);
		
		Macro* pMacro = 0;
		status = pParser_->parse(pBuffer_->getCharArray(), &pMacro);
		CHECK_QSTATUS();
		assert(pFilter_);
		pFilter_->setMacro(pMacro);
		pFilter_ = 0;
		pBuffer_->remove();
		
		state_ = STATE_FILTERS;
	}
	else {
		return QSTATUS_FAIL;
	}
	
	return QSTATUS_SUCCESS;
}

QSTATUS qm::FilterContentHandler::characters(
	const WCHAR* pwsz, size_t nStart, size_t nLength)
{
	DECLARE_QSTATUS();
	
	if (state_ == STATE_FILTER) {
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
