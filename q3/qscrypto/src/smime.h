/*
 * $Id$
 *
 * Copyright(C) 1998-2006 Satoshi Nakamura
 * All rights reserved.
 *
 */

#ifndef __SMIME_H__
#define __SMIME_H__

#include <qscrypto.h>
#include <qsmime.h>

#include <openssl/pkcs7.h>


namespace qscrypto {

/****************************************************************************
 *
 * SMIMEUtilityImpl
 *
 */

class SMIMEUtilityImpl : public qs::SMIMEUtility
{
public:
	SMIMEUtilityImpl();
	virtual ~SMIMEUtilityImpl();

public:
	virtual Type getType(const qs::Part& part) const;
	virtual Type getType(const qs::ContentTypeParser* pContentType) const;
	virtual qs::xstring_size_ptr sign(qs::Part* pPart,
									  bool bMultipart,
									  const qs::PrivateKey* pPrivateKey,
									  const qs::Certificate* pCertificate) const;
	virtual qs::xstring_size_ptr verify(const qs::Part& part,
										const qs::Store* pStoreCA,
										unsigned int* pnVerify,
										CertificateList* pListCertificate) const;
	virtual qs::xstring_size_ptr encrypt(qs::Part* pPart,
										 const qs::Cipher* pCipher,
										 qs::SMIMECallback* pCallback) const;
	virtual qs::xstring_size_ptr decrypt(const qs::Part& part,
										 const qs::PrivateKey* pPrivateKey,
										 const qs::Certificate* pCertificate) const;

private:
	static qs::xstring_size_ptr createMessage(const CHAR* pszHeader,
											  PKCS7* pPKCS7,
											  bool bEnveloped);
	static qs::xstring_size_ptr createMultipartMessage(const CHAR* pszHeader,
													   const qs::Part& part,
													   PKCS7* pPKCS7);
	static qs::xstring_size_ptr createMessage(const CHAR* pszContent,
											  size_t nLen,
											  const qs::Part& part);
	static qs::malloc_size_ptr<unsigned char> encodePKCS7(PKCS7* pPKCS7);
	static bool getCertificates(const qs::AddressListParser& addressList,
								qs::SMIMECallback* pCallback,
								STACK_OF(X509)* pCertificates);
	static bool getCertificates(const qs::AddressParser& address,
								qs::SMIMECallback* pCallback,
								STACK_OF(X509)* pCertificates);
	static bool match(const qs::Certificate* pCertificate,
					  const qs::AddressListParser* pFrom,
					  const qs::AddressListParser* pSender);
	static bool contains(const qs::AddressListParser* pFrom,
						 const qs::AddressListParser* pSender,
						 const WCHAR* pwszAddress);

private:
	SMIMEUtilityImpl(const SMIMEUtilityImpl&);
	SMIMEUtilityImpl& operator=(const SMIMEUtilityImpl&);
};

}

#endif // __SMIME_H__
