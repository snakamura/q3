/*
 * $Id$
 *
 * Copyright(C) 1998-2003 Satoshi Nakamura
 * All rights reserved.
 *
 */

#ifndef __STATUSBAR_H__
#define __STATUSBAR_H__

#include <qm.h>

#include <qswindow.h>


namespace qm {

/****************************************************************************
 *
 * StatusBar
 *
 */

class StatusBar : public qs::WindowBase, qs::DefaultWindowHandler
{
public:
	StatusBar(qs::QSTATUS* pstatus);
	virtual ~StatusBar();

public:
	qs::QSTATUS setParts(int* pnWidth, size_t nCount);
	qs::QSTATUS setText(int n, const WCHAR* pwszText);
	void setSimple(bool bSimple);

public:
	virtual LRESULT windowProc(UINT uMsg, WPARAM wParam, LPARAM lParam);

private:
	StatusBar(const StatusBar&);
	StatusBar& operator=(const StatusBar&);
};

}

#endif // __STATUSBAR_H__
