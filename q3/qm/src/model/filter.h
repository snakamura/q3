/*
 * $Id$
 *
 * Copyright(C) 1998-2004 Satoshi Nakamura
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
class FilterContentHandler;
class FilterWriter;

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
	FilterManager();
	~FilterManager();

public:
	const FilterList& getFilters();
	const FilterList& getFilters(bool bReload);
	const Filter* getFilter(const WCHAR* pwszName);
	void setFilters(FilterList& listFilter);
	bool save() const;

private:
	bool load();
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
	Filter();
	Filter(const WCHAR* pwszName,
		   std::auto_ptr<Macro> pCondition);
	Filter(const Filter& filter);
	~Filter();

public:
	const WCHAR* getName() const;
	void setName(const WCHAR* pwszName);
	const Macro* getCondition() const;
	void setCondition(std::auto_ptr<Macro> pCondition);
	bool match(MacroContext* pContext) const;

private:
	Filter& operator=(const Filter&);

private:
	qs::wstring_ptr wstrName_;
	std::auto_ptr<Macro> pCondition_;
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
	explicit FilterContentHandler(FilterList* pListFilter);
	virtual ~FilterContentHandler();

public:
	virtual bool startElement(const WCHAR* pwszNamespaceURI,
							  const WCHAR* pwszLocalName,
							  const WCHAR* pwszQName,
							  const qs::Attributes& attributes);
	virtual bool endElement(const WCHAR* pwszNamespaceURI,
						    const WCHAR* pwszLocalName,
						    const WCHAR* pwszQName);
	virtual bool characters(const WCHAR* pwsz,
							size_t nStart,
							size_t nLength);

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
	qs::wstring_ptr wstrName_;
	qs::StringBuffer<qs::WSTRING> buffer_;
};


/****************************************************************************
 *
 * FilterWriter
 *
 */

class FilterWriter
{
public:
	explicit FilterWriter(qs::Writer* pWriter);
	~FilterWriter();

public:
	bool write(const FilterManager* pManager);

private:
	bool write(const Filter* pFilter);

private:
	FilterWriter(const FilterWriter&);
	FilterWriter& operator=(const FilterWriter&);

private:
	qs::OutputHandler handler_;
};

}

#endif // __FILTER_H__
