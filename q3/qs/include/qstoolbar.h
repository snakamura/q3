/*
 * $Id$
 *
 * Copyright(C) 1998-2005 Satoshi Nakamura
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
	 * @param pMenuManager [in] Menu manager to get drop down menus.
	 * @exception std::bad_alloc Out of memory.
	 */
	ToolbarManager(const WCHAR* pwszPath,
				   HBITMAP hBitmap,
				   const ActionItem* pItem,
				   size_t nItemCount,
				   const MenuManager* pMenuManager);
	
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

private:
	ToolbarCookie();

public:
#ifndef _WIN32_WCE
	ToolbarCookie(const WCHAR* pwszName,
				  WindowBase* pParent,
				  std::auto_ptr<NotifyHandler> pNotifyHandler);
#else
	ToolbarCookie(const WCHAR* pwszName,
				  WindowBase* pParent,
				  std::auto_ptr<NotifyHandler> pNotifyHandler,
				  ToolTipList& listToolTip);
#endif
	~ToolbarCookie();

public:
	const WCHAR* getName() const;
	WindowBase* getParent() const;
	NotifyHandler* getNotifyHandler() const;

private:
	ToolbarCookie(const ToolbarCookie&);
	ToolbarCookie& operator=(const ToolbarCookie&);

private:
	wstring_ptr wstrName_;
	WindowBase* pParent_;
	std::auto_ptr<NotifyHandler> pNotifyHandler_;
#ifdef _WIN32_WCE
	ToolTipList listToolTip_;
#endif

public:
	static ToolbarCookie none__;
};

}

#endif // __QSTOOLBAR_H__
