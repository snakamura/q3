/*
 * $Id$
 *
 * Copyright(C) 1998-2003 Satoshi Nakamura
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
	SMIMEUtilityImpl(qs::QSTATUS* pstatus);
	virtual ~SMIMEUtilityImpl();

public:
	virtual Type getType(const qs::Part& part) const;
	virtual qs::QSTATUS sign(qs::Part* pPart, bool bMultipart,
		const qs::PrivateKey* pPrivateKey, const qs::Certificate* pCertificate,
		qs::SMIMECallback* pCallback, qs::STRING* pstrMessage) const;
	virtual qs::QSTATUS verify(const qs::Part& part,
		const qs::Store* pStoreCA, qs::STRING* pstrMessage) const;
	virtual qs::QSTATUS encrypt(qs::Part* pPart, const qs::Cipher* pCipher,
		qs::SMIMECallback* pCallback, qs::STRING* pstrMessage) const;
	virtual qs::QSTATUS decrypt(const qs::Part& part,
		const qs::PrivateKey* pPrivateKey, const qs::Certificate* pCertificate,
		qs::STRING* pstrMessage) const;

private:
	static qs::QSTATUS createMessage(const CHAR* pszHeader,
		PKCS7* pPKCS7, bool bEnveloped, qs::STRING* pstrMessage);

private:
	SMIMEUtilityImpl(const SMIMEUtilityImpl&);
	SMIMEUtilityImpl& operator=(const SMIMEUtilityImpl&);
};

}

#endif // __SMIME_H__
