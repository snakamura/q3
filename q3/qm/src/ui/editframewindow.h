/*
 * $Id$
 *
 * Copyright(C) 1998-2008 Satoshi Nakamura
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
class OptionDialogManager;
class PasswordManager;
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
						   PasswordManager* pPasswordManager,
						   SyncManager* pSyncManager,
						   SyncDialogManager* pSyncDialogManager,
						   OptionDialogManager* pOptionDialogManager,
						   qs::Profile* pProfile,
						   SecurityModel* pSecurityModel);
	~EditFrameWindowManager();

public:
	bool open(std::auto_ptr<EditMessage> pEditMessage);
	void close(EditFrameWindow* pEditFrameWindow);
	bool closeAll();
	void showAll();
	void hideAll();
	bool isOpen() const;
	void layout();
	void reloadProfiles();

private:
	EditFrameWindowManager(const EditFrameWindowManager&);
	EditFrameWindowManager& operator=(const EditFrameWindowManager&);

private:
	typedef std::vector<EditFrameWindow*> FrameList;

private:
	Document* pDocument_;
	UIManager* pUIManager_;
	PasswordManager* pPasswordManager_;
	SyncManager* pSyncManager_;
	SyncDialogManager* pSyncDialogManager_;
	OptionDialogManager* pOptionDialogManager_;
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
	PasswordManager* pPasswordManager_;
	SyncManager* pSyncManager_;
	SyncDialogManager* pSyncDialogManager_;
	OptionDialogManager* pOptionDialogManager_;
	SecurityModel* pSecurityModel_;
	const WCHAR* pwszClass_;
};

}

#endif // __EDITFRAMEWINDOW_H__
