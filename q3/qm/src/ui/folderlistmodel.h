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

#include "foldermodel.h"


namespace qm {

class FolderListModel;
class FolderListModelHandler;
class FolderListModelEvent;


/****************************************************************************
 *
 * FolderListModel
 *
 */

class FolderListModel : public DefaultAccountHandler
{
public:
	explicit FolderListModel(qs::QSTATUS* pstatus);
	~FolderListModel();

public:
	Account* getAccount() const;
	qs::QSTATUS setAccount(Account* pAccount);
	qs::QSTATUS getSelectedFolders(Account::FolderList* pList) const;
	bool hasSelectedFolder() const;
	qs::QSTATUS setSelectedFolders(const Account::FolderList& l);
	Folder* getFocusedFolder() const;
	void setFocusedFolder(Folder* pFolder);
	
	qs::QSTATUS addFolderListModelHandler(FolderListModelHandler* pHandler);
	qs::QSTATUS removeFolderListModelHandler(FolderListModelHandler* pHandler);

public:
	virtual qs::QSTATUS folderListChanged(const FolderListChangedEvent& event);

private:
	qs::QSTATUS fireAccountChanged();
	qs::QSTATUS fireFolderListChanged();

private:
	FolderListModel(const FolderListModel&);
	FolderListModel& operator=(const FolderListModel&);

private:
	typedef std::vector<FolderListModelHandler*> HandlerList;

private:
	Account* pAccount_;
	Account::FolderList listSelectedFolder_;
	Folder* pFocusedFolder_;
	HandlerList listHandler_;
};


/****************************************************************************
 *
 * FolderListModelHandler
 *
 */

class FolderListModelHandler
{
public:
	virtual ~FolderListModelHandler();

public:
	virtual qs::QSTATUS accountChanged(const FolderListModelEvent& event) = 0;
	virtual qs::QSTATUS folderListChanged(const FolderListModelEvent& event) = 0;
};


/****************************************************************************
 *
 * FolderListModelEvent
 *
 */

class FolderListModelEvent
{
public:
	FolderListModelEvent(FolderListModel* pFolderListModel);
	~FolderListModelEvent();

public:
	FolderListModel* getFolderListModel() const;

private:
	FolderListModelEvent(const FolderListModelEvent&);
	FolderListModelEvent& operator=(const FolderListModelEvent&);

private:
	FolderListModel* pFolderListModel_;
};

}

#endif // __FOLDERLISTMODEL_H__
