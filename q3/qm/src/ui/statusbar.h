/*
 * $Id$
 *
 * Copyright(C) 1998-2006 Satoshi Nakamura
 * All rights reserved.
 *
 */

#ifndef __STATUSBAR_H__
#define __STATUSBAR_H__

#include <qm.h>

#include <qsmenu.h>
#include <qsmime.h>
#include <qswindow.h>


namespace qm {

class StatusBar;
	class MessageStatusBar;

class EncodingModel;
class Message;
class MessageHolder;
class MessageWindow;


/****************************************************************************
 *
 * StatusBar
 *
 */

class StatusBar :
	public qs::WindowBase,
	public qs::DefaultWindowHandler
{
public:
	StatusBar();
	virtual ~StatusBar();

public:
	int getParts(int nParts,
				 int* pnWidth) const;
	bool setParts(int* pnWidth,
				  size_t nCount);
	bool setText(int nPart,
				 const WCHAR* pwszText);
#ifndef _WIN32_WCE
	bool setIcon(int nPart,
				 HICON hIcon);
#endif
	void setSimple(bool bSimple);
	bool getRect(int nPart,
				 RECT* pRect) const;

private:
	StatusBar(const StatusBar&);
	StatusBar& operator=(const StatusBar&);
};


/****************************************************************************
 *
 * MessageStatusBar
 *
 */

class MessageStatusBar : public StatusBar
{
public:
	MessageStatusBar(MessageWindow* pMessageWindow,
					 EncodingModel* pEncodingModel,
					 int nOffset,
					 qs::MenuManager* pMenuManager);
	virtual ~MessageStatusBar();

public:
	void updateMessageParts(MessageHolder* pmh,
							const Message& msg);

public:
	virtual LRESULT windowProc(UINT uMsg,
							   WPARAM wParam,
							   LPARAM lParam);

protected:
	LRESULT onContextMenu(HWND hwnd,
						  const POINT& pt);
	LRESULT onLButtonDown(UINT nFlags,
						  const POINT& pt);

protected:
	int getPart(const POINT& pt) const;
	void setIconOrText(int nPart,
					   UINT nIcon,
					   const WCHAR* pwszText);
#ifndef _WIN32_WCE
	void setIconId(int nPart,
				   UINT nIcon);
#endif

protected:
	virtual const WCHAR* getMenuName(int nPart);
	virtual UINT getClickActionId(int nPart);

private:
	MessageStatusBar(const MessageStatusBar&);
	MessageStatusBar& operator=(const MessageStatusBar&);

private:
	MessageWindow* pMessageWindow_;
	EncodingModel* pEncodingModel_;
	int nOffset_;
	qs::MenuManager* pMenuManager_;
};

}

#endif // __STATUSBAR_H__
