/*
 * $Id: crypto.h,v 1.1.1.1 2003/04/29 08:07:38 snakamura Exp $
 *
 * Copyright(C) 1998-2003 Satoshi Nakamura
 * All rights reserved.
 *
 */

#ifndef __SECURITY_H__
#define __SECURITY_H__

#include <qscrypto.h>

#include <openssl/x509.h>


namespace qscrypto {

/****************************************************************************
 *
 * CertificateImpl
 *
 */

class CertificateImpl : public qs::Certificate
{
public:
	CertificateImpl(qs::QSTATUS* pstatus);
	virtual ~CertificateImpl();

public:
	X509* getX509() const;
	X509* releaseX509();

public:
	virtual qs::QSTATUS load(const WCHAR* pwszPath,
		FileType type, qs::PasswordCallback* pCallback);
	virtual qs::QSTATUS load(qs::InputStream* pStream,
		FileType type, qs::PasswordCallback* pCallback);

private:
	CertificateImpl(const CertificateImpl&);
	CertificateImpl& operator=(const CertificateImpl&);

private:
	X509* pX509_;
};


/****************************************************************************
 *
 * PrivateKeyImpl
 *
 */

class PrivateKeyImpl : public qs::PrivateKey
{
public:
	PrivateKeyImpl(qs::QSTATUS* pstatus);
	virtual ~PrivateKeyImpl();

public:
	EVP_PKEY* getKey() const;

public:
	virtual qs::QSTATUS load(const WCHAR* pwszPath,
		FileType type, qs::PasswordCallback* pCallback);
	virtual qs::QSTATUS load(qs::InputStream* pStream,
		FileType type, qs::PasswordCallback* pCallback);

private:
	PrivateKeyImpl(const PrivateKeyImpl&);
	PrivateKeyImpl& operator=(const PrivateKeyImpl&);

private:
	EVP_PKEY* pKey_;
};


/****************************************************************************
 *
 * PublicKeyImpl
 *
 */

class PublicKeyImpl : public qs::PublicKey
{
public:
	PublicKeyImpl(qs::QSTATUS* pstatus);
	virtual ~PublicKeyImpl();

public:
	EVP_PKEY* getKey() const;

public:
	virtual qs::QSTATUS load(const WCHAR* pwszPath,
		FileType type, qs::PasswordCallback* pCallback);
	virtual qs::QSTATUS load(qs::InputStream* pStream,
		FileType type, qs::PasswordCallback* pCallback);

private:
	PublicKeyImpl(const PublicKeyImpl&);
	PublicKeyImpl& operator=(const PublicKeyImpl&);

private:
	EVP_PKEY* pKey_;
};


/****************************************************************************
 *
 * StoreImpl
 *
 */

class StoreImpl : public qs::Store
{
public:
	StoreImpl(qs::QSTATUS* pstatus);
	virtual ~StoreImpl();

public:
	X509_STORE* getStore() const;

public:
	virtual qs::QSTATUS load(const WCHAR* pwszFile, FileType type);

private:
	StoreImpl(const StoreImpl&);
	StoreImpl& operator=(const StoreImpl&);

private:
	X509_STORE* pStore_;
};


/****************************************************************************
 *
 * CipherImpl
 *
 */

class CipherImpl : public qs::Cipher
{
public:
	CipherImpl(const WCHAR* pwszName, qs::QSTATUS* pstatus);
	virtual ~CipherImpl();

public:
	const EVP_CIPHER* getCipher() const;

private:
	CipherImpl(const CipherImpl&);
	CipherImpl& operator=(const CipherImpl&);

private:
	const EVP_CIPHER* pCipher_;
};


/****************************************************************************
 *
 * CryptoFactoryImpl
 *
 */

class CryptoFactoryImpl : public qs::CryptoFactory
{
public:
	CryptoFactoryImpl();
	virtual ~CryptoFactoryImpl();

public:
	virtual qs::QSTATUS createCertificate(qs::Certificate** ppCertificate);
	virtual qs::QSTATUS createPrivateKey(qs::PrivateKey** ppPrivateKey);
	virtual qs::QSTATUS createPublicKey(qs::PublicKey** ppPublicKey);
	virtual qs::QSTATUS createStore(qs::Store** ppStore);
	virtual qs::QSTATUS createCipher(
		const WCHAR* pwszName, qs::Cipher** ppCipher);
	virtual qs::QSTATUS createSMIMEUtility(
		qs::SMIMEUtility** ppSMIMEUtility);

private:
	CryptoFactoryImpl(const CryptoFactoryImpl&);
	CryptoFactoryImpl& operator=(const CryptoFactoryImpl&);

private:
	static CryptoFactoryImpl factory__;
};

}

#endif // __SECURITY_H__
