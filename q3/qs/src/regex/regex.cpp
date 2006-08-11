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

wstring_ptr qs::RegexRange::getString() const
{
	if (!pStart_)
		return 0;
	assert(pEnd_);
	return allocWString(pStart_, pEnd_ - pStart_);
}

int qs::RegexRange::getInt() const
{
	wstring_ptr wstr(getString());
	WCHAR* pEnd = 0;
	long n = wcstol(wstr.get(), &pEnd, 10);
	if (*pEnd)
		return 0;
	return n;
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


/****************************************************************************
 *
 * Regex
 *
 */

bool qs::Regex::match(const WCHAR* pwszRegex,
					  const WCHAR* pwsz)
{
	return match(pwszRegex, pwsz, -1, 0);
}

bool qs::Regex::match(const WCHAR* pwszRegex,
					  const WCHAR* pwsz,
					  size_t nLen)
{
	return match(pwszRegex, pwsz, nLen, 0);
}

bool qs::Regex::match(const WCHAR* pwszRegex,
					  const WCHAR* pwsz,
					  RegexRangeList* pList)
{
	return match(pwszRegex, pwsz, -1, pList);
}

bool qs::Regex::match(const WCHAR* pwszRegex,
					  const WCHAR* pwsz,
					  size_t nLen,
					  RegexRangeList* pList)
{
	std::auto_ptr<RegexPattern> pPattern(RegexCompiler().compile(pwszRegex));
	if (!pPattern.get())
		return false;
	return pPattern->match(pwsz, nLen, pList);
}

std::pair<const WCHAR*, const WCHAR*> qs::Regex::search(const WCHAR* pwszRegex,
														const WCHAR* pwsz)
{
	return search(pwszRegex, pwsz, -1, pwsz, false, 0);
}

std::pair<const WCHAR*, const WCHAR*> qs::Regex::search(const WCHAR* pwszRegex,
														const WCHAR* pwsz,
														size_t nLen)
{
	return search(pwszRegex, pwsz, nLen, pwsz, false, 0);
}

std::pair<const WCHAR*, const WCHAR*> qs::Regex::search(const WCHAR* pwszRegex,
														const WCHAR* pwsz,
														size_t nLen,
														const WCHAR* p,
														bool bReverse,
														RegexRangeList* pList)
{
	std::pair<const WCHAR*, const WCHAR*> r(0, 0);
	std::auto_ptr<RegexPattern> pPattern(RegexCompiler().compile(pwszRegex));
	if (pPattern.get())
		pPattern->search(pwsz, nLen, p, bReverse, &r.first, &r.second, pList);
	return r;
}
