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
class ToolbarCookie;

struct ActionItem;
class NotifyHandler;
class WindowBase;


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
	 * @param hwnd [in] Window handle of the toolbar.
	 * @param pParent [in] WindowBase of the parent of the toolbar.
	 * @return Cookie if success, null otherwise.
	 */
	ToolbarCookie* createButtons(const WCHAR* pwszName,
								 HWND hwnd,
								 WindowBase* pParent) const;
	void destroy(ToolbarCookie* pCookie) const;

private:
	ToolbarManager(const ToolbarManager&);
	ToolbarManager& operator=(const ToolbarManager&);

private:
	struct ToolbarManagerImpl* pImpl_;
};


/****************************************************************************
 *
 * ToolbarCookie
 *
 */

class ToolbarCookie
{
public:
#ifdef _WIN32_WCE
	typedef std::vector<const WCHAR*> ToolTipList;
#endif

public:
#ifndef _WIN32_WCE
	ToolbarCookie(const WCHAR* pwszName,
				  WindowBase* pParent,
				  std::auto_ptr<NotifyHandler> pNotifyHandler);
#else
	ToolbarCookie(const WCHAR* pwszName,
				  ToolTipList& listToolTip);
#endif
	~ToolbarCookie();

public:
	const WCHAR* getName() const;
#ifndef _WIN32_WCE
	WindowBase* getParent() const;
	NotifyHandler* getNotifyHandler() const;
#endif

private:
	ToolbarCookie(const ToolbarCookie&);
	ToolbarCookie& operator=(const ToolbarCookie&);

private:
	wstring_ptr wstrName_;
#ifndef _WIN32_WCE
	WindowBase* pParent_;
	std::auto_ptr<NotifyHandler> pNotifyHandler_;
#else
	ToolTipList listToolTip_;
#endif
};

}

#endif // __QSTOOLBAR_H__
