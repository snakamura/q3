/*
 * $Id$
 *
 * Copyright(C) 1998-2004 Satoshi Nakamura
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
class FixedFormTextManager;
class Folder;
class MessageHolder;
class MessagePtr;
class PasswordManager;
class Recents;
class RuleManager;
class ScriptManager;
class Security;
class SignatureManager;
class JunkFilter;
class TemplateManager;
class URI;


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
	Document(qs::Profile* pProfile,
			 PasswordManager* pPasswordManager);
	~Document();

public:
	Account* getAccount(const WCHAR* pwszName) const;
	const AccountList& getAccounts() const;
	bool hasAccount(const WCHAR* pwszName) const;
	void addAccount(std::auto_ptr<Account> pAccount);
	void removeAccount(Account* pAccount);
	bool renameAccount(Account* pAccount,
					   const WCHAR* pwszName);
	bool loadAccounts(const WCHAR* pwszPath);
	Folder* getFolder(Account* pAccount,
					  const WCHAR* pwszName) const;
	MessagePtr getMessage(const URI& uri) const;
	
	RuleManager* getRuleManager() const;
	const TemplateManager* getTemplateManager() const;
	ScriptManager* getScriptManager() const;
	SignatureManager* getSignatureManager() const;
	FixedFormTextManager* getFixedFormTextManager() const;
	AddressBook* getAddressBook() const;
	const Security* getSecurity() const;
	Recents* getRecents() const;
	JunkFilter* getJunkFilter() const;
	
	bool isOffline() const;
	void setOffline(bool bOffline);
	void incrementInternalOnline();
	void decrementInternalOnline();
	bool save();
	
	void addDocumentHandler(DocumentHandler* pHandler);
	void removeDocumentHandler(DocumentHandler* pHandler);

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
	virtual void offlineStatusChanged(const DocumentEvent& event) = 0;
	virtual void accountListChanged(const AccountListChangedEvent& event) = 0;
	virtual void documentInitialized(const DocumentEvent& event) = 0;
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
	virtual void offlineStatusChanged(const DocumentEvent& event);
	virtual void accountListChanged(const AccountListChangedEvent& event);
	virtual void documentInitialized(const DocumentEvent& event);
};


/****************************************************************************
 *
 * DocumentEvent
 *
 */

class DocumentEvent
{
public:
	explicit DocumentEvent(Document* pDocument);
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
		TYPE_REMOVE
	};

public:
	AccountListChangedEvent(Document* pDocument,
							Type type,
							Account* pAccount);
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
