/*
 * $Id$
 *
 * Copyright(C) 1998-2005 Satoshi Nakamura
 * All rights reserved.
 *
 */

#ifndef __QMRULE_H__
#define __QMRULE_H__

#include <qm.h>
#include <qmmessageholderlist.h>

#include <qsprofile.h>


namespace qm {

class RuleManager;
class RuleCallback;

class Document;
class Folder;
class Rule;
class RuleSet;


/****************************************************************************
 *
 * RuleManager
 *
 */

class QMEXPORTCLASS RuleManager
{
public:
	typedef std::vector<RuleSet*> RuleSetList;

public:
	explicit RuleManager(const WCHAR* pwszPath);
	~RuleManager();

public:
	const RuleSetList& getRuleSets();
	const RuleSetList& getRuleSets(bool bReload);
	void setRuleSets(RuleSetList& listRuleSet);
	bool apply(Folder* pFolder,
			   Document* pDocument,
			   HWND hwnd,
			   qs::Profile* pProfile,
			   unsigned int nSecurityMode,
			   RuleCallback* pCallback);
	bool apply(Folder* pFolder,
			   const MessageHolderList& l,
			   Document* pDocument,
			   HWND hwnd,
			   qs::Profile* pProfile,
			   unsigned int nSecurityMode,
			   RuleCallback* pCallback);
	bool apply(Folder* pFolder,
			   MessageHolderList* pList,
			   Document* pDocument,
			   qs::Profile* pProfile,
			   bool bJunkFilter,
			   bool bJunkFilterOnly,
			   RuleCallback* pCallback);
	bool save() const;

public:
	void addRuleSet(std::auto_ptr<RuleSet> pRuleSet);
	void clear();

private:
	RuleManager(const RuleManager&);
	RuleManager& operator=(const RuleManager&);

private:
	class RuleManagerImpl* pImpl_;
};


/****************************************************************************
 *
 * RuleCallback
 *
 */

class QMEXPORTCLASS RuleCallback
{
public:
	virtual ~RuleCallback();

public:
	virtual bool isCanceled() = 0;
	virtual void checkingMessages(Folder* pFolder) = 0;
	virtual void applyingRule(Folder* pFolder) = 0;
	virtual void setRange(unsigned int nMin,
						  unsigned int nMax) = 0;
	virtual void setPos(unsigned int nPos) = 0;
};

}

#endif // __QMRULE_H__
