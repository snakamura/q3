/*
 * $Id$
 *
 * Copyright(C) 1998-2003 Satoshi Nakamura
 * All rights reserved.
 *
 */

#ifndef __QSMIME_INL__
#define __QSMIME_INL__


/****************************************************************************
 *
 * BoundaryFinder
 *
 */

template<class Char, class String>
qs::BoundaryFinder<Char, String>::BoundaryFinder(const Char* pszMessage,
	size_t nLen, const Char* pszBoundary, const Char* pszNewLine,
	bool bAllowIncomplete, QSTATUS* pstatus) :
	p_(pszMessage),
	nLen_(nLen),
	pFindString_(0),
	nBoundaryLen_(0),
	pszNewLine_(pszNewLine),
	bAllowIncomplete_(bAllowIncomplete)
{
	assert(CharTraits<Char>::compare(pszMessage, pszNewLine_,
		CharTraits<Char>::getLength(pszNewLine_)) == 0);
	
	DECLARE_QSTATUS();
	
	size_t nNewLineLen = CharTraits<Char>::getLength(pszNewLine_);
	size_t nBoundaryLen = CharTraits<Char>::getLength(pszBoundary);
	nBoundaryLen_ = nBoundaryLen + nNewLineLen + 2;
	string_ptr<String> strBoundary(
		StringTraits<String>::allocString(nBoundaryLen_ + 1));
	if (!strBoundary.get()) {
		*pstatus = QSTATUS_OUTOFMEMORY;
		return;
	}
	Char* p = strBoundary.get();
	memcpy(p, pszNewLine_, nNewLineLen*sizeof(Char));
	p += nNewLineLen;
	*p++ = '-';
	*p++ = '-';
	memcpy(p, pszBoundary, nBoundaryLen*sizeof(Char));
	p += nBoundaryLen;
	*p = 0;
	
	status = newQsObject(strBoundary.get(),
		nBoundaryLen_, &pFindString_);
	CHECK_QSTATUS_SET(pstatus);
	
	const Char* pBegin = 0;
	const Char* pEnd = 0;
	getNextBoundary(pszMessage, nLen, &pBegin, &pEnd);
	p_ = pEnd;
	nLen_ = p_ ? nLen - (p_ - pszMessage) : 0;
}

template<class Char, class String>
qs::BoundaryFinder<Char, String>::~BoundaryFinder()
{
	delete pFindString_;
}

template<class Char, class String>
qs::QSTATUS qs::BoundaryFinder<Char, String>::getNext(
	const Char** ppBegin, const Char** ppEnd, bool* pbEnd)
{
	assert(ppBegin);
	assert(ppEnd);
	assert(pbEnd);
	
	*ppBegin = 0;
	*ppEnd = 0;
	*pbEnd = false;
	
	if (p_) {
		const Char* pBegin = 0;
		const Char* pEnd = 0;
		getNextBoundary(p_, nLen_, &pBegin, &pEnd);
		if (pBegin) {
			*ppBegin = p_;
			*ppEnd = pBegin;
			*pbEnd = pEnd == 0;
			
			if (pEnd) {
				assert(pEnd <= p_ + nLen_);
				nLen_ -= pEnd - p_;
				p_ = pEnd;
			}
			else {
				nLen_ = 0;
				p_ = 0;
			}
		}
		else {
			if (bAllowIncomplete_) {
				*ppBegin = p_;
				*ppEnd = p_ + nLen_;
				*pbEnd = true;
				
				nLen_ = 0;
				p_ = 0;
			}
			else {
				return QSTATUS_FAIL;
			}
		}
	}
	else {
		*pbEnd = true;
	}
	
	return QSTATUS_SUCCESS;
}

template<class Char, class String>
void qs::BoundaryFinder<Char, String>::getNextBoundary(
	const Char* p, size_t nLen, const Char** ppBegin, const Char** ppEnd)
{
	assert(p);
	assert(ppBegin);
	assert(ppEnd);
	
	*ppBegin = 0;
	*ppEnd = 0;
	
	size_t nNewLineLen = CharTraits<Char>::getLength(pszNewLine_);
	
	while (!*ppBegin) {
		const Char* pBegin = pFindString_->find(p, nLen);
		if (!pBegin)
			break;
		
		const Char* pEnd = pBegin + nBoundaryLen_;
		bool bEnd = pEnd + 1 < p + nLen && *pEnd == '-' && *(pEnd + 1) == '-';
		if (bEnd)
			pEnd += 2;
		while (pEnd < p + nLen && (*pEnd == ' ' || *pEnd == '\t'))
			++pEnd;
		
		if (bEnd) {
			*ppBegin = pBegin;
		}
		else if (pEnd + nNewLineLen <= p + nLen &&
			CharTraits<Char>::compare(pEnd, pszNewLine_, nNewLineLen) == 0) {
			*ppBegin = pBegin;
			*ppEnd = pEnd + nNewLineLen;
		}
		
		p = pBegin + 1;
	}
}

#endif // __QSMIME_INL__
