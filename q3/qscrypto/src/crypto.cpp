/*
 * $Id$
 *
 * Copyright(C) 1998-2003 Satoshi Nakamura
 * All rights reserved.
 *
 */

#include <stdio.h>
#include <qsconv.h>
#include <qsnew.h>


#include <openssl/pem.h>

#include "crypto.h"
#include "smime.h"
#include "util.h"

using namespace qscrypto;
using namespace qs;


/****************************************************************************
 *
 * Global functions
 *
 */

extern "C" int passwordCallback(char* pBuf, int nSize, int nRWFlag, void* pParam);


/****************************************************************************
 *
 * NameImpl
 *
 */

qscrypto::NameImpl::NameImpl(X509_NAME* pName, QSTATUS* pstatus) :
	pName_(pName)
{
}

qscrypto::NameImpl::~NameImpl()
{
}

QSTATUS qscrypto::NameImpl::getCommonName(WSTRING* pwstrCommonName) const
{
	int nLen = X509_NAME_get_text_by_NID(pName_, NID_commonName, 0, 0);
	if (nLen == -1)
		return QSTATUS_FAIL;
	
	string_ptr<STRING> strName(allocString(nLen + 1));
	if (!strName.get())
		return QSTATUS_OUTOFMEMORY;
	
	X509_NAME_get_text_by_NID(pName_, NID_commonName, strName.get(), nLen + 1);
	
	*pwstrCommonName = mbs2wcs(strName.get());
	if (!*pwstrCommonName)
		return QSTATUS_OUTOFMEMORY;
	
	return QSTATUS_SUCCESS;
}


/****************************************************************************
 *
 * CertificateImpl
 *
 */

qscrypto::CertificateImpl::CertificateImpl(QSTATUS* pstatus) :
	pX509_(0),
	bFree_(true)
{
}

qscrypto::CertificateImpl::CertificateImpl(X509* pX509) :
	pX509_(pX509),
	bFree_(false)
{
}

qscrypto::CertificateImpl::~CertificateImpl()
{
	if (bFree_)
		X509_free(pX509_);
}

X509* qscrypto::CertificateImpl::getX509() const
{
	return pX509_;
}

X509* qscrypto::CertificateImpl::releaseX509()
{
	X509* p = pX509_;
	pX509_ = 0;
	return p;
}

QSTATUS qscrypto::CertificateImpl::load(const WCHAR* pwszPath,
	FileType type, PasswordCallback* pCallback)
{
	assert(pwszPath);
	
	DECLARE_QSTATUS();
	
	FileInputStream stream(pwszPath, &status);
	CHECK_QSTATUS();
	BufferedInputStream bufferedStream(&stream, false, &status);
	CHECK_QSTATUS();
	
	return load(&bufferedStream, type, pCallback);
}

QSTATUS qscrypto::CertificateImpl::load(InputStream* pStream,
	FileType type, PasswordCallback* pCallback)
{
	assert(pStream);
	
	DECLARE_QSTATUS();
	
	unsigned char* p = 0;
	size_t nLen = 0;
	status = Util::createBIOFromStream(pStream, &p, &nLen);
	CHECK_QSTATUS();
	malloc_ptr<unsigned char> pBuf(p);
	
	BIOPtr pIn(BIO_new_mem_buf(p, nLen));
	
	switch (type) {
	case FILETYPE_PEM:
		pX509_ = PEM_read_bio_X509(pIn.get(), 0,
			pCallback ? &passwordCallback : 0, pCallback);
		if (!pX509_)
			return QSTATUS_FAIL;
		break;
	default:
		assert(false);
		break;
	}
	
	return QSTATUS_SUCCESS;
}

QSTATUS qscrypto::CertificateImpl::getSubject(Name** ppName) const
{
	DECLARE_QSTATUS();
	
	X509_NAME* pX509Name = X509_get_subject_name(pX509_);
	if (!pX509Name)
		return QSTATUS_FAIL;
	
	std::auto_ptr<NameImpl> pName;
	status = newQsObject(pX509Name, &pName);
	CHECK_QSTATUS();
	
	*ppName = pName.release();
	
	return QSTATUS_SUCCESS;
}

QSTATUS qscrypto::CertificateImpl::getIssuer(Name** ppName) const
{
	DECLARE_QSTATUS();
	
	X509_NAME* pX509Name = X509_get_issuer_name(pX509_);
	if (!pX509Name)
		return QSTATUS_FAIL;
	
	std::auto_ptr<NameImpl> pName;
	status = newQsObject(pX509Name, &pName);
	CHECK_QSTATUS();
	
	*ppName = pName.release();
	
	return QSTATUS_SUCCESS;
}


/****************************************************************************
 *
 * PrivateKeyImpl
 *
 */

qscrypto::PrivateKeyImpl::PrivateKeyImpl(QSTATUS* pstatus) :
	pKey_(0)
{
}

qscrypto::PrivateKeyImpl::~PrivateKeyImpl()
{
	EVP_PKEY_free(pKey_);
}

EVP_PKEY* qscrypto::PrivateKeyImpl::getKey() const
{
	return pKey_;
}

QSTATUS qscrypto::PrivateKeyImpl::load(const WCHAR* pwszPath,
	FileType type, PasswordCallback* pCallback)
{
	assert(pwszPath);
	
	DECLARE_QSTATUS();
	
	FileInputStream stream(pwszPath, &status);
	CHECK_QSTATUS();
	BufferedInputStream bufferedStream(&stream, false, &status);
	CHECK_QSTATUS();
	
	return load(&bufferedStream, type, pCallback);
}

QSTATUS qscrypto::PrivateKeyImpl::load(InputStream* pStream,
	FileType type, PasswordCallback* pCallback)
{
	assert(pStream);
	
	DECLARE_QSTATUS();
	
	unsigned char* p = 0;
	size_t nLen = 0;
	status = Util::createBIOFromStream(pStream, &p, &nLen);
	CHECK_QSTATUS();
	malloc_ptr<unsigned char> pBuf(p);
	
	BIOPtr pIn(BIO_new_mem_buf(p, nLen));
	
	switch (type) {
	case FILETYPE_PEM:
		pKey_ = PEM_read_bio_PrivateKey(pIn.get(), 0,
			pCallback ? &passwordCallback : 0, pCallback);
		if (!pKey_)
			return QSTATUS_FAIL;
		break;
	default:
		assert(false);
		break;
	}
	
	return QSTATUS_SUCCESS;
}


/****************************************************************************
 *
 * PublicKeyImpl
 *
 */

qscrypto::PublicKeyImpl::PublicKeyImpl(QSTATUS* pstatus) :
	pKey_(0)
{
}

qscrypto::PublicKeyImpl::~PublicKeyImpl()
{
	EVP_PKEY_free(pKey_);
}

EVP_PKEY* qscrypto::PublicKeyImpl::getKey() const
{
	return pKey_;
}

QSTATUS qscrypto::PublicKeyImpl::load(const WCHAR* pwszPath,
	FileType type, PasswordCallback* pCallback)
{
	assert(pwszPath);
	
	DECLARE_QSTATUS();
	
	FileInputStream stream(pwszPath, &status);
	CHECK_QSTATUS();
	BufferedInputStream bufferedStream(&stream, false, &status);
	CHECK_QSTATUS();
	
	return load(&bufferedStream, type, pCallback);
}

QSTATUS qscrypto::PublicKeyImpl::load(InputStream* pStream,
	FileType type, PasswordCallback* pCallback)
{
	assert(pStream);
	
	DECLARE_QSTATUS();
	
	unsigned char* p = 0;
	size_t nLen = 0;
	status = Util::createBIOFromStream(pStream, &p, &nLen);
	CHECK_QSTATUS();
	malloc_ptr<unsigned char> pBuf(p);
	
	BIOPtr pIn(BIO_new_mem_buf(p, nLen));
	
	switch (type) {
	case FILETYPE_PEM:
		pKey_ = PEM_read_bio_PUBKEY(pIn.get(), 0,
			pCallback ? &passwordCallback : 0, pCallback);
		if (!pKey_)
			return QSTATUS_FAIL;
		break;
	default:
		assert(false);
		break;
	}
	
	return QSTATUS_SUCCESS;
}


/****************************************************************************
 *
 * StoreImpl
 *
 */

qscrypto::StoreImpl::StoreImpl(QSTATUS* pstatus) :
	pStore_(0)
{
	pStore_ = X509_STORE_new();
	if (!pStore_) {
		*pstatus = QSTATUS_FAIL;
		return;
	}
}

qscrypto::StoreImpl::~StoreImpl()
{
	X509_STORE_free(pStore_);
}

X509_STORE* qscrypto::StoreImpl::getStore() const
{
	return pStore_;
}

QSTATUS qscrypto::StoreImpl::load(const WCHAR* pwszFile, FileType type)
{
	assert(pwszFile);
	
	string_ptr<STRING> strFile(wcs2mbs(pwszFile));
	if (!strFile.get())
		return QSTATUS_OUTOFMEMORY;
	
	X509_LOOKUP* pLookup = X509_STORE_add_lookup(pStore_, X509_LOOKUP_file());
	switch (type) {
	case FILETYPE_PEM:
		if (!X509_LOOKUP_load_file(pLookup, strFile.get(), X509_FILETYPE_PEM))
			return QSTATUS_FAIL;
		break;
	default:
		assert(false);
		break;
	}
	
	return QSTATUS_SUCCESS;
}


/****************************************************************************
 *
 * CipherImpl
 *
 */

qscrypto::CipherImpl::CipherImpl(const WCHAR* pwszName, QSTATUS* pstatus) :
	pCipher_(0)
{
	string_ptr<STRING> strName(wcs2mbs(pwszName));
	if (!strName.get()) {
		*pstatus = QSTATUS_OUTOFMEMORY;
		return;
	}
	
	pCipher_ = EVP_get_cipherbyname(strName.get());
	if (!pCipher_) {
		*pstatus = QSTATUS_FAIL;
		return;
	}
}

qscrypto::CipherImpl::~CipherImpl()
{
}

const EVP_CIPHER* qscrypto::CipherImpl::getCipher() const
{
	return pCipher_;
}


/****************************************************************************
 *
 * CryptoFactoryImpl
 *
 */

CryptoFactoryImpl qscrypto::CryptoFactoryImpl::factory__;

qscrypto::CryptoFactoryImpl::CryptoFactoryImpl()
{
	regist(this);
}

qscrypto::CryptoFactoryImpl::~CryptoFactoryImpl()
{
	unregist(this);
}

QSTATUS qscrypto::CryptoFactoryImpl::createCertificate(
	Certificate** ppCertificate)
{
	assert(ppCertificate);
	
	DECLARE_QSTATUS();
	
	CertificateImpl* pCertificate;
	status = newQsObject(&pCertificate);
	CHECK_QSTATUS();
	
	*ppCertificate = pCertificate;
	
	return QSTATUS_SUCCESS;
}

QSTATUS qscrypto::CryptoFactoryImpl::createPrivateKey(
	PrivateKey** ppPrivateKey)
{
	assert(ppPrivateKey);
	
	DECLARE_QSTATUS();
	
	PrivateKeyImpl* pPrivateKey;
	status = newQsObject(&pPrivateKey);
	CHECK_QSTATUS();
	
	*ppPrivateKey = pPrivateKey;
	
	return QSTATUS_SUCCESS;
}

QSTATUS qscrypto::CryptoFactoryImpl::createPublicKey(
	PublicKey** ppPublicKey)
{
	assert(ppPublicKey);
	
	DECLARE_QSTATUS();
	
	PublicKeyImpl* pPublicKey;
	status = newQsObject(&pPublicKey);
	CHECK_QSTATUS();
	
	*ppPublicKey = pPublicKey;
	
	return QSTATUS_SUCCESS;
}

QSTATUS qscrypto::CryptoFactoryImpl::createStore(Store** ppStore)
{
	assert(ppStore);
	
	DECLARE_QSTATUS();
	
	StoreImpl* pStore;
	status = newQsObject(&pStore);
	CHECK_QSTATUS();
	
	*ppStore = pStore;
	
	return QSTATUS_SUCCESS;
}

QSTATUS qscrypto::CryptoFactoryImpl::createCipher(
	const WCHAR* pwszName, Cipher** ppCipher)
{
	assert(ppCipher);
	
	DECLARE_QSTATUS();
	
	CipherImpl* pCipher;
	status = newQsObject(pwszName, &pCipher);
	CHECK_QSTATUS();
	
	*ppCipher = pCipher;
	
	return QSTATUS_SUCCESS;
}

QSTATUS qscrypto::CryptoFactoryImpl::createSMIMEUtility(
	SMIMEUtility** ppSMIMEUtility)
{
	assert(ppSMIMEUtility);
	
	DECLARE_QSTATUS();
	
	SMIMEUtilityImpl* pSMIMEUtility;
	status = newQsObject(&pSMIMEUtility);
	CHECK_QSTATUS();
	
	*ppSMIMEUtility = pSMIMEUtility;
	
	return QSTATUS_SUCCESS;
}


/****************************************************************************
 *
 * Global functions
 *
 */

extern "C" int passwordCallback(char* pBuf, int nSize, int nRWFlag, void* pParam)
{
	DECLARE_QSTATUS();
	
	PasswordCallback* pCallback = static_cast<PasswordCallback*>(pParam);
	
	string_ptr<WSTRING> wstrPassword;
	status = pCallback->getPassword(&wstrPassword);
	CHECK_QSTATUS_VALUE(0);
	
	string_ptr<STRING> strPassword(wcs2mbs(wstrPassword.get()));
	if (!strPassword.get())
		return 0;
	int nLen = strlen(strPassword.get());
	if (nLen >= nSize)
		return 0;
	
	strcpy(pBuf, strPassword.get());
	
	return nLen;
}
