/*
 * $Id$
 *
 * Copyright(C) 1998-2006 Satoshi Nakamura
 * All rights reserved.
 *
 */

#ifndef __QSMIME_INL__
#define __QSMIME_INL__


/****************************************************************************
 *
 * FieldParserUtil
 *
 */

template<class String>
bool qs::FieldParserUtil<String>::isAscii(const Char* psz)
{
	return isAscii(psz, -1);
}

template<class String>
bool qs::FieldParserUtil<String>::isAscii(const Char* psz,
										  size_t nLen)
{
	assert(psz);
	
	if (nLen == -1)
		nLen = CharTraits<Char>::getLength(psz);
	
	const Char* pEnd = psz + nLen;
	const Char* p = std::find_if(psz, pEnd,
		std::bind2nd(std::greater<Char>(), 0x7f));
	return p == pEnd;
}

template<class String>
qs::basic_string_ptr<String> qs::FieldParserUtil<String>::getQString(const Char* psz,
																	 size_t nLen)
{
	assert(psz);
	
	if (nLen == -1)
		nLen = CharTraits<Char>::getLength(psz);
	
	StringBuffer<String> buf(nLen + 2);
	buf.append(Char('\"'));
	for (const Char* p = psz; p < psz + nLen; ++p) {
		if (*p == Char('\\') || *p == Char('\"'))
			buf.append(Char('\\'));
		buf.append(*p);
	}
	buf.append(Char('\"'));
	
	return buf.getString();
}

template<class String>
qs::basic_string_ptr<String> qs::FieldParserUtil<String>::getAtomOrQString(const Char* psz,
																		   size_t nLen)
{
	assert(psz);
	
	if (nLen == -1)
		nLen = CharTraits<Char>::getLength(psz);
	
	if (isNeedQuote(psz, nLen, true))
		return getQString(psz, nLen);
	else
		return StringTraits<String>::allocString(psz, nLen);
}

template<class String>
qs::basic_string_ptr<String> qs::FieldParserUtil<String>::getAtomsOrQString(const Char* psz,
																			size_t nLen)
{
	assert(psz);
	
	if (nLen == -1)
		nLen = CharTraits<Char>::getLength(psz);
	
	if (isNeedQuote(psz, nLen, false))
		return getQString(psz, nLen);
	else
		return StringTraits<String>::allocString(psz, nLen);
}

template<class String>
bool qs::FieldParserUtil<String>::isNeedQuote(const Char* psz,
											  size_t nLen,
											  bool bQuoteWhitespace)
{
	assert(psz);
	
	if (nLen == -1)
		nLen = CharTraits<Char>::getLength(psz);
	if (nLen == 0)
		return true;
	
	const Char* p = psz;
	while (p < psz + nLen) {
		if (unsigned int(*p) < 0x80) {
			if (FieldParser::isSpecial(char(*p)) ||
				(bQuoteWhitespace && (*p == Char(' ') || *p == Char('\t'))))
				break;
		}
		++p;
	}
	return p != psz + nLen;
}

template<class String>
qs::basic_string_ptr<String> qs::FieldParserUtil<String>::resolveQuotedPairs(const Char* psz,
																			 size_t nLen)
{
	assert(psz);
	
	if (nLen == -1)
		nLen = CharTraits<Char>::getLength(psz);
	
	StringBuffer<String> buf;
	
	bool bQuoted = false;
	for (size_t n = 0; n < nLen; ++n, ++psz) {
		if (!bQuoted && *psz == Char('\\')) {
			bQuoted = true;
		}
		else {
			buf.append(*psz);
			bQuoted = false;
		}
	}
	
	return buf.getString();
}


/****************************************************************************
 *
 * BoundaryFinder
 *
 */

template<class Char, class String>
qs::BoundaryFinder<Char, String>::BoundaryFinder(const Char* pszMessage,
												 size_t nLen,
												 const Char* pszBoundary,
												 const Char* pszNewLine,
												 bool bAllowIncomplete) :
	p_(pszMessage),
	nLen_(nLen),
	nBoundaryLen_(0),
	pszNewLine_(pszNewLine),
	bAllowIncomplete_(bAllowIncomplete),
	pPreamble_(0),
	nPreambleLen_(0),
	pEpilogue_(0),
	nEpilogueLen_(0)
{
	assert(CharTraits<Char>::compare(pszMessage, pszNewLine_,
		CharTraits<Char>::getLength(pszNewLine_)) == 0);
	
	size_t nNewLineLen = CharTraits<Char>::getLength(pszNewLine_);
	size_t nBoundaryLen = CharTraits<Char>::getLength(pszBoundary);
	nBoundaryLen_ = nBoundaryLen + nNewLineLen + 2;
	basic_string_ptr<String> strBoundary(
		StringTraits<String>::allocString(nBoundaryLen_ + 1));
	Char* p = strBoundary.get();
	memcpy(p, pszNewLine_, nNewLineLen*sizeof(Char));
	p += nNewLineLen;
	*p++ = '-';
	*p++ = '-';
	memcpy(p, pszBoundary, nBoundaryLen*sizeof(Char));
	p += nBoundaryLen;
	*p = 0;
	
	pFindString_.reset(new BMFindString<String>(strBoundary.get(), nBoundaryLen_));
	
	const Char* pBegin = 0;
	const Char* pEnd = 0;
	bool bEnd = false;
	getNextBoundary(pszMessage, nLen, &pBegin, &pEnd, &bEnd);
	if (!pBegin || pBegin != pszMessage) {
		pPreamble_ = pszMessage + nNewLineLen;
		nPreambleLen_ = (pBegin ? pBegin - pszMessage : nLen) - nNewLineLen;
	}
	p_ = bEnd ? 0 : pEnd;
	nLen_ = p_ ? nLen - (p_ - pszMessage) : 0;
}

template<class Char, class String>
qs::BoundaryFinder<Char, String>::~BoundaryFinder()
{
}

template<class Char, class String>
bool qs::BoundaryFinder<Char, String>::getNext(const Char** ppBegin,
											   const Char** ppEnd,
											   bool* pbEnd)
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
		bool bEnd = false;
		getNextBoundary(p_, nLen_, &pBegin, &pEnd, &bEnd);
		if (pBegin) {
			*ppBegin = p_;
			*ppEnd = pBegin;
			*pbEnd = bEnd;
			
			assert(pEnd <= p_ + nLen_);
			nLen_ -= pEnd - p_;
			p_ = pEnd;
			
			if (bEnd) {
				size_t nNewLineLen = CharTraits<Char>::getLength(pszNewLine_);
				assert(nLen_ == 0 || CharTraits<Char>::compare(pEnd, pszNewLine_, nNewLineLen) == 0);
				if (nLen_ != 0) {
					pEpilogue_ = pEnd + nNewLineLen;
					nEpilogueLen_ = nLen_ - nNewLineLen;
				}
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
				return false;
			}
		}
	}
	else {
		*pbEnd = true;
	}
	
	return true;
}

template<class Char, class String>
std::pair<const Char*, size_t> qs::BoundaryFinder<Char, String>::getPreamble() const
{
	return std::make_pair(pPreamble_, nPreambleLen_);
}

template<class Char, class String>
std::pair<const Char*, size_t> qs::BoundaryFinder<Char, String>::getEpilogue() const
{
	return std::make_pair(pEpilogue_, nEpilogueLen_);
}

template<class Char, class String>
void qs::BoundaryFinder<Char, String>::getNextBoundary(const Char* p,
													   size_t nLen,
													   const Char** ppBegin,
													   const Char** ppEnd,
													   bool* pbEnd)
{
	assert(p);
	assert(ppBegin);
	assert(ppEnd);
	assert(pbEnd);
	
	*ppBegin = 0;
	*ppEnd = 0;
	*pbEnd = false;
	
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
		
		if ((bEnd && pEnd == p + nLen) ||
			(pEnd + nNewLineLen <= p + nLen &&
			CharTraits<Char>::compare(pEnd, pszNewLine_, nNewLineLen) == 0)) {
			*ppBegin = pBegin;
			*ppEnd = pEnd + (bEnd ? 0 : nNewLineLen);
			*pbEnd = bEnd;
		}
		
		p = pBegin + 1;
	}
}

#endif // __QSMIME_INL__
