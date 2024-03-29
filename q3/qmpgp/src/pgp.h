/*
 * $Id$
 *
 * Copyright(C) 1998-2008 Satoshi Nakamura
 *
 */

#ifndef __PGP_H__
#define __PGP_H__

#include <qmpgp.h>

#include "driver.h"


namespace qmpgp {

class PGPUtilityImpl;
class PGPFactoryImpl;


/****************************************************************************
 *
 * PGPUtilityImpl
 *
 */

class PGPUtilityImpl : public qm::PGPUtility
{
public:
	explicit PGPUtilityImpl(qs::Profile* pProfile);
	virtual ~PGPUtilityImpl();

public:
	virtual Type getType(const qs::Part& part,
						 bool bCheckInline) const;
	virtual Type getType(const qs::ContentTypeParser* pContentType) const;
	virtual qs::xstring_size_ptr sign(qs::Part* pPart,
									  bool bMime,
									  const WCHAR* pwszUserId,
									  qm::PGPPassphraseCallback* pPassphraseCallback) const;
	virtual qs::xstring_size_ptr encrypt(qs::Part* pPart,
										 bool bMime) const;
	virtual qs::xstring_size_ptr signAndEncrypt(qs::Part* pPart,
												bool bMime,
												const WCHAR* pwszUserId,
												qm::PGPPassphraseCallback* pPassphraseCallback) const;
	virtual qs::xstring_size_ptr verify(const qs::Part& part,
										bool bMime,
										unsigned int* pnVerify,
										qs::wstring_ptr* pwstrSignedBy,
										qs::wstring_ptr* pwstrInfo) const;
	virtual qs::xstring_size_ptr decryptAndVerify(const qs::Part& part,
												  bool bMime,
												  qm::PGPPassphraseCallback* pPassphraseCallback,
												  unsigned int* pnVerify,
												  qs::wstring_ptr* pwstrSignedBy,
												  qs::wstring_ptr* pwstrInfo) const;

private:
	std::auto_ptr<Driver> getDriver() const;
	void getRecipients(const qs::Part& part,
					   Driver::UserIdList* pListUserId,
					   Driver::UserIdList* pListHiddenUserId) const;

private:
	static qs::xstring_size_ptr createMessage(const CHAR* pszHeader,
											  const CHAR* pszBody,
											  size_t nBodyLen);
	static qs::xstring_size_ptr createMessage(const CHAR* pszContent,
											  size_t nLen,
											  const qs::Part& part);
	static qs::xstring_size_ptr createMultipartSignedMessage(const CHAR* pszHeader,
															 const qs::Part& part,
															 const CHAR* pszSignature,
															 size_t nSignatureLen,
															 Driver::HashAlgorithm hashAlgorithm);
	static qs::xstring_size_ptr createMultipartEncryptedMessage(const CHAR* pszHeader,
																const CHAR* pszBody,
																size_t nBodyLen);
	static std::pair<const CHAR*, size_t> getBodyData(const qs::Part& part,
													  qs::malloc_size_ptr<unsigned char>* ppBodyData);
	static const CHAR* findInline(const CHAR* pszMarker,
								  const CHAR* psz,
								  size_t nLen);
	static void getUserIds(const qs::Part& part,
						   const WCHAR* pwszField,
						   Driver::UserIdList* pListUserId,
						   Driver::UserIdList* pListHiddenUserId);
	static qs::wstring_ptr formatHashAlgorithm(Driver::HashAlgorithm hashAlgorithm);

private:
	PGPUtilityImpl(const PGPUtilityImpl&);
	PGPUtilityImpl& operator=(const PGPUtilityImpl&);

private:
	qs::Profile* pProfile_;
};


/****************************************************************************
 *
 * PGPFactoryImpl
 *
 */

class PGPFactoryImpl : public qm::PGPFactory
{
public:
	PGPFactoryImpl();
	virtual ~PGPFactoryImpl();

public:
	virtual std::auto_ptr<qm::PGPUtility> createPGPUtility(qs::Profile* pProfile);

private:
	PGPFactoryImpl(const PGPFactoryImpl&);
	PGPFactoryImpl& operator=(const PGPFactoryImpl&);

private:
	static PGPFactoryImpl factory__;
};

}

#endif // __PGP_H__
