/*
 * $Id$
 *
 * Copyright(C) 1998-2003 Satoshi Nakamura
 * All rights reserved.
 *
 */

#ifndef __FILTER_H__
#define __FILTER_H__

#include <qm.h>

#include <qs.h>
#include <qsprofile.h>
#include <qssax.h>
#include <qsstring.h>


namespace qm {

class FilterManager;
class Filter;

class Macro;
class MacroContext;
class MacroParser;


/****************************************************************************
 *
 * FilterManager
 *
 */

class FilterManager
{
public:
	typedef std::vector<Filter*> FilterList;

public:
	explicit FilterManager(qs::QSTATUS* pstatus);
	~FilterManager();

public:
	qs::QSTATUS getFilters(const FilterList** ppList);

private:
	qs::QSTATUS load();
	void clear();

private:
	FilterManager(const FilterManager&);
	FilterManager& operator=(const FilterManager&);

private:
	FILETIME ft_;
	FilterList listFilter_;
};


/****************************************************************************
 *
 * Filter
 *
 */

class Filter
{
public:
	Filter(const WCHAR* pwszName, qs::QSTATUS* pstatus);
	~Filter();

public:
	const WCHAR* getName() const;
	qs::QSTATUS match(MacroContext* pContext, bool* pbMatch) const;

public:
	void setMacro(Macro* pMacro);

private:
	Filter(const Filter&);
	Filter& operator=(const Filter&);

private:
	qs::WSTRING wstrName_;
	Macro* pMacro_;
};


/****************************************************************************
 *
 * FilterContentHandler
 *
 */

class FilterContentHandler : public qs::DefaultHandler
{
public:
	typedef FilterManager::FilterList FilterList;

public:
	FilterContentHandler(FilterList* pListFilter, qs::QSTATUS* pstatus);
	virtual ~FilterContentHandler();

public:
	virtual qs::QSTATUS startElement(const WCHAR* pwszNamespaceURI,
		const WCHAR* pwszLocalName, const WCHAR* pwszQName,
		const qs::Attributes& attributes);
	virtual qs::QSTATUS endElement(const WCHAR* pwszNamespaceURI,
		const WCHAR* pwszLocalName, const WCHAR* pwszQName);
	virtual qs::QSTATUS characters(const WCHAR* pwsz,
		size_t nStart, size_t nLength);

private:
	FilterContentHandler(const FilterContentHandler&);
	FilterContentHandler& operator=(const FilterContentHandler&);

private:
	enum State {
		STATE_ROOT,
		STATE_FILTERS,
		STATE_FILTER
	};

private:
	FilterList* pListFilter_;
	State state_;
	Filter* pFilter_;
	qs::StringBuffer<qs::WSTRING>* pBuffer_;
	MacroParser* pParser_;
};

}

#endif // __FILTER_H__
