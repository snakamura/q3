/*
 * $Id$
 *
 * Copyright(C) 1998-2003 Satoshi Nakamura
 * All rights reserved.
 *
 */

#include <qmmessage.h>

#include <qsconv.h>
#include <qsencoder.h>
#include <qsnew.h>
#include <qsosutil.h>
#include <qsstl.h>
#include <qsstream.h>
#include <qsstring.h>

#include <algorithm>

#include "message.h"

#pragma warning(disable:4786)

using namespace qm;
using namespace qs;


/****************************************************************************
 *
 * Message
 *
 */

qm::Message::Message(QSTATUS* pstatus) :
	Part(pstatus),
	flag_(FLAG_EMPTY)
{
	if (*pstatus != QSTATUS_SUCCESS)
		return;
}

qm::Message::Message(const CHAR* pwszMessage, size_t nLen,
	Flag flag, QSTATUS* pstatus) :
	Part(0, pwszMessage, nLen, pstatus),
	flag_(flag)
{
	if (*pstatus != QSTATUS_SUCCESS)
		return;
}

qm::Message::~Message()
{
}

QSTATUS qm::Message::create(const CHAR* pszMessage, size_t nLen, Flag flag)
{
	return create(pszMessage, nLen, flag, SECURITY_NONE);
}

QSTATUS qm::Message::create(const CHAR* pszMessage,
	size_t nLen, Flag flag, unsigned int nSecurity)
{
	DECLARE_QSTATUS();
	
	status = Part::create(0, pszMessage, nLen);
	CHECK_QSTATUS();
	
	flag_ = flag;
	nSecurity_ = nSecurity;
	
	return QSTATUS_SUCCESS;
}

QSTATUS qm::Message::clear()
{
	flag_ = FLAG_EMPTY;
	return Part::clear();
}

Message::Flag qm::Message::getFlag() const
{
	return flag_;
}

void qm::Message::setFlag(Flag flag)
{
	flag_ = flag;
}

unsigned int qm::Message::getSecurity() const
{
	return nSecurity_;
}

void qm::Message::setSecurity(unsigned int nSecurity)
{
	nSecurity_ = nSecurity;
}


/****************************************************************************
 *
 * HeaderData
 *
 */

namespace {

struct Header
{
	size_t nLowerName_;
	size_t nName_;
	size_t nValue_;
};

}

/****************************************************************************
 *
 * MessageCreator
 *
 */

qm::MessageCreator::MessageCreator() :
	nFlags_(0)
{
}

qm::MessageCreator::MessageCreator(unsigned int nFlags) :
	nFlags_(nFlags)
{
}

qm::MessageCreator::~MessageCreator()
{
}

unsigned int qm::MessageCreator::getFlags() const
{
	return nFlags_;
}

void qm::MessageCreator::setFlags(unsigned int nFlags, unsigned int nMask)
{
	nFlags_ &= ~nMask;
	nFlags_ |= nFlags & nMask;
}

QSTATUS qm::MessageCreator::createMessage(const WCHAR* pwszMessage,
	size_t nLen, Message** ppMessage) const
{
	DECLARE_QSTATUS();
	
	Part* pPart = 0;
	status = createPart(pwszMessage, nLen, 0, true, &pPart);
	CHECK_QSTATUS();
	
	*ppMessage = static_cast<Message*>(pPart);
	(*ppMessage)->setFlag(Message::FLAG_NONE);
	
	return QSTATUS_SUCCESS;
}

QSTATUS qm::MessageCreator::createPart(const WCHAR* pwszMessage,
	size_t nLen, Part* pParent, bool bMessage, Part** ppPart) const
{
	DECLARE_QSTATUS();
	
	if (nLen == static_cast<size_t>(-1))
		nLen = wcslen(pwszMessage);
	
	std::auto_ptr<Part> pPart;
	if (bMessage) {
		Message* pMessage = 0;
		status = newQsObject(&pMessage);
		CHECK_QSTATUS();
		pPart.reset(pMessage);
	}
	else {
		status = newQsObject(&pPart);
		CHECK_QSTATUS();
	}
	
	const WCHAR* pBody = 0;
	size_t nBodyLen = 0;
	if (*pwszMessage == L'\n') {
		pBody = pwszMessage + 1;
		nBodyLen = nLen - 1;
	}
	else {
		BMFindString<WSTRING> bmfs(L"\n\n", &status);
		CHECK_QSTATUS();
		pBody = bmfs.find(pwszMessage, nLen);
		if (pBody) {
			status = createHeader(pPart.get(), pwszMessage,
				pBody - pwszMessage + 1);
			CHECK_QSTATUS();
			pBody += 2;
			nBodyLen = nLen - (pBody - pwszMessage);
		}
		else {
			StringBuffer<WSTRING> buf(pwszMessage, nLen, &status);
			CHECK_QSTATUS();
			if (*(buf.getCharArray() + buf.getLength() - 1) != L'\n') {
				status = buf.append(L'\n');
				CHECK_QSTATUS();
			}
			status = createHeader(pPart.get(),
				buf.getCharArray(), buf.getLength());
			CHECK_QSTATUS();
		}
	}
	
	const ContentTypeParser* pContentType = pPart->getContentType();
	const WCHAR* pwszMediaType = pContentType ?
		pContentType->getMediaType() : L"text";
	bool bMultipart = wcsicmp(pwszMediaType, L"multipart") == 0;
	bool bAttachment = false;
	status = PartUtil(*pPart).isAttachment(&bAttachment);
	CHECK_QSTATUS();
	
	if (pBody) {
		bool bRFC822 = false;
		if (!bMultipart) {
			bRFC822 = wcsicmp(pwszMediaType, L"message") == 0 &&
				wcsicmp(pContentType->getSubType(), L"rfc822") == 0;
			if (!bRFC822 && !pContentType && pParent) {
				const ContentTypeParser* pParentContentType =
					pParent->getContentType();
				bRFC822 = pParentContentType &&
					wcsicmp(pParentContentType->getMediaType(), L"multipart") == 0 &&
					wcsicmp(pParentContentType->getSubType(), L"digest") == 0;
			}
		}
		if (bMultipart) {
			string_ptr<WSTRING> wstrBoundary;
			status = pContentType->getParameter(L"boundary", &wstrBoundary);
			CHECK_QSTATUS();
			if (!wstrBoundary.get())
				return QSTATUS_FAIL;
			
			BoundaryFinder<WCHAR, WSTRING> finder(pBody - 1,
				nBodyLen + 1, wstrBoundary.get(), L"\n", false, &status);
			CHECK_QSTATUS();
			
			while (true) {
				const WCHAR* pBegin = 0;
				const WCHAR* pEnd = 0;
				bool bEnd = false;
				status = finder.getNext(&pBegin, &pEnd, &bEnd);
				CHECK_QSTATUS();
				if (pBegin) {
					Part* pChild = 0;
					MessageCreator creator;
					status = creator.createPart(pBegin, pEnd - pBegin,
						pPart.get(), false, &pChild);
					CHECK_QSTATUS();
					std::auto_ptr<Part> p(pChild);
					status = pPart->addPart(pChild);
					CHECK_QSTATUS();
					p.release();
				}
				if (bEnd)
					break;
			}
		}
		else if (bRFC822) {
			Part* pEnclosedPart = 0;
			MessageCreator creator;
			status = creator.createPart(pBody,
				nLen - nBodyLen, 0, false, &pEnclosedPart);
			CHECK_QSTATUS();
			pPart->setEnclosedPart(pEnclosedPart);
		}
		else if (wcsicmp(pwszMediaType, L"text") == 0 && !bAttachment) {
			string_ptr<WSTRING> wstrCharset;
			if (pContentType) {
				status = pContentType->getParameter(L"charset", &wstrCharset);
				CHECK_QSTATUS();
			}
			
			std::auto_ptr<Converter> pConverter;
			if (wstrCharset.get()) {
				status = ConverterFactory::getInstance(
					wstrCharset.get(), &pConverter);
				CHECK_QSTATUS();
			}
			if (!pConverter.get()) {
				size_t n = 0;
				while (n < nBodyLen && *(pBody + n) < 0x80)
					++n;
				const WCHAR* pwszCharset = n == nBodyLen ?
					L"us-ascii" : Part::getDefaultCharset();
				status = ConverterFactory::getInstance(
					pwszCharset, &pConverter);
				CHECK_QSTATUS();
				
				if (nFlags_ & FLAG_ADDCONTENTTYPE) {
					ContentTypeParser contentType(L"text", L"plain", &status);
					CHECK_QSTATUS();
					status = contentType.setParameter(L"charset", pwszCharset);
					CHECK_QSTATUS();
					status = pPart->replaceField(L"Content-Type", contentType);
					CHECK_QSTATUS();
					pContentType = pPart->getContentType();
				}
			}
			assert(pConverter.get());
			
			string_ptr<STRING> strBody;
			size_t nEncodedBodyLen = 0;
			status = convertBody(pConverter.get(), pBody,
				nBodyLen, &strBody, &nEncodedBodyLen);
			CHECK_QSTATUS();
			
			std::auto_ptr<Encoder> pEncoder;
			SimpleParser contentTransferEncoding(
				SimpleParser::FLAG_RECOGNIZECOMMENT | SimpleParser::FLAG_TSPECIAL,
				&status);
			CHECK_QSTATUS();
			Part::Field f;
			status = pPart->getField(L"Content-Transfer-Encoding",
				&contentTransferEncoding, &f);
			CHECK_QSTATUS();
			if (f == Part::FIELD_EXIST) {
				const WCHAR* pwszEncoding = contentTransferEncoding.getValue();
				status = EncoderFactory::getInstance(pwszEncoding, &pEncoder);
				CHECK_QSTATUS();
			}
			if (!pEncoder.get()) {
				size_t n = 0;
				while (n < nEncodedBodyLen && *(strBody.get() + n) < 0x80)
					++n;
				const WCHAR* pwszEncoding = 0;
				if (n == nEncodedBodyLen) {
					pwszEncoding = L"7bit";
				}
				else {
					pwszEncoding = L"base64";
					status = EncoderFactory::getInstance(pwszEncoding, &pEncoder);
					CHECK_QSTATUS();
				}
				
				if (nFlags_ & FLAG_ADDCONTENTTYPE) {
					SimpleParser contentTransferEncoding(pwszEncoding,
						SimpleParser::FLAG_RECOGNIZECOMMENT | SimpleParser::FLAG_TSPECIAL,
						&status);
					CHECK_QSTATUS();
					status = pPart->replaceField(L"Content-Transfer-Encoding",
						contentTransferEncoding);
					CHECK_QSTATUS();
				}
			}
			
			if (pEncoder.get()) {
				unsigned char* p = 0;
				size_t n = 0;
				status = pEncoder->encode(
					reinterpret_cast<unsigned char*>(strBody.get()),
					nEncodedBodyLen, &p, &n);
				CHECK_QSTATUS();
				malloc_ptr<unsigned char> pBody(p);
				
				status = pPart->setBody(reinterpret_cast<CHAR*>(pBody.get()), n);
				CHECK_QSTATUS();
			}
			else {
				status = pPart->setBody(strBody.get(), nEncodedBodyLen);
				CHECK_QSTATUS();
			}
		}
		else {
			string_ptr<STRING> strBody;
			status = PartUtil::w2a(pBody, nBodyLen, &strBody);
			CHECK_QSTATUS();
			status = pPart->setBody(strBody.get(), -1);
			CHECK_QSTATUS();
		}
	}
	
	if (nFlags_ & FLAG_ADDCONTENTTYPE) {
		SimpleParser mimeVersion(L"1.0", 0, &status);
		CHECK_QSTATUS();
		status = pPart->replaceField(L"MIME-Version", mimeVersion);
		CHECK_QSTATUS();
	}
	
	if (bMessage && nFlags_ & FLAG_EXTRACTATTACHMENT) {
		XQMAILAttachmentParser attachment(&status);
		CHECK_QSTATUS();
		Part::Field f;
		status = pPart->getField(L"X-QMAIL-Attachment", &attachment, &f);
		CHECK_QSTATUS();
		if (f == Part::FIELD_EXIST) {
			assert(pContentType);
			if (wcsicmp(pContentType->getMediaType(), L"multipart") != 0 ||
				wcsicmp(pContentType->getSubType(), L"mixed") != 0) {
				std::auto_ptr<Message> pParent;
				status = newQsObject(&pParent);
				CHECK_QSTATUS();
				status = makeMultipart(pParent.get(), pPart.get());
				CHECK_QSTATUS();
				pPart.release();
				pPart.reset(pParent.release());
			}
			
			const XQMAILAttachmentParser::AttachmentList& l =
				attachment.getAttachments();
			XQMAILAttachmentParser::AttachmentList::const_iterator it = l.begin();
			while (it != l.end()) {
				Part* p = 0;
				status = createPartFromFile(*it, &p);
				CHECK_QSTATUS();
				std::auto_ptr<Part> pChildPart(p);
				status = pPart->addPart(pChildPart.get());
				CHECK_QSTATUS();
				pChildPart.release();
				++it;
			}
		}
		status = pPart->removeField(L"X-QMAIL-Attachment");
		CHECK_QSTATUS();
	}
	
	*ppPart = pPart.release();
	
	return QSTATUS_SUCCESS;
}

QSTATUS qm::MessageCreator::createHeader(Part* pPart,
	const WCHAR* pwszMessage, size_t nLen) const
{
	assert(pPart);
	assert(pwszMessage);
	assert(*(pwszMessage + nLen - 1) == L'\n');
	
	DECLARE_QSTATUS();
	
	typedef std::vector<Header> HeaderList;
	HeaderList headers;
	
	typedef std::vector<WCHAR> Buffer;
	Buffer buf;
	
	size_t nLine = 0;
	for (size_t n = 0; n < nLen; ++n) {
		WCHAR c = pwszMessage[n];
		assert(c != L'\r');
		if (c == L'\n') {
			if (n == nLen - 1 ||
				(pwszMessage[n + 1] != L' ' && pwszMessage[n + 1] != '\t')) {
				status = STLWrapper<Buffer>(buf).push_back(L'\0');
				CHECK_QSTATUS();
				
				const WCHAR* pLine = &buf[0] + nLine;
				WCHAR* p = wcschr(pLine, L':');
				if (p) {
					Header header;
					
					const WCHAR* pName = pLine;
					WCHAR* pNameEnd = p;
					while (*(pNameEnd - 1) == L' ')
						--pNameEnd;
					*pNameEnd = L'\0';
					const WCHAR* pValue = p + 1;
					while (*pValue == L' ')
						++pValue;
					header.nName_ = nLine;
					header.nValue_ = nLine + (pValue - pLine);
					
					size_t nNameLen = pNameEnd - pName;
					string_ptr<WSTRING> wstrNameLower(tolower(pName, nNameLen));
					if (!wstrNameLower.get())
						return QSTATUS_OUTOFMEMORY;
					header.nLowerName_ = buf.size();
					status = STLWrapper<Buffer>(buf).resize(buf.size() + nNameLen + 1);
					CHECK_QSTATUS();
					std::copy(wstrNameLower.get(), wstrNameLower.get() + nNameLen + 1,
						buf.begin() + header.nLowerName_);
					
					status = STLWrapper<HeaderList>(headers).push_back(header);
					CHECK_QSTATUS();
				}
				nLine = buf.size();
			}
			else {
				++n;
				while (n < nLen) {
					if (pwszMessage[n + 1] != L' ' &&
						pwszMessage[n + 1] != L'\t')
						break;
					++n;
				}
				status = STLWrapper<Buffer>(buf).push_back(L' ');
				CHECK_QSTATUS();
			}
		}
		else {
			status = STLWrapper<Buffer>(buf).push_back(c);
			CHECK_QSTATUS();
		}
	}
	
	const WCHAR* ppwszAddresses[] = {
		L"to",
		L"cc",
		L"bcc",
		L"from",
		L"sender",
		L"reply-to",
		L"errors-to",
		L"resent-to",
		L"resent-cc",
		L"resent-bcc",
		L"resent-from",
		L"resent-sender",
		L"recent-reply-to"
	};
	const WCHAR* ppwszReplaceAliases[] = {
		L"to",
		L"cc",
		L"bcc"
	};
	const WCHAR* ppwszReplaceAliasesResent[] = {
		L"resent-to",
		L"resent-cc",
		L"resent-bcc"
	};
	
	const WCHAR* pBuf = &buf[0];
	HeaderList::iterator it = headers.begin();
	while (it != headers.end()) {
		const WCHAR* pwszName = pBuf + (*it).nName_;
		const WCHAR* pwszNameLower = pBuf + (*it).nLowerName_;
		const WCHAR* pwszValue = pBuf + (*it).nValue_;
		
		if (std::find_if(ppwszAddresses, ppwszAddresses + countof(ppwszAddresses),
			std::bind2nd(string_equal<WCHAR>(), pwszNameLower)) != ppwszAddresses + countof(ppwszAddresses)) {
			if (nFlags_ & FLAG_EXPANDALIAS) {
				// TODO
				// Replace alias
			}
			
			status = setField(pPart, pwszName, pwszValue, FIELDTYPE_ADDRESSLIST);
			CHECK_QSTATUS();
		}
		else if (wcscmp(pwszNameLower, L"content-type") == 0) {
			status = setField(pPart, pwszName, pwszValue, FIELDTYPE_CONTENTTYPE);
			CHECK_QSTATUS();
		}
		else if (wcscmp(pwszNameLower, L"content-transfer-encoding") == 0) {
			status = setField(pPart, pwszName, pwszValue, FIELDTYPE_CONTENTTRANSFERENCODING);
			CHECK_QSTATUS();
		}
		else if (wcscmp(pwszNameLower, L"content-disposition") == 0) {
			status = setField(pPart, pwszName, pwszValue, FIELDTYPE_CONTENTDISPOSITION);
			CHECK_QSTATUS();
		}
		else if (wcscmp(pwszNameLower, L"message-id") == 0) {
			status = setField(pPart, pwszName, pwszValue, FIELDTYPE_MESSAGEID);
			CHECK_QSTATUS();
		}
		else if (wcscmp(pwszNameLower, L"references") == 0 ||
			wcscmp(pwszNameLower, L"in-reply-to") == 0) {
			status = setField(pPart, pwszName, pwszValue, FIELDTYPE_REFERENCES);
			CHECK_QSTATUS();
		}
		else if (wcscmp(pwszNameLower, L"x-qmail-attachment") == 0) {
			status = setField(pPart, pwszName, pwszValue, FIELDTYPE_XQMAILATTACHMENT);
			CHECK_QSTATUS();
		}
		else {
			bool bMulti = wcslen(pwszNameLower) < 9 ||
				wcsncmp(pwszNameLower, L"x-qmail-", 8) != 0;
			status = setField(pPart, pwszName, pwszValue,
				bMulti ? FIELDTYPE_MULTIUNSTRUCTURED : FIELDTYPE_SINGLEUNSTRUCTURED);
			CHECK_QSTATUS();
		}
		
		++it;
	}
	
	return QSTATUS_SUCCESS;
}

QSTATUS qm::MessageCreator::convertBody(
	qs::Converter* pConverter, const WCHAR* pwszBody,
	size_t nBodyLen, qs::STRING* pstrBody, size_t* pnLen) const
{
	assert(pConverter);
	assert(pwszBody);
	assert(pstrBody);
	assert(pnLen);
	
	DECLARE_QSTATUS();
	
	*pstrBody = 0;
	*pnLen = 0;
	
	if (nBodyLen == -1)
		nBodyLen = wcslen(pwszBody);
	
	StringBuffer<STRING> buf(&status);
	CHECK_QSTATUS();
	
	size_t n = 0;
	const WCHAR* p = pwszBody;
	const WCHAR* pBegin = p;
	const WCHAR* pEnd = p + nBodyLen;
	while (true) {
		while (p  < pEnd && *p != L'\n')
			++p;
		size_t nLen = p - pBegin;
		if (nLen > 0) {
			string_ptr<STRING> str;
			size_t nResultLen = 0;
			status = pConverter->encode(pBegin, &nLen, &str, &nResultLen);
			CHECK_QSTATUS();
			status = buf.append(str.get(), nResultLen);
			CHECK_QSTATUS();
		}
		
		if (p == pEnd)
			break;
		
		assert(*p == L'\n');
		status = buf.append("\r\n", 2);
		CHECK_QSTATUS();
		
		++p;
		pBegin = p;
	}
	
	*pnLen = buf.getLength();
	*pstrBody = buf.getString();
	
	return QSTATUS_SUCCESS;
}

QSTATUS qm::MessageCreator::setField(Part* pPart,
	const WCHAR* pwszName, const WCHAR* pwszValue, FieldType type)
{
	assert(pPart);
	assert(pwszName);
	assert(pwszValue);
	
	DECLARE_QSTATUS();
	
	switch (type) {
	case FIELDTYPE_ADDRESSLIST:
		{
			DummyParser field(pwszValue, 0, &status);
			CHECK_QSTATUS();
			Part dummy(&status);
			CHECK_QSTATUS();
			status = dummy.setField(pwszName, field);
			CHECK_QSTATUS();
			AddressListParser addressList(0, &status);
			CHECK_QSTATUS();
			Part::Field f;
			status = dummy.getField(pwszName, &addressList, &f);
			CHECK_QSTATUS();
			if (f != Part::FIELD_EXIST)
				return QSTATUS_FAIL;
			status = pPart->replaceField(pwszName, addressList);
			CHECK_QSTATUS();
		}
		break;
	case FIELDTYPE_CONTENTTYPE:
		{
			DummyParser field(pwszValue, DummyParser::FLAG_TSPECIAL, &status);
			CHECK_QSTATUS();
			status = pPart->setField(pwszName, field);
			CHECK_QSTATUS();
			ContentTypeParser contentType(&status);
			CHECK_QSTATUS();
			Part::Field f;
			status = pPart->getField(pwszName, &contentType, &f);
			CHECK_QSTATUS();
			if (f != Part::FIELD_EXIST)
				return QSTATUS_FAIL;
			status = pPart->replaceField(pwszName, contentType);
			CHECK_QSTATUS();
		}
		break;
	case FIELDTYPE_CONTENTTRANSFERENCODING:
		{
			SimpleParser contentTransferEncoding(pwszValue, false, &status);
			CHECK_QSTATUS();
			status = pPart->replaceField(pwszName, contentTransferEncoding);
			CHECK_QSTATUS();
		}
		break;
	case FIELDTYPE_CONTENTDISPOSITION:
		{
			DummyParser field(pwszValue, DummyParser::FLAG_TSPECIAL, &status);
			CHECK_QSTATUS();
			status = pPart->setField(pwszName, field);
			CHECK_QSTATUS();
			ContentDispositionParser contentDisposition(&status);
			CHECK_QSTATUS();
			Part::Field f;
			status = pPart->getField(pwszName, &contentDisposition, &f);
			CHECK_QSTATUS();
			if (f != Part::FIELD_EXIST)
				return QSTATUS_FAIL;
			status = pPart->replaceField(pwszName, contentDisposition);
			CHECK_QSTATUS();
		}
		break;
	case FIELDTYPE_MESSAGEID:
		{
			DummyParser field(pwszValue, 0, &status);
			CHECK_QSTATUS();
			status = pPart->setField(pwszName, field);
			CHECK_QSTATUS();
			MessageIdParser messageId(&status);
			CHECK_QSTATUS();
			Part::Field f;
			status = pPart->getField(pwszName, &messageId, &f);
			CHECK_QSTATUS();
			if (f != Part::FIELD_EXIST)
				return QSTATUS_FAIL;
			status = pPart->replaceField(pwszName, messageId);
			CHECK_QSTATUS();
		}
		break;
	case FIELDTYPE_REFERENCES:
		{
			DummyParser field(pwszValue, 0, &status);
			CHECK_QSTATUS();
			status = pPart->setField(pwszName, field);
			CHECK_QSTATUS();
			ReferencesParser references(&status);
			CHECK_QSTATUS();
			Part::Field f;
			status = pPart->getField(pwszName, &references, &f);
			CHECK_QSTATUS();
			if (f != Part::FIELD_EXIST)
				return QSTATUS_FAIL;
			status = pPart->replaceField(pwszName, references);
			CHECK_QSTATUS();
		}
		break;
	case FIELDTYPE_XQMAILATTACHMENT:
		{
			DummyParser field(pwszValue, 0, &status);
			CHECK_QSTATUS();
			status = pPart->setField(pwszName, field);
			CHECK_QSTATUS();
			XQMAILAttachmentParser attachment(&status);
			CHECK_QSTATUS();
			Part::Field f;
			status = pPart->getField(pwszName, &attachment, &f);
			CHECK_QSTATUS();
			if (f != Part::FIELD_EXIST)
				return QSTATUS_FAIL;
			status = pPart->replaceField(pwszName, attachment);
			CHECK_QSTATUS();
		}
		break;
	case FIELDTYPE_SINGLEUNSTRUCTURED:
		{
			const WCHAR* pwszCharset = Part::getDefaultCharset();
			UnstructuredParser field(pwszValue, pwszCharset, &status);
			CHECK_QSTATUS();
			status = pPart->replaceField(pwszName, field);
			CHECK_QSTATUS();
		}
		break;
	case FIELDTYPE_MULTIUNSTRUCTURED:
		{
			const WCHAR* pwszCharset = Part::getDefaultCharset();
			UnstructuredParser field(pwszValue, pwszCharset, &status);
			CHECK_QSTATUS();
			status = pPart->setField(pwszName, field, true);
			CHECK_QSTATUS();
		}
		break;
	default:
		assert(false);
		break;
	}
	
	return QSTATUS_SUCCESS;
}

QSTATUS qm::MessageCreator::makeMultipart(Part* pParentPart, Part* pPart)
{
	assert(pParentPart);
	assert(pPart);
	
	DECLARE_QSTATUS();
	
	int n = 0;
	
	Part partTemp(&status);
	CHECK_QSTATUS();
	status = PartUtil(*pPart).copyContentFields(&partTemp);
	CHECK_QSTATUS();
	
	status = pParentPart->setHeader(pPart->getHeader());
	CHECK_QSTATUS();
	const WCHAR* pwszFields[] = {
		L"Content-Transfer-Encoding",
		L"Content-Disposition",
		L"Content-ID",
		L"Content-Description"
	};
	for (n = 0; n < countof(pwszFields); ++n) {
		status = pParentPart->removeField(pwszFields[n]);
		CHECK_QSTATUS();
	}
	ContentTypeParser contentTypeNew(L"multipart", L"mixed", &status);
	CHECK_QSTATUS();
	WCHAR wszBoundary[128];
	Time time(Time::getCurrentTime());
	swprintf(wszBoundary, L"__boundary-%04d%02d%02d%02d%02d%02d%03d%04d__",
		time.wYear, time.wMonth, time.wDay, time.wHour, time.wMinute,
		time.wSecond, time.wMilliseconds, ::GetCurrentThreadId());
	status = contentTypeNew.setParameter(L"boundary", wszBoundary);
	CHECK_QSTATUS();
	status = pParentPart->replaceField(L"Content-Type", contentTypeNew);
	CHECK_QSTATUS();
	
	status = pPart->setHeader(0);
	CHECK_QSTATUS();
	status = PartUtil(partTemp).copyContentFields(pPart);
	CHECK_QSTATUS();
	
	status = pParentPart->addPart(pPart);
	CHECK_QSTATUS();
	
	return QSTATUS_SUCCESS;
}

QSTATUS qm::MessageCreator::createPartFromFile(
	const WCHAR* pwszPath, Part** ppPart)
{
	assert(pwszPath);
	assert(ppPart);
	
	DECLARE_QSTATUS();
	
	*ppPart = 0;
	
	std::auto_ptr<Part> pPart;
	status = newQsObject(&pPart);
	CHECK_QSTATUS();
	
	const WCHAR* pwszContentType = L"application/octet-stream";
	string_ptr<WSTRING> wstrContentType;
	const WCHAR* pFileName = wcsrchr(pwszPath, L'\\');
	pFileName = pFileName ? pFileName + 1 : pwszPath;
	const WCHAR* pExt = wcschr(pFileName, L'.');
	if (pExt) {
		status = getContentTypeFromExtension(pExt + 1, &wstrContentType);
		CHECK_QSTATUS();
		if (wstrContentType.get())
			pwszContentType = wstrContentType.get();
	}
	
	DummyParser dummyContentType(pwszContentType, 0, &status);
	CHECK_QSTATUS();
	status = pPart->setField(L"Content-Type", dummyContentType);
	CHECK_QSTATUS();
	ContentTypeParser contentType(&status);
	CHECK_QSTATUS();
	Part::Field f;
	status = pPart->getField(L"Content-Type", &contentType, &f);
	CHECK_QSTATUS();
	if (f != Part::FIELD_EXIST)
		return QSTATUS_FAIL;
	status = contentType.setParameter(L"name", pFileName);
	CHECK_QSTATUS();
	status = pPart->replaceField(L"Content-Type", contentType);
	CHECK_QSTATUS();
	
	SimpleParser contentTransferEncoding(L"base64",
		SimpleParser::FLAG_RECOGNIZECOMMENT | SimpleParser::FLAG_TSPECIAL,
		&status);
	CHECK_QSTATUS();
	status = pPart->setField(L"Content-Transfer-Encoding", contentTransferEncoding);
	CHECK_QSTATUS();
	
	ContentDispositionParser contentDisposition(L"attachment", &status);
	CHECK_QSTATUS();
	status = contentDisposition.setParameter(L"filename", pFileName);
	CHECK_QSTATUS();
	status = pPart->setField(L"Content-Disposition", contentDisposition);
	CHECK_QSTATUS();
	
	FileInputStream stream(pwszPath, &status);
	CHECK_QSTATUS();
	BufferedInputStream bufferedStream(&stream, false, &status);
	CHECK_QSTATUS();
	StringBuffer<STRING> buf(&status);
	CHECK_QSTATUS();
	unsigned char b[48];
	unsigned char e[64];
	while (true) {
		size_t nLen = 0;
		status = bufferedStream.read(b, countof(b), &nLen);
		CHECK_QSTATUS();
		if (nLen == -1)
			break;
		size_t nEncodedLen = 0;
		Base64Encoder::encode(b, nLen, false, e, &nEncodedLen);
		status = buf.append(reinterpret_cast<CHAR*>(e), nEncodedLen);
		CHECK_QSTATUS();
		status = buf.append("\r\n");
		CHECK_QSTATUS();
	}
	
	status = pPart->setBody(buf.getCharArray(), buf.getLength());
	CHECK_QSTATUS();
	
	*ppPart = pPart.release();
	
	return QSTATUS_SUCCESS;
}

QSTATUS qm::MessageCreator::getContentTypeFromExtension(
	const WCHAR* pwszExtension, WSTRING* pwstrContentType)
{
	assert(pwszExtension);
	assert(pwstrContentType);
	
	DECLARE_QSTATUS();
	
	string_ptr<WSTRING> wstrExt(concat(L".", pwszExtension));
	if (!wstrExt.get())
		return QSTATUS_OUTOFMEMORY;
	
	string_ptr<WSTRING> wstrContentType;
	
	Registry reg(HKEY_CLASSES_ROOT, wstrExt.get(), &status);
	CHECK_QSTATUS();
	if (reg) {
		LONG nRet = 0;
		status = reg.getValue(L"Content Type", &wstrContentType, &nRet);
		CHECK_QSTATUS();
		if (nRet != ERROR_SUCCESS)
			wstrContentType.reset(0);
		if (wstrContentType.get() &&
			wcsnicmp(wstrContentType.get(), L"text/", 5) == 0)
			wstrContentType.reset(0);
	}
	
	*pwstrContentType = wstrContentType.release();
	
	return QSTATUS_SUCCESS;
}


/****************************************************************************
 *
 * PartUtil
 *
 */

qm::PartUtil::PartUtil(const Part& part) :
	part_(part)
{
}

qm::PartUtil::~PartUtil()
{
}

QSTATUS qm::PartUtil::isResent(bool* pbResent) const
{
	assert(pbResent);
	
	DECLARE_QSTATUS();
	
	*pbResent = false;
	
	const WCHAR* pwszFields[] = {
		L"Resent-To",
		L"Resent-Cc",
		L"Resent-Bcc",
		L"Resent-From",
		L"Resent-Date",
		L"Resent-Message-Id"
	};
	
	for (int n = 0; n < countof(pwszFields) && !*pbResent; ++n) {
		bool bHas = false;
		status = part_.hasField(pwszFields[n], &bHas);
		CHECK_QSTATUS();
		if (bHas)
			*pbResent = true;
	}
	
	return QSTATUS_SUCCESS;
}

bool qm::PartUtil::isMultipart() const
{
	return part_.isMultipart();
}

bool qm::PartUtil::isText() const
{
	const ContentTypeParser* pContentType = part_.getContentType();
	return !pContentType || _wcsicmp(pContentType->getMediaType(), L"text") == 0;
}

QSTATUS qm::PartUtil::isAttachment(bool* pbAttachment) const
{
	assert(pbAttachment);
	
	DECLARE_QSTATUS();
	
	*pbAttachment = false;
	
	if (isMultipart())
		return QSTATUS_SUCCESS;
	
	const WCHAR* pwszContentDisposition = 0;
	ContentDispositionParser contentDisposition(&status);
	CHECK_QSTATUS();
	Part::Field field;
	status = part_.getField(L"Content-Disposition", &contentDisposition, &field);
	CHECK_QSTATUS();
	if (field == Part::FIELD_EXIST) {
		pwszContentDisposition = contentDisposition.getDispositionType();
		if (_wcsicmp(pwszContentDisposition, L"attachment") == 0) {
			*pbAttachment = true;
			return QSTATUS_SUCCESS;
		}
		
		string_ptr<WSTRING> wstrFileName;
		status = contentDisposition.getParameter(L"filename", &wstrFileName);
		CHECK_QSTATUS();
		if (wstrFileName.get()) {
			*pbAttachment = true;
			return QSTATUS_SUCCESS;
		}
	}
	else if (field == Part::FIELD_ERROR) {
		*pbAttachment = true;
		return QSTATUS_SUCCESS;
	}
	
	const ContentTypeParser* pContentType = part_.getContentType();
	if (pContentType) {
		const WCHAR* pwszMediaType = pContentType->getMediaType();
		assert(pwszMediaType);
		const WCHAR* pwszSubType = pContentType->getSubType();
		assert(pwszSubType);
		bool bCanInline = _wcsicmp(pwszMediaType, L"text") == 0 ||
			(_wcsicmp(pwszMediaType, L"message") == 0 &&
				_wcsicmp(pwszSubType, L"rfc822") == 0);
		if (!bCanInline) {
			*pbAttachment = true;
			return QSTATUS_SUCCESS;
		}
		else if (pwszContentDisposition &&
			_wcsicmp(pwszContentDisposition, L"inline") == 0 &&
			_wcsicmp(pwszMediaType, L"text") == 0 &&
			_wcsicmp(pwszSubType, L"plain") == 0) {
			return QSTATUS_SUCCESS;
		}
		else {
			string_ptr<WSTRING> wstrName;
			status = pContentType->getParameter(L"name", &wstrName);
			CHECK_QSTATUS();
			if (wstrName.get()) {
				*pbAttachment = true;
				return QSTATUS_SUCCESS;
			}
		}
	
	}
	
	return QSTATUS_SUCCESS;
}

QSTATUS qm::PartUtil::getNames(
	const WCHAR* pwszField, WSTRING* pwstrNames) const
{
	assert(pwszField);
	assert(pwstrNames);
	
	DECLARE_QSTATUS();
	
	*pwstrNames = 0;
	
	StringBuffer<WSTRING> buf(&status);
	CHECK_QSTATUS();
	
	AddressListParser address(0, &status);
	CHECK_QSTATUS();
	Part::Field field;
	status = part_.getField(pwszField, &address, &field);
	CHECK_QSTATUS();
	if (field == Part::FIELD_EXIST) {
		const AddressListParser::AddressList& l = address.getAddressList();
		AddressListParser::AddressList::const_iterator it = l.begin();
		while (it != l.end()) {
			if (it != l.begin()) {
				status = buf.append(L", ");
				CHECK_QSTATUS();
			}
			
			if ((*it)->getPhrase()) {
				status = buf.append((*it)->getPhrase());
				CHECK_QSTATUS();
			}
			else {
				string_ptr<WSTRING> wstrAddress;
				status = (*it)->getAddress(&wstrAddress);
				CHECK_QSTATUS();
				status = buf.append(wstrAddress.get());
				CHECK_QSTATUS();
			}
			
			++it;
		}
	}
	*pwstrNames = buf.getString();
	
	return QSTATUS_SUCCESS;
}

QSTATUS qm::PartUtil::getReference(WSTRING* pwstrReference) const
{
	assert(pwstrReference);
	
	DECLARE_QSTATUS();
	
	*pwstrReference = 0;
	
	const WCHAR* pwszFields[] = {
		L"In-Reply-To",
		L"References"
	};
	for (int n = 0; n < countof(pwszFields) && !*pwstrReference; ++n) {
		ReferencesParser references(&status);
		CHECK_QSTATUS();
		Part::Field field;
		status = part_.getField(pwszFields[n], &references, &field);
		CHECK_QSTATUS();
		if (field == Part::FIELD_EXIST) {
			const ReferencesParser::ReferenceList& l = references.getReferences();
			ReferencesParser::ReferenceList::const_reverse_iterator it = l.rbegin();
			while (it != l.rend() && !*pwstrReference) {
				if ((*it).second == ReferencesParser::T_MSGID) {
					*pwstrReference = allocWString((*it).first);
					if (!*pwstrReference)
						return QSTATUS_OUTOFMEMORY;
				}
				++it;
			}
		}
	}
	
	return QSTATUS_SUCCESS;
}

QSTATUS qm::PartUtil::getReferences(ReferenceList* pList) const
{
	assert(pList);
	
	DECLARE_QSTATUS();
	
	STLWrapper<ReferenceList> wrapper(*pList);
	
	ReferencesParser references(&status);
	CHECK_QSTATUS();
	Part::Field field;
	status = part_.getField(L"References", &references, &field);
	CHECK_QSTATUS();
	if (field == Part::FIELD_EXIST) {
		const ReferencesParser::ReferenceList& l = references.getReferences();
		ReferencesParser::ReferenceList::const_iterator it = l.begin();
		while (it != l.end()) {
			if ((*it).second == ReferencesParser::T_MSGID) {
				string_ptr<WSTRING> wstr(allocWString((*it).first));
				if (!wstr.get())
					return QSTATUS_OUTOFMEMORY;
				status = wrapper.push_back(wstr.get());
				CHECK_QSTATUS();
				wstr.release();
			}
			++it;
		}
	}
	
	return QSTATUS_SUCCESS;
}

QSTATUS qm::PartUtil::getAllText(const WCHAR* pwszQuote,
	const WCHAR* pwszCharset, bool bBodyOnly, WSTRING* pwstrText) const
{
	assert(pwstrText);
	
	DECLARE_QSTATUS();
	
	*pwstrText = 0;
	
	StringBuffer<WSTRING> buf(&status);
	CHECK_QSTATUS();
	
	if (!bBodyOnly) {
		if (part_.getHeader()) {
			string_ptr<WSTRING> wstrHeader;
			status = a2w(part_.getHeader(), &wstrHeader);
			CHECK_QSTATUS();
			status = buf.append(wstrHeader.get());
			CHECK_QSTATUS();
		}
		status = buf.append(L"\n");
		CHECK_QSTATUS();
	}
	
	if (isMultipart()) {
		const ContentTypeParser* pContentType = part_.getContentType();
		string_ptr<WSTRING> wstrBoundary;
		status = pContentType->getParameter(L"boundary", &wstrBoundary);
		CHECK_QSTATUS();
		
		const Part::PartList& l = part_.getPartList();
		if (!l.empty()) {
			Part::PartList::const_iterator it = l.begin();
			while (it != l.end()) {
				status = buf.append(L"\n--");
				CHECK_QSTATUS();
				status = buf.append(wstrBoundary.get());
				CHECK_QSTATUS();
				status = buf.append(L"\n");
				CHECK_QSTATUS();
				string_ptr<WSTRING> wstr;
				status = PartUtil(**it).getAllText(
					pwszQuote, pwszCharset, false, &wstr);
				CHECK_QSTATUS();
				status = buf.append(wstr.get());
				CHECK_QSTATUS();
				++it;
			}
			status = buf.append(L"\n--");
			CHECK_QSTATUS();
			status = buf.append(wstrBoundary.get());
			CHECK_QSTATUS();
			status = buf.append(L"--\n");
			CHECK_QSTATUS();
		}
	}
	else if (part_.getEnclosedPart()) {
		string_ptr<WSTRING> wstr;
		status = PartUtil(*part_.getEnclosedPart()).getAllText(
			pwszQuote, pwszCharset, false, &wstr);
		CHECK_QSTATUS();
		status = buf.append(wstr.get());
		CHECK_QSTATUS();
	}
	else {
		bool bAttachment = false;
		status = isAttachment(&bAttachment);
		CHECK_QSTATUS();
		if (!bAttachment) {
			string_ptr<WSTRING> wstrBody;
			status = part_.getBodyText(pwszCharset, &wstrBody);
			CHECK_QSTATUS();
			if (pwszQuote) {
				string_ptr<WSTRING> wstr;
				status = quote(wstrBody.get(), pwszQuote, &wstr);
				CHECK_QSTATUS();
				wstrBody.reset(wstr.release());
			}
			status = buf.append(wstrBody.get());
			CHECK_QSTATUS();
		}
		else {
			string_ptr<WSTRING> wstrBody;
			status = a2w(part_.getBody(), &wstrBody);
			CHECK_QSTATUS();
			status = buf.append(wstrBody.get());
			CHECK_QSTATUS();
		}
	}
	*pwstrText = buf.getString();
	
	return QSTATUS_SUCCESS;
}

QSTATUS qm::PartUtil::getBodyText(const WCHAR* pwszQuote,
	const WCHAR* pwszCharset, WSTRING* pwstrText) const
{
	assert(pwstrText);
	
	DECLARE_QSTATUS();
	
	*pwstrText = 0;
	
	if (isMultipart()) {
		StringBuffer<WSTRING> buf(&status);
		CHECK_QSTATUS();
		
		const ContentTypeParser* pContentType = part_.getContentType();
		assert(pContentType);
		bool bAlternative = _wcsicmp(pContentType->getSubType(), L"alternative") == 0;
		const Part::PartList& l = part_.getPartList();
		Part::PartList::const_iterator it = l.begin();
		while (it != l.end()) {
			string_ptr<WSTRING> wstrBody;
			status = PartUtil(**it).getBodyText(pwszQuote, pwszCharset, &wstrBody);
			CHECK_QSTATUS();
			if (*wstrBody.get()) {
				if (buf.getLength() != 0) {
					if (*(buf.getCharArray() + buf.getLength() - 1) == L'\n' &&
						pwszQuote) {
						status = buf.append(pwszQuote);
						CHECK_QSTATUS();
					}
					status = buf.append(L"\n");
					CHECK_QSTATUS();
					if (pwszQuote) {
						status = buf.append(pwszQuote);
						CHECK_QSTATUS();
					}
					status = buf.append(
						L"------------------------------------"
						L"------------------------------------\n");
					CHECK_QSTATUS();
				}
				status = buf.append(wstrBody.get());
				CHECK_QSTATUS();
				if (bAlternative)
					break;
			}
			++it;
		}
		
		*pwstrText = buf.getString();
	}
	else {
		string_ptr<WSTRING> wstrBody;
		bool bAttachment = false;
		status = isAttachment(&bAttachment);
		if (!bAttachment && part_.getEnclosedPart()) {
			status = PartUtil(*part_.getEnclosedPart()).getFormattedText(
				false, pwszCharset, &wstrBody);
			CHECK_QSTATUS();
		}
		else if (!bAttachment) {
			status = part_.getBodyText(pwszCharset, &wstrBody);
			CHECK_QSTATUS();
		}
		
		if (wstrBody.get()) {
			if (pwszQuote) {
				string_ptr<WSTRING> wstr;
				status = quote(wstrBody.get(), pwszQuote, &wstr);
				CHECK_QSTATUS();
				wstrBody.reset(wstr.release());
			}
			*pwstrText = wstrBody.release();
		}
		else {
			*pwstrText = allocWString(L"");
			if (!*pwstrText)
				return QSTATUS_OUTOFMEMORY;
		}
	}
	
	return QSTATUS_SUCCESS;
}

QSTATUS qm::PartUtil::getFormattedText(bool bUseSendersTimeZone,
	const WCHAR* pwszCharset, WSTRING* pwstrText) const
{
	assert(pwstrText);
	
	DECLARE_QSTATUS();
	
	const WCHAR* pwszFields[] = {
		L"To",
		L"Cc",
		L"From"
	};
	const WCHAR* pwszFieldNames[] = {
		L"To:      ",
		L"Cc:      ",
		L"From:    " 
	};
	
	StringBuffer<WSTRING> buf(&status);
	CHECK_QSTATUS();
	
	Part::Field field;
	
	for (int n = 0; n < countof(pwszFields); ++n) {
		AddressListParser address(0, &status);
		CHECK_QSTATUS();
		status = part_.getField(pwszFields[n], &address, &field);
		CHECK_QSTATUS();
		if (field == Part::FIELD_EXIST) {
			const AddressListParser::AddressList& l = address.getAddressList();
			if (!l.empty()) {
				status = buf.append(pwszFieldNames[n]);
				CHECK_QSTATUS();
				string_ptr<WSTRING> wstrNames;
				status = address.getNames(&wstrNames);
				CHECK_QSTATUS();
				status = buf.append(wstrNames.get());
				CHECK_QSTATUS();
				status = buf.append(L"\n");
				CHECK_QSTATUS();
			}
		}
	}
	
	UnstructuredParser subject(&status);
	CHECK_QSTATUS();
	status = part_.getField(L"Subject", &subject, &field);
	CHECK_QSTATUS();
	if (field == Part::FIELD_EXIST) {
		status = buf.append(L"Subject: ");
		CHECK_QSTATUS();
		status = buf.append(subject.getValue());
		CHECK_QSTATUS();
		status = buf.append(L'\n');
		CHECK_QSTATUS();
	}
	
	DateParser date(&status);
	CHECK_QSTATUS();
	status = part_.getField(L"Date", &date, &field);
	CHECK_QSTATUS();
	if (field == Part::FIELD_EXIST) {
		string_ptr<WSTRING> wstrDate;
		status = date.getTime().format(L"Date:    %Y4/%M0/%D %h:%m:%s\n",
			bUseSendersTimeZone ? Time::FORMAT_ORIGINAL : Time::FORMAT_LOCAL,
			&wstrDate);
		CHECK_QSTATUS();
		status = buf.append(wstrDate.get());
		CHECK_QSTATUS();
	}
	
	AttachmentParser::AttachmentList names;
	AttachmentParser::AttachmentListFree free(names);
	status = AttachmentParser(part_).getAttachments(true, &names);
	CHECK_QSTATUS();
	if (!names.empty()) {
		status = buf.append(L"Attach:  ");
		CHECK_QSTATUS();
		AttachmentParser::AttachmentList::iterator it = names.begin();
		while (it != names.end()) {
			if (it != names.begin()) {
				status = buf.append(L", ");
				CHECK_QSTATUS();
			}
			status = buf.append((*it).first);
			CHECK_QSTATUS();
			++it;
		}
		status = buf.append(L"\n");
		CHECK_QSTATUS();
	}
	
	status = buf.append(L"\n");
	CHECK_QSTATUS();
	string_ptr<WSTRING> wstrBody;
	status = getBodyText(0, pwszCharset, &wstrBody);
	CHECK_QSTATUS();
	status = buf.append(wstrBody.get());
	CHECK_QSTATUS();
	
	*pwstrText = buf.getString();
	
	return QSTATUS_SUCCESS;
}

QSTATUS qm::PartUtil::getDigest(MessageList* pList) const
{
	assert(pList);
	
	DECLARE_QSTATUS();
	
	struct Deleter
	{
		Deleter(MessageList* p) :
			p_(p)
		{
		}
		
		~Deleter()
		{
			if (p_) {
				std::for_each(p_->begin(), p_->end(), deleter<Message>());
				p_->clear();
			}
		}
		
		void release()
		{
			p_ = 0;
		}
		
		MessageList* p_;
	} deleter(pList);
	
	DigestMode mode = DIGEST_NONE;
	status = getDigestMode(&mode);
	CHECK_QSTATUS();
	if (mode == DIGEST_NONE)
		return QSTATUS_SUCCESS;
	
	AddressListParser to(0, &status);
	CHECK_QSTATUS();
	Part::Field fieldTo;
	status = part_.getField(L"To", &to, &fieldTo);
	CHECK_QSTATUS();
	
	AddressListParser replyTo(0, &status);
	CHECK_QSTATUS();
	Part::Field fieldReplyTo;
	status = part_.getField(L"Reply-To", &replyTo, &fieldReplyTo);
	CHECK_QSTATUS();
	
	switch (mode) {
	case DIGEST_MULTIPART:
		{
			const Part::PartList& l = part_.getPartList();
			Part::PartList::const_iterator it = l.begin();
			while (it != l.end()) {
				const Part* pEnclosedPart = (*it)->getEnclosedPart();
				if (pEnclosedPart || PartUtil(**it).isText()) {
					string_ptr<STRING> strContent;
					if (pEnclosedPart) {
						status = pEnclosedPart->getContent(&strContent);
						CHECK_QSTATUS();
					}
					else {
						status = (*it)->getContent(&strContent);
						CHECK_QSTATUS();
					}
					std::auto_ptr<Message> pMessage;
					status = newQsObject(strContent.get(),
						-1, Message::FLAG_NONE, &pMessage);
					CHECK_QSTATUS();
					
					if (fieldTo == Part::FIELD_EXIST) {
						status = pMessage->setField(L"To", to);
						CHECK_QSTATUS();
					}
					if (fieldReplyTo == Part::FIELD_EXIST) {
						status = pMessage->setField(L"Reply-To", replyTo);
						CHECK_QSTATUS();
					}
					status = STLWrapper<MessageList>(
						*pList).push_back(pMessage.get());
					CHECK_QSTATUS();
					pMessage.release();
				}
				++it;
			}
		}
		break;
	case DIGEST_RFC1153:
		{
			BMFindString<WSTRING> bmfsStart(
				L"\n-----------------------------------"
				L"-----------------------------------\n", &status);
			CHECK_QSTATUS();
			const WCHAR* pwszSeparator = L"\n------------------------------\n";
			size_t nSeparatorLen = wcslen(pwszSeparator);
			BMFindString<WSTRING> bmfsSeparator(pwszSeparator, &status);
			CHECK_QSTATUS();
			
			MessageCreator creator;
			
			string_ptr<WSTRING> wstrBody;
			status = part_.getBodyText(&wstrBody);
			CHECK_QSTATUS();
			const WCHAR* p = bmfsStart.find(wstrBody.get());
			while (p) {
				p = wcschr(p + 1, L'\n');
				assert(p);
				++p;
				if (*p == L'\n')
					++p;
				
				const WCHAR* pEnd = bmfsSeparator.find(p);
				if (!pEnd)
					break;
				
				Message* pMessage = 0;
				status = creator.createMessage(p, pEnd - p, &pMessage);
				CHECK_QSTATUS();
				std::auto_ptr<Message> apMessage(pMessage);
				
				if (fieldTo == Part::FIELD_EXIST) {
					status = pMessage->setField(L"To", to);
					CHECK_QSTATUS();
				}
				if (fieldReplyTo == Part::FIELD_EXIST) {
					status = pMessage->setField(L"Reply-To", replyTo);
					CHECK_QSTATUS();
				}
				status = STLWrapper<MessageList>(*pList).push_back(pMessage);
				CHECK_QSTATUS();
				apMessage.release();
				
				if (wcsncmp(pEnd + nSeparatorLen + 1, L"End of ", 7) == 0)
					break;
				p = pEnd;
			}
		}
		break;
	default:
		assert(false);
		break;
	}
	
	deleter.release();
	
	return QSTATUS_SUCCESS;
}

QSTATUS qm::PartUtil::getDigestMode(DigestMode* pMode) const
{
	assert(pMode);
	
	DECLARE_QSTATUS();
	
	*pMode = DIGEST_NONE;
	
	const ContentTypeParser* pContentType = part_.getContentType();
	if (!pContentType || wcsicmp(pContentType->getMediaType(), L"text") == 0) {
		UnstructuredParser subject(&status);
		Part::Field f;
		status = part_.getField(L"Subject", &subject, &f);
		CHECK_QSTATUS();
		if (f != Part::FIELD_EXIST)
			return QSTATUS_SUCCESS;
		
		string_ptr<WSTRING> wstrSubject(tolower(subject.getValue()));
		if (!wstrSubject.get())
			return QSTATUS_OUTOFMEMORY;
		if (!wcsstr(wstrSubject.get(), L"digest"))
			return QSTATUS_SUCCESS;
		
		string_ptr<WSTRING> wstrBody;
		status = part_.getBodyText(&wstrBody);
		CHECK_QSTATUS();
		BMFindString<WSTRING> bmfs(
			L"\n----------------------------------"
			L"------------------------------------\n", &status);
		CHECK_QSTATUS();
		if (bmfs.find(wstrBody.get()))
			*pMode = DIGEST_RFC1153;
	}
	else if (wcsicmp(pContentType->getMediaType(), L"multipart") == 0) {
		if (wcsicmp(pContentType->getSubType(), L"mixed") != 0 &&
			wcsicmp(pContentType->getSubType(), L"digest") != 0)
			return QSTATUS_SUCCESS;
		
		const Part::PartList& l = part_.getPartList();
		Part::PartList::const_iterator it = l.begin();
		while (it != l.end()) {
			bool b = false;
			status = PartUtil(**it).isAttachment(&b);
			CHECK_QSTATUS();
			if (b)
				return QSTATUS_SUCCESS;
			++it;
		}
		*pMode = DIGEST_MULTIPART;
	}
	
	return QSTATUS_SUCCESS;
}

QSTATUS qm::PartUtil::getHeader(const WCHAR* pwszName, STRING* pstrHeader) const
{
	assert(pwszName);
	
	DECLARE_QSTATUS();
	
	string_ptr<STRING> strName(wcs2mbs(pwszName));
	if (!strName.get())
		return QSTATUS_OUTOFMEMORY;
	
	StringBuffer<STRING> buf(&status);
	CHECK_QSTATUS();
	
	unsigned int nIndex = 0;
	bool bExist = true;
	while (bExist) {
		string_ptr<STRING> str;
		status = part_.getRawField(pwszName, nIndex, &str, &bExist);
		CHECK_QSTATUS();
		if (!bExist)
			break;
		
		status = buf.append(strName.get());
		CHECK_QSTATUS();
		status = buf.append(": ");
		CHECK_QSTATUS();
		status = buf.append(str.get());
		CHECK_QSTATUS();
		status = buf.append("\r\n");
		CHECK_QSTATUS();
		
		++nIndex;
	}
	if (buf.getLength() != 0) {
		status = buf.append("\r\n");
		CHECK_QSTATUS();
		*pstrHeader = buf.getString();
	}
	
	return QSTATUS_SUCCESS;
}

QSTATUS qm::PartUtil::getAlternativeContentTypes(ContentTypeList* pList) const
{
	assert(pList);
	
	DECLARE_QSTATUS();
	
	STLWrapper<ContentTypeList> wrapper(*pList);
	
	const ContentTypeParser* pContentType = part_.getContentType();
	if (pContentType && _wcsicmp(pContentType->getMediaType(), L"multipart") == 0) {
		const Part::PartList& l = part_.getPartList();
		if (_wcsicmp(pContentType->getSubType(), L"alternative") == 0) {
			Part::PartList::const_reverse_iterator it = l.rbegin();
			while (it != l.rend()) {
				status = wrapper.push_back((*it)->getContentType());
				CHECK_QSTATUS();
				++it;
			}
		}
		else {
			if (!l.empty()) {
				status = PartUtil(*l.front()).getAlternativeContentTypes(pList);
				CHECK_QSTATUS();
			}
		}
	}
	else {
		status = wrapper.push_back(pContentType);
		CHECK_QSTATUS();
	}
	
	return QSTATUS_SUCCESS;
}

QSTATUS qm::PartUtil::getAlternativePart(const WCHAR* pwszMediaType,
	const WCHAR* pwszSubType, const Part** ppPart) const
{
	assert(pwszMediaType);
	assert(pwszSubType);
	assert(ppPart);
	
	DECLARE_QSTATUS();
	
	*ppPart = 0;
	
	const ContentTypeParser* pContentType = part_.getContentType();
	if (pContentType && _wcsicmp(pContentType->getMediaType(), L"multipart") == 0) {
		const Part::PartList& l = part_.getPartList();
		if (_wcsicmp(pContentType->getSubType(), L"alternative") == 0) {
			Part::PartList::const_reverse_iterator it = l.rbegin();
			while (it != l.rend()) {
				if (isContentType((*it)->getContentType(), pwszMediaType, pwszSubType)) {
					*ppPart = *it;
					break;
				}
				++it;
			}
		}
		else {
			if (!l.empty()) {
				status = PartUtil(*l.front()).getAlternativePart(
					pwszMediaType, pwszSubType, ppPart);
				CHECK_QSTATUS();
			}
		}
	}
	else {
		if (isContentType(pContentType, pwszMediaType, pwszSubType))
			*ppPart = &part_;
	}
	
	return QSTATUS_SUCCESS;
}

QSTATUS qm::PartUtil::getPartByContentId(
	const WCHAR* pwszContentId, const Part** ppPart) const
{
	assert(pwszContentId);
	assert(ppPart);
	
	DECLARE_QSTATUS();
	
	*ppPart = 0;
	
	if (isMultipart()) {
		const Part::PartList& l = part_.getPartList();
		Part::PartList::const_iterator it = l.begin();
		while (it != l.end() && !*ppPart) {
			status = PartUtil(**it).getPartByContentId(pwszContentId, ppPart);
			CHECK_QSTATUS();
			++it;
		}
	}
	else {
		MessageIdParser contentId(&status);
		CHECK_QSTATUS();
		Part::Field field;
		status = part_.getField(L"Content-Id", &contentId, &field);
		CHECK_QSTATUS();
		if (field == Part::FIELD_EXIST &&
			wcscmp(contentId.getMessageId(), pwszContentId) == 0) {
			*ppPart = &part_;
		}
	}
	
	return QSTATUS_SUCCESS;
}

Part* qm::PartUtil::getEnclosingPart(qs::Part* pCandidatePart) const
{
	assert(pCandidatePart);
	
	Part* pPart = 0;
	if (pCandidatePart->isMultipart()) {
		const Part::PartList& l = pCandidatePart->getPartList();
		Part::PartList::const_iterator it = l.begin();
		while (it != l.end() && !pPart) {
			pPart = getEnclosingPart(*it);
			++it;
		}
	}
	else {
		if (pCandidatePart->getEnclosedPart() == &part_)
			pPart = pCandidatePart;
	}
	
	return pPart;
}

QSTATUS qm::PartUtil::copyContentFields(Part* pPart) const
{
	DECLARE_QSTATUS();
	
	ContentTypeParser contentType(&status);
	CHECK_QSTATUS();
	Part::Field fieldContentType;
	status = part_.getField(L"Content-Type", &contentType, &fieldContentType);
	CHECK_QSTATUS();
	SimpleParser contentTransferEncoding(
		SimpleParser::FLAG_RECOGNIZECOMMENT | SimpleParser::FLAG_TSPECIAL,
		&status);
	CHECK_QSTATUS();
	Part::Field fieldContentTransferEncoding;
	status = part_.getField(L"Content-Transfer-Encoding",
		&contentTransferEncoding, &fieldContentTransferEncoding);
	CHECK_QSTATUS();
	ContentDispositionParser contentDisposition(&status);
	CHECK_QSTATUS();
	Part::Field fieldContentDisposition;
	status = part_.getField(L"Content-Disposition",
		&contentDisposition, &fieldContentDisposition);
	CHECK_QSTATUS();
	MessageIdParser contentId(&status);
	CHECK_QSTATUS();
	Part::Field fieldContentId;
	status = part_.getField(L"Content-ID", &contentId, &fieldContentId);
	CHECK_QSTATUS();
	UnstructuredParser contentDescription(&status);
	CHECK_QSTATUS();
	Part::Field fieldContentDescription;
	status = part_.getField(L"Content-Description",
		&contentDescription, &fieldContentDescription);
	CHECK_QSTATUS();
	
	struct {
		const WCHAR* pwszField_;
		FieldParser* pParser_;
		Part::Field field_;
	} fields[] = {
		{ L"Content-Type",				&contentType,				fieldContentType				},
		{ L"Content-Transfer-Encoding",	&contentTransferEncoding,	fieldContentTransferEncoding	},
		{ L"Content-Disposition",		&contentDisposition,		fieldContentDisposition			},
		{ L"Content-ID",				&contentId,					fieldContentId					},
		{ L"Content-Description",		&contentDescription,		fieldContentDescription			}
	};
	for (int n = 0; n < countof(fields); ++n) {
		if (fields[n].field_ == Part::FIELD_EXIST) {
			status = pPart->replaceField(fields[n].pwszField_, *fields[n].pParser_);
			CHECK_QSTATUS();
		}
	}
	
	return QSTATUS_SUCCESS;
}

QSTATUS qm::PartUtil::a2w(const CHAR* psz, WSTRING* pwstr)
{
	return a2w(psz, -1, pwstr);
}

QSTATUS qm::PartUtil::a2w(const CHAR* psz, size_t nLen, WSTRING* pwstr)
{
	assert(pwstr);
	
	if (nLen == static_cast<size_t>(-1))
		nLen = strlen(psz);
	
	string_ptr<WSTRING> wstr(allocWString(nLen + 1));
	if (!wstr.get())
		return QSTATUS_OUTOFMEMORY;
	
	WCHAR* p = wstr.get();
	for (size_t n = 0; n < nLen; ++n) {
		CHAR c = *(psz + n);
		if (c != '\r')
			*p++ = static_cast<WCHAR>(c);
	}
	*p = L'\0';
	
	*pwstr = wstr.release();
	
	return QSTATUS_SUCCESS;
}

QSTATUS qm::PartUtil::w2a(const WCHAR* pwsz, STRING* pstr)
{
	return w2a(pwsz, -1, pstr);
}

QSTATUS qm::PartUtil::w2a(const WCHAR* pwsz, size_t nLen, STRING* pstr)
{
	assert(pstr);
	
	if (nLen == static_cast<size_t>(-1))
		nLen = wcslen(pwsz);
	
	int nCount = std::count(pwsz, pwsz + nLen, L'\n');
	
	string_ptr<STRING> str(allocString(nLen + nCount + 1));
	if (!str.get())
		return QSTATUS_OUTOFMEMORY;
	
	CHAR* p = str.get();
	for (size_t n = 0; n < nLen; ++n) {
		WCHAR c = *(pwsz + n);
		if (c >= 0x80)
			return QSTATUS_FAIL;
		if (c == L'\n')
			*p++ = '\r';
		*p++ = static_cast<CHAR>(c);
	}
	*p = '\0';
	
	*pstr = str.release();
	
	return QSTATUS_SUCCESS;
}

QSTATUS qm::PartUtil::quote(const WCHAR* pwsz,
	const WCHAR* pwszQuote, WSTRING* pwstrQuoted)
{
	assert(pwstrQuoted);
	
	size_t nLen = wcslen(pwsz);
	size_t nQuoteLen = wcslen(pwszQuote);
	
	int nCount = std::count(pwsz, pwsz + nLen, L'\n');
	
	WSTRING wstr = allocWString(nLen + (nCount + 1)*nQuoteLen + 1);
	if (!wstr)
		return QSTATUS_OUTOFMEMORY;
	
	WCHAR* p = wstr;
	wcscpy(p, pwszQuote);
	p += nQuoteLen;
	while (*pwsz) {
		*p++ = *pwsz;
		if (*pwsz == L'\n' && *(pwsz + 1) != L'\0') {
			wcscpy(p, pwszQuote);
			p += nQuoteLen;
		}
		++pwsz;
	}
	*p = L'\0';
	*pwstrQuoted = wstr;
	
	return QSTATUS_SUCCESS;
}

QSTATUS qm::PartUtil::expandNames(const WCHAR** ppwszNames,
	unsigned int nCount, WSTRING* pwstrNames)
{
	assert(ppwszNames);
	assert(pwstrNames);
	
	unsigned int n = 0;
	
	size_t nLen = 0;
	for (n = 0; n < nCount; ++n)
		nLen += wcslen(ppwszNames[n]) + 2;
	
	string_ptr<WSTRING> wstrNames(allocWString(L"", nLen + 1));
	if (!wstrNames.get())
		return QSTATUS_OUTOFMEMORY;
	
	for (n = 0; n < nCount; ++n) {
		if (n != 0)
			wcscat(wstrNames.get(), L", ");
		wcscat(wstrNames.get(), ppwszNames[n]);
	}
	*pwstrNames = wstrNames.release();
	
	return QSTATUS_SUCCESS;
}

bool qm::PartUtil::isContentType(const ContentTypeParser* pContentType,
	const WCHAR* pwszMediaType, const WCHAR* pwszSubType)
{
	if (pContentType)
		return _wcsicmp(pContentType->getMediaType(), pwszMediaType) == 0 &&
			_wcsicmp(pContentType->getSubType(), pwszSubType) == 0;
	else
		return _wcsicmp(pwszMediaType, L"text") == 0 &&
			_wcsicmp(pwszSubType, L"plain") == 0;
}


/****************************************************************************
 *
 * AttachmentParser
 *
 */

qm::AttachmentParser::AttachmentParser(const Part& part) :
	part_(part)
{
}

qm::AttachmentParser::~AttachmentParser()
{
}

QSTATUS qm::AttachmentParser::hasAttachment(bool* pbHas) const
{
	assert(pbHas);
	
	DECLARE_QSTATUS();
	
	*pbHas = false;
	
	PartUtil util(part_);
	if (util.isMultipart()) {
		const Part::PartList& l = part_.getPartList();
		Part::PartList::const_iterator it = l.begin();
		while (it != l.end() && !*pbHas) {
			status = AttachmentParser(**it).hasAttachment(pbHas);
			CHECK_QSTATUS();
			++it;
		}
	}
	else if (part_.getEnclosedPart()) {
		status = AttachmentParser(*part_.getEnclosedPart()).hasAttachment(pbHas);
		CHECK_QSTATUS();
	}
	else {
		status = util.isAttachment(pbHas);
		CHECK_QSTATUS();
	}
	
	return QSTATUS_SUCCESS;
}

QSTATUS qm::AttachmentParser::getName(WSTRING* pwstrName) const
{
	assert(pwstrName);
	
	DECLARE_QSTATUS();
	
	string_ptr<WSTRING> wstrName;
	
	ContentDispositionParser contentDisposition(&status);
	CHECK_QSTATUS();
	Part::Field f;
	status = part_.getField(L"Content-Disposition", &contentDisposition, &f);
	CHECK_QSTATUS();
	if (f == Part::FIELD_EXIST) {
		status = contentDisposition.getParameter(L"filename", &wstrName);
		CHECK_QSTATUS();
	}
	if (!wstrName.get()) {
		const ContentTypeParser* pContentType = part_.getContentType();
		if (pContentType) {
			status = pContentType->getParameter(L"name", &wstrName);
			CHECK_QSTATUS();
		}
	}
	
	*pwstrName = wstrName.release();
	
	return QSTATUS_SUCCESS;
}

QSTATUS qm::AttachmentParser::getAttachments(
	bool bIncludeDeleted, AttachmentList* pList) const
{
	assert(pList);
	
	DECLARE_QSTATUS();
	
	if (!bIncludeDeleted) {
		bool bDeleted = false;
		status = isAttachmentDeleted(&bDeleted);
		CHECK_QSTATUS();
		if (bDeleted)
			return QSTATUS_SUCCESS;
	}
	
	PartUtil util(part_);
	if (util.isMultipart()) {
		const Part::PartList& l = part_.getPartList();
		Part::PartList::const_iterator it = l.begin();
		while (it != l.end()) {
			status = AttachmentParser(**it).getAttachments(bIncludeDeleted, pList);
			CHECK_QSTATUS();
			++it;
		}
	}
	else if (part_.getEnclosedPart()) {
		status = AttachmentParser(*part_.getEnclosedPart()).getAttachments(bIncludeDeleted, pList);
		CHECK_QSTATUS();
	}
	else {
		bool bAttachment = false;
		status = util.isAttachment(&bAttachment);
		CHECK_QSTATUS();
		if (bAttachment) {
			string_ptr<WSTRING> wstrName;
			status = getName(&wstrName);
			CHECK_QSTATUS();
			if (!wstrName.get() || !*wstrName.get()) {
				wstrName.reset(allocWString(L"Untitled"));
				if (!wstrName.get())
					return QSTATUS_OUTOFMEMORY;
			}
			string_ptr<WSTRING> wstrOrigName(allocWString(wstrName.get()));
			if (!wstrOrigName.get())
				return QSTATUS_OUTOFMEMORY;
			
			int n = 1;
			while (true) {
				AttachmentList::iterator it = std::find_if(
					pList->begin(), pList->end(),
					std::bind2nd(
						binary_compose_f_gx_hy(
							string_equal<WCHAR>(),
							std::select1st<AttachmentList::value_type>(),
							std::identity<const WCHAR*>()),
						wstrName.get()));
				if (it == pList->end())
					break;
				
				WCHAR wsz[32];
				swprintf(wsz, L"[%d]", n++);
				wstrName.reset(concat(wstrOrigName.get(), wsz));
				if (!wstrName.get())
					return QSTATUS_OUTOFMEMORY;
			}
			status = STLWrapper<AttachmentList>(*pList).push_back(
				AttachmentList::value_type(wstrName.get(), const_cast<Part*>(&part_)));
			CHECK_QSTATUS();
			wstrName.release();
		}
	}
	
	return QSTATUS_SUCCESS;
}

QSTATUS qm::AttachmentParser::detach(const WCHAR* pwszDir,
	const WCHAR* pwszName, DetachCallback* pCallback, WSTRING* pwstrPath) const
{
	assert(pwszDir);
	assert(pwszName);
	assert(pCallback);
	assert(pwstrPath);
	
	DECLARE_QSTATUS();
	
	*pwstrPath = 0;
	
	StringBuffer<WSTRING> buf(pwszDir, &status);
	CHECK_QSTATUS();
	if (*(pwszDir + (wcslen(pwszDir) - 1)) != L'\\') {
		status = buf.append(L'\\');
		CHECK_QSTATUS();
	}
	status = buf.append(pwszName);
	CHECK_QSTATUS();
	
	string_ptr<WSTRING> wstrPath(buf.getString());
	
	W2T(wstrPath.get(), ptszPath);
	if (::GetFileAttributes(ptszPath) != 0xffffffff) {
		string_ptr<WSTRING> wstr;
		status = pCallback->confirmOverwrite(wstrPath.get(), &wstr);
		CHECK_QSTATUS();
		if (!wstr.get())
			return QSTATUS_SUCCESS;
		wstrPath.reset(wstr.release());
	}
	
	const CHAR* p = part_.getBody();
	size_t nLen = strlen(p);
	const unsigned char* pBody =
		reinterpret_cast<const unsigned char*>(p);
	malloc_ptr<unsigned char> pBuf;
	
	SimpleParser contentTransferEncoding(
		SimpleParser::FLAG_RECOGNIZECOMMENT | SimpleParser::FLAG_TSPECIAL,
		&status);
	CHECK_QSTATUS();
	Part::Field f;
	status = part_.getField(L"Content-Transfer-Encoding",
		&contentTransferEncoding, &f);
	CHECK_QSTATUS();
	if (f == Part::FIELD_EXIST) {
		const WCHAR* pwszEncoding = contentTransferEncoding.getValue();
		if (wcscmp(pwszEncoding, L"7bit") != 0 &&
			wcscmp(pwszEncoding, L"8bit") != 0) {
			std::auto_ptr<Encoder> pEncoder;
			status = EncoderFactory::getInstance(pwszEncoding, &pEncoder);
			CHECK_QSTATUS();
			if (!pEncoder.get())
				return QSTATUS_FAIL;
			unsigned char* p = 0;
			status = pEncoder->decode(pBody, nLen, &p, &nLen);
			CHECK_QSTATUS();
			pBuf.reset(p);
			pBody = pBuf.get();
		}
	}
	
	FileOutputStream stream(wstrPath.get(), &status);
	CHECK_QSTATUS();
	BufferedOutputStream bufferedStream(&stream, false, &status);
	CHECK_QSTATUS();
	status = bufferedStream.write(pBody, nLen);
	CHECK_QSTATUS();
	
	*pwstrPath = wstrPath.release();
	
	return QSTATUS_SUCCESS;
}

QSTATUS qm::AttachmentParser::isAttachmentDeleted(bool* pbDeleted) const
{
	assert(pbDeleted);
	
	DECLARE_QSTATUS();
	
	*pbDeleted = false;
	
	NumberParser field(0, &status);
	CHECK_QSTATUS();
	Part::Field f;
	status = part_.getField(L"X-QMAIL-AttachmentDeleted", &field, &f);
	CHECK_QSTATUS();
	*pbDeleted = f == Part::FIELD_EXIST && field.getValue() != 0;
	
	return QSTATUS_SUCCESS;
}

QSTATUS qm::AttachmentParser::removeAttachments(Part* pPart)
{
	assert(pPart);
	
	DECLARE_QSTATUS();
	
	PartUtil util(*pPart);
	if (util.isMultipart()) {
		const Part::PartList& l = pPart->getPartList();
		Part::PartList::const_iterator it = l.begin();
		while (it != l.end()) {
			status = removeAttachments(*it);
			CHECK_QSTATUS();
			++it;
		}
	}
	else {
		bool bAttachment = false;
		status = util.isAttachment(&bAttachment);
		CHECK_QSTATUS();
		if (bAttachment) {
			status = pPart->setBody("", 0);
			CHECK_QSTATUS();
		}
	}
	
	return QSTATUS_SUCCESS;
}

QSTATUS qm::AttachmentParser::setAttachmentDeleted(Part* pPart)
{
	assert(pPart);
	
	DECLARE_QSTATUS();
	
	NumberParser field(1, 0, &status);
	CHECK_QSTATUS();
	status = pPart->replaceField(L"X-QMAIL-AttachmentDeleted", field);
	CHECK_QSTATUS();
	
	return QSTATUS_SUCCESS;
}


/****************************************************************************
 *
 * AttachmentParser::AttachmentListFree
 *
 */

qm::AttachmentParser::AttachmentListFree::AttachmentListFree(AttachmentList& l) :
	l_(l)
{
}

qm::AttachmentParser::AttachmentListFree::~AttachmentListFree()
{
	free();
}

void qm::AttachmentParser::AttachmentListFree::free()
{
	std::for_each(l_.begin(), l_.end(),
		unary_compose_f_gx(
			string_free<WSTRING>(),
			std::select1st<AttachmentList::value_type>()));
	l_.clear();
}


/****************************************************************************
 *
 * AttachmentParser::DetachCallback
 *
 */

qm::AttachmentParser::DetachCallback::~DetachCallback()
{
}


/****************************************************************************
 *
 * XQMAILAttachmentParser
 *
 */

qm::XQMAILAttachmentParser::XQMAILAttachmentParser(QSTATUS* pstatus)
{
	assert(pstatus);
	*pstatus = QSTATUS_SUCCESS;
}

qm::XQMAILAttachmentParser::XQMAILAttachmentParser(
	const AttachmentList& listAttachment, QSTATUS* pstatus)
{
}

qm::XQMAILAttachmentParser::~XQMAILAttachmentParser()
{
	std::for_each(listAttachment_.begin(),
		listAttachment_.end(), string_free<WSTRING>());
}

const XQMAILAttachmentParser::AttachmentList& qm::XQMAILAttachmentParser::getAttachments() const
{
	return listAttachment_;
}

QSTATUS qm::XQMAILAttachmentParser::parse(const Part& part,
	const WCHAR* pwszName, Part::Field* pField)
{
	assert(pwszName);
	assert(pField);
	
	DECLARE_QSTATUS();
	
	*pField = Part::FIELD_ERROR;
	
	string_ptr<STRING> strValue;
	bool bExist = false;
	status = part.getRawField(pwszName, 0, &strValue, &bExist);
	CHECK_QSTATUS();
	if (!bExist) {
		*pField = Part::FIELD_NOTEXIST;
		return QSTATUS_SUCCESS;
	}
	
	string_ptr<WSTRING> wstrValue;
	status = decode(strValue.get(), -1, &wstrValue, 0);
	CHECK_QSTATUS();
	
	StringBuffer<WSTRING> buf(&status);
	CHECK_QSTATUS();
	const WCHAR* p = wstrValue.get();
	while (true) {
		if (*p == L',' || *p == L'\0') {
			string_ptr<WSTRING> wstrName(buf.getString());
			if (*wstrName.get()) {
				status = STLWrapper<AttachmentList>(
					listAttachment_).push_back(wstrName.get());
				CHECK_QSTATUS();
				wstrName.release();
			}
			
			if (*p == L'\0')
				break;
		}
		else if (*p == L'\\') {
			if (*(p + 1) != L'\0') {
				++p;
				status = buf.append(*p);
				CHECK_QSTATUS();
			}
		}
		else {
			if ((*p != L' ' && *p != L'\t') ||
				buf.getLength() != 0) {
				status = buf.append(*p);
				CHECK_QSTATUS();
			}
		}
		++p;
	}
	
	*pField = Part::FIELD_EXIST;
	
	return QSTATUS_SUCCESS;
}

QSTATUS qm::XQMAILAttachmentParser::unparse(
	const Part& part, STRING* pstrValue) const
{
	assert(pstrValue);
	
	DECLARE_QSTATUS();
	
	string_ptr<WSTRING> wstrValue;
	status = format(listAttachment_, &wstrValue);
	CHECK_QSTATUS();
	
	status = encode(wstrValue.get(), -1, L"utf-8", L"B", false, pstrValue);
	CHECK_QSTATUS();
	
	return QSTATUS_SUCCESS;
}

QSTATUS qm::XQMAILAttachmentParser::format(
	const AttachmentList& l, WSTRING* pwstr)
{
	DECLARE_QSTATUS();
	
	StringBuffer<WSTRING> buf(&status);
	CHECK_QSTATUS();
	AttachmentList::const_iterator it = l.begin();
	while (it != l.end()) {
		if (it != l.begin()) {
			status = buf.append(L",\n ");
			CHECK_QSTATUS();
		}
		if (**it == L' ' || **it == L'\t') {
			status = buf.append(L'\\');
			CHECK_QSTATUS();
		}
		for (const WCHAR* p = *it; *p; ++p) {
			if (*p == L'\\' || *p == L',') {
				status = buf.append(L'\\');
				CHECK_QSTATUS();
			}
			status = buf.append(*p);
			CHECK_QSTATUS();
		}
		
		++it;
	}
	
	*pwstr = buf.getString();
	
	return QSTATUS_SUCCESS;
}
