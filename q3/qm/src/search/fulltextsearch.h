/*
 * $Id$
 *
 * Copyright(C) 1998-2007 Satoshi Nakamura
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
						 qs::Profile* pProfile);
	virtual ~FullTextSearchDriver();

public:
	virtual bool search(const SearchContext& context,
						MessageHolderList* pList);

private:
	static qs::wstring_ptr escapeQuote(const WCHAR* pwsz);

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
					 qs::Profile* pProfile);
	virtual ~FullTextSearchUI();

public:
	virtual int getIndex();
	virtual const WCHAR* getName();
	virtual qs::wstring_ptr getDisplayName();
	virtual std::auto_ptr<SearchPropertyPage> createPropertyPage(SearchPropertyData* pData);

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
	FullTextSearchPage(Account* pAccount,
					   qs::Profile* pProfile,
					   SearchPropertyData* pData);
	virtual ~FullTextSearchPage();

public:
	virtual const WCHAR* getDriver() const;
	virtual const WCHAR* getCondition() const;

public:
	virtual LRESULT onCommand(WORD nCode,
							  WORD nId);

protected:
	virtual LRESULT onInitDialog(HWND hwndFocus,
								 LPARAM lParam);

protected:
	virtual LRESULT onOk();

private:
	LRESULT onUpdateIndex();

private:
	bool updateIndex();

private:
	FullTextSearchPage(const FullTextSearchPage&);
	FullTextSearchPage& operator=(const FullTextSearchPage&);

private:
	Account* pAccount_;
	qs::Profile* pProfile_;
	qs::wstring_ptr wstrCondition_;
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
	virtual std::auto_ptr<SearchDriver> createDriver(Document* pDocument,
													 Account* pAccount,
													 ActionInvoker* pActionInvoker,
													 HWND hwnd,
													 qs::Profile* pProfile);
	virtual std::auto_ptr<SearchUI> createUI(Account* pAccount,
											 qs::Profile* pProfile);

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
		virtual bool init();
		virtual void term();
	} init__;
	friend class InitializerImpl;
};

}

#endif // _WIN32_WCE

#endif // __FULLTEXTSEARCH__
