/*
 * $Id$
 *
 * Copyright(C) 1998-2003 Satoshi Nakamura
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
	virtual Account* getCurrentAccount() const = 0;
	virtual Folder* getCurrentFolder() const = 0;
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
//	virtual Account* getCurrentAccount() const = 0;
	virtual qs::QSTATUS setCurrentAccount(Account* pAccount, bool bDelay) = 0;
//	virtual Folder* getCurrentFolder() const = 0;
	virtual qs::QSTATUS setCurrentFolder(Folder* pFolder, bool bDelay) = 0;
	virtual qs::QSTATUS addFolderModelHandler(FolderModelHandler* pHandler) = 0;
	virtual qs::QSTATUS removeFolderModelHandler(FolderModelHandler* pHandler) = 0;
};


/****************************************************************************
 *
 * DefaultFolderModel
 *
 */

class DefaultFolderModel : public FolderModel
{
public:
	explicit DefaultFolderModel(qs::QSTATUS* pstatus);
	~DefaultFolderModel();

public:
	virtual Account* getCurrentAccount() const;
	virtual qs::QSTATUS setCurrentAccount(Account* pAccount, bool bDelay);
	virtual Folder* getCurrentFolder() const;
	virtual qs::QSTATUS setCurrentFolder(Folder* pFolder, bool bDelay);
	virtual qs::QSTATUS addFolderModelHandler(FolderModelHandler* pHandler);
	virtual qs::QSTATUS removeFolderModelHandler(FolderModelHandler* pHandler);

private:
	qs::QSTATUS fireAccountSelected(Account* pAccount, bool bDelay) const;
	qs::QSTATUS fireFolderSelected(Folder* pFolder, bool bDelay) const;

private:
	DefaultFolderModel(const DefaultFolderModel&);
	DefaultFolderModel& operator=(const DefaultFolderModel&);

private:
	typedef std::vector<FolderModelHandler*> HandlerList;

private:
	Account* pCurrentAccount_;
	Folder* pCurrentFolder_;
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
	virtual qs::QSTATUS accountSelected(const FolderModelEvent& event) = 0;
	virtual qs::QSTATUS folderSelected(const FolderModelEvent& event) = 0;
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
	DelayedFolderModelHandler(FolderModelHandler* pHandler,
		qs::QSTATUS* pstatus);
	virtual ~DelayedFolderModelHandler();

public:
	virtual qs::QSTATUS accountSelected(const FolderModelEvent& event);
	virtual qs::QSTATUS folderSelected(const FolderModelEvent& event);

public:
	virtual qs::QSTATUS timerTimeout(unsigned int nId);

private:
	qs::QSTATUS set(Account* pAccount, Folder* pFolder);

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
	qs::Timer* pTimer_;
	unsigned int nTimerId_;
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
	FolderModelEvent(Account* pAccount, bool bDelay);
	FolderModelEvent(Folder* pFolder, bool bDelay);
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
