/*
 * $Id$
 *
 * Copyright(C) 1998-2004 Satoshi Nakamura
 * All rights reserved.
 *
 */

#ifndef __EDITFRAMEWINDOW_H__
#define __EDITFRAMEWINDOW_H__

#include <qsprofile.h>


namespace qm {

class EditFrameWindow;
class EditFrameWindowManager;

class Document;
class EditMessage;
class SecurityModel;
class SyncDialogManager;
class SyncManager;
class UIManager;


/****************************************************************************
 *
 * EditFrameWindowManager
 *
 */

class EditFrameWindowManager
{
public:
	EditFrameWindowManager(Document* pDocument,
						   UIManager* pUIManager,
						   SyncManager* pSyncManager,
						   SyncDialogManager* pSyncDialogManager,
						   qs::Profile* pProfile,
						   SecurityModel* pSecurityModel);
	~EditFrameWindowManager();

public:
	bool open(std::auto_ptr<EditMessage> pEditMessage);
	void close(EditFrameWindow* pEditFrameWindow);
	bool closeAll();
	void preModalDialog(HWND hwndParent);
	void postModalDialog(HWND hwndParent);

private:
	EditFrameWindowManager(const EditFrameWindowManager&);
	EditFrameWindowManager& operator=(const EditFrameWindowManager&);

private:
	typedef std::vector<EditFrameWindow*> FrameList;

private:
	Document* pDocument_;
	UIManager* pUIManager_;
	SyncManager* pSyncManager_;
	SyncDialogManager* pSyncDialogManager_;
	qs::Profile* pProfile_;
	SecurityModel* pSecurityModel_;
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
	UIManager* pUIManager_;
	SyncManager* pSyncManager_;
	SyncDialogManager* pSyncDialogManager_;
	SecurityModel* pSecurityModel_;
	const WCHAR* pwszClass_;
};

}

#endif // __EDITFRAMEWINDOW_H__
