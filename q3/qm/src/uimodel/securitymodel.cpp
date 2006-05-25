/*
 * $Id$
 *
 * Copyright(C) 1998-2006 Satoshi Nakamura
 * All rights reserved.
 *
 */

#include <algorithm>

#include "securitymodel.h"

using namespace qm;


/****************************************************************************
 *
 * SecurityModel
 *
 */

qm::SecurityModel::~SecurityModel()
{
}


/****************************************************************************
 *
 * DefaultSecurityModel
 *
 */

qm::DefaultSecurityModel::DefaultSecurityModel(unsigned int nMode) :
	nMode_(nMode)
{
}

qm::DefaultSecurityModel::~DefaultSecurityModel()
{
}

unsigned int qm::DefaultSecurityModel::getSecurityMode() const
{
	return nMode_;
}

void qm::DefaultSecurityModel::setSecurityMode(SecurityMode mode,
											   bool b)
{
	unsigned int nMode = nMode_;
	if (b)
		nMode |= mode;
	else
		nMode &= ~mode;
	
	if (nMode != nMode_) {
		nMode_ = nMode;
		fireSecurityModeChanged();
	}
}

void qm::DefaultSecurityModel::addSecurityModelHandler(SecurityModelHandler* pHandler)
{
	listHandler_.push_back(pHandler);
}

void qm::DefaultSecurityModel::removeSecurityModelHandler(SecurityModelHandler* pHandler)
{
	HandlerList::iterator it = std::remove(
		listHandler_.begin(), listHandler_.end(), pHandler);
	listHandler_.erase(it, listHandler_.end());
}

void qm::DefaultSecurityModel::fireSecurityModeChanged()
{
	SecurityModelEvent event(this);
	for (HandlerList::const_iterator it = listHandler_.begin(); it != listHandler_.end(); ++it)
		(*it)->securityModeChanged(event);
}


/****************************************************************************
 *
 * SecurityModelHandler
 *
 */

qm::SecurityModelHandler::~SecurityModelHandler()
{
}


/****************************************************************************
 *
 * SecurityModelEvent
 *
 */

qm::SecurityModelEvent::SecurityModelEvent(SecurityModel* pSecurityModel) :
	pSecurityModel_(pSecurityModel)
{
}

qm::SecurityModelEvent::~SecurityModelEvent()
{
}

SecurityModel* qm::SecurityModelEvent::getSecurityModel()
{
	return pSecurityModel_;
}
