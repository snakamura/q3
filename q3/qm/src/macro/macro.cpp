/*
 * $Id$
 *
 * Copyright(C) 1998-2003 Satoshi Nakamura
 * All rights reserved.
 *
 */

#include <qmaccount.h>
#include <qmapplication.h>
#include <qmmacro.h>
#include <qmmessage.h>
#include <qmmessageholder.h>

#include <qsstl.h>
#include <qsnew.h>
#include <qsconv.h>

#include <algorithm>
#include <memory>
#include <vector>

#include "macro.h"

#pragma warning(disable:4786)

using namespace qm;
using namespace qs;


/****************************************************************************
 *
 * MacroTokenizer
 *
 */

qm::MacroTokenizer::MacroTokenizer(const WCHAR* pwszMacro, QSTATUS* pstatus) :
	pwszMacro_(pwszMacro),
	p_(pwszMacro_),
	pLast_(pwszMacro)
{
	assert(pstatus);
	*pstatus = QSTATUS_SUCCESS;
}

qm::MacroTokenizer::~MacroTokenizer()
{
}

QSTATUS qm::MacroTokenizer::getToken(Token* pToken, WSTRING* pwstrToken)
{
	assert(pToken);
	assert(pwstrToken);
	
	DECLARE_QSTATUS();
	
	*pwstrToken = 0;
	
	while (*p_ == L' ' || *p_ == L'\t' || *p_ == L'\n')
		++p_;
	
	if (!*p_) {
		*pToken = TOKEN_END;
		return QSTATUS_SUCCESS;
	}
	else if (*p_ == L'#') {
		while (*p_ == L'#') {
			while (*p_ && *p_ != L'\n')
				++p_;
			while (*p_ == L' ' || *p_ == L'\t' || *p_ == L'\n')
				++p_;
		}
		if (!*p_) {
			*pToken = TOKEN_END;
			return QSTATUS_SUCCESS;
		}
	}
	
	pLast_ = p_;
	
	WCHAR c = *p_;
	if (c == L'\"' || c == L'\'') {
		StringBuffer<WSTRING> buf(&status);
		CHECK_QSTATUS();
		for (++p_; *p_ && *p_ != c; ++p_) {
			if (*p_ == L'\\') {
				++p_;
				switch (*p_) {
				case L'n':
					status = buf.append(L'\n');
					CHECK_QSTATUS();
					break;
				case L't':
					status = buf.append(L'\t');
					CHECK_QSTATUS();
					break;
				case L'\\':
				case L'\'':
				case L'\"':
					status = buf.append(*p_);
					CHECK_QSTATUS();
					break;
				case L'\0':
					*pToken = TOKEN_ERROR;
					return QSTATUS_SUCCESS;
					break;
				default:
					status = buf.append(*p_);
					CHECK_QSTATUS();
					break;
				}
			}
			else {
				status = buf.append(*p_);
				CHECK_QSTATUS();
			}
		}
		if (!*p_) {
			*pToken = TOKEN_ERROR;
			return QSTATUS_SUCCESS;
		}
		*pwstrToken = buf.getString();
		++p_;
		*pToken = TOKEN_LITERAL;
	}
	else if (c == L'/') {
		StringBuffer<WSTRING> buf(&status);
		CHECK_QSTATUS();
		for (++p_; *p_ && *p_ != L'/'; ++p_) {
			if (*p_ == L'\\') {
				if (*(p_ + 1) != L'/') {
					status = buf.append(*p_);
					CHECK_QSTATUS();
				}
				++p_;
			}
			status = buf.append(*p_);
			CHECK_QSTATUS();
		}
		if (!*p_) {
			*pToken = TOKEN_ERROR;
			return QSTATUS_SUCCESS;
		}
		*pwstrToken = buf.getString();
		++p_;
		*pToken = TOKEN_REGEX;
	}
	else if (c == L'@') {
		++p_;
		*pToken = TOKEN_AT;
	}
	else if (c == L'$') {
		++p_;
		*pToken = TOKEN_DOLLAR;
	}
	else if (c == L'%') {
		++p_;
		*pToken = TOKEN_PERCENT;
	}
	else if (c == L'(') {
		++p_;
		*pToken = TOKEN_LEFTPARENTHESIS;
	}
	else if (c == L')') {
		++p_;
		*pToken = TOKEN_RIGHTPARENTHESIS;
	}
	else if (c == L',') {
		++p_;
		*pToken = TOKEN_COMMA;
	}
	else {
		const WCHAR* pwszSep = L" \t\n,()#";
		const WCHAR* pBegin = p_;
		while (*p_ && !wcschr(pwszSep, *p_))
			++p_;
		*pwstrToken = allocWString(pBegin, p_ - pBegin);
		if (!*pwstrToken)
			return QSTATUS_OUTOFMEMORY;
		*pToken = TOKEN_TEXT;
	}
	
	return QSTATUS_SUCCESS;
}

const WCHAR* qm::MacroTokenizer::getLastPosition() const
{
	return pLast_;
}


/****************************************************************************
 *
 * MacroGlobalContext
 *
 */

qm::MacroGlobalContext::MacroGlobalContext(
	const Init& init, QSTATUS* pstatus) :
	pDocument_(init.pDocument_),
	hwnd_(init.hwnd_),
	pProfile_(init.pProfile_),
	bGetMessageAsPossible_(init.bGetMessageAsPossible_),
	pErrorHandler_(init.pErrorHandler_),
	pVariable_(0),
	pGlobalVariable_(init.pGlobalVariable_),
	pFunction_(0),
	pArgument_(0)
{
	assert(pDocument_);
	assert(hwnd_);
	assert(pstatus);
	
	*pstatus = QSTATUS_SUCCESS;
	
	DECLARE_QSTATUS();
	
	status = newQsObject(&pVariable_);
	CHECK_QSTATUS_SET(pstatus);
	status = newQsObject(&pFunction_);
	CHECK_QSTATUS_SET(pstatus);
	status = newQsObject(&pArgument_);
	CHECK_QSTATUS_SET(pstatus);
}

qm::MacroGlobalContext::~MacroGlobalContext()
{
	delete pVariable_;
	delete pFunction_;
	delete pArgument_;
}

Document* qm::MacroGlobalContext::getDocument() const
{
	return pDocument_;
}

HWND qm::MacroGlobalContext::getWindow() const
{
	return hwnd_;
}

Profile* qm::MacroGlobalContext::getProfile() const
{
	return pProfile_;
}

bool qm::MacroGlobalContext::isGetMessageAsPossible() const
{
	return bGetMessageAsPossible_;
}

MacroErrorHandler* qm::MacroGlobalContext::getErrorHandler() const
{
	return pErrorHandler_;
}

QSTATUS qm::MacroGlobalContext::getVariable(
	const WCHAR* pwszName, MacroValue** ppValue) const
{
	assert(pwszName);
	assert(ppValue);
	
	DECLARE_QSTATUS();
	
	*ppValue = 0;
	
	status = pVariable_->getVariable(pwszName, ppValue);
	CHECK_QSTATUS();
	if (!*ppValue && pGlobalVariable_) {
		status = pGlobalVariable_->getVariable(pwszName, ppValue);
		CHECK_QSTATUS();
	}
	
	return QSTATUS_SUCCESS;
}

QSTATUS qm::MacroGlobalContext::setVariable(
	const WCHAR* pwszName, MacroValue* pValue, bool bGlobal)
{
	assert(pwszName);
	assert(pValue);
	
	DECLARE_QSTATUS();
	
	if (bGlobal && pGlobalVariable_) {
		status = pGlobalVariable_->setVariable(pwszName, pValue);
		CHECK_QSTATUS();
	}
	else {
		status = pVariable_->setVariable(pwszName, pValue);
		CHECK_QSTATUS();
	}
	
	return QSTATUS_SUCCESS;
}

QSTATUS qm::MacroGlobalContext::getFunction(
	const WCHAR* pwszName, const MacroExpr** ppExpr) const
{
	return pFunction_->getFunction(pwszName, ppExpr);
}

QSTATUS qm::MacroGlobalContext::setFunction(
	const WCHAR* pwszName, const MacroExpr* pExpr, bool* pbSet)
{
	return pFunction_->setFunction(pwszName, pExpr, pbSet);
}

QSTATUS qm::MacroGlobalContext::pushArgumentContext()
{
	return pArgument_->pushContext();
}

void qm::MacroGlobalContext::popArgumentContext()
{
	pArgument_->popContext();
}

QSTATUS qm::MacroGlobalContext::addArgument(MacroValue* pValue)
{
	return pArgument_->addArgument(pValue);
}

QSTATUS qm::MacroGlobalContext::getArgument(
	unsigned int n, MacroValue** ppValue) const
{
	return pArgument_->getArgument(n, ppValue);
}


/****************************************************************************
 *
 * MacroFunctionHolder
 *
 */

qm::MacroFunctionHolder::MacroFunctionHolder(QSTATUS* pstatus)
{
	assert(pstatus);
	*pstatus = QSTATUS_SUCCESS;
}

qm::MacroFunctionHolder::~MacroFunctionHolder()
{
	std::for_each(mapFunction_.begin(), mapFunction_.end(),
		unary_compose_f_gx(
			string_free<WSTRING>(),
			std::select1st<FunctionMap::value_type>()));
}

QSTATUS qm::MacroFunctionHolder::getFunction(
	const WCHAR* pwszName, const MacroExpr** ppExpr) const
{
	assert(pwszName);
	assert(ppExpr);
	
	DECLARE_QSTATUS();
	
	*ppExpr = 0;
	
	FunctionMap::const_iterator it = std::find_if(
		mapFunction_.begin(), mapFunction_.end(),
		std::bind2nd(
			binary_compose_f_gx_hy(
				string_equal_i<WCHAR>(),
				std::select1st<FunctionMap::value_type>(),
				std::identity<const WCHAR*>()),
			pwszName));
	if (it != mapFunction_.end())
		*ppExpr = (*it).second;
	
	return QSTATUS_SUCCESS;
}

QSTATUS qm::MacroFunctionHolder::setFunction(
	const WCHAR* pwszName, const MacroExpr* pExpr, bool* pbSet)
{
	assert(pwszName);
	assert(pExpr);
	assert(pbSet);
	
	DECLARE_QSTATUS();
	
	*pbSet = false;
	
	FunctionMap::const_iterator it = std::find_if(
		mapFunction_.begin(), mapFunction_.end(),
		std::bind2nd(
			binary_compose_f_gx_hy(
				string_equal_i<WCHAR>(),
				std::select1st<FunctionMap::value_type>(),
				std::identity<const WCHAR*>()),
			pwszName));
	if (it == mapFunction_.end()) {
		string_ptr<WSTRING> wstrName(allocWString(pwszName));
		if (!wstrName.get())
			return QSTATUS_OUTOFMEMORY;
		status = STLWrapper<FunctionMap>(mapFunction_).push_back(
			std::make_pair(wstrName.get(), pExpr));
		CHECK_QSTATUS();
		wstrName.release();
		*pbSet = true;
	}
	
	return QSTATUS_SUCCESS;
}


/****************************************************************************
 *
 * MacroArgumentHolder
 *
 */

qm::MacroArgumentHolder::MacroArgumentHolder(QSTATUS* pstatus)
{
	assert(pstatus);
	*pstatus = QSTATUS_SUCCESS;
}

qm::MacroArgumentHolder::~MacroArgumentHolder()
{
}

QSTATUS qm::MacroArgumentHolder::pushContext()
{
	return STLWrapper<ArgumentList>(
		listArgument_).resize(listArgument_.size() + 1);
}

void qm::MacroArgumentHolder::popContext()
{
	assert(!listArgument_.empty());
	
	std::vector<MacroValue*>& v = listArgument_.back();
	std::vector<MacroValue*>::iterator it = v.begin();
	while (it != v.end()) {
		MacroValuePtr pValue(*it);
		++it;
	}
	listArgument_.pop_back();
}

QSTATUS qm::MacroArgumentHolder::addArgument(MacroValue* pValue)
{
	assert(!listArgument_.empty());
	return STLWrapper<std::vector<MacroValue*> >(
		listArgument_.back()).push_back(pValue);
}

QSTATUS qm::MacroArgumentHolder::getArgument(unsigned int n, MacroValue** ppValue) const
{
	assert(ppValue);
	assert(!listArgument_.empty());
	
	DECLARE_QSTATUS();
	
	*ppValue = 0;
	
	const std::vector<MacroValue*>& v = listArgument_.back();
	if (n < v.size()) {
		status = v[n]->clone(ppValue);
		CHECK_QSTATUS();
	}
	
	return QSTATUS_SUCCESS;
}


/****************************************************************************
 *
 * MacroExpr
 *
 */

qm::MacroExpr::~MacroExpr()
{
}

void qm::MacroExpr::release()
{
	delete this;
}

QSTATUS qm::MacroExpr::error(const MacroContext& context,
	MacroErrorHandler::Code code) const
{
	DECLARE_QSTATUS();
	
	if (context.getErrorHandler()) {
		string_ptr<WSTRING> wstr;
		status = getString(&wstr);
		CHECK_QSTATUS();
		context.getErrorHandler()->processError(
			code, wstr.get());
	}
	return QSTATUS_FAIL;
}


/****************************************************************************
 *
 * MacroField
 *
 */

qm::MacroField::MacroField(const WCHAR* pwszName, QSTATUS* pstatus) :
	wstrName_(0)
{
	assert(pstatus);
	
	*pstatus = QSTATUS_SUCCESS;
	
	string_ptr<WSTRING> wstrName(allocWString(pwszName));
	if (!wstrName.get()) {
		*pstatus = QSTATUS_OUTOFMEMORY;
		return;
	}
	wstrName_ = wstrName.release();
}

qm::MacroField::~MacroField()
{
	freeWString(wstrName_);
}

QSTATUS qm::MacroField::value(MacroContext* pContext,
	MacroValue** ppValue) const
{
	assert(pContext);
	assert(ppValue);
	
	DECLARE_QSTATUS();
	
	*ppValue = 0;
	
	MessageHolderBase* pmh = pContext->getMessageHolder();
	if (!pmh)
		return error(*pContext, MacroErrorHandler::CODE_NOCONTEXTMESSAGE);
	
	Message* pMessage = 0;
	status = pContext->getMessage(MacroContext::MESSAGETYPE_HEADER,
		wstrName_, &pMessage);
	CHECK_QSTATUS();
	
	string_ptr<STRING> strHeader;
	status = PartUtil(*pMessage).getHeader(wstrName_, &strHeader);
	CHECK_QSTATUS();
	
	return MacroValueFactory::getFactory().newField(wstrName_,
		strHeader.get(), reinterpret_cast<MacroValueField**>(ppValue));
}

QSTATUS qm::MacroField::getString(WSTRING* pwstrExpr) const
{
	assert(pwstrExpr);
	*pwstrExpr = allocWString(wstrName_);
	return *pwstrExpr ? QSTATUS_SUCCESS : QSTATUS_OUTOFMEMORY;
}


/****************************************************************************
 *
 * MacroFieldCache
 *
 */

qm::MacroFieldCache::MacroFieldCache(const WCHAR* pwszName, QSTATUS* pstatus)
{
	assert(pstatus);
	
	*pstatus = QSTATUS_SUCCESS;
	
	const WCHAR* pwszTypes[] = {
		L"id",
		L"date",
		L"from",
		L"to",
		L"fromto",
		L"subject",
		L"size"
	};
	Type types[] = {
		TYPE_ID,
		TYPE_DATE,
		TYPE_FROM,
		TYPE_TO,
		TYPE_FROMTO,
		TYPE_SUBJECT,
		TYPE_SIZE
	};
	
	for (int n = 0; n < countof(pwszTypes); ++n) {
		if (_wcsicmp(pwszName, pwszTypes[n]) == 0) {
			type_ = types[n];
			break;
		}
	}
	if (n == countof(pwszTypes)) {
		*pstatus = QSTATUS_FAIL;
		return;
	}
}

qm::MacroFieldCache::~MacroFieldCache()
{
}

QSTATUS qm::MacroFieldCache::value(MacroContext* pContext,
	MacroValue** ppValue) const
{
	assert(pContext);
	assert(ppValue);
	
	DECLARE_QSTATUS();
	
	MessageHolderBase* pmh = pContext->getMessageHolder();
	if (!pmh)
		return error(*pContext, MacroErrorHandler::CODE_NOCONTEXTMESSAGE);
	
	unsigned int (qm::MessageHolderBase::*pfnGetNumber)() const = 0;
	QSTATUS (qm::MessageHolderBase::*pfnGetString)(WSTRING*) const = 0;
	QSTATUS (qm::MessageHolderBase::*pfnGetTime)(Time*) const = 0;
	
	MacroValueFactory& factory = MacroValueFactory::getFactory();
	switch (type_) {
	case TYPE_ID:
		pfnGetNumber = &qm::MessageHolderBase::getId;
		break;
	case TYPE_DATE:
		pfnGetTime = &qm::MessageHolderBase::getDate;
		break;
	case TYPE_FROM:
		pfnGetString = qm::MessageHolderBase::getFrom;
		break;
	case TYPE_TO:
		pfnGetString = qm::MessageHolderBase::getTo;
		break;
	case TYPE_FROMTO:
		pfnGetString = qm::MessageHolderBase::getFromTo;
		break;
	case TYPE_SUBJECT:
		pfnGetString = qm::MessageHolderBase::getSubject;
		break;
	case TYPE_SIZE:
		pfnGetNumber = &qm::MessageHolderBase::getSize;
		break;
	}
	
	if (pfnGetNumber) {
		status = factory.newNumber((pmh->*pfnGetNumber)(),
			reinterpret_cast<MacroValueNumber**>(ppValue));
		CHECK_QSTATUS();
	}
	else if (pfnGetString) {
		string_ptr<WSTRING> wstr;
		status = (pmh->*pfnGetString)(&wstr);
		CHECK_QSTATUS();
		status = factory.newString(wstr.get(),
			reinterpret_cast<MacroValueString**>(ppValue));
		CHECK_QSTATUS();
	}
	else if (pfnGetTime) {
		Time time;
		status = (pmh->*pfnGetTime)(&time);
		CHECK_QSTATUS();
		status = factory.newTime(time,
			reinterpret_cast<MacroValueTime**>(ppValue));
		CHECK_QSTATUS();
	}
	else {
		assert(false);
	}
	
	return QSTATUS_SUCCESS;
}

QSTATUS qm::MacroFieldCache::getString(WSTRING* pwstrExpr) const
{
	assert(pwstrExpr);
	
	const WCHAR* pwszNames[] = {
		L"ID",
		L"Date",
		L"From",
		L"To",
		L"FromTo",
		L"Subject",
		L"Size"
	};
	
	*pwstrExpr = concat(L"%", pwszNames[type_]);
	return *pwstrExpr ? QSTATUS_SUCCESS : QSTATUS_OUTOFMEMORY;
}


/****************************************************************************
 *
 * MacroLiteral
 *
 */

qm::MacroLiteral::MacroLiteral(const WCHAR* pwszValue, QSTATUS* pstatus) :
	wstrValue_(0)
{
	assert(pstatus);
	
	*pstatus = QSTATUS_SUCCESS;
	
	wstrValue_ = allocWString(pwszValue);
	if (!wstrValue_) {
		*pstatus = QSTATUS_OUTOFMEMORY;
		return;
	}
}

qm::MacroLiteral::~MacroLiteral()
{
	freeWString(wstrValue_);
}

QSTATUS qm::MacroLiteral::value(MacroContext* pContext,
	MacroValue** ppValue) const
{
	assert(pContext);
	assert(ppValue);
	
	return MacroValueFactory::getFactory().newString(wstrValue_,
		reinterpret_cast<MacroValueString**>(ppValue));
}

QSTATUS qm::MacroLiteral::getString(WSTRING* pwstrExpr) const
{
	assert(pwstrExpr);
	
	DECLARE_QSTATUS();
	
	*pwstrExpr = 0;
	
	StringBuffer<WSTRING> buf(&status);
	CHECK_QSTATUS();
	
	status = buf.append(L'\'');
	CHECK_QSTATUS();
	
	for (const WCHAR* p = wstrValue_; *p; ++p) {
		switch (*p) {
		case L'\n':
			status = buf.append(L"\\n");
			CHECK_QSTATUS();
			break;
		case L'\t':
			status = buf.append(L"\\t");
			CHECK_QSTATUS();
			break;
		case L'\\':
			status = buf.append(L"\\\\");
			CHECK_QSTATUS();
			break;
		case L'\"':
			status = buf.append(L"\\\"");
			CHECK_QSTATUS();
			break;
		case L'\'':
			status = buf.append(L"\\\'");
			CHECK_QSTATUS();
			break;
		default:
			status = buf.append(*p);
			CHECK_QSTATUS();
			break;
		}
	}
	
	status = buf.append(L'\'');
	CHECK_QSTATUS();
	
	*pwstrExpr = buf.getString();
	
	return QSTATUS_SUCCESS;
}


/****************************************************************************
 *
 * MacroNumber
 *
 */

qm::MacroNumber::MacroNumber(long nValue, QSTATUS* pstatus) :
	nValue_(nValue)
{
}

qm::MacroNumber::~MacroNumber()
{
}

QSTATUS qm::MacroNumber::value(MacroContext* pContext,
	MacroValue** ppValue) const
{
	assert(pContext);
	assert(ppValue);
	
	return MacroValueFactory::getFactory().newNumber(nValue_,
		reinterpret_cast<MacroValueNumber**>(ppValue));
}

QSTATUS qm::MacroNumber::getString(WSTRING* pwstrExpr) const
{
	assert(pwstrExpr);
	WCHAR wsz[32];
	swprintf(wsz, L"%ld", nValue_);
	*pwstrExpr = allocWString(wsz);
	return *pwstrExpr ? QSTATUS_SUCCESS : QSTATUS_OUTOFMEMORY;
}


/****************************************************************************
 *
 * MacroRegex
 *
 */

qm::MacroRegex::MacroRegex(const WCHAR* pwszRegex, qs::QSTATUS* pstatus) :
	wstrPattern_(0),
	pPattern_(0)
{
	DECLARE_QSTATUS();
	
	string_ptr<WSTRING> wstrPattern(allocWString(pwszRegex));
	if (!wstrPattern.get()) {
		*pstatus = QSTATUS_OUTOFMEMORY;
		return;
	}
	
	status = RegexCompiler().compile(pwszRegex, &pPattern_);
	CHECK_QSTATUS_SET(pstatus);
	wstrPattern_ = wstrPattern.release();
}

qm::MacroRegex::~MacroRegex()
{
	freeWString(wstrPattern_);
	delete pPattern_;
}

QSTATUS qm::MacroRegex::value(MacroContext* pContext, MacroValue** ppValue) const
{
	assert(pContext);
	assert(ppValue);
	
	return MacroValueFactory::getFactory().newRegex(wstrPattern_,
		pPattern_, reinterpret_cast<MacroValueRegex**>(ppValue));
}

QSTATUS qm::MacroRegex::getString(qs::WSTRING* pwstrExpr) const
{
	assert(pwstrExpr);
	
	DECLARE_QSTATUS();
	
	*pwstrExpr = 0;
	
	StringBuffer<WSTRING> buf(&status);
	CHECK_QSTATUS();
	
	status = buf.append(L'/');
	CHECK_QSTATUS();
	
	for (const WCHAR* p = wstrPattern_; *p; ++p) {
		if (*p == L'/') {
			status = buf.append(L"\\/");
			CHECK_QSTATUS();
		}
		else {
			status = buf.append(*p);
			CHECK_QSTATUS();
		}
	}
	
	status = buf.append(L'/');
	CHECK_QSTATUS();
	
	*pwstrExpr = buf.getString();
	
	return QSTATUS_SUCCESS;
}


/****************************************************************************
 *
 * MacroVariable
 *
 */

qm::MacroVariable::MacroVariable(const WCHAR* pwszName, QSTATUS* pstatus) :
	wstrName_(0),
	n_(static_cast<unsigned int>(-1))
{
	assert(pstatus);
	
	*pstatus = QSTATUS_SUCCESS;
	
	if (L'0' <= *pwszName && *pwszName <= L'9') {
		for (const WCHAR* p = pwszName; *p; ++p) {
			if (*p < L'0' || L'9' < *p)
				break;
		}
		if (*p) {
			*pstatus = QSTATUS_FAIL;
			return;
		}
		WCHAR* pEnd = 0;
		n_ = wcstol(pwszName, &pEnd, 10);
	}
	else {
		wstrName_ = allocWString(pwszName);
		if (!wstrName_) {
			*pstatus = QSTATUS_OUTOFMEMORY;
			return;
		}
	}
}

qm::MacroVariable::~MacroVariable()
{
	freeWString(wstrName_);
}

QSTATUS qm::MacroVariable::value(MacroContext* pContext,
	MacroValue** ppValue) const
{
	assert(pContext);
	assert(ppValue);
	
	DECLARE_QSTATUS();
	
	*ppValue = 0;
	
	if (n_ != static_cast<unsigned int>(-1)) {
		status = pContext->getArgument(n_, ppValue);
		CHECK_QSTATUS();
	}
	else {
		status = pContext->getVariable(wstrName_, ppValue);
		CHECK_QSTATUS();
	}
	if (!*ppValue) {
		status = MacroValueFactory::getFactory().newString(
			L"", reinterpret_cast<MacroValueString**>(ppValue));
		CHECK_QSTATUS();
	}
	
	return QSTATUS_SUCCESS;
}

QSTATUS qm::MacroVariable::getString(WSTRING* pwstrExpr) const
{
	assert(pwstrExpr);
	*pwstrExpr = concat(L"$", wstrName_);
	return *pwstrExpr ? QSTATUS_SUCCESS : QSTATUS_OUTOFMEMORY;
}


/****************************************************************************
 *
 * MacroExprPtr
 *
 */

qm::MacroExprPtr::MacroExprPtr() :
	pExpr_(0)
{
}

qm::MacroExprPtr::MacroExprPtr(MacroExpr* pExpr) :
	pExpr_(pExpr)
{
}

qm::MacroExprPtr::~MacroExprPtr()
{
	if (pExpr_)
		pExpr_->release();
}

MacroExpr* qm::MacroExprPtr::get() const
{
	return pExpr_;
}

MacroExpr* qm::MacroExprPtr::release()
{
	MacroExpr* pExpr = pExpr_;
	pExpr_ = 0;
	return pExpr;
}

void qm::MacroExprPtr::reset(MacroExpr* pExpr)
{
	if (pExpr_)
		pExpr_->release();
	pExpr_ = pExpr;
}


/****************************************************************************
 *
 * MacroErrorHandler
 *
 */

qm::MacroErrorHandler::~MacroErrorHandler()
{
}


/****************************************************************************
 *
 * Macro
 *
 */

qm::Macro::Macro(MacroExpr* pExpr, QSTATUS* pstatus) :
	pExpr_(pExpr)
{
	assert(pstatus);
	*pstatus = QSTATUS_SUCCESS;
}

qm::Macro::~Macro()
{
	if (pExpr_)
		pExpr_->release();
}

QSTATUS qm::Macro::value(MacroContext* pContext, MacroValue** ppValue) const
{
	assert(pContext);
	assert(ppValue);
	
	return pExpr_->value(pContext, ppValue);
}

QSTATUS qm::Macro::getString(WSTRING* pwstrMacro) const
{
	assert(pwstrMacro);
	return pExpr_->getString(pwstrMacro);
}


/****************************************************************************
 *
 * MacroParser
 *
 */

qm::MacroParser::MacroParser(Type type, QSTATUS* pstatus) :
	type_(type),
	pErrorHandler_(0)
{
	assert(pstatus);
	*pstatus = QSTATUS_SUCCESS;
}

qm::MacroParser::~MacroParser()
{
}

QSTATUS qm::MacroParser::parse(const WCHAR* pwszMacro, Macro** ppMacro) const
{
	return parse(pwszMacro, 0, ppMacro);
}

QSTATUS qm::MacroParser::parse(const WCHAR* pwszMacro,
	Macro* pParentMacro, Macro** ppMacro) const
{
	assert(pwszMacro);
	assert(ppMacro);
	
	DECLARE_QSTATUS();
	
	MacroExprPtr pMacroExpr;
	
	MacroTokenizer tokenizer(pwszMacro, &status);
	CHECK_QSTATUS();
	
	typedef std::vector<MacroFunction*> FunctionStack;
	FunctionStack stackFunction;
	STLWrapper<FunctionStack> wrapper(stackFunction);
	
	bool bEnd = false;
	while (!bEnd) {
		MacroTokenizer::Token token;
		string_ptr<WSTRING> wstrToken;
		status = tokenizer.getToken(&token, &wstrToken);
		CHECK_QSTATUS();
		if (pMacroExpr.get() && token != MacroTokenizer::TOKEN_END) {
			return error(MacroErrorHandler::CODE_MACROCONTAINSMORETHANONEEXPR,
				tokenizer.getLastPosition());
		}
		else {
			MacroExprPtr pExpr;
			switch (token) {
			case MacroTokenizer::TOKEN_TEXT:
				if (isNumber(wstrToken.get())) {
					WCHAR* pEnd = 0;
					long n = wcstol(wstrToken.get(), &pEnd, 10);
					std::auto_ptr<MacroNumber> pNumber;
					status = newQsObject(n, &pNumber);
					CHECK_QSTATUS();
					pExpr.reset(pNumber.release());
				}
				else {
					std::auto_ptr<MacroField> pField;
					status = newQsObject(wstrToken.get(), &pField);
					CHECK_QSTATUS();
					pExpr.reset(pField.release());
				}
				break;
			case MacroTokenizer::TOKEN_LITERAL:
				{
					std::auto_ptr<MacroLiteral> pLiteral;
					status = newQsObject(wstrToken.get(), &pLiteral);
					CHECK_QSTATUS();
					pExpr.reset(pLiteral.release());
				}
				break;
			case MacroTokenizer::TOKEN_REGEX:
				{
					std::auto_ptr<MacroRegex> pRegex;
					status = newQsObject(wstrToken.get(), &pRegex);
					CHECK_QSTATUS();
					pExpr.reset(pRegex.release());
				}
				break;
			case MacroTokenizer::TOKEN_AT:
				{
					const WCHAR* pLast = tokenizer.getLastPosition();
					string_ptr<WSTRING> wstrFunction;
					status = tokenizer.getToken(&token, &wstrFunction);
					CHECK_QSTATUS();
					if (token != MacroTokenizer::TOKEN_TEXT)
						return error(MacroErrorHandler::CODE_INVALIDFUNCTIONNAME, pLast);
					status = tokenizer.getToken(&token, &wstrToken);
					CHECK_QSTATUS();
					if (token != MacroTokenizer::TOKEN_LEFTPARENTHESIS)
						return error(MacroErrorHandler::CODE_FUNCTIONWITHOUTPARENTHESIS, pLast);
					MacroFunction* pFunction = 0;
					status = MacroFunctionFactory::getFactory().newFunction(
						type_, wstrFunction.get(), &pFunction);
					CHECK_QSTATUS();
					assert(pFunction);
					std::auto_ptr<MacroFunction> apFunction(pFunction);
					status = wrapper.push_back(pFunction);
					CHECK_QSTATUS();
					apFunction.release();
				}
				break;
			case MacroTokenizer::TOKEN_DOLLAR:
				{
					const WCHAR* pLast = tokenizer.getLastPosition();
					string_ptr<WSTRING> wstrVariable;
					status = tokenizer.getToken(&token, &wstrVariable);
					CHECK_QSTATUS();
					if (token != MacroTokenizer::TOKEN_TEXT)
						return error(MacroErrorHandler::CODE_INVALIDVARIABLENAME, pLast);
					std::auto_ptr<MacroVariable> pVariable;
					status = newQsObject(wstrVariable.get(), &pVariable);
					CHECK_QSTATUS();
					pExpr.reset(pVariable.release());
				}
				break;
			case MacroTokenizer::TOKEN_PERCENT:
				{
					const WCHAR* pLast = tokenizer.getLastPosition();
					string_ptr<WSTRING> wstrField;
					status = tokenizer.getToken(&token, &wstrField);
					CHECK_QSTATUS();
					if (token != MacroTokenizer::TOKEN_TEXT)
						return error(MacroErrorHandler::CODE_INVALIDFIELDNAME, pLast);
					std::auto_ptr<MacroFieldCache> pField;
					status = newQsObject(wstrField.get(), &pField);
					if (status == QSTATUS_FAIL)
						return error(MacroErrorHandler::CODE_INVALIDFIELDNAME, pLast);
					CHECK_QSTATUS();
					pExpr.reset(pField.release());
				}
				break;
			case MacroTokenizer::TOKEN_LEFTPARENTHESIS:
				return error(MacroErrorHandler::CODE_SYNTAXERROR,
					tokenizer.getLastPosition());
			case MacroTokenizer::TOKEN_RIGHTPARENTHESIS:
				if (stackFunction.empty())
					return error(MacroErrorHandler::CODE_SYNTAXERROR,
						tokenizer.getLastPosition());
				pExpr.reset(stackFunction.back());
				stackFunction.pop_back();
				break;
			case MacroTokenizer::TOKEN_COMMA:
				if (stackFunction.empty())
					return error(MacroErrorHandler::CODE_SYNTAXERROR,
						tokenizer.getLastPosition());
				break;
			case MacroTokenizer::TOKEN_END:
				if (!stackFunction.empty())
					return error(MacroErrorHandler::CODE_SYNTAXERROR,
						tokenizer.getLastPosition());
				if (!pMacroExpr.get()) {
					std::auto_ptr<MacroLiteral> pLiteral;
					status = newQsObject(L"", &pLiteral);
					CHECK_QSTATUS();
					pMacroExpr.reset(pLiteral.release());
				}
				bEnd = true;
				break;
			case MacroTokenizer::TOKEN_ERROR:
				return error(MacroErrorHandler::CODE_SYNTAXERROR,
					tokenizer.getLastPosition());
			}
			
			if (pExpr.get()) {
				if (stackFunction.empty()) {
					assert(!pMacroExpr.get());
					pMacroExpr.reset(pExpr.release());
				}
				else {
					status = stackFunction.back()->addArg(pExpr.get());
					CHECK_QSTATUS();
					pExpr.release();
				}
			}
		}
	}
	
	assert(pMacroExpr.get());
	status = newQsObject(pMacroExpr.get(), ppMacro);
	CHECK_QSTATUS();
	pMacroExpr.release();
	
	return QSTATUS_SUCCESS;
}

void qm::MacroParser::setErrorHandler(MacroErrorHandler* pErrorHandler)
{
	pErrorHandler_ = pErrorHandler;
}

MacroErrorHandler* qm::MacroParser::getErrorHandler() const
{
	return pErrorHandler_;
}

bool qm::MacroParser::isNumber(const WCHAR* pwsz)
{
	if (*pwsz == L'-')
		++pwsz;
	while (L'0' <= *pwsz && *pwsz <= L'9')
		++pwsz;
	return !*pwsz;
}

QSTATUS qm::MacroParser::error(MacroErrorHandler::Code code, const WCHAR* p) const
{
	if (pErrorHandler_)
		pErrorHandler_->parseError(code, p);
	return QSTATUS_FAIL;
}


/****************************************************************************
 *
 * MacroVariableHolderImpl
 *
 */

struct qm::MacroVariableHolderImpl
{
public:
	typedef std::vector<std::pair<qs::WSTRING, MacroValue*> > VariableMap;

public:
	VariableMap mapVariable_;
};


/****************************************************************************
 *
 * MacroVariableHolder
 *
 */

qm::MacroVariableHolder::MacroVariableHolder(QSTATUS* pstatus) :
	pImpl_(0)
{
	assert(pstatus);
	
	DECLARE_QSTATUS();
	
	*pstatus = QSTATUS_SUCCESS;
	
	status = newObject(&pImpl_);
	CHECK_QSTATUS_SET(pstatus);
}

qm::MacroVariableHolder::~MacroVariableHolder()
{
	if (pImpl_) {
		MacroValuePtr pValue;
		MacroVariableHolderImpl::VariableMap::iterator it =
			pImpl_->mapVariable_.begin();
		while (it != pImpl_->mapVariable_.end()) {
			freeWString((*it).first);
			pValue.reset((*it).second);
			++it;
		}
		delete pImpl_;
	}
}

QSTATUS qm::MacroVariableHolder::getVariable(
	const WCHAR* pwszName, MacroValue** ppValue) const
{
	assert(pwszName);
	assert(ppValue);
	
	DECLARE_QSTATUS();
	
	*ppValue = 0;
	
	typedef MacroVariableHolderImpl::VariableMap Map;
	Map::const_iterator it = std::find_if(
		pImpl_->mapVariable_.begin(), pImpl_->mapVariable_.end(),
		std::bind2nd(
			binary_compose_f_gx_hy(
				string_equal<WCHAR>(),
				std::select1st<Map::value_type>(),
				std::identity<const WCHAR*>()),
			pwszName));
	if (it != pImpl_->mapVariable_.end()) {
		status = (*it).second->clone(ppValue);
		CHECK_QSTATUS();
	}
	
	return QSTATUS_SUCCESS;
}

QSTATUS qm::MacroVariableHolder::setVariable(
	const WCHAR* pwszName, MacroValue* pValue)
{
	assert(pwszName);
	assert(pValue);
	
	DECLARE_QSTATUS();
	
	MacroValue* pCloneValue = 0;
	status = pValue->clone(&pCloneValue);
	CHECK_QSTATUS();
	std::auto_ptr<MacroValue> apCloneValue(pCloneValue);
	
	typedef MacroVariableHolderImpl::VariableMap Map;
	Map::iterator it = std::find_if(
		pImpl_->mapVariable_.begin(), pImpl_->mapVariable_.end(),
		std::bind2nd(
			binary_compose_f_gx_hy(
				string_equal<WCHAR>(),
				std::select1st<Map::value_type>(),
				std::identity<const WCHAR*>()),
			pwszName));
	if (it != pImpl_->mapVariable_.end()) {
		MacroValuePtr pValue((*it).second);
		(*it).second = pCloneValue;
	}
	else {
		string_ptr<WSTRING> wstrName(allocWString(pwszName));
		if (!wstrName.get())
			return QSTATUS_OUTOFMEMORY;
		status = STLWrapper<Map>(pImpl_->mapVariable_).push_back(
			std::make_pair(wstrName.get(), pCloneValue));
		CHECK_QSTATUS();
		wstrName.release();
	}
	apCloneValue.release();
	
	return QSTATUS_SUCCESS;
}


/****************************************************************************
 *
 * MacroContext
 *
 */

qm::MacroContext::MacroContext(const Init& init, QSTATUS* pstatus) :
	pmh_(init.pmh_),
	pMessage_(init.pMessage_),
	pAccount_(init.pAccount_),
	pGlobalContext_(0),
	bOwnGlobalContext_(true)
{
	assert((!init.pmh_ && !init.pMessage_) || (init.pmh_ && init.pMessage_));
	assert(!init.pmh_ || init.pmh_->getAccount() == init.pAccount_);
	assert(!init.pmh_ || init.pmh_->getAccount()->isLocked());
	assert(init.pAccount_);
	assert(init.pDocument_);
	assert(init.hwnd_);
	assert(init.pProfile_);
	assert(pstatus);
	
	DECLARE_QSTATUS();
	
	*pstatus = QSTATUS_SUCCESS;
	
	MacroGlobalContext::Init globalInit = {
		init.pDocument_,
		init.hwnd_,
		init.pProfile_,
		init.bGetMessageAsPossible_,
		init.pErrorHandler_,
		init.pGlobalVariable_
	};
	status = newQsObject(globalInit, &pGlobalContext_);
	CHECK_QSTATUS_SET(pstatus);
}

qm::MacroContext::MacroContext(MessageHolderBase* pmh, Message* pMessage,
	const MacroContext* pContext, QSTATUS* pstatus) :
	pmh_(pmh),
	pMessage_(pMessage),
	pAccount_(pmh->getFolder()->getAccount()),
	pGlobalContext_(pContext->pGlobalContext_),
	bOwnGlobalContext_(false)
{
	assert(pmh);
	assert(pMessage);
	assert(pContext);
}

qm::MacroContext::~MacroContext()
{
	if (bOwnGlobalContext_)
		delete pGlobalContext_;
}

MessageHolderBase* qm::MacroContext::getMessageHolder() const
{
	return pmh_;
}

Message* qm::MacroContext::getMessage() const
{
	return pMessage_;
}

QSTATUS qm::MacroContext::getMessage(MessageType type,
	const WCHAR* pwszField, Message** ppMessage) const
{
	assert(ppMessage);
	
	DECLARE_QSTATUS();
	
	*ppMessage = 0;
	
	Message::Flag flag = pMessage_->getFlag();
	unsigned int nFlags = 0;
	switch (type) {
	case MESSAGETYPE_HEADER:
		if (flag == Message::FLAG_EMPTY ||
			flag == Message::FLAG_TEMPORARY)
			nFlags = Account::GETMESSAGEFLAG_HEADER;
		break;
	case MESSAGETYPE_TEXT:
		if (flag == Message::FLAG_EMPTY ||
			flag == Message::FLAG_HEADERONLY ||
			flag == Message::FLAG_TEMPORARY)
			nFlags = Account::GETMESSAGEFLAG_TEXT;
		break;
	case MESSAGETYPE_ALL:
		if (flag == Message::FLAG_EMPTY ||
			flag == Message::FLAG_HEADERONLY ||
			flag == Message::FLAG_TEXTONLY ||
			flag == Message::FLAG_TEMPORARY)
			nFlags = Account::GETMESSAGEFLAG_ALL;
		break;
	default:
		assert(false);
		break;
	}
	
	if (nFlags) {
		if (isGetMessageAsPossible())
			nFlags = Account::GETMESSAGEFLAG_POSSIBLE;
		status = pmh_->getMessage(nFlags, pwszField, pMessage_);
		CHECK_QSTATUS();
	}
	
	*ppMessage = pMessage_;
	
	return QSTATUS_SUCCESS;
}

Account* qm::MacroContext::getAccount() const
{
	return pAccount_;
}

Document* qm::MacroContext::getDocument() const
{
	return pGlobalContext_->getDocument();
}

HWND qm::MacroContext::getWindow() const
{
	return pGlobalContext_->getWindow();
}

Profile* qm::MacroContext::getProfile() const
{
	return pGlobalContext_->getProfile();
}

bool qm::MacroContext::isGetMessageAsPossible() const
{
	return pGlobalContext_->isGetMessageAsPossible();
}

MacroErrorHandler* qm::MacroContext::getErrorHandler() const
{
	return pGlobalContext_->getErrorHandler();
}

QSTATUS qm::MacroContext::getVariable(
	const WCHAR* pwszName, MacroValue** ppValue) const
{
	return pGlobalContext_->getVariable(pwszName, ppValue);
}

QSTATUS qm::MacroContext::setVariable(
	const WCHAR* pwszName, MacroValue* pValue, bool bGlobal)
{
	return pGlobalContext_->setVariable(pwszName, pValue, bGlobal);
}

QSTATUS qm::MacroContext::getFunction(
	const WCHAR* pwszName, const MacroExpr** ppExpr) const
{
	return pGlobalContext_->getFunction(pwszName, ppExpr);
}

QSTATUS qm::MacroContext::setFunction(
	const WCHAR* pwszName, const MacroExpr* pExpr, bool* pbSet)
{
	return pGlobalContext_->setFunction(pwszName, pExpr, pbSet);
}

QSTATUS qm::MacroContext::pushArgumentContext()
{
	return pGlobalContext_->pushArgumentContext();
}

void qm::MacroContext::popArgumentContext()
{
	pGlobalContext_->popArgumentContext();
}

QSTATUS qm::MacroContext::addArgument(MacroValue* pValue)
{
	return pGlobalContext_->addArgument(pValue);
}

QSTATUS qm::MacroContext::getArgument(
	unsigned int n, MacroValue** ppValue) const
{
	return pGlobalContext_->getArgument(n, ppValue);
}

QSTATUS qm::MacroContext::resolvePath(
	const WCHAR* pwszPath, qs::WSTRING* pwstrPath)
{
	assert(pwszPath);
	assert(pwstrPath);
	
	*pwstrPath = 0;
	
	string_ptr<WSTRING> wstrPath;
	
	if (*pwszPath == L'\\' || *pwszPath == L'/' ||
		(*pwszPath != L'\0' && *(pwszPath + 1) == L':')) {
		wstrPath.reset(allocWString(pwszPath));
	}
	else {
		const WCHAR* pwszMailFolder =
			Application::getApplication().getMailFolder();
		assert(pwszMailFolder[wcslen(pwszMailFolder) - 1] != L'\\');
		
#ifdef _WIN32_WCE
		int nParent = 0;
		const WCHAR* p = pwszPath;
		while (*p == L'.') {
			if (*(p + 1) == L'.' && (*(p + 2) == L'\\' || *(p + 2) == L'/')) {
				p += 3;
				++nParent;
			}
			else if (*(p + 1) == L'\\' || *(p + 1) == L'/') {
				p += 2;
			}
			else {
				break;
			}
		}
		
		const WCHAR* pEnd = pwszMailFolder + wcslen(pwszMailFolder);
		while (nParent > 0) {
			--pEnd;
			while (pEnd != pwszMailFolder && *pEnd != L'\\')
				--pEnd;
			if (pEnd == pwszMailFolder)
				break;
			--nParent;
		}
		ConcatW c[] = {
			{ pwszMailFolder,	pEnd - pwszMailFolder	},
			{ L"\\",			1						},
			{ p,				-1						}
		};
		wstrPath.reset(concat(c, countof(c)));
#else
		wstrPath.reset(concat(pwszMailFolder, L"\\", pwszPath));
#endif
	}
	if (!wstrPath.get())
		return QSTATUS_OUTOFMEMORY;
	
	*pwstrPath = wstrPath.release();
	
	return QSTATUS_SUCCESS;
}
