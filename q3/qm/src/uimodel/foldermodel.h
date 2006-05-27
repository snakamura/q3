/*
 * $Id$
 *
 * Copyright(C) 1998-2006 Satoshi Nakamura
 * All rights reserved.
 *
 */

#ifndef __FOLDERMODEL_H__
#define __FOLDERMODEL_H__

#include <qm.h>

#include <qs.h>
#include <qstimer.h>

#include <vector>


namespace qm {

class FolderModelBase;
	class FolderModel;
		class DefaultFolderModel;
class FolderModelHandler;
	class DelayedFolderModelHandler;
class FolderModelEvent;

class Account;
class Folder;


/****************************************************************************
 *
 * FolderModelBase
 *
 */

class FolderModelBase
{
public:
	virtual ~FolderModelBase();

public:
	virtual std::pair<Account*, Folder*> getCurrent() const = 0;
	virtual std::pair<Account*, Folder*> getTemporary() const = 0;
};


/****************************************************************************
 *
 * FolderModel
 *
 */

class FolderModel : public FolderModelBase
{
public:
	virtual ~FolderModel();

public:
	virtual void setCurrent(Account* pAccount,
							Folder* pFolder,
							bool bDelay) = 0;
	virtual void setTemporary(Account* pAccount,
							  Folder* pFolder) = 0;
	virtual void addFolderModelHandler(FolderModelHandler* pHandler) = 0;
	virtual void removeFolderModelHandler(FolderModelHandler* pHandler) = 0;
};


/****************************************************************************
 *
 * DefaultFolderModel
 *
 */

class DefaultFolderModel : public FolderModel
{
public:
	DefaultFolderModel();
	~DefaultFolderModel();

public:
	virtual std::pair<Account*, Folder*> getCurrent() const;
	virtual std::pair<Account*, Folder*> getTemporary() const;

public:
	virtual void setCurrent(Account* pAccount,
							Folder* pFolder,
							bool bDelay);
	virtual void setTemporary(Account* pAccount,
							  Folder* pFolder);
	virtual void addFolderModelHandler(FolderModelHandler* pHandler);
	virtual void removeFolderModelHandler(FolderModelHandler* pHandler);

private:
	void fireAccountSelected(Account* pAccount,
							 bool bDelay) const;
	void fireFolderSelected(Folder* pFolder,
							bool bDelay) const;

private:
	DefaultFolderModel(const DefaultFolderModel&);
	DefaultFolderModel& operator=(const DefaultFolderModel&);

private:
	typedef std::vector<FolderModelHandler*> HandlerList;

private:
	Account* pCurrentAccount_;
	Folder* pCurrentFolder_;
	Account* pTemporaryAccount_;
	Folder* pTemporaryFolder_;
	HandlerList listHandler_;
};


/****************************************************************************
 *
 * FolderModelHandler
 *
 */

class FolderModelHandler
{
public:
	virtual ~FolderModelHandler();

public:
	virtual void accountSelected(const FolderModelEvent& event) = 0;
	virtual void folderSelected(const FolderModelEvent& event) = 0;
};


/****************************************************************************
 *
 * DelayedFolderModelHandler
 *
 */

class DelayedFolderModelHandler :
	public FolderModelHandler,
	public qs::TimerHandler
{
public:
	DelayedFolderModelHandler(FolderModelHandler* pHandler);
	virtual ~DelayedFolderModelHandler();

public:
	virtual void accountSelected(const FolderModelEvent& event);
	virtual void folderSelected(const FolderModelEvent& event);

public:
	virtual void timerTimeout(qs::Timer::Id nId);

private:
	void set(Account* pAccount,
			 Folder* pFolder);

private:
	DelayedFolderModelHandler(const DelayedFolderModelHandler&);
	DelayedFolderModelHandler& operator=(const DelayedFolderModelHandler&);

private:
	enum {
		TIMERID	= 10,
		TIMEOUT	= 300
	};

private:
	FolderModelHandler* pHandler_;
	std::auto_ptr<qs::Timer> pTimer_;
	bool bTimer_;
	Account* pAccount_;
	Folder* pFolder_;
};


/****************************************************************************
 *
 * FolderModelEvent
 *
 */

class FolderModelEvent
{
public:
	FolderModelEvent(Account* pAccount,
					 bool bDelay);
	FolderModelEvent(Folder* pFolder,
					 bool bDelay);
	~FolderModelEvent();

public:
	Account* getAccount() const;
	Folder* getFolder() const;
	bool isDelay() const;

private:
	FolderModelEvent(const FolderModelEvent&);
	FolderModelEvent& operator=(const FolderModelEvent&);

private:
	Account* pAccount_;
	Folder* pFolder_;
	bool bDelay_;
};

}

#endif // __FOLDERMODEL_H__
