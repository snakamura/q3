/*
 * $Id: rule.h,v 1.1.1.1 2003/04/29 08:07:31 snakamura Exp $
 *
 * Copyright(C) 1998-2003 Satoshi Nakamura
 * All rights reserved.
 *
 */

#ifndef __RULE_H__
#define __RULE_H__

#include <qm.h>
#include <qmfolder.h>

#include <qs.h>
#include <qsprofile.h>
#include <qsregex.h>
#include <qssax.h>
#include <qsstream.h>
#include <qsstring.h>

#include <vector>


namespace qm {

class RuleManager;
class RuleCallback;
class RuleSet;
class Rule;
	class NullRule;
	class CopyRule;
class RuleContext;
class RuleContentHandler;

class Account;
class Document;
class Macro;
class MacroContext;
class MacroParser;
class NormalFolder;


/****************************************************************************
 *
 * RuleManager
 *
 */

class RuleManager
{
public:
	RuleManager(const WCHAR* pwszPath, qs::QSTATUS* pstatus);
	~RuleManager();

public:
	qs::QSTATUS apply(NormalFolder* pFolder, Document* pDocument,
		HWND hwnd, qs::Profile* pProfile, RuleCallback* pCallback);

public:
	qs::QSTATUS addRuleSet(RuleSet* pRuleSet);

private:
	qs::QSTATUS load();
	void clear();
	qs::QSTATUS getRuleSet(NormalFolder* pFolder,
		const RuleSet** ppRuleSet) const;

private:
	RuleManager(const RuleManager&);
	RuleManager& operator=(const RuleManager&);

private:
	typedef std::vector<RuleSet*> RuleSetList;

private:
	qs::WSTRING wstrPath_;
	FILETIME ft_;
	RuleSetList listRuleSet_;
};


/****************************************************************************
 *
 * RuleCallback
 *
 */

class RuleCallback
{
public:
	virtual ~RuleCallback();

public:
	virtual bool isCanceled() = 0;
	virtual qs::QSTATUS checkingMessages() = 0;
	virtual qs::QSTATUS applyingRule() = 0;
	virtual qs::QSTATUS setRange(unsigned int nMin, unsigned int nMax) = 0;
	virtual qs::QSTATUS setPos(unsigned int nPos) = 0;
};


/****************************************************************************
 *
 * RuleSet
 *
 */

class RuleSet
{
public:
	RuleSet(const WCHAR* pwszAccount,
		const WCHAR* pwszFolder, qs::QSTATUS* pstatus);
	~RuleSet();

public:
	qs::QSTATUS matchName(const NormalFolder* pFolder, bool* pbMatch) const;
	size_t getCount() const;
	const Rule* getRule(size_t nIndex) const;

public:
	qs::QSTATUS addRule(Rule* pRule);

private:
	RuleSet(const RuleSet&);
	RuleSet& operator=(const RuleSet&);

private:
	typedef std::vector<Rule*> RuleList;

private:
	qs::RegexPattern* pAccountName_;
	qs::RegexPattern* pFolderName_;
	RuleList listRule_;
};


/****************************************************************************
 *
 * Rule
 *
 */

class Rule
{
public:
	Rule(Macro* pMacro, qs::QSTATUS* pstatus);
	virtual ~Rule();

public:
	qs::QSTATUS match(MacroContext* pContext, bool* pbMatch) const;

public:
	virtual qs::QSTATUS apply(const RuleContext& context) const = 0;

private:
	Rule(const Rule&);
	Rule& operator=(const Rule&);

private:
	Macro* pMacro_;
};


/****************************************************************************
 *
 * NullRule
 *
 */

class NullRule : public Rule
{
public:
	NullRule(Macro* pMacro, qs::QSTATUS* pstatus);
	virtual ~NullRule();

public:
	virtual qs::QSTATUS apply(const RuleContext& context) const;

private:
	NullRule(const NullRule&);
	NullRule& operator=(const NullRule&);
};


/****************************************************************************
 *
 * CopyRule
 *
 */

class CopyRule : public Rule
{
public:
	CopyRule(Macro* pMacro, const WCHAR* pwszAccount,
		const WCHAR* pwszFolder, bool bMove, qs::QSTATUS* pstatus);
	virtual ~CopyRule();

public:
	virtual qs::QSTATUS apply(const RuleContext& context) const;

private:
	CopyRule(const CopyRule&);
	CopyRule& operator=(const CopyRule&);

private:
	qs::WSTRING wstrAccount_;
	qs::WSTRING wstrFolder_;
	bool bMove_;
};


/****************************************************************************
 *
 * RuleContext
 *
 */

class RuleContext
{
public:
	RuleContext(Folder* pFolder, const Folder::MessageHolderList& l,
		Document* pDocument, Account* pAccount);
	~RuleContext();

public:
	Folder* getFolder() const;
	const Folder::MessageHolderList& getMessageHolderList() const;
	Document* getDocument() const;
	Account* getAccount() const;

private:
	RuleContext(const RuleContext&);
	RuleContext& operator=(const RuleContext&);

private:
	Folder* pFolder_;
	const Folder::MessageHolderList& listMessageHolder_;
	Document* pDocument_;
	Account* pAccount_;
};


/****************************************************************************
 *
 * RuleContentHandler
 *
 */

class RuleContentHandler : public qs::DefaultHandler
{
public:
	RuleContentHandler(RuleManager* pManager, qs::QSTATUS* pstatus);
	virtual ~RuleContentHandler();

public:
	virtual qs::QSTATUS startElement(const WCHAR* pwszNamespaceURI,
		const WCHAR* pwszLocalName, const WCHAR* pwszQName,
		const qs::Attributes& attributes);
	virtual qs::QSTATUS endElement(const WCHAR* pwszNamespaceURI,
		const WCHAR* pwszLocalName, const WCHAR* pwszQName);
	virtual qs::QSTATUS characters(const WCHAR* pwsz,
		size_t nStart, size_t nLength);

private:
	RuleContentHandler(const RuleContentHandler&);
	RuleContentHandler& operator=(const RuleContentHandler&);

private:
	enum State {
		STATE_ROOT,
		STATE_RULES,
		STATE_RULESET,
		STATE_RULE,
		STATE_MOVE
	};

private:
	RuleManager* pManager_;
	State state_;
	RuleSet* pCurrentRuleSet_;
	Macro* pMacro_;
	MacroParser* pParser_;
};

}

#endif // __RULE_H__
