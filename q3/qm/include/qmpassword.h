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
class PasswordVisitor;
	class PasswordCondition;
		class AccountPasswordCondition;

class Password;
class AccountPassword;


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
	PasswordManager();
	~PasswordManager();

public:
	qs::wstring_ptr getPassword(const PasswordCondition& condition,
								bool bPermanentOnly) const;
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
 * PasswordVisitor
 *
 */

class QMEXPORTCLASS PasswordVisitor
{
public:
	virtual ~PasswordVisitor();

public:
	virtual bool visit(const AccountPassword& password) const = 0;
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
};


/****************************************************************************
 *
 * AccountPasswordCondition
 *
 */

class QMEXPORTCLASS AccountPasswordCondition : public PasswordCondition
{
public:
	AccountPasswordCondition(const WCHAR* pwszAccount,
							 const WCHAR* pwszSubAccount,
							 Account::Host host);
	virtual ~AccountPasswordCondition();

public:
	virtual std::auto_ptr<Password> createPassword(const WCHAR* pwszPassword,
												   bool bPermanent) const;
public:
	virtual bool visit(const AccountPassword& password) const;

private:
	AccountPasswordCondition(const AccountPasswordCondition&);
	AccountPasswordCondition& operator=(const AccountPasswordCondition&);

private:
	const WCHAR* pwszAccount_;
	const WCHAR* pwszSubAccount_;
	Account::Host host_;
};

}

#endif // __QMPASSWORD_H__
