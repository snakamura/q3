/*
 * $Id$
 *
 * Copyright(C) 1998-2006 Satoshi Nakamura
 * All rights reserved.
 *
 */

#pragma warning(disable:4786)

#include <qsconv.h>
#include <qsinit.h>
#include <qslog.h>
#include <qsstream.h>

#include <algorithm>

#include "sax.h"
#include "xmlparser.h"

using namespace qs;


/****************************************************************************
 *
 * XMLParser
 *
 */

qs::XMLParser::XMLParser(ContentHandler* pContentHandler,
						 unsigned int nFlags) :
	pContentHandler_(pContentHandler),
	nFlags_(nFlags)
{
	assert(pContentHandler);
}

qs::XMLParser::~XMLParser()
{
}

bool qs::XMLParser::parse(const InputSource& source)
{
	InputStream* pInputStream = source.getByteStream();
	Reader* pReader = 0;
	if (!pInputStream)
		pReader = source.getCharacterStream();
	std::auto_ptr<BufferedInputStream> pBufferedInputStream;
	if (!pInputStream && !pReader) {
		const WCHAR* pwszSystemId = source.getSystemId();
		if (!pwszSystemId)
			return false;
		std::auto_ptr<FileInputStream> pStream(new FileInputStream(pwszSystemId));
		if (!*pStream.get())
			return false;
		pBufferedInputStream.reset(new BufferedInputStream(pStream.get(), true));
		pStream.release();
		pInputStream = pBufferedInputStream.get();
	}
	assert(pInputStream || pReader);
	
	std::auto_ptr<Reader> apReader;
	if (pInputStream) {
		if (!parseXmlDecl(pInputStream, &apReader))
			return false;
		pReader = apReader.get();
	}
	else {
		if (!parseXmlDecl(pReader))
			return false;
	}
	assert(pReader);
	
	if (pContentHandler_) {
		if (!pContentHandler_->startDocument())
			return false;
	}
	
	XMLParserContext context(this, pReader,
		XMLParserContext::WAIT_STARTELEMENT |
		XMLParserContext::WAIT_PROCESSINGINSTRUCTION |
		XMLParserContext::WAIT_COMMENT |
		XMLParserContext::WAIT_WS |
		XMLParserContext::WAIT_DOCTYPE);
	if (!parse(context))
		return false;
	
	if (pContentHandler_) {
		if (!pContentHandler_->endDocument())
			return false;
	}
	
	return true;
}

bool qs::XMLParser::validateQName(const WCHAR* pwsz) const
{
	if (nFlags_ & FLAG_NAMESPACES)
		return isQName(pwsz);
	else
		return isName(pwsz);
}

bool qs::XMLParser::validateNCName(const WCHAR* pwsz) const
{
	if (nFlags_ & FLAG_NAMESPACES)
		return isNCName(pwsz);
	else
		return isName(pwsz);
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

bool qs::XMLParser::parseXmlDecl(InputStream* pInputStream,
								 std::auto_ptr<Reader>* ppReader)
{
	assert(pInputStream);
	assert(ppReader);
	
	std::auto_ptr<ResettableInputStream> pStream(
		new ResettableInputStream(10, pInputStream));
	if (!*pStream.get())
		return false;
	
	wstring_ptr wstrEncoding;
	
	unsigned char buf[16];
	size_t nRead = pStream->read(buf, 6);
	if (nRead != 6)
		return false;
	const unsigned char utf8bom[] = { 0xef, 0xbb, 0xbf };
	if (strncmp(reinterpret_cast<char*>(buf), reinterpret_cast<const char*>(utf8bom), countof(utf8bom)) == 0) {
		strncpy(reinterpret_cast<char*>(buf), reinterpret_cast<char*>(buf + countof(utf8bom)), 6 - countof(utf8bom));
		nRead = pStream->read(buf + (6 - countof(utf8bom)), countof(utf8bom));
		if (nRead != countof(utf8bom))
			return false;
	}
	if (strncmp(reinterpret_cast<char*>(buf), "<?xml", 5) == 0 && isWhitespace(buf[5])) {
		char c = 0;
		if (!skipWhitespace(pStream.get(), &c))
			return false;
		buf[0] = c;
		nRead = pStream->read(buf + 1, 6);
		if (nRead != 6 || strncmp(reinterpret_cast<char*>(buf), "version", 7) != 0)
			return false;
		if (!skipWhitespace(pStream.get(), &c))
			return false;
		if (c != '=')
			return false;
		if (!skipWhitespace(pStream.get(), &c))
			return false;
		if (c != '\"' && c != '\'')
			return false;
		nRead = pStream->read(buf, 4);
		if (nRead != 4 || strncmp(reinterpret_cast<char*>(buf), "1.0", 3) != 0 || buf[3] != c)
			return false;
		if (!skipWhitespace(pStream.get(), &c))
			return false;
		if (c != '?') {
			if (c == 'e') {
				buf[0] = c;
				nRead = pStream->read(buf + 1, 7);
				if (nRead != 7 || strncmp(reinterpret_cast<char*>(buf), "encoding", 8) != 0)
					return false;
				if (!skipWhitespace(pStream.get(), &c))
					return false;
				if (c != '=')
					return false;
				if (!skipWhitespace(pStream.get(), &c))
					return false;
				if (c != '\"' && c != '\'')
					return false;
				char cEnd = c;
				StringBuffer<STRING> encoding;
				while (true) {
					if (!getChar(pStream.get(), reinterpret_cast<unsigned char*>(&c)))
						return false;
					if (c == '\0')
						return false;
					else if (c == cEnd)
						break;
					encoding.append(c);
				}
				wstrEncoding = mbs2wcs(encoding.getCharArray());
				if (!skipWhitespace(pStream.get(), &c))
					return false;
			}
			if (c == 's') {
				buf[0] = c;
				nRead = pStream->read(buf + 1, 9);
				if (nRead != 9 || strncmp(reinterpret_cast<char*>(buf), "standalone", 10) != 0)
					return false;
				if (!skipWhitespace(pStream.get(), &c))
					return false;
				if (c != '=')
					return false;
				if (!skipWhitespace(pStream.get(), &c))
					return false;
				if (c != '\"' && c != '\'')
					return false;
				nRead = pStream->read(buf, 4);
				if (nRead != 4 ||
					(strncmp(reinterpret_cast<char*>(buf), "yes", 3) != 0 || buf[3] != c) &&
					(strncmp(reinterpret_cast<char*>(buf), "no", 2) != 0 || buf[2] != c))
					return false;
				if (buf[0] == 'y' || isWhitespace(buf[3])) {
					if (!skipWhitespace(pStream.get(), &c))
						return false;
				}
				else {
					c = buf[3];
				}
			}
			if (c != '?')
				return false;
		}
		assert(c == '?');
		if (!getChar(pStream.get(), reinterpret_cast<unsigned char*>(&c)))
			return false;
		if (c != L'>')
			return false;
	}
	else {
		if (!pStream->reset())
			return false;
	}
	
	const WCHAR* pwszEncoding = wstrEncoding.get();
	if (!pwszEncoding)
		pwszEncoding = L"utf-8";
	
	std::auto_ptr<InputStreamReader> pReader(
		new InputStreamReader(pStream.get(), true, pwszEncoding));
	if (!*pReader.get())
		return false;
	
	*ppReader = pReader;
	pStream.release();
	
	return true;
}

bool qs::XMLParser::parseXmlDecl(Reader* pReader)
{
	// TODO
	
	return false;
}

bool qs::XMLParser::parse(XMLParserContext& context)
{
	WCHAR c = L'\0';
	if (!context.getChar(&c))
		return false;
	while (c != L'\0') {
		WCHAR cNext = L'\0';
		if (c == L'<') {
			if (!context.getChar(&c) || c == L'\0')
				return false;
			if (c == '?') {
				if (!parsePI(context))
					return false;
			}
			else if (c == '!') {
				if (!context.getChar(&c))
					return false;
				if (c == '-') {
					if (!parseComment(context))
						return false;
				}
				else if (c == '[') {
					if (!parseCDATASection(context))
						return false;
				}
				else {
					wstring_ptr wstrName;
					WCHAR cNext = L'\0';
					if (!context.getString(c, L" \t\n\r", &wstrName, &cNext))
						return false;
					if (context.isWait(XMLParserContext::WAIT_DOCTYPE) &&
						wcscmp(wstrName.get(), L"DOCTYPE") == 0) {
						if (!parseDoctype(context))
							return false;
					}
					else {
						return false;
					}
				}
			}
			else if (c == '/') {
				if (context.isWait(XMLParserContext::WAIT_ENDELEMENT))
					return parseEndElement(context);
				else
					return false;
			}
			else {
				if (context.isWait(XMLParserContext::WAIT_STARTELEMENT)) {
					if (!parseStartElement(context, c))
						return false;
					if (!context.getParentContext())
						context.setWait(XMLParserContext::WAIT_WS,
							XMLParserContext::WAIT_WS |
							XMLParserContext::WAIT_CHARACTER |
							XMLParserContext::WAIT_STARTELEMENT);
				}
				else {
					return false;
				}
			}
		}
		else {
			if (context.isWait(XMLParserContext::WAIT_CHARACTER)) {
				if (!parseCharacter(context, c, &cNext))
					return false;
			}
			else if (context.isWait(XMLParserContext::WAIT_WS)) {
				if (!isWhitespace(c))
					return false;
			}
			else {
				return false;
			}
		}
		
		if (cNext != L'\0') {
			c = cNext;
		}
		else {
			if (!context.getChar(&c))
				return false;
		}
	}
	
	return !context.isWait(XMLParserContext::WAIT_STARTELEMENT);
}

bool qs::XMLParser::parseStartElement(XMLParserContext& context,
									  WCHAR c)
{
	assert(c != L'\0');
	
	Log log(InitThread::getInitThread().getLogger(), L"qs::XMLParser");
	log.debug(L"Begin parseStartElement");
	
	wstring_ptr wstrQName;
	WCHAR cNext = L'\0';
	if (!context.getString(c, L" \t\n\r>/", &wstrQName, &cNext)) {
		log.error(L"Failed to get an element name.");
		return false;
	}
	if (!validateQName(wstrQName.get())) {
		log.errorf(L"An invalid element name: %s", wstrQName.get());
		return false;
	}
	
	log.debugf(L"Element name: %s", wstrQName.get());
	
	if (!context.eatWhitespaces(cNext, &c))
		return false;
	
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
		if (!parseAttributes(childContext, c, &listAttribute, &cNext))
			return false;
		c = cNext;
	}
	assert(c == L'/' || c == L'>');
	
	if (c == L'/') {
		if (!context.getChar(&c))
			return false;
		if (c != L'>')
			return false;
		bEmptyTag = true;
	}
	
	if (nFlags_ & FLAG_NAMESPACES) {
		if (!childContext.fireStartPrefixMappings(pContentHandler_))
			return false;
	}
	
	const WCHAR* pwszNamespaceURI = 0;
	const WCHAR* pwszLocalName = 0;
	if (nFlags_ & FLAG_NAMESPACES) {
		if (!childContext.expandQName(wstrQName.get(),
			true, &pwszNamespaceURI, &pwszLocalName))
			return false;
	}
	
	if (pContentHandler_) {
		AttributesImpl attrs(listAttribute);
		if (!pContentHandler_->startElement(pwszNamespaceURI,
			pwszLocalName, wstrQName.get(), attrs))
			return false;
	}
	if (bEmptyTag) {
		if (pContentHandler_) {
			if (!pContentHandler_->endElement(pwszNamespaceURI,
				pwszLocalName, wstrQName.get()))
				return false;
		}
	}
	else {
		if (!parse(childContext))
			return false;
	}
	
	if (nFlags_ & FLAG_NAMESPACES) {
		if (!childContext.fireEndPrefixMappings(pContentHandler_))
			return false;
	}
	
	log.debug(L"End parseStartElement");
	
	return true;
}

bool qs::XMLParser::parseEndElement(XMLParserContext& context)
{
	Log log(InitThread::getInitThread().getLogger(), L"qs::XMLParser");
	log.debug(L"Begin parseEndElement");
	
	wstring_ptr wstrQName;
	WCHAR c = L'\0';
	if (!context.getString(L'\0', L" \t\n\r>", &wstrQName, &c)) {
		log.error(L"Failed to get an element name.");
		return false;
	}
	if (!validateQName(wstrQName.get())) {
		log.errorf(L"An invalid element name: %s", wstrQName.get());
		return false;
	}
	
	log.debugf(L"Element name: %s", wstrQName.get());
	
	const WCHAR* pwszNamespaceURI = 0;
	const WCHAR* pwszLocalName = 0;
	if (nFlags_ & FLAG_NAMESPACES) {
		if (!context.expandQName(wstrQName.get(),
			true, &pwszNamespaceURI, &pwszLocalName))
			return false;
	}
	
	if (!context.eatWhitespaces(c, &c))
		return false;
	if (c != L'>')
		return false;
	
	if (wcscmp(wstrQName.get(), context.getQName()) != 0) {
		log.errorf(L"Element names don't match: <%s> and </%s>", context.getQName(), wstrQName.get());
		return false;
	}
	
	if (pContentHandler_) {
		if (!pContentHandler_->endElement(
			pwszNamespaceURI, pwszLocalName, wstrQName.get()))
			return false;
	}
	
	log.debug(L"End parseEndElement");
	
	return true;
}

bool qs::XMLParser::parseAttributes(XMLParserContext& context,
									WCHAR c,
									AttributeList* pListAttribute,
									WCHAR* pNext)
{
	assert(pListAttribute);
	assert(pNext);
	
	Log log(InitThread::getInitThread().getLogger(), L"qs::XMLParser");
	log.debug(L"Begin parseAttributes");
	
	AttributeListDeleter deleter(pListAttribute);
	do {
		wstring_ptr wstrQName;
		WCHAR cNext = L'\0';
		if (!context.getString(c, L" \t\n\r=", &wstrQName, &cNext)) {
			log.error(L"Failed to get an attribute name.");
			return false;
		}
		if (!validateQName(wstrQName.get())) {
			log.errorf(L"An invalid attribute name: %s", wstrQName.get());
			return false;
		}
		
		log.debugf(L"Attribute name: %s", wstrQName.get());
		
		if (!context.eatWhitespaces(cNext, &c))
			return false;
		if (c != L'=')
			return false;
		
		if (!context.eatWhitespaces(L'\0', &c))
			return false;
		if (c != L'\'' && c != L'\"')
			return false;
		
		StringBuffer<WSTRING> buf;
		WCHAR cEnd = c;
		while (true) {
			if (!context.getChar(&c) || c == L'\0')
				return false;
			
			if (c == cEnd) {
				break;
			}
			else if (c == L'&') {
				wstring_ptr wstrValue;
				if (!context.expandReference(&c, &wstrValue)) {
					log.error(L"Failed to expand an entity reference.");
					return false;
				}
				if (wstrValue.get()) {
					for (const WCHAR* p = wstrValue.get(); *p; ++p) {
						if (isWhitespace(*p))
							buf.append(L' ');
						else
							buf.append(*p);
					}
				}
				else {
					buf.append(c);
				}
			}
			else if (c == '<') {
				log.error(L"< is not allowed in an attribute value.");
				return false;
			}
			else if (isWhitespace(c)) {
				buf.append(L' ');
			}
			else {
				buf.append(c);
			}
		}
		
		if (!context.getChar(&c)) {
			log.error(L"Failed to get a character.");
			return false;
		}
		if (!isWhitespace(c) && c != L'>' && c != L'/') {
			log.error(L"An invalid character after an end of an attribute value.");
			return false;
		}
		if (!context.eatWhitespaces(c, &c))
			return false;
		
		wstring_ptr wstrValue(buf.getString());
		
		log.debugf(L"Attribute value: %s", wstrValue.get());
		
		bool bNamespaceDecl = wcscmp(wstrQName.get(), L"xmlns") == 0 ||
			wcsncmp(wstrQName.get(), L"xmlns:", 6) == 0;
		if (bNamespaceDecl) {
			if (!context.addNamespace(wstrQName.get(), wstrValue.get()))
				return false;
		}
		if (!bNamespaceDecl || (nFlags_ & FLAG_NAMESPACEPREFIXES)) {
			Attribute attr = {
				wstrQName.get(),
				0,
				0,
				wstrValue.get(),
				bNamespaceDecl
			};
			pListAttribute->push_back(attr);
			wstrQName.release();
			wstrValue.release();
		}
	} while (c != L'>' && c != L'/');
	
	for (AttributeList::iterator it = pListAttribute->begin(); it != pListAttribute->end(); ++it) {
		if ((nFlags_ & FLAG_NAMESPACES)) {
			if ((*it).bNamespaceDecl_) {
				if (*((*it).wstrQName_ + 5) == L':')
					(*it).pwszLocalName_ = (*it).wstrQName_ + 6;
				else
					(*it).pwszLocalName_ = (*it).wstrQName_;
			}
			else {
				if (!context.expandQName((*it).wstrQName_,
					false, &(*it).pwszNamespaceURI_, &(*it).pwszLocalName_))
					return false;
			}
		}
		
		for (AttributeList::iterator itP = pListAttribute->begin(); itP != it; ++itP) {
			if (isEqualAttribute(*it, *itP, nFlags_ & FLAG_NAMESPACES))
				return false;
		}
	}
	
	deleter.release();
	*pNext = c;
	
	log.debug(L"End parseAttributes");
	
	return true;
}

bool qs::XMLParser::parseCharacter(XMLParserContext& context,
								   WCHAR c,
								   WCHAR* pNext)
{
	assert(pNext);
	
	Log log(InitThread::getInitThread().getLogger(), L"qs::XMLParser");
	log.debug(L"Begin parseCharacter");
	
	StringBuffer<WSTRING> buf;
	
	while (c != L'<') {
		if (c == L'&') {
			wstring_ptr wstrValue;
			if (!context.expandReference(&c, &wstrValue))
				return false;
			if (wstrValue.get())
				buf.append(wstrValue.get());
			else
				buf.append(c);
		}
		else {
			if (c == '>') {
				size_t nLen = buf.getLength();
				if (nLen >= 2 && buf.get(nLen - 1) == ']' && buf.get(nLen - 2) == ']')
					return false;
			}
			buf.append(c);
		}
		
		if (!context.getChar(&c) || c == L'\0')
			return false;
	}
	
	log.debugf(L"Characters: %s", buf.getCharArray());
	
	if (pContentHandler_) {
		if (!pContentHandler_->characters(
			buf.getCharArray(), 0, buf.getLength()))
			return false;
	}
	
	*pNext = c;
	
	log.debug(L"End parseCharacter");
	
	return true;
}

bool qs::XMLParser::parseComment(XMLParserContext& context)
{
	Log log(InitThread::getInitThread().getLogger(), L"qs::XMLParser");
	log.debug(L"Begin parseComment");
	
	WCHAR c = L'\0';
	if (!context.getChar(&c))
		return false;
	if (c != L'-')
		return false;
	
	WCHAR cPrev = L'\0';
	while (true) {
		if (!context.getChar(&c) || c == L'\0')
			return false;
		if (c == '-' && cPrev == '-')
			break;
		else
			cPrev = c;
	}
	
	if (!context.getChar(&c))
		return false;
	if (c == L'-') {
		if (!context.getChar(&c))
			return false;
	}
	if (c != L'>')
		return false;
	
	log.debug(L"End parseComment");
	
	return true;
}

bool qs::XMLParser::parseCDATASection(XMLParserContext& context)
{
	Log log(InitThread::getInitThread().getLogger(), L"qs::XMLParser");
	log.debug(L"Begin parseCDATASection");
	
	WCHAR c = L'\0';
	
	wstring_ptr wstrName;
	if (!context.getString(L'\0', L"[", &wstrName, &c))
		return false;
	if (wcscmp(wstrName.get(), L"CDATA") != 0 || c != L'[')
		return false;
	
	StringBuffer<WSTRING> buf;
	
	enum State {
		STATE_NONE,
		STATE_ONE,
		STATE_TWO
	} state = STATE_NONE;
	
	while (true) {
		if (!context.getChar(&c) || c == L'\0')
			return false;
		
		if (c == ']') {
			switch (state) {
			case STATE_NONE:
				state = STATE_ONE;
				break;
			case STATE_ONE:
				state = STATE_TWO;
				break;
			case STATE_TWO:
				break;
			default:
				assert(false);
				return false;
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
		
		buf.append(c);
	}
	
	size_t nLen = buf.getLength();
	assert(nLen >= 2 && buf.get(nLen - 1) == ']' && buf.get(nLen - 2) == ']');
	
	log.debugf(L"CDATA section: %s", buf.getCharArray());
	
	if (pContentHandler_) {
		if (!pContentHandler_->characters(buf.getCharArray(), 0, nLen - 2))
			return false;
	}
	
	log.debug(L"End parseCDATASection");
	
	return true;
}

bool qs::XMLParser::parsePI(XMLParserContext& context)
{
	Log log(InitThread::getInitThread().getLogger(), L"qs::XMLParser");
	log.debug(L"Begin parsePI");
	
	WCHAR c = L'\0';
	
	wstring_ptr wstrTarget;
	if (!context.getString(L'\0', L" \t\r\n?", &wstrTarget, &c)) {
		log.error(L"Failed to get a processing instruction target.");
		return false;
	}
	if (!validateNCName(wstrTarget.get()) ||
		_wcsicmp(wstrTarget.get(), L"xml") == 0) {
		log.errorf(L"An invalid processing instruction target: %s", wstrTarget.get());
		return false;
	}
	
	log.debugf(L"Processing instruction target: %s", wstrTarget.get());
	
	bool bEnd = false;
	if (c == L'?') {
		if (!context.getChar(&c))
			return false;
		if (c != L'>')
			return false;
		bEnd = true;
	}
	
	StringBuffer<WSTRING> buf;
	
	if (!bEnd) {
		if (!context.eatWhitespaces(c, &c))
			return false;
		
		while (true) {
			WCHAR cNext = L'\0';
			if (!context.getChar(&cNext) || cNext == L'\0')
				return false;
			if (c == L'?' && cNext == '>')
				break;
			
			buf.append(c);
			
			c = cNext;
		}
	}
	
	log.debugf(L"Processing instruction data: %s", buf.getCharArray());
	
	if (pContentHandler_) {
		if (!pContentHandler_->processingInstruction(
			wstrTarget.get(), buf.getCharArray()))
			return false;
	}
	
	log.debug(L"End parsePI");
	
	return true;
}

bool qs::XMLParser::parseDoctype(XMLParserContext& context)
{
	Log log(InitThread::getInitThread().getLogger(), L"qs::XMLParser");
	log.debug(L"Begin parseDoctype");
	
	WCHAR c = L'\0';
	if (!context.eatWhitespaces(L'\0', &c))
		return false;
	
	wstring_ptr wstrQName;
	WCHAR cNext = L'\0';
	if (!context.getString(c, L" \t\n\r>", &wstrQName, &cNext)) {
		log.error(L"Failed to get a doctype name.");
		return false;
	}
	if (!validateQName(wstrQName.get())) {
		log.errorf(L"An invalid doctype name: %s", wstrQName.get());
		return false;
	}
	
	if (!context.eatWhitespaces(cNext, &c))
		return false;
	
	if (c != L'[') {
		if (!parseExternalId(context, c, &c))
			return false;
	}
	
	if (c == L'[') {
		if (!parseInternalSubset(context))
			return false;
		
		if (!context.eatWhitespaces(L'\0', &c))
			return false;
	}
	if (c != L'>')
		return false;
	
	log.debug(L"End parseDoctype");
	
	return true;
}

bool qs::XMLParser::parseInternalSubset(XMLParserContext& context)
{
	WCHAR c = L'\0';
	if (!context.getChar(&c) || c == L'\0')
		return false;
	while (c != L']') {
		if (c == L'<') {
			if (!context.getChar(&c))
				return false;
			if (c != L'!')
				return false;
			
			// TODO
			// Allow comments and PIs.
			
			wstring_ptr wstrName;
			if (!context.getString(L'\0', L" \t\r\n", &wstrName, &c))
				return false;
			
			if (!context.eatWhitespaces(L'\0', &c))
				return false;
			
			if (wcscmp(wstrName.get(), L"ENTITY") == 0) {
				if (!parseEntityDecl(context, c))
					return false;
			}
			else if (wcscmp(wstrName.get(), L"ELEMENT") == 0) {
				if (!parseElementDecl(context, c))
					return false;
			}
			else if (wcscmp(wstrName.get(), L"ATTLIST") == 0) {
				if (!parseAttlistDecl(context, c))
					return false;
			}
			else if (wcscmp(wstrName.get(), L"NOTATION") == 0) {
				if (!parseNotationDecl(context, c))
					return false;
			}
			else {
				return false;
			}
			
			if (!context.getChar(&c) || c == L'\0')
				return false;
		}
		else if (!isWhitespace(c)) {
			// TODO
			// Skip parameter entity reference.
			return false;
		}
	}
	return true;
}

bool qs::XMLParser::parseElementDecl(XMLParserContext& context,
									 WCHAR c)
{
	// TODO
	// Parse elementDecl
	return false;
}

bool qs::XMLParser::parseAttlistDecl(XMLParserContext& context,
									 WCHAR c)
{
	// TODO
	// Parse attlistDecl
	return false;
}

bool qs::XMLParser::parseEntityDecl(XMLParserContext& context,
									WCHAR c)
{
	bool bPE = c == L'%';
	if (bPE) {
		if (!context.eatWhitespaces(L'\0', &c))
			return false;
	}
	
	wstring_ptr wstrName;
	if (!context.getString(c, L" \t\r\n", &wstrName, &c))
		return false;
	
	if (!context.eatWhitespaces(L'\0', &c))
		return false;
	
	if (c == L'\"' || c == L'\'') {
		// TODO
		// Parse EntityValue.
		return false;
	}
	else {
		if (!parseExternalId(context, c, &c))
			return false;
	}
	
	if (!context.eatWhitespaces(c, &c))
		return false;
	
	if (c != L'>')
		return false;
	
	return true;
}

bool qs::XMLParser::parseNotationDecl(XMLParserContext& context,
									  WCHAR c)
{
	// TODO
	// Parse notationDecl
	return false;
}

bool qs::XMLParser::parseExternalId(XMLParserContext& context,
									WCHAR c,
									WCHAR* pNext)
{
	assert(pNext);
	
	if (c == L'P') {
		if (!parsePublicId(context, c))
			return false;
	}
	else if (c == L'S') {
		if (!parseSystemId(context, c))
			return false;
	}
	else {
		return false;
	}
	
	if (!context.eatWhitespaces(L'\0', pNext))
		return false;
	
	return true;
}

bool qs::XMLParser::parsePublicId(XMLParserContext& context,
								  WCHAR c)
{
	assert(c == L'P');
	
	wstring_ptr wstrName;
	WCHAR cNext = L'\0';
	if (!context.getString(c, L" \t\n\r", &wstrName, &cNext))
		return false;
	if (wcscmp(wstrName.get(), L"PUBLIC") != 0)
		return false;
	
	if (!context.eatWhitespaces(L'\0', &c))
		return false;
	
	if (!parseLiteral(context, c))
		return false;
	
	if (!context.eatWhitespaces(L'\0', &c))
		return false;
	
	if (!parseLiteral(context, c))
		return false;
	
	return true;
}

bool qs::XMLParser::parseSystemId(XMLParserContext& context,
								  WCHAR c)
{
	assert(c == L'S');
	
	wstring_ptr wstrName;
	WCHAR cNext = L'\0';
	if (!context.getString(c, L" \t\n\r", &wstrName, &cNext))
		return false;
	if (wcscmp(wstrName.get(), L"SYSTEM") != 0)
		return false;
	
	if (!context.eatWhitespaces(L'\0', &c))
		return false;
	
	if (!parseLiteral(context, c))
		return false;
	
	return true;
}

bool qs::XMLParser::parseLiteral(XMLParserContext& context,
								 WCHAR c)
{
	if (c != L'\'' && c != L'\"')
		return false;
	WCHAR cQuote = c;
	
	do {
		if (!context.getChar(&c))
			return false;
	} while (c != cQuote);
	
	return true;
}

bool qs::XMLParser::getChar(InputStream* pInputStream,
							unsigned char* pChar)
{
	assert(pInputStream);
	assert(pChar);
	
	size_t nRead = pInputStream->read(pChar, 1);
	if (nRead == -1)
		return false;
	if (nRead == 0)
		*pChar = 0;
	
	return true;
}

bool qs::XMLParser::skipWhitespace(InputStream* pInputStream,
								   char* pNext)
{
	assert(pInputStream);
	assert(pNext);
	
	unsigned char c = 0;
	do {
		if (!getChar(pInputStream, &c))
			return false;
		if (c == 0)
			return false;
	} while (isWhitespace(c));
	
	*pNext = c;
	
	return true;
}

bool qs::XMLParser::isEqualAttribute(const Attribute& lhs,
									 const Attribute& rhs,
									 bool bNamespace)
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
		for (XMLParser::AttributeList::iterator it = p_->begin(); it != p_->end(); ++it) {
			freeWString((*it).wstrQName_);
			freeWString((*it).wstrValue_);
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
									   Reader* pReader,
									   unsigned int nWait) :
	pParser_(pParser),
	pReader_(pReader),
	pParentContext_(0),
	pwszQName_(0),
	nWait_(nWait)
{
}

qs::XMLParserContext::XMLParserContext(const XMLParserContext* pParentContext,
									   const WCHAR* pwszQName,
									   unsigned int nWait) :
	pParser_(pParentContext->pParser_),
	pReader_(pParentContext->pReader_),
	pParentContext_(pParentContext),
	pwszQName_(pwszQName),
	nWait_(nWait)
{
}

qs::XMLParserContext::~XMLParserContext()
{
	for (NamespaceMap::iterator it = mapNamespace_.begin(); it != mapNamespace_.end(); ++it) {
		freeWString((*it).first);
		assert((*it).second);
		freeWString((*it).second);
	}
}

bool qs::XMLParserContext::expandQName(const WCHAR* pwszQName,
									   bool bUseDefault,
									   const WCHAR** ppwszNamespaceURI,
									   const WCHAR** ppwszLocalName) const
{
	assert(pwszQName);
	assert(ppwszNamespaceURI);
	assert(ppwszLocalName);
	
	Log log(InitThread::getInitThread().getLogger(), L"qs::XMLParserContext");
	
	*ppwszNamespaceURI = 0;
	*ppwszLocalName = 0;
	
	const WCHAR* pwszNamespaceURI = 0;
	const WCHAR* pwszLocalName = wcschr(pwszQName, L':');
	if (pwszLocalName) {
		size_t nLen = pwszLocalName - pwszQName;
		if (nLen == 5 && wcsncmp(pwszQName, L"xmlns", nLen) == 0) {
			log.error(L"xmlns is an invalid prefix.");
			return false;
		}
		pwszNamespaceURI = getNamespaceURI(pwszQName, nLen);
		if (!pwszNamespaceURI) {
			log.errorf(L"No namespace is declared for a prefix: %s", pwszQName);
			return false;
		}
		++pwszLocalName;
	}
	else {
		if (bUseDefault)
			pwszNamespaceURI = getNamespaceURI(0, 0);
		pwszLocalName = pwszQName;
	}
	
	*ppwszNamespaceURI = pwszNamespaceURI;
	*ppwszLocalName = pwszLocalName;
	
	return true;
}

const WCHAR* qs::XMLParserContext::getNamespaceURI(const WCHAR* pwszPrefix,
												   size_t nLen) const
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

bool qs::XMLParserContext::addNamespace(const WCHAR* pwszQName,
										const WCHAR* pwszURI)
{
	assert(pwszQName);
	assert(wcscmp(pwszQName, L"xmlns") == 0 || wcsncmp(pwszQName, L"xmlns:", 6) == 0);
	
	wstring_ptr wstrURI(allocWString(pwszURI));
	
	wstring_ptr wstrPrefix;
	const WCHAR* pwszPrefix = wcschr(pwszQName, L':');
	if (pwszPrefix)
		wstrPrefix = allocWString(pwszPrefix + 1);
	
	if (wstrPrefix.get() && !*wstrURI.get())
		return false;
	
	NamespaceMap::iterator it = std::find_if(
		mapNamespace_.begin(), mapNamespace_.end(),
		std::bind2nd(
			binary_compose_f_gx_hy(
				string_equal<WCHAR>(),
				std::select1st<NamespaceMap::value_type>(),
				std::identity<const WCHAR*>()),
			wstrPrefix.get()));
	if (it != mapNamespace_.end())
		return false;
	
	mapNamespace_.push_back(std::make_pair(wstrPrefix.get(), wstrURI.get()));
	wstrPrefix.release();
	wstrURI.release();
	
	return true;
}

bool qs::XMLParserContext::fireStartPrefixMappings(ContentHandler* pContentHandler) const
{
	if (pContentHandler) {
		for (NamespaceMap::const_iterator it = mapNamespace_.begin(); it != mapNamespace_.end(); ++it) {
			if (!pContentHandler->startPrefixMapping(
				(*it).first ? (*it).first : L"", (*it).second))
				return false;
		}
	}
	
	return true;
}

bool qs::XMLParserContext::fireEndPrefixMappings(ContentHandler* pContentHandler) const
{
	if (pContentHandler) {
		for (NamespaceMap::const_iterator it = mapNamespace_.begin(); it != mapNamespace_.end(); ++it) {
			if (!pContentHandler->endPrefixMapping(
				(*it).first ? (*it).first : L""))
				return false;
		}
	}
	
	return true;
}

bool qs::XMLParserContext::getChar(WCHAR* pChar)
{
	assert(pChar);
	
	size_t nRead = pReader_->read(pChar, 1);
	if (nRead == -1) {
		Log log(InitThread::getInitThread().getLogger(), L"qs::XMLParserContext");
		log.error(L"Failed to get a character.");
		return false;
	}
	else if (nRead == 0) {
		*pChar = L'\0';
	}
	
	return true;
}

bool qs::XMLParserContext::getString(WCHAR cFirst,
									 WCHAR* pwszSeparator,
									 wstring_ptr* pwstr,
									 WCHAR* pNext)
{
	assert(pwszSeparator);
	assert(pwstr);
	assert(pNext);
	
	pwstr->reset(0);
	
	StringBuffer<WSTRING> buf(10);
	
	if (cFirst != L'\0')
		buf.append(cFirst);
	
	WCHAR c = L'\0';
	if (!getChar(&c))
		return false;
	while (c != L'\0' && !wcschr(pwszSeparator, c)) {
		buf.append(c);
		if (!getChar(&c))
			return false;
	}
	if (c == L'\0') {
		Log log(InitThread::getInitThread().getLogger(), L"qs::XMLParserContext");
		log.errorf(L"An unexpected end of stream: %s", buf.getCharArray());
		return false;
	}
	
	*pwstr = buf.getString();
	*pNext = c;
	
	return true;
}

bool qs::XMLParserContext::eatWhitespaces(WCHAR c,
										  WCHAR* pNext)
{
	assert(pNext);
	
	if (c != L'\0' && !XMLParser::isWhitespace(c)) {
		*pNext = c;
		return true;
	}
	
	do {
		if (!getChar(&c) || c == L'\0')
			return false;
	} while (XMLParser::isWhitespace(c));
	
	*pNext = c;
	
	return true;
}

bool qs::XMLParserContext::expandReference(WCHAR* pcValue,
										   wstring_ptr* pwstrValue)
{
	Log log(InitThread::getInitThread().getLogger(), L"qs::XMLParserContext");
	log.debug(L"Begin expandReference");
	
	assert(pcValue);
	assert(pwstrValue);
	
	*pcValue = L'\0';
	pwstrValue->reset(0);
	
	wstring_ptr wstrName;
	WCHAR cNext = L'\0';
	if (!getString(L'\0', L";", &wstrName, &cNext)) {
		log.error(L"Failed to get an entity name.");
		return false;
	}
	assert(cNext == L';');
	
	log.debugf(L"Expanding reference: %s", wstrName.get());
	
	if (*wstrName.get() == L'#') {
		const WCHAR* p = wstrName.get() + 1;
		int nBase = 10;
		if (*p == L'x') {
			++p;
			nBase = 16;
		}
		WCHAR* pEnd = 0;
		long nValue = wcstol(p, &pEnd, nBase);
		if (nValue == 0 || *pEnd || nValue > 0xffff) {
			log.errorf(L"An invalid character reference: %s", wstrName.get());
			return false;
		}
		
		*pcValue = static_cast<WCHAR>(nValue);
	}
	else {
		if (!pParser_->validateNCName(wstrName.get())) {
			log.errorf(L"An invalid entity reference name: %s", wstrName.get());
			return false;
		}
		
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
			log.errorf(L"An unknown entity reference: %s", wstrName.get());
			// TODO
			// Handle entity
			return false;
		}
		
		*pcValue = c;
	}
	
	log.debug(L"End expandReference");
	
	return true;
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

void qs::XMLParserContext::setWait(unsigned int nWait,
								   unsigned int nMask)
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
												 InputStream* pInputStream) :
	pInputStream_(pInputStream),
	pBuf_(0),
	nRead_(0),
	p_(0)
{
	assert(pInputStream);
	
	pBuf_.reset(new unsigned char[nResettableSize]);
	nRead_ = pInputStream_->read(pBuf_.get(), nResettableSize);
	p_ = pBuf_.get();
}

qs::ResettableInputStream::~ResettableInputStream()
{
}

bool qs::ResettableInputStream::operator!() const
{
	return nRead_ == -1;
}

bool qs::ResettableInputStream::reset()
{
	if (!p_)
		return false;
	p_ = pBuf_.get();
	return true;
}

bool qs::ResettableInputStream::close()
{
	return pInputStream_->close();
}

size_t qs::ResettableInputStream::read(unsigned char* p,
									   size_t nRead)
{
	size_t nSize = 0;
	
	if (p_) {
		size_t nInBuf = nRead_ - (p_ - pBuf_.get());
		if (nRead > nInBuf) {
			memcpy(p, p_, nInBuf);
			p_ = 0;
			nSize = pInputStream_->read(p + nInBuf, nRead - nInBuf);
			if (nSize == -1)
				return -1;
			nSize += nInBuf;
		}
		else {
			memcpy(p, p_, nRead);
			p_ += nRead;
			nSize = nRead;
		}
	}
	else {
		nSize = pInputStream_->read(p, nRead);
	}
	
	return nSize;
}
