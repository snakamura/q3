/*
 * $Id$
 *
 * Copyright(C) 1998-2005 Satoshi Nakamura
 * All rights reserved.
 *
 */

#ifndef __PROPERTYPAGES_H__
#define __PROPERTYPAGES_H__

#include <qm.h>
#include <qmfolder.h>

#include <qs.h>
#include <qsdialog.h>


namespace qm {

class DefaultPropertyPage;
	class AccountAdvancedPage;
	class AccountDialupPage;
	class AccountGeneralPage;
	class AccountUserPage;
	class FolderConditionPage;
	class FolderParameterPage;
	class FolderPropertyPage;
	class MessagePropertyPage;

class JunkFilter;
class OptionDialogManager;
class PasswordManager;
class ReceiveSessionUI;
class SearchUI;
class SendSessionUI;
class SubAccount;
class SyncFilterManager;


/****************************************************************************
 *
 * DefaultPropertyPage
 *
 */

class DefaultPropertyPage : public qs::DefaultPropertyPage
{
protected:
	explicit DefaultPropertyPage(UINT nId);

public:
	virtual ~DefaultPropertyPage();

private:
	DefaultPropertyPage(const DefaultPropertyPage&);
	DefaultPropertyPage& operator=(const DefaultPropertyPage&);
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
	LRESULT onEdit();

private:
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
 * FolderConditionPage
 *
 */

class FolderConditionPage : public DefaultPropertyPage
{
public:
	FolderConditionPage(QueryFolder* pFolder,
						qs::Profile* pProfile);
	virtual ~FolderConditionPage();

protected:
	virtual LRESULT onInitDialog(HWND hwndFocus,
								 LPARAM lParam);

protected:
	virtual LRESULT onOk();

private:
	void initDriver();
	void initFolder();

private:
	FolderConditionPage(const FolderConditionPage&);
	FolderConditionPage& operator=(const FolderConditionPage&);

private:
	typedef std::vector<SearchUI*> UIList;

private:
	QueryFolder* pFolder_;
	qs::Profile* pProfile_;
	UIList listUI_;
	Account::FolderList listFolder_;
};


/****************************************************************************
 *
 * FolderParameterPage
 *
 */

class FolderParameterPage : public DefaultPropertyPage
{
public:
	FolderParameterPage(Folder* pFolder,
						const WCHAR** ppwszParams,
						size_t nParamCount);
	virtual ~FolderParameterPage();

public:
	virtual LRESULT onCommand(WORD nCode,
							  WORD nId);

protected:
	virtual LRESULT onInitDialog(HWND hwndFocus,
								 LPARAM lParam);

protected:
	virtual LRESULT onOk();

public:
	virtual LRESULT onNotify(NMHDR* pnmhdr,
							 bool* pbHandled);

private:
	LRESULT onEdit();
	LRESULT onParameterDblClk(NMHDR* pnmhdr,
							  bool* pbHandled);
	LRESULT onParameterItemChanged(NMHDR* pnmhdr,
								   bool* pbHandled);

private:
	void edit();
	void updateState();

private:
	FolderParameterPage(const FolderParameterPage&);
	FolderParameterPage& operator=(const FolderParameterPage&);

private:
	Folder* pFolder_;
	const WCHAR** ppwszParams_;
	size_t nParamCount_;
};


/****************************************************************************
 *
 * FolderPropertyPage
 *
 */

class FolderPropertyPage : public DefaultPropertyPage
{
public:
	FolderPropertyPage(const Account::FolderList& l);
	virtual ~FolderPropertyPage();

protected:
	virtual LRESULT onInitDialog(HWND hwndFocus,
								 LPARAM lParam);

protected:
	virtual LRESULT onOk();

private:
	FolderPropertyPage(const FolderPropertyPage&);
	FolderPropertyPage& operator=(const FolderPropertyPage&);

private:
	const Account::FolderList& listFolder_;
};


/****************************************************************************
 *
 * MessagePropertyPage
 *
 */

class MessagePropertyPage : public DefaultPropertyPage
{
public:
	MessagePropertyPage(const MessageHolderList& l);
	virtual ~MessagePropertyPage();

public:
	unsigned int getFlags() const;
	unsigned int getMask() const;

protected:
	virtual LRESULT onInitDialog(HWND hwndFocus,
								 LPARAM lParam);

protected:
	virtual LRESULT onOk();

private:
	MessagePropertyPage(const MessagePropertyPage&);
	MessagePropertyPage& operator=(const MessagePropertyPage&);

private:
	const MessageHolderList& listMessage_;
	unsigned int nFlags_;
	unsigned int nMask_;
};

}

#endif // __PROPERTYPAGES_H__
