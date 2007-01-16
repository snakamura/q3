/*
 * $Id$
 *
 * Copyright(C) 1998-2007 Satoshi Nakamura
 * All rights reserved.
 *
 */

#ifndef __ACCOUNTDIALOG_H__
#define __ACCOUNTDIALOG_H__

#include <qm.h>
#include <qmfilenames.h>

#include "dialogs.h"
#include "propertypages.h"


namespace qm {

class AccountDialog;
class CreateAccountDialog;
class CreateSubAccountDialog;
class AccountGeneralPage;
class AccountUserPage;
class AccountDetailPage;
class AccountDialupPage;
class AccountAdvancedPage;

class JunkFilter;
class OptionDialogManager;
class PasswordManager;
class ReceiveSessionUI;
class SendSessionUI;
class SubAccount;
class SyncFilterManager;


/****************************************************************************
 *
 * AccountDialog
 *
 */

class AccountDialog :
	public DefaultDialog,
	public qs::NotifyHandler
{
public:
	AccountDialog(AccountManager* pAccountManager,
				  Account* pAccount,
				  PasswordManager* pPasswordManager,
				  SyncFilterManager* pSyncFilterManager,
				  const Security* pSecurity,
				  JunkFilter* pJunkFilter,
				  const FolderImage* pFolderImage,
				  OptionDialogManager* pOptionDialogManager,
				  qs::Profile* pProfile);
	virtual ~AccountDialog();

public:
	virtual LRESULT onCommand(WORD nCode,
							  WORD nId);

protected:
	virtual LRESULT onDestroy();
	virtual LRESULT onInitDialog(HWND hwndFocus,
								 LPARAM lParam);

public:
	virtual LRESULT onNotify(NMHDR* pnmhdr,
							 bool* pbHandled);

private:
	LRESULT onAddAccount();
	LRESULT onAddSubAccount();
	LRESULT onRemove();
	LRESULT onRename();
	LRESULT onProperty();
	LRESULT onAccountSelChanged(NMHDR* pnmhdr,
								bool* pbHandled);

private:
	void update();
	void updateState();

private:
	static void initProfileForClass(const WCHAR* pwszClass,
									qs::Profile* pProfile);

private:
	AccountDialog(const AccountDialog&);
	AccountDialog& operator=(const AccountDialog&);

private:
	AccountManager* pAccountManager_;
	SubAccount* pSubAccount_;
	PasswordManager* pPasswordManager_;
	SyncFilterManager* pSyncFilterManager_;
	const Security* pSecurity_;
	JunkFilter* pJunkFilter_;
	const FolderImage* pFolderImage_;
	OptionDialogManager* pOptionDialogManager_;
	qs::Profile* pProfile_;
	bool bAccountAdded_;
};


/****************************************************************************
 *
 * CreateAccountDialog
 *
 */

class CreateAccountDialog : public DefaultDialog
{
public:
	explicit CreateAccountDialog(qs::Profile* pProfile);
	virtual ~CreateAccountDialog();

public:
	const WCHAR* getName() const;
	const WCHAR* getClass() const;
	const WCHAR* getReceiveProtocol() const;
	short getReceivePort() const;
	const WCHAR* getSendProtocol() const;
	short getSendPort() const;
	unsigned int getBlockSize() const;
	unsigned int getIndexBlockSize() const;

public:
	virtual LRESULT onCommand(WORD nCode,
							  WORD nId);

protected:
	virtual LRESULT onInitDialog(HWND hwndFocus,
								 LPARAM lParam);

protected:
	virtual LRESULT onOk();

private:
	LRESULT onNameChange();
	LRESULT onClassChange();
	LRESULT onProtocolChange();
	LRESULT onTypeChange();

private:
	void updateProtocols();
	void clearProtocols();
	void updateState();

private:
	CreateAccountDialog(const CreateAccountDialog&);
	CreateAccountDialog& operator=(const CreateAccountDialog&);

private:
	struct Protocol
	{
		qs::WSTRING wstrName_;
		short nPort_;
	};

private:
	typedef std::vector<Protocol> ProtocolList;

private:
	qs::Profile* pProfile_;
	ProtocolList listReceiveProtocol_;
	ProtocolList listSendProtocol_;
	qs::wstring_ptr wstrName_;
	qs::wstring_ptr wstrClass_;
	int nReceiveProtocol_;
	int nSendProtocol_;
	unsigned int nBlockSize_;
	unsigned int nIndexBlockSize_;
};


/****************************************************************************
 *
 * CreateSubAccountDialog
 *
 */

class CreateSubAccountDialog : public DefaultDialog
{
public:
	explicit CreateSubAccountDialog(AccountManager* pAccountManager);
	virtual ~CreateSubAccountDialog();

public:
	const WCHAR* getName() const;

public:
	virtual LRESULT onCommand(WORD nCode,
							  WORD nId);

protected:
	virtual LRESULT onInitDialog(HWND hwndFocus,
								 LPARAM lParam);

protected:
	virtual LRESULT onOk();

private:
	LRESULT onNameChanged();

private:
	void updateState();

private:
	CreateSubAccountDialog(const CreateSubAccountDialog&);
	CreateSubAccountDialog& operator=(const CreateSubAccountDialog&);

private:
	AccountManager* pAccountManager_;
	qs::wstring_ptr wstrName_;
};


/****************************************************************************
 *
 * AccountGeneralPage
 *
 */

class AccountGeneralPage : public DefaultPropertyPage
{
public:
	AccountGeneralPage(SubAccount* pSubAccount,
					   ReceiveSessionUI* pReceiveUI,
					   SendSessionUI* pSendUI);
	virtual ~AccountGeneralPage();

protected:
	virtual LRESULT onInitDialog(HWND hwndFocus,
								 LPARAM lParam);

protected:
	virtual LRESULT onOk();

private:
	AccountGeneralPage(const AccountGeneralPage&);
	AccountGeneralPage& operator=(const AccountGeneralPage&);

private:
	SubAccount* pSubAccount_;
	ReceiveSessionUI* pReceiveUI_;
	SendSessionUI* pSendUI_;
};


/****************************************************************************
 *
 * AccountUserPage
 *
 */

class AccountUserPage : public DefaultPropertyPage
{
public:
	AccountUserPage(SubAccount* pSubAccount,
					PasswordManager* pPasswordManager,
					ReceiveSessionUI* pReceiveUI,
					SendSessionUI* pSendUI);
	virtual ~AccountUserPage();

public:
	virtual LRESULT onCommand(WORD nCode,
							  WORD nId);

protected:
	virtual LRESULT onInitDialog(HWND hwndFocus,
								 LPARAM lParam);

protected:
	virtual LRESULT onOk();

private:
	LRESULT onSendAuthenticate();

private:
	void setPassword(UINT nId,
					 Account::Host host);
	void getPassword(UINT nId,
					 Account::Host host,
					 bool bForceRemove);
	void updateState();

private:
	AccountUserPage(const AccountUserPage&);
	AccountUserPage& operator=(const AccountUserPage&);

private:
	SubAccount* pSubAccount_;
	PasswordManager* pPasswordManager_;
	ReceiveSessionUI* pReceiveUI_;
	SendSessionUI* pSendUI_;
};


/****************************************************************************
 *
 * AccountDetailPage
 *
 */

class AccountDetailPage : public DefaultPropertyPage
{
public:
	AccountDetailPage(SubAccount* pSubAccount,
					  ReceiveSessionUI* pReceiveUI,
					  SendSessionUI* pSendUI);
	virtual ~AccountDetailPage();

public:
	virtual LRESULT onCommand(WORD nCode,
							  WORD nId);

protected:
	virtual LRESULT onInitDialog(HWND hwndFocus,
								 LPARAM lParam);

protected:
	virtual LRESULT onOk();

private:
	LRESULT onSecure(UINT nId);

private:
	AccountDetailPage(const AccountDetailPage&);
	AccountDetailPage& operator=(const AccountDetailPage&);

private:
	SubAccount* pSubAccount_;
	ReceiveSessionUI* pReceiveUI_;
	SendSessionUI* pSendUI_;
};


/****************************************************************************
 *
 * AccountDialupPage
 *
 */

class AccountDialupPage : public DefaultPropertyPage
{
public:
	explicit AccountDialupPage(SubAccount* pSubAccount);
	virtual ~AccountDialupPage();

public:
	virtual LRESULT onCommand(WORD nCode,
							  WORD nId);

protected:
	virtual LRESULT onInitDialog(HWND hwndFocus,
								 LPARAM lParam);

protected:
	virtual LRESULT onOk();

private:
	LRESULT onDialProperty();
	LRESULT onTypeSelect(UINT nId);

private:
	void updateState();

private:
	AccountDialupPage(const AccountDialupPage&);
	AccountDialupPage& operator=(const AccountDialupPage&);

private:
	SubAccount* pSubAccount_;
};


/****************************************************************************
 *
 * AccountAdvancedPage
 *
 */

class AccountAdvancedPage : public DefaultPropertyPage
{
public:
	AccountAdvancedPage(SubAccount* pSubAccount,
						JunkFilter* pJunkFilter,
						SyncFilterManager* pSyncFilterManager,
						OptionDialogManager* pOptionDialogManager);
	virtual ~AccountAdvancedPage();

public:
	virtual LRESULT onCommand(WORD nCode,
							  WORD nId);

protected:
	virtual LRESULT onInitDialog(HWND hwndFocus,
								 LPARAM lParam);

protected:
	virtual LRESULT onOk();

private:
	LRESULT onAutoApplyRules();
	LRESULT onEdit();

private:
	void updateState();
	void updateFilter();

private:
	AccountAdvancedPage(const AccountAdvancedPage&);
	AccountAdvancedPage& operator=(const AccountAdvancedPage&);

private:
	SubAccount* pSubAccount_;
	JunkFilter* pJunkFilter_;
	SyncFilterManager* pSyncFilterManager_;
	OptionDialogManager* pOptionDialogManager_;
};

}

#endif // __ACCOUNTDIALOG_H__
