/*
 * $Id$
 *
 * Copyright(C) 1998-2003 Satoshi Nakamura
 * All rights reserved.
 *
 */

#include <qsencoder.h>
#include <qsmime.h>
#include <qsnew.h>
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

qscrypto::SMIMEUtilityImpl::SMIMEUtilityImpl(QSTATUS* pstatus)
{
}

qscrypto::SMIMEUtilityImpl::~SMIMEUtilityImpl()
{
}

SMIMEUtilityImpl::Type qscrypto::SMIMEUtilityImpl::getType(
	const Part& part) const
{
	DECLARE_QSTATUS();
	
	const ContentTypeParser* pContentType = part.getContentType();
	const WCHAR* pwszMediaType = pContentType->getMediaType();
	const WCHAR* pwszSubType = pContentType->getSubType();
	if (wcscmp(pwszMediaType, L"multipart") == 0 &&
		wcscmp(pwszSubType, L"signed") == 0) {
		string_ptr<WSTRING> wstrProtocol;
		status = pContentType->getParameter(L"protocol", &wstrProtocol);
		CHECK_QSTATUS_VALUE(TYPE_NONE);
		if (!wstrProtocol.get())
			return TYPE_NONE;
		if (wcscmp(wstrProtocol.get(), L"pkcs7-signature") == 0 ||
			wcscmp(wstrProtocol.get(), L"x-pkcs7-signature") == 0) {
			if (part.getPartCount() == 2) {
				Part* pChild = part.getPart(1);
				const ContentTypeParser* pChildType = pChild->getContentType();
				if (wcscmp(pChildType->getMediaType(), L"application") == 0 &&
					(wcscmp(pChildType->getSubType(), L"pkcs7-signature") == 0 ||
					wcscmp(pChildType->getSubType(), L"x-pkcs7-signature") == 0))
					return TYPE_MULTIPARTSIGNED;
			}
		}
	}
	else if (wcscmp(pwszMediaType, L"application") == 0 &&
		(wcscmp(pwszSubType, L"pkcs7-mime") == 0 ||
		wcscmp(pwszSubType, L"x-pkcs7-mime") == 0)) {
		string_ptr<WSTRING> wstrType;
		status = pContentType->getParameter(L"smime-type", &wstrType);
		CHECK_QSTATUS_VALUE(TYPE_NONE);
		if (!wstrType.get())
			return TYPE_NONE;
		else if (wcscmp(wstrType.get(), L"signed-data") == 0)
			return TYPE_SIGNED;
		else if (wcscmp(wstrType.get(), L"enveloped-data") == 0)
			return TYPE_ENVELOPED;
	}
	
	return TYPE_NONE;
}

QSTATUS qscrypto::SMIMEUtilityImpl::sign(Part* pPart, bool bMultipart,
	const PrivateKey* pPrivateKey, const Certificate* pCertificate,
	SMIMECallback* pCallback, STRING* pstrMessage) const
{
	assert(pPart);
	assert(pPrivateKey);
	assert(pCertificate);
	assert(pstrMessage);
	
	DECLARE_QSTATUS();
	
	X509* pX509 = static_cast<const CertificateImpl*>(pCertificate)->getX509();
	EVP_PKEY* pKey = static_cast<const PrivateKeyImpl*>(pPrivateKey)->getKey();
	
	if (bMultipart) {
		// TODO
	}
	else {
		string_ptr<STRING> strHeader(allocString(pPart->getHeader()));
		if (!strHeader.get())
			return QSTATUS_OUTOFMEMORY;
		
		const WCHAR* pwszFields[] = {
			L"Bcc",
			L"Resent-Bcc",
		};
		for (int n = 0; n < countof(pwszFields); ++n) {
			status = pPart->removeField(pwszFields[n]);
			CHECK_QSTATUS();
		}
		
		string_ptr<STRING> strContent;
		status = pCallback->getContent(pPart, &strContent);
		CHECK_QSTATUS();
		
		BIOPtr pIn(BIO_new_mem_buf(strContent.get(), strlen(strContent.get())));
		PKCS7Ptr pPKCS7(PKCS7_sign(pX509, pKey, 0, pIn.get(), 0));
		if (!pPKCS7.get())
			return QSTATUS_FAIL;
		
		status = createMessage(strHeader.get(), pPKCS7.get(), false, pstrMessage);
		CHECK_QSTATUS();
	}
	
	return QSTATUS_SUCCESS;
}

QSTATUS qscrypto::SMIMEUtilityImpl::verify(const Part& part,
	const Store* pStoreCA, STRING* pstrMessage) const
{
	assert(pStoreCA);
	assert(pstrMessage);
	
	DECLARE_QSTATUS();
	
	X509_STORE* pStore = static_cast<const StoreImpl*>(pStoreCA)->getStore();
	
	BIOPtr pOut(BIO_new(BIO_s_mem()));
	
	assert(getType(part) == TYPE_SIGNED || getType(part) == TYPE_MULTIPARTSIGNED);
	
	const ContentTypeParser* pContentType = part.getContentType();
	if (wcscmp(pContentType->getMediaType(), L"multipart") == 0) {
	}
	else {
		unsigned char* p = 0;
		size_t nDataLen = 0;
		status = part.getBodyData(&p, &nDataLen);
		CHECK_QSTATUS();
		malloc_ptr<unsigned char> pData(p);
		
		BIOPtr pIn(BIO_new_mem_buf(pData.get(), nDataLen));
		PKCS7Ptr pPKCS7(d2i_PKCS7_bio(pIn.get(), 0));
		if (!pPKCS7.get())
			return QSTATUS_FAIL;
		
		if (PKCS7_verify(pPKCS7.get(), 0, pStore, 0, pOut.get(), 0) != 1)
			return QSTATUS_FAIL;
	}
	
	char* pBuf = 0;
	int nBufLen = BIO_get_mem_data(pOut.get(), &pBuf);
	
	string_ptr<STRING> strMessage(allocString(pBuf, nBufLen));
	if (!strMessage.get())
		return QSTATUS_OUTOFMEMORY;
	*pstrMessage = strMessage.release();
	
	return QSTATUS_SUCCESS;
}

QSTATUS qscrypto::SMIMEUtilityImpl::encrypt(Part* pPart,
	const Cipher* pCipher, SMIMECallback* pCallback, STRING* pstrMessage) const
{
	assert(pPart);
	assert(pCipher);
	assert(pCallback);
	assert(pstrMessage);
	
	DECLARE_QSTATUS();
	
	X509StackPtr pCertificates(sk_X509_new_null());
	
	const WCHAR* pwszAddresses[] = {
		L"To",
		L"Cc",
		L"Bcc"
	};
	for (int n = 0; n < countof(pwszAddresses); ++n) {
		AddressListParser addressList(0, &status);
		CHECK_QSTATUS();
		Part::Field f;
		status = pPart->getField(pwszAddresses[n], &addressList, &f);
		CHECK_QSTATUS();
		if (f == Part::FIELD_EXIST) {
			string_ptr<WSTRING> wstrAddresses;
			status = addressList.getAddresses(&wstrAddresses);
			CHECK_QSTATUS();
			
			const WCHAR* p = wcstok(wstrAddresses.get(), L",");
			while (p) {
				Certificate* pCertificate = 0;
				status = pCallback->getCertificate(p, &pCertificate);
				CHECK_QSTATUS();
				if (!pCertificate)
					return QSTATUS_FAIL;
				sk_X509_push(pCertificates.get(),
					static_cast<CertificateImpl*>(pCertificate)->releaseX509());
				delete pCertificate;
				
				p = wcstok(0, L",");
			}
		}
	}
	
	string_ptr<STRING> strHeader(allocString(pPart->getHeader()));
	if (!strHeader.get())
		return QSTATUS_OUTOFMEMORY;
	
	const WCHAR* pwszFields[] = {
		L"Bcc",
		L"Resent-Bcc",
	};
	for (int m = 0; m < countof(pwszFields); ++m) {
		status = pPart->removeField(pwszFields[m]);
		CHECK_QSTATUS();
	}
	
	string_ptr<STRING> strContent;
	status = pCallback->getContent(pPart, &strContent);
	CHECK_QSTATUS();
	
	BIOPtr pIn(BIO_new_mem_buf(strContent.get(), strlen(strContent.get())));
	PKCS7Ptr pPKCS7(PKCS7_encrypt(pCertificates.get(), pIn.get(),
		static_cast<const CipherImpl*>(pCipher)->getCipher(), 0));
	if (!pPKCS7.get())
		return QSTATUS_FAIL;
	
	status = createMessage(strHeader.get(), pPKCS7.get(), true, pstrMessage);
	CHECK_QSTATUS();
	
	return QSTATUS_SUCCESS;
}

QSTATUS qscrypto::SMIMEUtilityImpl::decrypt(const Part& part,
	const PrivateKey* pPrivateKey, const Certificate* pCertificate,
	STRING* pstrMessage) const
{
	assert(pPrivateKey);
	assert(pCertificate);
	assert(pstrMessage);
	
	DECLARE_QSTATUS();
	
	assert(getType(part) == TYPE_ENVELOPED);
	
	unsigned char* p = 0;
	size_t nDataLen = 0;
	status = part.getBodyData(&p, &nDataLen);
	CHECK_QSTATUS();
	malloc_ptr<unsigned char> pData(p);
	
	BIOPtr pIn(BIO_new_mem_buf(pData.get(), nDataLen));
	PKCS7Ptr pPKCS7(d2i_PKCS7_bio(pIn.get(), 0));
	if (!pPKCS7.get())
		return QSTATUS_FAIL;
	
	EVP_PKEY* pKey = static_cast<const PrivateKeyImpl*>(pPrivateKey)->getKey();
	X509* pX509 = static_cast<const CertificateImpl*>(pCertificate)->getX509();
	
	BIOPtr pOut(BIO_new(BIO_s_mem()));
	if (PKCS7_decrypt(pPKCS7.get(), pKey, pX509, pOut.get(), 0) != 1)
		return QSTATUS_FAIL;
	
	char* pBuf = 0;
	int nBufLen = BIO_get_mem_data(pOut.get(), &pBuf);
	
	string_ptr<STRING> strMessage(allocString(pBuf, nBufLen));
	if (!strMessage.get())
		return QSTATUS_OUTOFMEMORY;
	*pstrMessage = strMessage.release();
	
	
	return QSTATUS_SUCCESS;
}

QSTATUS qscrypto::SMIMEUtilityImpl::createMessage(const CHAR* pszHeader,
	PKCS7* pPKCS7, bool bEnveloped, STRING* pstrMessage)
{
	assert(pPKCS7);
	assert(pstrMessage);
	
	DECLARE_QSTATUS();
	
	BIOPtr pOut(BIO_new(BIO_s_mem()));
	if (!pOut.get())
		return QSTATUS_FAIL;
	if (i2d_PKCS7_bio(pOut.get(), pPKCS7) != 1)
		return QSTATUS_FAIL;
	
	unsigned char* pBuf = 0;
	int nBufLen = BIO_get_mem_data(pOut.get(), &pBuf);
	Base64Encoder encoder(true, &status);
	CHECK_QSTATUS();
	unsigned char* p = 0;
	size_t nEncodedLen = 0;
	status = encoder.encode(pBuf, nBufLen, &p, &nEncodedLen);
	CHECK_QSTATUS();
	malloc_ptr<CHAR> pEncoded(reinterpret_cast<CHAR*>(p));
	
	Part part(0, pszHeader, -1, &status);
	CHECK_QSTATUS();
	const WCHAR* pwszFields[] = {
		L"Content-Type",
		L"Content-Transfer-Encoding",
		L"Content-Disposition",
		L"Content-ID",
		L"Content-Description"
	};
	for (int n = 0; n < countof(pwszFields); ++n) {
		status = part.removeField(pwszFields[n]);
		CHECK_QSTATUS();
	}
	
	ContentTypeParser contentType(L"application", L"pkcs7-mime", &status);
	CHECK_QSTATUS();
	status = contentType.setParameter(L"name", L"smime.p7m");
	CHECK_QSTATUS();
	const WCHAR* pwszType = bEnveloped ? L"enveloped-data" : L"signed-data";
	status = contentType.setParameter(L"smime-type", pwszType);
	CHECK_QSTATUS();
	status = part.setField(L"Content-Type", contentType);
	CHECK_QSTATUS();
	
	ContentDispositionParser contentDisposition(L"attachment", &status);
	CHECK_QSTATUS();
	status = contentDisposition.setParameter(L"filename", L"smime.p7m");
	CHECK_QSTATUS();
	status = part.setField(L"Content-Disposition", contentDisposition);
	CHECK_QSTATUS();
	
	SimpleParser contentTransferEncoding(L"base64",
		SimpleParser::FLAG_RECOGNIZECOMMENT | SimpleParser::FLAG_TSPECIAL, &status);
	CHECK_QSTATUS();
	status = part.setField(L"Content-Transfer-Encoding",
		contentTransferEncoding);
	CHECK_QSTATUS();
	
	status = part.sortHeader();
	CHECK_QSTATUS();
	
	status = part.setBody(pEncoded.get(), nEncodedLen);
	CHECK_QSTATUS();
	
	status = part.getContent(pstrMessage);
	CHECK_QSTATUS();
	
	return QSTATUS_SUCCESS;
}
