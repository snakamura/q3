/*
 * $Id$
 *
 * Copyright(C) 1998-2006 Satoshi Nakamura
 * All rights reserved.
 *
 */

#pragma warning(disable:4786)

#include <qsmime.h>
#include <qsconv.h>
#include <qsstl.h>
#include <qsencoder.h>

#include <algorithm>
#include <cstdio>
#include <memory>

#include "mime.h"

using namespace qs;


/****************************************************************************
 *
 * Tokenizer
 *
 */

qs::Tokenizer::Tokenizer(const CHAR* psz,
						 size_t nLen,
						 unsigned int nFlags) :
	p_(0),
	nFlags_(nFlags)
{
	assert((nFlags_ & 0x0f00) == F_SPECIAL ||
		(nFlags_ & 0x0f00) == F_TSPECIAL ||
		(nFlags_ & 0x0f00) == F_ESPECIAL);
	
	if (nLen == -1)
		nLen = strlen(psz);
	
	str_ = allocString(psz, nLen);
	p_ = str_.get();
}

qs::Tokenizer::~Tokenizer()
{
}

Tokenizer::Token qs::Tokenizer::getToken()
{
	Token token(T_ERROR);
	
	while (isSpace(*p_))
		++p_;
	if (!*p_)
		return Token(T_END);
	
	StringBuffer<STRING> buf;
	if ((nFlags_ && F_RECOGNIZECOMMENT) && *p_ == '(') {
		int nComment = 1;
		buf.append(*p_);
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
					return token;
			}
			buf.append(*p_);
			++p_;
		}
		if (*(p_ - 1) != ')')
			return token;
		
		token.type_ = T_COMMENT;
		token.str_ = buf.getString();
	}
	else if ((nFlags_ && F_RECOGNIZEDOMAIN) && *p_ == '[') {
		buf.append(*p_);
		++p_;
		while (*p_ && *p_ != ']') {
			if (*p_ == '[') {
				return token;
			}
			else if (*p_ == '\\') {
				++p_;
				if (!*p_)
					return token;
			}
			buf.append(*p_);
			++p_;
		}
		if (!*p_)
			return token;
		assert(*p_ == ']');
		buf.append(*p_);
		++p_;
		
		token.type_ = T_DOMAIN;
		token.str_ = buf.getString();
	}
	else if (*p_ == '\"') {
		++p_;
		while (*p_ && *p_ != '\"') {
			if (*p_ == '\\') {
				++p_;
				if (!*p_)
					return token;
			}
			buf.append(*p_);
			++p_;
		}
		if (!*p_)
			return token;
		assert(*p_ == '\"');
		++p_;
		
		token.type_ = T_QSTRING;
		token.str_ = buf.getString();
	}
	else if (isSpecial(*p_, nFlags_)) {
		buf.append(*p_);
		++p_;
		
		token.type_ = T_SPECIAL;
		token.str_ = buf.getString();
	}
	else if (isCtl(*p_)) {
	}
	else {
		while (*p_ &&
			!isSpecial(*p_, nFlags_) &&
			!isCtl(*p_) &&
			!isSpace(*p_)) {
			buf.append(*p_);
			++p_;
		}
		
		token.type_ = T_ATOM;
		token.str_ = buf.getString();
	}
	
	return token;
}

bool qs::Tokenizer::isCtl(unsigned char c)
{
	return (0x00 <= c && c <= 0x1f) || c == 0x7f;
}

bool qs::Tokenizer::isSpace(unsigned char c)
{
	return c == 0x20 || c == 0x09;
}

bool qs::Tokenizer::isSpecial(unsigned char c,
							  unsigned int nFlags)
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
			c == '=' ||
			c == '\\')
			return true;
	}
	return false;
}


/****************************************************************************
 *
 * Tokenizer::Token
 *
 */

qs::Tokenizer::Token::Token(TokenType type) :
	type_(type)
{
}

qs::Tokenizer::Token::Token(TokenType type,
							string_ptr str) :
	type_(type),
	str_(str)
{
}

qs::Tokenizer::Token::Token(Token& token) :
	type_(token.type_),
	str_(token.str_)
{
}

Tokenizer::Token& qs::Tokenizer::Token::operator=(Token& token)
{
	if (&token != this) {
		type_ = token.type_;
		str_ = token.str_;
	}
	return *this;
}


/****************************************************************************
 *
 * AddrSpecParser
 *
 */

qs::AddrSpecParser::AddrSpecParser()
{
}

qs::AddrSpecParser::~AddrSpecParser()
{
}

Part::Field qs::AddrSpecParser::parseAddrSpec(const Part& part,
											  Tokenizer& t,
											  State state,
											  Type type,
											  string_ptr* pstrMailbox,
											  string_ptr* pstrHost,
											  string_ptr* pstrComment,
											  bool* pbEnd) const
{
	assert(pstrMailbox);
	assert(pstrHost);
	assert(pstrComment);
	assert((type == TYPE_INGROUP && pbEnd) || (type != TYPE_INGROUP && !pbEnd));
	
	CHAR szEnd[] = {
		type == TYPE_INBRACKET ? '>' : ',',
		type == TYPE_INGROUP ? ';' : '\0',
		'\0'
	};
	
	StringBuffer<STRING> bufMailbox;
	StringBuffer<STRING> bufHost;
	
	string_ptr strComment;
	while (state != S_END) {
		Tokenizer::Token token(t.getToken());
		if (token.type_ == Tokenizer::T_COMMENT) {
			strComment = token.str_;
			continue;
		}
		
		if (part.isOption(Part::O_ALLOW_INVALID_PERIOD_IN_LOCALPART)) {
			if (state == S_BEGIN || state == S_LOCALPARTPERIOD)
				state = S_LOCALPARTWORD;
		}
		
		switch (state) {
		case S_BEGIN:
		case S_LOCALPARTPERIOD:
			switch (token.type_) {
			case Tokenizer::T_ATOM:
			case Tokenizer::T_QSTRING:
				bufMailbox.append(token.str_.get());
				state = S_LOCALPARTWORD;
				break;
			default:
				return FieldParser::parseError();
			}
			break;
		case S_LOCALPARTWORD:
			switch (token.type_) {
			case Tokenizer::T_ATOM:
			case Tokenizer::T_QSTRING:
				if (part.isOption(Part::O_ALLOW_INVALID_PERIOD_IN_LOCALPART))
					bufMailbox.append(token.str_.get());
				else
					return FieldParser::parseError();
				break;
			case Tokenizer::T_SPECIAL:
				if (*token.str_.get() == '@') {
					state = S_ADDRSPECAT;
				}
				else if (*token.str_.get() == '.') {
					bufMailbox.append(token.str_.get());
					state = S_LOCALPARTPERIOD;
				}
				else if (strchr(szEnd, *token.str_.get()) &&
					part.isOption(Part::O_ALLOW_ADDRESS_WITHOUT_DOMAIN)) {
					if (pbEnd)
						*pbEnd = *token.str_.get() == ';';
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
			switch (token.type_) {
			case Tokenizer::T_DOMAIN:
			case Tokenizer::T_ATOM:
				bufHost.append(token.str_.get());
				state = S_SUBDOMAIN;
				break;
			default:
				return FieldParser::parseError();
			}
			break;
		case S_SUBDOMAIN:
			switch (token.type_) {
			case Tokenizer::T_END:
				if (type == TYPE_INGROUP)
					return FieldParser::parseError();
				state = S_END;
				break;
			case Tokenizer::T_SPECIAL:
				if (strchr(szEnd, *token.str_.get())) {
					if (pbEnd)
						*pbEnd = *token.str_.get() == ';';
					state = S_END;
				}
				else if (*token.str_.get() == '.') {
					bufHost.append(token.str_.get());
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
			if (token.type_ != Tokenizer::T_ATOM)
				return FieldParser::parseError();
			bufHost.append(token.str_.get());
			state = S_SUBDOMAIN;
			break;
		default:
			return FieldParser::parseError();
		}
	}
	*pstrMailbox = bufMailbox.getString();
	*pstrHost = bufHost.getString();
	*pstrComment = strComment;
	
	return Part::FIELD_EXIST;
}


/****************************************************************************
 *
 * MimeUtil
 *
 */

void MimeUtil::appendCharArrayReplaceNewLine(const CHAR* psz,
											 size_t nLen,
											 StringBuffer<WSTRING>* pBuf)
{
	assert(psz);
	assert(pBuf);
	
	for (size_t n = 0; n < nLen; ++n, ++psz)
		pBuf->append(*psz != '\n' && *psz != '\r' ? static_cast<WCHAR>(*psz) : L' ');
}


/****************************************************************************
 *
 * FieldParser
 *
 */

qs::FieldParser::~FieldParser()
{
}

wstring_ptr qs::FieldParser::decode(const CHAR* psz,
									size_t nLen,
									bool bAllowUTF8,
									bool* pbDecoded)
{
	assert(psz);
	
	if (pbDecoded)
		*pbDecoded = false;
	
	if (nLen == -1)
		nLen = strlen(psz);
	
	std::auto_ptr<Converter> pConverter;
	std::auto_ptr<Encoder> pEncoder;
	const CHAR* pEncodedPrefix = 0;
	size_t nEncodedPrefixLen = 0;
	bool bDecode = false;
	bool bDecoded = false;
	StringBuffer<WSTRING> space;
	StringBuffer<WSTRING> decoded;
	StringBuffer<STRING> buf;
	for (const CHAR* p = psz; p < psz + nLen; ++p) {
		if (bDecode) {
			if (*p == '?' && p + 1 < psz + nLen && *(p + 1) == '=') {
				if (space.getLength() != 0) {
					if (!bDecoded)
						decoded.append(space.getCharArray());
					space.remove();
				}
				
				assert(pConverter.get());
				assert(pEncoder.get());
				
				bool b = false;
				malloc_size_ptr<unsigned char> pDecoded(pEncoder->decode(
					reinterpret_cast<const unsigned char*>(buf.getCharArray()), buf.getLength()));
				if (pDecoded.get()) {
					size_t nLen = pDecoded.size();
					wxstring_size_ptr wstrConverted(pConverter->decode(
						reinterpret_cast<const CHAR*>(pDecoded.get()), &nLen));
					if (wstrConverted.get()) {
						WCHAR* p = wstrConverted.get();
						for (size_t n = 0; n < wstrConverted.size(); ++n, ++p) {
							if (*p == L'\n' || *p == L'\r')
								*p = L' ';
						}
						decoded.append(wstrConverted.get(), wstrConverted.size());
						b = true;
					}
				}
				if (!b) {
					MimeUtil::appendCharArrayReplaceNewLine(
						pEncodedPrefix, nEncodedPrefixLen, &decoded);
					MimeUtil::appendCharArrayReplaceNewLine(
						buf.getCharArray(), buf.getLength(), &decoded);
					decoded.append(L"?=");
				}
				buf.remove();
				
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
			if (*p == '=' && p + 1 < psz + nLen && *(p + 1) == '?') {
				if (buf.getLength() != 0) {
					assert(bAllowUTF8);
					size_t nLen = buf.getLength();
					wxstring_size_ptr wstr(UTF8Converter().decode(buf.getCharArray(), &nLen));
					if (wstr.get())
						decoded.append(wstr.get(), wstr.size());
					else
						MimeUtil::appendCharArrayReplaceNewLine(
							buf.getCharArray(), buf.getLength(), &decoded);
					buf.remove();
				}
				
				wstring_ptr wstrCharset;
				wstring_ptr wstrEncoding;
				const CHAR* pBegin = p + 2;
				const CHAR* pEnd = strchr(p + 2, '?');
				if (pEnd) {
					wstrCharset = mbs2wcs(pBegin, pEnd - pBegin);
					WCHAR* pLang = wcschr(wstrCharset.get(), L'*');
					if (pLang)
						*pLang = L'\0';
					
					pBegin = pEnd + 1;
					pEnd = strchr(pBegin, '?');
					if (pEnd)
						wstrEncoding = mbs2wcs(pBegin, pEnd - pBegin);
				}
				
				if (wstrCharset.get() && wstrEncoding.get()) {
					pConverter = ConverterFactory::getInstance(wstrCharset.get());
					pEncoder = EncoderFactory::getInstance(wstrEncoding.get());
				}
				if (pConverter.get() && pEncoder.get()) {
					pEncodedPrefix = p;
					nEncodedPrefixLen = pEnd - p + 1;
					bDecode = true;
					p = pEnd;
					space.remove();
					if (pbDecoded)
						*pbDecoded = true;
				}
				else {
					decoded.append(static_cast<WCHAR>(*p));
				}
			}
			else if (bDecoded && (*p == ' ' || *p == '\t')) {
				space.append(static_cast<WCHAR>(*p));
			}
			else {
				if (space.getLength() != 0) {
					decoded.append(space.getCharArray(), space.getLength());
					space.remove();
				}
				if (bAllowUTF8)
					buf.append(*p);
				else
					decoded.append(*p);
				bDecoded = false;
			}
		}
	}
	
	if (buf.getLength()) {
		if (bDecode) {
			MimeUtil::appendCharArrayReplaceNewLine(
				pEncodedPrefix, nEncodedPrefixLen, &decoded);
			MimeUtil::appendCharArrayReplaceNewLine(
				buf.getCharArray(), buf.getLength(), &decoded);
		}
		else {
			size_t nLen = buf.getLength();
			wxstring_size_ptr wstr(UTF8Converter().decode(buf.getCharArray(), &nLen));
			if (wstr.get())
				decoded.append(wstr.get(), wstr.size());
			else
				MimeUtil::appendCharArrayReplaceNewLine(
					buf.getCharArray(), buf.getLength(), &decoded);
		}
	}
	
	return decoded.getString();
}

string_ptr qs::FieldParser::encode(const WCHAR* pwsz,
								   size_t nLen,
								   const WCHAR* pwszCharset,
								   const WCHAR* pwszEncoding,
								   bool bOneBlock,
								   bool bFallbackToUtf8)
{
	string_ptr str = encode(pwsz, nLen, pwszCharset, pwszEncoding, bOneBlock);
	if (!str.get() && bFallbackToUtf8 && _wcsicmp(pwszCharset, L"utf-8") != 0)
		str = encode(pwsz, nLen, L"utf-8", L"B", bOneBlock);
	return str;
}

string_ptr qs::FieldParser::convertToUTF8(const CHAR* psz)
{
	std::auto_ptr<Converter> pConverter(
		ConverterFactory::getInstance(Part::getDefaultCharset()));
	if (!pConverter.get())
		return 0;
	
	size_t nEncodedLen = strlen(psz);
	wxstring_size_ptr decoded(pConverter->decode(psz, &nEncodedLen));
	if (!decoded.get())
		return 0;
	
	size_t nDecodedLen = decoded.size();
	xstring_size_ptr str(UTF8Converter().encode(decoded.get(), &nDecodedLen));
	return allocString(str.get(), str.size());
}

bool qs::FieldParser::isSpecial(CHAR c)
{
	return Tokenizer::isSpecial(c, Tokenizer::F_SPECIAL | Tokenizer::F_TSPECIAL | Tokenizer::F_ESPECIAL);
}

Part::Field qs::FieldParser::parseError()
{
	return Part::FIELD_ERROR;
}

string_ptr qs::FieldParser::encode(const WCHAR* pwsz,
								   size_t nLen,
								   const WCHAR* pwszCharset,
								   const WCHAR* pwszEncoding,
								   bool bOneBlock)
{
	assert(pwsz);
	assert(pwszCharset);
	assert(!pwszEncoding ||
		wcscmp(pwszEncoding, L"B") == 0 ||
		wcscmp(pwszEncoding, L"Q") == 0);
	
	if (nLen == -1)
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
	
	std::auto_ptr<Encoder> pEncoder(EncoderFactory::getInstance(pwszEncodingSymbol));
	if (!pEncoder.get())
		return 0;
	
	std::auto_ptr<Converter> pConverter(ConverterFactory::getInstance(pwszCharset));
	if (!pConverter.get()) {
		pwszCharset = Part::getDefaultCharset();
		pConverter = ConverterFactory::getInstance(pwszCharset);
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
	
	StringBuffer<STRING> buf;
	for (n = 0; n <= nLine; ++n) {
		if (n != 0) {
			buf.append("\r\n");
		}
		if (lines[n].first != lines[n].second) {
			string_ptr str(encodeLine(lines[n].first,
				lines[n].second - lines[n].first, pwszCharset, pConverter.get(),
				pwszEncodingSymbol, pEncoder.get(), bOneBlock));
			if (!str.get())
				return 0;
			buf.append(str.get());
		}
	}
	
	return buf.getString();
}

string_ptr qs::FieldParser::encodeLine(const WCHAR* pwsz,
									   size_t nLen,
									   const WCHAR* pwszCharset,
									   Converter* pConverter,
									   const WCHAR* pwszEncoding,
									   Encoder* pEncoder,
									   bool bOneBlock)
{
	assert(pwsz);
	assert(pwszCharset);
	assert(pConverter);
	assert(pwszEncoding);
	assert(pEncoder);
	
	if (nLen == -1)
		nLen = wcslen(pwsz);
	
	typedef std::pair<const WCHAR*, const WCHAR*> Item;
	typedef std::pair<Item, Item> Block;
	typedef std::vector<Block> Blocks;
	Blocks blocks;
	if (bOneBlock) {
		blocks.push_back(Block(Item(pwsz, pwsz + nLen), Item(0, 0)));
	}
	else {
		const WCHAR wsz[] = L" \t";
		const WCHAR* pBegin = pwsz;
		while (pBegin < pwsz + nLen) {
			const WCHAR* pEnd = std::find_first_of(
				pBegin, pwsz + nLen, wsz, wsz + 2);
			blocks.push_back(Block(Item(pBegin, pEnd), Item(0, 0)));
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
	
	string_ptr strCharset(wcs2mbs(pwszCharset));
	string_ptr strEncoding(wcs2mbs(pwszEncoding));
	
	StringBuffer<STRING> buf;
	Blocks::iterator it = blocks.begin();
	while (it != blocks.end()) {
		Block& b = *it;
		Item& i = b.first;
		Item& s = b.second;
		if (!FieldParserUtil<WSTRING>::isAscii(i.first, i.second - i.first)) {
			Item itemAfter = (it + 1) != blocks.end() ? (*(it + 1)).first : Item(0, 0);
			if ((itemAfter.first == itemAfter.second ||
				!FieldParserUtil<WSTRING>::isAscii(itemAfter.first, itemAfter.second - itemAfter.first)) &&
				s.first) {
				i.second = s.second;
				s.first = 0;
				s.second = 0;
			}
			
			size_t nLen = i.second - i.first;
			xstring_size_ptr converted(pConverter->encode(i.first, &nLen));
			if (!converted.get())
				return 0;
			
			malloc_size_ptr<unsigned char> encoded(pEncoder->encode(
				reinterpret_cast<const unsigned char*>(converted.get()),
				converted.size()));
			if (!encoded.get())
				return 0;
			
			buf.append("=?");
			buf.append(strCharset.get());
			buf.append("?");
			buf.append(strEncoding.get());
			buf.append("?");
			buf.append(reinterpret_cast<CHAR*>(encoded.get()), encoded.size());
			buf.append("?=");
		}
		else {
			if (i.first != i.second) {
				string_ptr str(wcs2mbs(i.first, i.second - i.first));
				buf.append(str.get());
			}
		}
		
		if (s.first != s.second) {
			string_ptr str(wcs2mbs(s.first, s.second - s.first));
			buf.append(str.get());
		}
		
		++it;
	}
	
	return buf.getString();
}


/****************************************************************************
 *
 * UnstructuredParser
 *
 */

qs::UnstructuredParser::UnstructuredParser()
{
}

qs::UnstructuredParser::UnstructuredParser(const WCHAR* pwszValue)
{
	wstrValue_ = allocWString(pwszValue);
}

qs::UnstructuredParser::UnstructuredParser(const WCHAR* pwszValue,
										   const WCHAR* pwszCharset)
{
	wstrValue_ = allocWString(pwszValue);
	wstrCharset_ = allocWString(pwszCharset);
}

qs::UnstructuredParser::~UnstructuredParser()
{
}

const WCHAR* qs::UnstructuredParser::getValue() const
{
	return wstrValue_.get();
}

Part::Field qs::UnstructuredParser::parse(const Part& part,
										  const WCHAR* pwszName)
{
	return parse(part, pwszName, 0, &wstrValue_);
}

string_ptr qs::UnstructuredParser::unparse(const Part& part) const
{
	const WCHAR* pwszCharset = wstrCharset_.get();
	wstring_ptr wstrHeaderCharset;
	if (!pwszCharset) {
		wstrHeaderCharset = part.getHeaderCharset();
		pwszCharset = wstrHeaderCharset.get();
	}
	
	wstring_ptr wstrFoldedValue(foldValue(wstrValue_.get()));
	return encode(wstrFoldedValue.get(), -1, pwszCharset, 0, false, true);
}

wstring_ptr qs::UnstructuredParser::foldValue(const WCHAR* pwszValue) const
{
	assert(pwszValue);
	
	typedef std::pair<const WCHAR*, const WCHAR*> Block;
	typedef std::vector<Block> Blocks;
	Blocks blocks;
	
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
					blocks.push_back(Block(pBegin, pLastBreak));
					pBegin = pLastBreak;
					nLen = nLenBreak + (*pBegin > 0x7f ? ENCODE_MARKER_LENGTH : 0);
					pLastBreak = 0;
					pnLen = &nLenBreak;
					bProcess = true;
				}
				if (bCanBreak && (!bProcess || (c == L'\0' && nLen != 0))) {
					blocks.push_back(Block(pBegin, p));
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
	
	StringBuffer<WSTRING> buf;
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
			buf.append(b.first, b.second - b.first);
			buf.append(L"\n ");
		}
		else {
			buf.append(b.first, b.second - b.first);
		}
		++it;
	}
	
	return buf.getString();
}

Part::Field qs::UnstructuredParser::parse(const Part& part,
										  const WCHAR* pwszName,
										  unsigned int nIndex,
										  wstring_ptr* pwstrValue)
{
	assert(pwszName);
	
	string_ptr strValue(part.getRawField(pwszName, nIndex));
	if (!strValue.get())
		return Part::FIELD_NOTEXIST;
	
	std::auto_ptr<Converter> pConverter;
	if (isRawValue(strValue.get())) {
		if (!part.hasField(L"MIME-Version"))
			pConverter = ConverterFactory::getInstance(part.getDefaultCharset());
		else if (part.isOption(Part::O_ALLOW_RAW_FIELD))
			pConverter = ConverterFactory::getInstance(part.getHeaderCharset().get());
	}
	if (pConverter.get()) {
		size_t nLen = strlen(strValue.get());
		wxstring_size_ptr decoded(pConverter->decode(strValue.get(), &nLen));
		if (!decoded.get())
			return Part::FIELD_ERROR;
		*pwstrValue = allocWString(decoded.get());
	}
	else {
		*pwstrValue = decode(strValue.get(), -1, false, 0);
	}
	
	return Part::FIELD_EXIST;
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

bool qs::UnstructuredParser::isRawValue(const CHAR* psz)
{
	for (const CHAR* p = psz; *p; ++p) {
		unsigned char c = *p;
		if (c >= 0x80 || c == 0x1b)
			return true;
	}
	return false;
}


/****************************************************************************
 *
 * MultipleUnstructuredParser
 *
 */

qs::MultipleUnstructuredParser::MultipleUnstructuredParser()
{
}

qs::MultipleUnstructuredParser::~MultipleUnstructuredParser()
{
	std::for_each(listValue_.begin(), listValue_.end(), string_free<WSTRING>());
}

const MultipleUnstructuredParser::ValueList& qs::MultipleUnstructuredParser::getValues() const
{
	return listValue_;
}

Part::Field qs::MultipleUnstructuredParser::parse(const Part& part,
												  const WCHAR* pwszName)
{
	for (unsigned int nIndex = 0; ; ++nIndex) {
		wstring_ptr wstrValue;
		Part::Field field = UnstructuredParser::parse(part, pwszName, nIndex, &wstrValue);
		if (field == Part::FIELD_NOTEXIST)
			break;
		else if (field != Part::FIELD_EXIST)
			return field;
		listValue_.push_back(wstrValue.get());
		wstrValue.release();
	}
	return listValue_.empty() ? Part::FIELD_NOTEXIST : Part::FIELD_EXIST;
}

string_ptr qs::MultipleUnstructuredParser::unparse(const Part& part) const
{
	assert(false);
	return 0;
}


/****************************************************************************
 *
 * DummyParser
 *
 */

qs::DummyParser::DummyParser(const WCHAR* pwszValue,
							 unsigned int nFlags) :
	nFlags_(nFlags)
{
	wstrValue_ = allocWString(pwszValue);
}

qs::DummyParser::~DummyParser()
{
}

Part::Field qs::DummyParser::parse(const Part& part,
								   const WCHAR* pwszName)
{
	assert(false);
	return Part::FIELD_ERROR;
}

string_ptr qs::DummyParser::unparse(const Part& part) const
{
	wstring_ptr wstrCharset(part.getHeaderCharset());
	
	size_t nLen = wcslen(wstrValue_.get());
	const WCHAR* pBegin = wstrValue_.get();
	const WCHAR* pEnd = wstrValue_.get();
	bool bAscii = true;
	StringBuffer<STRING> buf;
	while (pEnd < wstrValue_.get() + nLen) {
		WCHAR c = *pEnd;
		if (bAscii)
			bAscii = c <= 0x7f;
		if (isSpecial(c)) {
			if (pBegin != pEnd) {
				if (bAscii) {
					string_ptr str(wcs2mbs(pBegin, pEnd - pBegin));
					buf.append(str.get());
				}
				else {
					const WCHAR* p = pEnd;
					if (*(p - 1) == L' ')
						--p;
					if (pBegin != wstrValue_.get() && *pBegin == L' ')
						++pBegin;
					string_ptr str(encode(pBegin, p - pBegin,
						wstrCharset.get(), 0, false, true));
					if (!str.get())
						return 0;
					buf.append(str.get());
					buf.append(' ');
				}
			}
			buf.append(static_cast<CHAR>(c));
			pBegin = pEnd + 1;
			bAscii = true;
		}
		++pEnd;
	}
	if (pBegin != pEnd) {
		string_ptr str(encode(pBegin, pEnd - pBegin,
			wstrCharset.get(), 0, false, true));
		if (!str.get())
			return 0;
		buf.append(str.get());
	}
	
	return buf.getString();
}

bool qs::DummyParser::isSpecial(WCHAR c) const
{
	if (c > 0x7f || c == L'@')
		return false;
	else
		return Tokenizer::isSpecial(static_cast<unsigned char>(c), nFlags_);
}


/****************************************************************************
 *
 * UTF8Parser
 *
 */

qs::UTF8Parser::UTF8Parser(const WCHAR* pwszValue)
{
	wstrValue_ = allocWString(pwszValue);
}

qs::UTF8Parser::~UTF8Parser()
{
}

Part::Field qs::UTF8Parser::parse(const Part& part,
								  const WCHAR* pwszName)
{
	assert(false);
	return Part::FIELD_ERROR;
}

string_ptr qs::UTF8Parser::unparse(const Part& part) const
{
	size_t nLen = wcslen(wstrValue_.get());
	xstring_size_ptr strValue(UTF8Converter().encode(wstrValue_.get(), &nLen));
	if (!strValue.get())
		return 0;
	return allocString(strValue.get(), strValue.size());
}


/****************************************************************************
 *
 * NoParseParser
 *
 */

qs::NoParseParser::NoParseParser(const WCHAR* pwszSeparator,
								 unsigned int nFlags) :
	nFlags_(nFlags)
{
	assert(pwszSeparator);
	strSeparator_ = wcs2mbs(pwszSeparator);
}

qs::NoParseParser::~NoParseParser()
{
}

const WCHAR* qs::NoParseParser::getValue() const
{
	return wstrValue_.get();
}

Part::Field qs::NoParseParser::parse(const Part& part,
									 const WCHAR* pwszName)
{
	assert(pwszName);
	
	string_ptr strValue(part.getRawField(pwszName, 0));
	if (!strValue.get())
		return Part::FIELD_NOTEXIST;
	
	if (!(nFlags_ & FLAG_SINGLEFIELD)) {
		StringBuffer<STRING> buf(strValue.get());
		unsigned int n = 1;
		while (true) {
			string_ptr str(part.getRawField(pwszName, n));
			if (!str.get())
				break;
			
			buf.append(strSeparator_.get());
			buf.append(str.get());
			
			++n;
		}
		strValue = buf.getString();
	}
	
	wstrValue_ = mbs2wcs(strValue.get());
	
	return Part::FIELD_EXIST;
}

string_ptr qs::NoParseParser::unparse(const Part& part) const
{
	assert(false);
	return 0;
}


/****************************************************************************
 *
 * SimpleParser
 *
 */

qs::SimpleParser::SimpleParser(unsigned int nFlags) :
	nFlags_(nFlags)
{
}

qs::SimpleParser::SimpleParser(const WCHAR* pwszValue,
							   unsigned int nFlags) :
	nFlags_(nFlags)
{
	wstrValue_ = allocWString(pwszValue);
}

qs::SimpleParser::~SimpleParser()
{
}

const WCHAR* qs::SimpleParser::getValue() const
{
	return wstrValue_.get();
}

Part::Field qs::SimpleParser::parse(const Part& part,
									const WCHAR* pwszName)
{
	assert(pwszName);
	
	string_ptr strValue(part.getRawField(pwszName, 0));
	if (!strValue.get())
		return Part::FIELD_NOTEXIST;
	
	unsigned int nFlags = 0;
	if (nFlags_ & FLAG_RECOGNIZECOMMENT)
		nFlags |= Tokenizer::F_RECOGNIZECOMMENT;
	if (nFlags_ & FLAG_TSPECIAL)
		nFlags |= Tokenizer::F_TSPECIAL;
	else
		nFlags |= Tokenizer::F_SPECIAL;
	Tokenizer t(strValue.get(), -1, nFlags);
	State state = S_BEGIN;
	while (state != S_END) {
		Tokenizer::Token token(t.getToken());
		
		if (token.type_ == Tokenizer::T_COMMENT)
			continue;
		
		switch (state) {
		case S_BEGIN:
			if (token.type_ == Tokenizer::T_ATOM) {
				if (nFlags_ & FLAG_DECODE)
					wstrValue_ = decode(token.str_.get(), -1, false, 0);
				else
					wstrValue_ = mbs2wcs(token.str_.get());
				state = S_ATOM;
			}
			else if (token.type_ == Tokenizer::T_QSTRING && nFlags_ & FLAG_ACCEPTQSTRING) {
				if (part.isOption(Part::O_ALLOW_ENCODED_QSTRING) && nFlags_ & FLAG_DECODE)
					wstrValue_ = FieldParserUtil<WSTRING>::resolveQuotedPairs(
						decode(token.str_.get(), -1, false, 0).get(), -1);
				else
					wstrValue_ = mbs2wcs(token.str_.get());
				state = S_ATOM;
			}
			else if (token.type_ == Tokenizer::T_END) {
				wstrValue_ = allocWString(L"");
				state = S_END;
			}
			else {
				return parseError();
			}
			break;
		case S_ATOM:
			if (token.type_ != Tokenizer::T_END)
				return parseError();
			state = S_END;
			break;
		case S_END:
			break;
		}
	}
	
	return Part::FIELD_EXIST;
}

string_ptr qs::SimpleParser::unparse(const Part& part) const
{
	if (nFlags_ & FLAG_DECODE) {
		wstring_ptr wstrCharset(part.getHeaderCharset());
		return encode(wstrValue_.get(), -1, wstrCharset.get(), 0, true, true);
	}
	else {
		if (!FieldParserUtil<WSTRING>::isAscii(wstrValue_.get()))
			return 0;
		return wcs2mbs(wstrValue_.get());
	}
}


/****************************************************************************
 *
 * NumberParser
 *
 */

qs::NumberParser::NumberParser(unsigned int nFlags) :
	nFlags_(nFlags),
	n_(0)
{
}

qs::NumberParser::NumberParser(unsigned int n,
							   unsigned int nFlags) :
	nFlags_(nFlags),
	n_(n)
{
}

qs::NumberParser::~NumberParser()
{
}

unsigned int qs::NumberParser::getValue() const
{
	return n_;
}

Part::Field qs::NumberParser::parse(const Part& part,
									const WCHAR* pwszName)
{
	assert(pwszName);
	
	string_ptr strValue(part.getRawField(pwszName, 0));
	if (!strValue.get())
		return Part::FIELD_NOTEXIST;
	
	unsigned int nFlags = Tokenizer::F_SPECIAL;
	if (nFlags_ & FLAG_RECOGNIZECOMMENT)
		nFlags |= Tokenizer::F_RECOGNIZECOMMENT;
	Tokenizer t(strValue.get(), -1, nFlags);
	State state = S_BEGIN;
	string_ptr strNumber;
	while (state != S_END) {
		Tokenizer::Token token(t.getToken());
		
		if (token.type_ == Tokenizer::T_COMMENT)
			continue;
		switch (state) {
		case S_BEGIN:
			if (token.type_ != Tokenizer::T_ATOM)
				return parseError();
			strNumber = token.str_;
			state = S_ATOM;
			break;
		case S_ATOM:
			if (token.type_ != Tokenizer::T_END)
				return parseError();
			state = S_END;
			break;
		case S_END:
			break;
		}
	}
	
	CHAR* pEnd = 0;
	n_ = strtol(strNumber.get(), &pEnd, nFlags_ & FLAG_HEX ? 16 : 10);
	if (*pEnd)
		return parseError();
	
	return Part::FIELD_EXIST;
}

string_ptr qs::NumberParser::unparse(const Part& part) const
{
	CHAR sz[32];
	if (nFlags_ & FLAG_HEX)
		sprintf(sz, "%x", n_);
	else
		sprintf(sz, "%d", n_);
	return allocString(sz);
}


/****************************************************************************
 *
 * DateParser
 *
 */

qs::DateParser::DateParser()
{
}

qs::DateParser::DateParser(const Time& date) :
	date_(date)
{
}

qs::DateParser::~DateParser()
{
}

const Time& qs::DateParser::getTime() const
{
	return date_;
}

Part::Field qs::DateParser::parse(const Part& part,
								  const WCHAR* pwszName)
{
	assert(pwszName);
	
	string_ptr strValue(part.getRawField(pwszName, 0));
	if (!strValue.get())
		return Part::FIELD_NOTEXIST;
	
	if (!parse(strValue.get(), -1, getFlagsFromPartOption(part.getOptions()), &date_))
		return Part::FIELD_ERROR;
	
	return Part::FIELD_EXIST;
}

string_ptr qs::DateParser::unparse(const Part& part) const
{
	wstring_ptr wstr(unparse(date_));
	assert(wstr.get());
	return wcs2mbs(wstr.get());
}

bool qs::DateParser::parse(const CHAR* psz,
						   size_t nLen,
						   unsigned int nFlags,
						   Time* pTime)
{
	assert(psz);
	assert(pTime);
	
	if (nFlags == FLAG_ALLOWDEFAULT)
		nFlags = getFlagsFromPartOption(Part::getGlobalOptions());
	
	Tokenizer t(psz, nLen, Tokenizer::F_RECOGNIZECOMMENT | Tokenizer::F_SPECIAL);
	
	State state = S_BEGIN;
	string_ptr strFirst;
	int nWeek = -1;
	int nDay = 0;
	int nMonth = 0;
	int nYear = 0;
	int nHour = 0;
	int nMinute = 0;
	int nSecond = 0;
	int nTimeZone = 0;
	while (state != S_END) {
		Tokenizer::Token token(t.getToken());
		
		if (token.type_ == Tokenizer::T_COMMENT)
			continue;
		switch (state) {
		case S_BEGIN:
			if (token.type_ != Tokenizer::T_ATOM)
				return false;
			strFirst = token.str_;
			state = S_FIRST;
			break;
		case S_FIRST:
			if (token.type_ == Tokenizer::T_SPECIAL && *token.str_.get() == L',') {
				nWeek = getWeek(strFirst.get());
				if (nWeek == -1)
					return false;
				state = S_WEEK;
			}
			else if (token.type_ == Tokenizer::T_ATOM) {
				nDay = getDay(strFirst.get());
				if (nDay == -1)
					return false;
				nMonth = getMonth(token.str_.get());
				if (nMonth == -1)
					return false;
				state = S_MONTH;
			}
			else {
				return false;
			}
			break;
		case S_WEEK:
			if (token.type_ != Tokenizer::T_ATOM)
				return false;
			nDay = getDay(token.str_.get());
			if (nDay == -1)
				return false;
			state = S_DAY;
			break;
		case S_DAY:
			if (token.type_ != Tokenizer::T_ATOM)
				return false;
			nMonth = getMonth(token.str_.get());
			if (nMonth == -1)
				return false;
			state = S_MONTH;
			break;
		case S_MONTH:
			if (token.type_ != Tokenizer::T_ATOM)
				return false;
			nYear = getYear(token.str_.get());
			if (nYear == -1)
				return false;
			state = S_YEAR;
			break;
		case S_YEAR:
			if (token.type_ != Tokenizer::T_ATOM)
				return false;
			nHour = getHour(token.str_.get(), (nFlags & FLAG_ALLOWSINGLEDIGITTIME) != 0);
			if (nHour == -1)
				return false;
			state = S_HOUR;
			break;
		case S_HOUR:
			if (token.type_ != Tokenizer::T_SPECIAL || *token.str_.get() != L':')
				return false;
			state = S_HOURSEP;
			break;
		case S_HOURSEP:
			if (token.type_ != Tokenizer::T_ATOM)
				return false;
			nMinute = getMinute(token.str_.get(), (nFlags & FLAG_ALLOWSINGLEDIGITTIME) != 0);
			if (nMinute == -1)
				return false;
			state = S_MINUTE;
			break;
		case S_MINUTE:
			if (token.type_ == Tokenizer::T_SPECIAL && *token.str_.get() == L':') {
				state = S_MINUTESEP;
			}
			else if (token.type_ == Tokenizer::T_ATOM) {
				nTimeZone = getTimeZone(token.str_.get());
				if (nTimeZone == -1)
					return false;
				state = S_TIMEZONE;
			}
			else if (token.type_ == Tokenizer::T_END && (nFlags & FLAG_ALLOWNOTIMEZONE) != 0) {
				state = S_TIMEZONE;
			}
			else {
				return false;
			}
			break;
		case S_MINUTESEP:
			if (token.type_ != Tokenizer::T_ATOM)
				return false;
			nSecond = getSecond(token.str_.get(), (nFlags & FLAG_ALLOWSINGLEDIGITTIME) != 0);
			if (nSecond == -1)
				return false;
			state = S_SECOND;
			break;
		case S_SECOND:
			if (token.type_ == Tokenizer::T_ATOM) {
				nTimeZone = getTimeZone(token.str_.get());
				if (nTimeZone == -1)
					return false;
				state = S_TIMEZONE;
			}
			else if (token.type_ == Tokenizer::T_END && (nFlags & FLAG_ALLOWNOTIMEZONE) != 0) {
				state = S_TIMEZONE;
			}
			else {
				return false;
			}
			break;
		case S_TIMEZONE:
			if ((nFlags & FLAG_ALLOWRUBBISH) == 0) {
				if (token.type_ != Tokenizer::T_END)
					return false;
			}
			state = S_END;
			break;
		case S_END:
			break;
		}
	}
	
	if (nDay > Time::getDayCount(nYear, nMonth))
		return false;
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
	
	return true;
}

wstring_ptr qs::DateParser::unparse(const Time& time)
{
	return time.format(L"%W, %D %M1 %Y4 %h:%m:%s %z", Time::FORMAT_ORIGINAL);
}

unsigned int qs::DateParser::getFlagsFromPartOption(unsigned int nOption)
{
	return (nOption & Part::O_ALLOW_SINGLE_DIGIT_TIME ? FLAG_ALLOWSINGLEDIGITTIME : 0) |
		(nOption & Part::O_ALLOW_DATE_WITH_RUBBISH ? FLAG_ALLOWRUBBISH : 0) |
		(nOption & Part::O_ALLOW_DATE_WITHOUT_TIMEZONE ? FLAG_ALLOWNOTIMEZONE : 0);
}

int qs::DateParser::getWeek(const CHAR* psz)
{
	assert(psz);
	
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
		if (_stricmp(psz, pszWeeks[n]) == 0)
			return n;
	}
	
	return -1;
}

int qs::DateParser::getDay(const CHAR* psz)
{
	assert(psz);
	
	size_t nLen = strlen(psz);
	if (nLen == 0 || nLen > 2 || !isDigit(psz))
		return -1;
	
	CHAR* pEnd = 0;
	return strtol(psz, &pEnd, 10);
}

int qs::DateParser::getMonth(const CHAR* psz)
{
	assert(psz);
	
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
		if (_stricmp(psz, pszMonths[n]) == 0)
			return n + 1;
	}
	return -1;
}

int qs::DateParser::getYear(const CHAR* psz)
{
	assert(psz);
	
	if (!isDigit(psz))
		return -1;
	
	CHAR* pEnd = 0;
	int nYear = strtol(psz, &pEnd, 10);
	
	size_t nLen = strlen(psz);
	if (nLen == 2)
		nYear += nYear >= 50 ? 1900 : 2000;
	else if (nLen != 4)
		return -1;
	
	return nYear;
}

int qs::DateParser::getHour(const CHAR* psz,
							bool bAllowSingleDigit)
{
	return getTime(psz, 0, 23, bAllowSingleDigit);
}

int qs::DateParser::getMinute(const CHAR* psz,
							  bool bAllowSingleDigit)
{
	return getTime(psz, 0, 59, bAllowSingleDigit);
}

int qs::DateParser::getSecond(const CHAR* psz,
							  bool bAllowSingleDigit)
{
	return getTime(psz, 0, 60, bAllowSingleDigit);
}

int qs::DateParser::getTime(const CHAR* psz,
							int nMin,
							int nMax,
							bool bAllowSingleDigit)
{
	assert(psz);
	
	size_t nLen = strlen(psz);
	if ((nLen != 2 && (!bAllowSingleDigit || nLen != 1)) || !isDigit(psz))
		return -1;
	
	CHAR* pEnd = 0;
	int n = strtol(psz, &pEnd, 10);
	if (n < nMin || nMax < n)
		return -1;
	
	return n;
}

int qs::DateParser::getTimeZone(const CHAR* psz)
{
	assert(psz);
	
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
	
	int nTimeZone = -1;
	if (*psz == '+' || *psz == '-') {
		if (strlen(psz) == 5 && isDigit(psz + 1)) {
			CHAR* pEnd = 0;
			nTimeZone = strtol(psz + 1, &pEnd, 10);
			if (*psz == '-')
				nTimeZone = -nTimeZone;
		}
	}
	else if (strlen(psz) == 1) {
		if (*psz != 'j')
			nTimeZone = 0;
	}
	else {
		for (int n = 0; n < countof(zones); ++n) {
			if (_stricmp(psz, zones[n].pszName_) == 0) {
				nTimeZone = zones[n].nTimeZone_;
				break;
			}
		}
	}
	
	if (nTimeZone == -1)
		nTimeZone = 0;
	
	return nTimeZone;
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

qs::AddressParser::AddressParser() :
	nFlags_(0)
{
}

qs::AddressParser::AddressParser(unsigned int nFlags) :
	nFlags_(nFlags)
{
}

qs::AddressParser::AddressParser(const WCHAR* pwszPhrase,
								 const WCHAR* pwszAddress) :
	nFlags_(0)
{
	assert(pwszAddress);
	
	if (pwszPhrase && *pwszPhrase)
		wstrPhrase_ = allocWString(pwszPhrase);
	
	const WCHAR* p = wcsrchr(pwszAddress, L'@');
	if (p) {
		wstrMailbox_ = allocWString(pwszAddress, p - pwszAddress);
		wstrHost_ = allocWString(p + 1);
	}
	else {
		wstrMailbox_ = allocWString(pwszAddress);
	}
}

qs::AddressParser::AddressParser(const WCHAR* pwszPhrase,
								 const WCHAR* pwszMailbox,
								 const WCHAR* pwszHost) :
	nFlags_(0)
{
	assert(pwszMailbox);
	
	if (pwszPhrase && *pwszPhrase)
		wstrPhrase_ = allocWString(pwszPhrase);
	
	wstrMailbox_ = allocWString(pwszMailbox);
	
	if (pwszHost)
		wstrHost_ = allocWString(pwszHost);
}

qs::AddressParser::~AddressParser()
{
}

const WCHAR* qs::AddressParser::getPhrase() const
{
	return wstrPhrase_.get();
}

const WCHAR* qs::AddressParser::getMailbox() const
{
	return wstrMailbox_.get();
}

const WCHAR* qs::AddressParser::getHost() const
{
	return wstrHost_.get() ? wstrHost_.get() : L"";
}

AddressListParser* qs::AddressParser::getGroup() const
{
	return pGroup_.get();
}

wstring_ptr qs::AddressParser::getAddress() const
{
	string_ptr strAddrSpec(getAddrSpec(wstrMailbox_.get(), wstrHost_.get()));
	return mbs2wcs(strAddrSpec.get());
}

wstring_ptr qs::AddressParser::getValue() const
{
	return getValue(true);
}

wstring_ptr qs::AddressParser::getValue(bool bAutoQuote) const
{
	const WCHAR* pwszPhrase = wstrPhrase_.get();
	wstring_ptr wstrPhrase;
	if (pwszPhrase && bAutoQuote) {
		bool bQuote = false;
		size_t nLen = wcslen(pwszPhrase);
		if (nLen != 0 && *pwszPhrase == L'\"' && *(pwszPhrase + nLen - 1) == L'\"') {
			// This may be the case where the phrase was encoded and
			// the encoded phrase is quoted by "".
			bQuote = false;
		}
#if 0
		else if (FieldParserUtil<WSTRING>::isAscii(pwszPhrase)) {
			bQuote = true;
		}
		else if (wcschr(pwszPhrase, L'\\')) {
			// This may be the case where
			// 1. the phrase contains backslash correctly, or
			// 2. the phrase was encoded and the encoded phrase contains backslash.
			bQuote = true;
		}
		else {
			bQuote = false;
		}
#else
		else {
			bQuote = true;
		}
#endif
		if (bQuote) {
			wstrPhrase = FieldParserUtil<WSTRING>::getAtomsOrQString(pwszPhrase, -1);
			pwszPhrase = wstrPhrase.get();
		}
	}
	
	StringBuffer<WSTRING> buf;
	if (pGroup_.get()) {
		assert(pwszPhrase);
		buf.append(pwszPhrase);
		buf.append(L": ");
		wstring_ptr wstrValue(pGroup_->getValue(bAutoQuote));
		buf.append(wstrValue.get());
		buf.append(L";");
	}
	else {
		if (pwszPhrase) {
			buf.append(pwszPhrase);
			buf.append(L" <");
		}
		
		string_ptr strAddrSpec(getAddrSpec(wstrMailbox_.get(), wstrHost_.get()));
		wstring_ptr wstrAddrSpec(mbs2wcs(strAddrSpec.get()));
		buf.append(wstrAddrSpec.get());
		
		if (pwszPhrase)
			buf.append(L">");
	}
	
	return buf.getString();
}

void qs::AddressParser::setPhrase(const WCHAR* pwszPhrase)
{
	wstrPhrase_ = allocWString(pwszPhrase);
}

bool qs::AddressParser::contains(const WCHAR* pwszAddress) const
{
	if (pGroup_.get())
		return pGroup_->contains(pwszAddress);
	else
		return _wcsicmp(getAddress().get(), pwszAddress) == 0;
}

Part::Field qs::AddressParser::parse(const Part& part,
									 const WCHAR* pwszName)
{
	assert(pwszName);
	
	string_ptr strValue(part.getRawField(pwszName, 0));
	if (!strValue.get())
		return Part::FIELD_NOTEXIST;
	
	unsigned int nFlags = Tokenizer::F_RECOGNIZECOMMENT |
		Tokenizer::F_RECOGNIZEDOMAIN | Tokenizer::F_SPECIAL;
	Tokenizer t(strValue.get(), -1, nFlags);
	return parseAddress(part, t, 0);
}

string_ptr qs::AddressParser::unparse(const Part& part) const
{
	StringBuffer<STRING> buf;
	
	if (wstrPhrase_.get()) {
		if (FieldParserUtil<WSTRING>::isAscii(wstrPhrase_.get())) {
			string_ptr str(wcs2mbs(wstrPhrase_.get()));
			string_ptr strAtoms(FieldParserUtil<STRING>::getAtomsOrQString(str.get(), -1));
			buf.append(strAtoms.get());
		}
		else {
			wstring_ptr wstrCharset(part.getHeaderCharset());
			string_ptr str(encode(wstrPhrase_.get(), -1, wstrCharset.get(), 0, true, true));
			if (!str.get())
				return 0;
			buf.append(str.get());
		}
	}
	
	if (pGroup_.get()) {
		buf.append(": ");
		string_ptr str(pGroup_->unparse(part));
		buf.append(str.get());
		buf.append(';');
	}
	else {
		if (wstrPhrase_.get())
			buf.append(" <");
		
		string_ptr strAddrSpec(getAddrSpec(wstrMailbox_.get(), wstrHost_.get()));
		buf.append(strAddrSpec.get());
		
		if (wstrPhrase_.get())
			buf.append('>');
	}
	
	return buf.getString();
}

bool qs::AddressParser::isNeedQuoteMailbox(const CHAR* pszMailbox)
{
	assert(pszMailbox);
	
	Tokenizer t(pszMailbox, -1, Tokenizer::F_SPECIAL);
	
	bool bDot = true;
	while (true) {
		Tokenizer::Token token(t.getToken());
		if (token.type_ == Tokenizer::T_END)
			break;
		
		if (bDot) {
			if (token.type_ != Tokenizer::T_ATOM)
				return true;
		}
		else {
			if (token.type_ != Tokenizer::T_SPECIAL || *token.str_.get() != '.')
				return true;
		}
		bDot = !bDot;
	}
	if (bDot)
		return true;
	
	return false;
}

string_ptr qs::AddressParser::getAddrSpec(const CHAR* pszMailbox,
										  const CHAR* pszHost)
{
	assert(pszMailbox);
	
	StringBuffer<STRING> buf;
	
	if (isNeedQuoteMailbox(pszMailbox)) {
		string_ptr strQuotedMailbox(FieldParserUtil<STRING>::getQString(pszMailbox, -1));
		buf.append(strQuotedMailbox.get());
	}
	else {
		buf.append(pszMailbox);
	}
	
	if (pszHost && *pszHost) {
		buf.append('@');
		buf.append(pszHost);
	}
	
	return buf.getString();
}

string_ptr qs::AddressParser::getAddrSpec(const WCHAR* pwszMailbox,
										  const WCHAR* pwszHost)
{
	assert(pwszMailbox);
	
	string_ptr strMailbox(wcs2mbs(pwszMailbox));
	string_ptr strHost;
	if (pwszHost)
		strHost = wcs2mbs(pwszHost);
	
	return getAddrSpec(strMailbox.get(), strHost.get());
}

Part::Field qs::AddressParser::parseAddress(const Part& part,
											Tokenizer& t,
											bool* pbEnd)
{
	assert(((nFlags_ & FLAG_INGROUP) == FLAG_INGROUP && pbEnd) ||
		((nFlags_ & FLAG_INGROUP) != FLAG_INGROUP && !pbEnd));
	
	if (pbEnd)
		*pbEnd = false;
	
	bool bDisallowGroup = (nFlags_ & FLAG_DISALLOWGROUP) != 0;
	bool bInGroup = (nFlags_ & FLAG_INGROUP) == FLAG_INGROUP;
	State state = S_BEGIN;
	string_ptr strComment;
	Phrases phrases;
	AddrSpecParser addrSpec;
	
	struct Deleter
	{
		typedef std::vector<std::pair<STRING, bool> > Phrases;
		
		Deleter(Phrases& phrases) : phrases_(phrases)
		{
		}
		
		~Deleter()
		{
			clear();
		}
		
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
		Tokenizer::Token token(t.getToken());
		if (token.type_ == Tokenizer::T_COMMENT) {
			strComment = token.str_;
			continue;
		}
		switch (state) {
		case S_BEGIN:
			switch (token.type_) {
			case Tokenizer::T_END:
				if (bInGroup)
					return parseError();
				state = S_END;
				break;
			case Tokenizer::T_SPECIAL:
				if (*token.str_.get() == ';' && bInGroup) {
					if (pbEnd)
						*pbEnd = true;
					state = S_END;
				}
				else if (*token.str_.get() == '<') {
					state = S_LEFTANGLE;
				}
				else if (*token.str_.get() != ',') {
					return parseError();
				}
				break;
			case Tokenizer::T_ATOM:
			case Tokenizer::T_QSTRING:
				phrases.push_back(std::make_pair(token.str_.get(),
					token.type_ == Tokenizer::T_ATOM));
				token.str_.release();
				state = S_PHRASE;
				break;
			default:
				return parseError();
			}
			break;
		case S_PHRASE:
			switch (token.type_) {
			case Tokenizer::T_SPECIAL:
				if (*token.str_.get() == '.') {
					string_ptr str(allocString("."));
					phrases.push_back(std::make_pair(str.get(), true));
					str.release();
				}
				else if (*token.str_.get() == '<') {
					state = S_LEFTANGLE;
				}
				else if (*token.str_.get() == ':' && !bDisallowGroup) {
					unsigned int nFlags = AddressListParser::FLAG_GROUP;
					if (nFlags_ & FLAG_ALLOWUTF8)
						nFlags |= AddressListParser::FLAG_ALLOWUTF8;
					pGroup_.reset(new AddressListParser(nFlags));
					if (pGroup_->parseAddressList(part, t) != Part::FIELD_EXIST)
						return parseError();
					state = S_SEMICOLON;
				}
				else if (*token.str_.get() == ';') {
					state = S_SEMICOLON;
				}
				else if ((*token.str_.get() == ',' ||
					(*token.str_.get() == ';' && bInGroup)) &&
					part.isOption(Part::O_ALLOW_ADDRESS_WITHOUT_DOMAIN)) {
					string_ptr strMailbox(getMailboxFromPhrases(phrases,
						part.isOption(Part::O_ALLOW_INVALID_PERIOD_IN_LOCALPART)));
					if (!strMailbox.get())
						return parseError();
					deleter.clear();
					
					wstrMailbox_ = convertMailbox(strMailbox.get());
					if (!wstrMailbox_.get())
						return parseError();
					if (pbEnd)
						*pbEnd = *token.str_.get() == ';';
					state = S_END;
				}
				else if (*token.str_.get() == '@') {
					string_ptr strMailbox(getMailboxFromPhrases(phrases,
						part.isOption(Part::O_ALLOW_INVALID_PERIOD_IN_LOCALPART)));
					if (!strMailbox.get())
						return parseError();
					deleter.clear();
					
					string_ptr strTemp;
					string_ptr strHost;
					string_ptr strAddrSpecComment;
					Part::Field field = addrSpec.parseAddrSpec(part, t, AddrSpecParser::S_ADDRSPECAT,
						bInGroup ? AddrSpecParser::TYPE_INGROUP : AddrSpecParser::TYPE_NORMAL,
						&strTemp, &strHost, &strAddrSpecComment, pbEnd);
					if (field != Part::FIELD_EXIST)
						return parseError();
					assert(!*strTemp.get());
					if (strAddrSpecComment.get())
						strComment = strAddrSpecComment;
					
					wstrMailbox_ = convertMailbox(strMailbox.get());
					if (!wstrMailbox_.get())
						return parseError();
					
					if (!FieldParserUtil<STRING>::isAscii(strHost.get()))
						return parseError();
					wstrHost_ = mbs2wcs(strHost.get());
					
					state = S_END;
				}
				else {
					return parseError();
				}
				break;
			case Tokenizer::T_ATOM:
			case Tokenizer::T_QSTRING:
				phrases.push_back(std::make_pair(token.str_.get(),
					token.type_ == Tokenizer::T_ATOM));
				token.str_.release();
				break;
			case Tokenizer::T_END:
				if (!bInGroup && part.isOption(Part::O_ALLOW_ADDRESS_WITHOUT_DOMAIN)) {
					string_ptr strMailbox(getMailboxFromPhrases(phrases,
						part.isOption(Part::O_ALLOW_INVALID_PERIOD_IN_LOCALPART)));
					if (!strMailbox.get())
						return parseError();
					deleter.clear();
					
					wstrMailbox_ = convertMailbox(strMailbox.get());
					if (!wstrMailbox_.get())
						return parseError();
					state = S_END;
					break;
				}
			default:
				return parseError();
			}
			break;
		case S_LEFTANGLE:
			switch (token.type_) {
			case Tokenizer::T_SPECIAL:
				if (*token.str_.get() != '@')
					return parseError();
				state = S_ROUTEAT;
				break;
			case Tokenizer::T_QSTRING:
			case Tokenizer::T_ATOM:
				{
					string_ptr strMailbox;
					string_ptr strHost;
					string_ptr strAddrSpecComment;
					Part::Field field = addrSpec.parseAddrSpec(part, t,
						AddrSpecParser::S_LOCALPARTWORD,
						AddrSpecParser::TYPE_INBRACKET, &strMailbox,
						&strHost, &strAddrSpecComment, 0);
					if (field != Part::FIELD_EXIST)
						return parseError();
					string_ptr strCompleteMailbox(concat(token.str_.get(), strMailbox.get()));
					wstrMailbox_ = convertMailbox(strCompleteMailbox.get());
					if (!wstrMailbox_.get())
						return parseError();
					if (*strHost.get()) {
						if (!FieldParserUtil<STRING>::isAscii(strHost.get()))
							return parseError();
						wstrHost_ = mbs2wcs(strHost.get());
					}
					if (strAddrSpecComment.get())
						strComment = strAddrSpecComment;
				}
				state = S_RIGHTANGLE;
				break;
			default:
				return parseError();
			}
			break;
		case S_RIGHTANGLE:
			if (token.type_ == Tokenizer::T_END) {
				if (bInGroup)
					return parseError();
			}
			else if (token.type_ == Tokenizer::T_SPECIAL) {
				if (*token.str_.get() == ',') {
				}
				else if (bInGroup && *token.str_.get() == ';') {
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
			switch (token.type_) {
			case Tokenizer::T_ATOM:
			case Tokenizer::T_DOMAIN:
				break;
			default:
				return parseError();
			}
			state = S_ROUTEDOMAIN;
			break;
		case S_ROUTEDOMAIN:
			if (token.type_ != Tokenizer::T_SPECIAL)
				return parseError();
			if (*token.str_.get() == ',')
				state = S_ROUTECANMA;
			else if (*token.str_.get() == ':')
				state = S_ROUTECOLON;
			else
				return parseError();
			break;
		case S_ROUTECANMA:
			if (token.type_ != Tokenizer::T_SPECIAL)
				return parseError();
			if (*token.str_.get() == '@')
				state = S_ROUTEAT;
			else if (*token.str_.get() == ',')
				state = S_ROUTECANMA;
			else
				return parseError();
			break;
		case S_ROUTECOLON:
			if (token.type_ == Tokenizer::T_ATOM ||
				token.type_ == Tokenizer::T_QSTRING) {
				string_ptr strMailbox;
				string_ptr strHost;
				string_ptr strAddrSpecComment;
				Part::Field field = addrSpec.parseAddrSpec(part, t,
					AddrSpecParser::S_LOCALPARTWORD,
					AddrSpecParser::TYPE_INBRACKET, &strMailbox,
					&strHost, &strAddrSpecComment, 0);
				if (field != Part::FIELD_EXIST)
					return parseError();
				wstrMailbox_ = convertMailbox(strMailbox.get());
				if (!wstrMailbox_.get())
					return parseError();
				if (strHost.get()) {
					if (!FieldParserUtil<STRING>::isAscii(strHost.get()))
						return parseError();
					wstrHost_ = mbs2wcs(strHost.get());
				}
				if (strAddrSpecComment.get())
					strComment = strAddrSpecComment;
			}
			else {
				return parseError();
			}
			state = S_RIGHTANGLE;
			break;
		case S_SEMICOLON:
			switch (token.type_) {
			case Tokenizer::T_END:
				state = S_END;
				break;
			case Tokenizer::T_SPECIAL:
				if (*token.str_.get() == ',')
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
	StringBuffer<WSTRING> bufPhrase;
	Phrases::iterator it = phrases.begin();
	while (it != phrases.end()) {
		const CHAR* psz = (*it).first;
		bool bAtom = (*it).second;
		bool bDecode = false;
		bool bPeriod = bAtom && *psz == '.';
		bool bDecoded = false;
		
		wstring_ptr wstrWord(decodePhrase(psz, bAtom,
			part.isOption(Part::O_ALLOW_ENCODED_QSTRING),
			(nFlags_ & FLAG_ALLOWUTF8) != 0, &bDecoded));
		if (it != phrases.begin()) {
			if ((!bPrevDecode || !bDecode) && !bPeriod && !bPrevPeriod)
				bufPhrase.append(L" ");
		}
		
		bool bQuote = false;
		if (nFlags_ & FLAG_ALLOWUTF8 && !bAtom) {
			const unsigned char* p = reinterpret_cast<const unsigned char*>(psz);
			while (*p && *p < 0x80)
				++p;
			bQuote = *p != L'\0';
		}
		
		if (bQuote) {
			bufPhrase.append(L'\"');
			for (const WCHAR* p = wstrWord.get(); *p; ++p) {
				if (*p == L'\\' || *p == L'\"')
					bufPhrase.append(L'\\');
				bufPhrase.append(*p);
			}
			bufPhrase.append(L'\"');
		}
		else {
			bufPhrase.append(wstrWord.get());
		}
		
		bPrevDecode = bDecoded;
		bPrevPeriod = bPeriod;
		
		++it;
	}
	if (bufPhrase.getLength() != 0)
		wstrPhrase_ = bufPhrase.getString();
	
	if (!wstrPhrase_.get() && strComment.get() &&
		part.isOption(Part::O_USE_COMMENT_AS_PHRASE))
		wstrPhrase_ = decode(strComment.get() + 1, strlen(strComment.get()) - 2,
			(nFlags_ & FLAG_ALLOWUTF8) != 0, 0);
	
	return Part::FIELD_EXIST;
}

wstring_ptr qs::AddressParser::convertMailbox(const CHAR* pszMailbox)
{
	if (nFlags_ & FLAG_ALLOWUTF8) {
		size_t nLen = strlen(pszMailbox);
		wxstring_size_ptr wstrMailbox(UTF8Converter().decode(pszMailbox, &nLen));
		if (!wstrMailbox.get())
			return 0;
		return allocWString(wstrMailbox.get());
	}
	else {
		if (!FieldParserUtil<STRING>::isAscii(pszMailbox))
			return 0;
		return mbs2wcs(pszMailbox);
	}
}

wstring_ptr qs::AddressParser::decodePhrase(const CHAR* psz,
											bool bAtom,
											bool bAllowEncodedQString,
											bool bAllowUTF8,
											bool* pbDecoded)
{
	assert(psz);
	assert(pbDecoded);
	
	*pbDecoded = false;
	
	if (bAtom)
		return decode(psz, -1, bAllowUTF8, pbDecoded);
	else if (bAllowEncodedQString)
		return FieldParserUtil<WSTRING>::resolveQuotedPairs(
			decode(psz, -1, bAllowUTF8, pbDecoded).get(), -1);
	else
		return mbs2wcs(psz);
}

string_ptr qs::AddressParser::getMailboxFromPhrases(const Phrases& phrases,
													bool bAllowInvalidPeriod)
{
	assert(!phrases.empty());
	
	StringBuffer<STRING> buf;
	
	bool bPeriod = true;
	Phrases::const_iterator it = phrases.begin();
	while (it != phrases.end()) {
		if (!bAllowInvalidPeriod) {
			if (bPeriod) {
				if ((*it).second && *(*it).first == '.')
					return 0;
				bPeriod = false;
			}
			else {
				if (!(*it).second || *(*it).first != '.')
					return 0;
				bPeriod = true;
			}
		}
		
		buf.append((*it).first);
		++it;
	}
	if (!bAllowInvalidPeriod && bPeriod)
		return 0;
	
	return buf.getString();
}


/****************************************************************************
 *
 * AddressListParser
 *
 */

size_t qs::AddressListParser::nMax__ = 128;

qs::AddressListParser::AddressListParser() :
	nFlags_(0),
	nMax_(getMaxAddresses())
{
}

qs::AddressListParser::AddressListParser(unsigned int nFlags) :
	nFlags_(nFlags),
	nMax_(getMaxAddresses())
{
}

qs::AddressListParser::AddressListParser(unsigned int nFlags,
										 size_t nMax) :
	nFlags_(nFlags),
	nMax_(nMax)
{
}

qs::AddressListParser::~AddressListParser()
{
	removeAllAddresses();
}

wstring_ptr qs::AddressListParser::getValue() const
{
	return getValue(true);
}

wstring_ptr qs::AddressListParser::getValue(bool bAutoQuote) const
{
	StringBuffer<WSTRING> buf;
	
	for (AddressList::const_iterator it = listAddress_.begin(); it != listAddress_.end(); ++it) {
		if (it != listAddress_.begin())
			buf.append(L", ");
		wstring_ptr strValue((*it)->getValue(bAutoQuote));
		buf.append(strValue.get());
	}
	
	return buf.getString();
}

wstring_ptr qs::AddressListParser::getNames() const
{
	StringBuffer<WSTRING> buf;
	
	for (AddressList::const_iterator it = listAddress_.begin(); it != listAddress_.end(); ++it) {
		if (buf.getLength() != 0)
			buf.append(L", ");
		if ((*it)->getPhrase()) {
			buf.append((*it)->getPhrase());
		}
		else {
			buf.append((*it)->getMailbox());
			if ((*it)->getHost()) {
				buf.append(L'@');
				buf.append((*it)->getHost());
			}
		}
	}
	
	return buf.getString();
}

wstring_ptr qs::AddressListParser::getAddresses() const
{
	StringBuffer<WSTRING> buf;
	
	for (AddressList::const_iterator it = listAddress_.begin(); it != listAddress_.end(); ++it) {
		AddressParser* pAddress = *it;
		const AddressListParser* pGroup = pAddress->getGroup();
		if (pGroup) {
			const AddressListParser::AddressList& groups = pGroup->getAddressList();
			for (AddressListParser::AddressList::const_iterator itG = groups.begin(); itG != groups.end(); ++itG) {
				if (buf.getLength() != 0)
					buf.append(L",");
				wstring_ptr wstrAddress((*itG)->getAddress());
				buf.append(wstrAddress.get());
			}
		}
		else {
			if (buf.getLength() != 0)
				buf.append(L",");
			wstring_ptr wstrAddress(pAddress->getAddress());
			buf.append(wstrAddress.get());
		}
	}
	
	return buf.getString();
}

const AddressListParser::AddressList& qs::AddressListParser::getAddressList() const
{
	return listAddress_;
}

void qs::AddressListParser::appendAddress(std::auto_ptr<AddressParser> pAddress)
{
	listAddress_.push_back(pAddress.get());
	pAddress.release();
}

void qs::AddressListParser::insertAddress(AddressParser* pAddressRef,
										  std::auto_ptr<AddressParser> pAddress)
{
	AddressList::iterator it = std::find(listAddress_.begin(),
		listAddress_.end(), pAddressRef);
	assert(it != listAddress_.end());
	listAddress_.insert(it, pAddress.get());
	pAddress.release();
}

void qs::AddressListParser::removeAddress(AddressParser* pAddress)
{
	AddressList::iterator it = std::find(listAddress_.begin(),
		listAddress_.end(), pAddress);
	assert(it != listAddress_.end());
	listAddress_.erase(it);
}

void qs::AddressListParser::removeAllAddresses()
{
	std::for_each(listAddress_.begin(), listAddress_.end(), deleter<AddressParser>());
	listAddress_.clear();
}

void qs::AddressListParser::replaceAddress(AddressParser* pAddressOld,
										   std::auto_ptr<AddressParser> pAddressNew)
{
	AddressList::iterator it = std::find(
		listAddress_.begin(), listAddress_.end(), pAddressOld);
	assert(it != listAddress_.end());
	*it = pAddressNew.release();
}

bool qs::AddressListParser::contains(const WCHAR* pwszAddress) const
{
	return std::find_if(listAddress_.begin(), listAddress_.end(),
		std::bind2nd(std::mem_fun(&AddressParser::contains), pwszAddress)) != listAddress_.end();
}

Part::Field qs::AddressListParser::parse(const Part& part,
										 const WCHAR* pwszName)
{
	assert(pwszName);
	
	string_ptr strValue(part.getRawField(pwszName, 0));
	if (!strValue.get())
		return Part::FIELD_NOTEXIST;
	
	if (!(nFlags_ & FLAG_SINGLEFIELD)) {
		StringBuffer<STRING> buf(strValue.get());
		unsigned int n = 1;
		while (true) {
			string_ptr str(part.getRawField(pwszName, n));
			if (!str.get())
				break;
			
			buf.append(", ");
			buf.append(str.get());
			
			++n;
		}
		strValue = buf.getString();
	}
	
	unsigned int nFlags = Tokenizer::F_RECOGNIZECOMMENT |
		Tokenizer::F_RECOGNIZEDOMAIN | Tokenizer::F_SPECIAL;
	Tokenizer t(strValue.get(), -1, nFlags);
	return parseAddressList(part, t);
}

string_ptr qs::AddressListParser::unparse(const Part& part) const
{
	StringBuffer<STRING> buf;
	
	for (AddressList::const_iterator it = listAddress_.begin(); it != listAddress_.end(); ++it) {
		if (it != listAddress_.begin())
			buf.append(",\r\n ");
		string_ptr str((*it)->unparse(part));
		if (!str.get())
			return 0;
		buf.append(str.get());
	}
	
	return buf.getString();
}

size_t qs::AddressListParser::getMaxAddresses()
{
	return nMax__;
}

void qs::AddressListParser::setMaxAddresses(size_t nMax)
{
	nMax__ = nMax;
}

Part::Field qs::AddressListParser::parseAddressList(const Part& part,
													Tokenizer& t)
{
	unsigned int nFlags = 0;
	if (nFlags_ & FLAG_DISALLOWGROUP)
		nFlags |= AddressParser::FLAG_DISALLOWGROUP;
	if ((nFlags_ & FLAG_GROUP) == FLAG_GROUP)
		nFlags |= AddressParser::FLAG_INGROUP;
	if (nFlags_ & FLAG_ALLOWUTF8)
		nFlags |= AddressParser::FLAG_ALLOWUTF8;
	
	struct Deleter
	{
		Deleter(AddressListParser::AddressList& l) :
			p_(&l)
		{
		}
		
		~Deleter()
		{
			if (p_) {
				std::for_each(p_->begin(), p_->end(), qs::deleter<AddressParser>());
				p_->clear();
			}
		}
		
		void release() { p_ = 0; }
		
		AddressListParser::AddressList* p_;
	} deleter(listAddress_);
	
	while (true) {
		std::auto_ptr<AddressParser> pParser(new AddressParser(nFlags));
		
		bool bEnd = false;
		Part::Field field = pParser->parseAddress(part, t,
			(nFlags_ & FLAG_GROUP) == FLAG_GROUP ? &bEnd : 0);
		if (field != Part::FIELD_EXIST)
			return Part::FIELD_ERROR;
		
		if (!pParser->getMailbox() && !pParser->getGroup())
			break;
		
		listAddress_.push_back(pParser.get());
		pParser.release();
		
		if (listAddress_.size() >= nMax_)
			break;
		
		if (bEnd)
			break;
	}
	deleter.release();
	
	return Part::FIELD_EXIST;
}


/****************************************************************************
 *
 * MessageIdParser
 *
 */

qs::MessageIdParser::MessageIdParser()
{
}

qs::MessageIdParser::MessageIdParser(const WCHAR* pwszMessageId)
{
	wstrMessageId_ = allocWString(pwszMessageId);
}

qs::MessageIdParser::~MessageIdParser()
{
}

const WCHAR* qs::MessageIdParser::getMessageId() const
{
	return wstrMessageId_.get();
}

Part::Field qs::MessageIdParser::parse(const Part& part,
									   const WCHAR* pwszName)
{
	assert(pwszName);
	
	string_ptr strValue(part.getRawField(pwszName, 0));
	if (!strValue.get())
		return Part::FIELD_NOTEXIST;
	
	Tokenizer t(strValue.get(), -1,
		Tokenizer::F_RECOGNIZECOMMENT | Tokenizer::F_SPECIAL);
	State state = S_BEGIN;
	string_ptr strMessageId;
	while (state != S_END) {
		Tokenizer::Token token(t.getToken());
		
		if (token.type_ == Tokenizer::T_COMMENT)
			continue;
		
		switch (state) {
		case S_BEGIN:
			if (token.type_ != Tokenizer::T_SPECIAL || *token.str_.get() != '<') {
				return parseError();
			}
			else {
				AddrSpecParser addrSpec;
				string_ptr strMailbox;
				string_ptr strHost;
				string_ptr strComment;
				Part::Field field = addrSpec.parseAddrSpec(part, t,
					AddrSpecParser::S_BEGIN, AddrSpecParser::TYPE_INBRACKET,
					&strMailbox, &strHost, &strComment, 0);
				if (field != Part::FIELD_EXIST)
					return parseError();
				strMessageId = AddressParser::getAddrSpec(
					strMailbox.get(), strHost.get());
				state = S_ADDRSPEC;
			}
			break;
		case S_ADDRSPEC:
			if (token.type_ != Tokenizer::T_END)
				return parseError();
			state = S_END;
			break;
		case S_END:
			break;
		}
	}
	
	wstrMessageId_ = mbs2wcs(strMessageId.get());
	
	return Part::FIELD_EXIST;
}

string_ptr qs::MessageIdParser::unparse(const Part& part) const
{
	assert(wstrMessageId_.get());
	
	if (!FieldParserUtil<WSTRING>::isAscii(wstrMessageId_.get()))
		return 0;
	
	string_ptr str(wcs2mbs(wstrMessageId_.get()));
	return concat("<", str.get(), ">");
}


/****************************************************************************
 *
 * ReferencesParser
 *
 */

size_t qs::ReferencesParser::nMax__ = 32;

qs::ReferencesParser::ReferencesParser() :
	nMax_(nMax__)
{
}

qs::ReferencesParser::ReferencesParser(size_t nMax) :
	nMax_(nMax)
{
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

wstring_ptr qs::ReferencesParser::getValue() const
{
	StringBuffer<WSTRING> buf;
	
	for (ReferenceList::const_iterator it = listReference_.begin(); it != listReference_.end(); ++it) {
		if (it != listReference_.begin())
			buf.append(L' ');
		
		switch ((*it).second) {
		case T_MSGID:
			buf.append(L'<');
			buf.append((*it).first);
			buf.append(L'>');
			break;
		case T_PHRASE:
			{
				wstring_ptr wstrAtoms(FieldParserUtil<WSTRING>::getAtomsOrQString((*it).first, -1));
				buf.append(wstrAtoms.get());
			}
			break;
		default:
			assert(false);
			break;
		}
	}
	
	return buf.getString();
}

Part::Field qs::ReferencesParser::parse(const Part& part,
										const WCHAR* pwszName)
{
	assert(pwszName);
	
	string_ptr strValue(part.getRawField(pwszName, 0));
	if (!strValue.get())
		return Part::FIELD_NOTEXIST;
	
	Tokenizer t(strValue.get(), -1,
		Tokenizer::F_RECOGNIZECOMMENT | Tokenizer::F_SPECIAL);
	
	AddrSpecParser addrSpec;
	
	State state = S_BEGIN;
	StringBuffer<STRING> buf;
	while (state != S_END) {
		Tokenizer::Token token(t.getToken());
		
		if (token.type_ == Tokenizer::T_COMMENT)
			continue;
		
		switch (state) {
		case S_BEGIN:
			if (token.type_ == Tokenizer::T_ATOM ||
				token.type_ == Tokenizer::T_QSTRING) {
				if (buf.getLength() != 0)
					buf.append(' ');
				buf.append(token.str_.get());
			}
			else if (token.type_ == Tokenizer::T_SPECIAL && *token.str_.get() == '<') {
				if (buf.getLength() != 0) {
					wstring_ptr str(mbs2wcs(buf.getCharArray()));
					listReference_.push_back(std::make_pair(str.get(), T_PHRASE));
					str.release();
					buf.remove();
				}
				
				string_ptr strMailbox;
				string_ptr strHost;
				string_ptr strComment;
				Part::Field field = addrSpec.parseAddrSpec(part, t,
					AddrSpecParser::S_BEGIN, AddrSpecParser::TYPE_INBRACKET,
					&strMailbox, &strHost, &strComment, 0);
				if (field != Part::FIELD_EXIST)
					return parseError();
				string_ptr strAddrSpec(AddressParser::getAddrSpec(
					strMailbox.get(), strHost.get()));
				wstring_ptr str(mbs2wcs(strAddrSpec.get()));
				listReference_.push_back(std::make_pair(str.get(), T_MSGID));
				str.release();
			}
			else if (token.type_ == Tokenizer::T_SPECIAL &&
				part.isOption(Part::O_ALLOW_SPECIALS_IN_REFERENCES)) {
				if (buf.getLength() != 0)
					buf.append(' ');
				buf.append(token.str_.get());
			}
			else if (token.type_ == Tokenizer::T_END) {
				if (buf.getLength() != 0) {
					wstring_ptr str(mbs2wcs(buf.getCharArray()));
					listReference_.push_back(std::make_pair(str.get(), T_PHRASE));
					str.release();
					buf.remove();
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
		
		if (listReference_.size() >= nMax_)
			break;
	}
	
	return Part::FIELD_EXIST;
}

string_ptr qs::ReferencesParser::unparse(const Part& part) const
{
	StringBuffer<STRING> buf;
	
	for (ReferenceList::const_iterator it = listReference_.begin(); it != listReference_.end(); ++it) {
		if (it != listReference_.begin())
			buf.append("\r\n ");
		assert(FieldParserUtil<WSTRING>::isAscii((*it).first));
		
		string_ptr str(wcs2mbs((*it).first));
		
		switch ((*it).second) {
		case T_MSGID:
			buf.append('<');
			buf.append(str.get());
			buf.append('>');
			break;
		case T_PHRASE:
			{
				string_ptr strAtoms(FieldParserUtil<STRING>::getAtomsOrQString(str.get(), -1));
				buf.append(strAtoms.get());
			}
			break;
		default:
			assert(false);
			break;
		}
	}
	
	return buf.getString();
}

size_t qs::ReferencesParser::getMaxReferences()
{
	return nMax__;
}

void qs::ReferencesParser::setMaxReferences(size_t nMax)
{
	nMax__ = nMax;
}


/****************************************************************************
 *
 * ParameterFieldParser
 *
 */

size_t qs::ParameterFieldParser::nMax__ = 32;

qs::ParameterFieldParser::ParameterFieldParser(size_t nMax) :
	nMax_(nMax)
{
}

qs::ParameterFieldParser::~ParameterFieldParser()
{
	std::for_each(listParameter_.begin(), listParameter_.end(),
		unary_compose_fx_gx(
			string_free<WSTRING>(),
			string_free<WSTRING>()));
}

wstring_ptr qs::ParameterFieldParser::getParameter(const WCHAR* pwszName) const
{
	assert(pwszName);
	
	const WCHAR* pwszValue = getRawParameter(pwszName);
	if (!pwszValue) {
		wstring_ptr wstrName(concat(pwszName, L"*"));
		pwszValue = getRawParameter(wstrName.get());
		std::auto_ptr<Converter> pConverter;
		string_ptr strDecode;
		if (pwszValue) {
			strDecode = decode2231FirstValue(pwszValue, &pConverter);
			if (!strDecode.get())
				return 0;
		}
		else {
			bool bFirstExtended = false;
			wstring_ptr wstrName(concat(pwszName, L"*0"));
			pwszValue = getRawParameter(wstrName.get());
			if (pwszValue) {
				strDecode = wcs2mbs(pwszValue);
			}
			else {
				wstring_ptr wstrName(concat(pwszName, L"*0*"));
				pwszValue = getRawParameter(wstrName.get());
				if (pwszValue) {
					strDecode = decode2231FirstValue(pwszValue, &pConverter);
					if (!strDecode.get())
						return 0;
					bFirstExtended = true;
				}
			}
			if (pwszValue) {
				const size_t nLen = wcslen(pwszName) + 10;
				wstring_ptr wstrName(allocWString(nLen));
				StringBuffer<STRING> buf(strDecode.get());
				bool bExistOther = true;
				for (int n = 1; bExistOther; ++n) {
					for (int m = 0; m < (bFirstExtended ? 2 : 1); ++m) {
						bool bExtended = m == 1;
						_snwprintf(wstrName.get(), nLen, L"%s*%d%c", pwszName, n,
							bExtended ? L'*' : L'\0');
						const WCHAR* pwsz = getRawParameter(wstrName.get());
						bExistOther = pwsz != 0;
						if (pwsz) {
							if (bExtended) {
								string_ptr str(decode2231Value(pwsz));
								buf.append(str.get());
							}
							else {
								string_ptr str(wcs2mbs(pwsz));
								buf.append(str.get());
							}
							break;
						}
					}
				}
				strDecode = buf.getString();
			}
		}
		if (pwszValue) {
			if (pConverter.get()) {
				size_t nLen = strlen(strDecode.get());
				wxstring_size_ptr decoded(pConverter->decode(strDecode.get(), &nLen));
				if (!decoded.get())
					return 0;
				return allocWString(decoded.get(), decoded.size());
			}
			else {
				return mbs2wcs(strDecode.get());
			}
		}
		else {
			return 0;
		}
	}
	else {
		return allocWString(pwszValue);
	}
}

void qs::ParameterFieldParser::setParameter(const WCHAR* pwszName,
											const WCHAR* pwszValue)
{
	setRawParameter(pwszName, pwszValue);
}

Part::Field qs::ParameterFieldParser::parseParameter(const Part& part,
													 Tokenizer& t,
													 State state)
{
	string_ptr strName;
	while (state != S_END) {
		Tokenizer::Token token(t.getToken());
		
		if (token.type_ == Tokenizer::T_COMMENT)
			continue;
		
		switch (state) {
		case S_BEGIN:
		case S_VALUE:
			if (token.type_ == Tokenizer::T_END)
				state = S_END;
			else if (token.type_ == Tokenizer::T_SPECIAL && *token.str_.get() == ';')
				state = S_SEMICOLON;
			else
				return parseError();
			break;
		case S_SEMICOLON:
			if (token.type_ == Tokenizer::T_END &&
				part.isOption(Part::O_ALLOW_PARAMETER_INVALID_SEMICOLON)) {
				state = S_END;
			}
			else if (token.type_ == Tokenizer::T_SPECIAL &&
				*token.str_.get() == ';' &&
				part.isOption(Part::O_ALLOW_PARAMETER_INVALID_SEMICOLON)) {
				state = S_SEMICOLON;
			}
			else if (token.type_ == Tokenizer::T_ATOM) {
				strName = token.str_;
				state = S_NAME;
			}
			else {
				return parseError();
			}
			break;
		case S_NAME:
			if (token.type_ != Tokenizer::T_SPECIAL || *token.str_.get() != '=')
				return parseError();
			state = S_EQUAL;
			break;
		case S_EQUAL:
			if (token.type_ != Tokenizer::T_ATOM && token.type_ != Tokenizer::T_QSTRING) {
				return parseError();
			}
			else {
				wstring_ptr wstrValue;
				if (part.isOption(Part::O_ALLOW_RAW_PARAMETER)) {
					size_t nLen = strlen(token.str_.get());
					wxstring_size_ptr wstr(UTF8Converter().decode(token.str_.get(), &nLen));
					if (!wstr.get())
						return parseError();
					else if (!FieldParserUtil<WSTRING>::isAscii(wstr.get(), wstr.size()))
						wstrValue = allocWString(wstr.get(), wstr.size());
				}
				if (!wstrValue.get()) {
					if (part.isOption(Part::O_ALLOW_ENCODED_PARAMETER) &&
						(part.isOption(Part::O_ALLOW_ENCODED_QSTRING) || token.type_ == Tokenizer::T_ATOM))
						wstrValue = decode(token.str_.get(), -1, false, 0);
					else
						wstrValue = mbs2wcs(token.str_.get());
				}
				wstring_ptr wstrName(mbs2wcs(strName.get()));
				listParameter_.push_back(std::make_pair(wstrName.get(), wstrValue.get()));
				wstrName.release();
				wstrValue.release();
				state = S_VALUE;
			}
			break;
		case S_END:
			break;
		}
		
		if (listParameter_.size() >= nMax_)
			break;
	}
	
	return Part::FIELD_EXIST;
}

string_ptr qs::ParameterFieldParser::unparseParameter(const Part& part) const
{
	StringBuffer<STRING> buf;
	
	for (ParameterList::const_iterator it = listParameter_.begin(); it != listParameter_.end(); ++it) {
		if (!FieldParserUtil<WSTRING>::isAscii((*it).first))
			return 0;
		
		const size_t nAsciiMax = 40;
		const size_t nNonAsciiMax = 10;
		string_ptr strParamName(wcs2mbs((*it).first));
		string_ptr strParamValue;
		const WCHAR* pwsz = (*it).second;
		string_ptr strValue(wcs2mbs(pwsz));
		size_t nLen = wcslen(pwsz);
		bool bAscii = FieldParserUtil<WSTRING>::isAscii(pwsz);
		bool bRFC2231Processed = false;
		if (part.isOption(Part::O_RFC2231) && (!bAscii || nLen > nAsciiMax)) {
			if (bAscii) {
				for (unsigned int n = 0; n < nLen/nAsciiMax + (nLen%nAsciiMax ? 1 : 0); ++n) {
					buf.append(";\r\n ");
					buf.append(strParamName.get());
					CHAR sz[32];
					sprintf(sz, "*%d=", n);
					buf.append(sz);
					
					const CHAR* p = strValue.get() + n*nAsciiMax;
					size_t nThisLen = QSMIN(nAsciiMax, strlen(p));
					string_ptr strValue;
					if (part.isOption(Part::O_FORCE_QSTRING_PARAMETER))
						strValue = FieldParserUtil<STRING>::getQString(p, nThisLen);
					else
						strValue = FieldParserUtil<STRING>::getAtomOrQString(p, nThisLen);
					buf.append(strValue.get());
				}
			}
			else {
				wstring_ptr wstrCharset(part.getHeaderCharset());
				std::auto_ptr<Converter> pConverter(
					ConverterFactory::getInstance(wstrCharset.get()));
				
				string_ptr strValue;
				if (pConverter.get()) {
					size_t nLen = wcslen(pwsz);
					xstring_size_ptr encoded(pConverter->encode(pwsz, &nLen));
					if (!encoded.get())
						return 0;
					strValue = allocString(encoded.get());
				}
				else {
					strValue = wcs2mbs(pwsz);
				}
				size_t nLen = strlen(strValue.get());
				
				for (unsigned int n = 0; n < nLen/nNonAsciiMax + (nLen%nNonAsciiMax ? 1 : 0); ++n) {
					buf.append(";\r\n ");
					buf.append(strParamName.get());
					if (nLen > nNonAsciiMax) {
						CHAR sz[32];
						sprintf(sz, "*%d", n);
						buf.append(sz);
					}
					buf.append("*=");
					
					if (n == 0) {
						string_ptr strCharset(wcs2mbs(wstrCharset.get()));
						buf.append(strCharset.get());
						buf.append("\'\'");
					}
					const CHAR* p = strValue.get() + n*nNonAsciiMax;
					size_t nThisLen = QSMIN(nNonAsciiMax, strlen(p));
					string_ptr strEncoded(encode2231Value(p, nThisLen));
					buf.append(strEncoded.get());
				}
			}
			bRFC2231Processed = true;
		}
		else if (bAscii) {
			if (part.isOption(Part::O_FORCE_QSTRING_PARAMETER))
				strParamValue = FieldParserUtil<STRING>::getQString(strValue.get(), -1);
			else
				strParamValue = FieldParserUtil<STRING>::getAtomOrQString(strValue.get(), -1);
		}
		else if (part.isOption(Part::O_ALLOW_ENCODED_PARAMETER)) {
			wstring_ptr wstrCharset(part.getHeaderCharset());
			strValue = encode(pwsz, -1, wstrCharset.get(), 0, true, true);
			if (!strValue.get())
				return 0;
			strParamValue = concat("\"", strValue.get(), "\"");
		}
		
		if (!bRFC2231Processed) {
			buf.append(";\r\n ");
			buf.append(strParamName.get());
			buf.append('=');
			buf.append(strParamValue.get());
		}
	}
	
	return buf.getString();
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

void qs::ParameterFieldParser::setRawParameter(const WCHAR* pwszName,
											   const WCHAR* pwszValue)
{
	assert(pwszName);
	assert(pwszValue);
	
	wstring_ptr wstrValue(allocWString(pwszValue));
	
	ParameterList::iterator it = listParameter_.begin();
	while (it != listParameter_.end()) {
		if (_wcsicmp((*it).first, pwszName) == 0)
			break;
		++it;
	}
	if (it == listParameter_.end()) {
		wstring_ptr wstrName(allocWString(pwszName));
		listParameter_.push_back(std::make_pair(wstrName.get(), wstrValue.get()));
		wstrName.release();
		wstrValue.release();
	}
	else {
		freeWString((*it).second);
		(*it).second = wstrValue.release();
	}
}

string_ptr qs::ParameterFieldParser::decode2231FirstValue(const WCHAR* pwszValue,
														  std::auto_ptr<Converter>* ppConverter)
{
	assert(pwszValue);
	assert(ppConverter);
	
	const WCHAR* p = wcschr(pwszValue, L'\'');
	if (!p || p == pwszValue)
		return 0;
	
	wstring_ptr wstrCharset(allocWString(pwszValue, p - pwszValue));
	
	p = wcschr(p + 1, L'\'');
	if (!p)
		return 0;
	
	std::auto_ptr<Converter> pConverter(ConverterFactory::getInstance(wstrCharset.get()));
	
	string_ptr strDecode(decode2231Value(p + 1));
	*ppConverter = pConverter;
	return strDecode;
}

string_ptr qs::ParameterFieldParser::decode2231Value(const WCHAR* pwszValue)
{
	assert(pwszValue);
	
	StringBuffer<STRING> buf;
	
	for (const WCHAR* p = pwszValue; *p; ++p) {
		CHAR c = static_cast<CHAR>(*p);
		if (c == '%' && isHex(*(p + 1)) && isHex(*(p + 2))) {
			buf.append(decodeHex(p + 1));
			p += 2;
		}
		else {
			buf.append(c);
		}
	}
	
	return buf.getString();
}

string_ptr qs::ParameterFieldParser::encode2231Value(const CHAR* pszValue,
													 size_t nLen)
{
	assert(pszValue);
	
	StringBuffer<STRING> buf;
	
	for (const CHAR* p = pszValue; p < pszValue + nLen; ++p) {
		unsigned char c = *p;
		if (('a' <= c && c <= 'z') || ('A' <= c && c <= 'Z') ||
			('0' <= c && c <= '9') || c == '.') {
			buf.append(c);
		}
		else {
			buf.append('%');
			CHAR sz[2];
			encodeHex(c, sz);
			buf.append(sz, 2);
		}
	}
	
	return buf.getString();
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

void qs::ParameterFieldParser::encodeHex(unsigned char c,
										 CHAR* pszEncoded)
{
	assert(pszEncoded);
	
	for (int n = 0; n < 2; ++n) {
		unsigned char cValue = (n == 0 ? c >> 4 : c) & 0x0f;
		pszEncoded[n] = cValue < 10 ? cValue + L'0' : cValue - 10 + L'A';
	}
}

size_t qs::ParameterFieldParser::getMaxParameters()
{
	return nMax__;
}

void qs::ParameterFieldParser::setMaxParameters(size_t nMax)
{
	nMax__ = nMax;
}


/****************************************************************************
 *
 * SimpleParameterParser
 *
 */

qs::SimpleParameterParser::SimpleParameterParser() :
	ParameterFieldParser(getMaxParameters())
{
}

qs::SimpleParameterParser::SimpleParameterParser(size_t nMax) :
	ParameterFieldParser(nMax)
{
}

qs::SimpleParameterParser::~SimpleParameterParser()
{
}

const WCHAR* qs::SimpleParameterParser::getValue() const
{
	return wstrValue_.get();
}

Part::Field qs::SimpleParameterParser::parse(const Part& part,
											 const WCHAR* pwszName)
{
	assert(pwszName);
	
	string_ptr strValue(part.getRawField(pwszName, 0));
	if (!strValue.get())
		return Part::FIELD_NOTEXIST;
	
	Tokenizer t(strValue.get(), -1, Tokenizer::F_TSPECIAL);
	
	StringBuffer<WSTRING> buf;
	bool bLastSpecial = false;
	
	Tokenizer::Token token(t.getToken());
	while (true) {
		if (token.type_ == Tokenizer::T_END ||
			(token.type_ == Tokenizer::T_SPECIAL && *token.str_.get() == ';')) {
			break;
		}
		else if (token.type_ != Tokenizer::T_COMMENT) {
			if (buf.getLength() != 0 && !bLastSpecial &&
				token.type_ != Tokenizer::T_SPECIAL)
				buf.append(L" ");
			wstring_ptr str(mbs2wcs(token.str_.get()));
			buf.append(str.get());
			bLastSpecial = token.type_ == Tokenizer::T_SPECIAL;
		}
		token = t.getToken();
	}
	if (token.type_ != Tokenizer::T_END) {
		if (parseParameter(part, t, ParameterFieldParser::S_SEMICOLON) == Part::FIELD_ERROR)
			return Part::FIELD_ERROR;
	}
	
	wstrValue_ = buf.getString();
	
	return Part::FIELD_EXIST;
}

string_ptr qs::SimpleParameterParser::unparse(const Part& part) const
{
	string_ptr strValue(wcs2mbs(wstrValue_.get()));
	
	string_ptr strParameter(unparseParameter(part));
	if (!strParameter.get())
		return 0;
	
	return concat(strValue.get(), strParameter.get());
}


/****************************************************************************
 *
 * ContentTypeParser
 *
 */

qs::ContentTypeParser::ContentTypeParser() :
	ParameterFieldParser(getMaxParameters())
{
}

qs::ContentTypeParser::ContentTypeParser(size_t nMax) :
	ParameterFieldParser(nMax)
{
}

qs::ContentTypeParser::ContentTypeParser(const WCHAR* pwszMediaType,
										 const WCHAR* pwszSubType) :
	ParameterFieldParser(getMaxParameters())
{
	wstrMediaType_ = allocWString(pwszMediaType);
	wstrSubType_ = allocWString(pwszSubType);
}

qs::ContentTypeParser::~ContentTypeParser()
{
}

const WCHAR* qs::ContentTypeParser::getMediaType() const
{
	return wstrMediaType_.get();
}

const WCHAR* qs::ContentTypeParser::getSubType() const
{
	return wstrSubType_.get();
}

Part::Field qs::ContentTypeParser::parse(const Part& part,
										 const WCHAR* pwszName)
{
	assert(pwszName);
	
	string_ptr strValue(part.getRawField(pwszName, 0));
	if (!strValue.get())
		return Part::FIELD_NOTEXIST;
	
	if (part.isOption(Part::O_ALLOW_RAW_PARAMETER)) {
		string_ptr str(convertToUTF8(strValue.get()));
		if (str.get())
			strValue = str;
	}
	
	Tokenizer t(strValue.get(), -1,
		Tokenizer::F_TSPECIAL | Tokenizer::F_RECOGNIZECOMMENT);
	
	State state = S_BEGIN;
	while (state != S_END) {
		Tokenizer::Token token(t.getToken());
		
		if (token.type_ == Tokenizer::T_COMMENT)
			continue;
		
		switch (state) {
		case S_BEGIN:
			if (token.type_ != Tokenizer::T_ATOM)
				return parseError();
			wstrMediaType_ = mbs2wcs(token.str_.get());
			state = S_MEDIATYPE;
			break;
		case S_MEDIATYPE:
			if (token.type_ != Tokenizer::T_SPECIAL || *token.str_.get() != '/')
				return parseError();
			state = S_SLASH;
			break;
		case S_SLASH:
			if (token.type_ != Tokenizer::T_ATOM)
				return parseError();
			wstrSubType_ = mbs2wcs(token.str_.get());
			state = S_END;
			break;
		case S_END:
			break;
		}
	}
	
	return parseParameter(part, t, ParameterFieldParser::S_BEGIN);
}

string_ptr qs::ContentTypeParser::unparse(const Part& part) const
{
	string_ptr strMediaType(wcs2mbs(wstrMediaType_.get()));
	string_ptr strSubType(wcs2mbs(wstrSubType_.get()));
	
	string_ptr strParameter(unparseParameter(part));
	if (!strParameter.get())
		return 0;
	
	Concat c[] = {
		{ strMediaType.get(),	-1	},
		{ "/",					1	},
		{ strSubType.get(),		-1	},
		{ strParameter.get(),	-1	}
	};
	return concat(c, countof(c));
}


/****************************************************************************
 *
 * ContentDispositionParser
 *
 */

qs::ContentDispositionParser::ContentDispositionParser() :
	ParameterFieldParser(getMaxParameters())
{
}

qs::ContentDispositionParser::ContentDispositionParser(size_t nMax) :
	ParameterFieldParser(nMax)
{
}

qs::ContentDispositionParser::ContentDispositionParser(const WCHAR* pwszDispositionType) :
	ParameterFieldParser(getMaxParameters())
{
	wstrDispositionType_ = allocWString(pwszDispositionType);
}

qs::ContentDispositionParser::~ContentDispositionParser()
{
}

const WCHAR* qs::ContentDispositionParser::getDispositionType() const
{
	return wstrDispositionType_.get();
}

Part::Field qs::ContentDispositionParser::parse(const Part& part,
												const WCHAR* pwszName)
{
	assert(pwszName);
	
	string_ptr strValue(part.getRawField(pwszName, 0));
	if (!strValue.get())
		return Part::FIELD_NOTEXIST;
	
	if (part.isOption(Part::O_ALLOW_RAW_PARAMETER)) {
		string_ptr str(convertToUTF8(strValue.get()));
		if (str.get())
			strValue = str;
	}
	
	Tokenizer t(strValue.get(), -1, Tokenizer::F_TSPECIAL);
	
	Tokenizer::Token token(t.getToken());
	if (token.type_ != Tokenizer::T_ATOM)
		return parseError();
	wstrDispositionType_ = mbs2wcs(token.str_.get());
	
	return parseParameter(part, t, ParameterFieldParser::S_BEGIN);
}

string_ptr qs::ContentDispositionParser::unparse(const Part& part) const
{
	string_ptr strDispositionType(wcs2mbs(wstrDispositionType_.get()));
	
	string_ptr strParameter(unparseParameter(part));
	if (!strParameter.get())
		return 0;
	
	return concat(strDispositionType.get(), strParameter.get());
}


/****************************************************************************
 *
 * ContentTransferEncodingParser
 *
 */

qs::ContentTransferEncodingParser::ContentTransferEncodingParser() :
	parser_(SimpleParser::FLAG_RECOGNIZECOMMENT | SimpleParser::FLAG_TSPECIAL)
{
}

qs::ContentTransferEncodingParser::ContentTransferEncodingParser(const WCHAR* pwszEncoding) :
	parser_(pwszEncoding, SimpleParser::FLAG_RECOGNIZECOMMENT | SimpleParser::FLAG_TSPECIAL)
{
}

qs::ContentTransferEncodingParser::~ContentTransferEncodingParser()
{
}

const WCHAR* qs::ContentTransferEncodingParser::getEncoding() const
{
	return parser_.getValue();
}

Part::Field qs::ContentTransferEncodingParser::parse(const Part& part,
													 const WCHAR* pwszName)
{
	return parser_.parse(part, pwszName);
}

string_ptr qs::ContentTransferEncodingParser::unparse(const Part& part) const
{
	return parser_.unparse(part);
}
