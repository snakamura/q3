/*
 * $Id$
 *
 * Copyright(C) 1998-2004 Satoshi Nakamura
 * All rights reserved.
 *
 */

#include <qsencoder.h>
#include <qsstl.h>
#include <qstextutil.h>

using namespace qs;


/****************************************************************************
 *
 * TextUtil
 *
 */

wxstring_ptr qs::TextUtil::fold(const WCHAR* pwszText,
								size_t nLen,
								size_t nLineWidth,
								const WCHAR* pwszQuote,
								size_t nQuoteLen,
								size_t nTabWidth)
{
	assert(pwszText);
	
	if (nLen == -1)
		nLen = wcslen(pwszText);
	
	size_t nQuoteWidth = 0;
	for (size_t n = 0; n < nQuoteLen; ++n)
		nQuoteWidth += isHalfWidth(*(pwszQuote + n)) ? 1 : 2;
	
	if (nLineWidth > nQuoteWidth) {
		nLineWidth -= nQuoteWidth;
		if (nLineWidth < 10)
			nLineWidth = 10;
	}
	else {
		nLineWidth = 10;
	}
	
	XStringBuffer<WXSTRING> buf;
	
	size_t nCurrentLen = 0;
	const WCHAR* pLine = pwszText;
	const WCHAR* pBreak = 0;
	const WCHAR* p = pwszText;
	while (p < pwszText + nLen) {
		WCHAR c = *p;
		size_t nCharLen = 1;
		if (c == L'\t')
			nCharLen = nTabWidth - nCurrentLen%nTabWidth;
		else if (!isHalfWidth(c))
			nCharLen = 2;
		
		if (p != pLine && (isBreakSelf(c) || isBreakBefore(c)))
			pBreak = p;
		
		bool bFlushed = true;
		if (c == L'\n') {
			if (!buf.append(pwszQuote, nQuoteLen) ||
				!buf.append(pLine, p - pLine + 1))
				return 0;
			nCurrentLen = 0;
			pLine = p + 1;
			pBreak = 0;
		}
		else if (nCurrentLen + nCharLen > nLineWidth) {
			if (!pBreak) {
				if (pwszQuote) {
					if (!buf.append(pwszQuote, nQuoteLen))
						return 0;
				}
				if (!buf.append(pLine, p - pLine) ||
					!buf.append(L'\n'))
					return 0;
				nCurrentLen = nCharLen;
				pLine = p;
			}
			else {
				WCHAR cBreak = *pBreak;
				const WCHAR* pEnd = pBreak;
				const WCHAR* pNext = pBreak;
				if (isBreakSelf(cBreak)) {
					++pNext;
				}
				else if (isBreakAfter(cBreak) &&
					(pBreak != p || nCurrentLen + nCharLen <= nLineWidth)) {
					++pEnd;
					++pNext;
				}
				else if (isBreakBefore(cBreak)) {
					;
				}
				else {
					assert(false);
				}
				if (pwszQuote) {
					if (!buf.append(pwszQuote, nQuoteLen))
						return 0;
				}
				if (!buf.append(pLine, pEnd - pLine) ||
					!buf.append(L'\n'))
					return 0;
				pLine = pNext;
				nCurrentLen = 0;
				pBreak = 0;
				p = pNext - 1;
			}
		}
		else {
			nCurrentLen += nCharLen;
			bFlushed = false;
		}
		
		if (!bFlushed && isBreakChar(c) && p != pLine)
			pBreak = p;
		
		++p;
	}
	if (p != pLine) {
		if (pwszQuote) {
			if (!buf.append(pwszQuote, nQuoteLen))
				return 0;
		}
		if (!buf.append(pLine, p - pLine))
			return 0;
	}
	
	return buf.getXString();
}

std::pair<size_t, size_t> qs::TextUtil::findURL(const WCHAR* pwszText,
												size_t nLen,
												const WCHAR* const* ppwszSchemas,
												size_t nSchemaCount)
{
	assert(pwszText);
	
	std::pair<size_t, size_t> url(-1, 0);
	if (nSchemaCount != 0) {
		bool bQuote = false;
		bool bEmail = false;
		const WCHAR* p = pwszText;
		while (nLen > 0) {
			if (p == pwszText || !isURLChar(*(p - 1)) || *(p - 1) == L'(') {
				bool bFound = false;
				bQuote = p != pwszText && *(p - 1) == L'(';
				for (size_t n = 0; n < nSchemaCount; ++n) {
					if (ppwszSchemas[n][0] == *p) {
						size_t nSchemaLen = wcslen(ppwszSchemas[n]);
						if (nLen > nSchemaLen &&
							wcsncmp(p, ppwszSchemas[n], nSchemaLen) == 0 &&
							*(p + nSchemaLen) == L':') {
							bFound = true;
							break;
						}
					}
				}
				if (bFound)
					break;
			}
			else if (*p == L'@' &&
				p != pwszText && isURLChar(*(p - 1)) &&
				nLen > 1 && isURLChar(*(p + 1))) {
				while (p >= pwszText && isURLChar(*p) && *p != L'(') {
					--p;
					++nLen;
				}
				++p;
				--nLen;
				bEmail = true;
				break;
			}
			--nLen;
			++p;
		}
		if (nLen > 0) {
			url.first = p - pwszText;
			while (nLen > 0 && isURLChar(*p)) {
				--nLen;
				++p;
			}
			if (p != pwszText + url.first) {
				WCHAR c = *(p - 1);
				if (c == L'.' || c == L',' ||
					(bQuote && c == L')') ||
					(bEmail && (c == L';' || c == L')')))
					--p;
			}
			url.second = p - (pwszText + url.first);
		}
	}
	
	return url;
}

bool qs::TextUtil::isURLChar(WCHAR c)
{
	return (L'A' <= c && c <= L'Z') ||
		(L'a' <= c && c <= L'z') ||
		(L'0' <= c && c <= L'9') ||
		c == L'.' ||
		c == L':' ||
		c == L'/' ||
		c == L'?' ||
		c == L'%' ||
		c == L'&' ||
		c == L'@' ||
		c == L'!' ||
		c == L'#' ||
		c == L'$' ||
		c == L'~' ||
		c == L'*' ||
		c == L'=' ||
		c == L'+' ||
		c == L'-' ||
		c == L'_' ||
		c == L';' ||
		c == L',' ||
		c == L'(' ||
		c == L')';
}

wstring_ptr qs::TextUtil::encodePassword(const WCHAR* pwsz)
{
	assert(pwsz);
	
	Base64Encoder encoder(false);
	malloc_size_ptr<unsigned char> encoded(encoder.encode(
		reinterpret_cast<const unsigned char*>(pwsz),
		wcslen(pwsz)*sizeof(WCHAR)));
	if (!encoded.get())
		return 0;
	
	const unsigned char* p = encoded.get();
	wstring_ptr wstr(allocWString(encoded.size()*2 + 1));
	WCHAR* pDst = wstr.get();
	for (size_t n = 0; n < encoded.size(); ++n) {
		swprintf(pDst, L"%02x", *(p + n) ^ 'q');
		pDst += 2;
	}
	*pDst = L'\0';
	
	return wstr;
}

wstring_ptr qs::TextUtil::decodePassword(const WCHAR* pwsz)
{
	assert(pwsz);
	
	size_t nLen = wcslen(pwsz);
	
	auto_ptr_array<unsigned char> pBuf(new unsigned char[nLen/2 + 1]);
	unsigned char* p = pBuf.get();
	
	for (size_t n = 0; n < nLen; n += 2) {
		WCHAR wsz[3] = L"";
		wcsncpy(wsz, pwsz + n, 2);
		unsigned int m = 0;
		swscanf(wsz, L"%02x", &m);
		*p++ = m ^= 'q';
	}
	
	Base64Encoder encoder(false);
	malloc_size_ptr<unsigned char> decoded(
		encoder.decode(pBuf.get(), p - pBuf.get()));
	if (!decoded.get())
		return 0;
	
	return allocWString(reinterpret_cast<WCHAR*>(decoded.get()),
		decoded.size()/sizeof(WCHAR));
}
