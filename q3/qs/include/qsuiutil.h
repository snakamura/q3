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


namespace qs {

class UIUtil;

class Profile;


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
};

}

#endif // __QSUIUTIL_H__
