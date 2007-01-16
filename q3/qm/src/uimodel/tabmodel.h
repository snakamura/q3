/*
 * $Id$
 *
 * Copyright(C) 1998-2007 Satoshi Nakamura
 * All rights reserved.
 *
 */

#ifndef __TABMODEL_H__
#define __TABMODEL_H__

#ifdef QMTABWINDOW

#include <qm.h>

#include <qsprofile.h>
#include <qssax.h>

#include <vector>


namespace qm {

class TabItem;
class TabModel;
	class DefaultTabModel;
class TabModelHandler;
	class DefaultTabModelHandler;
class TabModelEvent;
class TabModelContentHandler;
class TabModelWriter;

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
			bool bLocked,
			const WCHAR* pwszTitle);
	TabItem(Folder* pFolder,
			bool bLocked,
			const WCHAR* pwszTitle);
	~TabItem();

public:
	std::pair<Account*, Folder*> get() const;
	bool isLocked() const;
	void setLocked(bool bLocked);
	const WCHAR* getTitle() const;
	void setTitle(const WCHAR* pwszTitle);

private:
	Account* pAccount_;
	Folder* pFolder_;
	bool bLocked_;
	qs::wstring_ptr wstrTitle_;
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
	virtual const TabItem* getItem(int nItem) = 0;
	virtual void setLocked(int nItem,
						   bool bLocked) = 0;
	virtual void setTitle(int nItem,
						  const WCHAR* pwszTitle) = 0;
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
	public DefaultAccountManagerHandler,
	public DefaultAccountHandler
{
public:
	enum {
		REUSE_NONE		= 0x00,
		REUSE_OPEN		= 0x01,
		REUSE_CHANGE	= 0x02
	};

public:
	typedef std::vector<TabItem*> ItemList;

public:
	DefaultTabModel(AccountManager* pAccountManager,
					qs::Profile* pProfile,
					const WCHAR* pwszPath);
	virtual ~DefaultTabModel();

public:
	unsigned int getReuse() const;
	void setReuse(unsigned int nReuse);
	bool save(bool bForce) const;
	const ItemList& getItems() const;

public:
	virtual int getCount();
	virtual int getCurrent();
	virtual void setCurrent(int nItem);
	virtual int getTemporary();
	virtual void setTemporary(int nItem);
	virtual const TabItem* getItem(int nItem);
	virtual void setLocked(int nItem,
						   bool bLocked);
	virtual void setTitle(int nItem,
						  const WCHAR* pwszTitle);
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
	virtual void accountListChanged(const AccountManagerEvent& event);
	virtual void accountManagerInitialized(const AccountManagerEvent& event);

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
				   bool bLocked,
				   const WCHAR* pwszTitle);
	int addFolder(Folder* pFolder,
				  bool bLocked,
				  const WCHAR* pwszTitle);
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
	typedef std::vector<std::pair<Account*, int> > AccountList;
	typedef std::vector<TabModelHandler*> HandlerList;

private:
	AccountManager* pAccountManager_;
	qs::Profile* pProfile_;
	qs::wstring_ptr wstrPath_;
	ItemList listItem_;
	int nCurrent_;
	int nTemporary_;
	ItemList listItemOrder_;
	unsigned int nReuse_;
	AccountList listHandledAccount_;
	HandlerList listHandler_;
	
	friend class TabModelContentHandler;
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
				  const TabItem* pOldItem,
				  const TabItem* pNewItem);
	TabModelEvent(TabModel* pTabModel,
				  int nItem,
				  int nAmount);
	~TabModelEvent();

public:
	TabModel* getTabModel() const;
	int getItem() const;
	const TabItem* getOldItem() const;
	const TabItem* getNewItem() const;
	int getAmount() const;

private:
	TabModelEvent(const TabModelEvent&);
	TabModelEvent& operator=(const TabModelEvent&);

private:
	TabModel* pTabModel_;
	int nItem_;
	const TabItem* pOldItem_;
	const TabItem* pNewItem_;
	int nAmount_;
};


/****************************************************************************
 *
 * TabModelContentHandler
 *
 */

class TabModelContentHandler : public qs::DefaultHandler
{
public:
	TabModelContentHandler(DefaultTabModel* pTabModel,
						   AccountManager* pAccountManager);
	virtual ~TabModelContentHandler();

public:
	virtual bool startElement(const WCHAR* pwszNamespaceURI,
							  const WCHAR* pwszLocalName,
							  const WCHAR* pwszQName,
							  const qs::Attributes& attributes);
	virtual bool endElement(const WCHAR* pwszNamespaceURI,
							const WCHAR* pwszLocalName,
							const WCHAR* pwszQName);
	virtual bool characters(const WCHAR* pwsz,
							size_t nStart,
							size_t nLength);

private:
	TabModelContentHandler(const TabModelContentHandler&);
	TabModelContentHandler& operator=(const TabModelContentHandler&);

private:
	enum State {
		STATE_ROOT,
		STATE_TABS,
		STATE_TAB
	};

private:
	DefaultTabModel* pTabModel_;
	AccountManager* pAccountManager_;
	State state_;
};


/****************************************************************************
 *
 * TabModelWriter
 *
 */

class TabModelWriter
{
public:
	TabModelWriter(qs::Writer* pWriter,
				   const WCHAR* pwszEncoding);
	~TabModelWriter();

public:
	bool write(const DefaultTabModel* pTabModel);
	bool write(const TabItem* pItem);

private:
	TabModelWriter(const TabModelWriter&);
	TabModelWriter& operator=(const TabModelWriter&);

private:
	qs::OutputHandler handler_;
};

}

#endif // QMTABWINDOW

#endif // __TABMODEL_H__
