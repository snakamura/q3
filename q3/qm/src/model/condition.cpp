/*
 * $Id$
 *
 * Copyright(C) 1998-2008 Satoshi Nakamura
 *
 */

#pragma warning(disable:4786)

#include <qsutil.h>

#include "condition.h"
#include "modelresource.h"
#include "../main/main.h"

using namespace qm;
using namespace qs;


/****************************************************************************
 *
 * Condition
 *
 */

qm::Condition::Condition(const WCHAR* pwszName) :
	pwszName_(pwszName)
{
}

qm::Condition::~Condition()
{
}

const WCHAR* qm::Condition::getName() const
{
	return pwszName_;
}


/****************************************************************************
 *
 * NoArgumentCondition
 *
 */

qm::NoArgumentCondition::NoArgumentCondition(const WCHAR* pwszName,
											 UINT nDescriptionId,
											 const WCHAR* pwszMacro) :
	Condition(pwszName),
	nDescriptionId_(nDescriptionId),
	pwszMacro_(pwszMacro)
{
}

qm::NoArgumentCondition::~NoArgumentCondition()
{
}

size_t qm::NoArgumentCondition::getArgumentCount() const
{
	return 0;
}

wstring_ptr qm::NoArgumentCondition::getArgumentName(size_t n) const
{
	assert(false);
	return 0;
}

Condition::Type qm::NoArgumentCondition::getArgumentType(size_t n) const
{
	assert(false);
	return TYPE_TEXT;
}

wstring_ptr qm::NoArgumentCondition::getArgumentValue(size_t n) const
{
	assert(false);
	return 0;
}

void qm::NoArgumentCondition::setArgumentValue(size_t n,
											   const WCHAR* pwszValue)
{
	assert(false);
}

wstring_ptr qm::NoArgumentCondition::getDescription(bool bValue) const
{
	return loadString(getResourceHandle(), nDescriptionId_);
}

wstring_ptr qm::NoArgumentCondition::getMacro() const
{
	return allocWString(pwszMacro_);
}

std::auto_ptr<Condition> qm::NoArgumentCondition::clone() const
{
	return std::auto_ptr<Condition>(new NoArgumentCondition(
		getName(), nDescriptionId_, pwszMacro_));
}


/****************************************************************************
 *
 * FieldCondition
 *
 */

qm::FieldCondition::FieldCondition(const WCHAR* pwszName,
								   UINT nFieldId,
								   UINT nValueId,
								   UINT nDescriptionId,
								   const WCHAR* pwszMacro,
								   WCHAR cValueQuote,
								   const WCHAR* pwszValueEscape) :
	Condition(pwszName),
	nFieldId_(nFieldId),
	nValueId_(nValueId),
	nDescriptionId_(nDescriptionId),
	pwszMacro_(pwszMacro),
	cValueQuote_(cValueQuote),
	pwszValueEscape_(pwszValueEscape)
{
}

qm::FieldCondition::FieldCondition(const WCHAR* pwszName,
								   UINT nFieldId,
								   UINT nValueId,
								   UINT nDescriptionId,
								   const WCHAR* pwszMacro,
								   WCHAR cValueQuote,
								   const WCHAR* pwszValueEscape,
								   const WCHAR* pwszField,
								   const WCHAR* pwszValue) :
	Condition(pwszName),
	nFieldId_(nFieldId),
	nValueId_(nValueId),
	nDescriptionId_(nDescriptionId),
	pwszMacro_(pwszMacro),
	cValueQuote_(cValueQuote),
	pwszValueEscape_(pwszValueEscape)
{
	if (pwszField)
		wstrField_ = allocWString(pwszField);
	if (pwszValue)
		wstrValue_ = allocWString(pwszValue);
}

qm::FieldCondition::~FieldCondition()
{
}

size_t qm::FieldCondition::getArgumentCount() const
{
	return 2;
}

wstring_ptr qm::FieldCondition::getArgumentName(size_t n) const
{
	UINT nId = 0;
	switch (n) {
	case 0:
		nId = nFieldId_;
		break;
	case 1:
		nId = nValueId_;
		break;
	default:
		assert(false);
		break;
	}
	return loadString(getResourceHandle(), nId);
}

Condition::Type qm::FieldCondition::getArgumentType(size_t n) const
{
	switch (n) {
	case 0:
		return TYPE_FIELD;
	case 1:
		return TYPE_TEXT;
	default:
		assert(false);
		return TYPE_TEXT;
	}
}

wstring_ptr qm::FieldCondition::getArgumentValue(size_t n) const
{
	switch (n) {
	case 0:
		return wstrField_.get() ? allocWString(wstrField_.get()) : 0;
	case 1:
		return wstrValue_.get() ? allocWString(wstrValue_.get()) : 0;
	default:
		assert(false);
		return 0;
	}
}

void qm::FieldCondition::setArgumentValue(size_t n,
										  const WCHAR* pwszValue)
{
	switch (n) {
	case 0:
		if (pwszValue)
			wstrField_ = allocWString(pwszValue);
		else
			wstrField_.reset(0);
		break;
	case 1:
		if (pwszValue)
			wstrValue_ = allocWString(pwszValue);
		else
			wstrValue_.reset(0);
		break;
	default:
		assert(false);
		break;
	}
}

wstring_ptr qm::FieldCondition::getDescription(bool bValue) const
{
	wstring_ptr wstrTemplate = loadString(getResourceHandle(), nDescriptionId_);
	wstring_ptr wstrArg0(bValue ? getArgumentValue(0) : getArgumentName(0));
	wstring_ptr wstrArg1(bValue ? getArgumentValue(1) : getArgumentName(1));
	return MessageFormat::format(wstrTemplate.get(), wstrArg0.get(), wstrArg1.get());
}

wstring_ptr qm::FieldCondition::getMacro() const
{
	assert(wstrField_.get());
	assert(wstrValue_.get());
	
	wstring_ptr wstrValue(escape(wstrValue_.get(), cValueQuote_, pwszValueEscape_));
	ConcatW c[] = {
		{ L"@",				1	},
		{ pwszMacro_,		-1	},
		{ L"(",				1	},
		{ wstrField_.get(),	-1	},
		{ L", ",			2	},
		{ &cValueQuote_,	1	},
		{ wstrValue.get(),	-1	},
		{ &cValueQuote_,	1	},
		{ L")",				1	}
	};
	return concat(c, countof(c));
}

std::auto_ptr<Condition> qm::FieldCondition::clone() const
{
	return std::auto_ptr<Condition>(new FieldCondition(getName(),
		nFieldId_, nValueId_, nDescriptionId_, pwszMacro_, cValueQuote_,
		pwszValueEscape_, wstrField_.get(), wstrValue_.get()));
}

wstring_ptr qm::FieldCondition::escape(const WCHAR* pwsz,
									   WCHAR cQuote,
									   const WCHAR* pwszEscape)
{
	StringBuffer<WSTRING> buf;
	while (*pwsz) {
		if (*pwsz == cQuote || (pwszEscape && wcschr(pwszEscape, *pwsz)))
			buf.append(L'\\');
		buf.append(*pwsz);
		++pwsz;
	}
	return buf.getString();
}


/****************************************************************************
 *
 * SizeCondition
 *
 */

qm::SizeCondition::SizeCondition(const WCHAR* pwszName,
								 UINT nValueId,
								 UINT nDescriptionId,
								 const WCHAR* pwszMacro) :
	Condition(pwszName),
	nValueId_(nValueId),
	nDescriptionId_(nDescriptionId),
	pwszMacro_(pwszMacro),
	nSize_(-1)
{
}

qm::SizeCondition::SizeCondition(const WCHAR* pwszName,
								 UINT nValueId,
								 UINT nDescriptionId,
								 const WCHAR* pwszMacro,
								 unsigned int nSize) :
	Condition(pwszName),
	nValueId_(nValueId),
	nDescriptionId_(nDescriptionId),
	pwszMacro_(pwszMacro),
	nSize_(nSize)
{
}

qm::SizeCondition::~SizeCondition()
{
}

size_t qm::SizeCondition::getArgumentCount() const
{
	return 1;
}

wstring_ptr qm::SizeCondition::getArgumentName(size_t n) const
{
	return loadString(getResourceHandle(), nValueId_);
}

Condition::Type qm::SizeCondition::getArgumentType(size_t n) const
{
	return TYPE_NUMBER;
}

wstring_ptr qm::SizeCondition::getArgumentValue(size_t n) const
{
	if (nSize_ == -1)
		return 0;
	
	WCHAR wszSize[32];
	_snwprintf(wszSize, countof(wszSize), L"%u", nSize_);
	return allocWString(wszSize);
}

void qm::SizeCondition::setArgumentValue(size_t n,
										 const WCHAR* pwszValue)
{
	WCHAR* pEnd = 0;
	nSize_ = wcstol(pwszValue, &pEnd, 10);
}

wstring_ptr qm::SizeCondition::getDescription(bool bValue) const
{
	wstring_ptr wstrTemplate = loadString(getResourceHandle(), nDescriptionId_);
	wstring_ptr wstrArg0(bValue ? getArgumentValue(0) : getArgumentName(0));
	return MessageFormat::format(wstrTemplate.get(), wstrArg0.get());
}

wstring_ptr qm::SizeCondition::getMacro() const
{
	WCHAR wszSize[32];
	_snwprintf(wszSize, countof(wszSize), L"%u", nSize_);
	ConcatW c[] = {
		{ L"@",				1	},
		{ pwszMacro_,		-1	},
		{ L"(@Size(), ",	10	},
		{ wszSize,			-1	},
		{ L")",				1	}
	};
	return concat(c, countof(c));
}

std::auto_ptr<Condition> qm::SizeCondition::clone() const
{
	return std::auto_ptr<Condition>(new SizeCondition(getName(),
		nValueId_, nDescriptionId_, pwszMacro_, nSize_));
}


/****************************************************************************
 *
 * PassedCondition
 *
 */

qm::PassedCondition::PassedCondition(const WCHAR* pwszName,
									 UINT nValueId,
									 UINT nDescriptionId,
									 bool bNot) :
	Condition(pwszName),
	nValueId_(nValueId),
	nDescriptionId_(nDescriptionId),
	bNot_(bNot),
	nDays_(-1)
{
}

qm::PassedCondition::PassedCondition(const WCHAR* pwszName,
									 UINT nValueId,
									 UINT nDescriptionId,
									 bool bNot,
									 unsigned int nDays) :
	Condition(pwszName),
	nValueId_(nValueId),
	nDescriptionId_(nDescriptionId),
	bNot_(bNot),
	nDays_(nDays)
{
}

qm::PassedCondition::~PassedCondition()
{
}

size_t qm::PassedCondition::getArgumentCount() const
{
	return 1;
}

wstring_ptr qm::PassedCondition::getArgumentName(size_t n) const
{
	return loadString(getResourceHandle(), nValueId_);
}

Condition::Type qm::PassedCondition::getArgumentType(size_t n) const
{
	return TYPE_NUMBER;
}

wstring_ptr qm::PassedCondition::getArgumentValue(size_t n) const
{
	if (nDays_ == -1)
		return 0;
	
	WCHAR wszDays[32];
	_snwprintf(wszDays, countof(wszDays), L"%u", nDays_);
	return allocWString(wszDays);
}

void qm::PassedCondition::setArgumentValue(size_t n,
										   const WCHAR* pwszValue)
{
	WCHAR* pEnd = 0;
	nDays_ = wcstol(pwszValue, &pEnd, 10);
}

wstring_ptr qm::PassedCondition::getDescription(bool bValue) const
{
	wstring_ptr wstrTemplate = loadString(getResourceHandle(), nDescriptionId_);
	wstring_ptr wstrArg0(bValue ? getArgumentValue(0) : getArgumentName(0));
	return MessageFormat::format(wstrTemplate.get(), wstrArg0.get());
}

wstring_ptr qm::PassedCondition::getMacro() const
{
	WCHAR wszDays[32];
	_snwprintf(wszDays, countof(wszDays), L"%u", nDays_);
	ConcatW c[] = {
		{ L"@Not(",		bNot_ ? 5 : 0	},
		{ L"@Passed(",	8				},
		{ wszDays,		-1				},
		{ L"))",		bNot_ ? 2 : 1	}
	};
	return concat(c, countof(c));
}

std::auto_ptr<Condition> qm::PassedCondition::clone() const
{
	return std::auto_ptr<Condition>(new PassedCondition(
		getName(), nValueId_, nDescriptionId_, bNot_, nDays_));
}


/****************************************************************************
 *
 * ConditionList
 *
 */

qm::ConditionList::ConditionList() :
	type_(TYPE_AND)
{
}

qm::ConditionList::ConditionList(const List& listCondition,
								 Type type) :
	type_(type)
{
	list_.resize(listCondition.size());
	for (List::size_type n = 0; n < listCondition.size(); ++n)
		list_[n] = listCondition[n]->clone().release();
}

qm::ConditionList::~ConditionList()
{
	std::for_each(list_.begin(), list_.end(),
		boost::checked_deleter<Condition>());
}

const ConditionList::List& qm::ConditionList::getConditions() const
{
	return list_;
}

ConditionList::Type qm::ConditionList::getType() const
{
	return type_;
}

wstring_ptr qm::ConditionList::getMacro() const
{
	if (list_.empty()) {
		return allocWString(L"");
	}
	else if (list_.size() == 1) {
		return list_.front()->getMacro();
	}
	else {
		StringBuffer<WSTRING> buf;
		buf.append(type_ == TYPE_AND ? L"@And(" : L"@Or(");
		for (List::const_iterator it = list_.begin(); it != list_.end(); ++it) {
			wstring_ptr wstr((*it)->getMacro());
			if (it != list_.begin())
				buf.append(L", ");
			buf.append(wstr.get());
		}
		buf.append(L")");
		return buf.getString();
	}
}

wstring_ptr qm::ConditionList::getDescription(bool bValue) const
{
	if (list_.empty()) {
		return allocWString(L"");
	}
	else if (list_.size() == 1) {
		return list_.front()->getDescription(bValue);
	}
	else {
		wstring_ptr wstrSep(loadString(getResourceHandle(),
			type_ == TYPE_AND ? IDS_CONDITIONTYPE_AND : IDS_CONDITIONTYPE_OR));
		
		StringBuffer<WSTRING> buf;
		for (List::const_iterator it = list_.begin(); it != list_.end(); ++it) {
			wstring_ptr wstr((*it)->getDescription(bValue));
			if (it != list_.begin())
				buf.append(wstrSep.get());
			buf.append(wstr.get());
		}
		return buf.getString();
	}
}

void qm::ConditionList::setType(Type type)
{
	type_ = type;
}

void qm::ConditionList::add(std::auto_ptr<Condition> pCondition)
{
	list_.push_back(pCondition.get());
	pCondition.release();
}


/****************************************************************************
 *
 * ConditionFactory
 *
 */

const ConditionFactory qm::ConditionFactory::factory__;

qm::ConditionFactory::ConditionFactory()
{
	list_.push_back(new FieldCondition(
		L"Contain",
		IDS_CONDITION_CONTAIN_FIELD,
		IDS_CONDITION_CONTAIN_VALUE,
		IDS_CONDITION_CONTAIN_DESCRIPTION,
		L"Contain",
		L'\"',
		L"\\"));
	list_.push_back(new FieldCondition(
		L"BeginWith",
		IDS_CONDITION_BEGINWITH_FIELD,
		IDS_CONDITION_BEGINWITH_VALUE,
		IDS_CONDITION_BEGINWITH_DESCRIPTION,
		L"BeginWith",
		L'\"',
		L"\\"));
	list_.push_back(new FieldCondition(
		L"Equal",
		IDS_CONDITION_EQUAL_FIELD,
		IDS_CONDITION_EQUAL_VALUE,
		IDS_CONDITION_EQUAL_DESCRIPTION,
		L"Equal",
		L'\"',
		L"\\"));
	list_.push_back(new FieldCondition(
		L"RegexMatch",
		IDS_CONDITION_MATCH_FIELD,
		IDS_CONDITION_MATCH_VALUE,
		IDS_CONDITION_MATCH_DESCRIPTION,
		L"RegexMatch",
		L'/',
		0));
	list_.push_back(new NoArgumentCondition(
		L"Seen",
		IDS_CONDITION_SEEN_DESCRIPTION,
		L"@Seen()"));
	list_.push_back(new NoArgumentCondition(
		L"Unseen",
		IDS_CONDITION_UNSEEN_DESCRIPTION,
		L"@Not(@Seen())"));
	list_.push_back(new NoArgumentCondition(
		L"Marked",
		IDS_CONDITION_MARKED_DESCRIPTION,
		L"@Marked()"));
	list_.push_back(new NoArgumentCondition(
		L"Unmarked",
		IDS_CONDITION_UNMARKED_DESCRIPTION,
		L"@Not(@Marked())"));
	list_.push_back(new NoArgumentCondition(
		L"Deleted",
		IDS_CONDITION_DELETED_DESCRIPTION,
		L"@Deleted()"));
	list_.push_back(new NoArgumentCondition(
		L"Undeleted",
		IDS_CONDITION_UNDELETED_DESCRIPTION,
		L"@Not(@Deleted())"));
	list_.push_back(new SizeCondition(
		L"Less",
		IDS_CONDITION_SMALLER_VALUE,
		IDS_CONDITION_SMALLER_DESCRIPTION,
		L"Less"));
	list_.push_back(new SizeCondition(
		L"Greater",
		IDS_CONDITION_LARGER_VALUE,
		IDS_CONDITION_LARGER_DESCRIPTION,
		L"Greater"));
	list_.push_back(new PassedCondition(
		L"Older",
		IDS_CONDITION_OLDER_VALUE,
		IDS_CONDITION_OLDER_DESCRIPTION,
		false));
	list_.push_back(new PassedCondition(
		L"Newer",
		IDS_CONDITION_NEWER_VALUE,
		IDS_CONDITION_NEWER_DESCRIPTION,
		true));
	list_.push_back(new NoArgumentCondition(
		L"New",
		IDS_CONDITION_NEW_DESCRIPTION,
		L"@New()"));
	list_.push_back(new NoArgumentCondition(
		L"Old",
		IDS_CONDITION_OLD_DESCRIPTION,
		L"@Not(@New())"));
	list_.push_back(new NoArgumentCondition(
		L"Junk",
		IDS_CONDITION_JUNK_DESCRIPTION,
		L"@Junk()"));
	list_.push_back(new NoArgumentCondition(
		L"Multipart",
		IDS_CONDITION_MULTIPART_DESCRIPTION,
		L"@Multipart()"));
	list_.push_back(new NoArgumentCondition(
		L"True",
		IDS_CONDITION_TRUE_DESCRIPTION,
		L"@True()"));
}

qm::ConditionFactory::~ConditionFactory()
{
	for (List::iterator it = list_.begin(); it != list_.end(); ++it)
		delete *it;
}

const ConditionFactory::List& qm::ConditionFactory::getConditions() const
{
	return list_;
}

std::auto_ptr<ConditionList> qm::ConditionFactory::parse(const Macro* pMacro) const
{
	assert(pMacro);
	
	const MacroExpr* pExpr = pMacro->getExpr();
	GetFunctionNameMacroExprVisitor visitor;
	pExpr->visit(&visitor);
	
	std::auto_ptr<ConditionList> pConditionList(new ConditionList());
	
	const MacroFunction* pFunction = 0;
	const WCHAR* pwszName = visitor.getName();
	if (pwszName && wcscmp(pwszName, L"And") == 0) {
		pConditionList->setType(ConditionList::TYPE_AND);
		pFunction = static_cast<const MacroFunction*>(pExpr);
	}
	else if (pwszName && wcscmp(pwszName, L"Or") == 0) {
		pConditionList->setType(ConditionList::TYPE_OR);
		pFunction = static_cast<const MacroFunction*>(pExpr);
	}
	else {
		std::auto_ptr<Condition> p(parse(pExpr));
		if (!p.get())
			return std::auto_ptr<ConditionList>();
		pConditionList->add(p);
	}
	if (pFunction) {
		for (size_t n = 0; n < pFunction->getArgSize(); ++n) {
			std::auto_ptr<Condition> p(parse(pFunction->getArg(n)));
			if (!p.get())
				return std::auto_ptr<ConditionList>();
			pConditionList->add(p);
		}
	}
	
	return pConditionList;
}

const ConditionFactory& qm::ConditionFactory::getInstance()
{
	return factory__;
}

const Condition* qm::ConditionFactory::getCondition(const WCHAR* pwszName) const
{
	List::const_iterator it = std::find_if(
		list_.begin(), list_.end(),
		boost::bind(string_equal<WCHAR>(),
			boost::bind(&Condition::getName, _1), pwszName));
	return it != list_.end() ? *it : 0;
}

std::auto_ptr<Condition> qm::ConditionFactory::parse(const MacroExpr* pExpr) const
{
	assert(pExpr);
	
	GetFunctionNameMacroExprVisitor visitor;
	pExpr->visit(&visitor);
	const WCHAR* pwszName = visitor.getName();
	if (!pwszName)
		return std::auto_ptr<Condition>();
	
	const MacroFunction* pFunction = static_cast<const MacroFunction*>(pExpr);
	size_t nArgSize = pFunction->getArgSize();
	if (wcscmp(pwszName, L"Deleted") == 0 ||
		wcscmp(pwszName, L"Junk") == 0 ||
		wcscmp(pwszName, L"Marked") == 0 ||
		wcscmp(pwszName, L"Multipart") == 0 ||
		wcscmp(pwszName, L"Seen") == 0 ||
		wcscmp(pwszName, L"True") == 0 ||
		wcscmp(pwszName, L"New") == 0) {
		if (nArgSize != 0)
			return std::auto_ptr<Condition>();
		
		return getCondition(pwszName)->clone();
	}
	else if (wcscmp(pwszName, L"Not") == 0) {
		if (nArgSize != 1)
			return std::auto_ptr<Condition>();
		
		const MacroExpr* pArg = pFunction->getArg(0);
		GetFunctionNameMacroExprVisitor visitor;
		pArg->visit(&visitor);
		const WCHAR* pwszArgName = visitor.getName();
		if (!pwszArgName)
			return std::auto_ptr<Condition>();
		
		if (wcscmp(pwszArgName, L"Passed") == 0)
			return parsePassed(static_cast<const MacroFunction*>(pArg), L"Newer");
		
		if (static_cast<const MacroFunction*>(pArg)->getArgSize() != 0)
			return std::auto_ptr<Condition>();
		if (wcscmp(pwszArgName, L"Marked") == 0)
			return getCondition(L"Unmarked")->clone();
		else if (wcscmp(pwszArgName, L"Seen") == 0)
			return getCondition(L"Unseen")->clone();
		else if (wcscmp(pwszArgName, L"Deleted") == 0)
			return getCondition(L"Undeleted")->clone();
		else if (wcscmp(pwszArgName, L"New") == 0)
			return getCondition(L"Old")->clone();
		else
			return std::auto_ptr<Condition>();
	}
	else if (wcscmp(pwszName, L"BeginWith") == 0 ||
		wcscmp(pwszName, L"Contain") == 0 ||
		wcscmp(pwszName, L"Equal") == 0 ||
		wcscmp(pwszName, L"RegexMatch") == 0) {
		if (nArgSize != 2)
			return std::auto_ptr<Condition>();
		
		const MacroExpr* pArg0 = pFunction->getArg(0);
		GetTypeMacroExprVisitor visitor0;
		pArg0->visit(&visitor0);
		if (visitor0.getType() != GetTypeMacroExprVisitor::TYPE_FIELD &&
			visitor0.getType() != GetTypeMacroExprVisitor::TYPE_FIELDCACHE)
			return std::auto_ptr<Condition>();
		
		bool bRegexMatch = wcscmp(pwszName, L"RegexMatch") == 0;
		
		const MacroExpr* pArg1 = pFunction->getArg(1);
		GetTypeMacroExprVisitor visitor1;
		pArg1->visit(&visitor1);
		if (visitor1.getType() != GetTypeMacroExprVisitor::TYPE_LITERAL &&
			(!bRegexMatch || visitor1.getType() != GetTypeMacroExprVisitor::TYPE_REGEX))
			return std::auto_ptr<Condition>();
		
		std::auto_ptr<Condition> pCondition(getCondition(pwszName)->clone());
		pCondition->setArgumentValue(0, pArg0->getString().get());
		if (visitor1.getType() == GetTypeMacroExprVisitor::TYPE_REGEX)
			pCondition->setArgumentValue(1, static_cast<const MacroRegex*>(pArg1)->getPattern());
		else
			pCondition->setArgumentValue(1, static_cast<const MacroLiteral*>(pArg1)->getValue());
		return pCondition;
	}
	else if (wcscmp(pwszName, L"Less") == 0 ||
		wcscmp(pwszName, L"Greater") == 0) {
		if (nArgSize != 2)
			return std::auto_ptr<Condition>();
		
		const MacroExpr* pArg0 = pFunction->getArg(0);
		GetFunctionNameMacroExprVisitor visitor0;
		pArg0->visit(&visitor0);
		const WCHAR* pwszArg0Name = visitor0.getName();
		if (!pwszArg0Name || wcscmp(pwszArg0Name, L"Size") != 0)
			return std::auto_ptr<Condition>();
		
		const MacroExpr* pArg1 = pFunction->getArg(1);
		GetTypeMacroExprVisitor visitor1;
		pArg1->visit(&visitor1);
		if (visitor1.getType() != GetTypeMacroExprVisitor::TYPE_NUMBER)
			return std::auto_ptr<Condition>();
		
		WCHAR wszSize[32];
		_snwprintf(wszSize, countof(wszSize), L"%u",
			static_cast<const MacroNumber*>(pArg1)->getValue());
		
		std::auto_ptr<Condition> pCondition(getCondition(pwszName)->clone());
		pCondition->setArgumentValue(0, wszSize);
		return pCondition;
	}
	else if (wcscmp(pwszName, L"Passed") == 0) {
		return parsePassed(pFunction, L"Older");
	}
	else {
		return std::auto_ptr<Condition>();
	}
}

std::auto_ptr<Condition> qm::ConditionFactory::parsePassed(const MacroFunction* pFunction,
														   const WCHAR* pwszName) const
{
	size_t nArgSize = pFunction->getArgSize();
	if (nArgSize != 1)
		return std::auto_ptr<Condition>();
	
	const MacroExpr* pArg = pFunction->getArg(0);
	GetTypeMacroExprVisitor visitor;
	pArg->visit(&visitor);
	if (visitor.getType() != GetTypeMacroExprVisitor::TYPE_NUMBER)
		return std::auto_ptr<Condition>();
	
	WCHAR wszDays[32];
	_snwprintf(wszDays, countof(wszDays), L"%u",
		static_cast<const MacroNumber*>(pArg)->getValue());
	
	std::auto_ptr<Condition> pCondition(getCondition(pwszName)->clone());
	pCondition->setArgumentValue(0, wszDays);
	return pCondition;
}


/****************************************************************************
 *
 * ConditionFactory::GetTypeMacroExprVisitor
 *
 */

qm::ConditionFactory::GetTypeMacroExprVisitor::GetTypeMacroExprVisitor() :
	type_(TYPE_FIELD)
{
}

qm::ConditionFactory::GetTypeMacroExprVisitor::~GetTypeMacroExprVisitor()
{
}

ConditionFactory::GetTypeMacroExprVisitor::Type qm::ConditionFactory::GetTypeMacroExprVisitor::getType() const
{
	return type_;
}

void qm::ConditionFactory::GetTypeMacroExprVisitor::visitField(const MacroField& field)
{
	type_ = TYPE_FIELD;
}

void qm::ConditionFactory::GetTypeMacroExprVisitor::visitFieldCache(const MacroFieldCache& fieldCache)
{
	type_ = TYPE_FIELDCACHE;
}

void qm::ConditionFactory::GetTypeMacroExprVisitor::visitLiteral(const MacroLiteral& literal)
{
	type_ = TYPE_LITERAL;
}

void qm::ConditionFactory::GetTypeMacroExprVisitor::visitNumber(const MacroNumber& number)
{
	type_ = TYPE_NUMBER;
}

void qm::ConditionFactory::GetTypeMacroExprVisitor::visitBoolean(const MacroBoolean& boolean)
{
	type_ = TYPE_BOOLEAN;
}

void qm::ConditionFactory::GetTypeMacroExprVisitor::visitRegex(const MacroRegex& regex)
{
	type_ = TYPE_REGEX;
}

void qm::ConditionFactory::GetTypeMacroExprVisitor::visitVariable(const MacroVariable& variable)
{
	type_ = TYPE_VARIABLE;
}

void qm::ConditionFactory::GetTypeMacroExprVisitor::visitConstant(const MacroConstant& constant)
{
	type_ = TYPE_CONSTANT;
}

void qm::ConditionFactory::GetTypeMacroExprVisitor::visitFunction(const MacroFunction& function)
{
	type_ = TYPE_FUNCTION;
}


/****************************************************************************
 *
 * ConditionFactory::GetFunctionNameMacroExprVisitor
 *
 */

qm::ConditionFactory::GetFunctionNameMacroExprVisitor::GetFunctionNameMacroExprVisitor() :
	pwszName_(0)
{
}

qm::ConditionFactory::GetFunctionNameMacroExprVisitor::~GetFunctionNameMacroExprVisitor()
{
}

const WCHAR* qm::ConditionFactory::GetFunctionNameMacroExprVisitor::getName() const
{
	return pwszName_;
}

void qm::ConditionFactory::GetFunctionNameMacroExprVisitor::visitField(const MacroField& field)
{
}

void qm::ConditionFactory::GetFunctionNameMacroExprVisitor::visitFieldCache(const MacroFieldCache& fieldCache)
{
}

void qm::ConditionFactory::GetFunctionNameMacroExprVisitor::visitLiteral(const MacroLiteral& literal)
{
}

void qm::ConditionFactory::GetFunctionNameMacroExprVisitor::visitNumber(const MacroNumber& number)
{
}

void qm::ConditionFactory::GetFunctionNameMacroExprVisitor::visitBoolean(const MacroBoolean& boolean)
{
}

void qm::ConditionFactory::GetFunctionNameMacroExprVisitor::visitRegex(const MacroRegex& regex)
{
}

void qm::ConditionFactory::GetFunctionNameMacroExprVisitor::visitVariable(const MacroVariable& variable)
{
}

void qm::ConditionFactory::GetFunctionNameMacroExprVisitor::visitConstant(const MacroConstant& constant)
{
}

void qm::ConditionFactory::GetFunctionNameMacroExprVisitor::visitFunction(const MacroFunction& function)
{
	pwszName_ = function.getFunctionName();
}
