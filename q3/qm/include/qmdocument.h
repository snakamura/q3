/*
 * $Id$
 *
 * Copyright(C) 1998-2003 Satoshi Nakamura
 * All rights reserved.
 *
 */

#ifndef __QMDOCUMENT_H__
#define __QMDOCUMENT_H__

#include <qm.h>

#include <qsprofile.h>
#include <qsstring.h>

#include <vector>


namespace qm {

class Document;
class DocumentHandler;
	class DefaultDocumentHandler;
class DocumentEvent;
class AccountListChangedEvent;

class Account;
class AddressBook;
class Folder;
class MessageHolder;
class RuleManager;
class ScriptManager;
class Security;
class SignatureManager;
class TemplateManager;


/****************************************************************************
 *
 * Document
 *
 */

class QMEXPORTCLASS Document
{
public:
	typedef std::vector<Account*> AccountList;

public:
	Document(qs::Profile* pProfile, qs::QSTATUS* pstatus);
	~Document();

public:
	Account* getAccount(const WCHAR* pwszName) const;
	const AccountList& getAccounts() const;
	bool hasAccount(const WCHAR* pwszName) const;
	qs::QSTATUS addAccount(Account* pAccount);
	qs::QSTATUS removeAccount(Account* pAccount);
	qs::QSTATUS renameAccount(Account* pAccount, const WCHAR* pwszName);
	qs::QSTATUS loadAccounts(const WCHAR* pwszPath);
	qs::QSTATUS getFolder(Account* pAccount,
		const WCHAR* pwszName, Folder** ppFolder) const;
	
	RuleManager* getRuleManager() const;
	const TemplateManager* getTemplateManager() const;
	ScriptManager* getScriptManager() const;
	SignatureManager* getSignatureManager() const;
	AddressBook* getAddressBook() const;
	const Security* getSecurity() const;
	
	bool isOffline() const;
	qs::QSTATUS setOffline(bool bOffline);
	bool isCheckNewMail() const;
	void setCheckNewMail(bool bCheckNewMail);
	qs::QSTATUS save();
	
	qs::QSTATUS addDocumentHandler(DocumentHandler* pHandler);
	qs::QSTATUS removeDocumentHandler(DocumentHandler* pHandler);

private:
	Document(const Document&);
	Document& operator=(const Document&);

private:
	struct DocumentImpl* pImpl_;
};


/****************************************************************************
 *
 * DocumentHandler
 *
 */

class DocumentHandler
{
public:
	virtual ~DocumentHandler();

public:
	virtual qs::QSTATUS offlineStatusChanged(const DocumentEvent& event) = 0;
	virtual qs::QSTATUS accountListChanged(const AccountListChangedEvent& event) = 0;
};


/****************************************************************************
 *
 * DefaultDocumentHandler
 *
 */

class DefaultDocumentHandler : public DocumentHandler
{
public:
	DefaultDocumentHandler();
	virtual ~DefaultDocumentHandler();

public:
	virtual qs::QSTATUS offlineStatusChanged(const DocumentEvent& event);
	virtual qs::QSTATUS accountListChanged(const AccountListChangedEvent& event);
};


/****************************************************************************
 *
 * DocumentEvent
 *
 */

class DocumentEvent
{
public:
	DocumentEvent(Document* pDocument);
	~DocumentEvent();

public:
	Document* getDocument() const;

private:
	DocumentEvent(const DocumentEvent&);
	DocumentEvent& operator=(const DocumentEvent&);

private:
	Document* pDocument_;
};


/****************************************************************************
 *
 * AccountListChangedEvent
 *
 */

class AccountListChangedEvent
{
public:
	enum Type {
		TYPE_ALL,
		TYPE_ADD,
		TYPE_REMOVE,
		TYPE_RENAME
	};

public:
	AccountListChangedEvent(Document* pDocument, Type type, Account* pAccount);
	~AccountListChangedEvent();

public:
	Type getType() const;
	Account* getAccount() const;

private:
	AccountListChangedEvent(const AccountListChangedEvent&);
	AccountListChangedEvent& operator=(const AccountListChangedEvent&);

private:
	Document* pDocument_;
	Type type_;
	Account* pAccount_;
};

}

#endif // __QMDOCUMENT_H__
