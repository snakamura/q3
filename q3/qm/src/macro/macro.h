/*
 * $Id$
 *
 * Copyright(C) 1998-2003 Satoshi Nakamura
 * All rights reserved.
 *
 */

#ifndef __MACRO_H__
#define __MACRO_H__

#include <qmmacro.h>

#include <vector>
#include <utility>


namespace qm {

class MacroTokenizer;
class MacroGlobalContext;
class MacroFunctionHolder;
class MacroArgumentHolder;
class MacroExpr;
	class MacroField;
	class MacroFieldCache;
	class MacroLiteral;
	class MacroNumber;
	class MacroVariable;
	class MacroFunction;
		class MacroFunctionAccount;
		class MacroFunctionAccountDirectory;
		class MacroFunctionAdditive;
		class MacroFunctionAddress;
		class MacroFunctionAddressBook;
		class MacroFunctionAnd;
		class MacroFunctionAttachment;
		class MacroFunctionBody;
		class MacroFunctionBoolean;
		class MacroFunctionClipboard;
		class MacroFunctionComputerName;
		class MacroFunctionConcat;
		class MacroFunctionContain;
		class MacroFunctionCopy;
		class MacroFunctionDate;
		class MacroFunctionDecode;
		class MacroFunctionDefun;
		class MacroFunctionDelete;
		class MacroFunctionEqual;
		class MacroFunctionEval;
		class MacroFunctionExecute;
		class MacroFunctionExist;
		class MacroFunctionExit;
		class MacroFunctionField;
		class MacroFunctionFieldParameter;
		class MacroFunctionFind;
		class MacroFunctionFlag;
		class MacroFunctionFolder;
		class MacroFunctionForEach;
		class MacroFunctionFormatDate;
		class MacroFunctionFunction;
		class MacroFunctionHeader;
		class MacroFunctionI;
		class MacroFunctionId;
		class MacroFunctionIdentity;
		class MacroFunctionIf;
		class MacroFunctionInputBox;
		class MacroFunctionLength;
		class MacroFunctionLoad;
		class MacroFunctionMessageBox;
		class MacroFunctionMessages;
		class MacroFunctionNot;
		class MacroFunctionOr;
		class MacroFunctionOSVersion;
		class MacroFunctionParseURL;
		class MacroFunctionPart;
		class MacroFunctionPassed;
		class MacroFunctionProcessId;
		class MacroFunctionProfile;
		class MacroFunctionProfileName;
		class MacroFunctionProgn;
		class MacroFunctionReferences;
		class MacroFunctionRelative;
		class MacroFunctionRemove;
		class MacroFunctionSave;
		class MacroFunctionScript;
		class MacroFunctionSet;
		class MacroFunctionSize;
		class MacroFunctionSubAccount;
		class MacroFunctionSubject;
		class MacroFunctionSubstring;
		class MacroFunctionSubstringSep;
		class MacroFunctionVariable;
		class MacroFunctionWhile;
class MacroExprPtr;
class MacroFunctionFactory;


/****************************************************************************
 *
 * MacroTokenizer
 *
 */

class MacroTokenizer
{
public:
	enum Token
	{
		TOKEN_TEXT,
		TOKEN_LITERAL,
		TOKEN_AT,
		TOKEN_DOLLAR,
		TOKEN_PERCENT,
		TOKEN_LEFTPARENTHESIS,
		TOKEN_RIGHTPARENTHESIS,
		TOKEN_COMMA,
		TOKEN_END,
		TOKEN_ERROR
	};

public:
	MacroTokenizer(const WCHAR* pwszMacro, qs::QSTATUS* pstatus);
	~MacroTokenizer();

public:
	qs::QSTATUS getToken(Token* pToken, qs::WSTRING* pwstrToken);
	const WCHAR* getLastPosition() const;

private:
	MacroTokenizer(const MacroTokenizer&);
	MacroTokenizer& operator=(const MacroTokenizer&);

private:
	const WCHAR* pwszMacro_;
	const WCHAR* p_;
	const WCHAR* pLast_;
};


/****************************************************************************
 *
 * MacroGlobalContext
 *
 */

class MacroGlobalContext
{
public:
	struct Init
	{
		Document* pDocument_;
		HWND hwnd_;
		qs::Profile* pProfile_;
		bool bGetMessageAsPossible_;
		MacroErrorHandler* pErrorHandler_;
		MacroVariableHolder* pGlobalVariable_;
	};

public:
	MacroGlobalContext(const Init& init, qs::QSTATUS* pstatus);
	~MacroGlobalContext();

public:
	Document* getDocument() const;
	HWND getWindow() const;
	qs::Profile* getProfile() const;
	bool isGetMessageAsPossible() const;
	MacroErrorHandler* getErrorHandler() const;
	qs::QSTATUS getVariable(const WCHAR* pwszName,
		MacroValue** ppValue) const;
	qs::QSTATUS setVariable(const WCHAR* pwszName,
		MacroValue* pValue, bool bGlobal);
	qs::QSTATUS getFunction(const WCHAR* pwszName,
		const MacroExpr** ppExpr) const;
	qs::QSTATUS setFunction(const WCHAR* pwszName,
		const MacroExpr* pExpr, bool* pbSet);
	qs::QSTATUS pushArgumentContext();
	void popArgumentContext();
	qs::QSTATUS addArgument(MacroValue* pValue);
	qs::QSTATUS getArgument(unsigned int n, MacroValue** ppValue) const;

private:
	MacroGlobalContext(const MacroGlobalContext&);
	MacroGlobalContext& operator=(const MacroGlobalContext&);

private:
	Document* pDocument_;
	HWND hwnd_;
	qs::Profile* pProfile_;
	bool bGetMessageAsPossible_;
	MacroErrorHandler* pErrorHandler_;
	MacroVariableHolder* pVariable_;
	MacroVariableHolder* pGlobalVariable_;
	MacroFunctionHolder* pFunction_;
	MacroArgumentHolder* pArgument_;
};


/****************************************************************************
 *
 * MacroFunctionHolder
 *
 */

class MacroFunctionHolder
{
public:
	explicit MacroFunctionHolder(qs::QSTATUS* pstatus);
	~MacroFunctionHolder();

public:
	qs::QSTATUS getFunction(const WCHAR* pwszName,
		const MacroExpr** ppExpr) const;
	qs::QSTATUS setFunction(const WCHAR* pwszName,
		const MacroExpr* pExpr, bool* pbSet);

private:
	MacroFunctionHolder(const MacroFunctionHolder&);
	MacroFunctionHolder& operator=(const MacroFunctionHolder&);

private:
	typedef std::vector<std::pair<qs::WSTRING, const MacroExpr*> > FunctionMap;

private:
	FunctionMap mapFunction_;
};


/****************************************************************************
 *
 * MacroArgumentHolder
 *
 */

class MacroArgumentHolder
{
public:
	explicit MacroArgumentHolder(qs::QSTATUS* pstatus);
	~MacroArgumentHolder();

public:
	qs::QSTATUS pushContext();
	void popContext();
	qs::QSTATUS addArgument(MacroValue* pValue);
	qs::QSTATUS getArgument(unsigned int n, MacroValue** ppValue) const;

private:
	MacroArgumentHolder(const MacroArgumentHolder&);
	MacroArgumentHolder& operator=(const MacroArgumentHolder&);

private:
	typedef std::vector<std::vector<MacroValue*> > ArgumentList;

private:
	ArgumentList listArgument_;
};


/****************************************************************************
 *
 * MacroExpr
 *
 */

class MacroExpr
{
public:
	virtual ~MacroExpr();

public:
	virtual qs::QSTATUS value(MacroContext* pContext,
		MacroValue** ppValue) const = 0;
	virtual qs::QSTATUS getString(qs::WSTRING* pwstrExpr) const = 0;
	virtual void release();

protected:
	qs::QSTATUS error(const MacroContext& context,
		MacroErrorHandler::Code code) const;
};


/****************************************************************************
 *
 * MacroField
 *
 */

class MacroField : public MacroExpr
{
public:
	MacroField(const WCHAR* pwszName, qs::QSTATUS* pstatus);
	virtual ~MacroField();

public:
	virtual qs::QSTATUS value(MacroContext* pContext,
		MacroValue** ppValue) const;
	virtual qs::QSTATUS getString(qs::WSTRING* pwstrExpr) const;

private:
	MacroField(const MacroField&);
	MacroField& operator=(const MacroField&);

private:
	qs::WSTRING wstrName_;
};


/****************************************************************************
 *
 * MacroFieldCache
 *
 */

class MacroFieldCache : public MacroExpr
{
public:
	MacroFieldCache(const WCHAR* pwszName, qs::QSTATUS* pstatus);
	virtual ~MacroFieldCache();

public:
	virtual qs::QSTATUS value(MacroContext* pContext,
		MacroValue** ppValue) const;
	virtual qs::QSTATUS getString(qs::WSTRING* pwstrExpr) const;

private:
	MacroFieldCache(const MacroFieldCache&);
	MacroFieldCache& operator=(const MacroFieldCache&);

private:
	enum Type {
		TYPE_ID,
		TYPE_DATE,
		TYPE_FROM,
		TYPE_TO,
		TYPE_FROMTO,
		TYPE_SUBJECT,
		TYPE_SIZE
	};

private:
	Type type_;
};


/****************************************************************************
 *
 * MacroLiteral
 *
 */

class MacroLiteral : public MacroExpr
{
public:
	MacroLiteral(const WCHAR* pwszValue, qs::QSTATUS* pstatus);
	virtual ~MacroLiteral();

public:
	virtual qs::QSTATUS value(MacroContext* pContext,
		MacroValue** ppValue) const;
	virtual qs::QSTATUS getString(qs::WSTRING* pwstrExpr) const;

private:
	MacroLiteral(const MacroLiteral&);
	MacroLiteral& operator=(const MacroLiteral&);

private:
	qs::WSTRING wstrValue_;
};


/****************************************************************************
 *
 * MacroNumber
 *
 */

class MacroNumber : public MacroExpr
{
public:
	MacroNumber(long nValue, qs::QSTATUS* pstatus);
	virtual ~MacroNumber();

public:
	virtual qs::QSTATUS value(MacroContext* pContext,
		MacroValue** ppValue) const;
	virtual qs::QSTATUS getString(qs::WSTRING* pwstrExpr) const;

private:
	MacroNumber(const MacroNumber&);
	MacroNumber& operator=(const MacroNumber&);

private:
	long nValue_;
};


/****************************************************************************
 *
 * MacroVariable
 *
 */

class MacroVariable : public MacroExpr
{
public:
	MacroVariable(const WCHAR* pwszName, qs::QSTATUS* pstatus);
	virtual ~MacroVariable();

public:
	virtual qs::QSTATUS value(MacroContext* pContext,
		MacroValue** ppValue) const;
	virtual qs::QSTATUS getString(qs::WSTRING* pwstrExpr) const;

private:
	MacroVariable(const MacroVariable&);
	MacroVariable& operator=(const MacroVariable&);

private:
	qs::WSTRING wstrName_;
	unsigned int n_;
};


/****************************************************************************
 *
 * MacroFunction
 *
 */

class MacroFunction : public MacroExpr
{
public:
	explicit MacroFunction(qs::QSTATUS* pstatus);
	virtual ~MacroFunction();

public:
	qs::QSTATUS addArg(MacroExpr* pArg);

public:
	virtual qs::QSTATUS getString(qs::WSTRING* pwstrExpr) const;
	virtual void release();

protected:
	virtual const WCHAR* getName() const = 0;

protected:
	size_t getArgSize() const;
	const MacroExpr* getArg(size_t n) const;
	qs::QSTATUS getPart(MacroContext* pContext,
		size_t n, const qs::Part** ppPart) const;

private:
	qs::QSTATUS getArgString(qs::WSTRING* pwstrArg) const;

private:
	MacroFunction(const MacroFunction&);
	MacroFunction& operator=(const MacroFunction&);

private:
	typedef std::vector<MacroExpr*> ArgList;

private:
	ArgList listArg_;
};


/****************************************************************************
 *
 * MacroFunctionAccount
 *
 */

class MacroFunctionAccount : public MacroFunction
{
public:
	MacroFunctionAccount(qs::QSTATUS* pstatus);
	virtual ~MacroFunctionAccount();

public:
	virtual qs::QSTATUS value(MacroContext* pContext,
		MacroValue** ppValue) const;

protected:
	virtual const WCHAR* getName() const;

private:
	MacroFunctionAccount(const MacroFunctionAccount&);
	MacroFunctionAccount& operator=(const MacroFunctionAccount&);
};


/****************************************************************************
 *
 * MacroFunctionAccountDirectory
 *
 */

class MacroFunctionAccountDirectory : public MacroFunction
{
public:
	MacroFunctionAccountDirectory(qs::QSTATUS* pstatus);
	virtual ~MacroFunctionAccountDirectory();

public:
	virtual qs::QSTATUS value(MacroContext* pContext,
		MacroValue** ppValue) const;

protected:
	virtual const WCHAR* getName() const;

private:
	MacroFunctionAccountDirectory(const MacroFunctionAccountDirectory&);
	MacroFunctionAccountDirectory& operator=(const MacroFunctionAccountDirectory&);
};


/****************************************************************************
 *
 * MacroFunctionAdditive
 *
 */

class MacroFunctionAdditive : public MacroFunction
{
public:
	MacroFunctionAdditive(bool bAdd, qs::QSTATUS* pstatus);
	virtual ~MacroFunctionAdditive();

public:
	virtual qs::QSTATUS value(MacroContext* pContext,
		MacroValue** ppValue) const;

protected:
	virtual const WCHAR* getName() const;

private:
	MacroFunctionAdditive(const MacroFunctionAdditive&);
	MacroFunctionAdditive& operator=(const MacroFunctionAdditive&);

private:
	bool bAdd_;
};


/****************************************************************************
 *
 * MacroFunctionAddress
 *
 */

class MacroFunctionAddress : public MacroFunction
{
public:
	MacroFunctionAddress(bool bName, qs::QSTATUS* pstatus);
	virtual ~MacroFunctionAddress();

public:
	virtual qs::QSTATUS value(MacroContext* pContext,
		MacroValue** ppValue) const;

protected:
	virtual const WCHAR* getName() const;

private:
	MacroFunctionAddress(const MacroFunctionAddress&);
	MacroFunctionAddress& operator=(const MacroFunctionAddress&);

private:
	bool bName_;
};


/****************************************************************************
 *
 * MacroFunctionAddressBook
 *
 */

class MacroFunctionAddressBook : public MacroFunction
{
public:
	MacroFunctionAddressBook(qs::QSTATUS* pstatus);
	virtual ~MacroFunctionAddressBook();

public:
	virtual qs::QSTATUS value(MacroContext* pContext,
		MacroValue** ppValue) const;

protected:
	virtual const WCHAR* getName() const;

private:
	MacroFunctionAddressBook(const MacroFunctionAddressBook&);
	MacroFunctionAddressBook& operator=(const MacroFunctionAddressBook&);
};


/****************************************************************************
 *
 * MacroFunctionAnd
 *
 */

class MacroFunctionAnd : public MacroFunction
{
public:
	explicit MacroFunctionAnd(qs::QSTATUS* pstatus);
	virtual ~MacroFunctionAnd();

public:
	virtual qs::QSTATUS value(MacroContext* pContext,
		MacroValue** ppValue) const;

protected:
	virtual const WCHAR* getName() const;

private:
	MacroFunctionAnd(const MacroFunctionAnd&);
	MacroFunctionAnd& operator=(const MacroFunctionAnd&);
};


/****************************************************************************
 *
 * MacroFunctionAttachment
 *
 */

class MacroFunctionAttachment : public MacroFunction
{
public:
	explicit MacroFunctionAttachment(qs::QSTATUS* pstatus);
	virtual ~MacroFunctionAttachment();

public:
	virtual qs::QSTATUS value(MacroContext* pContext,
		MacroValue** ppValue) const;

protected:
	virtual const WCHAR* getName() const;

private:
	MacroFunctionAttachment(const MacroFunctionAttachment&);
	MacroFunctionAttachment& operator=(const MacroFunctionAttachment&);
};


/****************************************************************************
 *
 * MacroFunctionBody
 *
 */

class MacroFunctionBody : public MacroFunction
{
public:
	explicit MacroFunctionBody(qs::QSTATUS* pstatus);
	virtual ~MacroFunctionBody();

public:
	virtual qs::QSTATUS value(MacroContext* pContext,
		MacroValue** ppValue) const;

protected:
	virtual const WCHAR* getName() const;

private:
	MacroFunctionBody(const MacroFunctionBody&);
	MacroFunctionBody& operator=(const MacroFunctionBody&);
};


/****************************************************************************
 *
 * MacroFunctionBoolean
 *
 */

class MacroFunctionBoolean : public MacroFunction
{
public:
	MacroFunctionBoolean(bool b, qs::QSTATUS* pstatus);
	virtual ~MacroFunctionBoolean();

public:
	virtual qs::QSTATUS value(MacroContext* pContext,
		MacroValue** ppValue) const;

protected:
	virtual const WCHAR* getName() const;

private:
	MacroFunctionBoolean(const MacroFunctionBoolean&);
	MacroFunctionBoolean& operator=(const MacroFunctionBoolean&);

private:
	bool b_;
};


/****************************************************************************
 *
 * MacroFunctionClipboard
 *
 */

class MacroFunctionClipboard : public MacroFunction
{
public:
	explicit MacroFunctionClipboard(qs::QSTATUS* pstatus);
	virtual ~MacroFunctionClipboard();

public:
	virtual qs::QSTATUS value(MacroContext* pContext,
		MacroValue** ppValue) const;

protected:
	virtual const WCHAR* getName() const;

private:
	MacroFunctionClipboard(const MacroFunctionClipboard&);
	MacroFunctionClipboard& operator=(const MacroFunctionClipboard&);
};


/****************************************************************************
 *
 * MacroFunctionComputerName
 *
 */

class MacroFunctionComputerName : public MacroFunction
{
public:
	explicit MacroFunctionComputerName(qs::QSTATUS* pstatus);
	virtual ~MacroFunctionComputerName();

public:
	virtual qs::QSTATUS value(MacroContext* pContext,
		MacroValue** ppValue) const;

protected:
	virtual const WCHAR* getName() const;

private:
	MacroFunctionComputerName(const MacroFunctionComputerName&);
	MacroFunctionComputerName& operator=(const MacroFunctionComputerName&);
};


/****************************************************************************
 *
 * MacroFunctionConcat
 *
 */

class MacroFunctionConcat : public MacroFunction
{
public:
	explicit MacroFunctionConcat(qs::QSTATUS* pstatus);
	virtual ~MacroFunctionConcat();

public:
	virtual qs::QSTATUS value(MacroContext* pContext,
		MacroValue** ppValue) const;

protected:
	virtual const WCHAR* getName() const;

private:
	MacroFunctionConcat(const MacroFunctionConcat&);
	MacroFunctionConcat& operator=(const MacroFunctionConcat&);
};


/****************************************************************************
 *
 * MacroFunctionContain
 *
 */

class MacroFunctionContain : public MacroFunction
{
public:
	MacroFunctionContain(bool bBeginWith, qs::QSTATUS* pstatus);
	virtual ~MacroFunctionContain();

public:
	virtual qs::QSTATUS value(MacroContext* pContext,
		MacroValue** ppValue) const;

protected:
	virtual const WCHAR* getName() const;

private:
	MacroFunctionContain(const MacroFunctionContain&);
	MacroFunctionContain& operator=(const MacroFunctionContain&);

private:
	bool bBeginWith_;
};


/****************************************************************************
 *
 * MacroFunctionCopy
 *
 */

class MacroFunctionCopy : public MacroFunction
{
public:
	MacroFunctionCopy(bool bMove, qs::QSTATUS* pstatus);
	virtual ~MacroFunctionCopy();

public:
	virtual qs::QSTATUS value(MacroContext* pContext,
		MacroValue** ppValue) const;

protected:
	virtual const WCHAR* getName() const;

private:
	MacroFunctionCopy(const MacroFunctionCopy&);
	MacroFunctionCopy& operator=(const MacroFunctionCopy&);

private:
	bool bMove_;
};


/****************************************************************************
 *
 * MacroFunctionDate
 *
 */

class MacroFunctionDate : public MacroFunction
{
public:
	explicit MacroFunctionDate(qs::QSTATUS* pstatus);
	virtual ~MacroFunctionDate();

public:
	virtual qs::QSTATUS value(MacroContext* pContext,
		MacroValue** ppValue) const;

protected:
	virtual const WCHAR* getName() const;

private:
	MacroFunctionDate(const MacroFunctionDate&);
	MacroFunctionDate& operator=(const MacroFunctionDate&);
};


/****************************************************************************
 *
 * MacroFunctionDecode
 *
 */

class MacroFunctionDecode : public MacroFunction
{
public:
	explicit MacroFunctionDecode(qs::QSTATUS* pstatus);
	virtual ~MacroFunctionDecode();

public:
	virtual qs::QSTATUS value(MacroContext* pContext,
		MacroValue** ppValue) const;

protected:
	virtual const WCHAR* getName() const;

private:
	MacroFunctionDecode(const MacroFunctionDecode&);
	MacroFunctionDecode& operator=(const MacroFunctionDecode&);
};


/****************************************************************************
 *
 * MacroFunctionDefun
 *
 */

class MacroFunctionDefun : public MacroFunction
{
public:
	explicit MacroFunctionDefun(qs::QSTATUS* pstatus);
	virtual ~MacroFunctionDefun();

public:
	virtual qs::QSTATUS value(MacroContext* pContext,
		MacroValue** ppValue) const;

protected:
	virtual const WCHAR* getName() const;

private:
	MacroFunctionDefun(const MacroFunctionDefun&);
	MacroFunctionDefun& operator=(const MacroFunctionDefun&);
};


/****************************************************************************
 *
 * MacroFunctionDelete
 *
 */

class MacroFunctionDelete : public MacroFunction
{
public:
	explicit MacroFunctionDelete(qs::QSTATUS* pstatus);
	virtual ~MacroFunctionDelete();

public:
	virtual qs::QSTATUS value(MacroContext* pContext,
		MacroValue** ppValue) const;

protected:
	virtual const WCHAR* getName() const;

private:
	MacroFunctionDelete(const MacroFunctionDelete&);
	MacroFunctionDelete& operator=(const MacroFunctionDelete&);
};


/****************************************************************************
 *
 * MacroFunctionEqual
 *
 */

class MacroFunctionEqual : public MacroFunction
{
public:
	explicit MacroFunctionEqual(qs::QSTATUS* pstatus);
	virtual ~MacroFunctionEqual();

public:
	virtual qs::QSTATUS value(MacroContext* pContext,
		MacroValue** ppValue) const;

protected:
	virtual const WCHAR* getName() const;

private:
	MacroFunctionEqual(const MacroFunctionEqual&);
	MacroFunctionEqual& operator=(const MacroFunctionEqual&);
};


/****************************************************************************
 *
 * MacroFunctionEval
 *
 */

class MacroFunctionEval : public MacroFunction
{
public:
	MacroFunctionEval(MacroParser::Type type, qs::QSTATUS* pstatus);
	virtual ~MacroFunctionEval();

public:
	virtual qs::QSTATUS value(MacroContext* pContext,
		MacroValue** ppValue) const;

protected:
	virtual const WCHAR* getName() const;

private:
	MacroFunctionEval(const MacroFunctionEval&);
	MacroFunctionEval& operator=(const MacroFunctionEval&);

private:
	MacroParser::Type type_;
};


/****************************************************************************
 *
 * MacroFunctionExecute
 *
 */

class MacroFunctionExecute : public MacroFunction
{
public:
	explicit MacroFunctionExecute(qs::QSTATUS* pstatus);
	virtual ~MacroFunctionExecute();

public:
	virtual qs::QSTATUS value(MacroContext* pContext,
		MacroValue** ppValue) const;

protected:
	virtual const WCHAR* getName() const;

private:
	MacroFunctionExecute(const MacroFunctionExecute&);
	MacroFunctionExecute& operator=(const MacroFunctionExecute&);
};


/****************************************************************************
 *
 * MacroFunctionExist
 *
 */

class MacroFunctionExist : public MacroFunction
{
public:
	explicit MacroFunctionExist(qs::QSTATUS* pstatus);
	virtual ~MacroFunctionExist();

public:
	virtual qs::QSTATUS value(MacroContext* pContext,
		MacroValue** ppValue) const;

protected:
	virtual const WCHAR* getName() const;

private:
	MacroFunctionExist(const MacroFunctionExist&);
	MacroFunctionExist& operator=(const MacroFunctionExist&);
};


/****************************************************************************
 *
 * MacroFunctionExit
 *
 */

class MacroFunctionExit : public MacroFunction
{
public:
	explicit MacroFunctionExit(qs::QSTATUS* pstatus);
	virtual ~MacroFunctionExit();

public:
	virtual qs::QSTATUS value(MacroContext* pContext,
		MacroValue** ppValue) const;

protected:
	virtual const WCHAR* getName() const;

private:
	MacroFunctionExit(const MacroFunctionExit&);
	MacroFunctionExit& operator=(const MacroFunctionExit&);
};


/****************************************************************************
 *
 * MacroFunctionField
 *
 */

class MacroFunctionField : public MacroFunction
{
public:
	explicit MacroFunctionField(qs::QSTATUS* pstatus);
	virtual ~MacroFunctionField();

public:
	virtual qs::QSTATUS value(MacroContext* pContext,
		MacroValue** ppValue) const;

protected:
	virtual const WCHAR* getName() const;

private:
	MacroFunctionField(const MacroFunctionField&);
	MacroFunctionField& operator=(const MacroFunctionField&);
};


/****************************************************************************
 *
 * MacroFunctionFieldParameter
 *
 */

class MacroFunctionFieldParameter : public MacroFunction
{
public:
	explicit MacroFunctionFieldParameter(qs::QSTATUS* pstatus);
	virtual ~MacroFunctionFieldParameter();

public:
	virtual qs::QSTATUS value(MacroContext* pContext,
		MacroValue** ppValue) const;

protected:
	virtual const WCHAR* getName() const;

private:
	MacroFunctionFieldParameter(const MacroFunctionFieldParameter&);
	MacroFunctionFieldParameter& operator=(const MacroFunctionFieldParameter&);
};


/****************************************************************************
 *
 * MacroFunctionFind
 *
 */

class MacroFunctionFind : public MacroFunction
{
public:
	explicit MacroFunctionFind(qs::QSTATUS* pstatus);
	virtual ~MacroFunctionFind();

public:
	virtual qs::QSTATUS value(MacroContext* pContext,
		MacroValue** ppValue) const;

protected:
	virtual const WCHAR* getName() const;

private:
	MacroFunctionFind(const MacroFunctionFind&);
	MacroFunctionFind& operator=(const MacroFunctionFind&);
};


/****************************************************************************
 *
 * MacroFunctionFlag
 *
 */

class MacroFunctionFlag : public MacroFunction
{
public:
	explicit MacroFunctionFlag(qs::QSTATUS* pstatus);
	MacroFunctionFlag(MessageHolder::Flag flag, qs::QSTATUS* pstatus);
	virtual ~MacroFunctionFlag();

public:
	virtual qs::QSTATUS value(MacroContext* pContext,
		MacroValue** ppValue) const;

protected:
	virtual const WCHAR* getName() const;

private:
	MacroFunctionFlag(const MacroFunctionFlag&);
	MacroFunctionFlag& operator=(const MacroFunctionFlag&);

private:
	MessageHolder::Flag flag_;
	bool bCustom_;
};


/****************************************************************************
 *
 * MacroFunctionFolder
 *
 */

class MacroFunctionFolder : public MacroFunction
{
public:
	explicit MacroFunctionFolder(qs::QSTATUS* pstatus);
	virtual ~MacroFunctionFolder();

public:
	virtual qs::QSTATUS value(MacroContext* pContext,
		MacroValue** ppValue) const;

protected:
	virtual const WCHAR* getName() const;

private:
	MacroFunctionFolder(const MacroFunctionFolder&);
	MacroFunctionFolder& operator=(const MacroFunctionFolder&);
};


/****************************************************************************
 *
 * MacroFunctionForEach
 *
 */

class MacroFunctionForEach : public MacroFunction
{
public:
	explicit MacroFunctionForEach(qs::QSTATUS* pstatus);
	virtual ~MacroFunctionForEach();

public:
	virtual qs::QSTATUS value(MacroContext* pContext,
		MacroValue** ppValue) const;

protected:
	virtual const WCHAR* getName() const;

private:
	MacroFunctionForEach(const MacroFunctionForEach&);
	MacroFunctionForEach& operator=(const MacroFunctionForEach&);
};


/****************************************************************************
 *
 * MacroFunctionFormatDate
 *
 */

class MacroFunctionFormatDate : public MacroFunction
{
public:
	explicit MacroFunctionFormatDate(qs::QSTATUS* pstatus);
	virtual ~MacroFunctionFormatDate();

public:
	virtual qs::QSTATUS value(MacroContext* pContext,
		MacroValue** ppValue) const;

protected:
	virtual const WCHAR* getName() const;

private:
	MacroFunctionFormatDate(const MacroFunctionFormatDate&);
	MacroFunctionFormatDate& operator=(const MacroFunctionFormatDate&);
};


/****************************************************************************
 *
 * MacroFunctionFunction
 *
 */

class MacroFunctionFunction : public MacroFunction
{
public:
	MacroFunctionFunction(const WCHAR* pwszName, qs::QSTATUS* pstatus);
	virtual ~MacroFunctionFunction();

public:
	virtual qs::QSTATUS value(MacroContext* pContext,
		MacroValue** ppValue) const;
	virtual void release();

protected:
	virtual const WCHAR* getName() const;

private:
	MacroFunctionFunction(const MacroFunctionFunction&);
	MacroFunctionFunction& operator=(const MacroFunctionFunction&);

private:
	qs::WSTRING wstrName_;
};


/****************************************************************************
 *
 * MacroFunctionHeader
 *
 */

class MacroFunctionHeader : public MacroFunction
{
public:
	explicit MacroFunctionHeader(qs::QSTATUS* pstatus);
	virtual ~MacroFunctionHeader();

public:
	virtual qs::QSTATUS value(MacroContext* pContext,
		MacroValue** ppValue) const;

protected:
	virtual const WCHAR* getName() const;

private:
	MacroFunctionHeader(const MacroFunctionHeader&);
	MacroFunctionHeader& operator=(const MacroFunctionHeader&);
};


/****************************************************************************
 *
 * MacroFunctionI
 *
 */

class MacroFunctionI : public MacroFunction
{
public:
	explicit MacroFunctionI(qs::QSTATUS* pstatus);
	virtual ~MacroFunctionI();

public:
	virtual qs::QSTATUS value(MacroContext* pContext,
		MacroValue** ppValue) const;

protected:
	virtual const WCHAR* getName() const;

private:
	MacroFunctionI(const MacroFunctionI&);
	MacroFunctionI& operator=(const MacroFunctionI&);
};


/****************************************************************************
 *
 * MacroFunctionId
 *
 */

class MacroFunctionId : public MacroFunction
{
public:
	explicit MacroFunctionId(qs::QSTATUS* pstatus);
	virtual ~MacroFunctionId();

public:
	virtual qs::QSTATUS value(MacroContext* pContext,
		MacroValue** ppValue) const;

protected:
	virtual const WCHAR* getName() const;

private:
	MacroFunctionId(const MacroFunctionId&);
	MacroFunctionId& operator=(const MacroFunctionId&);
};


/****************************************************************************
 *
 * MacroFunctionIdentity
 *
 */

class MacroFunctionIdentity : public MacroFunction
{
public:
	explicit MacroFunctionIdentity(qs::QSTATUS* pstatus);
	virtual ~MacroFunctionIdentity();

public:
	virtual qs::QSTATUS value(MacroContext* pContext,
		MacroValue** ppValue) const;

protected:
	virtual const WCHAR* getName() const;

private:
	MacroFunctionIdentity(const MacroFunctionIdentity&);
	MacroFunctionIdentity& operator=(const MacroFunctionIdentity&);
};


/****************************************************************************
 *
 * MacroFunctionIf
 *
 */

class MacroFunctionIf : public MacroFunction
{
public:
	explicit MacroFunctionIf(qs::QSTATUS* pstatus);
	virtual ~MacroFunctionIf();

public:
	virtual qs::QSTATUS value(MacroContext* pContext,
		MacroValue** ppValue) const;

protected:
	virtual const WCHAR* getName() const;

private:
	MacroFunctionIf(const MacroFunctionIf&);
	MacroFunctionIf& operator=(const MacroFunctionIf&);
};


/****************************************************************************
 *
 * MacroFunctionInclude
 *
 */

class MacroFunctionInclude : public MacroFunction
{
public:
	MacroFunctionInclude(MacroParser::Type type, qs::QSTATUS* pstatus);
	virtual ~MacroFunctionInclude();

public:
	virtual qs::QSTATUS value(MacroContext* pContext,
		MacroValue** ppValue) const;

protected:
	virtual const WCHAR* getName() const;

private:
	MacroFunctionInclude(const MacroFunctionInclude&);
	MacroFunctionInclude& operator=(const MacroFunctionInclude&);

private:
	MacroParser::Type type_;
};


/****************************************************************************
 *
 * MacroFunctionInputBox
 *
 */

class MacroFunctionInputBox : public MacroFunction
{
public:
	explicit MacroFunctionInputBox(qs::QSTATUS* pstatus);
	virtual ~MacroFunctionInputBox();

public:
	virtual qs::QSTATUS value(MacroContext* pContext,
		MacroValue** ppValue) const;

protected:
	virtual const WCHAR* getName() const;

private:
	MacroFunctionInputBox(const MacroFunctionInputBox&);
	MacroFunctionInputBox& operator=(const MacroFunctionInputBox&);
};


/****************************************************************************
 *
 * MacroFunctionLength
 *
 */

class MacroFunctionLength : public MacroFunction
{
public:
	explicit MacroFunctionLength(qs::QSTATUS* pstatus);
	virtual ~MacroFunctionLength();

public:
	virtual qs::QSTATUS value(MacroContext* pContext,
		MacroValue** ppValue) const;

protected:
	virtual const WCHAR* getName() const;

private:
	MacroFunctionLength(const MacroFunctionLength&);
	MacroFunctionLength& operator=(const MacroFunctionLength&);
};


/****************************************************************************
 *
 * MacroFunctionLoad
 *
 */

class MacroFunctionLoad : public MacroFunction
{
public:
	explicit MacroFunctionLoad(qs::QSTATUS* pstatus);
	virtual ~MacroFunctionLoad();

public:
	virtual qs::QSTATUS value(MacroContext* pContext,
		MacroValue** ppValue) const;

protected:
	virtual const WCHAR* getName() const;

private:
	MacroFunctionLoad(const MacroFunctionLoad&);
	MacroFunctionLoad& operator=(const MacroFunctionLoad&);
};


/****************************************************************************
 *
 * MacroFunctionMessageBox
 *
 */

class MacroFunctionMessageBox : public MacroFunction
{
public:
	explicit MacroFunctionMessageBox(qs::QSTATUS* pstatus);
	virtual ~MacroFunctionMessageBox();

public:
	virtual qs::QSTATUS value(MacroContext* pContext,
		MacroValue** ppValue) const;

protected:
	virtual const WCHAR* getName() const;

private:
	MacroFunctionMessageBox(const MacroFunctionMessageBox&);
	MacroFunctionMessageBox& operator=(const MacroFunctionMessageBox&);
};


/****************************************************************************
 *
 * MacroFunctionMessages
 *
 */

class MacroFunctionMessages : public MacroFunction
{
public:
	explicit MacroFunctionMessages(qs::QSTATUS* pstatus);
	virtual ~MacroFunctionMessages();

public:
	virtual qs::QSTATUS value(MacroContext* pContext,
		MacroValue** ppValue) const;

protected:
	virtual const WCHAR* getName() const;

private:
	MacroFunctionMessages(const MacroFunctionMessages&);
	MacroFunctionMessages& operator=(const MacroFunctionMessages&);
};


/****************************************************************************
 *
 * MacroFunctionNot
 *
 */

class MacroFunctionNot : public MacroFunction
{
public:
	explicit MacroFunctionNot(qs::QSTATUS* pstatus);
	virtual ~MacroFunctionNot();

public:
	virtual qs::QSTATUS value(MacroContext* pContext,
		MacroValue** ppValue) const;

protected:
	virtual const WCHAR* getName() const;

private:
	MacroFunctionNot(const MacroFunctionNot&);
	MacroFunctionNot& operator=(const MacroFunctionNot&);
};


/****************************************************************************
 *
 * MacroFunctionOr
 *
 */

class MacroFunctionOr : public MacroFunction
{
public:
	explicit MacroFunctionOr(qs::QSTATUS* pstatus);
	virtual ~MacroFunctionOr();

public:
	virtual qs::QSTATUS value(MacroContext* pContext,
		MacroValue** ppValue) const;

protected:
	virtual const WCHAR* getName() const;

private:
	MacroFunctionOr(const MacroFunctionOr&);
	MacroFunctionOr& operator=(const MacroFunctionOr&);
};


/****************************************************************************
 *
 * MacroFunctionOSVersion
 *
 */

class MacroFunctionOSVersion : public MacroFunction
{
public:
	explicit MacroFunctionOSVersion(qs::QSTATUS* pstatus);
	virtual ~MacroFunctionOSVersion();

public:
	virtual qs::QSTATUS value(MacroContext* pContext,
		MacroValue** ppValue) const;

protected:
	virtual const WCHAR* getName() const;

private:
	MacroFunctionOSVersion(const MacroFunctionOSVersion&);
	MacroFunctionOSVersion& operator=(const MacroFunctionOSVersion&);
};


/****************************************************************************
 *
 * MacroFunctionParseURL
 *
 */

class MacroFunctionParseURL : public MacroFunction
{
public:
	explicit MacroFunctionParseURL(qs::QSTATUS* pstatus);
	virtual ~MacroFunctionParseURL();

public:
	virtual qs::QSTATUS value(MacroContext* pContext,
		MacroValue** ppValue) const;

protected:
	virtual const WCHAR* getName() const;

private:
	MacroFunctionParseURL(const MacroFunctionParseURL&);
	MacroFunctionParseURL& operator=(const MacroFunctionParseURL&);
};


/****************************************************************************
 *
 * MacroFunctionPart
 *
 */

class MacroFunctionPart : public MacroFunction
{
public:
	explicit MacroFunctionPart(qs::QSTATUS* pstatus);
	virtual ~MacroFunctionPart();

public:
	virtual qs::QSTATUS value(MacroContext* pContext,
		MacroValue** ppValue) const;

protected:
	virtual const WCHAR* getName() const;

private:
	MacroFunctionPart(const MacroFunctionPart&);
	MacroFunctionPart& operator=(const MacroFunctionPart&);
};


/****************************************************************************
 *
 * MacroFunctionPassed
 *
 */

class MacroFunctionPassed : public MacroFunction
{
public:
	explicit MacroFunctionPassed(qs::QSTATUS* pstatus);
	virtual ~MacroFunctionPassed();

public:
	virtual qs::QSTATUS value(MacroContext* pContext,
		MacroValue** ppValue) const;

protected:
	virtual const WCHAR* getName() const;

private:
	MacroFunctionPassed(const MacroFunctionPassed&);
	MacroFunctionPassed& operator=(const MacroFunctionPassed&);
};


/****************************************************************************
 *
 * MacroFunctionProcessId
 *
 */

class MacroFunctionProcessId : public MacroFunction
{
public:
	explicit MacroFunctionProcessId(qs::QSTATUS* pstatus);
	virtual ~MacroFunctionProcessId();

public:
	virtual qs::QSTATUS value(MacroContext* pContext,
		MacroValue** ppValue) const;

protected:
	virtual const WCHAR* getName() const;

private:
	MacroFunctionProcessId(const MacroFunctionProcessId&);
	MacroFunctionProcessId& operator=(const MacroFunctionProcessId&);
};


/****************************************************************************
 *
 * MacroFunctionProfile
 *
 */

class MacroFunctionProfile : public MacroFunction
{
public:
	explicit MacroFunctionProfile(qs::QSTATUS* pstatus);
	virtual ~MacroFunctionProfile();

public:
	virtual qs::QSTATUS value(MacroContext* pContext,
		MacroValue** ppValue) const;

protected:
	virtual const WCHAR* getName() const;

private:
	MacroFunctionProfile(const MacroFunctionProfile&);
	MacroFunctionProfile& operator=(const MacroFunctionProfile&);
};


/****************************************************************************
 *
 * MacroFunctionProfileName
 *
 */

class MacroFunctionProfileName : public MacroFunction
{
public:
	explicit MacroFunctionProfileName(qs::QSTATUS* pstatus);
	virtual ~MacroFunctionProfileName();

public:
	virtual qs::QSTATUS value(MacroContext* pContext,
		MacroValue** ppValue) const;

protected:
	virtual const WCHAR* getName() const;

private:
	MacroFunctionProfileName(const MacroFunctionProfileName&);
	MacroFunctionProfileName& operator=(const MacroFunctionProfileName&);
};


/****************************************************************************
 *
 * MacroFunctionProgn
 *
 */

class MacroFunctionProgn : public MacroFunction
{
public:
	explicit MacroFunctionProgn(qs::QSTATUS* pstatus);
	virtual ~MacroFunctionProgn();

public:
	virtual qs::QSTATUS value(MacroContext* pContext,
		MacroValue** ppValue) const;

protected:
	virtual const WCHAR* getName() const;

private:
	MacroFunctionProgn(const MacroFunctionProgn&);
	MacroFunctionProgn& operator=(const MacroFunctionProgn&);
};


/****************************************************************************
 *
 * MacroFunctionReferences
 *
 */

class MacroFunctionReferences : public MacroFunction
{
public:
	explicit MacroFunctionReferences(qs::QSTATUS* pstatus);
	virtual ~MacroFunctionReferences();

public:
	virtual qs::QSTATUS value(MacroContext* pContext,
		MacroValue** ppValue) const;

protected:
	virtual const WCHAR* getName() const;

private:
	MacroFunctionReferences(const MacroFunctionReferences&);
	MacroFunctionReferences& operator=(const MacroFunctionReferences&);
};


/****************************************************************************
 *
 * MacroFunctionRelative
 *
 */

class MacroFunctionRelative : public MacroFunction
{
public:
	explicit MacroFunctionRelative(bool bLess, qs::QSTATUS* pstatus);
	virtual ~MacroFunctionRelative();

public:
	virtual qs::QSTATUS value(MacroContext* pContext,
		MacroValue** ppValue) const;

protected:
	virtual const WCHAR* getName() const;

private:
	MacroFunctionRelative(const MacroFunctionRelative&);
	MacroFunctionRelative& operator=(const MacroFunctionRelative&);

private:
	bool bLess_;
};


/****************************************************************************
 *
 * MacroFunctionRemove
 *
 */

class MacroFunctionRemove : public MacroFunction
{
public:
	explicit MacroFunctionRemove(qs::QSTATUS* pstatus);
	virtual ~MacroFunctionRemove();

public:
	virtual qs::QSTATUS value(MacroContext* pContext,
		MacroValue** ppValue) const;

protected:
	virtual const WCHAR* getName() const;

private:
	static qs::QSTATUS remove(qs::AddressListParser* pAddressList,
		const WCHAR* pwszAddress);

private:
	MacroFunctionRemove(const MacroFunctionRemove&);
	MacroFunctionRemove& operator=(const MacroFunctionRemove&);
};


/****************************************************************************
 *
 * MacroFunctionSave
 *
 */

class MacroFunctionSave : public MacroFunction
{
public:
	explicit MacroFunctionSave(qs::QSTATUS* pstatus);
	virtual ~MacroFunctionSave();

public:
	virtual qs::QSTATUS value(MacroContext* pContext,
		MacroValue** ppValue) const;

protected:
	virtual const WCHAR* getName() const;

private:
	MacroFunctionSave(const MacroFunctionSave&);
	MacroFunctionSave& operator=(const MacroFunctionSave&);
};


/****************************************************************************
 *
 * MacroFunctionScript
 *
 */

class MacroFunctionScript : public MacroFunction
{
public:
	explicit MacroFunctionScript(qs::QSTATUS* pstatus);
	virtual ~MacroFunctionScript();

public:
	virtual qs::QSTATUS value(MacroContext* pContext,
		MacroValue** ppValue) const;

protected:
	virtual const WCHAR* getName() const;

private:
	MacroFunctionScript(const MacroFunctionScript&);
	MacroFunctionScript& operator=(const MacroFunctionScript&);
};


/****************************************************************************
 *
 * MacroFunctionSet
 *
 */

class MacroFunctionSet : public MacroFunction
{
public:
	explicit MacroFunctionSet(qs::QSTATUS* pstatus);
	virtual ~MacroFunctionSet();

public:
	virtual qs::QSTATUS value(MacroContext* pContext,
		MacroValue** ppValue) const;

protected:
	virtual const WCHAR* getName() const;

private:
	MacroFunctionSet(const MacroFunctionSet&);
	MacroFunctionSet& operator=(const MacroFunctionSet&);
};


/****************************************************************************
 *
 * MacroFunctionSize
 *
 */

class MacroFunctionSize : public MacroFunction
{
public:
	explicit MacroFunctionSize(qs::QSTATUS* pstatus);
	virtual ~MacroFunctionSize();

public:
	virtual qs::QSTATUS value(MacroContext* pContext,
		MacroValue** ppValue) const;

protected:
	virtual const WCHAR* getName() const;

private:
	MacroFunctionSize(const MacroFunctionSize&);
	MacroFunctionSize& operator=(const MacroFunctionSize&);
};


/****************************************************************************
 *
 * MacroFunctionSubAccount
 *
 */

class MacroFunctionSubAccount : public MacroFunction
{
public:
	explicit MacroFunctionSubAccount(qs::QSTATUS* pstatus);
	virtual ~MacroFunctionSubAccount();

public:
	virtual qs::QSTATUS value(MacroContext* pContext,
		MacroValue** ppValue) const;

protected:
	virtual const WCHAR* getName() const;

private:
	MacroFunctionSubAccount(const MacroFunctionSubAccount&);
	MacroFunctionSubAccount& operator=(const MacroFunctionSubAccount&);
};


/****************************************************************************
 *
 * MacroFunctionSubject
 *
 */

class MacroFunctionSubject : public MacroFunction
{
public:
	explicit MacroFunctionSubject(qs::QSTATUS* pstatus);
	virtual ~MacroFunctionSubject();

public:
	virtual qs::QSTATUS value(MacroContext* pContext,
		MacroValue** ppValue) const;

protected:
	virtual const WCHAR* getName() const;

private:
	MacroFunctionSubject(const MacroFunctionSubject&);
	MacroFunctionSubject& operator=(const MacroFunctionSubject&);
};


/****************************************************************************
 *
 * MacroFunctionSubstring
 *
 */

class MacroFunctionSubstring : public MacroFunction
{
public:
	explicit MacroFunctionSubstring(qs::QSTATUS* pstatus);
	virtual ~MacroFunctionSubstring();

public:
	virtual qs::QSTATUS value(MacroContext* pContext,
		MacroValue** ppValue) const;

protected:
	virtual const WCHAR* getName() const;

private:
	MacroFunctionSubstring(const MacroFunctionSubstring&);
	MacroFunctionSubstring& operator=(const MacroFunctionSubstring&);
};


/****************************************************************************
 *
 * MacroFunctionSubstringSep
 *
 */

class MacroFunctionSubstringSep : public MacroFunction
{
public:
	MacroFunctionSubstringSep(bool bAfter, qs::QSTATUS* pstatus);
	virtual ~MacroFunctionSubstringSep();

public:
	virtual qs::QSTATUS value(MacroContext* pContext,
		MacroValue** ppValue) const;

protected:
	virtual const WCHAR* getName() const;

private:
	MacroFunctionSubstringSep(const MacroFunctionSubstringSep&);
	MacroFunctionSubstringSep& operator=(const MacroFunctionSubstringSep&);

private:
	bool bAfter_;
};


/****************************************************************************
 *
 * MacroFunctionVariable
 *
 */

class MacroFunctionVariable : public MacroFunction
{
public:
	explicit MacroFunctionVariable(qs::QSTATUS* pstatus);
	virtual ~MacroFunctionVariable();

public:
	virtual qs::QSTATUS value(MacroContext* pContext,
		MacroValue** ppValue) const;

protected:
	virtual const WCHAR* getName() const;

private:
	MacroFunctionVariable(const MacroFunctionVariable&);
	MacroFunctionVariable& operator=(const MacroFunctionVariable&);
};


/****************************************************************************
 *
 * MacroFunctionWhile
 *
 */

class MacroFunctionWhile : public MacroFunction
{
public:
	explicit MacroFunctionWhile(qs::QSTATUS* pstatus);
	virtual ~MacroFunctionWhile();

public:
	virtual qs::QSTATUS value(MacroContext* pContext,
		MacroValue** ppValue) const;

protected:
	virtual const WCHAR* getName() const;

private:
	MacroFunctionWhile(const MacroFunctionWhile&);
	MacroFunctionWhile& operator=(const MacroFunctionWhile&);
};


/****************************************************************************
 *
 * MacroExprPtr
 *
 */

class MacroExprPtr
{
public:
	MacroExprPtr();
	MacroExprPtr(MacroExpr* pExpr);
	~MacroExprPtr();

public:
	MacroExpr* get() const;
	MacroExpr* release();
	void reset(MacroExpr* pExpr);

private:
	MacroExpr* pExpr_;
};


/****************************************************************************
 *
 * MacroFunctionFactory
 *
 */

class MacroFunctionFactory
{
private:
	MacroFunctionFactory();

public:
	~MacroFunctionFactory();

public:
	qs::QSTATUS newFunction(MacroParser::Type type,
		const WCHAR* pwszName, MacroFunction** ppFunction) const;

public:
	static const MacroFunctionFactory& getFactory();

private:
	MacroFunctionFactory(const MacroFunctionFactory&);
	MacroFunctionFactory& operator=(const MacroFunctionFactory&);

private:
	static MacroFunctionFactory factory__;
};

}

#endif // __MACRO_H__
