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
#include <qmrule.h>

#include <qs.h>
#include <qsprofile.h>
#include <qsregex.h>
#include <qssax.h>
#include <qsstream.h>
#include <qsstring.h>

#include <vector>

#include "term.h"
#include "../util/confighelper.h"


namespace qm {

class RuleSet;
class Rule;
class RuleAction;
	class NullRuleAction;
	class CopyRuleAction;
	class DeleteRuleAction;
	class LabelRuleAction;
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
 * RuleSet
 *
 */

class RuleSet
{
public:
	typedef std::vector<Rule*> RuleList;

public:
	RuleSet();
	RuleSet(Term& account,
			Term& folder);
	RuleSet(const RuleSet& ruleset);
	~RuleSet();

public:
	const WCHAR* getAccount() const;
	void setAccount(Term& account);
	const WCHAR* getFolder() const;
	void setFolder(Term& folder);
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
	Term account_;
	Term folder_;
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
	enum Use {
		USE_MANUAL	= 0x01,
		USE_AUTO	= 0x02
	};

public:
	Rule();
	Rule(std::auto_ptr<Macro> pCondition,
		 std::auto_ptr<RuleAction> pAction,
		 unsigned int nUse,
		 bool bContinue,
		 const WCHAR* pwszDescription);
	Rule(const Rule& rule);
	virtual ~Rule();

public:
	const Macro* getCondition() const;
	void setCondition(std::auto_ptr<Macro> pCondition);
	RuleAction* getAction() const;
	void setAction(std::auto_ptr<RuleAction> pAction);
	bool isUse(Use use) const;
	unsigned int getUse() const;
	void setUse(unsigned int nUse);
	bool isContinue() const;
	void setContinue(bool bContinue);
	const WCHAR* getDescription() const;
	void setDescription(const WCHAR* pwszDescription);
	bool match(MacroContext* pContext) const;
	bool apply(const RuleContext& context) const;
	bool isMessageDestroyed() const;
	bool isContinuable() const;
	MacroContext::MessageType getMessageType() const;

private:
	Rule& operator=(const Rule&);

private:
	std::auto_ptr<Macro> pCondition_;
	std::auto_ptr<RuleAction> pAction_;
	unsigned int nUse_;
	bool bContinue_;
	qs::wstring_ptr wstrDescription_;
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
		TYPE_NONE,
		TYPE_MOVE,
		TYPE_COPY,
		TYPE_DELETE,
		TYPE_LABEL,
		TYPE_DELETECACHE,
		TYPE_APPLY
	};
	
	enum Flag {
		FLAG_NONE				= 0x00,
		FLAG_MESSAGEDESTROYED	= 0x01,
		FLAG_CONTINUABLE		= 0x02
	};

public:
	virtual ~RuleAction();

public:
	virtual Type getType() const = 0;
	virtual bool apply(const RuleContext& context) const = 0;
	virtual unsigned int getFlags() const = 0;
	virtual qs::wstring_ptr getDescription() const = 0;
	virtual std::auto_ptr<RuleAction> clone() const = 0;
};


/****************************************************************************
 *
 * NoneRuleAction
 *
 */

class NoneRuleAction : public RuleAction
{
public:
	NoneRuleAction();
	virtual ~NoneRuleAction();

public:
	virtual Type getType() const;
	virtual bool apply(const RuleContext& context) const;
	virtual unsigned int getFlags() const;
	virtual qs::wstring_ptr getDescription() const;
	virtual std::auto_ptr<RuleAction> clone() const;

private:
	NoneRuleAction(const NoneRuleAction&);
	NoneRuleAction& operator=(const NoneRuleAction&);
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
	virtual unsigned int getFlags() const;
	virtual qs::wstring_ptr getDescription() const;
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
	virtual unsigned int getFlags() const;
	virtual qs::wstring_ptr getDescription() const;
	virtual std::auto_ptr<RuleAction> clone() const;

private:
	DeleteRuleAction& operator=(const DeleteRuleAction&);

private:
	bool bDirect_;
};


/****************************************************************************
 *
 * LabelRuleAction
 *
 */

class LabelRuleAction : public RuleAction
{
public:
	enum LabelType {
		LABELTYPE_SET,
		LABELTYPE_ADD,
		LABELTYPE_REMOVE
	};

public:
	LabelRuleAction(LabelType type,
					const WCHAR* pwszLabel);

private:
	LabelRuleAction(const LabelRuleAction& action);

public:
	virtual ~LabelRuleAction();

public:
	LabelType getLabelType() const;
	const WCHAR* getLabel() const;

public:
	virtual Type getType() const;
	virtual bool apply(const RuleContext& context) const;
	virtual unsigned int getFlags() const;
	virtual qs::wstring_ptr getDescription() const;
	virtual std::auto_ptr<RuleAction> clone() const;

private:
	LabelRuleAction& operator=(const LabelRuleAction&);

private:
	LabelType type_;
	qs::wstring_ptr wstrLabel_;
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
	virtual unsigned int getFlags() const;
	virtual qs::wstring_ptr getDescription() const;
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
	virtual unsigned int getFlags() const;
	virtual qs::wstring_ptr getDescription() const;
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
				bool bAuto,
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
	bool isAuto() const;
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
	bool bAuto_;
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
		STATE_LABEL,
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
	unsigned int nUse_;
	bool bContinue_;
	qs::wstring_ptr wstrDescription_;
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
	bool write(const LabelRuleAction* pAction);
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
