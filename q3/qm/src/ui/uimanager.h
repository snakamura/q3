/*
 * $Id$
 *
 * Copyright(C) 1998-2004 Satoshi Nakamura
 * All rights reserved.
 *
 */

#ifndef __UIMANAGER_H__
#define __UIMANAGER_H__

#include <qm.h>

#include <qskeymap.h>
#include <qsmenu.h>
#include <qstoolbar.h>


namespace qm {

class UIManager;

class DefaultViewData;


/****************************************************************************
 *
 * UIManager
 *
 */

class UIManager
{
public:
	UIManager();
	~UIManager();

public:
	qs::MenuManager* getMenuManager() const;
	qs::ToolbarManager* getToolbarManager() const;
	qs::KeyMap* getKeyMap() const;
	DefaultViewData* getDefaultViewData() const;
	
	bool save() const;

private:
	UIManager(const UIManager&);
	UIManager& operator=(const UIManager&);

private:
	std::auto_ptr<qs::MenuManager> pMenuManager_;
	std::auto_ptr<qs::ToolbarManager> pToolbarManager_;
	std::auto_ptr<qs::KeyMap> pKeyMap_;
	std::auto_ptr<DefaultViewData> pViewData_;
};

}

#endif // __UIMANAGER_H__
