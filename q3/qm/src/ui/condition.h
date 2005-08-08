/*
 * $Id$
 *
 * Copyright(C) 1998-2005 Satoshi Nakamura
 * All rights reserved.
 *
 */

#ifndef __CONDITION_H__
#define __CONDITION_H__

#include <qm.h>

#include <qsstring.h>

#include "../macro/macro.h"


namespace qm {

class Condition;
	class NoArgumentCondition;
	class FieldCondition;
	class SizeCondition;
	class PassedCondition;
class ConditionList;
class ConditionFactory;


/****************************************************************************
 *
 * Condition
 *
 */

class Condition
{
public:
	enum Type {
		TYPE_TEXT,
		TYPE_NUMBER,
		TYPE_FIELD
	};

protected:
	explicit Condition(const WCHAR* pwszName);

public:
	virtual ~Condition();

public:
	const WCHAR* getName() const;

public:
	virtual size_t getArgumentCount() const = 0;
	virtual qs::wstring_ptr getArgumentName(size_t n) const = 0;
	virtual Type getArgumentType(size_t n) const = 0;
	virtual qs::wstring_ptr getArgumentValue(size_t n) const = 0;
	virtual void setArgumentValue(size_t n,
								  const WCHAR* pwszValue) = 0;
	virtual qs::wstring_ptr getDescription(bool bValue) const = 0;
	virtual qs::wstring_ptr getMacro() const = 0;
	virtual std::auto_ptr<Condition> clone() const = 0;

private:
	Condition(const Condition&);
	Condition& operator=(const Condition&);

private:
	const WCHAR* pwszName_;
};


/****************************************************************************
 *
 * NoArgumentCondition
 *
 */

class NoArgumentCondition : public Condition
{
public:
	NoArgumentCondition(const WCHAR* pwszName,
						UINT nDescriptionId,
						const WCHAR* pwszMacro);
	virtual ~NoArgumentCondition();

public:
	virtual size_t getArgumentCount() const;
	virtual qs::wstring_ptr getArgumentName(size_t n) const;
	virtual Type getArgumentType(size_t n) const;
	virtual qs::wstring_ptr getArgumentValue(size_t n) const;
	virtual void setArgumentValue(size_t n,
								  const WCHAR* pwszValue);
	virtual qs::wstring_ptr getDescription(bool bValue) const;
	virtual qs::wstring_ptr getMacro() const;
	virtual std::auto_ptr<Condition> clone() const;

private:
	NoArgumentCondition(const NoArgumentCondition&);
	NoArgumentCondition& operator=(const NoArgumentCondition&);

private:
	UINT nDescriptionId_;
	const WCHAR* pwszMacro_;
};


/****************************************************************************
 *
 * FieldCondition
 *
 */

class FieldCondition : public Condition
{
public:
	FieldCondition(const WCHAR* pwszName,
				   UINT nFieldId,
				   UINT nValueId,
				   UINT nDescriptionId,
				   const WCHAR* pwszMacro,
				   WCHAR cValueQuote);
	FieldCondition(const WCHAR* pwszName,
				   UINT nFieldId,
				   UINT nValueId,
				   UINT nDescriptionId,
				   const WCHAR* pwszMacro,
				   WCHAR cValueQuote,
				   const WCHAR* pwszField,
				   const WCHAR* pwszValue);
	virtual ~FieldCondition();

public:
	virtual size_t getArgumentCount() const;
	virtual qs::wstring_ptr getArgumentName(size_t n) const;
	virtual Type getArgumentType(size_t n) const;
	virtual qs::wstring_ptr getArgumentValue(size_t n) const;
	virtual void setArgumentValue(size_t n,
								  const WCHAR* pwszValue);
	virtual qs::wstring_ptr getDescription(bool bValue) const;
	virtual qs::wstring_ptr getMacro() const;
	virtual std::auto_ptr<Condition> clone() const;

private:
	static qs::wstring_ptr escape(const WCHAR* pwsz,
								  WCHAR cQuote);

private:
	FieldCondition(const FieldCondition&);
	FieldCondition& operator=(const FieldCondition&);

private:
	UINT nFieldId_;
	UINT nValueId_;
	UINT nDescriptionId_;
	const WCHAR* pwszMacro_;
	WCHAR cValueQuote_;
	qs::wstring_ptr wstrField_;
	qs::wstring_ptr wstrValue_;
};


/****************************************************************************
 *
 * SizeCondition
 *
 */

class SizeCondition : public Condition
{
public:
	SizeCondition(const WCHAR* pwszName,
				  UINT nValueId,
				  UINT nDescriptionId,
				  const WCHAR* pwszMacro);
	SizeCondition(const WCHAR* pwszName,
				  UINT nValueId,
				  UINT nDescriptionId,
				  const WCHAR* pwszMacro,
				  unsigned int nSize);
	virtual ~SizeCondition();

public:
	virtual size_t getArgumentCount() const;
	virtual qs::wstring_ptr getArgumentName(size_t n) const;
	virtual Type getArgumentType(size_t n) const;
	virtual qs::wstring_ptr getArgumentValue(size_t n) const;
	virtual void setArgumentValue(size_t n,
								  const WCHAR* pwszValue);
	virtual qs::wstring_ptr getDescription(bool bValue) const;
	virtual qs::wstring_ptr getMacro() const;
	virtual std::auto_ptr<Condition> clone() const;

private:
	SizeCondition(const SizeCondition&);
	SizeCondition& operator=(const SizeCondition&);

private:
	UINT nValueId_;
	UINT nDescriptionId_;
	const WCHAR* pwszMacro_;
	unsigned int nSize_;
};


/****************************************************************************
 *
 * PassedCondition
 *
 */

class PassedCondition : public Condition
{
public:
	PassedCondition(const WCHAR* pwszName,
					UINT nValueId,
					UINT nDescriptionId,
					bool bNot);
	PassedCondition(const WCHAR* pwszName,
					UINT nValueId,
					UINT nDescriptionId,
					bool bNot,
					unsigned int nDays);
	virtual ~PassedCondition();

public:
	virtual size_t getArgumentCount() const;
	virtual qs::wstring_ptr getArgumentName(size_t n) const;
	virtual Type getArgumentType(size_t n) const;
	virtual qs::wstring_ptr getArgumentValue(size_t n) const;
	virtual void setArgumentValue(size_t n,
								  const WCHAR* pwszValue);
	virtual qs::wstring_ptr getDescription(bool bValue) const;
	virtual qs::wstring_ptr getMacro() const;
	virtual std::auto_ptr<Condition> clone() const;

private:
	PassedCondition(const PassedCondition&);
	PassedCondition& operator=(const PassedCondition&);

private:
	UINT nValueId_;
	UINT nDescriptionId_;
	bool bNot_;
	unsigned int nDays_;
};


/****************************************************************************
 *
 * ConditionList
 *
 */

class ConditionList
{
public:
	enum Type {
		TYPE_AND,
		TYPE_OR
	};

public:
	typedef std::vector<Condition*> List;

public:
	ConditionList();
	ConditionList(const List& listCondition,
				  Type type);
	~ConditionList();

public:
	const List& getConditions() const;
	Type getType() const;
	qs::wstring_ptr getMacro() const;

public:
	void setType(Type type);
	void add(std::auto_ptr<Condition> pCondition);

private:
	ConditionList(const ConditionList&);
	ConditionList& operator=(const ConditionList&);

private:
	List list_;
	Type type_;
};


/****************************************************************************
 *
 * ConditionFactory
 *
 */

class ConditionFactory
{
public:
	typedef std::vector<const Condition*> List;

private:
	ConditionFactory();

public:
	~ConditionFactory();

public:
	const List& getConditions() const;
	std::auto_ptr<ConditionList> parse(const Macro* pMacro) const;

public:
	static const ConditionFactory& getInstance();

private:
	const Condition* getCondition(const WCHAR* pwszName) const;
	std::auto_ptr<Condition> parse(const MacroExpr* pExpr) const;
	std::auto_ptr<Condition> parsePassed(const MacroFunction* pFunction,
										 const WCHAR* pwszName) const;

private:
	ConditionFactory(const ConditionFactory&);
	ConditionFactory& operator=(const ConditionFactory&);

private:
	class GetTypeMacroExprVisitor : public MacroExprVisitor
	{
	public:
		enum Type {
			TYPE_FIELD,
			TYPE_FIELDCACHE,
			TYPE_LITERAL,
			TYPE_NUMBER,
			TYPE_REGEX,
			TYPE_VARIABLE,
			TYPE_FUNCTION
		};
	
	public:
		GetTypeMacroExprVisitor();
		virtual ~GetTypeMacroExprVisitor();
	
	public:
		Type getType() const;
	
	public:
		virtual void visitField(const MacroField& field);
		virtual void visitFieldCache(const MacroFieldCache& fieldCache);
		virtual void visitLiteral(const MacroLiteral& literal);
		virtual void visitNumber(const MacroNumber& number);
		virtual void visitRegex(const MacroRegex& regex);
		virtual void visitVariable(const MacroVariable& variable);
		virtual void visitFunction(const MacroFunction& function);
	
	private:
		Type type_;
	};
	
	class GetFunctionNameMacroExprVisitor : public MacroExprVisitor
	{
	public:
		GetFunctionNameMacroExprVisitor();
		virtual ~GetFunctionNameMacroExprVisitor();
	
	public:
		const WCHAR* getName() const;
	
	public:
		virtual void visitField(const MacroField& field);
		virtual void visitFieldCache(const MacroFieldCache& fieldCache);
		virtual void visitLiteral(const MacroLiteral& literal);
		virtual void visitNumber(const MacroNumber& number);
		virtual void visitRegex(const MacroRegex& regex);
		virtual void visitVariable(const MacroVariable& variable);
		virtual void visitFunction(const MacroFunction& function);
	
	private:
		const WCHAR* pwszName_;
	};

private:
	List list_;

private:
	static const ConditionFactory factory__;
};

}

#endif // __CONDITION_H__
