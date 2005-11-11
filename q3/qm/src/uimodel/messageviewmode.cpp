/*
 * $Id$
 *
 * Copyright(C) 1998-2005 Satoshi Nakamura
 * All rights reserved.
 */

#pragma warning(disable:4786)

#include <algorithm>

#include "messageviewmode.h"

using namespace qm;


/****************************************************************************
 *
 * MessageViewMode
 *
 */

qm::MessageViewMode::~MessageViewMode()
{
}


/****************************************************************************
 *
 * AbstractMessageViewMode
 *
*/

qm::AbstractMessageViewMode::AbstractMessageViewMode()
{
}

qm::AbstractMessageViewMode::~AbstractMessageViewMode()
{
}

void qm::AbstractMessageViewMode::addMessageViewModeHandler(MessageViewModeHandler* pHandler)
{
	listHandler_.push_back(pHandler);
}

void qm::AbstractMessageViewMode::removeMessageViewModeHandler(MessageViewModeHandler* pHandler)
{
	HandlerList::iterator it = std::remove(
		listHandler_.begin(), listHandler_.end(), pHandler);
	listHandler_.erase(it, listHandler_.end());
}

void qm::AbstractMessageViewMode::fireModeChanged(Mode mode,
												  bool b) const
{
	MessageViewModeEvent event(mode, b);
	for (HandlerList::const_iterator it = listHandler_.begin(); it != listHandler_.end(); ++it)
		(*it)->modeChanged(event);
}

void qm::AbstractMessageViewMode::fireZoomChanged() const
{
	MessageViewModeEvent event;
	for (HandlerList::const_iterator it = listHandler_.begin(); it != listHandler_.end(); ++it)
		(*it)->zoomChanged(event);
}

void qm::AbstractMessageViewMode::fireFitChanged() const
{
	MessageViewModeEvent event;
	for (HandlerList::const_iterator it = listHandler_.begin(); it != listHandler_.end(); ++it)
		(*it)->fitChanged(event);
}


/****************************************************************************
*
* MessageViewModeHandler
*
*/

qm::MessageViewModeHandler::~MessageViewModeHandler()
{
}


/****************************************************************************
*
* MessageViewModeEvent
*
*/

qm::MessageViewModeEvent::MessageViewModeEvent() :
	mode_(MessageViewMode::MODE_NONE),
	b_(false)
{
}

qm::MessageViewModeEvent::MessageViewModeEvent(MessageViewMode::Mode mode,
											   bool b) :
	mode_(mode),
	b_(b)
{
}

qm::MessageViewModeEvent::~MessageViewModeEvent()
{
}

MessageViewMode::Mode qm::MessageViewModeEvent::getMode() const
{
	return mode_;
}

bool qm::MessageViewModeEvent::isSet() const
{
	return b_;
}


/****************************************************************************
 *
 * MessageViewModeHolder
 *
 */

qm::MessageViewModeHolder::~MessageViewModeHolder()
{
}


/****************************************************************************
 *
 * AbstractMessageViewModeHolder
 *
 */

qm::AbstractMessageViewModeHolder::AbstractMessageViewModeHolder()
{
}

qm::AbstractMessageViewModeHolder::~AbstractMessageViewModeHolder()
{
}

void qm::AbstractMessageViewModeHolder::addMessageViewModeHolderHandler(MessageViewModeHolderHandler* pHandler)
{
	listHandler_.push_back(pHandler);
}

void qm::AbstractMessageViewModeHolder::removeMessageViewModeHolderHandler(MessageViewModeHolderHandler* pHandler)
{
	HandlerList::iterator it = std::remove(
		listHandler_.begin(), listHandler_.end(), pHandler);
	listHandler_.erase(it, listHandler_.end());
}

void qm::AbstractMessageViewModeHolder::fireMessageViewModeChanged(MessageViewMode* pNewMode,
																   MessageViewMode* pOldMode)
{
	MessageViewModeHolderEvent event(this, pNewMode, pOldMode);
	for (HandlerList::const_iterator it = listHandler_.begin(); it != listHandler_.end(); ++it)
		(*it)->messageViewModeChanged(event);
}


/****************************************************************************
*
* MessageViewModeHolderHandler
*
*/

qm::MessageViewModeHolderHandler::~MessageViewModeHolderHandler()
{
}


/****************************************************************************
*
* MessageViewModeHolderEvent
*
*/

qm::MessageViewModeHolderEvent::MessageViewModeHolderEvent(MessageViewModeHolder* pHolder,
														   MessageViewMode* pNewMode,
														   MessageViewMode* pOldMode) :
	pHolder_(pHolder),
	pNewMode_(pNewMode),
	pOldMode_(pOldMode)
{
}

qm::MessageViewModeHolderEvent::~MessageViewModeHolderEvent()
{
}

MessageViewModeHolder* qm::MessageViewModeHolderEvent::getMessageViewModeHolder() const
{
	return pHolder_;
}

MessageViewMode* qm::MessageViewModeHolderEvent::getNewMessageViewMode() const
{
	return pNewMode_;
}

MessageViewMode* qm::MessageViewModeHolderEvent::getOldMessageViewMode() const
{
	return pOldMode_;
}
