/*
 * $Id$
 *
 * Copyright(C) 1998-2007 Satoshi Nakamura
 * All rights reserved.
 *
 */

#include <qmapplication.h>
#include <qmfilenames.h>

#include <qsconv.h>
#include <qsdevicecontext.h>

#include "actionitem.h"
#include "menucreator.h"
#include "uimanager.h"

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
	
	pActionParamMap_.reset(new ActionParamMap());
	pDynamicMenuMap_.reset(new MacroDynamicMenuMap());
	
	wstring_ptr wstrMenuPath(app.getProfilePath(FileNames::MENUS_XML));
	pMenuManager_.reset(new MenuManager(wstrMenuPath.get(), actionItems,
		countof(actionItems), pActionParamMap_.get(), pDynamicMenuMap_.get()));
	
	wstring_ptr wstrBitmapPath(app.getImagePath(FileNames::TOOLBAR_BMP));
	W2T(wstrBitmapPath.get(), ptszBitmapPath);
#ifdef _WIN32_WCE
	GdiObject<HBITMAP> hBitmap(::SHLoadDIBitmap(ptszBitmapPath));
#else
	GdiObject<HBITMAP> hBitmap(reinterpret_cast<HBITMAP>(::LoadImage(0,
		ptszBitmapPath, IMAGE_BITMAP, 0, 0, LR_LOADFROMFILE)));
#endif
	wstring_ptr wstrToolbarPath(app.getProfilePath(FileNames::TOOLBARS_XML));
	pToolbarManager_.reset(new ToolbarManager(wstrToolbarPath.get(), hBitmap.get(),
		actionItems, countof(actionItems), pMenuManager_.get(), pActionParamMap_.get()));
	
	wstring_ptr wstrKeyMapPath(app.getProfilePath(FileNames::KEYMAP_XML));
	pKeyMap_.reset(new KeyMap(wstrKeyMapPath.get(), actionItems,
		countof(actionItems), pActionParamMap_.get()));
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

ActionParamMap* qm::UIManager::getActionParamMap() const
{
	return pActionParamMap_.get();
}

DynamicMenuMap* qm::UIManager::getDynamicMenuMap() const
{
	return pDynamicMenuMap_.get();
}
