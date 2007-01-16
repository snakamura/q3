/*
 * $Id$
 *
 * Copyright(C) 1998-2007 Satoshi Nakamura
 * All rights reserved.
 *
 */

#ifndef __SECURITY_H__
#define __SECURITY_H__

#include <qm.h>
#include <qmpassword.h>

#include <qscrypto.h>


namespace qm {

class FileCryptoPasswordCallback;


/****************************************************************************
 *
 * FileCryptoPasswordCallback
 *
 */

class FileCryptoPasswordCallback : public qs::CryptoPasswordCallback
{
public:
	FileCryptoPasswordCallback(PasswordManager* pPasswordManager,
							   const WCHAR* pwszPath);
	virtual ~FileCryptoPasswordCallback();

public:
	void save();
	
public:
	virtual qs::wstring_ptr getPassword();

private:
	PasswordManager* pPasswordManager_;
	const WCHAR* pwszPath_;
	PasswordState state_;
	qs::wstring_ptr wstrPassword_;
};

}

#endif // __SECURITY_H__
