/*
 * $Id$
 *
 * Copyright(C) 1998-2004 Satoshi Nakamura
 * All rights reserved.
 *
 */

#include <qmdocument.h>
#include <qmgoround.h>

#include <qsras.h>

#include "newmailchecker.h"
#include "syncdialog.h"
#include "syncutil.h"
#include "../model/goround.h"
#include "../sync/syncmanager.h"

using namespace qm;
using namespace qs;


/****************************************************************************
 *
 * NewMailChecker
 *
 */

qm::NewMailChecker::NewMailChecker(Profile* pProfile,
								   Document* pDocument,
								   GoRound* pGoRound,
								   SyncManager* pSyncManager,
								   SyncDialogManager* pSyncDialogManager,
								   HWND hwnd,
								   NewMailCheckerCallback* pCallback) :
	pDocument_(pDocument),
	pGoRound_(pGoRound),
	pSyncManager_(pSyncManager),
	pSyncDialogManager_(pSyncDialogManager),
	hwnd_(hwnd),
	pCallback_(pCallback),
	bOnlyWhenConnected_(false),
	nId_(0)
{
	int nInterval = pProfile->getInt(L"NewMailCheck", L"Interval", 0);
	wstrCourse_ = pProfile->getString(L"NewMailCheck", L"Course", 0);
	bOnlyWhenConnected_ = pProfile->getInt(L"NewMailCheck", L"OnlyWhenConnected", 0) != 0;
	
	if (nInterval != 0) {
		pTimer_.reset(new Timer());
		nId_ = TIMER_CHECK;
		nId_ = pTimer_->setTimer(TIMER_CHECK, nInterval*60*1000, this);
	}
}

qm::NewMailChecker::~NewMailChecker()
{
	if (pTimer_.get()) {
		if (nId_ != 0)
			pTimer_->killTimer(nId_);
	}
}

void qm::NewMailChecker::timerTimeout(unsigned int nId)
{
	if (nId == nId_) {
		bool bCheck = pDocument_->isCheckNewMail() &&
			(!bOnlyWhenConnected_ || RasConnection::isNetworkConnected()) &&
			pCallback_->canCheck();
		if (bCheck) {
			const GoRoundCourseList* pCourseList = pGoRound_->getCourseList();
			const GoRoundCourse* pCourse = pCourseList->getCourse(wstrCourse_.get());
			
			SyncUtil::goRound(pSyncManager_, pDocument_, pSyncDialogManager_,
				hwnd_, SyncDialog::FLAG_NOTIFYNEWMESSAGE, pCourse);
		}
	}
}


/****************************************************************************
 *
 * NewMailCheckerCallback
 *
 */

qm::NewMailCheckerCallback::~NewMailCheckerCallback()
{
}
