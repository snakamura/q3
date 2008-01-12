/*
 * $Id$
 *
 * Copyright(C) 1998-2008 Satoshi Nakamura
 *
 */

#ifndef __UIUTIL_INL__
#define __UIUTIL_INL__


/****************************************************************************
 *
 * hash_hwnd
 *
 */

inline size_t qs::hash_hwnd::operator()(HWND hwnd) const
{
	return reinterpret_cast<int>(hwnd)/8;
}

#endif // __UIUTIL_INL__
