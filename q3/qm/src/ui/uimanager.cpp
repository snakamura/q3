/*
 * $Id$
 *
 * Copyright(C) 1998-2004 Satoshi Nakamura
 * All rights reserved.
 *
 */

#include <qmapplication.h>
#include <qmfilenames.h>

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
	wstring_ptr wstrPath = Application::getApplication().getProfilePath(FileNames::VIEWS_XML);
	pViewData_.reset(new ViewData(this, wstrPath.get()));
}

qm::UIManager::~UIManager()
{
}

bool qm::UIManager::save() const
{
	return pViewData_->save();
}

ViewDataItem* qm::UIManager::getDefaultViewDataItem() const
{
	return pViewData_->getItem(0);
}
