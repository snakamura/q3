/*
 * $Id$
 *
 * Copyright(C) 1998-2005 Satoshi Nakamura
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

class StatusBar :
	public qs::WindowBase,
	public qs::DefaultWindowHandler
{
public:
	StatusBar();
	virtual ~StatusBar();

public:
	bool setParts(int* pnWidth,
				 size_t nCount);
	bool setText(int n,
				 const WCHAR* pwszText);
	bool setIcon(int n,
				 HICON hIcon);
	void setSimple(bool bSimple);

public:
	virtual LRESULT windowProc(UINT uMsg,
							   WPARAM wParam,
							   LPARAM lParam);

private:
	StatusBar(const StatusBar&);
	StatusBar& operator=(const StatusBar&);
};

}

#endif // __STATUSBAR_H__
