/*
 * $Id$
 *
 * Copyright(C) 1998-2007 Satoshi Nakamura
 * All rights reserved.
 *
 */

#ifndef __UIMANAGER_H__
#define __UIMANAGER_H__

#include <qm.h>

#include <qsaction.h>
#include <qskeymap.h>
#include <qsmenu.h>
#include <qstoolbar.h>


namespace qm {

class UIManager;


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
	qs::ActionParamMap* getActionParamMap() const;
	qs::DynamicMenuMap* getDynamicMenuMap() const;

private:
	UIManager(const UIManager&);
	UIManager& operator=(const UIManager&);

private:
	std::auto_ptr<qs::MenuManager> pMenuManager_;
	std::auto_ptr<qs::ToolbarManager> pToolbarManager_;
	std::auto_ptr<qs::KeyMap> pKeyMap_;
	std::auto_ptr<qs::ActionParamMap> pActionParamMap_;
	std::auto_ptr<qs::DynamicMenuMap> pDynamicMenuMap_;
};

}

#endif // __UIMANAGER_H__
