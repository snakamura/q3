/*
 * $Id$
 *
 * Copyright(C) 1998-2005 Satoshi Nakamura
 * All rights reserved.
 *
 */

#pragma warning(disable:4786)

#include <qmmessage.h>
#include <qmmessageholder.h>
#include <qmaccount.h>

#include <qsstl.h>

#include <algorithm>

#include "messagemodel.h"

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
	MessageViewMode* pOldMode = 0;
	MessageViewMode* pNewMode = 0;
	
	if (pViewModel_) {
		pOldMode = pViewModel_->getMessageViewMode();
		pViewModel_->removeViewModelHandler(this);
	}
	pViewModel_ = pViewModel;
	if (pViewModel_) {
		pNewMode = pViewModel_->getMessageViewMode();
		pViewModel_->addViewModelHandler(this);
	}
	
	fireMessageViewModeChanged(pNewMode, pOldMode);
}

MessageViewMode* qm::AbstractMessageModel::getMessageViewMode()
{
	return pViewModel_ ? pViewModel_->getMessageViewMode() : 0;
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

void qm::AbstractMessageModel::fireUpdateRestoreInfo(ViewModel::RestoreInfo* pRestoreInfo) const
{
	MessageModelRestoreEvent event(this, pRestoreInfo);
	for (HandlerList::const_iterator it = listHandler_.begin(); it != listHandler_.end(); ++it)
		(*it)->updateRestoreInfo(event);
}

void qm::AbstractMessageModel::fireApplyRestoreInfo(ViewModel::RestoreInfo* pRestoreInfo) const
{
	MessageModelRestoreEvent event(this, pRestoreInfo);
	for (HandlerList::const_iterator it = listHandler_.begin(); it != listHandler_.end(); ++it)
		(*it)->applyRestoreInfo(event);
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
	updateToViewModel(false);
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

void qm::PreviewMessageModel::save() const
{
	ViewModel* pViewModel = getViewModel();
	if (pViewModel) {
		MessagePtrLock mpl(getCurrentMessage());
		ViewModel::RestoreInfo info(mpl);
		if (mpl)
			fireUpdateRestoreInfo(&info);
		pViewModel->setRestoreInfo(info);
	}
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

void qm::PreviewMessageModel::updated(const ViewModelEvent& event)
{
	updateToViewModel(true);
}

void qm::PreviewMessageModel::viewModelSelected(const ViewModelManagerEvent& event)
{
	ViewModel* pOldViewModel = getViewModel();
	assert(pOldViewModel == event.getOldViewModel());
	
	if (pOldViewModel) {
		MessagePtrLock mpl(getCurrentMessage());
		ViewModel::RestoreInfo info(mpl);
		if (mpl)
			fireUpdateRestoreInfo(&info);
		pOldViewModel->setRestoreInfo(info);
	}
	
	ViewModel* pNewViewModel = event.getNewViewModel();
	setViewModel(pNewViewModel);
	setCurrentAccount(pViewModelManager_->getCurrentAccount());
	
	if (pNewViewModel) {
		Lock<ViewModel> lock(*pNewViewModel);
		ViewModel::RestoreInfo info = pNewViewModel->getRestoreInfo();
		MessagePtrLock mpl(info.getMessagePtr());
		setMessage(mpl);
		if (mpl)
			fireApplyRestoreInfo(&info);
	}
	else {
		setMessage(0);
	}
}

void qm::PreviewMessageModel::timerTimeout(Timer::Id nId)
{
	assert(nId == nTimerId_);
	pTimer_->killTimer(nTimerId_);
	nTimerId_ = 0;
	updateToViewModel();
}

void qm::PreviewMessageModel::updateToViewModel(bool bClearMessage)
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
		setMessage(bClearMessage ? 0 : pmh);
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


/****************************************************************************
 *
 * MessageModelRestoreEvent
 *
 */

qm::MessageModelRestoreEvent::MessageModelRestoreEvent(const MessageModel* pModel,
													   ViewModel::RestoreInfo* pRestoreInfo) :
	pModel_(pModel),
	pRestoreInfo_(pRestoreInfo)
{
}

qm::MessageModelRestoreEvent::~MessageModelRestoreEvent()
{
}

const MessageModel* qm::MessageModelRestoreEvent::getMessageModel() const
{
	return pModel_;
}

ViewModel::RestoreInfo* qm::MessageModelRestoreEvent::getRestoreInfo() const
{
	return pRestoreInfo_;
}
