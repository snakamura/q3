/*
 * $Id$
 *
 * Copyright(C) 1998-2005 Satoshi Nakamura
 * All rights reserved.
 *
 */

#ifndef __FOLDERLISTWINDOW_H__
#define __FOLDERLISTWINDOW_H__


namespace qm {

struct FolderWindowCreateContext;

class Document;
class UIManager;


/****************************************************************************
 *
 * FolderWindowCreateContext
 *
 */

struct FolderListWindowCreateContext
{
	UIManager* pUIManager_;
};

}

#endif // __FOLDERLISTWINDOW_H__
