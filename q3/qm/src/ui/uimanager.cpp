/*
 * $Id$
 *
 * Copyright(C) 1998-2004 Satoshi Nakamura
 * All rights reserved.
 *
 */

#include <qmapplication.h>
#include <qmfilenames.h>

#include <qsconv.h>

#include "menu.h"
#include "toolbar.h"
#include "uimanager.h"
#include "viewmodel.h"

using namespace qm;
using namespace qs;


/****************************************************************************
 *
 * UIManager
 *
 */

qm::UIManager::UIManager()
{
	Application& app = Application::getApplication();
	
	wstring_ptr wstrMenuPath(app.getProfilePath(FileNames::MENUS_XML));
	PopupMenuManager popupMenuManager;
	std::auto_ptr<LoadMenuPopupMenu> pPopupMenus[countof(popupMenuItems)];
	for (int n = 0; n < countof(popupMenuItems); ++n) {
		pPopupMenus[n].reset(new LoadMenuPopupMenu(
			app.getResourceHandle(), popupMenuItems[n].nId_));
		popupMenuManager.addPopupMenu(
			popupMenuItems[n].pwszName_, pPopupMenus[n].get());
	}
	pMenuManager_.reset(new MenuManager(wstrMenuPath.get(),
		menuItems, countof(menuItems), popupMenuManager));
	
	wstring_ptr wstrBitmapPath(app.getProfilePath(FileNames::TOOLBAR_BMP));
	W2T(wstrBitmapPath.get(), ptszBitmapPath);
#ifdef _WIN32_WCE
	HBITMAP hBitmap = ::SHLoadDIBitmap(ptszBitmapPath);
#else
	HBITMAP hBitmap = reinterpret_cast<HBITMAP>(::LoadImage(0,
		ptszBitmapPath, IMAGE_BITMAP, 0, 0, LR_LOADFROMFILE));
#endif
	wstring_ptr wstrToolbarPath(app.getProfilePath(FileNames::TOOLBARS_XML));
	pToolbarManager_.reset(new ToolbarManager(wstrToolbarPath.get(),
		hBitmap, toolbarItems, countof(toolbarItems), pMenuManager_.get()));
	
	wstring_ptr wstrKeyMapPath(app.getProfilePath(FileNames::KEYMAP_XML));
	pKeyMap_.reset(new KeyMap(wstrKeyMapPath.get()));
	
	wstring_ptr wstrViewsPath = app.getProfilePath(FileNames::VIEWS_XML);
	pViewData_.reset(new DefaultViewData(wstrViewsPath.get()));
}

qm::UIManager::~UIManager()
{
}

MenuManager* qm::UIManager::getMenuManager() const
{
	return pMenuManager_.get();
}

ToolbarManager* qm::UIManager::getToolbarManager() const
{
	return pToolbarManager_.get();
}

KeyMap* qm::UIManager::getKeyMap() const
{
	return pKeyMap_.get();
}

DefaultViewData* qm::UIManager::getDefaultViewData() const
{
	return pViewData_.get();
}

bool qm::UIManager::save() const
{
	return pViewData_->save();
}
