/*
 * $Id$
 *
 * Copyright(C) 1998-2004 Satoshi Nakamura
 * All rights reserved.
 *
 */

#ifndef __DRIVER_H__
#define __DRIVER_H__

#include <qsstring.h>

#include <vector>


namespace qmpgp {

class Driver;


/****************************************************************************
 *
 * Driver
 *
 */

class Driver
{
public:
	enum SignFlag {
		SIGNFLAG_NONE,
		SIGNFLAG_CLEARTEXT,
		SIGNFLAG_DETACH
	};

public:
	typedef std::vector<qs::WSTRING> UserIdList;

public:
	virtual ~Driver();

public:
	virtual qs::xstring_size_ptr sign(const CHAR* pszText,
									  SignFlag signFlag,
									  const WCHAR* pwszUserId,
									  const WCHAR* pwszPassphrase) const = 0;
	virtual qs::xstring_size_ptr encrypt(const CHAR* pszText,
										 const UserIdList& listRecipient) const = 0;
	virtual qs::xstring_size_ptr signAndEncrypt(const CHAR* pszText,
												const WCHAR* pwszUserId,
												const WCHAR* pwszPassphrase,
												const UserIdList& listRecipient) const = 0;
	virtual bool verify(const CHAR* pszContent,
						const CHAR* pszSignature,
						qs::wstring_ptr* pwstrUserId) const = 0;
	virtual qs::xstring_size_ptr decryptAndVerify(const CHAR* pszContent,
												  const WCHAR* pwszPassphrase,
												  unsigned int* pnVerify,
												  qs::wstring_ptr* pwstrUserId) const = 0;
	virtual bool getAlternatives(const WCHAR* pwszUserId,
								 UserIdList* pList) const = 0;
};

}

#endif // __DRIVER_H__
