/*
 * $Id$
 *
 * Copyright(C) 1998-2003 Satoshi Nakamura
 * All rights reserved.
 *
 */

#include <qmmessage.h>
#include <qmmessageholder.h>
#include <qmaccount.h>

#include <qserror.h>
#include <qsnew.h>
#include <qsstl.h>

#include <algorithm>

#include "messagemodel.h"

#pragma warning(disable:4786)

using namespace qm;
using namespace qs;


/****************************************************************************
 *
 * MessageModel
 *
 */

qm::MessageModel::MessageModel(ViewModelManager* pViewModelManager,
	bool bConnectToViewModel, QSTATUS* pstatus) :
	bPreview_(true),
	pAccount_(0),
	pViewModelManager_(pViewModelManager),
	pViewModel_(pViewModelManager->getCurrentViewModel()),
	pTimer_(0),
	nTimerId_(0),
	bConnectedToViewModel_(false)
{
	assert(pstatus);
	
	DECLARE_QSTATUS();
	
	*pstatus = QSTATUS_SUCCESS;
	
	if (bConnectToViewModel) {
		status = connectToViewModel();
		CHECK_QSTATUS_SET(pstatus);
	}
	
	status = newQsObject(&pTimer_);
	CHECK_QSTATUS_SET(pstatus);
}

qm::MessageModel::MessageModel(QSTATUS* pstatus) :
	bPreview_(false),
	pAccount_(0),
	pViewModelManager_(0),
	pViewModel_(0),
	pTimer_(0),
	nTimerId_(0)
{
	assert(pstatus);
	*pstatus = QSTATUS_SUCCESS;
}

qm::MessageModel::~MessageModel()
{
	if (!pViewModelManager_ && pViewModel_)
		pViewModel_->removeViewModelHandler(this);
	
	delete pTimer_;
}

Account* qm::MessageModel::getCurrentAccount() const
{
	return pAccount_;
}

MessagePtr qm::MessageModel::getCurrentMessage() const
{
	return ptr_;
}

QSTATUS qm::MessageModel::setMessage(MessageHolder* pmh)
{
	DECLARE_QSTATUS();
	
	Message msg(&status);
	CHECK_QSTATUS();
	if (pmh)
		pAccount_ = pmh->getFolder()->getAccount();
	else
		assert(pAccount_);
	ptr_.reset(pmh);
	
	status = fireMessageChanged(pmh);
	CHECK_QSTATUS();
	
	return QSTATUS_SUCCESS;
}

ViewModel* qm::MessageModel::getViewModel() const
{
	return pViewModel_;
}

QSTATUS qm::MessageModel::setViewModel(ViewModel* pViewModel)
{
	assert(!pViewModelManager_);
	
	DECLARE_QSTATUS();
	
	if (pViewModel_) {
		status = pViewModel_->removeViewModelHandler(this);
		CHECK_QSTATUS();
	}
	
	pViewModel_ = pViewModel;
	
	if (pViewModel_) {
		status = pViewModel_->addViewModelHandler(this);
		CHECK_QSTATUS();
	}
	
	return QSTATUS_SUCCESS;
}

QSTATUS qm::MessageModel::updateToViewModel()
{
	DECLARE_QSTATUS();
	
	Lock<ViewModel> lock(*pViewModel_);
	
	unsigned int nFocused = pViewModel_->getFocused();
	MessageHolder* pmh = 0;
	if (nFocused < pViewModel_->getCount())
		pmh = pViewModel_->getMessageHolder(nFocused);
	
	MessagePtrLock mpl(ptr_);
	if (pmh != mpl) {
		status = setMessage(pmh);
		CHECK_QSTATUS();
	}
	
	return QSTATUS_SUCCESS;
}

QSTATUS qm::MessageModel::connectToViewModel()
{
	assert(pViewModelManager_);
	assert(!bConnectedToViewModel_);
	
	DECLARE_QSTATUS();
	
	status = pViewModelManager_->addViewModelManagerHandler(this);
	CHECK_QSTATUS();
	pViewModel_ = pViewModelManager_->getCurrentViewModel();
	if (pViewModel_) {
		status = pViewModel_->addViewModelHandler(this);
		CHECK_QSTATUS();
	}
	
	bConnectedToViewModel_ = true;
	
	return QSTATUS_SUCCESS;
}

QSTATUS qm::MessageModel::disconnectFromViewModel()
{
	assert(pViewModelManager_);
	assert(bConnectedToViewModel_);
	
	DECLARE_QSTATUS();
	
	status = pViewModelManager_->removeViewModelManagerHandler(this);
	CHECK_QSTATUS();
	if (pViewModel_) {
		status = pViewModel_->removeViewModelHandler(this);
		CHECK_QSTATUS();
	}
	
	bConnectedToViewModel_ = false;
	
	return QSTATUS_SUCCESS;
}

bool qm::MessageModel::isConnectedToViewModel() const
{
	return bConnectedToViewModel_;
}

QSTATUS qm::MessageModel::addMessageModelHandler(MessageModelHandler* pHandler)
{
	return STLWrapper<HandlerList>(listHandler_).push_back(pHandler);
}

QSTATUS qm::MessageModel::removeMessageModelHandler(MessageModelHandler* pHandler)
{
	HandlerList::iterator it = std::remove(
		listHandler_.begin(), listHandler_.end(), pHandler);
	listHandler_.erase(it, listHandler_.end());
	return QSTATUS_SUCCESS;
}

QSTATUS qm::MessageModel::viewModelSelected(const ViewModelManagerEvent& event)
{
	assert(pViewModel_ == event.getOldViewModel());
	
	DECLARE_QSTATUS();
	
	if (bPreview_ && pViewModel_) {
		status = pViewModel_->removeViewModelHandler(this);
		CHECK_QSTATUS();
	}
	
	pAccount_ = pViewModelManager_->getCurrentAccount();
	pViewModel_ = event.getNewViewModel();
	
	if (bPreview_ && pViewModel_) {
		status = pViewModel_->addViewModelHandler(this);
		CHECK_QSTATUS();
	}
	
	setMessage(0);
	
	return QSTATUS_SUCCESS;
}

QSTATUS qm::MessageModel::itemRemoved(const ViewModelEvent& event)
{
	DECLARE_QSTATUS();
	
	if (pViewModelManager_) {
		status = setMessage(0);
		CHECK_QSTATUS();
	}
	
	return QSTATUS_SUCCESS;
}

QSTATUS qm::MessageModel::itemStateChanged(const ViewModelEvent& event)
{
	DECLARE_QSTATUS();
	
	if (pViewModelManager_) {
		assert(pViewModel_ == event.getViewModel());
		
		if (event.getItem() == pViewModel_->getFocused()) {
			assert(pTimer_);
			if (nTimerId_ != 0)
				pTimer_->killTimer(nTimerId_);
			nTimerId_ = TIMER_ITEMSTATECHANGED;
			status = pTimer_->setTimer(&nTimerId_, TIMEOUT, this);
			CHECK_QSTATUS();
		}
	}
	
	return QSTATUS_SUCCESS;
}

QSTATUS qm::MessageModel::destroyed(const ViewModelEvent& event)
{
	assert(!pViewModelManager_);
	assert(pViewModel_);
	
	DECLARE_QSTATUS();
	
	status = setViewModel(0);
	CHECK_QSTATUS();
	status = setMessage(0);
	CHECK_QSTATUS();
	
	return QSTATUS_SUCCESS;
}

QSTATUS qm::MessageModel::timerTimeout(unsigned int nId)
{
	assert(nId == nTimerId_);
	pTimer_->killTimer(nTimerId_);
	updateToViewModel();
	return QSTATUS_SUCCESS;
}

QSTATUS qm::MessageModel::fireMessageChanged(MessageHolder* pmh) const
{
	DECLARE_QSTATUS();
	
	MessageModelEvent event(this, pmh);
	
	HandlerList::const_iterator it = listHandler_.begin();
	while (it != listHandler_.end()) {
		status = (*it)->messageChanged(event);
		CHECK_QSTATUS();
		++it;
	}
	
	return QSTATUS_SUCCESS;
}


/****************************************************************************
 *
 * MessageModelHandler
 *
 */

qm::MessageModelHandler::~MessageModelHandler()
{
}


/****************************************************************************
 *
 * MessageModelEvent
 *
 */

qm::MessageModelEvent::MessageModelEvent(
	const MessageModel* pModel, MessageHolder* pmh) :
	pModel_(pModel),
	pmh_(pmh)
{
}

qm::MessageModelEvent::~MessageModelEvent()
{
}

const MessageModel* qm::MessageModelEvent::getMessageModel() const
{
	return pModel_;
}

MessageHolder* qm::MessageModelEvent::getMessageHolder() const
{
	return pmh_;
}
