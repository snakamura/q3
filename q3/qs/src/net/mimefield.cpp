/*
 * $Id$
 *
 * Copyright(C) 1998-2003 Satoshi Nakamura
 * All rights reserved.
 *
 */

#include <qsmime.h>
#include <qsconv.h>
#include <qsstl.h>
#include <qsencoder.h>
#include <qsnew.h>
#include <qswce.h>

#include <algorithm>
#include <memory>
#include <cstdio>

#include "mime.h"

#pragma warning(disable:4786)

using namespace qs;


/****************************************************************************
 *
 * Tokenizer
 *
 */

qs::Tokenizer::Tokenizer(const CHAR* psz, size_t nLen,
	unsigned int nFlags, QSTATUS* pstatus) :
	str_(0),
	p_(0),
	nFlags_(nFlags)
{
	assert((nFlags_ & 0x0f00) == F_SPECIAL ||
		(nFlags_ & 0x0f00) == F_TSPECIAL ||
		(nFlags_ & 0x0f00) == F_ESPECIAL);
	assert(pstatus);
	
	*pstatus = QSTATUS_SUCCESS;
	
	if (nLen == static_cast<size_t>(-1))
		nLen = strlen(psz);
	
	str_ = allocString(psz, nLen);
	if (!str_) {
		*pstatus = QSTATUS_OUTOFMEMORY;
		return;
	}
	p_ = str_;
}

qs::Tokenizer::~Tokenizer()
{
	freeString(str_);
}

QSTATUS qs::Tokenizer::getToken(Token* pToken, STRING* pstrToken)
{
	assert(pToken);
	assert(pstrToken);
	
	*pToken = T_ERROR;
	*pstrToken = 0;
	
	DECLARE_QSTATUS();
	
	while (isSpace(*p_))
		++p_;
	if (!*p_) {
		*pToken = T_END;
		return QSTATUS_SUCCESS;
	}
	
	StringBuffer<STRING> buf(&status);
	CHECK_QSTATUS();
	if ((nFlags_ && F_RECOGNIZECOMMENT) && *p_ == '(') {
		int nComment = 1;
		status = buf.append(*p_);
		CHECK_QSTATUS();
		++p_;
		while (*p_ && nComment != 0) {
			if (*p_ == '(') {
				++nComment;
			}
			else if (*p_ == ')') {
				--nComment;
			}
			else if (*p_ == '\\') {
				++p_;
				if (!*p_)
					return QSTATUS_SUCCESS;
			}
			status = buf.append(*p_);
			CHECK_QSTATUS();
			++p_;
		}
		if (*(p_ - 1) != ')')
			return QSTATUS_SUCCESS;
		
		*pToken = T_COMMENT;
		*pstrToken = buf.getString();
	}
	else if ((nFlags_ && F_RECOGNIZEDOMAIN) && *p_ == '[') {
		status = buf.append(*p_);
		CHECK_QSTATUS();
		++p_;
		while (*p_ && *p_ != ']') {
			if (*p_ == '[') {
				return QSTATUS_SUCCESS;
			}
			else if (*p_ == '\\') {
				++p_;
				if (!*p_)
					return QSTATUS_SUCCESS;
			}
			status = buf.append(*p_);
			CHECK_QSTATUS();
			++p_;
		}
		if (!*p_)
			return QSTATUS_SUCCESS;
		assert(*p_ == ']');
		status = buf.append(*p_);
		CHECK_QSTATUS();
		++p_;
		
		*pToken = T_DOMAIN;
		*pstrToken = buf.getString();
	}
	else if (*p_ == '\"') {
		++p_;
		while (*p_ && *p_ != '\"') {
			if (*p_ == '\\') {
				++p_;
				if (!*p_)
					return QSTATUS_SUCCESS;
			}
			status = buf.append(*p_);
			CHECK_QSTATUS();
			++p_;
		}
		if (!*p_)
			return QSTATUS_SUCCESS;
		assert(*p_ == '\"');
		++p_;
		
		*pToken = T_QSTRING;
		*pstrToken = buf.getString();
	}
	else if (isSpecial(*p_, nFlags_)) {
		status = buf.append(*p_);
		CHECK_QSTATUS();
		++p_;
		
		*pToken = T_SPECIAL;
		*pstrToken = buf.getString();
	}
	else if (isCtl(*p_)) {
	}
	else {
		while (*p_ &&
			!isSpecial(*p_, nFlags_) &&
			!isCtl(*p_) &&
			!isSpace(*p_)) {
			status = buf.append(*p_);
			CHECK_QSTATUS();
			++p_;
		}
		
		*pToken = T_ATOM;
		*pstrToken = buf.getString();
	}
	
	return QSTATUS_SUCCESS;
}

bool qs::Tokenizer::isCtl(unsigned char c)
{
	return (0x00 <= c && c <= 0x1f) || c == 0x7f;
}

bool qs::Tokenizer::isSpace(unsigned char c)
{
	return c == 0x20 || c == 0x09;
}

bool qs::Tokenizer::isSpecial(unsigned char c, unsigned int nFlags)
{
	if (c == '(' ||
		c == ')' ||
		c == '<' ||
		c == '>' ||
		c == '@' ||
		c == ',' ||
		c == ';' ||
		c == ':' ||
		c == '\"' ||
		c == '[' ||
		c == ']')
		return true;
	if (nFlags & F_SPECIAL) {
		if (c == '.' || c == '\\')
			return true;
	}
	if (nFlags & F_TSPECIAL) {
		if (c == '/' ||
			c == '?' ||
			c == '=' ||
			c == '\\')
			return true;
	}
	if (nFlags & F_ESPECIAL) {
		if (c == '/' ||
			c == '?' ||
			c == '.' ||
			c == '=')
			return true;
	}
	return false;
}


/****************************************************************************
 *
 * AddrSpecParser
 *
 */

qs::AddrSpecParser::AddrSpecParser(QSTATUS* pstatus)
{
	assert(pstatus);
	*pstatus = QSTATUS_SUCCESS;
}

qs::AddrSpecParser::~AddrSpecParser()
{
}

QSTATUS qs::AddrSpecParser::parseAddrSpec(const Part& part, Tokenizer& t,
	State state, Type type, STRING* pstrMailbox, STRING* pstrHost,
	STRING* pstrComment, Part::Field* pField, bool* pbEnd) const
{
	assert(pstrMailbox);
	assert(pstrHost);
	assert(pstrComment);
	assert(pField);
	assert((type == TYPE_INGROUP && pbEnd) || (type != TYPE_INGROUP && !pbEnd));
	
	DECLARE_QSTATUS();
	
	*pField = Part::FIELD_ERROR;
	
	CHAR szEnd[] = {
		type == TYPE_INBRACKET ? '>' : ',',
		type == TYPE_INGROUP ? ';' : '\0',
		'\0'
	};
	
	StringBuffer<STRING> bufMailbox(&status);
	CHECK_QSTATUS();
	StringBuffer<STRING> bufHost(&status);
	CHECK_QSTATUS();
	
	string_ptr<STRING> strComment;
	while (state != S_END) {
		Tokenizer::Token token;
		string_ptr<STRING> strToken;
		status = t.getToken(&token, &strToken);
		if (token == Tokenizer::T_COMMENT) {
			strComment.reset(strToken.release());
			continue;
		}
		
		if (part.isOption(Part::O_ALLOW_INVALID_PERIOD_IN_LOCALPART)) {
			if (state == S_BEGIN || state == S_LOCALPARTPERIOD)
				state = S_LOCALPARTWORD;
		}
		
		switch (state) {
		case S_BEGIN:
		case S_LOCALPARTPERIOD:
			switch (token) {
			case Tokenizer::T_ATOM:
			case Tokenizer::T_QSTRING:
				status = bufMailbox.append(strToken.get());
				CHECK_QSTATUS();
				state = S_LOCALPARTWORD;
				break;
			default:
				return FieldParser::parseError();
			}
			break;
		case S_LOCALPARTWORD:
			switch (token) {
			case Tokenizer::T_ATOM:
			case Tokenizer::T_QSTRING:
				if (part.isOption(Part::O_ALLOW_INVALID_PERIOD_IN_LOCALPART)) {
					status = bufMailbox.append(strToken.get());
					CHECK_QSTATUS();
				}
				else {
					return FieldParser::parseError();
				}
				break;
			case Tokenizer::T_SPECIAL:
				if (*strToken.get() == '@') {
					state = S_ADDRSPECAT;
				}
				else if (*strToken.get() == '.') {
					status = bufMailbox.append(strToken.get());
					CHECK_QSTATUS();
					state = S_LOCALPARTPERIOD;
				}
				else if (strchr(szEnd, *strToken.get()) &&
					part.isOption(Part::O_ALLOW_ADDRESS_WITHOUT_DOMAIN)) {
					if (pbEnd)
						*pbEnd = *strToken.get() == ';';
					state = S_END;
				}
				else {
					return FieldParser::parseError();
				}
				break;
			default:
				return FieldParser::parseError();
			}
			break;
		case S_ADDRSPECAT:
			switch (token) {
			case Tokenizer::T_DOMAIN:
			case Tokenizer::T_ATOM:
				status = bufHost.append(strToken.get());
				CHECK_QSTATUS();
				state = S_SUBDOMAIN;
				break;
			default:
				return FieldParser::parseError();
			}
			break;
		case S_SUBDOMAIN:
			switch (token) {
			case Tokenizer::T_END:
				if (type == TYPE_INGROUP)
					return FieldParser::parseError();
				state = S_END;
				break;
			case Tokenizer::T_SPECIAL:
				if (strchr(szEnd, *strToken.get())) {
					if (pbEnd)
						*pbEnd = *strToken.get() == ';';
					state = S_END;
				}
				else if (*strToken.get() == '.') {
					status = bufHost.append(strToken.get());
					CHECK_QSTATUS();
					state = S_SUBDOMAINPERIOD;
				}
				else {
					return FieldParser::parseError();
				}
				break;
			default:
				return FieldParser::parseError();
			}
			break;
		case S_SUBDOMAINPERIOD:
			if (token != Tokenizer::T_ATOM)
				return FieldParser::parseError();
			status = bufHost.append(strToken.get());
			CHECK_QSTATUS();
			state = S_SUBDOMAIN;
			break;
		default:
			return FieldParser::parseError();
		}
	}
	*pstrMailbox = bufMailbox.getString();
	*pstrHost = bufHost.getString();
	*pstrComment = strComment.release();
	*pField = Part::FIELD_EXIST;
	
	return QSTATUS_SUCCESS;
}


/****************************************************************************
 *
 * FieldParser
 *
 */

qs::FieldParser::~FieldParser()
{
}

QSTATUS qs::FieldParser::decode(const CHAR* psz, size_t nLen,
	WSTRING* pwstrDecoded, bool* pbDecode)
{
	assert(psz);
	assert(pwstrDecoded);
	
	DECLARE_QSTATUS();
	
	*pwstrDecoded = 0;
	if (pbDecode)
		*pbDecode = false;
	
	if (nLen == static_cast<size_t>(-1))
		nLen = strlen(psz);
	
	std::auto_ptr<Converter> pConverter;
	std::auto_ptr<Encoder> pEncoder;
	bool bDecode = false;
	bool bDecoded = false;
	StringBuffer<WSTRING> space(&status);
	CHECK_QSTATUS();
	StringBuffer<WSTRING> decoded(&status);
	CHECK_QSTATUS();
	StringBuffer<STRING> buf(&status);
	CHECK_QSTATUS();
	for (const CHAR* p = psz; p < psz + nLen; ++p) {
		if (bDecode) {
			if (*p == '?' && *(p + 1) == '=') {
				if (space.getLength() != 0) {
					if (!bDecoded) {
						status = decoded.append(space.getCharArray());
						CHECK_QSTATUS();
						status = space.remove();
						CHECK_QSTATUS();
					}
				}
				
				unsigned char* pDecode = 0;
				size_t nDecodeLen = 0;
				status = pEncoder->decode(
					reinterpret_cast<const unsigned char*>(buf.getCharArray()),
					buf.getLength(), &pDecode, &nDecodeLen);
				CHECK_QSTATUS();
				malloc_ptr<unsigned char> pRelease(pDecode);
				
				string_ptr<WSTRING> wstr;
				size_t nResultLen = 0;
				status = pConverter->decode(reinterpret_cast<const CHAR*>(pDecode),
					&nDecodeLen, &wstr, &nResultLen);
				CHECK_QSTATUS();
				
				status = decoded.append(wstr.get(), nResultLen);
				CHECK_QSTATUS();
				
				status = buf.remove();
				CHECK_QSTATUS();
				
				pConverter.reset(0);
				pEncoder.reset(0);
				bDecode = false;
				bDecoded = true;
				
				++p;
			}
			else {
				buf.append(*p);
			}
		}
		else {
			if (*p == '=' && *(p + 1) == '?') {
				string_ptr<WSTRING> wstrCharset;
				string_ptr<WSTRING> wstrEncoding;
				const CHAR* pBegin = p + 2;
				const CHAR* pEnd = strchr(p + 2, '?');
				if (pEnd) {
					wstrCharset.reset(mbs2wcs(pBegin, pEnd - pBegin));
					if (!wstrCharset.get())
						return QSTATUS_OUTOFMEMORY;
					WCHAR* pLang = wcschr(wstrCharset.get(), L'*');
					if (pLang)
						*pLang = L'\0';
					
					pBegin = pEnd + 1;
					pEnd = strchr(pBegin, '?');
					if (pEnd) {
						wstrEncoding.reset(mbs2wcs(pBegin, pEnd - pBegin));
						if (!wstrEncoding.get())
							return QSTATUS_OUTOFMEMORY;
					}
				}
				
				if (wstrCharset.get() && wstrEncoding.get()) {
					status = ConverterFactory::getInstance(
						wstrCharset.get(), &pConverter);
					CHECK_QSTATUS();
					
					status = EncoderFactory::getInstance(
						wstrEncoding.get(), &pEncoder);
					CHECK_QSTATUS();
				}
				if (pConverter.get() && pEncoder.get()) {
					bDecode = true;
					p = pEnd;
					status = space.remove();
					CHECK_QSTATUS();
					if (pbDecode)
						*pbDecode = true;
				}
				else {
					status = decoded.append(static_cast<WCHAR>(*p));
					CHECK_QSTATUS();
				}
			}
			else if (bDecoded && (*p == ' ' || *p == '\t')) {
				status = space.append(static_cast<WCHAR>(*p));
				CHECK_QSTATUS();
			}
			else {
				if (space.getLength() != 0) {
					status = decoded.append(
						space.getCharArray(), space.getLength());
					CHECK_QSTATUS();
					status = space.remove();
					CHECK_QSTATUS();
				}
				status = decoded.append(static_cast<WCHAR>(*p));
				CHECK_QSTATUS();
				bDecoded = false;
			}
		}
	}
	*pwstrDecoded = decoded.getString();
	
	return QSTATUS_SUCCESS;
}

QSTATUS qs::FieldParser::encode(const WCHAR* pwsz, size_t nLen,
	const WCHAR* pwszCharset, const WCHAR* pwszEncoding,
	bool bOneBlock, STRING* pstrEncoded)
{
	assert(pwsz);
	assert(pwszCharset);
	assert(!pwszEncoding ||
		wcscmp(pwszEncoding, L"B") == 0 ||
		wcscmp(pwszEncoding, L"Q") == 0);
	assert(pstrEncoded);
	
	DECLARE_QSTATUS();
	
	if (nLen == static_cast<size_t>(-1))
		nLen = wcslen(pwsz);
	
	const WCHAR* pwszEncodingSymbol = pwszEncoding;
	if (!pwszEncodingSymbol) {
		if (_wcsicmp(pwszCharset, L"us-ascii") == 0 ||
			_wcsicmp(pwszCharset, L"iso-8859-1") == 0 ||
			_wcsicmp(pwszCharset, L"utf-7") == 0)
			pwszEncodingSymbol = L"Q";
		else
			pwszEncodingSymbol = L"B";
	}
	
	std::auto_ptr<Encoder> pEncoder;
	status = EncoderFactory::getInstance(pwszEncodingSymbol, &pEncoder);
	CHECK_QSTATUS();
	assert(pEncoder.get());
	
	std::auto_ptr<Converter> pConverter;
	status = ConverterFactory::getInstance(pwszCharset, &pConverter);
	CHECK_QSTATUS();
	if (!pConverter.get()) {
		pwszCharset = Part::getDefaultCharset();
		status = ConverterFactory::getInstance(pwsz, &pConverter);
		CHECK_QSTATUS();
	}
	assert(pConverter.get());
	
	size_t nLine = std::count(pwsz, pwsz + nLen, L'\n');
	typedef std::pair<const WCHAR*, const WCHAR*> Line;
	auto_ptr_array<Line> lines(new Line[nLine + 1]);
	
	size_t n = 0;
	const WCHAR* pBegin = pwsz;
	while (pBegin <= pwsz + nLen) {
		const WCHAR* pEnd = wcschr(pBegin, L'\n');
		if (!pEnd)
			pEnd = pwsz + nLen;
		lines[n].first = pBegin;
		lines[n].second = pEnd;
		pBegin = pEnd + 1;
		++n;
	}
	assert(n == nLine + 1);
	
	StringBuffer<STRING> buf(&status);
	CHECK_QSTATUS();
	for (n = 0; n <= nLine; ++n) {
		if (n != 0) {
			status = buf.append("\r\n");
			CHECK_QSTATUS();
		}
		if (lines[n].first != lines[n].second) {
			string_ptr<STRING> str;
			status = encodeLine(lines[n].first,
				lines[n].second - lines[n].first, pwszCharset,
				pConverter.get(), pwszEncodingSymbol,
				pEncoder.get(), bOneBlock, &str);
			CHECK_QSTATUS();
			status = buf.append(str.get());
			CHECK_QSTATUS();
		}
	}
	*pstrEncoded = buf.getString();
	
	return QSTATUS_SUCCESS;
}

QSTATUS qs::FieldParser::getQString(const CHAR* psz,
	size_t nLen, STRING* pstrValue)
{
	assert(psz);
	assert(pstrValue);
	
	DECLARE_QSTATUS();
	
	if (nLen == static_cast<size_t>(-1))
		nLen = strlen(psz);
	
	StringBuffer<STRING> buf(nLen + 2, &status);
	CHECK_QSTATUS();
	
	status = buf.append('\"');
	CHECK_QSTATUS();
	
	for (const CHAR* p = psz; p < psz + nLen; ++p) {
		if (*p == '\\' || *p == '\"') {
			status = buf.append('\\');
			CHECK_QSTATUS();
		}
		status = buf.append(*p);
		CHECK_QSTATUS();
	}
	
	status = buf.append('\"');
	CHECK_QSTATUS();
	
	*pstrValue = buf.getString();
	
	return QSTATUS_SUCCESS;
}

QSTATUS qs::FieldParser::getAtomOrQString(const CHAR* psz,
	size_t nLen, STRING* pstrValue)
{
	assert(psz);
	assert(pstrValue);
	
	DECLARE_QSTATUS();
	
	if (nLen == static_cast<size_t>(-1))
		nLen = strlen(psz);
	
	if (isNeedQuote(psz, nLen, true)) {
		status = getQString(psz, nLen, pstrValue);
	}
	else {
		*pstrValue = allocString(psz, nLen);
		if (!*pstrValue)
			return QSTATUS_OUTOFMEMORY;
	}
	
	return status;
}

QSTATUS qs::FieldParser::getAtomsOrQString(const CHAR* psz,
	size_t nLen, STRING* pstrValue)
{
	assert(psz);
	assert(pstrValue);
	
	DECLARE_QSTATUS();
	
	if (nLen == static_cast<size_t>(-1))
		nLen = strlen(psz);
	
	if (isNeedQuote(psz, nLen, false)) {
		status = getQString(psz, nLen, pstrValue);
	}
	else {
		*pstrValue = allocString(psz, nLen);
		if (!*pstrValue)
			return QSTATUS_OUTOFMEMORY;
	}
	
	return status;
}

bool qs::FieldParser::isAscii(const WCHAR* pwsz)
{
	return isAscii(pwsz, static_cast<size_t>(-1));
}

bool qs::FieldParser::isAscii(const WCHAR* pwsz, size_t nLen)
{
	assert(pwsz);
	
	if (nLen == static_cast<size_t>(-1))
		nLen = wcslen(pwsz);
	
	const WCHAR* pEnd = pwsz + nLen;
	const WCHAR* p = std::find_if(pwsz, pEnd,
		std::bind2nd(std::greater<WCHAR>(), 0x7f));
	return p == pEnd;
}

bool qs::FieldParser::isNeedQuote(const CHAR* psz,
	size_t nLen, bool bQuoteWhitespace)
{
	assert(psz);
	
	if (nLen == static_cast<size_t>(-1))
		nLen = strlen(psz);
	
	if (nLen == 0)
		return true;
	
	for (const CHAR* p = psz; p < psz + nLen; ++p) {
		if (Tokenizer::isSpecial(*p,
			Tokenizer::F_SPECIAL | Tokenizer::F_TSPECIAL | Tokenizer::F_ESPECIAL) ||
			(bQuoteWhitespace && (*p == ' ' || *p == L'\t')))
			break;
	}
	return p != psz + nLen;
}

QSTATUS qs::FieldParser::parseError()
{
	return QSTATUS_SUCCESS;
}

QSTATUS qs::FieldParser::encodeLine(const WCHAR* pwsz, size_t nLen,
	const WCHAR* pwszCharset, Converter* pConverter, const WCHAR* pwszEncoding,
	Encoder* pEncoder, bool bOneBlock, STRING* pstrEncoded)
{
	assert(pwsz);
	assert(pwszCharset);
	assert(pConverter);
	assert(pwszEncoding);
	assert(pEncoder);
	assert(pstrEncoded);
	
	DECLARE_QSTATUS();
	
	if (nLen == static_cast<size_t>(-1))
		nLen = wcslen(pwsz);
	
	typedef std::pair<const WCHAR*, const WCHAR*> Item;
	typedef std::pair<Item, Item> Block;
	typedef std::vector<Block> Blocks;
	Blocks blocks;
	STLWrapper<Blocks> wrapper(blocks);
	if (bOneBlock) {
		status = wrapper.push_back(
			Block(Item(pwsz, pwsz + nLen), Item(0, 0)));
		CHECK_QSTATUS();
	}
	else {
		const WCHAR wsz[] = L" \t";
		const WCHAR* pBegin = pwsz;
		while (pBegin < pwsz + nLen) {
			const WCHAR* pEnd = std::find_first_of(
				pBegin, pwsz + nLen, wsz, wsz + 2);
			status = wrapper.push_back(
				Block(Item(pBegin, pEnd), Item(0, 0)));
			CHECK_QSTATUS();
			if (pEnd != pwsz + nLen) {
				pBegin = pEnd;
				while (*pEnd == L' ' || *pEnd == L'\t')
					++pEnd;
				blocks.back().second.first = pBegin;
				blocks.back().second.second = pEnd;
			}
			pBegin = pEnd;
		}
	}
	
	string_ptr<STRING> strCharset(wcs2mbs(pwszCharset));
	if (!strCharset.get())
		return QSTATUS_OUTOFMEMORY;
	string_ptr<STRING> strEncoding(wcs2mbs(pwszEncoding));
	if (!strEncoding.get())
		return QSTATUS_OUTOFMEMORY;
	
	StringBuffer<STRING> buf(&status);
	CHECK_QSTATUS();
	Blocks::iterator it = blocks.begin();
	while (it != blocks.end()) {
		Block& b = *it;
		Item& i = b.first;
		Item& s = b.second;
		if (!isAscii(i.first, i.second - i.first)) {
			Item itemAfter = (it + 1) != blocks.end() ? (*(it + 1)).first : Item(0, 0);
			if ((itemAfter.first == itemAfter.second ||
				!isAscii(itemAfter.first, itemAfter.second - itemAfter.first)) &&
				s.first) {
				i.second = s.second;
				s.first = 0;
				s.second = 0;
			}
			
			string_ptr<STRING> str;
			size_t nLen = i.second - i.first;
			size_t nResultLen = 0;
			status = pConverter->encode(i.first, &nLen, &str, &nResultLen);
			CHECK_QSTATUS();
			
			unsigned char* p = 0;
			status = pEncoder->encode(
				reinterpret_cast<const unsigned char*>(str.get()),
				nResultLen, &p, &nLen);
			malloc_ptr<unsigned char> pRelease(p);
			
			status = buf.append("=?");
			CHECK_QSTATUS();
			status = buf.append(strCharset.get());
			CHECK_QSTATUS();
			status = buf.append("?");
			CHECK_QSTATUS();
			status = buf.append(strEncoding.get());
			CHECK_QSTATUS();
			status = buf.append("?");
			CHECK_QSTATUS();
			status = buf.append(reinterpret_cast<CHAR*>(p), nLen);
			CHECK_QSTATUS();
			status = buf.append("?=");
			CHECK_QSTATUS();
		}
		else {
			if (i.first != i.second) {
				string_ptr<STRING> str(wcs2mbs(i.first, i.second - i.first));
				if (!str.get())
					return QSTATUS_OUTOFMEMORY;
				status = buf.append(str.get());
				CHECK_QSTATUS();
			}
		}
		
		if (s.first != s.second) {
			string_ptr<STRING> str(wcs2mbs(s.first, s.second - s.first));
			if (!str.get())
				return QSTATUS_OUTOFMEMORY;
			status = buf.append(str.get());
			CHECK_QSTATUS();
		}
		
		++it;
	}
	
	*pstrEncoded = buf.getString();
	
	return QSTATUS_SUCCESS;
}


/****************************************************************************
 *
 * UnstructuredParser
 *
 */

qs::UnstructuredParser::UnstructuredParser(QSTATUS* pstatus) :
	wstrValue_(0),
	wstrCharset_(0)
{
	assert(pstatus);
	*pstatus = QSTATUS_SUCCESS;
}

qs::UnstructuredParser::UnstructuredParser(const WCHAR* pwszValue,
	const WCHAR* pwszCharset, QSTATUS* pstatus):
	wstrValue_(0),
	wstrCharset_(0)
{
	assert(pstatus);
	
	*pstatus = QSTATUS_SUCCESS;
	
	string_ptr<WSTRING> wstrValue(allocWString(pwszValue));
	if (!wstrValue.get()) {
		*pstatus = QSTATUS_OUTOFMEMORY;
		return;
	}
	
	string_ptr<WSTRING> wstrCharset(allocWString(pwszCharset));
	if (!wstrCharset.get()) {
		*pstatus = QSTATUS_OUTOFMEMORY;
		return;
	}
	
	wstrValue_ = wstrValue.release();
	wstrCharset_ = wstrCharset.release();
}

qs::UnstructuredParser::~UnstructuredParser()
{
	freeWString(wstrValue_);
	freeWString(wstrCharset_);
}

const WCHAR* qs::UnstructuredParser::getValue() const
{
	return wstrValue_;
}

QSTATUS qs::UnstructuredParser::parse(const Part& part,
	const WCHAR* pwszName, Part::Field* pField)
{
	assert(pwszName);
	assert(pField);
	
	DECLARE_QSTATUS();
	
	*pField = Part::FIELD_ERROR;
	
	string_ptr<STRING> strValue;
	bool bExist = false;
	status = part.getRawField(pwszName, 0, &strValue, &bExist);
	CHECK_QSTATUS();
	if (!bExist) {
		*pField = Part::FIELD_NOTEXIST;
		return QSTATUS_SUCCESS;
	}
	
	status = decode(strValue.get(), static_cast<size_t>(-1), &wstrValue_, 0);
	CHECK_QSTATUS();
	
	if (isAscii(wstrValue_)) {
		bool bRaw = part.isOption(Part::O_ALLOW_RAW_FIELD);
		if (!bRaw) {
			bool bHas = false;
			status = part.hasField(L"MIME-Version", &bHas);
			CHECK_QSTATUS();
			bRaw = !bHas;
		}
		if (bRaw) {
			std::auto_ptr<Converter> pConverter;
			status = ConverterFactory::getInstance(
				part.getDefaultCharset(), &pConverter);
			CHECK_QSTATUS();
			assert(pConverter.get());
			
			string_ptr<STRING> strRawValue(wcs2mbs(wstrValue_));
			if (!strRawValue.get())
				return QSTATUS_OUTOFMEMORY;
			size_t nLen = strlen(strRawValue.get());
			string_ptr<WSTRING> wstrValue;
			status = pConverter->decode(strRawValue.get(),
				&nLen, &wstrValue, 0);
			CHECK_QSTATUS();
			
			freeWString(wstrValue_);
			wstrValue_ = wstrValue.release();
		}
	}
	*pField = Part::FIELD_EXIST;
	
	return QSTATUS_SUCCESS;
}

QSTATUS qs::UnstructuredParser::unparse(
	const Part& part, STRING* pstrValue) const
{
	assert(pstrValue);
	
	DECLARE_QSTATUS();
	
	const WCHAR* pwszCharset = wstrCharset_;
	string_ptr<WSTRING> wstrHeaderCharset;
	if (!pwszCharset) {
		status = part.getHeaderCharset(&wstrHeaderCharset);
		CHECK_QSTATUS();
		pwszCharset = wstrHeaderCharset.get();
	}
	
	string_ptr<WSTRING> wstrFoldedValue;
	status = foldValue(wstrValue_, &wstrFoldedValue);
	CHECK_QSTATUS();
	
	status = encode(wstrFoldedValue.get(), static_cast<size_t>(-1),
		pwszCharset, 0, false, pstrValue);
	CHECK_QSTATUS();
	
	return QSTATUS_SUCCESS;
}

QSTATUS qs::UnstructuredParser::foldValue(
	const WCHAR* pwszValue, WSTRING* pwstrFolded) const
{
	assert(pwszValue);
	assert(pwstrFolded);
	
	DECLARE_QSTATUS();
	
	typedef std::pair<const WCHAR*, const WCHAR*> Block;
	typedef std::vector<Block> Blocks;
	Blocks blocks;
	STLWrapper<Blocks> wrapper(blocks);
	
	size_t nValueLen = wcslen(pwszValue);
	const WCHAR* pBegin = pwszValue;
	size_t nLen = 0;
	size_t nLenBreak = 0;
	size_t* pnLen = &nLen;
	const WCHAR* pLastBreak = 0;
	for (const WCHAR* p = pwszValue; p <= pwszValue + nValueLen; ++p) {
		WCHAR c = *p;
		if (c == L' ' || c == L'\0') {
		}
		else if (c <= 0x7f) {
			++(*pnLen);
		}
		else {
			if (p == pwszValue || nLen + nLenBreak == 0 || *(p - 1) <= 0x7f)
				*pnLen += ENCODE_MARKER_LENGTH;
			*pnLen += ENCODE_CHAR_LENGTH;
		}
		
		bool bCanBreak = c == L' ' || c == L'\0' || (c > 0x7f && p != pwszValue && *(p - 1) > 0x7f);
		bool bNeedBreak = nLen + nLenBreak > FOLD_LENGTH;
		if (bCanBreak || (bNeedBreak && pLastBreak)) {
			while (*p == L' ')
				++p;
			if (bNeedBreak || c == L'\0') {
				bool bProcess = false;
				if (bNeedBreak && pLastBreak) {
					status = wrapper.push_back(Block(pBegin, pLastBreak));
					CHECK_QSTATUS();
					pBegin = pLastBreak;
					nLen = nLenBreak + (*pBegin > 0x7f ? ENCODE_MARKER_LENGTH : 0);
					pLastBreak = 0;
					pnLen = &nLenBreak;
					bProcess = true;
				}
				if (bCanBreak && (!bProcess || (c == L'\0' && nLen != 0))) {
					status = wrapper.push_back(Block(pBegin, p));
					pBegin = p;
					nLen = 0;
					pLastBreak = 0;
					pnLen = &nLen;
				}
				nLenBreak = 0;
			}
			else if (bCanBreak) {
				pLastBreak = p;
				nLen += nLenBreak;
				nLenBreak = 0;
				pnLen = &nLenBreak;
			}
			if (c == L' ')
				--p;
		}
	}
	assert(nLen == 0 && !pLastBreak && pBegin == pwszValue + nValueLen);
	
	StringBuffer<WSTRING> buf(&status);
	CHECK_QSTATUS();
	Blocks::iterator it = blocks.begin();
	while (it != blocks.end()) {
		Block& b = *it;
		if (it + 1 != blocks.end()) {
			if (*(b.second - 1) == L' ') {
				Block& bNext = *(it + 1);
				if (!isLastTokenEncode(b.first, b.second - b.first) ||
					!isFirstTokenEncode(bNext.first, bNext.second - bNext.first))
					--b.second;
			}
			status = buf.append(b.first, b.second - b.first);
			CHECK_QSTATUS();
			status = buf.append(L"\n ");
			CHECK_QSTATUS();
		}
		else {
			status = buf.append(b.first, b.second - b.first);
			CHECK_QSTATUS();
		}
		++it;
	}
	*pwstrFolded = buf.getString();
	
	return QSTATUS_SUCCESS;
}

bool qs::UnstructuredParser::isFirstTokenEncode(const WCHAR* pwsz, size_t nLen)
{
	const WCHAR* p = pwsz;
	while (p < pwsz + nLen && *p != L' ' && *p != L'\t') {
		if (*p >= 0x80)
			return true;
		++p;
	}
	return false;
}

bool qs::UnstructuredParser::isLastTokenEncode(const WCHAR* pwsz, size_t nLen)
{
	const WCHAR* p = pwsz + nLen - 1;
	if (*p == L' ' || *p == L'\t')
		--p;
	while (*p != L' ' && *p != L'\t') {
		if (*p >= 0x80)
			return true;
		--p;
		if (p == pwsz)
			break;
	}
	return false;
}


/****************************************************************************
 *
 * DummyParser
 *
 */

qs::DummyParser::DummyParser(const WCHAR* pwszValue,
	unsigned int nFlags, QSTATUS* pstatus) :
	nFlags_(nFlags),
	wstrValue_(0)
{
	assert(pstatus);
	
	*pstatus = QSTATUS_SUCCESS;
	
	string_ptr<WSTRING> wstrValue(allocWString(pwszValue));
	if (!wstrValue.get()) {
		*pstatus = QSTATUS_OUTOFMEMORY;
		return;
	}
	
	wstrValue_ = wstrValue.release();
}

qs::DummyParser::~DummyParser()
{
	freeWString(wstrValue_);
}

const WCHAR* qs::DummyParser::getValue() const
{
	return wstrValue_;
}

QSTATUS qs::DummyParser::parse(const Part& part,
	const WCHAR* pwszName, Part::Field* pField)
{
	assert(false);
	return QSTATUS_FAIL;
}

QSTATUS qs::DummyParser::unparse(const Part& part, STRING* pstrValue) const
{
	assert(pstrValue);
	
	DECLARE_QSTATUS();
	
	string_ptr<WSTRING> wstrCharset;
	status = part.getHeaderCharset(&wstrCharset);
	CHECK_QSTATUS();
	
	size_t nLen = wcslen(wstrValue_);
	const WCHAR* pBegin = wstrValue_;
	const WCHAR* pEnd = wstrValue_;
	bool bAscii = true;
	StringBuffer<STRING> buf(&status);
	CHECK_QSTATUS();
	while (pEnd < wstrValue_ + nLen) {
		WCHAR c = *pEnd;
		if (bAscii)
			bAscii = c <= 0x7f;
		if (isSpecial(c)) {
			if (pBegin != pEnd) {
				if (bAscii) {
					string_ptr<STRING> str(wcs2mbs(pBegin, pEnd - pBegin));
					if (!str.get())
						return QSTATUS_OUTOFMEMORY;
					status = buf.append(str.get());
					CHECK_QSTATUS();
				}
				else {
					const WCHAR* p = pEnd;
					if (*(p - 1) == L' ')
						--p;
					if (pBegin != wstrValue_ && *pBegin == L' ')
						++pBegin;
					string_ptr<STRING> str;
					status = encode(pBegin, p - pBegin,
						wstrCharset.get(), 0, false, &str);
					CHECK_QSTATUS();
					status = buf.append(str.get());
					CHECK_QSTATUS();
				}
			}
			status = buf.append(static_cast<CHAR>(c));
			CHECK_QSTATUS();
			pBegin = pEnd + 1;
			bAscii = true;
		}
		++pEnd;
	}
	if (pBegin != pEnd) {
		string_ptr<STRING> str;
		status = encode(pBegin, pEnd - pBegin,
			wstrCharset.get(), 0, false, &str);
		CHECK_QSTATUS();
		status = buf.append(str.get());
		CHECK_QSTATUS();
	}
	*pstrValue = buf.getString();
	
	return QSTATUS_SUCCESS;
}

bool qs::DummyParser::isSpecial(WCHAR c) const
{
	if (c == L'(' ||
		c == L')' ||
		c == L'<' ||
		c == L'>' ||
		c == L'@' ||
		c == L',' ||
		c == L';' ||
		c == L':' ||
		c == L'\"' ||
		c == L'[' ||
		c == L']')
		return true;
	if (nFlags_ & FLAG_TSPECIAL) {
		if (c == L'/' ||
			c == L'?' ||
			c == L'=' ||
			c == L'\\')
			return true;
	}
	else if (nFlags_ & FLAG_ESPECIAL) {
		if (c == L'/' ||
			c == L'?' ||
			c == L'.' ||
			c == L'=')
			return true;
	}
	else {
		if (c == L'.' || c == L'\\')
			return true;
	}
	return false;
}


/****************************************************************************
 *
 * NoParseParser
 *
 */

qs::NoParseParser::NoParseParser(const WCHAR* pwszSeparator,
	unsigned int nFlags, QSTATUS* pstatus) :
	nFlags_(nFlags),
	strSeparator_(0),
	wstrValue_(0)
{
	assert(pwszSeparator);
	assert(pstatus);
	
	*pstatus = QSTATUS_SUCCESS;
	
	strSeparator_ = wcs2mbs(pwszSeparator);
	if (!strSeparator_) {
		*pstatus = QSTATUS_OUTOFMEMORY;
		return;
	}
}

qs::NoParseParser::~NoParseParser()
{
	freeString(strSeparator_);
	freeWString(wstrValue_);
}

const WCHAR* qs::NoParseParser::getValue() const
{
	return wstrValue_;
}

QSTATUS qs::NoParseParser::parse(const Part& part,
	const WCHAR* pwszName, Part::Field* pField)
{
	assert(pwszName);
	assert(pField);
	
	DECLARE_QSTATUS();
	
	*pField = Part::FIELD_ERROR;
	
	string_ptr<STRING> strValue;
	bool bExist = false;
	status = part.getRawField(pwszName, 0, &strValue, &bExist);
	CHECK_QSTATUS();
	if (!bExist) {
		*pField = Part::FIELD_NOTEXIST;
		return QSTATUS_SUCCESS;
	}
	
	if (!(nFlags_ & FLAG_SINGLEFIELD)) {
		StringBuffer<STRING> buf(strValue.get(), &status);
		CHECK_QSTATUS();
		unsigned int n = 1;
		bool bExist = true;
		while (bExist) {
			string_ptr<STRING> str;
			status = part.getRawField(pwszName, n, &str, &bExist);
			CHECK_QSTATUS();
			if (bExist) {
				status = buf.append(strSeparator_);
				CHECK_QSTATUS();
				status = buf.append(str.get());
				CHECK_QSTATUS();
			}
			++n;
		}
		strValue.reset(buf.getString());
	}
	
	wstrValue_ = mbs2wcs(strValue.get());
	if (!wstrValue_)
		return QSTATUS_OUTOFMEMORY;
	
	return QSTATUS_SUCCESS;
}

QSTATUS qs::NoParseParser::unparse(const Part& part, STRING* pstrValue) const
{
	assert(false);
	return QSTATUS_FAIL;
}


/****************************************************************************
 *
 * SimpleParser
 *
 */

qs::SimpleParser::SimpleParser(unsigned int nFlags, QSTATUS* pstatus) :
	nFlags_(nFlags),
	wstrValue_(0)
{
	assert(pstatus);
	*pstatus = QSTATUS_SUCCESS;
}

qs::SimpleParser::SimpleParser(const WCHAR* pwszValue,
	unsigned int nFlags, QSTATUS* pstatus) :
	nFlags_(nFlags),
	wstrValue_(0)
{
	assert(pstatus);
	
	*pstatus = QSTATUS_SUCCESS;
	
	string_ptr<WSTRING> wstrValue(allocWString(pwszValue));
	if (!wstrValue.get()) {
		*pstatus = QSTATUS_OUTOFMEMORY;
		return;
	}
	
	wstrValue_ = wstrValue.release();
}

qs::SimpleParser::~SimpleParser()
{
	freeWString(wstrValue_);
}

const WCHAR* qs::SimpleParser::getValue() const
{
	return wstrValue_;
}

QSTATUS qs::SimpleParser::parse(const Part& part,
	const WCHAR* pwszName, Part::Field* pField)
{
	assert(pwszName);
	assert(pField);
	
	DECLARE_QSTATUS();
	
	*pField = Part::FIELD_ERROR;
	
	string_ptr<STRING> strValue;
	bool bExist = false;
	status = part.getRawField(pwszName, 0, &strValue, &bExist);
	CHECK_QSTATUS();
	if (!bExist) {
		*pField = Part::FIELD_NOTEXIST;
		return QSTATUS_SUCCESS;
	}
	
	unsigned int nFlags = 0;
	if (nFlags_ & FLAG_RECOGNIZECOMMENT)
		nFlags |= Tokenizer::F_RECOGNIZECOMMENT;
	if (nFlags_ & FLAG_TSPECIAL)
		nFlags |= Tokenizer::F_TSPECIAL;
	else
		nFlags |= Tokenizer::F_SPECIAL;
	Tokenizer t(strValue.get(), static_cast<size_t>(-1), nFlags, &status);
	CHECK_QSTATUS();
	State state = S_BEGIN;
	while (state != S_END) {
		Tokenizer::Token token;
		string_ptr<STRING> strToken;
		status = t.getToken(&token, &strToken);
		CHECK_QSTATUS();
		
		if (token == Tokenizer::T_COMMENT)
			continue;
		
		switch (state) {
		case S_BEGIN:
			if (token == Tokenizer::T_ATOM) {
				if (nFlags_ & FLAG_DECODE) {
					status = decode(strToken.get(),
						static_cast<size_t>(-1), &wstrValue_, 0);
					CHECK_QSTATUS();
				}
				else {
					wstrValue_ = mbs2wcs(strToken.get());
					if (!wstrValue_)
						return QSTATUS_OUTOFMEMORY;
				}
				state = S_ATOM;
			}
			else if (token == Tokenizer::T_QSTRING && nFlags_ & FLAG_ACCEPTQSTRING) {
				if (part.isOption(Part::O_ALLOW_ENCODED_QSTRING) &&
					nFlags_ & FLAG_DECODE) {
					status = decode(strToken.get(),
						static_cast<size_t>(-1), &wstrValue_, 0);
					CHECK_QSTATUS();
				}
				else {
					wstrValue_ = mbs2wcs(strToken.get());
					if (!wstrValue_)
						return QSTATUS_OUTOFMEMORY;
				}
				state = S_ATOM;
			}
			else if (token == Tokenizer::T_END) {
				wstrValue_ = allocWString(L"");
				if (!wstrValue_)
					return QSTATUS_OUTOFMEMORY;
				state = S_END;
			}
			else {
				return parseError();
			}
			break;
		case S_ATOM:
			if (token != Tokenizer::T_END)
				return parseError();
			state = S_END;
			break;
		case S_END:
			break;
		}
	}
	*pField = Part::FIELD_EXIST;
	
	return QSTATUS_SUCCESS;
}

QSTATUS qs::SimpleParser::unparse(const Part& part, STRING* pstrValue) const
{
	DECLARE_QSTATUS();
	
	if (nFlags_ & FLAG_DECODE) {
		string_ptr<WSTRING> wstrCharset;
		status = part.getHeaderCharset(&wstrCharset);
		CHECK_QSTATUS();
		
		status = encode(wstrValue_, static_cast<size_t>(-1),
			wstrCharset.get(), 0, true, pstrValue);
		CHECK_QSTATUS();
	}
	else {
		if (!isAscii(wstrValue_))
			return QSTATUS_FAIL;
		*pstrValue = wcs2mbs(wstrValue_);
		if (!*pstrValue)
			return QSTATUS_OUTOFMEMORY;
	}
	
	return QSTATUS_SUCCESS;
}


/****************************************************************************
 *
 * NumberParser
 *
 */

qs::NumberParser::NumberParser(unsigned int nFlags, QSTATUS* pstatus) :
	nFlags_(nFlags),
	n_(0)
{
	assert(pstatus);
	*pstatus = QSTATUS_SUCCESS;
}

qs::NumberParser::NumberParser(unsigned int n,
	unsigned int nFlags, QSTATUS* pstatus) :
	nFlags_(nFlags), n_(n)
{
	assert(pstatus);
	*pstatus = QSTATUS_SUCCESS;
}

qs::NumberParser::~NumberParser()
{
}

unsigned int qs::NumberParser::getValue() const
{
	return n_;
}

QSTATUS qs::NumberParser::parse(const Part& part,
	const WCHAR* pwszName, Part::Field* pField)
{
	assert(pwszName);
	assert(pField);
	
	DECLARE_QSTATUS();
	
	*pField = Part::FIELD_ERROR;
	
	string_ptr<STRING> strValue;
	bool bExist = false;
	status = part.getRawField(pwszName, 0, &strValue, &bExist);
	CHECK_QSTATUS();
	if (!bExist) {
		*pField = Part::FIELD_NOTEXIST;
		return QSTATUS_SUCCESS;
	}
	
	unsigned int nFlags = Tokenizer::F_SPECIAL;
	if (nFlags_ & FLAG_RECOGNIZECOMMENT)
		nFlags |= Tokenizer::F_RECOGNIZECOMMENT;
	Tokenizer t(strValue.get(), static_cast<size_t>(-1), nFlags, &status);
	CHECK_QSTATUS();
	State state = S_BEGIN;
	string_ptr<STRING> strNumber;
	while (state != S_END) {
		Tokenizer::Token token;
		string_ptr<STRING> strToken;
		status = t.getToken(&token, &strToken);
		CHECK_QSTATUS();
		
		if (token == Tokenizer::T_COMMENT)
			continue;
		switch (state) {
		case S_BEGIN:
			if (token != Tokenizer::T_ATOM)
				return parseError();
			strNumber.reset(strToken.release());
			state = S_ATOM;
			break;
		case S_ATOM:
			if (token != Tokenizer::T_END)
				return parseError();
			state = S_END;
			break;
		case S_END:
			break;
		}
	}
	// TODO
	// Check value
	if (nFlags_ & FLAG_HEX)
		sscanf(strNumber.get(), "%x", &n_);
	else
		sscanf(strNumber.get(), "%d", &n_);
	*pField = Part::FIELD_EXIST;
	
	return QSTATUS_SUCCESS;
}

QSTATUS qs::NumberParser::unparse(const Part& part, STRING* pstrValue) const
{
	CHAR sz[32];
	if (nFlags_ & FLAG_HEX)
		sprintf(sz, "%x", n_);
	else
		sprintf(sz, "%d", n_);
	
	*pstrValue = allocString(sz);
	
	return *pstrValue ? QSTATUS_SUCCESS : QSTATUS_OUTOFMEMORY;
}


/****************************************************************************
 *
 * DateParser
 *
 */

qs::DateParser::DateParser(QSTATUS* pstatus)
{
	assert(pstatus);
	*pstatus = QSTATUS_SUCCESS;
}

qs::DateParser::DateParser(const Time& date, QSTATUS* pstatus) :
	date_(date)
{
	assert(pstatus);
	*pstatus = QSTATUS_SUCCESS;
}

qs::DateParser::~DateParser()
{
}

const Time& qs::DateParser::getTime() const
{
	return date_;
}

QSTATUS qs::DateParser::parse(const Part& part,
	const WCHAR* pwszName, Part::Field* pField)
{
	assert(pwszName);
	assert(pField);
	
	DECLARE_QSTATUS();
	
	*pField = Part::FIELD_ERROR;
	
	string_ptr<STRING> strValue;
	bool bExist = false;
	status = part.getRawField(pwszName, 0, &strValue, &bExist);
	CHECK_QSTATUS();
	if (!bExist) {
		*pField = Part::FIELD_NOTEXIST;
		return QSTATUS_SUCCESS;
	}
	
	status = parse(strValue.get(), static_cast<size_t>(-1),
		part.isOption(Part::O_ALLOW_SINGLE_DIGIT_TIME), &date_);
	if (status == QSTATUS_FAIL) {
		*pField = Part::FIELD_ERROR;
	}
	else {
		CHECK_QSTATUS();
		*pField = Part::FIELD_EXIST;
	}
	
	return QSTATUS_SUCCESS;
}

QSTATUS qs::DateParser::unparse(const Part& part, STRING* pstrValue) const
{
	DECLARE_QSTATUS();
	
	string_ptr<WSTRING> wstr;
	status = date_.format(L"%W, %D %M1 %Y4 %h:%m:%s %z",
		Time::FORMAT_ORIGINAL, &wstr);
	CHECK_QSTATUS();
	
	*pstrValue = wcs2mbs(wstr.get());
	
	return *pstrValue ? QSTATUS_SUCCESS : QSTATUS_OUTOFMEMORY;
}

QSTATUS qs::DateParser::parse(const CHAR* psz, size_t nLen,
	bool bAllowSingleDigitTime, Time* pTime)
{
	assert(psz);
	assert(pTime);
	
	DECLARE_QSTATUS();
	
	Tokenizer t(psz, nLen,
		Tokenizer::F_RECOGNIZECOMMENT | Tokenizer::F_SPECIAL, &status);
	CHECK_QSTATUS();
	
	State state = S_BEGIN;
	string_ptr<STRING> strFirst;
	int nWeek = -1;
	int nDay = 0;
	int nMonth = 0;
	int nYear = 0;
	int nHour = 0;
	int nMinute = 0;
	int nSecond = 0;
	int nTimeZone = 0;
	while (state != S_END) {
		Tokenizer::Token token;
		string_ptr<STRING> strToken;
		status = t.getToken(&token, &strToken);
		CHECK_QSTATUS();
		
		if (token == Tokenizer::T_COMMENT)
			continue;
		switch (state) {
		case S_BEGIN:
			if (token != Tokenizer::T_ATOM)
				return QSTATUS_FAIL;
			strFirst.reset(strToken.release());
			state = S_FIRST;
			break;
		case S_FIRST:
			if (token == Tokenizer::T_SPECIAL && *strToken.get() == L',') {
				status = getWeek(strFirst.get(), &nWeek);
				CHECK_QSTATUS();
				state = S_WEEK;
			}
			else if (token == Tokenizer::T_ATOM) {
				status = getDay(strFirst.get(), &nDay);
				CHECK_QSTATUS();
				status = getMonth(strToken.get(), &nMonth);
				CHECK_QSTATUS();
				state = S_MONTH;
			}
			else {
				return QSTATUS_FAIL;
			}
			break;
		case S_WEEK:
			if (token != Tokenizer::T_ATOM)
				return QSTATUS_FAIL;
			status = getDay(strToken.get(), &nDay);
			CHECK_QSTATUS();
			state = S_DAY;
			break;
		case S_DAY:
			if (token != Tokenizer::T_ATOM)
				return QSTATUS_FAIL;
			status = getMonth(strToken.get(), &nMonth);
			CHECK_QSTATUS();
			state = S_MONTH;
			break;
		case S_MONTH:
			if (token != Tokenizer::T_ATOM)
				return QSTATUS_FAIL;
			status = getYear(strToken.get(), &nYear);
			CHECK_QSTATUS();
			state = S_YEAR;
			break;
		case S_YEAR:
			if (token != Tokenizer::T_ATOM)
				return QSTATUS_FAIL;
			status = getHour(strToken.get(), bAllowSingleDigitTime, &nHour);
			CHECK_QSTATUS();
			state = S_HOUR;
			break;
		case S_HOUR:
			if (token != Tokenizer::T_SPECIAL || *strToken.get() != L':')
				return QSTATUS_FAIL;
			state = S_HOURSEP;
			break;
		case S_HOURSEP:
			if (token != Tokenizer::T_ATOM)
				return QSTATUS_FAIL;
			status = getMinute(strToken.get(), bAllowSingleDigitTime, &nMinute);
			CHECK_QSTATUS();
			state = S_MINUTE;
			break;
		case S_MINUTE:
			if (token == Tokenizer::T_SPECIAL && *strToken.get() == L':') {
				state = S_MINUTESEP;
			}
			else if (token == Tokenizer::T_ATOM) {
				status = getTimeZone(strToken.get(), &nTimeZone);
				CHECK_QSTATUS();
				state = S_TIMEZONE;
			}
			else {
				return QSTATUS_FAIL;
			}
			break;
		case S_MINUTESEP:
			if (token != Tokenizer::T_ATOM)
				return QSTATUS_FAIL;
			status = getSecond(strToken.get(), bAllowSingleDigitTime, &nSecond);
			CHECK_QSTATUS();
			state = S_SECOND;
			break;
		case S_SECOND:
			if (token != Tokenizer::T_ATOM)
				return QSTATUS_FAIL;
			status = getTimeZone(strToken.get(), &nTimeZone);
			CHECK_QSTATUS();
			state = S_TIMEZONE;
			break;
		case S_TIMEZONE:
			if (token != Tokenizer::T_END)
				return QSTATUS_FAIL;
			state = S_END;
			break;
		case S_END:
			break;
		}
	}
	
	if (nDay > Time::getDayCount(nYear, nMonth))
		return QSTATUS_FAIL;
	if (nWeek == -1)
		nWeek = Time::getDayOfWeek(nYear, nMonth, nDay);
	
	pTime->wYear = nYear;
	pTime->wMonth = nMonth;
	pTime->wDayOfWeek = nWeek;
	pTime->wDay = nDay;
	pTime->wHour = nHour;
	pTime->wMinute = nMinute;
	pTime->wSecond = nSecond;
	pTime->wMilliseconds = 0;
	pTime->setTimeZone(nTimeZone);
	
	pTime->addHour(-nTimeZone/100);
	pTime->addMinute(-nTimeZone%100);
	
	return QSTATUS_SUCCESS;
}

QSTATUS qs::DateParser::getWeek(const CHAR* psz, int* pnWeek)
{
	assert(psz);
	assert(pnWeek);
	
	*pnWeek = 0;
	
	const CHAR* pszWeeks[] = {
		"sun",
		"mon",
		"tue",
		"wed",
		"thu",
		"fri",
		"sat"
	};
	for (int n = 0; n < countof(pszWeeks); ++n) {
		if (_stricmp(psz, pszWeeks[n]) == 0) {
			*pnWeek = n;
			break;
		}
	}
	if (n == countof(pszWeeks))
		return QSTATUS_FAIL;
	
	return QSTATUS_SUCCESS;
}

QSTATUS qs::DateParser::getDay(const CHAR* psz, int* pnDay)
{
	assert(psz);
	assert(pnDay);
	
	*pnDay = 0;
	
	size_t nLen = strlen(psz);
	if (nLen == 0 || nLen > 2 || !isDigit(psz))
		return QSTATUS_FAIL;
	
	CHAR* pEnd = 0;
	*pnDay = strtol(psz, &pEnd, 10);
	
	return QSTATUS_SUCCESS;
}

QSTATUS qs::DateParser::getMonth(const CHAR* psz, int* pnMonth)
{
	assert(psz);
	assert(pnMonth);
	
	*pnMonth = 0;
	
	const CHAR* pszMonths[] = {
		"jan",
		"feb",
		"mar",
		"apr",
		"may",
		"jun",
		"jul",
		"aug",
		"sep",
		"oct",
		"nov",
		"dec"
	};
	for (int n = 0; n < countof(pszMonths); ++n) {
		if (_stricmp(psz, pszMonths[n]) == 0) {
			*pnMonth = n + 1;
			break;
		}
	}
	if (n == countof(pszMonths))
		return QSTATUS_FAIL;
	
	return QSTATUS_SUCCESS;
}

QSTATUS qs::DateParser::getYear(const CHAR* psz, int* pnYear)
{
	assert(psz);
	assert(pnYear);
	
	*pnYear = 0;
	
	if (!isDigit(psz))
		return QSTATUS_FAIL;
	
	CHAR* pEnd = 0;
	*pnYear = strtol(psz, &pEnd, 10);
	
	size_t nLen = strlen(psz);
	if (nLen == 2)
		*pnYear += *pnYear >= 50 ? 1900 : 2000;
	else if (nLen != 4)
		return QSTATUS_FAIL;
	
	return QSTATUS_SUCCESS;
}

QSTATUS qs::DateParser::getHour(const CHAR* psz,
	bool bAllowSingleDigit, int* pnHour)
{
	assert(psz);
	assert(pnHour);
	
	*pnHour = 0;
	
	size_t nLen = strlen(psz);
	if ((nLen != 2 && (!bAllowSingleDigit || nLen != 1)) || !isDigit(psz))
		return QSTATUS_FAIL;
	
	CHAR* pEnd = 0;
	*pnHour = strtol(psz, &pEnd, 10);
	if (*pnHour < 0 || 23 < *pnHour)
		return QSTATUS_FAIL;
	
	return QSTATUS_SUCCESS;
}

QSTATUS qs::DateParser::getMinute(const CHAR* psz,
	bool bAllowSingleDigit, int* pnMinute)
{
	assert(psz);
	assert(pnMinute);
	
	*pnMinute = 0;
	
	size_t nLen = strlen(psz);
	if ((nLen != 2 && (!bAllowSingleDigit || nLen != 1)) || !isDigit(psz))
		return QSTATUS_FAIL;
	
	CHAR* pEnd = 0;
	*pnMinute = strtol(psz, &pEnd, 10);
	if (*pnMinute < 0 || 59 < *pnMinute)
		return QSTATUS_FAIL;
	
	return QSTATUS_SUCCESS;
}

QSTATUS qs::DateParser::getSecond(const CHAR* psz,
	bool bAllowSingleDigit, int* pnSecond)
{
	assert(psz);
	assert(pnSecond);
	
	*pnSecond = 0;
	
	size_t nLen = strlen(psz);
	if ((nLen != 2 && (!bAllowSingleDigit || nLen != 1)) || !isDigit(psz))
		return QSTATUS_FAIL;
	
	CHAR* pEnd = 0;
	*pnSecond = strtol(psz, &pEnd, 10);
	if (*pnSecond < 0 || 60 < *pnSecond)
		return QSTATUS_FAIL;
	
	return QSTATUS_SUCCESS;
}

QSTATUS qs::DateParser::getTimeZone(const CHAR* psz, int* pnTimeZone)
{
	assert(psz);
	assert(pnTimeZone);
	
	struct {
		const CHAR* pszName_;
		int nTimeZone_;
	} zones[] = {
		{ "ut",		0		},
		{ "gmt",	0		},
		{ "est",	-500	},
		{ "edt",	-400	},
		{ "cst",	-600	},
		{ "cdt",	-500	},
		{ "mst",	-700	},
		{ "mdt",	-600	},
		{ "pst",	-800	},
		{ "pdt",	-700	},
		{ "jst",	900		},
		{ "utc",	0		}
	};
	
	if (*psz == '+' || *psz == '-') {
		if (strlen(psz) != 5 || !isDigit(psz + 1))
			return QSTATUS_FAIL;
		CHAR* pEnd = 0;
		*pnTimeZone = strtol(psz + 1, &pEnd, 10);
		if (*psz == '-')
			*pnTimeZone = -*pnTimeZone;
	}
	else if (strlen(psz) == 1) {
		if (*psz == 'j')
			return QSTATUS_FAIL;
		*pnTimeZone = 0;
	}
	else {
		for (int n = 0; n < countof(zones); ++n) {
			if (_stricmp(psz, zones[n].pszName_) == 0) {
				*pnTimeZone = zones[n].nTimeZone_;
				break;
			}
		}
		if (n == countof(zones))
//			return QSTATUS_FAIL;
			*pnTimeZone = 0;
	}
	
	return QSTATUS_SUCCESS;
}

bool qs::DateParser::isDigit(const CHAR* psz)
{
	while (*psz) {
		if (*psz < '0' || '9' < *psz)
			return false;
		++psz;
	}
	return true;
}


/****************************************************************************
 *
 * AddressParser
 *
 */

qs::AddressParser::AddressParser(unsigned int nFlags, QSTATUS* pstatus) :
	nFlags_(nFlags),
	wstrPhrase_(0),
	wstrMailbox_(0),
	wstrHost_(0),
	pGroup_(0)
{
}

qs::AddressParser::AddressParser(const WCHAR* pwszPhrase,
	const WCHAR* pwszAddress, QSTATUS* pstatus) :
	nFlags_(0),
	wstrPhrase_(0),
	wstrMailbox_(0),
	wstrHost_(0),
	pGroup_(0)
{
	assert(pwszAddress);
	
	*pstatus = QSTATUS_OUTOFMEMORY;
	
	if (pwszPhrase) {
		wstrPhrase_ = allocWString(pwszPhrase);
		if (!wstrPhrase_)
			return;
	}
	
	const WCHAR* p = wcsrchr(pwszAddress, L'@');
	if (p) {
		wstrMailbox_ = allocWString(pwszAddress, p - pwszAddress);
		if (!wstrMailbox_)
			return;
		wstrHost_ = allocWString(p + 1);
		if (!wstrHost_)
			return;
	}
	else {
		wstrMailbox_ = allocWString(pwszAddress);
		if (!wstrMailbox_)
			return;
	}
	
	*pstatus = QSTATUS_SUCCESS;
}

qs::AddressParser::AddressParser(const WCHAR* pwszPhrase,
	const WCHAR* pwszMailbox, const WCHAR* pwszHost, QSTATUS* pstatus) :
	nFlags_(0),
	wstrPhrase_(0),
	wstrMailbox_(0),
	wstrHost_(0),
	pGroup_(0)
{
	assert(pwszMailbox);
	
	*pstatus = QSTATUS_OUTOFMEMORY;
	
	if (pwszPhrase) {
		wstrPhrase_ = allocWString(pwszPhrase);
		if (!wstrPhrase_)
			return;
	}
	
	wstrMailbox_ = allocWString(pwszMailbox);
	if (!wstrMailbox_)
		return;
	
	if (pwszHost) {
		wstrHost_ = allocWString(pwszHost);
		if (!wstrHost_)
			return;
	}
	
	*pstatus = QSTATUS_SUCCESS;
}

qs::AddressParser::~AddressParser()
{
	freeWString(wstrPhrase_);
	freeWString(wstrMailbox_);
	freeWString(wstrHost_);
	delete pGroup_;
}

const WCHAR* qs::AddressParser::getPhrase() const
{
	return wstrPhrase_;
}

const WCHAR* qs::AddressParser::getMailbox() const
{
	return wstrMailbox_;
}

const WCHAR* qs::AddressParser::getHost() const
{
	return wstrHost_;
}

AddressListParser* qs::AddressParser::getGroup() const
{
	return pGroup_;
}

QSTATUS qs::AddressParser::getAddress(WSTRING* pwstrAddress) const
{
	assert(pwstrAddress);
	
	DECLARE_QSTATUS();
	
	string_ptr<STRING> strAddrSpec;
	status = getAddrSpec(wstrMailbox_, wstrHost_, &strAddrSpec);
	CHECK_QSTATUS();
	
	*pwstrAddress = mbs2wcs(strAddrSpec.get());
	if (!*pwstrAddress)
		return QSTATUS_OUTOFMEMORY;
	
	return QSTATUS_SUCCESS;
}

QSTATUS qs::AddressParser::getValue(WSTRING* pwstrValue) const
{
	assert(pwstrValue);
	
	DECLARE_QSTATUS();
	
	*pwstrValue = 0;
	
	const WCHAR* pwszPhrase = wstrPhrase_;
	string_ptr<WSTRING> wstrPhrase;
	if (pwszPhrase) {
		if (isAscii(pwszPhrase, static_cast<size_t>(-1))) {
			string_ptr<STRING> str(wcs2mbs(pwszPhrase));
			if (!str.get())
				return QSTATUS_OUTOFMEMORY;
			string_ptr<STRING> strAtoms;
			status = getAtomsOrQString(str.get(),
				static_cast<size_t>(-1), &strAtoms);
			CHECK_QSTATUS();
			wstrPhrase.reset(mbs2wcs(strAtoms.get()));
			if (!wstrPhrase.get())
				return QSTATUS_OUTOFMEMORY;
			pwszPhrase = wstrPhrase.get();
		}
	}
	
	StringBuffer<WSTRING> buf(&status);
	CHECK_QSTATUS();
	if (pGroup_) {
		assert(pwszPhrase);
		status = buf.append(pwszPhrase);
		CHECK_QSTATUS();
		status = buf.append(L": ");
		CHECK_QSTATUS();
		string_ptr<WSTRING> wstrValue;
		status = pGroup_->getValue(&wstrValue);
		CHECK_QSTATUS();
		status = buf.append(wstrValue.get());
		CHECK_QSTATUS();
		status = buf.append(L";");
		CHECK_QSTATUS();
	}
	else {
		if (pwszPhrase) {
			status = buf.append(pwszPhrase);
			CHECK_QSTATUS();
			status = buf.append(L" <");
			CHECK_QSTATUS();
		}
		
		string_ptr<STRING> strAddrSpec;
		status = getAddrSpec(wstrMailbox_, wstrHost_, &strAddrSpec);
		CHECK_QSTATUS();
		string_ptr<WSTRING> wstrAddrSpec(mbs2wcs(strAddrSpec.get()));
		if (!wstrAddrSpec.get())
			return QSTATUS_OUTOFMEMORY;
		status = buf.append(wstrAddrSpec.get());
		CHECK_QSTATUS();
		
		if (pwszPhrase) {
			status = buf.append(L">");
			CHECK_QSTATUS();
		}
	}
	
	*pwstrValue = buf.getString();
	
	return QSTATUS_SUCCESS;
}

QSTATUS qs::AddressParser::setPhrase(const WCHAR* pwszPhrase)
{
	string_ptr<WSTRING> wstrPhrase(allocWString(pwszPhrase));
	if (!wstrPhrase.get())
		return QSTATUS_OUTOFMEMORY;
	
	freeWString(wstrPhrase_);
	wstrPhrase_ = wstrPhrase.release();
	
	return QSTATUS_SUCCESS;
}

QSTATUS qs::AddressParser::parse(const Part& part,
	const WCHAR* pwszName, Part::Field* pField)
{
	assert(pwszName);
	assert(pField);
	
	DECLARE_QSTATUS();
	
	*pField = Part::FIELD_ERROR;
	
	string_ptr<STRING> strValue;
	bool bExist = false;
	status = part.getRawField(pwszName, 0, &strValue, &bExist);
	CHECK_QSTATUS();
	if (!bExist) {
		*pField = Part::FIELD_NOTEXIST;
		return QSTATUS_SUCCESS;
	}
	
	unsigned int nFlags = Tokenizer::F_RECOGNIZECOMMENT |
		Tokenizer::F_RECOGNIZEDOMAIN | Tokenizer::F_SPECIAL;
	Tokenizer t(strValue.get(), static_cast<size_t>(-1), nFlags, &status);
	CHECK_QSTATUS();
	
	status = parseAddress(part, t, pField, 0);
	CHECK_QSTATUS();
	
	return QSTATUS_SUCCESS;
}

QSTATUS qs::AddressParser::unparse(const Part& part, STRING* pstrValue) const
{
	assert(pstrValue);
	
	DECLARE_QSTATUS();
	
	StringBuffer<STRING> buf(&status);
	CHECK_QSTATUS();
	
	if (wstrPhrase_) {
		if (isAscii(wstrPhrase_)) {
			string_ptr<STRING> str(wcs2mbs(wstrPhrase_));
			if (!str.get())
				return QSTATUS_OUTOFMEMORY;
			string_ptr<STRING> strAtoms;
			status = getAtomsOrQString(str.get(),
				static_cast<size_t>(-1), &strAtoms);
			CHECK_QSTATUS();
			status = buf.append(strAtoms.get());
			CHECK_QSTATUS();
		}
		else {
			string_ptr<WSTRING> wstrCharset;
			status = part.getHeaderCharset(&wstrCharset);
			CHECK_QSTATUS();
			
			string_ptr<STRING> str;
			status = encode(wstrPhrase_, static_cast<size_t>(-1),
				wstrCharset.get(), 0, true, &str);
			CHECK_QSTATUS();
			status = buf.append(str.get());
			CHECK_QSTATUS();
		}
	}
	
	if (pGroup_) {
		status = buf.append(": ");
		CHECK_QSTATUS();
		string_ptr<STRING> str;
		status = pGroup_->unparse(part, &str);
		CHECK_QSTATUS();
		status = buf.append(str.get());
		CHECK_QSTATUS();
		status = buf.append(';');
		CHECK_QSTATUS();
	}
	else {
		if (wstrPhrase_) {
			status = buf.append(" <");
			CHECK_QSTATUS();
		}
		
		string_ptr<STRING> strAddrSpec;
		status = getAddrSpec(wstrMailbox_, wstrHost_, &strAddrSpec);
		CHECK_QSTATUS();
		status = buf.append(strAddrSpec.get());
		CHECK_QSTATUS();
		
		if (wstrPhrase_) {
			status = buf.append('>');
			CHECK_QSTATUS();
		}
	}
	*pstrValue = buf.getString();
	
	return QSTATUS_SUCCESS;
}

QSTATUS qs::AddressParser::isNeedQuoteMailbox(const CHAR* pszMailbox, bool* pbNeed)
{
	assert(pszMailbox);
	assert(pbNeed);
	
	DECLARE_QSTATUS();
	
	*pbNeed = true;
	
	Tokenizer t(pszMailbox, static_cast<size_t>(-1),
		Tokenizer::F_SPECIAL, &status);
	
	bool bDot = true;
	Tokenizer::Token token;
	while (true) {
		string_ptr<STRING> strToken;
		status = t.getToken(&token, &strToken);
		CHECK_QSTATUS();
		if (token == Tokenizer::T_END)
			break;
		
		if (bDot) {
			if (token != Tokenizer::T_ATOM)
				return QSTATUS_SUCCESS;
		}
		else {
			if (token != Tokenizer::T_SPECIAL || *strToken.get() != '.')
				return QSTATUS_SUCCESS;
		}
		bDot = !bDot;
	}
	if (bDot)
		return QSTATUS_SUCCESS;
	
	*pbNeed = false;
	
	return QSTATUS_SUCCESS;
}

QSTATUS qs::AddressParser::getAddrSpec(const CHAR* pszMailbox,
	const CHAR* pszHost, STRING* pstrAddrSpec)
{
	assert(pszMailbox);
	assert(pstrAddrSpec);
	
	DECLARE_QSTATUS();
	
	*pstrAddrSpec = 0;
	
	StringBuffer<STRING> buf(&status);
	CHECK_QSTATUS();
	
	bool bNeedQuote = true;
	status = isNeedQuoteMailbox(pszMailbox, &bNeedQuote);
	CHECK_QSTATUS();
	if (bNeedQuote) {
		string_ptr<STRING> strQuotedMailbox;
		status = getQString(pszMailbox,
			static_cast<size_t>(-1), &strQuotedMailbox);
		CHECK_QSTATUS();
		status = buf.append(strQuotedMailbox.get());
		CHECK_QSTATUS();
	}
	else {
		status = buf.append(pszMailbox);
		CHECK_QSTATUS();
	}
	
	if (pszHost) {
		status = buf.append('@');
		CHECK_QSTATUS();
		status = buf.append(pszHost);
		CHECK_QSTATUS();
	}

	*pstrAddrSpec = buf.getString();
	
	return QSTATUS_SUCCESS;
}

QSTATUS qs::AddressParser::getAddrSpec(const WCHAR* pwszMailbox,
	const WCHAR* pwszHost, STRING* pstrAddrSpec)
{
	assert(pwszMailbox);
	assert(pstrAddrSpec);
	
	string_ptr<STRING> strMailbox(wcs2mbs(pwszMailbox));
	if (!strMailbox.get())
		return QSTATUS_OUTOFMEMORY;
	string_ptr<STRING> strHost;
	if (pwszHost) {
		strHost.reset(wcs2mbs(pwszHost));
		if (!strHost.get())
			return QSTATUS_OUTOFMEMORY;
	}
	
	return getAddrSpec(strMailbox.get(), strHost.get(), pstrAddrSpec);
}

QSTATUS qs::AddressParser::parseAddress(const Part& part,
	Tokenizer& t, Part::Field* pField, bool* pbEnd)
{
	assert(pField);
	assert((nFlags_ & FLAG_INGROUP && pbEnd) ||
		(!(nFlags_ & FLAG_INGROUP) && !pbEnd));
	
	DECLARE_QSTATUS();
	
	*pField = Part::FIELD_ERROR;
	if (pbEnd)
		*pbEnd = false;
	
	bool bDisallowGroup = (nFlags_ & FLAG_DISALLOWGROUP) != 0;
	bool bInGroup = (nFlags_ & FLAG_INGROUP) != 0;
	State state = S_BEGIN;
	string_ptr<STRING> strComment;
	Phrases phrases;
	STLWrapper<Phrases> wrapper(phrases);
	AddrSpecParser addrSpec(&status);
	CHECK_QSTATUS();
	
	struct Deleter
	{
		typedef std::vector<std::pair<STRING, bool> > Phrases;
		Deleter(Phrases& phrases) : phrases_(phrases) {}
		~Deleter() { clear(); }
		void clear()
		{
			std::for_each(phrases_.begin(), phrases_.end(),
				unary_compose_f_gx(
					string_free<STRING>(),
					std::select1st<Phrases::value_type>()));
			phrases_.clear();
		}
		Phrases& phrases_;
	} deleter(phrases);
	
	while (state != S_END) {
		Tokenizer::Token token;
		string_ptr<STRING> strToken;
		status = t.getToken(&token, &strToken);
		CHECK_QSTATUS();
		if (token == Tokenizer::T_COMMENT) {
			strComment.reset(strToken.release());
			continue;
		}
		switch (state) {
		case S_BEGIN:
			switch (token) {
			case Tokenizer::T_END:
				if (bInGroup)
					return parseError();
				state = S_END;
				break;
			case Tokenizer::T_SPECIAL:
				if (*strToken.get() == ';' && bInGroup) {
					if (pbEnd)
						*pbEnd = true;
					state = S_END;
				}
				else if (*strToken.get() == '<') {
					state = S_LEFTANGLE;
				}
				else if (*strToken.get() != ',') {
					return parseError();
				}
				break;
			case Tokenizer::T_ATOM:
			case Tokenizer::T_QSTRING:
				status = wrapper.push_back(std::make_pair(
					strToken.get(), token == Tokenizer::T_ATOM));
				CHECK_QSTATUS();
				strToken.release();
				state = S_PHRASE;
				break;
			default:
				return parseError();
			}
			break;
		case S_PHRASE:
			switch (token) {
			case Tokenizer::T_SPECIAL:
				if (*strToken.get() == '.') {
					string_ptr<STRING> str(allocString("."));
					if (!str.get())
						return QSTATUS_OUTOFMEMORY;
					status = wrapper.push_back(std::make_pair(str.get(), true));
					CHECK_QSTATUS();
					str.release();
				}
				else if (*strToken.get() == '<') {
					state = S_LEFTANGLE;
				}
				else if (*strToken.get() == ':' && !bDisallowGroup) {
					status = newQsObject(AddressListParser::FLAG_GROUP, &pGroup_);
					CHECK_QSTATUS();
					status = pGroup_->parseAddressList(part, t, pField);
					CHECK_QSTATUS();
					if (*pField != Part::FIELD_EXIST)
						return parseError();
					state = S_SEMICOLON;
				}
				else if (*strToken.get() == ';') {
					state = S_SEMICOLON;
				}
				else if ((*strToken.get() == ',' ||
					(*strToken.get() == ';' && bInGroup)) &&
					part.isOption(Part::O_ALLOW_ADDRESS_WITHOUT_DOMAIN)) {
					string_ptr<STRING> strMailbox;
					status = getMailboxFromPhrases(phrases,
						part.isOption(Part::O_ALLOW_INVALID_PERIOD_IN_LOCALPART),
						&strMailbox);
					CHECK_QSTATUS_VALUE(parseError());
					deleter.clear();
					
					wstrMailbox_ = mbs2wcs(strMailbox.get());
					if (!wstrMailbox_)
						return QSTATUS_OUTOFMEMORY;
					if (pbEnd)
						*pbEnd = *strToken.get() == ';';
					state = S_END;
				}
				else if (*strToken.get() == '@') {
					string_ptr<STRING> strMailbox;
					status = getMailboxFromPhrases(phrases,
						part.isOption(Part::O_ALLOW_INVALID_PERIOD_IN_LOCALPART),
						&strMailbox);
					CHECK_QSTATUS_VALUE(parseError());
					deleter.clear();
					
					string_ptr<STRING> strTemp;
					string_ptr<STRING> strHost;
					string_ptr<STRING> strAddrSpecComment;
					status = addrSpec.parseAddrSpec(part, t, AddrSpecParser::S_ADDRSPECAT,
						bInGroup ? AddrSpecParser::TYPE_INGROUP : AddrSpecParser::TYPE_NORMAL,
						&strTemp, &strHost, &strAddrSpecComment, pField, pbEnd);
					CHECK_QSTATUS();
					if (*pField != Part::FIELD_EXIST)
						return parseError();
					assert(!*strTemp.get());
					if (strAddrSpecComment.get())
						strComment.reset(strAddrSpecComment.release());
					
					wstrMailbox_ = mbs2wcs(strMailbox.get());
					if (!wstrMailbox_)
						return QSTATUS_OUTOFMEMORY;
					wstrHost_ = mbs2wcs(strHost.get());
					if (!wstrHost_)
						return QSTATUS_OUTOFMEMORY;
					state = S_END;
				}
				else {
					return parseError();
				}
				break;
			case Tokenizer::T_ATOM:
			case Tokenizer::T_QSTRING:
				status = wrapper.push_back(std::make_pair(
					strToken.get(), token == Tokenizer::T_ATOM));
				CHECK_QSTATUS();
				strToken.release();
				break;
			case Tokenizer::T_END:
				if (!bInGroup && part.isOption(Part::O_ALLOW_ADDRESS_WITHOUT_DOMAIN)) {
					string_ptr<STRING> strMailbox;
					status = getMailboxFromPhrases(phrases,
						part.isOption(Part::O_ALLOW_INVALID_PERIOD_IN_LOCALPART),
						&strMailbox);
					CHECK_QSTATUS_VALUE(parseError());
					deleter.clear();
					
					wstrMailbox_ = mbs2wcs(strMailbox.get());
					if (!wstrMailbox_)
						return QSTATUS_OUTOFMEMORY;
					state = S_END;
					break;
				}
			default:
				return parseError();
			}
			break;
		case S_LEFTANGLE:
			switch (token) {
			case Tokenizer::T_SPECIAL:
				if (*strToken.get() != '@')
					return parseError();
				state = S_ROUTEAT;
				break;
			case Tokenizer::T_QSTRING:
			case Tokenizer::T_ATOM:
				{
					string_ptr<STRING> strMailbox;
					string_ptr<STRING> strHost;
					string_ptr<STRING> strAddrSpecComment;
					status = addrSpec.parseAddrSpec(part, t,
						AddrSpecParser::S_LOCALPARTWORD,
						AddrSpecParser::TYPE_INBRACKET, &strMailbox,
						&strHost, &strAddrSpecComment, pField, 0);
					CHECK_QSTATUS();
					if (*pField != Part::FIELD_EXIST)
						return parseError();
					string_ptr<STRING> strCompleteMailbox(
						concat(strToken.get(), strMailbox.get()));
					if (!strCompleteMailbox.get())
						return QSTATUS_OUTOFMEMORY;
					wstrMailbox_ = mbs2wcs(strCompleteMailbox.get());
					if (!wstrMailbox_)
						return QSTATUS_OUTOFMEMORY;
					if (*strHost.get()) {
						wstrHost_ = mbs2wcs(strHost.get());
						if (!wstrHost_)
							return QSTATUS_OUTOFMEMORY;
					}
					if (strAddrSpecComment.get())
						strComment.reset(strAddrSpecComment.release());
				}
				state = S_RIGHTANGLE;
				break;
			default:
				return parseError();
			}
			break;
		case S_RIGHTANGLE:
			if (token == Tokenizer::T_END) {
				if (bInGroup)
					return parseError();
			}
			else if (token == Tokenizer::T_SPECIAL) {
				if (*strToken.get() == ',') {
				}
				else if (!bInGroup && *strToken.get() == ';') {
					if (pbEnd)
						*pbEnd = true;
				}
				else {
					return parseError();
				}
			}
			else {
				return parseError();
			}
			state = S_END;
			break;
		case S_ROUTEAT:
			switch (token) {
			case Tokenizer::T_ATOM:
			case Tokenizer::T_DOMAIN:
				break;
			default:
				return parseError();
			}
			state = S_ROUTEDOMAIN;
			break;
		case S_ROUTEDOMAIN:
			if (token != Tokenizer::T_SPECIAL)
				return parseError();
			if (*strToken.get() == ',')
				state = S_ROUTECANMA;
			else if (*strToken.get() == ':')
				state = S_ROUTECOLON;
			else
				return parseError();
			break;
		case S_ROUTECANMA:
			if (token != Tokenizer::T_SPECIAL)
				return parseError();
			if (*strToken.get() == '@')
				state = S_ROUTEAT;
			else if (*strToken.get() == ',')
				state = S_ROUTECANMA;
			else
				return parseError();
			break;
		case S_ROUTECOLON:
			if (token == Tokenizer::T_ATOM || token == Tokenizer::T_QSTRING) {
				string_ptr<STRING> strMailbox;
				string_ptr<STRING> strHost;
				string_ptr<STRING> strAddrSpecComment;
				status = addrSpec.parseAddrSpec(part, t,
					AddrSpecParser::S_LOCALPARTWORD,
					AddrSpecParser::TYPE_INBRACKET, &strMailbox,
					&strHost, &strAddrSpecComment, pField, 0);
				CHECK_QSTATUS();
				if (*pField != Part::FIELD_EXIST)
					return parseError();
				wstrMailbox_ = mbs2wcs(strMailbox.get());
				if (!wstrMailbox_)
					return QSTATUS_OUTOFMEMORY;
				if (strHost.get()) {
					wstrHost_ = mbs2wcs(strHost.get());
					if (!wstrHost_)
						return QSTATUS_OUTOFMEMORY;
				}
				if (strAddrSpecComment.get())
					strComment.reset(strAddrSpecComment.release());
			}
			else {
				return parseError();
			}
			state = S_RIGHTANGLE;
			break;
		case S_SEMICOLON:
			switch (token) {
			case Tokenizer::T_END:
				state = S_END;
				break;
			case Tokenizer::T_SPECIAL:
				if (*strToken.get() == ',')
					state = S_END;
				else
					return parseError();
				break;
			default:
				return parseError();
			}
			break;
		case S_END:
			break;
		default:
			return parseError();
		}
	}
	
	bool bPrevDecode = false;
	bool bPrevPeriod = false;
	StringBuffer<WSTRING> bufPhrase(&status);
	CHECK_QSTATUS();
	Phrases::iterator it = phrases.begin();
	while (it != phrases.end()) {
		bool bDecode = false;
		bool bPeriod = (*it).second && *(*it).first == '.';
		string_ptr<WSTRING> wstrWord;
		status = decodePhrase((*it).first, (*it).second,
			part.isOption(Part::O_ALLOW_ENCODED_QSTRING), &wstrWord, &bDecode);
		CHECK_QSTATUS();
		if (it != phrases.begin()) {
			if ((!bPrevDecode || !bDecode) && !bPeriod && !bPrevPeriod) {
				status = bufPhrase.append(L" ");
				CHECK_QSTATUS();
			}
		}
		status = bufPhrase.append(wstrWord.get());
		CHECK_QSTATUS();
		bPrevDecode = bDecode;
		bPrevPeriod = bPeriod;
		++it;
	}
	if (bufPhrase.getLength() != 0)
		wstrPhrase_ = bufPhrase.getString();
	
	if (!wstrPhrase_ && strComment.get() &&
		part.isOption(Part::O_USE_COMMENT_AS_PHRASE)) {
		status = decode(strComment.get() + 1,
			strlen(strComment.get()) - 2, &wstrPhrase_, 0);
		CHECK_QSTATUS();
	}
	
	*pField = Part::FIELD_EXIST;
	
	return QSTATUS_SUCCESS;
}

QSTATUS qs::AddressParser::decodePhrase(const CHAR* psz, bool bAtom,
	bool bAllowEncodedQString, WSTRING* pwstrDecoded, bool* pbDecode)
{
	assert(psz);
	assert(pwstrDecoded);
	assert(pbDecode);
	
	DECLARE_QSTATUS();
	
	if (bAtom || bAllowEncodedQString) {
		status = decode(psz, static_cast<size_t>(-1),
			pwstrDecoded, pbDecode);
		CHECK_QSTATUS();
		*pbDecode = *pbDecode && bAtom;
	}
	else {
		*pwstrDecoded = mbs2wcs(psz);
		if (!*pwstrDecoded)
			return QSTATUS_OUTOFMEMORY;
	}
	
	return QSTATUS_SUCCESS;
}

QSTATUS qs::AddressParser::getMailboxFromPhrases(const Phrases& phrases,
	bool bAllowInvalidPeriod, STRING* pstrMailbox)
{
	assert(!phrases.empty());
	assert(pstrMailbox);
	
	DECLARE_QSTATUS();
	
	StringBuffer<STRING> buf(&status);
	CHECK_QSTATUS();
	
	bool bPeriod = true;
	Phrases::const_iterator it = phrases.begin();
	while (it != phrases.end()) {
		if (!bAllowInvalidPeriod) {
			if (bPeriod) {
				if ((*it).second && *(*it).first == '.')
					return QSTATUS_FAIL;
				bPeriod = false;
			}
			else {
				if (!(*it).second || *(*it).first != '.')
					return QSTATUS_FAIL;
				bPeriod = true;
			}
		}
		
		status = buf.append((*it).first);
		CHECK_QSTATUS();
		++it;
	}
	if (!bAllowInvalidPeriod && bPeriod)
		return QSTATUS_FAIL;
	
	*pstrMailbox = buf.getString();
	
	return QSTATUS_SUCCESS;
}


/****************************************************************************
 *
 * AddressListParser
 *
 */

qs::AddressListParser::AddressListParser(unsigned int nFlags, QSTATUS* pstatus) :
	nFlags_(nFlags)
{
}

qs::AddressListParser::~AddressListParser()
{
	removeAllAddresses();
}

QSTATUS qs::AddressListParser::getValue(WSTRING* pwstrValue) const
{
	assert(pwstrValue);
	
	DECLARE_QSTATUS();
	
	StringBuffer<WSTRING> buf(&status);
	CHECK_QSTATUS();
	
	AddressList::const_iterator it = listAddress_.begin();
	while (it != listAddress_.end()) {
		if (it != listAddress_.begin()) {
			status = buf.append(L", ");
			CHECK_QSTATUS();
		}
		
		string_ptr<WSTRING> strValue;
		status = (*it)->getValue(&strValue);
		CHECK_QSTATUS();
		
		status = buf.append(strValue.get());
		CHECK_QSTATUS();
		
		++it;
	}
	*pwstrValue = buf.getString();
	
	return QSTATUS_SUCCESS;
}

QSTATUS qs::AddressListParser::getNames(WSTRING* pwstrNames) const
{
	assert(pwstrNames);
	
	DECLARE_QSTATUS();
	
	StringBuffer<WSTRING> buf(&status);
	CHECK_QSTATUS();
	
	AddressList::const_iterator it = listAddress_.begin();
	while (it != listAddress_.end()) {
		if (buf.getLength() != 0) {
			status = buf.append(L", ");
			CHECK_QSTATUS();
		}
		if ((*it)->getPhrase()) {
			status = buf.append((*it)->getPhrase());
			CHECK_QSTATUS();
		}
		else {
			status = buf.append((*it)->getMailbox());
			CHECK_QSTATUS();
			if ((*it)->getHost()) {
				status = buf.append(L'@');
				CHECK_QSTATUS();
				status = buf.append((*it)->getHost());
				CHECK_QSTATUS();
			}
		}
		++it;
	}
	*pwstrNames = buf.getString();
	
	return QSTATUS_SUCCESS;
}

QSTATUS qs::AddressListParser::getAddresses(WSTRING* pwstrAddresses) const
{
	assert(pwstrAddresses);
	
	DECLARE_QSTATUS();
	
	StringBuffer<WSTRING> buf(&status);
	CHECK_QSTATUS();
	
	AddressList::const_iterator it = listAddress_.begin();
	while (it != listAddress_.end()) {
		AddressParser* pAddress = *it;
		const AddressListParser* pGroup = pAddress->getGroup();
		if (pGroup) {
			const AddressListParser::AddressList& groups = pGroup->getAddressList();
			AddressListParser::AddressList::const_iterator itG = groups.begin();
			while (itG != groups.end()) {
				if (buf.getLength() != 0) {
					status = buf.append(L",");
					CHECK_QSTATUS();
				}
				string_ptr<WSTRING> wstrAddress;
				status = (*itG)->getAddress(&wstrAddress);
				CHECK_QSTATUS();
				status = buf.append(wstrAddress.get());
				CHECK_QSTATUS();
				++itG;
			}
		}
		else {
			if (buf.getLength() != 0) {
				status = buf.append(L",");
				CHECK_QSTATUS();
			}
			string_ptr<WSTRING> wstrAddress;
			status = pAddress->getAddress(&wstrAddress);
			CHECK_QSTATUS();
			status = buf.append(wstrAddress.get());
			CHECK_QSTATUS();
		}
		++it;
	}
	
	*pwstrAddresses = buf.getString();
	
	return QSTATUS_SUCCESS;
}

const AddressListParser::AddressList& qs::AddressListParser::getAddressList() const
{
	return listAddress_;
}

QSTATUS qs::AddressListParser::appendAddress(AddressParser* pAddress)
{
	return STLWrapper<AddressList>(listAddress_).push_back(pAddress);
}

QSTATUS qs::AddressListParser::insertAddress(
	AddressParser* pAddressRef, AddressParser* pAddress)
{
	AddressList::iterator it = std::find(listAddress_.begin(),
		listAddress_.end(), pAddressRef);
	if (it == listAddress_.end())
		return QSTATUS_FAIL;
	AddressList::iterator itInsert;
	return STLWrapper<AddressList>(listAddress_).insert(it, pAddress, &itInsert);
}

QSTATUS qs::AddressListParser::removeAddress(AddressParser* pAddress)
{
	AddressList::iterator it = std::find(listAddress_.begin(),
		listAddress_.end(), pAddress);
	if (it == listAddress_.end())
		return QSTATUS_FAIL;
	listAddress_.erase(it);
	return QSTATUS_SUCCESS;
}

void qs::AddressListParser::removeAllAddresses()
{
	std::for_each(listAddress_.begin(), listAddress_.end(),
		deleter<AddressParser>());
	listAddress_.clear();
}

QSTATUS qs::AddressListParser::replaceAddress(
	AddressParser* pAddressOld, AddressParser* pAddressNew)
{
	AddressList::iterator it = std::find(listAddress_.begin(),
		listAddress_.end(), pAddressOld);
	if (it == listAddress_.end())
		return QSTATUS_FAIL;
	*it = pAddressNew;
	return QSTATUS_SUCCESS;
}

QSTATUS qs::AddressListParser::parse(const Part& part,
	const WCHAR* pwszName, Part::Field* pField)
{
	assert(pwszName);
	assert(pField);
	
	DECLARE_QSTATUS();
	
	*pField = Part::FIELD_ERROR;
	
	string_ptr<STRING> strValue;
	bool bExist = false;
	status = part.getRawField(pwszName, 0, &strValue, &bExist);
	CHECK_QSTATUS();
	if (!bExist) {
		*pField = Part::FIELD_NOTEXIST;
		return QSTATUS_SUCCESS;
	}
	
	if (!(nFlags_ & FLAG_SINGLEFIELD)) {
		StringBuffer<STRING> buf(strValue.get(), &status);
		CHECK_QSTATUS();
		unsigned int n = 1;
		bool bExist = true;
		while (bExist) {
			string_ptr<STRING> str;
			status = part.getRawField(pwszName, n, &str, &bExist);
			CHECK_QSTATUS();
			if (bExist) {
				status = buf.append(", ");
				CHECK_QSTATUS();
				status = buf.append(str.get());
				CHECK_QSTATUS();
			}
			++n;
		}
		strValue.reset(buf.getString());
	}
	
	unsigned int nFlags = Tokenizer::F_RECOGNIZECOMMENT |
		Tokenizer::F_RECOGNIZEDOMAIN | Tokenizer::F_SPECIAL;
	Tokenizer t(strValue.get(), static_cast<size_t>(-1), nFlags, &status);
	CHECK_QSTATUS();
	
	status = parseAddressList(part, t, pField);
	CHECK_QSTATUS();
	
	return QSTATUS_SUCCESS;
}

QSTATUS qs::AddressListParser::unparse(const Part& part, STRING* pstrValue) const
{
	assert(pstrValue);
	
	DECLARE_QSTATUS();
	
	StringBuffer<STRING> buf(&status);
	CHECK_QSTATUS();
	
	AddressList::const_iterator it = listAddress_.begin();
	while (it != listAddress_.end()) {
		if (it != listAddress_.begin()) {
			status = buf.append(",\r\n\t");
			CHECK_QSTATUS();
		}
		
		string_ptr<STRING> str;
		status = (*it)->unparse(part, &str);
		CHECK_QSTATUS();
		
		status = buf.append(str.get());
		CHECK_QSTATUS();
		
		++it;
	}
	
	*pstrValue = buf.getString();
	
	return QSTATUS_SUCCESS;
}

QSTATUS qs::AddressListParser::parseAddressList(
	const Part& part, Tokenizer& t, Part::Field* pField)
{
	assert(pField);
	
	DECLARE_QSTATUS();
	
	*pField = Part::FIELD_ERROR;
	
	unsigned int nFlags = 0;
	if (nFlags_ & FLAG_DISALLOWGROUP)
		nFlags |= AddressParser::FLAG_DISALLOWGROUP;
	if (nFlags_ & FLAG_GROUP)
		nFlags |= AddressParser::FLAG_INGROUP;
	
	STLWrapper<AddressList> wrapper(listAddress_);
	
	struct Deleter
	{
		Deleter(AddressListParser::AddressList& l) : p_(&l) {}
		~Deleter()
		{
			if (p_) {
				std::for_each(p_->begin(), p_->end(),
					deleter<AddressParser>());
				p_->clear();
			}
		}
		void release() { p_ = 0; }
		AddressListParser::AddressList* p_;
	} deleter(listAddress_);
	
	while (true) {
		std::auto_ptr<AddressParser> pParser;
		status = newQsObject(nFlags, &pParser);
		CHECK_QSTATUS();
		
		bool bEnd = false;
		status = pParser->parseAddress(part, t, pField,
			nFlags_ & FLAG_GROUP ? &bEnd : 0);
		CHECK_QSTATUS();
		if (*pField != Part::FIELD_EXIST)
			return QSTATUS_SUCCESS;
		
		if (!pParser->getMailbox() && !pParser->getGroup())
			break;
		
		status = wrapper.push_back(pParser.get());
		CHECK_QSTATUS();
		pParser.release();
		
		if (bEnd)
			break;
	}
	deleter.release();
	*pField = Part::FIELD_EXIST;
	
	return QSTATUS_SUCCESS;
}


/****************************************************************************
 *
 * MessageIdParser
 *
 */

qs::MessageIdParser::MessageIdParser(QSTATUS* pstatus) :
	wstrMessageId_(0)
{
	assert(pstatus);
	*pstatus = QSTATUS_SUCCESS;
}

qs::MessageIdParser::MessageIdParser(const WCHAR* pwszMessageId, QSTATUS* pstatus) :
	wstrMessageId_(0)
{
	assert(pstatus);
	
	*pstatus = QSTATUS_SUCCESS;
	
	string_ptr<WSTRING> wstrMessageId(allocWString(pwszMessageId));
	if (!wstrMessageId.get()) {
		*pstatus = QSTATUS_OUTOFMEMORY;
		return;
	}
	wstrMessageId_ = wstrMessageId.release();
}

qs::MessageIdParser::~MessageIdParser()
{
	freeWString(wstrMessageId_);
}

const WCHAR* qs::MessageIdParser::getMessageId() const
{
	return wstrMessageId_;
}

QSTATUS qs::MessageIdParser::parse(const Part& part,
	const WCHAR* pwszName, Part::Field* pField)
{
	assert(pwszName);
	assert(pField);
	
	DECLARE_QSTATUS();
	
	*pField = Part::FIELD_ERROR;
	
	string_ptr<STRING> strValue;
	bool bExist = false;
	status = part.getRawField(pwszName, 0, &strValue, &bExist);
	CHECK_QSTATUS();
	if (!bExist) {
		*pField = Part::FIELD_NOTEXIST;
		return QSTATUS_SUCCESS;
	}
	
	Tokenizer t(strValue.get(), static_cast<size_t>(-1),
		Tokenizer::F_RECOGNIZECOMMENT | Tokenizer::F_SPECIAL, &status);
	CHECK_QSTATUS();
	State state = S_BEGIN;
	string_ptr<STRING> strMessageId;
	while (state != S_END) {
		Tokenizer::Token token;
		string_ptr<STRING> strToken;
		status = t.getToken(&token, &strToken);
		CHECK_QSTATUS();
		
		if (token == Tokenizer::T_COMMENT)
			continue;
		
		switch (state) {
		case S_BEGIN:
			if (token != Tokenizer::T_SPECIAL || *strToken.get() != '<') {
				return parseError();
			}
			else {
				AddrSpecParser addrSpec(&status);
				CHECK_QSTATUS();
				string_ptr<STRING> strMailbox;
				string_ptr<STRING> strHost;
				string_ptr<STRING> strComment;
				status = addrSpec.parseAddrSpec(part,
					t, AddrSpecParser::S_BEGIN,
					AddrSpecParser::TYPE_INBRACKET, &strMailbox,
					&strHost, &strComment, pField, 0);
				CHECK_QSTATUS();
				if (*pField != Part::FIELD_EXIST)
					return parseError();
				status = AddressParser::getAddrSpec(
					strMailbox.get(), strHost.get(), &strMessageId);
				CHECK_QSTATUS();
				state = S_ADDRSPEC;
			}
			break;
		case S_ADDRSPEC:
			if (token != Tokenizer::T_END)
				return parseError();
			state = S_END;
			break;
		case S_END:
			break;
		}
	}
	
	wstrMessageId_ = mbs2wcs(strMessageId.get());
	if (!wstrMessageId_)
		return QSTATUS_OUTOFMEMORY;
	
	*pField = Part::FIELD_EXIST;
	
	return QSTATUS_SUCCESS;
}

QSTATUS qs::MessageIdParser::unparse(const Part& part, STRING* pstrValue) const
{
	assert(pstrValue);
	assert(wstrMessageId_);
	
	if (!isAscii(wstrMessageId_))
		return QSTATUS_FAIL;
	
	string_ptr<STRING> str(wcs2mbs(wstrMessageId_));
	if (!str.get())
		return QSTATUS_OUTOFMEMORY;
	
	*pstrValue = concat("<", str.get(), ">");
	if (!*pstrValue)
		return QSTATUS_OUTOFMEMORY;
	
	return QSTATUS_SUCCESS;
}


/****************************************************************************
 *
 * ReferencesParser
 *
 */

qs::ReferencesParser::ReferencesParser(QSTATUS* pstatus)
{
	assert(pstatus);
	*pstatus = QSTATUS_SUCCESS;
}

qs::ReferencesParser::~ReferencesParser()
{
	std::for_each(listReference_.begin(), listReference_.end(),
		unary_compose_f_gx(
			string_free<WSTRING>(),
			std::select1st<ReferenceList::value_type>()));
}

const ReferencesParser::ReferenceList& qs::ReferencesParser::getReferences() const
{
	return listReference_;
}

QSTATUS qs::ReferencesParser::parse(const Part& part,
	const WCHAR* pwszName, Part::Field* pField)
{
	assert(pwszName);
	assert(pField);
	
	DECLARE_QSTATUS();
	
	*pField = Part::FIELD_ERROR;
	
	string_ptr<STRING> strValue;
	bool bExist = false;
	status = part.getRawField(pwszName, 0, &strValue, &bExist);
	CHECK_QSTATUS();
	if (!bExist) {
		*pField = Part::FIELD_NOTEXIST;
		return QSTATUS_SUCCESS;
	}
	
	Tokenizer t(strValue.get(), static_cast<size_t>(-1),
		Tokenizer::F_RECOGNIZECOMMENT | Tokenizer::F_SPECIAL, &status);
	CHECK_QSTATUS();
	
	STLWrapper<ReferenceList> wrapper(listReference_);
	AddrSpecParser addrSpec(&status);
	CHECK_QSTATUS();
	
	State state = S_BEGIN;
	StringBuffer<STRING> buf(&status);
	CHECK_QSTATUS();
	while (state != S_END) {
		Tokenizer::Token token;
		string_ptr<STRING> strToken;
		status = t.getToken(&token, &strToken);
		CHECK_QSTATUS();
		
		if (token == Tokenizer::T_COMMENT)
			continue;
		
		switch (state) {
		case S_BEGIN:
			if (token == Tokenizer::T_ATOM || token == Tokenizer::T_QSTRING) {
				if (buf.getLength() != 0) {
					status = buf.append(' ');
					CHECK_QSTATUS();
				}
				status = buf.append(strToken.get());
				CHECK_QSTATUS();
			}
			else if (token == Tokenizer::T_SPECIAL && *strToken.get() == '<') {
				if (buf.getLength() != 0) {
					string_ptr<WSTRING> str(mbs2wcs(buf.getCharArray()));
					if (!str.get())
						return QSTATUS_OUTOFMEMORY;
					status = wrapper.push_back(std::make_pair(str.get(), T_PHRASE));
					CHECK_QSTATUS();
					str.release();
					status = buf.remove();
					CHECK_QSTATUS();
				}
				
				string_ptr<STRING> strMailbox;
				string_ptr<STRING> strHost;
				string_ptr<STRING> strComment;
				status = addrSpec.parseAddrSpec(part,
					t, AddrSpecParser::S_BEGIN,
					AddrSpecParser::TYPE_INBRACKET, &strMailbox,
					&strHost, &strComment, pField, 0);
				CHECK_QSTATUS();
				if (*pField != Part::FIELD_EXIST)
					return parseError();
				string_ptr<STRING> strAddrSpec;
				status = AddressParser::getAddrSpec(
					strMailbox.get(), strHost.get(), &strAddrSpec);
				CHECK_QSTATUS();
				string_ptr<WSTRING> str(mbs2wcs(strAddrSpec.get()));
				if (!str.get())
					return QSTATUS_OUTOFMEMORY;
				status = wrapper.push_back(std::make_pair(str.get(), T_MSGID));
				CHECK_QSTATUS();
				str.release();
			}
			else if (token == Tokenizer::T_SPECIAL &&
				part.isOption(Part::O_ALLOW_SPECIALS_IN_REFERENCES)) {
				if (buf.getLength() != 0) {
					status = buf.append(' ');
					CHECK_QSTATUS();
				}
				status = buf.append(strToken.get());
				CHECK_QSTATUS();
			}
			else if (token == Tokenizer::T_END) {
				if (buf.getLength() != 0) {
					string_ptr<WSTRING> str(mbs2wcs(buf.getCharArray()));
					if (!str.get())
						return QSTATUS_OUTOFMEMORY;
					status = wrapper.push_back(std::make_pair(str.get(), T_PHRASE));
					CHECK_QSTATUS();
					str.release();
					status = buf.remove();
					CHECK_QSTATUS();
				}
				state = S_END;
			}
			else {
				return parseError();
			}
			break;
		case S_END:
			break;
		}
	}
	*pField = Part::FIELD_EXIST;
	
	return QSTATUS_SUCCESS;
}

QSTATUS qs::ReferencesParser::unparse(const Part& part, STRING* pstrValue) const
{
	assert(pstrValue);
	
	DECLARE_QSTATUS();
	
	StringBuffer<STRING> buf(&status);
	CHECK_QSTATUS();
	
	ReferenceList::const_iterator it = listReference_.begin();
	while (it != listReference_.end()) {
		if (it != listReference_.begin()) {
			status = buf.append("\r\n\t");
			CHECK_QSTATUS();
		}
		assert(isAscii((*it).first));
		
		string_ptr<STRING> str(wcs2mbs((*it).first));
		if (!str.get())
			return QSTATUS_OUTOFMEMORY;
		
		switch ((*it).second) {
		case T_MSGID:
			status = buf.append('<');
			CHECK_QSTATUS();
			status = buf.append(str.get());
			CHECK_QSTATUS();
			status = buf.append('>');
			CHECK_QSTATUS();
			break;
		case T_PHRASE:
			{
				string_ptr<STRING> strAtoms;
				status = getAtomsOrQString(str.get(),
					static_cast<size_t>(-1), &strAtoms);
				CHECK_QSTATUS();
				status = buf.append(strAtoms.get());
				CHECK_QSTATUS();
			}
			break;
		default:
			assert(false);
			break;
		}
		++it;
	}
	
	*pstrValue = buf.getString();
	
	return QSTATUS_SUCCESS;
}


/****************************************************************************
 *
 * ParameterFieldParser
 *
 */

qs::ParameterFieldParser::ParameterFieldParser(QSTATUS* pstatus)
{
	assert(pstatus);
	*pstatus = QSTATUS_SUCCESS;
}

qs::ParameterFieldParser::~ParameterFieldParser()
{
	std::for_each(listParameter_.begin(), listParameter_.end(),
		unary_compose_fx_gx(
			string_free<WSTRING>(),
			string_free<WSTRING>()));
}

QSTATUS qs::ParameterFieldParser::getParameter(
	const WCHAR* pwszName, WSTRING* pwstrValue) const
{
	assert(pwszName);
	assert(pwstrValue);
	
	*pwstrValue = 0;
	
	DECLARE_QSTATUS();
	
	const WCHAR* pwszValue = getRawParameter(pwszName);
	if (!pwszValue) {
		string_ptr<WSTRING> wstrName(concat(pwszName, L"*"));
		if (!wstrName.get())
			return QSTATUS_OUTOFMEMORY;
		pwszValue = getRawParameter(wstrName.get());
		std::auto_ptr<Converter> pConverter;
		string_ptr<STRING> strDecode;
		if (pwszValue) {
			Converter* p = 0;
			status = decode2231FirstValue(pwszValue, &strDecode, &p);
			CHECK_QSTATUS();
			pConverter.reset(p);
		}
		else {
			bool bFirstExtended = false;
			string_ptr<WSTRING> wstrName(concat(pwszName, L"*0"));
			if (!wstrName.get())
				return QSTATUS_OUTOFMEMORY;
			pwszValue = getRawParameter(wstrName.get());
			if (pwszValue) {
				strDecode.reset(wcs2mbs(pwszValue));
				if (!strDecode.get())
					return QSTATUS_OUTOFMEMORY;
			}
			else {
				string_ptr<WSTRING> wstrName(concat(pwszName, L"*0*"));
				if (!wstrName.get())
					return QSTATUS_OUTOFMEMORY;
				pwszValue = getRawParameter(wstrName.get());
				if (pwszValue) {
					Converter* p = 0;
					status = decode2231FirstValue(pwszValue, &strDecode, &p);
					CHECK_QSTATUS();
					bFirstExtended = true;
					pConverter.reset(p);
				}
			}
			if (pwszValue) {
				string_ptr<WSTRING> wstrName(allocWString(wcslen(pwszName) + 10));
				if (!wstrName.get())
					return QSTATUS_OUTOFMEMORY;
				StringBuffer<STRING> buf(strDecode.get(), &status);
				CHECK_QSTATUS();
				bool bExistOther = true;
				for (int n = 1; bExistOther; ++n) {
					for (int m = 0; m < (bFirstExtended ? 2 : 1); ++m) {
						bool bExtended = m == 1;
						swprintf(wstrName.get(), L"%s*%d%c", pwszName, n,
							bExtended ? L'*' : L'\0');
						const WCHAR* pwsz = getRawParameter(wstrName.get());
						bExistOther = pwsz != 0;
						if (pwsz) {
							if (bExtended) {
								string_ptr<STRING> str;
								status = decode2231Value(pwsz, &str);
								CHECK_QSTATUS();
								status = buf.append(str.get());
								CHECK_QSTATUS();
							}
							else {
								string_ptr<STRING> str(wcs2mbs(pwsz));
								if (!str.get())
									return QSTATUS_OUTOFMEMORY;
								status = buf.append(str.get());
								CHECK_QSTATUS();
							}
							break;
						}
					}
				}
				strDecode.reset(buf.getString());
			}
		}
		if (pwszValue) {
			if (pConverter.get()) {
				size_t nLen = strlen(strDecode.get());
				status = pConverter->decode(strDecode.get(),
					&nLen, pwstrValue, 0);
				CHECK_QSTATUS();
			}
			else {
				*pwstrValue = mbs2wcs(strDecode.get());
				if (!*pwstrValue)
					return QSTATUS_OUTOFMEMORY;
			}
		}
	}
	else {
		*pwstrValue = allocWString(pwszValue);
		if (!*pwstrValue)
			return QSTATUS_OUTOFMEMORY;
	}
	
	return QSTATUS_SUCCESS;
}

QSTATUS qs::ParameterFieldParser::setParameter(
	const WCHAR* pwszName, const WCHAR* pwszValue)
{
	return setRawParameter(pwszName, pwszValue);
}

QSTATUS qs::ParameterFieldParser::parseParameter(
	const Part& part, Tokenizer& t, State state, Part::Field* pField)
{
	assert(pField);
	
	DECLARE_QSTATUS();
	
	*pField = Part::FIELD_ERROR;
	
	STLWrapper<ParameterList> wrapper(listParameter_);
	
	string_ptr<STRING> strName;
	while (state != S_END) {
		Tokenizer::Token token;
		string_ptr<STRING> strToken;
		status = t.getToken(&token, &strToken);
		CHECK_QSTATUS();
		
		if (token == Tokenizer::T_COMMENT)
			continue;
		
		switch (state) {
		case S_BEGIN:
		case S_VALUE:
			if (token == Tokenizer::T_END)
				state = S_END;
			else if (token == Tokenizer::T_SPECIAL && *strToken.get() == ';')
				state = S_SEMICOLON;
			else
				return parseError();
			break;
		case S_SEMICOLON:
			if (token == Tokenizer::T_END &&
				part.isOption(Part::O_ALLOW_PARAMETER_INVALID_SEMICOLON)) {
				state = S_END;
			}
			else if (token == Tokenizer::T_SPECIAL &&
				*strToken.get() == ';' &&
				part.isOption(Part::O_ALLOW_PARAMETER_INVALID_SEMICOLON)) {
				state = S_SEMICOLON;
			}
			else if (token == Tokenizer::T_ATOM) {
				strName.reset(strToken.release());
				state = S_NAME;
			}
			else {
				return parseError();
			}
			break;
		case S_NAME:
			if (token != Tokenizer::T_SPECIAL || *strToken.get() != '=')
				return parseError();
			state = S_EQUAL;
			break;
		case S_EQUAL:
			if (token != Tokenizer::T_ATOM && token != Tokenizer::T_QSTRING) {
				return parseError();
			}
			else {
				string_ptr<WSTRING> wstrValue;
				if (part.isOption(Part::O_ALLOW_ENCODED_PARAMETER) &&
					(part.isOption(Part::O_ALLOW_ENCODED_QSTRING) || token == Tokenizer::T_ATOM)) {
					status = decode(strToken.get(),
						static_cast<size_t>(-1), &wstrValue, 0);
					CHECK_QSTATUS();
				}
				else {
					wstrValue.reset(mbs2wcs(strToken.get()));
					if (!wstrValue.get())
						return QSTATUS_OUTOFMEMORY;
				}
				string_ptr<WSTRING> wstrName(mbs2wcs(strName.get()));
				if (!wstrName.get())
					return QSTATUS_OUTOFMEMORY;
				status = wrapper.push_back(std::make_pair(
					wstrName.get(), wstrValue.get()));
				CHECK_QSTATUS();
				wstrName.release();
				wstrValue.release();
				state = S_VALUE;
			}
			break;
		case S_END:
			break;
		}
	}
	*pField = Part::FIELD_EXIST;
	
	return QSTATUS_SUCCESS;
}

QSTATUS qs::ParameterFieldParser::unparseParameter(
	const Part& part, STRING* pstrValue) const
{
	assert(pstrValue);
	
	DECLARE_QSTATUS();
	
	StringBuffer<STRING> buf(&status);
	CHECK_QSTATUS();
	
	ParameterList::const_iterator it = listParameter_.begin();
	while (it != listParameter_.end()) {
		if (!isAscii((*it).first))
			return QSTATUS_FAIL;
		
		const size_t nAsciiMax = 40;
		const size_t nNonAsciiMax = 10;
		string_ptr<STRING> strParamName(wcs2mbs((*it).first));
		if (!strParamName.get())
			return QSTATUS_OUTOFMEMORY;
		string_ptr<STRING> strParamValue;
		const WCHAR* pwsz = (*it).second;
		string_ptr<STRING> strValue(wcs2mbs(pwsz));
		if (!strValue.get())
			return QSTATUS_OUTOFMEMORY;
		size_t nLen = wcslen(pwsz);
		bool bAscii = isAscii(pwsz);
		bool bRFC2231Processed = false;
		if (part.isOption(Part::O_RFC2231) && (!bAscii || nLen > nAsciiMax)) {
			if (bAscii) {
				unsigned int n = 0;
				while (n < nLen/nAsciiMax + (nLen%nAsciiMax ? 1 : 0)) {
					status = buf.append(";\r\n\t");
					CHECK_QSTATUS();
					status = buf.append(strParamName.get());
					CHECK_QSTATUS();
					CHAR sz[32];
					sprintf(sz, "*%d=", n);
					status = buf.append(sz);
					CHECK_QSTATUS();
					
					const CHAR* p = strValue.get() + n*nAsciiMax;
					size_t nThisLen = QSMIN(nAsciiMax, strlen(p));
					string_ptr<STRING> strValue;
					if (part.isOption(Part::O_FORCE_QSTRING_PARAMETER)) {
						status = getQString(p, nThisLen, &strValue);
						CHECK_QSTATUS();
					}
					else {
						status = getAtomOrQString(p, nThisLen, &strValue);
						CHECK_QSTATUS();
					}
					status = buf.append(strValue.get());
					CHECK_QSTATUS();
					
					++n;
				}
			}
			else {
				string_ptr<WSTRING> wstrCharset;
				status = part.getHeaderCharset(&wstrCharset);
				CHECK_QSTATUS();
				
				std::auto_ptr<Converter> pConverter;
				status = ConverterFactory::getInstance(
					wstrCharset.get(), &pConverter);
				CHECK_QSTATUS();
				
				string_ptr<STRING> strValue;
				if (pConverter.get()) {
					strValue.reset(0);
					size_t nLen = wcslen(pwsz);
					status = pConverter->encode(pwsz, &nLen, &strValue, 0);
					CHECK_QSTATUS();
				}
				size_t nLen = strlen(strValue.get());
				
				unsigned int n = 0;
				while (n < nLen/nNonAsciiMax + (nLen%nNonAsciiMax ? 1 : 0)) {
					status = buf.append(";\r\n\t");
					CHECK_QSTATUS();
					status = buf.append(strParamName.get());
					CHECK_QSTATUS();
					if (nLen > nNonAsciiMax) {
						CHAR sz[32];
						sprintf(sz, "*%d", n);
						status = buf.append(sz);
						CHECK_QSTATUS();
					}
					status = buf.append("*=");
					CHECK_QSTATUS();
					
					if (n == 0) {
						string_ptr<STRING> strCharset(wcs2mbs(wstrCharset.get()));
						if (!strCharset.get())
							return QSTATUS_OUTOFMEMORY;
						status = buf.append(strCharset.get());
						CHECK_QSTATUS();
						status = buf.append("\'\'");
						CHECK_QSTATUS();
					}
					const CHAR* p = strValue.get() + n*nNonAsciiMax;
					size_t nThisLen = QSMIN(nNonAsciiMax, strlen(p));
					string_ptr<STRING> strEncoded;
					status = encode2231Value(p, nThisLen, &strEncoded);
					CHECK_QSTATUS();
					status = buf.append(strEncoded.get());
					CHECK_QSTATUS();
					
					++n;
				}
			}
			bRFC2231Processed = true;
		}
		else if (bAscii) {
			if (part.isOption(Part::O_FORCE_QSTRING_PARAMETER)) {
				status = getQString(strValue.get(),
					static_cast<size_t>(-1), &strParamValue);
				CHECK_QSTATUS();
			}
			else {
				status = getAtomOrQString(strValue.get(),
					static_cast<size_t>(-1), &strParamValue);
				CHECK_QSTATUS();
			}
		}
		else if (part.isOption(Part::O_ALLOW_ENCODED_PARAMETER)) {
			string_ptr<WSTRING> wstrCharset;
			status = part.getHeaderCharset(&wstrCharset);
			CHECK_QSTATUS();
			
			strValue.reset(0);
			status = encode(pwsz, static_cast<size_t>(-1),
				wstrCharset.get(), 0, true, &strValue);
			CHECK_QSTATUS();
			
			strParamValue.reset(concat("\"", strValue.get(), "\""));
			if (!strParamValue.get())
				return QSTATUS_OUTOFMEMORY;
		}
		
		if (!bRFC2231Processed) {
			status = buf.append(";\r\n\t");
			CHECK_QSTATUS();
			status = buf.append(strParamName.get());
			CHECK_QSTATUS();
			status = buf.append('=');
			CHECK_QSTATUS();
			status = buf.append(strParamValue.get());
			CHECK_QSTATUS();
		}
		
		++it;
	}
	*pstrValue = buf.getString();
	
	return QSTATUS_SUCCESS;
}

const WCHAR* qs::ParameterFieldParser::getRawParameter(const WCHAR* pwszName) const
{
	assert(pwszName);
	
	ParameterList::const_iterator it = listParameter_.begin();
	while (it != listParameter_.end()) {
		if (_wcsicmp((*it).first, pwszName) == 0)
			break;
		++it;
	}
	return it != listParameter_.end() ? (*it).second : 0;
}

QSTATUS qs::ParameterFieldParser::setRawParameter(
	const WCHAR* pwszName, const WCHAR* pwszValue)
{
	assert(pwszName);
	assert(pwszValue);
	
	DECLARE_QSTATUS();
	
	string_ptr<WSTRING> wstrValue(allocWString(pwszValue));
	if (!wstrValue.get())
		return QSTATUS_OUTOFMEMORY;
	
	ParameterList::iterator it = listParameter_.begin();
	while (it != listParameter_.end()) {
		if (_wcsicmp((*it).first, pwszName) == 0)
			break;
		++it;
	}
	if (it == listParameter_.end()) {
		string_ptr<WSTRING> wstrName(allocWString(pwszName));
		if (!wstrName.get())
			return QSTATUS_OUTOFMEMORY;
		status = STLWrapper<ParameterList>(listParameter_).push_back(
			std::make_pair(wstrName.get(), wstrValue.get()));
		CHECK_QSTATUS();
		wstrName.release();
		wstrValue.release();
	}
	else {
		freeWString((*it).second);
		(*it).second = wstrValue.release();
	}
	return QSTATUS_SUCCESS;
}

QSTATUS qs::ParameterFieldParser::decode2231FirstValue(
	const WCHAR* pwszValue, STRING* pstrDecode, Converter** ppConverter)
{
	assert(pwszValue);
	assert(pstrDecode);
	assert(ppConverter);
	
	DECLARE_QSTATUS();
	
	const WCHAR* p = wcschr(pwszValue, L'\'');
	if (!p || p == pwszValue)
		return QSTATUS_FAIL;
	
	string_ptr<WSTRING> wstrCharset(allocWString(pwszValue, p - pwszValue));
	if (!wstrCharset.get())
		return QSTATUS_OUTOFMEMORY;
	
	p = wcschr(p + 1, L'\'');
	if (!p)
		return QSTATUS_FAIL;
	
	std::auto_ptr<Converter> pConverter;
	status = ConverterFactory::getInstance(wstrCharset.get(), &pConverter);
	CHECK_QSTATUS();
	
	status = decode2231Value(p + 1, pstrDecode);
	CHECK_QSTATUS();
	
	*ppConverter = pConverter.release();
	
	return QSTATUS_SUCCESS;
}

QSTATUS qs::ParameterFieldParser::decode2231Value(
	const WCHAR* pwszValue, STRING* pstrDecoded)
{
	assert(pwszValue);
	assert(pstrDecoded);
	
	DECLARE_QSTATUS();
	
	StringBuffer<STRING> buf(&status);
	CHECK_QSTATUS();
	
	for (const WCHAR* p = pwszValue; *p; ++p) {
		CHAR c = static_cast<CHAR>(*p);
		if (c == '%' && isHex(*(p + 1)) && isHex(*(p + 2))) {
			status = buf.append(decodeHex(p + 1));
			CHECK_QSTATUS();
			p += 2;
		}
		else {
			status = buf.append(c);
			CHECK_QSTATUS();
		}
	}
	*pstrDecoded = buf.getString();
	
	return QSTATUS_SUCCESS;
}

QSTATUS qs::ParameterFieldParser::encode2231Value(
	const CHAR* pszValue, size_t nLen, STRING* pstrEncoded)
{
	assert(pszValue);
	assert(pstrEncoded);
	
	DECLARE_QSTATUS();
	
	StringBuffer<STRING> buf(&status);
	CHECK_QSTATUS();
	
	for (const CHAR* p = pszValue; p < pszValue + nLen; ++p) {
		unsigned char c = *p;
		if (('a' <= c && c <= 'z') || ('A' <= c && c <= 'Z') ||
			('0' <= c && c <= '9') || c == '.') {
			status = buf.append(c);
			CHECK_QSTATUS();
		}
		else {
			status = buf.append('%');
			CHECK_QSTATUS();
			CHAR sz[2];
			status = encodeHex(c, sz);
			CHECK_QSTATUS();
			status = buf.append(sz, 2);
			CHECK_QSTATUS();
		}
	}
	*pstrEncoded = buf.getString();
	
	return QSTATUS_SUCCESS;
}

bool qs::ParameterFieldParser::isHex(WCHAR c)
{
	return (L'0' <= c && c <= L'9') ||
		(L'A' <= c && c <= L'F')/* ||
		(L'a' <= c && c <= L'f')*/;
}

unsigned char qs::ParameterFieldParser::decodeHex(const WCHAR* pwszValue)
{
	unsigned char cValue = 0;
	for (const WCHAR* p = pwszValue; p < pwszValue + 2; ++p) {
		WCHAR c = *p;
		assert(isHex(c));
		cValue <<= 4;
		if (L'0' <= c && c <= L'9')
			cValue |= c - L'0';
		else if (L'A' <= c && c <= L'F')
			cValue |= c - L'A' + 10;
//		else if (L'a' <= c && c <= L'f')
//			cValue |= c - L'a' + 10;
		else
			assert(false);
	}
	return cValue;
}

QSTATUS qs::ParameterFieldParser::encodeHex(
	unsigned char c, CHAR* pszEncoded)
{
	assert(pszEncoded);
	
	for (int n = 0; n < 2; ++n) {
		unsigned char cValue = (n == 0 ? c >> 4 : c) & 0x0f;
		pszEncoded[n] = cValue < 10 ? cValue + L'0' : cValue - 10 + L'A';
	}
	
	return QSTATUS_SUCCESS;
}


qs::SimpleParameterParser::SimpleParameterParser(QSTATUS* pstatus) :
	ParameterFieldParser(pstatus),
	wstrValue_(0)
{
	if (*pstatus != QSTATUS_SUCCESS)
		return;
}

qs::SimpleParameterParser::~SimpleParameterParser()
{
	freeWString(wstrValue_);
}

const WCHAR* qs::SimpleParameterParser::getValue() const
{
	return wstrValue_;
}

QSTATUS qs::SimpleParameterParser::parse(const Part& part,
	const WCHAR* pwszName, Part::Field* pField)
{
	assert(pwszName);
	assert(pField);
	
	DECLARE_QSTATUS();
	
	*pField = Part::FIELD_ERROR;
	
	string_ptr<STRING> strValue;
	bool bExist = false;
	status = part.getRawField(pwszName, 0, &strValue, &bExist);
	CHECK_QSTATUS();
	if (!bExist) {
		*pField = Part::FIELD_NOTEXIST;
		return QSTATUS_SUCCESS;
	}
	
	Tokenizer t(strValue.get(), static_cast<size_t>(-1),
		Tokenizer::F_TSPECIAL, &status);
	CHECK_QSTATUS();
	
	StringBuffer<WSTRING> buf(&status);
	CHECK_QSTATUS();
	bool bLastSpecial = false;
	
	string_ptr<STRING> strToken;
	Tokenizer::Token token;
	status = t.getToken(&token, &strToken);
	CHECK_QSTATUS();
	
	while (true) {
		if (token == Tokenizer::T_END ||
			(token == Tokenizer::T_SPECIAL && *strToken.get() == ';')) {
			break;
		}
		else if (token != Tokenizer::T_COMMENT) {
			if (buf.getLength() != 0 && !bLastSpecial &&
				token != Tokenizer::T_SPECIAL) {
				status = buf.append(L" ");
				CHECK_QSTATUS();
			}
			string_ptr<WSTRING> str(mbs2wcs(strToken.get()));
			if (!str.get())
				return QSTATUS_OUTOFMEMORY;
			status = buf.append(str.get());
			CHECK_QSTATUS();
			bLastSpecial = token == Tokenizer::T_SPECIAL;
		}
		strToken.reset(0);
		status = t. getToken(&token, &strToken);
		CHECK_QSTATUS();
	}
	if (token != Tokenizer::T_END) {
		status = parseParameter(part, t,
			ParameterFieldParser::S_SEMICOLON, pField);
		CHECK_QSTATUS();
	}
	else {
		*pField = Part::FIELD_EXIST;
	}
	
	wstrValue_ = buf.getString();
	
	return QSTATUS_SUCCESS;
}

QSTATUS qs::SimpleParameterParser::unparse(
	const Part& part, STRING* pstrValue) const
{
	assert(pstrValue);
	
	DECLARE_QSTATUS();
	
	string_ptr<STRING> strValue(wcs2mbs(wstrValue_));
	if (!strValue.get())
		return QSTATUS_OUTOFMEMORY;
	
	string_ptr<STRING> strParameter;
	status = unparseParameter(part, &strParameter);
	CHECK_QSTATUS();
	
	*pstrValue = concat(strValue.get(), strParameter.get());
	CHECK_QSTATUS();
	
	return QSTATUS_SUCCESS;
}


/****************************************************************************
 *
 * ContentTypeParser
 *
 */

qs::ContentTypeParser::ContentTypeParser(QSTATUS* pstatus) :
	ParameterFieldParser(pstatus),
	wstrMediaType_(0),
	wstrSubType_(0)
{
	if (*pstatus != QSTATUS_SUCCESS)
		return;
}

qs::ContentTypeParser::ContentTypeParser(const WCHAR* pwszMediaType,
	const WCHAR* pwszSubType, QSTATUS* pstatus) :
	ParameterFieldParser(pstatus),
	wstrMediaType_(0),
	wstrSubType_(0)
{
	if (*pstatus != QSTATUS_SUCCESS)
		return;
	
	string_ptr<WSTRING> wstrMediaType(allocWString(pwszMediaType));
	if (!wstrMediaType.get()) {
		*pstatus = QSTATUS_SUCCESS;
		return;
	}
	
	string_ptr<WSTRING> wstrSubType(allocWString(pwszSubType));
	if (!wstrSubType.get()) {
		*pstatus = QSTATUS_SUCCESS;
		return;
	}
	
	wstrMediaType_ = wstrMediaType.release();
	wstrSubType_ = wstrSubType.release();
}

qs::ContentTypeParser::~ContentTypeParser()
{
	freeWString(wstrMediaType_);
	freeWString(wstrSubType_);
}

const WCHAR* qs::ContentTypeParser::getMediaType() const
{
	return wstrMediaType_;
}

const WCHAR* qs::ContentTypeParser::getSubType() const
{
	return wstrSubType_;
}

QSTATUS qs::ContentTypeParser::parse(const Part& part,
	const WCHAR* pwszName, Part::Field* pField)
{
	assert(pwszName);
	assert(pField);
	
	DECLARE_QSTATUS();
	
	*pField = Part::FIELD_ERROR;
	
	string_ptr<STRING> strValue;
	bool bExist = false;
	status = part.getRawField(pwszName, 0, &strValue, &bExist);
	CHECK_QSTATUS();
	if (!bExist) {
		*pField = Part::FIELD_NOTEXIST;
		return QSTATUS_SUCCESS;
	}
	
	Tokenizer t(strValue.get(), static_cast<size_t>(-1),
		Tokenizer::F_TSPECIAL | Tokenizer::F_RECOGNIZECOMMENT, &status);
	CHECK_QSTATUS();
	
	State state = S_BEGIN;
	while (state != S_END) {
		Tokenizer::Token token;
		string_ptr<STRING> strToken;
		status = t.getToken(&token, &strToken);
		CHECK_QSTATUS();
		
		if (token == Tokenizer::T_COMMENT)
			continue;
		
		switch (state) {
		case S_BEGIN:
			if (token != Tokenizer::T_ATOM)
				return parseError();
			wstrMediaType_ = mbs2wcs(strToken.get());
			if (!wstrMediaType_)
				return QSTATUS_OUTOFMEMORY;
			state = S_MEDIATYPE;
			break;
		case S_MEDIATYPE:
			if (token != Tokenizer::T_SPECIAL || *strToken.get() != '/')
				return parseError();
			state = S_SLASH;
			break;
		case S_SLASH:
			if (token != Tokenizer::T_ATOM)
				return parseError();
			wstrSubType_ = mbs2wcs(strToken.get());
			if (!wstrSubType_)
				return QSTATUS_OUTOFMEMORY;
			state = S_END;
			break;
		case S_END:
			break;
		}
	}
	status = parseParameter(part, t, ParameterFieldParser::S_BEGIN, pField);
	CHECK_QSTATUS();
	
	return QSTATUS_SUCCESS;
}

QSTATUS qs::ContentTypeParser::unparse(const Part& part, STRING* pstrValue) const
{
	assert(pstrValue);
	
	DECLARE_QSTATUS();
	
	string_ptr<STRING> strMediaType(wcs2mbs(wstrMediaType_));
	if (!strMediaType.get())
		return QSTATUS_OUTOFMEMORY;
	
	string_ptr<STRING> strSubType(wcs2mbs(wstrSubType_));
	if (!strSubType.get())
		return QSTATUS_OUTOFMEMORY;
	
	string_ptr<STRING> strParameter;
	status = unparseParameter(part, &strParameter);
	CHECK_QSTATUS();
	
	Concat c[] = {
		{ strMediaType.get(),	-1	},
		{ "/",					1	},
		{ strSubType.get(),		-1	},
		{ strParameter.get(),	-1	}
	};
	*pstrValue = concat(c, countof(c));
	if (!*pstrValue)
		return QSTATUS_OUTOFMEMORY;
	
	return QSTATUS_SUCCESS;
}


/****************************************************************************
 *
 * ContentDispositionParser
 *
 */

qs::ContentDispositionParser::ContentDispositionParser(QSTATUS* pstatus) :
	ParameterFieldParser(pstatus),
	wstrDispositionType_(0)
{
	if (*pstatus != QSTATUS_SUCCESS)
		return;
}

qs::ContentDispositionParser::ContentDispositionParser(
	const WCHAR* pwszDispositionType, QSTATUS* pstatus) :
	ParameterFieldParser(pstatus),
	wstrDispositionType_(0)
{
	if (*pstatus != QSTATUS_SUCCESS)
		return;
	
	wstrDispositionType_ = allocWString(pwszDispositionType);
	if (!wstrDispositionType_) {
		*pstatus = QSTATUS_OUTOFMEMORY;
		return;
	}
}

qs::ContentDispositionParser::~ContentDispositionParser()
{
	freeWString(wstrDispositionType_);
}

const WCHAR* qs::ContentDispositionParser::getDispositionType() const
{
	return wstrDispositionType_;
}

QSTATUS qs::ContentDispositionParser::parse(const Part& part,
	const WCHAR* pwszName, Part::Field* pField)
{
	assert(pwszName);
	assert(pField);
	
	*pField = Part::FIELD_ERROR;
	
	DECLARE_QSTATUS();
	
	string_ptr<STRING> strValue;
	bool bExist = false;
	status = part.getRawField(pwszName, 0, &strValue, &bExist);
	CHECK_QSTATUS();
	if (!bExist) {
		*pField = Part::FIELD_NOTEXIST;
		return QSTATUS_SUCCESS;
	}
	
	Tokenizer t(strValue.get(), static_cast<size_t>(-1),
		Tokenizer::F_TSPECIAL, &status);
	CHECK_QSTATUS();
	
	Tokenizer::Token token;
	string_ptr<STRING> strToken;
	status = t.getToken(&token, &strToken);
	CHECK_QSTATUS();
	if (token != Tokenizer::T_ATOM)
		return parseError();
	wstrDispositionType_ = mbs2wcs(strToken.get());
	if (!wstrDispositionType_)
		return QSTATUS_OUTOFMEMORY;
	
	status = parseParameter(part, t, ParameterFieldParser::S_BEGIN, pField);
	CHECK_QSTATUS();
	
	return QSTATUS_SUCCESS;
}

QSTATUS qs::ContentDispositionParser::unparse(
	const Part& part, STRING* pstrValue) const
{
	assert(pstrValue);
	
	DECLARE_QSTATUS();
	
	string_ptr<STRING> strDispositionType(wcs2mbs(wstrDispositionType_));
	if (!strDispositionType.get())
		return QSTATUS_OUTOFMEMORY;
	
	string_ptr<STRING> strParameter;
	status = unparseParameter(part, &strParameter);
	CHECK_QSTATUS();
	
	*pstrValue = concat(strDispositionType.get(), strParameter.get());
	CHECK_QSTATUS();
	
	return QSTATUS_SUCCESS;
}


/****************************************************************************
 *
 * ContentTransferEncodingParser
 *
 */

qs::ContentTransferEncodingParser::ContentTransferEncodingParser(
	QSTATUS* pstatus) :
	parser_(SimpleParser::FLAG_RECOGNIZECOMMENT | SimpleParser::FLAG_TSPECIAL,
		pstatus)
{
}

qs::ContentTransferEncodingParser::ContentTransferEncodingParser(
	const WCHAR* pwszEncoding, QSTATUS* pstatus) :
	parser_(pwszEncoding,
		SimpleParser::FLAG_RECOGNIZECOMMENT | SimpleParser::FLAG_TSPECIAL,
		pstatus)
{
}

qs::ContentTransferEncodingParser::~ContentTransferEncodingParser()
{
}

const WCHAR* qs::ContentTransferEncodingParser::getEncoding() const
{
	return parser_.getValue();
}

QSTATUS qs::ContentTransferEncodingParser::parse(const Part& part,
	const WCHAR* pwszName, Part::Field* pField)
{
	return parser_.parse(part, pwszName, pField);
}

QSTATUS qs::ContentTransferEncodingParser::unparse(
	const Part& part, STRING* pstrValue) const
{
	return parser_.unparse(part, pstrValue);
}
