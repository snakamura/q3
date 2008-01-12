/*
 * $Id$
 *
 * Copyright(C) 1998-2008 Satoshi Nakamura
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
	XMLReader();
	~XMLReader();

public:
	bool getFeature(const WCHAR* pwszName) const;
	void setFeature(const WCHAR* pwszName,
					bool bValue);
	void* getProperty(const WCHAR* pwszName) const;
	void setProperty(const WCHAR* pwszName,
					 void* pValue);
	EntityResolver* getEntityResolver() const;
	void setEntityResolver(EntityResolver* pEntityResolver);
	DTDHandler* getDTDHandler() const;
	void setDTDHandler(DTDHandler* pDTDHandler);
	ContentHandler* getContentHandler() const;
	void setContentHandler(ContentHandler* pContentHandler);
	ErrorHandler* getErrorHandler() const;
	void setErrorHandler(ErrorHandler* pErrorHandler);
	bool parse(InputSource* pInputSource);
	bool parse(const WCHAR* pwszSystemId);

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
	virtual bool setDocumentLocator(const Locator& locator) = 0;
	virtual bool startDocument() = 0;
	virtual bool endDocument() = 0;
	virtual bool startPrefixMapping(const WCHAR* pwszPrefix,
									const WCHAR* pwszURI) = 0;
	virtual bool endPrefixMapping(const WCHAR* pwszPrefix) = 0;
	virtual bool startElement(const WCHAR* pwszNamespaceURI,
							  const WCHAR* pwszLocalName,
							  const WCHAR* pwszQName,
							  const Attributes& attributes) = 0;
	virtual bool endElement(const WCHAR* pwszNamespaceURI,
							const WCHAR* pwszLocalName,
							const WCHAR* pwszQName) = 0;
	virtual bool characters(const WCHAR* pwsz,
							size_t nStart,
							size_t nLength) = 0;
	virtual bool ignorableWhitespace(const WCHAR* pwsz,
									 size_t nStart,
									 size_t nLength) = 0;
	virtual bool processingInstruction(const WCHAR* pwszTarget,
									   const WCHAR* pwszData) = 0;
	virtual bool skippedEntity(const WCHAR* pwszName) = 0;
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
	virtual bool notationDecl(const WCHAR* pwszName,
							  const WCHAR* pwszPublicId,
							  const WCHAR* pwszSystemId) = 0;
	virtual bool unparsedEntityDecl(const WCHAR* pwszName,
									const WCHAR* pwszPublicId,
									const WCHAR* pwszSystemId,
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
	virtual bool elementDecl(const WCHAR* pwszName,
							 const WCHAR* pwszModel) = 0;
	virtual bool attributeDecl(const WCHAR* pwszElementName,
							   const WCHAR* pwszAttributeName,
							   const WCHAR* pwszType,
							   const WCHAR* pwszMode,
							   const WCHAR* pwszValue) = 0;
	virtual bool internalEntityDecl(const WCHAR* pwszName,
									const WCHAR* pwszValue) = 0;
	virtual bool externalEntityDecl(const WCHAR* pwszName,
									const WCHAR* pwszPublicId,
									const WCHAR* pwszSystemId) = 0;
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
	virtual bool startDTD(const WCHAR* pwszName,
						  const WCHAR* pwszPublicId,
						  const WCHAR* pwszSystemId) = 0;
	virtual bool endDTD() = 0;
	virtual bool startEntity(const WCHAR* pwszName) = 0;
	virtual bool endEntity(const WCHAR* pwszName) = 0;
	virtual bool startCDATA() = 0;
	virtual bool endCDATA() = 0;
	virtual bool comment(const WCHAR* pwsz,
						 size_t nStart,
						 size_t nLength) = 0;
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
	virtual std::auto_ptr<InputSource> resolveEntity(const WCHAR* pwszPublicId,
													const WCHAR* pwszSystemId) = 0;
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
	virtual std::auto_ptr<InputSource> getExternalSubset(const WCHAR* pwszName,
														 const WCHAR* pwzzBaseURI) = 0;
	virtual std::auto_ptr<InputSource> resolveEntity(const WCHAR* pwszName,
													 const WCHAR* pwszPublicId,
													 const WCHAR* pwszBaseURI,
													 const WCHAR* pwszSystemId) = 0;
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
	DefaultHandler();
	virtual ~DefaultHandler();

public:
	virtual bool setDocumentLocator(const Locator& locator);
	virtual bool startDocument();
	virtual bool endDocument();
	virtual bool startPrefixMapping(const WCHAR* pwszPrefix,
									const WCHAR* pwszURI);
	virtual bool endPrefixMapping(const WCHAR* pwszPrefix);
	virtual bool startElement(const WCHAR* pwszNamespaceURI,
							  const WCHAR* pwszLocalName,
							  const WCHAR* pwszQName,
							  const Attributes& attributes);
	virtual bool endElement(const WCHAR* pwszNamespaceURI,
							const WCHAR* pwszLocalName,
							const WCHAR* pwszQName);
	virtual bool characters(const WCHAR* pwsz,
							size_t nStart,
							size_t nLength);
	virtual bool ignorableWhitespace(const WCHAR* pwsz,
									 size_t nStart,
									 size_t nLength);
	virtual bool processingInstruction(const WCHAR* pwszTarget,
									   const WCHAR* pwszData);
	virtual bool skippedEntity(const WCHAR* pwszName);

public:
	virtual bool notationDecl(const WCHAR* pwszName,
							  const WCHAR* pwszPublicId,
							  const WCHAR* pwszSystemId);
	virtual bool unparsedEntityDecl(const WCHAR* pwszName,
									const WCHAR* pwszPublicId,
									const WCHAR* pwszSystemId,
									const WCHAR* pwszNotationName);

public:
	virtual std::auto_ptr<InputSource> resolveEntity(const WCHAR* pwszPublicId,
													 const WCHAR* pwszSystemId);
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
	DefaultHandler2();
	virtual ~DefaultHandler2();

public:
	virtual bool elementDecl(const WCHAR* pwszName,
							 const WCHAR* pwszModel);
	virtual bool attributeDecl(const WCHAR* pwszElementName,
							   const WCHAR* pwszAttributeName,
							   const WCHAR* pwszType,
							   const WCHAR* pwszMode,
							   const WCHAR* pwszValue);
	virtual bool internalEntityDecl(const WCHAR* pwszName,
									const WCHAR* pwszValue);
	virtual bool externalEntityDecl(const WCHAR* pwszName,
									const WCHAR* pwszPublicId,
									const WCHAR* pwszSystemId);

public:
	virtual bool startDTD(const WCHAR* pwszName,
						  const WCHAR* pwszPublicId,
						  const WCHAR* pwszSystemId);
	virtual bool endDTD();
	virtual bool startEntity(const WCHAR* pwszName);
	virtual bool endEntity(const WCHAR* pwszName);
	virtual bool startCDATA();
	virtual bool endCDATA();
	virtual bool comment(const WCHAR* pwsz,
						 size_t nStart,
						 size_t nLength);

public:
	virtual std::auto_ptr<InputSource> resolveEntity(const WCHAR* pwszPublicId,
													 const WCHAR* pwszSystemId);

public:
	virtual std::auto_ptr<InputSource> getExternalSubset(const WCHAR* pwszName,
														 const WCHAR* pwzzBaseURI);
	virtual std::auto_ptr<InputSource> resolveEntity(const WCHAR* pwszName,
													 const WCHAR* pwszPublicId,
													 const WCHAR* pwszBaseURI,
													 const WCHAR* pwszSystemId);
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
	InputSource();
	explicit InputSource(const WCHAR* pwszSystemId);
	explicit InputSource(InputStream* pInputStream);
	explicit InputSource(Reader* pReader);
	~InputSource();

public:
	void setPublicId(const WCHAR* pwszPublicId);
	const WCHAR* getPublicId() const;
	void setSystemId(const WCHAR* pwszSystemId);
	const WCHAR* getSystemId() const;
	void setByteStream(InputStream* pInputStream);
	InputStream* getByteStream() const;
	void setEncoding(const WCHAR* pwszEncoding);
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
	OutputHandler(Writer* pWriter,
				  const WCHAR* pwszEncoding);
	virtual ~OutputHandler();

public:
	virtual bool startDocument();
	virtual bool endDocument();
	virtual bool startElement(const WCHAR* pwszNamespaceURI,
							  const WCHAR* pwszLocalName,
							  const WCHAR* pwszQName,
							  const Attributes& attributes);
	virtual bool endElement(const WCHAR* pwszNamespaceURI,
							const WCHAR* pwszLocalName,
							const WCHAR* pwszQName);
	virtual bool characters(const WCHAR* pwsz,
							size_t nStart,
							size_t nLength);

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


/****************************************************************************
 *
 * SimpleAttributes
 *
 */

class QSEXPORTCLASS SimpleAttributes : public DefaultAttributes
{
public:
	struct Item
	{
		const WCHAR* pwszQName_;
		const WCHAR* pwszValue_;
		bool bOmit_;
	};

public:
	SimpleAttributes(const WCHAR* pwszQName,
					 const WCHAR* pwszValue);
	SimpleAttributes(const Item* pItems,
					 int nSize);
	virtual ~SimpleAttributes();

public:
	virtual int getLength() const;
	virtual const WCHAR* getQName(int nIndex) const;
	virtual const WCHAR* getValue(int nIndex) const;

private:
	SimpleAttributes(const SimpleAttributes&);
	SimpleAttributes& operator=(const SimpleAttributes&);

private:
	const Item* pItems_;
	int nSize_;
	int nCount_;
	Item item_;
};


/****************************************************************************
 *
 * HandlerHelper
 *
 */

class QSEXPORTCLASS HandlerHelper
{
public:
	static bool textElement(ContentHandler* pHandler,
							const WCHAR* pwszQName,
							const WCHAR* pwszValue,
							size_t nLen);
	static bool numberElement(ContentHandler* pHandler,
							  const WCHAR* pwszQName,
							  unsigned int nValue);
};

}

#endif // __QSSAX_H__
