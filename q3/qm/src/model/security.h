/*
 * $Id: security.h,v 1.1.1.1 2003/04/29 08:07:31 snakamura Exp $
 *
 * Copyright(C) 1998-2003 Satoshi Nakamura
 * All rights reserved.
 *
 */

#ifndef __SECURITY_H__
#define __SECURITY_H__

#include <qm.h>

#include <qs.h>
#include <qscrypto.h>


namespace qm {

/****************************************************************************
 *
 * Security
 *
 */

class Security
{
public:
	Security(const WCHAR* pwszPath, qs::QSTATUS* pstatus);
	~Security();

public:
	const qs::Store* getCA() const;
	const qs::SMIMEUtility* getSMIMEUtility() const;
	qs::QSTATUS getCertificate(const WCHAR* pwszName,
		qs::Certificate** ppCertificate) const;

public:
	static void init();
	static void term();
	static bool isEnabled();

private:
	Security(const Security&);
	Security& operator=(const Security&);

private:
	qs::WSTRING wstrPath_;
	qs::Store* pStoreCA_;
	qs::SMIMEUtility* pSMIMEUtility_;

private:
	static HINSTANCE hInst__;
};

}

#endif // __SECURITY_H__
