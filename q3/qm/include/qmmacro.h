/*
 * $Id$
 *
 * Copyright(C) 1998-2004 Satoshi Nakamura
 * All rights reserved.
 *
 */

#ifndef __QMMACRO_H__
#define __QMMACRO_H__

#include <qm.h>
#include <qmmessageholder.h>

#include <qs.h>
#include <qsmime.h>
#include <qsprofile.h>
#include <qsregex.h>
#include <qsstring.h>
#include <qsthread.h>
#include <qsutil.h>

#include <vector>


namespace qm {

class MacroErrorHandler;
class Macro;
class MacroParser;
class MacroVariableHolder;
class MacroContext;
class MacroValue;
	class MacroValueBoolean;
	class MacroValueString;
	class MacroValueNumber;
	class MacroValueRegex;
	class MacroValueField;
	class MacroValueAddress;
	class MacroValueTime;
	class MacroValuePart;
	class MacroValueMessageList;
class MacroValuePtr;
class MacroFactory;

class Account;
class Document;
class MacroExpr;
	class MacroFunction;
class MacroExprPtr;
class MacroGlobalContext;
class MacroFunctionHolder;
class Message;


/****************************************************************************
 *
 * MacroErrorHandler
 *
 */

class MacroErrorHandler
{
public:
	enum Code {
		CODE_SYNTAXERROR,
		CODE_INVALIDFUNCTIONNAME,
		CODE_FUNCTIONWITHOUTPARENTHESIS,
		CODE_INVALIDVARIABLENAME,
		CODE_INVALIDFIELDNAME,
		CODE_INVALIDREGEX,
		CODE_MACROCONTAINSMORETHANONEEXPR,
		
		CODE_FAIL,
		CODE_INVALIDARGSIZE,
		CODE_INVALIDARGTYPE,
		CODE_INVALIDARGVALUE,
		CODE_NOCONTEXTMESSAGE,
		CODE_UNKNOWNFUNCTION,
		CODE_INVALIDPART,
		CODE_UNKNOWNACCOUNT,
		CODE_GETMESSAGE
	};

public:
	virtual ~MacroErrorHandler();
	virtual void parseError(Code code,
							const WCHAR* pwsz) = 0;
	virtual void processError(Code code,
							  const WCHAR* pwsz) = 0;
};


/****************************************************************************
 *
 * Macro
 *
 */

#pragma warning(push)
#pragma warning(disable:4251)

class QMEXPORTCLASS Macro
{
public:
	explicit Macro(MacroExprPtr pExpr);
	~Macro();

public:
	MacroValuePtr value(MacroContext* pContext) const;
	qs::wstring_ptr getString() const;

private:
	Macro(const Macro&);
	Macro& operator=(const Macro&);

private:
	MacroExpr* pExpr_;
};

#pragma warning(pop)


/****************************************************************************
 *
 * MacroParser
 *
 */

class QMEXPORTCLASS MacroParser
{
public:
	enum Type {
		TYPE_COLOR			= 0x01,
		TYPE_COLUMN			= 0x02,
		TYPE_FILTER			= 0x04,
		TYPE_HEADER			= 0x08,
		TYPE_MESSAGE		= 0x10,
		TYPE_RULE			= 0x20,
		TYPE_SYNCFILTER		= 0x40,
		TYPE_TEMPLATE		= 0x80
	};

public:
	explicit MacroParser(Type type);
	~MacroParser();

public:
	std::auto_ptr<Macro> parse(const WCHAR* pwszMacro) const;
	std::auto_ptr<Macro> parse(const WCHAR* pwszMacro,
							   Macro* pParentMacro) const;
	void setErrorHandler(MacroErrorHandler* pErrorHandler);
	MacroErrorHandler* getErrorHandler() const;

public:
	static bool isNumber(const WCHAR* pwsz);

private:
	std::auto_ptr<Macro> error(MacroErrorHandler::Code code,
							   const WCHAR* p) const;

private:
	MacroParser(const MacroParser&);
	MacroParser& operator=(const MacroParser&);

private:
	Type type_;
	MacroErrorHandler* pErrorHandler_;
};


/****************************************************************************
 *
 * MacroVariableHolder
 *
 */

class QMEXPORTCLASS MacroVariableHolder
{
public:
	MacroVariableHolder();
	~MacroVariableHolder();

public:
	MacroValuePtr getVariable(const WCHAR* pwszName) const;
	void setVariable(const WCHAR* pwszName,
					 MacroValuePtr pValue);
	void removeVariable(const WCHAR* pwszName);

private:
	MacroVariableHolder(const MacroVariableHolder&);
	MacroVariableHolder& operator=(const MacroVariableHolder&);

private:
	struct MacroVariableHolderImpl* pImpl_;
};


/****************************************************************************
 *
 * MacroContext
 *
 */

class QMEXPORTCLASS MacroContext
{
public:
	enum MessageType {
		MESSAGETYPE_HEADER,
		MESSAGETYPE_TEXT,
		MESSAGETYPE_ALL
	};
	
	enum ReturnType {
		RETURNTYPE_NONE,
		RETURNTYPE_CANCEL,
		RETURNTYPE_EXIT
	};

public:
	MacroContext(MessageHolderBase* pmh,
				 Message* pMessage,
				 const MessageHolderList& listSelected,
				 Account* pAccount,
				 Document* pDocument,
				 HWND hwnd,
				 qs::Profile* pProfile,
				 bool bGetMessageAsPossible,
				 bool bDecryptVerify,
				 MacroErrorHandler* pErrorHandler,
				 MacroVariableHolder* pGlobalVariable);
	MacroContext(MessageHolderBase* pmh,
				 Message* pMessage,
				 const MacroContext* pContext);
	~MacroContext();

public:
	MessageHolderBase* getMessageHolder() const;
	Message* getMessage() const;
	Message* getMessage(MessageType type,
						const WCHAR* pwszField) const;
	const MessageHolderList& getSelectedMessageHolders() const;
	Account* getAccount() const;
	Document* getDocument() const;
	HWND getWindow() const;
	qs::Profile* getProfile() const;
	bool isGetMessageAsPossible() const;
	bool isDecryptVerify() const;
	MacroErrorHandler* getErrorHandler() const;
	ReturnType getReturnType() const;
	void setReturnType(ReturnType type);
	MacroValuePtr getVariable(const WCHAR* pwszName) const;
	bool setVariable(const WCHAR* pwszName,
					 MacroValue* pValue,
					 bool bGlobal);
	const MacroExpr* getFunction(const WCHAR* pwszName) const;
	bool setFunction(const WCHAR* pwszName,
					 const MacroExpr* pExpr);
	void pushArgumentContext();
	void popArgumentContext();
	void addArgument(MacroValuePtr pValue);
	MacroValuePtr getArgument(unsigned int n) const;
	bool setRegexResult(const qs::RegexRangeList& listRange);
	void clearRegexResult();
	qs::wstring_ptr resolvePath(const WCHAR* pwszPath);

private:
	MacroContext(MacroContext&);
	MacroContext& operator=(MacroContext&);

private:
	MessageHolderBase* pmh_;
	Message* pMessage_;
	Account* pAccount_;
	MacroGlobalContext* pGlobalContext_;
	bool bOwnGlobalContext_;
};


/****************************************************************************
 *
 * MacroValue
 *
 */

class QMEXPORTCLASS MacroValue
{
public:
	enum Type {
		TYPE_BOOLEAN,
		TYPE_STRING,
		TYPE_NUMBER,
		TYPE_REGEX,
		TYPE_FIELD,
		TYPE_ADDRESS,
		TYPE_TIME,
		TYPE_PART,
		TYPE_MESSAGELIST
	};

protected:
	explicit MacroValue(Type type);

public:
	virtual ~MacroValue();

public:
	Type getType() const;

public:
	virtual qs::wstring_ptr string() const = 0;
	virtual bool boolean() const = 0;
	virtual unsigned int number() const = 0;
	virtual MacroValuePtr clone() const = 0;

private:
	MacroValue(const MacroValue&);
	MacroValue& operator=(const MacroValue&);

private:
	Type type_;
};


/****************************************************************************
 *
 * MacroValueBoolean
 *
 */

class MacroValueBoolean : public MacroValue
{
public:
	MacroValueBoolean();
	virtual ~MacroValueBoolean();

public:
	void init(bool b);
	void term();

public:
	virtual qs::wstring_ptr string() const;
	virtual bool boolean() const;
	virtual unsigned int number() const;
	virtual MacroValuePtr clone() const;

private:
	MacroValueBoolean(const MacroValueBoolean&);
	MacroValueBoolean& operator=(const MacroValueBoolean&);

private:
	bool b_;
};


/****************************************************************************
 *
 * MacroValueString
 *
 */

class MacroValueString : public MacroValue
{
public:
	MacroValueString();
	virtual ~MacroValueString();

public:
	void init(const WCHAR* pwsz,
			  size_t nLen);
	void term();

public:
	virtual qs::wstring_ptr string() const;
	virtual bool boolean() const;
	virtual unsigned int number() const;
	virtual MacroValuePtr clone() const;

private:
	MacroValueString(const MacroValueString&);
	MacroValueString& operator=(const MacroValueString&);

private:
	qs::wstring_ptr wstr_;
};


/****************************************************************************
 *
 * MacroValueNumber
 *
 */

class MacroValueNumber : public MacroValue
{
public:
	MacroValueNumber();
	virtual ~MacroValueNumber();

public:
	void init(unsigned int n);
	void term();

public:
	virtual qs::wstring_ptr string() const;
	virtual bool boolean() const;
	virtual unsigned int number() const;
	virtual MacroValuePtr clone() const;

private:
	MacroValueNumber(const MacroValueNumber&);
	MacroValueNumber& operator=(const MacroValueNumber&);

private:
	unsigned int n_;
};


/****************************************************************************
 *
 * MacroValueRegex
 *
 */

class MacroValueRegex : public MacroValue
{
public:
	MacroValueRegex();
	virtual ~MacroValueRegex();

public:
	void init(const WCHAR* pwszPattern,
			  const qs::RegexPattern* pPattern);
	void term();

public:
	const qs::RegexPattern* getPattern() const;

public:
	virtual qs::wstring_ptr string() const;
	virtual bool boolean() const;
	virtual unsigned int number() const;
	virtual MacroValuePtr clone() const;

private:
	MacroValueRegex(const MacroValueRegex&);
	MacroValueRegex& operator=(const MacroValueRegex&);

private:
	const WCHAR* pwszPattern_;
	const qs::RegexPattern* pPattern_;
};


/****************************************************************************
 *
 * MacroValueField
 *
 */

class MacroValueField : public MacroValue
{
public:
	MacroValueField();
	virtual ~MacroValueField();

public:
	void init(const WCHAR* pwszName,
			  const CHAR* pszField);
	void term();

public:
	const WCHAR* getName() const;
	const CHAR* getField() const;
	bool getAddresses(std::vector<qs::WSTRING>* pAddresses) const;
	bool getNames(std::vector<qs::WSTRING>* pNames) const;

public:
	virtual qs::wstring_ptr string() const;
	virtual bool boolean() const;
	virtual unsigned int number() const;
	virtual MacroValuePtr clone() const;

private:
	bool isAddress() const;

private:
	MacroValueField(const MacroValueField&);
	MacroValueField& operator=(const MacroValueField&);

private:
	qs::wstring_ptr wstrName_;
	qs::string_ptr strField_;
};


/****************************************************************************
 *
 * MacroValueAddress
 *
 */

class MacroValueAddress : public MacroValue
{
public:
	typedef std::vector<qs::WSTRING> AddressList;

public:
	MacroValueAddress();
	virtual ~MacroValueAddress();

public:
	void init(const AddressList& l);
	void term();

public:
	const AddressList& getAddress() const;
	void remove(const WCHAR* pwszAddress);

public:
	virtual qs::wstring_ptr string() const;
	virtual bool boolean() const;
	virtual unsigned int number() const;
	virtual MacroValuePtr clone() const;

private:
	MacroValueAddress(const MacroValueAddress&);
	MacroValueAddress& operator=(const MacroValueAddress&);

private:
	AddressList listAddress_;
};


/****************************************************************************
 *
 * MacroValueTime
 *
 */

class MacroValueTime : public MacroValue
{
public:
	MacroValueTime();
	virtual ~MacroValueTime();

public:
	void init(const qs::Time& time);
	void term();

public:
	const qs::Time& getTime() const;

public:
	virtual qs::wstring_ptr string() const;
	virtual bool boolean() const;
	virtual unsigned int number() const;
	virtual MacroValuePtr clone() const;

private:
	MacroValueTime(const MacroValueTime&);
	MacroValueTime& operator=(const MacroValueTime&);

private:
	qs::Time time_;
};


/****************************************************************************
 *
 * MacroValuePart
 *
 */

class MacroValuePart : public MacroValue
{
public:
	MacroValuePart();
	virtual ~MacroValuePart();

public:
	void init(const qs::Part* pPart);
	void term();

public:
	const qs::Part* getPart() const;

public:
	virtual qs::wstring_ptr string() const;
	virtual bool boolean() const;
	virtual unsigned int number() const;
	virtual MacroValuePtr clone() const;

private:
	MacroValuePart(const MacroValuePart&);
	MacroValuePart& operator=(const MacroValuePart&);

private:
	const qs::Part* pPart_;
};


/****************************************************************************
 *
 * MacroValueMessageList
 *
 */

class MacroValueMessageList : public MacroValue
{
public:
	typedef std::vector<MessagePtr> MessageList;

public:
	MacroValueMessageList();
	virtual ~MacroValueMessageList();

public:
	void init(const MessageList& l);
	void term();

public:
	const MessageList& getMessageList() const;

public:
	virtual qs::wstring_ptr string() const;
	virtual bool boolean() const;
	virtual unsigned int number() const;
	virtual MacroValuePtr clone() const;

private:
	MacroValueMessageList(const MacroValueMessageList&);
	MacroValueMessageList& operator=(const MacroValueMessageList&);

private:
	MessageList list_;
};


/****************************************************************************
 *
 * MacroValuePtr
 *
 */

class QMEXPORTCLASS MacroValuePtr
{
public:
	MacroValuePtr();
	explicit MacroValuePtr(MacroValue* pValue);
	MacroValuePtr(MacroValuePtr& pValue);
	~MacroValuePtr();

public:
	MacroValue* operator->() const;
	MacroValue** operator&();
	MacroValuePtr& operator=(MacroValuePtr& pValue);

public:
	MacroValue* get() const;
	MacroValue* release();
	void reset(MacroValue* pValue);

private:
	MacroValue* pValue_;
};


/****************************************************************************
 *
 * MacroValueFactory
 *
 */

class QMEXPORTCLASS MacroValueFactory
{
private:
	MacroValueFactory();

public:
	~MacroValueFactory();

public:
	MacroValuePtr newBoolean(bool b);
	void deleteBoolean(MacroValueBoolean* pmvb);
	
	MacroValuePtr newString(const WCHAR* pwsz);
	MacroValuePtr newString(const WCHAR* pwsz,
							size_t nLen);
	void deleteString(MacroValueString* pmvs);
	
	MacroValuePtr newNumber(unsigned int n);
	void deleteNumber(MacroValueNumber* pmvn);
	
	MacroValuePtr newRegex(const WCHAR* pwszPattern,
						   const qs::RegexPattern* pPattern);
	void deleteRegex(MacroValueRegex* pmvr);
	
	MacroValuePtr newField(const WCHAR* pwszName,
						   const CHAR* pszField);
	void deleteField(MacroValueField* pmvf);
	
	MacroValuePtr newAddress(const MacroValueAddress::AddressList& l);
	void deleteAddress(MacroValueAddress* pmva);
	
	MacroValuePtr newTime(const qs::Time& time);
	void deleteTime(MacroValueTime* pmvt);
	
	MacroValuePtr newPart(const qs::Part* pPart);
	void deletePart(MacroValuePart* pmvp);
	
	MacroValuePtr newMessageList(const MacroValueMessageList::MessageList& l);
	void deleteMessageList(MacroValueMessageList* pmvml);
	
	void deleteValue(MacroValue* pmv);

public:
	static MacroValueFactory& getFactory();

private:
	MacroValueFactory(const MacroValueFactory&);
	MacroValueFactory& operator=(const MacroValueFactory&);

private:
	struct MacroValueFactoryImpl* pImpl_;

private:
	static MacroValueFactory factory__;
};

}

#endif // __QMMACRO_H__
