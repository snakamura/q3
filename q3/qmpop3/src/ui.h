/*
 * $Id: ui.h,v 1.1.1.1 2003/04/29 08:07:34 snakamura Exp $
 *
 * Copyright(C) 1998-2003 Satoshi Nakamura
 * All rights reserved.
 *
 */

#ifndef __UI_H__
#define __UI_H__

#include <qmaccount.h>

#include <qsdialog.h>


namespace qmpop3 {

/****************************************************************************
 *
 * ReceivePage
 *
 */

class ReceivePage :
	public qs::PropertyPage,
	public qs::DefaultDialogHandler,
	public qs::DefaultCommandHandler,
	public qs::NotifyHandler
{
public:
	ReceivePage(qm::SubAccount* pSubAccount, qs::QSTATUS* pstatus);
	virtual ~ReceivePage();

public:
	virtual INT_PTR dialogProc(UINT uMsg, WPARAM wParam, LPARAM lParam);

public:
	virtual LRESULT onCommand(WORD nCode, WORD nId);

public:
	virtual LRESULT onNotify(NMHDR* pnmhdr, bool* pbHandled);

protected:
	virtual LRESULT onDestroy();
	virtual LRESULT onInitDialog(HWND hwndFocus, LPARAM lParam);

private:
	LRESULT onApply(NMHDR* pnmhdr, bool* pbHandled);

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

class SendPage :
	public qs::PropertyPage,
	public qs::DefaultDialogHandler,
	public qs::DefaultCommandHandler,
	public qs::NotifyHandler
{
public:
	SendPage(qm::SubAccount* pSubAccount, qs::QSTATUS* pstatus);
	virtual ~SendPage();

public:
	virtual INT_PTR dialogProc(UINT uMsg, WPARAM wParam, LPARAM lParam);

public:
	virtual LRESULT onCommand(WORD nCode, WORD nId);

public:
	virtual LRESULT onNotify(NMHDR* pnmhdr, bool* pbHandled);

protected:
	virtual LRESULT onDestroy();
	virtual LRESULT onInitDialog(HWND hwndFocus, LPARAM lParam);

private:
	LRESULT onApply(NMHDR* pnmhdr, bool* pbHandled);

private:
	SendPage(const SendPage&);
	SendPage& operator=(const SendPage&);

private:
	qm::SubAccount* pSubAccount_;
};

}

#endif // __UI_H__
