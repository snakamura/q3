/*
 * $Id$
 *
 * Copyright(C) 1998-2003 Satoshi Nakamura
 * All rights reserved.
 *
 */

#ifndef __QMSEARCH_H__
#define __QMSEARCH_H__

#include <qs.h>
#include <qsdialog.h>
#include <qsprofile.h>

#include <qm.h>
#include <qmmessageholder.h>


namespace qm {

class SearchDriver;
class SearchUI;
class SearchPropertyPage;
class SearchDriverFactory;
class SearchContext;

class Account;
class AccountLock;
class Document;
class Folder;


/****************************************************************************
 *
 * SearchDriver
 *
 */

class QMEXPORTCLASS SearchDriver
{
public:
	virtual ~SearchDriver();

public:
	virtual qs::QSTATUS search(const SearchContext& context,
		MessageHolderList* pList) = 0;
};


/****************************************************************************
 *
 * SearchUI
 *
 */

class QMEXPORTCLASS SearchUI
{
public:
	virtual ~SearchUI();

public:
	virtual int getIndex() = 0;
	virtual const WCHAR* getName() = 0;
	virtual qs::QSTATUS getDisplayName(qs::WSTRING* pwstrName) = 0;
	virtual qs::QSTATUS createPropertyPage(
		bool bAllFolder, SearchPropertyPage** ppPage) = 0;
};


/****************************************************************************
 *
 * SearchPropertyPage
 *
 */

class QMEXPORTCLASS SearchPropertyPage : public qs::DefaultPropertyPage
{
protected:
	SearchPropertyPage(HINSTANCE hInst, UINT nId, qs::QSTATUS* pstatus);

public:
	virtual ~SearchPropertyPage();

public:
	virtual const WCHAR* getDriver() const = 0;
	virtual const WCHAR* getCondition() const = 0;
	virtual bool isAllFolder() const = 0;
	virtual bool isRecursive() const = 0;

private:
	SearchPropertyPage(const SearchPropertyPage&);
	SearchPropertyPage& operator=(const SearchPropertyPage&);
};


/****************************************************************************
 *
 * SearchDriverFactory
 *
 */

class QMEXPORTCLASS SearchDriverFactory
{
public:
	typedef std::vector<const WCHAR*> NameList;

protected:
	SearchDriverFactory();

public:
	virtual ~SearchDriverFactory();

public:
	static qs::QSTATUS getDriver(const WCHAR* pwszName,
		Document* pDocument, Account* pAccount, HWND hwnd,
		qs::Profile* pProfile, SearchDriver** ppDriver);
	static qs::QSTATUS getDriver(const WCHAR* pwszName,
		Document* pDocument, Account* pAccount, HWND hwnd,
		qs::Profile* pProfile, std::auto_ptr<SearchDriver>* papDriver);
	static qs::QSTATUS getUI(const WCHAR* pwszName,
		Account* pAccount, qs::Profile* pProfile, SearchUI** ppUI);
	static qs::QSTATUS getUI(const WCHAR* pwszName, Account* pAccount,
		qs::Profile* pProfile, std::auto_ptr<SearchUI>* papUI);
	static qs::QSTATUS getNames(NameList* pList);

protected:
	virtual qs::QSTATUS createDriver(Document* pDocument, Account* pAccount,
		HWND hwnd, qs::Profile* pProfile, SearchDriver** ppDriver) = 0;
	virtual qs::QSTATUS createUI(Account* pAccount,
		qs::Profile* pProfile, SearchUI** ppUI) = 0;

protected:
	static qs::QSTATUS regist(const WCHAR* pwszName,
		SearchDriverFactory* pFactory);
	static qs::QSTATUS unregist(const WCHAR* pwszName);

private:
	SearchDriverFactory(const SearchDriverFactory&);
	SearchDriverFactory& operator=(const SearchDriverFactory&);
};


/****************************************************************************
 *
 * SearchContext
 *
 */

class QMEXPORTCLASS SearchContext
{
public:
	typedef std::vector<NormalFolder*> FolderList;

public:
	SearchContext(const WCHAR* pwszCondition, const WCHAR* pwszTargetFolder,
		bool bRecursive, qs::QSTATUS* pstatus);
	~SearchContext();

public:
	const WCHAR* getCondition() const;
	const WCHAR* getTargetFolder() const;
	bool isRecursive() const;
	qs::QSTATUS getTargetFolders(Account* pAccount, FolderList* pList) const;

private:
	SearchContext(const SearchContext&);
	SearchContext& operator=(const SearchContext&);

private:
	qs::WSTRING wstrCondition_;
	qs::WSTRING wstrTargetFolder_;
	bool bRecursive_;
};

}

#endif // __QMSEARCH__
