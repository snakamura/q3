/*
 * $Id$
 *
 * Copyright(C) 1998-2005 Satoshi Nakamura
 * All rights reserved.
 *
 */

#pragma warning(disable:4786)

#include <qsaction.h>
#include <qsconv.h>
#include <qsinit.h>
#include <qslog.h>
#include <qsmenu.h>
#include <qsstl.h>

#include <algorithm>

#include <tchar.h>

#include "menu.h"

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
	
	bool load(const WCHAR* pwszPath,
			  const ActionItem* pItem,
			  size_t nItemCount,
			  const DynamicMenuItem* pDynamicItem,
			  size_t nDynamicItemCount,
			  ActionParamMap* pActionParamMap);
	
	static HMENU cloneMenu(HMENU hmenu,
						   bool bBar);
	
	MenuManager* pThis_;
	MenuMap mapMenu_;
};

bool qs::MenuManagerImpl::load(const WCHAR* pwszPath,
							   const ActionItem* pItem,
							   size_t nItemCount,
							   const DynamicMenuItem* pDynamicItem,
							   size_t nDynamicItemCount,
							   ActionParamMap* pActionParamMap)
{
	assert(pwszPath);
	assert(pActionParamMap);
	
	XMLReader reader;
	MenuContentHandler handler(pThis_, pItem, nItemCount,
		pDynamicItem, nDynamicItemCount, pActionParamMap);
	reader.setContentHandler(&handler);
	return reader.parse(pwszPath);
}

HMENU qs::MenuManagerImpl::cloneMenu(HMENU hmenu, bool bBar)
{
	HMENU hmenuNew = bBar ? ::CreateMenu() : ::CreatePopupMenu();
	if (!hmenuNew)
		return 0;
	
	TCHAR tszText[256];
	UINT n = 0;
	while (true) {
		MENUITEMINFO mii = {
			sizeof(mii),
			/*MIIM_FTYPE |*/ MIIM_ID | MIIM_STATE |
			/*MIIM_STRING |*/ MIIM_SUBMENU | MIIM_TYPE | MIIM_DATA,
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
			HMENU hmenuSub = cloneMenu(mii.hSubMenu, false);
			if (!hmenuSub)
				return 0;
			::AppendMenu(hmenuNew, MF_POPUP,
				reinterpret_cast<UINT_PTR>(hmenuSub), tszText);
		}
		else {
			UINT nFlags = mii.fType == MFT_STRING ? MF_STRING : MF_SEPARATOR;
			::AppendMenu(hmenuNew, nFlags, mii.wID, tszText);
		}
		if (mii.dwItemData != 0) {
			MENUITEMINFO miiNew = {
				sizeof(miiNew),
				MIIM_DATA,
				0,
				0,
				0,
				0,
				0,
				0,
				mii.dwItemData
			};
			::SetMenuItemInfo(hmenuNew, n, TRUE, &miiNew);
		}
		
		++n;
	}
	
	return hmenuNew;
}


/****************************************************************************
 *
 * MenuManager
 *
 */

qs::MenuManager::MenuManager(const WCHAR* pwszPath,
							 const ActionItem* pItem,
							 size_t nItemCount,
							 const DynamicMenuItem* pDynamicItem,
							 size_t nDynamicItemCount,
							 ActionParamMap* pActionParamMap) :
	pImpl_(0)
{
	assert(pwszPath);
	assert(pItem);
	
	Log log(InitThread::getInitThread().getLogger(), L"qs::MenuManager");
	
	pImpl_ = new MenuManagerImpl();
	pImpl_->pThis_ = this;
	
	if (!pImpl_->load(pwszPath, pItem, nItemCount,
		pDynamicItem, nDynamicItemCount, pActionParamMap)) {
		log.error(L"Could not load menu.");
		return;
	}
}

qs::MenuManager::~MenuManager()
{
	if (pImpl_) {
		for (MenuManagerImpl::MenuMap::const_iterator it = pImpl_->mapMenu_.begin(); it != pImpl_->mapMenu_.end(); ++it) {
			freeWString((*it).wstrName_);
			::DestroyMenu((*it).hmenu_);
		}
		delete pImpl_;
	}
}

HMENU qs::MenuManager::getMenu(const WCHAR* pwszName,
							   bool bBar,
							   bool bClone) const
{
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
	
	if (bClone)
		return MenuManagerImpl::cloneMenu((*it).hmenu_, bBar);
	else
		return (*it).hmenu_;
}

void qs::MenuManager::add(wstring_ptr wstrName,
						  HMENU hmenu,
						  bool bBar)
{
	MenuManagerImpl::Menu menu = {
		wstrName.get(),
		hmenu,
		bBar
	};
	pImpl_->mapMenu_.push_back(menu);
	wstrName.release();
}


/****************************************************************************
 *
 * DynamicMenuCreator
 *
 */

qs::DynamicMenuCreator::~DynamicMenuCreator()
{
}


/****************************************************************************
 *
 * MenuContentHandler
 *
 */

qs::MenuContentHandler::MenuContentHandler(MenuManager* pMenuManager,
										   const ActionItem* pItem,
										   size_t nItemCount,
										   const DynamicMenuItem* pDynamicItem,
										   size_t nDynamicItemCount,
										   ActionParamMap* pActionParamMap) :
	pMenuManager_(pMenuManager),
	pActionItem_(pItem),
	nActionItemCount_(nItemCount),
	pDynamicItem_(pDynamicItem),
	nDynamicItemCount_(nDynamicItemCount),
	pActionParamMap_(pActionParamMap)
{
	stackState_.push_back(STATE_ROOT);
}

qs::MenuContentHandler::~MenuContentHandler()
{
}

bool qs::MenuContentHandler::startElement(const WCHAR* pwszNamespaceURI,
										  const WCHAR* pwszLocalName,
										  const WCHAR* pwszQName,
										  const Attributes& attributes)
{
	assert(!stackState_.empty());
	
	Log log(InitThread::getInitThread().getLogger(), L"qs::MenuContentHandler");
	log.debugf(L"Begin startElement: %s", pwszLocalName);
	
	if (wcscmp(pwszLocalName, L"menus") == 0) {
		if (stackState_.back() != STATE_ROOT)
			return false;
		if (attributes.getLength() != 0)
			return false;
		stackState_.push_back(STATE_MENUS);
	}
	else if (wcscmp(pwszLocalName, L"menu") == 0 ||
		wcscmp(pwszLocalName, L"menubar") == 0) {
		if (stackState_.back() != STATE_MENUS)
			return false;
		
		bool bBar = wcscmp(pwszLocalName, L"menubar") == 0;
		
		const WCHAR* pwszName = 0;
		for (int n = 0; n < attributes.getLength(); ++n) {
			const WCHAR* pwszAttrName = attributes.getLocalName(n);
			if (wcscmp(pwszAttrName, L"name") == 0)
				pwszName = attributes.getValue(n);
			else
				return false;
		}
		if (!pwszName)
			return false;
		
		wstring_ptr wstrName(allocWString(pwszName));
		
		HMENU hmenu = bBar ? ::CreateMenu() : ::CreatePopupMenu();
		if (!hmenu)
			return false;
		
		struct Deleter
		{
			Deleter(HMENU hmenu) :
				hmenu_(hmenu)
			{
			}
			
			~Deleter()
			{
				if (hmenu_)
					::DestroyMenu(hmenu_);
			}
			
			void release() { hmenu_ = 0; }
			
			HMENU hmenu_;
		} deleter(hmenu);
		
		pMenuManager_->add(wstrName, hmenu, bBar);
		deleter.release();
		
		stackMenu_.push_back(hmenu);
		stackState_.push_back(STATE_MENU);
	}
	else if (wcscmp(pwszLocalName, L"menuitem") == 0) {
		if (stackState_.back() != STATE_MENU)
			return false;
		
		const WCHAR* pwszText = 0;
		const WCHAR* pwszAction = 0;
		const WCHAR* pwszParam = 0;
		const WCHAR* pwszDynamic = 0;
		for (int n = 0; n < attributes.getLength(); ++n) {
			const WCHAR* pwszAttrName = attributes.getLocalName(n);
			if (wcscmp(pwszAttrName, L"text") == 0)
				pwszText = attributes.getValue(n);
			else if (wcscmp(pwszAttrName, L"action") == 0)
				pwszAction = attributes.getValue(n);
			else if (wcscmp(pwszAttrName, L"param") == 0)
				pwszParam = attributes.getValue(n);
			else if (wcscmp(pwszAttrName, L"dynamic") == 0)
				pwszDynamic = attributes.getValue(n);
			else
				return false;
		}
		if ((!pwszAction && !pwszDynamic) || (pwszAction && pwszDynamic) || (pwszAction && !pwszText))
			return false;
		
		assert(!stackMenu_.empty());
		HMENU hmenu = stackMenu_.back();
		
		W2T(pwszText, ptszText);
		
		if (pwszDynamic) {
			const DynamicMenuItem* pItem = getDynamicMenuItem(pwszDynamic);
			if (pItem) {
				::AppendMenu(hmenu, MF_STRING, pItem->nId_, _T(""));
				int nItem = ::GetMenuItemCount(hmenu);
				MENUITEMINFO mii = {
					sizeof(mii),
					MIIM_DATA,
					0,
					0,
					0,
					0,
					0,
					0,
					pItem->dwMenuItemData_
				};
				::SetMenuItemInfo(hmenu, nItem - 1, TRUE, &mii);
			}
		}
		else {
			const ActionItem* pItem = getActionItem(pwszAction);
			if (pItem) {
				unsigned int nId = pItem->nId_;
				if (pwszParam) {
					std::auto_ptr<ActionParam> pParam(new ActionParam(nId, pwszParam));
					nId = pActionParamMap_->addActionParam(pItem->nMaxParamCount_, pParam);
					if (nId == -1)
						return false;
				}
				::AppendMenu(hmenu, MF_STRING, nId, ptszText);
			}
		}
		
		stackState_.push_back(STATE_MENUITEM);
	}
	else if (wcscmp(pwszLocalName, L"separator") == 0) {
		if (stackState_.back() != STATE_MENU)
			return false;
		if (attributes.getLength() != 0)
			return false;
		
		assert(!stackMenu_.empty());
		HMENU hmenu = stackMenu_.back();
		::AppendMenu(hmenu, MF_SEPARATOR, -1, 0);
		
		stackState_.push_back(STATE_SEPARATOR);
	}
	else if (wcscmp(pwszLocalName, L"popupmenu") == 0) {
		if (stackState_.back() != STATE_MENU)
			return false;
		
		const WCHAR* pwszText = 0;
		for (int n = 0; n < attributes.getLength(); ++n) {
			const WCHAR* pwszAttrName = attributes.getLocalName(n);
			if (wcscmp(pwszAttrName, L"text") == 0)
				pwszText = attributes.getValue(n);
			else
				return false;
		}
		if (!pwszText)
			return false;
		
		assert(!stackMenu_.empty());
		HMENU hmenu = stackMenu_.back();
		W2T(pwszText, ptszText);
		
		HMENU hmenuSub = ::CreatePopupMenu();
		if (!hmenuSub)
			return false;
		::AppendMenu(hmenu, MF_POPUP, reinterpret_cast<UINT_PTR>(hmenuSub), ptszText);
		
		stackMenu_.push_back(hmenuSub);
		stackState_.push_back(STATE_MENU);
	}
	else {
		return false;
	}
	
	log.debug(L"End startElement");
	
	return true;
}

bool qs::MenuContentHandler::endElement(const WCHAR* pwszNamespaceURI,
										const WCHAR* pwszLocalName,
										const WCHAR* pwszQName)
{
	assert(!stackState_.empty());
	
	Log log(InitThread::getInitThread().getLogger(), L"qs::MenuContentHandler");
	log.debugf(L"Begin endElement: %s", pwszLocalName);
	
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
		assert(stackState_.back() == STATE_MENU);
		stackMenu_.pop_back();
		assert(!stackMenu_.empty());
		stackState_.pop_back();
	}
	else {
		return false;
	}
	
	log.debug(L"End endElement");
	
	return true;
}

bool qs::MenuContentHandler::characters(const WCHAR* pwsz,
										size_t nStart,
										size_t nLength)
{
	assert(!stackState_.empty());
	
	Log log(InitThread::getInitThread().getLogger(), L"qs::MenuContentHandler");
	if (log.isDebugEnabled()) {
		wstring_ptr wstr(allocWString(pwsz + nStart, nLength));
		log.debugf(L"Begin characters: %s", wstr.get());
	}
	
	const WCHAR* p = pwsz + nStart;
	for (size_t n = 0; n < nLength; ++n, ++p) {
		if (*p != L' ' && *p != L'\t' && *p != '\n')
			return false;
	}
	
	log.debug(L"End characters");
	
	return true;
}

const ActionItem* qs::MenuContentHandler::getActionItem(const WCHAR* pwszAction) const
{
	ActionItem item = {
		pwszAction,
		0
	};
	
	const ActionItem* pItem = std::lower_bound(
		pActionItem_, pActionItem_ + nActionItemCount_, item,
		binary_compose_f_gx_hy(
			string_less<WCHAR>(),
			mem_data_ref(&ActionItem::pwszAction_),
			mem_data_ref(&ActionItem::pwszAction_)));
	if (pItem == pActionItem_ + nActionItemCount_ ||
		wcscmp(pItem->pwszAction_, pwszAction) != 0 ||
		(pItem->nFlags_ != 0 && !(pItem->nFlags_ & ActionItem::FLAG_MENU)))
		return 0;
	return pItem;
}

const DynamicMenuItem* qs::MenuContentHandler::getDynamicMenuItem(const WCHAR* pwszName) const
{
	DynamicMenuItem item = {
		pwszName,
		0
	};
	
	const DynamicMenuItem* pItem = std::lower_bound(
		pDynamicItem_, pDynamicItem_ + nDynamicItemCount_, item,
		binary_compose_f_gx_hy(
			string_less<WCHAR>(),
			mem_data_ref(&DynamicMenuItem::pwszName_),
			mem_data_ref(&DynamicMenuItem::pwszName_)));
	if (pItem == pDynamicItem_ + nDynamicItemCount_ ||
		wcscmp(pItem->pwszName_, pwszName) != 0)
		return 0;
	return pItem;
}
