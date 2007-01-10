/*
 * $Id$
 *
 * Copyright(C) 1998-2006 Satoshi Nakamura
 * All rights reserved.
 *
 */

#include <qsconv.h>
#include <qsencoder.h>
#include <qsstl.h>
#include <qstextutil.h>

using namespace qs;


/****************************************************************************
 *
 * TextUtil
 *
 */

const WCHAR qs::TextUtil::wszLineStartProhibited__[] = {
	L'!',
	L')',
	L',',
	L'.',
	L':',
	L';',
	L'?',
	L']',
	L'}',
	0x3001,	// Ideographic Comma
	0x3002,	// Ideographic Full Stop
	0xff0c,	// Fullwidth Comma
	0xff0e,	// Fullwidth Full Stop
	0x30fb,	// Katakana Middle Dot
	0xff1f,	// Fullwidth Question Mark
	0xff01,	// Fullwidth Exclamation Mark
	0x309b,	// Katakana-Hiragana Voiced Sound Mark
	0x309c,	// Katakana-Hiragana Semi-Voiced Sound Mark
	0x30fd,	// Katakana Iteration Mark
	0x30fe,	// Katakana Voiced iteration Mark
	0x309d,	// Hiragana Iteration Mark
	0x309e,	// Hiragana Voiced Iteration Mark
	0x3005,	// Ideographic Iteration Mark
	0x30fc,	// Katakana-Hiragana Prolonged Sound Mark
	0xff09,	// Fullwidth Right Parenthesis
	0xff3d,	// Fullwidth Left Square Bracket
	0xff5d,	// Fullwidth Right Curly Bracket
	0x300d,	// Right Corner Bracket
	0x300f,	// Right White Corner Bracket
	0xff61,	// Halfwidth Ideographic Full Stop
	0xff63,	// Halfwidth Right Corner Bracket
	0xff64,	// Halfwidth Ideographic Comma
	0xff65,	// Halfwidth Katakana Middle Dot
	0xff70,	// Halfwidth Katakana-Hiragana Prolonged Sound Mark
	0xff9e,	// Halfwidth Katakana Voiced Sound Mark
	0xff9f,	// Halfwidth Katakana Semi-Voiced Sound Mark
	
	L'%',
	0xff0a,	// Fullwidth Colon
	0xff1b,	// Fullwidth Semicolon
	0x2019,	// Right Single Quotation Mark
	0x201d,	// Right Double Quotation Mark
	0x3015,	// Right Tortoise Shell Bracket
	0x3009,	// Right Angle Bracket
	0x300b,	// Right Double Angle Bracket
	0x3011,	// Right Black Lenticular Bracket
	0x00b0,	// Degree Sign
	0x2032,	// Prime
	0x2033,	// Double Prime
	0x2103,	// Degree Celsius
	0xffe0,	// Fullwidth Cent Sign
	0xff05,	// Fullwidth Percent Sign
	0x3041,	// Hiragana Letter Small A
	0x3043,	// Hiragana Letter Small I
	0x3045,	// Hiragana Letter Small U
	0x3047,	// Hiragana Letter Small E
	0x3049,	// Hiragana Letter Small O
	0x3063,	// Hiragana Letter Small Tu
	0x3083,	// Hiragana Letter Small Ya
	0x3085,	// Hiragana Letter Small Yu
	0x3087,	// Hiragana Letter Small Yo
	0x308e,	// Hiragana Letter Small Wa
	0x30a1,	// Katakana Letter Small A
	0x30a3,	// Katakana Letter Small I
	0x30a5,	// Katakana Letter Small U
	0x30a7,	// Katakana Letter Small E
	0x30a9,	// Katakana Letter Small O
	0x30c3,	// Katakana Letter Small Tu
	0x30e3,	// Katakana Letter Small Ya
	0x30e5,	// Katakana Letter Small Yu
	0x30e7,	// Katakana Letter Small Yo
	0x30ee,	// Katakana Letter Small Wa
	0x30f5,	// Katakana Letter Small Ka
	0x30f6,	// Katakana Letter Small Ke
	0xff67,	// Halfwidth Katakana Letter Small A
	0xff68,	// Halfwidth Katakana Letter Small I
	0xff69,	// Halfwidth Katakana Letter Small U
	0xff6a,	// Halfwidth Katakana Letter Small E
	0xff6b,	// Halfwidth Katakana Letter Small O
	0xff6c,	// Halfwidth Katakana Letter Small Ya
	0xff6d,	// Halfwidth Katakana Letter Small Yu
	0xff6e,	// Halfwidth Katakana Letter Small Yo
	0xff6f,	// Halfwidth Katakana Letter Small Tu
	0
};

const WCHAR qs::TextUtil::wszLineEndProhibited__[] = {
	L'(',
	L'[',
	L'{',
	0xff08,	// Fullwidth Left Parenthesis
	0xff3b,	// Fullwidth Left Square Bracket
	0xff5b,	// Fullwidth Left Curly Bracket
	0x300c,	// Left Corner Bracket
	0x300e,	// Left White Corner Bracket
	0xff62,	// Halfwidth Left Corner Bracket
	
	L'$',
	L'\\',
	0x2018,	// Left Single Quotation Mark
	0x201c,	// Left Double Quotation Mark
	0x3014,	// Left Tortoise Shell Bracket
	0x3008,	// Left Angle Bracket
	0x300a,	// Left Double Angle Bracket
	0x3010,	// Left Black Lenticular Bracket
	0xffe5,	// Fullwidth Yen Sign
	0xff04,	// Fullwidth Dollar Sign
	0
};

const WCHAR qs::TextUtil::wszDangling__[] = {
	L',',
	L'.',
	0x3001,	// Ideographic Comma
	0x3002,	// Ideographic Full Stop
	0xff0c,	// Fullwidth Comma
	0xff0e,	// Fullwidth Full Stop
	0
};

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
	const WCHAR* p = pwszText;
	while (p < pwszText + nLen) {
		WCHAR c = *p;
		size_t nCharLen = 1;
		if (c == L'\t')
			nCharLen = nTabWidth - nCurrentLen%nTabWidth;
		else if (!isHalfWidth(c))
			nCharLen = 2;
		
		if (c == L'\n') {
			if (!buf.append(pwszQuote, nQuoteLen) ||
				!buf.append(pLine, p - pLine + 1))
				return 0;
			nCurrentLen = 0;
			pLine = p + 1;
		}
		else if (nCurrentLen + nCharLen > nLineWidth) {
			const WCHAR* pEnd = wcschr(p, L'\n');
			if (!pEnd)
				pEnd = p + wcslen(p);
			const WCHAR* pBreak = getBreak(pLine, pEnd, p);
			
			p = pBreak - 1;
			
			if (TextUtil::isBreakSelf(*(pBreak - 1)))
				--pBreak;
			
			if (pwszQuote) {
				if (!buf.append(pwszQuote, nQuoteLen))
					return 0;
			}
			if (!buf.append(pLine, pBreak - pLine) || !buf.append(L'\n'))
				return 0;
			
			nCurrentLen = 0;
			pLine = p + 1;
		}
		else {
			nCurrentLen += nCharLen;
		}
		
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

const WCHAR* qs::TextUtil::getBreak(const WCHAR* pBegin,
									const WCHAR* pEnd,
									const WCHAR* p)
{
	assert(pBegin < p && p <= pEnd);
	
	if (p < pEnd && isDangling(*p)) {
		if (p + 1 == pEnd ||
			(isBreakBefore(*(p + 1)) && !isLineStartProhibited(*(p + 1))))
			return p + 1;
		else if (isBreakSelf(*(p + 1)) &&
			(p + 2 == pEnd || !isLineStartProhibited(*(p + 2))))
			return p + 2;
	}
	
	ssize_t nFit = p - pBegin;
	for (ssize_t n = nFit; n > 0; --n) {
		WCHAR c = *(pBegin + n);
		if (n != nFit &&
			(isBreakSelf(c) ||
			(isBreakAfter(c) && !isLineEndProhibited(c))) &&
			!isLineStartProhibited(*(pBegin + n + 1))) {
			nFit = n + 1;
			break;
		}
		else if (isBreakBefore(c) &&
			!isLineStartProhibited(c) &&
			!isLineEndProhibited(*(pBegin + n - 1))) {
			nFit = n;
			break;
		}
	}
	
	return pBegin + nFit;
}

bool qs::TextUtil::isHex(WCHAR c)
{
	return (L'0' <= c && c <= L'9') ||
		(L'A' <= c && c <= L'F') ||
		(L'a' <= c && c <= L'f');
}

std::pair<size_t, size_t> qs::TextUtil::findURL(const WCHAR* pwszText,
												size_t nLen,
												const WCHAR* const* ppwszSchemas,
												size_t nSchemaCount)
{
	assert(pwszText);
	
	std::pair<size_t, size_t> url(-1, 0);
	
	enum {
		TYPE_URL,
		TYPE_EMAIL,
		TYPE_PATH
	} type = TYPE_URL;
	
	const WCHAR* p = pwszText;
	while (nLen > 0) {
		if (*p == L':' && p != pwszText) {
			bool bFound = false;
			for (size_t n = 0; n < nSchemaCount; ++n) {
				size_t nSchemaLen = wcslen(ppwszSchemas[n]);
				if (static_cast<size_t>(p - pwszText) >= nSchemaLen &&
					wcsncmp(p - nSchemaLen, ppwszSchemas[n], nSchemaLen) == 0) {
					p -= nSchemaLen;
					nLen += nSchemaLen;
					bFound = true;
					break;
				}
			}
			if (bFound)
				break;
		}
		if (*p == L'@' &&
			p != pwszText && isCommonEmailAddressChar(*(p - 1)) &&
			nLen > 1 && isCommonEmailAddressChar(*(p + 1))) {
			while (p != pwszText && isCommonEmailAddressChar(*(p - 1))) {
				--p;
				++nLen;
			}
			type = TYPE_EMAIL;
			break;
		}
		if (*p == L'\\' && p != pwszText) {
			bool bFound = false;
			if (*(p - 1) == L'\\') {
				const WCHAR* pEnd = p + 1;
				for (size_t n = nLen - 1; n > 0; --n, ++pEnd) {
					WCHAR c = *pEnd;
					if (c == L'\\') {
						bFound = true;
						break;
					}
					else if ((c < L'a' || L'z' < c) &&
						(c < L'A' || L'Z' < c) &&
						(c < L'0' || L'9' < c) &&
						c != L'_' && c != L'.' && c != L'-') {
						break;
					}
				}
				if (bFound) {
					--p;
					++nLen;
				}
			}
			else if ((*(p - 1) == L':' && p - 1 != pwszText && isDriveLetterChar(*(p - 2)))) {
				p -= 2;
				nLen += 2;
				bFound = true;
			}
			if (bFound) {
				type = TYPE_PATH;
				break;
			}
		}
		--nLen;
		++p;
	}
	if (nLen > 0) {
		WCHAR cQuote = L'\0';
		if (p != pwszText && (type == TYPE_URL || type == TYPE_PATH)) {
			const WCHAR* pwszQuotes = L"()<>[]{}\"\"''";
			for (const WCHAR* pQuote = pwszQuotes; *pQuote; pQuote += 2) {
				if (*(p - 1) == *pQuote) {
					cQuote = *(pQuote + 1);
					break;
				}
			}
		}
		
		url.first = p - pwszText;
		switch (type) {
		case TYPE_URL:
			while (nLen > 0 && isURLChar(*p)) {
				--nLen;
				++p;
			}
			if (p != pwszText + url.first) {
				WCHAR c = *(p - 1);
				if (c == L'.' || c == L',' || c == cQuote)
					--p;
			}
			break;
		case TYPE_EMAIL:
			{
				const WCHAR* pAt = wcschr(p, L'@');
				assert(pAt);
				nLen -= pAt - p + 1;
				p = pAt + 1;
				while (nLen > 0 && isCommonEmailAddressChar(*p)) {
					--nLen;
					++p;
				}
				if (p != pwszText + url.first) {
					WCHAR c = *(p - 1);
					if (c == L'.')
						--p;
				}
			}
			break;
		case TYPE_PATH:
			while (nLen > 0) {
				if (!isPathChar(*p) ||
					(cQuote != L'\0' && *p == cQuote) ||
					(cQuote == L'\0' && *p == L' '))
					break;
				--nLen;
				++p;
			}
			break;
		default:
			assert(false);
			break;
		}
		url.second = p - (pwszText + url.first);
	}
	
	return url;
}

wstring_ptr qs::TextUtil::escapeURLComponent(const WCHAR* pwsz)
{
	size_t nLen = wcslen(pwsz);
	xstring_size_ptr str(UTF8Converter().encode(pwsz, &nLen));
	if (!str.get())
		return 0;
	
	StringBuffer<WSTRING> buf;
	for (const CHAR* p = str.get(); *p; ++p) {
		if (!isURLComponentChar(*p)) {
			WCHAR wsz[16];
			_snwprintf(wsz, countof(wsz), L"%%%02X",
				static_cast<int>(static_cast<unsigned char>(*p)));
			buf.append(wsz);
		}
		else {
			buf.append(static_cast<WCHAR>(*p));
		}
	}
	return buf.getString();
}

wstring_ptr qs::TextUtil::unescapeURLComponent(const WCHAR* pwsz)
{
	StringBuffer<STRING> bufUtf8;
	StringBuffer<WSTRING> buf;
	
	for (const WCHAR* p = pwsz; *p; ++p) {
		if (*p == L'%' && isHex(*(p + 1)) && isHex(*(p + 2))) {
			WCHAR wsz[3];
			wcsncpy(wsz, p + 1, 2);
			wsz[2] = L'\0';
			WCHAR* pEnd = 0;
			unsigned char c = static_cast<unsigned char>(wcstol(wsz, &pEnd, 16));
			if (c != 0)
				bufUtf8.append(static_cast<CHAR>(c));
			p += 2;
		}
		else if (*p < 0x80) {
			bufUtf8.append(static_cast<CHAR>(*p));
		}
		else {
			if (bufUtf8.getLength() != 0) {
				size_t nLen = bufUtf8.getLength();
				wxstring_size_ptr wstr(UTF8Converter().decode(bufUtf8.getCharArray(), &nLen));
				if (!wstr.get())
					return 0;
				buf.append(wstr.get(), wstr.size());
				bufUtf8.remove();
			}
			buf.append(*p);
		}
	}
	if (bufUtf8.getLength() != 0) {
		size_t nLen = bufUtf8.getLength();
		wxstring_size_ptr wstr(UTF8Converter().decode(bufUtf8.getCharArray(), &nLen));
		if (!wstr.get())
			return 0;
		buf.append(wstr.get(), wstr.size());
		bufUtf8.remove();
	}
	
	return buf.getString();
}

wstring_ptr qs::TextUtil::escapeIURIComponent(const WCHAR* pwsz)
{
	StringBuffer<WSTRING> buf;
	for (const WCHAR* p = pwsz; *p; ++p) {
		if (*p < 0x80 && !isURLComponentChar(*p)) {
			WCHAR wsz[16];
			_snwprintf(wsz, countof(wsz), L"%%%02X",
				static_cast<int>(static_cast<unsigned char>(*p)));
			buf.append(wsz);
		}
		else {
			buf.append(static_cast<WCHAR>(*p));
		}
	}
	return buf.getString();
}

wstring_ptr qs::TextUtil::unescapeIURIComponent(const WCHAR* pwsz)
{
	StringBuffer<WSTRING> buf;
	for (const WCHAR* p = pwsz; *p; ++p) {
		if (*p == L'%' && isHex(*(p + 1)) && isHex(*(p + 2))) {
			WCHAR wsz[3];
			wcsncpy(wsz, p + 1, 2);
			wsz[2] = L'\0';
			WCHAR* pEnd = 0;
			unsigned char c = static_cast<unsigned char>(wcstol(wsz, &pEnd, 16));
			if (c != 0)
				buf.append(static_cast<WCHAR>(c));
			p += 2;
		}
		else {
			buf.append(*p);
		}
	}
	return buf.getString();
}

bool qs::TextUtil::isURLChar(CHAR c)
{
	return ('A' <= c && c <= 'Z') ||
		('a' <= c && c <= 'z') ||
		('0' <= c && c <= '9') ||
		strchr(".:/?%&@!#$~*=+-_;,()[]'", c) != 0;
}

bool qs::TextUtil::isURLChar(WCHAR c)
{
	return (L'A' <= c && c <= L'Z') ||
		(L'a' <= c && c <= L'z') ||
		(L'0' <= c && c <= L'9') ||
		wcschr(L".:/?%&@!#$~*=+-_;,()[]'", c) != 0;
}

bool qs::TextUtil::isURLComponentChar(CHAR c)
{
	return ('A' <= c && c <= 'Z') ||
		('a' <= c && c <= 'z') ||
		('0' <= c && c <= '9') ||
		strchr(".&!$~*=+-_;,()'", c) != 0;
}

bool qs::TextUtil::isURLComponentChar(WCHAR c)
{
	return (L'A' <= c && c <= L'Z') ||
		(L'a' <= c && c <= L'z') ||
		(L'0' <= c && c <= L'9') ||
		wcschr(L".&!$~*=+-_;,()'", c) != 0;
}

bool qs::TextUtil::isFileNameChar(CHAR c)
{
	return !strchr("\"<>*?|\\/:", c);
}

bool qs::TextUtil::isFileNameChar(WCHAR c)
{
	return !wcschr(L"\"<>*?|\\/:", c);
}

bool qs::TextUtil::isPathChar(CHAR c)
{
	return !strchr("\"<>*?|", c);
}

bool qs::TextUtil::isPathChar(WCHAR c)
{
	return !wcschr(L"\"<>*?|", c);
}

bool qs::TextUtil::isDriveLetterChar(WCHAR c)
{
	return (L'a' <= c && c <= L'z') || (L'A' <= c && c <= L'Z');
}

bool qs::TextUtil::isCommonEmailAddressChar(WCHAR c)
{
	return (L'a' <= c && c <= L'z') ||
		(L'A' <= c && c <= L'Z') ||
		(L'0' <= c && c <= L'9') ||
		wcschr(L".$%+-=_'", c);
}

wstring_ptr qs::TextUtil::replace(const WCHAR* pwsz,
								  const WCHAR* pwszFind,
								  const WCHAR* pwszReplace)
{
	const WCHAR* p = wcsstr(pwsz, pwszFind);
	if (!p)
		return allocWString(pwsz);
	
	ConcatW c[] = {
		{ pwsz,					p - pwsz	},
		{ pwszReplace,			-1			},
		{ p + wcslen(pwszFind),	-1			}
	};
	return concat(c, countof(c));
}

wstring_ptr qs::TextUtil::replaceAll(const WCHAR* pwsz,
									 const WCHAR* pwszFind,
									 const WCHAR* pwszReplace)
{
	size_t nLen = wcslen(pwszFind);
	StringBuffer<WSTRING> buf;
	while (*pwsz) {
		if (wcsncmp(pwsz, pwszFind, nLen) == 0) {
			buf.append(pwszReplace);
			pwsz += nLen;
		}
		else {
			buf.append(*pwsz);
			++pwsz;
		}
	}
	return buf.getString();
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
		_snwprintf(pDst, 3, L"%02x", *(p + n) ^ 'q');
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
