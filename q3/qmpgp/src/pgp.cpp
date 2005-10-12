/*
 * $Id$
 *
 * Copyright(C) 1998-2005 Satoshi Nakamura
 * All rights reserved.
 *
 */

#include <qsconv.h>
#include <qsencoder.h>

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
			malloc_size_ptr<unsigned char> pBodyData;
			std::pair<const CHAR*, size_t> body(getBodyData(part, &pBodyData));
			if (findInline("-----BEGIN PGP SIGNED MESSAGE-----\r\n", body.first, body.second))
				return TYPE_INLINESIGNED;
			else if (findInline("-----BEGIN PGP MESSAGE-----\r\n", body.first, body.second))
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

xstring_size_ptr qmpgp::PGPUtilityImpl::sign(Part* pPart,
											 bool bMime,
											 const WCHAR* pwszUserId,
											 const WCHAR* pwszPassphrase) const
{
	assert(pPart);
	assert(pwszUserId);
	assert(pwszPassphrase);
	
	std::auto_ptr<Driver> pDriver(getDriver());
	
	if (!bMime && (pPart->isMultipart() || !pPart->isText()))
		bMime = true;
	
	if (bMime) {
		xstring_ptr strHeader(allocXString(pPart->getHeader()));
		if (!strHeader.get())
			return xstring_size_ptr();
		
		PrefixFieldFilter filter("content-", true);
		if (!pPart->removeFields(&filter))
			return xstring_size_ptr();
		
		xstring_size_ptr strContent(pPart->getContent());
		if (!strContent.get())
			return xstring_size_ptr();
		
		xstring_size_ptr strSignature(pDriver->sign(strContent.get(),
			-1, Driver::SIGNFLAG_DETACH, pwszUserId, pwszPassphrase));
		if (!strSignature.get())
			return xstring_size_ptr();
		
		return createMultipartSignedMessage(strHeader.get(),
			*pPart, strSignature.get(), strSignature.size());
	}
	else {
		xstring_size_ptr strContent(pPart->getContent());
		if (!strContent.get())
			return xstring_size_ptr();
		
		const CHAR* pBody = Part::getBody(strContent.get(), strContent.size());
		xstring_size_ptr strBody(pDriver->sign(pBody, -1,
			Driver::SIGNFLAG_CLEARTEXT, pwszUserId, pwszPassphrase));
		if (!strBody.get())
			return xstring_size_ptr();
		
		return createMessage(pPart->getHeader(), strBody.get(), strBody.size());
	}
}

xstring_size_ptr qmpgp::PGPUtilityImpl::encrypt(Part* pPart,
												bool bMime) const
{
	assert(pPart);
	
	std::auto_ptr<Driver> pDriver(getDriver());
	
	if (!bMime && (pPart->isMultipart() || !pPart->isText()))
		bMime = true;
	
	Driver::UserIdList listRecipient;
	StringListFree<Driver::UserIdList> free(listRecipient);
	getRecipients(*pPart, &listRecipient);
	
	if (bMime) {
		xstring_ptr strHeader(allocXString(pPart->getHeader()));
		if (!strHeader.get())
			return xstring_size_ptr();
		
		PrefixFieldFilter filter("content-", true);
		if (!pPart->removeFields(&filter))
			return xstring_size_ptr();
		
		xstring_size_ptr strContent(pPart->getContent());
		if (!strContent.get())
			return xstring_size_ptr();
		
		xstring_size_ptr strBody(pDriver->encrypt(strContent.get(), -1, listRecipient));
		if (!strBody.get())
			return xstring_size_ptr();
		
		return createMultipartEncryptedMessage(strHeader.get(), strBody.get(), strBody.size());
	}
	else {
		xstring_size_ptr strContent(pPart->getContent());
		if (!strContent.get())
			return xstring_size_ptr();
		
		const CHAR* pBody = Part::getBody(strContent.get(), strContent.size());
		xstring_size_ptr strBody(pDriver->encrypt(pBody, -1, listRecipient));
		if (!strBody.get())
			return xstring_size_ptr();
		
		return createMessage(pPart->getHeader(), strBody.get(), strBody.size());
	}
}

xstring_size_ptr qmpgp::PGPUtilityImpl::signAndEncrypt(Part* pPart,
													   bool bMime,
													   const WCHAR* pwszUserId,
													   const WCHAR* pwszPassphrase) const
{
	assert(pPart);
	assert(pwszUserId);
	assert(pwszPassphrase);
	
	std::auto_ptr<Driver> pDriver(getDriver());
	
	if (!bMime && (pPart->isMultipart() || !pPart->isText()))
		bMime = true;
	
	Driver::UserIdList listRecipient;
	StringListFree<Driver::UserIdList> free(listRecipient);
	getRecipients(*pPart, &listRecipient);
	
	if (bMime) {
		xstring_ptr strHeader(allocXString(pPart->getHeader()));
		if (!strHeader.get())
			return xstring_size_ptr();
		
		PrefixFieldFilter filter("content-", true);
		if (!pPart->removeFields(&filter))
			return xstring_size_ptr();
		
		xstring_size_ptr strContent(pPart->getContent());
		if (!strContent.get())
			return xstring_size_ptr();
		
		xstring_size_ptr strBody(pDriver->signAndEncrypt(strContent.get(),
			-1, pwszUserId, pwszPassphrase, listRecipient));
		if (!strBody.get())
			return xstring_size_ptr();
		
		return createMultipartEncryptedMessage(strHeader.get(), strBody.get(), strBody.size());
	}
	else {
		xstring_size_ptr strContent(pPart->getContent());
		if (!strContent.get())
			return xstring_size_ptr();
		
		const CHAR* pBody = Part::getBody(strContent.get(), strContent.size());
		xstring_size_ptr strBody(pDriver->signAndEncrypt(pBody,
			-1, pwszUserId, pwszPassphrase, listRecipient));
		if (!strBody.get())
			return xstring_size_ptr();
		
		return createMessage(pPart->getHeader(), strBody.get(), strBody.size());
	}
}

xstring_size_ptr qmpgp::PGPUtilityImpl::verify(const Part& part,
											   bool bMime,
											   unsigned int* pnVerify,
											   wstring_ptr* pwstrSignedBy,
											   Validity* pValidity,
											   qs::wstring_ptr* pwstrInfo) const
{
	assert(pnVerify);
	assert(pwstrSignedBy);
	assert(pValidity);
	assert(pwstrInfo);
	
	*pnVerify = VERIFY_NONE;
	pwstrSignedBy->reset(0);
	
	std::auto_ptr<Driver> pDriver(getDriver());
	
	if (bMime) {
		assert(getType(part, false) == TYPE_MIMESIGNED);
		
		xstring_size_ptr strContent(part.getPart(0)->getContent());
		if (!strContent.get())
			return xstring_size_ptr();
		
		bool bVerified = pDriver->verify(strContent.get(), -1,
			part.getPart(1)->getBody(), pwstrSignedBy, pValidity, pwstrInfo);
		
		*pnVerify = bVerified ? VERIFY_OK : VERIFY_FAILED;
		if (!checkUserId(part, pwstrSignedBy->get()))
			*pnVerify |= VERIFY_ADDRESSNOTMATCH;
		
		return createMessage(strContent.get(), strContent.size(), part);
	}
	else {
		assert(getType(part, true) == TYPE_INLINESIGNED);
		
		malloc_size_ptr<unsigned char> pBodyData;
		std::pair<const CHAR*, size_t> body(getBodyData(part, &pBodyData));
		
		const CHAR* pBegin = findInline("-----BEGIN PGP SIGNED MESSAGE-----\r\n", body.first, body.second);
		assert(pBegin);
		size_t nLen = body.second - (pBegin - body.first);
		const CHAR* pEnd = findInline("-----END PGP SIGNATURE-----\r\n", pBegin, nLen);
		if (pEnd)
			nLen = pEnd - pBegin + 29;
		
		xstring_size_ptr strBody(pDriver->decryptAndVerify(pBegin,
			nLen, 0, pnVerify, pwstrSignedBy, pValidity, pwstrInfo));
		if (!strBody.get())
			return xstring_size_ptr();
		if (!checkUserId(part, pwstrSignedBy->get()))
			*pnVerify |= VERIFY_ADDRESSNOTMATCH;
		
		return createMessage(part.getHeader(), strBody.get(), strBody.size());
	}
}

xstring_size_ptr qmpgp::PGPUtilityImpl::decryptAndVerify(const Part& part,
														 bool bMime,
														 const WCHAR* pwszPassphrase,
														 unsigned int* pnVerify,
														 wstring_ptr* pwstrSignedBy,
														 Validity* pValidity,
														 qs::wstring_ptr* pwstrInfo) const
{
	assert(pnVerify);
	assert(pwstrSignedBy);
	assert(pValidity);
	assert(pwstrInfo);
	
	*pnVerify = VERIFY_NONE;
	pwstrSignedBy->reset(0);
	
	std::auto_ptr<Driver> pDriver(getDriver());
	
	if (bMime) {
		assert(getType(part, false) == TYPE_MIMEENCRYPTED);
		
		xstring_size_ptr strContent(pDriver->decryptAndVerify(part.getPart(1)->getBody(),
			-1, pwszPassphrase, pnVerify, pwstrSignedBy, pValidity, pwstrInfo));
		if (!strContent.get())
			return xstring_size_ptr();
		if (*pnVerify != VERIFY_NONE) {
			if (!checkUserId(part, pwstrSignedBy->get()))
				*pnVerify |= VERIFY_ADDRESSNOTMATCH;
		}
		return createMessage(strContent.get(), strContent.size(), part);
	}
	else {
		assert(getType(part, true) == TYPE_INLINEENCRYPTED);
		
		malloc_size_ptr<unsigned char> pBodyData;
		std::pair<const CHAR*, size_t> body(getBodyData(part, &pBodyData));
		
		XStringBuffer<XSTRING> buf;
		
		const CHAR* p = body.first;
		while (true) {
			const CHAR* pBegin = findInline("-----BEGIN PGP MESSAGE-----\r\n", p, body.second - (p - body.first));
			if (pBegin) {
				if (pBegin != p) {
					if (!buf.append(p, pBegin - p))
						return xstring_size_ptr();
				}
				size_t nLen = body.second - (pBegin - body.first);
				const CHAR* pEnd = findInline("-----END PGP MESSAGE-----\r\n", pBegin, nLen);
				if (pEnd)
					nLen = pEnd - pBegin + 27;
				
				xstring_size_ptr strBody(pDriver->decryptAndVerify(pBegin, nLen,
					pwszPassphrase, pnVerify, pwstrSignedBy, pValidity, pwstrInfo));
				if (!strBody.get())
					return xstring_size_ptr();
				if (*pnVerify != VERIFY_NONE) {
					if (!checkUserId(part, pwstrSignedBy->get()))
						*pnVerify |= VERIFY_ADDRESSNOTMATCH;
					return createMessage(part.getHeader(), strBody.get(), strBody.size());
				}
				else {
					if (!buf.append(strBody.get()))
						return xstring_size_ptr();
				}
				
				p = pBegin + nLen;
			}
			else {
				if (!buf.append(p, body.second - (p - body.first)))
					return xstring_size_ptr();
				break;
			}
		}
		return createMessage(part.getHeader(), buf.getCharArray(), buf.getLength());
	}
}

std::auto_ptr<Driver> qmpgp::PGPUtilityImpl::getDriver() const
{
	if (pProfile_->getInt(L"PGP", L"UseGPG", 1))
		return std::auto_ptr<Driver>(new GPGDriver(pProfile_));
	else
		return std::auto_ptr<Driver>(new PGPDriver(pProfile_));
}

bool qmpgp::PGPUtilityImpl::checkUserId(const qs::Part& part,
										const WCHAR* pwszUserId) const
{
	return checkUserId(part, pwszUserId, true);
}

bool qmpgp::PGPUtilityImpl::checkUserId(const qs::Part& part,
										const WCHAR* pwszUserId,
										bool bCheckAlternative) const
{
	if (!pwszUserId)
		return false;
	
	const WCHAR* pStart = wcsrchr(pwszUserId, L'<');
	if (!pStart)
		return false;
	const WCHAR* pEnd = wcsrchr(pwszUserId, L'>');
	if (!pEnd || pStart > pEnd)
		return false;
	
	wstring_ptr wstrAddress(allocWString(pStart + 1, pEnd - pStart - 1));
	
	AddressListParser from(AddressListParser::FLAG_DISALLOWGROUP);
	if (part.getField(L"From", &from) == Part::FIELD_EXIST &&
		contains(from, wstrAddress.get()))
		return true;
	
	AddressListParser sender(AddressListParser::FLAG_DISALLOWGROUP);
	if (part.getField(L"Sender", &sender) == Part::FIELD_EXIST &&
		contains(sender, wstrAddress.get()))
		return true;
	
	if (bCheckAlternative) {
		std::auto_ptr<Driver> pDriver(getDriver());
		
		Driver::UserIdList listUserId;
		StringListFree<Driver::UserIdList> free(listUserId);
		if (!pDriver->getAlternatives(pwszUserId, &listUserId))
			return false;
		
		for (Driver::UserIdList::const_iterator it = listUserId.begin(); it != listUserId.end(); ++it) {
			if (checkUserId(part, *it, false))
				return true;
		}
	}
	
	return false;
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
		AddressListParser addressList;
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

xstring_size_ptr qmpgp::PGPUtilityImpl::createMessage(const CHAR* pszHeader,
													  const CHAR* pszBody,
													  size_t nBodyLen)
{
	XStringBuffer<XSTRING> buf;
	if (!buf.append(pszHeader) || !buf.append("\r\n") || !buf.append(pszBody, nBodyLen))
		return xstring_size_ptr();
	return buf.getXStringSize();
}

xstring_size_ptr qmpgp::PGPUtilityImpl::createMessage(const CHAR* pszContent,
													  size_t nLen,
													  const Part& part)
{
	const CHAR* pBody = Part::getBody(pszContent, nLen);
	size_t nHeaderLen = pBody ? pBody - pszContent : nLen;
	size_t nBodyLen = pBody ? pszContent + nLen - pBody : 0;
	
	Part header;
	if (!header.create(0, pszContent, nHeaderLen))
		return xstring_size_ptr();
	
	PrefixFieldFilter filter("content-", true);
	if (!header.copyFields(part, &filter))
		return xstring_size_ptr();
	
	const CHAR* pszHeader = header.getHeader();
	size_t nNewHeaderLen = strlen(pszHeader);
	
	xstring_ptr strMessage(allocXString(nNewHeaderLen + nBodyLen + 5));
	if (!strMessage.get())
		return xstring_size_ptr();
	
	CHAR* p = strMessage.get();
	strncpy(p, pszHeader, nNewHeaderLen);
	p += nNewHeaderLen;
	strncpy(p, "\r\n", 2);
	p += 2;
	if (pBody)
		strncpy(p, pBody, nBodyLen);
	*(p + nBodyLen) = '\0';
	
	return xstring_size_ptr(strMessage.release(), nNewHeaderLen + 2 + nBodyLen);
}

xstring_size_ptr qmpgp::PGPUtilityImpl::createMultipartSignedMessage(const CHAR* pszHeader,
																	 const Part& part,
																	 const CHAR* pszSignature,
																	 size_t nSignatureLen)
{
	assert(pszHeader);
	assert(pszSignature);
	
	Part msg;
	{
		if (!msg.create(0, pszHeader, -1))
			return xstring_size_ptr();
		
		PrefixFieldFilter filter("content-");
		if (!msg.removeFields(&filter))
			return xstring_size_ptr();
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
			return xstring_size_ptr();
	}
	
	std::auto_ptr<Part> pPartContent(part.clone());
	if (!pPartContent.get())
		return xstring_size_ptr();
	msg.addPart(pPartContent);
	
	std::auto_ptr<Part> pPartSignature(new Part());
	{
		ContentTypeParser contentType(L"application", L"pgp-signature");
		contentType.setParameter(L"name", L"signature.asc");
		if (!pPartSignature->setField(L"Content-Type", contentType))
			return xstring_size_ptr();
		
		if (!pPartSignature->setBody(pszSignature, nSignatureLen))
			return xstring_size_ptr();
	}
	msg.addPart(pPartSignature);
	
	if (!msg.sortHeader())
		return xstring_size_ptr();
	
	return msg.getContent();
}

xstring_size_ptr qmpgp::PGPUtilityImpl::createMultipartEncryptedMessage(const CHAR* pszHeader,
																		const CHAR* pszBody,
																		size_t nBodyLen)
{
	assert(pszHeader);
	assert(pszBody);
	
	Part msg;
	{
		if (!msg.create(0, pszHeader, -1))
			return xstring_size_ptr();
		
		PrefixFieldFilter filter("content-");
		if (!msg.removeFields(&filter))
			return xstring_size_ptr();
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
			return xstring_size_ptr();
	}
	
	std::auto_ptr<Part> pPartHeader(new Part());
	{
		ContentTypeParser contentType(L"application", L"pgp-encrypted");
		if (!pPartHeader->setField(L"Content-Type", contentType))
			return xstring_size_ptr();
		
		if (!pPartHeader->setBody("Version: 1\r\n", -1))
			return xstring_size_ptr();
	}
	msg.addPart(pPartHeader);
	
	std::auto_ptr<Part> pPartBody(new Part());
	{
		ContentTypeParser contentType(L"application", L"octet-stream");
		if (!pPartBody->setField(L"Content-Type", contentType))
			return xstring_size_ptr();
		
		if (!pPartBody->setBody(pszBody, nBodyLen))
			return xstring_size_ptr();
	}
	msg.addPart(pPartBody);
	
	if (!msg.sortHeader())
		return xstring_size_ptr();
	
	return msg.getContent();
}

std::pair<const CHAR*, size_t> qmpgp::PGPUtilityImpl::getBodyData(const Part& part,
																  malloc_size_ptr<unsigned char>* ppBodyData)
{
	const CHAR* pBody = 0;
	size_t nLen = 0;
	if (part.getEncoder().get()) {
		*ppBodyData = part.getBodyData();
		pBody = reinterpret_cast<const CHAR*>(ppBodyData->get());
		nLen = ppBodyData->size();
	}
	else {
		pBody = part.getBody();
		if (pBody)
			nLen = strlen(pBody);
	}
	
	return std::make_pair(pBody, nLen);
}

const CHAR* qmpgp::PGPUtilityImpl::findInline(const CHAR* pszMarker,
											  const CHAR* psz,
											  size_t nLen)
{
	assert(pszMarker);
	assert(strncmp(pszMarker + (strlen(pszMarker) - 2), "\r\n", 2) == 0);
	assert(psz);
	
	BMFindString<STRING> bmfs(pszMarker);
	const CHAR* p = psz;
	while (true) {
		p = bmfs.find(p, nLen - (p - psz));
		if (!p)
			return 0;
		else if (p == psz || (p > psz + 1 && strncmp(p - 2, "\r\n", 2) == 0))
			return p;
		else
			++p;
	}
	
	return 0;
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
