/*
 * $Id$
 *
 * Copyright(C) 1998-2003 Satoshi Nakamura
 * All rights reserved.
 *
 */

#ifndef __FOLDERSELECTIONMODEL_H__
#define __FOLDERSELECTIONMODEL_H__

#include <qm.h>
#include <qmaccount.h>

#include <qs.h>


namespace qm {

/****************************************************************************
 *
 * FolderSelectionModel
 *
 */

class FolderSelectionModel
{
public:
	virtual ~FolderSelectionModel();

public:
	virtual Account* getAccount() = 0;
	virtual qs::QSTATUS getSelectedFolders(Account::FolderList* pList) = 0;
	virtual qs::QSTATUS hasSelectedFolder(bool* pbHas) = 0;
	virtual Folder* getFocusedFolder() = 0;
};

}

#endif // __FOLDERSELECTIONMODEL_H__
