/*
 * $Id$
 *
 * Copyright(C) 1998-2005 Satoshi Nakamura
 * All rights reserved.
 *
 */

#ifndef __QSTHEME_H__
#define __QSTHEME_H__

#ifndef _WIN32_WCE

#include <qs.h>


namespace qs {

class Theme;


/****************************************************************************
 *
 * Theme
 *
 */

class QSEXPORTCLASS Theme
{
public:
	Theme(HWND hwnd,
		  const WCHAR* pwszClasses);
	~Theme();

public:
	bool isActive() const;
	bool drawBackground(HDC hdc,
						int nPartId,
						int nStateId,
						const RECT& rect,
						const RECT* pRectClip);
	bool drawEdge(HDC hdc,
				  int nPartId,
				  int nStateId,
				  const RECT& rect,
				  UINT nEdge,
				  UINT nFlags,
				  RECT* pRect);
	int getSysSize(int nId);

private:
	Theme(const Theme&);
	Theme& operator=(const Theme&);

private:
	struct ThemeImpl* pImpl_;
};

}

#endif // _WIN32_WCE

#endif // __QSTHEME_H__
