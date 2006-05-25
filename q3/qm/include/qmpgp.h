/*
 * $Id$
 *
 * Copyright(C) 1998-2006 Satoshi Nakamura
 * All rights reserved.
 *
 */

#ifndef __QMPGP_H__
#define __QMPGP_H__

#include <qm.h>

#include <qsmime.h>
#include <qsprofile.h>
#include <qsstring.h>

#include <memory>


namespace qm {

class PGPUtility;
class PGPFactory;


/****************************************************************************
 *
 * PGPUtility
 *
 */

class QMEXPORTCLASS PGPUtility
{
public:
	enum Type {
		TYPE_NONE,
		TYPE_MIMEENCRYPTED,
		TYPE_MIMESIGNED,
		TYPE_INLINEENCRYPTED,
		TYPE_INLINESIGNED
	};
	
	enum Verify {
		VERIFY_NONE				= 0x00,
		VERIFY_OK				= 0x01,
		VERIFY_FAILED			= 0x02,
		VERIFY_ADDRESSNOTMATCH	= 0x04
	};

public:
	virtual ~PGPUtility();

public:
	virtual Type getType(const qs::Part& part,
						 bool bCheckInline) const = 0;
	virtual Type getType(const qs::ContentTypeParser* pContentType) const = 0;
	virtual qs::xstring_size_ptr sign(qs::Part* pPart,
									  bool bMime,
									  const WCHAR* pwszUserId,
									  const WCHAR* pwszPasspharse) const = 0;
	virtual qs::xstring_size_ptr encrypt(qs::Part* pPart,
										 bool bMime) const = 0;
	virtual qs::xstring_size_ptr signAndEncrypt(qs::Part* pPart,
												bool bMime,
												const WCHAR* pwszUserId,
												const WCHAR* pwszPassphrase) const = 0;
	virtual qs::xstring_size_ptr verify(const qs::Part& part,
										bool bMime,
										unsigned int* pnVerify,
										qs::wstring_ptr* pwstrSignedBy,
										qs::wstring_ptr* pwstrInfo) const = 0;
	virtual qs::xstring_size_ptr decryptAndVerify(const qs::Part& part,
												  bool bMime,
												  const WCHAR* pwszPassphrase,
												  unsigned int* pnVerify,
												  qs::wstring_ptr* pwstrSignedBy,
												  qs::wstring_ptr* pwstrInfo) const = 0;

public:
	static std::auto_ptr<PGPUtility> getInstance(qs::Profile* pProfile);
};


/****************************************************************************
 *
 * PGPFactory
 *
 */

class QMEXPORTCLASS PGPFactory
{
protected:
	PGPFactory();

public:
	virtual ~PGPFactory();

public:
	virtual std::auto_ptr<PGPUtility> createPGPUtility(qs::Profile* pProfile) = 0;

public:
	static PGPFactory* getFactory();

protected:
	static void registerFactory(PGPFactory* pFactory);
	static void unregisterFactory(PGPFactory* pFactory);

private:
	PGPFactory(const PGPFactory&);
	PGPFactory& operator=(const PGPFactory&);
};

}

#endif // __QMPGP_H__
