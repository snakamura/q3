/*
 * $Id$
 *
 * Copyright(C) 1998-2005 Satoshi Nakamura
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

qm::StatusBar::StatusBar() :
	WindowBase(true)
{
	setWindowHandler(this, false);
}

qm::StatusBar::~StatusBar()
{
}

bool qm::StatusBar::setParts(int* pnWidth,
							 size_t nCount)
{
	return sendMessage(SB_SETPARTS, nCount, reinterpret_cast<LPARAM>(pnWidth)) != 0;
}

bool qm::StatusBar::setText(int n,
							const WCHAR* pwszText)
{
	W2T(pwszText, ptszText);
	return sendMessage(SB_SETTEXT, n, reinterpret_cast<LPARAM>(ptszText)) != 0;
}

bool qm::StatusBar::setIcon(int n,
							HICON hIcon)
{
	return sendMessage(SB_SETICON, n, reinterpret_cast<LPARAM>(hIcon)) != 0;
}

void qm::StatusBar::setSimple(bool bSimple)
{
	sendMessage(SB_SIMPLE, bSimple);
}

LRESULT qm::StatusBar::windowProc(UINT uMsg,
								  WPARAM wParam,
								  LPARAM lParam)
{
	return DefaultWindowHandler::windowProc(uMsg, wParam, lParam);
}
