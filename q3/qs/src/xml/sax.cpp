/*
 * $Id: sax.cpp,v 1.1.1.1 2003/04/29 08:07:37 snakamura Exp $
 *
 * Copyright(C) 1998-2003 Satoshi Nakamura
 * All rights reserved.
 *
 */

#include <qserror.h>
#include <qsnew.h>
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

qs::XMLReader::XMLReader(QSTATUS* pstatus) :
	pImpl_(0)
{
	DECLARE_QSTATUS();
	
	status = newObject(&pImpl_);
	CHECK_QSTATUS_SET(pstatus);
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

QSTATUS qs::XMLReader::getFeature(const WCHAR* pwszName, bool* pbValue) const
{
	assert(pbValue);
	
	// TODO
	*pbValue = true;
	
	if (wcscmp(pwszName, L"http://xml.org/sax/features/external-general-entities") == 0) {
		*pbValue = false;
	}
	else if (wcscmp(pwszName, L"http://xml.org/sax/features/external-parameter-entities") == 0) {
		*pbValue = false;
	}
	else if (wcscmp(pwszName, L"http://xml.org/sax/features/is-standalone") == 0) {
	}
	else if (wcscmp(pwszName, L"http://xml.org/sax/features/lexical-handler/parameter-entities") == 0) {
		*pbValue = false;
	}
	else if (wcscmp(pwszName, L"http://xml.org/sax/features/namespaces") == 0) {
		*pbValue = (pImpl_->nFlags_ & XMLParser::FLAG_NAMESPACES) != 0;
	}
	else if (wcscmp(pwszName, L"http://xml.org/sax/features/namespace-prefixes") == 0) {
		*pbValue = (pImpl_->nFlags_ & XMLParser::FLAG_NAMESPACEPREFIXES) != 0;
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
	
	return QSTATUS_SUCCESS;
}

QSTATUS qs::XMLReader::setFeature(const WCHAR* pwszName, bool bValue)
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
	return QSTATUS_SUCCESS;
}

QSTATUS qs::XMLReader::getProperty(const WCHAR* pwszName, void** ppValue) const
{
	assert(ppValue);
	
	*ppValue = 0;
	
	if (wcscmp(pwszName, L"http://xml.org/sax/properties/declaration-handler") == 0) {
		*ppValue = pImpl_->pDeclHandler_;
	}
	else if (wcscmp(pwszName, L"http://xml.org/sax/properties/dom-node") == 0) {
		return QSTATUS_FAIL;
	}
	else if (wcscmp(pwszName, L"http://xml.org/sax/properties/lexical-handler") == 0) {
		*ppValue = pImpl_->pLexicalHandler_;
	}
	else if (wcscmp(pwszName, L"http://xml.org/sax/properties/xml-string") == 0) {
		return QSTATUS_FAIL;
	}
	else {
		return QSTATUS_FAIL;
	}
	
	return QSTATUS_SUCCESS;
}

QSTATUS qs::XMLReader::setProperty(const WCHAR* pwszName, void* pValue)
{
	if (wcscmp(pwszName, L"http://xml.org/sax/properties/declaration-handler") == 0) {
		pImpl_->pDeclHandler_ = static_cast<DeclHandler*>(pValue);
	}
	else if (wcscmp(pwszName, L"http://xml.org/sax/properties/dom-node") == 0) {
		return QSTATUS_FAIL;
	}
	else if (wcscmp(pwszName, L"http://xml.org/sax/properties/lexical-handler") == 0) {
		pImpl_->pLexicalHandler_ = static_cast<LexicalHandler*>(pValue);
	}
	else if (wcscmp(pwszName, L"http://xml.org/sax/properties/xml-string") == 0) {
		return QSTATUS_FAIL;
	}
	else {
		return QSTATUS_FAIL;
	}
	
	return QSTATUS_SUCCESS;
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

QSTATUS qs::XMLReader::parse(InputSource* pInputSource)
{
	assert(pInputSource);
	
	DECLARE_QSTATUS();
	
	XMLParser parser(pImpl_->pContentHandler_, pImpl_->nFlags_, &status);
	CHECK_QSTATUS();
	
	return parser.parse(*pInputSource);
}

QSTATUS qs::XMLReader::parse(const WCHAR* pwszSystemId)
{
	DECLARE_QSTATUS();
	
	FileInputStream stream(pwszSystemId, &status);
	CHECK_QSTATUS();
	BufferedInputStream bufferedStream(&stream, false, &status);
	CHECK_QSTATUS();
	InputSource source(&bufferedStream, &status);
	CHECK_QSTATUS();
	status = source.setSystemId(pwszSystemId);
	CHECK_QSTATUS();
	
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

qs::DefaultHandler::DefaultHandler(QSTATUS* pstatus)
{
	assert(pstatus);
	*pstatus = QSTATUS_SUCCESS;
}

qs::DefaultHandler::~DefaultHandler()
{
}

QSTATUS qs::DefaultHandler::setDocumentLocator(const Locator& locator)
{
	return QSTATUS_SUCCESS;
}

QSTATUS qs::DefaultHandler::startDocument()
{
	return QSTATUS_SUCCESS;
}

QSTATUS qs::DefaultHandler::endDocument()
{
	return QSTATUS_SUCCESS;
}

QSTATUS qs::DefaultHandler::startPrefixMapping(
	const WCHAR* pwszPrefix, const WCHAR* pwszURI)
{
	return QSTATUS_SUCCESS;
}

QSTATUS qs::DefaultHandler::endPrefixMapping(const WCHAR* pwszPrefix)
{
	return QSTATUS_SUCCESS;
}

QSTATUS qs::DefaultHandler::startElement(const WCHAR* pwszNamespaceURI,
	const WCHAR* pwszLocalName, const WCHAR* pwszQName,
	const Attributes& attributes)
{
	return QSTATUS_SUCCESS;
}

QSTATUS qs::DefaultHandler::endElement(const WCHAR* pwszNamespaceURI,
	const WCHAR* pwszLocalName, const WCHAR* pwszQName)
{
	return QSTATUS_SUCCESS;
}

QSTATUS qs::DefaultHandler::characters(
	const WCHAR* pwsz, size_t nStart, size_t nLength)
{
	return QSTATUS_SUCCESS;
}

QSTATUS qs::DefaultHandler::ignorableWhitespace(
	const WCHAR* pwsz, size_t nStart, size_t nLength)
{
	return QSTATUS_SUCCESS;
}

QSTATUS qs::DefaultHandler::processingInstruction(
	const WCHAR* pwszTarget, const WCHAR* pwszData)
{
	return QSTATUS_SUCCESS;
}

QSTATUS qs::DefaultHandler::skippedEntity(const WCHAR* pwszName)
{
	return QSTATUS_SUCCESS;
}

QSTATUS qs::DefaultHandler::notationDecl(const WCHAR* pwszName,
	const WCHAR* pwszPublicId, const WCHAR* pwszSystemId)
{
	return QSTATUS_SUCCESS;
}

QSTATUS qs::DefaultHandler::unparsedEntityDecl(const WCHAR* pwszName,
	const WCHAR* pwszPublicId, const WCHAR* pwszSystemId,
	const WCHAR* pwszNotationName)
{
	return QSTATUS_SUCCESS;
}

QSTATUS qs::DefaultHandler::resolveEntity(const WCHAR* pwszPublicId,
	const WCHAR* pwszSystemId, InputSource** ppInputSource)
{
	return QSTATUS_SUCCESS;
}


/****************************************************************************
 *
 * DefaultHandler2
 *
 */

qs::DefaultHandler2::DefaultHandler2(QSTATUS* pstatus) :
	DefaultHandler(pstatus)
{
}

qs::DefaultHandler2::~DefaultHandler2()
{
}

QSTATUS qs::DefaultHandler2::elementDecl(
	const WCHAR* pwszName, const WCHAR* pwszModel)
{
	return QSTATUS_SUCCESS;
}

QSTATUS qs::DefaultHandler2::attributeDecl(const WCHAR* pwszElementName,
	const WCHAR* pwszAttributeName, const WCHAR* pwszType,
	const WCHAR* pwszMode, const WCHAR* pwszValue)
{
	return QSTATUS_SUCCESS;
}

QSTATUS qs::DefaultHandler2::internalEntityDecl(
	const WCHAR* pwszName, const WCHAR* pwszValue)
{
	return QSTATUS_SUCCESS;
}

QSTATUS qs::DefaultHandler2::externalEntityDecl(const WCHAR* pwszName,
	const WCHAR* pwszPublicId, const WCHAR* pwszSystemId)
{
	return QSTATUS_SUCCESS;
}

QSTATUS qs::DefaultHandler2::startDTD(const WCHAR* pwszName,
	const WCHAR* pwszPublicId, const WCHAR* pwszSystemId)
{
	return QSTATUS_SUCCESS;
}

QSTATUS qs::DefaultHandler2::endDTD()
{
	return QSTATUS_SUCCESS;
}

QSTATUS qs::DefaultHandler2::startEntity(const WCHAR* pwszName)
{
	return QSTATUS_SUCCESS;
}

QSTATUS qs::DefaultHandler2::endEntity(const WCHAR* pwszName)
{
	return QSTATUS_SUCCESS;
}

QSTATUS qs::DefaultHandler2::startCDATA()
{
	return QSTATUS_SUCCESS;
}

QSTATUS qs::DefaultHandler2::endCDATA()
{
	return QSTATUS_SUCCESS;
}

QSTATUS qs::DefaultHandler2::comment(
	const WCHAR* pwsz, size_t nStart, size_t nLength)
{
	return QSTATUS_SUCCESS;
}

QSTATUS qs::DefaultHandler2::resolveEntity(const WCHAR* pwszPublicId,
	const WCHAR* pwszSystemId, InputSource** ppInputSource)
{
	return QSTATUS_SUCCESS;
}

QSTATUS qs::DefaultHandler2::getExternalSubset(const WCHAR* pwszName,
	const WCHAR* pwzzBaseURI, InputSource** ppInputSource)
{
	return QSTATUS_SUCCESS;
}

QSTATUS qs::DefaultHandler2::resolveEntity(const WCHAR* pwszName,
	const WCHAR* pwszPublicId, const WCHAR* pwszBaseURI,
	const WCHAR* pwszSystemId, InputSource** ppInputSource)
{
	return QSTATUS_SUCCESS;
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
	InputSourceImpl();
	~InputSourceImpl();
	
	WSTRING wstrPublicId_;
	WSTRING wstrSystemId_;
	WSTRING wstrEncoding_;
	InputStream* pInputStream_;
	Reader* pReader_;
};

qs::InputSourceImpl::InputSourceImpl() :
	wstrPublicId_(0),
	wstrSystemId_(0),
	wstrEncoding_(0),
	pInputStream_(0),
	pReader_(0)
{
}

qs::InputSourceImpl::~InputSourceImpl()
{
	freeWString(wstrPublicId_);
	freeWString(wstrSystemId_);
	freeWString(wstrEncoding_);
}


/****************************************************************************
 *
 * InputSource
 *
 */

qs::InputSource::InputSource(QSTATUS* pstatus) :
	pImpl_(0)
{
	assert(pstatus);
	
	DECLARE_QSTATUS();
	
	status = newObject(&pImpl_);
	CHECK_QSTATUS_SET(pstatus);
}

qs::InputSource::InputSource(const WCHAR* pwszSystemId, QSTATUS* pstatus) :
	pImpl_(0)
{
	assert(pstatus);
	
	DECLARE_QSTATUS();
	
	status = newObject(&pImpl_);
	CHECK_QSTATUS_SET(pstatus);
	status = setSystemId(pwszSystemId);
	CHECK_QSTATUS_SET(pstatus);
}

qs::InputSource::InputSource(InputStream* pInputStream, QSTATUS* pstatus) :
	pImpl_(0)
{
	assert(pstatus);
	
	DECLARE_QSTATUS();
	
	status = newObject(&pImpl_);
	CHECK_QSTATUS_SET(pstatus);
	setByteStream(pInputStream);
}

qs::InputSource::InputSource(Reader* pReader, QSTATUS* pstatus) :
	pImpl_(0)
{
	assert(pstatus);
	
	DECLARE_QSTATUS();
	
	status = newObject(&pImpl_);
	CHECK_QSTATUS_SET(pstatus);
	setCharacterStream(pReader);
}

qs::InputSource::~InputSource()
{
	delete pImpl_;
}

QSTATUS qs::InputSource::setPublicId(const WCHAR* pwszPublicId)
{
	string_ptr<WSTRING> wstrPublicId(allocWString(pwszPublicId));
	if (!wstrPublicId.get())
		return QSTATUS_OUTOFMEMORY;
	freeWString(pImpl_->wstrPublicId_);
	pImpl_->wstrPublicId_ = wstrPublicId.release();
	return QSTATUS_SUCCESS;
}

const WCHAR* qs::InputSource::getPublicId() const
{
	return pImpl_->wstrPublicId_;
}

QSTATUS qs::InputSource::setSystemId(const WCHAR* pwszSystemId)
{
	string_ptr<WSTRING> wstrSystemId(allocWString(pwszSystemId));
	if (!wstrSystemId.get())
		return QSTATUS_OUTOFMEMORY;
	freeWString(pImpl_->wstrSystemId_);
	pImpl_->wstrSystemId_ = wstrSystemId.release();
	return QSTATUS_SUCCESS;
}

const WCHAR* qs::InputSource::getSystemId() const
{
	return pImpl_->wstrSystemId_;
}

void qs::InputSource::setByteStream(InputStream* pInputStream)
{
	pImpl_->pInputStream_ = pInputStream;
}

InputStream* qs::InputSource::getByteStream() const
{
	return pImpl_->pInputStream_;
}

QSTATUS qs::InputSource::setEncoding(const WCHAR* pwszEncoding)
{
	string_ptr<WSTRING> wstrEncoding(allocWString(pwszEncoding));
	if (!wstrEncoding.get())
		return QSTATUS_OUTOFMEMORY;
	freeWString(pImpl_->wstrEncoding_);
	pImpl_->wstrEncoding_ = wstrEncoding.release();
	return QSTATUS_SUCCESS;
}

const WCHAR* qs::InputSource::getEncoding() const
{
	return pImpl_->wstrEncoding_;
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
	
	QSTATUS indent();
	
	Writer* pWriter_;
	State state_;
	unsigned int nIndent_;
};

QSTATUS qs::OutputHandlerImpl::indent()
{
	DECLARE_QSTATUS();
	
	status = pWriter_->write(L"\n", 1);
	CHECK_QSTATUS();
	for (unsigned int n = 0; n < nIndent_; ++n) {
		status = pWriter_->write(L"\t", 1);
		CHECK_QSTATUS();
	}
	
	return QSTATUS_SUCCESS;
}


/****************************************************************************
 *
 * OutputHandler
 *
 */

qs::OutputHandler::OutputHandler(Writer* pWriter, qs::QSTATUS* pstatus) :
	DefaultHandler2(pstatus),
	pImpl_(0)
{
	DECLARE_QSTATUS();
	
	status = newObject(&pImpl_);
	CHECK_QSTATUS_SET(pstatus);
	pImpl_->pWriter_ = pWriter;
	pImpl_->state_ = OutputHandlerImpl::STATE_ROOT;
	pImpl_->nIndent_ = 0;
}

qs::OutputHandler::~OutputHandler()
{
	delete pImpl_;
}

QSTATUS qs::OutputHandler::startElement(const WCHAR* pwszNamespaceURI,
	const WCHAR* pwszLocalName, const WCHAR* pwszQName,
	const Attributes& attributes)
{
	DECLARE_QSTATUS();
	
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
	
	status = pImpl_->pWriter_->write(L"<", 1);
	CHECK_QSTATUS();
	status = pImpl_->pWriter_->write(pwszQName, wcslen(pwszQName));
	CHECK_QSTATUS();
	for (int n = 0; n < attributes.getLength(); ++n) {
		status = pImpl_->pWriter_->write(L" ", 1);
		CHECK_QSTATUS();
		const WCHAR* pwszQName = attributes.getQName(n);
		status = pImpl_->pWriter_->write(pwszQName, wcslen(pwszQName));
		CHECK_QSTATUS();
		status = pImpl_->pWriter_->write(L"=\"", 2);
		CHECK_QSTATUS();
		const WCHAR* p = attributes.getValue(n);
		while (*p) {
			if (*p == L'\"') {
				status = pImpl_->pWriter_->write(L"&quot;", 6);
				CHECK_QSTATUS();
			}
			else if (*p == L'<') {
				status = pImpl_->pWriter_->write(L"&lt;", 4);
				CHECK_QSTATUS();
			}
			else if (*p == L'&') {
				status = pImpl_->pWriter_->write(L"&amp;", 5);
				CHECK_QSTATUS();
			}
			else {
				status = pImpl_->pWriter_->write(p, 1);
				CHECK_QSTATUS();
			}
			++p;
		}
		status = pImpl_->pWriter_->write(L"\"", 1);
		CHECK_QSTATUS();
	}
	status = pImpl_->pWriter_->write(L">", 1);
	CHECK_QSTATUS();
	
	pImpl_->state_ = OutputHandlerImpl::STATE_STARTELEMENT;
	++pImpl_->nIndent_;
	
	return QSTATUS_SUCCESS;
}

QSTATUS qs::OutputHandler::endElement(const WCHAR* pwszNamespaceURI,
	const WCHAR* pwszLocalName, const WCHAR* pwszQName)
{
	DECLARE_QSTATUS();
	
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
	
	status = pImpl_->pWriter_->write(L"</", 2);
	CHECK_QSTATUS();
	status = pImpl_->pWriter_->write(pwszQName, wcslen(pwszQName));
	CHECK_QSTATUS();
	status = pImpl_->pWriter_->write(L">", 1);
	CHECK_QSTATUS();
	
	pImpl_->state_ = OutputHandlerImpl::STATE_ENDELEMENT;
	
	return QSTATUS_SUCCESS;
}

QSTATUS qs::OutputHandler::characters(
	const WCHAR* pwsz, size_t nStart, size_t nLength)
{
	DECLARE_QSTATUS();
	
	const WCHAR* p = pwsz + nStart;
	for (size_t n = 0; n < nLength; ++n, ++p) {
		if (*p == L'<') {
			status = pImpl_->pWriter_->write(L"&lt;", 4);
			CHECK_QSTATUS();
		}
		else if (*p == L'&') {
			status = pImpl_->pWriter_->write(L"&amp;", 5);
			CHECK_QSTATUS();
		}
		else {
			status = pImpl_->pWriter_->write(p, 1);
			CHECK_QSTATUS();
		}
	}
	
	pImpl_->state_ = OutputHandlerImpl::STATE_CHARACTERS;
	
	return QSTATUS_SUCCESS;
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

int qs::DefaultAttributes::getIndex(
	const WCHAR* pwszURI, const WCHAR* pwszLocalName) const
{
	assert(false);
	return -1;
}

int qs::DefaultAttributes::getIndex(const WCHAR* pwszQName) const
{
	assert(false);
	return -1;
}

const WCHAR* qs::DefaultAttributes::getType(
	const WCHAR* pwszURI, const WCHAR* pwszLocalName) const
{
	assert(false);
	return 0;
}

const WCHAR* qs::DefaultAttributes::getType(const WCHAR* pwszQName) const
{
	assert(false);
	return 0;
}

const WCHAR* qs::DefaultAttributes::getValue(
	const WCHAR* pwszURI, const WCHAR* pwszLocalName) const
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

bool qs::DefaultAttributes::isDeclared(
	const WCHAR* pwszURI, const WCHAR* pwszLocalName) const
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

bool qs::DefaultAttributes::isSpecified(
	const WCHAR* pwszURI, const WCHAR* pwszLocalName) const
{
	assert(false);
	return false;
}
