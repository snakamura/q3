/*
 * $Id$
 *
 * Copyright(C) 1998-2003 Satoshi Nakamura
 * All rights reserved.
 *
 */

#ifndef __QSREGEX_H__
#define __QSREGEX_H__

#include <qs.h>

#include <vector>


namespace qs {

struct RegexRange;
class RegexPattern;
class RegexCompiler;

class RegexNfa;


/****************************************************************************
 *
 * RegexRange
 *
 */

struct RegexRange
{
	RegexRange();
	
	const WCHAR* pStart_;
	const WCHAR* pEnd_;
};


/****************************************************************************
 *
 * RegexPattern
 *
 */

class QSEXPORTCLASS RegexPattern
{
public:
	typedef std::vector<RegexRange> RangeList;

public:
	RegexPattern(RegexNfa* pNfa, QSTATUS* pstatus);
	~RegexPattern();

public:
	QSTATUS match(const WCHAR* pwsz, bool* pbMatch) const;
	QSTATUS match(const WCHAR* pwsz, size_t nLen,
		bool* pbMatch, RangeList* pList) const;
	QSTATUS search(const WCHAR* pwsz, size_t nLen, const WCHAR** ppStart,
		const WCHAR** ppEnd, RangeList* pList) const;

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
