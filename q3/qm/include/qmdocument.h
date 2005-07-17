/*
 * $Id$
 *
 * Copyright(C) 1998-2005 Satoshi Nakamura
 * All rights reserved.
 *
 */

#ifndef __QMDOCUMENT_H__
#define __QMDOCUMENT_H__

#include <qm.h>
#include <qmaccount.h>

#include <qsprofile.h>
#include <qsstring.h>

#include <vector>


namespace qm {

class Document;
class DocumentHandler;
	class DefaultDocumentHandler;
class DocumentEvent;

class Account;
class AddressBook;
class FixedFormTextManager;
class Folder;
class MessageHolder;
class MessagePtr;
class PasswordManager;
class RecentAddress;
class Recents;
class RuleManager;
class ScriptManager;
class Security;
class SignatureManager;
class JunkFilter;
class TemplateManager;
class UndoManager;
class URI;


/****************************************************************************
 *
 * Document
 *
 */

class QMEXPORTCLASS Document : public AccountManager
{
public:
	Document(qs::Profile* pProfile,
			 PasswordManager* pPasswordManager);
	~Document();

public:
	virtual Account* getAccount(const WCHAR* pwszName) const;
	virtual const AccountList& getAccounts() const;
	virtual bool hasAccount(const WCHAR* pwszName) const;
	virtual void addAccount(std::auto_ptr<Account> pAccount);
	virtual void removeAccount(Account* pAccount);
	virtual bool renameAccount(Account* pAccount,
							   const WCHAR* pwszName);
	virtual Folder* getFolder(Account* pAccount,
							  const WCHAR* pwszName) const;
	virtual MessagePtr getMessage(const URI& uri) const;
	virtual void addAccountManagerHandler(AccountManagerHandler* pHandler);
	virtual void removeAccountManagerHandler(AccountManagerHandler* pHandler);

public:
	bool loadAccounts(const WCHAR* pwszPath);

public:
	RuleManager* getRuleManager() const;
	const TemplateManager* getTemplateManager() const;
	ScriptManager* getScriptManager() const;
	SignatureManager* getSignatureManager() const;
	FixedFormTextManager* getFixedFormTextManager() const;
	AddressBook* getAddressBook() const;
	RecentAddress* getRecentAddress() const;
	Security* getSecurity() const;
	Recents* getRecents() const;
	UndoManager* getUndoManager() const;
	JunkFilter* getJunkFilter() const;
	
	bool isOffline() const;
	void setOffline(bool bOffline);
	void incrementInternalOnline();
	void decrementInternalOnline();
	bool save(bool bForce);
	
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

class QMEXPORTCLASS DocumentHandler
{
public:
	virtual ~DocumentHandler();

public:
	virtual void offlineStatusChanged(const DocumentEvent& event) = 0;
	virtual void documentInitialized(const DocumentEvent& event) = 0;
};


/****************************************************************************
 *
 * DefaultDocumentHandler
 *
 */

class QMEXPORTCLASS DefaultDocumentHandler : public DocumentHandler
{
public:
	DefaultDocumentHandler();
	virtual ~DefaultDocumentHandler();

public:
	virtual void offlineStatusChanged(const DocumentEvent& event);
	virtual void documentInitialized(const DocumentEvent& event);
};


/****************************************************************************
 *
 * DocumentEvent
 *
 */

class QMEXPORTCLASS DocumentEvent
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

}

#endif // __QMDOCUMENT_H__
