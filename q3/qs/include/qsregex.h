/*
 * $Id: qsregex.h,v 1.1.1.1 2003/04/29 08:07:35 snakamura Exp $
 *
 * Copyright(C) 1998-2003 Satoshi Nakamura
 * All rights reserved.
 *
 */

#ifndef __QSREGEX_H__
#define __QSREGEX_H__

#include <qs.h>


namespace qs {

class RegexPattern;
class RegexCompiler;

class RegexNfa;


/****************************************************************************
 *
 * RegexPattern
 *
 */

class QSEXPORTCLASS RegexPattern
{
public:
	RegexPattern(RegexNfa* pNfa, QSTATUS* pstatus);
	~RegexPattern();

public:
	QSTATUS match(const WCHAR* pwsz, bool* pbMatch) const;

private:
	RegexPattern(const RegexPattern&);
	RegexPattern& operator=(const RegexPattern&);

private:
	struct RegexPatternImpl* pImpl_;
};


/****************************************************************************
 *
 * RegexCompiler
 *
 */

class QSEXPORTCLASS RegexCompiler
{
public:
	RegexCompiler();
	~RegexCompiler();

public:
	QSTATUS compile(const WCHAR* pwszPattern, RegexPattern** ppPattern) const;

private:
	RegexCompiler(const RegexCompiler&);
	RegexCompiler& operator=(const RegexCompiler&);
};

}

#endif // __QSREGEX_H__
