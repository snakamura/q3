/*
 * $Id: qsmenu.h,v 1.1.1.1 2003/04/29 08:07:34 snakamura Exp $
 *
 * Copyright(C) 1998-2003 Satoshi Nakamura
 * All rights reserved.
 *
 */

#ifndef __QSMENU_H__
#define __QSMENU_H__

#include <qs.h>
#include <qsstring.h>


namespace qs {

struct MenuItem;
class MenuManager;
class PopupMenu;
	class LoadMenuPopupMenu;
class PopupMenuManager;


/****************************************************************************
 *
 * MenuItem
 *
 */

struct QSEXPORTCLASS MenuItem
{
	const WCHAR* pwszAction_;
	UINT nId_;
};


/****************************************************************************
 *
 * MenuManager
 *
 */

class QSEXPORTCLASS MenuManager
{
public:
	MenuManager(const WCHAR* pwszPath,
		const MenuItem* pMenuItem, size_t nMenuItemCount,
		const PopupMenuManager& popupMenuManager, QSTATUS* pstatus);
	~MenuManager();

public:
	QSTATUS getMenu(const WCHAR* pwszName,
		bool bBar, bool bClone, HMENU* phmenu) const;

public:
	QSTATUS add(WSTRING wstrName, HMENU hmenu, bool bBar);

private:
	MenuManager(const MenuManager&);
	MenuManager& operator=(const MenuManager&);

private:
	struct MenuManagerImpl* pImpl_;
};


/****************************************************************************
 *
 * PopupMenu
 *
 */

class QSEXPORTCLASS PopupMenu
{
public:
	virtual ~PopupMenu();

public:
	virtual QSTATUS create(HMENU* phmenu) const = 0;
};


/****************************************************************************
 *
 * LoadMenuPopupMenu
 *
 */

class QSEXPORTCLASS LoadMenuPopupMenu : public PopupMenu
{
public:
	LoadMenuPopupMenu(HINSTANCE hInst, UINT nId, QSTATUS* pstatus);
	virtual ~LoadMenuPopupMenu();

public:
	virtual QSTATUS create(HMENU* phmenu) const;

private:
	LoadMenuPopupMenu(const LoadMenuPopupMenu&);
	LoadMenuPopupMenu& operator=(const LoadMenuPopupMenu&);

private:
	HINSTANCE hInst_;
	UINT nId_;
};


/****************************************************************************
 *
 * PopupMenuManager
 *
 */

class QSEXPORTCLASS PopupMenuManager
{
public:
	explicit PopupMenuManager(QSTATUS* pstatus);
	~PopupMenuManager();

public:
	QSTATUS addPopupMenu(const WCHAR* pwszName, const PopupMenu* pPopupMenu);

public:
	QSTATUS createSubMenu(const WCHAR* pwszAction, HMENU* phmenu) const;

private:
	PopupMenuManager(const PopupMenuManager&);
	PopupMenuManager& operator=(const PopupMenuManager&);

private:
	struct PopupMenuManagerImpl* pImpl_;
};

}

#endif // __QSMENU_H__
