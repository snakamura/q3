/*
 * $Id: textutil.cpp,v 1.1.1.1 2003/04/29 08:07:37 snakamura Exp $
 *
 * Copyright(C) 1998-2003 Satoshi Nakamura
 * All rights reserved.
 *
 */

#include <qstextutil.h>

using namespace qs;


/****************************************************************************
 *
 * TextUtil
 *
 */

QSTATUS qs::TextUtil::fold(const WCHAR* pwszText, size_t nLen,
	size_t nLineWidth, const WCHAR* pwszQuote, size_t nQuoteLen,
	size_t nTabWidth, WSTRING* pwstrText)
{
	assert(pwszText);
	assert(pwstrText);
	
	DECLARE_QSTATUS();
	
	*pwstrText = 0;
	
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
	
	StringBuffer<WSTRING> buf(&status);
	CHECK_QSTATUS();
	
	size_t nCurrentLen = 0;
	const WCHAR* pLine = pwszText;
	const WCHAR* pBreak = 0;
	for (const WCHAR* p = pwszText; p < pwszText + nLen; ++p) {
		WCHAR c = *p;
		size_t nCharLen = 1;
		if (c == L'\t')
			nCharLen = nTabWidth - nCurrentLen%nTabWidth;
		else if (!isHalfWidth(c))
			nCharLen = 2;
		
		if (isBreakSelf(c))
			pBreak = p;
		
		bool bFlushed = true;
		if (c == L'\n') {
			status = buf.append(pwszQuote, nQuoteLen);
			CHECK_QSTATUS();
			status = buf.append(pLine, p - pLine + 1);
			CHECK_QSTATUS();
			nCurrentLen = 0;
			pLine = p + 1;
			pBreak = 0;
		}
		else if (nCurrentLen + nCharLen > nLineWidth) {
			if (!pBreak) {
				if (pwszQuote) {
					status = buf.append(pwszQuote, nQuoteLen);
					CHECK_QSTATUS();
				}
				status = buf.append(pLine, p - pLine);
				CHECK_QSTATUS();
				status = buf.append(L'\n');
				CHECK_QSTATUS();
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
				else if (isBreakBefore(cBreak)) {
					;
				}
				else if (isBreakAfter(cBreak)) {
					++pEnd;
					++pNext;
				}
				else {
					assert(false);
				}
				if (pwszQuote) {
					status = buf.append(pwszQuote, nQuoteLen);
					CHECK_QSTATUS();
				}
				status = buf.append(pLine, pEnd - pLine);
				CHECK_QSTATUS();
				status = buf.append(L'\n');
				CHECK_QSTATUS();
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
	}
	if (p != pLine) {
		if (pwszQuote) {
			status = buf.append(pwszQuote, nQuoteLen);
			CHECK_QSTATUS();
		}
		status = buf.append(pLine, p - pLine);
		CHECK_QSTATUS();
	}
	
	*pwstrText = buf.getString();
	
	return QSTATUS_SUCCESS;
}

std::pair<size_t, size_t> qs::TextUtil::findURL(const WCHAR* pwszText,
	size_t nLen, const WCHAR* const* ppwszSchemas, size_t nSchemaCount)
{
	assert(pwszText);
	
	std::pair<size_t, size_t> url(-1, 0);
	if (nSchemaCount != 0) {
		const WCHAR* p = pwszText;
		while (nLen > 0) {
			if (p == pwszText || !isURLChar(*(p - 1))) {
				bool bFound = false;
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
				while (p >= pwszText && isURLChar(*p)) {
					--p;
					++nLen;
				}
				++p;
				--nLen;
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
