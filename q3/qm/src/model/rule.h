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
	class DeleteRule;
	class ApplyRule;
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
	bool apply(Folder* pFolder,
			   const MessageHolderList* pList,
			   Document* pDocument,
			   HWND hwnd,
			   qs::Profile* pProfile,
			   bool bDecryptVerify,
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
	virtual void checkingMessages(Folder* pFolder) = 0;
	virtual void applyingRule(Folder* pFolder) = 0;
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

public:
	void setTemplate(const WCHAR* pwszName);
	void addTemplateArgument(qs::wstring_ptr wstrName,
							 qs::wstring_ptr wstrValue);

private:
	CopyRule(const CopyRule&);
	CopyRule& operator=(const CopyRule&);

private:
	typedef std::vector<std::pair<qs::WSTRING, qs::WSTRING> > ArgumentList;

private:
	qs::wstring_ptr wstrAccount_;
	qs::wstring_ptr wstrFolder_;
	bool bMove_;
	qs::wstring_ptr wstrTemplate_;
	ArgumentList listArgument_;
};


/****************************************************************************
 *
 * DeleteRule
 *
 */

class DeleteRule : public Rule
{
public:
	DeleteRule(std::auto_ptr<Macro> pMacro,
			   bool bDirect);
	virtual ~DeleteRule();

public:
	virtual bool apply(const RuleContext& context) const;

private:
	DeleteRule(const DeleteRule&);
	DeleteRule& operator=(const DeleteRule&);

private:
	bool bDirect_;
};


/****************************************************************************
 *
 * ApplyRule
 *
 */

class ApplyRule : public Rule
{
public:
	ApplyRule(std::auto_ptr<Macro> pMacro,
			  std::auto_ptr<Macro> pMacroApply);
	virtual ~ApplyRule();

public:
	virtual bool apply(const RuleContext& context) const;

private:
	ApplyRule(const ApplyRule&);
	ApplyRule& operator=(const ApplyRule&);

private:
	std::auto_ptr<Macro> pMacroApply_;
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
				Account* pAccount,
				Folder* pFolder,
				HWND hwnd,
				qs::Profile* pProfile,
				bool bDecryptVerify);
	~RuleContext();

public:
	const MessageHolderList& getMessageHolderList() const;
	Document* getDocument() const;
	Account* getAccount() const;
	Folder* getFolder() const;
	HWND getWindow() const;
	qs::Profile* getProfile() const;
	bool isDecryptVerify() const;

private:
	RuleContext(const RuleContext&);
	RuleContext& operator=(const RuleContext&);

private:
	const MessageHolderList& listMessageHolder_;
	Document* pDocument_;
	Account* pAccount_;
	Folder* pFolder_;
	HWND hwnd_;
	qs::Profile* pProfile_;
	bool bDecryptVerify_;
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
		STATE_MOVE,
		STATE_TEMPLATE,
		STATE_ARGUMENT,
		STATE_DELETE,
		STATE_APPLY
	};

private:
	RuleManager* pManager_;
	State state_;
	RuleSet* pCurrentRuleSet_;
	CopyRule* pCurrentCopyRule_;
	qs::wstring_ptr wstrTemplateArgumentName_;
	std::auto_ptr<Macro> pMacro_;
	MacroParser parser_;
	qs::StringBuffer<qs::WSTRING> buffer_;
};

}

#endif // __RULE_H__
