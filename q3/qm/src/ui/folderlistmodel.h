/*
 * $Id$
 *
 * Copyright(C) 1998-2003 Satoshi Nakamura
 * All rights reserved.
 *
 */

#ifndef __FOLDERLISTMODEL_H__
#define __FOLDERLISTMODEL_H__

#include <qm.h>
#include <qmaccount.h>

#include <qs.h>


namespace qm {

/****************************************************************************
 *
 * FolderListModel
 *
 */

class FolderListModel
{
public:
	explicit FolderListModel(qs::QSTATUS* pstatus);
	~FolderListModel();

public:
	qs::QSTATUS getSelectedFolders(Account::FolderList* pList) const;
	bool hasSelectedFolder() const;
	qs::QSTATUS setSelectedFolder(const Account::FolderList& l);

private:
	FolderListModel(const FolderListModel&);
	FolderListModel& operator=(const FolderListModel&);

private:
	Account::FolderList listFolder_;
};

}

#endif // __FOLDERLISTMODEL_H__
