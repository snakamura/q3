/*
 * $Id: xmlparser.cpp,v 1.1.1.1 2003/04/29 08:07:37 snakamura Exp $
 *
 * Copyright(C) 1998-2003 Satoshi Nakamura
 * All rights reserved.
 *
 */

#include <qsconv.h>
#include <qsnew.h>
#include <qsstream.h>

#include <algorithm>

#include "sax.h"
#include "xmlparser.h"

#pragma warning(disable:4786)

using namespace qs;


/****************************************************************************
 *
 * XMLParser
 *
 */

qs::XMLParser::XMLParser(ContentHandler* pContentHandler,
	unsigned int nFlags, QSTATUS* pstatus) :
	pContentHandler_(pContentHandler),
	nFlags_(nFlags)
{
	assert(pContentHandler);
	assert(pstatus);
	*pstatus = QSTATUS_SUCCESS;
}

qs::XMLParser::~XMLParser()
{
}

QSTATUS qs::XMLParser::parse(const InputSource& source)
{
	DECLARE_QSTATUS();
	
	InputStream* pInputStream = source.getByteStream();
	Reader* pReader = 0;
	if (!pInputStream)
		pReader = source.getCharacterStream();
	std::auto_ptr<BufferedInputStream> pBufferedInputStream;
	if (!pInputStream && !pReader) {
		const WCHAR* pwszSystemId = source.getSystemId();
		if (!pwszSystemId)
			return QSTATUS_FAIL;
		std::auto_ptr<FileInputStream> pStream;
		status = newQsObject(pwszSystemId, &pStream);
		CHECK_QSTATUS();
		status = newQsObject(pStream.get(), true, &pBufferedInputStream);
		CHECK_QSTATUS();
		pStream.release();
		pInputStream = pBufferedInputStream.get();
	}
	assert(pInputStream || pReader);
	
	std::auto_ptr<Reader> apReader;
	if (pInputStream) {
		status = parseXmlDecl(pInputStream, &pReader);
		CHECK_QSTATUS();
		apReader.reset(pReader);
	}
	else {
		status = parseXmlDecl(pReader);
		CHECK_QSTATUS();
	}
	assert(pReader);
	
	if (pContentHandler_) {
		status = pContentHandler_->startDocument();
		CHECK_QSTATUS();
	}
	
	XMLParserContext context(this, pReader,
		XMLParserContext::WAIT_STARTELEMENT |
		XMLParserContext::WAIT_PROCESSINGINSTRUCTION |
		XMLParserContext::WAIT_COMMENT |
		XMLParserContext::WAIT_WS);
	status = parse(context);
	CHECK_QSTATUS();
	
	if (pContentHandler_) {
		status = pContentHandler_->endDocument();
		CHECK_QSTATUS();
	}
	
	return QSTATUS_SUCCESS;
}

QSTATUS qs::XMLParser::validateQName(const WCHAR* pwsz) const
{
	if (nFlags_ & FLAG_NAMESPACES)
		return isQName(pwsz) ? QSTATUS_SUCCESS : QSTATUS_FAIL;
	else
		return isName(pwsz) ? QSTATUS_SUCCESS : QSTATUS_FAIL;
}

QSTATUS qs::XMLParser::validateNCName(const WCHAR* pwsz) const
{
	if (nFlags_ & FLAG_NAMESPACES)
		return isNCName(pwsz) ? QSTATUS_SUCCESS : QSTATUS_FAIL;
	else
		return isName(pwsz) ? QSTATUS_SUCCESS : QSTATUS_FAIL;
}

bool qs::XMLParser::isWhitespace(WCHAR c)
{
	return c == L' ' || c == L'\t' || c == L'\n' || c == L'\r';
}

bool qs::XMLParser::isName(const WCHAR* pwsz)
{
	assert(pwsz);
	
	// TODO
	return *pwsz != L'\0';
}

bool qs::XMLParser::isQName(const WCHAR* pwsz)
{
	assert(pwsz);
	
	// TODO
	if (*pwsz == L'\0')
		return false;
	
	const WCHAR* pColon = wcschr(pwsz, L':');
	if (pColon && (pColon == pwsz || *(pColon + 1) == L'\0'))
		return false;
	
	return true;
}

bool qs::XMLParser::isNCName(const WCHAR* pwsz)
{
	assert(pwsz);
	
	// TODO
	return *pwsz != L'\0';
}

QSTATUS qs::XMLParser::parseXmlDecl(InputStream* pInputStream, Reader** ppReader)
{
	assert(pInputStream);
	assert(ppReader);
	
	DECLARE_QSTATUS();
	
	*ppReader = 0;
	
	std::auto_ptr<ResettableInputStream> pStream;
	status = newQsObject(10, pInputStream, &pStream);
	CHECK_QSTATUS();
	
	string_ptr<WSTRING> wstrEncoding;
	
	unsigned char buf[16];
	size_t nRead = 0;
	status = pStream->read(buf, 6, &nRead);
	CHECK_QSTATUS();
	if (nRead != 6)
		return QSTATUS_FAIL;
	if (strncmp(reinterpret_cast<char*>(buf), "<?xml", 5) == 0 && isWhitespace(buf[5])) {
		char c = 0;
		status = skipWhitespace(pStream.get(), &c);
		CHECK_QSTATUS();
		buf[0] = c;
		status = pStream->read(buf + 1, 6, &nRead);
		CHECK_QSTATUS();
		if (nRead != 6 || strncmp(reinterpret_cast<char*>(buf), "version", 7) != 0)
			return QSTATUS_FAIL;
		status = skipWhitespace(pStream.get(), &c);
		CHECK_QSTATUS();
		if (c != '=')
			return QSTATUS_FAIL;
		status = skipWhitespace(pStream.get(), &c);
		CHECK_QSTATUS();
		if (c != '\"' && c != '\'')
			return QSTATUS_FAIL;
		status = pStream->read(buf, 4, &nRead);
		CHECK_QSTATUS();
		if (nRead != 4 || strncmp(reinterpret_cast<char*>(buf), "1.0", 3) != 0 || buf[3] != c)
			return QSTATUS_FAIL;
		status = skipWhitespace(pStream.get(), &c);
		CHECK_QSTATUS();
		if (c != '?') {
			if (c == 'e') {
				buf[0] = c;
				status = pStream->read(buf + 1, 7, &nRead);
				CHECK_QSTATUS();
				if (nRead != 7 || strncmp(reinterpret_cast<char*>(buf), "encoding", 8) != 0)
					return QSTATUS_FAIL;
				status = skipWhitespace(pStream.get(), &c);
				CHECK_QSTATUS();
				if (c != '=')
					return QSTATUS_FAIL;
				status = skipWhitespace(pStream.get(), &c);
				CHECK_QSTATUS();
				if (c != '\"' && c != '\'')
					return QSTATUS_FAIL;
				char cEnd = c;
				StringBuffer<STRING> encoding(&status);
				CHECK_QSTATUS();
				while (true) {
					status = getChar(pStream.get(), reinterpret_cast<unsigned char*>(&c));
					CHECK_QSTATUS();
					if (c == '\0')
						return QSTATUS_FAIL;
					else if (c == cEnd)
						break;
					status = encoding.append(c);
					CHECK_QSTATUS();
				}
				wstrEncoding.reset(mbs2wcs(encoding.getCharArray()));
				if (!wstrEncoding.get())
					return QSTATUS_OUTOFMEMORY;
				status = skipWhitespace(pStream.get(), &c);
				CHECK_QSTATUS();
			}
			if (c == 's') {
				buf[0] = c;
				status = pStream->read(buf + 1, 9, &nRead);
				CHECK_QSTATUS();
				if (nRead != 9 || strncmp(reinterpret_cast<char*>(buf), "standalone", 10) != 0)
					return QSTATUS_FAIL;
				status = skipWhitespace(pStream.get(), &c);
				CHECK_QSTATUS();
				if (c != '=')
					return QSTATUS_FAIL;
				status = skipWhitespace(pStream.get(), &c);
				CHECK_QSTATUS();
				if (c != '\"' && c != '\'')
					return QSTATUS_FAIL;
				status = pStream->read(buf, 4, &nRead);
				CHECK_QSTATUS();
				if (nRead != 4 ||
					(strncmp(reinterpret_cast<char*>(buf), "yes", 3) != 0 || buf[3] != c) &&
					(strncmp(reinterpret_cast<char*>(buf), "no", 2) != 0 || buf[2] != c))
					return QSTATUS_FAIL;
				if (buf[0] == 'y' || isWhitespace(buf[3])) {
					status = skipWhitespace(pStream.get(), &c);
					CHECK_QSTATUS();
				}
				else {
					c = buf[3];
				}
			}
			if (c != '?')
				return QSTATUS_FAIL;
		}
		assert(c == '?');
		status = getChar(pStream.get(), reinterpret_cast<unsigned char*>(&c));
		CHECK_QSTATUS();
		if (c != L'>')
			return QSTATUS_FAIL;
	}
	else {
		status = pStream->reset();
		CHECK_QSTATUS();
	}
	
	const WCHAR* pwszEncoding = wstrEncoding.get();
	if (!pwszEncoding)
		pwszEncoding = L"utf-8";
	
	std::auto_ptr<InputStreamReader> pReader;
	status = newQsObject(pStream.get(), true, pwszEncoding, &pReader);
	CHECK_QSTATUS();
	pStream.release();
	
	*ppReader = pReader.release();
	
	return QSTATUS_SUCCESS;
}

QSTATUS qs::XMLParser::parseXmlDecl(Reader* pReader)
{
	// TODO
	
	return QSTATUS_SUCCESS;
}

QSTATUS qs::XMLParser::parse(XMLParserContext& context)
{
	DECLARE_QSTATUS();
	
	WCHAR c = L'\0';
	status = context.getChar(&c);
	CHECK_QSTATUS();
	while (c != L'\0') {
		WCHAR cNext = L'\0';
		if (c == L'<') {
			status = context.getChar(&c);
			CHECK_QSTATUS();
			if (c == L'\0')
				return QSTATUS_FAIL;
			if (c == '?') {
				status = parsePI(context);
				CHECK_QSTATUS();
			}
			else if (c == '!') {
				status = context.getChar(&c);
				CHECK_QSTATUS();
				if (c == '-') {
					status = parseComment(context);
					CHECK_QSTATUS();
				}
				else if (c == '[') {
					status = parseCDATASection(context);
					CHECK_QSTATUS();
				}
				else {
					return QSTATUS_FAIL;
				}
			}
			else if (c == '/') {
				if (context.isWait(XMLParserContext::WAIT_ENDELEMENT))
					return parseEndElement(context);
				else
					return QSTATUS_FAIL;
			}
			else {
				if (context.isWait(XMLParserContext::WAIT_STARTELEMENT)) {
					status = parseStartElement(context, c);
					CHECK_QSTATUS();
					if (!context.getParentContext())
						context.setWait(XMLParserContext::WAIT_WS,
							XMLParserContext::WAIT_WS |
							XMLParserContext::WAIT_CHARACTER |
							XMLParserContext::WAIT_STARTELEMENT);
				}
				else {
					return QSTATUS_FAIL;
				}
			}
		}
		else {
			if (context.isWait(XMLParserContext::WAIT_CHARACTER)) {
				status = parseCharacter(context, c, &cNext);
				CHECK_QSTATUS();
			}
			else if (context.isWait(XMLParserContext::WAIT_WS)) {
				if (!isWhitespace(c))
					return QSTATUS_FAIL;
			}
			else {
				return QSTATUS_FAIL;
			}
		}
		
		if (cNext != L'\0') {
			c = cNext;
		}
		else {
			status = context.getChar(&c);
			CHECK_QSTATUS();
		}
	}
	
	return context.isWait(XMLParserContext::WAIT_STARTELEMENT) ?
		QSTATUS_FAIL : QSTATUS_SUCCESS;
}

QSTATUS qs::XMLParser::parseStartElement(XMLParserContext& context, WCHAR c)
{
	assert(c != L'\0');
	
	DECLARE_QSTATUS();
	
	string_ptr<WSTRING> wstrQName;
	WCHAR cNext = L'\0';
	status = context.getString(c, L" \t\n\r>/", &wstrQName, &cNext);
	CHECK_QSTATUS();
	status = validateQName(wstrQName.get());
	CHECK_QSTATUS();
	
	c = cNext;
	while (isWhitespace(c)) {
		status = context.getChar(&c);
		CHECK_QSTATUS();
		if (c == L'\0')
			return QSTATUS_FAIL;
	}
	
	XMLParserContext childContext(&context, wstrQName.get(),
		XMLParserContext::WAIT_STARTELEMENT |
		XMLParserContext::WAIT_ENDELEMENT |
		XMLParserContext::WAIT_PROCESSINGINSTRUCTION |
		XMLParserContext::WAIT_COMMENT |
		XMLParserContext::WAIT_CHARACTER);
	
	AttributeList listAttribute;
	AttributeListDeleter deleter(&listAttribute);
	bool bEmptyTag = false;
	if (c != L'>' && c != L'/') {
		status = parseAttributes(childContext, c, &listAttribute, &cNext);
		CHECK_QSTATUS();
		c = cNext;
	}
	assert(c == L'/' || c == L'>');
	
	if (c == L'/') {
		status = context.getChar(&c);
		CHECK_QSTATUS();
		if (c != L'>')
			return QSTATUS_FAIL;
		bEmptyTag = true;
	}
	
	if (nFlags_ & FLAG_NAMESPACES) {
		status = childContext.fireStartPrefixMappings(pContentHandler_);
		CHECK_QSTATUS();
	}
	
	const WCHAR* pwszNamespaceURI = 0;
	const WCHAR* pwszLocalName = 0;
	if (nFlags_ & FLAG_NAMESPACES) {
		status = childContext.expandQName(wstrQName.get(),
			true, &pwszNamespaceURI, &pwszLocalName);
		CHECK_QSTATUS();
	}
	
	if (pContentHandler_) {
		AttributesImpl attrs(listAttribute);
		status = pContentHandler_->startElement(pwszNamespaceURI,
			pwszLocalName, wstrQName.get(), attrs);
		CHECK_QSTATUS();
	}
	if (bEmptyTag) {
		if (pContentHandler_) {
			status = pContentHandler_->endElement(pwszNamespaceURI,
				pwszLocalName, wstrQName.get());
			CHECK_QSTATUS();
		}
	}
	else {
		status = parse(childContext);
		CHECK_QSTATUS();
	}
	
	if (nFlags_ & FLAG_NAMESPACES) {
		status = childContext.fireEndPrefixMappings(pContentHandler_);
		CHECK_QSTATUS();
	}
	
	return QSTATUS_SUCCESS;
}

QSTATUS qs::XMLParser::parseEndElement(XMLParserContext& context)
{
	DECLARE_QSTATUS();
	
	string_ptr<WSTRING> wstrQName;
	WCHAR c = L'\0';
	status = context.getString(L'\0', L" \t\n\r>", &wstrQName, &c);
	CHECK_QSTATUS();
	status = validateQName(wstrQName.get());
	CHECK_QSTATUS();
	
	const WCHAR* pwszNamespaceURI = 0;
	const WCHAR* pwszLocalName = 0;
	if (nFlags_ & FLAG_NAMESPACES) {
		status = context.expandQName(wstrQName.get(),
			true, &pwszNamespaceURI, &pwszLocalName);
		CHECK_QSTATUS();
	}
	
	while (isWhitespace(c)) {
		status = context.getChar(&c);
		CHECK_QSTATUS();
		if (c == L'\0')
			return QSTATUS_FAIL;
	}
	
	if (c != L'>')
		return QSTATUS_FAIL;
	
	if (wcscmp(wstrQName.get(), context.getQName()) != 0)
		return QSTATUS_FAIL;
	
	if (pContentHandler_) {
		status = pContentHandler_->endElement(
			pwszNamespaceURI, pwszLocalName, wstrQName.get());
		CHECK_QSTATUS();
	}
	
	return QSTATUS_SUCCESS;
}

QSTATUS qs::XMLParser::parseAttributes(XMLParserContext& context,
	WCHAR c, AttributeList* pListAttribute, WCHAR* pNext)
{
	assert(pListAttribute);
	assert(pNext);
	
	DECLARE_QSTATUS();
	
	AttributeListDeleter deleter(pListAttribute);
	do {
		string_ptr<WSTRING> wstrQName;
		WCHAR cNext = L'\0';
		status = context.getString(c, L" \t\n\r=", &wstrQName, &cNext);
		CHECK_QSTATUS();
		status = validateQName(wstrQName.get());
		CHECK_QSTATUS();
		
		c = cNext;
		while (isWhitespace(c)) {
			status = context.getChar(&c);
			CHECK_QSTATUS();
			if (c == L'\0')
				return QSTATUS_FAIL;
		}
		if (c != L'=')
			return QSTATUS_FAIL;
		
		do {
			status = context.getChar(&c);
			CHECK_QSTATUS();
			if (c == L'\0')
				return QSTATUS_FAIL;
		} while (isWhitespace(c));
		if (c != L'\'' && c != L'\"')
			return QSTATUS_FAIL;
		
		StringBuffer<WSTRING> buf(&status);
		CHECK_QSTATUS();
		WCHAR cEnd = c;
		while (true) {
			status = context.getChar(&c);
			CHECK_QSTATUS();
			
			if (c == L'\0') {
				return QSTATUS_FAIL;
			}
			else if (c == cEnd) {
				break;
			}
			else if (c == L'&') {
				string_ptr<WSTRING> wstrValue;
				status = context.expandReference(&wstrValue);
				CHECK_QSTATUS();
				for (const WCHAR* p = wstrValue.get(); *p; ++p) {
					// TODO
					// Expand entity
					if (isWhitespace(*p)) {
						status = buf.append(L' ');
						CHECK_QSTATUS();
					}
					else {
						status = buf.append(*p);
						CHECK_QSTATUS();
					}
				}
			}
			else if (c == '<') {
				return QSTATUS_FAIL;
			}
			else if (isWhitespace(c)) {
				status = buf.append(L' ');
				CHECK_QSTATUS();
			}
			else {
				status = buf.append(c);
				CHECK_QSTATUS();
			}
		}
		
		status = context.getChar(&c);
		CHECK_QSTATUS();
		if (!isWhitespace(c) && c != L'>' && c != L'/')
			return QSTATUS_FAIL;
		while (isWhitespace(c)) {
			status = context.getChar(&c);
			CHECK_QSTATUS();
			if (c == L'\0')
				return QSTATUS_FAIL;
		}
		
		string_ptr<WSTRING> wstrValue(buf.getString());
		
		bool bNamespaceDecl = wcscmp(wstrQName.get(), L"xmlns") == 0 ||
			wcsncmp(wstrQName.get(), L"xmlns:", 6) == 0;
		if (bNamespaceDecl) {
			status = context.addNamespace(wstrQName.get(), wstrValue.get());
			CHECK_QSTATUS();
		}
		if (!bNamespaceDecl || (nFlags_ & FLAG_NAMESPACEPREFIXES)) {
			Attribute attr = {
				wstrQName.get(),
				0,
				0,
				wstrValue.get(),
				bNamespaceDecl
			};
			status = STLWrapper<AttributeList>(*pListAttribute).push_back(attr);
			CHECK_QSTATUS();
			wstrQName.release();
			wstrValue.release();
		}
	} while (c != L'>' && c != L'/');
	
	AttributeList::iterator it = pListAttribute->begin();
	while (it != pListAttribute->end()) {
		if ((nFlags_ & FLAG_NAMESPACES)) {
			if ((*it).bNamespaceDecl_) {
				if (*((*it).wstrQName_ + 5) == L':')
					(*it).pwszLocalName_ = (*it).wstrQName_ + 6;
				else
					(*it).pwszLocalName_ = (*it).wstrQName_;
			}
			else {
				status = context.expandQName((*it).wstrQName_,
					false, &(*it).pwszNamespaceURI_, &(*it).pwszLocalName_);
				CHECK_QSTATUS();
			}
		}
		
		AttributeList::iterator itP = pListAttribute->begin();
		while (itP != it) {
			if (isEqualAttribute(*it, *itP, nFlags_ & FLAG_NAMESPACES))
				return QSTATUS_FAIL;
			++itP;
		}
		
		++it;
	}
	
	deleter.release();
	*pNext = c;
	
	return QSTATUS_SUCCESS;
}

QSTATUS qs::XMLParser::parseCharacter(
	XMLParserContext& context, WCHAR c, WCHAR* pNext)
{
	assert(pNext);
	
	DECLARE_QSTATUS();
	
	StringBuffer<WSTRING> buf(&status);
	CHECK_QSTATUS();
	
	while (c != L'<') {
		if (c == L'&') {
			string_ptr<WSTRING> wstrValue;
			status = context.expandReference(&wstrValue);
			CHECK_QSTATUS();
			status = buf.append(wstrValue.get());
			CHECK_QSTATUS();
		}
		else {
			if (c == '>') {
				size_t nLen = buf.getLength();
				if (nLen >= 2 && buf.get(nLen - 1) == ']' && buf.get(nLen - 2) == ']')
					return QSTATUS_FAIL;
			}
			status = buf.append(c);
			CHECK_QSTATUS();
		}
		
		status = context.getChar(&c);
		CHECK_QSTATUS();
		if (c == L'\0')
			return QSTATUS_FAIL;
	}
	
	if (pContentHandler_) {
		status = pContentHandler_->characters(
			buf.getCharArray(), 0, buf.getLength());
		CHECK_QSTATUS();
	}
	
	*pNext = c;
	
	return QSTATUS_SUCCESS;
}

QSTATUS qs::XMLParser::parseComment(XMLParserContext& context)
{
	DECLARE_QSTATUS();
	
	WCHAR c = L'\0';
	status = context.getChar(&c);
	CHECK_QSTATUS();
	if (c != L'-')
		return QSTATUS_FAIL;
	
	WCHAR cPrev = L'\0';
	while (true) {
		status = context.getChar(&c);
		CHECK_QSTATUS();
		if (c == L'\0')
			return QSTATUS_FAIL;
		else if (c == '-' && cPrev == '-')
			break;
		else
			cPrev = c;
	}
	
	status = context.getChar(&c);
	CHECK_QSTATUS();
	if (c == L'-') {
		status = context.getChar(&c);
		CHECK_QSTATUS();
	}
	if (c != L'>')
		return QSTATUS_FAIL;
	
	return QSTATUS_SUCCESS;
}

QSTATUS qs::XMLParser::parseCDATASection(XMLParserContext& context)
{
	DECLARE_QSTATUS();
	
	WCHAR c = L'\0';
	
	string_ptr<WSTRING> wstrName;
	status = context.getString(L'\0', L"[", &wstrName, &c);
	CHECK_QSTATUS();
	if (wcscmp(wstrName.get(), L"CDATA") != 0 || c != L'[')
		return QSTATUS_FAIL;
	
	StringBuffer<WSTRING> buf(&status);
	CHECK_QSTATUS();
	
	enum State {
		STATE_NONE,
		STATE_ONE,
		STATE_TWO
	} state = STATE_NONE;
	
	while (true) {
		status = context.getChar(&c);
		CHECK_QSTATUS();
		if (c == L'\0')
			return QSTATUS_FAIL;
		
		if (c == ']') {
			switch (state) {
			case STATE_NONE:
				state = STATE_ONE;
				break;
			case STATE_ONE:
				state = STATE_TWO;
				break;
			case STATE_TWO:
				state = STATE_NONE;
				break;
			default:
				assert(false);
				return QSTATUS_FAIL;
			}
		}
		else if (c == L'>') {
			if (state == STATE_TWO)
				break;
			else
				state = STATE_NONE;
		}
		else {
			state = STATE_NONE;
		}
		
		status = buf.append(c);
		CHECK_QSTATUS();
	}
	
	size_t nLen = buf.getLength();
	assert(nLen >= 2 && buf.get(nLen - 1) == ']' && buf.get(nLen - 2) == ']');
	
	if (pContentHandler_) {
		status = pContentHandler_->characters(buf.getCharArray(), 0, nLen - 2);
		CHECK_QSTATUS();
	}
	
	return QSTATUS_SUCCESS;
}

QSTATUS qs::XMLParser::parsePI(XMLParserContext& context)
{
	DECLARE_QSTATUS();
	
	WCHAR c = L'\0';
	
	string_ptr<WSTRING> wstrTarget;
	status = context.getString(L'\0', L" \t\r\n?", &wstrTarget, &c);
	CHECK_QSTATUS();
	status = validateNCName(wstrTarget.get());
	CHECK_QSTATUS();
	if (_wcsicmp(wstrTarget.get(), L"xml") == 0)
		return QSTATUS_FAIL;
	
	bool bEnd = false;
	if (c == L'?') {
		status = context.getChar(&c);
		CHECK_QSTATUS();
		if (c != L'>')
			return QSTATUS_FAIL;
		bEnd = true;
	}
	
	StringBuffer<WSTRING> buf(&status);
	CHECK_QSTATUS();
	
	if (!bEnd) {
		while (isWhitespace(c)) {
			status = context.getChar(&c);
			CHECK_QSTATUS();
			if (c == L'\0')
				return QSTATUS_FAIL;
		}
		
		while (true) {
			WCHAR cNext = L'\0';
			status = context.getChar(&cNext);
			CHECK_QSTATUS();
			if (cNext == L'\0')
				return QSTATUS_FAIL;
			if (c == L'?' && cNext == '>')
				break;
			
			status = buf.append(c);
			CHECK_QSTATUS();
			
			c = cNext;
		}
	}
	
	if (pContentHandler_) {
		status = pContentHandler_->processingInstruction(
			wstrTarget.get(), buf.getCharArray());
		CHECK_QSTATUS();
	}
	
	return QSTATUS_SUCCESS;
}

QSTATUS qs::XMLParser::getChar(InputStream* pInputStream, unsigned char* pChar)
{
	assert(pInputStream);
	assert(pChar);
	
	DECLARE_QSTATUS();
	
	size_t nRead = 0;
	status = pInputStream->read(pChar, 1, &nRead);
	CHECK_QSTATUS();
	if (nRead == 0)
		*pChar = 0;
	
	return QSTATUS_SUCCESS;
}

QSTATUS qs::XMLParser::skipWhitespace(InputStream* pInputStream, char* pNext)
{
	assert(pInputStream);
	assert(pNext);
	
	DECLARE_QSTATUS();
	
	unsigned char c = 0;
	do {
		status = getChar(pInputStream, &c);
		CHECK_QSTATUS();
		if (c == 0)
			return QSTATUS_FAIL;
	} while (isWhitespace(c));
	
	*pNext = c;
	
	return QSTATUS_SUCCESS;
}

bool qs::XMLParser::isEqualAttribute(const Attribute& lhs,
	const Attribute& rhs, bool bNamespace)
{
	if (bNamespace) {
		if (lhs.bNamespaceDecl_ != rhs.bNamespaceDecl_) {
			return false;
		}
		else if (lhs.bNamespaceDecl_) {
			return wcscmp(lhs.wstrQName_, rhs.wstrQName_) == 0;
		}
		else {
			if (!lhs.pwszNamespaceURI_) {
				if (rhs.pwszNamespaceURI_)
					return false;
			}
			else {
				if (!rhs.pwszNamespaceURI_)
					return false;
				else if (wcscmp(lhs.pwszNamespaceURI_, rhs.pwszNamespaceURI_) != 0)
					return false;
			}
			return wcscmp(lhs.pwszLocalName_, rhs.pwszLocalName_) == 0;
		}
	}
	else {
		return wcscmp(lhs.wstrQName_, rhs.wstrQName_) == 0;
	}
}


/****************************************************************************
 *
 * XMLParser::AttributeListDeleter
 *
 */

qs::XMLParser::AttributeListDeleter::AttributeListDeleter(AttributeList* p) :
	p_(p)
{
}

qs::XMLParser::AttributeListDeleter::~AttributeListDeleter()
{
	if (p_) {
		XMLParser::AttributeList::iterator it = p_->begin();
		while (it != p_->end()) {
			freeWString((*it).wstrQName_);
			freeWString((*it).wstrValue_);
			++it;
		}
		p_->clear();
	}
}

void qs::XMLParser::AttributeListDeleter::release()
{
	p_ = 0;
}


/****************************************************************************
 *
 * XMLParserContext
 *
 */

qs::XMLParserContext::XMLParserContext(XMLParser* pParser,
	Reader* pReader, unsigned int nWait) :
	pParser_(pParser),
	pReader_(pReader),
	pParentContext_(0),
	pwszQName_(0),
	nWait_(nWait)
{
}

qs::XMLParserContext::XMLParserContext(const XMLParserContext* pParentContext,
	const WCHAR* pwszQName, unsigned int nWait) :
	pParser_(pParentContext->pParser_),
	pReader_(pParentContext->pReader_),
	pParentContext_(pParentContext),
	pwszQName_(pwszQName),
	nWait_(nWait)
{
}

qs::XMLParserContext::~XMLParserContext()
{
	NamespaceMap::iterator it = mapNamespace_.begin();
	while (it != mapNamespace_.end()) {
		freeWString((*it).first);
		assert((*it).second);
		freeWString((*it).second);
		++it;
	}
}

QSTATUS qs::XMLParserContext::expandQName(const WCHAR* pwszQName, bool bUseDefault,
	const WCHAR** ppwszNamespaceURI, const WCHAR** ppwszLocalName) const
{
	assert(pwszQName);
	assert(ppwszNamespaceURI);
	assert(ppwszLocalName);
	
	*ppwszNamespaceURI = 0;
	*ppwszLocalName = 0;
	
	const WCHAR* pwszNamespaceURI = 0;
	const WCHAR* pwszLocalName = wcschr(pwszQName, L':');
	if (pwszLocalName) {
		size_t nLen = pwszLocalName - pwszQName;
		if (nLen == 5 && wcsncmp(pwszQName, L"xmlns", nLen) == 0)
			return QSTATUS_FAIL;
		pwszNamespaceURI = getNamespaceURI(pwszQName, nLen);
		if (!pwszNamespaceURI)
			return QSTATUS_FAIL;
		++pwszLocalName;
	}
	else {
		if (bUseDefault)
			pwszNamespaceURI = getNamespaceURI(0, 0);
		pwszLocalName = pwszQName;
	}
	
	*ppwszNamespaceURI = pwszNamespaceURI;
	*ppwszLocalName = pwszLocalName;
	
	return QSTATUS_SUCCESS;
}

const WCHAR* qs::XMLParserContext::getNamespaceURI(
	const WCHAR* pwszPrefix, size_t nLen) const
{
	if (pwszPrefix && nLen == 3 && wcsncmp(pwszPrefix, L"xml", nLen) == 0)
		return L"http://www.w3.org/XML/1998/namespace";
	
	NamespaceMap::const_iterator it = mapNamespace_.begin();
	while (it != mapNamespace_.end()) {
		if (!((*it).first)) {
			if (!pwszPrefix)
				break;
		}
		else {
			if (wcslen((*it).first) == nLen &&
				wcsncmp((*it).first, pwszPrefix, nLen) == 0)
				break;
		}
		++it;
	}
	if (it != mapNamespace_.end()) {
		if (*(*it).second)
			return (*it).second;
		else
			return 0;
	}
	else if (pParentContext_) {
		return pParentContext_->getNamespaceURI(pwszPrefix, nLen);
	}
	else {
		return 0;
	}
}

QSTATUS qs::XMLParserContext::addNamespace(const WCHAR* pwszQName, const WCHAR* pwszURI)
{
	assert(pwszQName);
	assert(wcscmp(pwszQName, L"xmlns") == 0 || wcsncmp(pwszQName, L"xmlns:", 6) == 0);
	
	DECLARE_QSTATUS();
	
	string_ptr<WSTRING> wstrURI(allocWString(pwszURI));
	if (!wstrURI.get())
		return QSTATUS_OUTOFMEMORY;
	
	string_ptr<WSTRING> wstrPrefix;
	const WCHAR* pwszPrefix = wcschr(pwszQName, L':');
	if (pwszPrefix) {
		wstrPrefix.reset(allocWString(pwszPrefix + 1));
		if (!wstrPrefix.get())
			return QSTATUS_OUTOFMEMORY;
	}
	
	if (wstrPrefix.get() && !*wstrURI.get())
		return QSTATUS_FAIL;
	
	NamespaceMap::iterator it = std::find_if(
		mapNamespace_.begin(), mapNamespace_.end(),
		std::bind2nd(
			binary_compose_f_gx_hy(
				string_equal<WCHAR>(),
				std::select1st<NamespaceMap::value_type>(),
				std::identity<const WCHAR*>()),
			wstrPrefix.get()));
	if (it != mapNamespace_.end())
		return QSTATUS_FAIL;
	
	status = STLWrapper<NamespaceMap>(mapNamespace_).push_back(
		std::make_pair(wstrPrefix.get(), wstrURI.get()));
	CHECK_QSTATUS();
	wstrPrefix.release();
	wstrURI.release();
	
	return QSTATUS_SUCCESS;
}

QSTATUS qs::XMLParserContext::fireStartPrefixMappings(
	ContentHandler* pContentHandler) const
{
	DECLARE_QSTATUS();
	
	if (pContentHandler) {
		NamespaceMap::const_iterator it = mapNamespace_.begin();
		while (it != mapNamespace_.end()) {
			status = pContentHandler->startPrefixMapping(
				(*it).first ? (*it).first : L"", (*it).second);
			CHECK_QSTATUS();
			++it;
		}
	}
	
	return QSTATUS_SUCCESS;
}

QSTATUS qs::XMLParserContext::fireEndPrefixMappings(
	ContentHandler* pContentHandler) const
{
	DECLARE_QSTATUS();
	
	if (pContentHandler) {
		NamespaceMap::const_iterator it = mapNamespace_.begin();
		while (it != mapNamespace_.end()) {
			status = pContentHandler->endPrefixMapping(
				(*it).first ? (*it).first : L"");
			CHECK_QSTATUS();
			++it;
		}
	}
	
	return QSTATUS_SUCCESS;
}

QSTATUS qs::XMLParserContext::getChar(WCHAR* pChar)
{
	assert(pChar);
	
	DECLARE_QSTATUS();
	
	size_t nRead = 0;
	status = pReader_->read(pChar, 1, &nRead);
	CHECK_QSTATUS();
	if (nRead != 1)
		*pChar = L'\0';
	
	return QSTATUS_SUCCESS;
}

QSTATUS qs::XMLParserContext::getString(WCHAR cFirst,
	WCHAR* pwszSeparator, WSTRING* pwstr, WCHAR* pNext)
{
	assert(pwszSeparator);
	assert(pwstr);
	
	DECLARE_QSTATUS();
	
	*pwstr = 0;
	
	StringBuffer<WSTRING> buf(10, &status);
	CHECK_QSTATUS();
	
	if (cFirst != L'\0') {
		status = buf.append(cFirst);
		CHECK_QSTATUS();
	}
	
	WCHAR c = L'\0';
	status = getChar(&c);
	CHECK_QSTATUS();
	while (c != L'\0' && !wcschr(pwszSeparator, c)) {
		status = buf.append(c);
		CHECK_QSTATUS();
		status = getChar(&c);
		CHECK_QSTATUS();
	}
	if (c == L'\0')
		return QSTATUS_FAIL;
	
	*pwstr = buf.getString();
	*pNext = c;
	
	return QSTATUS_SUCCESS;
}

QSTATUS qs::XMLParserContext::expandReference(WSTRING* pwstrValue)
{
	assert(pwstrValue);
	
	DECLARE_QSTATUS();
	
	string_ptr<WSTRING> wstrName;
	WCHAR cNext = L'\0';
	status = getString(L'\0', L";", &wstrName, &cNext);
	CHECK_QSTATUS();
	assert(cNext == L';');
	
	if (*wstrName.get() == L'#') {
		const WCHAR* p = wstrName.get() + 1;
		int nBase = 10;
		if (*p == L'x') {
			++p;
			nBase = 16;
		}
		WCHAR* pEnd = 0;
		long nValue = wcstol(p, &pEnd, nBase);
		if (nValue == 0 || *pEnd || nValue > 0xffff)
			return QSTATUS_FAIL;
		
		WCHAR c = static_cast<WCHAR>(nValue);
		string_ptr<WSTRING> wstrValue(allocWString(&c, 1));
		if (!wstrValue.get())
			return QSTATUS_FAIL;
		
		*pwstrValue = wstrValue.release();
	}
	else {
		status = pParser_->validateNCName(wstrName.get());
		CHECK_QSTATUS();
		
		struct {
			const WCHAR* pwszName_;
			WCHAR c_;
		} predefined[] = {
			{ L"lt",	L'<'	},
			{ L"gt",	L'>'	},
			{ L"amp",	L'&'	},
			{ L"quot",	L'\"'	},
			{ L"apos",	L'\''	}
		};
		WCHAR c = L'\0';
		for (int n = 0; n < countof(predefined) && c == L'\0'; ++n) {
			if (wcscmp(wstrName.get(), predefined[n].pwszName_) == 0)
				c = predefined[n].c_;
		}
		if (c == L'\0') {
			// TODO
			// Handle entity
			return QSTATUS_FAIL;
		}
		
		string_ptr<WSTRING> wstrValue(allocWString(&c, 1));
		if (!wstrValue.get())
			return QSTATUS_FAIL;
		
		*pwstrValue = wstrValue.release();
	}
	
	return QSTATUS_SUCCESS;
}

const XMLParserContext* qs::XMLParserContext::getParentContext() const
{
	return pParentContext_;
}

const WCHAR* qs::XMLParserContext::getQName() const
{
	return pwszQName_;
}

bool qs::XMLParserContext::isWait(Wait wait) const
{
	return (nWait_ & wait) != 0;
}

void qs::XMLParserContext::setWait(unsigned int nWait, unsigned int nMask)
{
	nWait_ &= ~nMask;
	nWait_ |= nWait;
}


/****************************************************************************
 *
 * ResettableInputStream
 *
 */

qs::ResettableInputStream::ResettableInputStream(size_t nResettableSize,
	InputStream* pInputStream, QSTATUS* pstatus) :
	pInputStream_(pInputStream),
	pBuf_(0),
	nRead_(0),
	p_(0)
{
	assert(pInputStream);
	assert(pstatus);
	
	DECLARE_QSTATUS();
	
	pBuf_ = static_cast<unsigned char*>(malloc(nResettableSize));
	if (!pBuf_) {
		*pstatus = QSTATUS_OUTOFMEMORY;
		return;
	}
	status = pInputStream_->read(pBuf_, nResettableSize, &nRead_);
	CHECK_QSTATUS_SET(pstatus);
	p_ = pBuf_;
}

qs::ResettableInputStream::~ResettableInputStream()
{
	free(pBuf_);
}

QSTATUS qs::ResettableInputStream::reset()
{
	if (!p_)
		return QSTATUS_FAIL;
	p_ = pBuf_;
	return QSTATUS_SUCCESS;
}

QSTATUS qs::ResettableInputStream::close()
{
	return pInputStream_->close();
}

QSTATUS qs::ResettableInputStream::read(unsigned char* p, size_t nRead, size_t* pnRead)
{
	DECLARE_QSTATUS();
	
	if (p_) {
		size_t nInBuf = nRead_ - (p_ - pBuf_);
		if (nRead > nInBuf) {
			memcpy(p, p_, nInBuf);
			p_ = 0;
			status = pInputStream_->read(p + nInBuf, nRead - nInBuf, pnRead);
			CHECK_QSTATUS();
			*pnRead = *pnRead == -1 ? nInBuf : *pnRead + nInBuf;
		}
		else {
			memcpy(p, p_, nRead);
			p_ += nRead;
			*pnRead = nRead;
		}
	}
	else {
		status = pInputStream_->read(p, nRead, pnRead);
		CHECK_QSTATUS();
	}
	
	return QSTATUS_SUCCESS;
}
