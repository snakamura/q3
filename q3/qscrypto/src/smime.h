/*
 * $Id: smime.h,v 1.1.1.1 2003/04/29 08:07:38 snakamura Exp $
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
	virtual qs::QSTATUS sign(const qs::Part& part,
		bool bMultipart, const qs::PrivateKey* pPrivateKey,
		const qs::Certificate* pCertificate, qs::STRING* pstrMessage) const;
	virtual qs::QSTATUS verify(const qs::Part& part,
		const qs::Store* pStoreCA, qs::STRING* pstrMessage) const;
	virtual qs::QSTATUS encrypt(const qs::Part& part, const qs::Cipher* pCipher,
		qs::SMIMECallback* pCallback, qs::STRING* pstrMessage) const;
	virtual qs::QSTATUS decrypt(const qs::Part& part,
		const qs::PrivateKey* pPrivateKey, const qs::Certificate* pCertificate,
		qs::STRING* pstrMessage) const;

private:
	static qs::QSTATUS createMessage(const qs::Part& part,
		PKCS7* pPKCS7, bool bEnveloped, qs::STRING* pstrMessage);

private:
	SMIMEUtilityImpl(const SMIMEUtilityImpl&);
	SMIMEUtilityImpl& operator=(const SMIMEUtilityImpl&);
};

}

#endif // __SMIME_H__
