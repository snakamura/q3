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

qm::MessageModel::~MessageModel()
{
}


/****************************************************************************
 *
 * AbstractMessageModel
 *
 */

qm::AbstractMessageModel::AbstractMessageModel(qs::QSTATUS* pstatus) :
	pAccount_(0),
	pViewModel_(0)
{
}

qm::AbstractMessageModel::~AbstractMessageModel()
{
	if (pViewModel_)
		pViewModel_->removeViewModelHandler(this);
	setCurrentAccount(0);
}

Account* qm::AbstractMessageModel::getCurrentAccount() const
{
	return pAccount_;
}

QSTATUS qm::AbstractMessageModel::setCurrentAccount(Account* pAccount)
{
	DECLARE_QSTATUS();
	
	if (pAccount != pAccount_) {
		if (pAccount_) {
			status = pAccount_->removeAccountHandler(this);
			CHECK_QSTATUS();
		}
		
		pAccount_ = pAccount;
		
		if (pAccount_) {
			status = pAccount_->addAccountHandler(this);
			CHECK_QSTATUS();
		}
	}
	
	return QSTATUS_SUCCESS;
}

MessagePtr qm::AbstractMessageModel::getCurrentMessage() const
{
	return ptr_;
}

QSTATUS qm::AbstractMessageModel::setMessage(MessageHolder* pmh)
{
	DECLARE_QSTATUS();
	
	Message msg(&status);
	CHECK_QSTATUS();
	if (pmh) {
		status = setCurrentAccount(pmh->getFolder()->getAccount());
		CHECK_QSTATUS();
	}
	
	ptr_.reset(pmh);
	
	status = fireMessageChanged(pmh);
	CHECK_QSTATUS();
	
	return QSTATUS_SUCCESS;
}

QSTATUS qm::AbstractMessageModel::addMessageModelHandler(
	MessageModelHandler* pHandler)
{
	return STLWrapper<HandlerList>(listHandler_).push_back(pHandler);
}

QSTATUS qm::AbstractMessageModel::removeMessageModelHandler(
	MessageModelHandler* pHandler)
{
	HandlerList::iterator it = std::remove(
		listHandler_.begin(), listHandler_.end(), pHandler);
	listHandler_.erase(it, listHandler_.end());
	return QSTATUS_SUCCESS;
}

ViewModel* qm::AbstractMessageModel::getViewModel() const
{
	return pViewModel_;
}

QSTATUS qm::AbstractMessageModel::setViewModel(ViewModel* pViewModel)
{
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

QSTATUS qm::AbstractMessageModel::itemRemoved(const ViewModelEvent& event)
{
	DECLARE_QSTATUS();
	
	Lock<ViewModel> lock(*pViewModel_);
	
	const ViewModelItem* pItem = pViewModel_->getItem(event.getItem());
	MessagePtrLock mpl(ptr_);
	if (!mpl || mpl == pItem->getMessageHolder()) {
		status = setMessage(0);
		CHECK_QSTATUS();
	}
	
	return QSTATUS_SUCCESS;
}

QSTATUS qm::AbstractMessageModel::destroyed(const ViewModelEvent& event)
{
	assert(pViewModel_);
	
	DECLARE_QSTATUS();
	
	status = setViewModel(0);
	CHECK_QSTATUS();
	status = setMessage(0);
	CHECK_QSTATUS();
	
	return QSTATUS_SUCCESS;
}

QSTATUS qm::AbstractMessageModel::accountDestroyed(const AccountEvent& event)
{
	DECLARE_QSTATUS();
	
	status = setMessage(0);
	CHECK_QSTATUS();
	status = setCurrentAccount(0);
	CHECK_QSTATUS();
	
	return QSTATUS_SUCCESS;
}

QSTATUS qm::AbstractMessageModel::fireMessageChanged(MessageHolder* pmh) const
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
 * MessageMessageModel
 *
 */

qm::MessageMessageModel::MessageMessageModel(qs::QSTATUS* pstatus) :
	AbstractMessageModel(pstatus)
{
}

qm::MessageMessageModel::~MessageMessageModel()
{
}


/****************************************************************************
 *
 * PreviewMessageModel
 *
 */

qm::PreviewMessageModel::PreviewMessageModel(ViewModelManager* pViewModelManager,
	bool bConnectToViewModel, QSTATUS* pstatus) :
	AbstractMessageModel(pstatus),
	pViewModelManager_(pViewModelManager),
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

qm::PreviewMessageModel::~PreviewMessageModel()
{
	delete pTimer_;
}

QSTATUS qm::PreviewMessageModel::updateToViewModel()
{
	DECLARE_QSTATUS();
	
	ViewModel* pViewModel = getViewModel();
	assert(pViewModel);
	
	Lock<ViewModel> lock(*pViewModel);
	
	unsigned int nFocused = pViewModel->getFocused();
	MessageHolder* pmh = 0;
	if (nFocused < pViewModel->getCount())
		pmh = pViewModel->getMessageHolder(nFocused);
	
	MessagePtrLock mpl(getCurrentMessage());
	if (pmh != mpl) {
		status = setMessage(pmh);
		CHECK_QSTATUS();
	}
	
	return QSTATUS_SUCCESS;
}

QSTATUS qm::PreviewMessageModel::connectToViewModel()
{
	assert(!bConnectedToViewModel_);
	
	DECLARE_QSTATUS();
	
	status = pViewModelManager_->addViewModelManagerHandler(this);
	CHECK_QSTATUS();
	status = setViewModel(pViewModelManager_->getCurrentViewModel());
	CHECK_QSTATUS();
	
	bConnectedToViewModel_ = true;
	
	return QSTATUS_SUCCESS;
}

QSTATUS qm::PreviewMessageModel::disconnectFromViewModel()
{
	assert(bConnectedToViewModel_);
	
	DECLARE_QSTATUS();
	
	status = pViewModelManager_->removeViewModelManagerHandler(this);
	CHECK_QSTATUS();
	status = setViewModel(0);
	CHECK_QSTATUS();
	
	bConnectedToViewModel_ = false;
	
	return QSTATUS_SUCCESS;
}

bool qm::PreviewMessageModel::isConnectedToViewModel() const
{
	return bConnectedToViewModel_;
}

QSTATUS qm::PreviewMessageModel::itemStateChanged(const ViewModelEvent& event)
{
	DECLARE_QSTATUS();
	
	ViewModel* pViewModel = getViewModel();
	assert(pViewModel == event.getViewModel());
	
	if (event.getItem() == pViewModel->getFocused()) {
		assert(pTimer_);
		if (nTimerId_ != 0)
			pTimer_->killTimer(nTimerId_);
		nTimerId_ = TIMER_ITEMSTATECHANGED;
		status = pTimer_->setTimer(&nTimerId_, TIMEOUT, this);
		CHECK_QSTATUS();
	}
	
	return QSTATUS_SUCCESS;
}

QSTATUS qm::PreviewMessageModel::viewModelSelected(const ViewModelManagerEvent& event)
{
	DECLARE_QSTATUS();
	
	ViewModel* pViewModel = getViewModel();
	assert(pViewModel == event.getOldViewModel());
	
	status = setViewModel(event.getNewViewModel());
	CHECK_QSTATUS();
	
	setCurrentAccount(pViewModelManager_->getCurrentAccount());
	
	status = setMessage(0);
	CHECK_QSTATUS();
	
	return QSTATUS_SUCCESS;
}

QSTATUS qm::PreviewMessageModel::timerTimeout(unsigned int nId)
{
	assert(nId == nTimerId_);
	pTimer_->killTimer(nTimerId_);
	updateToViewModel();
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
