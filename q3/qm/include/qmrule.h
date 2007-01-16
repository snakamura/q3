/*
 * $Id$
 *
 * Copyright(C) 1998-2007 Satoshi Nakamura
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
	enum AutoFlag {
		AUTOFLAG_NONE			= 0x00,
		AUTOFLAG_JUNKFILTER		= 0x01,
		AUTOFLAG_JUNKFILTERONLY	= 0x02,
		AUTOFLAG_EXISTING		= 0x04
	};

public:
	typedef std::vector<RuleSet*> RuleSetList;

public:
	explicit RuleManager(const WCHAR* pwszPath);
	~RuleManager();

public:
	const RuleSetList& getRuleSets();
	const RuleSetList& getRuleSets(bool bReload);
	void setRuleSets(RuleSetList& listRuleSet);
	bool applyManual(Folder* pFolder,
					 Document* pDocument,
					 HWND hwnd,
					 qs::Profile* pProfile,
					 unsigned int nSecurityMode,
					 UndoItemList* pUndoItemList,
					 RuleCallback* pCallback);
	bool applyManual(Folder* pFolder,
					 const MessageHolderList& l,
					 Document* pDocument,
					 HWND hwnd,
					 qs::Profile* pProfile,
					 unsigned int nSecurityMode,
					 UndoItemList* pUndoItemList,
					 RuleCallback* pCallback);
	bool applyAuto(Folder* pFolder,
				   MessagePtrList* pList,
				   Document* pDocument,
				   qs::Profile* pProfile,
				   unsigned int nAutoFlags,
				   RuleCallback* pCallback);
	bool applyActive(Folder* pFolder,
					 const MessageHolderList& l,
					 Document* pDocument,
					 HWND hwnd,
					 qs::Profile* pProfile,
					 unsigned int nSecurityMode,
					 bool bBackground,
					 unsigned int* pnResultFlags);
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
	virtual void setRange(size_t nMin,
						  size_t nMax) = 0;
	virtual void setPos(size_t nPos) = 0;
};

}

#endif // __QMRULE_H__
