/*
 * $Id$
 *
 * Copyright(C) 1998-2003 Satoshi Nakamura
 * All rights reserved.
 *
 */

#ifndef __UI_H__
#define __UI_H__

#include <qmaccount.h>

#include <qsdialog.h>


namespace qmsmtp {

/****************************************************************************
 *
 * SendPage
 *
 */

class SendPage : public qs::DefaultPropertyPage
{
public:
	SendPage(qm::SubAccount* pSubAccount, qs::QSTATUS* pstatus);
	virtual ~SendPage();

protected:
	virtual LRESULT onInitDialog(HWND hwndFocus, LPARAM lParam);

protected:
	virtual LRESULT onOk();

private:
	SendPage(const SendPage&);
	SendPage& operator=(const SendPage&);

private:
	qm::SubAccount* pSubAccount_;
};

}

#endif // __UI_H__
