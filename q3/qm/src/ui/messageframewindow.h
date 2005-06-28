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

#include "statusbar.h"


namespace qm {

class MessageFrameWindowManager;
class MessageFrameWindowStatusBar;
struct MessageFrameWindowCreateContext;

class Document;
class EditFrameWindowManager;
class EncodingMenu;
class EncodingModel;
class ExternalEditorManager;
class MessageFrameWindow;
class MessageHolder;
class MessageModel;
class MessageWindow;
class TempFileCleaner;
class UIManager;
class ViewModelManager;
class ViewTemplateMenu;


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
	void closeAll();
	void preModalDialog(HWND hwndParent);
	void postModalDialog(HWND hwndParent);
	void layout();
	void reloadProfiles();
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
 * MessageFrameWindowStatusBar
 *
 */

class MessageFrameWindowStatusBar : public MessageStatusBar
{
public:
	MessageFrameWindowStatusBar(MessageModel* pMessageModel,
								MessageWindow* pMessageWindow,
								EncodingModel* pEncodingModel,
								int nOffset,
								EncodingMenu* pEncodingMenu,
								ViewTemplateMenu* pViewTemplateMenu);
	virtual ~MessageFrameWindowStatusBar();

private:
	virtual Account* getAccount();

private:
	MessageFrameWindowStatusBar(const MessageFrameWindowStatusBar&);
	MessageFrameWindowStatusBar& operator=(const MessageFrameWindowStatusBar&);

private:
	MessageModel* pMessageModel_;
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
