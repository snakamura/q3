/*
 * $Id$
 *
 * Copyright(C) 1998-2007 Satoshi Nakamura
 * All rights reserved.
 *
 */

#ifndef __MACROSEARCH_H__
#define __MACROSEARCH_H__

#include <qmsearch.h>

#include <qsinit.h>


namespace qm {

/****************************************************************************
 *
 * MacroSearchDriver
 *
 */

class MacroSearchDriver : public SearchDriver
{
public:
	MacroSearchDriver(Document* pDocument,
					  Account* pAccount,
					  ActionInvoker* pActionInvoker,
					  HWND hwnd,
					  qs::Profile* pProfile);
	virtual ~MacroSearchDriver();

public:
	virtual bool search(const SearchContext& context,
						MessageHolderList* pList);

private:
	MacroSearchDriver(const MacroSearchDriver&);
	MacroSearchDriver& operator=(const MacroSearchDriver&);

private:
	Document* pDocument_;
	Account* pAccount_;
	ActionInvoker* pActionInvoker_;
	HWND hwnd_;
	qs::Profile* pProfile_;
};


/****************************************************************************
 *
 * MacroSearchUI
 *
 */

class MacroSearchUI : public SearchUI
{
public:
	explicit MacroSearchUI(qs::Profile* pProfile);
	virtual ~MacroSearchUI();

public:
	virtual int getIndex();
	virtual const WCHAR* getName();
	virtual qs::wstring_ptr getDisplayName();
	virtual std::auto_ptr<SearchPropertyPage> createPropertyPage(SearchPropertyData* pData);

private:
	MacroSearchUI(const MacroSearchUI&);
	MacroSearchUI& operator=(const MacroSearchUI&);

private:
	qs::Profile* pProfile_;
};


/****************************************************************************
 *
 * MacroSearchPage
 *
 */

class MacroSearchPage : public SearchPropertyPage
{
public:
	MacroSearchPage(qs::Profile* pProfile,
					SearchPropertyData* pData);
	virtual ~MacroSearchPage();

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
	LRESULT onMacro();

private:
	void updateState();

private:
	static qs::wstring_ptr getLiteral(const WCHAR* pwsz);
	static void createMacro(qs::StringBuffer<qs::WSTRING>* pBuf,
							const WCHAR* pwszField,
							const WCHAR* pwszLiteral,
							bool bCase);

private:
	MacroSearchPage(const MacroSearchPage&);
	MacroSearchPage& operator=(const MacroSearchPage&);

private:
	qs::Profile* pProfile_;
	qs::wstring_ptr wstrCondition_;
};


/****************************************************************************
 *
 * MacroSearchDriverFactory
 *
 */

class MacroSearchDriverFactory : public SearchDriverFactory
{
public:
	MacroSearchDriverFactory();
	virtual ~MacroSearchDriverFactory();

protected:
	virtual std::auto_ptr<SearchDriver> createDriver(Document* pDocument,
													 Account* pAccount,
													 ActionInvoker* pActionInvoker,
													 HWND hwnd,
													 qs::Profile* pProfile);
	virtual std::auto_ptr<SearchUI> createUI(Account* pAccount,
											 qs::Profile* pProfile);

private:
	MacroSearchDriverFactory(const MacroSearchDriverFactory&);
	MacroSearchDriverFactory& operator=(const MacroSearchDriverFactory&);

private:
	static MacroSearchDriverFactory* pFactory__;
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

#endif // __MACROSEARCH__
