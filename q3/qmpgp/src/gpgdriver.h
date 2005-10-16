/*
 * $Id$
 *
 * Copyright(C) 1998-2005 Satoshi Nakamura
 * All rights reserved.
 *
 */

#ifndef __GPGDRIVER_H__
#define __GPGDRIVER_H__

#include <qsprofile.h>

#include "driver.h"


namespace qmpgp {

class GPGDriver;


/****************************************************************************
 *
 * GPGDriver
 *
 */

class GPGDriver : public Driver
{
public:
	explicit GPGDriver(qs::Profile* pProfile);
	virtual ~GPGDriver();

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
	unsigned int parseStatus(const unsigned char* pBuf,
							 size_t nLen,
							 const qs::AddressListParser* pFrom,
							 const qs::AddressListParser* pSender,
							 qs::wstring_ptr* pwstrUserId,
							 qs::wstring_ptr* pwstrInfo) const;
	bool getUserIdFromFingerPrint(const WCHAR* pwszFingerPrint,
								  const qs::AddressListParser* pFrom,
								  const qs::AddressListParser* pSender,
								  qs::wstring_ptr* pwstrUserId,
								  bool* pbMatch) const;

private:
	static qs::wstring_ptr getAddressFromUserId(const CHAR* pszUserId);
	static const CHAR* getToken(CHAR** pp,
								CHAR c);

private:
	GPGDriver(const GPGDriver&);
	GPGDriver& operator=(const GPGDriver&);

private:
	qs::Profile* pProfile_;
};

}

#endif // __GPGDRIVER_H__
