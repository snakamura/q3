/*
 * $Id$
 *
 * Copyright(C) 1998-2003 Satoshi Nakamura
 * All rights reserved.
 *
 */

#ifndef __QMEXTENSIONS_H__
#define __QMEXTENSIONS_H__

#include <qm.h>

#include <windows.h>


namespace qm {

/****************************************************************************
 *
 * Extensions
 *
 */

struct Extensions
{
	static const WCHAR* ACCOUNT;
	static const WCHAR* ADDRESSBOOK;
	static const WCHAR* BOX;
	static const WCHAR* CERT;
	static const WCHAR* COLORS;
	static const WCHAR* FILTERS;
	static const WCHAR* FOLDERS;
	static const WCHAR* GOROUND;
	static const WCHAR* HEADER;
	static const WCHAR* HEADEREDIT;
	static const WCHAR* KEY;
	static const WCHAR* KEYMAP;
	static const WCHAR* MAP;
	static const WCHAR* MENUS;
	static const WCHAR* MSGLIST;
	static const WCHAR* RULES;
	static const WCHAR* SIGNATURES;
	static const WCHAR* SYNCFILTERS;
	static const WCHAR* TEXTS;
	static const WCHAR* TOOLBARS;
	static const WCHAR* VIEW;
	static const WCHAR* QMAIL;
};

};

#endif // __QMEXTENSIONS_H__
