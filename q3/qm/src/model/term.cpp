/*
 * $Id$
 *
 * Copyright(C) 1998-2008 Satoshi Nakamura
 *
 */

#include "term.h"

using namespace qm;
using namespace qs;


/****************************************************************************
 *
 * Term
 *
 */

qm::Term::Term()
{
}

qm::Term::Term(const Term& term)
{
	setValue(term.getValue());
}

qm::Term::~Term()
{
}

Term& qm::Term::operator=(const Term& term)
{
	if (&term != this)
		setValue(term.getValue());
	return *this;
}

const WCHAR* qm::Term::getValue() const
{
	return wstrValue_.get();
}

bool qm::Term::setValue(const WCHAR* pwszValue)
{
	std::auto_ptr<RegexPattern> pRegex;
	if (pwszValue) {
		size_t nLen = wcslen(pwszValue);
		if (nLen >= 2 && *pwszValue == L'/' && *(pwszValue + nLen - 1) == L'/') {
			wstring_ptr wstrValue(allocWString(pwszValue + 1, nLen - 2));
			pRegex = RegexCompiler().compile(wstrValue.get());
			if (!pRegex.get())
				return false;
		}
	}
	set(pwszValue, pRegex);
	
	return true;
}

void qm::Term::assign(Term& term)
{
	wstrValue_ = term.wstrValue_;
	pRegex_ = term.pRegex_;
}

bool qm::Term::match(const WCHAR* pwsz) const
{
	assert(pwsz);
	
	if (pRegex_.get())
		return pRegex_->match(pwsz);
	else if (wstrValue_.get())
		return wcscmp(wstrValue_.get(), pwsz) == 0;
	else
		return true;
}

bool qm::Term::isSpecified() const
{
	return wstrValue_.get() != 0;
}

void qm::Term::set(const WCHAR* pwszValue,
				   std::auto_ptr<RegexPattern> pRegex)
{
#ifndef NDEBUG
	if (pRegex.get()) {
		assert(pwszValue);
	}
	else if (!pwszValue) {
		assert(!pRegex.get());
	}
#endif
	
	if (pwszValue)
		wstrValue_ = allocWString(pwszValue);
	else
		wstrValue_.reset(0);
	
	pRegex_ = pRegex;
}
