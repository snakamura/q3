/*
 * $Id$
 *
 * Copyright(C) 1998-2004 Satoshi Nakamura
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
class Security;
class SubAccount;


/****************************************************************************
 *
 * MessageComposer
 *
 */

class MessageComposer
{
public:
	enum Flag {
		FLAG_SIGN		= 0x01,
		FLAG_ENCRYPT	= 0x02
	};

public:
	MessageComposer(bool bDraft,
					Document* pDocument,
					qs::Profile* pProfile,
					HWND hwnd,
					FolderModel* pFolderModel);
	~MessageComposer();

public:
	bool compose(Account* pAccount,
				 SubAccount* pSubAccount,
				 Message* pMessage,
				 unsigned int nFlags) const;

private:
	MessageComposer(const MessageComposer&);
	MessageComposer& operator=(const MessageComposer&);

private:
	bool bDraft_;
	Document* pDocument_;
	qs::Profile* pProfile_;
	HWND hwnd_;
	FolderModel* pFolderModel_;
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
					  AddressBook* pAddressBook);
	~SMIMECallbackImpl();

public:
	virtual qs::xstring_ptr getContent(qs::Part* pPart);
	virtual std::auto_ptr<qs::Certificate> getCertificate(const WCHAR* pwszAddress);

private:
	SMIMECallbackImpl(const SMIMECallbackImpl&);
	SMIMECallbackImpl& operator=(const SMIMECallbackImpl&);

private:
	const Security* pSecurity_;
	AddressBook* pAddressBook_;
};

}

#endif // __MESSAGECOMPOSER_H__
