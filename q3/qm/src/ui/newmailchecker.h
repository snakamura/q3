/*
 * $Id$
 *
 * Copyright(C) 1998-2003 Satoshi Nakamura
 * All rights reserved.
 *
 */

#ifndef __NEWMAIL_H__
#define __NEWMAIL_H__

#include <qm.h>

#include <qsprofile.h>
#include <qstimer.h>


namespace qm {

class NewMailChecker;
class NewMailCheckerCallback;

class Document;
class GoRound;
class SyncDialogManager;
class SyncManager;


/****************************************************************************
 *
 * NewMailChecker
 *
 */

class NewMailChecker : public qs::TimerHandler
{
public:
	enum {
		TIMER_CHECK	= 1000
	};

public:
	NewMailChecker(qs::Profile* pProfile, Document* pDocument,
		GoRound* pGoRound, SyncManager* pSyncManager,
		SyncDialogManager* pSyncDialogManager, HWND hwnd,
		NewMailCheckerCallback* pCallback, qs::QSTATUS* pstatus);
	~NewMailChecker();

public:
	virtual qs::QSTATUS timerTimeout(unsigned int nId);

private:
	NewMailChecker(const NewMailChecker&);
	NewMailChecker& operator=(const NewMailChecker&);

private:
	Document* pDocument_;
	GoRound* pGoRound_;
	SyncManager* pSyncManager_;
	SyncDialogManager* pSyncDialogManager_;
	HWND hwnd_;
	NewMailCheckerCallback* pCallback_;
	qs::WSTRING wstrCourse_;
	bool bOnlyWhenConnected_;
	qs::Timer* pTimer_;
	unsigned int nId_;
};


/****************************************************************************
 *
 * NewMailCheckerCallback
 *
 */

class NewMailCheckerCallback
{
public:
	virtual ~NewMailCheckerCallback();

public:
	virtual bool canCheck() = 0;
};

}

#endif // __NEWMAIL_H__
