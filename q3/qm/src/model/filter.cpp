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
#include <qsfile.h>
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
	return getFilters(true);
}

const FilterManager::FilterList& qm::FilterManager::getFilters(bool bReload)
{
	if (bReload)
		load();
	return listFilter_;
}

const Filter* qm::FilterManager::getFilter(const WCHAR* pwszName)
{
	load();
	FilterList::const_iterator it = std::find_if(
		listFilter_.begin(), listFilter_.end(),
		std::bind2nd(
			binary_compose_f_gx_hy(
				string_equal<WCHAR>(),
				std::mem_fun(&Filter::getName),
				std::identity<const WCHAR*>()),
			pwszName));
	return it != listFilter_.end() ? *it : 0;
}

void qm::FilterManager::setFilters(FilterList& listFilter)
{
	clear();
	listFilter_.swap(listFilter);
}

bool qm::FilterManager::save() const
{
	wstring_ptr wstrPath(Application::getApplication().getProfilePath(FileNames::FILTERS_XML));
	
	TemporaryFileRenamer renamer(wstrPath.get());
	
	FileOutputStream os(renamer.getPath());
	if (!os)
		return false;
	OutputStreamWriter writer(&os, false, L"utf-8");
	if (!writer)
		return false;
	BufferedWriter bufferedWriter(&writer, false);
	
	FilterWriter filterWriter(&bufferedWriter);
	if (!filterWriter.write(this))
		return false;
	
	if (!bufferedWriter.close())
		return false;
	
	if (!renamer.rename())
		return false;
	
	return true;
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

qm::Filter::Filter()
{
	wstrName_ = allocWString(L"");
	pCondition_ = MacroParser(MacroParser::TYPE_FILTER).parse(L"@True()");
	assert(pCondition_.get());
}

qm::Filter::Filter(const WCHAR* pwszName,
				   std::auto_ptr<Macro> pCondition) :
	pCondition_(pCondition)
{
	wstrName_ = allocWString(pwszName);
}

qm::Filter::Filter(const Filter& filter)
{
	wstrName_ = allocWString(filter.wstrName_.get());
	
	wstring_ptr wstrCondition(filter.pCondition_->getString());
	MacroParser parser(MacroParser::TYPE_FILTER);
	pCondition_ = parser.parse(wstrCondition.get());
	assert(pCondition_.get());
}

qm::Filter::~Filter()
{
}

const WCHAR* qm::Filter::getName() const
{
	assert(wstrName_.get());
	return wstrName_.get();
}

void qm::Filter::setName(const WCHAR* pwszName)
{
	assert(pwszName);
	wstrName_ = allocWString(pwszName);
}

const Macro* qm::Filter::getCondition() const
{
	assert(pCondition_.get());
	return pCondition_.get();
}

void qm::Filter::setCondition(std::auto_ptr<Macro> pCondition)
{
	assert(pCondition.get());
	pCondition_ = pCondition;
}

bool qm::Filter::match(MacroContext* pContext) const
{
	assert(pContext);
	
	MacroValuePtr pValue(pCondition_->value(pContext));
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


/****************************************************************************
 *
 * FilterWriter
 *
 */

qm::FilterWriter::FilterWriter(Writer* pWriter) :
	handler_(pWriter)
{
}

qm::FilterWriter::~FilterWriter()
{
}

bool qm::FilterWriter::write(const FilterManager* pManager)
{
	if (!handler_.startDocument())
		return false;
	if (!handler_.startElement(0, 0, L"filters", DefaultAttributes()))
		return false;
	
	const FilterManager::FilterList& l = const_cast<FilterManager*>(pManager)->getFilters(false);
	for (FilterManager::FilterList::const_iterator it = l.begin(); it != l.end(); ++it) {
		const Filter* pFilter = *it;
		if (!write(pFilter))
			return false;
	}
	
	if (!handler_.endElement(0, 0, L"filters"))
		return false;
	if (!handler_.endDocument())
		return false;
	
	return true;
}

bool qm::FilterWriter::write(const Filter* pFilter)
{
	SimpleAttributes attrs(L"name", pFilter->getName());
	if (!handler_.startElement(0, 0, L"filter", attrs))
		return false;
	
	const Macro* pCondition = pFilter->getCondition();
	wstring_ptr wstrCondition(pCondition->getString());
	if (!handler_.characters(wstrCondition.get(), 0, wcslen(wstrCondition.get())))
		return false;
	
	if (!handler_.endElement(0, 0, L"filter"))
		return false;
	
	return true;
}
