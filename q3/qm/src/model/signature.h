/*
 * $Id$
 *
 * Copyright(C) 1998-2004 Satoshi Nakamura
 * All rights reserved.
 *
 */

#ifndef __SIGNATURE_H__
#define __SIGNATURE_H__

#include <qm.h>

#include <qs.h>
#include <qsregex.h>
#include <qssax.h>
#include <qsstring.h>

#include <vector>


namespace qm {

class SignatureManager;
class Signature;
class SignatureContentHandler;

class Account;


/****************************************************************************
 *
 * SignatureManager
 *
 */

class SignatureManager
{
public:
	typedef std::vector<Signature*> SignatureList;

public:
	SignatureManager();
	~SignatureManager();

public:
	void getSignatures(Account* pAccount,
					   SignatureList* pList);
	const Signature* getSignature(Account* pAccount,
								  const WCHAR* pwszName);
	const Signature* getDefaultSignature(Account* pAccount);

public:
	void addSignature(std::auto_ptr<Signature> pSignature);

private:
	bool load();
	void clear();

private:
	SignatureManager(const SignatureManager&);
	SignatureManager& operator=(const SignatureManager&);

private:
	FILETIME ft_;
	SignatureList listSignature_;
};


/****************************************************************************
 *
 * Signature
 *
 */

class Signature
{
public:
	Signature(std::auto_ptr<qs::RegexPattern> pAccountName,
			  qs::wstring_ptr wstrName,
			  bool bDefault,
			  qs::wstring_ptr wstrSignature);
	~Signature();

public:
	bool match(Account* pAccount) const;
	const WCHAR* getName() const;
	bool isDefault() const;
	const WCHAR* getSignature() const;

private:
	Signature(const Signature&);
	Signature& operator=(const Signature&);

private:
	std::auto_ptr<qs::RegexPattern> pAccountName_;
	qs::wstring_ptr wstrName_;
	bool bDefault_;
	qs::wstring_ptr wstrSignature_;
};


/****************************************************************************
 *
 * SignatureContentHandler
 *
 */

class SignatureContentHandler : public qs::DefaultHandler
{
public:
	enum State {
		STATE_ROOT,
		STATE_SIGNATURES,
		STATE_SIGNATURE
	};

public:
	explicit SignatureContentHandler(SignatureManager* pManager);
	virtual ~SignatureContentHandler();

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
	SignatureContentHandler(const SignatureContentHandler&);
	SignatureContentHandler& operator=(const SignatureContentHandler&);

private:
	SignatureManager* pManager_;
	State state_;
	std::auto_ptr<qs::RegexPattern> pAccountName_;
	qs::wstring_ptr wstrName_;
	bool bDefault_;
	qs::StringBuffer<qs::WSTRING> buffer_;
};

}

#endif // __SIGNATURE_H__
