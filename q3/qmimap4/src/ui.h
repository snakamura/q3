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


namespace qmimap4 {

/****************************************************************************
 *
 * ReceivePage
 *
 */

class ReceivePage : public qs::DefaultPropertyPage
{
public:
	ReceivePage(qm::SubAccount* pSubAccount, qs::QSTATUS* pstatus);
	virtual ~ReceivePage();

protected:
	virtual LRESULT onInitDialog(HWND hwndFocus, LPARAM lParam);

protected:
	virtual LRESULT onOk();

private:
	ReceivePage(const ReceivePage&);
	ReceivePage& operator=(const ReceivePage&);

private:
	qm::SubAccount* pSubAccount_;
};

}

#endif // __UI_H__
