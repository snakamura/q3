/*
 * $Id$
 *
 * Copyright(C) 1998-2004 Satoshi Nakamura
 * All rights reserved.
 *
 */

#include <qsencoder.h>
#include <qsmime.h>
#include <qsstl.h>

#include <openssl/pem.h>

#include "crypto.h"
#include "smime.h"
#include "util.h"

using namespace qscrypto;
using namespace qs;


/****************************************************************************
 *
 * SMIMEUtilityImpl
 *
 */

qscrypto::SMIMEUtilityImpl::SMIMEUtilityImpl()
{
}

qscrypto::SMIMEUtilityImpl::~SMIMEUtilityImpl()
{
}

SMIMEUtility::Type qscrypto::SMIMEUtilityImpl::getType(const Part& part) const
{
	return getType(part.getContentType());
}

SMIMEUtility::Type qscrypto::SMIMEUtilityImpl::getType(const ContentTypeParser* pContentType) const
{
	if (!pContentType)
		return TYPE_NONE;
	
	const WCHAR* pwszMediaType = pContentType->getMediaType();
	const WCHAR* pwszSubType = pContentType->getSubType();
	if (_wcsicmp(pwszMediaType, L"multipart") == 0 &&
		_wcsicmp(pwszSubType, L"signed") == 0) {
		wstring_ptr wstrProtocol(pContentType->getParameter(L"protocol"));
		if (!wstrProtocol.get())
			return TYPE_NONE;
		if (_wcsicmp(wstrProtocol.get(), L"pkcs7-signature") == 0 ||
			_wcsicmp(wstrProtocol.get(), L"x-pkcs7-signature") == 0) {
//			if (part.getPartCount() == 2) {
//				Part* pChild = part.getPart(1);
//				const ContentTypeParser* pChildType = pChild->getContentType();
//				if (_wcsicmp(pChildType->getMediaType(), L"application") == 0 &&
//					(_wcsicmp(pChildType->getSubType(), L"pkcs7-signature") == 0 ||
//					_wcsicmp(pChildType->getSubType(), L"x-pkcs7-signature") == 0))
					return TYPE_MULTIPARTSIGNED;
//			}
		}
	}
	else if (_wcsicmp(pwszMediaType, L"application") == 0 &&
		(_wcsicmp(pwszSubType, L"pkcs7-mime") == 0 ||
		_wcsicmp(pwszSubType, L"x-pkcs7-mime") == 0)) {
		wstring_ptr wstrType(pContentType->getParameter(L"smime-type"));
		if (!wstrType.get())
			;
		else if (_wcsicmp(wstrType.get(), L"signed-data") == 0)
			return TYPE_SIGNED;
		else if (_wcsicmp(wstrType.get(), L"enveloped-data") == 0)
			return TYPE_ENVELOPED;
		
		wstring_ptr wstrName(pContentType->getParameter(L"name"));
		if (!wstrName.get())
			return TYPE_NONE;
//		else if (_wcsicmp(wstrName.get(), L"smime.p7s") == 0)
//			return TYPE_SIGNED;
		else if (_wcsicmp(wstrName.get(), L"smime.p7m") == 0)
			return TYPE_ENVELOPED;
	}
	
	return TYPE_NONE;
}

xstring_ptr qscrypto::SMIMEUtilityImpl::sign(Part* pPart,
											 bool bMultipart,
											 const PrivateKey* pPrivateKey,
											 const Certificate* pCertificate,
											 SMIMECallback* pCallback) const
{
	assert(pPart);
	assert(pPrivateKey);
	assert(pCertificate);
	
	X509* pX509 = static_cast<const CertificateImpl*>(pCertificate)->getX509();
	EVP_PKEY* pKey = static_cast<const PrivateKeyImpl*>(pPrivateKey)->getKey();
	
	if (bMultipart) {
		// TODO
		return 0;
	}
	else {
		xstring_ptr strHeader(allocXString(pPart->getHeader()));
		if (!strHeader.get())
			return 0;
		
		PrefixFieldFilter filter("content-", true);
		if (!pPart->removeFields(&filter))
			return 0;
		
		xstring_ptr strContent(pPart->getContent());
		if (!strContent.get())
			return 0;
		
		BIOPtr pIn(BIO_new_mem_buf(strContent.get(), strlen(strContent.get())));
		PKCS7Ptr pPKCS7(PKCS7_sign(pX509, pKey, 0, pIn.get(), 0));
		if (!pPKCS7.get())
			return 0;
		
		return createMessage(strHeader.get(), pPKCS7.get(), false);
	}
}

xstring_ptr qscrypto::SMIMEUtilityImpl::verify(const Part& part,
											   const Store* pStoreCA) const
{
	assert(pStoreCA);
	
	X509_STORE* pStore = static_cast<const StoreImpl*>(pStoreCA)->getStore();
	
	BIOPtr pOut(BIO_new(BIO_s_mem()));
	
	assert(getType(part) == TYPE_SIGNED || getType(part) == TYPE_MULTIPARTSIGNED);
	
	const ContentTypeParser* pContentType = part.getContentType();
	if (_wcsicmp(pContentType->getMediaType(), L"multipart") == 0) {
		// TODO
	}
	else {
		malloc_size_ptr<unsigned char> buf(part.getBodyData());
		if (!buf.get())
			return 0;
		
		BIOPtr pIn(BIO_new_mem_buf(buf.get(), buf.size()));
		PKCS7Ptr pPKCS7(d2i_PKCS7_bio(pIn.get(), 0));
		if (!pPKCS7.get())
			return 0;
		
		if (PKCS7_verify(pPKCS7.get(), 0, pStore, 0, pOut.get(), 0) != 1)
			return 0;
	}
	
	char* pBuf = 0;
	int nBufLen = BIO_get_mem_data(pOut.get(), &pBuf);
	return createMessage(pBuf, nBufLen, part);
}

xstring_ptr qscrypto::SMIMEUtilityImpl::encrypt(Part* pPart,
												const Cipher* pCipher,
												SMIMECallback* pCallback) const
{
	assert(pPart);
	assert(pCipher);
	assert(pCallback);
	
	X509StackPtr pCertificates(sk_X509_new_null());
	
	const WCHAR* pwszAddresses[] = {
		L"To",
		L"Cc",
		L"Bcc"
	};
	for (int n = 0; n < countof(pwszAddresses); ++n) {
		AddressListParser addressList(0);
		Part::Field f = pPart->getField(pwszAddresses[n], &addressList);
		if (f == Part::FIELD_EXIST) {
			wstring_ptr wstrAddresses(addressList.getAddresses());
			
			typedef std::vector<const WCHAR*> AddressList;
			AddressList l;
			const WCHAR* p = wcstok(wstrAddresses.get(), L",");
			while (p) {
				l.push_back(p);
				p = wcstok(0, L",");
			}
			for (AddressList::const_iterator it = l.begin(); it != l.end(); ++it) {
				const WCHAR* pwszAddress = *it;
				std::auto_ptr<Certificate> pCertificate(pCallback->getCertificate(pwszAddress));
				if (!pCertificate.get())
					return 0;
				sk_X509_push(pCertificates.get(),
					static_cast<CertificateImpl*>(pCertificate.get())->releaseX509());
			}
		}
	}
	
	const Certificate* pCertificate = pCallback->getSelfCertificate();
	if (pCertificate) {
		X509* pX509 = X509_dup(static_cast<const CertificateImpl*>(pCertificate)->getX509());
		if (!pX509)
			return 0;
		sk_X509_push(pCertificates.get(), pX509);
	}
	
	xstring_ptr strHeader(allocXString(pPart->getHeader()));
	if (!strHeader.get())
		return 0;
	
	PrefixFieldFilter filter("content-", true);
	if (!pPart->removeFields(&filter))
		return 0;
	
	xstring_ptr strContent(pPart->getContent());
	if (!strContent.get())
		return 0;
	
	BIOPtr pIn(BIO_new_mem_buf(strContent.get(), strlen(strContent.get())));
	PKCS7Ptr pPKCS7(PKCS7_encrypt(pCertificates.get(), pIn.get(),
		static_cast<const CipherImpl*>(pCipher)->getCipher(), 0));
	if (!pPKCS7.get())
		return 0;
	
	return createMessage(strHeader.get(), pPKCS7.get(), true);
}

xstring_ptr qscrypto::SMIMEUtilityImpl::decrypt(const Part& part,
												const PrivateKey* pPrivateKey,
												const Certificate* pCertificate) const
{
	assert(pPrivateKey);
	assert(pCertificate);
	
	assert(getType(part) == TYPE_ENVELOPED);
	
	malloc_size_ptr<unsigned char> buf(part.getBodyData());
	if (!buf.get())
		return 0;
	
	BIOPtr pIn(BIO_new_mem_buf(buf.get(), buf.size()));
	PKCS7Ptr pPKCS7(d2i_PKCS7_bio(pIn.get(), 0));
	if (!pPKCS7.get())
		return 0;
	
	EVP_PKEY* pKey = static_cast<const PrivateKeyImpl*>(pPrivateKey)->getKey();
	X509* pX509 = static_cast<const CertificateImpl*>(pCertificate)->getX509();
	
	BIOPtr pOut(BIO_new(BIO_s_mem()));
	if (PKCS7_decrypt(pPKCS7.get(), pKey, pX509, pOut.get(), 0) != 1)
		return 0;
	
	char* pBuf = 0;
	int nBufLen = BIO_get_mem_data(pOut.get(), &pBuf);
	return createMessage(pBuf, nBufLen, part);
}

xstring_ptr qscrypto::SMIMEUtilityImpl::createMessage(const CHAR* pszHeader,
													  PKCS7* pPKCS7,
													  bool bEnveloped)
{
	assert(pPKCS7);
	
	BIOPtr pOut(BIO_new(BIO_s_mem()));
	if (!pOut.get())
		return 0;
	if (i2d_PKCS7_bio(pOut.get(), pPKCS7) != 1)
		return 0;
	
	unsigned char* pBuf = 0;
	int nBufLen = BIO_get_mem_data(pOut.get(), &pBuf);
	Base64Encoder encoder(true);
	malloc_size_ptr<unsigned char> buf(encoder.encode(pBuf, nBufLen));
	if (!buf.get())
		return 0;
	
	Part part;
	if (!part.create(0, pszHeader, -1))
		return 0;
	
	PrefixFieldFilter filter("content-");
	if (!part.removeFields(&filter))
		return 0;
	
	ContentTypeParser contentType(L"application", L"pkcs7-mime");
	contentType.setParameter(L"name", L"smime.p7m");
	const WCHAR* pwszType = bEnveloped ? L"enveloped-data" : L"signed-data";
	contentType.setParameter(L"smime-type", pwszType);
	if (!part.setField(L"Content-Type", contentType))
		return false;
	
	ContentDispositionParser contentDisposition(L"attachment");
	contentDisposition.setParameter(L"filename", L"smime.p7m");
	if (!part.setField(L"Content-Disposition", contentDisposition))
		return false;
	
	ContentTransferEncodingParser contentTransferEncoding(L"base64");
	if (!part.setField(L"Content-Transfer-Encoding", contentTransferEncoding))
		return 0;
	
	if (!part.sortHeader())
		return 0;
	
	if (!part.setBody(reinterpret_cast<CHAR*>(buf.get()), buf.size()))
		return false;
	
	return part.getContent();
}

xstring_ptr qscrypto::SMIMEUtilityImpl::createMessage(const CHAR* pszContent,
													  size_t nLen,
													  const Part& part)
{
	BMFindString<STRING> bmfs("\r\n\r\n");
	const CHAR* pBody = bmfs.find(pszContent, nLen);
	if (pBody)
		pBody += 4;
	size_t nHeaderLen = pBody ? pBody - pszContent : nLen;
	size_t nBodyLen = pBody ? pszContent + nLen - pBody : 0;
	
	Part headerPart;
	if (!headerPart.create(0, pszContent, nHeaderLen))
		return 0;
	
	PrefixFieldFilter filter("content-", true);
	if (!headerPart.copyFields(part, &filter))
		return 0;
	const CHAR* pszHeader = headerPart.getHeader();
	size_t nNewHeaderLen = strlen(pszHeader);
	
	xstring_ptr strMessage(allocXString(nNewHeaderLen + nBodyLen + 5));
	if (!strMessage.get())
		return 0;
	
	CHAR* p = strMessage.get();
	strncpy(p, pszHeader, nNewHeaderLen);
	p += nNewHeaderLen;
	strncpy(p, "\r\n\r\n", 4);
	p += 4;
	if (pBody)
		strncpy(p, pBody, nBodyLen);
	*(p + nBodyLen) = '\0';
	
	return strMessage;
}
