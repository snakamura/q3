/*
 * $Id: menu.cpp,v 1.2 2003/05/18 02:52:36 snakamura Exp $
 *
 * Copyright(C) 1998-2003 Satoshi Nakamura
 * All rights reserved.
 *
 */

#include <qsconv.h>
#include <qsmenu.h>
#include <qsnew.h>
#include <qsstl.h>

#include <algorithm>

#include "menu.h"

#pragma warning(disable:4786)

using namespace qs;


/****************************************************************************
 *
 * MenuManagerImpl
 *
 */

struct qs::MenuManagerImpl
{
	struct Menu
	{
		WSTRING wstrName_;
		HMENU hmenu_;
		bool bBar_;
	};
	
	typedef std::vector<Menu> MenuMap;
	
	QSTATUS load(const WCHAR* pwszPath,
		const MenuItem* pMenuItem, size_t nMenuItemCount,
		const PopupMenuManager& popupMenuManager);
	
	static QSTATUS cloneMenu(HMENU hmenu, bool bBar, HMENU* phmenu);
	
	MenuManager* pThis_;
	MenuMap mapMenu_;
};

QSTATUS qs::MenuManagerImpl::load(const WCHAR* pwszPath,
	const MenuItem* pMenuItem, size_t nMenuItemCount,
	const PopupMenuManager& popupMenuManager)
{
	assert(pwszPath);
	
	DECLARE_QSTATUS();
	
	XMLReader reader(&status);
	CHECK_QSTATUS();
	MenuContentHandler handler(pThis_, pMenuItem,
		nMenuItemCount, popupMenuManager, &status);
	CHECK_QSTATUS();
	reader.setContentHandler(&handler);
	status = reader.parse(pwszPath);
	CHECK_QSTATUS();
	
	return QSTATUS_SUCCESS;
}

QSTATUS qs::MenuManagerImpl::cloneMenu(HMENU hmenu, bool bBar, HMENU* phmenu)
{
	DECLARE_QSTATUS();
	
	HMENU hmenuNew = bBar ? ::CreateMenu() : ::CreatePopupMenu();
	if (!hmenuNew)
		return QSTATUS_FAIL;
	
	TCHAR tszText[256];
	UINT n = 0;
	while (true) {
		MENUITEMINFO mii = {
			sizeof(mii),
			/*MIIM_FTYPE |*/ MIIM_ID | MIIM_STATE |
			/*MIIM_STRING |*/ MIIM_SUBMENU | MIIM_TYPE,
			0,
			0,
			0,
			0,
			0,
			0,
			0,
			tszText,
			countof(tszText) - 1
		};
		if (!::GetMenuItemInfo(hmenu, n, TRUE, &mii))
			break;
		
		if (mii.hSubMenu) {
			HMENU hmenuSub = 0;
			status = cloneMenu(mii.hSubMenu, false, &hmenuSub);
			CHECK_QSTATUS();
			::AppendMenu(hmenuNew, MF_POPUP,
				reinterpret_cast<UINT_PTR>(hmenuSub), tszText);
		}
		else {
			UINT nFlags = mii.fType == MFT_STRING ? MF_STRING : MF_SEPARATOR;
			::AppendMenu(hmenuNew, nFlags, mii.wID, tszText);
		}
		
		++n;
	}
	
	*phmenu = hmenuNew;
	
	return 0;
}


/****************************************************************************
 *
 * MenuManager
 *
 */

qs::MenuManager::MenuManager(const WCHAR* pwszPath,
	const MenuItem* pMenuItem, size_t nMenuItemCount,
	const PopupMenuManager& popupMenuManager, QSTATUS* pstatus) :
	pImpl_(0)
{
	assert(pwszPath);
	assert(pMenuItem);
	assert(pstatus);
	
	DECLARE_QSTATUS();
	
	*pstatus = QSTATUS_SUCCESS;
	
	status = newObject(&pImpl_);
	CHECK_QSTATUS_SET(pstatus);
	pImpl_->pThis_ = this;
	
	status = pImpl_->load(pwszPath, pMenuItem,
		nMenuItemCount, popupMenuManager);
	CHECK_QSTATUS_SET(pstatus);
}

qs::MenuManager::~MenuManager()
{
	if (pImpl_) {
		MenuManagerImpl::MenuMap::const_iterator it = pImpl_->mapMenu_.begin();
		while (it != pImpl_->mapMenu_.end()) {
			freeWString((*it).wstrName_);
			::DestroyMenu((*it).hmenu_);
			++it;
		}
		delete pImpl_;
	}
}

QSTATUS qs::MenuManager::getMenu(const WCHAR* pwszName,
	bool bBar, bool bClone, HMENU* phmenu) const
{
	DECLARE_QSTATUS();
	
	MenuManagerImpl::MenuMap::const_iterator it = std::find_if(
		pImpl_->mapMenu_.begin(), pImpl_->mapMenu_.end(),
		std::bind2nd(
			binary_compose_f_gx_hy(
				string_equal<WCHAR>(),
				mem_data_ref(&MenuManagerImpl::Menu::wstrName_),
				std::identity<const WCHAR*>()),
			pwszName));
	if (it == pImpl_->mapMenu_.end() || (*it).bBar_ != bBar)
		return 0;
	
	if (bClone) {
		status = MenuManagerImpl::cloneMenu((*it).hmenu_, bBar, phmenu);
		CHECK_QSTATUS();
	}
	else {
		*phmenu = (*it).hmenu_;
	}
	
	return QSTATUS_SUCCESS;
}

QSTATUS qs::MenuManager::add(WSTRING wstrName, HMENU hmenu, bool bBar)
{
	MenuManagerImpl::Menu menu = {
		wstrName,
		hmenu,
		bBar
	};
	return STLWrapper<MenuManagerImpl::MenuMap>(
		pImpl_->mapMenu_).push_back(menu);
}


/****************************************************************************
 *
 * PopupMenu
 *
 */

qs::PopupMenu::~PopupMenu()
{
}


/****************************************************************************
 *
 * LoadMenuPopupMenu
 *
 */

qs::LoadMenuPopupMenu::LoadMenuPopupMenu(
	HINSTANCE hInst, UINT nId, qs::QSTATUS* pstatus) :
	hInst_(hInst),
	nId_(nId)
{
}

qs::LoadMenuPopupMenu::~LoadMenuPopupMenu()
{
}

qs::QSTATUS qs::LoadMenuPopupMenu::create(HMENU* phmenu) const
{
	HMENU hmenu = ::LoadMenu(hInst_, MAKEINTRESOURCE(nId_));
	if (!hmenu)
		return QSTATUS_FAIL;
	*phmenu = ::GetSubMenu(hmenu, 0);
	assert(*phmenu);
	::RemoveMenu(hmenu, 0, MF_BYPOSITION);
	::DestroyMenu(hmenu);
	return QSTATUS_SUCCESS;
}


/****************************************************************************
 *
 * PopupMenuManagerImpl
 *
 */

struct qs::PopupMenuManagerImpl
{
	typedef std::vector<std::pair<WSTRING, const PopupMenu*> > PopupMenuMap;
	
	PopupMenuMap mapPopupMenu_;
};


/****************************************************************************
 *
 * PopupMenuManager
 *
 */

qs::PopupMenuManager::PopupMenuManager(QSTATUS* pstatus) :
	pImpl_(0)
{
	DECLARE_QSTATUS();
	
	status = newObject(&pImpl_);
	CHECK_QSTATUS_SET(pstatus);
}

qs::PopupMenuManager::~PopupMenuManager()
{
	if (pImpl_) {
		std::for_each(
			pImpl_->mapPopupMenu_.begin(), pImpl_->mapPopupMenu_.end(),
			unary_compose_f_gx(
				string_free<WSTRING>(),
				std::select1st<PopupMenuManagerImpl::PopupMenuMap::value_type>()));
		delete pImpl_;
	}
}

QSTATUS qs::PopupMenuManager::addPopupMenu(
	const WCHAR* pwszName, const PopupMenu* pPopupMenu)
{
	DECLARE_QSTATUS();
	
	string_ptr<WSTRING> wstrName(allocWString(pwszName));
	if (!wstrName.get())
		return QSTATUS_OUTOFMEMORY;
	
	typedef PopupMenuManagerImpl::PopupMenuMap Map;
	status = STLWrapper<Map>(pImpl_->mapPopupMenu_).push_back(
		Map::value_type(wstrName.get(), pPopupMenu));
	CHECK_QSTATUS();
	wstrName.release();
	
	return QSTATUS_SUCCESS;
}

QSTATUS qs::PopupMenuManager::createSubMenu(
	const WCHAR* pwszAction, HMENU* phmenu) const
{
	PopupMenuManagerImpl::PopupMenuMap::const_iterator it = std::find_if(
		pImpl_->mapPopupMenu_.begin(), pImpl_->mapPopupMenu_.end(),
		std::bind2nd(
			binary_compose_f_gx_hy(
				string_equal<WCHAR>(),
				std::select1st<PopupMenuManagerImpl::PopupMenuMap::value_type>(),
				std::identity<const WCHAR*>()),
			pwszAction));
	if (it == pImpl_->mapPopupMenu_.end())
		return QSTATUS_FAIL;
	else
		return (*it).second->create(phmenu);
}


/****************************************************************************
 *
 * MenuContentHandler
 *
 */

qs::MenuContentHandler::MenuContentHandler(MenuManager* pMenuManager,
	const MenuItem* pMenuItem, size_t nMenuItemCount,
	const PopupMenuManager& popupMenuManager, QSTATUS* pstatus) :
	DefaultHandler(pstatus),
	pMenuManager_(pMenuManager),
	pMenuItem_(pMenuItem),
	nMenuItemCount_(nMenuItemCount),
	popupMenuManager_(popupMenuManager)
{
	assert(pstatus);
	
	DECLARE_QSTATUS();
	
	*pstatus = QSTATUS_SUCCESS;
	
	status = STLWrapper<StateStack>(stackState_).push_back(STATE_ROOT);
	CHECK_QSTATUS_SET(pstatus);
}

qs::MenuContentHandler::~MenuContentHandler()
{
}

QSTATUS qs::MenuContentHandler::startElement(
	const WCHAR* pwszNamespaceURI, const WCHAR* pwszLocalName,
	const WCHAR* pwszQName, const Attributes& attributes)
{
	assert(!stackState_.empty());
	
	DECLARE_QSTATUS();
	
	if (wcscmp(pwszLocalName, L"menus") == 0) {
		if (stackState_.back() != STATE_ROOT)
			return QSTATUS_FAIL;
		if (attributes.getLength() != 0)
			return QSTATUS_FAIL;
		status = STLWrapper<StateStack>(stackState_).push_back(STATE_MENUS);
		CHECK_QSTATUS();
	}
	else if (wcscmp(pwszLocalName, L"menu") == 0 ||
		wcscmp(pwszLocalName, L"menubar") == 0) {
		if (stackState_.back() != STATE_MENUS)
			return QSTATUS_FAIL;
		
		bool bBar = wcscmp(pwszLocalName, L"menubar") == 0;
		
		const WCHAR* pwszName = 0;
		for (int n = 0; n < attributes.getLength(); ++n) {
			const WCHAR* pwszAttrName = attributes.getLocalName(n);
			if (wcscmp(pwszAttrName, L"name") == 0)
				pwszName = attributes.getValue(n);
			else
				return QSTATUS_FAIL;
		}
		if (!pwszName)
			return QSTATUS_FAIL;
		
		string_ptr<WSTRING> wstrName(allocWString(pwszName));
		if (!wstrName.get())
			return QSTATUS_OUTOFMEMORY;
		
		HMENU hmenu = bBar ? ::CreateMenu() : ::CreatePopupMenu();
		if (!hmenu)
			return QSTATUS_FAIL;
		
		struct Deleter
		{
			Deleter(HMENU hmenu) : hmenu_(hmenu) {}
			~Deleter()
			{
				if (hmenu_)
					::DestroyMenu(hmenu_);
			}
			void release() { hmenu_ = 0; }
			HMENU hmenu_;
		} deleter(hmenu);
		
		status = pMenuManager_->add(wstrName.get(), hmenu, bBar);
		CHECK_QSTATUS();
		wstrName.release();
		deleter.release();
		
		status = STLWrapper<MenuStack>(stackMenu_).push_back(hmenu);
		CHECK_QSTATUS();
		
		status = STLWrapper<StateStack>(stackState_).push_back(STATE_MENU);
		CHECK_QSTATUS();
	}
	else if (wcscmp(pwszLocalName, L"menuitem") == 0) {
		if (stackState_.back() != STATE_MENU)
			return QSTATUS_FAIL;
		
		const WCHAR* pwszText = 0;
		const WCHAR* pwszAction = 0;
		for (int n = 0; n < attributes.getLength(); ++n) {
			const WCHAR* pwszAttrName = attributes.getLocalName(n);
			if (wcscmp(pwszAttrName, L"text") == 0)
				pwszText = attributes.getValue(n);
			else if (wcscmp(pwszAttrName, L"action") == 0)
				pwszAction = attributes.getValue(n);
			else
				return QSTATUS_FAIL;
		}
		if (!pwszText || !pwszAction)
			return QSTATUS_FAIL;
		
		assert(!stackMenu_.empty());
		HMENU hmenu = stackMenu_.back();
		
		W2T(pwszText, ptszText);
		
		UINT nId = getActionId(pwszAction);
		if (nId != -1)
			::AppendMenu(hmenu, MF_STRING, nId, ptszText);
		
		status = STLWrapper<StateStack>(stackState_).push_back(STATE_MENUITEM);
		CHECK_QSTATUS();
	}
	else if (wcscmp(pwszLocalName, L"separator") == 0) {
		if (stackState_.back() != STATE_MENU)
			return QSTATUS_FAIL;
		if (attributes.getLength() != 0)
			return QSTATUS_FAIL;
		
		assert(!stackMenu_.empty());
		HMENU hmenu = stackMenu_.back();
		::AppendMenu(hmenu, MF_SEPARATOR, -1, 0);
		
		status = STLWrapper<StateStack>(stackState_).push_back(STATE_SEPARATOR);
		CHECK_QSTATUS();
	}
	else if (wcscmp(pwszLocalName, L"popupmenu") == 0) {
		if (stackState_.back() != STATE_MENU)
			return QSTATUS_FAIL;
		
		const WCHAR* pwszText = 0;
		const WCHAR* pwszAction = 0;
		for (int n = 0; n < attributes.getLength(); ++n) {
			const WCHAR* pwszAttrName = attributes.getLocalName(n);
			if (wcscmp(pwszAttrName, L"text") == 0)
				pwszText = attributes.getValue(n);
			else if (wcscmp(pwszAttrName, L"action") == 0)
				pwszAction = attributes.getValue(n);
			else
				return QSTATUS_FAIL;
		}
		if (!pwszText)
			return QSTATUS_FAIL;
		
		assert(!stackMenu_.empty());
		HMENU hmenu = stackMenu_.back();
		W2T(pwszText, ptszText);
		
		State state = STATE_MENU;
		HMENU hmenuSub = 0;
		if (pwszAction) {
			status = popupMenuManager_.createSubMenu(pwszAction, &hmenuSub);
			CHECK_QSTATUS();
			state = STATE_POPUPMENU;
		}
		else {
			hmenuSub = ::CreatePopupMenu();
			if (!hmenuSub)
				return QSTATUS_FAIL;
			state = STATE_MENU;
		}
		::AppendMenu(hmenu, MF_POPUP,
			reinterpret_cast<UINT_PTR>(hmenuSub), ptszText);
		
		if (state == STATE_MENU) {
			status = STLWrapper<MenuStack>(stackMenu_).push_back(hmenuSub);
			CHECK_QSTATUS();
		}
		
		status = STLWrapper<StateStack>(stackState_).push_back(state);
		CHECK_QSTATUS();
	}
	else {
		return QSTATUS_FAIL;
	}
	
	return QSTATUS_SUCCESS;
}

QSTATUS qs::MenuContentHandler::endElement(
	const WCHAR* pwszNamespaceURI, const WCHAR* pwszLocalName,
	const WCHAR* pwszQName)
{
	assert(!stackState_.empty());
	
	DECLARE_QSTATUS();
	
	if (wcscmp(pwszLocalName, L"menus") == 0) {
		assert(stackState_.back() == STATE_MENUS);
		stackState_.pop_back();
	}
	else if (wcscmp(pwszLocalName, L"menu") == 0 ||
		wcscmp(pwszLocalName, L"menubar") == 0) {
		assert(stackState_.back() == STATE_MENU);
		stackMenu_.pop_back();
		assert(stackMenu_.empty());
		stackState_.pop_back();
	}
	else if (wcscmp(pwszLocalName, L"menuitem") == 0) {
		assert(stackState_.back() == STATE_MENUITEM);
		stackState_.pop_back();
	}
	else if (wcscmp(pwszLocalName, L"separator") == 0) {
		assert(stackState_.back() == STATE_SEPARATOR);
		stackState_.pop_back();
	}
	else if (wcscmp(pwszLocalName, L"popupmenu") == 0) {
		assert(stackState_.back() == STATE_MENU ||
			stackState_.back() == STATE_POPUPMENU);
		if (stackState_.back() == STATE_MENU)
			stackMenu_.pop_back();
		assert(!stackMenu_.empty());
		stackState_.pop_back();
	}
	else {
		return QSTATUS_FAIL;
	}
	
	return QSTATUS_SUCCESS;
}

QSTATUS qs::MenuContentHandler::characters(
	const WCHAR* pwsz, size_t nStart, size_t nLength)
{
	assert(!stackState_.empty());
	
	const WCHAR* p = pwsz + nStart;
	for (size_t n = 0; n < nLength; ++n, ++p) {
		if (*p != L' ' && *p != L'\t' && *p != '\n')
			return QSTATUS_FAIL;
	}
	
	return QSTATUS_SUCCESS;
}

UINT qs::MenuContentHandler::getActionId(const WCHAR* pwszAction)
{
	UINT nId = -1;
	for (size_t n = 0; n < nMenuItemCount_ && nId == -1; ++n) {
		if (wcscmp((pMenuItem_ + n)->pwszAction_, pwszAction) == 0)
			nId = (pMenuItem_ + n)->nId_;
	}
	return nId;
}
