/*
 * $Id: folderlistwindow.h,v 1.1.1.1 2003/04/29 08:07:32 snakamura Exp $
 *
 * Copyright(C) 1998-2003 Satoshi Nakamura
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
