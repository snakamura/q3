/*
 * $Id$
 *
 * Copyright(C) 1998-2004 Satoshi Nakamura
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
	/**
	 * Create instance.
	 *
	 * @param pszPath [in] Path to the file which data will be loaded from.
	 * @param hBitmap [in] Toolbar image.
	 * @param pItem [in] Action items.
	 * @param nItemCount [in] Count of action items.
	 * @exception std::bad_alloc Out of memory.
	 */
	ToolbarManager(const WCHAR* pwszPath,
				   HBITMAP hBitmap,
				   const ActionItem* pItem,
				   size_t nItemCount);
	
	~ToolbarManager();

public:
	/**
	 * Create toolbar.
	 *
	 * @param pwszName [in] Name of toolbar.
	 * @param hwnd [in] Window handle of the parent window of toolbar.
	 * @return true if success, false otherwise.
	 */
	bool createToolbar(const WCHAR* pwszName,
					   HWND hwnd) const;

private:
	ToolbarManager(const ToolbarManager&);
	ToolbarManager& operator=(const ToolbarManager&);

private:
	struct ToolbarManagerImpl* pImpl_;
};

}

#endif // __QSTOOLBAR_H__
