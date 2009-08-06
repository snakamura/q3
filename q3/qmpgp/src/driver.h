/*
 * $Id$
 *
 * Copyright(C) 1998-2008 Satoshi Nakamura
 *
 */

#ifndef __DRIVER_H__
#define __DRIVER_H__

#include <qsstring.h>

#include <qmpgp.h>

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

	enum HashAlgorithm {
		HASHALGORITHM_NONE = -1,
		HASHALGORITHM_MD5,
		HASHALGORITHM_SHA1,
		HASHALGORITHM_RMD160,
		HASHALGORITHM_SHA256,
		HASHALGORITHM_SHA384,
		HASHALGORITHM_SHA512,
		HASHALGORITHM_SHA224
	};

public:
	typedef std::vector<qs::WSTRING> UserIdList;

public:
	virtual ~Driver();

public:
	virtual qs::xstring_size_ptr sign(const CHAR* pszText,
									  size_t nLen,
									  SignFlag signFlag,
									  const WCHAR* pwszUserId,
									  qm::PGPPassphraseCallback* pPassphraseCallback,
									  HashAlgorithm* pHashAlgorithm) const = 0;
	virtual qs::xstring_size_ptr encrypt(const CHAR* pszText,
										 size_t nLen,
										 const UserIdList& listRecipient,
										 const UserIdList& listHiddenRecipient) const = 0;
	virtual qs::xstring_size_ptr signAndEncrypt(const CHAR* pszText,
												size_t nLen,
												const WCHAR* pwszUserId,
												qm::PGPPassphraseCallback* pPassphraseCallback,
												const UserIdList& listRecipient,
												const UserIdList& listHiddenRecipient,
												HashAlgorithm* pHashAlgorithm) const = 0;
	virtual bool verify(const CHAR* pszContent,
						size_t nLen,
						const CHAR* pszSignature,
						const qs::AddressListParser* pFrom,
						const qs::AddressListParser* pSender,
						unsigned int* pnVerify,
						qs::wstring_ptr* pwstrUserId,
						qs::wstring_ptr* pwstrInfo) const = 0;
	virtual qs::xstring_size_ptr decryptAndVerify(const CHAR* pszContent,
												  size_t nLen,
												  qm::PGPPassphraseCallback* pPassphraseCallback,
												  const qs::AddressListParser* pFrom,
												  const qs::AddressListParser* pSender,
												  unsigned int* pnVerify,
												  qs::wstring_ptr* pwstrUserId,
												  qs::wstring_ptr* pwstrInfo) const = 0;
};

}

#endif // __DRIVER_H__
