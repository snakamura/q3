/*
 * $Id$
 *
 * Copyright(C) 1998-2004 Satoshi Nakamura
 * All rights reserved.
 *
 */

#ifndef __QSUIUTIL_H__
#define __QSUIUTIL_H__

#include <qs.h>
#include <qsstring.h>


namespace qs {

class UIUtil;

class Profile;
class Theme;


/****************************************************************************
 *
 * UIUtil
 *
 */

class QSEXPORTCLASS UIUtil
{
public:
	static HFONT createFontFromProfile(Profile* pProfile,
									   const WCHAR* pwszSection,
									   bool bDefaultFixedWidth);
	static wstring_ptr browseFolder(HWND hwnd,
									const WCHAR* pwszTitle,
									const WCHAR* pwszInitialPath);
#ifndef _WIN32_WCE
	static bool drawThemeBorder(Theme* pTheme,
								HWND hwnd,
								int nPartId,
								int nStateId,
								COLORREF crBackground);
#endif
	static int getLogPixel();
#if defined _WIN32_WCE && _WIN32_WCE >= 300 && defined _WIN32_WCE_PSPC
	static void getWorkArea(RECT* pRect);
#endif
};

}

#endif // __QSUIUTIL_H__
