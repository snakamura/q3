/*
 * $Id$
 *
 * Copyright(C) 1998-2004 Satoshi Nakamura
 * All rights reserved.
 *
 */

#include <qmapplication.h>
#include <qmfilenames.h>
#include <qmmacro.h>

#include <qsconv.h>
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

qm::FilterManager::FilterManager()
{
	SYSTEMTIME st;
	::GetSystemTime(&st);
	::SystemTimeToFileTime(&st, &ft_);
}

qm::FilterManager::~FilterManager()
{
	clear();
}

const FilterManager::FilterList& qm::FilterManager::getFilters()
{
	load();
	return listFilter_;
}

bool qm::FilterManager::load()
{
	wstring_ptr wstrPath(Application::getApplication().getProfilePath(FileNames::FILTERS_XML));
	
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
			FilterContentHandler handler(&listFilter_);
			reader.setContentHandler(&handler);
			if (!reader.parse(wstrPath.get()))
				return false;
			
			ft_ = ft;
		}
	}
	else {
		clear();
		
		SYSTEMTIME st;
		::GetSystemTime(&st);
		::SystemTimeToFileTime(&st, &ft_);
	}
	
	return true;
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

qm::Filter::Filter(const WCHAR* pwszName,
				   std::auto_ptr<Macro> pMacro) :
	pMacro_(pMacro)
{
	wstrName_ = allocWString(pwszName);
}

qm::Filter::Filter(const Filter& filter)
{
	wstrName_ = allocWString(filter.wstrName_.get());
	
	wstring_ptr wstrMacro(filter.pMacro_->getString());
	MacroParser parser(MacroParser::TYPE_FILTER);
	pMacro_ = parser.parse(wstrMacro.get());
	assert(pMacro_.get());
}

qm::Filter::~Filter()
{
}

const WCHAR* qm::Filter::getName() const
{
	return wstrName_.get();
}

const Macro* qm::Filter::getMacro() const
{
	return pMacro_.get();
}

bool qm::Filter::match(MacroContext* pContext) const
{
	assert(pContext);
	
	MacroValuePtr pValue(pMacro_->value(pContext));
	return pValue.get() && pValue->boolean();
}


/****************************************************************************
 *
 * FilterContentHandler
 *
 */

qm::FilterContentHandler::FilterContentHandler(FilterList* pListFilter) :
	pListFilter_(pListFilter),
	state_(STATE_ROOT)
{
}

qm::FilterContentHandler::~FilterContentHandler()
{
}

bool qm::FilterContentHandler::startElement(const WCHAR* pwszNamespaceURI,
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
	else if (wcscmp(pwszLocalName, L"filter") == 0) {
		if (state_ != STATE_FILTERS)
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
		
		assert(!wstrName_.get());
		wstrName_ = allocWString(pwszName);
		
		state_ = STATE_FILTER;
	}
	else {
		return false;
	}
	
	return true;
}

bool qm::FilterContentHandler::endElement(const WCHAR* pwszNamespaceURI,
										  const WCHAR* pwszLocalName,
										  const WCHAR* pwszQName)
{
	if (wcscmp(pwszLocalName, L"filters") == 0) {
		assert(state_ == STATE_FILTERS);
		state_ = STATE_ROOT;
	}
	else if (wcscmp(pwszLocalName, L"filter") == 0) {
		assert(state_ == STATE_FILTER);
		
		const WCHAR* pwszMacro = buffer_.getCharArray();
		MacroParser parser(MacroParser::TYPE_FILTER);
		std::auto_ptr<Macro> pMacro(parser.parse(pwszMacro));
		if (!pMacro.get())
			return false;
		std::auto_ptr<Filter> pFilter(new Filter(wstrName_.get(), pMacro));
		pListFilter_->push_back(pFilter.get());
		pFilter.release();
		
		wstrName_.reset(0);
		buffer_.remove();
		
		state_ = STATE_FILTERS;
	}
	else {
		return false;
	}
	
	return true;
}

bool qm::FilterContentHandler::characters(const WCHAR* pwsz,
										  size_t nStart,
										  size_t nLength)
{
	if (state_ == STATE_FILTER) {
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
