/*
 * $Id$
 *
 * Copyright(C) 1998-2004 Satoshi Nakamura
 * All rights reserved.
 *
 */

#ifndef __SMIME_H__
#define __SMIME_H__

#include <qscrypto.h>

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
	virtual qs::xstring_ptr sign(qs::Part* pPart,
								 bool bMultipart,
								 const qs::PrivateKey* pPrivateKey,
								 const qs::Certificate* pCertificate,
								 qs::SMIMECallback* pCallback) const;
	virtual qs::xstring_ptr verify(const qs::Part& part,
								   const qs::Store* pStoreCA) const;
	virtual qs::xstring_ptr encrypt(qs::Part* pPart,
									const qs::Cipher* pCipher,
									qs::SMIMECallback* pCallback) const;
	virtual qs::xstring_ptr decrypt(const qs::Part& part,
									const qs::PrivateKey* pPrivateKey,
									const qs::Certificate* pCertificate) const;

private:
	static qs::xstring_ptr createMessage(const CHAR* pszHeader,
										 PKCS7* pPKCS7,
										 bool bEnveloped);
	static qs::xstring_ptr createMessage(const CHAR* pszContent,
										 size_t nLen,
										 const qs::Part& part);

private:
	SMIMEUtilityImpl(const SMIMEUtilityImpl&);
	SMIMEUtilityImpl& operator=(const SMIMEUtilityImpl&);
};

}

#endif // __SMIME_H__
