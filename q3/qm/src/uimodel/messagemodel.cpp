/*
 * $Id$
 *
 * Copyright(C) 1998-2005 Satoshi Nakamura
 * All rights reserved.
 *
 */

#include <qmmessage.h>
#include <qmmessageholder.h>
#include <qmaccount.h>

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

qm::AbstractMessageModel::AbstractMessageModel() :
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

void qm::AbstractMessageModel::setCurrentAccount(Account* pAccount)
{
	if (pAccount != pAccount_) {
		if (pAccount_)
			pAccount_->removeAccountHandler(this);
		pAccount_ = pAccount;
		if (pAccount_)
			pAccount_->addAccountHandler(this);
	}
}

MessagePtr qm::AbstractMessageModel::getCurrentMessage() const
{
	return ptr_;
}

void qm::AbstractMessageModel::setMessage(MessageHolder* pmh)
{
	Message msg;
	if (pmh)
		setCurrentAccount(pmh->getFolder()->getAccount());
	
	ptr_.reset(pmh);
	
	fireMessageChanged(pmh);
}

void qm::AbstractMessageModel::addMessageModelHandler(MessageModelHandler* pHandler)
{
	listHandler_.push_back(pHandler);
}

void qm::AbstractMessageModel::removeMessageModelHandler(MessageModelHandler* pHandler)
{
	HandlerList::iterator it = std::remove(
		listHandler_.begin(), listHandler_.end(), pHandler);
	listHandler_.erase(it, listHandler_.end());
}

ViewModel* qm::AbstractMessageModel::getViewModel() const
{
	return pViewModel_;
}

void qm::AbstractMessageModel::setViewModel(ViewModel* pViewModel)
{
	ViewModel* pOld = pViewModel_;
	
	if (pViewModel_)
		pViewModel_->removeViewModelHandler(this);
	pViewModel_ = pViewModel;
	if (pViewModel_)
		pViewModel_->addViewModelHandler(this);
	
	fireMessageViewModeChanged(pViewModel_, pOld);
}

MessageViewMode* qm::AbstractMessageModel::getMessageViewMode()
{
	return pViewModel_;
}

void qm::AbstractMessageModel::itemRemoved(const ViewModelEvent& event)
{
	Lock<ViewModel> lock(*pViewModel_);
	
	MessagePtrLock mpl(ptr_);
	if (!mpl)
		setMessage(0);
}

void qm::AbstractMessageModel::destroyed(const ViewModelEvent& event)
{
	assert(pViewModel_);
	
	setViewModel(0);
	setMessage(0);
}

void qm::AbstractMessageModel::accountDestroyed(const AccountEvent& event)
{
	setMessage(0);
	setCurrentAccount(0);
}

void qm::AbstractMessageModel::fireMessageChanged(MessageHolder* pmh) const
{
	MessageModelEvent event(this, pmh);
	
	for (HandlerList::const_iterator it = listHandler_.begin(); it != listHandler_.end(); ++it)
		(*it)->messageChanged(event);
}


/****************************************************************************
 *
 * MessageMessageModel
 *
 */

qm::MessageMessageModel::MessageMessageModel()
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
											 bool bConnectToViewModel) :
	pViewModelManager_(pViewModelManager),
	nTimerId_(0),
	bConnectedToViewModel_(false)
{
	if (bConnectToViewModel)
		connectToViewModel();
	
	pTimer_.reset(new Timer());
}

qm::PreviewMessageModel::~PreviewMessageModel()
{
}

void qm::PreviewMessageModel::updateToViewModel()
{
	ViewModel* pViewModel = getViewModel();
	assert(pViewModel);
	
	Lock<ViewModel> lock(*pViewModel);
	
	unsigned int nFocused = pViewModel->getFocused();
	MessageHolder* pmh = 0;
	if (nFocused < pViewModel->getCount())
		pmh = pViewModel->getMessageHolder(nFocused);
	
	MessagePtrLock mpl(getCurrentMessage());
	if (pmh != mpl)
		setMessage(pmh);
}

void qm::PreviewMessageModel::connectToViewModel()
{
	assert(!bConnectedToViewModel_);
	
	pViewModelManager_->addViewModelManagerHandler(this);
	setViewModel(pViewModelManager_->getCurrentViewModel());
	bConnectedToViewModel_ = true;
}

void qm::PreviewMessageModel::disconnectFromViewModel()
{
	assert(bConnectedToViewModel_);
	
	if (nTimerId_ != 0) {
		pTimer_->killTimer(nTimerId_);
		nTimerId_ = 0;
	}
	
	pViewModelManager_->removeViewModelManagerHandler(this);
	setViewModel(0);
	bConnectedToViewModel_ = false;
}

bool qm::PreviewMessageModel::isConnectedToViewModel() const
{
	return bConnectedToViewModel_;
}

void qm::PreviewMessageModel::itemStateChanged(const ViewModelEvent& event)
{
	ViewModel* pViewModel = getViewModel();
	assert(pViewModel == event.getViewModel());
	
	if (event.getItem() == pViewModel->getFocused()) {
		if (nTimerId_ != 0) {
			pTimer_->killTimer(nTimerId_);
			nTimerId_ = 0;
		}
		if (event.isDelay())
			nTimerId_ = pTimer_->setTimer(TIMER_ITEMSTATECHANGED, TIMEOUT, this);
		else
			updateToViewModel();
	}
}

void qm::PreviewMessageModel::viewModelSelected(const ViewModelManagerEvent& event)
{
	ViewModel* pViewModel = getViewModel();
	assert(pViewModel == event.getOldViewModel());
	
	setViewModel(event.getNewViewModel());
	setCurrentAccount(pViewModelManager_->getCurrentAccount());
	setMessage(0);
}

void qm::PreviewMessageModel::timerTimeout(unsigned int nId)
{
	assert(nId == nTimerId_);
	pTimer_->killTimer(nTimerId_);
	nTimerId_ = 0;
	updateToViewModel();
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

qm::MessageModelEvent::MessageModelEvent(const MessageModel* pModel,
										 MessageHolder* pmh) :
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
