/*
 * $Id: qsuiutil.h,v 1.2 2003/06/01 08:40:31 snakamura Exp $
 *
 * Copyright(C) 1998-2003 Satoshi Nakamura
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
	static QSTATUS createFontFromProfile(Profile* pProfile,
		const WCHAR* pwszSection, bool bDefaultFixedWidth, HFONT* phfont);
	static QSTATUS browseFolder(HWND hwnd, const WCHAR* pwszTitle,
		const WCHAR* pwszInitialPath, WSTRING* pwstrPath);
};

}

#endif // __QSUIUTIL_H__
