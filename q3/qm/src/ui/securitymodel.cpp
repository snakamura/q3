/*
 * $Id$
 *
 * Copyright(C) 1998-2004 Satoshi Nakamura
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

qm::DefaultSecurityModel::DefaultSecurityModel(bool bDecryptVerify) :
	bDecryptVerify_(bDecryptVerify)
{
}

qm::DefaultSecurityModel::~DefaultSecurityModel()
{
}

bool qm::DefaultSecurityModel::isDecryptVerify()
{
	return bDecryptVerify_;
}

void qm::DefaultSecurityModel::setDecryptVerify(bool bDecryptVerify)
{
	if (bDecryptVerify != bDecryptVerify_) {
		bDecryptVerify_ = bDecryptVerify;
		fireDecryptVerifyChanged();
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

void qm::DefaultSecurityModel::fireDecryptVerifyChanged()
{
	SecurityModelEvent event(this);
	for (HandlerList::const_iterator it = listHandler_.begin(); it != listHandler_.end(); ++it)
		(*it)->decryptVerifyChanged(event);
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
