/*
 * $Id$
 *
 * Copyright(C) 1998-2005 Satoshi Nakamura
 * All rights reserved.
 *
 */

#include <qsmime.h>
#include <qsconv.h>
#include <qsencoder.h>

#include <algorithm>
#include <memory>

#include "mime.h"

using namespace qs;


/****************************************************************************
 *
 * FieldComparator
 *
 */

bool FieldComparator::operator()(const std::pair<STRING, STRING>& lhs,
	const std::pair<STRING, STRING>& rhs) const
{
	const CHAR* pszFieldLhs = lhs.first;
	const CHAR* pszFieldRhs = rhs.first;
	
	const CHAR* pszFields[] = {
		"received",
		"to",
		"cc",
		"bcc",
		"from",
		"reply-to",
		"errors-to",
		"message-id",
		"in-reply-to",
		"references",
		"subject",
		"date",
		"resent-to",
		"resent-cc",
		"resent-bcc",
		"resent-from",
		"resent-message-id",
		"resent-date",
		"mime-version",
		"content-type",
		"content-transfer-encoding",
		"content-disposition",
		"content-description",
		"content-id"
	};
	
	for (int n = 0; n < countof(pszFields); ++n) {
		bool l = strcmp(pszFieldLhs, pszFields[n]) == 0;
		bool r = strcmp(pszFieldRhs, pszFields[n]) == 0;
		if (l)
			return !r;
		else if (r)
			return false;
	}
	
	bool l = strncmp(pszFieldLhs, "x-", 2) == 0;
	bool r = strncmp(pszFieldRhs, "x-", 2) == 0;
	if (l)
		return r ? strcmp(pszFieldLhs, pszFieldRhs) < 0 : false;
	else if (r)
		return true;
	
	return false;
}


/****************************************************************************
 *
 * Part
 *
 */

wstring_ptr qs::Part::wstrDefaultCharset__;
unsigned int qs::Part::nGlobalOptions__ = 0;
size_t qs::Part::nMaxHeaderLength__ = 32*1024;
size_t qs::Part::nMaxPartCount__ = 64;

qs::Part::Part() :
	pParent_(0),
	nOptions_(nGlobalOptions__)
{
}

qs::Part::Part(unsigned int nOptions) :
	pParent_(0),
	nOptions_(nOptions)
{
}

qs::Part::~Part()
{
	clear();
}

bool qs::Part::create(const Part* pParent,
					  const CHAR* pszContent,
					  size_t nLen)
{
	size_t nMaxPartCount = nMaxPartCount__;
	return create(pParent, pszContent, nLen, &nMaxPartCount);
}

void qs::Part::clear()
{
	strHeader_.reset(0);
	clearHeaderLower();
	strBody_.reset(0);
	std::for_each(listPart_.begin(), listPart_.end(), deleter<Part>());
	listPart_.clear();
	pPartEnclosed_.reset(0);
	pParent_ = 0;
	
	updateContentType();
}

std::auto_ptr<Part> qs::Part::clone() const
{
	std::auto_ptr<Part> pPart(new Part(nOptions_));
	
	if (strHeader_.get()) {
		if (!pPart->setHeader(strHeader_.get()))
			return std::auto_ptr<Part>(0);
	}
	
	if (strBody_.get()) {
		if (!pPart->setBody(strBody_.get(), -1))
			return std::auto_ptr<Part>(0);
	}
	
	if (pPartEnclosed_.get()) {
		pPart->pPartEnclosed_ = pPartEnclosed_->clone();
		if (!pPart->pPartEnclosed_.get())
			return std::auto_ptr<Part>(0);
	}
	
	PartList::const_iterator it = listPart_.begin();
	while (it != listPart_.end()) {
		std::auto_ptr<Part> pPartChild((*it)->clone());
		if (!pPartChild.get())
			return std::auto_ptr<Part>(0);
		pPart->addPart(pPartChild);
		++it;
	}
	
	if (strPreamble_.get()) {
		if (!pPart->setPreamble(strPreamble_.get()))
			return std::auto_ptr<Part>(0);
	}
	if (strEpilogue_.get()) {
		if (!pPart->setEpilogue(strEpilogue_.get()))
			return std::auto_ptr<Part>(0);
	}
	
	return pPart;
}

xstring_size_ptr qs::Part::getContent() const
{
	XStringBuffer<XSTRING> buf;
	if (!getContent(&buf))
		return xstring_size_ptr();
	return buf.getXStringSize();
}

bool qs::Part::getContent(XStringBuffer<XSTRING>* pBuf) const
{
	if (strHeader_.get()) {
		if (!pBuf->append(strHeader_.get()))
			return 0;
	}
	
	if (!pBuf->append("\r\n"))
		return 0;
	
	bool bProcessed = false;
	const ContentTypeParser* pContentType = getContentType();
	if (pContentType &&
		_wcsicmp(pContentType->getMediaType(), L"multipart") == 0) {
		wstring_ptr wstrBoundary(pContentType->getParameter(L"boundary"));
		if (wstrBoundary.get()) {
			string_ptr strBoundary(wcs2mbs(wstrBoundary.get()));
			
			bProcessed = true;
			
			if (strPreamble_.get()) {
				if (!pBuf->append(strPreamble_.get()) ||
					!pBuf->append("\r\n"))
					return false;
			}
			if (!listPart_.empty()) {
				for (PartList::const_iterator it = listPart_.begin(); it != listPart_.end(); ++it) {
					if (it != listPart_.begin()) {
						if (!pBuf->append("\r\n"))
							return false;
					}
					if (!pBuf->append("--") ||
						!pBuf->append(strBoundary.get()) ||
						!pBuf->append("\r\n"))
						return false;
					if (!(*it)->getContent(pBuf))
						return false;
				}
				if (!pBuf->append("\r\n--") ||
					!pBuf->append(strBoundary.get()) ||
					!pBuf->append("--"))
					return false;
			}
			if (strEpilogue_.get()) {
				if (!pBuf->append("\r\n") ||
					!pBuf->append(strEpilogue_.get()))
					return false;
			}
		}
	}
	if (pPartEnclosed_.get()) {
		bProcessed = true;
		if (!pPartEnclosed_->getContent(pBuf))
			return false;
	}
	if (!bProcessed) {
		assert(listPart_.empty());
		if (strBody_.get()) {
			if (!pBuf->append(strBody_.get()))
				return false;
		}
	}
	
	return true;
}

const CHAR* qs::Part::getHeader() const
{
	return strHeader_.get() ? strHeader_.get() : "";
}

bool qs::Part::setHeader(const CHAR* pszHeader)
{
	xstring_ptr strHeader;
	if (pszHeader && *pszHeader) {
		strHeader = allocXString(pszHeader);
		if (!strHeader.get())
			return false;
	}
	
	strHeader_ = strHeader;
	clearHeaderLower();
	
	updateContentType();
	
	return true;
}

Part::Field qs::Part::getField(const WCHAR* pwszName,
							   FieldParser* pParser) const
{
	assert(pwszName);
	assert(pParser);
	return pParser->parse(*this, pwszName);
}

bool qs::Part::hasField(const WCHAR* pwszName) const
{
	assert(pwszName);
	string_ptr strName(wcs2mbs(pwszName));
	return getFieldPos(strName.get(), 0) != 0;
}

bool qs::Part::setField(const WCHAR* pwszName,
						const FieldParser& parser)
{
	return setField(pwszName, parser, false, 0);
}

bool qs::Part::setField(const WCHAR* pwszName,
						const FieldParser& parser,
						bool* pbSet)
{
	return setField(pwszName, parser, false, pbSet);
}

bool qs::Part::setField(const WCHAR* pwszName,
						const FieldParser& parser,
						bool bAllowMultiple)
{
	return setField(pwszName, parser, bAllowMultiple, 0);
}

bool qs::Part::setField(const WCHAR* pwszName,
						const FieldParser& parser,
						bool bAllowMultiple,
						bool* pbSet)
{
	assert(pwszName);
	
	if (pbSet)
		*pbSet = false;
	
	if (!bAllowMultiple) {
		if (hasField(pwszName))
			return true;
	}
	
	string_ptr strName(wcs2mbs(pwszName));
	string_ptr strValue(parser.unparse(*this));
	if (!strValue.get())
		return false;
	
	assert(!strHeader_.get() ||
		strncmp(strHeader_.get() + strlen(strHeader_.get()) - 2, "\r\n", 2) == 0);
	
	XStringBuffer<XSTRING> buf;
	if (strHeader_.get()) {
		if (!buf.append(strHeader_.get()))
			return false;
	}
	if (!buf.append(strName.get()) ||
		!buf.append(": ") ||
		!buf.append(strValue.get()) ||
		!buf.append("\r\n"))
		return false;
	
	strHeader_ = buf.getXString();
	
	clearHeaderLower();
	if (_wcsicmp(pwszName, L"Content-Type") == 0)
		updateContentType();
	
	if (pbSet)
		*pbSet = true;
	
	return true;
}

bool qs::Part::replaceField(const WCHAR* pwszName,
							const FieldParser& parser)
{
	return replaceField(pwszName, parser, 0);
}

bool qs::Part::replaceField(const WCHAR* pwszName,
							const FieldParser& parser,
							unsigned int nIndex)
{
	assert(pwszName);
	
	if (nIndex == 0xffffffff) {
		removeField(pwszName, 0xffffffff);
		return setField(pwszName, parser, true);
	}
	else {
		string_ptr strName(wcs2mbs(pwszName));
		
		CHAR* pBegin = getFieldPos(strName.get(), nIndex);
		if (!pBegin) {
			if (!setField(pwszName, parser, true))
				return false;
		}
		else {
			CHAR* pEnd = getFieldEndPos(pBegin);
			assert(pEnd);
			
			string_ptr strValue(parser.unparse(*this));
			if (!strValue.get())
				return false;
			
			XStringBuffer<XSTRING> buf;
			if (!buf.append(strHeader_.get(), pBegin - strHeader_.get()) ||
				!buf.append(strName.get()) ||
				!buf.append(": ") ||
				!buf.append(strValue.get()) ||
				!buf.append("\r\n") ||
				!buf.append(pEnd))
				return false;
			
			strHeader_ = buf.getXString();
			
			clearHeaderLower();
			if (_wcsicmp(pwszName, L"Content-Type") == 0)
				updateContentType();
		}
	}
	
	return true;
}

bool qs::Part::removeField(const WCHAR* pwszName)
{
	return removeField(pwszName, 0xffffffff);
}

bool qs::Part::removeField(const WCHAR* pwszName,
						   unsigned int nIndex)
{
	assert(pwszName);
	
	bool bRemove = false;
	
	if (nIndex == 0xffffffff) {
		while (true) {
			if (!removeField(pwszName, 0))
				break;
			bRemove = true;
		}
	}
	else {
		string_ptr strName(wcs2mbs(pwszName));
		
		CHAR* pBegin = getFieldPos(strName.get(), nIndex);
		if (pBegin) {
			CHAR* pEnd = getFieldEndPos(pBegin);
			if (pEnd) {
				size_t nLen = strlen(pEnd);
				memmove(pBegin, pEnd, nLen);
				*(pBegin + nLen) = '\0';
				if (*strHeader_.get() == '\0')
					strHeader_.reset(0);
				clearHeaderLower();
				if (_wcsicmp(pwszName, L"Content-Type") == 0)
					updateContentType();
				bRemove = true;
			}
		}
	}
	
	return bRemove;
}

void qs::Part::getFields(FieldList* pListField) const
{
	assert(pListField);
	
	if (!strHeader_.get())
		return;
	
	StringBuffer<STRING> buf;
	
	for (const CHAR* p = strHeader_.get(); *p; ++p) {
		CHAR c = *p;
		if (c == '\r' && *(p + 1) == '\n' && *(p + 2) != ' ' && *(p + 2) != '\t') {
			string_ptr strLine(buf.getString());
			const CHAR* pIndex = strchr(strLine.get(), ':');
			if (pIndex) {
				--pIndex;
				while (*pIndex == ' ' || *pIndex == '\t')
					--pIndex;
				string_ptr strName(tolower(strLine.get(), pIndex - strLine.get() + 1));
				pListField->push_back(std::make_pair(strName.get(), strLine.get()));
				strName.release();
				strLine.release();
			}
			++p;
		}
		else {
			buf.append(c);
		}
	}
}

bool qs::Part::copyFields(const Part& part,
						  FieldFilter* pFilter)
{
	assert(pFilter);
	
	FieldList listOld;
	FieldListFree freeOld(listOld);
	getFields(&listOld);
	
	FieldList listNew;
	FieldListFree freeNew(listNew);
	part.getFields(&listNew);
	
	XStringBuffer<XSTRING> buf;
	for (FieldList::iterator it = listOld.begin(); it != listOld.end(); ++it) {
		if (!pFilter->accept((*it).first)) {
			if (!buf.append((*it).second) ||
				!buf.append("\r\n"))
				return false;
		}
	}
	for (FieldList::iterator it = listNew.begin(); it != listNew.end(); ++it) {
		if (pFilter->accept((*it).first)) {
			if (!buf.append((*it).second) ||
				!buf.append("\r\n"))
				return false;
		}
	}
	xstring_ptr strHeader(buf.getXString());
	if (!strHeader.get())
		return false;
	strHeader_ = strHeader;
	
	clearHeaderLower();
	updateContentType();
	
	return true;
}

bool qs::Part::removeFields(FieldFilter* pFilter)
{
	assert(pFilter);
	
	FieldList l;
	FieldListFree free(l);
	getFields(&l);
	
	XStringBuffer<XSTRING> buf;
	for (FieldList::iterator it = l.begin(); it != l.end(); ++it) {
		if (!pFilter->accept((*it).first)) {
			if (!buf.append((*it).second) ||
				!buf.append("\r\n"))
				return false;
		}
	}
	xstring_ptr strHeader(buf.getXString());
	if (!strHeader.get())
		return false;
	strHeader_ = strHeader;
	
	clearHeaderLower();
	updateContentType();
	
	return true;
}

bool qs::Part::sortHeader()
{
	FieldList l;
	FieldListFree free(l);
	getFields(&l);
	std::sort(l.begin(), l.end(), FieldComparator());
	
	XStringBuffer<XSTRING> buf;
	for (FieldList::iterator it = l.begin(); it != l.end(); ++it) {
		if (!buf.append((*it).second) ||
			!buf.append("\r\n"))
			return false;
	}
	xstring_ptr strHeader(buf.getXString());
	if (!strHeader.get())
		return false;
	strHeader_ = strHeader;
	
	clearHeaderLower();
	
	return true;
}

const ContentTypeParser* qs::Part::getContentType() const
{
	return pContentType_.get();
}

wstring_ptr qs::Part::getCharset() const
{
	wstring_ptr wstrCharset;
	const ContentTypeParser* pContentType = getContentType();
	if (!pContentType) {
		if (!isOption(O_ALLOW_USE_DEFAULT_ENCODING) && hasField(L"MIME-Version"))
			wstrCharset = allocWString(L"us-ascii");
		else
			wstrCharset = allocWString(getDefaultCharset());
	}
	else {
		assert(pContentType->getMediaType());
		if (_wcsicmp(pContentType->getMediaType(), L"text") == 0) {
			wstrCharset = pContentType->getParameter(L"charset");
			if (!wstrCharset.get()) {
				if (isOption(O_ALLOW_USE_DEFAULT_ENCODING))
					wstrCharset = allocWString(getDefaultCharset());
				else
					wstrCharset = allocWString(L"us-ascii");
			}
		}
	}
	
	return wstrCharset;
}

bool qs::Part::isMultipart() const
{
	const ContentTypeParser* pContentType = getContentType();
	return pContentType &&
		_wcsicmp(pContentType->getMediaType(), L"multipart") == 0;
}

bool qs::Part::isText() const
{
	return !pContentType_.get() || _wcsicmp(pContentType_->getMediaType(), L"text") == 0;
}

bool qs::Part::isAttachment() const
{
	if (isMultipart())
		return false;
	
	const WCHAR* pwszContentDisposition = 0;
	ContentDispositionParser contentDisposition;
	Part::Field field = getField(L"Content-Disposition", &contentDisposition);
	if (field == Part::FIELD_EXIST) {
		pwszContentDisposition = contentDisposition.getDispositionType();
		if (_wcsicmp(pwszContentDisposition, L"attachment") == 0)
			return true;
		
		wstring_ptr wstrFileName(contentDisposition.getParameter(L"filename"));
		if (wstrFileName.get())
			return true;
	}
	else if (field == Part::FIELD_ERROR) {
		return true;
	}
	
	if (pContentType_.get()) {
		const WCHAR* pwszMediaType = pContentType_->getMediaType();
		const WCHAR* pwszSubType = pContentType_->getSubType();
		bool bCanInline = _wcsicmp(pwszMediaType, L"text") == 0 ||
			(_wcsicmp(pwszMediaType, L"message") == 0 &&
				_wcsicmp(pwszSubType, L"rfc822") == 0);
		if (!bCanInline) {
			return true;
		}
		else if (pwszContentDisposition &&
			_wcsicmp(pwszContentDisposition, L"inline") == 0 &&
			_wcsicmp(pwszMediaType, L"text") == 0 &&
			_wcsicmp(pwszSubType, L"plain") == 0) {
			return false;
		}
		else {
			wstring_ptr wstrName(pContentType_->getParameter(L"name"));
			if (wstrName.get())
				return true;
		}
	
	}
	
	return false;
}

Part::Format qs::Part::getFormat() const
{
	if (!isOption(O_INTERPRET_FORMAT_FLOWED))
		return FORMAT_NONE;
	else if (!isText())
		return FORMAT_NONE;
	else if (!pContentType_.get())
		return FORMAT_NONE;
	
	wstring_ptr wstrFormat(pContentType_->getParameter(L"format"));
	if (!wstrFormat.get() || _wcsicmp(wstrFormat.get(), L"flowed") != 0)
		return FORMAT_NONE;
	
	wstring_ptr wstrDelSp(pContentType_->getParameter(L"delsp"));
	if (wstrDelSp.get() && _wcsicmp(wstrDelSp.get(), L"yes") == 0)
		return FORMAT_FLOWED_DELSP;
	
	return FORMAT_FLOWED;
}

std::auto_ptr<Encoder> qs::Part::getEncoder() const
{
	ContentTransferEncodingParser contentTransferEncoding;
	if (getField(L"Content-Transfer-Encoding", &contentTransferEncoding) != FIELD_EXIST)
		return std::auto_ptr<Encoder>();
	return EncoderFactory::getInstance(contentTransferEncoding.getEncoding());
}

string_ptr qs::Part::getRawField(const WCHAR* pwszName,
								 unsigned int nIndex) const
{
	assert(pwszName);
	
	string_ptr strName(wcs2mbs(pwszName));
	
	CHAR* p = getFieldPos(strName.get(), nIndex);
	if (!p)
		return 0;
	
	StringBuffer<STRING> buf;
	
	p = strchr(p, ':');
	assert(p);
	++p;
	while (Tokenizer::isSpace(*p))
		++p;
	while (true) {
		CHAR c = *p;
		assert(c != '\0');
		if (c == '\r' && *(p + 1) == '\n') {
			CHAR cNext = *(p + 2);
			if (cNext != ' ' && cNext != '\t')
				break;
			buf.append(' ');
			p += 2;
			while (*(p + 1) == ' ' || *(p + 1) == '\t')
				++p;
		}
		else if (c == '\r' || c == '\n') {
			buf.append(' ');
		}
		else {
			buf.append(c);
		}
		++p;
	}
	
	return buf.getString();
}

wstring_ptr qs::Part::getHeaderCharset() const
{
	wstring_ptr wstrCharset;
	if (pParent_) {
		wstrCharset = pParent_->getHeaderCharset();
	}
	else {
		const ContentTypeParser* pContentType = getContentType();
		if (pContentType && _wcsicmp(pContentType->getMediaType(), L"multipart") == 0) {
			if (!listPart_.empty())
				wstrCharset = listPart_.front()->getCharset();
		}
		else {
			wstrCharset = getCharset();
		}
		if (!wstrCharset.get() || _wcsicmp(wstrCharset.get(), L"us-ascii") == 0)
			wstrCharset = allocWString(getDefaultCharset());
	}
	
	assert(wstrCharset.get());
	
	return wstrCharset;
}

const CHAR* qs::Part::getBody() const
{
	return strBody_.get();
}

bool qs::Part::setBody(const CHAR* pszBody,
					   size_t nLen)
{
	if (nLen == static_cast<size_t>(-1))
		nLen = strlen(pszBody);
	
	xstring_ptr strBody(allocXString(pszBody, nLen));
	if (!strBody.get())
		return false;
	
	strBody_ = strBody;
	
	return true;
}

void qs::Part::setBody(xstring_ptr strBody)
{
	strBody_ = strBody;
}

wxstring_size_ptr qs::Part::getBodyText() const
{
	return getBodyText(0);
}

wxstring_size_ptr qs::Part::getBodyText(const WCHAR* pwszCharset) const
{
	XStringBuffer<WXSTRING> buf;
	if (!getBodyText(pwszCharset, &buf))
		return wxstring_size_ptr();
	return buf.getXStringSize();
}

bool qs::Part::getBodyText(const WCHAR* pwszCharset,
						   XStringBuffer<WXSTRING>* pBuf) const
{
	assert(pBuf);
	
	if (strBody_.get()) {
		if (!isText())
			return true;
		
		std::auto_ptr<Encoder> pEncoder(getEncoder());
		
		const CHAR* pszDecodedBody = strBody_.get();
		size_t nDecodedBodyLen = 0;
		malloc_size_ptr<unsigned char> decoded;
		if (pEncoder.get()) {
			decoded = pEncoder->decode(
				reinterpret_cast<const unsigned char*>(strBody_.get()),
				strlen(strBody_.get()));
			if (!decoded.get())
				return false;
			pszDecodedBody = reinterpret_cast<CHAR*>(decoded.get());
			nDecodedBodyLen = decoded.size();
		}
		else {
			nDecodedBodyLen = strlen(pszDecodedBody);
		}
		
		xstring_ptr strDecode(allocXString(nDecodedBodyLen));
		if (!strDecode.get())
			return false;
		CHAR* pDst = strDecode.get();
		for (size_t n = 0; n < nDecodedBodyLen; ++n) {
			CHAR c = *(pszDecodedBody + n);
			if (c == '\r') {
				if (n + 1 >= nDecodedBodyLen || *(pszDecodedBody + n + 1) != '\n')
					*pDst++ = '\n';
			}
			else {
				*pDst++ = c;
			}
		}
		*pDst = '\0';
		
		wstring_ptr wstrCharset;
		if (!pwszCharset) {
			wstrCharset = getCharset();
			pwszCharset = wstrCharset.get();
		}
		
		std::auto_ptr<Converter> pConverter(
			ConverterFactory::getInstance(pwszCharset));
		if (!pConverter.get())
			pConverter = ConverterFactory::getInstance(getDefaultCharset());
		assert(pConverter.get());
		
		size_t nLen = pDst - strDecode.get();
		wxstring_size_ptr converted(pConverter->decode(strDecode.get(), &nLen));
		if (!converted.get())
			return false;
		
		Format format = getFormat();
		if (format != FORMAT_NONE) {
			if (!interpretFlowedFormat(converted.get(), format == FORMAT_FLOWED_DELSP, pBuf))
				return false;
		}
		else {
			if (!pBuf->append(converted.get(), converted.size()))
				return false;
		}
	}
	
	return true;
}

malloc_size_ptr<unsigned char> qs::Part::getBodyData() const
{
	std::auto_ptr<Encoder> pEncoder(getEncoder());
	malloc_size_ptr<unsigned char> decoded;
	if (pEncoder.get()) {
		decoded = pEncoder->decode(
			reinterpret_cast<const unsigned char*>(strBody_.get()),
			strlen(strBody_.get()));
		if (!decoded.get())
			return malloc_size_ptr<unsigned char>();
	}
	else {
		size_t nLen = strlen(strBody_.get());
		malloc_ptr<unsigned char> p(static_cast<unsigned char*>(malloc(nLen + 1)));
		if (!p.get())
			return malloc_size_ptr<unsigned char>();
		decoded = malloc_size_ptr<unsigned char>(p.release(), nLen);
		memcpy(decoded.get(), strBody_.get(), nLen);
	}
	
	return decoded;
}

const Part::PartList& qs::Part::getPartList() const
{
	return listPart_;
}

size_t qs::Part::getPartCount() const
{
	return listPart_.size();
}

Part* qs::Part::getPart(unsigned int n) const
{
	assert(n < listPart_.size());
	assert(listPart_[n]->getParentPart() == this);
	return listPart_[n];
}

void qs::Part::addPart(std::auto_ptr<Part> pPart)
{
	assert(pPart.get());
	assert(!strBody_.get());
	assert(!pPartEnclosed_.get());
	
	listPart_.push_back(pPart.get());
	pPart->pParent_ = this;
	pPart.release();
}

void qs::Part::insertPart(unsigned int n,
						  std::auto_ptr<Part> pPart)
{
	assert(n <= listPart_.size());
	assert(pPart.get());
	assert(!strBody_.get());
	assert(!pPartEnclosed_.get());
	
	listPart_.insert(listPart_.begin() + n, pPart.get());
	pPart->pParent_ = this;
	pPart.release();
}

void qs::Part::removePart(Part* pPart)
{
	assert(pPart);
	
	PartList::iterator it = std::find(
		listPart_.begin(), listPart_.end(), pPart);
	assert(it != listPart_.end());
	listPart_.erase(it);
	
	pPart->pParent_ = 0;
}

Part* qs::Part::getParentPart() const
{
	return pParent_;
}

const CHAR* qs::Part::getPreamble() const
{
	return strPreamble_.get();
}

bool qs::Part::setPreamble(const CHAR* pszPreamble)
{
	if (pszPreamble) {
		xstring_ptr strPreamble(allocXString(pszPreamble));
		if (!strPreamble.get())
			return false;
		strPreamble_ = strPreamble;
	}
	else {
		strPreamble_.reset(0);
	}
	return true;
}

const CHAR* qs::Part::getEpilogue() const
{
	return strEpilogue_.get();
}

bool qs::Part::setEpilogue(const CHAR* pszEpilogue)
{
	if (pszEpilogue) {
		xstring_ptr strEpilogue(allocXString(pszEpilogue));
		if (!strEpilogue.get())
			return false;
		strEpilogue_ = strEpilogue;
	}
	else {
		strEpilogue_.reset(0);
	}
	return true;
}

Part* qs::Part::getEnclosedPart() const
{
	return pPartEnclosed_.get();
}

void qs::Part::setEnclosedPart(std::auto_ptr<Part> pPart)
{
	pPartEnclosed_ = pPart;
}

bool qs::Part::isOption(Option option) const
{
	return (nOptions_ & option) != 0;
}

void qs::Part::setOptions(unsigned int nOptions)
{
	nOptions_ = nOptions;
}

const WCHAR* qs::Part::getDefaultCharset()
{
	return wstrDefaultCharset__.get();
}

void qs::Part::setDefaultCharset(const WCHAR* pwszCharset)
{
	if (pwszCharset)
		wstrDefaultCharset__ = allocWString(pwszCharset);
	else
		wstrDefaultCharset__.reset(0);
}

bool qs::Part::isGlobalOption(Option option)
{
	return (nGlobalOptions__ && option) != 0;
}

void qs::Part::setGlobalOptions(unsigned int nOptions)
{
	nGlobalOptions__ = nOptions;
}

size_t qs::Part::getMaxHeaderLength()
{
	return nMaxHeaderLength__;
}

void qs::Part::setMaxHeaderLength(size_t nMax)
{
	nMaxHeaderLength__ = nMax;
}

size_t qs::Part::getMaxPartCount()
{
	return nMaxPartCount__;
}

void qs::Part::setMaxPartCount(size_t nMax)
{
	nMaxPartCount__ = nMax;
}

const CHAR* qs::Part::getBody(const CHAR* pszContent,
							  size_t nLen)
{
	assert(pszContent);
	
	if (nLen == -1)
		nLen = strlen(pszContent);
	
	const CHAR* pBody = pszContent;
	if (nLen < 2 || strncmp(pBody, "\r\n", 2) != 0) {
		BMFindString<STRING> bmfs("\r\n\r\n");
		pBody = bmfs.find(pszContent, nLen);
		if (pBody)
			pBody += 2;
	}
	if (pBody)
		pBody += 2;
	
	assert(!pBody || (pszContent <= pBody && pBody <= pszContent + nLen));
	
	return pBody;
}

bool qs::Part::create(const Part* pParent,
					  const CHAR* pszContent,
					  size_t nLen,
					  size_t* pnMaxPartCount)
{
	assert(pszContent);
	assert(pnMaxPartCount);
	
	if (nLen == static_cast<size_t>(-1))
		nLen = strlen(pszContent);
	
	clear();
	
	const CHAR* pBody = getBody(pszContent, nLen);
	size_t nHeaderLen = 0;
	if (!pBody)
		nHeaderLen = nLen;
	else if (pBody != pszContent + 2)
		nHeaderLen = pBody - pszContent - 2;
	if (nHeaderLen != 0) {
		if (nHeaderLen > nMaxHeaderLength__)
			nHeaderLen = nMaxHeaderLength__;
		
		strHeader_ = allocXString(nHeaderLen + 2);
		if (!strHeader_.get())
			return false;
		strncpy(strHeader_.get(), pszContent, nHeaderLen);
		if (strncmp(strHeader_.get() + nHeaderLen - 2, "\r\n", 2) != 0)
			strcpy(strHeader_.get() + nHeaderLen, "\r\n");
		else
			*(strHeader_.get() + nHeaderLen) = '\0';
	}
	
	updateContentType();
	
	if (pBody) {
		bool bProcessed = false;
		const ContentTypeParser* pContentType = getContentType();
		const WCHAR* pwszMediaType = L"text";
		wstring_ptr strMediaType;
		if (pContentType) {
			assert(pContentType->getMediaType());
			strMediaType = tolower(pContentType->getMediaType());
			pwszMediaType = strMediaType.get();
		}
		if (wcscmp(pwszMediaType, L"multipart") == 0) {
			wstring_ptr wstrBoundary(pContentType->getParameter(L"boundary"));
			if (wstrBoundary.get()) {
				bProcessed = true;
				
				string_ptr strBoundary(wcs2mbs(wstrBoundary.get()));
				BoundaryFinder<CHAR, STRING> finder(pBody - 2,
					nLen - (pBody - 2 - pszContent), strBoundary.get(),
					"\r\n", isOption(O_ALLOW_INCOMPLETE_MULTIPART));
				
				bool bFirst = true;
				while (true) {
					if (*pnMaxPartCount == 0)
						break;
					--*pnMaxPartCount;
					
					const CHAR* pBegin = 0;
					const CHAR* pEnd = 0;
					bool bEnd = false;
					if (!finder.getNext(&pBegin, &pEnd, &bEnd))
						return false;
					
					if (pBegin) {
						std::auto_ptr<Part> pChildPart(new Part(nOptions_));
						if (!pChildPart->create(this, pBegin, pEnd - pBegin, pnMaxPartCount))
							return false;
						addPart(pChildPart);
					}
					
					if (bEnd)
						break;
				}
				
				std::pair<const CHAR*, size_t> preamble(finder.getPreamble());
				if (preamble.first) {
					strPreamble_ = allocXString(preamble.first, preamble.second);
					if (!strPreamble_.get())
						return false;
				}
				std::pair<const CHAR*, size_t> epilogue(finder.getEpilogue());
				if (epilogue.first) {
					strEpilogue_ = allocXString(epilogue.first, epilogue.second);
					if (!strEpilogue_.get())
						return false;
				}
			}
		}
		else {
			bool bRFC822 = _wcsicmp(pwszMediaType, L"message") == 0 &&
				_wcsicmp(pContentType->getSubType(), L"rfc822") == 0;
			if (!bRFC822 && !pContentType && pParent) {
				const ContentTypeParser* pContentTypeParent = pParent->getContentType();
				bRFC822 = pContentTypeParent &&
					_wcsicmp(pContentTypeParent->getMediaType(), L"multipart") == 0 &&
					_wcsicmp(pContentTypeParent->getSubType(), L"digest") == 0;
			}
			if (bRFC822) {
				if (*pnMaxPartCount == 0)
					bRFC822 = false;
				--*pnMaxPartCount;
			}
			if (bRFC822) {
				bProcessed = true;
				pPartEnclosed_.reset(new Part(nOptions_));
				if (!pPartEnclosed_->create(0, pBody, nLen - (pBody - pszContent), pnMaxPartCount))
					return false;
			}
		}
		if (!bProcessed) {
			strBody_ = allocXString(pBody, nLen - (pBody - pszContent));
			if (!strBody_.get())
				return false;
		}
	}
	else {
		strBody_ = allocXString("");
	}
	
	return true;
}

const CHAR* qs::Part::getHeaderLower() const
{
	if (!strHeaderLower_.get()) {
		if (strHeader_.get()) {
			size_t nLen = strlen(strHeader_.get());
			xstring_ptr strHeaderLower(allocXString(nLen + 3));
			CHAR* pDst = strHeaderLower.get();
			*pDst++ = '\r';
			*pDst++ = '\n';
			for (const CHAR* p = strHeader_.get(); *p; ++p)
				*pDst++ = ::tolower(*p);
			*pDst = '\0';
			strHeaderLower_ = strHeaderLower;
		}
		else {
			strHeaderLower_ = allocXString("\r\n");
		}
	}
	
#ifndef NDEBUG
	if (strHeader_.get()) {
		string_ptr strLower(tolower(strHeader_.get()));
		string_ptr strHeaderLower(concat("\r\n", strLower.get()));
		assert(strcmp(strHeaderLower.get(), strHeaderLower_.get()) == 0);
	}
#endif
	
	return strHeaderLower_.get();
}

void qs::Part::clearHeaderLower() const
{
	strHeaderLower_.reset(0);
}

void qs::Part::updateContentType()
{
	pContentType_.reset(0);
	
	if (strHeader_.get()) {
		std::auto_ptr<ContentTypeParser> pContentType(new ContentTypeParser());
		if (getField(L"Content-Type", pContentType.get()) == FIELD_EXIST)
			pContentType_ = pContentType;
	}
}

CHAR* qs::Part::getFieldPos(const CHAR* pszName,
							unsigned int nIndex) const
{
	assert(pszName);
	
	string_ptr strNameLower(tolower(pszName));
	string_ptr strFind(concat("\r\n", strNameLower.get()));
	
	BMFindString<STRING> bmfs(strFind.get());
	const CHAR* pszHeader = getHeaderLower();
	if (!pszHeader)
		return 0;
	
	const CHAR* pFind = pszHeader;
	while (true) {
		const CHAR* pBegin = bmfs.find(pFind);
		if (!pBegin)
			return 0;
		
		const CHAR* p = pBegin + strlen(pszName) + 2;
		while (*p) {
			CHAR c = *p;
			if (c == ':') {
				break;
			}
			else if (c == '\r') {
				++p;
				if (*p == '\n') {
					++p;
					if (*p == ' ' || *p == '\t')
						continue;
				}
				break;
			}
			else if (c != ' ' && c != '\t')
				break;
			++p;
		}
		if (!*p)
			return 0;
		
		if (*p == L':') {
			if (nIndex == 0)
				return const_cast<CHAR*>(strHeader_.get() + (pBegin - pszHeader));
			else
				--nIndex;
		}
		pFind = pBegin + 2;
	}
	
	return 0;
}

CHAR* qs::Part::getFieldEndPos(const CHAR* pBegin) const
{
	assert(pBegin);
	
	const CHAR* p = pBegin;
	while (*p) {
		CHAR c = *p;
		if (c == '\r' && *(p + 1) == '\n' &&
			*(p + 2) != ' ' && *(p + 2) != '\t') {
			p += 2;
			break;
		}
		++p;
	}
	assert(*(p - 2) == '\r' && *(p - 1) == '\n');
	assert(*p != ' ' && *p != '\t');
	
	return const_cast<CHAR*>(p);
}

bool qs::Part::interpretFlowedFormat(const WCHAR* pwszText,
									 bool bDelSp,
									 XStringBuffer<WXSTRING>* pBuf)
{
	size_t nPrevQuoteDepth = 0;
	bool bPrevSoftBreak = false;
	const WCHAR* pBegin = pwszText;
	while (true) {
		const WCHAR* pEnd = wcschr(pBegin, L'\n');
		if (!pEnd)
			pEnd = pBegin + wcslen(pBegin);
		
		const WCHAR* p = pBegin;
		size_t nQuoteDepth = 0;
		while (*p == L'>') {
			++p;
			++nQuoteDepth;
		}
		if (*p == L' ')
			++p;
		
		bool bSoftBreak = pEnd > p && *(pEnd - 1) == L' ';
		if (bSoftBreak) {
			if (pEnd - p == 3 && *p == '-' && *(p + 1) == L'-')
				bSoftBreak = false;
		}
		
		if (bPrevSoftBreak && nPrevQuoteDepth == nQuoteDepth) {
			if (!pBuf->append(p, pEnd - p - (bSoftBreak && bDelSp ? 1 : 0)))
				return false;
		}
		else {
			if (pBegin != pwszText) {
				if (!pBuf->append(L'\n'))
					return false;
			}
			if (!pBuf->append(pBegin, pEnd - pBegin - (bSoftBreak && bDelSp ? 1 : 0)))
				return false;
		}
		
		if (!*pEnd)
			break;
		
		nPrevQuoteDepth = nQuoteDepth;
		bPrevSoftBreak = bSoftBreak;
		pBegin = pEnd + 1;
	}
	if (!pBuf->append(L'\n'))
		return false;
	
	return true;
}


/****************************************************************************
 *
 * Part::FieldListFree
 *
 */

qs::Part::FieldListFree::FieldListFree(FieldList& l) :
	l_(l)
{
}

qs::Part::FieldListFree::~FieldListFree()
{
	free();
}

void qs::Part::FieldListFree::free()
{
	std::for_each(l_.begin(), l_.end(),
		unary_compose_fx_gx(
			string_free<STRING>(),
			string_free<STRING>()));
	l_.clear();
}


/****************************************************************************
 *
 * FieldFilter
 *
 */

qs::FieldFilter::~FieldFilter()
{
}


/****************************************************************************
 *
 * PrefixFieldFilter
 *
 */

qs::PrefixFieldFilter::PrefixFieldFilter(const CHAR* pszPrefix) :
	pszPrefix_(pszPrefix),
	nLen_(strlen(pszPrefix)),
	bNot_(false)
{
}

qs::PrefixFieldFilter::PrefixFieldFilter(const CHAR* pszPrefix,
										 bool bNot) :
	pszPrefix_(pszPrefix),
	nLen_(strlen(pszPrefix)),
	bNot_(bNot)
{
}

qs::PrefixFieldFilter::~PrefixFieldFilter()
{
}

bool qs::PrefixFieldFilter::accept(const CHAR* pszName)
{
	bool bAccept = strlen(pszName) > nLen_ &&
		strncmp(pszName, pszPrefix_, nLen_) == 0;
	return bNot_ ? !bAccept : bAccept;
}
