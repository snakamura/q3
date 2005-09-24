/*
 * $Id$
 *
 * Copyright(C) 1998-2005 Satoshi Nakamura
 * All rights reserved.
 *
 */

#ifndef __PGP_H__
#define __PGP_H__

#include <qmpgp.h>

#include "driver.h"


namespace qmpgp {

class PGPUtilityImpl;
class PGPFactoryImpl;

class Driver;


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
									  const WCHAR* pwszPassphrase) const;
	virtual qs::xstring_size_ptr encrypt(qs::Part* pPart,
										 bool bMime) const;
	virtual qs::xstring_size_ptr signAndEncrypt(qs::Part* pPart,
												bool bMime,
												const WCHAR* pwszUserId,
												const WCHAR* pwszPassphrase) const;
	virtual qs::xstring_size_ptr verify(const qs::Part& part,
										bool bMime,
										unsigned int* pnVerify,
										qs::wstring_ptr* pwstrSignedBy) const;
	virtual qs::xstring_size_ptr decryptAndVerify(const qs::Part& part,
												  bool bMime,
												  const WCHAR* pwszPassphrase,
												  unsigned int* pnVerify,
												  qs::wstring_ptr* pwstrSignedBy) const;

private:
	std::auto_ptr<Driver> getDriver() const;
	bool checkUserId(const qs::Part& part,
					 const WCHAR* pwszUserId) const;
	bool checkUserId(const qs::Part& part,
					 const WCHAR* pwszUserId,
					 bool bCheckAlternative) const;

private:
	static void getRecipients(const qs::Part& part,
							  Driver::UserIdList* pListUserId);
	static bool contains(const qs::AddressListParser& addressList,
						 const WCHAR* pwszAddress);
	static bool contains(const qs::AddressParser& address,
						 const WCHAR* pwszAddress);
	static qs::xstring_size_ptr createMessage(const CHAR* pszHeader,
											  const CHAR* pszBody,
											  size_t nBodyLen);
	static qs::xstring_size_ptr createMessage(const CHAR* pszContent,
											  size_t nLen,
											  const qs::Part& part);
	static qs::xstring_size_ptr createMultipartSignedMessage(const CHAR* pszHeader,
															 const qs::Part& part,
															 const CHAR* pszSignature,
															 size_t nSignatureLen);
	static qs::xstring_size_ptr createMultipartEncryptedMessage(const CHAR* pszHeader,
																const CHAR* pszBody,
																size_t nBodyLen);
	static std::pair<const CHAR*, size_t> getBodyData(const qs::Part& part,
													  qs::malloc_size_ptr<unsigned char>* ppBodyData);

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
