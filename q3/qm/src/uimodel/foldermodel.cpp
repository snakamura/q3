/*
 * $Id$
 *
 * Copyright(C) 1998-2007 Satoshi Nakamura
 * All rights reserved.
 *
 */

#include <qmfolder.h>

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

qm::DefaultFolderModel::DefaultFolderModel() :
	pCurrentAccount_(0),
	pCurrentFolder_(0),
	pTemporaryAccount_(0),
	pTemporaryFolder_(0)
{
}

qm::DefaultFolderModel::~DefaultFolderModel()
{
}

std::pair<Account*, Folder*> qm::DefaultFolderModel::getCurrent() const
{
	return std::make_pair(pCurrentAccount_, pCurrentFolder_);
}

std::pair<Account*, Folder*> qm::DefaultFolderModel::getTemporary() const
{
	return std::make_pair(pTemporaryAccount_, pTemporaryFolder_);
}

void qm::DefaultFolderModel::setCurrent(Account* pAccount,
										Folder* pFolder,
										bool bDelay)
{
	assert(!pAccount || !pFolder);
	assert(!pFolder || !pFolder->isHidden());
	
	if (pFolder) {
		if (pFolder != pCurrentFolder_) {
			pCurrentAccount_ = 0;
			pCurrentFolder_ = pFolder;
			fireFolderSelected(pFolder, bDelay);
		}
	}
	else {
		if (pAccount != pCurrentAccount_ || !pAccount) {
			pCurrentAccount_ = pAccount;
			pCurrentFolder_ = 0;
			fireAccountSelected(pAccount, bDelay);
		}
	}
}

void qm::DefaultFolderModel::setTemporary(Account* pAccount,
										  Folder* pFolder)
{
	pTemporaryAccount_ = pAccount;
	pTemporaryFolder_ = pFolder;
}

void qm::DefaultFolderModel::addFolderModelHandler(FolderModelHandler* pHandler)
{
	listHandler_.push_back(pHandler);
}

void qm::DefaultFolderModel::removeFolderModelHandler(FolderModelHandler* pHandler)
{
	HandlerList::iterator it = std::remove(listHandler_.begin(),
		listHandler_.end(), pHandler);
	listHandler_.erase(it, listHandler_.end());
}

void qm::DefaultFolderModel::fireAccountSelected(Account* pAccount,
												 bool bDelay) const
{
	FolderModelEvent event(pAccount, bDelay);
	for (HandlerList::const_iterator it = listHandler_.begin(); it != listHandler_.end(); ++it)
		(*it)->accountSelected(event);
}

void qm::DefaultFolderModel::fireFolderSelected(Folder* pFolder,
												bool bDelay) const
{
	assert(pFolder);
	
	FolderModelEvent event(pFolder, bDelay);
	for (HandlerList::const_iterator it = listHandler_.begin(); it != listHandler_.end(); ++it)
		(*it)->folderSelected(event);
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

qm::DelayedFolderModelHandler::DelayedFolderModelHandler(FolderModelHandler* pHandler) :
	pHandler_(pHandler),
	bTimer_(false),
	pAccount_(0),
	pFolder_(0)
{
	pTimer_.reset(new Timer());
}

qm::DelayedFolderModelHandler::~DelayedFolderModelHandler()
{
}

void qm::DelayedFolderModelHandler::accountSelected(const FolderModelEvent& event)
{
	if (event.isDelay())
		set(event.getAccount(), 0);
	else
		pHandler_->accountSelected(event);
}

void qm::DelayedFolderModelHandler::folderSelected(const FolderModelEvent& event)
{
	if (event.isDelay())
		set(0, event.getFolder());
	else
		pHandler_->folderSelected(event);
}

void qm::DelayedFolderModelHandler::timerTimeout(Timer::Id nId)
{
	assert((pAccount_ && !pFolder_) || (!pAccount_ && pFolder_));
	
	pTimer_->killTimer(TIMERID);
	bTimer_ = false;
	
	if (pAccount_)
		pHandler_->accountSelected(FolderModelEvent(pAccount_, false));
	else
		pHandler_->folderSelected(FolderModelEvent(pFolder_, false));
}

void qm::DelayedFolderModelHandler::set(Account* pAccount,
										Folder* pFolder)
{
	if (bTimer_)
		pTimer_->killTimer(TIMERID);
	
	pAccount_ = pAccount;
	pFolder_ = pFolder;
	
	bTimer_ = pTimer_->setTimer(TIMERID, TIMEOUT, this);
}


/****************************************************************************
 *
 * FolderModelEvent
 *
 */

qm::FolderModelEvent::FolderModelEvent(Account* pAccount,
									   bool bDelay) :
	pAccount_(pAccount),
	pFolder_(0),
	bDelay_(bDelay)
{
}

qm::FolderModelEvent::FolderModelEvent(Folder* pFolder,
									   bool bDelay) :
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
