/*
 * $Id$
 *
 * Copyright(C) 1998-2004 Satoshi Nakamura
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
	explicit Security(const WCHAR* pwszPath);
	~Security();

public:
	const qs::Store* getCA() const;
	const qs::SMIMEUtility* getSMIMEUtility() const;
	std::auto_ptr<qs::Certificate> getCertificate(const WCHAR* pwszName) const;

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
