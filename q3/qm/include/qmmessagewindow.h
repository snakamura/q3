/*
 * $Id$
 *
 * Copyright(C) 1998-2004 Satoshi Nakamura
 * All rights reserved.
 *
 */

#ifndef __QMMESSAGEWINDOW_H__
#define __QMMESSAGEWINDOW_H__

#include <qm.h>
#include <qmaction.h>
#include <qmview.h>

#include <qskeymap.h>
#include <qsmime.h>
#include <qsprofile.h>
#include <qswindow.h>


namespace qm {

class MessageFrameWindow;
class MessageWindow;
class MessageWindowHandler;
class MessageWindowEvent;
class MessageWindowStatusTextEvent;
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
					   qs::Profile* pProfile);
	virtual ~MessageFrameWindow();

public:
	MessageMessageModel* getMessageModel() const;
	const ActionInvoker* getActionInvoker() const;
	void initialShow();
	bool save();
	
	bool isShowToolbar() const;
	void setShowToolbar(bool bShow);
	bool isShowStatusBar() const;
	void setShowStatusBar(bool bShow);

protected:
	virtual bool getToolbarButtons(Toolbar* pToolbar);
	virtual bool createToolbarButtons(void* pCreateParam,
									  HWND hwndToolbar);
#if defined _WIN32_WCE && (_WIN32_WCE < 300 || !defined _WIN32_WCE_PSPC)
	virtual UINT getBarId(int n) const;
	virtual bool getCommandBandsRestoreInfo(int n,
											COMMANDBANDSRESTOREINFO* pcbri) const;
	virtual bool setCommandBandsRestoreInfo(int n,
											const COMMANDBANDSRESTOREINFO& cbri);
#endif
	virtual HMENU getMenuHandle(void* pCreateParam);
	virtual UINT getIconId();

public:
	virtual void getWindowClass(WNDCLASS* pwc);
	virtual bool preCreateWindow(CREATESTRUCT* pCreateStruct);
	virtual qs::Action* getAction(UINT nId);
	virtual qs::Accelerator* getAccelerator();
	virtual LRESULT windowProc(UINT uMsg,
							   WPARAM wParam,
							   LPARAM lParam);

protected:
	LRESULT onActivate(UINT nFlags,
					   HWND hwnd,
					   bool bMinimized);
	LRESULT onClose();
	LRESULT onCreate(CREATESTRUCT* pCreateStruct);
	LRESULT onDestroy();
	LRESULT onInitMenuPopup(HMENU hmenu,
							UINT nIndex,
							bool bSysMenu);
	LRESULT onSize(UINT nFlags,
				   int cx,
				   int cy);

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
	enum Find {
		FIND_MATCHCASE	= 0x01,
		FIND_REGEX		= 0x02,
		FIND_PREVIOUS	= 0x04
	};
	
	enum Mode {
		MODE_RAW			= 0x01,
		MODE_HTML			= 0x02,
		MODE_HTMLONLINE		= 0x04,
		MODE_SELECT			= 0x08,
		MODE_QUOTE			= 0x10
	};

public:
	MessageWindow(MessageModel* pMessageModel,
				  qs::Profile* pProfile,
				  const WCHAR* pwszSection);
	virtual ~MessageWindow();

public:
	bool isShowHeaderWindow() const;
	void setShowHeaderWindow(bool bShow);
	bool isMode(Mode mode) const;
	void setMode(Mode mode,
				 bool b);
	const WCHAR* getEncoding() const;
	void setEncoding(const WCHAR* pwszEncoding);
	const WCHAR* getTemplate() const;
	void setTemplate(const WCHAR* pwszTemplate);
	bool scrollPage(bool bPrev);
	bool find(const WCHAR* pwszFind,
			  unsigned int nFlags);
	unsigned int getSupportedFindFlags() const;
	bool openLink();
	MessageWindowItem* getFocusedItem() const;
	
	MessageModel* getMessageModel() const;
	AttachmentSelectionModel* getAttachmentSelectionModel() const;
	
	bool save();
	
	void addMessageWindowHandler(MessageWindowHandler* pHandler);
	void removeMessageWindowHandler(MessageWindowHandler* pHandler);

public:
	virtual qs::Accelerator* getAccelerator();
	virtual LRESULT windowProc(UINT uMsg,
							   WPARAM wParam,
							   LPARAM lParam);

protected:
	LRESULT onCreate(CREATESTRUCT* pCreateStruct);
	LRESULT onDestroy();
	LRESULT onLButtonDown(UINT nFlags,
						  const POINT& pt);
	LRESULT onSize(UINT nFlags,
				   int cx,
				   int cy);
	
	LRESULT onMessageModelMessageChanged(WPARAM wParam,
										 LPARAM lParam);

public:
	virtual bool isShow() const;
	virtual bool isActive() const;
	virtual void setActive();

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
	virtual void messageChanged(const MessageWindowEvent& event) = 0;
	virtual void statusTextChanged(const MessageWindowStatusTextEvent& event) = 0;
};


/****************************************************************************
 *
 * MessageWindowEvent
 *
 */

class MessageWindowEvent
{
public:
	MessageWindowEvent(MessageHolder* pmh,
					   Message& msg,
					   const qs::ContentTypeParser* pContentType);
	~MessageWindowEvent();

public:
	MessageHolder* getMessageHolder() const;
	Message& getMessage() const;
	const qs::ContentTypeParser* getContentType() const;

private:
	MessageWindowEvent(const MessageWindowEvent&);
	MessageWindowEvent& operator=(const MessageWindowEvent&);

private:
	MessageHolder* pmh_;
	Message& msg_;
	const qs::ContentTypeParser* pContentType_;
};


/****************************************************************************
 *
 * MessageWindowStatusTextEvent
 *
 */

class MessageWindowStatusTextEvent
{
public:
	explicit MessageWindowStatusTextEvent(const WCHAR* pwszText);
	~MessageWindowStatusTextEvent();

public:
	const WCHAR* getText() const;

private:
	MessageWindowStatusTextEvent(const MessageWindowStatusTextEvent&);
	MessageWindowStatusTextEvent& operator=(const MessageWindowStatusTextEvent&);

private:
	const WCHAR* pwszText_;
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
	explicit HeaderWindow(qs::Profile* pProfile);
	virtual ~HeaderWindow();

public:
	int getHeight() const;
	void setMessage(const TemplateContext* pContext);
	void layout(const RECT& rect);
	bool isActive() const;
	MessageWindowItem* getFocusedItem() const;
	AttachmentSelectionModel* getAttachmentSelectionModel() const;

public:
	virtual void getWindowClass(WNDCLASS* pwc);
	virtual LRESULT windowProc(UINT uMsg,
							   WPARAM wParam,
							   LPARAM lParam);

protected:
	LRESULT onCreate(CREATESTRUCT* pCreateStruct);
	LRESULT onCtlColorStatic(HDC hdc,
							 HWND hwnd);
	LRESULT onDestroy();

private:
	HeaderWindow(const HeaderWindow&);
	HeaderWindow& operator=(const HeaderWindow&);

private:
	class HeaderWindowImpl* pImpl_;
};

}

#endif // __QMMESSAGEWINDOW_H__
