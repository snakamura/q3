/*
 * $Id$
 *
 * Copyright(C) 1998-2003 Satoshi Nakamura
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

qm::FolderListModel::FolderListModel(QSTATUS* pstatus) :
	pAccount_(0),
	pFocusedFolder_(0)
{
}

qm::FolderListModel::~FolderListModel()
{
}

Account* qm::FolderListModel::getAccount() const
{
	return pAccount_;
}

void qm::FolderListModel::setAccount(Account* pAccount)
{
	pAccount_ = pAccount;
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
