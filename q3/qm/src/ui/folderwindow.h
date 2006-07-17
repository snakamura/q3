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

class AccountManager;
class FolderImage;
class SyncManager;
class UIManager;
class UndoManager;


/****************************************************************************
 *
 * FolderWindowCreateContext
 *
 */

struct FolderWindowCreateContext
{
	AccountManager* pAccountManager_;
	UndoManager* pUndoManager_;
	UIManager* pUIManager_;
	const FolderImage* pFolderImage_;
	SyncManager* pSyncManager_;
};

}

#endif // __FOLDERWINDOW_H__
