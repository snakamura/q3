/*
 * $Id$
 *
 * Copyright(C) 1998-2005 Satoshi Nakamura
 * All rights reserved.
 *
 */

#include <qsstl.h>

#include "folderlistmodel.h"

using namespace qm;
using namespace qs;


/****************************************************************************
 *
 * FolderListModel
 *
 */

qm::FolderListModel::FolderListModel() :
	pAccount_(0),
	pFocusedFolder_(0)
{
}

qm::FolderListModel::~FolderListModel()
{
	if (pAccount_)
		pAccount_->removeAccountHandler(this);
}

Account* qm::FolderListModel::getAccount() const
{
	return pAccount_;
}

void qm::FolderListModel::setAccount(Account* pAccount)
{
	if (pAccount != pAccount_) {
		if (pAccount_)
			pAccount_->removeAccountHandler(this);
		pAccount_ = pAccount;
		if (pAccount_)
			pAccount_->addAccountHandler(this);
		fireAccountChanged();
	}
}

void qm::FolderListModel::getSelectedFolders(Account::FolderList* pList) const
{
	assert(pList->empty());
	*pList = listSelectedFolder_;
}

bool qm::FolderListModel::hasSelectedFolder() const
{
	return !listSelectedFolder_.empty();
}

void qm::FolderListModel::setSelectedFolders(const Account::FolderList& l)
{
	listSelectedFolder_ = l;
}

Folder* qm::FolderListModel::getFocusedFolder() const
{
	return pFocusedFolder_;
}

void qm::FolderListModel::setFocusedFolder(Folder* pFolder)
{
	pFocusedFolder_ = pFolder;
}

void qm::FolderListModel::addFolderListModelHandler(FolderListModelHandler* pHandler)
{
	listHandler_.push_back(pHandler);
}

void qm::FolderListModel::removeFolderListModelHandler(FolderListModelHandler* pHandler)
{
	HandlerList::iterator it = std::remove(
		listHandler_.begin(), listHandler_.end(), pHandler);
	listHandler_.erase(it, listHandler_.end());
}

void qm::FolderListModel::folderListChanged(const FolderListChangedEvent& event)
{
	switch (event.getType()) {
	case FolderListChangedEvent::TYPE_ALL:
	case FolderListChangedEvent::TYPE_ADD:
	case FolderListChangedEvent::TYPE_REMOVE:
	case FolderListChangedEvent::TYPE_RENAME:
		fireFolderListChanged();
		break;
	case FolderListChangedEvent::TYPE_FLAGS:
		if ((event.getOldFlags() & Folder::FLAG_BOX_MASK) !=
			(event.getNewFlags() & Folder::FLAG_BOX_MASK))
			fireFolderListChanged();
		break;
	default:
		assert(false);
		break;
	}
}

void qm::FolderListModel::fireAccountChanged()
{
	FolderListModelEvent event(this);
	for (HandlerList::const_iterator it = listHandler_.begin(); it != listHandler_.end(); ++it)
		(*it)->accountChanged(event);
}

void qm::FolderListModel::fireFolderListChanged()
{
	FolderListModelEvent event(this);
	for (HandlerList::const_iterator it = listHandler_.begin(); it != listHandler_.end(); ++it)
		(*it)->folderListChanged(event);
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
