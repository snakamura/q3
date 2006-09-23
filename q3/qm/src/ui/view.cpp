/*
 * $Id$
 *
 * Copyright(C) 1998-2006 Satoshi Nakamura
 * All rights reserved.
 *
 */

#include <qmview.h>

using namespace qm;


/****************************************************************************
 *
 * View
 *
 */

qm::View::~View()
{
}

FocusControllerBase* qm::View::getViewFocusController() const
{
	return 0;
}

