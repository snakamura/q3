/*
 * $Id$
 *
 * Copyright(C) 1998-2003 Satoshi Nakamura
 * All rights reserved.
 *
 */

#ifndef __QSSAX_H__
#define __QSSAX_H__

#include <qs.h>


namespace qs {

class XMLReader;
class ContentHandler;
class DTDHandler;
class DeclHandler;
class LexicalHandler;
class ErrorHandler;
class EntityResolver;
	class EntityResolver2;
	class DefaultHandler;
		class DefaultHandler2;
			class OutputHandler;
class Attributes;
	class Attributes2;
		class DefaultAttributes;
class Locator;
	class Locator2;
class InputSource;

class InputStream;
class Reader;
class Writer;


/****************************************************************************
 *
 * XMLReader
 *
 */

class QSEXPORTCLASS XMLReader
{
public:
	XMLReader(QSTATUS* pstatus);
	~XMLReader();

public:
	QSTATUS getFeature(const WCHAR* pwszName, bool* pbValue) const;
	QSTATUS setFeature(const WCHAR* pwszName, bool bValue);
	QSTATUS getProperty(const WCHAR* pwszName, void** ppValue) const;
	QSTATUS setProperty(const WCHAR* pwszName, void* pValue);
	EntityResolver* getEntityResolver() const;
	void setEntityResolver(EntityResolver* pEntityResolver);
	DTDHandler* getDTDHandler() const;
	void setDTDHandler(DTDHandler* pDTDHandler);
	ContentHandler* getContentHandler() const;
	void setContentHandler(ContentHandler* pContentHandler);
	ErrorHandler* getErrorHandler() const;
	void setErrorHandler(ErrorHandler* pErrorHandler);
	QSTATUS parse(InputSource* pInputSource);
	QSTATUS parse(const WCHAR* pwszSystemId);

private:
	XMLReader(const XMLReader&);
	XMLReader& operator=(const XMLReader&);

private:
	struct XMLReaderImpl* pImpl_;
};


/****************************************************************************
 *
 * ContentHandler
 *
 */

class QSEXPORTCLASS ContentHandler
{
public:
	virtual ~ContentHandler();

public:
	virtual QSTATUS setDocumentLocator(const Locator& locator) = 0;
	virtual QSTATUS startDocument() = 0;
	virtual QSTATUS endDocument() = 0;
	virtual QSTATUS startPrefixMapping(
		const WCHAR* pwszPrefix, const WCHAR* pwszURI) = 0;
	virtual QSTATUS endPrefixMapping(const WCHAR* pwszPrefix) = 0;
	virtual QSTATUS startElement(const WCHAR* pwszNamespaceURI,
		const WCHAR* pwszLocalName, const WCHAR* pwszQName,
		const Attributes& attributes) = 0;
	virtual QSTATUS endElement(const WCHAR* pwszNamespaceURI,
		const WCHAR* pwszLocalName, const WCHAR* pwszQName) = 0;
	virtual QSTATUS characters(const WCHAR* pwsz,
		size_t nStart, size_t nLength) = 0;
	virtual QSTATUS ignorableWhitespace(const WCHAR* pwsz,
		size_t nStart, size_t nLength) = 0;
	virtual QSTATUS processingInstruction(
		const WCHAR* pwszTarget, const WCHAR* pwszData) = 0;
	virtual QSTATUS skippedEntity(const WCHAR* pwszName) = 0;
};


/****************************************************************************
 *
 * DTDHandler
 *
 */

class QSEXPORTCLASS DTDHandler
{
public:
	virtual ~DTDHandler();

public:
	virtual QSTATUS notationDecl(const WCHAR* pwszName,
		const WCHAR* pwszPublicId, const WCHAR* pwszSystemId) = 0;
	virtual QSTATUS unparsedEntityDecl(const WCHAR* pwszName,
		const WCHAR* pwszPublicId, const WCHAR* pwszSystemId,
		const WCHAR* pwszNotationName) = 0;
};


/****************************************************************************
 *
 * DeclHandler
 *
 */

class QSEXPORTCLASS DeclHandler
{
public:
	virtual ~DeclHandler();

public:
	virtual QSTATUS elementDecl(const WCHAR* pwszName,
		const WCHAR* pwszModel) = 0;
	virtual QSTATUS attributeDecl(const WCHAR* pwszElementName,
		const WCHAR* pwszAttributeName, const WCHAR* pwszType,
		const WCHAR* pwszMode, const WCHAR* pwszValue) = 0;
	virtual QSTATUS internalEntityDecl(const WCHAR* pwszName,
		const WCHAR* pwszValue) = 0;
	virtual QSTATUS externalEntityDecl(const WCHAR* pwszName,
		const WCHAR* pwszPublicId, const WCHAR* pwszSystemId) = 0;
};


/****************************************************************************
 *
 * LexicalHandler
 *
 */

class QSEXPORTCLASS LexicalHandler
{
public:
	virtual ~LexicalHandler();

public:
	virtual QSTATUS startDTD(const WCHAR* pwszName,
		const WCHAR* pwszPublicId, const WCHAR* pwszSystemId) = 0;
	virtual QSTATUS endDTD() = 0;
	virtual QSTATUS startEntity(const WCHAR* pwszName) = 0;
	virtual QSTATUS endEntity(const WCHAR* pwszName) = 0;
	virtual QSTATUS startCDATA() = 0;
	virtual QSTATUS endCDATA() = 0;
	virtual QSTATUS comment(const WCHAR* pwsz,
		size_t nStart, size_t nLength) = 0;
};


/****************************************************************************
 *
 * ErrorHandler
 *
 */

class QSEXPORTCLASS ErrorHandler
{
public:
	virtual ~ErrorHandler();
};


/****************************************************************************
 *
 * EntityResolver
 *
 */

class QSEXPORTCLASS EntityResolver
{
public:
	virtual ~EntityResolver();

public:
	virtual QSTATUS resolveEntity(const WCHAR* pwszPublicId,
		const WCHAR* pwszSystemId, InputSource** ppInputSource) = 0;
};


/****************************************************************************
 *
 * EntityResolver2
 *
 */

class QSEXPORTCLASS EntityResolver2 : public EntityResolver
{
public:
	virtual ~EntityResolver2();

public:
	virtual QSTATUS getExternalSubset(const WCHAR* pwszName,
		const WCHAR* pwzzBaseURI, InputSource** ppInputSource) = 0;
	virtual QSTATUS resolveEntity(const WCHAR* pwszName,
		const WCHAR* pwszPublicId, const WCHAR* pwszBaseURI,
		const WCHAR* pwszSystemId, InputSource** ppSource) = 0;
};


/****************************************************************************
 *
 * DefaultHandler
 *
 */

class QSEXPORTCLASS DefaultHandler :
	public ContentHandler,
	public DTDHandler,
	public ErrorHandler,
	public EntityResolver
{
public:
	DefaultHandler(QSTATUS* pstatus);
	virtual ~DefaultHandler();

public:
	virtual QSTATUS setDocumentLocator(const Locator& locator);
	virtual QSTATUS startDocument();
	virtual QSTATUS endDocument();
	virtual QSTATUS startPrefixMapping(
		const WCHAR* pwszPrefix, const WCHAR* pwszURI);
	virtual QSTATUS endPrefixMapping(const WCHAR* pwszPrefix);
	virtual QSTATUS startElement(const WCHAR* pwszNamespaceURI,
		const WCHAR* pwszLocalName, const WCHAR* pwszQName,
		const Attributes& attributes);
	virtual QSTATUS endElement(const WCHAR* pwszNamespaceURI,
		const WCHAR* pwszLocalName, const WCHAR* pwszQName);
	virtual QSTATUS characters(const WCHAR* pwsz,
		size_t nStart, size_t nLength);
	virtual QSTATUS ignorableWhitespace(const WCHAR* pwsz,
		size_t nStart, size_t nLength);
	virtual QSTATUS processingInstruction(
		const WCHAR* pwszTarget, const WCHAR* pwszData);
	virtual QSTATUS skippedEntity(const WCHAR* pwszName);

public:
	virtual QSTATUS notationDecl(const WCHAR* pwszName,
		const WCHAR* pwszPublicId, const WCHAR* pwszSystemId);
	virtual QSTATUS unparsedEntityDecl(const WCHAR* pwszName,
		const WCHAR* pwszPublicId, const WCHAR* pwszSystemId,
		const WCHAR* pwszNotationName);

public:
	virtual QSTATUS resolveEntity(const WCHAR* pwszPublicId,
		const WCHAR* pwszSystemId, InputSource** ppInputSource);
};


/****************************************************************************
 *
 * DefaultHandler2
 *
 */

class QSEXPORTCLASS DefaultHandler2 :
	public DefaultHandler,
	public DeclHandler,
	public LexicalHandler,
	public EntityResolver2
{
public:
	DefaultHandler2(QSTATUS* pstatus);
	virtual ~DefaultHandler2();

public:
	virtual QSTATUS elementDecl(const WCHAR* pwszName,
		const WCHAR* pwszModel);
	virtual QSTATUS attributeDecl(const WCHAR* pwszElementName,
		const WCHAR* pwszAttributeName, const WCHAR* pwszType,
		const WCHAR* pwszMode, const WCHAR* pwszValue);
	virtual QSTATUS internalEntityDecl(const WCHAR* pwszName,
		const WCHAR* pwszValue);
	virtual QSTATUS externalEntityDecl(const WCHAR* pwszName,
		const WCHAR* pwszPublicId, const WCHAR* pwszSystemId);

public:
	virtual QSTATUS startDTD(const WCHAR* pwszName,
		const WCHAR* pwszPublicId, const WCHAR* pwszSystemId);
	virtual QSTATUS endDTD();
	virtual QSTATUS startEntity(const WCHAR* pwszName);
	virtual QSTATUS endEntity(const WCHAR* pwszName);
	virtual QSTATUS startCDATA();
	virtual QSTATUS endCDATA();
	virtual QSTATUS comment(const WCHAR* pwsz,
		size_t nStart, size_t nLength);

public:
	virtual QSTATUS resolveEntity(const WCHAR* pwszPublicId,
		const WCHAR* pwszSystemId, InputSource** ppInputSource);

public:
	virtual QSTATUS getExternalSubset(const WCHAR* pwszName,
		const WCHAR* pwzzBaseURI, InputSource** ppInputSource);
	virtual QSTATUS resolveEntity(const WCHAR* pwszName,
		const WCHAR* pwszPublicId, const WCHAR* pwszBaseURI,
		const WCHAR* pwszSystemId, InputSource** ppSource);
};


/****************************************************************************
 *
 * Attributes
 *
 */

class QSEXPORTCLASS Attributes
{
public:
	virtual ~Attributes();

public:
	virtual int getLength() const = 0;
	virtual const WCHAR* getURI(int nIndex) const = 0;
	virtual const WCHAR* getLocalName(int nIndex) const = 0;
	virtual const WCHAR* getQName(int nIndex) const = 0;
	virtual const WCHAR* getType(int nIndex) const = 0;
	virtual const WCHAR* getValue(int nIndex) const = 0;
	virtual int getIndex(const WCHAR* pwszURI,
		const WCHAR* pwszLocalName) const = 0;
	virtual int getIndex(const WCHAR* pwszQName) const = 0;
	virtual const WCHAR* getType(const WCHAR* pwszURI,
		const WCHAR* pwszLocalName) const = 0;
	virtual const WCHAR* getType(const WCHAR* pwszQName) const = 0;
	virtual const WCHAR* getValue(const WCHAR* pwszURI,
		const WCHAR* pwszLocalName) const = 0;
	virtual const WCHAR* getValue(const WCHAR* pwszQName) const = 0;
};


/****************************************************************************
 *
 * Attributes2
 *
 */

class QSEXPORTCLASS Attributes2 : public Attributes
{
public:
	virtual ~Attributes2();

public:
	virtual bool isDeclared(int index) const = 0;
	virtual bool isDeclared(const WCHAR* pwszQName) const = 0;
	virtual bool isDeclared(const WCHAR* pwszURI,
		const WCHAR* pwszLocalName) const = 0;
	virtual bool isSpecified(int index) const = 0;
	virtual bool isSpecified(const WCHAR* pwszQName) const = 0;
	virtual bool isSpecified(const WCHAR* pwszURI,
		const WCHAR* pwszLocalName) const = 0;
};


/****************************************************************************
 *
 * Locator
 *
 */

class QSEXPORTCLASS Locator
{
public:
	virtual ~Locator();

public:
	virtual int getColumnNumber() const = 0;
	virtual int getLineNumber() const = 0;
	virtual const WCHAR* getPublicId() const = 0;
	virtual const WCHAR* getSystemId() const = 0;
};


/****************************************************************************
 *
 * Locator2
 *
 */

class QSEXPORTCLASS Locator2 : public Locator
{
public:
	virtual ~Locator2();

public:
	virtual const WCHAR* getEncoding() const = 0;
	virtual const WCHAR* getXMLVersion() const = 0;
};


/****************************************************************************
 *
 * InputSource
 *
 */

class QSEXPORTCLASS InputSource
{
public:
	InputSource(QSTATUS* pstatus);
	InputSource(const WCHAR* pwszSystemId, QSTATUS* pstatus);
	InputSource(InputStream* pInputStream, QSTATUS* pstatus);
	InputSource(Reader* pReader, QSTATUS* pstatus);
	~InputSource();

public:
	QSTATUS setPublicId(const WCHAR* pwszPublicId);
	const WCHAR* getPublicId() const;
	QSTATUS setSystemId(const WCHAR* pwszSystemId);
	const WCHAR* getSystemId() const;
	void setByteStream(InputStream* pInputStream);
	InputStream* getByteStream() const;
	QSTATUS setEncoding(const WCHAR* pwszEncoding);
	const WCHAR* getEncoding() const;
	void setCharacterStream(Reader* pReader);
	Reader* getCharacterStream() const;

private:
	struct InputSourceImpl* pImpl_;
};


/****************************************************************************
 *
 * OutputHandler
 *
 */

class QSEXPORTCLASS OutputHandler : public DefaultHandler2
{
public:
	OutputHandler(Writer* pWriter, qs::QSTATUS* pstatus);
	virtual ~OutputHandler();

public:
	virtual QSTATUS startElement(const WCHAR* pwszNamespaceURI,
		const WCHAR* pwszLocalName, const WCHAR* pwszQName,
		const Attributes& attributes);
	virtual QSTATUS endElement(const WCHAR* pwszNamespaceURI,
		const WCHAR* pwszLocalName, const WCHAR* pwszQName);
	virtual QSTATUS characters(const WCHAR* pwsz,
		size_t nStart, size_t nLength);

private:
	OutputHandler(const OutputHandler&);
	OutputHandler& operator=(const OutputHandler&);

private:
	struct OutputHandlerImpl* pImpl_;
};


/****************************************************************************
 *
 * DefaultAttributes
 *
 */

class QSEXPORTCLASS DefaultAttributes : public Attributes2
{
public:
	DefaultAttributes();
	virtual ~DefaultAttributes();

public:
	virtual int getLength() const;
	virtual const WCHAR* getURI(int nIndex) const;
	virtual const WCHAR* getLocalName(int nIndex) const;
	virtual const WCHAR* getQName(int nIndex) const;
	virtual const WCHAR* getType(int nIndex) const;
	virtual const WCHAR* getValue(int nIndex) const;
	virtual int getIndex(const WCHAR* pwszURI,
		const WCHAR* pwszLocalName) const;
	virtual int getIndex(const WCHAR* pwszQName) const;
	virtual const WCHAR* getType(const WCHAR* pwszURI,
		const WCHAR* pwszLocalName) const;
	virtual const WCHAR* getType(const WCHAR* pwszQName) const;
	virtual const WCHAR* getValue(const WCHAR* pwszURI,
		const WCHAR* pwszLocalName) const;
	virtual const WCHAR* getValue(const WCHAR* pwszQName) const;

public:
	virtual bool isDeclared(int index) const;
	virtual bool isDeclared(const WCHAR* pwszQName) const;
	virtual bool isDeclared(const WCHAR* pwszURI,
		const WCHAR* pwszLocalName) const;
	virtual bool isSpecified(int index) const;
	virtual bool isSpecified(const WCHAR* pwszQName) const;
	virtual bool isSpecified(const WCHAR* pwszURI,
		const WCHAR* pwszLocalName) const;

private:
	DefaultAttributes(const DefaultAttributes&);
	DefaultAttributes& operator=(const DefaultAttributes&);
};

}

#endif // __QSSAX_H__
