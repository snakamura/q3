/*
 * $Id$
 *
 * Copyright(C) 1998-2003 Satoshi Nakamura
 * All rights reserved.
 *
 */

#include <qserror.h>
#include <qsnew.h>
#include <qsstl.h>

#include <algorithm>

#include "foldermodel.h"

using namespace qm;
using namespace qs;


/****************************************************************************
 *
 * FolderModelBase
 *
 */

qm::FolderModelBase::~FolderModelBase()
{
}


/****************************************************************************
 *
 * FolderModel
 *
 */

qm::FolderModel::~FolderModel()
{
}


/****************************************************************************
 *
 * DefaultFolderModel
 *
 */

qm::DefaultFolderModel::DefaultFolderModel(QSTATUS* pstatus) :
	pCurrentAccount_(0),
	pCurrentFolder_(0)
{
	assert(pstatus);
	*pstatus = QSTATUS_SUCCESS;
}

qm::DefaultFolderModel::~DefaultFolderModel()
{
}

Account* qm::DefaultFolderModel::getCurrentAccount() const
{
	return pCurrentAccount_;
}

QSTATUS qm::DefaultFolderModel::setCurrentAccount(Account* pAccount, bool bDelay)
{
	assert(pAccount);
	
	DECLARE_QSTATUS();
	
	if (pAccount != pCurrentAccount_) {
		pCurrentAccount_ = pAccount;
		pCurrentFolder_ = 0;
		
		status = fireAccountSelected(pAccount, bDelay);
		CHECK_QSTATUS();
	}
	
	return QSTATUS_SUCCESS;
}

Folder* qm::DefaultFolderModel::getCurrentFolder() const
{
	return pCurrentFolder_;
}

QSTATUS qm::DefaultFolderModel::setCurrentFolder(Folder* pFolder, bool bDelay)
{
	assert(pFolder);
	
	DECLARE_QSTATUS();
	
	if (pFolder != pCurrentFolder_) {
		pCurrentAccount_ = 0;
		pCurrentFolder_ = pFolder;
		
		status = fireFolderSelected(pFolder, bDelay);
		CHECK_QSTATUS();
	}
	
	return QSTATUS_SUCCESS;
}

QSTATUS qm::DefaultFolderModel::addFolderModelHandler(FolderModelHandler* pHandler)
{
	return STLWrapper<HandlerList>(listHandler_).push_back(pHandler);
}

QSTATUS qm::DefaultFolderModel::removeFolderModelHandler(FolderModelHandler* pHandler)
{
	HandlerList::iterator it = std::remove(listHandler_.begin(),
		listHandler_.end(), pHandler);
	listHandler_.erase(it, listHandler_.end());
	return QSTATUS_SUCCESS;
}

QSTATUS qm::DefaultFolderModel::fireAccountSelected(
	Account* pAccount, bool bDelay) const
{
	assert(pAccount);
	
	DECLARE_QSTATUS();
	
	FolderModelEvent event(pAccount, bDelay);
	
	HandlerList::const_iterator it = listHandler_.begin();
	while (it != listHandler_.end()) {
		status = (*it)->accountSelected(event);
		CHECK_QSTATUS();
		++it;
	}
	
	return QSTATUS_SUCCESS;
}

QSTATUS qm::DefaultFolderModel::fireFolderSelected(
	Folder* pFolder, bool bDelay) const
{
	assert(pFolder);
	
	DECLARE_QSTATUS();
	
	FolderModelEvent event(pFolder, bDelay);
	
	HandlerList::const_iterator it = listHandler_.begin();
	while (it != listHandler_.end()) {
		status = (*it)->folderSelected(event);
		CHECK_QSTATUS();
		++it;
	}
	
	return QSTATUS_SUCCESS;
}


/****************************************************************************
 *
 * FolderModelHandler
 *
 */

qm::FolderModelHandler::~FolderModelHandler()
{
}


/****************************************************************************
 *
 * DelayedFolderModelHandler
 *
 */

qm::DelayedFolderModelHandler::DelayedFolderModelHandler(
	FolderModelHandler* pHandler, QSTATUS* pstatus) :
	pHandler_(pHandler),
	pTimer_(0),
	nTimerId_(0),
	pAccount_(0),
	pFolder_(0)
{
	DECLARE_QSTATUS();
	
	status = newQsObject(&pTimer_);
	CHECK_QSTATUS_SET(pstatus);
}

qm::DelayedFolderModelHandler::~DelayedFolderModelHandler()
{
	delete pTimer_;
}

QSTATUS qm::DelayedFolderModelHandler::accountSelected(
	const FolderModelEvent& event)
{
	if (event.isDelay())
		return set(event.getAccount(), 0);
	else
		return pHandler_->accountSelected(event);
}

QSTATUS qm::DelayedFolderModelHandler::folderSelected(
	const FolderModelEvent& event)
{
	if (event.isDelay())
		return set(0, event.getFolder());
	else
		return pHandler_->folderSelected(event);
}

QSTATUS qm::DelayedFolderModelHandler::timerTimeout(unsigned int nId)
{
	assert(nId == nTimerId_);
	assert((pAccount_ && !pFolder_) || (!pAccount_ && pFolder_));
	
	DECLARE_QSTATUS();
	
	pTimer_->killTimer(nTimerId_);
	
	if (pAccount_) {
		status = pHandler_->accountSelected(
			FolderModelEvent(pAccount_, false));
		CHECK_QSTATUS();
	}
	else {
		status = pHandler_->folderSelected(
			FolderModelEvent(pFolder_, false));
		CHECK_QSTATUS();
	}
	
	return QSTATUS_SUCCESS;
}

QSTATUS qm::DelayedFolderModelHandler::set(Account* pAccount, Folder* pFolder)
{
	DECLARE_QSTATUS();
	
	pAccount_ = pAccount;
	pFolder_ = pFolder;
	
	nTimerId_ = TIMERID;
	status = pTimer_->setTimer(&nTimerId_, TIMEOUT, this);
	CHECK_QSTATUS();
	
	return QSTATUS_SUCCESS;
}


/****************************************************************************
 *
 * FolderModelEvent
 *
 */

qm::FolderModelEvent::FolderModelEvent(Account* pAccount, bool bDelay) :
	pAccount_(pAccount),
	pFolder_(0),
	bDelay_(bDelay)
{
}

qm::FolderModelEvent::FolderModelEvent(Folder* pFolder, bool bDelay) :
	pAccount_(0),
	pFolder_(pFolder),
	bDelay_(bDelay)
{
}

qm::FolderModelEvent::~FolderModelEvent()
{
}

Account* qm::FolderModelEvent::getAccount() const
{
	return pAccount_;
}

Folder* qm::FolderModelEvent::getFolder() const
{
	return pFolder_;
}

bool qm::FolderModelEvent::isDelay() const
{
	return bDelay_;
}
