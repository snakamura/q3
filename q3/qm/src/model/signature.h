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
	const SignatureList& getSignatures();
	const SignatureList& getSignatures(bool bRealod);
	void setSignatures(SignatureList& listSignature);
	void getSignatures(Account* pAccount,
					   SignatureList* pList);
	const Signature* getSignature(Account* pAccount,
								  const WCHAR* pwszName);
	const Signature* getDefaultSignature(Account* pAccount);
	bool save() const;

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
	Signature();
	Signature(const WCHAR* pwszAccount,
			  std::auto_ptr<qs::RegexPattern> pAccount,
			  const WCHAR* pwszName,
			  bool bDefault,
			  const WCHAR* pwszSignature);
	Signature(const Signature& signature);
	~Signature();

public:
	const WCHAR* getAccount() const;
	void setAccount(const WCHAR* pwszAccount,
					std::auto_ptr<qs::RegexPattern> pAccount);
	bool match(Account* pAccount) const;
	const WCHAR* getName() const;
	void setName(const WCHAR* pwszName);
	bool isDefault() const;
	void setDefault(bool bDefault);
	const WCHAR* getSignature() const;
	void setSignature(const WCHAR* pwszSignature);

private:
	Signature& operator=(const Signature&);

private:
	qs::wstring_ptr wstrAccount_;
	std::auto_ptr<qs::RegexPattern> pAccount_;
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
	qs::wstring_ptr wstrAccount_;
	std::auto_ptr<qs::RegexPattern> pAccount_;
	qs::wstring_ptr wstrName_;
	bool bDefault_;
	qs::StringBuffer<qs::WSTRING> buffer_;
};


/****************************************************************************
 *
 * SignatureWriter
 *
 */

class SignatureWriter
{
public:
	explicit SignatureWriter(qs::Writer* pWriter);
	~SignatureWriter();

public:
	bool write(const SignatureManager* pManager);

private:
	bool write(const Signature* pSignature);

private:
	SignatureWriter(const SignatureWriter&);
	SignatureWriter& operator=(const SignatureWriter&);

private:
	qs::OutputHandler handler_;
};

}

#endif // __SIGNATURE_H__
