/*
 * $Id$
 *
 * Copyright(C) 1998-2004 Satoshi Nakamura
 * All rights reserved.
 */

#ifndef __MESSAGEVIEWMODE_H__
#define __MESSAGEVIEWMODE_H__

#include <qm.h>

#include <vector>

namespace qm {

class MessageViewMode;
	class AbstractMessageViewMode;
class MessageViewModeHandler;
class MessageViewModeEvent;
class MessageViewModeHolder;
	class AbstractMessageViewModeHolder;
class MessageViewModeHolderHandler;
class MessageViewModeHolderEvent;


/****************************************************************************
 *
 * MessageViewMode
 *
 */

class MessageViewMode
{
public:
	enum Mode {
		MODE_RAW			= 0x01,
		MODE_HTML			= 0x02,
		MODE_HTMLONLINE		= 0x04,
		MODE_SELECT			= 0x08,
		MODE_QUOTE			= 0x10
	};

public:
	virtual ~MessageViewMode();

public:
	virtual bool isMode(Mode mode) const = 0;
	virtual void setMode(Mode mode,
						 bool b) = 0;
	virtual void addMessageViewModeHandler(MessageViewModeHandler* pHandler) = 0;
	virtual void removeMessageViewModeHandler(MessageViewModeHandler* pHandler) = 0;
};


/****************************************************************************
 *
 * AbstractMessageViewMode
 *
 */

class AbstractMessageViewMode : public MessageViewMode
{
public:
	AbstractMessageViewMode();
	virtual ~AbstractMessageViewMode();

public:
	virtual void addMessageViewModeHandler(MessageViewModeHandler* pHandler);
	virtual void removeMessageViewModeHandler(MessageViewModeHandler* pHandler);

protected:
	void fireMessageViewModeChanged(Mode mode,
									bool b) const;

private:
	AbstractMessageViewMode(const AbstractMessageViewMode&);
	AbstractMessageViewMode& operator=(const AbstractMessageViewMode&);

private:
	typedef std::vector<MessageViewModeHandler*> HandlerList;

private:
	HandlerList listHandler_;
};


/****************************************************************************
 *
 * MessageViewModeHandler
 *
 */

class MessageViewModeHandler
{
public:
	virtual ~MessageViewModeHandler();

public:
	virtual void messageViewModeChanged(const MessageViewModeEvent& event) = 0;
};


/****************************************************************************
 *
 * MessageViewModeEvent
 *
 */

class MessageViewModeEvent
{
public:
	MessageViewModeEvent(MessageViewMode::Mode mode,
						 bool b);
	~MessageViewModeEvent();

public:
	MessageViewMode::Mode getMode() const;
	bool isSet() const;

private:
	MessageViewModeEvent(const MessageViewModeEvent&);
	MessageViewModeEvent& operator=(const MessageViewModeEvent&);

private:
	MessageViewMode::Mode mode_;
	bool b_;
};


/****************************************************************************
 *
 * MessageViewModeHolder
 *
 */

class MessageViewModeHolder
{
public:
	virtual ~MessageViewModeHolder();

public:
	virtual MessageViewMode* getMessageViewMode() const = 0;
	virtual void addMessageViewModeHolderHandler(MessageViewModeHolderHandler* pHandler) = 0;
	virtual void removeMessageViewModeHolderHandler(MessageViewModeHolderHandler* pHandler) = 0;
};


/****************************************************************************
 *
 * AbstractMessageViewModeHolder
 *
 */

class AbstractMessageViewModeHolder : public MessageViewModeHolder
{
public:
	AbstractMessageViewModeHolder();
	virtual ~AbstractMessageViewModeHolder();

public:
	virtual void addMessageViewModeHolderHandler(MessageViewModeHolderHandler* pHandler);
	virtual void removeMessageViewModeHolderHandler(MessageViewModeHolderHandler* pHandler);

protected:
	void fireMessageViewModeChanged(MessageViewMode* pNewMode,
									MessageViewMode* pOldMode);

private:
	AbstractMessageViewModeHolder(const AbstractMessageViewModeHolder&);
	AbstractMessageViewModeHolder& operator=(const AbstractMessageViewModeHolder&);

private:
	typedef std::vector<MessageViewModeHolderHandler*> HandlerList;

private:
	HandlerList listHandler_;
};


/****************************************************************************
 *
 * MessageViewModeHolderHandler
 *
 */

class MessageViewModeHolderHandler
{
public:
	virtual ~MessageViewModeHolderHandler();

public:
	virtual void messageViewModeChanged(const MessageViewModeHolderEvent& event) = 0;
};


/****************************************************************************
 *
 * MessageViewModeHolderEvent
 *
 */

class MessageViewModeHolderEvent
{
public:
	MessageViewModeHolderEvent(MessageViewModeHolder* pHolder,
							   MessageViewMode* pNewMode,
							   MessageViewMode* pOldMode);
	~MessageViewModeHolderEvent();

public:
	MessageViewModeHolder* getMessageViewModeHolder() const;
	MessageViewMode* getNewMessageViewMode() const;
	MessageViewMode* getOldMessageViewMode() const;

private:
	MessageViewModeHolderEvent(const MessageViewModeHolderEvent&);
	MessageViewModeHolderEvent& operator=(const MessageViewModeHolderEvent&);

private:
	MessageViewModeHolder* pHolder_;
	MessageViewMode* pNewMode_;
	MessageViewMode* pOldMode_;
};

}

#endif // __MESSAGEVIEWMODE_H__
