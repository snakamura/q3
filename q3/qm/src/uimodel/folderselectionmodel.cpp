/*
 * $Id$
 *
 * Copyright(C) 1998-2007 Satoshi Nakamura
 * All rights reserved.
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
