/*
 * $Id$
 *
 * Copyright(C) 1998-2007 Satoshi Nakamura
 * All rights reserved.
 *
 */

#ifndef __FOLDERSELECTIONMODEL_H__
#define __FOLDERSELECTIONMODEL_H__

#include <qm.h>
#include <qmaccount.h>

#include <qs.h>


namespace qm {

class AccountSelectionModel;
class FolderSelectionModel;


/****************************************************************************
 *
 * AccountSelectionModel
 *
 */

class AccountSelectionModel
{
public:
	virtual ~AccountSelectionModel();

public:
	virtual Account* getAccount() = 0;
};


/****************************************************************************
 *
 * FolderSelectionModel
 *
 */

class FolderSelectionModel : public AccountSelectionModel
{
public:
	virtual ~FolderSelectionModel();

public:
	virtual Account* getAccount();

public:
	virtual std::pair<Account*, Folder*> getFocusedAccountOrFolder() = 0;
	virtual void getSelectedFolders(Account::FolderList* pList) = 0;
	virtual bool hasSelectedFolder() = 0;
};

}

#endif // __FOLDERSELECTIONMODEL_H__
