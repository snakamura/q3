/*
 * $Id$
 *
 * Copyright(C) 1998-2003 Satoshi Nakamura
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
	MessageComposer(bool bDraft, Document* pDocument,
		qs::Profile* pProfile, HWND hwnd, FolderModel* pFolderModel);
	~MessageComposer();

public:
	qs::QSTATUS compose(Account* pAccount, SubAccount* pSubAccount,
		Message* pMessage, unsigned int nFlags) const;

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
		AddressBook* pAddressBook, qs::QSTATUS* pstatus);
	~SMIMECallbackImpl();

public:
	virtual qs::QSTATUS getContent(qs::Part* pPart, qs::STRING* pstrContent);
	virtual qs::QSTATUS getCertificate(const WCHAR* pwszAddress,
		qs::Certificate** ppCertificate);

private:
	SMIMECallbackImpl(const SMIMECallbackImpl&);
	SMIMECallbackImpl& operator=(const SMIMECallbackImpl&);

private:
	const Security* pSecurity_;
	AddressBook* pAddressBook_;
};

}

#endif // __MESSAGECOMPOSER_H__
