/*
 * $Id: uiutil.cpp,v 1.2 2003/05/18 04:43:46 snakamura Exp $
 *
 * Copyright(C) 1998-2003 Satoshi Nakamura
 * All rights reserved.
 *
 */

#include <qmfolder.h>

#include <qsconv.h>

#include <tchar.h>

#include "uiutil.h"

using namespace qm;
using namespace qs;


/****************************************************************************
 *
 * UIUtil
 *
 */

QSTATUS qm::UIUtil::formatMenu(const WCHAR* pwszText, WSTRING* pwstrText)
{
	assert(pwszText);
	assert(pwstrText);
	
	DECLARE_QSTATUS();
	
	StringBuffer<WSTRING> buf(&status);
	CHECK_QSTATUS();
	status = buf.append(L'&');
	CHECK_QSTATUS();
	while (*pwszText) {
		if (*pwszText == L'&') {
			status = buf.append(L'&');
			CHECK_QSTATUS();
		}
		status = buf.append(*pwszText);
		CHECK_QSTATUS();
		++pwszText;
	}
	
	*pwstrText = buf.getString();
	
	return QSTATUS_SUCCESS;
}

QSTATUS qm::UIUtil::openURL(HWND hwnd, const WCHAR* pwszURL)
{
	assert(pwszURL);
	
	DECLARE_QSTATUS();
	
	W2T(pwszURL, ptszURL);
	
	SHELLEXECUTEINFO info = {
		sizeof(info),
		0,
		hwnd,
		_T("open"),
		ptszURL,
		0,
		0,
		SW_SHOW
	};
	::ShellExecuteEx(&info);
	
	return QSTATUS_SUCCESS;
}

bool qm::UIUtil::isShowFolder(Folder* pFolder)
{
	Folder* p = pFolder;
	while (p) {
		if (p->isFlag(Folder::FLAG_HIDE))
			return false;
		p = p->getParentFolder();
	}
	return true;
}
