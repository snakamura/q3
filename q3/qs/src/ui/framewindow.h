/*
 * $Id: framewindow.h,v 1.1.1.1 2003/04/29 08:07:35 snakamura Exp $
 *
 * Copyright(C) 1998-2003 Satoshi Nakamura
 * All rights reserved.
 *
 */

#ifndef __FRAMEWINDOW_H__
#define __FRAMEWINDOW_H__

#include <qswindow.h>


namespace qs {

#if defined _WIN32_WCE && _WIN32_WCE >= 300 && defined _WIN32_WCE_PSPC
/****************************************************************************
 *
 * MenuBarWindow
 *
 */

class MenuBarWindow : public WindowBase, public DefaultWindowHandler
{
public:
	MenuBarWindow(HWND hwndFrame, QSTATUS* pstatus);
	virtual ~MenuBarWindow();

public:
	virtual LRESULT windowProc(UINT uMsg, WPARAM wParam, LPARAM lParam);

protected:
	LRESULT onInitMenuPopup(HMENU hmenu, UINT nIndex, bool bSysMenu);

private:
	MenuBarWindow(const MenuBarWindow&);
	MenuBarWindow& operator=(const MenuBarWindow&);

private:
	HWND hwndFrame_;
};
#endif

}

#endif // __FRAMEWINDOW_H__
