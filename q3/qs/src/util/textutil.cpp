/*
 * $Id$
 *
 * Copyright(C) 1998-2003 Satoshi Nakamura
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

QSTATUS qs::TextUtil::encodePassword(const WCHAR* pwsz, WSTRING* pwstr)
{
	assert(pwsz);
	assert(pwstr);
	
	DECLARE_QSTATUS();
	
	Base64Encoder encoder(false, &status);
	CHECK_QSTATUS();
	unsigned char* p = 0;
	size_t nLen = 0;
	status = encoder.encode(reinterpret_cast<const unsigned char*>(pwsz),
		wcslen(pwsz)*sizeof(WCHAR), &p, &nLen);
	CHECK_QSTATUS();
	malloc_ptr<unsigned char> pBuffer(p);
	
	string_ptr<WSTRING> wstr(allocWString(nLen*2 + 1));
	if (!wstr.get())
		return QSTATUS_OUTOFMEMORY;
	
	WCHAR* pDst = wstr.get();
	for (size_t n = 0; n < nLen; ++n) {
		swprintf(pDst, L"%02x", *(p + n) ^ 'q');
		pDst += 2;
	}
	*pDst = L'\0';
	
	*pwstr = wstr.release();
	
	return QSTATUS_SUCCESS;
}

QSTATUS qs::TextUtil::decodePassword(const WCHAR* pwsz, WSTRING* pwstr)
{
	assert(pwsz);
	assert(pwstr);
	
	DECLARE_QSTATUS();
	
	size_t nLen = wcslen(pwsz);
	
	malloc_ptr<unsigned char> pBuf(
		static_cast<unsigned char*>(malloc(nLen/2 + 1)));
	if (!pBuf.get())
		return QSTATUS_OUTOFMEMORY;
	unsigned char* p = pBuf.get();
	
	for (size_t n = 0; n < nLen; n += 2) {
		WCHAR wsz[3] = L"";
		wcsncpy(wsz, pwsz + n, 2);
		unsigned int m = 0;
		swscanf(wsz, L"%02x", &m);
		*p++ = m ^= 'q';
	}
	
	Base64Encoder encoder(false, &status);
	CHECK_QSTATUS();
	unsigned char* pDecode = 0;
	size_t nDecodeLen = 0;
	status = encoder.decode(pBuf.get(), p - pBuf.get(), &pDecode, &nDecodeLen);
	CHECK_QSTATUS();
	
	string_ptr<WSTRING> wstr(allocWString(
		reinterpret_cast<WCHAR*>(pDecode), nDecodeLen/sizeof(WCHAR)));
	if (!wstr.get())
		return QSTATUS_OUTOFMEMORY;
	
	*pwstr = wstr.release();
	
	return QSTATUS_SUCCESS;
}
