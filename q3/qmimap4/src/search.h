/*
 * $Id$
 *
 * Copyright(C) 1998-2003 Satoshi Nakamura
 * All rights reserved.
 *
 */

#ifndef __SEARCH_H__
#define __SEARCH_H__

#include <qmsearch.h>


namespace qmimap4 {

/****************************************************************************
 *
 * Imap4SearchDriver
 *
 */

class Imap4SearchDriver : public qm::SearchDriver
{
public:
	Imap4SearchDriver(qm::Account* pAccount,
		qs::Profile* pProfile, qs::QSTATUS* pstatus);
	virtual ~Imap4SearchDriver();

public:
	virtual qs::QSTATUS search(const qm::SearchContext& context,
		qm::MessageHolderList* pList);

private:
	Imap4SearchDriver(const Imap4SearchDriver&);
	Imap4SearchDriver& operator=(const Imap4SearchDriver&);

private:
	qm::Account* pAccount_;
	qs::Profile* pProfile_;
};


/****************************************************************************
 *
 * Imap4SearchUI
 *
 */

class Imap4SearchUI : public qm::SearchUI
{
public:
	Imap4SearchUI(qm::Account* pAccount,
		qs::Profile* pProfile, qs::QSTATUS* pstatus);
	virtual ~Imap4SearchUI();

public:
	virtual int getIndex();
	virtual qs::QSTATUS createPropertyPage(
		bool bAllFolder, qm::SearchPropertyPage** ppPage);

private:
	Imap4SearchUI(const Imap4SearchUI&);
	Imap4SearchUI& operator=(const Imap4SearchUI&);

private:
	qm::Account* pAccount_;
	qs::Profile* pProfile_;
};


/****************************************************************************
 *
 * Imap4SearchPage
 *
 */

class Imap4SearchPage : public qm::SearchPropertyPage
{
public:
	Imap4SearchPage(qm::Account* pAccount, qs::Profile* pProfile,
		bool bAllFolder, qs::QSTATUS* pstatus);
	virtual ~Imap4SearchPage();

public:
	virtual const WCHAR* getDriver() const;
	virtual const WCHAR* getCondition() const;
	virtual bool isAllFolder() const;
	virtual bool isRecursive() const;

protected:
	virtual LRESULT onInitDialog(HWND hwndFocus, LPARAM lParam);

protected:
	virtual LRESULT onOk();

private:
	static qs::QSTATUS getLiteral(const WCHAR* pwsz, qs::WSTRING* pwstr);

private:
	Imap4SearchPage(const Imap4SearchPage&);
	Imap4SearchPage& operator=(const Imap4SearchPage&);

private:
	qm::Account* pAccount_;
	qs::Profile* pProfile_;
	qs::WSTRING wstrCondition_;
	bool bAllFolder_;
	bool bRecursive_;
};


/****************************************************************************
 *
 * Imap4SearchDriverFactory
 *
 */

class Imap4SearchDriverFactory : public qm::SearchDriverFactory
{
public:
	Imap4SearchDriverFactory();
	virtual ~Imap4SearchDriverFactory();

protected:
	virtual qs::QSTATUS createDriver(qm::Document* pDocument, qm::Account* pAccount,
		HWND hwnd, qs::Profile* pProfile, qm::SearchDriver** ppDriver);
	virtual qs::QSTATUS createUI(qm::Account* pAccount,
		qs::Profile* pProfile, qm::SearchUI** ppUI);

private:
	Imap4SearchDriverFactory(const Imap4SearchDriverFactory&);
	Imap4SearchDriverFactory& operator=(const Imap4SearchDriverFactory&);

private:
	static Imap4SearchDriverFactory factory__;
};

}

#endif // __SEARCH_H__
