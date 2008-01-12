/*
 * $Id$
 *
 * Copyright(C) 1998-2007 Satoshi Nakamura
 * All rights reserved.
 *
 */

#pragma warning(disable:4786)

#include <qmaccount.h>
#include <qmmessage.h>
#include <qmmessageholder.h>
#include <qmsecurity.h>

#include <qsinit.h>
#include <qsstl.h>

#include <algorithm>

#include <boost/bind.hpp>

#include "../model/messagecontext.h"
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

qm::AbstractMessageModel::AbstractMessageModel(MessageViewMode* pDefaultMessageViewMode) :
	pDefaultMessageViewMode_(pDefaultMessageViewMode),
	pAccount_(0),
	pViewModel_(0),
	pSynchronizer_(InitThread::getInitThread().getSynchronizer())
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

Folder* qm::AbstractMessageModel::getCurrentFolder() const
{
	return pViewModel_ ? pViewModel_->getFolder() : 0;
}

MessageContext* qm::AbstractMessageModel::getCurrentMessage() const
{
	return pContext_.get();
}

void qm::AbstractMessageModel::setMessage(std::auto_ptr<MessageContext> pContext)
{
	if (pContext.get())
		setCurrentAccount(pContext->getAccount());
	pContext_ = pContext;
	fireMessageChanged(pContext_.get());
}

void qm::AbstractMessageModel::clearMessage()
{
	setMessage(std::auto_ptr<MessageContext>());
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
		pOldMode = getMessageViewMode(pViewModel_);
		pViewModel_->removeViewModelHandler(this);
	}
	pViewModel_ = pViewModel;
	if (pViewModel_) {
		pNewMode = getMessageViewMode(pViewModel_);
		pViewModel_->addViewModelHandler(this);
	}
	
	fireMessageViewModeChanged(pNewMode, pOldMode);
}

MessageViewMode* qm::AbstractMessageModel::getMessageViewMode()
{
	return pViewModel_ ? getMessageViewMode(pViewModel_) : pDefaultMessageViewMode_;
}

void qm::AbstractMessageModel::itemRemoved(const ViewModelEvent& event)
{
	struct RunnableImpl : public Runnable
	{
		RunnableImpl(AbstractMessageModel* pModel) :
			pModel_(pModel)
		{
		}
		
		virtual void run()
		{
			MessageContext* pContext = pModel_->pContext_.get();
			if (!pContext)
				return;
			MessagePtr ptr(pContext->getMessagePtr());
			if (!ptr)
				return;
			MessagePtrLock mpl(ptr);
			if (!mpl)
				pModel_->updateCurrentMessage();
		}
		
		AbstractMessageModel* pModel_;
	};
	asyncExec(new RunnableImpl(this));
}

void qm::AbstractMessageModel::destroyed(const ViewModelEvent& event)
{
	assert(Init::getInit().isPrimaryThread());
	assert(pViewModel_);
	
	setViewModel(0);
	clearMessage();
}

void qm::AbstractMessageModel::accountDestroyed(const AccountEvent& event)
{
	clearMessage();
	setCurrentAccount(0);
}

void qm::AbstractMessageModel::messageHolderKeysChanged(const MessageHolderEvent& event)
{
	if (pContext_.get()) {
		MessagePtrLock mpl(pContext_->getMessagePtr());
		if (mpl && mpl == event.getMessageHolder())
			setMessage(std::auto_ptr<MessageContext>(
				new MessagePtrMessageContext(mpl)));
	}
}

void qm::AbstractMessageModel::setCurrentAccount(Account* pAccount)
{
	if (pAccount != pAccount_) {
		if (pAccount_) {
			pAccount_->removeMessageHolderHandler(this);
			pAccount_->removeAccountHandler(this);
		}
		pAccount_ = pAccount;
		if (pAccount_) {
			pAccount_->addAccountHandler(this);
			pAccount_->addMessageHolderHandler(this);
		}
	}
}

void qm::AbstractMessageModel::asyncExec(Runnable* pRunnable)
{
	std::auto_ptr<Runnable> p(pRunnable);
	if (Init::getInit().isPrimaryThread())
		p->run();
	else
		pSynchronizer_->asyncExec(p);
}

void qm::AbstractMessageModel::fireMessageChanged(MessageContext* pContext) const
{
	MessageModelEvent event(this, pContext);
	std::for_each(listHandler_.begin(), listHandler_.end(),
		boost::bind(&MessageModelHandler::messageChanged, _1, boost::cref(event)));
}

void qm::AbstractMessageModel::fireUpdateRestoreInfo(ViewModel::RestoreInfo* pRestoreInfo) const
{
	MessageModelRestoreEvent event(this, pRestoreInfo);
	std::for_each(listHandler_.begin(), listHandler_.end(),
		boost::bind(&MessageModelHandler::updateRestoreInfo, _1, boost::cref(event)));
}

void qm::AbstractMessageModel::fireApplyRestoreInfo(ViewModel::RestoreInfo* pRestoreInfo) const
{
	MessageModelRestoreEvent event(this, pRestoreInfo);
	std::for_each(listHandler_.begin(), listHandler_.end(),
		boost::bind(&MessageModelHandler::applyRestoreInfo, _1, boost::cref(event)));
}


/****************************************************************************
 *
 * MessageMessageModel
 *
 */

qm::MessageMessageModel::MessageMessageModel(MessageViewMode* pDefaultMessageViewMode) :
	AbstractMessageModel(pDefaultMessageViewMode)
{
}

qm::MessageMessageModel::~MessageMessageModel()
{
}

void qm::MessageMessageModel::reloadProfiles()
{
}

void qm::MessageMessageModel::updateCurrentMessage()
{
	clearMessage();
}

MessageViewMode* qm::MessageMessageModel::getMessageViewMode(ViewModel* pViewModel) const
{
	return pViewModel->getMessageViewMode(ViewModel::MODETYPE_MESSAGE);
}


/****************************************************************************
 *
 * PreviewMessageModel
 *
 */

qm::PreviewMessageModel::PreviewMessageModel(ViewModelManager* pViewModelManager,
											 Profile* pProfile,
											 bool bConnectToViewModel,
											 MessageViewMode* pDefaultMessageViewMode) :
	AbstractMessageModel(pDefaultMessageViewMode),
	pViewModelManager_(pViewModelManager),
	pProfile_(pProfile),
	nDelay_(300),
	bUpdateAlways_(false),
	bTimer_(false),
	bConnectedToViewModel_(false)
{
	reloadProfiles();
	
	if (bConnectToViewModel)
		connectToViewModel();
	
	pTimer_.reset(new Timer());
}

qm::PreviewMessageModel::~PreviewMessageModel()
{
}

void qm::PreviewMessageModel::reloadProfiles()
{
	nDelay_ = pProfile_->getInt(L"PreviewWindow", L"Delay");
	bUpdateAlways_ = pProfile_->getInt(L"PreviewWindow", L"UpdateAlways") != 0;
}

void qm::PreviewMessageModel::updateToViewModel(bool bClearIfChanged)
{
	ViewModel* pViewModel = getViewModel();
	assert(pViewModel);
	
	Lock<ViewModel> lock(*pViewModel);
	
	unsigned int nFocused = pViewModel->getFocused();
	MessageHolder* pmh = 0;
	if (nFocused < pViewModel->getCount())
		pmh = pViewModel->getMessageHolder(nFocused);
	
	MessageContext* pContext = getCurrentMessage();
	MessagePtrLock mpl(pContext ? pContext->getMessagePtr() : MessagePtr());
	if (pmh != mpl || (!mpl && !pmh)) {
		if (bClearIfChanged || !pmh)
			clearMessage();
		else
			setMessage(std::auto_ptr<MessageContext>(
				new MessagePtrMessageContext(pmh)));
	}
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
	
	killTimer();
	
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
		MessageContext* pContext = getCurrentMessage();
		MessagePtrLock mpl(pContext ? pContext->getMessagePtr() : MessagePtr());
		ViewModel::RestoreInfo info(mpl);
		if (mpl)
			fireUpdateRestoreInfo(&info);
		pViewModel->setRestoreInfo(info);
	}
}

void qm::PreviewMessageModel::itemStateChanged(const ViewModelEvent& event)
{
	assert(Init::getInit().isPrimaryThread());
	
	if (!(event.getMask() & ViewModelItem::FLAG_FOCUSED))
		return;
	
	ViewModel* pViewModel = getViewModel();
	assert(pViewModel == event.getViewModel());
	
	if (event.getItem() == pViewModel->getFocused()) {
		killTimer();
		if (event.isDelay() && nDelay_ != 0)
			bTimer_ = pTimer_->setTimer(TIMER_ITEMSTATECHANGED, nDelay_, this);
		else
			updateToViewModel(false);
	}
}

void qm::PreviewMessageModel::updated(const ViewModelEvent& event)
{
	struct RunnableImpl : public Runnable
	{
		RunnableImpl(PreviewMessageModel* pModel) :
			pModel_(pModel)
		{
		}
		
		virtual void run()
		{
			pModel_->updateToViewModel(!pModel_->bUpdateAlways_);
		}
		
		PreviewMessageModel* pModel_;
	};
	asyncExec(new RunnableImpl(this));
}

void qm::PreviewMessageModel::viewModelSelected(const ViewModelManagerEvent& event)
{
	killTimer();
	
	ViewModel* pOldViewModel = getViewModel();
	assert(pOldViewModel == event.getOldViewModel());
	
	if (pOldViewModel) {
		MessageContext* pContext = getCurrentMessage();
		MessagePtrLock mpl(pContext ? pContext->getMessagePtr() : MessagePtr());
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
		if (mpl) {
			setMessage(std::auto_ptr<MessageContext>(
				new MessagePtrMessageContext(mpl)));
			fireApplyRestoreInfo(&info);
		}
		else {
			updateCurrentMessage();
		}
	}
	else {
		clearMessage();
	}
}

void qm::PreviewMessageModel::timerTimeout(Timer::Id nId)
{
	killTimer();
	updateToViewModel(false);
}

void qm::PreviewMessageModel::updateCurrentMessage()
{
	if (bUpdateAlways_)
		updateToViewModel(false);
	else
		clearMessage();
}

MessageViewMode* qm::PreviewMessageModel::getMessageViewMode(ViewModel* pViewModel) const
{
	return pViewModel->getMessageViewMode(ViewModel::MODETYPE_PREVIEW);
}

void qm::PreviewMessageModel::killTimer()
{
	if (!bTimer_)
		return;
	
	pTimer_->killTimer(TIMER_ITEMSTATECHANGED);
	bTimer_ = false;
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
										 MessageContext* pContext) :
	pModel_(pModel),
	pContext_(pContext)
{
}

qm::MessageModelEvent::~MessageModelEvent()
{
}

const MessageModel* qm::MessageModelEvent::getMessageModel() const
{
	return pModel_;
}

MessageContext* qm::MessageModelEvent::getMessageContext() const
{
	return pContext_;
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
