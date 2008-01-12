/*
 * $Id$
 *
 * Copyright(C) 1998-2008 Satoshi Nakamura
 *
 */

#ifndef __ACTION_INL__
#define __ACTION_INL__

#include <qmapplication.h>


/****************************************************************************
 *
 * EditCommandAction
 *
 */

template<class Item>
qm::EditCommandAction<Item>::EditCommandAction(FocusController<Item>* pFocusController,
											   PFN_DO pfnDo,
											   PFN_CANDO pfnCanDo) :
	pFocusController_(pFocusController),
	pfnDo_(pfnDo),
	pfnCanDo_(pfnCanDo)
{
}

template<class Item>
qm::EditCommandAction<Item>::~EditCommandAction()
{
}

template<class Item>
void qm::EditCommandAction<Item>::invoke(const qs::ActionEvent& event)
{
	Item* pItem = pFocusController_->getFocusedItem();
	if (pItem)
		(pItem->*pfnDo_)();
}

template<class Item>
bool qm::EditCommandAction<Item>::isEnabled(const qs::ActionEvent& event)
{
	Item* pItem = pFocusController_->getFocusedItem();
	return pItem ? (pItem->*pfnCanDo_)() : false;
}


/****************************************************************************
 *
 * ViewShowControlAction
 *
 */

template<class WindowX>
qm::ViewShowControlAction<WindowX>::ViewShowControlAction(WindowX* pWindow,
														  PFN_SET pfnSet,
														  PFN_IS pfnIs,
														  UINT nShowId,
														  UINT nHideId) :
	pWindow_(pWindow),
	pfnSet_(pfnSet),
	pfnIs_(pfnIs),
	nShowId_(nShowId),
	nHideId_(nHideId)
{
}

template<class WindowX>
qm::ViewShowControlAction<WindowX>::~ViewShowControlAction()
{
}

template<class WindowX>
void qm::ViewShowControlAction<WindowX>::invoke(const qs::ActionEvent& event)
{
	(pWindow_->*pfnSet_)(!(pWindow_->*pfnIs_)());
}

template<class WindowX>
qs::wstring_ptr qm::ViewShowControlAction<WindowX>::getText(const qs::ActionEvent& event)
{
	HINSTANCE hInst = Application::getApplication().getResourceHandle();
	return loadString(hInst, (pWindow_->*pfnIs_)() ? nHideId_ : nShowId_);
}


/****************************************************************************
 *
 * ViewShowStatusBarAction
 *
 */

template<class WindowX>
qm::ViewShowStatusBarAction<WindowX>::ViewShowStatusBarAction(WindowX* pWindow) :
	ViewShowControlAction<WindowX>(pWindow, &WindowX::setShowStatusBar,
		&WindowX::isShowStatusBar, IDS_ACTION_SHOWSTATUSBAR, IDS_ACTION_HIDESTATUSBAR)
{
}

template<class WindowX>
qm::ViewShowStatusBarAction<WindowX>::~ViewShowStatusBarAction()
{
}


/****************************************************************************
 *
 * ViewShowToolbarAction
 *
 */

template<class WindowX>
qm::ViewShowToolbarAction<WindowX>::ViewShowToolbarAction(WindowX* pWindow) :
	ViewShowControlAction<WindowX>(pWindow, &WindowX::setShowToolbar,
		&WindowX::isShowToolbar, IDS_ACTION_SHOWTOOLBAR, IDS_ACTION_HIDETOOLBAR)
{
}

template<class WindowX>
qm::ViewShowToolbarAction<WindowX>::~ViewShowToolbarAction()
{
}

#endif // __ACTION_INL__
