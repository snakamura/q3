/*
 * $Id$
 *
 * Copyright(C) 1998-2008 Satoshi Nakamura
 *
 */

#pragma warning(disable:4786)

#include <qmaccount.h>
#include <qmapplication.h>
#include <qmmessage.h>
#include <qmmessageholder.h>
#include <qmsecurity.h>

#include <qsconv.h>
#include <qsencoder.h>
#include <qsfile.h>
#include <qsosutil.h>
#include <qsregex.h>
#include <qsstl.h>
#include <qsstream.h>
#include <qsstring.h>
#include <qstextutil.h>

#include <algorithm>

#include <boost/bind.hpp>
#include <boost/bind/make_adaptable.hpp>
#include <boost/lambda/bind.hpp>
#include <boost/lambda/lambda.hpp>

#include "message.h"
#include "messagecontext.h"
#include "signature.h"
#include "uri.h"
#include "zip.h"

using namespace qm;
using namespace qs;


/****************************************************************************
 *
 * Message
 *
 */

qm::Message::Message() :
	flag_(FLAG_EMPTY),
	nSecurity_(SECURITY_NONE)
{
}

qm::Message::~Message()
{
	clearParams();
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

bool qm::Message::createHeader(const CHAR* pszHeader,
							   size_t nLen)
{
	if (nLen == -1)
		nLen = strlen(pszHeader);
	
	const CHAR* pBody = Part::getBody(pszHeader, nLen);
	return create(pszHeader, pBody ? pBody - pszHeader + 4 : nLen, FLAG_HEADERONLY);
}

void qm::Message::clear()
{
	flag_ = FLAG_EMPTY;
	Part::clear();
	clearParams();
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

const WCHAR* qm::Message::getParam(const WCHAR* pwszName) const
{
	ParamList::const_iterator it = std::find_if(
		listParam_.begin(), listParam_.end(),
		boost::bind(string_equal<WCHAR>(),
			boost::bind(&ParamList::value_type::first, _1), pwszName));
	return it != listParam_.end() ? (*it).second : 0;
}

void qm::Message::setParam(const WCHAR* pwszName,
						   const WCHAR* pwszValue)
{
	assert(pwszName);
	
	wstring_ptr wstrValue;
	if (pwszValue)
		wstrValue = allocWString(pwszValue);
	
	ParamList::iterator it = std::find_if(
		listParam_.begin(), listParam_.end(),
		boost::bind(string_equal<WCHAR>(),
			boost::bind(&ParamList::value_type::first, _1), pwszName));
	if (it != listParam_.end()) {
		freeWString((*it).second);
		if (wstrValue.get())
			(*it).second = wstrValue.release();
		else
			listParam_.erase(it);
	}
	else if (wstrValue.get()) {
		wstring_ptr wstrName(allocWString(pwszName));
		listParam_.push_back(std::make_pair(wstrName.get(), wstrValue.get()));
		wstrName.release();
		wstrValue.release();
	}
}

bool qm::Message::removePrivateFields()
{
	PrefixFieldFilter filter("x-qmail-");
	return removeFields(&filter);
}

void qm::Message::clearParams()
{
	using namespace boost::lambda;
	using boost::lambda::_1;
	std::for_each(listParam_.begin(), listParam_.end(),
		(bind(&freeWString, bind(&ParamList::value_type::first, _1)),
		 bind(&freeWString, bind(&ParamList::value_type::second, _1))));
	listParam_.clear();
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
	nSecurityMode_(SECURITYMODE_NONE),
	pURIResolver_(0),
	pSignatureManager_(0),
	pAccount_(0)
{
}

qm::MessageCreator::MessageCreator(unsigned int nFlags,
								   unsigned int nSecurityMode) :
	nFlags_(nFlags),
	nSecurityMode_(nSecurityMode),
	pURIResolver_(0),
	pSignatureManager_(0),
	pAccount_(0)
{
	assert(!(nFlags & FLAG_ENCODETEXT) &&
		!(nFlags & FLAG_EXTRACTATTACHMENT) &&
		!(nFlags & FLAG_ADDSIGNATURE));
}

qm::MessageCreator::MessageCreator(unsigned int nFlags,
								   unsigned int nSecurityMode,
								   const WCHAR* pwszTransferEncodingFor8Bit) :
	nFlags_(nFlags),
	nSecurityMode_(nSecurityMode),
	pURIResolver_(0),
	pSignatureManager_(0),
	pAccount_(0)
{
	assert(!(nFlags & FLAG_EXTRACTATTACHMENT) &&
		!(nFlags & FLAG_ADDSIGNATURE));
	
	if (pwszTransferEncodingFor8Bit && *pwszTransferEncodingFor8Bit)
		wstrTransferEncodingFor8Bit_ = allocWString(pwszTransferEncodingFor8Bit);
}

qm::MessageCreator::MessageCreator(unsigned int nFlags,
								   unsigned int nSecurityMode,
								   const WCHAR* pwszTransferEncodingFor8Bit,
								   const URIResolver* pURIResolver,
								   SignatureManager* pSignatureManager,
								   Account* pAccount,
								   const WCHAR* pwszArchiveAttachmentExcludePattern,
								   const WCHAR* pwszTempDir) :
	nFlags_(nFlags),
	nSecurityMode_(nSecurityMode),
	pURIResolver_(pURIResolver),
	pSignatureManager_(pSignatureManager),
	pAccount_(pAccount)
{
	assert(pURIResolver);
	assert(pSignatureManager);
	assert(pAccount);
	assert(pwszTempDir);
	
	if (pwszTransferEncodingFor8Bit && *pwszTransferEncodingFor8Bit)
		wstrTransferEncodingFor8Bit_ = allocWString(pwszTransferEncodingFor8Bit);
	if (pwszArchiveAttachmentExcludePattern)
		wstrArchiveAttachmentExcludePattern_ = allocWString(pwszArchiveAttachmentExcludePattern);
	wstrTempDir_ = allocWString(pwszTempDir);
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

std::auto_ptr<Message> qm::MessageCreator::createMessage(const WCHAR* pwszMessage,
														 size_t nLen) const
{
	std::auto_ptr<Part> pPart(createPart(pwszMessage, nLen, 0, true));
	if (!pPart.get())
		return std::auto_ptr<Message>();
	
	std::auto_ptr<Message> pMessage(static_cast<Message*>(pPart.release()));
	pMessage->setFlag(Message::FLAG_NONE);
	
	return pMessage;
}

std::auto_ptr<Part> qm::MessageCreator::createPart(const WCHAR* pwszMessage,
												   size_t nLen,
												   Part* pParent,
												   bool bMessage) const
{
	if (nLen == -1)
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
				return std::auto_ptr<Part>();
			pBody += 2;
			nBodyLen = nLen - (pBody - pwszMessage);
		}
		else {
			StringBuffer<WSTRING> buf(pwszMessage, nLen);
			if (*(buf.getCharArray() + buf.getLength() - 1) != L'\n')
				buf.append(L'\n');
			if (!createHeader(pPart.get(), buf.getCharArray(), buf.getLength()))
				return std::auto_ptr<Part>();
		}
	}
	
	const ContentTypeParser* pContentType = pPart->getContentType();
	const WCHAR* pwszMediaType = pContentType ?
		pContentType->getMediaType() : L"text";
	bool bMultipart = PartUtil::isMultipart(pContentType);
	
	if (pBody) {
		bool bRFC822 = false;
		if (!bMultipart) {
			bRFC822 = PartUtil::isContentType(pContentType, L"message", L"rfc822");
			if (!bRFC822 && !pContentType && pParent)
				bRFC822 = PartUtil::isContentType(pParent->getContentType(),
					L"multipart", L"digest");
		}
		if (bMultipart) {
			wstring_ptr wstrBoundary(pContentType->getParameter(L"boundary"));
			if (!wstrBoundary.get())
				return std::auto_ptr<Part>();
			
			BoundaryFinder<WCHAR, WSTRING> finder(pBody - 1, nBodyLen + 1,
				wstrBoundary.get(), L"\n", (nFlags_ & FLAG_RECOVERBODY) != 0);
			
			while (true) {
				const WCHAR* pBegin = 0;
				const WCHAR* pEnd = 0;
				bool bEnd = false;
				if (!finder.getNext(&pBegin, &pEnd, &bEnd))
					return std::auto_ptr<Part>();
				if (pBegin) {
					MessageCreator creator(getCreatorForChild());
					std::auto_ptr<Part> pChild(creator.createPart(
						pBegin, pEnd - pBegin, pPart.get(), false));
					if (!pChild.get())
						return std::auto_ptr<Part>();
					pPart->addPart(pChild);
				}
				if (bEnd)
					break;
			}
			
			std::pair<const WCHAR*, size_t> preamble(finder.getPreamble());
			if (preamble.first) {
				if (!pPart->setPreamble(wcs2mbs(preamble.first, preamble.second).get()))
					return std::auto_ptr<Part>();
			}
			std::pair<const WCHAR*, size_t> epilogue(finder.getEpilogue());
			if (epilogue.first) {
				if (!pPart->setEpilogue(wcs2mbs(epilogue.first, epilogue.second).get()))
					return std::auto_ptr<Part>();
			}
		}
		else if (bRFC822) {
			MessageCreator creator(getCreatorForChild());
			std::auto_ptr<Part> pEnclosed(creator.createPart(pBody, nBodyLen, 0, false));
			if (!pEnclosed.get())
				return std::auto_ptr<Part>();
			pPart->setEnclosedPart(pEnclosed);
		}
		else if (_wcsicmp(pwszMediaType, L"text") == 0) {
			wstring_ptr wstrCharset;
			if (pContentType) {
				wstrCharset = pContentType->getParameter(L"charset");
				if (!wstrCharset.get())
					wstrCharset = allocWString(L"us-ascii");
			}
			
			std::auto_ptr<Converter> pConverter;
			if (wstrCharset.get())
				pConverter = ConverterFactory::getInstance(wstrCharset.get());
			
			xstring_size_ptr strBody;
			
			if (!pConverter.get()) {
				bool bDefault = true;
				const WCHAR* pwszCharset = Part::getDefaultCharset();
				size_t n = 0;
				while (n < nBodyLen && *(pBody + n) < 0x80)
					++n;
				if (n == nBodyLen) {
					pwszCharset = L"us-ascii";
				}
				else {
					SimpleParser charset(0);
					if (pPart->getField(L"X-QMAIL-OriginalCharset", &charset) == Part::FIELD_EXIST &&
						_wcsicmp(charset.getValue(), L"us-ascii") != 0 &&
						_wcsicmp(charset.getValue(), pwszCharset) != 0) {
						bDefault = false;
						pwszCharset = L"utf-8";
					}
				}
				wstrCharset = allocWString(pwszCharset);
				pConverter = ConverterFactory::getInstance(wstrCharset.get());
				
				xstring_size_ptr strBody(convertBody(pConverter.get(), pBody, nBodyLen));
				if (!strBody.get()) {
					wstrCharset = allocWString(L"utf-8");
					pConverter = ConverterFactory::getInstance(wstrCharset.get());
					bDefault = false;
				}
				
				if (nFlags_ & FLAG_ADDCONTENTTYPE ||
					(nFlags_ & FLAG_ADDNODEFAULTCONTENTTYPE && !bDefault)) {
					ContentTypeParser contentType(L"text", L"plain");
					contentType.setParameter(L"charset", wstrCharset.get());
					if (!pPart->replaceField(L"Content-Type", contentType))
						return std::auto_ptr<Part>();
					pContentType = pPart->getContentType();
				}
			}
			assert(pConverter.get());
			
			pPart->removeField(L"X-QMAIL-OriginalCharset");
			
			if (!strBody.get()) {
				strBody = convertBody(pConverter.get(), pBody, nBodyLen);
				if (!strBody.get())
					return std::auto_ptr<Part>();
			}
			
			std::auto_ptr<Encoder> pEncoder;
			if (nFlags_ & FLAG_ENCODETEXT) {
				pEncoder = pPart->getEncoder();
				if (!pEncoder.get()) {
					unsigned char* p = reinterpret_cast<unsigned char*>(strBody.get());
					unsigned char* pEnd = p + strBody.size();
					size_t nLineLen = 0;
					while (p < pEnd) {
						if (*p >= 0x80)
							break;
						
						if (*p == '\r' && p + 1 < pEnd && *(p + 1) == '\n') {
							nLineLen = 0;
							++p;
						}
						else {
							++nLineLen;
							if (nLineLen >= MAX_LINE_LENGTH)
								break;
						}
						
						++p;
					}
					
					const WCHAR* pwszEncoding = 0;
					if (p == pEnd) {
						pwszEncoding = L"7bit";
					}
					else {
						if (wstrTransferEncodingFor8Bit_.get())
							pwszEncoding = wstrTransferEncodingFor8Bit_.get();
						else
							pwszEncoding = getEncodingForCharset(wstrCharset.get());
						pEncoder = EncoderFactory::getInstance(pwszEncoding);
						if (!pEncoder.get())
							return std::auto_ptr<Part>();
					}
					
					if (nFlags_ & FLAG_ADDCONTENTTYPE) {
						ContentTransferEncodingParser contentTransferEncoding(pwszEncoding);
						if (!pPart->replaceField(L"Content-Transfer-Encoding", contentTransferEncoding))
							return std::auto_ptr<Part>();
					}
				}
			}
			
			if (pEncoder.get()) {
				malloc_size_ptr<unsigned char> pBody(pEncoder->encode(
					reinterpret_cast<unsigned char*>(strBody.get()), strBody.size()));
				if (!pBody.get())
					return std::auto_ptr<Part>();
				
				if (!pPart->setBody(reinterpret_cast<CHAR*>(pBody.get()), pBody.size()))
					return std::auto_ptr<Part>();
			}
			else {
				pPart->setBody(xstring_ptr(strBody.release()));
			}
		}
		else {
			xstring_ptr strBody(PartUtil::w2a(pBody, nBodyLen));
			if (!strBody.get() && nFlags_ & FLAG_RECOVERBODY) {
				std::auto_ptr<Converter> pConverter(
					ConverterFactory::getInstance(Part::getDefaultCharset()));
				size_t nLen = nBodyLen;
				strBody.reset(pConverter->encode(pBody, &nLen).release());
			}
			if (!strBody.get())
				return std::auto_ptr<Part>();
			pPart->setBody(strBody);
		}
	}
	
	if (bMessage && (nFlags_ & FLAG_ADDCONTENTTYPE || nFlags_ & FLAG_ADDNODEFAULTCONTENTTYPE)) {
		SimpleParser mimeVersion(L"1.0", 0);
		if (!pPart->replaceField(L"MIME-Version", mimeVersion))
			return std::auto_ptr<Part>();
	}
	
	if (bMessage && nFlags_ & FLAG_ADDSIGNATURE) {
		assert(pSignatureManager_);
		
		UnstructuredParser signature;
		if (pPart->getField(L"X-QMAIL-Signature", &signature) == Part::FIELD_EXIST) {
			const Signature* pSignature = pSignatureManager_->getSignature(
				pAccount_, signature.getValue());
			if (pSignature && *pSignature->getSignature()) {
				if (PartUtil::isContentType(pContentType, L"text", L"plain")) {
					std::auto_ptr<Part> p(createPartWithSignature(pPart.get(), 0, pSignature));
					if (!p.get())
						return std::auto_ptr<Part>();
					pPart = p;
				}
				else if (PartUtil::isContentType(pContentType, L"multipart", L"mixed")) {
					if (pPart->getPartCount() != 0 &&
						PartUtil::isContentType(pPart->getPart(0)->getContentType(), L"text", L"plain")) {
						Part* pTextPart = pPart->getPart(0);
						std::auto_ptr<Part> p(createPartWithSignature(pTextPart, pPart.get(), pSignature));
						if (!p.get())
							return std::auto_ptr<Part>();
						pPart->removePart(pTextPart);
						delete pTextPart;
						pPart->insertPart(0, p);
					}
					else {
						std::auto_ptr<Part> p(createPartWithSignature(0, pPart.get(), pSignature));
						if (!p.get())
							return std::auto_ptr<Part>();
						pPart->insertPart(0, p);
					}
				}
				else {
					std::auto_ptr<Message> pParent(new Message());
					if (!makeMultipart(pParent.get(), pPart))
						return std::auto_ptr<Part>();
					pPart = pParent;
					
					std::auto_ptr<Part> p(createPartWithSignature(0, pPart.get(), pSignature));
					if (!p.get())
						return std::auto_ptr<Part>();
					pPart->insertPart(0, p);
				}
				pContentType = pPart->getContentType();
			}
			pPart->removeField(L"X-QMAIL-Signature");
		}
	}
	
	if (bMessage && nFlags_ & FLAG_EXTRACTATTACHMENT) {
		assert(pURIResolver_);
		
		XQMAILAttachmentParser attachment;
		if (pPart->getField(L"X-QMAIL-Attachment", &attachment) == Part::FIELD_EXIST &&
			!attachment.getAttachments().empty()) {
			const XQMAILAttachmentParser::AttachmentList& l = attachment.getAttachments();
			assert(pContentType);
			if (!PartUtil::isContentType(pContentType, L"multipart", L"mixed")) {
				std::auto_ptr<Message> pParent(new Message());
				if (!makeMultipart(pParent.get(), pPart))
					return std::auto_ptr<Part>();
				pPart = pParent;
				pContentType = pPart->getContentType();
			}
			
			const WCHAR* pwszArchive = 0;
#ifdef QMZIP
			UnstructuredParser archive;
			if (pPart->getField(L"X-QMAIL-ArchiveAttachment", &archive) == Part::FIELD_EXIST)
				pwszArchive = archive.getValue();
#endif
			if (!attachFilesOrURIs(pPart.get(), l, pURIResolver_, nSecurityMode_,
				pwszArchive, wstrArchiveAttachmentExcludePattern_.get(), wstrTempDir_.get()))
				return std::auto_ptr<Part>();
#ifdef QMZIP
			pPart->removeField(L"X-QMAIL-ArchiveAttachment");
#endif
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
				
				WCHAR* pLine = &buf[0] + nLine;
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
		
		bool bSet = false;
		if (std::find_if(ppwszAddresses, ppwszAddresses + countof(ppwszAddresses),
			std::bind2nd(string_equal<WCHAR>(), pwszNameLower)) != ppwszAddresses + countof(ppwszAddresses)) {
			if (nFlags_ & FLAG_EXPANDALIAS) {
				// TODO
				// Replace alias
			}
			
			bSet = setField(pPart, pwszName, pwszValue, FIELDTYPE_ADDRESSLIST);
		}
		else if (wcscmp(pwszNameLower, L"content-type") == 0) {
			bSet = setField(pPart, pwszName, pwszValue, FIELDTYPE_CONTENTTYPE);
		}
		else if (wcscmp(pwszNameLower, L"content-transfer-encoding") == 0) {
			bSet = setField(pPart, pwszName, pwszValue, FIELDTYPE_CONTENTTRANSFERENCODING);
		}
		else if (wcscmp(pwszNameLower, L"content-disposition") == 0) {
			bSet = setField(pPart, pwszName, pwszValue, FIELDTYPE_CONTENTDISPOSITION);
		}
		else if (wcscmp(pwszNameLower, L"message-id") == 0) {
			bSet = setField(pPart, pwszName, pwszValue, FIELDTYPE_MESSAGEID);
		}
		else if (wcscmp(pwszNameLower, L"references") == 0 ||
			wcscmp(pwszNameLower, L"in-reply-to") == 0) {
			bSet = setField(pPart, pwszName, pwszValue, FIELDTYPE_REFERENCES);
		}
		else if (wcscmp(pwszNameLower, L"x-qmail-attachment") == 0) {
			bSet = setField(pPart, pwszName, pwszValue, FIELDTYPE_XQMAILATTACHMENT);
		}
		else {
			bool bMulti = wcslen(pwszNameLower) < 9 ||
				wcsncmp(pwszNameLower, L"x-qmail-", 8) != 0;
			bSet = setField(pPart, pwszName, pwszValue,
				bMulti ? FIELDTYPE_MULTIUNSTRUCTURED : FIELDTYPE_SINGLEUNSTRUCTURED);
		}
		if (!bSet) {
			if ((nFlags_ & FLAG_RECOVERHEADER) == 0 ||
				!setField(pPart, pwszName, pwszValue, FIELDTYPE_MULTIUNSTRUCTURED))
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

std::auto_ptr<Part> qm::MessageCreator::createPartWithSignature(const Part* pPart,
																Part* pParent,
																const Signature* pSignature) const
{
	assert(pSignature);
	
	XStringBuffer<WXSTRING> buf;
	if (pPart) {
		if (!PartUtil::a2w(pPart->getHeader(), -1, &buf) ||
			!buf.append(L'\n') ||
			!pPart->getBodyText(0, &buf))
			return std::auto_ptr<Part>();
	}
	else {
		if (!buf.append(L'\n'))
			return std::auto_ptr<Part>();
	}
	buf.append(pSignature->getSignature());
	
	MessageCreator creator(nFlags_ & ~FLAG_ADDSIGNATURE, SECURITYMODE_NONE,
		wstrTransferEncodingFor8Bit_.get(), pURIResolver_, pSignatureManager_,
		pAccount_, wstrArchiveAttachmentExcludePattern_.get(), wstrTempDir_.get());
	return creator.createPart(buf.getCharArray(), buf.getLength(), pParent, pParent == 0);
}

MessageCreator qm::MessageCreator::getCreatorForChild() const
{
	unsigned int nFlags = nFlags_ & (FLAG_ADDCONTENTTYPE | FLAG_ENCODETEXT | FLAG_RECOVER);
	return MessageCreator(nFlags, SECURITYMODE_NONE, wstrTransferEncodingFor8Bit_.get());
}

bool qm::MessageCreator::setField(Part* pPart,
								  const WCHAR* pwszName,
								  const WCHAR* pwszValue,
								  FieldType type)
{
	assert(pPart);
	assert(pwszName);
	assert(pwszValue);
	
	Part partDummy;
	switch (type) {
	case FIELDTYPE_ADDRESSLIST:
		{
			UTF8Parser field(pwszValue);
			if (!partDummy.setField(pwszName, field))
				return false;
			AddressListParser addressList(AddressListParser::FLAG_ALLOWUTF8);
			if (partDummy.getField(pwszName, &addressList) != Part::FIELD_EXIST)
				return false;
			if (!pPart->replaceField(pwszName, addressList))
				return false;
		}
		break;
	case FIELDTYPE_CONTENTTYPE:
		{
			DummyParser field(pwszValue, DummyParser::FLAG_TSPECIAL);
			if (!partDummy.setField(pwszName, field))
				return false;
			ContentTypeParser contentType;
			if (partDummy.getField(pwszName, &contentType) != Part::FIELD_EXIST)
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
			if (!partDummy.setField(pwszName, field))
				return false;
			ContentDispositionParser contentDisposition;
			if (partDummy.getField(pwszName, &contentDisposition) != Part::FIELD_EXIST)
				return false;
			if (!pPart->replaceField(pwszName, contentDisposition))
				return false;
		}
		break;
	case FIELDTYPE_MESSAGEID:
		{
			DummyParser field(pwszValue, 0);
			if (!partDummy.setField(pwszName, field))
				return false;
			MessageIdParser messageId;
			if (partDummy.getField(pwszName, &messageId) != Part::FIELD_EXIST)
				return false;
			if (!pPart->replaceField(pwszName, messageId))
				return false;
		}
		break;
	case FIELDTYPE_REFERENCES:
		{
			DummyParser field(pwszValue, 0);
			if (!partDummy.setField(pwszName, field))
				return false;
			ReferencesParser references;
			if (partDummy.getField(pwszName, &references) != Part::FIELD_EXIST)
				return false;
			if (!pPart->replaceField(pwszName, references))
				return false;
		}
		break;
	case FIELDTYPE_XQMAILATTACHMENT:
		{
			XQMAILAttachmentParser attachment(pwszValue);
			if (!pPart->replaceField(pwszName, attachment))
				return false;
		}
		break;
	case FIELDTYPE_SINGLEUNSTRUCTURED:
		{
			UnstructuredParser field(pwszValue);
			if (!pPart->replaceField(pwszName, field))
				return false;
		}
		break;
	case FIELDTYPE_MULTIUNSTRUCTURED:
		{
			UnstructuredParser field(pwszValue);
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

bool qm::MessageCreator::getAddressList(const WCHAR* pwszAddresses,
										AddressListParser* pAddressList)
{
	Part part;
	if (!MessageCreator::setField(&part, L"Address",
		pwszAddresses, MessageCreator::FIELDTYPE_ADDRESSLIST))
		return false;
	return part.getField(L"Address", pAddressList) == Part::FIELD_EXIST;
}

bool qm::MessageCreator::getAddress(const WCHAR* pwszAddress,
									AddressParser* pAddress)
{
	Part part;
	if (!MessageCreator::setField(&part, L"Address",
		pwszAddress, MessageCreator::FIELDTYPE_ADDRESSLIST))
		return false;
	return part.getField(L"Address", pAddress) == Part::FIELD_EXIST;
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
	_snwprintf(wszBoundary, countof(wszBoundary),
		L"__boundary-%04d%02d%02d%02d%02d%02d%03d%04d__",
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
	pParentPart->setEpilogue("");
	
	return true;
}

bool qm::MessageCreator::attachFilesOrURIs(qs::Part* pPart,
										   const AttachmentList& l,
										   const URIResolver* pURIResolver,
										   unsigned int nSecurityMode,
										   const WCHAR* pwszArchiveName,
										   const WCHAR* pwszExcludePattern,
										   const WCHAR* pwszTempDir)
{
#ifdef QMZIP
	bool bArchive = pwszArchiveName && *pwszArchiveName;
#else
	bool bArchive = false;
#endif
	
	typedef std::vector<const WCHAR*> List;
	List listAttach;
	List listArchive;
	
	if (bArchive) {
		std::auto_ptr<RegexPattern> pExcludePattern;
		if (pwszExcludePattern && *pwszExcludePattern)
			pExcludePattern = RegexCompiler().compile(
				pwszExcludePattern, RegexCompiler::MODE_CASEINSENSITIVE);
		for (AttachmentList::const_iterator it = l.begin(); it != l.end(); ++it) {
			const WCHAR* pwszFileOrURI = *it;
			if (isAttachmentURI(pwszFileOrURI) ||
				(pExcludePattern.get() && pExcludePattern->search(pwszFileOrURI)))
				listAttach.push_back(pwszFileOrURI);
			else
				listArchive.push_back(pwszFileOrURI);
		}
	}
	else {
		std::copy(l.begin(), l.end(), std::back_inserter(listAttach));
	}
	
#ifdef QMZIP
	if (!listArchive.empty()) {
		if (!attachArchivedFile(pPart, pwszArchiveName, listArchive, pwszTempDir))
			return false;
	}
#endif
	if (std::find_if(listAttach.begin(), listAttach.end(),
		std::not1(boost::make_adaptable<bool, const WCHAR*>(
			boost::bind(&MessageCreator::attachFileOrURI,
				pPart, _1, pURIResolver, nSecurityMode)))) != listAttach.end())
		return false;
	
	return true;
}

bool qm::MessageCreator::attachFileOrURI(qs::Part* pPart,
										 const WCHAR* pwszFileOrURI,
										 const URIResolver* pURIResolver,
										 unsigned int nSecurityMode)
{
	assert(pPart->isMultipart());
	
	std::auto_ptr<Part> pChildPart;
	if (isAttachmentURI(pwszFileOrURI)) {
		std::auto_ptr<URI> pURI(URIFactory::parseURI(pwszFileOrURI));
		if (!pURI.get())
			return false;
		
		std::auto_ptr<MessageContext> pContext(pURI->resolve(pURIResolver));
		if (!pContext.get())
			return false;
		
		Message* pMessage = pContext->getMessage(Account::GMF_ALL, 0, nSecurityMode);
		if (!pMessage)
			return false;
		
		const URIFragment& fragment = pURI->getFragment();
		const Part* pPart = fragment.getPart(pMessage);
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
		pChildPart = createPartFromFile(pPart, pwszFileOrURI);
	}
	if (!pChildPart.get())
		return false;
	pPart->addPart(pChildPart);
	
	return true;
}

#ifdef QMZIP
bool qm::MessageCreator::attachArchivedFile(Part* pPart,
											const WCHAR* pwszFileName,
											const FileNameList& l,
											const WCHAR* pwszTempDir)
{
	assert(pPart);
	assert(pPart->isMultipart());
	assert(pwszFileName);
	assert(!l.empty());
	assert(pwszTempDir);
	
	ZipFile::PathList listPath;
	listPath.resize(l.size());
	std::copy(l.begin(), l.end(), listPath.begin());
	
	ZipFile zip(pwszFileName, listPath, pwszTempDir);
	const WCHAR* pwszPath = zip.getPath();
	if (!pwszPath)
		return false;
	
	std::auto_ptr<Part> pChildPart(createPartFromFile(pPart, pwszPath));
	if (!pChildPart.get())
		return false;
	pPart->addPart(pChildPart);
	
	return true;
}
#endif

std::auto_ptr<Part> qm::MessageCreator::createPartFromFile(Part* pParentPart,
														   const WCHAR* pwszPath)
{
	assert(pwszPath);
	
	// Add a new part to parent part temporarily to make that
	// the file name is encoded using parent part's header charset.
	// The part is removed from its parent before the function returns.
	std::auto_ptr<Part> pNewPart(new Part());
	Part* pPart = pNewPart.get();
	pParentPart->addPart(pNewPart);
	struct Remover
	{
		Remover(Part* pParentPart,
				Part* pPart) :
			pParentPart_(pParentPart),
			pPart_(pPart)
		{
		}
		
		~Remover()
		{
			pParentPart_->removePart(pPart_);
		}
		
		Part* pParentPart_;
		Part* pPart_;
	} remover(pParentPart, pPart);
	
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
		return std::auto_ptr<Part>();
	ContentTypeParser contentType;
	if (pPart->getField(L"Content-Type", &contentType) != Part::FIELD_EXIST) {
		ContentTypeParser dummyContentType(L"application", L"octet-stream");
		if (!pPart->setField(L"Content-Type", dummyContentType))
			return std::auto_ptr<Part>();
		if (pPart->getField(L"Content-Type", &contentType) != Part::FIELD_EXIST)
			return std::auto_ptr<Part>();
	}
	contentType.setParameter(L"name", pFileName);
	if (!pPart->replaceField(L"Content-Type", contentType))
		return std::auto_ptr<Part>();
	
	bool bBase64 = !PartUtil::isContentType(&contentType, L"message", L"rfc822");
	if (bBase64) {
		ContentTransferEncodingParser contentTransferEncoding(L"base64");
		if (!pPart->setField(L"Content-Transfer-Encoding", contentTransferEncoding))
			return std::auto_ptr<Part>();
	}
	
	ContentDispositionParser contentDisposition(L"attachment");
	contentDisposition.setParameter(L"filename", pFileName);
	if (!pPart->setField(L"Content-Disposition", contentDisposition))
		return std::auto_ptr<Part>();
	
	W2T(pwszPath, ptszPath);
	WIN32_FIND_DATA fd;
	AutoFindHandle hFind(::FindFirstFile(ptszPath, &fd));
	if (!hFind.get())
		return std::auto_ptr<Part>();
	size_t nSize = fd.nFileSizeLow;
	hFind.close();
	
	FileInputStream stream(pwszPath);
	if (!stream)
		return std::auto_ptr<Part>();
	BufferedInputStream bufferedStream(&stream, false);
	
	if (bBase64) {
		Base64Encoder encoder(true);
		XStringOutputStream outputStream;
		if (!outputStream.reserve((nSize/3 + 1)*4 + nSize/45*2))
			return std::auto_ptr<Part>();
		if (!encoder.encode(&bufferedStream, &outputStream))
			return std::auto_ptr<Part>();
		pPart->setBody(outputStream.getXString());
	}
	else {
		xstring_ptr strBody(allocXString(nSize));
		if (!strBody.get())
			return std::auto_ptr<Part>();
		if (bufferedStream.read(reinterpret_cast<unsigned char*>(strBody.get()), nSize) != nSize)
			return std::auto_ptr<Part>();
		pPart->setBody(strBody);
	}
	
	return std::auto_ptr<Part>(pPart);
}

std::auto_ptr<Part> qm::MessageCreator::createRfc822Part(const Part& part,
														 bool bHeaderOnly)
{
	std::auto_ptr<Part> pPart(new Part());
	
	ContentTypeParser contentType(L"message", L"rfc822");
	if (!pPart->setField(L"Content-Type", contentType))
		return std::auto_ptr<Part>();
	
	wstring_ptr wstrFileName;
	UnstructuredParser subject;
	if (part.getField(L"Subject", &subject) == Part::FIELD_EXIST && *subject.getValue())
		wstrFileName = concat(subject.getValue(), L".eml");
	else
		wstrFileName = allocWString(L"Untitled.eml");
	
	ContentDispositionParser contentDisposition(L"attachment");
	contentDisposition.setParameter(L"filename", wstrFileName.get());
	if (!pPart->setField(L"Content-Disposition", contentDisposition))
		return std::auto_ptr<Part>();
	
	if (bHeaderOnly) {
		pPart->setBody(part.getHeader(), -1);
	}
	else {
		xstring_size_ptr strContent(part.getContent());
		if (!strContent.get())
			return std::auto_ptr<Part>();
		xstring_ptr strBody(strContent.release());
		pPart->setBody(strBody);
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
	
	Registry reg(HKEY_CLASSES_ROOT, wstrExt.get(), true);
	if (reg) {
		if (reg.getValue(L"Content Type", &wstrContentType)) {
			if (wstrContentType.get() &&
				_wcsnicmp(wstrContentType.get(), L"text/", 5) == 0)
				wstrContentType.reset(0);
		}
	}
	
	return wstrContentType;
}

const WCHAR* qm::MessageCreator::getEncodingForCharset(const WCHAR* pwszCharset)
{
	size_t nLen = wcslen(pwszCharset);
	if (_wcsicmp(pwszCharset, L"us-ascii") == 0 ||
		((nLen == 10 || nLen == 11) && _wcsnicmp(pwszCharset, L"iso-8859-", 9) == 0) ||
		_wcsicmp(pwszCharset, L"utf-7") == 0)
		return L"quoted-printable";
	else
		return L"base64";
}

bool qm::MessageCreator::isAttachmentURI(const WCHAR* pwszAttachment)
{
	return URIFactory::isURI(pwszAttachment);
}


/****************************************************************************
 *
 * BodyData
 *
 */

qm::BodyData::BodyData(malloc_size_ptr<unsigned char>& p) :
	p_(p)
{
}

qm::BodyData::BodyData(xstring_size_ptr& str) :
	str_(str)
{
}

qm::BodyData::BodyData(BodyData& bodyData) :
	p_(bodyData.p_),
	str_(bodyData.str_)
{
}

qm::BodyData::~BodyData()
{
}

const unsigned char* qm::BodyData::get() const
{
	return p_.get() ? p_.get() : reinterpret_cast<const unsigned char*>(str_.get());
}

size_t qm::BodyData::size() const
{
	return p_.get() ? p_.size() : str_.size();
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
	
	AddressListParser address;
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
		L"References",
		L"In-Reply-To"
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

wxstring_size_ptr qm::PartUtil::getAllText(const WCHAR* pwszQuote,
										   const WCHAR* pwszCharset,
										   bool bBodyOnly) const
{
	XStringBuffer<WXSTRING> buf;
	if (!getAllText(pwszQuote, pwszCharset, bBodyOnly, &buf))
		return wxstring_size_ptr();
	return buf.getXStringSize();
}

bool qm::PartUtil::getAllText(const WCHAR* pwszQuote,
							  const WCHAR* pwszCharset,
							  bool bBodyOnly,
							  qs::XStringBuffer<qs::WXSTRING>* pBuf) const
{
	if (!bBodyOnly) {
		if (part_.getHeader()) {
			Part::FieldList l;
			Part::FieldListFree free(l);
			part_.getFields(&l);
			for (Part::FieldList::const_iterator it = l.begin(); it != l.end(); ++it) {
				if (!normalizeHeader((*it).second, pBuf) || !pBuf->append(L'\n'))
					return false;
			}
		}
		if (!pBuf->append(L'\n'))
			return false;
	}
	
	if (part_.isMultipart()) {
		const ContentTypeParser* pContentType = part_.getContentType();
		wstring_ptr wstrBoundary(pContentType->getParameter(L"boundary"));
		
		const CHAR* pszPreamble = part_.getPreamble();
		if (pszPreamble) {
			if (!pBuf->append(mbs2wcs(pszPreamble).get()) || !pBuf->append(L"\n"))
				return false;
		}
		
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
		
		const CHAR* pszEpilogue = part_.getEpilogue();
		if (pszEpilogue) {
			if (!pBuf->append(L"\n") || !pBuf->append(mbs2wcs(pszEpilogue).get()))
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
			if (pwszQuote && *pwszQuote) {
				wxstring_size_ptr wstrBody(part_.getBodyText(pwszCharset));
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

qs::wstring_ptr qm::PartUtil::getAllTextCharset() const
{
	wstring_ptr wstrAllCharset;
	if (part_.isMultipart()) {
		const Part::PartList& l = part_.getPartList();
		for (Part::PartList::const_iterator it = l.begin(); it != l.end(); ++it) {
			wstring_ptr wstrCharset(PartUtil(**it).getAllTextCharset());
			if (wstrAllCharset.get()) {
				if (_wcsicmp(wstrAllCharset.get(), L"us-ascii") == 0) {
					wstrAllCharset = wstrCharset;
				}
				else if (_wcsicmp(wstrCharset.get(), L"us-ascii") != 0 &&
					_wcsicmp(wstrAllCharset.get(), wstrCharset.get()) != 0) {
					wstrAllCharset = allocWString(L"utf-8");
					break;
				}
			}
			else {
				wstrAllCharset = wstrCharset;
			}
		}
	}
	else if (part_.getEnclosedPart()) {
		wstrAllCharset = PartUtil(*part_.getEnclosedPart()).getAllTextCharset();
	}
	else {
		wstrAllCharset = part_.getCharset();
	}
	return wstrAllCharset.get() ? wstrAllCharset : allocWString(L"us-ascii");
}

wxstring_size_ptr qm::PartUtil::getBodyText(const WCHAR* pwszQuote,
											const WCHAR* pwszCharset,
											Rfc822Mode rfc822Mode) const
{
	XStringBuffer<WXSTRING> buf;
	if (!getBodyText(pwszQuote, pwszCharset, rfc822Mode, &buf))
		return wxstring_size_ptr();
	return buf.getXStringSize();
}

bool qm::PartUtil::getBodyText(const WCHAR* pwszQuote,
							   const WCHAR* pwszCharset,
							   Rfc822Mode rfc822Mode,
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
				if (*(pBuf->getCharArray() + pBuf->getLength() - 1) == L'\n' && pwszQuote && *pwszQuote) {
					if (!pBuf->append(pwszQuote))
						return false;
				}
				if (!pBuf->append(L"\n"))
					return false;
				if (pwszQuote && *pwszQuote) {
					if (!pBuf->append(pwszQuote))
						return false;
				}
				if (!pBuf->append(L"------------------------------------"
					L"------------------------------------\n"))
					return false;
			}
			
			size_t nPrevLen = pBuf->getLength();
			if (!PartUtil(**it).getBodyText(pwszQuote, pwszCharset, rfc822Mode, pBuf))
				return false;
			if (pBuf->getLength() == nPrevLen)
				pBuf->remove(nLen, -1);
			
			if (bAlternative)
				break;
		}
	}
	else {
		bool bAttachment = part_.isAttachment();
		if (pwszQuote && *pwszQuote) {
			wxstring_size_ptr wstrBody;
			if (part_.getEnclosedPart() && isRfc822Rendered(rfc822Mode, bAttachment)) {
				PartUtil util(*part_.getEnclosedPart());
				wstrBody = util.getFormattedText(false, pwszCharset, rfc822Mode);
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
			if (part_.getEnclosedPart() && isRfc822Rendered(rfc822Mode, bAttachment)) {
				PartUtil util(*part_.getEnclosedPart());
				if (!util.getFormattedText(false, pwszCharset, rfc822Mode, pBuf))
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

qs::wstring_ptr qm::PartUtil::getBodyTextCharset(Rfc822Mode rfc822Mode) const
{
	wstring_ptr wstrAllCharset;
	if (part_.isMultipart()) {
		const ContentTypeParser* pContentType = part_.getContentType();
		assert(pContentType);
		bool bAlternative = _wcsicmp(pContentType->getSubType(), L"alternative") == 0;
		const Part::PartList& l = part_.getPartList();
		for (Part::PartList::const_iterator it = l.begin(); it != l.end(); ++it) {
			wstring_ptr wstrCharset = PartUtil(**it).getBodyTextCharset(rfc822Mode);
			if (wstrAllCharset.get()) {
				if (_wcsicmp(wstrAllCharset.get(), L"us-ascii") == 0) {
					wstrAllCharset = wstrCharset;
				}
				else if (_wcsicmp(wstrCharset.get(), L"us-ascii") != 0 &&
					_wcsicmp(wstrAllCharset.get(), wstrCharset.get()) != 0) {
					wstrAllCharset = allocWString(L"utf-8");
					break;
				}
			}
			else {
				wstrAllCharset = wstrCharset;
			}
			if (bAlternative)
				break;
		}
	}
	else {
		bool bAttachment = part_.isAttachment();
		if (part_.getEnclosedPart() && isRfc822Rendered(rfc822Mode, bAttachment))
			wstrAllCharset = PartUtil(*part_.getEnclosedPart()).getBodyTextCharset(rfc822Mode);
		else if (!bAttachment)
			wstrAllCharset = part_.getCharset();
	}
	return wstrAllCharset.get() ? wstrAllCharset : allocWString(L"us-ascii");
}

wxstring_size_ptr qm::PartUtil::getFormattedText(bool bUseSendersTimeZone,
												 const WCHAR* pwszCharset,
												 Rfc822Mode rfc822Mode) const
{
	XStringBuffer<WXSTRING> buf;
	if (!getFormattedText(bUseSendersTimeZone, pwszCharset, rfc822Mode, &buf))
		return wxstring_size_ptr();
	return buf.getXStringSize();
}

bool qm::PartUtil::getFormattedText(bool bUseSendersTimeZone,
									const WCHAR* pwszCharset,
									Rfc822Mode rfc822Mode,
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
		AddressListParser address;
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
		wstring_ptr wstrFormat(concat(L"Date:    ", Time::getDefaultFormat(), L"\n"));
		wstring_ptr wstrDate(date.getTime().format(wstrFormat.get(),
			bUseSendersTimeZone ? Time::FORMAT_ORIGINAL : Time::FORMAT_LOCAL));
		if (!pBuf->append(wstrDate.get()))
			return false;
	}
	
	AttachmentParser::AttachmentList names;
	AttachmentParser::AttachmentListFree free(names);
	AttachmentParser(part_).getAttachments(AttachmentParser::GAF_INCLUDEDELETED, &names);
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
	if (!getBodyText(0, pwszCharset, rfc822Mode, pBuf))
		return false;
	
	return true;
}

BodyData qm::PartUtil::getBodyData() const
{
	const Part* pEnclosedPart = part_.getEnclosedPart();
	if (pEnclosedPart)
		return BodyData(pEnclosedPart->getContent());
	else
		return BodyData(part_.getBodyData());
}

bool qm::PartUtil::getDigest(MessageList* pList) const
{
	assert(pList);
	
	CONTAINER_DELETER(deleter, *pList);
	
	DigestMode mode = getDigestMode();
	if (mode == DIGEST_NONE)
		return true;
	
	AddressListParser to;
	Part::Field fieldTo = part_.getField(L"To", &to);
	
	AddressListParser replyTo;
	Part::Field fieldReplyTo = part_.getField(L"Reply-To", &replyTo);
	
	switch (mode) {
	case DIGEST_MULTIPART:
		{
			const Part::PartList& l = part_.getPartList();
			for (Part::PartList::const_iterator it = l.begin(); it != l.end(); ++it) {
				const Part* pEnclosedPart = (*it)->getEnclosedPart();
				if (pEnclosedPart || (*it)->isText()) {
					xstring_size_ptr strContent;
					if (pEnclosedPart)
						strContent = pEnclosedPart->getContent();
					else
						strContent = (*it)->getContent();
					
					std::auto_ptr<Message> pMessage(new Message());
					if (!pMessage->create(strContent.get(), strContent.size(), Message::FLAG_NONE))
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
			
			wxstring_size_ptr wstrBody(part_.getBodyText());
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
				
				std::auto_ptr<Message> pMessage(creator.createMessage(p, pEnd - p));
				
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
	if (!pContentType || _wcsicmp(pContentType->getMediaType(), L"text") == 0) {
		UnstructuredParser subject;
		if (part_.getField(L"Subject", &subject) == Part::FIELD_EXIST) {
			wstring_ptr wstrSubject(tolower(subject.getValue()));
			if (wcsstr(wstrSubject.get(), L"digest")) {
				wxstring_size_ptr wstrBody = part_.getBodyText();
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
	else if (PartUtil::isMultipart(pContentType)) {
		if (_wcsicmp(pContentType->getSubType(), L"mixed") == 0 ||
			_wcsicmp(pContentType->getSubType(), L"digest") == 0) {
			const Part::PartList& l = part_.getPartList();
			Part::PartList::const_iterator it = l.begin();
			while (it != l.end()) {
				const Part* pEnclosedPart = (*it)->getEnclosedPart();
				if (!pEnclosedPart && !(*it)->isText())
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
	if (PartUtil::isMultipart(pContentType)) {
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
	if (PartUtil::isMultipart(pContentType)) {
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

const Part* qm::PartUtil::getEnclosingPart(const Part* pCandidatePart) const
{
	assert(pCandidatePart);
	
	const Part* pPart = 0;
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

Part* qm::PartUtil::getEnclosingPart(Part* pCandidatePart) const
{
	return const_cast<Part*>(getEnclosingPart(static_cast<const Part*>(pCandidatePart)));
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

bool qm::PartUtil::normalizeHeader(const CHAR* pszHeader,
								   XStringBuffer<qs::WXSTRING>* pBuf)
{
	assert(pszHeader);
	assert(pBuf);
	
	StringBuffer<STRING> buf;
	bool bSkipWhitespace = false;
	for (const CHAR* p = pszHeader; *p; ++p) {
		if (*p == '\r' && *(p + 1) == '\n') {
			buf.append(' ');
			bSkipWhitespace = true;
			++p;
		}
		else if (bSkipWhitespace) {
			if (*p != ' ' && *p != '\t') {
				bSkipWhitespace = false;
				buf.append(*p);
			}
		}
		else {
			buf.append(*p);
		}
	}
	
	wstring_ptr wstr(FieldParser::decode(buf.getCharArray(), buf.getLength(), false, 0));
	if (wstr.get()) {
		if (!pBuf->append(wstr.get()))
			return false;
	}
	else {
		if (!a2w(buf.getCharArray(), buf.getLength(), pBuf))
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

bool qm::PartUtil::isMultipart(const qs::ContentTypeParser* pContentType)
{
	return pContentType && _wcsicmp(pContentType->getMediaType(), L"multipart") == 0;
}

bool qm::PartUtil::isRfc822Rendered(Rfc822Mode rfc822Mode,
									bool bAttachment)
{
	switch (rfc822Mode) {
	case RFC822_AUTO:
		return !bAttachment;
	case RFC822_INLINE:
		return true;
	case RFC822_ATTACHMENT:
		return false;
	default:
		assert(false);
		return false;
	}
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
	
	if (!wstrName.get() && PartUtil::isContentType(part_.getContentType(), L"message", L"rfc822")) {
		UnstructuredParser subject;
		if (part_.getEnclosedPart()->getField(L"Subject", &subject) == Part::FIELD_EXIST)
			wstrName = concat(subject.getValue(), L".eml");
	}
	
	if (wstrName.get()) {
		if (File::isDeviceName(wstrName.get()))
			wstrName = concat(L"_", wstrName.get());
		
#ifdef UNICODE
		for (WCHAR* p = wstrName.get(); *p; ++p) {
			if (!TextUtil::isFileNameChar(*p))
				*p = L'_';
		}
#else
		string_ptr strName(wcs2mbs(wstrName.get()));
		for (CHAR* p = strName.get(); *p; ++p) {
			if (::IsDBCSLeadByte(*p))
				++p;
			else if (!TextUtil::isFileNameChar(*p))
				*p = '_';
		}
		wstrName = mbs2wcs(strName.get());
#endif
	}
	
	return wstrName;
}

void qm::AttachmentParser::getAttachments(unsigned int nFlags,
										  AttachmentList* pList) const
{
	assert(pList);
	
	if ((nFlags & GAF_INCLUDEDELETED) == 0 && isAttachmentDeleted())
		return;
	
	if (part_.isMultipart()) {
		bool bAppleDouble = _wcsicmp(part_.getContentType()->getSubType(), L"appledouble") == 0;
		const Part::PartList& l = part_.getPartList();
		for (Part::PartList::const_iterator it = l.begin(); it != l.end(); ++it) {
			const Part* p = *it;
			if ((nFlags & GAF_INCLUDEAPPLEFILE) == 0 && bAppleDouble) {
				const ContentTypeParser* pContentType = p->getContentType();
				if (pContentType &&
					_wcsicmp(pContentType->getMediaType(), L"application") == 0 &&
					_wcsicmp(pContentType->getSubType(), L"applefile") == 0)
					continue;
			}
			AttachmentParser(*p).getAttachments(nFlags, pList);
		}
	}
	else if (part_.isAttachment()) {
		wstring_ptr wstrName(getName());
		if (!wstrName.get() || !*wstrName.get())
			wstrName = allocWString(L"Untitled");
		wstring_ptr wstrOrigName = allocWString(wstrName.get());
		
		int n = 1;
		while (true) {
			AttachmentList::iterator it = std::find_if(pList->begin(), pList->end(),
				boost::bind(string_equal<WCHAR>(),
					boost::bind(&AttachmentList::value_type::first, _1), wstrName.get()));
			if (it == pList->end())
				break;
			
			WCHAR wsz[32];
			_snwprintf(wsz, countof(wsz), L"[%d]", n++);
			const WCHAR* pExt = wcsrchr(wstrOrigName.get(), L'.');
			StringBuffer<WSTRING> buf;
			buf.append(wstrOrigName.get(), pExt ? pExt - wstrOrigName.get() : -1);
			buf.append(wsz);
			if (pExt)
				buf.append(pExt);
			wstrName = buf.getString();
		}
		pList->push_back(AttachmentList::value_type(
			wstrName.get(), const_cast<Part*>(&part_)));
		wstrName.release();
	}
	
	if (nFlags & GAF_INCLUDERFC822 && part_.getEnclosedPart())
		AttachmentParser(*part_.getEnclosedPart()).getAttachments(nFlags, pList);
}

AttachmentParser::Result qm::AttachmentParser::detach(const WCHAR* pwszDir,
													  const WCHAR* pwszName,
													  bool bAddZoneId,
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
	
	if (File::isFileExisting(wstrPath.get())) {
		wstring_ptr wstr(pCallback->confirmOverwrite(wstrPath.get()));
		if (!wstr.get())
			return RESULT_CANCEL;
		wstrPath = wstr;
	}
	
	FileOutputStream stream(wstrPath.get());
	if (!stream || !detach(&stream) || !stream.close())
		return RESULT_FAIL;
	
#ifndef _WIN32_WCE
	if (bAddZoneId) {
		wstring_ptr wstrZonePath = concat(wstrPath.get(), L":Zone.Identifier");
		FileOutputStream zoneStream(wstrZonePath.get());
		if (!!zoneStream) {
			const CHAR* pszZoneInfo = "[ZoneTransfer]\r\nZoneId=3\r\n";
			bool b = zoneStream.write(reinterpret_cast<const unsigned char*>(pszZoneInfo), strlen(pszZoneInfo)) != -1;
			b = zoneStream.close() && b;
			if (!b) {
				W2T(wstrZonePath.get(), ptszZonePath);
				::DeleteFile(ptszZonePath);
			}
		}
	}
#endif
	
	if (pwstrPath)
		*pwstrPath = wstrPath;
	
	return RESULT_OK;
}

bool qm::AttachmentParser::detach(OutputStream* pStream) const
{
	const Part* pEnclosedPart = part_.getEnclosedPart();
	
	if (pEnclosedPart) {
		xstring_size_ptr strContent(pEnclosedPart->getContent());
		size_t nLen = strContent.size();
		const unsigned char* p = reinterpret_cast<const unsigned char*>(strContent.get());
		if (pStream->write(p, nLen) != nLen)
			return false;
	}
	else {
		const CHAR* pBody = part_.getBody();
		size_t nLen = strlen(pBody);
		const unsigned char* p = reinterpret_cast<const unsigned char*>(pBody);
		
		std::auto_ptr<Encoder> pEncoder;
		ContentTransferEncodingParser contentTransferEncoding;
		if (part_.getField(L"Content-Transfer-Encoding", &contentTransferEncoding) == Part::FIELD_EXIST) {
			const WCHAR* pwszEncoding = contentTransferEncoding.getEncoding();
			if (_wcsicmp(pwszEncoding, L"7bit") != 0 &&
				_wcsicmp(pwszEncoding, L"8bit") != 0) {
				pEncoder= EncoderFactory::getInstance(pwszEncoding);
				if (!pEncoder.get())
					return false;
			}
		}
		if (pEncoder.get()) {
			ByteInputStream inputStream(p, nLen, false);
			BufferedOutputStream bufferedStream(pStream, false);
			if (!pEncoder->decode(&inputStream, &bufferedStream) ||
				!bufferedStream.close())
				return false;
		}
		else {
			if (pStream->write(p, nLen) != nLen)
				return false;
		}
	}
	
	return true;
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
		boost::bind(&freeWString,
			boost::bind(&AttachmentList::value_type::first, _1)));
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

qm::XQMAILAttachmentParser::XQMAILAttachmentParser(const WCHAR* pwsz)
{
	assert(pwsz);
	
	parse(pwsz, &listAttachment_);
}

qm::XQMAILAttachmentParser::~XQMAILAttachmentParser()
{
	std::for_each(listAttachment_.begin(), listAttachment_.end(), &freeWString);
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
	parse(wstrValue.get(), &listAttachment_);
	
	return Part::FIELD_EXIST;
}

string_ptr qm::XQMAILAttachmentParser::unparse(const Part& part) const
{
	wstring_ptr wstrValue(format(listAttachment_));
	return encode(wstrValue.get(), -1, L"utf-8", L"B", false, false);
}

void qm::XQMAILAttachmentParser::parse(const WCHAR* pwsz,
									   AttachmentList* pList)
{
	assert(pwsz);
	assert(pList);
	assert(pList->empty());
	
	StringBuffer<WSTRING> buf;
	const WCHAR* p = pwsz;
	while (true) {
		if (*p == L',' || *p == L'\0') {
			wstring_ptr wstrName(buf.getString());
			if (*wstrName.get()) {
				pList->push_back(wstrName.get());
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
