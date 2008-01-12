/*
 * $Id$
 *
 * Copyright(C) 1998-2008 Satoshi Nakamura
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
	ReceivePage(qm::SubAccount* pSubAccount);
	virtual ~ReceivePage();

protected:
	virtual LRESULT onInitDialog(HWND hwndFocus,
								 LPARAM lParam);

protected:
	virtual LRESULT onOk();

public:
	virtual LRESULT onCommand(WORD nCode,
							  WORD nId);

private:
	LRESULT onNamespace();

private:
	void updateState();

private:
	ReceivePage(const ReceivePage&);
	ReceivePage& operator=(const ReceivePage&);

private:
	qm::SubAccount* pSubAccount_;
};

}

#endif // __UI_H__
