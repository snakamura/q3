/*
 * $Id$
 *
 * Copyright(C) 1998-2008 Satoshi Nakamura
 *
 */

#ifndef __FOLDERLISTWINDOW_H__
#define __FOLDERLISTWINDOW_H__


namespace qm {

struct FolderWindowCreateContext;

class Document;
class FolderImage;
class UIManager;


/****************************************************************************
 *
 * FolderWindowCreateContext
 *
 */

struct FolderListWindowCreateContext
{
	UIManager* pUIManager_;
	const FolderImage* pFolderImage_;
};

}

#endif // __FOLDERLISTWINDOW_H__
