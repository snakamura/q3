/*
 * $Id$
 *
 * Copyright(C) 1998-2004 Satoshi Nakamura
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
 * NameImpl
 *
 */

class NameImpl : public qs::Name
{
public:
	explicit NameImpl(X509_NAME* pName);
	virtual ~NameImpl();

public:
	virtual qs::wstring_ptr getText() const;
	virtual qs::wstring_ptr getCommonName() const;
	virtual qs::wstring_ptr getEmailAddress() const;

private:
	qs::wstring_ptr getText(int nid) const;

private:
	NameImpl(const NameImpl&);
	NameImpl& operator=(const NameImpl&);

private:
	X509_NAME* pName_;
};


/****************************************************************************
 *
 * CertificateImpl
 *
 */

class CertificateImpl : public qs::Certificate
{
public:
	CertificateImpl();
	explicit CertificateImpl(X509* pX509);
	virtual ~CertificateImpl();

public:
	X509* getX509() const;
	X509* releaseX509();

public:
	virtual bool load(const WCHAR* pwszPath,
					  FileType type,
					  qs::CryptoPasswordCallback* pCallback);
	virtual bool load(qs::InputStream* pStream,
					  FileType type,
					  qs::CryptoPasswordCallback* pCallback);
	virtual qs::wstring_ptr getText() const;
	virtual std::auto_ptr<qs::Name> getSubject() const;
	virtual std::auto_ptr<qs::Name> getIssuer() const;

private:
	CertificateImpl(const CertificateImpl&);
	CertificateImpl& operator=(const CertificateImpl&);

private:
	X509* pX509_;
	bool bFree_;
};


/****************************************************************************
 *
 * PrivateKeyImpl
 *
 */

class PrivateKeyImpl : public qs::PrivateKey
{
public:
	PrivateKeyImpl();
	virtual ~PrivateKeyImpl();

public:
	EVP_PKEY* getKey() const;

public:
	virtual bool load(const WCHAR* pwszPath,
					  FileType type,
					  qs::CryptoPasswordCallback* pCallback);
	virtual bool load(qs::InputStream* pStream,
					  FileType type,
					  qs::CryptoPasswordCallback* pCallback);

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
	PublicKeyImpl();
	virtual ~PublicKeyImpl();

public:
	EVP_PKEY* getKey() const;

public:
	virtual bool load(const WCHAR* pwszPath,
					  FileType type,
					  qs::CryptoPasswordCallback* pCallback);
	virtual bool load(qs::InputStream* pStream,
					  FileType type,
					  qs::CryptoPasswordCallback* pCallback);

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
	StoreImpl();
	virtual ~StoreImpl();

public:
	X509_STORE* getStore() const;

public:
	virtual bool load(const WCHAR* pwszPath,
					  FileType type);

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
	explicit CipherImpl(const WCHAR* pwszName);
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
	virtual std::auto_ptr<qs::Certificate> createCertificate();
	virtual std::auto_ptr<qs::PrivateKey> createPrivateKey();
	virtual std::auto_ptr<qs::PublicKey> createPublicKey();
	virtual std::auto_ptr<qs::Store> createStore();
	virtual std::auto_ptr<qs::Cipher> createCipher(const WCHAR* pwszName);
	virtual std::auto_ptr<qs::SMIMEUtility> createSMIMEUtility();

private:
	CryptoFactoryImpl(const CryptoFactoryImpl&);
	CryptoFactoryImpl& operator=(const CryptoFactoryImpl&);

private:
	static CryptoFactoryImpl factory__;
};

}

#endif // __SECURITY_H__
