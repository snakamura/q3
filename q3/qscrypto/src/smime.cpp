/*
 * $Id$
 *
 * Copyright(C) 1998-2008 Satoshi Nakamura
 *
 */

#include <qsencoder.h>
#include <qsinit.h>
#include <qslog.h>
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
	Type type = getType(part.getContentType());
	if (type == TYPE_MULTIPARTSIGNED) {
		type = TYPE_NONE;
		
		if (part.getPartCount() == 2) {
			Part* pChild = part.getPart(1);
			const ContentTypeParser* pChildType = pChild->getContentType();
			if (pChildType &&
				_wcsicmp(pChildType->getMediaType(), L"application") == 0 &&
				(_wcsicmp(pChildType->getSubType(), L"pkcs7-signature") == 0 ||
				_wcsicmp(pChildType->getSubType(), L"x-pkcs7-signature") == 0))
				type = TYPE_MULTIPARTSIGNED;
		}
	}
	else if (type == TYPE_ENVELOPEDORSIGNED) {
		type = TYPE_NONE;
		
		malloc_size_ptr<unsigned char> buf(part.getBodyData());
		if (buf.get()) {
			BIOPtr pIn(BIO_new_mem_buf(buf.get(), static_cast<int>(buf.size())));
			PKCS7Ptr pPKCS7(d2i_PKCS7_bio(pIn.get(), 0));
			if (pPKCS7.get()) {
				if (PKCS7_type_is_signed(pPKCS7.get()))
					type = TYPE_SIGNED;
				else if (PKCS7_type_is_enveloped(pPKCS7.get()))
					type = TYPE_ENVELOPED;
			}
		}
	}
	return type;
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
		if (_wcsicmp(wstrProtocol.get(), L"application/pkcs7-signature") == 0 ||
			_wcsicmp(wstrProtocol.get(), L"application/x-pkcs7-signature") == 0)
			return TYPE_MULTIPARTSIGNED;
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
		else if (_wcsicmp(wstrName.get(), L"smime.p7m") == 0)
			return TYPE_ENVELOPEDORSIGNED;
	}
	
	return TYPE_NONE;
}

xstring_size_ptr qscrypto::SMIMEUtilityImpl::sign(Part* pPart,
												  bool bMultipart,
												  const PrivateKey* pPrivateKey,
												  const Certificate* pCertificate) const
{
	assert(pPart);
	assert(pPrivateKey);
	assert(pCertificate);
	
	Log log(InitThread::getInitThread().getLogger(), L"qscrypto::SMIMEUtilityImpl");
	
	X509* pX509 = static_cast<const CertificateImpl*>(pCertificate)->getX509();
	EVP_PKEY* pKey = static_cast<const PrivateKeyImpl*>(pPrivateKey)->getKey();
	
	xstring_ptr strHeader(allocXString(pPart->getHeader()));
	if (!strHeader.get())
		return xstring_size_ptr();
	
	PrefixFieldFilter filter("content-", true);
	if (!pPart->removeFields(&filter))
		return xstring_size_ptr();
	
	xstring_size_ptr strContent(pPart->getContent());
	if (!strContent.get())
		return xstring_size_ptr();
	
	log.debug(L"Signed content.", reinterpret_cast<const unsigned char*>(strContent.get()), strContent.size());
	
	BIOPtr pIn(BIO_new_mem_buf(strContent.get(), static_cast<int>(strContent.size())));
	PKCS7Ptr pPKCS7(PKCS7_sign(pX509, pKey, 0, pIn.get(), bMultipart ? PKCS7_DETACHED : 0));
	if (!pPKCS7.get()) {
		Util::logError(log, L"Failed to create signed PKCS#7.");
		return xstring_size_ptr();
	}
	
	if (bMultipart)
		return createMultipartMessage(strHeader.get(), *pPart, pPKCS7.get());
	else
		return createMessage(strHeader.get(), pPKCS7.get(), false);
}

xstring_size_ptr qscrypto::SMIMEUtilityImpl::verify(const Part& part,
													const Store* pStoreCA,
													unsigned int* pnVerify,
													CertificateList* pListCertificate) const
{
	assert(pStoreCA);
	assert(pnVerify);
	assert(pListCertificate);
	assert(getType(part) == TYPE_SIGNED || getType(part) == TYPE_MULTIPARTSIGNED);
	assert(part.getContentType());
	assert(pListCertificate->empty());
	
	Log log(InitThread::getInitThread().getLogger(), L"qscrypto::SMIMEUtilityImpl");
	
	*pnVerify = VERIFY_OK;
	
	bool bMultipart = _wcsicmp(part.getContentType()->getMediaType(), L"multipart") == 0;
	
	malloc_size_ptr<unsigned char> pBody;
	if (bMultipart) {
		Part* pChild = part.getPart(1);
		assert(pChild);
		pBody = pChild->getBodyData();
	}
	else {
		pBody = part.getBodyData();
	}
	if (!pBody.get())
		return xstring_size_ptr();
	
	BIOPtr pIn(BIO_new_mem_buf(pBody.get(), static_cast<int>(pBody.size())));
	PKCS7Ptr pPKCS7(d2i_PKCS7_bio(pIn.get(), 0));
	if (!pPKCS7.get()) {
		Util::logError(log, L"Failed to load PKCS#7.");
		return xstring_size_ptr();
	}
	
	X509_STORE* pStore = static_cast<const StoreImpl*>(pStoreCA)->getStore();
	BIOPtr pOut(BIO_new(BIO_s_mem()));
	
	xstring_size_ptr strContent;
	size_t nLen = 0;
	if (bMultipart) {
		strContent = part.getPart(0)->getContent();
		nLen = strContent.size();
	}
	
	log.debug(L"Verified content.", reinterpret_cast<const unsigned char*>(strContent.get()), strContent.size());
	
	BIOPtr pContent(bMultipart ? BIO_new_mem_buf(strContent.get(), static_cast<int>(nLen)) : 0);
	
	if (PKCS7_verify(pPKCS7.get(), 0, pStore, pContent.get(), pOut.get(), 0) != 1) {
		Util::logError(log, L"Failed to verify PKCS#7.");
		
		*pnVerify |= VERIFY_FAILED;
		BIO_ctrl(pOut.get(), BIO_CTRL_RESET, 0, 0);
		if (bMultipart) {
			if (BIO_write(pOut.get(), strContent.get(), static_cast<int>(nLen)) != static_cast<int>(nLen))
				return xstring_size_ptr();
		}
		else {
			if (PKCS7_verify(pPKCS7.get(), 0, pStore, 0, pOut.get(), PKCS7_NOVERIFY | PKCS7_NOSIGS) != 1) {
				Util::logError(log, L"Failed to get content from PKCS#7.");
				return xstring_size_ptr();
			}
		}
	}
	
	X509StackPtr certs(PKCS7_get0_signers(pPKCS7.get(), 0, 0), false);
	if (!certs.get()) {
		Util::logError(log, L"Failed to get signers.");
		return xstring_size_ptr();
	}
	
	AddressListParser from(AddressListParser::FLAG_DISALLOWGROUP);
	const AddressListParser* pFrom = part.getField(L"From", &from) == Part::FIELD_EXIST ? &from : 0;
	AddressListParser sender(AddressListParser::FLAG_DISALLOWGROUP);
	const AddressListParser* pSender = part.getField(L"Sender", &sender) == Part::FIELD_EXIST ? &sender : 0;
	
	bool bMatch = false;
	for (int n = 0; n < sk_X509_num(certs.get()); ++n) {
		std::auto_ptr<Certificate> pCert(new CertificateImpl(sk_X509_value(certs.get(), n), true));
		if (match(pCert.get(), pFrom, pSender)) {
			bMatch = true;
			pListCertificate->insert(pListCertificate->begin(), pCert.get());
		}
		else {
			pListCertificate->push_back(pCert.get());
		}
		pCert.release();
	}
	if (!bMatch)
		*pnVerify |= VERIFY_ADDRESSNOTMATCH;
	
	char* pBuf = 0;
	int nBufLen = BIO_get_mem_data(pOut.get(), &pBuf);
	if (!pBuf)
		return xstring_size_ptr();
	log.debug(L"Verified data.", reinterpret_cast<const unsigned char*>(pBuf), nBufLen);
	
	return createMessage(pBuf, nBufLen, part);
}

xstring_size_ptr qscrypto::SMIMEUtilityImpl::encrypt(Part* pPart,
													 const Cipher* pCipher,
													 SMIMECallback* pCallback) const
{
	assert(pPart);
	assert(pCipher);
	assert(pCallback);
	
	Log log(InitThread::getInitThread().getLogger(), L"qscrypto::SMIMEUtilityImpl");
	
	X509StackPtr pCertificates(sk_X509_new_null(), true);
	
	const WCHAR* pwszAddresses[] = {
		L"To",
		L"Cc",
		L"Bcc"
	};
	for (int n = 0; n < countof(pwszAddresses); ++n) {
		AddressListParser addressList;
		if (pPart->getField(pwszAddresses[n], &addressList) == Part::FIELD_EXIST) {
			if (!getCertificates(addressList, pCallback, pCertificates.get()))
				return xstring_size_ptr();
		}
	}
	
	const Certificate* pCertificate = pCallback->getSelfCertificate();
	if (pCertificate) {
		X509* pX509 = X509_dup(static_cast<const CertificateImpl*>(pCertificate)->getX509());
		if (!pX509)
			return xstring_size_ptr();
		sk_X509_push(pCertificates.get(), pX509);
	}
	
	xstring_ptr strHeader(allocXString(pPart->getHeader()));
	if (!strHeader.get())
		return xstring_size_ptr();
	
	PrefixFieldFilter filter("content-", true);
	if (!pPart->removeFields(&filter))
		return xstring_size_ptr();
	
	xstring_size_ptr strContent(pPart->getContent());
	if (!strContent.get())
		return xstring_size_ptr();
	
	BIOPtr pIn(BIO_new_mem_buf(strContent.get(), static_cast<int>(strContent.size())));
	PKCS7Ptr pPKCS7(PKCS7_encrypt(pCertificates.get(), pIn.get(),
		static_cast<const CipherImpl*>(pCipher)->getCipher(), 0));
	if (!pPKCS7.get()) {
		Util::logError(log, L"Failed to create encrypted PKCS#7.");
		return xstring_size_ptr();
	}
	
	return createMessage(strHeader.get(), pPKCS7.get(), true);
}

xstring_size_ptr qscrypto::SMIMEUtilityImpl::decrypt(const Part& part,
													 const PrivateKey* pPrivateKey,
													 const Certificate* pCertificate) const
{
	assert(pPrivateKey);
	assert(pCertificate);
	assert(getType(part) == TYPE_ENVELOPED);
	
	Log log(InitThread::getInitThread().getLogger(), L"qscrypto::SMIMEUtilityImpl");
	
	malloc_size_ptr<unsigned char> buf(part.getBodyData());
	if (!buf.get())
		return xstring_size_ptr();
	
	BIOPtr pIn(BIO_new_mem_buf(buf.get(), static_cast<int>(buf.size())));
	PKCS7Ptr pPKCS7(d2i_PKCS7_bio(pIn.get(), 0));
	if (!pPKCS7.get()) {
		Util::logError(log, L"Failed to load PKCS#7.");
		return xstring_size_ptr();
	}
	
	EVP_PKEY* pKey = static_cast<const PrivateKeyImpl*>(pPrivateKey)->getKey();
	X509* pX509 = static_cast<const CertificateImpl*>(pCertificate)->getX509();
	
	BIOPtr pOut(BIO_new(BIO_s_mem()));
	if (PKCS7_decrypt(pPKCS7.get(), pKey, pX509, pOut.get(), 0) != 1) {
		Util::logError(log, L"Failed to decrypt PKCS#7.");
		return xstring_size_ptr();
	}
	
	char* pBuf = 0;
	int nBufLen = BIO_get_mem_data(pOut.get(), &pBuf);
	if (!pBuf)
		return xstring_size_ptr();
	log.debug(L"Decrypted data.", reinterpret_cast<const unsigned char*>(pBuf), nBufLen);
	
	return createMessage(pBuf, nBufLen, part);
}

xstring_size_ptr qscrypto::SMIMEUtilityImpl::createMessage(const CHAR* pszHeader,
														   PKCS7* pPKCS7,
														   bool bEnveloped)
{
	assert(pPKCS7);
	
	malloc_size_ptr<unsigned char> buf(encodePKCS7(pPKCS7));
	if (!buf.get())
		return xstring_size_ptr();
	
	Part part;
	if (!part.create(0, pszHeader, -1))
		return xstring_size_ptr();
	
	PrefixFieldFilter filter("content-");
	if (!part.removeFields(&filter))
		return xstring_size_ptr();
	
	ContentTypeParser contentType(L"application", L"pkcs7-mime");
	contentType.setParameter(L"name", L"smime.p7m");
	const WCHAR* pwszType = bEnveloped ? L"enveloped-data" : L"signed-data";
	contentType.setParameter(L"smime-type", pwszType);
	if (!part.setField(L"Content-Type", contentType))
		return xstring_size_ptr();
	
	ContentDispositionParser contentDisposition(L"attachment");
	contentDisposition.setParameter(L"filename", L"smime.p7m");
	if (!part.setField(L"Content-Disposition", contentDisposition))
		return xstring_size_ptr();
	
	ContentTransferEncodingParser contentTransferEncoding(L"base64");
	if (!part.setField(L"Content-Transfer-Encoding", contentTransferEncoding))
		return xstring_size_ptr();
	
	if (!part.sortHeader())
		return xstring_size_ptr();
	
	if (!part.setBody(reinterpret_cast<CHAR*>(buf.get()), buf.size()))
		return xstring_size_ptr();
	
	return part.getContent();
}

xstring_size_ptr qscrypto::SMIMEUtilityImpl::createMultipartMessage(const CHAR* pszHeader,
																	const Part& part,
																	PKCS7* pPKCS7)
{
	assert(pPKCS7);
	
	malloc_size_ptr<unsigned char> buf(encodePKCS7(pPKCS7));
	if (!buf.get())
		return xstring_size_ptr();
	
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
		_snwprintf(wszBoundary, countof(wszBoundary),
			L"__boundary-%04d%02d%02d%02d%02d%02d%03d%04d__",
			time.wYear, time.wMonth, time.wDay, time.wHour, time.wMinute,
			time.wSecond, time.wMilliseconds, ::GetCurrentThreadId());
		contentType.setParameter(L"boundary", wszBoundary);
		contentType.setParameter(L"protocol", L"application/pkcs7-signature");
		contentType.setParameter(L"micalg", L"SHA1");
		if (!msg.setField(L"Content-Type", contentType))
			return xstring_size_ptr();
	}
	
	std::auto_ptr<Part> pPartContent(part.clone());
	if (!pPartContent.get())
		return xstring_size_ptr();
	msg.addPart(pPartContent);
	
	std::auto_ptr<Part> pPartSignature(new Part());
	{
		ContentTypeParser contentType(L"application", L"pkcs7-signature");
		contentType.setParameter(L"name", L"smime.p7s");
		if (!pPartSignature->setField(L"Content-Type", contentType))
			return xstring_size_ptr();
		
		ContentDispositionParser contentDisposition(L"attachment");
		contentDisposition.setParameter(L"filename", L"smime.p7s");
		if (!pPartSignature->setField(L"Content-Disposition", contentDisposition))
			return xstring_size_ptr();
		
		ContentTransferEncodingParser contentTransferEncoding(L"base64");
		if (!pPartSignature->setField(L"Content-Transfer-Encoding", contentTransferEncoding))
			return xstring_size_ptr();
		
		if (!pPartSignature->setBody(reinterpret_cast<CHAR*>(buf.get()), buf.size()))
			return xstring_size_ptr();
	}
	msg.addPart(pPartSignature);
	
	if (!msg.sortHeader())
		return xstring_size_ptr();
	
	return msg.getContent();
}

xstring_size_ptr qscrypto::SMIMEUtilityImpl::createMessage(const CHAR* pszContent,
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

malloc_size_ptr<unsigned char> qscrypto::SMIMEUtilityImpl::encodePKCS7(PKCS7* pPKCS7)
{
	assert(pPKCS7);
	
	BIOPtr pOut(BIO_new(BIO_s_mem()));
	if (!pOut.get())
		return malloc_size_ptr<unsigned char>();
	if (i2d_PKCS7_bio(pOut.get(), pPKCS7) != 1)
		return malloc_size_ptr<unsigned char>();
	
	unsigned char* pBuf = 0;
	int nBufLen = BIO_get_mem_data(pOut.get(), &pBuf);
	return Base64Encoder(true).encode(pBuf, nBufLen);
}

bool qscrypto::SMIMEUtilityImpl::getCertificates(const qs::AddressListParser& addressList,
												 qs::SMIMECallback* pCallback,
												 STACK_OF(X509)* pCertificates)
{
	const AddressListParser::AddressList& l = addressList.getAddressList();
	for (AddressListParser::AddressList::const_iterator it = l.begin(); it != l.end(); ++it) {
		if (!getCertificates(**it, pCallback, pCertificates))
			return false;
	}
	return true;
}

bool qscrypto::SMIMEUtilityImpl::getCertificates(const qs::AddressParser& address,
												 qs::SMIMECallback* pCallback,
												 STACK_OF(X509)* pCertificates)
{
	Log log(InitThread::getInitThread().getLogger(), L"qscrypto::SMIMEUtilityImpl");
	
	AddressListParser* pGroup = address.getGroup();
	if (pGroup) {
		if (!getCertificates(*pGroup, pCallback, pCertificates))
			return false;
	}
	else {
		wstring_ptr wstrAddress(address.getAddress());
		std::auto_ptr<Certificate> pCertificate(pCallback->getCertificate(wstrAddress.get()));
		if (!pCertificate.get()) {
			log.errorf(L"Failed to get a certificate for %s.", wstrAddress.get());
			return false;
		}
		X509* pX509 = static_cast<CertificateImpl*>(pCertificate.get())->releaseX509();
		sk_X509_push(pCertificates, pX509);
	}
	return true;
}

bool qscrypto::SMIMEUtilityImpl::match(const Certificate* pCertificate,
									   const AddressListParser* pFrom,
									   const AddressListParser* pSender)
{
	std::auto_ptr<Name> pName(pCertificate->getSubject());
	if (pName.get()) {
		wstring_ptr wstrAddress = pName->getEmailAddress();
		if (wstrAddress.get()) {
			if (contains(pFrom, pSender, wstrAddress.get()))
				return true;
		}
	}
	
	std::auto_ptr<GeneralNames> pSubjectAltNames(pCertificate->getSubjectAltNames());
	if (pSubjectAltNames.get()) {
		for (int n = 0; n < pSubjectAltNames->getCount(); ++n) {
			std::auto_ptr<GeneralName> pGeneralName(pSubjectAltNames->getGeneralName(n));
			if (pGeneralName->getType() == GeneralName::TYPE_EMAIL &&
				contains(pFrom, pSender, pGeneralName->getValue().get()))
				return true;
		}
	}
	
	return false;
}

bool qscrypto::SMIMEUtilityImpl::contains(const qs::AddressListParser* pFrom,
										  const qs::AddressListParser* pSender,
										  const WCHAR* pwszAddress)
{
	return (pFrom && pFrom->contains(pwszAddress)) ||
		(pSender && pSender->contains(pwszAddress));
}
