/*
 * $Id$
 *
 * Copyright(C) 1998-2003 Satoshi Nakamura
 * All rights reserved.
 *
 */

#ifndef __FULLTEXTSEARCH__
#define __FULLTEXTSEARCH__

#include <qmsearch.h>

#include <qsinit.h>

#ifndef _WIN32_WCE

namespace qm {

/****************************************************************************
 *
 * FullTextSearchDriver
 *
 */

class FullTextSearchDriver : public SearchDriver
{
public:
	FullTextSearchDriver(Account* pAccount,
		qs::Profile* pProfile, qs::QSTATUS* pstatus);
	virtual ~FullTextSearchDriver();

public:
	virtual qs::QSTATUS search(const SearchContext& context,
		MessageHolderList* pList);

private:
	static qs::QSTATUS replace(const WCHAR* pwsz, const WCHAR* pwszFind,
		const WCHAR* pwszReplace, qs::WSTRING* pwstr);

private:
	FullTextSearchDriver(const FullTextSearchDriver&);
	FullTextSearchDriver& operator=(const FullTextSearchDriver&);

private:
	Account* pAccount_;
	qs::Profile* pProfile_;
};


/****************************************************************************
 *
 * FullTextSearchUI
 *
 */

class FullTextSearchUI : public SearchUI
{
public:
	FullTextSearchUI(Account* pAccount,
		qs::Profile* pProfile, qs::QSTATUS* pstatus);
	virtual ~FullTextSearchUI();

public:
	virtual int getIndex();
	virtual const WCHAR* getName();
	virtual qs::QSTATUS getDisplayName(qs::WSTRING* pwstrName);
	virtual qs::QSTATUS createPropertyPage(
		bool bAllFolder, SearchPropertyPage** ppPage);

private:
	FullTextSearchUI(const FullTextSearchUI&);
	FullTextSearchUI& operator=(const FullTextSearchUI&);

private:
	Account* pAccount_;
	qs::Profile* pProfile_;
};


/****************************************************************************
 *
 * FullTextSearchPage
 *
 */

class FullTextSearchPage : public SearchPropertyPage
{
public:
	FullTextSearchPage(Account* pAccount, qs::Profile* pProfile,
		bool bAllFolder, qs::QSTATUS* pstatus);
	virtual ~FullTextSearchPage();

public:
	virtual const WCHAR* getDriver() const;
	virtual const WCHAR* getCondition() const;
	virtual bool isAllFolder() const;
	virtual bool isRecursive() const;

public:
	virtual LRESULT onCommand(WORD nCode, WORD nId);

protected:
	virtual LRESULT onInitDialog(HWND hwndFocus, LPARAM lParam);

protected:
	virtual LRESULT onOk();

private:
	LRESULT onUpdateIndex();

private:
	qs::QSTATUS updateIndex();

private:
	FullTextSearchPage(const FullTextSearchPage&);
	FullTextSearchPage& operator=(const FullTextSearchPage&);

private:
	Account* pAccount_;
	qs::Profile* pProfile_;
	qs::WSTRING wstrCondition_;
	bool bAllFolder_;
	bool bRecursive_;
};


/****************************************************************************
 *
 * FullTextSearchDriverFactory
 *
 */

class FullTextSearchDriverFactory : public SearchDriverFactory
{
public:
	FullTextSearchDriverFactory();
	virtual ~FullTextSearchDriverFactory();

protected:
	virtual qs::QSTATUS createDriver(Document* pDocument, Account* pAccount,
		HWND hwnd, qs::Profile* pProfile, SearchDriver** ppDriver);
	virtual qs::QSTATUS createUI(Account* pAccount,
		qs::Profile* pProfile, SearchUI** ppUI);

private:
	FullTextSearchDriverFactory(const FullTextSearchDriverFactory&);
	FullTextSearchDriverFactory& operator=(const FullTextSearchDriverFactory&);

private:
	static FullTextSearchDriverFactory* pFactory__;
	static class InitializerImpl : public qs::Initializer
	{
	public:
		InitializerImpl();
		virtual ~InitializerImpl();
	
	public:
		virtual qs::QSTATUS init();
		virtual qs::QSTATUS term();
	} init__;
	friend class InitializerImpl;
};


/****************************************************************************
 *
 * FullTextSearchUtil
 *
 */

class FullTextSearchUtil
{
public:
	static qs::QSTATUS replace(const WCHAR* pwsz, const WCHAR* pwszFind,
		const WCHAR* pwszReplace, qs::WSTRING* pwstr);
};

}

#endif // _WIN32_WCE

#endif // __FULLTEXTSEARCH__
