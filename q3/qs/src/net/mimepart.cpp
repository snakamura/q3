/*
 * $Id$
 *
 * Copyright(C) 1998-2003 Satoshi Nakamura
 * All rights reserved.
 *
 */

#include <qsmime.h>
#include <qsconv.h>
#include <qsnew.h>
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
		if (strcmp(pszFieldLhs, pszFields[n]) == 0)
			return true;
		else if (strcmp(pszFieldRhs, pszFields[n]) == 0)
			return false;
	}
	if (strncmp(pszFieldLhs, "x-", 2) == 0)
		return strncmp(pszFieldRhs, "x-", 2) == 0 ? strcmp(pszFieldLhs, pszFieldRhs) < 0 : false;
	else if (strncmp(pszFieldRhs, "x-", 2) == 0)
		return true;
	return false;
}


/****************************************************************************
 *
 * Part
 *
 */

WSTRING qs::Part::wstrDefaultCharset__ = 0;
unsigned int qs::Part::nGlobalOptions__ = 0;

qs::Part::Part(QSTATUS* pstatus) :
	strHeader_(0),
	strBody_(0),
	pPartEnclosed_(0),
	pParent_(0),
	nOptions_(nGlobalOptions__),
	strHeaderLower_(0),
	pContentType_(0)
{
	assert(pstatus);
	*pstatus = QSTATUS_SUCCESS;
}

qs::Part::Part(unsigned int nOptions, QSTATUS* pstatus) :
	strHeader_(0),
	strBody_(0),
	pPartEnclosed_(0),
	pParent_(0),
	nOptions_(nOptions),
	strHeaderLower_(0),
	pContentType_(0)
{
	assert(pstatus);
	*pstatus = QSTATUS_SUCCESS;
}

qs::Part::Part(const Part* pParent, const CHAR* pszContent,
	size_t nLen, QSTATUS* pstatus) :
	strHeader_(0),
	strBody_(0),
	pPartEnclosed_(0),
	pParent_(0),
	nOptions_(nGlobalOptions__),
	strHeaderLower_(0),
	pContentType_(0)
{
	assert(pstatus);
	
	*pstatus = create(pParent, pszContent, nLen);
}

qs::Part::Part(const Part* pParent, const CHAR* pszContent,
	size_t nLen, unsigned int nOptions, QSTATUS* pstatus) :
	strHeader_(0),
	strBody_(0),
	pPartEnclosed_(0),
	pParent_(0),
	nOptions_(nOptions),
	strHeaderLower_(0),
	pContentType_(0)
{
	assert(pstatus);
	
	*pstatus = create(pParent, pszContent, nLen);
}

qs::Part::~Part()
{
	clear();
}

QSTATUS qs::Part::create(const Part* pParent,
	const CHAR* pszContent, size_t nLen)
{
	assert(pszContent);
	
	DECLARE_QSTATUS();
	
	status = clear();
	CHECK_QSTATUS();
	
	if (nLen == static_cast<size_t>(-1))
		nLen = strlen(pszContent);
	
	const CHAR* pBody = pszContent;
	if (strncmp(pBody, "\r\n", 2) != 0) {
		BMFindString<STRING> bmfs("\r\n\r\n", &status);
		CHECK_QSTATUS();
		pBody = bmfs.find(pszContent, nLen);
		if (pBody)
			pBody += 2;
	}
	if (pBody)
		pBody += 2;
	if (!pBody || pBody > pszContent + nLen) {
		strHeader_ = allocString(nLen + 2);
		if (!strHeader_)
			return QSTATUS_OUTOFMEMORY;
		strncpy(strHeader_, pszContent, nLen);
		if (strncmp(pszContent + nLen - 2, "\r\n", 2) != 0) {
			*(strHeader_ + nLen) = '\r';
			*(strHeader_ + nLen + 1) = '\n';
			*(strHeader_ + nLen + 2) = '\0';
		}
		else {
			*(strHeader_ + nLen) = '\0';
		}
		pBody = 0;
	}
	else if (pBody != pszContent + 2) {
		strHeader_ = allocString(pszContent, pBody - pszContent - 2);
		if (!strHeader_)
			return QSTATUS_OUTOFMEMORY;
		assert(strncmp(strHeader_ + strlen(strHeader_) - 2, "\r\n", 2) == 0);
	}
	
	status = updateContentType();
	CHECK_QSTATUS();
	
	if (pBody) {
		bool bProcessed = false;
		const ContentTypeParser* pContentType = getContentType();
		const WCHAR* pwszMediaType = L"text";
		string_ptr<WSTRING> strMediaType;
		if (pContentType) {
			assert(pContentType->getMediaType());
			strMediaType.reset(tolower(pContentType->getMediaType()));
			if (!strMediaType.get())
				return QSTATUS_OUTOFMEMORY;
			pwszMediaType = strMediaType.get();
		}
		if (wcscmp(pwszMediaType, L"multipart") == 0) {
			string_ptr<WSTRING> wstrBoundary;
			status = pContentType->getParameter(L"boundary", &wstrBoundary);
			CHECK_QSTATUS();
			if (wstrBoundary.get()) {
				bProcessed = true;
				
				string_ptr<STRING> strBoundary(wcs2mbs(wstrBoundary.get()));
				if (!strBoundary.get())
					return QSTATUS_OUTOFMEMORY;
				BoundaryFinder<CHAR, STRING> finder(pBody - 2,
					nLen - (pBody - 2 - pszContent), strBoundary.get(),
					"\r\n", isOption(O_ALLOW_INCOMPLETE_MULTIPART), &status);
				CHECK_QSTATUS();
				
				while (true) {
					const CHAR* pBegin = 0;
					const CHAR* pEnd = 0;
					bool bEnd = false;
					status = finder.getNext(&pBegin, &pEnd, &bEnd);
					CHECK_QSTATUS();
					if (pBegin) {
						std::auto_ptr<Part> pChildPart;
						status = newQsObject(this, pBegin,
							pEnd - pBegin, nOptions_, &pChildPart);
						CHECK_QSTATUS();
						status = addPart(pChildPart.get());
						CHECK_QSTATUS();
						pChildPart.release();
					}
					if (bEnd)
						break;
				}
			}
		}
		else {
			bool bRFC822 = wcscmp(pwszMediaType, L"message") == 0 &&
				_wcsicmp(pContentType->getSubType(), L"rfc822") == 0;
			if (!bRFC822 && !pContentType && pParent) {
				const ContentTypeParser* pContentTypeParent = pParent->getContentType();
				bRFC822 = pContentTypeParent &&
					_wcsicmp(pContentTypeParent->getMediaType(), L"multipart") == 0 &&
					_wcsicmp(pContentTypeParent->getSubType(), L"digest") == 0;
			}
			if (bRFC822) {
				bProcessed = true;
				status = newQsObject(static_cast<Part*>(0), pBody,
					nLen - (pBody - pszContent), nOptions_, &pPartEnclosed_);
				CHECK_QSTATUS();
			}
		}
		if (!bProcessed) {
			strBody_ = allocString(pBody, nLen - (pBody - pszContent));
			if (!strBody_)
				return QSTATUS_OUTOFMEMORY;
		}
	}
	else {
		strBody_ = allocString("");
	}
	
	return QSTATUS_SUCCESS;
}

QSTATUS qs::Part::clear()
{
	DECLARE_QSTATUS();
	
	freeString(strHeader_);
	strHeader_ = 0;
	clearHeaderLower();
	freeString(strBody_);
	strBody_ = 0;
	std::for_each(listPart_.begin(), listPart_.end(), deleter<Part>());
	listPart_.clear();
	delete pPartEnclosed_;
	pPartEnclosed_ = 0;
	pParent_ = 0;
	
	status = updateContentType();
	assert(status == QSTATUS_SUCCESS);
	
	return QSTATUS_SUCCESS;
}

QSTATUS qs::Part::clone(Part** ppPart) const
{
	DECLARE_QSTATUS();
	
	std::auto_ptr<Part> pPart;
	status = newQsObject(nOptions_, &pPart);
	CHECK_QSTATUS();
	
	if (strHeader_) {
		status = pPart->setHeader(strHeader_);
		CHECK_QSTATUS();
	}
	
	if (strBody_) {
		status = pPart->setBody(strBody_, -1);
		CHECK_QSTATUS();
	}
	
	if (pPartEnclosed_) {
		status = pPartEnclosed_->clone(&pPart->pPartEnclosed_);
		CHECK_QSTATUS();
	}
	
	PartList::const_iterator it = listPart_.begin();
	while (it != listPart_.end()) {
		Part* p = 0;
		status = (*it)->clone(&p);
		CHECK_QSTATUS();
		std::auto_ptr<Part> pPartChild(p);
		status = pPart->addPart(pPartChild.get());
		CHECK_QSTATUS();
		pPartChild.release();
		++it;
	}
	
	*ppPart = pPart.release();
	
	return QSTATUS_SUCCESS;
}

QSTATUS qs::Part::getContent(STRING* pstrContent) const
{
	assert(pstrContent);
	
	DECLARE_QSTATUS();
	
	StringBuffer<STRING> buf(&status);
	CHECK_QSTATUS();
	
	if (strHeader_) {
		status = buf.append(strHeader_);
		CHECK_QSTATUS();
	}
	
	status = buf.append("\r\n");
	CHECK_QSTATUS();
	
	bool bProcessed = false;
	const ContentTypeParser* pContentType = getContentType();
	if (pContentType &&
		_wcsicmp(pContentType->getMediaType(), L"multipart") == 0) {
		string_ptr<WSTRING> wstrBoundary;
		status = pContentType->getParameter(L"boundary", &wstrBoundary);
		CHECK_QSTATUS();
		if (!wstrBoundary.get())
			return QSTATUS_OUTOFMEMORY;
		if (wstrBoundary.get()) {
			string_ptr<STRING> strBoundary(wcs2mbs(wstrBoundary.get()));
			if (!strBoundary.get())
				return QSTATUS_OUTOFMEMORY;
			
			bProcessed = true;
			
			if (!listPart_.empty()) {
				PartList::const_iterator it = listPart_.begin();
				while (it != listPart_.end()) {
					status = buf.append("\r\n--");
					CHECK_QSTATUS();
					status = buf.append(strBoundary.get());
					CHECK_QSTATUS();
					status = buf.append("\r\n");
					CHECK_QSTATUS();
					string_ptr<STRING> strContent;
					status = (*it)->getContent(&strContent);
					CHECK_QSTATUS();
					status = buf.append(strContent.get());
					CHECK_QSTATUS();
					++it;
				}
				status = buf.append("\r\n--");
				CHECK_QSTATUS();
				status = buf.append(strBoundary.get());
				CHECK_QSTATUS();
				status = buf.append("--\r\n");
				CHECK_QSTATUS();
			}
		}
	}
	if (pPartEnclosed_) {
		bProcessed = true;
		string_ptr<STRING> strContent;
		status = pPartEnclosed_->getContent(&strContent);
		CHECK_QSTATUS();
		status = buf.append(strContent.get());
		CHECK_QSTATUS();
	}
	if (!bProcessed) {
		assert(listPart_.empty());
		if (strBody_) {
			status = buf.append(strBody_);
			CHECK_QSTATUS();
		}
	}
	
	*pstrContent = buf.getString();
	
	return QSTATUS_SUCCESS;
}

const CHAR* qs::Part::getHeader() const
{
	return strHeader_;
}

QSTATUS qs::Part::setHeader(const CHAR* pszHeader)
{
	DECLARE_QSTATUS();
	
	string_ptr<STRING> strHeader;
	if (pszHeader) {
		strHeader.reset(allocString(pszHeader));
		if (!strHeader.get())
			return QSTATUS_OUTOFMEMORY;
	}
	
	freeString(strHeader_);
	strHeader_ = strHeader.release();
	clearHeaderLower();
	
	status = updateContentType();
	CHECK_QSTATUS();
	
	return QSTATUS_SUCCESS;
}

QSTATUS qs::Part::getField(const WCHAR* pwszName,
	FieldParser* pParser, Field* pField) const
{
	assert(pwszName);
	assert(pParser);
	assert(pField);
	
	return pParser->parse(*this, pwszName, pField);
}

QSTATUS qs::Part::hasField(const WCHAR* pwszName, bool* pbHas) const
{
	assert(pwszName);
	assert(pbHas);
	
	DECLARE_QSTATUS();
	
	string_ptr<STRING> strName(wcs2mbs(pwszName));
	if (!strName.get())
		return QSTATUS_OUTOFMEMORY;
	
	CHAR* pBegin = 0;
	status = getFieldPos(strName.get(), 0, &pBegin);
	CHECK_QSTATUS();
	*pbHas = pBegin != 0;
	
	return QSTATUS_SUCCESS;
}

QSTATUS qs::Part::setField(const WCHAR* pwszName, const FieldParser& parser)
{
	return setField(pwszName, parser, false, 0);
}

QSTATUS qs::Part::setField(const WCHAR* pwszName,
	const FieldParser& parser, bool* pbSet)
{
	return setField(pwszName, parser, false, pbSet);
}

QSTATUS qs::Part::setField(const WCHAR* pwszName,
	const FieldParser& parser, bool bAllowMultiple)
{
	return setField(pwszName, parser, bAllowMultiple, 0);
}

QSTATUS qs::Part::setField(const WCHAR* pwszName,
	const FieldParser& parser, bool bAllowMultiple, bool* pbSet)
{
	assert(pwszName);
	
	DECLARE_QSTATUS();
	
	if (pbSet)
		*pbSet = false;
	
	if (!bAllowMultiple) {
		bool bHas = false;
		status = hasField(pwszName, &bHas);
		if (bHas)
			return QSTATUS_SUCCESS;
	}
	
	string_ptr<STRING> strName(wcs2mbs(pwszName));
	if (!strName.get())
		return QSTATUS_OUTOFMEMORY;
	
	string_ptr<STRING> strValue;
	status = parser.unparse(*this, &strValue);
	CHECK_QSTATUS();
	
	assert(!strHeader_ ||
		strncmp(strHeader_ + strlen(strHeader_) - 2, "\r\n", 2) == 0);
	
	StringBuffer<STRING> buf(&status);
	CHECK_QSTATUS();
	
	if (strHeader_) {
		status = buf.append(strHeader_);
		CHECK_QSTATUS();
	}
	status = buf.append(strName.get());
	CHECK_QSTATUS();
	status = buf.append(": ");
	CHECK_QSTATUS();
	status = buf.append(strValue.get());
	CHECK_QSTATUS();
	status = buf.append("\r\n");
	CHECK_QSTATUS();
	
	freeString(strHeader_);
	strHeader_ = buf.getString();
	
	clearHeaderLower();
	if (_wcsicmp(pwszName, L"Content-Type") == 0) {
		status = updateContentType();
		CHECK_QSTATUS();
	}
	
	if (pbSet)
		*pbSet = true;
	
	return QSTATUS_SUCCESS;
}

QSTATUS qs::Part::replaceField(const WCHAR* pwszName,
	const FieldParser& parser)
{
	return replaceField(pwszName, parser, 0);
}

QSTATUS qs::Part::replaceField(const WCHAR* pwszName,
	const FieldParser& parser, unsigned int nIndex)
{
	assert(pwszName);
	
	DECLARE_QSTATUS();
	
	if (nIndex == 0xffffffff) {
		status = removeField(pwszName, 0xffffffff);
		CHECK_QSTATUS();
		status = setField(pwszName, parser, true);
		CHECK_QSTATUS();
	}
	else {
		string_ptr<STRING> strName(wcs2mbs(pwszName));
		if (!strName.get())
			return QSTATUS_OUTOFMEMORY;
		
		CHAR* pBegin = 0;
		status = getFieldPos(strName.get(), nIndex, &pBegin);
		CHECK_QSTATUS();
		if (!pBegin) {
			status = setField(pwszName, parser, true);
			CHECK_QSTATUS();
		}
		else {
			CHAR* pEnd = getFieldEndPos(pBegin);
			assert(pEnd);
			
			string_ptr<STRING> strValue;
			status = parser.unparse(*this, &strValue);
			CHECK_QSTATUS();
			
			StringBuffer<STRING> buf(strHeader_, pBegin - strHeader_, &status);
			CHECK_QSTATUS();
			status = buf.append(strName.get());
			CHECK_QSTATUS();
			status = buf.append(": ");
			CHECK_QSTATUS();
			status = buf.append(strValue.get());
			CHECK_QSTATUS();
			status = buf.append("\r\n");
			CHECK_QSTATUS();
			buf.append(pEnd);
			CHECK_QSTATUS();
			
			freeString(strHeader_);
			strHeader_ = buf.getString();
			
			clearHeaderLower();
			if (_wcsicmp(pwszName, L"Content-Type") == 0) {
				status = updateContentType();
				CHECK_QSTATUS();
			}
		}
	}
	
	return QSTATUS_SUCCESS;
}

QSTATUS qs::Part::removeField(const WCHAR* pwszName)
{
	return removeField(pwszName, 0xffffffff, 0);
}

QSTATUS qs::Part::removeField(const WCHAR* pwszName, bool* pbRemove)
{
	return removeField(pwszName, 0xffffffff, pbRemove);
}

QSTATUS qs::Part::removeField(const WCHAR* pwszName, unsigned int nIndex)
{
	return removeField(pwszName, nIndex, 0);
}

QSTATUS qs::Part::removeField(const WCHAR* pwszName,
	unsigned int nIndex, bool* pbRemove)
{
	assert(pwszName);
	
	DECLARE_QSTATUS();
	
	if (pbRemove)
		*pbRemove = false;
	
	if (nIndex == 0xffffffff) {
		bool bRemove = true;
		while (bRemove) {
			status = removeField(pwszName, 0, &bRemove);
			CHECK_QSTATUS();
			if (bRemove && pbRemove)
				*pbRemove = true;
		}
	}
	else {
		string_ptr<STRING> strName(wcs2mbs(pwszName));
		if (!strName.get())
			return QSTATUS_OUTOFMEMORY;
		
		CHAR* pBegin = 0;
		status = getFieldPos(strName.get(), nIndex, &pBegin);
		CHECK_QSTATUS();
		if (pBegin) {
			CHAR* pEnd = getFieldEndPos(pBegin);
			if (pEnd) {
				size_t nLen = strlen(pEnd);
				memmove(pBegin, pEnd, nLen);
				*(pBegin + nLen) = '\0';
				if (*strHeader_ == '\0') {
					freeString(strHeader_);
					strHeader_ = 0;
				}
				clearHeaderLower();
				if (_wcsicmp(pwszName, L"Content-Type") == 0) {
					status = updateContentType();
					CHECK_QSTATUS();
				}
				if (pbRemove)
					*pbRemove = true;
			}
		}
	}
	
	return QSTATUS_SUCCESS;
}

QSTATUS qs::Part::getFields(FieldList* pListField) const
{
	assert(pListField);
	
	DECLARE_QSTATUS();
	
	STLWrapper<FieldList> wrapper(*pListField);
	
	StringBuffer<STRING> buf(&status);
	CHECK_QSTATUS();
	
	for (const CHAR* p = strHeader_; *p; ++p) {
		CHAR c = *p;
		if (c == '\r' && *(p + 1) == '\n' && *(p + 2) != ' ' && *(p + 2) != '\t') {
			string_ptr<STRING> strLine(buf.getString());
			const CHAR* pIndex = strchr(strLine.get(), ':');
			if (pIndex) {
				--pIndex;
				while (*pIndex == ' ' || *pIndex == '\t')
					--pIndex;
				string_ptr<STRING> strName(tolower(strLine.get(), pIndex - strLine.get() + 1));
				if (!strName.get())
					return QSTATUS_OUTOFMEMORY;
				status = wrapper.push_back(std::make_pair(strName.get(), strLine.get()));
				CHECK_QSTATUS();
				strName.release();
				strLine.release();
			}
			++p;
		}
		else {
			status = buf.append(c);
			CHECK_QSTATUS();
		}
	}
	
	return QSTATUS_SUCCESS;
}

QSTATUS qs::Part::sortHeader()
{
	DECLARE_QSTATUS();
	
	FieldList l;
	struct Deleter
	{
		Deleter(Part::FieldList& l) : l_(l) {}
		~Deleter()
		{
			std::for_each(l_.begin(), l_.end(),
				unary_compose_fx_gx(
					string_free<STRING>(),
					string_free<STRING>()));
		}
		Part::FieldList& l_;
	} deleter(l);
	status = getFields(&l);
	CHECK_QSTATUS();
	std::sort(l.begin(), l.end(), FieldComparator());
	
	StringBuffer<STRING> buf(&status);
	CHECK_QSTATUS();
	
	FieldList::iterator it = l.begin();
	while (it != l.end()) {
		status = buf.append((*it).second);
		CHECK_QSTATUS();
		status = buf.append("\r\n");
		CHECK_QSTATUS();
		++it;
	}
	
	freeString(strHeader_);
	strHeader_ = buf.getString();
	clearHeaderLower();
	
	return QSTATUS_SUCCESS;
}

const ContentTypeParser* qs::Part::getContentType() const
{
	return pContentType_;
}

QSTATUS qs::Part::getCharset(WSTRING* pwstrCharset) const
{
	assert(pwstrCharset);
	
	*pwstrCharset = 0;
	
	DECLARE_QSTATUS();
	
	const ContentTypeParser* pContentType = getContentType();
	if (!pContentType) {
		bool bHas = false;
		status = hasField(L"MIME-Version", &bHas);
		CHECK_QSTATUS();
		if (bHas)
			*pwstrCharset = allocWString(L"us-ascii");
		else
			*pwstrCharset = allocWString(getDefaultCharset());
		if (!*pwstrCharset)
			return QSTATUS_OUTOFMEMORY;
	}
	else {
		assert(pContentType->getMediaType());
		if (_wcsicmp(pContentType->getMediaType(), L"text") == 0) {
			status = pContentType->getParameter(L"charset", pwstrCharset);
			CHECK_QSTATUS();
			if (!*pwstrCharset) {
				*pwstrCharset = allocWString(L"us-ascii");
				if (!*pwstrCharset)
					return QSTATUS_OUTOFMEMORY;
			}
		}
	}
	
	return QSTATUS_SUCCESS;
}

bool qs::Part::isMultipart() const
{
	const ContentTypeParser* pContentType = getContentType();
	return pContentType &&
		_wcsicmp(pContentType->getMediaType(), L"multipart") == 0;
}

QSTATUS qs::Part::getRawField(const WCHAR* pwszName,
	unsigned int nIndex, STRING* pstrValue, bool* pbExist) const
{
	assert(pwszName);
	assert(pstrValue);
	assert(pbExist);
	
	DECLARE_QSTATUS();
	
	*pbExist = false;
	*pstrValue = 0;
	
	string_ptr<STRING> strName(wcs2mbs(pwszName));
	if (!strName.get())
		return QSTATUS_OUTOFMEMORY;
	
	CHAR* p = 0;
	status = getFieldPos(strName.get(), nIndex, &p);
	CHECK_QSTATUS();
	if (!p)
		return QSTATUS_SUCCESS;
	
	StringBuffer<STRING> buf(&status);
	CHECK_QSTATUS();
	
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
			status = buf.append(' ');
			CHECK_QSTATUS();
			p += 2;
			while (*(p + 1) == ' ' || *(p + 1) == '\t')
				++p;
		}
		else {
			status = buf.append(c);
			CHECK_QSTATUS();
		}
		++p;
	}
	*pstrValue = buf.getString();
	*pbExist = true;
	
	return QSTATUS_SUCCESS;
}

QSTATUS qs::Part::getHeaderCharset(WSTRING* pwstrCharset) const
{
	assert(pwstrCharset);
	
	*pwstrCharset = 0;
	
	DECLARE_QSTATUS();

	string_ptr<WSTRING> wstrCharset;
	if (pParent_) {
		status = pParent_->getHeaderCharset(&wstrCharset);
		CHECK_QSTATUS();
	}
	else {
		const ContentTypeParser* pContentType = getContentType();
		if (pContentType && _wcsicmp(pContentType->getMediaType(), L"multipart") == 0) {
			if (!listPart_.empty()) {
				status = listPart_.front()->getCharset(&wstrCharset);
				CHECK_QSTATUS();
			}
		}
		else {
			status = getCharset(&wstrCharset);
			CHECK_QSTATUS();
		}
		if (!wstrCharset.get() || _wcsicmp(wstrCharset.get(), L"us-ascii") == 0) {
			wstrCharset.reset(allocWString(getDefaultCharset()));
			if (!wstrCharset.get())
				return QSTATUS_OUTOFMEMORY;
		}
	}
	
	assert(wstrCharset.get());
	*pwstrCharset = wstrCharset.release();
	
	return QSTATUS_SUCCESS;
}

const CHAR* qs::Part::getBody() const
{
	return strBody_;
}

QSTATUS qs::Part::setBody(const CHAR* pszBody, size_t nLen)
{
	if (nLen == static_cast<size_t>(-1))
		nLen = strlen(pszBody);
	
	if (strBody_)
		freeString(strBody_);
	strBody_ = allocString(pszBody, nLen);
	
	return strBody_ ? QSTATUS_SUCCESS : QSTATUS_OUTOFMEMORY;
}

QSTATUS qs::Part::getBodyText(WSTRING* pwstrBodyText) const
{
	return getBodyText(0, pwstrBodyText);
}

QSTATUS qs::Part::getBodyText(const WCHAR* pwszCharset,
	WSTRING* pwstrBodyText) const
{
	assert(pwstrBodyText);
	
	DECLARE_QSTATUS();
	
	*pwstrBodyText = 0;
	
	if (!strBody_) {
		*pwstrBodyText = allocWString(L"");
		if (!*pwstrBodyText)
			return QSTATUS_OUTOFMEMORY;
	}
	else {
		const ContentTypeParser* pContentType = getContentType();
		if (pContentType && _wcsicmp(pContentType->getMediaType(), L"text") != 0)
			return QSTATUS_SUCCESS;
		
		ContentDispositionParser contentDisposition(&status);
		CHECK_QSTATUS();
		Field field;
		status = getField(L"Content-Disposition", &contentDisposition, &field);
		CHECK_QSTATUS();
		if (field == FIELD_EXIST &&
			_wcsicmp(contentDisposition.getDispositionType(), L"attachment") == 0)
			return QSTATUS_SUCCESS;
		
		std::auto_ptr<Encoder> pEncoder;
		ContentTransferEncodingParser contentTransferEncoding(&status);
		CHECK_QSTATUS();
		status = getField(L"Content-Transfer-Encoding",
			&contentTransferEncoding, &field);
		CHECK_QSTATUS();
		if (field == FIELD_EXIST) {
			status = EncoderFactory::getInstance(
				contentTransferEncoding.getEncoding(), &pEncoder);
			CHECK_QSTATUS();
		}
		
		const CHAR* pszDecodedBody = strBody_;
		size_t nDecodedBodyLen = 0;
		malloc_ptr<unsigned char> pDecode;
		if (pEncoder.get()) {
			unsigned char* p = 0;
			status = pEncoder->decode(reinterpret_cast<const unsigned char*>(strBody_),
				strlen(strBody_), &p, &nDecodedBodyLen);
			CHECK_QSTATUS();
			pDecode.reset(p);
			pszDecodedBody = reinterpret_cast<CHAR*>(p);
		}
		else {
			nDecodedBodyLen = strlen(pszDecodedBody);
		}
		
		string_ptr<STRING> strDecode(allocString(nDecodedBodyLen));
		if (!strDecode.get())
			return QSTATUS_OUTOFMEMORY;
		CHAR* pDst = strDecode.get();
		for (size_t n = 0; n < nDecodedBodyLen; ++n) {
			CHAR c = *(pszDecodedBody + n);
			if (c != '\r')
				*pDst++ = c;
		}
		*pDst = '\0';
		
		string_ptr<WSTRING> wstrCharset;
		if (!pwszCharset) {
			status = getCharset(&wstrCharset);
			CHECK_QSTATUS();
			pwszCharset = wstrCharset.get();
		}
		
		std::auto_ptr<Converter> pConverter;
		status = ConverterFactory::getInstance(pwszCharset, &pConverter);
		CHECK_QSTATUS();
		if (!pConverter.get()) {
			status = ConverterFactory::getInstance(getDefaultCharset(), &pConverter);
			CHECK_QSTATUS();
		}
		assert(pConverter.get());
		
		size_t nLen = pDst - strDecode.get();
		status = pConverter->decode(strDecode.get(), &nLen, pwstrBodyText, 0);
		CHECK_QSTATUS();
	}
	
	return QSTATUS_SUCCESS;
}

QSTATUS qs::Part::getBodyData(unsigned char** ppData, size_t* pnLen) const
{
	assert(ppData);
	
	DECLARE_QSTATUS();
	
	std::auto_ptr<Encoder> pEncoder;
	ContentTransferEncodingParser contentTransferEncoding(&status);
	CHECK_QSTATUS();
	Field field;
	status = getField(L"Content-Transfer-Encoding",
		&contentTransferEncoding, &field);
	CHECK_QSTATUS();
	if (field == FIELD_EXIST) {
		status = EncoderFactory::getInstance(
			contentTransferEncoding.getEncoding(), &pEncoder);
		CHECK_QSTATUS();
	}
	
	malloc_ptr<unsigned char> pDecode;
	if (pEncoder.get()) {
		unsigned char* p = 0;
		status = pEncoder->decode(
			reinterpret_cast<const unsigned char*>(strBody_),
			strlen(strBody_), &p, pnLen);
		CHECK_QSTATUS();
		pDecode.reset(p);
	}
	else {
		*pnLen = strlen(strBody_);
		pDecode.reset(static_cast<unsigned char*>(malloc(*pnLen + 1)));
		if (!pDecode.get())
			return QSTATUS_OUTOFMEMORY;
		memcpy(pDecode.get(), strBody_, *pnLen);
	}
	*ppData = pDecode.release();
	
	return QSTATUS_SUCCESS;
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

QSTATUS qs::Part::addPart(Part* pPart)
{
	assert(pPart);
	assert(!strBody_);
	assert(!pPartEnclosed_);
	
	DECLARE_QSTATUS();
	
	status = STLWrapper<PartList>(listPart_).push_back(pPart);
	CHECK_QSTATUS();
	pPart->pParent_ = this;
	
	return QSTATUS_SUCCESS;
}

QSTATUS qs::Part::insertPart(unsigned int n, Part* pPart)
{
	assert(n <= listPart_.size());
	assert(pPart);
	assert(!strBody_);
	assert(!pPartEnclosed_);
	
	DECLARE_QSTATUS();
	
	PartList::iterator it;
	status = STLWrapper<PartList>(listPart_).insert(
		listPart_.begin() + n, pPart, &it);
	CHECK_QSTATUS();
	pPart->pParent_ = this;
	
	return QSTATUS_SUCCESS;
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

const Part* qs::Part::getEnclosedPart() const
{
	return pPartEnclosed_;
}

void qs::Part::setEnclosedPart(Part* pPart)
{
	delete pPartEnclosed_;
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
	return wstrDefaultCharset__;
}

QSTATUS qs::Part::setDefaultCharset(const WCHAR* pwszCharset)
{
	freeWString(wstrDefaultCharset__);
	if (pwszCharset) {
		wstrDefaultCharset__ = allocWString(pwszCharset);
		if (!wstrDefaultCharset__)
			return QSTATUS_OUTOFMEMORY;
	}
	else {
		wstrDefaultCharset__ = 0;
	}
	
	return QSTATUS_SUCCESS;
}

bool qs::Part::isGlobalOption(Option option)
{
	return (nGlobalOptions__ && option) != 0;
}

void qs::Part::setGlobalOptions(unsigned int nOptions)
{
	nGlobalOptions__ = nOptions;
}

QSTATUS qs::Part::getHeaderLower(const CHAR** ppHeader) const
{
	if (!strHeaderLower_) {
		if (strHeader_) {
			string_ptr<STRING> strLower(tolower(strHeader_));
			if (!strLower.get())
				return QSTATUS_OUTOFMEMORY;
			strHeaderLower_ = concat("\r\n", strLower.get());
		}
		else {
			strHeaderLower_ = allocString("\r\n");
		}
		if (!strHeaderLower_)
			return QSTATUS_OUTOFMEMORY;
	}
	
#ifndef NDEBUG
	if (strHeader_) {
		string_ptr<STRING> strLower(tolower(strHeader_));
		if (strLower.get()) {
			string_ptr<STRING> strHeaderLower = concat("\r\n", strLower.get());
			if (strHeaderLower.get())
				assert(strcmp(strHeaderLower.get(), strHeaderLower_) == 0);
		}
	}
#endif
	
	*ppHeader = strHeaderLower_;
	
	return QSTATUS_SUCCESS;
}

void qs::Part::clearHeaderLower() const
{
	freeString(strHeaderLower_);
	strHeaderLower_ = 0;
}

QSTATUS qs::Part::updateContentType()
{
	DECLARE_QSTATUS();
	
	delete pContentType_;
	pContentType_ = 0;
	
	if (strHeader_) {
		std::auto_ptr<ContentTypeParser> pContentType;
		status = newQsObject(&pContentType);
		CHECK_QSTATUS();
		
		Field field;
		status = getField(L"Content-Type", pContentType.get(), &field);
		CHECK_QSTATUS();
		if (field == FIELD_EXIST)
			pContentType_ = pContentType.release();
	}
	
	return QSTATUS_SUCCESS;
}

QSTATUS qs::Part::getFieldPos(const CHAR* pszName,
	unsigned int nIndex, CHAR** ppBegin) const
{
	assert(pszName);
	assert(ppBegin);
	
	DECLARE_QSTATUS();
	
	*ppBegin = 0;
	
	string_ptr<STRING> strNameLower(tolower(pszName));
	if (!strNameLower.get())
		return QSTATUS_OUTOFMEMORY;
	
	string_ptr<STRING> strFind(concat("\r\n", strNameLower.get()));
	if (!strFind.get())
		return QSTATUS_OUTOFMEMORY;
	
	BMFindString<STRING> bmfs(strFind.get(), &status);
	CHECK_QSTATUS();
	const CHAR* pszHeader = 0;
	status = getHeaderLower(&pszHeader);
	CHECK_QSTATUS();
	
	const CHAR* pFind = pszHeader;
	while (true) {
		const CHAR* p = bmfs.find(pFind);
		if (!p)
			return QSTATUS_SUCCESS;
		
		const CHAR* pBegin = p;
		if (nIndex == 0) {
			p += strlen(pszName) + 2;
			while (*p) {
				CHAR c = *p;
				if (c == ':') {
					*ppBegin = const_cast<CHAR*>(strHeader_ + (pBegin - pszHeader));
					return QSTATUS_SUCCESS;
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
				return QSTATUS_SUCCESS;
		}
		else {
			--nIndex;
		}
		pFind = pBegin + 2;
	}
	
	return QSTATUS_SUCCESS;
}

CHAR* qs::Part::getFieldEndPos(const CHAR* pBegin) const
{
	assert(pBegin);
	
	for (const CHAR* p = pBegin; *p; ++p) {
		CHAR c = *p;
		if (c == '\r' && *(p + 1) == '\n' &&
			*(p + 2) != ' ' && *(p + 2) != '\t') {
			p += 2;
			break;
		}
	}
	assert(*(p - 2) == '\r' && *(p - 1) == '\n');
	assert(*p != ' ' && *p != '\t');
	
	return const_cast<CHAR*>(p);
}
