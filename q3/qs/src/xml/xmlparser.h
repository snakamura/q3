/*
 * $Id$
 *
 * Copyright(C) 1998-2005 Satoshi Nakamura
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
			  unsigned int nFlags);
	~XMLParser();

public:
	bool parse(const InputSource& source);

public:
	bool validateQName(const WCHAR* pwsz) const;
	bool validateNCName(const WCHAR* pwsz) const;

public:
	static bool isWhitespace(WCHAR c);
	static bool isName(const WCHAR* pwsz);
	static bool isQName(const WCHAR* pwsz);
	static bool isNCName(const WCHAR* pwsz);

private:
	bool parseXmlDecl(InputStream* pInputStream,
					  std::auto_ptr<Reader>* ppReader);
	bool parseXmlDecl(Reader* pReader);
	bool parse(XMLParserContext& context);
	bool parseStartElement(XMLParserContext& context,
						   WCHAR c);
	bool parseEndElement(XMLParserContext& context);
	bool parseAttributes(XMLParserContext& context,
						 WCHAR c,
						 AttributeList* pListAttribute,
						 WCHAR* pNext);
	bool parseCharacter(XMLParserContext& context,
						WCHAR c,
						WCHAR* pNext);
	bool parseComment(XMLParserContext& context);
	bool parseCDATASection(XMLParserContext& context);
	bool parsePI(XMLParserContext& context);
	bool parseDoctype(XMLParserContext& context);
	bool parsePublicId(XMLParserContext& context,
					   WCHAR c);
	bool parseSystemId(XMLParserContext& context,
					   WCHAR c);
	bool parseLiteral(XMLParserContext& context,
					  WCHAR c);

private:
	static bool getChar(InputStream* pInputStream,
						unsigned char* pChar);
	static bool skipWhitespace(InputStream* pInputStream,
							   char* pNext);
	static bool isEqualAttribute(const Attribute& lhs,
								 const Attribute& rhs,
								 bool bNamespace);

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
		WAIT_WS						= 0x20,
		WAIT_DOCTYPE				= 0x40
	};

public:
	typedef std::vector<std::pair<WSTRING, WSTRING> > NamespaceMap;

public:
	XMLParserContext(XMLParser* pParser,
					 Reader* pReader,
					 unsigned int nWait);
	XMLParserContext(const XMLParserContext* pParentContext,
					 const WCHAR* pwszQName,
					 unsigned int nWait);
	~XMLParserContext();

public:
	bool expandQName(const WCHAR* pwszQName,
					 bool bUseDefault,
					 const WCHAR** ppwszNamespaceURI,
					 const WCHAR** ppwszLocalName) const;
	const WCHAR* getNamespaceURI(const WCHAR* pwszPrefix,
								 size_t nLen) const;
	bool addNamespace(const WCHAR* pwszQName,
					  const WCHAR* pwszURI);
	bool fireStartPrefixMappings(ContentHandler* pContentHandler) const;
	bool fireEndPrefixMappings(ContentHandler* pContentHandler) const;
	bool getChar(WCHAR* pChar);
	bool getString(WCHAR cFirst,
				   WCHAR* pwszSeparator,
				   wstring_ptr* pwstr,
				   WCHAR* pNext);
	bool expandReference(wstring_ptr* pwstrValue);
	const XMLParserContext* getParentContext() const;
	const WCHAR* getQName() const;
	bool isWait(Wait wait) const;
	void setWait(unsigned int nWait,
				 unsigned int nMask);

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
						  InputStream* pInputStream);
	virtual ~ResettableInputStream();

public:
	bool operator!() const;

public:
	bool reset();

public:
	virtual bool close();
	virtual size_t read(unsigned char* p,
						size_t nRead);

private:
	ResettableInputStream(const ResettableInputStream&);
	ResettableInputStream& operator=(const ResettableInputStream&);

private:
	InputStream* pInputStream_;
	auto_ptr_array<unsigned char> pBuf_;
	size_t nRead_;
	unsigned char* p_;
};

}

#endif // __PARSER_H__
