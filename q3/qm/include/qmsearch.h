/*
 * $Id$
 *
 * Copyright(C) 1998-2007 Satoshi Nakamura
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
class SearchPropertyData;
class SearchPropertyPage;
class SearchDriverFactory;
class SearchContext;

class Account;
class AccountLock;
class ActionInvoker;
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
	virtual std::auto_ptr<SearchPropertyPage> createPropertyPage(SearchPropertyData* pData) = 0;
};


/****************************************************************************
 *
 * SearchPropertyData
 *
 */

class QMEXPORTCLASS SearchPropertyData
{
public:
	enum ImeFlag {
		IMEFLAG_NONE	= 0x00,
		IMEFLAG_IME		= 0x01,
		IMEFLAG_SIP		= 0x02
	};

public:
	SearchPropertyData(const Account* pAccount,
					   const Folder* pFolder,
					   qs::Profile* pProfile);
	~SearchPropertyData();

public:
	const WCHAR* getCondition() const;
	const Account* getAccount() const;
	const Folder* getFolder() const;
	bool isRecursive() const;
	bool isNewFolder() const;
	unsigned int getImeFlags() const;
	DWORD getImeStatus() const;
	void set(const WCHAR* pwszCondition,
			 const Folder* pFolder,
			 bool bRecursive,
			 bool bNewFolder,
			 unsigned int nImeFlags,
			 DWORD dwImeStatus);
	void save() const;
	bool isImeControl() const;

private:
	SearchPropertyData(const SearchPropertyData&);
	SearchPropertyData& operator=(const SearchPropertyData&);

private:
	struct SearchPropertyDataImpl* pImpl_;
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
					   UINT nIdPortrait,
					   UINT nIdLandscape,
					   UINT nConditionId,
					   UINT nFolderId,
					   UINT nRecursiveId,
					   UINT nNewFolderId,
					   SearchPropertyData* pData);

public:
	virtual ~SearchPropertyPage();

public:
	virtual LRESULT onNotify(NMHDR* pnmhdr,
							 bool* pbHandled);

protected:
	virtual LRESULT onInitDialog(HWND hwndFocus,
								 LPARAM lParam);

public:
	virtual const WCHAR* getDriver() const = 0;
	virtual const WCHAR* getCondition() const = 0;

protected:
	virtual void updateData(SearchPropertyData* pData);
	virtual void updateUI(const SearchPropertyData* pData);

protected:
	unsigned int getImeFlags() const;
	void setImeFlags(unsigned int nFlags);
	DWORD getImeStatus() const;
	void setImeStatus(DWORD dwStatus);

private:
	LRESULT onKillActive(NMHDR* pnmhdr,
						 bool* pbHandled);
	LRESULT onSetActive(NMHDR* pnmhdr,
						bool* pbHandled);

private:
	SearchPropertyPage(const SearchPropertyPage&);
	SearchPropertyPage& operator=(const SearchPropertyPage&);

private:
	SearchPropertyData* pData_;
	UINT nConditionId_;
	UINT nFolderId_;
	UINT nRecursiveId_;
	UINT nNewFolderId_;
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
												 ActionInvoker* pActionInvoker,
												 HWND hwnd,
												 qs::Profile* pProfile);
	static std::auto_ptr<SearchUI> getUI(const WCHAR* pwszName,
										 Account* pAccount,
										 qs::Profile* pProfile);
	static void getNames(NameList* pList);

protected:
	virtual std::auto_ptr<SearchDriver> createDriver(Document* pDocument,
													 Account* pAccount,
													 ActionInvoker* pActionInvoker,
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
