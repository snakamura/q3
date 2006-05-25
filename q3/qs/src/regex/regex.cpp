/*
 * $Id$
 *
 * Copyright(C) 1998-2006 Satoshi Nakamura
 * All rights reserved.
 *
 */

#pragma warning(disable:4786)

#include <qsassert.h>
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

wstring_ptr qs::RegexRangeList::getReplace(const WCHAR* pwszReplace) const
{
	assert(pwszReplace);
	
	StringBuffer<WSTRING> buf;
	getReplace(pwszReplace, &buf);
	
	return buf.getString();
}

void qs::RegexRangeList::getReplace(const WCHAR* pwszReplace,
									StringBuffer<WSTRING>* pBuf) const
{
	for (const WCHAR* p = pwszReplace; *p; ++p) {
		if (*p == L'$') {
			WCHAR c = *(p + 1);
			if (L'0' <= c && c <= L'9') {
				unsigned int nIndex = c - L'0';
				if (nIndex < list_.size() && list_[nIndex].pStart_) {
					const RegexRange& range = list_[nIndex];
					pBuf->append(range.pStart_, range.pEnd_ - range.pStart_);
					++p;
				}
				else {
					pBuf->append(*p);
				}
			}
			else if (c == L'$') {
				pBuf->append(c);
				++p;
			}
			else {
				pBuf->append(L'$');
			}
		}
		else {
			pBuf->append(*p);
		}
	}
}


/****************************************************************************
 *
 * RegexPatternImpl
 *
 */

struct qs::RegexPatternImpl
{
	std::auto_ptr<RegexNfa> pNfa_;
};


/****************************************************************************
 *
 * RegexPattern
 *
 */

qs::RegexPattern::RegexPattern(std::auto_ptr<RegexNfa> pNfa) :
	pImpl_(0)
{
	pImpl_ = new RegexPatternImpl();
	pImpl_->pNfa_ = pNfa;
}

qs::RegexPattern::~RegexPattern()
{
	delete pImpl_;
}

bool qs::RegexPattern::match(const WCHAR* pwsz) const
{
	return match(pwsz, -1, 0);
}

bool qs::RegexPattern::match(const WCHAR* pwsz,
							 size_t nLen,
							 RegexRangeList* pList) const
{
	assert(pwsz);
	
	if (nLen == -1)
		nLen = wcslen(pwsz);
	
	RegexNfaMatcher matcher(pImpl_->pNfa_.get());
	return matcher.match(pwsz, nLen, pList);
}

void qs::RegexPattern::search(const WCHAR* pwsz,
							  size_t nLen,
							  const WCHAR* p,
							  bool bReverse,
							  const WCHAR** ppStart,
							  const WCHAR** ppEnd,
							  RegexRangeList* pList) const
{
	assert(pwsz);
	assert(ppStart);
	assert(ppEnd);
	
	if (nLen == -1)
		nLen = wcslen(pwsz);
	
	RegexNfaMatcher matcher(pImpl_->pNfa_.get());
	matcher.search(pwsz, nLen, p, bReverse, ppStart, ppEnd, pList);
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

std::auto_ptr<RegexPattern> qs::RegexCompiler::compile(const WCHAR* pwszPattern) const
{
	return compile(pwszPattern, 0);
}

std::auto_ptr<RegexPattern> qs::RegexCompiler::compile(const WCHAR* pwszPattern,
													   unsigned int nMode) const
{
	assert(pwszPattern);
	
	RegexParser parser(pwszPattern, nMode);
	std::auto_ptr<RegexRegexNode> pNode(parser.parse());
	if (!pNode.get())
		return std::auto_ptr<RegexPattern>(0);
	
	RegexNfaCompiler compiler;
	std::auto_ptr<RegexNfa> pNfa(compiler.compile(pNode));
	
	return std::auto_ptr<RegexPattern>(new RegexPattern(pNfa));
}
