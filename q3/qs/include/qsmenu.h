/*
 * $Id$
 *
 * Copyright(C) 1998-2005 Satoshi Nakamura
 * All rights reserved.
 *
 */

#ifndef __QSMENU_H__
#define __QSMENU_H__

#include <qs.h>
#include <qsstring.h>


namespace qs {

class MenuManager;
class PopupMenu;
	class LoadMenuPopupMenu;
class PopupMenuManager;

struct ActionItem;


/****************************************************************************
 *
 * MenuManager
 *
 */

class QSEXPORTCLASS MenuManager
{
public:
	/**
	 * Create instance.
	 *
	 * @param pwszPath [in] Path to the file where data will be loaded from.
	 * @param pItem [in] Action items.
	 * @param nItemCount [in] Count of action items.
	 * @param popupMenuManager [in] PopupMenuManager which creates popup menus.
	 * @exception std::bad_alloc Out of memory.
	 */
	MenuManager(const WCHAR* pwszPath,
				const ActionItem* pItem,
				size_t nItemCount,
				const PopupMenuManager& popupMenuManager);
	
	~MenuManager();

public:
	/**
	 * Create menu.
	 *
	 * @param pwszName [in] Menu name.
	 * @param bBar [in] true if create menubar, false if create popup menu.
	 * @param bClone [in] true if clone menu handle.
	 * @return Created menu.
	 */
	HMENU getMenu(const WCHAR* pwszName,
				  bool bBar,
				  bool bClone) const;

public:
	/**
	 * Add menu handle.
	 *
	 * @param wstrName [in] Menu name.
	 * @param hmenu [in] Menu.
	 * @param bBar [in] true if menu is menubar, false if menu is popup menu.
	 * @exception std::bad_alloc Out of memory.
	 */
	void add(wstring_ptr wstrName,
			 HMENU hmenu,
			 bool bBar);

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
	/**
	 * Create popup menu.
	 *
	 * @return Created menu. null if error occured.
	 */
	virtual HMENU create() const = 0;
};


/****************************************************************************
 *
 * LoadMenuPopupMenu
 *
 */

class QSEXPORTCLASS LoadMenuPopupMenu : public PopupMenu
{
public:
	/**
	 * Create instance.
	 *
	 * @param hInst [in] Instance handle where menu resource exists.
	 * @param nId [in] ID of menu.
	 */
	LoadMenuPopupMenu(HINSTANCE hInst,
					  UINT nId);
	
	virtual ~LoadMenuPopupMenu();

public:
	virtual HMENU create() const;

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
	/**
	 * Create instance.
	 *
	 * @exception std::bad_alloc Out of memory.
	 */
	PopupMenuManager();
	
	~PopupMenuManager();

public:
	/**
	 * Add popup menu.
	 *
	 * @param pwszName [in] Action name.
	 * @param pPopupMenu [in] Popup menu.
	 * @exception std::bad_alloc Out of memory.
	 */
	void addPopupMenu(const WCHAR* pwszName,
					  const PopupMenu* pPopupMenu);

public:
	/**
	 * Create sub menu.
	 * @param pwszAction [in] Action name.
	 * @return Created menu. null if action is not found or error occured.
	 */
	HMENU createSubMenu(const WCHAR* pwszAction) const;

private:
	PopupMenuManager(const PopupMenuManager&);
	PopupMenuManager& operator=(const PopupMenuManager&);

private:
	struct PopupMenuManagerImpl* pImpl_;
};

}

#endif // __QSMENU_H__
