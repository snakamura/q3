/*
 * $Id$
 *
 * Copyright(C) 1998-2005 Satoshi Nakamura
 * All rights reserved.
 */

#ifndef __MESSAGEVIEWMODE_H__
#define __MESSAGEVIEWMODE_H__

#include <qm.h>

#include <vector>

namespace qm {

class MessageViewMode;
	class AbstractMessageViewMode;
		class DefaultMessageViewMode;
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
		MODE_NONE			= 0x0000,
		MODE_RAW			= 0x0001,
		MODE_SOURCE			= 0x0002,
		MODE_SELECT			= 0x0010,
		MODE_QUOTE			= 0x0020,
		MODE_HTML			= 0x0100,
		MODE_HTMLONLINE		= 0x0200,
		MODE_INTERNETZONE	= 0x0400
	};
	
	enum Fit {
		FIT_NONE,
		FIT_NORMAL,
		FIT_SUPER
	};
	
	enum {
		ZOOM_NONE		= -1,
		ZOOM_MIN		= 0,
		ZOOM_MAX		= 4
	};

public:
	virtual ~MessageViewMode();

public:
	virtual unsigned int getMode() const = 0;
	virtual void setMode(unsigned int nMode,
						 unsigned int nMask) = 0;
	virtual unsigned int getZoom() const = 0;
	virtual void setZoom(unsigned int nZoom) = 0;
	virtual Fit getFit() const = 0;
	virtual void setFit(Fit fit) = 0;
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
	void fireModeChanged(unsigned int nModeAdded,
						 unsigned int nModeRemoved) const;
	void fireZoomChanged() const;
	void fireFitChanged() const;

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
 * DefaultMessageViewMode
 *
 */

class DefaultMessageViewMode : public AbstractMessageViewMode
{
public:
	DefaultMessageViewMode();
	DefaultMessageViewMode(unsigned int nMode,
						   unsigned int nZoom,
						   Fit fit);
	virtual ~DefaultMessageViewMode();

public:
	virtual unsigned int getMode() const;
	virtual void setMode(unsigned int nMode,
						 unsigned int nMask);
	virtual unsigned int getZoom() const;
	virtual void setZoom(unsigned int nZoom);
	virtual Fit getFit() const;
	virtual void setFit(Fit fit);

private:
	DefaultMessageViewMode(const DefaultMessageViewMode&);
	DefaultMessageViewMode& operator=(const DefaultMessageViewMode&);

private:
	unsigned int nMode_;
	unsigned int nZoom_;
	Fit fit_;
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
	virtual void modeChanged(const MessageViewModeEvent& event) = 0;
	virtual void zoomChanged(const MessageViewModeEvent& event) = 0;
	virtual void fitChanged(const MessageViewModeEvent& event) = 0;
};


/****************************************************************************
 *
 * MessageViewModeEvent
 *
 */

class MessageViewModeEvent
{
public:
	MessageViewModeEvent();
	MessageViewModeEvent(unsigned int nModeAdded,
						 unsigned int nModeRemoved);
	~MessageViewModeEvent();

public:
	unsigned int getAddedMode() const;
	unsigned int getRemovedMode() const;

private:
	MessageViewModeEvent(const MessageViewModeEvent&);
	MessageViewModeEvent& operator=(const MessageViewModeEvent&);

private:
	unsigned int nModeAdded_;
	unsigned int nModeRemoved_;
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
	virtual MessageViewMode* getMessageViewMode() = 0;
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
