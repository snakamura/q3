/*
 * $Id$
 *
 * Copyright(C) 1998-2004 Satoshi Nakamura
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
	SyncFilterManager();
	~SyncFilterManager();

public:
	const SyncFilterSet* getFilterSet(const Account* pAccount,
									  const WCHAR* pwszName) const;
	void getFilterSets(const Account* pAccount,
					   FilterSetList* pList) const;

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
	SyncFilterSet(std::auto_ptr<qs::RegexPattern> pAccountName,
				  const WCHAR* pwszName);
	~SyncFilterSet();

public:
	const WCHAR* getName() const;
	const SyncFilter* getFilter(SyncFilterCallback* pCallback) const;
	bool match(const Account* pAccount,
			   const WCHAR* pwszName) const;

public:
	void addFilter(std::auto_ptr<SyncFilter> pFilter);

private:
	SyncFilterSet(const SyncFilterSet&);
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
	SyncFilter(std::auto_ptr<qs::RegexPattern> pFolderName,
			   std::auto_ptr<Macro> pMacro);
	~SyncFilter();

public:
	bool match(SyncFilterCallback* pCallback) const;
	const ActionList& getActions() const;

public:
	void addAction(std::auto_ptr<SyncFilterAction> pAction);

private:
	SyncFilter(const SyncFilter&);
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
	explicit SyncFilterAction(const WCHAR* pwszName);
	~SyncFilterAction();

public:
	const WCHAR* getName() const;
	const WCHAR* getParam(const WCHAR* pwszName) const;

public:
	void addParam(qs::wstring_ptr wstrName,
				  qs::wstring_ptr wstrValue);

private:
	SyncFilterAction(const SyncFilterAction&);
	SyncFilterAction& operator=(const SyncFilterAction&);

private:
	struct SyncFilterActionImpl* pImpl_;
};

}

#endif // __QMSYNCFILTER_H__
