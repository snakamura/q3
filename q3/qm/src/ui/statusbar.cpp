/*
 * $Id$
 *
 * Copyright(C) 1998-2003 Satoshi Nakamura
 * All rights reserved.
 *
 */

#include "statusbar.h"

using namespace qm;
using namespace qs;


/****************************************************************************
 *
 * StatusBar
 *
 */

qm::StatusBar::StatusBar(QSTATUS* pstatus) :
	WindowBase(true, pstatus),
	DefaultWindowHandler(pstatus)
{
	setWindowHandler(this, false);
}

qm::StatusBar::~StatusBar()
{
}

QSTATUS qm::StatusBar::setParts(int* pnWidth, size_t nCount)
{
	BOOL b = sendMessage(SB_SETPARTS, nCount,
		reinterpret_cast<LPARAM>(pnWidth));
	return b ? QSTATUS_SUCCESS : QSTATUS_FAIL;
}

QSTATUS qm::StatusBar::setText(int n, const WCHAR* pwszText)
{
	DECLARE_QSTATUS();
	
	W2T(pwszText, ptszText);
	BOOL b = sendMessage(SB_SETTEXT, n, reinterpret_cast<LPARAM>(ptszText));
	return b ? QSTATUS_SUCCESS : QSTATUS_FAIL;
}

void qm::StatusBar::setSimple(bool bSimple)
{
	sendMessage(SB_SIMPLE, bSimple);
}

LRESULT qm::StatusBar::windowProc(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	return DefaultWindowHandler::windowProc(uMsg, wParam, lParam);
}
