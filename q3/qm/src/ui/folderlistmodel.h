/*
 * $Id$
 *
 * Copyright(C) 1998-2004 Satoshi Nakamura
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
	FolderListModel();
	~FolderListModel();

public:
	Account* getAccount() const;
	void setAccount(Account* pAccount);
	void getSelectedFolders(Account::FolderList* pList) const;
	bool hasSelectedFolder() const;
	void setSelectedFolders(const Account::FolderList& l);
	Folder* getFocusedFolder() const;
	void setFocusedFolder(Folder* pFolder);
	
	void addFolderListModelHandler(FolderListModelHandler* pHandler);
	void removeFolderListModelHandler(FolderListModelHandler* pHandler);

public:
	virtual void folderListChanged(const FolderListChangedEvent& event);

private:
	void fireAccountChanged();
	void fireFolderListChanged();

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
	virtual void accountChanged(const FolderListModelEvent& event) = 0;
	virtual void folderListChanged(const FolderListModelEvent& event) = 0;
};


/****************************************************************************
 *
 * FolderListModelEvent
 *
 */

class FolderListModelEvent
{
public:
	explicit FolderListModelEvent(FolderListModel* pFolderListModel);
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
