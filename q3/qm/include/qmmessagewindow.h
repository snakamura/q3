/*
 * $Id$
 *
 * Copyright(C) 1998-2003 Satoshi Nakamura
 * All rights reserved.
 *
 */

#ifndef __QMMESSAGEWINDOW_H__
#define __QMMESSAGEWINDOW_H__

#include <qm.h>
#include <qmaction.h>
#include <qmview.h>

#include <qskeymap.h>
#include <qsprofile.h>
#include <qswindow.h>


namespace qm {

class MessageFrameWindow;
class MessageWindow;
class MessageWindowHandler;
class MessageWindowEvent;
class HeaderWindow;

class AttachmentSelectionModel;
class Document;
class Message;
class MessageFrameWindowManager;
class MessageHolder;
class MessageMessageModel;
class MessageModel;
class MessageWindowItem;
class ViewModel;
class ViewModelManager;
class TemplateContext;


/****************************************************************************
 *
 * MessageFrameWindow
 *
 */

class QMEXPORTCLASS MessageFrameWindow : public qs::FrameWindow
{
public:
	MessageFrameWindow(MessageFrameWindowManager* pMessageFrameWindowManager,
		ViewModelManager* pViewModelManager,
		qs::Profile* pProfile, qs::QSTATUS* pstatus);
	virtual ~MessageFrameWindow();

public:
	MessageMessageModel* getMessageModel() const;
	const ActionInvoker* getActionInvoker() const;
	void initialShow();
	qs::QSTATUS save();
	
	bool isShowToolbar() const;
	qs::QSTATUS setShowToolbar(bool bShow);
	bool isShowStatusBar() const;
	qs::QSTATUS setShowStatusBar(bool bShow);

protected:
	virtual qs::QSTATUS getToolbarButtons(Toolbar* pToolbar, bool* pbToolbar);
	virtual qs::QSTATUS createToolbarButtons(void* pCreateParam, HWND hwndToolbar);
#if defined _WIN32_WCE && (_WIN32_WCE < 300 || !defined _WIN32_WCE_PSPC)
	virtual qs::QSTATUS getBarId(int n, UINT* pnId) const;
	virtual qs::QSTATUS getCommandBandsRestoreInfo(int n,
		COMMANDBANDSRESTOREINFO* pcbri) const;
	virtual qs::QSTATUS setCommandBandsRestoreInfo(int n,
		const COMMANDBANDSRESTOREINFO& cbri);
#endif
	virtual qs::QSTATUS getMenuHandle(void* pCreateParam, HMENU* phmenu);
	virtual qs::QSTATUS getIconId(UINT* pnId);

public:
	virtual qs::QSTATUS getWindowClass(WNDCLASS* pwc);
	virtual qs::QSTATUS preCreateWindow(CREATESTRUCT* pCreateStruct);
	virtual qs::QSTATUS getAction(UINT nId, qs::Action** ppAction);
	virtual qs::QSTATUS getAccelerator(qs::Accelerator** ppAccelerator);
	virtual LRESULT windowProc(UINT uMsg, WPARAM wParam, LPARAM lParam);

protected:
	LRESULT onActivate(UINT nFlags, HWND hwnd, bool bMinimized);
	LRESULT onClose();
	LRESULT onCreate(CREATESTRUCT* pCreateStruct);
	LRESULT onDestroy();
	LRESULT onInitMenuPopup(HMENU hmenu, UINT nIndex, bool bSysMenu);
	LRESULT onSize(UINT nFlags, int cx, int cy);

private:
	MessageFrameWindow(const MessageFrameWindow&);
	MessageFrameWindow& operator=(const MessageFrameWindow&);

private:
	class MessageFrameWindowImpl* pImpl_;
};


/****************************************************************************
 *
 * MessageWindow
 *
 */

class MessageWindow :
	public qs::WindowBase,
	public qs::DefaultWindowHandler,
	public View
{
public:
	MessageWindow(MessageModel* pMessageModel, qs::Profile* pProfile,
		const WCHAR* pwszSection, qs::QSTATUS* pstatus);
	virtual ~MessageWindow();

public:
	bool isShowHeaderWindow() const;
	qs::QSTATUS setShowHeaderWindow(bool bShow);
	bool isRawMode() const;
	qs::QSTATUS setRawMode(bool bRawMode);
	bool isHtmlMode() const;
	qs::QSTATUS setHtmlMode(bool bHtmlMode);
	bool isHtmlOnlineMode() const;
	qs::QSTATUS setHtmlOnlineMode(bool bHtmlOnlineMode);
	bool isDecryptVerifyMode() const;
	qs::QSTATUS setDecryptVerifyMode(bool bDecryptVerifyMode);
	const WCHAR* getEncoding() const;
	qs::QSTATUS setEncoding(const WCHAR* pwszEncoding);
	const WCHAR* getTemplate() const;
	qs::QSTATUS setTemplate(const WCHAR* pwszTemplate);
	qs::QSTATUS scrollPage(bool bPrev, bool* pbScrolled);
	bool isSelectMode() const;
	qs::QSTATUS setSelectMode(bool bSelectMode);
	qs::QSTATUS find(const WCHAR* pwszFind,
		bool bMatchCase, bool bPrev, bool* pbFound);
	qs::QSTATUS openLink();
	MessageWindowItem* getFocusedItem() const;
	
	MessageModel* getMessageModel() const;
	AttachmentSelectionModel* getAttachmentSelectionModel() const;
	
	qs::QSTATUS save();
	
	qs::QSTATUS addMessageWindowHandler(MessageWindowHandler* pHandler);
	qs::QSTATUS removeMessageWindowHandler(MessageWindowHandler* pHandler);

public:
	virtual qs::QSTATUS getAccelerator(qs::Accelerator** ppAccelerator);
	virtual LRESULT windowProc(UINT uMsg, WPARAM wParam, LPARAM lParam);

protected:
	LRESULT onCreate(CREATESTRUCT* pCreateStruct);
	LRESULT onDestroy();
	LRESULT onLButtonDown(UINT nFlags, const POINT& pt);
	LRESULT onSize(UINT nFlags, int cx, int cy);
	
	LRESULT onMessageModelMessageChanged(WPARAM wParam, LPARAM lParam);

public:
	virtual bool isShow() const;
	virtual bool isActive() const;
	virtual qs::QSTATUS setActive();

private:
	MessageWindow(const MessageWindow&);
	MessageWindow& operator=(const MessageWindow&);

private:
	class MessageWindowImpl* pImpl_;
};


/****************************************************************************
 *
 * MessageWindowHandler
 *
 */

class MessageWindowHandler
{
public:
	virtual ~MessageWindowHandler();

public:
	virtual qs::QSTATUS messageChanged(const MessageWindowEvent& event) = 0;
};


/****************************************************************************
 *
 * MessageWindowEvent
 *
 */

class MessageWindowEvent
{
public:
	MessageWindowEvent(MessageHolder* pmh, Message& msg);
	~MessageWindowEvent();

public:
	MessageHolder* getMessageHolder() const;
	Message& getMessage() const;

private:
	MessageWindowEvent(const MessageWindowEvent&);
	MessageWindowEvent& operator=(const MessageWindowEvent&);

private:
	MessageHolder* pmh_;
	Message& msg_;
};


/****************************************************************************
 *
 * HeaderWindow
 *
 */

class HeaderWindow :
	public qs::WindowBase,
	public qs::DefaultWindowHandler
{
public:
	HeaderWindow(qs::Profile* pProfile, qs::QSTATUS* pstatus);
	virtual ~HeaderWindow();

public:
	int getHeight() const;
	qs::QSTATUS setMessage(const TemplateContext* pContext);
	qs::QSTATUS layout();
	bool isActive() const;
	MessageWindowItem* getFocusedItem() const;
	AttachmentSelectionModel* getAttachmentSelectionModel() const;

public:
	virtual qs::QSTATUS getWindowClass(WNDCLASS* pwc);
	virtual LRESULT windowProc(UINT uMsg, WPARAM wParam, LPARAM lParam);

protected:
	LRESULT onCreate(CREATESTRUCT* pCreateStruct);
	LRESULT onCtlColorStatic(HDC hdc, HWND hwnd);
	LRESULT onDestroy();
	LRESULT onSize(UINT nFlags, int cx, int cy);

private:
	HeaderWindow(const HeaderWindow&);
	HeaderWindow& operator=(const HeaderWindow&);

private:
	class HeaderWindowImpl* pImpl_;
};

}

#endif // __QMMESSAGEWINDOW_H__
