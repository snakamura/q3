/*
 * $Id$
 *
 * Copyright(C) 1998-2003 Satoshi Nakamura
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
		CODE_MACROCONTAINSMORETHANONEEXPR,
		
		CODE_FAIL,
		CODE_INVALIDARGSIZE,
		CODE_INVALIDARGTYPE,
		CODE_INVALIDARGVALUE,
		CODE_NOCONTEXTMESSAGE,
		CODE_UNKNOWNFUNCTION,
		CODE_INVALIDPART,
		CODE_UNKNOWNACCOUNT
	};

public:
	virtual ~MacroErrorHandler();
	virtual void parseError(Code code, const WCHAR* pwsz) = 0;
	virtual void processError(Code code, const WCHAR* pwsz) = 0;
};


/****************************************************************************
 *
 * Macro
 *
 */

class QMEXPORTCLASS Macro
{
public:
	Macro(MacroExpr* pExpr, qs::QSTATUS* pstatus);
	~Macro();

public:
	qs::QSTATUS value(MacroContext* pContext, MacroValue** ppValue) const;
	qs::QSTATUS getString(qs::WSTRING* pwstrMacro) const;

private:
	Macro(const Macro&);
	Macro& operator=(const Macro&);

private:
	MacroExpr* pExpr_;
};


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
	MacroParser(Type type, qs::QSTATUS* pstatus);
	~MacroParser();

public:
	qs::QSTATUS parse(const WCHAR* pwszMacro, Macro** ppMacro) const;
	qs::QSTATUS parse(const WCHAR* pwszMacro,
		Macro* pParentMacro, Macro** ppMacro) const;
	void setErrorHandler(MacroErrorHandler* pErrorHandler);
	MacroErrorHandler* getErrorHandler() const;

public:
	static bool isNumber(const WCHAR* pwsz);

private:
	qs::QSTATUS error(MacroErrorHandler::Code code, const WCHAR* p) const;

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
	explicit MacroVariableHolder(qs::QSTATUS* pstatus);
	~MacroVariableHolder();

public:
	qs::QSTATUS getVariable(const WCHAR* pwszName, MacroValue** ppValue) const;
	qs::QSTATUS setVariable(const WCHAR* pwszName, MacroValue* pValue);

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

public:
	struct Init
	{
		MessageHolderBase* pmh_;
		Message* pMessage_;
		Account* pAccount_;
		Document* pDocument_;
		HWND hwnd_;
		qs::Profile* pProfile_;
		bool bGetMessageAsPossible_;
		MacroErrorHandler* pErrorHandler_;
		MacroVariableHolder* pGlobalVariable_;
	};
	
public:
	MacroContext(const Init& init, qs::QSTATUS* pstatus);
	MacroContext(MessageHolderBase* pmh, Message* pMessage,
		const MacroContext* pContext, qs::QSTATUS* pstatus);
	~MacroContext();

public:
	MessageHolderBase* getMessageHolder() const;
	Message* getMessage() const;
	qs::QSTATUS getMessage(MessageType type,
		const WCHAR* pwszField, Message** ppMessage) const;
	Account* getAccount() const;
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
	qs::QSTATUS resolvePath(const WCHAR* pwszPath, qs::WSTRING* pwstrPath);

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
		TYPE_FIELD,
		TYPE_ADDRESS,
		TYPE_TIME,
		TYPE_PART,
		TYPE_MESSAGELIST
	};

public:
	MacroValue(Type type, qs::QSTATUS* pstatus);
	virtual ~MacroValue();

public:
	Type getType() const;

public:
	virtual qs::QSTATUS string(qs::WSTRING* pwstr) const = 0;
	virtual bool boolean() const = 0;
	virtual unsigned int number() const = 0;
	virtual qs::QSTATUS clone(MacroValue** ppValue) const = 0;

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
	MacroValueBoolean(qs::QSTATUS* pstatus);
	virtual ~MacroValueBoolean();

public:
	qs::QSTATUS init(bool b);
	void term();

public:
	virtual qs::QSTATUS string(qs::WSTRING* pwstr) const;
	virtual bool boolean() const;
	virtual unsigned int number() const;
	virtual qs::QSTATUS clone(MacroValue** ppValue) const;

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
	MacroValueString(qs::QSTATUS* pstatus);
	virtual ~MacroValueString();

public:
	qs::QSTATUS init(const WCHAR* pwsz, size_t nLen);
	void term();

public:
	virtual qs::QSTATUS string(qs::WSTRING* pwstr) const;
	virtual bool boolean() const;
	virtual unsigned int number() const;
	virtual qs::QSTATUS clone(MacroValue** ppValue) const;

private:
	MacroValueString(const MacroValueString&);
	MacroValueString& operator=(const MacroValueString&);

private:
	qs::WSTRING wstr_;
};


/****************************************************************************
 *
 * MacroValueNumber
 *
 */

class MacroValueNumber : public MacroValue
{
public:
	MacroValueNumber(qs::QSTATUS* pstatus);
	virtual ~MacroValueNumber();

public:
	qs::QSTATUS init(long n);
	void term();

public:
	virtual qs::QSTATUS string(qs::WSTRING* pwstr) const;
	virtual bool boolean() const;
	virtual unsigned int number() const;
	virtual qs::QSTATUS clone(MacroValue** ppValue) const;

private:
	MacroValueNumber(const MacroValueNumber&);
	MacroValueNumber& operator=(const MacroValueNumber&);

private:
	unsigned int n_;
};


/****************************************************************************
 *
 * MacroValueField
 *
 */

class MacroValueField : public MacroValue
{
public:
	MacroValueField(qs::QSTATUS* pstatus);
	virtual ~MacroValueField();

public:
	qs::QSTATUS init(const WCHAR* pwszName, const CHAR* pszField);
	void term();

public:
	const WCHAR* getName() const;
	const CHAR* getField() const;
	qs::QSTATUS getAddresses(std::vector<qs::WSTRING>* pAddresses) const;
	qs::QSTATUS getNames(std::vector<qs::WSTRING>* pNames) const;

public:
	virtual qs::QSTATUS string(qs::WSTRING* pwstr) const;
	virtual bool boolean() const;
	virtual unsigned int number() const;
	virtual qs::QSTATUS clone(MacroValue** ppValue) const;

private:
	bool isAddress() const;

private:
	MacroValueField(const MacroValueField&);
	MacroValueField& operator=(const MacroValueField&);

private:
	qs::WSTRING wstrName_;
	qs::STRING strField_;
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
	MacroValueAddress(qs::QSTATUS* pstatus);
	virtual ~MacroValueAddress();

public:
	qs::QSTATUS init(const AddressList& l);
	void term();

public:
	const AddressList& getAddress() const;
	void remove(const WCHAR* pwszAddress);

public:
	virtual qs::QSTATUS string(qs::WSTRING* pwstr) const;
	virtual bool boolean() const;
	virtual unsigned int number() const;
	virtual qs::QSTATUS clone(MacroValue** ppValue) const;

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
	MacroValueTime(qs::QSTATUS* pstatus);
	virtual ~MacroValueTime();

public:
	qs::QSTATUS init(const qs::Time& time);
	void term();

public:
	const qs::Time& getTime() const;

public:
	virtual qs::QSTATUS string(qs::WSTRING* pwstr) const;
	virtual bool boolean() const;
	virtual unsigned int number() const;
	virtual qs::QSTATUS clone(MacroValue** ppValue) const;

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
	MacroValuePart(qs::QSTATUS* pstatus);
	virtual ~MacroValuePart();

public:
	qs::QSTATUS init(const qs::Part* pPart);
	void term();

public:
	const qs::Part* getPart() const;

public:
	virtual qs::QSTATUS string(qs::WSTRING* pwstr) const;
	virtual bool boolean() const;
	virtual unsigned int number() const;
	virtual qs::QSTATUS clone(MacroValue** ppValue) const;

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
	MacroValueMessageList(qs::QSTATUS* pstatus);
	virtual ~MacroValueMessageList();

public:
	qs::QSTATUS init(const MessageList& l);
	void term();

public:
	const MessageList& getMessageList() const;

public:
	virtual qs::QSTATUS string(qs::WSTRING* pwstr) const;
	virtual bool boolean() const;
	virtual unsigned int number() const;
	virtual qs::QSTATUS clone(MacroValue** ppValue) const;

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
	MacroValuePtr(MacroValue* pValue);
	~MacroValuePtr();

public:
	MacroValue* operator->() const;
	MacroValue** operator&();

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
	qs::QSTATUS newBoolean(bool b, MacroValueBoolean** ppmvb);
	void deleteBoolean(MacroValueBoolean* pmvb);
	qs::QSTATUS newString(const WCHAR* pwsz, MacroValueString** ppmvs);
	qs::QSTATUS newString(const WCHAR* pwsz, size_t nLen,
		MacroValueString** ppmvs);
	void deleteString(MacroValueString* pmvs);
	qs::QSTATUS newNumber(unsigned int n, MacroValueNumber** ppmvn);
	void deleteNumber(MacroValueNumber* pmvn);
	qs::QSTATUS newField(const WCHAR* pwszName,
		const CHAR* pszField, MacroValueField** ppmvf);
	void deleteField(MacroValueField* pmvf);
	qs::QSTATUS newAddress(const MacroValueAddress::AddressList& l,
		MacroValueAddress** ppmva);
	void deleteAddress(MacroValueAddress* pmva);
	qs::QSTATUS newTime(const qs::Time& time, MacroValueTime** ppmvt);
	void deleteTime(MacroValueTime* pmvt);
	qs::QSTATUS newPart(const qs::Part* pPart, MacroValuePart** ppmvp);
	void deletePart(MacroValuePart* pmvp);
	qs::QSTATUS newMessageList(const MacroValueMessageList::MessageList& l,
		MacroValueMessageList** ppmvml);
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
