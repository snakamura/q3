/*
 * $Id$
 *
 * Copyright(C) 1998-2006 Satoshi Nakamura
 * All rights reserved.
 *
 */

#ifndef __QMSECURITY_H__
#define __QMSECURITY_H__

#include <qm.h>

#include <qs.h>
#include <qscrypto.h>
#include <qsprofile.h>


namespace qm {

class Security;

class PGPUtility;


/****************************************************************************
 *
 * Security
 *
 */

class QMEXPORTCLASS Security
{
public:
	Security(const WCHAR* pwszPath,
			 qs::Profile* pProfile);
	~Security();

public:
	const qs::Store* getCA() const;
	const qs::SMIMEUtility* getSMIMEUtility() const;
	std::auto_ptr<qs::Certificate> getCertificate(const WCHAR* pwszName) const;

public:
	const PGPUtility* getPGPUtility() const;

public:
	void reload();

public:
	static void init();
	static void term();
	static bool isSSLEnabled();
	static bool isSMIMEEnabled();
	static bool isPGPEnabled();

private:
	Security(const Security&);
	Security& operator=(const Security&);

private:
	struct SecurityImpl* pImpl_;
};


/****************************************************************************
 *
 * SecurityMode
 *
 */

enum SecurityMode {
	SECURITYMODE_NONE	= 0x00,
	SECURITYMODE_SMIME	= 0x01,
	SECURITYMODE_PGP	= 0x02
};

}

#endif // __QMSECURITY_H__
