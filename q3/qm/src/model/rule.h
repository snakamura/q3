/*
 * $Id$
 *
 * Copyright(C) 1998-2004 Satoshi Nakamura
 * All rights reserved.
 *
 */

#ifndef __RULE_H__
#define __RULE_H__

#include <qm.h>
#include <qmfolder.h>
#include <qmmacro.h>

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
	RuleManager();
	~RuleManager();

public:
	bool apply(const Folder* pFolder,
			   const MessageHolderList* pList,
			   Document* pDocument,
			   HWND hwnd,
			   qs::Profile* pProfile,
			   RuleCallback* pCallback);

public:
	void addRuleSet(std::auto_ptr<RuleSet> pRuleSet);

private:
	bool load();
	void clear();
	const RuleSet* getRuleSet(const Folder* pFolder) const;

private:
	RuleManager(const RuleManager&);
	RuleManager& operator=(const RuleManager&);

private:
	typedef std::vector<RuleSet*> RuleSetList;

private:
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
	virtual void checkingMessages() = 0;
	virtual void applyingRule() = 0;
	virtual void setRange(unsigned int nMin,
						  unsigned int nMax) = 0;
	virtual void setPos(unsigned int nPos) = 0;
};


/****************************************************************************
 *
 * RuleSet
 *
 */

class RuleSet
{
public:
	RuleSet(std::auto_ptr<qs::RegexPattern> pAccountName,
			std::auto_ptr<qs::RegexPattern> pFolderName);
	~RuleSet();

public:
	bool matchName(const Folder* pFolder) const;
	size_t getCount() const;
	const Rule* getRule(size_t nIndex) const;

public:
	void addRule(std::auto_ptr<Rule> pRule);

private:
	RuleSet(const RuleSet&);
	RuleSet& operator=(const RuleSet&);

private:
	typedef std::vector<Rule*> RuleList;

private:
	std::auto_ptr<qs::RegexPattern> pAccountName_;
	std::auto_ptr<qs::RegexPattern> pFolderName_;
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
	explicit Rule(std::auto_ptr<Macro> pMacro);
	virtual ~Rule();

public:
	bool match(MacroContext* pContext) const;

public:
	virtual bool apply(const RuleContext& context) const = 0;

private:
	Rule(const Rule&);
	Rule& operator=(const Rule&);

private:
	std::auto_ptr<Macro> pMacro_;
};


/****************************************************************************
 *
 * NullRule
 *
 */

class NullRule : public Rule
{
public:
	explicit NullRule(std::auto_ptr<Macro> pMacro);
	virtual ~NullRule();

public:
	virtual bool apply(const RuleContext& context) const;

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
	CopyRule(std::auto_ptr<Macro> pMacro,
			 const WCHAR* pwszAccount,
			 const WCHAR* pwszFolder,
			 bool bMove);
	virtual ~CopyRule();

public:
	virtual bool apply(const RuleContext& context) const;

private:
	CopyRule(const CopyRule&);
	CopyRule& operator=(const CopyRule&);

private:
	qs::wstring_ptr wstrAccount_;
	qs::wstring_ptr wstrFolder_;
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
	RuleContext(const MessageHolderList& l,
				Document* pDocument,
				Account* pAccount);
	~RuleContext();

public:
	const MessageHolderList& getMessageHolderList() const;
	Document* getDocument() const;
	Account* getAccount() const;

private:
	RuleContext(const RuleContext&);
	RuleContext& operator=(const RuleContext&);

private:
	const MessageHolderList& listMessageHolder_;
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
	explicit RuleContentHandler(RuleManager* pManager);
	virtual ~RuleContentHandler();

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
	std::auto_ptr<Macro> pMacro_;
	MacroParser parser_;
};

}

#endif // __RULE_H__
