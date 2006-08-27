/*
 * $Id$
 *
 * Copyright(C) 1998-2006 Satoshi Nakamura
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
			  ActionParamMap* pActionParamMap,
			  DynamicMenuMap* pDynamicMenuMap);
	
	static HMENU cloneMenu(HMENU hmenu,
						   bool bBar);
	
	MenuManager* pThis_;
	MenuMap mapMenu_;
};

bool qs::MenuManagerImpl::load(const WCHAR* pwszPath,
							   const ActionItem* pItem,
							   size_t nItemCount,
							   ActionParamMap* pActionParamMap,
							   DynamicMenuMap* pDynamicMenuMap)
{
	assert(pwszPath);
	assert(pActionParamMap);
	assert(pDynamicMenuMap);
	
	XMLReader reader;
	MenuContentHandler handler(pThis_, pItem,
		nItemCount, pActionParamMap, pDynamicMenuMap);
	reader.setContentHandler(&handler);
	return reader.parse(pwszPath);
}

HMENU qs::MenuManagerImpl::cloneMenu(HMENU hmenu, bool bBar)
{
	HMENU hmenuNew = bBar ? ::CreateMenu() : ::CreatePopupMenu();
	if (!hmenuNew)
		return 0;
	
	TCHAR tszText[256];
	for (UINT n = 0; ; ++n) {
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
			if (mii.fType & MFT_SEPARATOR)
				::AppendMenu(hmenuNew, MF_SEPARATOR, -1, 0);
			else
				::AppendMenu(hmenuNew, MF_STRING, mii.wID, tszText);
		}
		if (mii.dwItemData != 0) {
			mii.fMask = MIIM_DATA;
			::SetMenuItemInfo(hmenuNew, n, TRUE, &mii);
		}
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
							 ActionParamMap* pActionParamMap,
							 DynamicMenuMap* pDynamicMenuMap) :
	pImpl_(0)
{
	assert(pwszPath);
	assert(pItem);
	
	Log log(InitThread::getInitThread().getLogger(), L"qs::MenuManager");
	
	pImpl_ = new MenuManagerImpl();
	pImpl_->pThis_ = this;
	
	if (!pImpl_->load(pwszPath, pItem, nItemCount, pActionParamMap, pDynamicMenuMap)) {
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
 * DynamicMenuItem
 *
 */

qs::DynamicMenuItem::DynamicMenuItem(unsigned int nId,
									 const WCHAR* pwszName,
									 const WCHAR* pwszParam) :
	nId_(nId)
{
	assert(pwszName);
	
	wstrName_ = allocWString(pwszName);
	if (pwszParam)
		wstrParam_ = allocWString(pwszParam);
}

qs::DynamicMenuItem::~DynamicMenuItem()
{
}

unsigned int qs::DynamicMenuItem::getId() const
{
	return nId_;
}

const WCHAR* qs::DynamicMenuItem::getName() const
{
	return wstrName_.get();
}

const WCHAR* qs::DynamicMenuItem::getParam() const
{
	return wstrParam_.get();
}


/****************************************************************************
 *
 * DynamicMenuMap
 *
 */

qs::DynamicMenuMap::DynamicMenuMap()
{
}

qs::DynamicMenuMap::~DynamicMenuMap()
{
	std::for_each(listItem_.begin(), listItem_.end(),
		qs::deleter<DynamicMenuItem>());
}

const DynamicMenuItem* qs::DynamicMenuMap::getItem(unsigned int nId) const
{
	ItemList::const_iterator it = std::find_if(
		listItem_.begin(), listItem_.end(),
		std::bind2nd(
			binary_compose_f_gx_hy(
				std::equal_to<unsigned int>(),
				std::mem_fun(&DynamicMenuItem::getId),
				std::identity<unsigned int>()),
			nId));
	return it != listItem_.end() ? *it : 0;
}

unsigned int qs::DynamicMenuMap::addItem(const WCHAR* pwszName,
										 const WCHAR* pwszParam)
{
	assert(pwszName);
	
	ItemList::const_iterator it = listItem_.begin();
	while (it != listItem_.end()) {
		if (wcscmp((*it)->getName(), pwszName) == 0 &&
			string_equal<WCHAR>()((*it)->getParam(), pwszParam))
			break;
		++it;
	}
	if (it != listItem_.end())
		return (*it)->getId();
	
	unsigned int nId = listItem_.empty() ? 1 : listItem_.back()->getId() + 1;
	std::auto_ptr<DynamicMenuItem> pItem(createItem(nId, pwszName, pwszParam));
	listItem_.push_back(pItem.get());
	pItem.release();
	return nId;
}

std::auto_ptr<DynamicMenuItem> qs::DynamicMenuMap::createItem(unsigned int nId,
															  const WCHAR* pwszName,
															  const WCHAR* pwszParam) const
{
	return std::auto_ptr<DynamicMenuItem>(new DynamicMenuItem(nId, pwszName, pwszParam));
}


/****************************************************************************
 *
 * MenuContentHandler
 *
 */

qs::MenuContentHandler::MenuContentHandler(MenuManager* pMenuManager,
										   const ActionItem* pItem,
										   size_t nItemCount,
										   ActionParamMap* pActionParamMap,
										   DynamicMenuMap* pDynamicMenuMap) :
	pMenuManager_(pMenuManager),
	pActionItem_(pItem),
	nActionItemCount_(nItemCount),
	pActionParamMap_(pActionParamMap),
	pDynamicMenuMap_(pDynamicMenuMap)
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
			const ActionItem* pItem = getActionItem(pwszDynamic);
			if (pItem && pItem->nMaxParamCount_ > 1) {
				unsigned int nId = pDynamicMenuMap_->addItem(pwszDynamic, pwszParam);
				::AppendMenu(hmenu, MF_STRING, nId, _T(""));
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
					nId
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
				}
				if (nId != -1)
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
	assert(pwszAction);
	
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
