/*
 * $Id$
 *
 * Copyright(C) 1998-2008 Satoshi Nakamura
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
					   const ActionItem* pItem,
					   size_t nItemCount,
					   ActionParamMap* pActionParamMap,
					   DynamicMenuMap* pDynamicMenuMap);
	virtual ~MenuContentHandler();

public:
	virtual bool startElement(const WCHAR* pwszNamespaceURI,
							  const WCHAR* pwszLocalName,
							  const WCHAR* pwszQName,
							  const Attributes& attributes);
	virtual bool endElement(const WCHAR* pwszNamespaceURI,
							const WCHAR* pwszLocalName,
							const WCHAR* pwszQName);
	virtual bool characters(const WCHAR* pwsz,
							size_t nStart,
							size_t nLength);

private:
	const ActionItem* getActionItem(const WCHAR* pwszAction) const;

private:
	MenuContentHandler(const MenuContentHandler&);
	MenuContentHandler& operator=(const MenuContentHandler&);

private:
	enum State {
		STATE_ROOT,
		STATE_MENUS,
		STATE_MENU,
		STATE_MENUITEM,
		STATE_SEPARATOR
	};

private:
	typedef std::vector<State> StateStack;
	typedef std::vector<HMENU> MenuStack;

private:
	MenuManager* pMenuManager_;
	const ActionItem* pActionItem_;
	size_t nActionItemCount_;
	ActionParamMap* pActionParamMap_;
	DynamicMenuMap* pDynamicMenuMap_;
	StateStack stackState_;
	MenuStack stackMenu_;
};

}

#endif // __MENU_H__
