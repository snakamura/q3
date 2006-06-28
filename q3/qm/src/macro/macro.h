/*
 * $Id$
 *
 * Copyright(C) 1998-2006 Satoshi Nakamura
 * All rights reserved.
 *
 */

#ifndef __MACRO_H__
#define __MACRO_H__

#include <qmmacro.h>

#include <qsregex.h>

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
	class MacroBoolean;
	class MacroRegex;
	class MacroVariable;
	class MacroConstant;
	class MacroFunction;
		class MacroFunctionAccount;
		class MacroFunctionAccountDirectory;
		class MacroFunctionAdditive;
		class MacroFunctionAddress;
		class MacroFunctionAddressBook;
		class MacroFunctionAnd;
		class MacroFunctionAttachment;
		class MacroFunctionBody;
		class MacroFunctionBodyCharset;
		class MacroFunctionBoolean;
		class MacroFunctionCatch;
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
		class MacroFunctionFindEach;
		class MacroFunctionFlag;
		class MacroFunctionFolder;
		class MacroFunctionFolderFlag;
		class MacroFunctionFolderParameter;
		class MacroFunctionForEach;
		class MacroFunctionFormatAddress;
		class MacroFunctionFormatDate;
		class MacroFunctionFunction;
		class MacroFunctionHeader;
		class MacroFunctionHtmlEscape;
		class MacroFunctionI;
		class MacroFunctionId;
		class MacroFunctionIdentity;
		class MacroFunctionIf;
		class MacroFunctionInputBox;
		class MacroFunctionJunk;
		class MacroFunctionLabel;
		class MacroFunctionLength;
		class MacroFunctionLoad;
		class MacroFunctionLookupAddressBook;
		class MacroFunctionMessageBox;
		class MacroFunctionMessages;
		class MacroFunctionNew;
		class MacroFunctionNot;
		class MacroFunctionOr;
		class MacroFunctionOSVersion;
		class MacroFunctionParseURL;
		class MacroFunctionParam;
		class MacroFunctionPart;
		class MacroFunctionPassed;
		class MacroFunctionProcessId;
		class MacroFunctionProfile;
		class MacroFunctionProfileName;
		class MacroFunctionProgn;
		class MacroFunctionQuote;
		class MacroFunctionReferences;
		class MacroFunctionRegexFind;
		class MacroFunctionRegexMatch;
		class MacroFunctionRegexReplace;
		class MacroFunctionRelative;
		class MacroFunctionRemove;
		class MacroFunctionSave;
		class MacroFunctionScript;
		class MacroFunctionSelectBox;
		class MacroFunctionSelected;
		class MacroFunctionSet;
		class MacroFunctionSize;
		class MacroFunctionSpecialFolder;
		class MacroFunctionSubAccount;
		class MacroFunctionSubject;
		class MacroFunctionSubstring;
		class MacroFunctionSubstringSep;
		class MacroFunctionThread;
		class MacroFunctionURI;
		class MacroFunctionVariable;
		class MacroFunctionWhile;
class MacroExprVisitor;
class MacroExprPtr;
class MacroFunctionFactory;
class MacroConstantFactory;

class AddressBook;
class AddressBookCategory;


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
		TOKEN_REGEX,
		TOKEN_AT,
		TOKEN_DOLLAR,
		TOKEN_COMMA,
		TOKEN_PERCENT,
		TOKEN_LEFTPARENTHESIS,
		TOKEN_RIGHTPARENTHESIS,
		TOKEN_COLON,
		TOKEN_END,
		TOKEN_ERROR
	};

public:
	explicit MacroTokenizer(const WCHAR* pwszMacro);
	~MacroTokenizer();

public:
	Token getToken(qs::wstring_ptr* pwstrToken,
				   qs::wstring_ptr* pwstrTokenEx);
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
	MacroGlobalContext(const MessageHolderList& listSelected,
					   Folder* pFolder,
					   Document* pDocument,
					   HWND hwnd,
					   qs::Profile* pProfile,
					   const WCHAR* pwszBodyCharset,
					   unsigned int nFlags,
					   unsigned int nSecurityMode,
					   MacroErrorHandler* pErrorHandler,
					   MacroVariableHolder* pGlobalVariable);
	~MacroGlobalContext();

public:
	const MessageHolderList& getSelectedMessageHolders() const;
	Folder* getFolder() const;
	Document* getDocument() const;
	HWND getWindow() const;
	qs::Profile* getProfile() const;
	const WCHAR* getBodyCharset() const;
	unsigned int getFlags() const;
	unsigned int getSecurityMode() const;
	MacroErrorHandler* getErrorHandler() const;
	MacroContext::ReturnType getReturnType() const;
	void setReturnType(MacroContext::ReturnType type);
	MacroValuePtr getVariable(const WCHAR* pwszName) const;
	bool setVariable(const WCHAR* pwszName,
					 MacroValue* pValue,
					 bool bGlobal);
	void removeVariable(const WCHAR* pwszName,
						bool bGlobal);
	const MacroExpr* getFunction(const WCHAR* pwszName) const;
	bool setFunction(const WCHAR* pwszName,
					 const MacroExpr* pExpr);
	void pushArguments(MacroContext::ArgumentList& listArgument);
	void popArguments();
	MacroValuePtr getArgument(unsigned int n) const;
	bool setRegexResult(const qs::RegexRangeList& listRange);
	void clearRegexResult();
	void storeParsedMacro(std::auto_ptr<Macro> pMacro);

private:
	MacroGlobalContext(const MacroGlobalContext&);
	MacroGlobalContext& operator=(const MacroGlobalContext&);

private:
	typedef std::vector<Macro*> MacroList;

private:
	const MessageHolderList& listSelected_;
	Folder* pFolder_;
	Document* pDocument_;
	HWND hwnd_;
	qs::Profile* pProfile_;
	qs::wstring_ptr wstrBodyCharset_;
	unsigned int nFlags_;
	unsigned int nSecurityMode_;
	MacroErrorHandler* pErrorHandler_;
	MacroVariableHolder* pGlobalVariable_;
	MacroContext::ReturnType returnType_;
	std::auto_ptr<MacroVariableHolder> pVariable_;
	std::auto_ptr<MacroFunctionHolder> pFunction_;
	std::auto_ptr<MacroArgumentHolder> pArgument_;
	size_t nRegexResultCount_;
	MacroList listParsedMacro_;
};


/****************************************************************************
 *
 * MacroFunctionHolder
 *
 */

class MacroFunctionHolder
{
public:
	MacroFunctionHolder();
	~MacroFunctionHolder();

public:
	const MacroExpr* getFunction(const WCHAR* pwszName) const;
	bool setFunction(const WCHAR* pwszName,
					 const MacroExpr* pExpr);

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
	MacroArgumentHolder();
	~MacroArgumentHolder();

public:
	void push(MacroContext::ArgumentList& listArgument);
	void pop();
	MacroValuePtr getArgument(unsigned int n) const;

private:
	MacroArgumentHolder(const MacroArgumentHolder&);
	MacroArgumentHolder& operator=(const MacroArgumentHolder&);

private:
	typedef std::vector<MacroContext::ArgumentList> ArgumentStack;

private:
	ArgumentStack stackArgument_;
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
	virtual MacroValuePtr value(MacroContext* pContext) const = 0;
	virtual qs::wstring_ptr getString() const = 0;
	virtual void release();
	virtual MacroContext::MessageType getMessageTypeHint() const;
	virtual void visit(MacroExprVisitor* pVisitor) const = 0;

protected:
	MacroValuePtr error(const MacroContext& context,
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
	explicit MacroField(const WCHAR* pwszName);
	virtual ~MacroField();

public:
	const WCHAR* getName() const;

public:
	virtual MacroValuePtr value(MacroContext* pContext) const;
	virtual qs::wstring_ptr getString() const;
	virtual MacroContext::MessageType getMessageTypeHint() const;
	virtual void visit(MacroExprVisitor* pVisitor) const;

private:
	MacroField(const MacroField&);
	MacroField& operator=(const MacroField&);

private:
	qs::wstring_ptr wstrName_;
};


/****************************************************************************
 *
 * MacroFieldCache
 *
 */

class MacroFieldCache : public MacroExpr
{
public:
	enum Type {
		TYPE_ID,
		TYPE_DATE,
		TYPE_FROM,
		TYPE_TO,
		TYPE_FROMTO,
		TYPE_SUBJECT,
		TYPE_SIZE,
		TYPE_ERROR
	};

public:
	explicit MacroFieldCache(Type type);
	virtual ~MacroFieldCache();

public:
	bool operator!() const;

public:
	Type getType() const;

public:
	virtual MacroValuePtr value(MacroContext* pContext) const;
	virtual qs::wstring_ptr getString() const;
	virtual void visit(MacroExprVisitor* pVisitor) const;

public:
	static Type getType(const WCHAR* pwszType);

private:
	MacroFieldCache(const MacroFieldCache&);
	MacroFieldCache& operator=(const MacroFieldCache&);

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
	explicit MacroLiteral(const WCHAR* pwszValue);
	virtual ~MacroLiteral();

public:
	const WCHAR* getValue() const;

public:
	virtual MacroValuePtr value(MacroContext* pContext) const;
	virtual qs::wstring_ptr getString() const;
	virtual void visit(MacroExprVisitor* pVisitor) const;

private:
	MacroLiteral(const MacroLiteral&);
	MacroLiteral& operator=(const MacroLiteral&);

private:
	qs::wstring_ptr wstrValue_;
};


/****************************************************************************
 *
 * MacroNumber
 *
 */

class MacroNumber : public MacroExpr
{
public:
	explicit MacroNumber(unsigned int nValue);
	virtual ~MacroNumber();

public:
	unsigned int getValue() const;

public:
	virtual MacroValuePtr value(MacroContext* pContext) const;
	virtual qs::wstring_ptr getString() const;
	virtual void visit(MacroExprVisitor* pVisitor) const;

private:
	MacroNumber(const MacroNumber&);
	MacroNumber& operator=(const MacroNumber&);

private:
	unsigned int nValue_;
};


/****************************************************************************
 *
 * MacroBoolean
 *
 */

class MacroBoolean : public MacroExpr
{
public:
	explicit MacroBoolean(bool bValue);
	virtual ~MacroBoolean();

public:
	bool getValue() const;

public:
	virtual MacroValuePtr value(MacroContext* pContext) const;
	virtual qs::wstring_ptr getString() const;
	virtual void visit(MacroExprVisitor* pVisitor) const;

private:
	MacroBoolean(const MacroBoolean&);
	MacroBoolean& operator=(const MacroBoolean&);

private:
	bool bValue_;
};


/****************************************************************************
 *
 * MacroRegex
 *
 */

class MacroRegex : public MacroExpr
{
public:
	MacroRegex(const WCHAR* pwszPattern,
			   const WCHAR* pwszMode);
	virtual ~MacroRegex();

public:
	bool operator!() const;

public:
	const WCHAR* getPattern() const;
	const WCHAR* getMode() const;

public:
	virtual MacroValuePtr value(MacroContext* pContext) const;
	virtual qs::wstring_ptr getString() const;
	virtual void visit(MacroExprVisitor* pVisitor) const;

private:
	MacroRegex(const MacroRegex&);
	MacroRegex& operator=(const MacroRegex&);

private:
	qs::wstring_ptr wstrPattern_;
	qs::wstring_ptr wstrMode_;
	std::auto_ptr<qs::RegexPattern> pPattern_;
};


/****************************************************************************
 *
 * MacroVariable
 *
 */

class MacroVariable : public MacroExpr
{
public:
	explicit MacroVariable(const WCHAR* pwszName);
	virtual ~MacroVariable();

public:
	const WCHAR* getName() const;
	unsigned int getIndex() const;

public:
	virtual MacroValuePtr value(MacroContext* pContext) const;
	virtual qs::wstring_ptr getString() const;
	virtual void visit(MacroExprVisitor* pVisitor) const;

private:
	MacroVariable(const MacroVariable&);
	MacroVariable& operator=(const MacroVariable&);

private:
	qs::wstring_ptr wstrName_;
	unsigned int n_;
};


/****************************************************************************
 *
 * MacroConstant
 *
 */

class MacroConstant : public MacroExpr
{
public:
	MacroConstant(const WCHAR* pwszName,
				  std::auto_ptr<MacroExpr> pExpr);
	virtual ~MacroConstant();

public:
	virtual MacroValuePtr value(MacroContext* pContext) const;
	virtual qs::wstring_ptr getString() const;
	virtual void visit(MacroExprVisitor* pVisitor) const;

private:
	MacroConstant(const MacroConstant&);
	MacroConstant& operator=(const MacroConstant&);

private:
	qs::wstring_ptr wstrName_;
	std::auto_ptr<MacroExpr> pExpr_;
};


/****************************************************************************
 *
 * MacroFunction
 *
 */

class MacroFunction : public MacroExpr
{
protected:
	MacroFunction();

public:
	virtual ~MacroFunction();

public:
	void addArg(MacroExprPtr pArg);

public:
	virtual qs::wstring_ptr getString() const;
	virtual MacroContext::MessageType getMessageTypeHint() const;
	virtual void visit(MacroExprVisitor* pVisitor) const;

protected:
	virtual const WCHAR* getName() const = 0;
	virtual MacroContext::MessageType getFunctionMessageTypeHint() const;

public:
	const WCHAR* getFunctionName() const;
	size_t getArgSize() const;
	const MacroExpr* getArg(size_t n) const;

protected:
	bool checkArgSize(MacroContext* pContext,
					  size_t n) const;
	bool checkArgSizeRange(MacroContext* pContext,
						   size_t nMin,
						   size_t nMax) const;
	bool checkArgSizeMin(MacroContext* pContext,
						 size_t nMin) const;
	const qs::Part* getPart(MacroContext* pContext,
							size_t n) const;
	Message* getMessage(MacroContext* pContext,
						MacroContext::MessageType type,
						const WCHAR* pwszField) const;

private:
	qs::wstring_ptr getArgString() const;

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
	MacroFunctionAccount();
	virtual ~MacroFunctionAccount();

public:
	virtual MacroValuePtr value(MacroContext* pContext) const;

protected:
	virtual const WCHAR* getName() const;

private:
	MacroFunctionAccount(const MacroFunctionAccount&);
	MacroFunctionAccount& operator=(const MacroFunctionAccount&);
};


/****************************************************************************
 *
 * MacroFunctionAccountClass
 *
 */

class MacroFunctionAccountClass : public MacroFunction
{
public:
	MacroFunctionAccountClass();
	virtual ~MacroFunctionAccountClass();

public:
	virtual MacroValuePtr value(MacroContext* pContext) const;

protected:
	virtual const WCHAR* getName() const;

private:
	MacroFunctionAccountClass(const MacroFunctionAccountClass&);
	MacroFunctionAccountClass& operator=(const MacroFunctionAccountClass&);
};


/****************************************************************************
 *
 * MacroFunctionAccountDirectory
 *
 */

class MacroFunctionAccountDirectory : public MacroFunction
{
public:
	MacroFunctionAccountDirectory();
	virtual ~MacroFunctionAccountDirectory();

public:
	virtual MacroValuePtr value(MacroContext* pContext) const;

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
	explicit MacroFunctionAdditive(bool bAdd);
	virtual ~MacroFunctionAdditive();

public:
	virtual MacroValuePtr value(MacroContext* pContext) const;

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
	explicit MacroFunctionAddress(bool bName);
	virtual ~MacroFunctionAddress();

public:
	virtual MacroValuePtr value(MacroContext* pContext) const;

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
	MacroFunctionAddressBook();
	virtual ~MacroFunctionAddressBook();

public:
	virtual MacroValuePtr value(MacroContext* pContext) const;

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
	MacroFunctionAnd();
	virtual ~MacroFunctionAnd();

public:
	virtual MacroValuePtr value(MacroContext* pContext) const;

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
	MacroFunctionAttachment();
	virtual ~MacroFunctionAttachment();

public:
	virtual MacroValuePtr value(MacroContext* pContext) const;

protected:
	virtual const WCHAR* getName() const;
	virtual MacroContext::MessageType getFunctionMessageTypeHint() const;

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
	MacroFunctionBody();
	virtual ~MacroFunctionBody();

public:
	virtual MacroValuePtr value(MacroContext* pContext) const;

protected:
	virtual const WCHAR* getName() const;
	virtual MacroContext::MessageType getFunctionMessageTypeHint() const;

private:
	MacroFunctionBody(const MacroFunctionBody&);
	MacroFunctionBody& operator=(const MacroFunctionBody&);
};


/****************************************************************************
 *
 * MacroFunctionBodyCharset
 *
 */

class MacroFunctionBodyCharset : public MacroFunction
{
public:
	MacroFunctionBodyCharset();
	virtual ~MacroFunctionBodyCharset();

public:
	virtual MacroValuePtr value(MacroContext* pContext) const;

protected:
	virtual const WCHAR* getName() const;
	virtual MacroContext::MessageType getFunctionMessageTypeHint() const;

private:
	MacroFunctionBodyCharset(const MacroFunctionBodyCharset&);
	MacroFunctionBodyCharset& operator=(const MacroFunctionBodyCharset&);
};


/****************************************************************************
 *
 * MacroFunctionBoolean
 *
 */

class MacroFunctionBoolean : public MacroFunction
{
public:
	explicit MacroFunctionBoolean(bool b);
	virtual ~MacroFunctionBoolean();

public:
	virtual MacroValuePtr value(MacroContext* pContext) const;

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
 * MacroFunctionCatch
 *
 */

class MacroFunctionCatch : public MacroFunction
{
public:
	MacroFunctionCatch();
	virtual ~MacroFunctionCatch();

public:
	virtual MacroValuePtr value(MacroContext* pContext) const;

protected:
	virtual const WCHAR* getName() const;

private:
	MacroFunctionCatch(const MacroFunctionCatch&);
	MacroFunctionCatch& operator=(const MacroFunctionCatch&);
};


/****************************************************************************
 *
 * MacroFunctionClipboard
 *
 */

class MacroFunctionClipboard : public MacroFunction
{
public:
	MacroFunctionClipboard();
	virtual ~MacroFunctionClipboard();

public:
	virtual MacroValuePtr value(MacroContext* pContext) const;

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
	MacroFunctionComputerName();
	virtual ~MacroFunctionComputerName();

public:
	virtual MacroValuePtr value(MacroContext* pContext) const;

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
	MacroFunctionConcat();
	virtual ~MacroFunctionConcat();

public:
	virtual MacroValuePtr value(MacroContext* pContext) const;

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
	explicit MacroFunctionContain(bool bBeginWith);
	virtual ~MacroFunctionContain();

public:
	virtual MacroValuePtr value(MacroContext* pContext) const;

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
	explicit MacroFunctionCopy(bool bMove);
	virtual ~MacroFunctionCopy();

public:
	virtual MacroValuePtr value(MacroContext* pContext) const;

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
	MacroFunctionDate();
	virtual ~MacroFunctionDate();

public:
	virtual MacroValuePtr value(MacroContext* pContext) const;

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
	MacroFunctionDecode();
	virtual ~MacroFunctionDecode();

public:
	virtual MacroValuePtr value(MacroContext* pContext) const;

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
	MacroFunctionDefun();
	virtual ~MacroFunctionDefun();

public:
	virtual MacroValuePtr value(MacroContext* pContext) const;

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
	MacroFunctionDelete();
	virtual ~MacroFunctionDelete();

public:
	virtual MacroValuePtr value(MacroContext* pContext) const;

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
	MacroFunctionEqual();
	virtual ~MacroFunctionEqual();

public:
	virtual MacroValuePtr value(MacroContext* pContext) const;

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
	MacroFunctionEval();
	virtual ~MacroFunctionEval();

public:
	virtual MacroValuePtr value(MacroContext* pContext) const;

protected:
	virtual const WCHAR* getName() const;

private:
	MacroFunctionEval(const MacroFunctionEval&);
	MacroFunctionEval& operator=(const MacroFunctionEval&);
};


/****************************************************************************
 *
 * MacroFunctionExecute
 *
 */

class MacroFunctionExecute : public MacroFunction
{
public:
	MacroFunctionExecute();
	virtual ~MacroFunctionExecute();

public:
	virtual MacroValuePtr value(MacroContext* pContext) const;

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
	MacroFunctionExist();
	virtual ~MacroFunctionExist();

public:
	virtual MacroValuePtr value(MacroContext* pContext) const;

protected:
	virtual const WCHAR* getName() const;
	virtual MacroContext::MessageType getFunctionMessageTypeHint() const;

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
	MacroFunctionExit();
	virtual ~MacroFunctionExit();

public:
	virtual MacroValuePtr value(MacroContext* pContext) const;

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
	MacroFunctionField();
	virtual ~MacroFunctionField();

public:
	virtual MacroValuePtr value(MacroContext* pContext) const;

protected:
	virtual const WCHAR* getName() const;
	virtual MacroContext::MessageType getFunctionMessageTypeHint() const;

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
	MacroFunctionFieldParameter();
	virtual ~MacroFunctionFieldParameter();

public:
	virtual MacroValuePtr value(MacroContext* pContext) const;

protected:
	virtual const WCHAR* getName() const;
	virtual MacroContext::MessageType getFunctionMessageTypeHint() const;

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
	MacroFunctionFind();
	virtual ~MacroFunctionFind();

public:
	virtual MacroValuePtr value(MacroContext* pContext) const;

protected:
	virtual const WCHAR* getName() const;

private:
	MacroFunctionFind(const MacroFunctionFind&);
	MacroFunctionFind& operator=(const MacroFunctionFind&);
};


/****************************************************************************
 *
 * MacroFunctionFindEach
 *
 */

class MacroFunctionFindEach : public MacroFunction
{
public:
	MacroFunctionFindEach();
	virtual ~MacroFunctionFindEach();

public:
	virtual MacroValuePtr value(MacroContext* pContext) const;

protected:
	virtual const WCHAR* getName() const;

private:
	MacroFunctionFindEach(const MacroFunctionFindEach&);
	MacroFunctionFindEach& operator=(const MacroFunctionFindEach&);
};


/****************************************************************************
 *
 * MacroFunctionFlag
 *
 */

class MacroFunctionFlag : public MacroFunction
{
public:
	MacroFunctionFlag();
	explicit MacroFunctionFlag(MessageHolder::Flag flag);
	virtual ~MacroFunctionFlag();

public:
	virtual MacroValuePtr value(MacroContext* pContext) const;

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
	MacroFunctionFolder();
	virtual ~MacroFunctionFolder();

public:
	virtual MacroValuePtr value(MacroContext* pContext) const;

protected:
	virtual const WCHAR* getName() const;

private:
	MacroFunctionFolder(const MacroFunctionFolder&);
	MacroFunctionFolder& operator=(const MacroFunctionFolder&);
};


/****************************************************************************
 *
 * MacroFunctionFolderFlag
 *
 */

class MacroFunctionFolderFlag : public MacroFunction
{
public:
	MacroFunctionFolderFlag();
	virtual ~MacroFunctionFolderFlag();

public:
	virtual MacroValuePtr value(MacroContext* pContext) const;

protected:
	virtual const WCHAR* getName() const;

private:
	MacroFunctionFolderFlag(const MacroFunctionFolderFlag&);
	MacroFunctionFolderFlag& operator=(const MacroFunctionFolderFlag&);
};


/****************************************************************************
 *
 * MacroFunctionFolderParameter
 *
 */

class MacroFunctionFolderParameter : public MacroFunction
{
public:
	MacroFunctionFolderParameter();
	virtual ~MacroFunctionFolderParameter();

public:
	virtual MacroValuePtr value(MacroContext* pContext) const;

protected:
	virtual const WCHAR* getName() const;

private:
	MacroFunctionFolderParameter(const MacroFunctionFolderParameter&);
	MacroFunctionFolderParameter& operator=(const MacroFunctionFolderParameter&);
};


/****************************************************************************
 *
 * MacroFunctionForEach
 *
 */

class MacroFunctionForEach : public MacroFunction
{
public:
	MacroFunctionForEach();
	virtual ~MacroFunctionForEach();

public:
	virtual MacroValuePtr value(MacroContext* pContext) const;

protected:
	virtual const WCHAR* getName() const;

private:
	MacroFunctionForEach(const MacroFunctionForEach&);
	MacroFunctionForEach& operator=(const MacroFunctionForEach&);
};


/****************************************************************************
 *
 * MacroFunctionFormatAddress
 *
 */

class MacroFunctionFormatAddress : public MacroFunction
{
public:
	MacroFunctionFormatAddress();
	virtual ~MacroFunctionFormatAddress();

public:
	virtual MacroValuePtr value(MacroContext* pContext) const;

protected:
	virtual const WCHAR* getName() const;

private:
	static void replacePhrase(AddressBook* pAddressBook,
							  qs::AddressListParser* pAddressList,
							  bool bForce);
	static void replacePhrase(AddressBook* pAddressBook,
							  qs::AddressParser* pAddress,
							  bool bForce);

private:
	MacroFunctionFormatAddress(const MacroFunctionFormatAddress&);
	MacroFunctionFormatAddress& operator=(const MacroFunctionFormatAddress&);
};


/****************************************************************************
 *
 * MacroFunctionFormatDate
 *
 */

class MacroFunctionFormatDate : public MacroFunction
{
public:
	MacroFunctionFormatDate();
	virtual ~MacroFunctionFormatDate();

public:
	virtual MacroValuePtr value(MacroContext* pContext) const;

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
	explicit MacroFunctionFunction(const WCHAR* pwszName);
	virtual ~MacroFunctionFunction();

public:
	virtual MacroValuePtr value(MacroContext* pContext) const;

protected:
	virtual const WCHAR* getName() const;

private:
	MacroFunctionFunction(const MacroFunctionFunction&);
	MacroFunctionFunction& operator=(const MacroFunctionFunction&);

private:
	qs::wstring_ptr wstrName_;
};


/****************************************************************************
 *
 * MacroFunctionHeader
 *
 */

class MacroFunctionHeader : public MacroFunction
{
public:
	MacroFunctionHeader();
	virtual ~MacroFunctionHeader();

public:
	virtual MacroValuePtr value(MacroContext* pContext) const;

protected:
	virtual const WCHAR* getName() const;
	virtual MacroContext::MessageType getFunctionMessageTypeHint() const;

private:
	MacroFunctionHeader(const MacroFunctionHeader&);
	MacroFunctionHeader& operator=(const MacroFunctionHeader&);
};


/****************************************************************************
 *
 * MacroFunctionHtmlEscape
 *
 */

class MacroFunctionHtmlEscape : public MacroFunction
{
public:
	MacroFunctionHtmlEscape();
	virtual ~MacroFunctionHtmlEscape();

public:
	virtual MacroValuePtr value(MacroContext* pContext) const;

protected:
	virtual const WCHAR* getName() const;

private:
	MacroFunctionHtmlEscape(const MacroFunctionHtmlEscape&);
	MacroFunctionHtmlEscape& operator=(const MacroFunctionHtmlEscape&);
};


/****************************************************************************
 *
 * MacroFunctionI
 *
 */

class MacroFunctionI : public MacroFunction
{
public:
	MacroFunctionI();
	virtual ~MacroFunctionI();

public:
	virtual MacroValuePtr value(MacroContext* pContext) const;

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
	MacroFunctionId();
	virtual ~MacroFunctionId();

public:
	virtual MacroValuePtr value(MacroContext* pContext) const;

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
	MacroFunctionIdentity();
	virtual ~MacroFunctionIdentity();

public:
	virtual MacroValuePtr value(MacroContext* pContext) const;

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
	MacroFunctionIf();
	virtual ~MacroFunctionIf();

public:
	virtual MacroValuePtr value(MacroContext* pContext) const;

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
	MacroFunctionInclude();
	virtual ~MacroFunctionInclude();

public:
	virtual MacroValuePtr value(MacroContext* pContext) const;

protected:
	virtual const WCHAR* getName() const;

private:
	MacroFunctionInclude(const MacroFunctionInclude&);
	MacroFunctionInclude& operator=(const MacroFunctionInclude&);
};


/****************************************************************************
 *
 * MacroFunctionInputBox
 *
 */

class MacroFunctionInputBox : public MacroFunction
{
public:
	MacroFunctionInputBox();
	virtual ~MacroFunctionInputBox();

public:
	virtual MacroValuePtr value(MacroContext* pContext) const;

protected:
	virtual const WCHAR* getName() const;

private:
	MacroFunctionInputBox(const MacroFunctionInputBox&);
	MacroFunctionInputBox& operator=(const MacroFunctionInputBox&);
};


/****************************************************************************
 *
 * MacroFunctionJunk
 *
 */

class MacroFunctionJunk : public MacroFunction
{
public:
	MacroFunctionJunk();
	virtual ~MacroFunctionJunk();

public:
	virtual MacroValuePtr value(MacroContext* pContext) const;

protected:
	virtual const WCHAR* getName() const;

private:
	MacroFunctionJunk(const MacroFunctionJunk&);
	MacroFunctionJunk& operator=(const MacroFunctionJunk&);
};


/****************************************************************************
 *
 * MacroFunctionLabel
 *
 */

class MacroFunctionLabel : public MacroFunction
{
public:
	MacroFunctionLabel();
	virtual ~MacroFunctionLabel();

public:
	virtual MacroValuePtr value(MacroContext* pContext) const;

protected:
	virtual const WCHAR* getName() const;

private:
	MacroFunctionLabel(const MacroFunctionLabel&);
	MacroFunctionLabel& operator=(const MacroFunctionLabel&);
};


/****************************************************************************
 *
 * MacroFunctionLength
 *
 */

class MacroFunctionLength : public MacroFunction
{
public:
	MacroFunctionLength();
	virtual ~MacroFunctionLength();

public:
	virtual MacroValuePtr value(MacroContext* pContext) const;

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
	MacroFunctionLoad();
	virtual ~MacroFunctionLoad();

public:
	virtual MacroValuePtr value(MacroContext* pContext) const;

protected:
	virtual const WCHAR* getName() const;

private:
	MacroFunctionLoad(const MacroFunctionLoad&);
	MacroFunctionLoad& operator=(const MacroFunctionLoad&);
};


/****************************************************************************
 *
 * MacroFunctionLookupAddressBook
 *
 */

class MacroFunctionLookupAddressBook : public MacroFunction
{
public:
	MacroFunctionLookupAddressBook();
	virtual ~MacroFunctionLookupAddressBook();

public:
	virtual MacroValuePtr value(MacroContext* pContext) const;

protected:
	virtual const WCHAR* getName() const;

private:
	MacroFunctionLookupAddressBook(const MacroFunctionLookupAddressBook&);
	MacroFunctionLookupAddressBook& operator=(const MacroFunctionLookupAddressBook&);
};


/****************************************************************************
 *
 * MacroFunctionMessageBox
 *
 */

class MacroFunctionMessageBox : public MacroFunction
{
public:
	MacroFunctionMessageBox();
	virtual ~MacroFunctionMessageBox();

public:
	virtual MacroValuePtr value(MacroContext* pContext) const;

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
	MacroFunctionMessages();
	virtual ~MacroFunctionMessages();

public:
	virtual MacroValuePtr value(MacroContext* pContext) const;

protected:
	virtual const WCHAR* getName() const;

private:
	MacroFunctionMessages(const MacroFunctionMessages&);
	MacroFunctionMessages& operator=(const MacroFunctionMessages&);
};


/****************************************************************************
 *
 * MacroFunctionNew
 *
 */

class MacroFunctionNew : public MacroFunction
{
public:
	MacroFunctionNew();
	virtual ~MacroFunctionNew();

public:
	virtual MacroValuePtr value(MacroContext* pContext) const;

protected:
	virtual const WCHAR* getName() const;

private:
	MacroFunctionNew(const MacroFunctionNew&);
	MacroFunctionNew& operator=(const MacroFunctionNew&);
};


/****************************************************************************
 *
 * MacroFunctionNot
 *
 */

class MacroFunctionNot : public MacroFunction
{
public:
	MacroFunctionNot();
	virtual ~MacroFunctionNot();

public:
	virtual MacroValuePtr value(MacroContext* pContext) const;

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
	MacroFunctionOr();
	virtual ~MacroFunctionOr();

public:
	virtual MacroValuePtr value(MacroContext* pContext) const;

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
	MacroFunctionOSVersion();
	virtual ~MacroFunctionOSVersion();

public:
	virtual MacroValuePtr value(MacroContext* pContext) const;

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
	MacroFunctionParseURL();
	virtual ~MacroFunctionParseURL();

public:
	virtual MacroValuePtr value(MacroContext* pContext) const;

protected:
	virtual const WCHAR* getName() const;

private:
	static qs::wstring_ptr decode(const WCHAR* p,
								  size_t nLen);
	static bool isHex(WCHAR c);

private:
	MacroFunctionParseURL(const MacroFunctionParseURL&);
	MacroFunctionParseURL& operator=(const MacroFunctionParseURL&);
};


/****************************************************************************
 *
 * MacroFunctionParam
 *
 */

class MacroFunctionParam : public MacroFunction
{
public:
	MacroFunctionParam();
	virtual ~MacroFunctionParam();

public:
	virtual MacroValuePtr value(MacroContext* pContext) const;

protected:
	virtual const WCHAR* getName() const;

private:
	MacroFunctionParam(const MacroFunctionParam&);
	MacroFunctionParam& operator=(const MacroFunctionParam&);
};


/****************************************************************************
 *
 * MacroFunctionPart
 *
 */

class MacroFunctionPart : public MacroFunction
{
public:
	MacroFunctionPart();
	virtual ~MacroFunctionPart();

public:
	virtual MacroValuePtr value(MacroContext* pContext) const;

protected:
	virtual const WCHAR* getName() const;
	virtual MacroContext::MessageType getFunctionMessageTypeHint() const;

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
	MacroFunctionPassed();
	virtual ~MacroFunctionPassed();

public:
	virtual MacroValuePtr value(MacroContext* pContext) const;

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
	MacroFunctionProcessId();
	virtual ~MacroFunctionProcessId();

public:
	virtual MacroValuePtr value(MacroContext* pContext) const;

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
	MacroFunctionProfile();
	virtual ~MacroFunctionProfile();

public:
	virtual MacroValuePtr value(MacroContext* pContext) const;

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
	MacroFunctionProfileName();
	virtual ~MacroFunctionProfileName();

public:
	virtual MacroValuePtr value(MacroContext* pContext) const;

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
	MacroFunctionProgn();
	virtual ~MacroFunctionProgn();

public:
	virtual MacroValuePtr value(MacroContext* pContext) const;

protected:
	virtual const WCHAR* getName() const;

private:
	MacroFunctionProgn(const MacroFunctionProgn&);
	MacroFunctionProgn& operator=(const MacroFunctionProgn&);
};


/****************************************************************************
 *
 * MacroFunctionQuote
 *
 */

class MacroFunctionQuote : public MacroFunction
{
public:
	MacroFunctionQuote();
	virtual ~MacroFunctionQuote();

public:
	virtual MacroValuePtr value(MacroContext* pContext) const;

protected:
	virtual const WCHAR* getName() const;

private:
	MacroFunctionQuote(const MacroFunctionQuote&);
	MacroFunctionQuote& operator=(const MacroFunctionQuote&);
};


/****************************************************************************
 *
 * MacroFunctionReferences
 *
 */

class MacroFunctionReferences : public MacroFunction
{
public:
	MacroFunctionReferences();
	virtual ~MacroFunctionReferences();

public:
	virtual MacroValuePtr value(MacroContext* pContext) const;

protected:
	virtual const WCHAR* getName() const;
	virtual MacroContext::MessageType getFunctionMessageTypeHint() const;

private:
	MacroFunctionReferences(const MacroFunctionReferences&);
	MacroFunctionReferences& operator=(const MacroFunctionReferences&);
};


/****************************************************************************
 *
 * MacroFunctionRegexFind
 *
 */

class MacroFunctionRegexFind : public MacroFunction
{
public:
	MacroFunctionRegexFind();
	virtual ~MacroFunctionRegexFind();

public:
	virtual MacroValuePtr value(MacroContext* pContext) const;

protected:
	virtual const WCHAR* getName() const;

private:
	MacroFunctionRegexFind(const MacroFunctionRegexFind&);
	MacroFunctionRegexFind& operator=(const MacroFunctionRegexFind&);
};


/****************************************************************************
 *
 * MacroFunctionRegexMatch
 *
 */

class MacroFunctionRegexMatch : public MacroFunction
{
public:
	MacroFunctionRegexMatch();
	virtual ~MacroFunctionRegexMatch();

public:
	virtual MacroValuePtr value(MacroContext* pContext) const;

protected:
	virtual const WCHAR* getName() const;

private:
	MacroFunctionRegexMatch(const MacroFunctionRegexMatch&);
	MacroFunctionRegexMatch& operator=(const MacroFunctionRegexMatch&);
};


/****************************************************************************
 *
 * MacroFunctionRegexReplace
 *
 */

class MacroFunctionRegexReplace : public MacroFunction
{
public:
	MacroFunctionRegexReplace();
	virtual ~MacroFunctionRegexReplace();

public:
	virtual MacroValuePtr value(MacroContext* pContext) const;

protected:
	virtual const WCHAR* getName() const;

private:
	MacroFunctionRegexReplace(const MacroFunctionRegexReplace&);
	MacroFunctionRegexReplace& operator=(const MacroFunctionRegexReplace&);
};


/****************************************************************************
 *
 * MacroFunctionRelative
 *
 */

class MacroFunctionRelative : public MacroFunction
{
public:
	explicit MacroFunctionRelative(bool bLess);
	virtual ~MacroFunctionRelative();

public:
	virtual MacroValuePtr value(MacroContext* pContext) const;

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
	MacroFunctionRemove();
	virtual ~MacroFunctionRemove();

public:
	virtual MacroValuePtr value(MacroContext* pContext) const;

protected:
	virtual const WCHAR* getName() const;

private:
	static void remove(qs::AddressListParser* pAddressList,
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
	MacroFunctionSave();
	virtual ~MacroFunctionSave();

public:
	virtual MacroValuePtr value(MacroContext* pContext) const;

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
	MacroFunctionScript();
	virtual ~MacroFunctionScript();

public:
	virtual MacroValuePtr value(MacroContext* pContext) const;

protected:
	virtual const WCHAR* getName() const;

private:
	MacroFunctionScript(const MacroFunctionScript&);
	MacroFunctionScript& operator=(const MacroFunctionScript&);
};


/****************************************************************************
 *
 * MacroFunctionSelectBox
 *
 */

class MacroFunctionSelectBox : public MacroFunction
{
public:
	MacroFunctionSelectBox();
	virtual ~MacroFunctionSelectBox();

public:
	virtual MacroValuePtr value(MacroContext* pContext) const;

protected:
	virtual const WCHAR* getName() const;

private:
	MacroFunctionSelectBox(const MacroFunctionSelectBox&);
	MacroFunctionSelectBox& operator=(const MacroFunctionSelectBox&);
};


/****************************************************************************
 *
 * MacroFunctionSelected
 *
 */

class MacroFunctionSelected : public MacroFunction
{
public:
	MacroFunctionSelected();
	virtual ~MacroFunctionSelected();

public:
	virtual MacroValuePtr value(MacroContext* pContext) const;

protected:
	virtual const WCHAR* getName() const;

private:
	MacroFunctionSelected(const MacroFunctionSelected&);
	MacroFunctionSelected& operator=(const MacroFunctionSelected&);
};


/****************************************************************************
 *
 * MacroFunctionSet
 *
 */

class MacroFunctionSet : public MacroFunction
{
public:
	MacroFunctionSet();
	virtual ~MacroFunctionSet();

public:
	virtual MacroValuePtr value(MacroContext* pContext) const;

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
	MacroFunctionSize();
	virtual ~MacroFunctionSize();

public:
	virtual MacroValuePtr value(MacroContext* pContext) const;

protected:
	virtual const WCHAR* getName() const;

private:
	MacroFunctionSize(const MacroFunctionSize&);
	MacroFunctionSize& operator=(const MacroFunctionSize&);
};


/****************************************************************************
 *
 * MacroFunctionSpecialFolder
 *
 */

class MacroFunctionSpecialFolder : public MacroFunction
{
public:
	MacroFunctionSpecialFolder();
	virtual ~MacroFunctionSpecialFolder();

public:
	virtual MacroValuePtr value(MacroContext* pContext) const;

protected:
	virtual const WCHAR* getName() const;

private:
	MacroFunctionSpecialFolder(const MacroFunctionSpecialFolder&);
	MacroFunctionSpecialFolder& operator=(const MacroFunctionSpecialFolder&);
};


/****************************************************************************
 *
 * MacroFunctionSubAccount
 *
 */

class MacroFunctionSubAccount : public MacroFunction
{
public:
	MacroFunctionSubAccount();
	virtual ~MacroFunctionSubAccount();

public:
	virtual MacroValuePtr value(MacroContext* pContext) const;

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
	MacroFunctionSubject();
	virtual ~MacroFunctionSubject();

public:
	virtual MacroValuePtr value(MacroContext* pContext) const;

protected:
	virtual const WCHAR* getName() const;
	virtual MacroContext::MessageType getFunctionMessageTypeHint() const;

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
	MacroFunctionSubstring();
	virtual ~MacroFunctionSubstring();

public:
	virtual MacroValuePtr value(MacroContext* pContext) const;

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
	explicit MacroFunctionSubstringSep(bool bAfter);
	virtual ~MacroFunctionSubstringSep();

public:
	virtual MacroValuePtr value(MacroContext* pContext) const;

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
 * MacroFunctionThread
 *
 */

class MacroFunctionThread : public MacroFunction
{
private:
	class Item
	{
	public:
		explicit Item(MessageHolder* pmh);
		Item(const Item& item);
	
	private:
		explicit Item(unsigned int nMessageIdHash);
	
	public:
		~Item();
	
	public:
		MessageHolder* getMessageHolder() const;
		unsigned int getMessageIdHash() const;
		Item* getParentItem() const;
		void setParentItem(Item* pParent);
		unsigned int getLevel() const;
		bool isChecked() const;
		bool isIncluded() const;
		void setChecked(bool bIncluded);
	
	public:
		static Item createItemWithMessageIdHash(unsigned int nMessageIdHash);
	
	private:
		Item& operator=(const Item&);
	
	private:
		enum Flag {
			FLAG_NONE		= 0x00,
			FLAG_CHECKED	= 0x01,
			FLAG_INCLUDED	= 0x02
		};
	
	private:
		MessageHolder* pmh_;
		union {
			Item* pParent_;
			unsigned int nMessageIdHash_;
		};
		unsigned int nFlags_;
	};

private:
	typedef std::vector<Item> ItemList;
	typedef std::vector<Item*> ItemPtrList;

public:
	MacroFunctionThread();
	virtual ~MacroFunctionThread();

public:
	virtual MacroValuePtr value(MacroContext* pContext) const;

protected:
	virtual const WCHAR* getName() const;

private:
	static unsigned int getMessageCount(const Account* pAccount);
	static void getItems(const Folder* pFolder,
						 const MessageHolder* pmhThis,
						 ItemList* pListItem,
						 ItemPtrList* pListItemPtr,
						 Item** ppItemThis);
	static void checkItem(Item* pItem,
						  const MessageHolder* pmhThis,
						  ItemPtrList* pListItemInThread);

private:
	MacroFunctionThread(const MacroFunctionThread&);
	MacroFunctionThread& operator=(const MacroFunctionThread&);

private:
	struct ItemLess : public std::binary_function<Item*, Item*, bool>
	{
		bool operator()(const Item* pLhs,
						const Item* pRhs);
	};
};


/****************************************************************************
 *
 * MacroFunctionURI
 *
 */

class MacroFunctionURI : public MacroFunction
{
public:
	MacroFunctionURI();
	virtual ~MacroFunctionURI();

public:
	virtual MacroValuePtr value(MacroContext* pContext) const;

protected:
	virtual const WCHAR* getName() const;

private:
	MacroFunctionURI(const MacroFunctionURI&);
	MacroFunctionURI& operator=(const MacroFunctionURI&);
};


/****************************************************************************
 *
 * MacroFunctionVariable
 *
 */

class MacroFunctionVariable : public MacroFunction
{
public:
	MacroFunctionVariable();
	virtual ~MacroFunctionVariable();

public:
	virtual MacroValuePtr value(MacroContext* pContext) const;

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
	MacroFunctionWhile();
	virtual ~MacroFunctionWhile();

public:
	virtual MacroValuePtr value(MacroContext* pContext) const;

protected:
	virtual const WCHAR* getName() const;

private:
	MacroFunctionWhile(const MacroFunctionWhile&);
	MacroFunctionWhile& operator=(const MacroFunctionWhile&);
};


/****************************************************************************
 *
 * MacroExprVisitor
 *
 */

class MacroExprVisitor
{
public:
	virtual ~MacroExprVisitor();

public:
	virtual void visitField(const MacroField& field) = 0;
	virtual void visitFieldCache(const MacroFieldCache& fieldCache) = 0;
	virtual void visitLiteral(const MacroLiteral& literal) = 0;
	virtual void visitNumber(const MacroNumber& number) = 0;
	virtual void visitBoolean(const MacroBoolean& boolean) = 0;
	virtual void visitRegex(const MacroRegex& regex) = 0;
	virtual void visitVariable(const MacroVariable& variable) = 0;
	virtual void visitConstant(const MacroConstant& constant) = 0;
	virtual void visitFunction(const MacroFunction& function) = 0;
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
	explicit MacroExprPtr(MacroExpr* pExpr);
	MacroExprPtr(MacroExprPtr& pExpr);
	~MacroExprPtr();

public:
	MacroExprPtr& operator=(MacroExprPtr& pExpr);

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
	std::auto_ptr<MacroFunction> newFunction(const WCHAR* pwszName) const;

public:
	static const MacroFunctionFactory& getFactory();

private:
	MacroFunctionFactory(const MacroFunctionFactory&);
	MacroFunctionFactory& operator=(const MacroFunctionFactory&);

private:
	static MacroFunctionFactory factory__;
};


/****************************************************************************
 *
 * MacroConstantFactory
 *
 */

class MacroConstantFactory
{
private:
	MacroConstantFactory();

public:
	virtual ~MacroConstantFactory();

public:
	std::auto_ptr<MacroConstant> getConstant(const WCHAR* pwszName) const;

public:
	static const MacroConstantFactory& getFactory();

private:
	MacroConstantFactory(const MacroConstantFactory&);
	MacroConstantFactory& operator=(const MacroConstantFactory&);

private:
	static MacroConstantFactory factory__;
};

}

#endif // __MACRO_H__
