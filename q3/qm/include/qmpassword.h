/*
 * $Id$
 *
 * Copyright(C) 1998-2004 Satoshi Nakamura
 * All rights reserved.
 *
 */

#ifndef __QMPASSWORD_H__
#define __QMPASSWORD_H__

#include <qm.h>
#include <qmaccount.h>

#include <qsstring.h>

#include <vector>


namespace qm {

class PasswordManager;
class PasswordManagerCallback;
class PasswordVisitor;
	class PasswordCondition;
		class AccountPasswordCondition;
		class PGPPasswordCondition;

class Password;
class AccountPassword;
class PGPPassword;


/****************************************************************************
 *
 * PasswordState
 *
 */

enum PasswordState {
	PASSWORDSTATE_NONE,
	PASSWORDSTATE_ONETIME,
	PASSWORDSTATE_SESSION,
	PASSWORDSTATE_SAVE
};


/****************************************************************************
 *
 * PasswordManager
 *
 */

class QMEXPORTCLASS PasswordManager
{
public:
	typedef std::vector<Password*> PasswordList;

public:
	explicit PasswordManager(PasswordManagerCallback* pCallback);
	~PasswordManager();

public:
	qs::wstring_ptr getPassword(const PasswordCondition& condition,
								bool bPermanentOnly,
								PasswordState* pState) const;
	void setPassword(const PasswordCondition& condition,
					 const WCHAR* pwszPassword,
					 bool bPermanent);
	void removePassword(const PasswordCondition& condition);
	bool save() const;

public:
	const PasswordList& getPasswords() const;

private:
	PasswordManager(const PasswordManager&);
	PasswordManager& operator=(const PasswordManager&);

private:
	struct PasswordManagerImpl* pImpl_;
};


/****************************************************************************
 *
 * PasswordManagerCallback
 *
 */

class PasswordManagerCallback
{
public:
	virtual ~PasswordManagerCallback();

public:
	virtual PasswordState getPassword(const PasswordCondition& condition,
									  qs::wstring_ptr* pwstrPassword) = 0;
};


/****************************************************************************
 *
 * PasswordVisitor
 *
 */

class QMEXPORTCLASS PasswordVisitor
{
public:
	virtual ~PasswordVisitor();

public:
	virtual bool visit(const AccountPassword& password) const = 0;
	virtual bool visit(const PGPPassword& password) const = 0;
};


/****************************************************************************
 *
 * PasswordCondition
 *
 */

class QMEXPORTCLASS PasswordCondition : public PasswordVisitor
{
public:
	virtual ~PasswordCondition();

public:
	virtual std::auto_ptr<Password> createPassword(const WCHAR* pwszPassword,
												   bool bPermanent) const = 0;
	virtual qs::wstring_ptr getHint() const = 0;

public:
	virtual bool visit(const AccountPassword& password) const;
	virtual bool visit(const PGPPassword& password) const;
};


/****************************************************************************
 *
 * AccountPasswordCondition
 *
 */

class QMEXPORTCLASS AccountPasswordCondition : public PasswordCondition
{
public:
	AccountPasswordCondition(Account* pAccount,
							 SubAccount* pSubAccount,
							 Account::Host host);
	virtual ~AccountPasswordCondition();

public:
	virtual std::auto_ptr<Password> createPassword(const WCHAR* pwszPassword,
												   bool bPermanent) const;
	virtual qs::wstring_ptr getHint() const;

public:
	virtual bool visit(const AccountPassword& password) const;

private:
	AccountPasswordCondition(const AccountPasswordCondition&);
	AccountPasswordCondition& operator=(const AccountPasswordCondition&);

private:
	Account* pAccount_;
	SubAccount* pSubAccount_;
	Account::Host host_;
};


/****************************************************************************
 *
 * PGPPasswordCondition
 *
 */

class QMEXPORTCLASS PGPPasswordCondition : public PasswordCondition
{
public:
	explicit PGPPasswordCondition(const WCHAR* pwszUserId);
	virtual ~PGPPasswordCondition();

public:
	virtual std::auto_ptr<Password> createPassword(const WCHAR* pwszPassword,
												   bool bPermanent) const;
	virtual qs::wstring_ptr getHint() const;

public:
	virtual bool visit(const PGPPassword& password) const;

private:
	PGPPasswordCondition(const PGPPasswordCondition&);
	PGPPasswordCondition& operator=(const PGPPasswordCondition&);

private:
	const WCHAR* pwszUserId_;
};

}

#endif // __QMPASSWORD_H__
