/*
 * $Id$
 *
 * Copyright(C) 1998-2003 Satoshi Nakamura
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
	explicit SignatureManager(qs::QSTATUS* pstatus);
	~SignatureManager();

public:
	qs::QSTATUS getSignatures(Account* pAccount, SignatureList* pList);
	qs::QSTATUS getSignature(Account* pAccount,
		const WCHAR* pwszName, const Signature** ppSignature);
	qs::QSTATUS getDefaultSignature(Account* pAccount,
		const Signature** ppSignature);

public:
	qs::QSTATUS addSignature(Signature* pSignature);

private:
	qs::QSTATUS load();
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
	Signature(qs::RegexPattern* pAccountName, qs::WSTRING wstrName,
		bool bDefault, qs::WSTRING wstrSignature, qs::QSTATUS* pstatus);
	~Signature();

public:
	qs::QSTATUS match(Account* pAccount, bool* pbMatch) const;
	const WCHAR* getName() const;
	bool isDefault() const;
	const WCHAR* getSignature() const;

private:
	Signature(const Signature&);
	Signature& operator=(const Signature&);

private:
	qs::RegexPattern* pAccountName_;
	qs::WSTRING wstrName_;
	bool bDefault_;
	qs::WSTRING wstrSignature_;
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
	SignatureContentHandler(SignatureManager* pManager, qs::QSTATUS* pstatus);
	virtual ~SignatureContentHandler();

public:
	virtual qs::QSTATUS startElement(const WCHAR* pwszNamespaceURI,
		const WCHAR* pwszLocalName, const WCHAR* pwszQName,
		const qs::Attributes& attributes);
	virtual qs::QSTATUS endElement(const WCHAR* pwszNamespaceURI,
		const WCHAR* pwszLocalName, const WCHAR* pwszQName);
	virtual qs::QSTATUS characters(const WCHAR* pwsz,
		size_t nStart, size_t nLength);

private:
	SignatureContentHandler(const SignatureContentHandler&);
	SignatureContentHandler& operator=(const SignatureContentHandler&);

private:
	SignatureManager* pManager_;
	State state_;
	qs::RegexPattern* pAccountName_;
	qs::WSTRING wstrName_;
	bool bDefault_;
	qs::StringBuffer<qs::WSTRING>* pBuffer_;
};

}

#endif // __SIGNATURE_H__
