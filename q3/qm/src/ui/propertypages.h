/*
 * $Id$
 *
 * Copyright(C) 1998-2004 Satoshi Nakamura
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
	class FolderPropertyPage;
	class MessagePropertyPage;

class SearchUI;
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
						SyncFilterManager* pSyncFilterManager);
	virtual ~AccountAdvancedPage();

protected:
	virtual LRESULT onInitDialog(HWND hwndFocus,
								 LPARAM lParam);

protected:
	virtual LRESULT onOk();

private:
	AccountAdvancedPage(const AccountAdvancedPage&);
	AccountAdvancedPage& operator=(const AccountAdvancedPage&);

private:
	SubAccount* pSubAccount_;
	SyncFilterManager* pSyncFilterManager_;
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
	explicit AccountGeneralPage(SubAccount* pSubAccount);
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
};


/****************************************************************************
 *
 * AccountUserPage
 *
 */

class AccountUserPage : public DefaultPropertyPage
{
public:
	explicit AccountUserPage(SubAccount* pSubAccount);
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
	void updateState();

private:
	AccountUserPage(const AccountUserPage&);
	AccountUserPage& operator=(const AccountUserPage&);

private:
	SubAccount* pSubAccount_;
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

public:
	const WCHAR* getDriver() const;
	const WCHAR* getCondition() const;
	const WCHAR* getTargetFolder() const;
	bool isRecursive() const;
	bool isModified() const;

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
	int nDriver_;
	qs::wstring_ptr wstrCondition_;
	qs::wstring_ptr wstrTargetFolder_;
	bool bRecursive_;
	bool bModified_;
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

public:
	unsigned int getFlags() const;
	unsigned int getMask() const;

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
	unsigned int nFlags_;
	unsigned int nMask_;
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
