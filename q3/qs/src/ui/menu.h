/*
 * $Id: menu.h,v 1.1.1.1 2003/04/29 08:07:36 snakamura Exp $
 *
 * Copyright(C) 1998-2003 Satoshi Nakamura
 * All rights reserved.
 *
 */

#ifndef __MENU_H__
#define __MENU_H__

#include <qsmenu.h>
#include <qssax.h>


namespace qs {

/****************************************************************************
 *
 * MenuContentHandler
 *
 */

class MenuContentHandler : public DefaultHandler
{
public:
	MenuContentHandler(MenuManager* pMenuManager,
		const MenuItem* pMenuItem, size_t nMenuItemCount,
		const PopupMenuManager& popupMenuManager, QSTATUS* pstatus);
	virtual ~MenuContentHandler();

public:
	virtual QSTATUS startElement(const WCHAR* pwszNamespaceURI,
		const WCHAR* pwszLocalName, const WCHAR* pwszQName,
		const Attributes& attributes);
	virtual QSTATUS endElement(const WCHAR* pwszNamespaceURI,
		const WCHAR* pwszLocalName, const WCHAR* pwszQName);
	virtual QSTATUS characters(const WCHAR* pwsz,
		size_t nStart, size_t nLength);

private:
	UINT getActionId(const WCHAR* pwszAction);

private:
	MenuContentHandler(const MenuContentHandler&);
	MenuContentHandler& operator=(const MenuContentHandler&);

private:
	enum State {
		STATE_ROOT,
		STATE_MENUS,
		STATE_MENU,
		STATE_MENUITEM,
		STATE_SEPARATOR,
		STATE_POPUPMENU
	};

private:
	typedef std::vector<State> StateStack;
	typedef std::vector<HMENU> MenuStack;

private:
	MenuManager* pMenuManager_;
	const MenuItem* pMenuItem_;
	size_t nMenuItemCount_;
	const PopupMenuManager& popupMenuManager_;
	StateStack stackState_;
	MenuStack stackMenu_;
};

}

#endif // __MENU_H__
