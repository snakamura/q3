/*
 * $Id: statusbar.h,v 1.1.1.1 2003/04/29 08:07:32 snakamura Exp $
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
	virtual LRESULT windowProc(UINT uMsg, WPARAM wParam, LPARAM lParam);

private:
	StatusBar(const StatusBar&);
	StatusBar& operator=(const StatusBar&);
};

}

#endif // __STATUSBAR_H__
