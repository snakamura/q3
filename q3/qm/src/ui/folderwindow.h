/*
 * $Id$
 *
 * Copyright(C) 1998-2004 Satoshi Nakamura
 * All rights reserved.
 *
 */

#ifndef __FOLDERWINDOW_H__
#define __FOLDERWINDOW_H__


namespace qm {

struct FolderWindowCreateContext;

class Document;
class UIManager;


/****************************************************************************
 *
 * FolderWindowCreateContext
 *
 */

struct FolderWindowCreateContext
{
	Document* pDocument_;
	UIManager* pUIManager_;
};

}

#endif // __FOLDERWINDOW_H__
