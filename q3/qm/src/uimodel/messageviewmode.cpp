/*
 * $Id$
 *
 * Copyright(C) 1998-2006 Satoshi Nakamura
 * All rights reserved.
 */

#pragma warning(disable:4786)

#include <qsassert.h>

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

void qm::AbstractMessageViewMode::fireModeChanged(unsigned int nModeAdded,
												  unsigned int nModeRemoved) const
{
	MessageViewModeEvent event(nModeAdded, nModeRemoved);
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
 * DefaultMessageViewMode
 *
 */

qm::DefaultMessageViewMode::DefaultMessageViewMode() :
	nMode_(MODE_NONE),
	nZoom_(ZOOM_NONE),
	fit_(FIT_NONE)
{
}

qm::DefaultMessageViewMode::DefaultMessageViewMode(unsigned int nMode,
												   unsigned int nZoom,
												   Fit fit) :
	nMode_(nMode),
	nZoom_(nZoom),
	fit_(fit)
{
}

qm::DefaultMessageViewMode::~DefaultMessageViewMode()
{
}

unsigned int qm::DefaultMessageViewMode::getMode() const
{
	return nMode_;
}

void qm::DefaultMessageViewMode::setMode(unsigned int nMode,
										 unsigned int nMask)
{
	unsigned int nOld = nMode_;
	nMode_ = (nMode & nMask) | (nMode_ & ~nMask);
	if (nMode_ != nOld)
		fireModeChanged(nMode_ - (nOld & nMode_), nOld - (nOld & nMode_));
}

unsigned int qm::DefaultMessageViewMode::getZoom() const
{
	return nZoom_;
}

void qm::DefaultMessageViewMode::setZoom(unsigned int nZoom)
{
	assert(nZoom == -1 || (ZOOM_MIN <= nZoom && nZoom <= ZOOM_MAX));
	
	if (nZoom != nZoom_) {
		nZoom_ = nZoom;
		fireZoomChanged();
	}
}

MessageViewMode::Fit qm::DefaultMessageViewMode::getFit() const
{
	return fit_;
}

void qm::DefaultMessageViewMode::setFit(Fit fit)
{
	if (fit != fit_) {
		fit_ = fit;
		fireFitChanged();
	}
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
	nModeAdded_(0),
	nModeRemoved_(0)
{
}

qm::MessageViewModeEvent::MessageViewModeEvent(unsigned int nModeAdded,
											   unsigned int nModeRemoved) :
	nModeAdded_(nModeAdded),
	nModeRemoved_(nModeRemoved)
{
}

qm::MessageViewModeEvent::~MessageViewModeEvent()
{
}

unsigned int qm::MessageViewModeEvent::getAddedMode() const
{
	return nModeAdded_;
}

unsigned int qm::MessageViewModeEvent::getRemovedMode() const
{
	return nModeRemoved_;
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
