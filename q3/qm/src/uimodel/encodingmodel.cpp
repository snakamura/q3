/*
 * $Id$
 *
 * Copyright(C) 1998-2007 Satoshi Nakamura
 * All rights reserved.
 *
 */

#include "encodingmodel.h"

using namespace qm;
using namespace qs;


/****************************************************************************
 *
 * EncodingModel
 *
 */

qm::EncodingModel::~EncodingModel()
{
}


/****************************************************************************
 *
 * DefaultEncodingModel
 *
 */

qm::DefaultEncodingModel::DefaultEncodingModel()
{
}

qm::DefaultEncodingModel::~DefaultEncodingModel()
{
}

const WCHAR* qm::DefaultEncodingModel::getEncoding() const
{
	return wstrEncoding_.get();
}

void qm::DefaultEncodingModel::setEncoding(const WCHAR* pwszEncoding)
{
	if ((!wstrEncoding_.get() && !pwszEncoding) ||
		(wstrEncoding_.get() && pwszEncoding && wcscmp(wstrEncoding_.get(), pwszEncoding) == 0))
		return;
	
	if (pwszEncoding)
		wstrEncoding_ = allocWString(pwszEncoding);
	else
		wstrEncoding_.reset(0);
	
	fireEncodingChanged(pwszEncoding);
}

void qm::DefaultEncodingModel::addEncodingModelHandler(EncodingModelHandler* pHandler)
{
	listHandler_.push_back(pHandler);
}

void qm::DefaultEncodingModel::removeEncodingModelHandler(EncodingModelHandler* pHandler)
{
	HandlerList::iterator it = std::remove(
		listHandler_.begin(), listHandler_.end(), pHandler);
	listHandler_.erase(it, listHandler_.end());
}

void qm::DefaultEncodingModel::fireEncodingChanged(const WCHAR* pwszEncoding)
{
	EncodingModelEvent event(this, pwszEncoding);
	for (HandlerList::const_iterator it = listHandler_.begin(); it != listHandler_.end(); ++it)
		(*it)->encodingChanged(event);
}


/****************************************************************************
 *
 * EncodingModelHandler
 *
 */

qm::EncodingModelHandler::~EncodingModelHandler()
{
}


/****************************************************************************
 *
 * EncodingModelEvent
 *
 */

qm::EncodingModelEvent::EncodingModelEvent(EncodingModel* pEncodingModel,
										   const WCHAR* pwszEncoding) :
	pEncodingModel_(pEncodingModel),
	pwszEncoding_(pwszEncoding)
{
}

qm::EncodingModelEvent::~EncodingModelEvent()
{
}

EncodingModel* qm::EncodingModelEvent::getEncodingModel() const
{
	return pEncodingModel_;
}

const WCHAR* qm::EncodingModelEvent::getEncoding() const
{
	return pwszEncoding_;
}
