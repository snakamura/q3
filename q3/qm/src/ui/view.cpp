/*
 * $Id$
 *
 * Copyright(C) 1998-2008 Satoshi Nakamura
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

