/*
 * $Id$
 *
 * Copyright(C) 1998-2004 Satoshi Nakamura
 * All rights reserved.
 *
 */

#ifndef __TABMODEL_H__
#define __TABMODEL_H__

#ifdef QMTABWINDOW

#include <qm.h>

#include <qsprofile.h>

#include <vector>


namespace qm {

class TabItem;
class TabModel;
	class DefaultTabModel;
class TabModelHandler;
	class DefaultTabModelHandler;
class TabModelEvent;

class Account;
class Folder;


/****************************************************************************
 *
 * TabItem
 *
 */

class TabItem
{
public:
	TabItem(Account* pAccount,
			bool bLocked);
	TabItem(Folder* pFolder,
			bool bLocked);
	~TabItem();

public:
	std::pair<Account*, Folder*> get() const;
	bool isLocked() const;
	void setLocked(bool bLocked);

private:
	Account* pAccount_;
	Folder* pFolder_;
	bool bLocked_;
};


/****************************************************************************
 *
 * TabModel
 *
 */

class TabModel
{
public:
	virtual ~TabModel();

public:
	virtual int getCount() = 0;
	virtual int getCurrent() = 0;
	virtual void setCurrent(int nItem) = 0;
	virtual int getTemporary() = 0;
	virtual void setTemporary(int nItem) = 0;
	virtual TabItem* getItem(int nItem) = 0;
	virtual bool isLocked(int nItem) = 0;
	virtual void setLocked(int nItem,
						   bool bLocked) = 0;
	virtual void open(Account* pAccount) = 0;
	virtual void open(Folder* pFolder) = 0;
	virtual void close(int nItem) = 0;
	virtual void moveItem(int nItem,
						  int nAmount) = 0;
	virtual void setAccount(Account* pAccount) = 0;
	virtual void setFolder(Folder* pFolder) = 0;
	virtual void addTabModelHandler(TabModelHandler* pHandler) = 0;
	virtual void removeTabModelHandler(TabModelHandler* pHandler) = 0;
};


/****************************************************************************
 *
 * DefaultTabModel
 *
 */

class DefaultTabModel :
	public TabModel,
	public DefaultDocumentHandler,
	public DefaultAccountHandler
{
public:
	DefaultTabModel(Document* pDocument,
					qs::Profile* pProfile);
	virtual ~DefaultTabModel();

public:
	bool save() const;

public:
	virtual int getCount();
	virtual int getCurrent();
	virtual void setCurrent(int nItem);
	virtual int getTemporary();
	virtual void setTemporary(int nItem);
	virtual TabItem* getItem(int nItem);
	virtual bool isLocked(int nItem);
	virtual void setLocked(int nItem,
						   bool bLocked);
	virtual void open(Account* pAccount);
	virtual void open(Folder* pFolder);
	virtual void close(int nItem);
	virtual void moveItem(int nItem,
						  int nAmount);
	virtual void setAccount(Account* pAccount);
	virtual void setFolder(Folder* pFolder);
	virtual void addTabModelHandler(TabModelHandler* pHandler);
	virtual void removeTabModelHandler(TabModelHandler* pHandler);

public:
	virtual void accountListChanged(const AccountListChangedEvent& event);
	virtual void documentInitialized(const DocumentEvent& event);

public:
	virtual void folderListChanged(const FolderListChangedEvent& event);

protected:
	void fireItemAdded(int nItem,
					   TabItem* pItem);
	void fireItemRemoved(int nItem,
						 TabItem* pItem);
	void fireItemChanged(int nItem,
						 TabItem* pOldItem,
						 TabItem* pNewItem);
	void fireItemMoved(int nItem,
					   int nAmount);
	void fireCurrentChanged();

private:
	void setCurrent(int nItem,
					bool bForce);
	void open(Account* pAccount,
			  bool bForce);
	void open(Folder* pFolder,
			  bool bForce);
	int addAccount(Account* pAccount,
				   bool bLocked);
	int addFolder(Folder* pFolder,
				  bool bLocked);
	void removeItem(int nItem);
	void setAccount(int nItem,
					Account* pAccount);
	void setFolder(int nItem,
				   Folder* pFolder);
	void setItem(int nItem,
				 std::auto_ptr<TabItem> pItem);
	int getItem(Account* pAccount) const;
	int getItem(Folder* pFolder) const;
	int getReusableItem() const;
	void resetHandlers(Account* pOldAccount,
					   Folder* pOldFolder,
					   Account* pNewAccount,
					   Folder* pNewFolder);

private:
	DefaultTabModel(const DefaultTabModel&);
	DefaultTabModel& operator=(const DefaultTabModel&);

private:
	enum {
		REUSE_NONE		= 0x00,
		REUSE_OPEN		= 0x01,
		REUSE_CHANGE	= 0x02
	};

private:
	typedef std::vector<TabItem*> ItemList;
	typedef std::vector<std::pair<Account*, int> > AccountList;
	typedef std::vector<TabModelHandler*> HandlerList;

private:
	Document* pDocument_;
	qs::Profile* pProfile_;
	ItemList listItem_;
	int nCurrent_;
	int nTemporary_;
	ItemList listItemOrder_;
	unsigned int nReuse_;
	AccountList listHandledAccount_;
	HandlerList listHandler_;
};


/****************************************************************************
 *
 * TabModelHandler
 *
 */

class TabModelHandler
{
public:
	virtual ~TabModelHandler();

public:
	virtual void itemAdded(const TabModelEvent& event) = 0;
	virtual void itemRemoved(const TabModelEvent& event) = 0;
	virtual void itemChanged(const TabModelEvent& event) = 0;
	virtual void itemMoved(const TabModelEvent& event) = 0;
	virtual void currentChanged(const TabModelEvent& event) = 0;
};


/****************************************************************************
 *
 * DefaultTabModelHandler
 *
 */

class DefaultTabModelHandler : public TabModelHandler
{
public:
	DefaultTabModelHandler();
	virtual ~DefaultTabModelHandler();

public:
	virtual void itemAdded(const TabModelEvent& event);
	virtual void itemRemoved(const TabModelEvent& event);
	virtual void itemChanged(const TabModelEvent& event);
	virtual void itemMoved(const TabModelEvent& event);
	virtual void currentChanged(const TabModelEvent& event);
};


/****************************************************************************
 *
 * TabModelEvent
 *
 */

class TabModelEvent
{
public:
	TabModelEvent(TabModel* pTabModel,
				  int nItem);
	TabModelEvent(TabModel* pTabModel,
				  int nItem,
				  TabItem* pOldItem,
				  TabItem* pNewItem);
	TabModelEvent(TabModel* pTabModel,
				  int nItem,
				  int nAmount);
	~TabModelEvent();

public:
	TabModel* getTabModel() const;
	int getItem() const;
	TabItem* getOldItem() const;
	TabItem* getNewItem() const;
	int getAmount() const;

private:
	TabModelEvent(const TabModelEvent&);
	TabModelEvent& operator=(const TabModelEvent&);

private:
	TabModel* pTabModel_;
	int nItem_;
	TabItem* pOldItem_;
	TabItem* pNewItem_;
	int nAmount_;
};

}

#endif // QMTABWINDOW

#endif // __TABMODEL_H__
