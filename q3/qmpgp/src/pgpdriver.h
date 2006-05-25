/*
 * $Id$
 *
 * Copyright(C) 1998-2006 Satoshi Nakamura
 * All rights reserved.
 *
 */

#ifndef __PGPDRIVER_H__
#define __PGPDRIVER_H__

#include <qsprofile.h>

#include "driver.h"


namespace qmpgp {

class PGPDriver;


/****************************************************************************
 *
 * PGPDriver
 *
 */

class PGPDriver : public Driver
{
public:
	explicit PGPDriver(qs::Profile* pProfile);
	virtual ~PGPDriver();

public:
	virtual qs::xstring_size_ptr sign(const CHAR* pszText,
									  size_t nLen,
									  SignFlag signFlag,
									  const WCHAR* pwszUserId,
									  const WCHAR* pwszPassphrase) const;
	virtual qs::xstring_size_ptr encrypt(const CHAR* pszText,
										 size_t nLen,
										 const UserIdList& listRecipient) const;
	virtual qs::xstring_size_ptr signAndEncrypt(const CHAR* pszText,
												size_t nLen,
												const WCHAR* pwszUserId,
												const WCHAR* pwszPassphrase,
												const UserIdList& listRecipient) const;
	virtual bool verify(const CHAR* pszContent,
						size_t nLen,
						const CHAR* pszSignature,
						const qs::AddressListParser* pFrom,
						const qs::AddressListParser* pSender,
						unsigned int* pnVerify,
						qs::wstring_ptr* pwstrUserId,
						qs::wstring_ptr* pwstrInfo) const;
	virtual qs::xstring_size_ptr decryptAndVerify(const CHAR* pszContent,
												  size_t nLen,
												  const WCHAR* pwszPassphrase,
												  const qs::AddressListParser* pFrom,
												  const qs::AddressListParser* pSender,
												  unsigned int* pnVerify,
												  qs::wstring_ptr* pwstrUserId,
												  qs::wstring_ptr* pwstrInfo) const;

private:
	qs::wstring_ptr getCommand() const;

private:
	static unsigned int checkVerified(const unsigned char* pBuf,
									  size_t nLen,
									  qs::wstring_ptr* pwstrUserId);

private:
	PGPDriver(const PGPDriver&);
	PGPDriver& operator=(const PGPDriver&);

private:
	qs::Profile* pProfile_;
};

}

#endif // __PGPDRIVER_H__
