/*
 * $Id$
 *
 * Copyright(C) 1998-2008 Satoshi Nakamura
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
class URIResolver;


/****************************************************************************
 *
 * FolderWindowCreateContext
 *
 */

struct FolderWindowCreateContext
{
	AccountManager* pAccountManager_;
	const URIResolver* pURIResolver_;
	UndoManager* pUndoManager_;
	UIManager* pUIManager_;
	const FolderImage* pFolderImage_;
	SyncManager* pSyncManager_;
};

}

#endif // __FOLDERWINDOW_H__
