/*
 * $Id$
 *
 * Copyright(C) 1998-2005 Satoshi Nakamura
 * All rights reserved.
 *
 */

#ifndef __MESSAGECOMPOSER_H__
#define __MESSAGECOMPOSER_H__

#include <qm.h>

#include <qs.h>
#include <qscrypto.h>
#include <qsprofile.h>


namespace qm {

class MessageComposer;

class Account;
class AddressBook;
class Document;
class FolderModel;
class Message;
class MessagePtr;
class PasswordManager;
class Security;
class SecurityModel;
class SubAccount;


/****************************************************************************
 *
 * MessageComposer
 *
 */

class MessageComposer
{
public:
	MessageComposer(bool bDraft,
					Document* pDocument,
					PasswordManager* pPasswordManager,
					qs::Profile* pProfile,
					HWND hwnd,
					FolderModel* pFolderModel,
					SecurityModel* pSecurityModel);
	~MessageComposer();

public:
	bool compose(Account* pAccount,
				 SubAccount* pSubAccount,
				 Message* pMessage,
				 unsigned int nMessageSecurity,
				 MessagePtr* pptr) const;
	bool compose(Account* pAccount,
				 SubAccount* pSubAccount,
				 const WCHAR* pwszPath,
				 unsigned int nMessageSecurity) const;

private:
	bool processSMIME(Message* pMessage,
					  unsigned int nMessageSecurity,
					  SubAccount* pSubAccount) const;
	bool processPGP(Message* pMessage,
					unsigned int nMessageSecurity,
					SubAccount* pSubAccount) const;

private:
	MessageComposer(const MessageComposer&);
	MessageComposer& operator=(const MessageComposer&);

private:
	bool bDraft_;
	Document* pDocument_;
	PasswordManager* pPasswordManager_;
	qs::Profile* pProfile_;
	HWND hwnd_;
	FolderModel* pFolderModel_;
	SecurityModel* pSecurityModel_;
};


/****************************************************************************
 *
 * SMIMECallbackImpl
 *
 */

class SMIMECallbackImpl : public qs::SMIMECallback
{
public:
	SMIMECallbackImpl(const Security* pSecurity,
					  AddressBook* pAddressBook,
					  const qs::Certificate* pSelfCertificate);
	~SMIMECallbackImpl();

public:
	virtual std::auto_ptr<qs::Certificate> getCertificate(const WCHAR* pwszAddress);
	virtual const qs::Certificate* getSelfCertificate();

private:
	SMIMECallbackImpl(const SMIMECallbackImpl&);
	SMIMECallbackImpl& operator=(const SMIMECallbackImpl&);

private:
	const Security* pSecurity_;
	AddressBook* pAddressBook_;
	const qs::Certificate* pSelfCertificate_;
};

}

#endif // __MESSAGECOMPOSER_H__
