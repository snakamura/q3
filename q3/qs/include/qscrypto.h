/*
 * $Id: qscrypto.h,v 1.1.1.1 2003/04/29 08:07:34 snakamura Exp $
 *
 * Copyright(C) 1998-2003 Satoshi Nakamura
 * All rights reserved.
 *
 */

#ifndef __QSCRYPTO_H__
#define __QSCRYPTO_H__

#include <qs.h>
#include <qsstring.h>

#include <memory>


namespace qs {

class Certificate;
class PrivateKey;
class PublicKey;
class PasswordCallback;
class Store;
class Cipher;
class SMIMEUtility;
class SMIMECallback;
class CryptoFactory;

class InputStream;
class Part;


/****************************************************************************
 *
 * Certificate
 *
 */

class QSEXPORTCLASS Certificate
{
public:
	enum FileType {
		FILETYPE_PEM
	};

public:
	virtual ~Certificate();

public:
	virtual QSTATUS load(const WCHAR* pwszPath,
		FileType type, PasswordCallback* pCallback) = 0;
	virtual QSTATUS load(InputStream* pStream,
		FileType type, PasswordCallback* pCallback) = 0;

public:
	static QSTATUS getInstance(Certificate** ppCertificate);
};


/****************************************************************************
 *
 * PrivateKey
 *
 */

class QSEXPORTCLASS PrivateKey
{
public:
	enum FileType {
		FILETYPE_PEM
	};

public:
	virtual ~PrivateKey();

public:
	virtual QSTATUS load(const WCHAR* pwszPath,
		FileType type, PasswordCallback* pCallback) = 0;
	virtual QSTATUS load(InputStream* pStream,
		FileType type, PasswordCallback* pCallback) = 0;

public:
	static QSTATUS getInstance(PrivateKey** ppPrivateKey);
};


/****************************************************************************
 *
 * PublicKey
 *
 */

class QSEXPORTCLASS PublicKey
{
public:
	enum FileType {
		FILETYPE_PEM
	};

public:
	virtual ~PublicKey();

public:
	virtual QSTATUS load(const WCHAR* pwszPath,
		FileType type, PasswordCallback* pCallback) = 0;
	virtual QSTATUS load(InputStream* pStream,
		FileType type, PasswordCallback* pCallback) = 0;

public:
	static QSTATUS getInstance(PublicKey** ppPublicKey);
};


/****************************************************************************
 *
 * PasswordCallback
 *
 */

class QSEXPORTCLASS PasswordCallback
{
public:
	virtual ~PasswordCallback();

public:
	virtual QSTATUS getPassword(WSTRING* pwstrPassword) = 0;
};


/****************************************************************************
 *
 * Store
 *
 */

class QSEXPORTCLASS Store
{
public:
	enum FileType {
		FILETYPE_PEM
	};

public:
	virtual ~Store();

public:
	virtual QSTATUS load(const WCHAR* pwszFile, FileType type) = 0;

public:
	static QSTATUS getInstance(Store** ppStore);
};


/****************************************************************************
 *
 * Cipher
 *
 */

class QSEXPORTCLASS Cipher
{
public:
	virtual ~Cipher();

public:
	static QSTATUS getInstance(const WCHAR* pwszName, Cipher** ppCipher);
};


/****************************************************************************
 *
 * SMIMEUtility
 *
 */

class QSEXPORTCLASS SMIMEUtility
{
public:
	enum Type {
		TYPE_NONE,
		TYPE_SIGNED,
		TYPE_MULTIPARTSIGNED,
		TYPE_ENVELOPED
	};

public:
	virtual ~SMIMEUtility();

public:
	virtual Type getType(const Part& part) const = 0;
	virtual QSTATUS sign(const Part& part, bool bMultipart,
		const PrivateKey* pPrivateKey, const Certificate* pCertificate,
		STRING* pstrMessage) const = 0;
	virtual QSTATUS verify(const Part& part,
		const Store* pStoreCA, STRING* pstrMessage) const = 0;
	virtual QSTATUS encrypt(const Part& part, const Cipher* pCipher,
		SMIMECallback* pCallback, STRING* pstrMessage) const = 0;
	virtual QSTATUS decrypt(const Part& part,
		const PrivateKey* pPrivateKey, const Certificate* pCertificate,
		STRING* pstrMessage) const = 0;

public:
	static QSTATUS getInstance(SMIMEUtility** ppSMIMEUtility);
};


/****************************************************************************
 *
 * SMIMECallback
 *
 */

class QSEXPORTCLASS SMIMECallback
{
public:
	virtual ~SMIMECallback();

public:
	virtual QSTATUS getCertificate(const WCHAR* pwszAddress,
		Certificate** ppCertificate) = 0;
};


/****************************************************************************
 *
 * CryptoFactory
 *
 */

class QSEXPORTCLASS CryptoFactory
{
protected:
	CryptoFactory();

public:
	virtual ~CryptoFactory();

public:
	virtual QSTATUS createCertificate(Certificate** ppCertificate) = 0;
	virtual QSTATUS createPrivateKey(PrivateKey** ppPrivateKey) = 0;
	virtual QSTATUS createPublicKey(PublicKey** ppPublicKey) = 0;
	virtual QSTATUS createStore(Store** ppStore) = 0;
	virtual QSTATUS createCipher(const WCHAR* pwszName, Cipher** ppCipher) = 0;
	virtual QSTATUS createSMIMEUtility(SMIMEUtility** ppSMIMEUtility) = 0;

public:
	static CryptoFactory* getFactory();

protected:
	static QSTATUS regist(CryptoFactory* pFactory);
	static QSTATUS unregist(CryptoFactory* pFactory);

private:
	CryptoFactory(const CryptoFactory&);
	CryptoFactory& operator=(const CryptoFactory&);
};


/****************************************************************************
 *
 * CryptoUtil
 *
 */

template<class T>
class CryptoUtil
{
public:
	static QSTATUS getInstance(std::auto_ptr<T>* pap);
	static QSTATUS getInstance(const WCHAR* pwszName, std::auto_ptr<T>* pap);
};

}

#include "qscrypto.inl"

#endif // __QSCRYPTO_H__
