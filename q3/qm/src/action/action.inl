/*
 * $Id$
 *
 * Copyright(C) 1998-2003 Satoshi Nakamura
 * All rights reserved.
 *
 */

#ifndef __ACTION_INL__
#define __ACTION_INL__

#include <qmapplication.h>

#include <qserror.h>


/****************************************************************************
 *
 * ViewShowControlAction
 *
 */

template<class WindowX>
qm::ViewShowControlAction<WindowX>::ViewShowControlAction(
	WindowX* pWindow, PFN_SET pfnSet, PFN_IS pfnIs,
	UINT nShowId, UINT nHideId, qs::QSTATUS* pstatus) :
	pWindow_(pWindow),
	pfnSet_(pfnSet),
	pfnIs_(pfnIs),
	nShowId_(nShowId),
	nHideId_(nHideId)
{
	assert(pstatus);
	*pstatus = qs::QSTATUS_SUCCESS;
}

template<class WindowX>
qm::ViewShowControlAction<WindowX>::~ViewShowControlAction()
{
}

template<class WindowX>
qs::QSTATUS qm::ViewShowControlAction<WindowX>::invoke(
	const qs::ActionEvent& event)
{
	return (pWindow_->*pfnSet_)(!(pWindow_->*pfnIs_)());
}

template<class WindowX>
qs::QSTATUS qm::ViewShowControlAction<WindowX>::getText(
	const qs::ActionEvent& event, qs::WSTRING* pwstrText)
{
	assert(pwstrText);
	
	return loadString(Application::getApplication().getResourceHandle(),
		(pWindow_->*pfnIs_)() ? nHideId_ : nShowId_, pwstrText);
}


/****************************************************************************
 *
 * ViewShowStatusBarAction
 *
 */

template<class WindowX>
qm::ViewShowStatusBarAction<WindowX>::ViewShowStatusBarAction(
	WindowX* pWindow, qs::QSTATUS* pstatus) :
	ViewShowControlAction<WindowX>(pWindow,
		&WindowX::setShowStatusBar,
		&WindowX::isShowStatusBar,
		IDS_SHOWSTATUSBAR, IDS_HIDESTATUSBAR, pstatus)
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
qm::ViewShowToolbarAction<WindowX>::ViewShowToolbarAction(
	WindowX* pWindow, qs::QSTATUS* pstatus) :
	ViewShowControlAction<WindowX>(pWindow,
		&WindowX::setShowToolbar,
		&WindowX::isShowToolbar,
		IDS_SHOWTOOLBAR, IDS_HIDETOOLBAR, pstatus)
{
}

template<class WindowX>
qm::ViewShowToolbarAction<WindowX>::~ViewShowToolbarAction()
{
}



#endif // __ACTION_INL__
