/*
 * $Id: editframewindow.h,v 1.1.1.1 2003/04/29 08:07:32 snakamura Exp $
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


namespace qm {

class EditFrameWindow;
class EditFrameWindowManager;

class Document;
class EditMessage;


/****************************************************************************
 *
 * EditFrameWindowManager
 *
 */

class EditFrameWindowManager
{
public:
	EditFrameWindowManager(Document* pDocument, qs::KeyMap* pKeyMap,
		qs::Profile* pProfile, qs::MenuManager* pContextMenuManager,
		qs::QSTATUS* pstatus);
	~EditFrameWindowManager();

public:
	qs::QSTATUS open(EditMessage* pEditMessage);
	void close(EditFrameWindow* pEditFrameWindow);
	qs::QSTATUS preModalDialog(HWND hwndParent);
	qs::QSTATUS postModalDialog(HWND hwndParent);

private:
	EditFrameWindowManager(const EditFrameWindowManager&);
	EditFrameWindowManager& operator=(const EditFrameWindowManager&);

private:
	typedef std::vector<EditFrameWindow*> FrameList;

private:
	Document* pDocument_;
	qs::KeyMap* pKeyMap_;
	qs::Profile* pProfile_;
	qs::MenuManager* pMenuManager_;
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
	qs::MenuManager* pMenuManager_;
	qs::KeyMap* pKeyMap_;
};

}

#endif // __EDITFRAMEWINDOW_H__
