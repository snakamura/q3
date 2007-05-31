/*
 * $Id$
 *
 * Copyright(C) 1998-2007 Satoshi Nakamura
 * All rights reserved.
 *
 */

#pragma warning(disable:4786)

#include <qmaction.h>

#include <qsaction.h>

#include <algorithm>

#include <boost/bind.hpp>

#include "actioninvoker.h"
#include "../ui/actionitem.h"

using namespace qm;
using namespace qs;


/****************************************************************************
 *
 * ActionInvokerImpl
 *
 */

struct qm::ActionInvokerImpl
{
	void invoke(UINT nId,
				const WCHAR** ppParams,
				size_t nParams) const;
	
	const qs::ActionMap* pActionMap_;
	bool bPending_;
	ActionInvokerQueue queue_;
};

void qm::ActionInvokerImpl::invoke(UINT nId,
								   const WCHAR** ppParams,
								   size_t nParams) const
{
	Action* pAction = pActionMap_->getAction(nId);
	if (pAction) {
		ActionParam param(nId, ppParams, nParams);
		ActionEvent event(nId, 0, nParams != 0 ? &param : 0);
		if (pAction->isEnabled(event))
			pAction->invoke(event);
	}
}


/****************************************************************************
 *
 * ActionInvoker
 *
 */

qm::ActionInvoker::ActionInvoker(const ActionMap* pActionMap) :
	pImpl_(0)
{
	pImpl_ = new ActionInvokerImpl();
	pImpl_->pActionMap_ = pActionMap;
	pImpl_->bPending_ = false;
}

qm::ActionInvoker::~ActionInvoker()
{
	delete pImpl_;
}

void qm::ActionInvoker::invoke(UINT nId,
							   const WCHAR** ppParams,
							   size_t nParams) const
{
	if (pImpl_->bPending_) {
		std::auto_ptr<ActionInvokerQueueItem> pItem(
			new ActionInvokerQueueItem(nId, ppParams, nParams));
		pImpl_->queue_.add(pItem);
	}
	else {
		pImpl_->invoke(nId, ppParams, nParams);
	}
}

void qm::ActionInvoker::invoke(const WCHAR* pwszAction,
							   const WCHAR** ppParams,
							   size_t nParams) const
{
	assert(pwszAction);
	
	ActionItem item = {
		pwszAction,
		0
	};
	
	const ActionItem* pItem = std::lower_bound(
		actionItems, actionItems + countof(actionItems), item,
		boost::bind(string_less<WCHAR>(),
			boost::bind(&ActionItem::pwszAction_, _1),
			boost::bind(&ActionItem::pwszAction_, _2)));
	if (pItem != actionItems + countof(actionItems) &&
		wcscmp(pItem->pwszAction_, pwszAction) == 0)
		invoke(pItem->nId_, ppParams, nParams);
}

void qm::ActionInvoker::startPending()
{
	assert(pImpl_->queue_.isEmpty());
	
	pImpl_->bPending_ = true;
}

void qm::ActionInvoker::stopPending()
{
	while (!pImpl_->queue_.isEmpty()) {
		std::auto_ptr<ActionInvokerQueueItem> pItem(pImpl_->queue_.remove());
		pImpl_->invoke(pItem->getId(), pItem->getParams(), pItem->getParamCount());
	}
	
	pImpl_->bPending_ = false;
}


/****************************************************************************
 *
 * ActionInvokerQueue
 *
 */

qm::ActionInvokerQueue::ActionInvokerQueue()
{
}

qm::ActionInvokerQueue::~ActionInvokerQueue()
{
}

void qm::ActionInvokerQueue::add(std::auto_ptr<ActionInvokerQueueItem> pItem)
{
	listItem_.push_back(pItem.get());
	pItem.release();
}

std::auto_ptr<ActionInvokerQueueItem> qm::ActionInvokerQueue::remove()
{
	assert(!listItem_.empty());
	
	std::auto_ptr<ActionInvokerQueueItem> pItem(listItem_.front());
	listItem_.erase(listItem_.begin());
	return pItem;
}

bool qm::ActionInvokerQueue::isEmpty() const
{
	return listItem_.empty();
}


/****************************************************************************
 *
 * ActionInvokerQueueItem
 *
 */

qm::ActionInvokerQueueItem::ActionInvokerQueueItem(UINT nId,
												   const WCHAR** ppParams,
												   size_t nParams) :
	nId_(nId)
{
	listParam_.resize(nParams);
	for (size_t n = 0; n < nParams; ++n) {
		wstring_ptr wstr(allocWString(ppParams[n]));
		listParam_[n] = wstr.release();
	}
}

qm::ActionInvokerQueueItem::~ActionInvokerQueueItem()
{
	std::for_each(listParam_.begin(), listParam_.end(), &freeWString);
}

UINT qm::ActionInvokerQueueItem::getId() const
{
	return nId_;
}

const WCHAR** qm::ActionInvokerQueueItem::getParams() const
{
	return const_cast<const WCHAR**>(&listParam_[0]);
}

size_t qm::ActionInvokerQueueItem::getParamCount() const
{
	return listParam_.size();
}
