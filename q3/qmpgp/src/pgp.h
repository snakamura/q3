/*
 * $Id$
 *
 * Copyright(C) 1998-2004 Satoshi Nakamura
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
	virtual qs::xstring_ptr sign(qs::Part* pPart,
								 bool bMime,
								 const WCHAR* pwszUserId,
								 const WCHAR* pwszPassphrase) const;
	virtual qs::xstring_ptr encrypt(qs::Part* pPart,
									bool bMime) const;
	virtual qs::xstring_ptr signAndEncrypt(qs::Part* pPart,
										   bool bMime,
										   const WCHAR* pwszUserId,
										   const WCHAR* pwszPassphrase) const;
	virtual qs::xstring_ptr verify(const qs::Part& part,
								   bool bMime,
								   unsigned int* pnVerify) const;
	virtual qs::xstring_ptr decryptAndVerify(const qs::Part& part,
											 bool bMime,
											 const WCHAR* pwszPassphrase,
											 unsigned int* pnVerify) const;

private:
	static void getRecipients(const qs::Part& part,
							  Driver::UserIdList* pListUserId);
	static bool checkUserId(const qs::Part& part,
							const WCHAR* pwszUserId);
	static bool contains(const qs::AddressListParser& addressList,
						 const WCHAR* pwszAddress);
	static bool contains(const qs::AddressParser& address,
						 const WCHAR* pwszAddress);
	static qs::xstring_ptr createMessage(const CHAR* pszHeader,
										 const CHAR* pszBody,
										 const WCHAR* pwszSignedBy);
	static qs::xstring_ptr createMessage(const CHAR* pszContent,
										 const qs::Part& part,
										 const WCHAR* pwszSignedBy);
	static qs::xstring_ptr createMultipartSignedMessage(const CHAR* pszHeader,
														const qs::Part& part,
														const CHAR* pszSignature);
	static qs::xstring_ptr createMultipartEncryptedMessage(const CHAR* pszHeader,
														   const CHAR* pszBody);

private:
	PGPUtilityImpl(const PGPUtilityImpl&);
	PGPUtilityImpl& operator=(const PGPUtilityImpl&);

private:
	qs::Profile* pProfile_;
	std::auto_ptr<Driver> pDriver_;
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
