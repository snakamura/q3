/*
 * $Id$
 *
 * Copyright(C) 1998-2003 Satoshi Nakamura
 * All rights reserved.
 *
 */

#ifndef __QMSYNCFILTER_H__
#define __QMSYNCFILTER_H__

#include <qm.h>

#include <qs.h>

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
	explicit SyncFilterManager(qs::QSTATUS* pstatus);
	~SyncFilterManager();

public:
	qs::QSTATUS getFilterSet(const Account* pAccount,
		const WCHAR* pwszName, const SyncFilterSet** ppSyncFilterSet) const;
	qs::QSTATUS getFilterSets(const Account* pAccount,
		FilterSetList* pList) const;

public:
	qs::QSTATUS addFilterSet(SyncFilterSet* pFilterSet);

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
	SyncFilterSet(const WCHAR* pwszAccount,
		const WCHAR* pwszName, qs::QSTATUS* pstatus);
	~SyncFilterSet();

public:
	const WCHAR* getName() const;
	qs::QSTATUS getFilter(SyncFilterCallback* pCallback,
		const SyncFilter** ppFilter) const;
	qs::QSTATUS match(const Account* pAccount,
		const WCHAR* pwszName, bool* pbMatch) const;

public:
	qs::QSTATUS addFilter(SyncFilter* pFilter);

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
	SyncFilter(const WCHAR* pwszFolder, Macro* pMacro, qs::QSTATUS* pstatus);
	~SyncFilter();

public:
	qs::QSTATUS match(SyncFilterCallback* pCallback, bool* pbMatch) const;
	const ActionList& getActions() const;

public:
	qs::QSTATUS addAction(SyncFilterAction* pAction);

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
	virtual qs::QSTATUS getMacroContext(MacroContext** ppContext) = 0;
};


/****************************************************************************
 *
 * SyncFilterAction
 *
 */

class QMEXPORTCLASS SyncFilterAction
{
public:
	SyncFilterAction(const WCHAR* pwszName, qs::QSTATUS* pstatus);
	~SyncFilterAction();

public:
	const WCHAR* getName() const;
	const WCHAR* getParam(const WCHAR* pwszName) const;

public:
	qs::QSTATUS addParam(const WCHAR* pwszName, const WCHAR* pwszValue);

private:
	SyncFilterAction(const SyncFilterAction&);
	SyncFilterAction& operator=(const SyncFilterAction&);

private:
	struct SyncFilterActionImpl* pImpl_;
};

}

#endif // __QMSYNCFILTER_H__
