/*
 * $Id$
 *
 * Copyright(C) 1998-2007 Satoshi Nakamura
 * All rights reserved.
 *
 */

#ifndef __UPDATECHECKER_H__
#define __UPDATECHECKER_H__

#include <qm.h>

#include <qsthread.h>

#include "../sync/syncmanager.h"


namespace qm {

class UpdateChecker;
class UpdateCheckThread;


/****************************************************************************
 *
 * UpdateChecker
 *
 */

class UpdateChecker : public SyncManagerHandler
{
public:
	enum Update {
		UPDATE_UPDATED,
		UPDATE_LATEST,
		UPDATE_ERROR
	};

public:
	UpdateChecker(SyncManager* pSyncManager,
				  qs::Profile* pProfile);
	~UpdateChecker();

public:
	Update checkUpdate();
	bool isUpdated() const;
	void setUpdated();
	void clearUpdated();
	bool isAutoCheck() const;
	void setAutoCheck(bool bAutoCheck);
	void save();

public:
	virtual void statusChanged(const SyncManagerEvent& event);

private:
	UpdateChecker(const UpdateChecker&);
	UpdateChecker& operator=(const UpdateChecker&);

private:
	SyncManager* pSyncManager_;
	qs::Profile* pProfile_;
	bool bAutoCheck_;
	volatile bool bUpdated_;
	
	qs::Time timeNextCheck_;
	std::auto_ptr<UpdateCheckThread> pThread_;
	qs::CriticalSection cs_;
};


/****************************************************************************
 *
 * UpdateCheckThread
 *
 */

class UpdateCheckThread : public qs::Thread
{
public:
	explicit UpdateCheckThread(UpdateChecker* pUpdateChecker);
	virtual ~UpdateCheckThread();

public:
	bool isRunning() const;
	void stop();

public:
	virtual void run();

private:
	UpdateCheckThread(const UpdateCheckThread&);
	UpdateCheckThread& operator=(const UpdateCheckThread&);

private:
	UpdateChecker* pUpdateChecker_;
	volatile bool bRunning_;
};

}

#endif // __UPDATECHECKER_H__
