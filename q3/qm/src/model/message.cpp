/*
 * $Id$
 *
 * Copyright(C) 1998-2004 Satoshi Nakamura
 * All rights reserved.
 *
 */

#include <qmaccount.h>
#include <qmdocument.h>
#include <qmmessage.h>
#include <qmmessageholder.h>
#include <qmsecurity.h>

#include <qsconv.h>
#include <qsencoder.h>
#include <qsfile.h>
#include <qsosutil.h>
#include <qsstl.h>
#include <qsstream.h>
#include <qsstring.h>

#include <algorithm>

#include "message.h"
#include "uri.h"

#pragma warning(disable:4786)

using namespace qm;
using namespace qs;


/****************************************************************************
 *
 * Message
 *
 */

qm::Message::Message() :
	flag_(FLAG_EMPTY)
{
}

qm::Message::~Message()
{
}

bool qm::Message::create(const CHAR* pszMessage,
						 size_t nLen,
						 Flag flag)
{
	return create(pszMessage, nLen, flag, SECURITY_NONE);
}

bool qm::Message::create(const CHAR* pszMessage,
						 size_t nLen,
						 Flag flag,
						 unsigned int nSecurity)
{
	if (!Part::create(0, pszMessage, nLen))
		return false;
	
	flag_ = flag;
	nSecurity_ = nSecurity;
	
	return true;
}

void qm::Message::clear()
{
	flag_ = FLAG_EMPTY;
	Part::clear();
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
	nFlags_(0),
	nSecurityMode_(0)
{
}

qm::MessageCreator::MessageCreator(unsigned int nFlags,
								   unsigned int nSecurityMode) :
	nFlags_(nFlags),
	nSecurityMode_(nSecurityMode)
{
}

qm::MessageCreator::~MessageCreator()
{
}

unsigned int qm::MessageCreator::getFlags() const
{
	return nFlags_;
}

void qm::MessageCreator::setFlags(unsigned int nFlags,
								  unsigned int nMask)
{
	nFlags_ &= ~nMask;
	nFlags_ |= nFlags & nMask;
}

std::auto_ptr<Message> qm::MessageCreator::createMessage(Document* pDocument,
														 const WCHAR* pwszMessage,
														 size_t nLen) const
{
	std::auto_ptr<Part> pPart(createPart(pDocument, pwszMessage, nLen, 0, true));
	if (!pPart.get())
		return std::auto_ptr<Message>(0);
	
	std::auto_ptr<Message> pMessage(static_cast<Message*>(pPart.release()));
	pMessage->setFlag(Message::FLAG_NONE);
	
	return pMessage;
}

std::auto_ptr<Part> qm::MessageCreator::createPart(Document* pDocument,
												   const WCHAR* pwszMessage,
												   size_t nLen,
												   Part* pParent,
												   bool bMessage) const
{
	if (nLen == static_cast<size_t>(-1))
		nLen = wcslen(pwszMessage);
	
	std::auto_ptr<Part> pPart;
	if (bMessage)
		pPart.reset(new Message());
	else
		pPart.reset(new Part());
	
	const WCHAR* pBody = 0;
	size_t nBodyLen = 0;
	if (*pwszMessage == L'\n') {
		pBody = pwszMessage + 1;
		nBodyLen = nLen - 1;
	}
	else {
		BMFindString<WSTRING> bmfs(L"\n\n");
		pBody = bmfs.find(pwszMessage, nLen);
		if (pBody) {
			if (!createHeader(pPart.get(), pwszMessage, pBody - pwszMessage + 1))
				return std::auto_ptr<Part>(0);
			pBody += 2;
			nBodyLen = nLen - (pBody - pwszMessage);
		}
		else {
			StringBuffer<WSTRING> buf(pwszMessage, nLen);
			if (*(buf.getCharArray() + buf.getLength() - 1) != L'\n')
				buf.append(L'\n');
			if (!createHeader(pPart.get(), buf.getCharArray(), buf.getLength()))
				return std::auto_ptr<Part>(0);
		}
	}
	
	const ContentTypeParser* pContentType = pPart->getContentType();
	const WCHAR* pwszMediaType = pContentType ?
		pContentType->getMediaType() : L"text";
	bool bMultipart = wcsicmp(pwszMediaType, L"multipart") == 0;
	
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
			wstring_ptr wstrBoundary(pContentType->getParameter(L"boundary"));
			if (!wstrBoundary.get())
				return std::auto_ptr<Part>(0);
			
			BoundaryFinder<WCHAR, WSTRING> finder(pBody - 1,
				nBodyLen + 1, wstrBoundary.get(), L"\n", false);
			
			while (true) {
				const WCHAR* pBegin = 0;
				const WCHAR* pEnd = 0;
				bool bEnd = false;
				if (!finder.getNext(&pBegin, &pEnd, &bEnd))
					return std::auto_ptr<Part>(0);
				if (pBegin) {
					MessageCreator creator(nFlags_ & FLAG_ENCODETEXT, SECURITYMODE_NONE);
					std::auto_ptr<Part> pChild(creator.createPart(
						pDocument, pBegin, pEnd - pBegin, pPart.get(), false));
					if (!pChild.get())
						return std::auto_ptr<Part>(0);
					pPart->addPart(pChild);
				}
				if (bEnd)
					break;
			}
		}
		else if (bRFC822) {
			MessageCreator creator(nFlags_ & FLAG_ENCODETEXT, SECURITYMODE_NONE);
			std::auto_ptr<Part> pEnclosed(creator.createPart(
				pDocument, pBody, nBodyLen, 0, false));
			if (!pEnclosed.get())
				return std::auto_ptr<Part>(0);
			pPart->setEnclosedPart(pEnclosed);
		}
		else if (_wcsicmp(pwszMediaType, L"text") == 0) {
			wstring_ptr wstrCharset;
			if (pContentType)
				wstrCharset = pContentType->getParameter(L"charset");
			
			std::auto_ptr<Converter> pConverter;
			if (wstrCharset.get())
				pConverter = ConverterFactory::getInstance(wstrCharset.get());
			
			if (!pConverter.get()) {
				size_t n = 0;
				while (n < nBodyLen && *(pBody + n) < 0x80)
					++n;
				const WCHAR* pwszCharset = n == nBodyLen ?
					L"us-ascii" : Part::getDefaultCharset();
				pConverter = ConverterFactory::getInstance(pwszCharset);
				
				if (nFlags_ & FLAG_ADDCONTENTTYPE) {
					ContentTypeParser contentType(L"text", L"plain");
					contentType.setParameter(L"charset", pwszCharset);
					if (!pPart->replaceField(L"Content-Type", contentType))
						return std::auto_ptr<Part>(0);
					pContentType = pPart->getContentType();
				}
			}
			assert(pConverter.get());
			
			xstring_size_ptr strBody(convertBody(pConverter.get(), pBody, nBodyLen));
			if (!strBody.get())
				return std::auto_ptr<Part>(0);
			
			std::auto_ptr<Encoder> pEncoder;
			if (nFlags_ & FLAG_ENCODETEXT) {
				ContentTransferEncodingParser contentTransferEncoding;
				if (pPart->getField(L"Content-Transfer-Encoding", &contentTransferEncoding) == Part::FIELD_EXIST) {
					const WCHAR* pwszEncoding = contentTransferEncoding.getEncoding();
					pEncoder = EncoderFactory::getInstance(pwszEncoding);
				}
				if (!pEncoder.get()) {
					size_t n = 0;
					while (n < strBody.size() && *(strBody.get() + n) < 0x80)
						++n;
					const WCHAR* pwszEncoding = 0;
					if (n == strBody.size()) {
						pwszEncoding = L"7bit";
					}
					else {
						pwszEncoding = L"base64";
						pEncoder = EncoderFactory::getInstance(pwszEncoding);
					}
					
					if (nFlags_ & FLAG_ADDCONTENTTYPE) {
						ContentTransferEncodingParser contentTransferEncoding(pwszEncoding);
						if (!pPart->replaceField(L"Content-Transfer-Encoding", contentTransferEncoding))
							return std::auto_ptr<Part>(0);
					}
				}
			}
			
			if (pEncoder.get()) {
				malloc_size_ptr<unsigned char> pBody(pEncoder->encode(
					reinterpret_cast<unsigned char*>(strBody.get()), strBody.size()));
				if (!pBody.get())
					return std::auto_ptr<Part>(0);
				
				if (!pPart->setBody(reinterpret_cast<CHAR*>(pBody.get()), pBody.size()))
					return std::auto_ptr<Part>(0);
			}
			else {
				pPart->setBody(xstring_ptr(strBody.release()));
			}
		}
		else {
			xstring_ptr strBody(PartUtil::w2a(pBody, nBodyLen));
			if (!strBody.get())
				return std::auto_ptr<Part>(0);
			pPart->setBody(strBody);
		}
	}
	
	if (nFlags_ & FLAG_ADDCONTENTTYPE) {
		SimpleParser mimeVersion(L"1.0", 0);
		if (!pPart->replaceField(L"MIME-Version", mimeVersion))
			return std::auto_ptr<Part>(0);
	}
	
	if (bMessage && nFlags_ & FLAG_EXTRACTATTACHMENT) {
		assert(pDocument);
		
		XQMAILAttachmentParser attachment;
		if (pPart->getField(L"X-QMAIL-Attachment", &attachment) == Part::FIELD_EXIST) {
			assert(pContentType);
			if (wcsicmp(pContentType->getMediaType(), L"multipart") != 0 ||
				wcsicmp(pContentType->getSubType(), L"mixed") != 0) {
				std::auto_ptr<Message> pParent(new Message());
				if (!makeMultipart(pParent.get(), pPart))
					return std::auto_ptr<Part>(0);
				pPart = pParent;
			}
			
			const XQMAILAttachmentParser::AttachmentList& l = attachment.getAttachments();
			if (!attachFileOrURI(pPart.get(), l, pDocument, nSecurityMode_))
				return std::auto_ptr<Part>(0);
		}
		pPart->removeField(L"X-QMAIL-Attachment");
	}
	
	return pPart;
}

bool qm::MessageCreator::createHeader(Part* pPart,
									  const WCHAR* pwszMessage,
									  size_t nLen) const
{
	assert(pPart);
	assert(pwszMessage);
	assert(*(pwszMessage + nLen - 1) == L'\n');
	
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
				buf.push_back(L'\0');
				
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
					wstring_ptr wstrNameLower(tolower(pName, nNameLen));
					header.nLowerName_ = buf.size();
					buf.resize(buf.size() + nNameLen + 1);
					std::copy(wstrNameLower.get(), wstrNameLower.get() + nNameLen + 1,
						buf.begin() + header.nLowerName_);
					
					headers.push_back(header);
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
				buf.push_back(L' ');
			}
		}
		else {
			buf.push_back(c);
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
	for (HeaderList::iterator it = headers.begin(); it != headers.end(); ++it) {
		const WCHAR* pwszName = pBuf + (*it).nName_;
		const WCHAR* pwszNameLower = pBuf + (*it).nLowerName_;
		const WCHAR* pwszValue = pBuf + (*it).nValue_;
		
		if (std::find_if(ppwszAddresses, ppwszAddresses + countof(ppwszAddresses),
			std::bind2nd(string_equal<WCHAR>(), pwszNameLower)) != ppwszAddresses + countof(ppwszAddresses)) {
			if (nFlags_ & FLAG_EXPANDALIAS) {
				// TODO
				// Replace alias
			}
			
			if (!setField(pPart, pwszName, pwszValue, FIELDTYPE_ADDRESSLIST))
				return false;
		}
		else if (wcscmp(pwszNameLower, L"content-type") == 0) {
			if (!setField(pPart, pwszName, pwszValue, FIELDTYPE_CONTENTTYPE))
				return false;
		}
		else if (wcscmp(pwszNameLower, L"content-transfer-encoding") == 0) {
			if (!setField(pPart, pwszName, pwszValue, FIELDTYPE_CONTENTTRANSFERENCODING))
				return false;
		}
		else if (wcscmp(pwszNameLower, L"content-disposition") == 0) {
			if (!setField(pPart, pwszName, pwszValue, FIELDTYPE_CONTENTDISPOSITION))
				return false;
		}
		else if (wcscmp(pwszNameLower, L"message-id") == 0) {
			if (!setField(pPart, pwszName, pwszValue, FIELDTYPE_MESSAGEID))
				return false;
		}
		else if (wcscmp(pwszNameLower, L"references") == 0 ||
			wcscmp(pwszNameLower, L"in-reply-to") == 0) {
			if (!setField(pPart, pwszName, pwszValue, FIELDTYPE_REFERENCES))
				return false;
		}
		else if (wcscmp(pwszNameLower, L"x-qmail-attachment") == 0) {
			if (!setField(pPart, pwszName, pwszValue, FIELDTYPE_XQMAILATTACHMENT))
				return false;
		}
		else {
			bool bMulti = wcslen(pwszNameLower) < 9 ||
				wcsncmp(pwszNameLower, L"x-qmail-", 8) != 0;
			if (!setField(pPart, pwszName, pwszValue,
				bMulti ? FIELDTYPE_MULTIUNSTRUCTURED : FIELDTYPE_SINGLEUNSTRUCTURED))
				return false;
		}
	}
	
	return true;
}

xstring_size_ptr qm::MessageCreator::convertBody(Converter* pConverter,
												 const WCHAR* pwszBody,
												 size_t nBodyLen) const
{
	assert(pConverter);
	assert(pwszBody);
	
	if (nBodyLen == -1)
		nBodyLen = wcslen(pwszBody);
	
	XStringBuffer<XSTRING> buf;
	
	size_t n = 0;
	const WCHAR* p = pwszBody;
	const WCHAR* pBegin = p;
	const WCHAR* pEnd = p + nBodyLen;
	while (true) {
		while (p  < pEnd && *p != L'\n')
			++p;
		size_t nLen = p - pBegin;
		if (nLen > 0) {
			xstring_size_ptr strEncoded(pConverter->encode(pBegin, &nLen));
			if (!strEncoded.get())
				return xstring_size_ptr();
			if (!buf.append(strEncoded.get(), strEncoded.size()))
				return xstring_size_ptr();
		}
		
		if (p == pEnd)
			break;
		
		assert(*p == L'\n');
		if (!buf.append("\r\n", 2))
			return xstring_size_ptr();
		
		++p;
		pBegin = p;
	}
	
	return buf.getXStringSize();
}

bool qm::MessageCreator::setField(Part* pPart,
								  const WCHAR* pwszName,
								  const WCHAR* pwszValue,
								  FieldType type)
{
	assert(pPart);
	assert(pwszName);
	assert(pwszValue);
	
	switch (type) {
	case FIELDTYPE_ADDRESSLIST:
		{
			UTF8Parser field(pwszValue);
			Part dummy;
			if (!dummy.setField(pwszName, field))
				return false;
			AddressListParser addressList(AddressListParser::FLAG_ALLOWUTF8);
			if (dummy.getField(pwszName, &addressList) != Part::FIELD_EXIST)
				return false;
			if (!pPart->replaceField(pwszName, addressList))
				return false;
		}
		break;
	case FIELDTYPE_CONTENTTYPE:
		{
			DummyParser field(pwszValue, DummyParser::FLAG_TSPECIAL);
			if (!pPart->setField(pwszName, field))
				return false;
			ContentTypeParser contentType;
			if (pPart->getField(pwszName, &contentType) != Part::FIELD_EXIST)
				return false;
			if (!pPart->replaceField(pwszName, contentType))
				return false;
		}
		break;
	case FIELDTYPE_CONTENTTRANSFERENCODING:
		{
			ContentTransferEncodingParser contentTransferEncoding(pwszValue);
			if (!pPart->replaceField(pwszName, contentTransferEncoding))
				return false;
		}
		break;
	case FIELDTYPE_CONTENTDISPOSITION:
		{
			DummyParser field(pwszValue, DummyParser::FLAG_TSPECIAL);
			if (!pPart->setField(pwszName, field))
				return false;
			ContentDispositionParser contentDisposition;
			if (pPart->getField(pwszName, &contentDisposition) != Part::FIELD_EXIST)
				return false;
			if (!pPart->replaceField(pwszName, contentDisposition))
				return false;
		}
		break;
	case FIELDTYPE_MESSAGEID:
		{
			DummyParser field(pwszValue, 0);
			if (!pPart->setField(pwszName, field))
				return false;
			MessageIdParser messageId;
			if (pPart->getField(pwszName, &messageId) != Part::FIELD_EXIST)
				return false;
			if (!pPart->replaceField(pwszName, messageId))
				return false;
		}
		break;
	case FIELDTYPE_REFERENCES:
		{
			DummyParser field(pwszValue, 0);
			if (!pPart->setField(pwszName, field))
				return false;
			ReferencesParser references;
			if (pPart->getField(pwszName, &references) != Part::FIELD_EXIST)
				return false;
			if (!pPart->replaceField(pwszName, references))
				return false;
		}
		break;
	case FIELDTYPE_XQMAILATTACHMENT:
		{
			DummyParser field(pwszValue, 0);
			if (!pPart->setField(pwszName, field))
				return false;
			XQMAILAttachmentParser attachment;
			if (pPart->getField(pwszName, &attachment) != Part::FIELD_EXIST)
				return false;
			if (!pPart->replaceField(pwszName, attachment))
				return false;
		}
		break;
	case FIELDTYPE_SINGLEUNSTRUCTURED:
		{
			const WCHAR* pwszCharset = Part::getDefaultCharset();
			UnstructuredParser field(pwszValue, pwszCharset);
			if (!pPart->replaceField(pwszName, field))
				return false;
		}
		break;
	case FIELDTYPE_MULTIUNSTRUCTURED:
		{
			const WCHAR* pwszCharset = Part::getDefaultCharset();
			UnstructuredParser field(pwszValue, pwszCharset);
			if (!pPart->setField(pwszName, field, true))
				return false;
		}
		break;
	default:
		assert(false);
		break;
	}
	
	return true;
}

bool qm::MessageCreator::makeMultipart(Part* pParentPart,
									   std::auto_ptr<Part> pPart)
{
	assert(pParentPart);
	assert(pPart.get());
	
	int n = 0;
	
	Part partTemp;
	if (!PartUtil(*pPart).copyContentFields(&partTemp))
		return false;
	
	if (!pParentPart->setHeader(pPart->getHeader()))
		return false;
	
	PrefixFieldFilter filter("content-");
	if (!pParentPart->removeFields(&filter))
		return false;
	
	ContentTypeParser contentTypeNew(L"multipart", L"mixed");
	WCHAR wszBoundary[128];
	Time time(Time::getCurrentTime());
	swprintf(wszBoundary, L"__boundary-%04d%02d%02d%02d%02d%02d%03d%04d__",
		time.wYear, time.wMonth, time.wDay, time.wHour, time.wMinute,
		time.wSecond, time.wMilliseconds, ::GetCurrentThreadId());
	contentTypeNew.setParameter(L"boundary", wszBoundary);
	if (!pParentPart->replaceField(L"Content-Type", contentTypeNew))
		return false;
	
	if (!pPart->setHeader(0))
		return false;
	if (!PartUtil(partTemp).copyContentFields(pPart.get()))
		return false;
	
	pParentPart->addPart(pPart);
	
	return true;
}

bool qm::MessageCreator::attachFileOrURI(qs::Part* pPart,
										 const AttachmentList& l,
										 Document* pDocument,
										 unsigned int nSecurityMode)
{
	assert(pPart->isMultipart());
	
	wstring_ptr wstrSchemePrefix(concat(URI::getScheme(), L"://"));
	size_t nSchemePrefixLen = wcslen(wstrSchemePrefix.get());
	for (AttachmentList::const_iterator it = l.begin(); it != l.end(); ++it) {
		const WCHAR* pwszAttachment = *it;
		std::auto_ptr<Part> pChildPart;
		if (wcsncmp(pwszAttachment, wstrSchemePrefix.get(), nSchemePrefixLen) == 0) {
			std::auto_ptr<URI> pURI(URI::parse(pwszAttachment));
			if (!pURI.get())
				return false;
			
			MessagePtrLock mpl(pDocument->getMessage(*pURI.get()));
			if (!mpl)
				return false;
			
			Message msg;
			if (!mpl->getMessage(Account::GETMESSAGEFLAG_ALL, 0, nSecurityMode, &msg))
				return false;
			
			const URIFragment& fragment = pURI->getFragment();
			const Part* pPart = fragment.getPart(&msg);
			if (!pPart)
				return false;
			switch (fragment.getType()) {
			case URIFragment::TYPE_NONE:
				pChildPart = createRfc822Part(*pPart, false);
				break;
			case URIFragment::TYPE_MIME:
				pChildPart = createRfc822Part(*pPart, true);
				break;
			case URIFragment::TYPE_BODY:
				pChildPart = createClonedPart(*pPart);
				break;
			case URIFragment::TYPE_HEADER:
				assert(pPart->getEnclosedPart());
				pChildPart = createRfc822Part(*pPart->getEnclosedPart(), true);
				break;
			case URIFragment::TYPE_TEXT:
				assert(pPart->getEnclosedPart());
				pChildPart = createClonedPart(*pPart->getEnclosedPart());
				break;
			default:
				assert(false);
				break;
			}
		}
		else {
			pChildPart = createPartFromFile(pwszAttachment);
		}
		if (!pChildPart.get())
			return 0;
		pPart->addPart(pChildPart);
	}
	return true;
}

std::auto_ptr<Part> qm::MessageCreator::createPartFromFile(const WCHAR* pwszPath)
{
	assert(pwszPath);
	
	std::auto_ptr<Part> pPart(new Part());
	
	const WCHAR* pwszContentType = L"application/octet-stream";
	wstring_ptr wstrContentType;
	const WCHAR* pFileName = wcsrchr(pwszPath, L'\\');
	pFileName = pFileName ? pFileName + 1 : pwszPath;
	const WCHAR* pExt = wcschr(pFileName, L'.');
	if (pExt) {
		wstrContentType = getContentTypeFromExtension(pExt + 1);
		if (wstrContentType.get())
			pwszContentType = wstrContentType.get();
	}
	
	DummyParser dummyContentType(pwszContentType, 0);
	if (!pPart->setField(L"Content-Type", dummyContentType))
		return std::auto_ptr<Part>(0);
	ContentTypeParser contentType;
	if (pPart->getField(L"Content-Type", &contentType) != Part::FIELD_EXIST)
		return std::auto_ptr<Part>(0);
	contentType.setParameter(L"name", pFileName);
	if (!pPart->replaceField(L"Content-Type", contentType))
		return std::auto_ptr<Part>(0);
	
	ContentTransferEncodingParser contentTransferEncoding(L"base64");
	if (!pPart->setField(L"Content-Transfer-Encoding", contentTransferEncoding))
		return std::auto_ptr<Part>(0);
	
	ContentDispositionParser contentDisposition(L"attachment");
	contentDisposition.setParameter(L"filename", pFileName);
	if (!pPart->setField(L"Content-Disposition", contentDisposition))
		return std::auto_ptr<Part>(0);
	
	W2T(pwszPath, ptszPath);
	WIN32_FIND_DATA fd;
	AutoFindHandle hFind(::FindFirstFile(ptszPath, &fd));
	if (!hFind.get())
		return std::auto_ptr<Part>(0);
	size_t nSize = fd.nFileSizeLow;
	hFind.close();
	
	FileInputStream stream(pwszPath);
	if (!stream)
		return std::auto_ptr<Part>(0);
	BufferedInputStream bufferedStream(&stream, false);
	
	Base64Encoder encoder(true);
	XStringOutputStream outputStream;
	if (!outputStream.reserve((nSize/3 + 1)*4 + nSize/45*2))
		return std::auto_ptr<Part>(0);
	if (!encoder.encode(&bufferedStream, &outputStream))
		return std::auto_ptr<Part>(0);
	
	pPart->setBody(outputStream.getXString());
	
	return pPart;
}

std::auto_ptr<Part> qm::MessageCreator::createRfc822Part(const Part& part,
														 bool bHeaderOnly)
{
	std::auto_ptr<Part> pPart(new Part());
	
	ContentTypeParser contentType(L"message", L"rfc822");
	if (!pPart->setField(L"Content-Type", contentType))
		return std::auto_ptr<Part>(0);
	
	wstring_ptr wstrFileName;
	UnstructuredParser subject;
	if (part.getField(L"Subject", &subject) == Part::FIELD_EXIST && *subject.getValue())
		wstrFileName = concat(subject.getValue(), L".msg");
	else
		wstrFileName = allocWString(L"Untitled.msg");
	
	ContentDispositionParser contentDisposition(L"attachment");
	contentDisposition.setParameter(L"filename", wstrFileName.get());
	if (!pPart->setField(L"Content-Disposition", contentDisposition))
		return std::auto_ptr<Part>(0);
	
	if (bHeaderOnly) {
		pPart->setBody(part.getHeader(), -1);
	}
	else {
		xstring_ptr strContent(part.getContent());
		if (!strContent.get())
			return std::auto_ptr<Part>(0);
		pPart->setBody(strContent);
	}
	
	return pPart;
}

std::auto_ptr<Part> qm::MessageCreator::createClonedPart(const Part& part)
{
	std::auto_ptr<Part> pPart(part.clone());
	if (!pPart.get())
		return std::auto_ptr<Part>();
	
	PrefixFieldFilter filter("content-", true);
	if (!pPart->removeFields(&filter))
		return std::auto_ptr<Part>();
	
	return pPart;
}

wstring_ptr qm::MessageCreator::getContentTypeFromExtension(const WCHAR* pwszExtension)
{
	assert(pwszExtension);
	
	wstring_ptr wstrExt(concat(L".", pwszExtension));
	wstring_ptr wstrContentType;
	
	Registry reg(HKEY_CLASSES_ROOT, wstrExt.get());
	if (reg) {
		if (reg.getValue(L"Content Type", &wstrContentType)) {
			if (wstrContentType.get() &&
				wcsnicmp(wstrContentType.get(), L"text/", 5) == 0)
				wstrContentType.reset(0);
		}
	}
	
	return wstrContentType;
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

bool qm::PartUtil::isResent() const
{
	const WCHAR* pwszFields[] = {
		L"Resent-To",
		L"Resent-Cc",
		L"Resent-Bcc",
		L"Resent-From",
		L"Resent-Date",
		L"Resent-Message-Id"
	};
	
	for (int n = 0; n < countof(pwszFields); ++n) {
		if (part_.hasField(pwszFields[n]))
			return true;
	}
	
	return false;
}

wstring_ptr qm::PartUtil::getNames(const WCHAR* pwszField) const
{
	assert(pwszField);
	
	StringBuffer<WSTRING> buf;
	
	AddressListParser address(0);
	if (part_.getField(pwszField, &address) == Part::FIELD_EXIST) {
		const AddressListParser::AddressList& l = address.getAddressList();
		for (AddressListParser::AddressList::const_iterator it = l.begin(); it != l.end(); ++it) {
			if (it != l.begin())
				buf.append(L", ");
			
			if ((*it)->getPhrase()) {
				buf.append((*it)->getPhrase());
			}
			else {
				wstring_ptr wstrAddress((*it)->getAddress());
				buf.append(wstrAddress.get());
			}
		}
	}
	return buf.getString();
}

wstring_ptr qm::PartUtil::getReference() const
{
	const WCHAR* pwszFields[] = {
		L"In-Reply-To",
		L"References"
	};
	for (int n = 0; n < countof(pwszFields); ++n) {
		ReferencesParser references;
		if (part_.getField(pwszFields[n], &references) == Part::FIELD_EXIST) {
			const ReferencesParser::ReferenceList& l = references.getReferences();
			for (ReferencesParser::ReferenceList::const_reverse_iterator it = l.rbegin(); it != l.rend(); ++it) {
				if ((*it).second == ReferencesParser::T_MSGID)
					return allocWString((*it).first);
			}
		}
	}
	
	return 0;
}

void qm::PartUtil::getReferences(ReferenceList* pList) const
{
	assert(pList);
	
	ReferencesParser references;
	if (part_.getField(L"References", &references) == Part::FIELD_EXIST) {
		const ReferencesParser::ReferenceList& l = references.getReferences();
		for (ReferencesParser::ReferenceList::const_iterator it = l.begin(); it != l.end(); ++it) {
			if ((*it).second == ReferencesParser::T_MSGID) {
				wstring_ptr wstr(allocWString((*it).first));
				pList->push_back(wstr.get());
				wstr.release();
			}
		}
	}
}

wxstring_ptr qm::PartUtil::getAllText(const WCHAR* pwszQuote,
									 const WCHAR* pwszCharset,
									 bool bBodyOnly) const
{
	XStringBuffer<WXSTRING> buf;
	if (!getAllText(pwszQuote, pwszCharset, bBodyOnly, &buf))
		return 0;
	return buf.getXString();
}

bool qm::PartUtil::getAllText(const WCHAR* pwszQuote,
							  const WCHAR* pwszCharset,
							  bool bBodyOnly,
							  qs::XStringBuffer<qs::WXSTRING>* pBuf) const
{
	if (!bBodyOnly) {
		if (part_.getHeader()) {
			wxstring_ptr wstrHeader(a2w(part_.getHeader()));
			if (!wstrHeader.get())
				return false;
			if (!pBuf->append(wstrHeader.get()))
				return false;
		}
		if (!pBuf->append(L"\n"))
			return false;
	}
	
	if (part_.isMultipart()) {
		const ContentTypeParser* pContentType = part_.getContentType();
		wstring_ptr wstrBoundary(pContentType->getParameter(L"boundary"));
		
		const Part::PartList& l = part_.getPartList();
		if (!l.empty()) {
			for (Part::PartList::const_iterator it = l.begin(); it != l.end(); ++it) {
				if (!pBuf->append(L"\n--") ||
					!pBuf->append(wstrBoundary.get()) ||
					!pBuf->append(L"\n") ||
					!PartUtil(**it).getAllText(pwszQuote, pwszCharset, false, pBuf))
					return false;
			}
			if (!pBuf->append(L"\n--") ||
				!pBuf->append(wstrBoundary.get()) ||
				!pBuf->append(L"--\n"))
				return false;
		}
	}
	else if (part_.getEnclosedPart()) {
		PartUtil util(*part_.getEnclosedPart());
		if (!util.getAllText(pwszQuote, pwszCharset, false, pBuf))
			return false;
	}
	else {
		if (part_.isText()) {
			if (pwszQuote) {
				wxstring_ptr wstrBody(part_.getBodyText(pwszCharset));
				if (!wstrBody.get())
					return false;
				if (!quote(wstrBody.get(), pwszQuote, pBuf))
					return false;
			}
			else {
				if (!part_.getBodyText(pwszCharset, pBuf))
					return false;
			}
		}
		else {
			wxstring_ptr wstrBody(a2w(part_.getBody()));
			if (!pBuf->append(wstrBody.get()))
				return false;
		}
	}
	return true;
}

wxstring_ptr qm::PartUtil::getBodyText(const WCHAR* pwszQuote,
									   const WCHAR* pwszCharset,
									   bool bForceRfc822Inline) const
{
	XStringBuffer<WXSTRING> buf;
	if (!getBodyText(pwszQuote, pwszCharset, bForceRfc822Inline, &buf))
		return 0;
	return buf.getXString();
}

bool qm::PartUtil::getBodyText(const WCHAR* pwszQuote,
							   const WCHAR* pwszCharset,
							   bool bForceRfc822Inline,
							   XStringBuffer<WXSTRING>* pBuf) const
{
	if (part_.isMultipart()) {
		const ContentTypeParser* pContentType = part_.getContentType();
		assert(pContentType);
		bool bAlternative = _wcsicmp(pContentType->getSubType(), L"alternative") == 0;
		bool bFirst = true;
		const Part::PartList& l = part_.getPartList();
		for (Part::PartList::const_iterator it = l.begin(); it != l.end(); ++it) {
			size_t nLen = pBuf->getLength();
			
			if (bFirst) {
				bFirst = false;
			}
			else {
				if (*(pBuf->getCharArray() + pBuf->getLength() - 1) == L'\n' && pwszQuote) {
					if (!pBuf->append(pwszQuote))
						return false;
				}
				if (!pBuf->append(L"\n"))
					return false;
				if (pwszQuote) {
					if (!pBuf->append(pwszQuote))
						return false;
				}
				if (!pBuf->append(L"------------------------------------"
					L"------------------------------------\n"))
					return false;
			}
			
			size_t nPrevLen = pBuf->getLength();
			if (!PartUtil(**it).getBodyText(pwszQuote, pwszCharset, bForceRfc822Inline, pBuf))
				return false;
			if (pBuf->getLength() == nPrevLen)
				pBuf->remove(nLen, -1);
			
			if (bAlternative)
				break;
		}
	}
	else {
		bool bAttachment = part_.isAttachment();
		if (pwszQuote) {
			wxstring_ptr wstrBody;
			if (part_.getEnclosedPart() && (!bAttachment || bForceRfc822Inline)) {
				PartUtil util(*part_.getEnclosedPart());
				wstrBody = util.getFormattedText(false, pwszCharset, bForceRfc822Inline);
				if (!wstrBody.get())
					return false;
			}
			else if (!bAttachment) {
				wstrBody = part_.getBodyText(pwszCharset);
				if (!wstrBody.get())
					return false;
			}
			
			if (wstrBody.get()) {
				if (!quote(wstrBody.get(), pwszQuote, pBuf))
					return false;
			}
		}
		else {
			if (part_.getEnclosedPart() && (!bAttachment || bForceRfc822Inline)) {
				PartUtil util(*part_.getEnclosedPart());
				if (!util.getFormattedText(false, pwszCharset, bForceRfc822Inline, pBuf))
					return false;
			}
			else if (!bAttachment) {
				if (!part_.getBodyText(pwszCharset, pBuf))
					return false;
			}
		}
	}
	
	return true;
}

wxstring_ptr qm::PartUtil::getFormattedText(bool bUseSendersTimeZone,
											const WCHAR* pwszCharset,
											bool bForceRfc822Inline) const
{
	XStringBuffer<WXSTRING> buf;
	if (!getFormattedText(bUseSendersTimeZone, pwszCharset, bForceRfc822Inline, &buf))
		return 0;
	return buf.getXString();
}

bool qm::PartUtil::getFormattedText(bool bUseSendersTimeZone,
									const WCHAR* pwszCharset,
									bool bForceRfc822Inline,
									XStringBuffer<WXSTRING>* pBuf) const
{
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
	
	for (int n = 0; n < countof(pwszFields); ++n) {
		AddressListParser address(0);
		if (part_.getField(pwszFields[n], &address) == Part::FIELD_EXIST) {
			if (!address.getAddressList().empty()) {
				if (!pBuf->append(pwszFieldNames[n]))
					return false;
				wstring_ptr wstrNames(address.getNames());
				if (!pBuf->append(wstrNames.get()) ||
					!pBuf->append(L"\n"))
					return false;
			}
		}
	}
	
	UnstructuredParser subject;
	if (part_.getField(L"Subject", &subject) == Part::FIELD_EXIST) {
		if (!pBuf->append(L"Subject: ") ||
			!pBuf->append(subject.getValue()) ||
			!pBuf->append(L'\n'))
			return false;
	}
	
	DateParser date;
	if (part_.getField(L"Date", &date) == Part::FIELD_EXIST) {
		wstring_ptr wstrDate(date.getTime().format(L"Date:    %Y4/%M0/%D %h:%m:%s\n",
			bUseSendersTimeZone ? Time::FORMAT_ORIGINAL : Time::FORMAT_LOCAL));
		if (!pBuf->append(wstrDate.get()))
			return false;
	}
	
	AttachmentParser::AttachmentList names;
	AttachmentParser::AttachmentListFree free(names);
	AttachmentParser(part_).getAttachments(true, &names);
	if (!names.empty()) {
		if (!pBuf->append(L"Attach:  "))
			return false;
		for (AttachmentParser::AttachmentList::iterator it = names.begin(); it != names.end(); ++it) {
			if (it != names.begin()) {
				if (!pBuf->append(L", "))
					return false;
			}
			if (!pBuf->append((*it).first))
				return false;
		}
		if (!pBuf->append(L"\n"))
			return false;
	}
	
	if (!pBuf->append(L"\n"))
		return false;
	if (!getBodyText(0, pwszCharset, bForceRfc822Inline, pBuf))
		return false;
	
	return true;
}

bool qm::PartUtil::getDigest(MessageList* pList) const
{
	assert(pList);
	
	struct Deleter
	{
		Deleter(MessageList* p) :
			p_(p)
		{
		}
		
		~Deleter()
		{
			if (p_) {
				std::for_each(p_->begin(), p_->end(), qs::deleter<Message>());
				p_->clear();
			}
		}
		
		void release()
		{
			p_ = 0;
		}
		
		MessageList* p_;
	} deleter(pList);
	
	DigestMode mode = getDigestMode();
	if (mode == DIGEST_NONE)
		return true;
	
	AddressListParser to(0);
	Part::Field fieldTo = part_.getField(L"To", &to);
	
	AddressListParser replyTo(0);
	Part::Field fieldReplyTo = part_.getField(L"Reply-To", &replyTo);
	
	switch (mode) {
	case DIGEST_MULTIPART:
		{
			const Part::PartList& l = part_.getPartList();
			for (Part::PartList::const_iterator it = l.begin(); it != l.end(); ++it) {
				const Part* pEnclosedPart = (*it)->getEnclosedPart();
				if (pEnclosedPart || (*it)->isText()) {
					xstring_ptr strContent;
					if (pEnclosedPart)
						strContent = pEnclosedPart->getContent();
					else
						strContent = (*it)->getContent();
					
					std::auto_ptr<Message> pMessage(new Message());
					if (!pMessage->create(strContent.get(), -1, Message::FLAG_NONE))
						return false;
					
					if (fieldTo == Part::FIELD_EXIST) {
						if (!pMessage->setField(L"To", to))
							return false;
					}
					if (fieldReplyTo == Part::FIELD_EXIST) {
						if (!pMessage->setField(L"Reply-To", replyTo))
							return false;
					}
					pList->push_back(pMessage.get());
					pMessage.release();
				}
			}
		}
		break;
	case DIGEST_RFC1153:
		{
			BMFindString<WSTRING> bmfsStart(
				L"\n-----------------------------------"
				L"-----------------------------------\n");
			const WCHAR* pwszSeparator = L"\n------------------------------\n";
			size_t nSeparatorLen = wcslen(pwszSeparator);
			BMFindString<WSTRING> bmfsSeparator(pwszSeparator);
			
			MessageCreator creator;
			
			wxstring_ptr wstrBody(part_.getBodyText());
			if (!wstrBody.get())
				return false;
			
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
				
				std::auto_ptr<Message> pMessage(creator.createMessage(0, p, pEnd - p));
				
				if (!pMessage->hasField(L"To") && fieldTo == Part::FIELD_EXIST) {
					if (!pMessage->setField(L"To", to))
						return false;
				}
				if (!pMessage->hasField(L"Reply-To") && fieldReplyTo == Part::FIELD_EXIST) {
					if (!pMessage->setField(L"Reply-To", replyTo))
						return false;
				}
				pList->push_back(pMessage.get());
				pMessage.release();
				
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
	
	return true;
}

PartUtil::DigestMode qm::PartUtil::getDigestMode() const
{
	const ContentTypeParser* pContentType = part_.getContentType();
	if (!pContentType || wcsicmp(pContentType->getMediaType(), L"text") == 0) {
		UnstructuredParser subject;
		if (part_.getField(L"Subject", &subject) == Part::FIELD_EXIST) {
			wstring_ptr wstrSubject(tolower(subject.getValue()));
			if (wcsstr(wstrSubject.get(), L"digest")) {
				wxstring_ptr wstrBody = part_.getBodyText();
				if (wstrBody.get()) {
					BMFindString<WSTRING> bmfs(
						L"\n----------------------------------"
						L"------------------------------------\n");
					if (bmfs.find(wstrBody.get()))
						return DIGEST_RFC1153;
				}
			}
		}
	}
	else if (wcsicmp(pContentType->getMediaType(), L"multipart") == 0) {
		if (wcsicmp(pContentType->getSubType(), L"mixed") == 0 ||
			wcsicmp(pContentType->getSubType(), L"digest") == 0) {
			const Part::PartList& l = part_.getPartList();
			Part::PartList::const_iterator it = l.begin();
			while (it != l.end()) {
				if ((*it)->isAttachment())
					break;
				++it;
			}
			if (it == l.end())
				return DIGEST_MULTIPART;
		}
	}
	
	return DIGEST_NONE;
}

string_ptr qm::PartUtil::getHeader(const WCHAR* pwszName) const
{
	assert(pwszName);
	
	string_ptr strName(wcs2mbs(pwszName));
	
	StringBuffer<STRING> buf;
	
	unsigned int nIndex = 0;
	bool bExist = true;
	while (bExist) {
		string_ptr str(part_.getRawField(pwszName, nIndex));
		if (!str.get())
			break;
		
		buf.append(strName.get());
		buf.append(": ");
		buf.append(str.get());
		buf.append("\r\n");
		
		++nIndex;
	}
	if (buf.getLength() == 0)
		return 0;
	
	buf.append("\r\n");
	return buf.getString();
}

void qm::PartUtil::getAlternativeContentTypes(ContentTypeList* pList) const
{
	assert(pList);
	
	const ContentTypeParser* pContentType = part_.getContentType();
	if (pContentType && _wcsicmp(pContentType->getMediaType(), L"multipart") == 0) {
		const Part::PartList& l = part_.getPartList();
		if (_wcsicmp(pContentType->getSubType(), L"alternative") == 0) {
			for (Part::PartList::const_reverse_iterator it = l.rbegin(); it != l.rend(); ++it)
				PartUtil(**it).getAlternativeContentTypes(pList);
		}
		else {
			if (!l.empty())
				PartUtil(*l.front()).getAlternativeContentTypes(pList);
		}
	}
	else {
		pList->push_back(pContentType);
	}
}

const Part* qm::PartUtil::getAlternativePart(const WCHAR* pwszMediaType,
											 const WCHAR* pwszSubType) const
{
	assert(pwszMediaType);
	assert(pwszSubType);
	
	const ContentTypeParser* pContentType = part_.getContentType();
	if (pContentType && _wcsicmp(pContentType->getMediaType(), L"multipart") == 0) {
		const Part::PartList& l = part_.getPartList();
		if (_wcsicmp(pContentType->getSubType(), L"alternative") == 0) {
			for (Part::PartList::const_reverse_iterator it = l.rbegin(); it != l.rend(); ++it) {
				const Part* pPart = PartUtil(**it).getAlternativePart(pwszMediaType, pwszSubType);
				if (pPart)
					return pPart;
			}
		}
		else {
			if (!l.empty())
				return PartUtil(*l.front()).getAlternativePart(pwszMediaType, pwszSubType);
		}
	}
	else {
		if (isContentType(pContentType, pwszMediaType, pwszSubType))
			return &part_;
	}
	
	return 0;
}

const Part* qm::PartUtil::getPartByContentId(const WCHAR* pwszContentId) const
{
	assert(pwszContentId);
	
	if (part_.isMultipart()) {
		const Part::PartList& l = part_.getPartList();
		for (Part::PartList::const_iterator it = l.begin(); it != l.end(); ++it) {
			const Part* pPart = PartUtil(**it).getPartByContentId(pwszContentId);
			if (pPart)
				return pPart;
		}
	}
	else {
		MessageIdParser contentId;
		if (part_.getField(L"Content-Id", &contentId) == Part::FIELD_EXIST &&
			wcscmp(contentId.getMessageId(), pwszContentId) == 0)
			return &part_;
	}
	
	return 0;
}

Part* qm::PartUtil::getEnclosingPart(Part* pCandidatePart) const
{
	assert(pCandidatePart);
	
	Part* pPart = 0;
	if (pCandidatePart->isMultipart()) {
		const Part::PartList& l = pCandidatePart->getPartList();
		for (Part::PartList::const_iterator it = l.begin(); it != l.end() && !pPart; ++it)
			pPart = getEnclosingPart(*it);
	}
	else {
		Part* pEnclosedPart = pCandidatePart->getEnclosedPart();
		if (pEnclosedPart == &part_)
			pPart = pCandidatePart;
		else if (pEnclosedPart)
			pPart = getEnclosingPart(pEnclosedPart);
	}
	
	return pPart;
}

bool qm::PartUtil::copyContentFields(Part* pPart) const
{
	PrefixFieldFilter filter("content-");
	return pPart->copyFields(part_, &filter);
}

wxstring_ptr qm::PartUtil::a2w(const CHAR* psz)
{
	return a2w(psz, -1);
}

wxstring_ptr qm::PartUtil::a2w(const CHAR* psz,
							   size_t nLen)
{
	XStringBuffer<WXSTRING> buf;
	if (!a2w(psz, nLen, &buf))
		return 0;
	return buf.getXString();
}

bool qm::PartUtil::a2w(const CHAR* psz,
					   size_t nLen,
					   XStringBuffer<WXSTRING>* pBuf)
{
	assert(pBuf);
	
	if (nLen == -1)
		nLen = strlen(psz);
	
	for (size_t n = 0; n < nLen; ++n) {
		CHAR c = *(psz + n);
		if (c != '\r') {
			if (!pBuf->append(static_cast<WCHAR>(c)))
				return false;
		}
	}
	
	return true;
}

xstring_ptr qm::PartUtil::w2a(const WCHAR* pwsz)
{
	return w2a(pwsz, -1);
}

xstring_ptr qm::PartUtil::w2a(const WCHAR* pwsz,
							  size_t nLen)
{
	XStringBuffer<XSTRING> buf;
	if (!w2a(pwsz, nLen, &buf))
		return 0;
	return buf.getXString();
}

bool qm::PartUtil::w2a(const WCHAR* pwsz,
					   size_t nLen,
					   XStringBuffer<XSTRING>* pBuf)
{
	assert(pBuf);
	
	if (nLen == -1)
		nLen = wcslen(pwsz);
	
	int nCount = std::count(pwsz, pwsz + nLen, L'\n');
	
	for (size_t n = 0; n < nLen; ++n) {
		WCHAR c = *(pwsz + n);
		if (c >= 0x80)
			return false;
		if (c == L'\n') {
			if (!pBuf->append('\r'))
				return false;
		}
		if (!pBuf->append(static_cast<CHAR>(c)))
			return false;
	}
	
	return true;
}

wxstring_ptr qm::PartUtil::quote(const WCHAR* pwsz,
								 const WCHAR* pwszQuote)
{
	XStringBuffer<WXSTRING> buf;
	if (!quote(pwsz, pwszQuote, &buf))
		return 0;
	return buf.getXString();
}

bool qm::PartUtil::quote(const WCHAR* pwsz,
						 const WCHAR* pwszQuote,
						 XStringBuffer<WXSTRING>* pBuf)
{
	assert(pwsz);
	assert(pwszQuote);
	assert(pBuf);
	
	if (!pBuf->append(pwszQuote))
		return false;
	while (*pwsz) {
		if (!pBuf->append(*pwsz))
			return false;
		if (*pwsz == L'\n' && *(pwsz + 1) != L'\0') {
			if (!pBuf->append(pwszQuote))
				return false;
		}
		++pwsz;
	}
	
	return true;
}

wstring_ptr qm::PartUtil::expandNames(const WCHAR** ppwszNames,
									  unsigned int nCount)
{
	assert(ppwszNames);
	
	StringBuffer<WSTRING> buf;
	for (unsigned int n = 0; n < nCount; ++n) {
		if (n != 0)
			buf.append(L", ");
		buf.append(ppwszNames[n]);
	}
	return buf.getString();
}

bool qm::PartUtil::isContentType(const ContentTypeParser* pContentType,
								 const WCHAR* pwszMediaType,
								 const WCHAR* pwszSubType)
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

bool qm::AttachmentParser::hasAttachment() const
{
	if (part_.isMultipart()) {
		const Part::PartList& l = part_.getPartList();
		for (Part::PartList::const_iterator it = l.begin(); it != l.end(); ++it) {
			if (AttachmentParser(**it).hasAttachment())
				return true;
		}
		return false;
	}
	else if (part_.getEnclosedPart()) {
		return AttachmentParser(*part_.getEnclosedPart()).hasAttachment();
	}
	else {
		return part_.isAttachment();
	}
}

wstring_ptr qm::AttachmentParser::getName() const
{
	wstring_ptr wstrName;
	
	ContentDispositionParser contentDisposition;
	if (part_.getField(L"Content-Disposition", &contentDisposition) == Part::FIELD_EXIST)
		wstrName = contentDisposition.getParameter(L"filename");
	
	if (!wstrName.get()) {
		const ContentTypeParser* pContentType = part_.getContentType();
		if (pContentType)
			wstrName = pContentType->getParameter(L"name");
	}
	
	return wstrName;
}

void qm::AttachmentParser::getAttachments(bool bIncludeDeleted,
										  AttachmentList* pList) const
{
	assert(pList);
	
	if (!bIncludeDeleted) {
		if (isAttachmentDeleted())
			return;
	}
	
	if (part_.isMultipart()) {
		const Part::PartList& l = part_.getPartList();
		for (Part::PartList::const_iterator it = l.begin(); it != l.end(); ++it)
			AttachmentParser(**it).getAttachments(bIncludeDeleted, pList);
	}
	else if (part_.isAttachment()) {
		wstring_ptr wstrName(getName());
		if (!wstrName.get() || !*wstrName.get())
			wstrName = allocWString(L"Untitled");
		wstring_ptr wstrOrigName = allocWString(wstrName.get());
		
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
			wstrName = concat(wstrOrigName.get(), wsz);
		}
		pList->push_back(AttachmentList::value_type(
			wstrName.get(), const_cast<Part*>(&part_)));
		wstrName.release();
	}
	else if (part_.getEnclosedPart()) {
		AttachmentParser(*part_.getEnclosedPart()).getAttachments(bIncludeDeleted, pList);
	}
}

AttachmentParser::Result qm::AttachmentParser::detach(const WCHAR* pwszDir,
													  const WCHAR* pwszName,
													  DetachCallback* pCallback,
													  wstring_ptr* pwstrPath) const
{
	assert(pwszDir);
	assert(pwszName);
	assert(pCallback);
	
	if (pwstrPath)
		pwstrPath->reset(0);
	
	StringBuffer<WSTRING> buf(pwszDir);
	if (buf.getLength() != 0 && buf.get(buf.getLength() - 1) == L'\\')
		buf.remove(buf.getLength() - 1, buf.getLength());
	
	if (!File::createDirectory(buf.getCharArray()))
		return RESULT_FAIL;
	
	buf.append(L'\\');
	buf.append(pwszName);
	
	wstring_ptr wstrPath(buf.getString());
	
	W2T(wstrPath.get(), ptszPath);
	if (::GetFileAttributes(ptszPath) != 0xffffffff) {
		wstring_ptr wstr(pCallback->confirmOverwrite(wstrPath.get()));
		if (!wstr.get())
			return RESULT_CANCEL;
		wstrPath = wstr;
	}
	
	std::auto_ptr<Encoder> pEncoder;
	ContentTransferEncodingParser contentTransferEncoding;
	if (part_.getField(L"Content-Transfer-Encoding", &contentTransferEncoding) == Part::FIELD_EXIST) {
		const WCHAR* pwszEncoding = contentTransferEncoding.getEncoding();
		if (_wcsicmp(pwszEncoding, L"7bit") != 0 &&
			_wcsicmp(pwszEncoding, L"8bit") != 0) {
			pEncoder= EncoderFactory::getInstance(pwszEncoding);
			if (!pEncoder.get())
				return RESULT_FAIL;
		}
	}
	
	FileOutputStream stream(wstrPath.get());
	if (!stream)
		return RESULT_FAIL;
	BufferedOutputStream bufferedStream(&stream, false);
	
	const Part* pEnclosedPart = part_.getEnclosedPart();
	if (pEnclosedPart) {
		xstring_ptr strContent(pEnclosedPart->getContent());
		size_t nLen = strlen(strContent.get());
		const unsigned char* p = reinterpret_cast<const unsigned char*>(strContent.get());
		if (bufferedStream.write(p, nLen) != nLen)
			return RESULT_FAIL;
	}
	else {
		const CHAR* pBody = part_.getBody();
		size_t nLen = strlen(pBody);
		const unsigned char* p = reinterpret_cast<const unsigned char*>(pBody);
		if (pEncoder.get()) {
			ByteInputStream inputStream(p, nLen, false);
			if (!pEncoder->decode(&inputStream, &bufferedStream))
				return RESULT_FAIL;
		}
		else {
			if (bufferedStream.write(p, nLen) != nLen)
				return RESULT_FAIL;
		}
	}
	
	if (!bufferedStream.close())
		return RESULT_FAIL;
	
	if (pwstrPath)
		*pwstrPath = wstrPath;
	
	return RESULT_OK;
}

bool qm::AttachmentParser::isAttachmentDeleted() const
{
	NumberParser field(0);
	Part::Field f = part_.getField(L"X-QMAIL-AttachmentDeleted", &field);
	return f == Part::FIELD_EXIST && field.getValue() != 0;
}

void qm::AttachmentParser::removeAttachments(Part* pPart)
{
	assert(pPart);
	
	if (pPart->isMultipart()) {
		const Part::PartList& l = pPart->getPartList();
		for (Part::PartList::const_iterator it = l.begin(); it != l.end(); ++it)
			removeAttachments(*it);
	}
	else {
		Part* pEnclosedPart = pPart->getEnclosedPart();
		if (pPart->isAttachment()) {
			if (pEnclosedPart) {
				pEnclosedPart->setBody("", 0);
				pEnclosedPart->setHeader("");
			}
			else {
				pPart->setBody("", 0);
			}
		}
		else if (pEnclosedPart) {
			removeAttachments(pEnclosedPart);
		}
	}
}

void qm::AttachmentParser::setAttachmentDeleted(Part* pPart)
{
	assert(pPart);
	
	NumberParser field(1, 0);
	pPart->replaceField(L"X-QMAIL-AttachmentDeleted", field);
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

qm::XQMAILAttachmentParser::XQMAILAttachmentParser()
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

Part::Field qm::XQMAILAttachmentParser::parse(const Part& part,
											  const WCHAR* pwszName)
{
	assert(pwszName);
	
	string_ptr strValue(part.getRawField(pwszName, 0));
	if (!strValue.get())
		return Part::FIELD_NOTEXIST;
	
	wstring_ptr wstrValue(decode(strValue.get(), -1, false, 0));
	
	StringBuffer<WSTRING> buf;
	const WCHAR* p = wstrValue.get();
	while (true) {
		if (*p == L',' || *p == L'\0') {
			wstring_ptr wstrName(buf.getString());
			if (*wstrName.get()) {
				listAttachment_.push_back(wstrName.get());
				wstrName.release();
			}
			
			if (*p == L'\0')
				break;
		}
		else if (*p == L'\\') {
			if (*(p + 1) != L'\0') {
				++p;
				buf.append(*p);
			}
		}
		else {
			if ((*p != L' ' && *p != L'\t') || buf.getLength() != 0)
				buf.append(*p);
		}
		++p;
	}
	
	return Part::FIELD_EXIST;
}

string_ptr qm::XQMAILAttachmentParser::unparse(const Part& part) const
{
	wstring_ptr wstrValue(format(listAttachment_));
	return encode(wstrValue.get(), -1, L"utf-8", L"B", false);
}

wstring_ptr qm::XQMAILAttachmentParser::format(const AttachmentList& l)
{
	StringBuffer<WSTRING> buf;
	
	for (AttachmentList::const_iterator it = l.begin(); it != l.end(); ++it) {
		if (it != l.begin())
			buf.append(L",\n ");
		
		if (**it == L' ' || **it == L'\t')
			buf.append(L'\\');
		
		for (const WCHAR* p = *it; *p; ++p) {
			if (*p == L'\\' || *p == L',')
				buf.append(L'\\');
			buf.append(*p);
		}
	}
	
	return buf.getString();
}
