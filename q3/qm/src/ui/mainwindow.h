/*
 * $Id: mainwindow.h,v 1.1.1.1 2003/04/29 08:07:32 snakamura Exp $
 *
 * Copyright(C) 1998-2003 Satoshi Nakamura
 * All rights reserved.
 *
 */

#ifndef __MAINWINDOW_H__
#define __MAINWINDOW_H__

#include <qskeymap.h>
#include <qsmenu.h>
#include <qswindow.h>

#include "foldermodel.h"

namespace qm {

struct MainWindowCreateContext;

class Document;
class FolderListWindow;
class GoRound;
class ListWindow;
class SyncDialogManager;
class SyncManager;
class TempFileCleaner;


/****************************************************************************
 *
 * ListContainerWindow
 *
 */

class ListContainerWindow :
	public qs::WindowBase,
	public qs::DefaultWindowHandler,
	public FolderModelHandler
{
public:
	ListContainerWindow(FolderModel* pFolderModel, qs::QSTATUS* pstatus);
	virtual ~ListContainerWindow();

public:
	void setFolderListWindow(FolderListWindow* pFolderListWindow);
	void setListWindow(ListWindow* pListWindow);

public:
	virtual LRESULT windowProc(UINT uMsg, WPARAM wParam, LPARAM lParam);

protected:
	LRESULT onSize(UINT nFlags, int cx, int cy);

public:
	virtual qs::QSTATUS accountSelected(const FolderModelEvent& event);
	virtual qs::QSTATUS folderSelected(const FolderModelEvent& event);

private:
	ListContainerWindow(const ListContainerWindow&);
	ListContainerWindow& operator=(const ListContainerWindow&);

private:
	FolderModel* pFolderModel_;
	FolderListWindow* pFolderListWindow_;
	ListWindow* pListWindow_;
	DelayedFolderModelHandler* pDelayedFolderModelHandler_;
};


/****************************************************************************
 *
 * MainWindowCreateContext
 *
 */

struct MainWindowCreateContext
{
	Document* pDocument_;
	SyncManager* pSyncManager_;
	SyncDialogManager* pSyncDialogManager_;
	GoRound* pGoRound_;
	TempFileCleaner* pTempFileCleaner_;
	qs::MenuManager* pMenuManager_;
	qs::KeyMap* pKeyMap_;
};


}

#endif // __MAINWINDOW_H__
