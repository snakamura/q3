/*
 * $Id$
 *
 * Copyright(C) 1998-2003 Satoshi Nakamura
 * All rights reserved.
 *
 */

#ifndef __XMLPARSER_H__
#define __XMLPARSER_H__

#include <qs.h>
#include <qsstream.h>
#include <qsstring.h>

namespace qs {

class XMLParser;
class XMLParserContext;

class InputSource;
class InputStream;
class Reader;


/****************************************************************************
 *
 * XMLParser
 *
 */

class XMLParser
{
public:
	enum Flag {
		FLAG_NAMESPACES			= 0x01,
		FLAG_NAMESPACEPREFIXES	= 0x02
	};

public:
	typedef std::vector<Attribute> AttributeList;

public:
	XMLParser(ContentHandler* pContentHandler,
		unsigned int nFlags, QSTATUS* pstatus);
	~XMLParser();

public:
	QSTATUS parse(const InputSource& source);

public:
	QSTATUS validateQName(const WCHAR* pwsz) const;
	QSTATUS validateNCName(const WCHAR* pwsz) const;

public:
	static bool isWhitespace(WCHAR c);
	static bool isName(const WCHAR* pwsz);
	static bool isQName(const WCHAR* pwsz);
	static bool isNCName(const WCHAR* pwsz);

private:
	QSTATUS parseXmlDecl(InputStream* pInputStream, Reader** ppReader);
	QSTATUS parseXmlDecl(Reader* pReader);
	QSTATUS parse(XMLParserContext& context);
	QSTATUS parseStartElement(XMLParserContext& context, WCHAR c);
	QSTATUS parseEndElement(XMLParserContext& context);
	QSTATUS parseAttributes(XMLParserContext& context, WCHAR c,
		AttributeList* pListAttribute, WCHAR* pNext);
	QSTATUS parseCharacter(XMLParserContext& context, WCHAR c, WCHAR* pNext);
	QSTATUS parseComment(XMLParserContext& context);
	QSTATUS parseCDATASection(XMLParserContext& context);
	QSTATUS parsePI(XMLParserContext& context);

private:
	static QSTATUS getChar(InputStream* pInputStream, unsigned char* pChar);
	static QSTATUS skipWhitespace(InputStream* pInputStream, char* pNext);
	static bool isEqualAttribute(const Attribute& lhs,
		const Attribute& rhs, bool bNamespace);

private:
	struct AttributeListDeleter
	{
		AttributeListDeleter(AttributeList* p);
		~AttributeListDeleter();
		void release();
		AttributeList* p_;
	};

private:
	XMLParser(const XMLParser&);
	XMLParser& operator=(const XMLParser&);

private:
	ContentHandler* pContentHandler_;
	unsigned int nFlags_;
};


/****************************************************************************
 *
 * XMLParserContext
 *
 */

class XMLParserContext
{
public:
	enum Wait {
		WAIT_STARTELEMENT			= 0x01,
		WAIT_ENDELEMENT				= 0x02,
		WAIT_PROCESSINGINSTRUCTION	= 0x04,
		WAIT_COMMENT				= 0x08,
		WAIT_CHARACTER				= 0x10,
		WAIT_WS						= 0x20
	};

public:
	typedef std::vector<std::pair<WSTRING, WSTRING> > NamespaceMap;

public:
	XMLParserContext(XMLParser* pParser, Reader* pReader, unsigned int nWait);
	XMLParserContext(const XMLParserContext* pParentContext,
		const WCHAR* pwszQName, unsigned int nWait);
	~XMLParserContext();

public:
	QSTATUS expandQName(const WCHAR* pwszQName, bool bUseDefault,
		const WCHAR** ppwszNamespaceURI, const WCHAR** ppwszLocalName) const;
	const WCHAR* getNamespaceURI(const WCHAR* pwszPrefix, size_t nLen) const;
	QSTATUS addNamespace(const WCHAR* pwszQName, const WCHAR* pwszURI);
	QSTATUS fireStartPrefixMappings(ContentHandler* pContentHandler) const;
	QSTATUS fireEndPrefixMappings(ContentHandler* pContentHandler) const;
	QSTATUS getChar(WCHAR* pChar);
	QSTATUS getString(WCHAR cFirst, WCHAR* pwszSeparator,
		WSTRING* pwstr, WCHAR* pNext);
	QSTATUS expandReference(WSTRING* pwstrValue);
	const XMLParserContext* getParentContext() const;
	const WCHAR* getQName() const;
	bool isWait(Wait wait) const;
	void setWait(unsigned int nWait, unsigned int nMask);

private:
	XMLParserContext(const XMLParserContext&);
	XMLParserContext& operator=(const XMLParserContext&);

private:
	XMLParser* pParser_;
	Reader* pReader_;
	const XMLParserContext* pParentContext_;
	const WCHAR* pwszQName_;
	unsigned int nWait_;
	NamespaceMap mapNamespace_;
};


/****************************************************************************
 *
 * ResettableInputStream
 *
 */

class ResettableInputStream : public InputStream
{
public:
	ResettableInputStream(size_t nResettableSize,
		InputStream* pInputStream, QSTATUS* pstatus);
	virtual ~ResettableInputStream();

public:
	QSTATUS reset();

public:
	virtual QSTATUS close();
	virtual QSTATUS read(unsigned char* p, size_t nRead, size_t* pnRead);

private:
	ResettableInputStream(const ResettableInputStream&);
	ResettableInputStream& operator=(const ResettableInputStream&);

private:
	InputStream* pInputStream_;
	unsigned char* pBuf_;
	size_t nRead_;
	unsigned char* p_;
};

}

#endif // __PARSER_H__
