/*
 * $Id$
 *
 * Copyright(C) 1998-2006 Satoshi Nakamura
 * All rights reserved.
 *
 */

#ifndef __FOLDERWINDOW_H__
#define __FOLDERWINDOW_H__


namespace qm {

struct FolderWindowCreateContext;

class Document;
class FolderImage;
class SyncManager;
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
	const FolderImage* pFolderImage_;
	SyncManager* pSyncManager_;
};

}

#endif // __FOLDERWINDOW_H__
