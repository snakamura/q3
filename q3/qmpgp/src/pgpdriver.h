/*
 * $Id$
 *
 * Copyright(C) 1998-2004 Satoshi Nakamura
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
	virtual qs::xstring_ptr sign(const CHAR* pszText,
								 SignFlag signFlag,
								 const WCHAR* pwszUserId,
								 const WCHAR* pwszPassphrase) const;
	virtual qs::xstring_ptr encrypt(const CHAR* pszText,
									const UserIdList& listRecipient) const;
	virtual qs::xstring_ptr signAndEncrypt(const CHAR* pszText,
										   const WCHAR* pwszUserId,
										   const WCHAR* pwszPassphrase,
										   const UserIdList& listRecipient) const;
	virtual bool verify(const CHAR* pszContent,
						const CHAR* pszSignature,
						qs::wstring_ptr* pwstrUserId) const;
	virtual qs::xstring_ptr decryptAndVerify(const CHAR* pszContent,
											 const WCHAR* pwszPassphrase,
											 unsigned int* pnVerify,
											 qs::wstring_ptr* pwstrUserId) const;
	virtual bool getAlternatives(const WCHAR* pwszUserId,
								 UserIdList* pList) const;

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
