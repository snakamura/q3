/*
 * $Id$
 *
 * Copyright(C) 1998-2005 Satoshi Nakamura
 * All rights reserved.
 *
 */

#ifndef __MESSAGEFRAMEWINDOW_H__
#define __MESSAGEFRAMEWINDOW_H__

#include <qm.h>

#include <qsprofile.h>


namespace qm {

class MessageFrameWindowManager;

class Document;
class EditFrameWindowManager;
class ExternalEditorManager;
class MessageFrameWindow;
class MessageHolder;
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
							  TempFileCleaner* pTempFileCleaner,
							  qs::Profile* pProfile,
							  ViewModelManager* pViewModelManager,
							  EditFrameWindowManager* pEditFrameWindowManager,
							  ExternalEditorManager* pExternalEditorManager);
	~MessageFrameWindowManager();

public:
	bool open(ViewModel* pViewModel,
			  MessageHolder* pmh);
	void close(MessageFrameWindow* pMessageFrameWindow);
	void preModalDialog(HWND hwndParent);
	void postModalDialog(HWND hwndParent);
	bool save() const;

private:
	MessageFrameWindow* create();

private:
	MessageFrameWindowManager(const MessageFrameWindowManager&);
	MessageFrameWindowManager& operator=(const MessageFrameWindowManager&);

private:
	typedef std::vector<MessageFrameWindow*> FrameList;

private:
	Document* pDocument_;
	UIManager* pUIManager_;
	TempFileCleaner* pTempFileCleaner_;
	qs::Profile* pProfile_;
	ViewModelManager* pViewModelManager_;
	EditFrameWindowManager* pEditFrameWindowManager_;
	ExternalEditorManager* pExternalEditorManager_;
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
	EditFrameWindowManager* pEditFrameWindowManager_;
	ExternalEditorManager* pExternalEditorManager_;
	TempFileCleaner* pTempFileCleaner_;
};

}

#endif // __MESSAGEFRAMEWINDOW_H__
