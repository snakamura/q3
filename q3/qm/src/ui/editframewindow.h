/*
 * $Id$
 *
 * Copyright(C) 1998-2003 Satoshi Nakamura
 * All rights reserved.
 *
 */

#ifndef __EDITFRAMEWINDOW_H__
#define __EDITFRAMEWINDOW_H__

#include <qskeymap.h>
#include <qsmenu.h>
#include <qsprofile.h>
#include <qstoolbar.h>


namespace qm {

class EditFrameWindow;
class EditFrameWindowManager;

class Document;
class EditMessage;
class SyncDialogManager;
class SyncManager;


/****************************************************************************
 *
 * EditFrameWindowManager
 *
 */

class EditFrameWindowManager
{
public:
	EditFrameWindowManager(Document* pDocument, SyncManager* pSyncManager,
		SyncDialogManager* pSyncDialogManager, qs::KeyMap* pKeyMap,
		qs::Profile* pProfile, qs::MenuManager* pMenuManager,
		qs::ToolbarManager* pToolbarManager, qs::QSTATUS* pstatus);
	~EditFrameWindowManager();

public:
	qs::QSTATUS open(EditMessage* pEditMessage);
	void close(EditFrameWindow* pEditFrameWindow);
	qs::QSTATUS closeAll(bool* pbClosed);
	qs::QSTATUS preModalDialog(HWND hwndParent);
	qs::QSTATUS postModalDialog(HWND hwndParent);

private:
	EditFrameWindowManager(const EditFrameWindowManager&);
	EditFrameWindowManager& operator=(const EditFrameWindowManager&);

private:
	typedef std::vector<EditFrameWindow*> FrameList;

private:
	Document* pDocument_;
	SyncManager* pSyncManager_;
	SyncDialogManager* pSyncDialogManager_;
	qs::KeyMap* pKeyMap_;
	qs::Profile* pProfile_;
	qs::MenuManager* pMenuManager_;
	qs::ToolbarManager* pToolbarManager_;
	FrameList listFrame_;
};


/****************************************************************************
 *
 * EditFrameWindowCreateContext
 *
 */

struct EditFrameWindowCreateContext
{
	Document* pDocument_;
	SyncManager* pSyncManager_;
	SyncDialogManager* pSyncDialogManager_;
	qs::MenuManager* pMenuManager_;
	qs::ToolbarManager* pToolbarManager_;
	qs::KeyMap* pKeyMap_;
};

}

#endif // __EDITFRAMEWINDOW_H__
