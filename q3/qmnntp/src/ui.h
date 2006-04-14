/*
 * $Id$
 *
 * Copyright(C) 1998-2005 Satoshi Nakamura
 * All rights reserved.
 *
 */

#ifndef __UI_H__
#define __UI_H__

#include <qmaccount.h>
#include <qmsession.h>

#include <qsdialog.h>


namespace qmnntp {

class ReceivePage;
class SendPage;
class SubscribeDialog;

class Groups;
class Group;


/****************************************************************************
 *
 * ReceivePage
 *
 */

class ReceivePage : public qs::DefaultPropertyPage
{
public:
	explicit ReceivePage(qm::SubAccount* pSubAccount);
	virtual ~ReceivePage();

protected:
	virtual LRESULT onInitDialog(HWND hwndFocus,
								 LPARAM lParam);

protected:
	virtual LRESULT onOk();

private:
	ReceivePage(const ReceivePage&);
	ReceivePage& operator=(const ReceivePage&);

private:
	qm::SubAccount* pSubAccount_;
};


/****************************************************************************
 *
 * SendPage
 *
 */

class SendPage : public qs::DefaultPropertyPage
{
public:
	explicit SendPage(qm::SubAccount* pSubAccount);
	virtual ~SendPage();

protected:
	virtual LRESULT onInitDialog(HWND hwndFocus,
								 LPARAM lParam);

protected:
	virtual LRESULT onOk();

private:
	SendPage(const SendPage&);
	SendPage& operator=(const SendPage&);

private:
	qm::SubAccount* pSubAccount_;
};


/****************************************************************************
 *
 * SubscribeDialog
 *
 */

class SubscribeDialog :
	public qs::DefaultDialog,
	public qs::NotifyHandler
{
public:
	SubscribeDialog(qm::Document* pDocument,
					qm::Account* pAccount,
					qm::PasswordCallback* pPasswordCallback);
	virtual ~SubscribeDialog();

public:
	const WCHAR* getGroup() const;

protected:
	virtual LRESULT onDestroy();
	virtual LRESULT onInitDialog(HWND hwndFocus,
								 LPARAM lParam);

protected:
	virtual LRESULT onOk();

public:
	virtual INT_PTR dialogProc(UINT uMsg,
							   WPARAM wParam,
							   LPARAM lParam);

protected:
	LRESULT onTimer(UINT_PTR nId);

public:
	virtual LRESULT onCommand(WORD nCode,
							  WORD nId);

private:
	LRESULT onFilterChange();
	LRESULT onRefresh();

public:
	virtual LRESULT onNotify(NMHDR* pnmhdr,
							 bool* pbHandled);

private:
	LRESULT onGetDispInfo(NMHDR* pnmhdr,
						  bool* pbHandled);
	LRESULT onItemChanged(NMHDR* pnmhdr,
						  bool* pbHandled);

private:
	void refresh();
	bool refreshGroup();
	void updateState();

private:
	SubscribeDialog(const SubscribeDialog&);
	SubscribeDialog& operator=(const SubscribeDialog&);

private:
	enum {
		TIMERID	= 10,
		TIMEOUT	= 300
	};

private:
	typedef std::vector<const Group*> GroupList;

private:
	qm::Document* pDocument_;
	qm::Account* pAccount_;
	qm::PasswordCallback* pPasswordCallback_;
	qs::wstring_ptr wstrGroup_;
	std::auto_ptr<Groups> pGroups_;
	GroupList listGroup_;
	UINT_PTR nTimerId_;
};

}

#endif // __UI_H__
