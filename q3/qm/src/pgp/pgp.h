/*
 * $Id$
 *
 * Copyright(C) 1998-2007 Satoshi Nakamura
 * All rights reserved.
 *
 */

#include <qmpassword.h>
#include <qmpgp.h>


namespace qm {

/****************************************************************************
 *
 * PGPPassphraseCallbackImpl
 *
 */

class PGPPassphraseCallbackImpl : public PGPPassphraseCallback
{
public:
	PGPPassphraseCallbackImpl(PasswordManager* pPasswordManager,
							  const WCHAR* pwszDefaultUserId);
	~PGPPassphraseCallbackImpl();

public:
	void save();

public:
	virtual qs::wstring_ptr getPassphrase(const WCHAR* pwszUserId);
	virtual void clear();

private:
	PGPPassphraseCallbackImpl(const PGPPassphraseCallbackImpl&);
	PGPPassphraseCallbackImpl& operator=(const PGPPassphraseCallbackImpl&);

private:
	PasswordManager* pPasswordManager_;
	const WCHAR* pwszDefaultUserId_;
	qs::wstring_ptr wstrUserId_;
	qs::wstring_ptr wstrPassword_;
	PasswordState state_;
};

}
