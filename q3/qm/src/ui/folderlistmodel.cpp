/*
 * $Id$
 *
 * Copyright(C) 1998-2003 Satoshi Nakamura
 * All rights reserved.
 *
 */

#include <qsnew.h>
#include <qsstl.h>

#include "folderlistmodel.h"

using namespace qm;
using namespace qs;


/****************************************************************************
 *
 * FolderListModel
 *
 */

qm::FolderListModel::FolderListModel(FolderModel* pFolderModel, QSTATUS* pstatus) :
	pFolderModel_(pFolderModel),
	pDelayedFolderModelHandler_(0),
	pAccount_(0),
	pFocusedFolder_(0)
{
	DECLARE_QSTATUS();
	
	status = newQsObject(this, &pDelayedFolderModelHandler_);
	CHECK_QSTATUS_SET(pstatus);
	status = pFolderModel_->addFolderModelHandler(pDelayedFolderModelHandler_);
	CHECK_QSTATUS_SET(pstatus);
}

qm::FolderListModel::~FolderListModel()
{
	if (pAccount_)
		pAccount_->removeAccountHandler(this);
	
	pFolderModel_->removeFolderModelHandler(pDelayedFolderModelHandler_);
	delete pDelayedFolderModelHandler_;
}

Account* qm::FolderListModel::getAccount() const
{
	return pAccount_;
}

QSTATUS qm::FolderListModel::setAccount(Account* pAccount)
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
		
		status = fireAccountChanged();
		CHECK_QSTATUS();
	}
	
	return QSTATUS_SUCCESS;
}

QSTATUS qm::FolderListModel::getSelectedFolders(Account::FolderList* pList) const
{
	assert(pList->empty());
	
	DECLARE_QSTATUS();
	
	status = STLWrapper<Account::FolderList>(
		*pList).resize(listSelectedFolder_.size());
	CHECK_QSTATUS();
	std::copy(listSelectedFolder_.begin(),
		listSelectedFolder_.end(), pList->begin());
	
	return QSTATUS_SUCCESS;
}

bool qm::FolderListModel::hasSelectedFolder() const
{
	return !listSelectedFolder_.empty();
}

QSTATUS qm::FolderListModel::setSelectedFolders(const Account::FolderList& l)
{
	DECLARE_QSTATUS();
	
	status = STLWrapper<Account::FolderList>(
		listSelectedFolder_).resize(l.size());
	CHECK_QSTATUS();
	std::copy(l.begin(), l.end(), listSelectedFolder_.begin());
	
	return QSTATUS_SUCCESS;
}

Folder* qm::FolderListModel::getFocusedFolder() const
{
	return pFocusedFolder_;
}

void qm::FolderListModel::setFocusedFolder(Folder* pFolder)
{
	pFocusedFolder_ = pFolder;
}

QSTATUS qm::FolderListModel::addFolderListModelHandler(
	FolderListModelHandler* pHandler)
{
	return STLWrapper<HandlerList>(listHandler_).push_back(pHandler);
}

QSTATUS qm::FolderListModel::removeFolderListModelHandler(
	FolderListModelHandler* pHandler)
{
	HandlerList::iterator it = std::remove(listHandler_.begin(),
		listHandler_.end(), pHandler);
	listHandler_.erase(it, listHandler_.end());
	return QSTATUS_SUCCESS;
}

QSTATUS qm::FolderListModel::accountSelected(const FolderModelEvent& event)
{
	return setAccount(event.getAccount());
}

QSTATUS qm::FolderListModel::folderSelected(const FolderModelEvent& event)
{
	return setAccount(0);
}

QSTATUS qm::FolderListModel::folderListChanged(const FolderListChangedEvent& event)
{
	return fireFolderListChanged();
}

QSTATUS qm::FolderListModel::fireAccountChanged()
{
	DECLARE_QSTATUS();
	
	FolderListModelEvent event(this);
	
	HandlerList::const_iterator it = listHandler_.begin();
	while (it != listHandler_.end()) {
		status = (*it)->accountChanged(event);
		CHECK_QSTATUS();
		++it;
	}
	
	return QSTATUS_SUCCESS;
}

QSTATUS qm::FolderListModel::fireFolderListChanged()
{
	DECLARE_QSTATUS();
	
	FolderListModelEvent event(this);
	
	HandlerList::const_iterator it = listHandler_.begin();
	while (it != listHandler_.end()) {
		status = (*it)->folderListChanged(event);
		CHECK_QSTATUS();
		++it;
	}
	
	return QSTATUS_SUCCESS;
}


/****************************************************************************
 *
 * FolderListModelHandler
 *
 */

qm::FolderListModelHandler::~FolderListModelHandler()
{
}


/****************************************************************************
 *
 * FolderListModelEvent
 *
 */

qm::FolderListModelEvent::FolderListModelEvent(FolderListModel* pFolderListModel) :
	pFolderListModel_(pFolderListModel)
{
}

qm::FolderListModelEvent::~FolderListModelEvent()
{
}

FolderListModel* qm::FolderListModelEvent::getFolderListModel() const
{
	return pFolderListModel_;
}
