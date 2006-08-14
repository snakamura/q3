/*
 * $Id$
 *
 * Copyright(C) 1998-2006 Satoshi Nakamura
 * All rights reserved.
 *
 */

#ifndef __QSMENU_H__
#define __QSMENU_H__

#include <qs.h>
#include <qsstring.h>


namespace qs {

class MenuManager;
class DynamicMenuCreator;
class DynamicMenuItem;
class DynamicMenuMap;

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
				ActionParamMap* pActionParamMap,
				DynamicMenuMap* pDynamicMenuMap);
	
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
 * DynamicMenuCreator
 *
 */

class QSEXPORTCLASS DynamicMenuCreator
{
public:
	virtual ~DynamicMenuCreator();

public:
	virtual UINT createMenu(HMENU hmenu,
							UINT nIndex,
							const DynamicMenuItem* pItem) = 0;
};


/****************************************************************************
 *
 * DynamicMenuItem
 *
 */

#pragma warning(push)
#pragma warning(disable:4251)

class QSEXPORTCLASS DynamicMenuItem
{
public:
	DynamicMenuItem(unsigned int nId,
					const WCHAR* pwszName,
					const WCHAR* pwszParam);
	virtual ~DynamicMenuItem();

public:
	unsigned int getId() const;
	const WCHAR* getName() const;
	const WCHAR* getParam() const;

private:
	DynamicMenuItem(const DynamicMenuItem&);
	DynamicMenuItem& operator=(const DynamicMenuItem&);

private:
	unsigned int nId_;
	wstring_ptr wstrName_;
	wstring_ptr wstrParam_;
};

#pragma warning(pop)


/****************************************************************************
 *
 * DynamicMenuMap
 *
 */

#pragma warning(push)
#pragma warning(disable:4251)

class QSEXPORTCLASS DynamicMenuMap
{
public:
	DynamicMenuMap();
	virtual ~DynamicMenuMap();

public:
	const DynamicMenuItem* getItem(unsigned int nId) const;
	unsigned int addItem(const WCHAR* pwszName,
						 const WCHAR* pwszParam);

protected:
	virtual std::auto_ptr<DynamicMenuItem> createItem(unsigned int nId,
													  const WCHAR* pwszName,
													  const WCHAR* pwszParam) const;

private:
	DynamicMenuMap(const DynamicMenuMap&);
	DynamicMenuMap& operator=(const DynamicMenuMap&);

private:
	typedef std::vector<DynamicMenuItem*> ItemList;

private:
	ItemList listItem_;
};

#pragma warning(pop)

}

#endif // __QSMENU_H__
