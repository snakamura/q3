/*
 * $Id$
 *
 * Copyright(C) 1998-2004 Satoshi Nakamura
 * All rights reserved.
 *
 */

#ifndef __FOLDERWINDOW_H__
#define __FOLDERWINDOW_H__

#include <qskeymap.h>
#include <qsmenu.h>

namespace qm {

class Document;


/****************************************************************************
 *
 * FolderWindowCreateContext
 *
 */

struct FolderWindowCreateContext
{
	Document* pDocument_;
	qs::MenuManager* pMenuManager_;
	qs::KeyMap* pKeyMap_;
};

}

#endif // __FOLDERWINDOW_H__
