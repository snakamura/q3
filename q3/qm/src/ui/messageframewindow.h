/*
 * $Id$
 *
 * Copyright(C) 1998-2003 Satoshi Nakamura
 * All rights reserved.
 *
 */

#ifndef __MESSAGEFRAMEWINDOW_H__
#define __MESSAGEFRAMEWINDOW_H__

#include <qm.h>

#include <qskeymap.h>
#include <qsmenu.h>
#include <qsprofile.h>


namespace qm {

class MessageFrameWindowManager;

class Document;
class EditFrameWindowManager;
class ExternalEditorManager;
class MessageFrameWindow;
class MessageHolder;
class TempFileCleaner;
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
		TempFileCleaner* pTempFileCleaner, qs::MenuManager* pMenuManager,
		qs::KeyMap* pKeyMap, qs::Profile* pProfile, ViewModelManager* pViewModelManager,
		EditFrameWindowManager* pEditFrameWindowManager,
		ExternalEditorManager* pExternalEditorManager, qs::QSTATUS* pstatus);
	~MessageFrameWindowManager();

public:
	qs::QSTATUS open(ViewModel* pViewModel, MessageHolder* pmh);
	void close(MessageFrameWindow* pMessageFrameWindow);
	qs::QSTATUS preModalDialog(HWND hwndParent);
	qs::QSTATUS postModalDialog(HWND hwndParent);

private:
	qs::QSTATUS create(MessageFrameWindow** ppFrame);

private:
	MessageFrameWindowManager(const MessageFrameWindowManager&);
	MessageFrameWindowManager& operator=(const MessageFrameWindowManager&);

private:
	typedef std::vector<MessageFrameWindow*> FrameList;

private:
	Document* pDocument_;
	TempFileCleaner* pTempFileCleaner_;
	qs::MenuManager* pMenuManager_;
	qs::KeyMap* pKeyMap_;
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
	EditFrameWindowManager* pEditFrameWindowManager_;
	ExternalEditorManager* pExternalEditorManager_;
	TempFileCleaner* pTempFileCleaner_;
	qs::MenuManager* pMenuManager_;
	qs::KeyMap* pKeyMap_;
};

}

#endif // __MESSAGEFRAMEWINDOW_H__
