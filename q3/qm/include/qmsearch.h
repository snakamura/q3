/*
 * $Id$
 *
 * Copyright(C) 1998-2004 Satoshi Nakamura
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
	virtual bool search(const SearchContext& context,
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
	virtual qs::wstring_ptr getDisplayName() = 0;
	virtual std::auto_ptr<SearchPropertyPage> createPropertyPage(bool bAllFolder) = 0;
};


/****************************************************************************
 *
 * SearchPropertyPage
 *
 */

class QMEXPORTCLASS SearchPropertyPage : public qs::DefaultPropertyPage
{
protected:
	SearchPropertyPage(HINSTANCE hInst,
					   UINT nId);

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
	static std::auto_ptr<SearchDriver> getDriver(const WCHAR* pwszName,
												 Document* pDocument,
												 Account* pAccount,
												 HWND hwnd,
												 qs::Profile* pProfile);
	static std::auto_ptr<SearchUI> getUI(const WCHAR* pwszName,
										 Account* pAccount,
										 qs::Profile* pProfile);
	static void getNames(NameList* pList);

protected:
	virtual std::auto_ptr<SearchDriver> createDriver(Document* pDocument,
													 Account* pAccount,
													 HWND hwnd,
													 qs::Profile* pProfile) = 0;
	virtual std::auto_ptr<SearchUI> createUI(Account* pAccount,
											 qs::Profile* pProfile) = 0;

protected:
	static void registerFactory(const WCHAR* pwszName,
								SearchDriverFactory* pFactory);
	static void unregisterFactory(const WCHAR* pwszName);

private:
	SearchDriverFactory(const SearchDriverFactory&);
	SearchDriverFactory& operator=(const SearchDriverFactory&);
};


/****************************************************************************
 *
 * SearchContext
 *
 */

#pragma warning(push)
#pragma warning(disable:4251)

class QMEXPORTCLASS SearchContext
{
public:
	typedef std::vector<NormalFolder*> FolderList;

public:
	SearchContext(const WCHAR* pwszCondition,
				  const WCHAR* pwszTargetFolder,
				  bool bRecursive,
				  unsigned int nSecurityMode);
	~SearchContext();

public:
	const WCHAR* getCondition() const;
	const WCHAR* getTargetFolder() const;
	bool isRecursive() const;
	unsigned int getSecurityMode() const;
	void getTargetFolders(Account* pAccount,
						  FolderList* pList) const;

private:
	SearchContext(const SearchContext&);
	SearchContext& operator=(const SearchContext&);

private:
	qs::wstring_ptr wstrCondition_;
	qs::wstring_ptr wstrTargetFolder_;
	bool bRecursive_;
	unsigned int nSecurityMode_;
};

#pragma warning(pop)

}

#endif // __QMSEARCH__
