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

qm::FolderListModel::FolderListModel(QSTATUS* pstatus)
{
}

qm::FolderListModel::~FolderListModel()
{
}

QSTATUS qm::FolderListModel::getSelectedFolders(Account::FolderList* pList) const
{
	assert(pList->empty());
	
	DECLARE_QSTATUS();
	
	status = STLWrapper<Account::FolderList>(*pList).resize(listFolder_.size());
	CHECK_QSTATUS();
	std::copy(listFolder_.begin(), listFolder_.end(), pList->begin());
	
	return QSTATUS_SUCCESS;
}

bool qm::FolderListModel::hasSelectedFolder() const
{
	return !listFolder_.empty();
}

QSTATUS qm::FolderListModel::setSelectedFolder(const Account::FolderList& l)
{
	DECLARE_QSTATUS();
	
	status = STLWrapper<Account::FolderList>(listFolder_).resize(l.size());
	CHECK_QSTATUS();
	std::copy(l.begin(), l.end(), listFolder_.begin());
	
	return QSTATUS_SUCCESS;
}
