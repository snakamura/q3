/*
 * $Id$
 *
 * Copyright(C) 1998-2003 Satoshi Nakamura
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
	MacroSearchDriver(Document* pDocument, Account* pAccount,
		HWND hwnd, qs::Profile* pProfile, qs::QSTATUS* pstatus);
	virtual ~MacroSearchDriver();

public:
	virtual qs::QSTATUS search(const SearchContext& context,
		MessageHolderList* pList);

private:
	MacroSearchDriver(const MacroSearchDriver&);
	MacroSearchDriver& operator=(const MacroSearchDriver&);

private:
	Document* pDocument_;
	Account* pAccount_;
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
	MacroSearchUI(qs::Profile* pProfile, qs::QSTATUS* pstatus);
	virtual ~MacroSearchUI();

public:
	virtual int getIndex();
	virtual qs::QSTATUS createPropertyPage(
		bool bAllFolder, SearchPropertyPage** ppPage);

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
		bool bAllFolder, qs::QSTATUS* pstatus);
	virtual ~MacroSearchPage();

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
	LRESULT onMacro();
	LRESULT onMatchCase();
	LRESULT onSearchBody();

private:
	void updateState();

private:
	static qs::QSTATUS getLiteral(const WCHAR* pwsz, qs::WSTRING* pwstr);
	static qs::QSTATUS createMacro(qs::StringBuffer<qs::WSTRING>* pBuf,
		const WCHAR* pwszField, const WCHAR* pwszLiteral, bool bCase);

private:
	MacroSearchPage(const MacroSearchPage&);
	MacroSearchPage& operator=(const MacroSearchPage&);

private:
	qs::Profile* pProfile_;
	qs::WSTRING wstrCondition_;
	bool bAllFolder_;
	bool bRecursive_;
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
	virtual qs::QSTATUS createDriver(Document* pDocument, Account* pAccount,
		HWND hwnd, qs::Profile* pProfile, SearchDriver** ppDriver);
	virtual qs::QSTATUS createUI(Account* pAccount,
		qs::Profile* pProfile, SearchUI** ppUI);

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
		virtual qs::QSTATUS init();
		virtual qs::QSTATUS term();
	} init__;
	friend class InitializerImpl;
};

}

#endif // __MACROSEARCH__
