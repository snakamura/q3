/*
 * $Id: statusbar.cpp,v 1.1.1.1 2003/04/29 08:07:32 snakamura Exp $
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

LRESULT qm::StatusBar::windowProc(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	return DefaultWindowHandler::windowProc(uMsg, wParam, lParam);
}
