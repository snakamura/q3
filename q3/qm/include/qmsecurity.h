/*
 * $Id$
 *
 * Copyright(C) 1998-2003 Satoshi Nakamura
 * All rights reserved.
 *
 */

#ifndef __QMSECURITY_H__
#define __QMSECURITY_H__

#include <qm.h>

#include <qs.h>
#include <qscrypto.h>


namespace qm {

/****************************************************************************
 *
 * Security
 *
 */

class QMEXPORTCLASS Security
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
	struct SecurityImpl* pImpl_;
};

}

#endif // __QMSECURITY_H__
