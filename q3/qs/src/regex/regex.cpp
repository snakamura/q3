/*
 * $Id$
 *
 * Copyright(C) 1998-2003 Satoshi Nakamura
 * All rights reserved.
 *
 */

#include <qsassert.h>
#include <qserror.h>
#include <qsnew.h>
#include <qsregex.h>

#include "regexnfa.h"
#include "regexparser.h"

using namespace qs;


/****************************************************************************
 *
 * RegexRange
 *
 */

qs::RegexRange::RegexRange() :
	pStart_(0),
	pEnd_(0)
{
}


/****************************************************************************
 *
 * RegexPatternImpl
 *
 */

struct qs::RegexPatternImpl
{
	RegexNfa* pNfa_;
};


/****************************************************************************
 *
 * RegexPattern
 *
 */

qs::RegexPattern::RegexPattern(RegexNfa* pNfa, QSTATUS* pstatus) :
	pImpl_(0)
{
	assert(pstatus);
	
	DECLARE_QSTATUS();
	
	status = newObject(&pImpl_);
	CHECK_QSTATUS_SET(pstatus);
	pImpl_->pNfa_ = pNfa;
}

qs::RegexPattern::~RegexPattern()
{
	if (pImpl_) {
		delete pImpl_->pNfa_;
		delete pImpl_;
	}
}

QSTATUS qs::RegexPattern::match(const WCHAR* pwsz, bool* pbMatch) const
{
	return match(pwsz, -1, pbMatch, 0);
}

QSTATUS qs::RegexPattern::match(const WCHAR* pwsz,
	size_t nLen, bool* pbMatch, RangeList* pList) const
{
	assert(pwsz);
	assert(pbMatch);
	
	DECLARE_QSTATUS();
	
	if (nLen == -1)
		nLen = wcslen(pwsz);
	
	RegexNfaMatcher matcher(pImpl_->pNfa_);
	status = matcher.match(pwsz, nLen, pbMatch, pList);
	CHECK_QSTATUS();
	
	return QSTATUS_SUCCESS;
}


/****************************************************************************
 *
 * RegexCompiler
 *
 */

qs::RegexCompiler::RegexCompiler()
{
}

qs::RegexCompiler::~RegexCompiler()
{
}

QSTATUS qs::RegexCompiler::compile(
	const WCHAR* pwszPattern, RegexPattern** ppPattern) const
{
	assert(pwszPattern);
	assert(ppPattern);
	
	DECLARE_QSTATUS();
	
	*ppPattern = 0;
	
	RegexParser parser(pwszPattern, &status);
	CHECK_QSTATUS();
	RegexRegexNode* pNode = 0;
	status = parser.parse(&pNode);
	CHECK_QSTATUS();
	std::auto_ptr<RegexRegexNode> apNode(pNode);
	
	RegexNfaCompiler compiler(&status);
	CHECK_QSTATUS();
	RegexNfa* pNfa = 0;
	status = compiler.compile(apNode.get(), &pNfa);
	CHECK_QSTATUS();
	apNode.release();
	std::auto_ptr<RegexNfa> apNfa(pNfa);
	
	std::auto_ptr<RegexPattern> pPattern;
	status = newQsObject(apNfa.get(), &pPattern);
	CHECK_QSTATUS();
	apNfa.release();
	
	*ppPattern = pPattern.release();
	
	return QSTATUS_SUCCESS;
}
