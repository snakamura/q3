/*
 * $Id$
 *
 * Copyright(C) 1998-2008 Satoshi Nakamura
 *
 */

#include "folderselectionmodel.h"

using namespace qm;


/****************************************************************************
 *
 * AccountSelectionModel
 *
 */

qm::AccountSelectionModel::~AccountSelectionModel()
{
}


/****************************************************************************
 *
 * FolderSelectionModel
 *
 */

qm::FolderSelectionModel::~FolderSelectionModel()
{
}

Account* qm::FolderSelectionModel::getAccount()
{
	std::pair<Account*, Folder*> p(getFocusedAccountOrFolder());
	return p.first ? p.first : p.second ? p.second->getAccount() : 0;
}
