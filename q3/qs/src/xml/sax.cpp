/*
 * $Id$
 *
 * Copyright(C) 1998-2004 Satoshi Nakamura
 * All rights reserved.
 *
 */

#include <qssax.h>
#include <qsstream.h>
#include <qsstring.h>

#include "sax.h"
#include "xmlparser.h"

using namespace qs;


/****************************************************************************
 *
 * XMLReaderImpl
 *
 */

struct qs::XMLReaderImpl
{
	ContentHandler* pContentHandler_;
	DTDHandler* pDTDHandler_;
	DeclHandler* pDeclHandler_;
	LexicalHandler* pLexicalHandler_;
	ErrorHandler* pErrorHandler_;
	EntityResolver* pEntityResolver_;
	unsigned int nFlags_;
};


/****************************************************************************
 *
 * XMLReader
 *
 */

qs::XMLReader::XMLReader() :
	pImpl_(0)
{
	pImpl_ = new XMLReaderImpl();
	pImpl_->pContentHandler_ = 0;
	pImpl_->pDTDHandler_ = 0;
	pImpl_->pDeclHandler_ = 0;
	pImpl_->pLexicalHandler_ = 0;
	pImpl_->pErrorHandler_ = 0;
	pImpl_->pEntityResolver_ = 0;
	pImpl_->nFlags_ = XMLParser::FLAG_NAMESPACES;
}

qs::XMLReader::~XMLReader()
{
	delete pImpl_;
}

bool qs::XMLReader::getFeature(const WCHAR* pwszName) const
{
	if (wcscmp(pwszName, L"http://xml.org/sax/features/external-general-entities") == 0) {
		return false;
	}
	else if (wcscmp(pwszName, L"http://xml.org/sax/features/external-parameter-entities") == 0) {
		return false;
	}
	else if (wcscmp(pwszName, L"http://xml.org/sax/features/is-standalone") == 0) {
	}
	else if (wcscmp(pwszName, L"http://xml.org/sax/features/lexical-handler/parameter-entities") == 0) {
		return false;
	}
	else if (wcscmp(pwszName, L"http://xml.org/sax/features/namespaces") == 0) {
		return (pImpl_->nFlags_ & XMLParser::FLAG_NAMESPACES) != 0;
	}
	else if (wcscmp(pwszName, L"http://xml.org/sax/features/namespace-prefixes") == 0) {
		return (pImpl_->nFlags_ & XMLParser::FLAG_NAMESPACEPREFIXES) != 0;
	}
	else if (wcscmp(pwszName, L"http://xml.org/sax/features/resolve-dtd-uris") == 0) {
	}
	else if (wcscmp(pwszName, L"http://xml.org/sax/features/string-interning") == 0) {
	}
	else if (wcscmp(pwszName, L"http://xml.org/sax/features/use-attributes2") == 0) {
	}
	else if (wcscmp(pwszName, L"http://xml.org/sax/features/use-locator2") == 0) {
	}
	else if (wcscmp(pwszName, L"http://xml.org/sax/features/use-entity-resolver2") == 0) {
	}
	else if (wcscmp(pwszName, L"http://xml.org/sax/features/validation") == 0) {
	}
	else {
	}
	
	return true;
}

void qs::XMLReader::setFeature(const WCHAR* pwszName,
							   bool bValue)
{
	// TODO
	
	if (wcscmp(pwszName, L"http://xml.org/sax/features/external-general-entities") == 0) {
	}
	else if (wcscmp(pwszName, L"http://xml.org/sax/features/external-parameter-entities") == 0) {
	}
	else if (wcscmp(pwszName, L"http://xml.org/sax/features/is-standalone") == 0) {
	}
	else if (wcscmp(pwszName, L"http://xml.org/sax/features/lexical-handler/parameter-entities") == 0) {
	}
	else if (wcscmp(pwszName, L"http://xml.org/sax/features/namespaces") == 0) {
		if (bValue)
			pImpl_->nFlags_ |= XMLParser::FLAG_NAMESPACES;
		else
			pImpl_->nFlags_ &= ~XMLParser::FLAG_NAMESPACES;
	}
	else if (wcscmp(pwszName, L"http://xml.org/sax/features/namespace-prefixes") == 0) {
		if (bValue)
			pImpl_->nFlags_ |= XMLParser::FLAG_NAMESPACEPREFIXES;
		else
			pImpl_->nFlags_ &= ~XMLParser::FLAG_NAMESPACEPREFIXES;
	}
	else if (wcscmp(pwszName, L"http://xml.org/sax/features/resolve-dtd-uris") == 0) {
	}
	else if (wcscmp(pwszName, L"http://xml.org/sax/features/string-interning") == 0) {
	}
	else if (wcscmp(pwszName, L"http://xml.org/sax/features/use-attributes2") == 0) {
	}
	else if (wcscmp(pwszName, L"http://xml.org/sax/features/use-locator2") == 0) {
	}
	else if (wcscmp(pwszName, L"http://xml.org/sax/features/use-entity-resolver2") == 0) {
	}
	else if (wcscmp(pwszName, L"http://xml.org/sax/features/validation") == 0) {
	}
	else {
	}
}

void* qs::XMLReader::getProperty(const WCHAR* pwszName) const
{
	if (wcscmp(pwszName, L"http://xml.org/sax/properties/declaration-handler") == 0) {
		return pImpl_->pDeclHandler_;
	}
	else if (wcscmp(pwszName, L"http://xml.org/sax/properties/dom-node") == 0) {
		return 0;
	}
	else if (wcscmp(pwszName, L"http://xml.org/sax/properties/lexical-handler") == 0) {
		return pImpl_->pLexicalHandler_;
	}
	else if (wcscmp(pwszName, L"http://xml.org/sax/properties/xml-string") == 0) {
		return 0;
	}
	else {
		return 0;
	}
}

void qs::XMLReader::setProperty(const WCHAR* pwszName,
								void* pValue)
{
	if (wcscmp(pwszName, L"http://xml.org/sax/properties/declaration-handler") == 0) {
		pImpl_->pDeclHandler_ = static_cast<DeclHandler*>(pValue);
	}
	else if (wcscmp(pwszName, L"http://xml.org/sax/properties/dom-node") == 0) {
	}
	else if (wcscmp(pwszName, L"http://xml.org/sax/properties/lexical-handler") == 0) {
		pImpl_->pLexicalHandler_ = static_cast<LexicalHandler*>(pValue);
	}
	else if (wcscmp(pwszName, L"http://xml.org/sax/properties/xml-string") == 0) {
	}
	else {
	}
}

EntityResolver* qs::XMLReader::getEntityResolver() const
{
	return pImpl_->pEntityResolver_;
}

void qs::XMLReader::setEntityResolver(EntityResolver* pEntityResolver)
{
	pImpl_->pEntityResolver_ = pEntityResolver;
}

DTDHandler* qs::XMLReader::getDTDHandler() const
{
	return pImpl_->pDTDHandler_;
}

void qs::XMLReader::setDTDHandler(DTDHandler* pDTDHandler)
{
	pImpl_->pDTDHandler_ = pDTDHandler;
}

ContentHandler* qs::XMLReader::getContentHandler() const
{
	return pImpl_->pContentHandler_;
}

void qs::XMLReader::setContentHandler(ContentHandler* pContentHandler)
{
	pImpl_->pContentHandler_ = pContentHandler;
}

ErrorHandler* qs::XMLReader::getErrorHandler() const
{
	return pImpl_->pErrorHandler_;
}

void qs::XMLReader::setErrorHandler(ErrorHandler* pErrorHandler)
{
	pImpl_->pErrorHandler_ = pErrorHandler;
}

bool qs::XMLReader::parse(InputSource* pInputSource)
{
	assert(pInputSource);
	
	XMLParser parser(pImpl_->pContentHandler_, pImpl_->nFlags_);
	return parser.parse(*pInputSource);
}

bool qs::XMLReader::parse(const WCHAR* pwszSystemId)
{
	FileInputStream stream(pwszSystemId);
	if (!stream)
		return false;
	BufferedInputStream bufferedStream(&stream, false);
	InputSource source(&bufferedStream);
	source.setSystemId(pwszSystemId);
	return parse(&source);
}


/****************************************************************************
 *
 * ContentHandler
 *
 */

qs::ContentHandler::~ContentHandler()
{
}


/****************************************************************************
 *
 * DTDHandler
 *
 */

qs::DTDHandler::~DTDHandler()
{
}


/****************************************************************************
 *
 * DeclHandler
 *
 */

qs::DeclHandler::~DeclHandler()
{
}


/****************************************************************************
 *
 * LexicalHandler
 *
 */

qs::LexicalHandler::~LexicalHandler()
{
}


/****************************************************************************
 *
 * ErrorHandler
 *
 */

qs::ErrorHandler::~ErrorHandler()
{
}


/****************************************************************************
 *
 * EntityResolver
 *
 */

qs::EntityResolver::~EntityResolver()
{
}


/****************************************************************************
 *
 * EntityResolver2
 *
 */

qs::EntityResolver2::~EntityResolver2()
{
}


/****************************************************************************
 *
 * DefaultHandler
 *
 */

qs::DefaultHandler::DefaultHandler()
{
}

qs::DefaultHandler::~DefaultHandler()
{
}

bool qs::DefaultHandler::setDocumentLocator(const Locator& locator)
{
	return true;
}

bool qs::DefaultHandler::startDocument()
{
	return true;
}

bool qs::DefaultHandler::endDocument()
{
	return true;
}

bool qs::DefaultHandler::startPrefixMapping(const WCHAR* pwszPrefix,
											const WCHAR* pwszURI)
{
	return true;
}

bool qs::DefaultHandler::endPrefixMapping(const WCHAR* pwszPrefix)
{
	return true;
}

bool qs::DefaultHandler::startElement(const WCHAR* pwszNamespaceURI,
									  const WCHAR* pwszLocalName,
									  const WCHAR* pwszQName,
									  const Attributes& attributes)
{
	return true;
}

bool qs::DefaultHandler::endElement(const WCHAR* pwszNamespaceURI,
									const WCHAR* pwszLocalName,
									const WCHAR* pwszQName)
{
	return true;
}

bool qs::DefaultHandler::characters(const WCHAR* pwsz,
									size_t nStart,
									size_t nLength)
{
	return true;
}

bool qs::DefaultHandler::ignorableWhitespace(const WCHAR* pwsz,
											 size_t nStart,
											 size_t nLength)
{
	return true;
}

bool qs::DefaultHandler::processingInstruction(const WCHAR* pwszTarget,
											   const WCHAR* pwszData)
{
	return true;
}

bool qs::DefaultHandler::skippedEntity(const WCHAR* pwszName)
{
	return true;
}

bool qs::DefaultHandler::notationDecl(const WCHAR* pwszName,
									  const WCHAR* pwszPublicId,
									  const WCHAR* pwszSystemId)
{
	return true;
}

bool qs::DefaultHandler::unparsedEntityDecl(const WCHAR* pwszName,
											const WCHAR* pwszPublicId,
											const WCHAR* pwszSystemId,
											const WCHAR* pwszNotationName)
{
	return true;
}

std::auto_ptr<InputSource> qs::DefaultHandler::resolveEntity(const WCHAR* pwszPublicId,
															 const WCHAR* pwszSystemId)
{
	return std::auto_ptr<InputSource>(0);
}


/****************************************************************************
 *
 * DefaultHandler2
 *
 */

qs::DefaultHandler2::DefaultHandler2()
{
}

qs::DefaultHandler2::~DefaultHandler2()
{
}

bool qs::DefaultHandler2::elementDecl(const WCHAR* pwszName,
									  const WCHAR* pwszModel)
{
	return true;
}

bool qs::DefaultHandler2::attributeDecl(const WCHAR* pwszElementName,
										const WCHAR* pwszAttributeName,
										const WCHAR* pwszType,
										const WCHAR* pwszMode,
										const WCHAR* pwszValue)
{
	return true;
}

bool qs::DefaultHandler2::internalEntityDecl(const WCHAR* pwszName,
											 const WCHAR* pwszValue)
{
	return true;
}

bool qs::DefaultHandler2::externalEntityDecl(const WCHAR* pwszName,
											 const WCHAR* pwszPublicId,
											 const WCHAR* pwszSystemId)
{
	return true;
}

bool qs::DefaultHandler2::startDTD(const WCHAR* pwszName,
								   const WCHAR* pwszPublicId,
								   const WCHAR* pwszSystemId)
{
	return true;
}

bool qs::DefaultHandler2::endDTD()
{
	return true;
}

bool qs::DefaultHandler2::startEntity(const WCHAR* pwszName)
{
	return true;
}

bool qs::DefaultHandler2::endEntity(const WCHAR* pwszName)
{
	return true;
}

bool qs::DefaultHandler2::startCDATA()
{
	return true;
}

bool qs::DefaultHandler2::endCDATA()
{
	return true;
}

bool qs::DefaultHandler2::comment(const WCHAR* pwsz,
								  size_t nStart,
								  size_t nLength)
{
	return true;
}

std::auto_ptr<InputSource> qs::DefaultHandler2::resolveEntity(const WCHAR* pwszPublicId,
															  const WCHAR* pwszSystemId)
{
	return std::auto_ptr<InputSource>(0);
}

std::auto_ptr<InputSource> qs::DefaultHandler2::getExternalSubset(const WCHAR* pwszName,
																  const WCHAR* pwzzBaseURI)
{
	return std::auto_ptr<InputSource>(0);
}

std::auto_ptr<InputSource> qs::DefaultHandler2::resolveEntity(const WCHAR* pwszName,
															  const WCHAR* pwszPublicId,
															  const WCHAR* pwszBaseURI,
															  const WCHAR* pwszSystemId)
{
	return std::auto_ptr<InputSource>(0);
}


/****************************************************************************
 *
 * Attributes
 *
 */

qs::Attributes::~Attributes()
{
}


/****************************************************************************
 *
 * Attributes2
 *
 */

qs::Attributes2::~Attributes2()
{
}


/****************************************************************************
 *
 * AttributesImpl
 *
 */

qs::AttributesImpl::AttributesImpl(const AttributeList& l) :
	listAttribute_(l)
{
}

qs::AttributesImpl::~AttributesImpl()
{
}

int qs::AttributesImpl::getLength() const
{
	return listAttribute_.size();
}

const WCHAR* qs::AttributesImpl::getURI(int nIndex) const
{
	if (nIndex < 0 || static_cast<int>(listAttribute_.size()) <= nIndex)
		return 0;
	return listAttribute_[nIndex].pwszNamespaceURI_;
}

const WCHAR* qs::AttributesImpl::getLocalName(int nIndex) const
{
	if (nIndex < 0 || static_cast<int>(listAttribute_.size()) <= nIndex)
		return 0;
	return listAttribute_[nIndex].pwszLocalName_;
}

const WCHAR* qs::AttributesImpl::getQName(int nIndex) const
{
	if (nIndex < 0 || static_cast<int>(listAttribute_.size()) <= nIndex)
		return 0;
	return listAttribute_[nIndex].wstrQName_;
}

const WCHAR* qs::AttributesImpl::getType(int nIndex) const
{
	if (nIndex < 0 || static_cast<int>(listAttribute_.size()) <= nIndex)
		return 0;
	return L"CDATA";
}

const WCHAR* qs::AttributesImpl::getValue(int nIndex) const
{
	if (nIndex < 0 || static_cast<int>(listAttribute_.size()) <= nIndex)
		return 0;
	return listAttribute_[nIndex].wstrValue_;
}

int qs::AttributesImpl::getIndex(const WCHAR* pwszURI,
								 const WCHAR* pwszLocalName) const
{
	assert(pwszLocalName);
	
	AttributeList::size_type n = 0;
	for (n = 0; n < listAttribute_.size(); ++n) {
		const Attribute& attr = listAttribute_[n];
		if ((pwszURI == attr.pwszNamespaceURI_ ||
			wcscmp(pwszURI, attr.pwszNamespaceURI_) == 0) &&
			wcscmp(pwszLocalName, attr.pwszLocalName_) == 0)
			break;
	}
	
	return n == listAttribute_.size() ? -1 : static_cast<int>(n);
}

int qs::AttributesImpl::getIndex(const WCHAR* pwszQName) const
{
	assert(pwszQName);
	
	AttributeList::size_type n = 0;
	for (n = 0; n < listAttribute_.size(); ++n) {
		const Attribute& attr = listAttribute_[n];
		if (wcscmp(pwszQName, attr.wstrQName_) == 0)
			break;
	}
	
	return n == listAttribute_.size() ? -1 : static_cast<int>(n);
}

const WCHAR* qs::AttributesImpl::getType(const WCHAR* pwszURI,
										 const WCHAR* pwszLocalName) const
{
	if (getIndex(pwszURI, pwszLocalName) == -1)
		return 0;
	return L"CDATA";
}

const WCHAR* qs::AttributesImpl::getType(const WCHAR* pwszQName) const
{
	if (getIndex(pwszQName) == -1)
		return 0;
	return L"CDATA";
}

const WCHAR* qs::AttributesImpl::getValue(const WCHAR* pwszURI,
										  const WCHAR* pwszLocalName) const
{
	return getValue(getIndex(pwszURI, pwszLocalName));
}

const WCHAR* qs::AttributesImpl::getValue(const WCHAR* pwszQName) const
{
	return getValue(getIndex(pwszQName));
}

bool qs::AttributesImpl::isDeclared(int index) const
{
	return false;
}

bool qs::AttributesImpl::isDeclared(const WCHAR* pwszQName) const
{
	return isDeclared(getIndex(pwszQName));
}

bool qs::AttributesImpl::isDeclared(const WCHAR* pwszURI,
									const WCHAR* pwszLocalName) const
{
	return isDeclared(getIndex(pwszURI, pwszLocalName));
}

bool qs::AttributesImpl::isSpecified(int index) const
{
	return true;
}

bool qs::AttributesImpl::isSpecified(const WCHAR* pwszQName) const
{
	return isSpecified(getIndex(pwszQName));
}

bool qs::AttributesImpl::isSpecified(const WCHAR* pwszURI,
									 const WCHAR* pwszLocalName) const
{
	return isSpecified(getIndex(pwszURI, pwszLocalName));
}


/****************************************************************************
 *
 * Locator
 *
 */

qs::Locator::~Locator()
{
}


/****************************************************************************
 *
 * Locator2
 *
 */

qs::Locator2::~Locator2()
{
}


/****************************************************************************
 *
 * InputSourceImpl
 *
 */

struct qs::InputSourceImpl
{
	wstring_ptr wstrPublicId_;
	wstring_ptr wstrSystemId_;
	wstring_ptr wstrEncoding_;
	InputStream* pInputStream_;
	Reader* pReader_;
};


/****************************************************************************
 *
 * InputSource
 *
 */

qs::InputSource::InputSource() :
	pImpl_(0)
{
	pImpl_ = new InputSourceImpl();
	pImpl_->pInputStream_ = 0;
	pImpl_->pReader_ = 0;
}

qs::InputSource::InputSource(const WCHAR* pwszSystemId) :
	pImpl_(0)
{
	pImpl_ = new InputSourceImpl();
	pImpl_->pInputStream_ = 0;
	pImpl_->pReader_ = 0;
	
	setSystemId(pwszSystemId);
}

qs::InputSource::InputSource(InputStream* pInputStream) :
	pImpl_(0)
{
	pImpl_ = new InputSourceImpl();
	pImpl_->pInputStream_ = 0;
	pImpl_->pReader_ = 0;
	
	setByteStream(pInputStream);
}

qs::InputSource::InputSource(Reader* pReader) :
	pImpl_(0)
{
	pImpl_ = new InputSourceImpl();
	pImpl_->pInputStream_ = 0;
	pImpl_->pReader_ = 0;
	
	setCharacterStream(pReader);
}

qs::InputSource::~InputSource()
{
	delete pImpl_;
}

void qs::InputSource::setPublicId(const WCHAR* pwszPublicId)
{
	pImpl_->wstrPublicId_ = allocWString(pwszPublicId);
}

const WCHAR* qs::InputSource::getPublicId() const
{
	return pImpl_->wstrPublicId_.get();
}

void qs::InputSource::setSystemId(const WCHAR* pwszSystemId)
{
	pImpl_->wstrSystemId_ = allocWString(pwszSystemId);
}

const WCHAR* qs::InputSource::getSystemId() const
{
	return pImpl_->wstrSystemId_.get();
}

void qs::InputSource::setByteStream(InputStream* pInputStream)
{
	pImpl_->pInputStream_ = pInputStream;
}

InputStream* qs::InputSource::getByteStream() const
{
	return pImpl_->pInputStream_;
}

void qs::InputSource::setEncoding(const WCHAR* pwszEncoding)
{
	pImpl_->wstrEncoding_ = allocWString(pwszEncoding);
}

const WCHAR* qs::InputSource::getEncoding() const
{
	return pImpl_->wstrEncoding_.get();
}

void qs::InputSource::setCharacterStream(Reader* pReader)
{
	pImpl_->pReader_ = pReader;
}

Reader* qs::InputSource::getCharacterStream() const
{
	return pImpl_->pReader_;
}


/****************************************************************************
 *
 * OutputHandlerImpl
 *
 */

struct qs::OutputHandlerImpl
{
	enum State {
		STATE_ROOT,
		STATE_STARTELEMENT,
		STATE_ENDELEMENT,
		STATE_CHARACTERS
	};
	
	bool indent();
	
	Writer* pWriter_;
	State state_;
	unsigned int nIndent_;
};

bool qs::OutputHandlerImpl::indent()
{
	if (pWriter_->write(L"\n", 1) == -1)
		return false;
	for (unsigned int n = 0; n < nIndent_; ++n) {
		if (pWriter_->write(L"\t", 1) == -1)
			return false;
	}
	return true;
}


/****************************************************************************
 *
 * OutputHandler
 *
 */

qs::OutputHandler::OutputHandler(Writer* pWriter) :
	pImpl_(0)
{
	pImpl_ = new OutputHandlerImpl();
	pImpl_->pWriter_ = pWriter;
	pImpl_->state_ = OutputHandlerImpl::STATE_ROOT;
	pImpl_->nIndent_ = 0;
}

qs::OutputHandler::~OutputHandler()
{
	delete pImpl_;
}

bool qs::OutputHandler::startElement(const WCHAR* pwszNamespaceURI,
									 const WCHAR* pwszLocalName,
									 const WCHAR* pwszQName,
									 const Attributes& attributes)
{
	switch (pImpl_->state_) {
	case OutputHandlerImpl::STATE_ROOT:
	case OutputHandlerImpl::STATE_CHARACTERS:
		break;
	case OutputHandlerImpl::STATE_STARTELEMENT:
	case OutputHandlerImpl::STATE_ENDELEMENT:
		pImpl_->indent();
		break;
	default:
		assert(false);
		break;
	}
	
	if (pImpl_->pWriter_->write(L"<", 1) == -1)
		return false;
	if (pImpl_->pWriter_->write(pwszQName, wcslen(pwszQName)) == -1)
		return false;
	for (int n = 0; n < attributes.getLength(); ++n) {
		if (pImpl_->pWriter_->write(L" ", 1) == -1)
			return false;
		const WCHAR* pwszQName = attributes.getQName(n);
		if (pImpl_->pWriter_->write(pwszQName, wcslen(pwszQName)) == -1)
			return false;
		if (pImpl_->pWriter_->write(L"=\"", 2) == -1)
			return false;
		const WCHAR* p = attributes.getValue(n);
		while (*p) {
			if (*p == L'\"') {
				if (pImpl_->pWriter_->write(L"&quot;", 6) == -1)
					return false;
			}
			else if (*p == L'<') {
				if (pImpl_->pWriter_->write(L"&lt;", 4) == -1)
					return false;
			}
			else if (*p == L'&') {
				if (pImpl_->pWriter_->write(L"&amp;", 5) == -1)
					return false;
			}
			else {
				if (pImpl_->pWriter_->write(p, 1) == -1)
					return false;
			}
			++p;
		}
		if (pImpl_->pWriter_->write(L"\"", 1) == -1)
			return false;
	}
	if (pImpl_->pWriter_->write(L">", 1) == -1)
		return false;
	
	pImpl_->state_ = OutputHandlerImpl::STATE_STARTELEMENT;
	++pImpl_->nIndent_;
	
	return true;
}

bool qs::OutputHandler::endElement(const WCHAR* pwszNamespaceURI,
								   const WCHAR* pwszLocalName,
								   const WCHAR* pwszQName)
{
	--pImpl_->nIndent_;
	switch (pImpl_->state_) {
	case OutputHandlerImpl::STATE_ROOT:
	case OutputHandlerImpl::STATE_STARTELEMENT:
	case OutputHandlerImpl::STATE_CHARACTERS:
		break;
	case OutputHandlerImpl::STATE_ENDELEMENT:
		pImpl_->indent();
		break;
	default:
		assert(false);
		break;
	}
	
	if (pImpl_->pWriter_->write(L"</", 2) == -1)
		return false;
	if (pImpl_->pWriter_->write(pwszQName, wcslen(pwszQName)) == -1)
		return false;
	if (pImpl_->pWriter_->write(L">", 1) == -1)
		return false;
	
	pImpl_->state_ = OutputHandlerImpl::STATE_ENDELEMENT;
	
	return true;
}

bool qs::OutputHandler::characters(const WCHAR* pwsz,
								   size_t nStart,
								   size_t nLength)
{
	const WCHAR* p = pwsz + nStart;
	for (size_t n = 0; n < nLength; ++n, ++p) {
		if (*p == L'<') {
			if (pImpl_->pWriter_->write(L"&lt;", 4) == -1)
				return false;
		}
		else if (*p == L'&') {
			if (pImpl_->pWriter_->write(L"&amp;", 5) == -1)
				return false;
		}
		else {
			if (pImpl_->pWriter_->write(p, 1) == -1)
				return false;
		}
	}
	
	pImpl_->state_ = OutputHandlerImpl::STATE_CHARACTERS;
	
	return true;
}


/****************************************************************************
 *
 * DefaultAttributes
 *
 */

qs::DefaultAttributes::DefaultAttributes()
{
}

qs::DefaultAttributes::~DefaultAttributes()
{
}

int qs::DefaultAttributes::getLength() const
{
	return 0;
}

const WCHAR* qs::DefaultAttributes::getURI(int nIndex) const
{
	assert(false);
	return 0;
}

const WCHAR* qs::DefaultAttributes::getLocalName(int nIndex) const
{
	assert(false);
	return 0;
}

const WCHAR* qs::DefaultAttributes::getQName(int nIndex) const
{
	assert(false);
	return 0;
}

const WCHAR* qs::DefaultAttributes::getType(int nIndex) const
{
	assert(false);
	return 0;
}

const WCHAR* qs::DefaultAttributes::getValue(int nIndex) const
{
	assert(false);
	return 0;
}

int qs::DefaultAttributes::getIndex(const WCHAR* pwszURI,
									const WCHAR* pwszLocalName) const
{
	assert(false);
	return -1;
}

int qs::DefaultAttributes::getIndex(const WCHAR* pwszQName) const
{
	assert(false);
	return -1;
}

const WCHAR* qs::DefaultAttributes::getType(const WCHAR* pwszURI,
											const WCHAR* pwszLocalName) const
{
	assert(false);
	return 0;
}

const WCHAR* qs::DefaultAttributes::getType(const WCHAR* pwszQName) const
{
	assert(false);
	return 0;
}

const WCHAR* qs::DefaultAttributes::getValue(const WCHAR* pwszURI,
											 const WCHAR* pwszLocalName) const
{
	assert(false);
	return 0;
}

const WCHAR* qs::DefaultAttributes::getValue(const WCHAR* pwszQName) const
{
	assert(false);
	return 0;
}

bool qs::DefaultAttributes::isDeclared(int index) const
{
	assert(false);
	return false;
}

bool qs::DefaultAttributes::isDeclared(const WCHAR* pwszQName) const
{
	assert(false);
	return false;
}

bool qs::DefaultAttributes::isDeclared(const WCHAR* pwszURI,
									   const WCHAR* pwszLocalName) const
{
	assert(false);
	return false;
}

bool qs::DefaultAttributes::isSpecified(int index) const
{
	assert(false);
	return false;
}

bool qs::DefaultAttributes::isSpecified(const WCHAR* pwszQName) const
{
	assert(false);
	return false;
}

bool qs::DefaultAttributes::isSpecified(const WCHAR* pwszURI,
										const WCHAR* pwszLocalName) const
{
	assert(false);
	return false;
}


/****************************************************************************
 *
 * SimpleAttribute
 *
 */

qs::SimpleAttributes::SimpleAttributes(const WCHAR* pwszQName,
									   const WCHAR* pwszValue) :
	pItems_(&item_),
	nSize_(1),
	nCount_(1)
{
	item_.pwszQName_ = pwszQName;
	item_.pwszValue_ = pwszValue;
	item_.bOmit_ = false;
}

qs::SimpleAttributes::SimpleAttributes(const Item* pItems,
									   size_t nSize) :
	pItems_(pItems),
	nSize_(nSize),
	nCount_(0)
{
	for (size_t n = 0; n < nSize; ++n) {
		if (!(pItems_ + n)->bOmit_)
			++nCount_;
	}
}

qs::SimpleAttributes::~SimpleAttributes()
{
}

int qs::SimpleAttributes::getLength() const
{
	return nCount_;
}

const WCHAR* qs::SimpleAttributes::getQName(int nIndex) const
{
	assert(0 <= nIndex && size_t(nIndex) < nSize_);
	for (size_t n = 0; n < nSize_; ++n) {
		if (!(pItems_ + n)->bOmit_) {
			if (nIndex-- == 0)
				return (pItems_ + n)->pwszQName_;
		}
	}
	return 0;
}

const WCHAR* qs::SimpleAttributes::getValue(int nIndex) const
{
	assert(0 <= nIndex && size_t(nIndex) < nSize_);
	for (size_t n = 0; n < nSize_; ++n) {
		if (!(pItems_ + n)->bOmit_) {
			if (nIndex-- == 0)
				return (pItems_ + n)->pwszValue_;
		}
	}
	return 0;
}


/****************************************************************************
 *
 * HandlerHelper
 *
 */

bool qs::HandlerHelper::textElement(ContentHandler* pHandler,
									const WCHAR* pwszQName,
									const WCHAR* pwszValue,
									size_t nLen)
{
	if (nLen == -1)
		nLen = wcslen(pwszValue);
	return pHandler->startElement(0, 0, pwszQName, DefaultAttributes()) &&
		pHandler->characters(pwszValue, 0, nLen) &&
		pHandler->endElement(0, 0, pwszQName);
}

bool qs::HandlerHelper::numberElement(ContentHandler* pHandler,
									  const WCHAR* pwszQName,
									  unsigned int nValue)
{
	WCHAR wsz[32];
	swprintf(wsz, L"%u", nValue);
	return textElement(pHandler, pwszQName, wsz, -1);
}
