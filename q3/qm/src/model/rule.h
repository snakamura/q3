/*
 * $Id$
 *
 * Copyright(C) 1998-2005 Satoshi Nakamura
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

#include "../util/confighelper.h"

namespace qm {

class RuleManager;
class RuleCallback;
class RuleSet;
class Rule;
class RuleAction;
	class NullRuleAction;
	class CopyRuleAction;
	class DeleteRuleAction;
	class DeleteCacheRuleAction;
	class ApplyRuleAction;
class RuleContext;
class RuleContentHandler;
class RuleWriter;

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
	typedef std::vector<RuleSet*> RuleSetList;
	typedef std::vector<Rule*> RuleList;

public:
	explicit RuleManager(const WCHAR* pwszPath);
	~RuleManager();

public:
	const RuleSetList& getRuleSets();
	const RuleSetList& getRuleSets(bool bReload);
	void setRuleSets(RuleSetList& listRuleSet);
	bool apply(Folder* pFolder,
			   const MessageHolderList* pList,
			   Document* pDocument,
			   HWND hwnd,
			   qs::Profile* pProfile,
			   unsigned int nSecurityMode,
			   RuleCallback* pCallback);
	bool save() const;

public:
	void addRuleSet(std::auto_ptr<RuleSet> pRuleSet);
	void clear();

private:
	bool load();
	void getRules(const Folder* pFolder,
				  RuleList* pList) const;

private:
	RuleManager(const RuleManager&);
	RuleManager& operator=(const RuleManager&);

private:
	RuleSetList listRuleSet_;
	ConfigHelper<RuleManager, RuleContentHandler, RuleWriter> helper_;
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
	typedef std::vector<Rule*> RuleList;

public:
	RuleSet();
	RuleSet(const WCHAR* pwszAccount,
			std::auto_ptr<qs::RegexPattern> pAccount,
			const WCHAR* pwszFolder,
			std::auto_ptr<qs::RegexPattern> pFolder);
	RuleSet(const RuleSet& ruleset);
	~RuleSet();

public:
	const WCHAR* getAccount() const;
	void setAccount(const WCHAR* pwszAccount,
					std::auto_ptr<qs::RegexPattern> pAccount);
	const WCHAR* getFolder() const;
	void setFolder(const WCHAR* pwszFolder,
				   std::auto_ptr<qs::RegexPattern> pFolder);
	bool matchName(const Folder* pFolder) const;
	const RuleList& getRules() const;
	void setRules(RuleList& listRule);

public:
	void addRule(std::auto_ptr<Rule> pRule);

private:
	void clear();

private:
	RuleSet& operator=(const RuleSet&);

private:
	qs::wstring_ptr wstrAccount_;
	std::auto_ptr<qs::RegexPattern> pAccount_;
	qs::wstring_ptr wstrFolder_;
	std::auto_ptr<qs::RegexPattern> pFolder_;
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
	Rule();
	Rule(std::auto_ptr<Macro> pCondition,
		 std::auto_ptr<RuleAction> pAction);
	Rule(const Rule& rule);
	virtual ~Rule();

public:
	const Macro* getCondition() const;
	void setCondition(std::auto_ptr<Macro> pCondition);
	RuleAction* getAction() const;
	void setAction(std::auto_ptr<RuleAction> pAction);
	bool match(MacroContext* pContext) const;
	bool apply(const RuleContext& context) const;

private:
	Rule& operator=(const Rule&);

private:
	std::auto_ptr<Macro> pCondition_;
	std::auto_ptr<RuleAction> pAction_;
};


/****************************************************************************
 *
 * RuleAction
 *
 */

class RuleAction
{
public:
	enum Type {
		TYPE_MOVE,
		TYPE_COPY,
		TYPE_DELETE,
		TYPE_DELETECACHE,
		TYPE_APPLY
	};

public:
	virtual ~RuleAction();

public:
	virtual Type getType() const = 0;
	virtual bool apply(const RuleContext& context) const = 0;
	virtual std::auto_ptr<RuleAction> clone() const = 0;
};


/****************************************************************************
 *
 * CopyRuleAction
 *
 */

class CopyRuleAction : public RuleAction
{
public:
	typedef std::vector<std::pair<qs::WSTRING, qs::WSTRING> > ArgumentList;

public:
	CopyRuleAction(const WCHAR* pwszAccount,
				   const WCHAR* pwszFolder,
				   bool bMove);

private:
	CopyRuleAction(const CopyRuleAction& action);

public:
	virtual ~CopyRuleAction();

public:
	const WCHAR* getAccount() const;
	const WCHAR* getFolder() const;
	const WCHAR* getTemplate() const;
	const ArgumentList& getArguments() const;

public:
	virtual Type getType() const;
	virtual bool apply(const RuleContext& context) const;
	virtual std::auto_ptr<RuleAction> clone() const;

public:
	void setTemplate(const WCHAR* pwszName);
	void setTemplateArguments(ArgumentList& listArgument);
	void addTemplateArgument(qs::wstring_ptr wstrName,
							 qs::wstring_ptr wstrValue);

private:
	void clearTemplateArguments();

private:
	CopyRuleAction& operator=(const CopyRuleAction&);

private:
	qs::wstring_ptr wstrAccount_;
	qs::wstring_ptr wstrFolder_;
	bool bMove_;
	qs::wstring_ptr wstrTemplate_;
	ArgumentList listArgument_;
};


/****************************************************************************
 *
 * DeleteRuleAction
 *
 */

class DeleteRuleAction : public RuleAction
{
public:
	explicit DeleteRuleAction(bool bDirect);

private:
	DeleteRuleAction(const DeleteRuleAction& action);

public:
	virtual ~DeleteRuleAction();

public:
	bool isDirect() const;

public:
	virtual Type getType() const;
	virtual bool apply(const RuleContext& context) const;
	virtual std::auto_ptr<RuleAction> clone() const;

private:
	DeleteRuleAction& operator=(const DeleteRuleAction&);

private:
	bool bDirect_;
};


/****************************************************************************
 *
 * DeleteCacheRuleAction
 *
 */

class DeleteCacheRuleAction : public RuleAction
{
public:
	DeleteCacheRuleAction();

private:
	DeleteCacheRuleAction(const DeleteCacheRuleAction& action);

public:
	virtual ~DeleteCacheRuleAction();

public:
	virtual Type getType() const;
	virtual bool apply(const RuleContext& context) const;
	virtual std::auto_ptr<RuleAction> clone() const;

private:
	DeleteCacheRuleAction& operator=(const DeleteCacheRuleAction&);
};


/****************************************************************************
 *
 * ApplyRuleAction
 *
 */

class ApplyRuleAction : public RuleAction
{
public:
	explicit ApplyRuleAction(std::auto_ptr<Macro> pMacro);

private:
	ApplyRuleAction(const ApplyRuleAction& action);

public:
	virtual ~ApplyRuleAction();

public:
	const Macro* getMacro() const;

public:
	virtual Type getType() const;
	virtual bool apply(const RuleContext& context) const;
	virtual std::auto_ptr<RuleAction> clone() const;

private:
	ApplyRuleAction& operator=(const ApplyRuleAction&);

private:
	std::auto_ptr<Macro> pMacro_;
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
				MacroVariableHolder* pGlobalVariable,
				unsigned int nSecurityMode,
				UndoItemList* pUndoItemList);
	~RuleContext();

public:
	const MessageHolderList& getMessageHolderList() const;
	Document* getDocument() const;
	Account* getAccount() const;
	Folder* getFolder() const;
	HWND getWindow() const;
	qs::Profile* getProfile() const;
	MacroVariableHolder* getGlobalVariable() const;
	unsigned int getSecurityMode() const;
	UndoItemList* getUndoItemList() const;

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
	MacroVariableHolder* pGlobalVariable_;
	unsigned int nSecurityMode_;
	UndoItemList* pUndoItemList_;
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
		STATE_DELETECACHE,
		STATE_APPLY
	};

private:
	RuleManager* pManager_;
	State state_;
	RuleSet* pCurrentRuleSet_;
	CopyRuleAction* pCurrentCopyRuleAction_;
	qs::wstring_ptr wstrTemplateArgumentName_;
	std::auto_ptr<Macro> pCondition_;
	MacroParser parser_;
	qs::StringBuffer<qs::WSTRING> buffer_;
};


/****************************************************************************
 *
 * RuleWriter
 *
 */

class RuleWriter
{
public:
	explicit RuleWriter(qs::Writer* pWriter);
	~RuleWriter();

public:
	bool write(const RuleManager* pManager);

private:
	bool write(const RuleSet* pRuleSet);
	bool write(const Rule* pRule);
	bool write(const CopyRuleAction* pAction);
	bool write(const DeleteRuleAction* pAction);
	bool write(const DeleteCacheRuleAction* pAction);
	bool write(const ApplyRuleAction* pAction);

private:
	RuleWriter(const RuleWriter&);
	RuleWriter& operator=(const RuleWriter&);

private:
	qs::OutputHandler handler_;
};

}

#endif // __RULE_H__
