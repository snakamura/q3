/*
 * $Id: uiutil.h,v 1.2 2003/05/18 04:43:46 snakamura Exp $
 *
 * Copyright(C) 1998-2003 Satoshi Nakamura
 * All rights reserved.
 *
 */

#ifndef __UIUTIL_H__
#define __UIUTIL_H__

#include <qm.h>

#include <qs.h>
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
	static qs::QSTATUS formatMenu(const WCHAR* pwszText, qs::WSTRING* pwstrText);
	static qs::QSTATUS openURL(HWND hwnd, const WCHAR* pwszURL);
	
	static bool isShowFolder(Folder* pFolder);
};

}

#endif // __UIUTIL_H__
