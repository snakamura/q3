/*
 * $Id$
 *
 * Copyright(C) 1998-2005 Satoshi Nakamura
 * All rights reserved.
 *
 */

#ifndef __ACTION_INL__
#define __ACTION_INL__

#include <qmapplication.h>


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
		&WindowX::isShowStatusBar, IDS_SHOWSTATUSBAR, IDS_HIDESTATUSBAR)
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
		&WindowX::isShowToolbar, IDS_SHOWTOOLBAR, IDS_HIDETOOLBAR)
{
}

template<class WindowX>
qm::ViewShowToolbarAction<WindowX>::~ViewShowToolbarAction()
{
}

#endif // __ACTION_INL__
