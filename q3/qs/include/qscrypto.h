/*
 * $Id$
 *
 * Copyright(C) 1998-2004 Satoshi Nakamura
 * All rights reserved.
 *
 */

#ifndef __QSCRYPTO_H__
#define __QSCRYPTO_H__

#include <qs.h>
#include <qsstring.h>

#include <memory>


namespace qs {

class Name;
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
 * Name
 *
 */

class QSEXPORTCLASS Name
{
public:
	virtual ~Name();

public:
	/**
	 * Get common name.
	 *
	 * @return Common name. null if failed.
	 * @exception std::bad_alloc Out of memory.
	 */
	virtual wstring_ptr getCommonName() const = 0;
};


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
	/**
	 * Load certificate from the specified file.
	 *
	 * @param pwszPath [in] Path to the file.
	 * @param type [in] Type.
	 * @param pCallback [in] Callback which suplies password.
	 * @return true if success, false otherwise.
	 * @exception std::bad_alloc Out of memory.
	 */
	virtual bool load(const WCHAR* pwszPath,
					  FileType type,
					  PasswordCallback* pCallback) = 0;
	
	/**
	 * Load certificate from the specified stream.
	 *
	 * @param pStream [in] Stream.
	 * @param type [in] Type.
	 * @return true if success, false otherwise.
	 * @exception std::bad_alloc Out of memory.
	 */
	virtual bool load(InputStream* pStream,
					  FileType type,
					  PasswordCallback* pCallback) = 0;
	
	/**
	 * Serialize this certificate to text.
	 *
	 * @return Serialized certificate. null if error occured.
	 * @exception std::bad_alloc Out of memory.
	 */
	virtual wstring_ptr getText() const = 0;
	
	/**
	 * Get the subject of this certificate.
	 *
	 * @return Subject. null if error occured.
	 * @exception std::bad_alloc Out of memory.
	 */
	virtual std::auto_ptr<Name> getSubject() const = 0;
	
	/**
	 * Get the issuer of this certificate.
	 *
	 * @return Issuer.
	 * @exception std::bad_alloc Out of memory.
	 */
	virtual std::auto_ptr<Name> getIssuer() const = 0;

public:
	/**
	 * Get instance of new certificate.
	 *
	 * @return Certificate. null if error occured.
	 * @exception std::bad_alloc Out of memory.
	 */
	static std::auto_ptr<Certificate> getInstance();
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
	/**
	 * Load private key from the specified file.
	 *
	 * @param pwszPath [in] Path to the file.
	 * @param type [in] Type.
	 * @param pCallback [in] Callback which suplies password.
	 * @return true if success, false otherwise.
	 * @exception std::bad_alloc Out of memory.
	 */
	virtual bool load(const WCHAR* pwszPath,
					  FileType type,
					  PasswordCallback* pCallback) = 0;
	
	/**
	 * Load private key from the specified stream.
	 *
	 * @param pStream [in] Stream.
	 * @param type [in] Type.
	 * @param pCallback [in] Callback which suplies password.
	 * @return true if success, false otherwise.
	 * @exception std::bad_alloc Out of memory.
	 */
	virtual bool load(InputStream* pStream,
					  FileType type,
					  PasswordCallback* pCallback) = 0;

public:
	/**
	 * Get instance of new private key.
	 *
	 * @return Private key.
	 * @exception std::bad_alloc Out of memory.
	 */
	static std::auto_ptr<PrivateKey> getInstance();
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
	/**
	 * Load public key from the specified file.
	 *
	 * @param pwszPath [in] Path to the file.
	 * @param type [in] Type.
	 * @param pCallback [in] Callback which suplies password.
	 * @return true if success, false otherwise.
	 * @exception std::bad_alloc Out of memory.
	 */
	virtual bool load(const WCHAR* pwszPath,
					  FileType type,
					  PasswordCallback* pCallback) = 0;
	
	/**
	 * Load public key from the specified stream.
	 *
	 * @param pStream [in] Stream.
	 * @param type [in] Type.
	 * @param pCallback [in] Callback which suplies password.
	 * @return true if success, false otherwise.
	 * @exception std::bad_alloc Out of memory.
	 */
	virtual bool load(InputStream* pStream,
					  FileType type,
					  PasswordCallback* pCallback) = 0;

public:
	/**
	 * Get instance of new public key.
	 *
	 * @return Public key.
	 * @exception std::bad_alloc Out of memory.
	 */
	static std::auto_ptr<PublicKey> getInstance();
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
	/**
	 * Get password.
	 *
	 * @return Password. null if there is no password.
	 * @exception std::bad_alloc Out of memory.
	 */
	virtual wstring_ptr getPassword() = 0;
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
	/**
	 * Load certificate store from the specified file.
	 *
	 * @param pwszPath [in] Path to the file.
	 * @param type [in] Type.
	 * @return true if success, false otherwise.
	 * @exception std::bad_alloc Out of memory.
	 */
	virtual bool load(const WCHAR* pwszPath,
					  FileType type) = 0;

public:
	/**
	 * Get instance of new certificate store.
	 *
	 * @return Certificate store.
	 * @exception std::bad_alloc Out of memory.
	 */
	static std::auto_ptr<Store> getInstance();
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
	/**
	 * Get instance of new cipher.
	 *
	 * @param pwszName [in] Name of the cipher.
	 * @return Cipher.
	 * @exception std::bad_alloc Out of memory.
	 */
	static std::auto_ptr<Cipher> getInstance(const WCHAR* pwszName);
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
	/**
	 * Get type of the specified part.
	 *
	 * @param part [in] Part.
	 * @return Type.
	 * @exception std::bad_alloc Out of memory.
	 */
	virtual Type getType(const Part& part) const = 0;
	
	/**
	 * Sign the specified part.
	 *
	 * @param pPart [in] Part to be signed. This part will be modified.
	 * @param bMultipart [in] true if sign as multipart/signed, false otherwise.
	 * @param pPrivateKey [in] Private key to sign.
	 * @param pCertificate [in] Certificate collesponding to the private key.
	 * @param pCallback [in] Callback.
	 * @return Signed content. null if error occured.
	 * @exception std::bad_alloc Out of memory.
	 */
	virtual xstring_ptr sign(Part* pPart,
							 bool bMultipart,
							 const PrivateKey* pPrivateKey,
							 const Certificate* pCertificate,
							 SMIMECallback* pCallback) const = 0;
	
	/**
	 * Verify the specified part.
	 *
	 * @param part [in] Part to be verified.
	 * @param pStoreCA [in] Certificate store of CA.
	 * @return Verified content. null if error occured.
	 * @exception std::bad_alloc Out of memory.
	 */
	virtual xstring_ptr verify(const Part& part,
							   const Store* pStoreCA) const = 0;
	
	/**
	 * Encrypt the specified part.
	 *
	 * @param pPart [in] Part to be signed. This part will be modified.
	 * @param pCipher [in] Cipher to encrypt.
	 * @param pCallback [in] Callback.
	 * @return Encrypted content. null if error occured.
	 * @exception std::bad_alloc Out of memory.
	 */
	virtual xstring_ptr encrypt(Part* pPart,
								const Cipher* pCipher,
								SMIMECallback* pCallback) const = 0;
	
	/**
	 * Decrypt the specified part.
	 *
	 * @param part [in] Part to be decrypted.
	 * @param pPrivateKey [in] Private key to decrypt.
	 * @param pCertificate [in] Certificate collesponding to the private key.
	 * @return Decrypted content. null if error occured.
	 * @exception std::bad_alloc Out of memory.
	 */
	virtual xstring_ptr decrypt(const Part& part,
								const PrivateKey* pPrivateKey,
								const Certificate* pCertificate) const = 0;

public:
	/**
	 * Get new S/MIME utility.
	 *
	 * @return S/MIME utility.
	 * @exception std::bad_alloc Out of memory.
	 */
	static std::auto_ptr<SMIMEUtility> getInstance();
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
	/**
	 * Get content of the specified part.
	 * Bcc and other unneeded fields will be removed.
	 *
	 * @param pPart [in] Part. This part will be modified.
	 * @return Content. null if error occured.
	 */
	virtual xstring_ptr getContent(Part* pPart) = 0;
	
	/**
	 * Get the certificate of the specified address.
	 *
	 * @param pwszAddress Address.
	 * @return Certificate. null if not found.
	 * @exception std::bad_alloc Out of memory.
	 */
	virtual std::auto_ptr<Certificate> getCertificate(const WCHAR* pwszAddress) = 0;
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
	/**
	 * Create new certificate.
	 *
	 * @return Certificate.
	 * @exception std::bad_alloc Out of memory.
	 */
	virtual std::auto_ptr<Certificate> createCertificate() = 0;
	
	/**
	 * Create new private key.
	 *
	 * @return Private key.
	 * @exception std::bad_alloc Out of memory.
	 */
	virtual std::auto_ptr<PrivateKey> createPrivateKey() = 0;
	
	/**
	 * Create new public key.
	 *
	 * @return Public Key.
	 * @exception std::bad_alloc Out of memory.
	 */
	virtual std::auto_ptr<PublicKey> createPublicKey() = 0;
	
	/**
	 * Create new certificate store.
	 *
	 * @return Certificate store.
	 * @exception std::bad_alloc Out of memory.
	 */
	virtual std::auto_ptr<Store> createStore() = 0;
	
	/**
	 * Create new cipher.
	 *
	 * @return Cipher.
	 * @exception std::bad_alloc Out of memory.
	 */
	virtual std::auto_ptr<Cipher> createCipher(const WCHAR* pwszName) = 0;
	
	/**
	 * Create new S/MIME utility.
	 *
	 * @return S/MIME utility.
	 * @exception std::bad_alloc Out of memory.
	 */
	virtual std::auto_ptr<SMIMEUtility> createSMIMEUtility() = 0;

public:
	/**
	 * Get factory.
	 *
	 * @return Factory. null if no factory is registered.
	 */
	static CryptoFactory* getFactory();

protected:
	/**
	 * Register factory.
	 *
	 * @param pFactory Factory.
	 */
	static void registerFactory(CryptoFactory* pFactory);
	
	/**
	 * Unregister factory.
	 *
	 * @param pFactory Factory.
	 */
	static void unregisterFactory(CryptoFactory* pFactory);

private:
	CryptoFactory(const CryptoFactory&);
	CryptoFactory& operator=(const CryptoFactory&);
};

}

#include "qscrypto.inl"

#endif // __QSCRYPTO_H__
