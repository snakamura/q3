/*
 * $Id$
 *
 * Copyright(C) 1998-2003 Satoshi Nakamura
 * All rights reserved.
 *
 */

#ifndef __QSTOOLBAR_H__
#define __QSTOOLBAR_H__

#include <qs.h>


namespace qs {

class ToolbarManager;

struct ActionItem;


/****************************************************************************
 *
 * ToolbarManager
 *
 */

class QSEXPORTCLASS ToolbarManager
{
public:
	ToolbarManager(const WCHAR* pwszPath, HBITMAP hBitmap,
		const ActionItem* pItem, size_t nItemCount, QSTATUS* pstatus);
	~ToolbarManager();

public:
	QSTATUS createToolbar(const WCHAR* pwszName, HWND hwnd) const;

private:
	ToolbarManager(const ToolbarManager&);
	ToolbarManager& operator=(const ToolbarManager&);

private:
	struct ToolbarManagerImpl* pImpl_;
};

}

#endif // __QSTOOLBAR_H__
