/*
 * $Id$
 *
 * Copyright(C) 1998-2005 Satoshi Nakamura
 * All rights reserved.
 *
 */

#ifndef __STATUSBAR_H__
#define __STATUSBAR_H__

#include <qm.h>

#include <qsmime.h>
#include <qswindow.h>


namespace qm {

class StatusBar;
	class MessageStatusBar;

class Message;
class MessageHolder;
class MessageWindow;
class EncodingModel;


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
	bool setParts(int* pnWidth,
				 size_t nCount);
	bool setText(int nPart,
				 const WCHAR* pwszText);
#ifndef _WIN32_WCE
	bool setIcon(int nPart,
				 HICON hIcon);
#endif
	void setSimple(bool bSimple);

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
					 int nOffset);
	virtual ~MessageStatusBar();

public:
	void updateMessageParts(MessageHolder* pmh,
							Message& msg,
							const qs::ContentTypeParser* pContentType);

private:
	void setIconOrText(int nPart,
					   UINT nIcon,
					   const WCHAR* pwszText);
#ifndef _WIN32_WCE
	void setIconId(int nPart,
				   UINT nIcon);
#endif

private:
	MessageStatusBar(const MessageStatusBar&);
	MessageStatusBar& operator=(const MessageStatusBar&);

private:
	MessageWindow* pMessageWindow_;
	EncodingModel* pEncodingModel_;
	int nOffset_;
};

}

#endif // __STATUSBAR_H__
