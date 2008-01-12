/*
 * $Id$
 *
 * Copyright(C) 1998-2008 Satoshi Nakamura
 *
 */

#ifndef __MESSAGEFRAMEWINDOW_H__
#define __MESSAGEFRAMEWINDOW_H__

#include <qm.h>

#include <qsprofile.h>

#include "statusbar.h"


namespace qm {

class MessageFrameWindowManager;
struct MessageFrameWindowCreateContext;

class Document;
class EditFrameWindowManager;
class EncodingModel;
class ExternalEditorManager;
class FolderImage;
class MessageFrameWindow;
class MessageHolder;
class MessageModel;
class MessageWindow;
class MessageWindowFontManager;
class SyncManager;
class TempFileCleaner;
class UIManager;
class ViewModelManager;


/****************************************************************************
 *
 * MessageFrameWindowManager
 *
 */

class MessageFrameWindowManager
{
public:
	MessageFrameWindowManager(Document* pDocument,
							  UIManager* pUIManager,
							  SyncManager* pSyncManager,
							  TempFileCleaner* pTempFileCleaner,
							  const FolderImage* pFolderImage,
							  qs::Profile* pProfile,
							  ViewModelManager* pViewModelManager,
							  EditFrameWindowManager* pEditFrameWindowManager,
							  ExternalEditorManager* pExternalEditorManager,
							  MessageWindowFontManager* pFontManager);
	~MessageFrameWindowManager();

public:
	bool open(ViewModel* pViewModel,
			  MessageHolder* pmh);
	bool open(const CHAR* pszMessage,
			  size_t nLen,
			  const MessagePtr& parentPtr);
	void close(MessageFrameWindow* pMessageFrameWindow);
	void closeAll();
	void showAll();
	void hideAll();
	void layout();
	void reloadProfiles();
	void save() const;

private:
	MessageFrameWindow* create();
	MessageFrameWindow* prepare();

private:
	MessageFrameWindowManager(const MessageFrameWindowManager&);
	MessageFrameWindowManager& operator=(const MessageFrameWindowManager&);

private:
	typedef std::vector<MessageFrameWindow*> FrameList;

private:
	Document* pDocument_;
	UIManager* pUIManager_;
	SyncManager* pSyncManager_;
	TempFileCleaner* pTempFileCleaner_;
	const FolderImage* pFolderImage_;
	qs::Profile* pProfile_;
	ViewModelManager* pViewModelManager_;
	EditFrameWindowManager* pEditFrameWindowManager_;
	ExternalEditorManager* pExternalEditorManager_;
	MessageWindowFontManager* pFontManager_;
	FrameList listFrame_;
	MessageFrameWindow* pCachedFrame_;
};


/****************************************************************************
 *
 * MessageFrameWindowCreateContext
 *
 */

struct MessageFrameWindowCreateContext
{
	Document* pDocument_;
	UIManager* pUIManager_;
	SyncManager* pSyncManager_;
	EditFrameWindowManager* pEditFrameWindowManager_;
	ExternalEditorManager* pExternalEditorManager_;
	TempFileCleaner* pTempFileCleaner_;
	const FolderImage* pFolderImage_;
	const MessageWindowFontManager* pFontManager_;
};

}

#endif // __MESSAGEFRAMEWINDOW_H__
