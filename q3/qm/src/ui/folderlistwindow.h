/*
 * $Id$
 *
 * Copyright(C) 1998-2004 Satoshi Nakamura
 * All rights reserved.
 *
 */

#ifndef __FOLDERLISTWINDOW_H__
#define __FOLDERLISTWINDOW_H__

#include <qskeymap.h>
#include <qsmenu.h>


namespace qm {

class Document;


/****************************************************************************
 *
 * FolderWindowCreateContext
 *
 */

struct FolderListWindowCreateContext
{
	Document* pDocument_;
	qs::MenuManager* pMenuManager_;
	qs::KeyMap* pKeyMap_;
};

}

#endif // __FOLDERLISTWINDOW_H__
