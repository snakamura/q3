/*
 * $Id$
 *
 * Copyright(C) 1998-2004 Satoshi Nakamura
 * All rights reserved.
 *
 */

#include <qmaccount.h>
#include <qmapplication.h>
#include <qmmacro.h>
#include <qmmessage.h>
#include <qmmessageholder.h>

#include <qsconv.h>
#include <qsinit.h>
#include <qsstl.h>

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

qm::MacroTokenizer::MacroTokenizer(const WCHAR* pwszMacro) :
	pwszMacro_(pwszMacro),
	p_(pwszMacro_),
	pLast_(pwszMacro)
{
}

qm::MacroTokenizer::~MacroTokenizer()
{
}

MacroTokenizer::Token qm::MacroTokenizer::getToken(wstring_ptr* pwstrToken,
												   wstring_ptr* pwstrTokenEx)
{
	assert(pwstrToken);
	
	pwstrToken->reset(0);
	
	while (*p_ == L' ' || *p_ == L'\t' || *p_ == L'\n')
		++p_;
	
	if (!*p_) {
		return TOKEN_END;
	}
	else if (*p_ == L'#') {
		while (*p_ == L'#') {
			while (*p_ && *p_ != L'\n')
				++p_;
			while (*p_ == L' ' || *p_ == L'\t' || *p_ == L'\n')
				++p_;
		}
		if (!*p_)
			return TOKEN_END;
	}
	
	pLast_ = p_;
	
	WCHAR c = *p_;
	if (c == L'\"' || c == L'\'') {
		StringBuffer<WSTRING> buf;
		for (++p_; *p_ && *p_ != c; ++p_) {
			if (*p_ == L'\\') {
				++p_;
				switch (*p_) {
				case L'n':
					buf.append(L'\n');
					break;
				case L't':
					buf.append(L'\t');
					break;
				case L'\\':
				case L'\'':
				case L'\"':
					buf.append(*p_);
					break;
				case L'\0':
					return TOKEN_ERROR;
					break;
				default:
					buf.append(*p_);
					break;
				}
			}
			else {
				buf.append(*p_);
			}
		}
		if (!*p_)
			return TOKEN_ERROR;
		*pwstrToken = buf.getString();
		++p_;
		return TOKEN_LITERAL;
	}
	else if (c == L'/') {
		StringBuffer<WSTRING> buf;
		for (++p_; *p_ && *p_ != L'/'; ++p_) {
			if (*p_ == L'\\') {
				if (*(p_ + 1) != L'/')
					buf.append(*p_);
				++p_;
			}
			buf.append(*p_);
		}
		if (!*p_)
			return TOKEN_ERROR;
		*pwstrToken = buf.getString();
		++p_;
		
		while (L'a' <= *p_ && *p_ <= L'z')
			buf.append(*p_++);
		if (pwstrTokenEx)
			*pwstrTokenEx = buf.getString();
		
		return TOKEN_REGEX;
	}
	else if (c == L'@') {
		++p_;
		return TOKEN_AT;
	}
	else if (c == L'$') {
		++p_;
		return TOKEN_DOLLAR;
	}
	else if (c == L'%') {
		++p_;
		return TOKEN_PERCENT;
	}
	else if (c == L'(') {
		++p_;
		return TOKEN_LEFTPARENTHESIS;
	}
	else if (c == L')') {
		++p_;
		return TOKEN_RIGHTPARENTHESIS;
	}
	else if (c == L',') {
		++p_;
		return TOKEN_COMMA;
	}
	else {
		const WCHAR* pwszSep = L" \t\n,()#";
		const WCHAR* pBegin = p_;
		while (*p_ && !wcschr(pwszSep, *p_))
			++p_;
		*pwstrToken = allocWString(pBegin, p_ - pBegin);
		return TOKEN_TEXT;
	}
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

qm::MacroGlobalContext::MacroGlobalContext(const MessageHolderList& listSelected,
										   Document* pDocument,
										   HWND hwnd,
										   Profile* pProfile,
										   bool bGetMessageAsPossible,
										   bool bDecryptVerify,
										   MacroErrorHandler* pErrorHandler,
										   MacroVariableHolder* pGlobalVariable) :
	listSelected_(listSelected),
	pDocument_(pDocument),
	hwnd_(hwnd),
	pProfile_(pProfile),
	bGetMessageAsPossible_(bGetMessageAsPossible),
	bDecryptVerify_(bDecryptVerify),
	pErrorHandler_(pErrorHandler),
	pGlobalVariable_(pGlobalVariable),
	returnType_(MacroContext::RETURNTYPE_NONE),
	nRegexResultCount_(0)
{
	assert(pDocument_);
	assert(hwnd_);
	
	pVariable_.reset(new MacroVariableHolder());
	pFunction_.reset(new MacroFunctionHolder());
	pArgument_.reset(new MacroArgumentHolder());
}

qm::MacroGlobalContext::~MacroGlobalContext()
{
}

const MessageHolderList& qm::MacroGlobalContext::getSelectedMessageHolders() const
{
	return listSelected_;
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

bool qm::MacroGlobalContext::isDecryptVerify() const
{
	return bDecryptVerify_;
}

MacroErrorHandler* qm::MacroGlobalContext::getErrorHandler() const
{
	return pErrorHandler_;
}

MacroContext::ReturnType qm::MacroGlobalContext::getReturnType() const
{
	return returnType_;
}

void qm::MacroGlobalContext::setReturnType(MacroContext::ReturnType type)
{
	returnType_ = type;
}

MacroValuePtr qm::MacroGlobalContext::getVariable(const WCHAR* pwszName) const
{
	assert(pwszName);
	
	MacroValuePtr pValue(pVariable_->getVariable(pwszName));
	if (!pValue.get() && pGlobalVariable_)
		pValue = pGlobalVariable_->getVariable(pwszName);
	return pValue;
}

bool qm::MacroGlobalContext::setVariable(const WCHAR* pwszName,
										 MacroValue* pValue,
										 bool bGlobal)
{
	assert(pwszName);
	assert(pValue);
	
	MacroValuePtr pValueClone(pValue->clone());
	if (!pValueClone.get())
		return false;
	
	if (bGlobal && pGlobalVariable_)
		pGlobalVariable_->setVariable(pwszName, pValueClone);
	else
		pVariable_->setVariable(pwszName, pValueClone);
	
	return true;
}

void qm::MacroGlobalContext::removeVariable(const WCHAR* pwszName,
											bool bGlobal)
{
	if (bGlobal && pGlobalVariable_)
		pGlobalVariable_->removeVariable(pwszName);
	else
		pVariable_->removeVariable(pwszName);
}

const MacroExpr* qm::MacroGlobalContext::getFunction(const WCHAR* pwszName) const
{
	return pFunction_->getFunction(pwszName);
}

bool qm::MacroGlobalContext::setFunction(const WCHAR* pwszName,
										 const MacroExpr* pExpr)
{
	return pFunction_->setFunction(pwszName, pExpr);
}

void qm::MacroGlobalContext::pushArgumentContext()
{
	pArgument_->pushContext();
}

void qm::MacroGlobalContext::popArgumentContext()
{
	pArgument_->popContext();
}

void qm::MacroGlobalContext::addArgument(MacroValuePtr pValue)
{
	pArgument_->addArgument(pValue);
}

MacroValuePtr qm::MacroGlobalContext::getArgument(unsigned int n) const
{
	return pArgument_->getArgument(n);
}

bool qm::MacroGlobalContext::setRegexResult(const RegexRangeList& listRange)
{
	clearRegexResult();
	
	for (RegexRangeList::List::size_type n = 0; n < listRange.list_.size(); ++n) {
		const RegexRange& range = listRange.list_[n];
		
		WCHAR wszName[32];
		swprintf(wszName, L"_%u", n);
		MacroValuePtr pValue(MacroValueFactory::getFactory().newString(
			range.pStart_, range.pEnd_ - range.pStart_));
		if (!pValue.get())
			return false;
		if (!setVariable(wszName, pValue.get(), false))
			return false;
	}
	
	nRegexResultCount_ = listRange.list_.size();
	
	return true;
}

void qm::MacroGlobalContext::clearRegexResult()
{
	for (size_t n = 0; n < nRegexResultCount_; ++n) {
		WCHAR wszName[32];
		swprintf(wszName, L"_%u", n);
		removeVariable(wszName, false);
	}
	nRegexResultCount_ = 0;
}


/****************************************************************************
 *
 * MacroFunctionHolder
 *
 */

qm::MacroFunctionHolder::MacroFunctionHolder()
{
}

qm::MacroFunctionHolder::~MacroFunctionHolder()
{
	std::for_each(mapFunction_.begin(), mapFunction_.end(),
		unary_compose_f_gx(
			string_free<WSTRING>(),
			std::select1st<FunctionMap::value_type>()));
}

const MacroExpr* qm::MacroFunctionHolder::getFunction(const WCHAR* pwszName) const
{
	assert(pwszName);
	
	FunctionMap::const_iterator it = std::find_if(
		mapFunction_.begin(), mapFunction_.end(),
		std::bind2nd(
			binary_compose_f_gx_hy(
				string_equal_i<WCHAR>(),
				std::select1st<FunctionMap::value_type>(),
				std::identity<const WCHAR*>()),
			pwszName));
	if (it != mapFunction_.end())
		return (*it).second;
	else
		return 0;
}

bool qm::MacroFunctionHolder::setFunction(const WCHAR* pwszName,
										  const MacroExpr* pExpr)
{
	assert(pwszName);
	assert(pExpr);
	
	FunctionMap::const_iterator it = std::find_if(
		mapFunction_.begin(), mapFunction_.end(),
		std::bind2nd(
			binary_compose_f_gx_hy(
				string_equal_i<WCHAR>(),
				std::select1st<FunctionMap::value_type>(),
				std::identity<const WCHAR*>()),
			pwszName));
	if (it != mapFunction_.end())
		return false;
	
	wstring_ptr wstrName(allocWString(pwszName));
	mapFunction_.push_back(std::make_pair(wstrName.get(), pExpr));
	wstrName.release();
	return true;
}


/****************************************************************************
 *
 * MacroArgumentHolder
 *
 */

qm::MacroArgumentHolder::MacroArgumentHolder()
{
}

qm::MacroArgumentHolder::~MacroArgumentHolder()
{
}

void qm::MacroArgumentHolder::pushContext()
{
	listArgument_.resize(listArgument_.size() + 1);
}

void qm::MacroArgumentHolder::popContext()
{
	assert(!listArgument_.empty());
	
	std::vector<MacroValue*>& v = listArgument_.back();
	for (std::vector<MacroValue*>::iterator it = v.begin(); it != v.end(); ++it)
		MacroValuePtr pValue(*it);
	listArgument_.pop_back();
}

void qm::MacroArgumentHolder::addArgument(MacroValuePtr pValue)
{
	assert(!listArgument_.empty());
	listArgument_.back().push_back(pValue.get());
	pValue.release();
}

MacroValuePtr qm::MacroArgumentHolder::getArgument(unsigned int n) const
{
	if (listArgument_.empty())
		return MacroValuePtr();
	
	const std::vector<MacroValue*>& v = listArgument_.back();
	if (n >= v.size())
		return MacroValuePtr();
	else
		return v[n]->clone();
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

MacroValuePtr qm::MacroExpr::error(const MacroContext& context,
								   MacroErrorHandler::Code code) const
{
	if (context.getErrorHandler()) {
		wstring_ptr wstr(getString());
		context.getErrorHandler()->processError(code, wstr.get());
	}
	
	Log log(InitThread::getInitThread().getLogger(), L"qm::MacroExpr");
	if (log.isErrorEnabled()) {
		WCHAR wsz[128];
		swprintf(wsz, L"Error occured while processing macro: code=%u at ", code);
		wstring_ptr wstr(getString());
		wstring_ptr wstrLog(concat(wsz, wstr.get()));
		log.error(wstrLog.get());
	}
	
	return MacroValuePtr();
}


/****************************************************************************
 *
 * MacroField
 *
 */

qm::MacroField::MacroField(const WCHAR* pwszName)
{
	wstrName_ = allocWString(pwszName);
}

qm::MacroField::~MacroField()
{
}

MacroValuePtr qm::MacroField::value(MacroContext* pContext) const
{
	assert(pContext);
	
	MessageHolderBase* pmh = pContext->getMessageHolder();
	if (!pmh)
		return error(*pContext, MacroErrorHandler::CODE_NOCONTEXTMESSAGE);
	
	Message* pMessage = pContext->getMessage(
		MacroContext::MESSAGETYPE_HEADER, wstrName_.get());
	if (!pMessage)
		return error(*pContext, MacroErrorHandler::CODE_GETMESSAGE);
	
	string_ptr strHeader(PartUtil(*pMessage).getHeader(wstrName_.get()));
	return MacroValueFactory::getFactory().newField(wstrName_.get(), strHeader.get());
}

wstring_ptr qm::MacroField::getString() const
{
	return allocWString(wstrName_.get());
}


/****************************************************************************
 *
 * MacroFieldCache
 *
 */

qm::MacroFieldCache::MacroFieldCache(Type type) :
	type_(type)
{
}

qm::MacroFieldCache::~MacroFieldCache()
{
}

MacroValuePtr qm::MacroFieldCache::value(MacroContext* pContext) const
{
	assert(pContext);
	
	MessageHolderBase* pmh = pContext->getMessageHolder();
	if (!pmh)
		return error(*pContext, MacroErrorHandler::CODE_NOCONTEXTMESSAGE);
	
	unsigned int (qm::MessageHolderBase::*pfnGetNumber)() const = 0;
	wstring_ptr (qm::MessageHolderBase::*pfnGetString)() const = 0;
	void (qm::MessageHolderBase::*pfnGetTime)(Time*) const = 0;
	
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
		return factory.newNumber((pmh->*pfnGetNumber)());
	}
	else if (pfnGetString) {
		wstring_ptr wstr((pmh->*pfnGetString)());
		return factory.newString(wstr.get());
	}
	else if (pfnGetTime) {
		Time time;
		(pmh->*pfnGetTime)(&time);
		return factory.newTime(time);
	}
	else {
		assert(false);
		return MacroValuePtr();
	}
}

wstring_ptr qm::MacroFieldCache::getString() const
{
	const WCHAR* pwszNames[] = {
		L"ID",
		L"Date",
		L"From",
		L"To",
		L"FromTo",
		L"Subject",
		L"Size"
	};
	
	return concat(L"%", pwszNames[type_]);
}

MacroFieldCache::Type qm::MacroFieldCache::getType(const WCHAR* pwszType)
{
	struct {
		const WCHAR* pwsz_;
		Type type_;
	} types[] = {
		{ L"id",		TYPE_ID			},
		{ L"date",		TYPE_DATE		},
		{ L"from",		TYPE_FROM		},
		{ L"to",		TYPE_TO			},
		{ L"fromto",	TYPE_FROMTO		},
		{ L"subject",	TYPE_SUBJECT	},
		{ L"size",		TYPE_SIZE		}
	};
	for (int n = 0; n < countof(types); ++n) {
		if (_wcsicmp(pwszType, types[n].pwsz_) == 0)
			return types[n].type_;
	}
	return TYPE_ERROR;
}


/****************************************************************************
 *
 * MacroLiteral
 *
 */

qm::MacroLiteral::MacroLiteral(const WCHAR* pwszValue)
{
	wstrValue_ = allocWString(pwszValue);
}

qm::MacroLiteral::~MacroLiteral()
{
}

MacroValuePtr qm::MacroLiteral::value(MacroContext* pContext) const
{
	assert(pContext);
	return MacroValueFactory::getFactory().newString(wstrValue_.get());
}

wstring_ptr qm::MacroLiteral::getString() const
{
	StringBuffer<WSTRING> buf;
	buf.append(L'\'');
	for (const WCHAR* p = wstrValue_.get(); *p; ++p) {
		switch (*p) {
		case L'\n':
			buf.append(L"\\n");
			break;
		case L'\t':
			buf.append(L"\\t");
			break;
		case L'\\':
			buf.append(L"\\\\");
			break;
		case L'\"':
			buf.append(L"\\\"");
			break;
		case L'\'':
			buf.append(L"\\\'");
			break;
		default:
			buf.append(*p);
			break;
		}
	}
	buf.append(L'\'');
	
	return buf.getString();
}


/****************************************************************************
 *
 * MacroNumber
 *
 */

qm::MacroNumber::MacroNumber(unsigned int nValue) :
	nValue_(nValue)
{
}

qm::MacroNumber::~MacroNumber()
{
}

MacroValuePtr qm::MacroNumber::value(MacroContext* pContext) const
{
	assert(pContext);
	return MacroValueFactory::getFactory().newNumber(nValue_);
}

wstring_ptr qm::MacroNumber::getString() const
{
	WCHAR wsz[32];
	swprintf(wsz, L"%u", nValue_);
	return allocWString(wsz);
}


/****************************************************************************
 *
 * MacroRegex
 *
 */

qm::MacroRegex::MacroRegex(const WCHAR* pwszPattern,
						   const WCHAR* pwszMode)
{
	wstrPattern_ = allocWString(pwszPattern);
	wstrMode_ = allocWString(pwszMode);
	
	unsigned int nMode = 0;
	for (const WCHAR* p = pwszMode; *p; ++p) {
		switch (*p) {
		case L'm':
			nMode |= RegexCompiler::MODE_MULTILINE;
			break;
		case L's':
			nMode |= RegexCompiler::MODE_DOTALL;
			break;
		default:
			break;
		}
	}
	pPattern_ = RegexCompiler().compile(pwszPattern, nMode);
}

qm::MacroRegex::~MacroRegex()
{
}

bool qm::MacroRegex::operator!() const
{
	return pPattern_.get() == 0;
}

MacroValuePtr qm::MacroRegex::value(MacroContext* pContext) const
{
	assert(pContext);
	return MacroValueFactory::getFactory().newRegex(
		wstrPattern_.get(), pPattern_.get());
}

wstring_ptr qm::MacroRegex::getString() const
{
	StringBuffer<WSTRING> buf;
	
	buf.append(L'/');
	for (const WCHAR* p = wstrPattern_.get(); *p; ++p) {
		if (*p == L'/')
			buf.append(L"\\/");
		else
			buf.append(*p);
	}
	buf.append(L'/');
	
	buf.append(wstrMode_.get());
	
	return buf.getString();
}


/****************************************************************************
 *
 * MacroVariable
 *
 */

qm::MacroVariable::MacroVariable(const WCHAR* pwszName) :
	n_(-1)
{
	if (L'0' <= *pwszName && *pwszName <= L'9') {
		const WCHAR* p = pwszName;
		while (*p) {
			if (*p < L'0' || L'9' < *p)
				break;
			++p;
		}
		if (!*p) {
			WCHAR* pEnd = 0;
			n_ = wcstol(pwszName, &pEnd, 10);
		}
	}
	if (n_ == -1)
		wstrName_ = allocWString(pwszName);
}

qm::MacroVariable::~MacroVariable()
{
}

MacroValuePtr qm::MacroVariable::value(MacroContext* pContext) const
{
	assert(pContext);
	
	MacroValuePtr pValue;
	if (n_ != -1)
		pValue = pContext->getArgument(n_);
	else
		pValue = pContext->getVariable(wstrName_.get());
	if (!pValue.get())
		pValue = MacroValueFactory::getFactory().newString(L"");
	return pValue;
}

wstring_ptr qm::MacroVariable::getString() const
{
	if (n_ != -1) {
		wstring_ptr wstrName(allocWString(32));
		swprintf(wstrName.get(), L"$%u", n_);
		return wstrName;
	}
	else {
		return concat(L"$", wstrName_.get());
	}
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

qm::MacroExprPtr::MacroExprPtr(MacroExprPtr& pExpr) :
	pExpr_(pExpr.release())
{
}

qm::MacroExprPtr::~MacroExprPtr()
{
	if (pExpr_)
		pExpr_->release();
}

MacroExprPtr& qm::MacroExprPtr::operator=(MacroExprPtr& pExpr)
{
	if (&pExpr != this)
		reset(pExpr.release());
	return *this;
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

qm::Macro::Macro(MacroExprPtr pExpr) :
	pExpr_(pExpr.release())
{
}

qm::Macro::~Macro()
{
	pExpr_->release();
}

MacroValuePtr qm::Macro::value(MacroContext* pContext) const
{
	assert(pContext);
	return pExpr_->value(pContext);
}

wstring_ptr qm::Macro::getString() const
{
	return pExpr_->getString();
}


/****************************************************************************
 *
 * MacroParser
 *
 */

qm::MacroParser::MacroParser(Type type) :
	type_(type),
	pErrorHandler_(0)
{
}

qm::MacroParser::~MacroParser()
{
}

std::auto_ptr<Macro> qm::MacroParser::parse(const WCHAR* pwszMacro) const
{
	return parse(pwszMacro, 0);
}

std::auto_ptr<Macro> qm::MacroParser::parse(const WCHAR* pwszMacro,
											Macro* pParentMacro) const
{
	assert(pwszMacro);
	
	MacroExprPtr pMacroExpr;
	
	MacroTokenizer tokenizer(pwszMacro);
	
	typedef std::vector<MacroFunction*> FunctionStack;
	FunctionStack stackFunction;
	struct Deleter
	{
		typedef std::vector<MacroFunction*> FunctionStack;
		
		Deleter(FunctionStack& stack) :
			stack_(stack)
		{
		}
		
		~Deleter()
		{
			std::for_each(stack_.begin(), stack_.end(), qs::deleter<MacroFunction>());
		}
		
		FunctionStack& stack_;
	} deleter(stackFunction);
	
	bool bEnd = false;
	while (!bEnd) {
		wstring_ptr wstrToken;
		wstring_ptr wstrTokenEx;
		MacroTokenizer::Token token = tokenizer.getToken(&wstrToken, &wstrTokenEx);
		if (pMacroExpr.get() && token != MacroTokenizer::TOKEN_END) {
			return error(MacroErrorHandler::CODE_MACROCONTAINSMORETHANONEEXPR,
				tokenizer.getLastPosition());
		}
		else {
			MacroExprPtr pExpr;
			switch (token) {
			case MacroTokenizer::TOKEN_TEXT:
				if (isNumber(wstrToken.get())) {
					unsigned int n = 0;
					swscanf(wstrToken.get(), L"%u", &n);
					pExpr.reset(new MacroNumber(n));
				}
				else {
					pExpr.reset(new MacroField(wstrToken.get()));
				}
				break;
			case MacroTokenizer::TOKEN_LITERAL:
				pExpr.reset(new MacroLiteral(wstrToken.get()));
				break;
			case MacroTokenizer::TOKEN_REGEX:
				{
					std::auto_ptr<MacroRegex> pRegex(new MacroRegex(
						wstrToken.get(), wstrTokenEx.get()));
					if (!*pRegex.get())
						return error(MacroErrorHandler::CODE_INVALIDREGEX,
							tokenizer.getLastPosition());
					pExpr.reset(pRegex.release());
				}
				break;
			case MacroTokenizer::TOKEN_AT:
				{
					const WCHAR* pLast = tokenizer.getLastPosition();
					wstring_ptr wstrFunction;
					token = tokenizer.getToken(&wstrFunction, 0);
					if (token != MacroTokenizer::TOKEN_TEXT)
						return error(MacroErrorHandler::CODE_INVALIDFUNCTIONNAME, pLast);
					token = tokenizer.getToken(&wstrToken, 0);
					if (token != MacroTokenizer::TOKEN_LEFTPARENTHESIS)
						return error(MacroErrorHandler::CODE_FUNCTIONWITHOUTPARENTHESIS, pLast);
					
					std::auto_ptr<MacroFunction> pFunction(
						MacroFunctionFactory::getFactory().newFunction(
							type_, wstrFunction.get()));
					stackFunction.push_back(pFunction.get());
					pFunction.release();
				}
				break;
			case MacroTokenizer::TOKEN_DOLLAR:
				{
					const WCHAR* pLast = tokenizer.getLastPosition();
					wstring_ptr wstrVariable;
					token = tokenizer.getToken(&wstrVariable, 0);
					if (token != MacroTokenizer::TOKEN_TEXT)
						return error(MacroErrorHandler::CODE_INVALIDVARIABLENAME, pLast);
					
					pExpr.reset(new MacroVariable(wstrVariable.get()));
				}
				break;
			case MacroTokenizer::TOKEN_PERCENT:
				{
					const WCHAR* pLast = tokenizer.getLastPosition();
					wstring_ptr wstrType;
					token = tokenizer.getToken(&wstrType, 0);
					if (token != MacroTokenizer::TOKEN_TEXT)
						return error(MacroErrorHandler::CODE_INVALIDFIELDNAME, pLast);
					
					MacroFieldCache::Type type = MacroFieldCache::getType(wstrType.get());
					if (type == MacroFieldCache::TYPE_ERROR)
						return error(MacroErrorHandler::CODE_INVALIDFIELDNAME, pLast);
					pExpr.reset(new MacroFieldCache(type));
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
				if (!pMacroExpr.get())
					pMacroExpr.reset(new MacroLiteral(L""));
				bEnd = true;
				break;
			case MacroTokenizer::TOKEN_ERROR:
				return error(MacroErrorHandler::CODE_SYNTAXERROR,
					tokenizer.getLastPosition());
			}
			
			if (pExpr.get()) {
				if (stackFunction.empty()) {
					assert(!pMacroExpr.get());
					pMacroExpr = pExpr;
				}
				else {
					stackFunction.back()->addArg(pExpr);
				}
			}
		}
	}
	
	assert(pMacroExpr.get());
	
	return std::auto_ptr<Macro>(new Macro(pMacroExpr));
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

std::auto_ptr<Macro> qm::MacroParser::error(MacroErrorHandler::Code code,
											const WCHAR* p) const
{
	if (pErrorHandler_)
		pErrorHandler_->parseError(code, p);
	
	
	Log log(InitThread::getInitThread().getLogger(), L"qm::MacroParser");
	if (log.isErrorEnabled()) {
		WCHAR wsz[128];
		swprintf(wsz, L"Error occured while parsing macro: code=%u at ", code);
		wstring_ptr wstrLog(concat(wsz, p));
		log.error(wstrLog.get());
	}
	
	return std::auto_ptr<Macro>(0);
}


/****************************************************************************
 *
 * MacroVariableHolderImpl
 *
 */

struct qm::MacroVariableHolderImpl
{
public:
	typedef std::vector<std::pair<WSTRING, MacroValue*> > VariableMap;

public:
	VariableMap mapVariable_;
};


/****************************************************************************
 *
 * MacroVariableHolder
 *
 */

qm::MacroVariableHolder::MacroVariableHolder() :
	pImpl_(0)
{
	pImpl_ = new MacroVariableHolderImpl();
}

qm::MacroVariableHolder::~MacroVariableHolder()
{
	if (pImpl_) {
		for (MacroVariableHolderImpl::VariableMap::iterator it = pImpl_->mapVariable_.begin(); it != pImpl_->mapVariable_.end(); ++it) {
			freeWString((*it).first);
			MacroValuePtr pValue((*it).second);
		}
		delete pImpl_;
	}
}

MacroValuePtr qm::MacroVariableHolder::getVariable(const WCHAR* pwszName) const
{
	assert(pwszName);
	
	typedef MacroVariableHolderImpl::VariableMap Map;
	Map::const_iterator it = std::find_if(
		pImpl_->mapVariable_.begin(), pImpl_->mapVariable_.end(),
		std::bind2nd(
			binary_compose_f_gx_hy(
				string_equal<WCHAR>(),
				std::select1st<Map::value_type>(),
				std::identity<const WCHAR*>()),
			pwszName));
	if (it != pImpl_->mapVariable_.end())
		return (*it).second->clone();
	else
		return MacroValuePtr();
}

void qm::MacroVariableHolder::setVariable(const WCHAR* pwszName,
										  MacroValuePtr pValue)
{
	assert(pwszName);
	assert(pValue.get());
	
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
		MacroValuePtr pValueOld((*it).second);
		(*it).second = pValue.release();
	}
	else {
		wstring_ptr wstrName(allocWString(pwszName));
		pImpl_->mapVariable_.push_back(std::make_pair(wstrName.get(), pValue.get()));
		wstrName.release();
		pValue.release();
	}
}

void qm::MacroVariableHolder::removeVariable(const WCHAR* pwszName)
{
	assert(pwszName);
	
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
		freeWString((*it).first);
		MacroValuePtr pValue((*it).second);
		pImpl_->mapVariable_.erase(it);
	}
}


/****************************************************************************
 *
 * MacroContext
 *
 */

qm::MacroContext::MacroContext(MessageHolderBase* pmh,
							   Message* pMessage,
							   const MessageHolderList& listSelected,
							   Account* pAccount,
							   Document* pDocument,
							   HWND hwnd,
							   Profile* pProfile,
							   bool bGetMessageAsPossible,
							   bool bDecryptVerify,
							   MacroErrorHandler* pErrorHandler,
							   MacroVariableHolder* pGlobalVariable) :
	pmh_(pmh),
	pMessage_(pMessage),
	pAccount_(pAccount),
	pGlobalContext_(0),
	bOwnGlobalContext_(true)
{
	assert((!pmh && !pMessage) || (pmh && pMessage));
	assert(!pmh || pmh->getAccount() == pAccount);
	assert(!pmh || pmh->getAccount()->isLocked());
	assert(pAccount);
	assert(pDocument);
	assert(hwnd);
	assert(pProfile);
	
	pGlobalContext_ = new MacroGlobalContext(listSelected, pDocument, hwnd, pProfile,
		bGetMessageAsPossible, bDecryptVerify, pErrorHandler, pGlobalVariable);
}

qm::MacroContext::MacroContext(MessageHolderBase* pmh,
							   Message* pMessage,
							   const MacroContext* pContext) :
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

Message* qm::MacroContext::getMessage(MessageType type,
									  const WCHAR* pwszField) const
{
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
		if (!isDecryptVerify())
			nFlags |= Account::GETMESSAGEFLAG_NOSECURITY;
		if (!pmh_->getMessage(nFlags, pwszField, pMessage_))
			return 0;
	}
	
	return pMessage_;
}

const MessageHolderList& qm::MacroContext::getSelectedMessageHolders() const
{
	return pGlobalContext_->getSelectedMessageHolders();
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

bool qm::MacroContext::isDecryptVerify() const
{
	return pGlobalContext_->isDecryptVerify();
}

MacroErrorHandler* qm::MacroContext::getErrorHandler() const
{
	return pGlobalContext_->getErrorHandler();
}

MacroContext::ReturnType qm::MacroContext::getReturnType() const
{
	return pGlobalContext_->getReturnType();
}

void qm::MacroContext::setReturnType(ReturnType type)
{
	pGlobalContext_->setReturnType(type);
}

MacroValuePtr qm::MacroContext::getVariable(const WCHAR* pwszName) const
{
	return pGlobalContext_->getVariable(pwszName);
}

bool qm::MacroContext::setVariable(const WCHAR* pwszName,
								   MacroValue* pValue,
								   bool bGlobal)
{
	return pGlobalContext_->setVariable(pwszName, pValue, bGlobal);
}

const MacroExpr* qm::MacroContext::getFunction(const WCHAR* pwszName) const
{
	return pGlobalContext_->getFunction(pwszName);
}

bool qm::MacroContext::setFunction(const WCHAR* pwszName,
								   const MacroExpr* pExpr)
{
	return pGlobalContext_->setFunction(pwszName, pExpr);
}

void qm::MacroContext::pushArgumentContext()
{
	pGlobalContext_->pushArgumentContext();
}

void qm::MacroContext::popArgumentContext()
{
	pGlobalContext_->popArgumentContext();
}

void qm::MacroContext::addArgument(MacroValuePtr pValue)
{
	pGlobalContext_->addArgument(pValue);
}

MacroValuePtr qm::MacroContext::getArgument(unsigned int n) const
{
	return pGlobalContext_->getArgument(n);
}

bool qm::MacroContext::setRegexResult(const RegexRangeList& listRange)
{
	return pGlobalContext_->setRegexResult(listRange);
}

void qm::MacroContext::clearRegexResult()
{
	pGlobalContext_->clearRegexResult();
}

wstring_ptr qm::MacroContext::resolvePath(const WCHAR* pwszPath)
{
	assert(pwszPath);
	
	wstring_ptr wstrPath;
	
	if (*pwszPath == L'\\' || *pwszPath == L'/' ||
		(*pwszPath != L'\0' && *(pwszPath + 1) == L':')) {
		wstrPath =allocWString(pwszPath);
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
		wstrPath = concat(c, countof(c));
#else
		wstrPath = concat(pwszMailFolder, L"\\", pwszPath);
#endif
	}
	
	return wstrPath;
}
