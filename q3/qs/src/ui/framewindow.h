/*
 * $Id$
 *
 * Copyright(C) 1998-2008 Satoshi Nakamura
 *
 */

#ifndef __FRAMEWINDOW_H__
#define __FRAMEWINDOW_H__

#include <qswindow.h>


namespace qs {

#if defined _WIN32_WCE && _WIN32_WCE >= 0x300 && defined _WIN32_WCE_PSPC
/****************************************************************************
 *
 * MenuBarWindow
 *
 */

class MenuBarWindow :
	public WindowBase,
	public DefaultWindowHandler
{
public:
	MenuBarWindow(HWND hwndFrame);
	virtual ~MenuBarWindow();

public:
	virtual LRESULT windowProc(UINT uMsg,
							   WPARAM wParam,
							   LPARAM lParam);

protected:
	LRESULT onInitMenuPopup(HMENU hmenu,
							UINT nIndex,
							bool bSysMenu);

private:
	MenuBarWindow(const MenuBarWindow&);
	MenuBarWindow& operator=(const MenuBarWindow&);

private:
	HWND hwndFrame_;
};
#endif

}

#endif // __FRAMEWINDOW_H__
