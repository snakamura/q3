/*
 * $Id$
 *
 * Copyright(C) 1998-2004 Satoshi Nakamura
 * All rights reserved.
 *
 */

#include <qsconv.h>

#include "driver.h"
#include "gpgdriver.h"
#include "pgp.h"
#include "pgpdriver.h"

using namespace qmpgp;
using namespace qm;
using namespace qs;


/****************************************************************************
 *
 * PGPUtilityImpl
 *
 */

qmpgp::PGPUtilityImpl::PGPUtilityImpl(Profile* pProfile) :
	pProfile_(pProfile)
{
	if (pProfile->getInt(L"PGP", L"UseGPG", 1))
		pDriver_.reset(new GPGDriver(pProfile));
	else
		pDriver_.reset(new PGPDriver(pProfile));
}

qmpgp::PGPUtilityImpl::~PGPUtilityImpl()
{
}

PGPUtility::Type qmpgp::PGPUtilityImpl::getType(const qs::Part& part,
												bool bCheckInline) const
{
	switch (getType(part.getContentType())) {
	case TYPE_NONE:
		if (bCheckInline && part.isText()) {
			const CHAR* pBody = part.getBody();
			if (strncmp(pBody, "-----BEGIN PGP SIGNED MESSAGE-----\r\n", 36) == 0)
				return TYPE_INLINESIGNED;
			else if (strncmp(pBody, "-----BEGIN PGP MESSAGE-----\r\n", 29) == 0)
				return TYPE_INLINEENCRYPTED;
		}
		break;
	case TYPE_MIMEENCRYPTED:
		if (part.getPartCount() == 2) {
			const Part* pEncryptedPart = part.getPart(0);
			const ContentTypeParser* pContentType = pEncryptedPart->getContentType();
			if (pContentType &&
				_wcsicmp(pContentType->getMediaType(), L"application") == 0 &&
				_wcsicmp(pContentType->getSubType(), L"pgp-encrypted") == 0)
				return TYPE_MIMEENCRYPTED;
		}
		break;
	case TYPE_MIMESIGNED:
		if (part.getPartCount() == 2) {
			const Part* pSignaturePart = part.getPart(1);
			const ContentTypeParser* pContentType = pSignaturePart->getContentType();
			if (pContentType &&
				_wcsicmp(pContentType->getMediaType(), L"application") == 0 &&
				_wcsicmp(pContentType->getSubType(), L"pgp-signature") == 0)
				return TYPE_MIMESIGNED;
		}
		break;
	default:
		assert(false);
		break;
	}
	return TYPE_NONE;
}

PGPUtility::Type qmpgp::PGPUtilityImpl::getType(const ContentTypeParser* pContentType) const
{
	if (!pContentType)
		return TYPE_NONE;
	
	const WCHAR* pwszMediaType = pContentType->getMediaType();
	const WCHAR* pwszSubType = pContentType->getSubType();
	if (_wcsicmp(pwszMediaType, L"multipart") == 0) {
		if (_wcsicmp(pwszSubType, L"encrypted") == 0) {
			wstring_ptr wstrProtocol(pContentType->getParameter(L"protocol"));
			if (wstrProtocol.get() && _wcsicmp(wstrProtocol.get(), L"application/pgp-encrypted") == 0)
				return TYPE_MIMEENCRYPTED;
		}
		else if (_wcsicmp(pwszSubType, L"signed") == 0) {
			wstring_ptr wstrProtocol(pContentType->getParameter(L"protocol"));
			if (wstrProtocol.get() && _wcsicmp(wstrProtocol.get(), L"application/pgp-signature") == 0)
				return TYPE_MIMESIGNED;
		}
	}
	
	return TYPE_NONE;
}

xstring_ptr qmpgp::PGPUtilityImpl::sign(Part* pPart,
										bool bMime,
										const WCHAR* pwszUserId,
										const WCHAR* pwszPassphrase) const
{
	assert(pPart);
	assert(pwszUserId);
	assert(pwszPassphrase);
	
	if (!bMime && (pPart->isMultipart() || !pPart->isText()))
		bMime = true;
	
	if (bMime) {
		xstring_ptr strHeader(allocXString(pPart->getHeader()));
		if (!strHeader.get())
			return 0;
		
		PrefixFieldFilter filter("content-", true);
		if (!pPart->removeFields(&filter))
			return 0;
		
		xstring_ptr strContent(pPart->getContent());
		if (!strContent.get())
			return 0;
		
		xstring_ptr strSignature(pDriver_->sign(strContent.get(),
			Driver::SIGNFLAG_DETACH, pwszUserId, pwszPassphrase));
		if (!strSignature.get())
			return 0;
		
		return createMultipartSignedMessage(strHeader.get(), *pPart, strSignature.get());
	}
	else {
		xstring_ptr strContent(pPart->getContent());
		if (!strContent.get())
			return 0;
		
		const CHAR* pBody = strstr(strContent.get(), "\r\n\r\n");
		assert(pBody);
		pBody += 4;
		
		xstring_ptr strBody(pDriver_->sign(pBody,
			Driver::SIGNFLAG_CLEARTEXT, pwszUserId, pwszPassphrase));
		if (!strBody.get())
			return 0;
		
		return createMessage(pPart->getHeader(), strBody.get(), 0);
	}
}

xstring_ptr qmpgp::PGPUtilityImpl::encrypt(Part* pPart,
										   bool bMime) const
{
	assert(pPart);
	
	if (!bMime && (pPart->isMultipart() || !pPart->isText()))
		bMime = true;
	
	Driver::UserIdList listRecipient;
	StringListFree<Driver::UserIdList> free(listRecipient);
	getRecipients(*pPart, &listRecipient);
	
	if (bMime) {
		xstring_ptr strHeader(allocXString(pPart->getHeader()));
		if (!strHeader.get())
			return 0;
		
		PrefixFieldFilter filter("content-", true);
		if (!pPart->removeFields(&filter))
			return 0;
		
		xstring_ptr strContent(pPart->getContent());
		if (!strContent.get())
			return 0;
		
		xstring_ptr strBody(pDriver_->encrypt(strContent.get(), listRecipient));
		if (!strBody.get())
			return 0;
		
		return createMultipartEncryptedMessage(strHeader.get(), strBody.get());
	}
	else {
		xstring_ptr strContent(pPart->getContent());
		if (!strContent.get())
			return 0;
		
		const CHAR* pBody = strstr(strContent.get(), "\r\n\r\n");
		assert(pBody);
		pBody += 4;
		
		xstring_ptr strBody(pDriver_->encrypt(pBody, listRecipient));
		if (!strBody.get())
			return 0;
		
		return createMessage(pPart->getHeader(), strBody.get(), 0);
	}
}

xstring_ptr qmpgp::PGPUtilityImpl::signAndEncrypt(Part* pPart,
												  bool bMime,
												  const WCHAR* pwszUserId,
												  const WCHAR* pwszPassphrase) const
{
	assert(pPart);
	assert(pwszUserId);
	assert(pwszPassphrase);
	
	if (!bMime && (pPart->isMultipart() || !pPart->isText()))
		bMime = true;
	
	Driver::UserIdList listRecipient;
	StringListFree<Driver::UserIdList> free(listRecipient);
	getRecipients(*pPart, &listRecipient);
	
	if (bMime) {
		xstring_ptr strHeader(allocXString(pPart->getHeader()));
		if (!strHeader.get())
			return 0;
		
		PrefixFieldFilter filter("content-", true);
		if (!pPart->removeFields(&filter))
			return 0;
		
		xstring_ptr strContent(pPart->getContent());
		if (!strContent.get())
			return 0;
		
		xstring_ptr strBody(pDriver_->signAndEncrypt(strContent.get(),
			pwszUserId, pwszPassphrase, listRecipient));
		if (!strBody.get())
			return 0;
		
		return createMultipartEncryptedMessage(strHeader.get(), strBody.get());
	}
	else {
		xstring_ptr strContent(pPart->getContent());
		if (!strContent.get())
			return 0;
		
		const CHAR* pBody = strstr(strContent.get(), "\r\n\r\n");
		assert(pBody);
		pBody += 4;
		
		xstring_ptr strBody(pDriver_->signAndEncrypt(pBody,
			pwszUserId, pwszPassphrase, listRecipient));
		if (!strBody.get())
			return 0;
		
		return createMessage(pPart->getHeader(), strBody.get(), 0);
	}
}

xstring_ptr qmpgp::PGPUtilityImpl::verify(const Part& part,
										  bool bMime,
										  unsigned int* pnVerify) const
{
	assert(pnVerify);
	
	if (bMime) {
		assert(getType(part, false) == TYPE_MIMESIGNED);
		
		*pnVerify = VERIFY_OK;
		
		xstring_ptr strContent(part.getPart(0)->getContent());
		if (!strContent.get())
			return 0;
		
		wstring_ptr wstrUserId;
		bool bVerified = pDriver_->verify(strContent.get(),
			part.getPart(1)->getBody(), &wstrUserId);
		
		if (!bVerified)
			*pnVerify = VERIFY_FAILED;
		if (!checkUserId(part, wstrUserId.get()))
			*pnVerify |= VERIFY_ADDRESSNOTMATCH;
		
		return createMessage(strContent.get(), part, wstrUserId.get());
	}
	else {
		assert(getType(part, true) == TYPE_INLINESIGNED);
		
		wstring_ptr wstrUserId;
		xstring_ptr strBody(pDriver_->decryptAndVerify(
			part.getBody(), 0, pnVerify, &wstrUserId));
		if (!strBody.get())
			return 0;
		if (!checkUserId(part, wstrUserId.get()))
			*pnVerify |= VERIFY_ADDRESSNOTMATCH;
		
		return createMessage(part.getHeader(), strBody.get(), wstrUserId.get());
	}
}

xstring_ptr qmpgp::PGPUtilityImpl::decryptAndVerify(const Part& part,
													bool bMime,
													const WCHAR* pwszPassphrase,
													unsigned int* pnVerify) const
{
	assert(pnVerify);
	
	*pnVerify = VERIFY_NONE;
	
	if (bMime) {
		assert(getType(part, false) == TYPE_MIMEENCRYPTED);
		
		wstring_ptr wstrUserId;
		xstring_ptr strContent(pDriver_->decryptAndVerify(part.getPart(1)->getBody(),
			pwszPassphrase, pnVerify, &wstrUserId));
		if (!strContent.get())
			return 0;
		if (*pnVerify != VERIFY_NONE) {
			if (!checkUserId(part, wstrUserId.get()))
				*pnVerify |= VERIFY_ADDRESSNOTMATCH;
		}
		return createMessage(strContent.get(), part, wstrUserId.get());
	}
	else {
		assert(getType(part, true) == TYPE_INLINEENCRYPTED);
		
		wstring_ptr wstrUserId;
		xstring_ptr strBody(pDriver_->decryptAndVerify(part.getBody(),
			pwszPassphrase, pnVerify, &wstrUserId));
		if (!strBody.get())
			return 0;
		if (*pnVerify != VERIFY_NONE) {
			if (!checkUserId(part, wstrUserId.get()))
				*pnVerify |= VERIFY_ADDRESSNOTMATCH;
		}
		return createMessage(part.getHeader(), strBody.get(), wstrUserId.get());
	}
}

void qmpgp::PGPUtilityImpl::getRecipients(const Part& part,
										  Driver::UserIdList* pListUserId)
{
	const WCHAR* pwszAddresses[] = {
		L"To",
		L"Cc",
		L"Bcc"
	};
	for (int n = 0; n < countof(pwszAddresses); ++n) {
		AddressListParser addressList(0);
		Part::Field f = part.getField(pwszAddresses[n], &addressList);
		if (f == Part::FIELD_EXIST) {
			wstring_ptr wstrAddresses(addressList.getAddresses());
			
			const WCHAR* p = wcstok(wstrAddresses.get(), L",");
			while (p) {
				wstring_ptr wstr(allocWString(p));
				pListUserId->push_back(wstr.release());
				p = wcstok(0, L",");
			}
		}
	}
}

bool qmpgp::PGPUtilityImpl::checkUserId(const qs::Part& part,
										const WCHAR* pwszUserId)
{
	if (!pwszUserId)
		return false;
	
	const WCHAR* pStart = wcsrchr(pwszUserId, L'<');
	if (!pStart)
		return false;
	const WCHAR* pEnd = wcsrchr(pwszUserId, L'>');
	if (!pEnd || pStart > pEnd)
		return false;
	
	wstring_ptr wstrUserId(allocWString(pStart + 1, pEnd - pStart - 1));
	
	AddressListParser from(AddressListParser::FLAG_DISALLOWGROUP);
	if (part.getField(L"From", &from) == Part::FIELD_EXIST &&
		contains(from, wstrUserId.get()))
		return true;
	
	AddressListParser sender(AddressListParser::FLAG_DISALLOWGROUP);
	if (part.getField(L"Sender", &sender) == Part::FIELD_EXIST &&
		contains(sender, wstrUserId.get()))
		return true;
	
	return false;
}

bool qmpgp::PGPUtilityImpl::contains(const AddressListParser& addressList,
									 const WCHAR* pwszAddress)
{
	const AddressListParser::AddressList& l = addressList.getAddressList();
	for (AddressListParser::AddressList::const_iterator it = l.begin(); it != l.end(); ++it) {
		if (contains(**it, pwszAddress))
			return true;
	}
	return false;
}

bool qmpgp::PGPUtilityImpl::contains(const AddressParser& address,
									 const WCHAR* pwszAddress)
{
	assert(!address.getGroup());
	wstring_ptr wstrAddress(address.getAddress());
	return _wcsicmp(wstrAddress.get(), pwszAddress) == 0;
}

xstring_ptr qmpgp::PGPUtilityImpl::createMessage(const CHAR* pszHeader,
												 const CHAR* pszBody,
												 const WCHAR* pwszSignedBy)
{
	Part header;
	if (pwszSignedBy) {
		if (!header.create(0, pszHeader, -1))
			return 0;
		
		UnstructuredParser signedBy(pwszSignedBy, L"utf-8");
		if (!header.setField(L"X-QMAIL-SignedBy", signedBy))
			return 0;
		
		pszHeader = header.getHeader();
	}
	
	XStringBuffer<XSTRING> buf;
	if (!buf.append(pszHeader) || !buf.append("\r\n") || !buf.append(pszBody))
		return 0;
	return buf.getXString();
}

xstring_ptr qmpgp::PGPUtilityImpl::createMessage(const CHAR* pszContent,
												 const Part& part,
												 const WCHAR* pwszSignedBy)
{
	size_t nLen = strlen(pszContent);
	
	BMFindString<STRING> bmfs("\r\n\r\n");
	const CHAR* pBody = bmfs.find(pszContent, nLen);
	if (pBody)
		pBody += 4;
	size_t nHeaderLen = pBody ? pBody - pszContent : nLen;
	size_t nBodyLen = pBody ? pszContent + nLen - pBody : 0;
	
	Part header;
	if (!header.create(0, pszContent, nHeaderLen))
		return 0;
	
	PrefixFieldFilter filter("content-", true);
	if (!header.copyFields(part, &filter))
		return 0;
	
	if (pwszSignedBy) {
		UnstructuredParser signedBy(pwszSignedBy, L"utf-8");
		if (!header.setField(L"X-QMAIL-SignedBy", signedBy))
			return 0;
	}
	
	const CHAR* pszHeader = header.getHeader();
	size_t nNewHeaderLen = strlen(pszHeader);
	
	xstring_ptr strMessage(allocXString(nNewHeaderLen + nBodyLen + 5));
	if (!strMessage.get())
		return 0;
	
	CHAR* p = strMessage.get();
	strncpy(p, pszHeader, nNewHeaderLen);
	p += nNewHeaderLen;
	strncpy(p, "\r\n", 2);
	p += 2;
	if (pBody)
		strncpy(p, pBody, nBodyLen);
	*(p + nBodyLen) = '\0';
	
	return strMessage;
}

xstring_ptr qmpgp::PGPUtilityImpl::createMultipartSignedMessage(const CHAR* pszHeader,
																const Part& part,
																const CHAR* pszSignature)
{
	assert(pszHeader);
	assert(pszSignature);
	
	Part msg;
	{
		if (!msg.create(0, pszHeader, -1))
			return 0;
		
		PrefixFieldFilter filter("content-");
		if (!msg.removeFields(&filter))
			return 0;
		msg.setBody(0);
		
		ContentTypeParser contentType(L"multipart", L"signed");
		WCHAR wszBoundary[128];
		Time time(Time::getCurrentTime());
		swprintf(wszBoundary, L"__boundary-%04d%02d%02d%02d%02d%02d%03d%04d__",
			time.wYear, time.wMonth, time.wDay, time.wHour, time.wMinute,
			time.wSecond, time.wMilliseconds, ::GetCurrentThreadId());
		contentType.setParameter(L"boundary", wszBoundary);
		contentType.setParameter(L"protocol", L"application/pgp-signature");
		contentType.setParameter(L"micalg", L"pgp-sha1");
		if (!msg.setField(L"Content-Type", contentType))
			return false;
	}
	
	std::auto_ptr<Part> pPartContent(part.clone());
	if (!pPartContent.get())
		return false;
	msg.addPart(pPartContent);
	
	std::auto_ptr<Part> pPartSignature(new Part());
	{
		ContentTypeParser contentType(L"application", L"pgp-signature");
		contentType.setParameter(L"name", L"signature.asc");
		if (!pPartSignature->setField(L"Content-Type", contentType))
			return false;
		
		if (!pPartSignature->setBody(pszSignature, -1))
			return false;
	}
	msg.addPart(pPartSignature);
	
	if (!msg.sortHeader())
		return 0;
	
	return msg.getContent();
}

xstring_ptr qmpgp::PGPUtilityImpl::createMultipartEncryptedMessage(const CHAR* pszHeader,
																   const CHAR* pszBody)
{
	assert(pszHeader);
	assert(pszBody);
	
	Part msg;
	{
		if (!msg.create(0, pszHeader, -1))
			return 0;
		
		PrefixFieldFilter filter("content-");
		if (!msg.removeFields(&filter))
			return 0;
		msg.setBody(0);
		
		ContentTypeParser contentType(L"multipart", L"encrypted");
		WCHAR wszBoundary[128];
		Time time(Time::getCurrentTime());
		swprintf(wszBoundary, L"__boundary-%04d%02d%02d%02d%02d%02d%03d%04d__",
			time.wYear, time.wMonth, time.wDay, time.wHour, time.wMinute,
			time.wSecond, time.wMilliseconds, ::GetCurrentThreadId());
		contentType.setParameter(L"boundary", wszBoundary);
		contentType.setParameter(L"protocol", L"application/pgp-encrypted");
		if (!msg.setField(L"Content-Type", contentType))
			return false;
	}
	
	std::auto_ptr<Part> pPartHeader(new Part());
	{
		ContentTypeParser contentType(L"application", L"pgp-encrypted");
		if (!pPartHeader->setField(L"Content-Type", contentType))
			return false;
		
		if (!pPartHeader->setBody("Version: 1\r\n", -1))
			return false;
	}
	msg.addPart(pPartHeader);
	
	std::auto_ptr<Part> pPartBody(new Part());
	{
		ContentTypeParser contentType(L"application", L"octet-stream");
		if (!pPartBody->setField(L"Content-Type", contentType))
			return false;
		
		if (!pPartBody->setBody(pszBody, -1))
			return false;
	}
	msg.addPart(pPartBody);
	
	if (!msg.sortHeader())
		return 0;
	
	return msg.getContent();
}


/****************************************************************************
 *
 * PGPFactoryImpl
 *
 */

PGPFactoryImpl qmpgp::PGPFactoryImpl::factory__;

qmpgp::PGPFactoryImpl::PGPFactoryImpl()
{
	registerFactory(this);
}

qmpgp::PGPFactoryImpl::~PGPFactoryImpl()
{
	unregisterFactory(this);
}

std::auto_ptr<PGPUtility> qmpgp::PGPFactoryImpl::createPGPUtility(Profile* pProfile)
{
	return std::auto_ptr<PGPUtility>(new PGPUtilityImpl(pProfile));
}
