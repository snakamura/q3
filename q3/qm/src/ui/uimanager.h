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


namespace qm {

class UIManager;

class ViewData;
class ViewDataItem;


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
	bool save() const;
	ViewDataItem* getDefaultViewDataItem() const;

private:
	UIManager(const UIManager&);
	UIManager& operator=(const UIManager&);

private:
	std::auto_ptr<ViewData> pViewData_;
};

}

#endif // __UIMANAGER_H__
