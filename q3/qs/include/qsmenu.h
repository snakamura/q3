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
struct DynamicMenuItem;

struct ActionItem;
class ActionParamMap;


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
	 * @exception std::bad_alloc Out of memory.
	 */
	MenuManager(const WCHAR* pwszPath,
				const ActionItem* pItem,
				size_t nItemCount,
				const DynamicMenuItem* pDynamicItem,
				size_t nDynamicItemCount,
				ActionParamMap* pActionParamMap);
	
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
 * DynamicMenuItem
 *
 */

struct DynamicMenuItem
{
	const WCHAR* pwszName_;
	UINT nId_;
	DWORD dwMenuItemData_;
};


/****************************************************************************
 *
 * DynamicMenuCreator
 *
 */

class QSEXPORTCLASS DynamicMenuCreator
{
public:
	virtual ~DynamicMenuCreator();

public:
	virtual UINT createMenu(HMENU hmenu,
							UINT nIndex) = 0;
	virtual DWORD getMenuItemData() const = 0;
};



}

#endif // __QSMENU_H__
