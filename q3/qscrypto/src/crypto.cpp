/*
 * $Id$
 *
 * Copyright(C) 1998-2004 Satoshi Nakamura
 * All rights reserved.
 *
 */

#include <qsconv.h>
#include <qsinit.h>
#include <qslog.h>
#include <qsstl.h>

#include <stdio.h>

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

extern "C" int passwordCallback(char* pBuf,
								int nSize,
								int nRWFlag,
								void* pParam);


/****************************************************************************
 *
 * NameImpl
 *
 */

qscrypto::NameImpl::NameImpl(X509_NAME* pName) :
	pName_(pName)
{
}

qscrypto::NameImpl::~NameImpl()
{
}

wstring_ptr qscrypto::NameImpl::getCommonName() const
{
	return getText(NID_commonName);
}

wstring_ptr qscrypto::NameImpl::getEmailAddress() const
{
	return getText(NID_pkcs9_emailAddress);
}

wstring_ptr qscrypto::NameImpl::getText(int nid) const
{
	int nLen = X509_NAME_get_text_by_NID(pName_, nid, 0, 0);
	if (nLen == -1)
		return 0;
	
	string_ptr strText(allocString(nLen + 1));
	if (X509_NAME_get_text_by_NID(pName_, nid, strText.get(), nLen + 1) == -1)
		return 0;
	
	return mbs2wcs(strText.get());
}


/****************************************************************************
 *
 * CertificateImpl
 *
 */

qscrypto::CertificateImpl::CertificateImpl() :
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
	if (bFree_ && pX509_)
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

bool qscrypto::CertificateImpl::load(const WCHAR* pwszPath,
									 FileType type,
									 CryptoPasswordCallback* pCallback)
{
	assert(pwszPath);
	
	FileInputStream stream(pwszPath);
	if (!stream)
		return false;
	BufferedInputStream bufferedStream(&stream, false);
	
	return load(&bufferedStream, type, pCallback);
}

bool qscrypto::CertificateImpl::load(InputStream* pStream,
									 FileType type,
									 CryptoPasswordCallback* pCallback)
{
	assert(pStream);
	
	malloc_size_ptr<unsigned char> buf(Util::createBIOFromStream(pStream));
	if (!buf.get())
		return false;
	
	BIOPtr pIn(BIO_new_mem_buf(buf.get(), buf.size()));
	
	switch (type) {
	case FILETYPE_PEM:
		pX509_ = PEM_read_bio_X509(pIn.get(), 0,
			pCallback ? &passwordCallback : 0, pCallback);
		if (!pX509_)
			return false;
		break;
	default:
		assert(false);
		break;
	}
	
	return true;
}

wstring_ptr qscrypto::CertificateImpl::getText() const
{
	BIOPtr pOut(BIO_new(BIO_s_mem()));
	if (X509_print(pOut.get(), pX509_) == 0)
		return 0;
	
	char* pBuf = 0;
	int nLen = BIO_get_mem_data(pOut.get(), &pBuf);
	
	return mbs2wcs(pBuf, nLen);
}

std::auto_ptr<Name> qscrypto::CertificateImpl::getSubject() const
{
	X509_NAME* pX509Name = X509_get_subject_name(pX509_);
	if (!pX509Name)
		return std::auto_ptr<Name>(0);
	return std::auto_ptr<Name>(new NameImpl(pX509Name));
}

std::auto_ptr<Name> qscrypto::CertificateImpl::getIssuer() const
{
	X509_NAME* pX509Name = X509_get_issuer_name(pX509_);
	if (!pX509Name)
		return std::auto_ptr<Name>(0);
	return std::auto_ptr<Name>(new NameImpl(pX509Name));
}


/****************************************************************************
 *
 * PrivateKeyImpl
 *
 */

qscrypto::PrivateKeyImpl::PrivateKeyImpl() :
	pKey_(0)
{
}

qscrypto::PrivateKeyImpl::~PrivateKeyImpl()
{
	if (pKey_)
		EVP_PKEY_free(pKey_);
}

EVP_PKEY* qscrypto::PrivateKeyImpl::getKey() const
{
	return pKey_;
}

bool qscrypto::PrivateKeyImpl::load(const WCHAR* pwszPath,
									FileType type,
									CryptoPasswordCallback* pCallback)
{
	assert(pwszPath);
	
	FileInputStream stream(pwszPath);
	if (!stream)
		return false;
	BufferedInputStream bufferedStream(&stream, false);
	
	return load(&bufferedStream, type, pCallback);
}

bool qscrypto::PrivateKeyImpl::load(InputStream* pStream,
									FileType type,
									CryptoPasswordCallback* pCallback)
{
	assert(pStream);
	
	malloc_size_ptr<unsigned char> buf(Util::createBIOFromStream(pStream));
	if (!buf.get())
		return false;
	
	BIOPtr pIn(BIO_new_mem_buf(buf.get(), buf.size()));
	
	switch (type) {
	case FILETYPE_PEM:
		pKey_ = PEM_read_bio_PrivateKey(pIn.get(), 0,
			pCallback ? &passwordCallback : 0, pCallback);
		if (!pKey_)
			return false;
		break;
	default:
		assert(false);
		break;
	}
	
	return true;
}


/****************************************************************************
 *
 * PublicKeyImpl
 *
 */

qscrypto::PublicKeyImpl::PublicKeyImpl() :
	pKey_(0)
{
}

qscrypto::PublicKeyImpl::~PublicKeyImpl()
{
	if (pKey_)
		EVP_PKEY_free(pKey_);
}

EVP_PKEY* qscrypto::PublicKeyImpl::getKey() const
{
	return pKey_;
}

bool qscrypto::PublicKeyImpl::load(const WCHAR* pwszPath,
								   FileType type,
								   CryptoPasswordCallback* pCallback)
{
	assert(pwszPath);
	
	FileInputStream stream(pwszPath);
	if (!stream)
		return false;
	BufferedInputStream bufferedStream(&stream, false);
	
	return load(&bufferedStream, type, pCallback);
}

bool qscrypto::PublicKeyImpl::load(InputStream* pStream,
								   FileType type,
								   CryptoPasswordCallback* pCallback)
{
	assert(pStream);
	
	malloc_size_ptr<unsigned char> buf(Util::createBIOFromStream(pStream));
	if (!buf.get())
		return false;
	
	BIOPtr pIn(BIO_new_mem_buf(buf.get(), buf.size()));
	
	switch (type) {
	case FILETYPE_PEM:
		pKey_ = PEM_read_bio_PUBKEY(pIn.get(), 0,
			pCallback ? &passwordCallback : 0, pCallback);
		if (!pKey_)
			return false;
		break;
	default:
		assert(false);
		break;
	}
	
	return true;
}


/****************************************************************************
 *
 * StoreImpl
 *
 */

qscrypto::StoreImpl::StoreImpl() :
	pStore_(0)
{
	pStore_ = X509_STORE_new();
	if (!pStore_)
		return;
}

qscrypto::StoreImpl::~StoreImpl()
{
	if (pStore_)
		X509_STORE_free(pStore_);
}

X509_STORE* qscrypto::StoreImpl::getStore() const
{
	return pStore_;
}

bool qscrypto::StoreImpl::load(const WCHAR* pwszPath,
							   FileType type)
{
	assert(pwszPath);
	
	Log log(InitThread::getInitThread().getLogger(), L"qscrypto::StoreImpl");
	
#if 1
	switch (type) {
	case FILETYPE_PEM:
		{
			FileInputStream stream(pwszPath);
			if (!stream)
				return false;
			BufferedInputStream bufferedStream(&stream, false);
			
			malloc_size_ptr<unsigned char> buf(Util::createBIOFromStream(&bufferedStream));
			if (!buf.get())
				return false;
			
			BIOPtr pIn(BIO_new_mem_buf(buf.get(), buf.size()));
			
			STACK_OF(X509_INFO)* pStackInfo =
				PEM_X509_INFO_read_bio(pIn.get(), 0, 0, 0);
			if (!pStackInfo)
				return false;
			for (int n = 0; n < sk_X509_INFO_num(pStackInfo); ++n) {
				X509_INFO* pInfo = sk_X509_INFO_value(pStackInfo, n);
				if (pInfo->x509)
					X509_STORE_add_cert(pStore_, pInfo->x509);
				if (pInfo->crl)
					X509_STORE_add_crl(pStore_, pInfo->crl);
			}
			sk_X509_INFO_pop_free(pStackInfo, X509_INFO_free);
		}
		break;
	default:
		assert(false);
		break;
	}
#else
	string_ptr strPath(wcs2mbs(pwszPath));
	
	X509_LOOKUP* pLookup = X509_STORE_add_lookup(pStore_, X509_LOOKUP_file());
	if (!pLookup) {
		Util::logError(log, L"X509_STORE_add_lookup failed");
		return false;
	}
	switch (type) {
	case FILETYPE_PEM:
		{
			int n = X509_LOOKUP_load_file(pLookup,
				strPath.get(), X509_FILETYPE_PEM);
			if (!n) {
				Util::logError(log, L"X509_LOOKUP_load_file failed");
				return false;
			}
		}
		break;
	default:
		assert(false);
		break;
	}
#endif
	
	return true;
}


/****************************************************************************
 *
 * CipherImpl
 *
 */

qscrypto::CipherImpl::CipherImpl(const WCHAR* pwszName) :
	pCipher_(0)
{
	string_ptr strName(wcs2mbs(pwszName));
	pCipher_ = EVP_get_cipherbyname(strName.get());
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
	registerFactory(this);
}

qscrypto::CryptoFactoryImpl::~CryptoFactoryImpl()
{
	unregisterFactory(this);
}

std::auto_ptr<Certificate> qscrypto::CryptoFactoryImpl::createCertificate()
{
	return std::auto_ptr<Certificate>(new CertificateImpl());
}

std::auto_ptr<PrivateKey> qscrypto::CryptoFactoryImpl::createPrivateKey()
{
	return std::auto_ptr<PrivateKey>(new PrivateKeyImpl());
}

std::auto_ptr<PublicKey> qscrypto::CryptoFactoryImpl::createPublicKey()
{
	return std::auto_ptr<PublicKey>(new PublicKeyImpl());
}

std::auto_ptr<Store> qscrypto::CryptoFactoryImpl::createStore()
{
	std::auto_ptr<StoreImpl> pStore(new StoreImpl());
	if (!pStore->getStore())
		return std::auto_ptr<Store>(0);
	return pStore;
}

std::auto_ptr<Cipher> qscrypto::CryptoFactoryImpl::createCipher(const WCHAR* pwszName)
{
	std::auto_ptr<CipherImpl> pCipher(new CipherImpl(pwszName));
	if (!pCipher->getCipher())
		return std::auto_ptr<Cipher>(0);
	return pCipher;
}

std::auto_ptr<SMIMEUtility> qscrypto::CryptoFactoryImpl::createSMIMEUtility()
{
	return std::auto_ptr<SMIMEUtility>(new SMIMEUtilityImpl());
}


/****************************************************************************
 *
 * Global functions
 *
 */

extern "C" int passwordCallback(char* pBuf,
								int nSize,
								int nRWFlag,
								void* pParam)
{
	CryptoPasswordCallback* pCallback = static_cast<CryptoPasswordCallback*>(pParam);
	
	QTRY {
		wstring_ptr wstrPassword(pCallback->getPassword());
		string_ptr strPassword(wcs2mbs(wstrPassword.get()));
		int nLen = strlen(strPassword.get());
		if (nLen >= nSize)
			return 0;
		strcpy(pBuf, strPassword.get());
		return nLen;
	}
	QCATCH_ALL() {
		return 0;
	}
}
