/*
 * $Id$
 *
 * Copyright(C) 1998-2006 Satoshi Nakamura
 * All rights reserved.
 *
 */

#ifndef __PASSWORD_H__
#define __PASSWORD_H__

#include <qmpassword.h>

#include <qssax.h>
#include <qsstream.h>


namespace qm {

class Password;
	class AccountPassword;
	class PGPPassword;
class PasswordContentHandler;
class PasswordWriter;


/****************************************************************************
 *
 * Password
 *
 */

class Password
{
protected:
	Password(const WCHAR* pwszPassword,
			 bool bPermanent);

public:
	virtual ~Password();

public:
	const WCHAR* getPassword() const;
	bool isPermanent() const;
	void set(const WCHAR* pwszPassword,
			 bool bPermanent);

public:
	virtual bool visit(const PasswordVisitor& visitor) const = 0;

private:
	Password(const Password&);
	Password& operator=(const Password&);

private:
	qs::wstring_ptr wstrPassword_;
	bool bPermanent_;
};


/****************************************************************************
 *
 * AccountPassword
 *
 */

class AccountPassword : public Password
{
public:
	AccountPassword(const WCHAR* pwszAccount,
					const WCHAR* pwszSubAccount,
					Account::Host host,
					const WCHAR* pwszPassword,
					bool bPermanent);
	virtual ~AccountPassword();

public:
	const WCHAR* getAccount() const;
	const WCHAR* getSubAccount() const;
	Account::Host getHost() const;

public:
	virtual bool visit(const PasswordVisitor& visitor) const;

private:
	AccountPassword(const AccountPassword&);
	AccountPassword& operator=(const AccountPassword&);

private:
	qs::wstring_ptr wstrAccount_;
	qs::wstring_ptr wstrSubAccount_;
	Account::Host host_;
};


/****************************************************************************
 *
 * FilePassword
 *
 */

class FilePassword : public Password
{
public:
	FilePassword(const WCHAR* pwszPath,
				 const WCHAR* pwszPassword,
				 bool bPermanent);
	virtual ~FilePassword();

public:
	const WCHAR* getPath() const;

public:
	virtual bool visit(const PasswordVisitor& visitor) const;

private:
	FilePassword(const FilePassword&);
	FilePassword& operator=(const FilePassword&);

private:
	qs::wstring_ptr wstrPath_;
};


/****************************************************************************
 *
 * PGPPassword
 *
 */

class PGPPassword : public Password
{
public:
	PGPPassword(const WCHAR* pwszUserId,
				const WCHAR* pwszPassword,
				bool bPermanent);
	virtual ~PGPPassword();

public:
	const WCHAR* getUserId() const;

public:
	virtual bool visit(const PasswordVisitor& visitor) const;

private:
	PGPPassword(const PGPPassword&);
	PGPPassword& operator=(const PGPPassword&);

private:
	qs::wstring_ptr wstrUserId_;
};


/****************************************************************************
 *
 * PasswordContentHandler
 *
 */

class PasswordContentHandler : public qs::DefaultHandler
{
public:
	explicit PasswordContentHandler(PasswordManager::PasswordList* pList);
	virtual ~PasswordContentHandler();

public:
	virtual bool startElement(const WCHAR* pwszNamespaceURI,
							  const WCHAR* pwszLocalName,
							  const WCHAR* pwszQName,
							  const qs::Attributes& attributes);
	virtual bool endElement(const WCHAR* pwszNamespaceURI,
							const WCHAR* pwszLocalName,
							const WCHAR* pwszQName);
	virtual bool characters(const WCHAR* pwsz,
							size_t nStart,
							size_t nLength);

private:
	PasswordContentHandler(const PasswordContentHandler&);
	PasswordContentHandler& operator=(const PasswordContentHandler&);

private:
	enum State {
		STATE_ROOT,
		STATE_PASSWORDS,
		STATE_CONDITION,
		STATE_PASSWORD
	};

private:
	PasswordManager::PasswordList* pList_;
	State state_;
	std::auto_ptr<Password> pPassword_;
	qs::StringBuffer<qs::WSTRING> buffer_;
};


/****************************************************************************
 *
 * PasswordWriter
 *
 */

class PasswordWriter
{
public:
	explicit PasswordWriter(qs::Writer* pWriter);
	~PasswordWriter();

public:
	bool write(const PasswordManager* pManager);

public:
	bool write(const Password& password,
			   const WCHAR* pwszName,
			   const qs::Attributes& attrs);

private:
	PasswordWriter(const PasswordWriter&);
	PasswordWriter& operator=(const PasswordWriter&);

private:
	qs::OutputHandler handler_;
};

}

#endif // __PASSWORD_H__
