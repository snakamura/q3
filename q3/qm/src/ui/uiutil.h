/*
 * $Id$
 *
 * Copyright(C) 1998-2003 Satoshi Nakamura
 * All rights reserved.
 *
 */

#ifndef __UIUTIL_H__
#define __UIUTIL_H__

#include <qm.h>

#include <qs.h>
#include <qsprofile.h>
#include <qsstring.h>


namespace qm {

class Folder;


/****************************************************************************
 *
 * UIUtil
 *
 */

class UIUtil
{
public:
	static qs::QSTATUS loadWindowPlacement(qs::Profile* pProfile,
		const WCHAR* pwszSection, CREATESTRUCT* pCreateStruct, int* pnShow);
	static qs::QSTATUS saveWindowPlacement(HWND hwnd,
		qs::Profile* pProfile, const WCHAR* pwszSection);
	
	static qs::QSTATUS formatMenu(const WCHAR* pwszText, qs::WSTRING* pwstrText);
	static qs::QSTATUS openURL(HWND hwnd, const WCHAR* pwszURL);
	
	static bool isShowFolder(Folder* pFolder);
};

}

#endif // __UIUTIL_H__
