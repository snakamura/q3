/*
 * $Id$
 *
 * Copyright(C) 1998-2006 Satoshi Nakamura
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
	enum DefaultFont {
		DEFAULTFONT_PROPORTIONAL,
		DEFAULTFONT_FIXED,
		DEFAULTFONT_UI
	};

public:
	static HFONT createFontFromProfile(Profile* pProfile,
									   const WCHAR* pwszSection,
									   DefaultFont defaultFont);
	static void getLogFontFromProfile(Profile* pProfile,
									  const WCHAR* pwszSection,
									  DefaultFont defaultFont,
									  LOGFONT* pLogFont);
	static void setLogFontToProfile(Profile* pProfile,
									const WCHAR* pwszSection,
									const LOGFONT& lf);
	
	static bool isImeEnabled(HWND hwnd);
	static void setImeEnabled(HWND hwnd,
							  bool bEnabled);
	
#ifdef _WIN32_WCE_PSPC
	static bool isSipEnabled();
	static void setSipEnabled(bool bEnabled);
#endif
	
	static bool browseFont(HWND hwnd,
						   LOGFONT* pLogFont);
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
	
#if defined _WIN32_WCE && _WIN32_WCE >= 0x300 && defined _WIN32_WCE_PSPC
	static void getWorkArea(RECT* pRect);
#endif
};

}

#endif // __QSUIUTIL_H__
