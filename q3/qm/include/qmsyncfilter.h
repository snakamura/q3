/*
 * $Id$
 *
 * Copyright(C) 1998-2005 Satoshi Nakamura
 * All rights reserved.
 *
 */

#ifndef __QMSYNCFILTER_H__
#define __QMSYNCFILTER_H__

#include <qm.h>

#include <qs.h>
#include <qsregex.h>

#include <vector>


namespace qm {

class SyncFilterManager;
class SyncFilterSet;
class SyncFilter;
class SyncFilterCallback;
class SyncFilterAction;

class Account;
class Macro;
class MacroContext;
class NormalFolder;
class RegexValue;


/****************************************************************************
 *
 * SyncFilterManager
 *
 */

class QMEXPORTCLASS SyncFilterManager
{
public:
	typedef std::vector<SyncFilterSet*> FilterSetList;

public:
	explicit SyncFilterManager(const WCHAR* pwszPath);
	~SyncFilterManager();

public:
	const FilterSetList& getFilterSets();
	const FilterSetList& getFilterSets(bool bReload);
	std::auto_ptr<SyncFilterSet> getFilterSet(const WCHAR* pwszName) const;
	void setFilterSets(FilterSetList& listFilterSet);
	bool save() const;
	void clear();

public:
	void addFilterSet(std::auto_ptr<SyncFilterSet> pFilterSet);

private:
	SyncFilterManager(const SyncFilterManager&);
	SyncFilterManager& operator=(const SyncFilterManager&);

private:
	struct SyncFilterManagerImpl* pImpl_;
};


/****************************************************************************
 *
 * SyncFilterSet
 *
 */

class QMEXPORTCLASS SyncFilterSet
{
public:
	typedef std::vector<SyncFilter*> FilterList;

public:
	SyncFilterSet();
	SyncFilterSet(const WCHAR* pwszName);
	SyncFilterSet(const SyncFilterSet& filterSet);
	~SyncFilterSet();

public:
	const WCHAR* getName() const;
	void setName(const WCHAR* pwszAccount);
	const FilterList& getFilters() const;
	const SyncFilter* getFilter(SyncFilterCallback* pCallback) const;
	void setFilters(FilterList& listFilter);

public:
	void addFilter(std::auto_ptr<SyncFilter> pFilter);

private:
	SyncFilterSet& operator=(const SyncFilterSet&);

private:
	struct SyncFilterSetImpl* pImpl_;
};


/****************************************************************************
 *
 * SyncFilter
 *
 */

class QMEXPORTCLASS SyncFilter
{
public:
	typedef std::vector<SyncFilterAction*> ActionList;

public:
	SyncFilter();
	SyncFilter(RegexValue& folder,
			   std::auto_ptr<Macro> pCondition,
			   const WCHAR* pwszDescription);
	SyncFilter(const SyncFilter& filter);
	~SyncFilter();

public:
	const WCHAR* getFolder() const;
	void setFolder(RegexValue& folder);
	const Macro* getCondition() const;
	void setCondition(std::auto_ptr<Macro> pCondition);
	const WCHAR* getDescription() const;
	void setDescription(const WCHAR* pwszDescription);
	const ActionList& getActions() const;
	void setActions(ActionList& listAction);
	bool match(SyncFilterCallback* pCallback) const;

public:
	void addAction(std::auto_ptr<SyncFilterAction> pAction);

private:
	SyncFilter& operator=(const SyncFilter&);

private:
	struct SyncFilterImpl* pImpl_;
};


/****************************************************************************
 *
 * SyncFilterCallback
 *
 */

class QMEXPORTCLASS SyncFilterCallback
{
public:
	virtual ~SyncFilterCallback();

public:
	virtual const NormalFolder* getFolder() = 0;
	virtual std::auto_ptr<MacroContext> getMacroContext() = 0;
};


/****************************************************************************
 *
 * SyncFilterAction
 *
 */

class QMEXPORTCLASS SyncFilterAction
{
public:
	typedef std::vector<std::pair<qs::WSTRING, qs::WSTRING> > ParamList;

public:
	explicit SyncFilterAction(const WCHAR* pwszName);
	SyncFilterAction(const SyncFilterAction& action);
	~SyncFilterAction();

public:
	const WCHAR* getName() const;
	const WCHAR* getParam(const WCHAR* pwszName) const;
	const ParamList& getParams() const;

public:
	void addParam(qs::wstring_ptr wstrName,
				  qs::wstring_ptr wstrValue);

private:
	SyncFilterAction& operator=(const SyncFilterAction&);

private:
	struct SyncFilterActionImpl* pImpl_;
};

}

#endif // __QMSYNCFILTER_H__
