/*
 * $Id$
 *
 * Copyright(C) 1998-2003 Satoshi Nakamura
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
	DefaultPropertyPage(UINT nId, qs::QSTATUS* pstatus);

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
		SyncFilterManager* pSyncFilterManager, qs::QSTATUS* pstatus);
	virtual ~AccountAdvancedPage();

protected:
	virtual LRESULT onInitDialog(HWND hwndFocus, LPARAM lParam);

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
	AccountDialupPage(SubAccount* pSubAccount, qs::QSTATUS* pstatus);
	virtual ~AccountDialupPage();

public:
	virtual LRESULT onCommand(WORD nCode, WORD nId);

protected:
	virtual LRESULT onInitDialog(HWND hwndFocus, LPARAM lParam);

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
	AccountGeneralPage(SubAccount* pSubAccount, qs::QSTATUS* pstatus);
	virtual ~AccountGeneralPage();

protected:
	virtual LRESULT onInitDialog(HWND hwndFocus, LPARAM lParam);

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
	AccountUserPage(SubAccount* pSubAccount, qs::QSTATUS* pstatus);
	virtual ~AccountUserPage();

public:
	virtual LRESULT onCommand(WORD nCode, WORD nId);

protected:
	virtual LRESULT onInitDialog(HWND hwndFocus, LPARAM lParam);

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
		qs::Profile* pProfile, qs::QSTATUS* pstatus);
	virtual ~FolderConditionPage();

public:
	const WCHAR* getDriver() const;
	const WCHAR* getCondition() const;
	const WCHAR* getTargetFolder() const;
	bool isRecursive() const;

protected:
	virtual LRESULT onInitDialog(HWND hwndFocus, LPARAM lParam);

protected:
	virtual LRESULT onOk();

private:
	qs::QSTATUS initDriver();
	qs::QSTATUS initFolder();

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
	qs::WSTRING wstrCondition_;
	qs::WSTRING wstrTargetFolder_;
	bool bRecursive_;
};


/****************************************************************************
 *
 * FolderPropertyPage
 *
 */

class FolderPropertyPage : public DefaultPropertyPage
{
public:
	FolderPropertyPage(const Account::FolderList& l, qs::QSTATUS* pstatus);
	virtual ~FolderPropertyPage();

public:
	unsigned int getFlags() const;
	unsigned int getMask() const;

protected:
	virtual LRESULT onInitDialog(HWND hwndFocus, LPARAM lParam);

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
	MessagePropertyPage(const MessageHolderList& l, qs::QSTATUS* pstatus);
	virtual ~MessagePropertyPage();

public:
	unsigned int getFlags() const;
	unsigned int getMask() const;

protected:
	virtual LRESULT onInitDialog(HWND hwndFocus, LPARAM lParam);

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
