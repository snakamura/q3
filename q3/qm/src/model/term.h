/*
 * $Id$
 *
 * Copyright(C) 1998-2007 Satoshi Nakamura
 * All rights reserved.
 *
 */

#ifndef __TERM_H__
#define __TERM_H__

#include <qm.h>

#include <qsregex.h>
#include <qsstring.h>


namespace qm {

/****************************************************************************
 *
 * Term
 *
 */

class Term
{
public:
	Term();
	Term(const Term& term);
	~Term();

public:
	Term& operator=(const Term& term);

public:
	const WCHAR* getValue() const;
	bool setValue(const WCHAR* pwszValue);
	void assign(Term& term);
	bool match(const WCHAR* pwsz) const;
	bool isSpecified() const;

private:
	void set(const WCHAR* pwszValue,
			 std::auto_ptr<qs::RegexPattern> pRegex);

private:
	qs::wstring_ptr wstrValue_;
	std::auto_ptr<qs::RegexPattern> pRegex_;
};

}

#endif // __TERM_H__
