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
 * RegexRangeList
 *
 */

QSTATUS qs::RegexRangeList::getReplace(const WCHAR* pwszReplace, WSTRING* pwstr) const
{
	assert(pwszReplace);
	assert(pwstr);
	
	DECLARE_QSTATUS();
	
	*pwstr = 0;
	
	StringBuffer<WSTRING> buf(&status);
	CHECK_QSTATUS();
	status = getReplace(pwszReplace, &buf);
	CHECK_QSTATUS();
	
	*pwstr = buf.getString();
	
	return QSTATUS_SUCCESS;
}

QSTATUS qs::RegexRangeList::getReplace(const WCHAR* pwszReplace, StringBuffer<WSTRING>* pBuf) const
{
	DECLARE_QSTATUS();
	
	for (const WCHAR* p = pwszReplace; *p; ++p) {
		if (*p == L'$') {
			WCHAR c = *(p + 1);
			if (L'0' <= c && c <= L'9') {
				unsigned int nIndex = c - L'0';
				if (nIndex < list_.size() && list_[nIndex].pStart_) {
					const RegexRange& range = list_[nIndex];
					status = pBuf->append(range.pStart_, range.pEnd_ - range.pStart_);
					CHECK_QSTATUS();
					++p;
				}
				else {
					status = pBuf->append(*p);
					CHECK_QSTATUS();
				}
			}
			else if (c == L'$') {
				status = pBuf->append(c);
				CHECK_QSTATUS();
				++p;
			}
			else {
				status = pBuf->append(L'$');
				CHECK_QSTATUS();
			}
		}
		else {
			status = pBuf->append(*p);
			CHECK_QSTATUS();
		}
	}
	
	return QSTATUS_SUCCESS;
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
	size_t nLen, bool* pbMatch, RegexRangeList* pList) const
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

QSTATUS qs::RegexPattern::search(const WCHAR* pwsz, size_t nLen,
	const WCHAR* p, bool bReverse, const WCHAR** ppStart,
	const WCHAR** ppEnd, RegexRangeList* pList) const
{
	assert(pwsz);
	assert(ppStart);
	assert(ppEnd);
	
	DECLARE_QSTATUS();
	
	if (nLen == -1)
		nLen = wcslen(pwsz);
	
	RegexNfaMatcher matcher(pImpl_->pNfa_);
	status = matcher.search(pwsz, nLen, p, bReverse, ppStart, ppEnd, pList);
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
