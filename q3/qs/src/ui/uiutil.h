/*
 * $Id$
 *
 * Copyright(C) 1998-2005 Satoshi Nakamura
 * All rights reserved.
 *
 */

#ifndef __UIUTIL_H__
#define __UIUTIL_H__

namespace qs {

/****************************************************************************
 *
 * hash_hwnd
 *
 */

struct hash_hwnd
{
	size_t operator()(HWND hwnd) const;
};

}

#include "uiutil.inl"

#endif // __UIUTIL_H__
