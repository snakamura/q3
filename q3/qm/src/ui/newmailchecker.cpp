/*
 * $Id$
 *
 * Copyright(C) 1998-2003 Satoshi Nakamura
 * All rights reserved.
 *
 */

#include <qmdocument.h>
#include <qmgoround.h>

#include <qsnew.h>

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

qm::NewMailChecker::NewMailChecker(Profile* pProfile, Document* pDocument,
	GoRound* pGoRound, SyncManager* pSyncManager, SyncDialogManager* pSyncDialogManager,
	HWND hwnd, NewMailCheckerCallback* pCallback, QSTATUS* pstatus) :
	pDocument_(pDocument),
	pGoRound_(pGoRound),
	pSyncManager_(pSyncManager),
	pSyncDialogManager_(pSyncDialogManager),
	hwnd_(hwnd),
	pCallback_(pCallback),
	wstrCourse_(0),
	pTimer_(0),
	nId_(0)
{
	DECLARE_QSTATUS();
	
	int nInterval = 0;
	status = pProfile->getInt(L"NewMailCheck", L"Interval", 0, &nInterval);
	CHECK_QSTATUS_SET(pstatus);
	status = pProfile->getString(L"NewMailCheck", L"Course", 0, &wstrCourse_);
	CHECK_QSTATUS_SET(pstatus);
	
	if (nInterval != 0) {
		status = newQsObject(&pTimer_);
		CHECK_QSTATUS_SET(pstatus);
		nId_ = TIMER_CHECK;
		status = pTimer_->setTimer(&nId_, nInterval*60*1000, this);
		CHECK_QSTATUS_SET(pstatus);
	}
}

qm::NewMailChecker::~NewMailChecker()
{
	if (pTimer_) {
		if (nId_ != 0)
			pTimer_->killTimer(nId_);
		delete pTimer_;
	}
	freeWString(wstrCourse_);
}

QSTATUS qm::NewMailChecker::timerTimeout(unsigned int nId)
{
	DECLARE_QSTATUS();
	
	if (nId == nId_ &&
		pDocument_->isCheckNewMail() &&
		pCallback_->canCheck()) {
		std::auto_ptr<SyncData> pData;
		status = newQsObject(pSyncManager_, pDocument_,
			hwnd_, SyncDialog::FLAG_NOTIFYNEWMESSAGE, &pData);
		
		const GoRoundCourse* pCourse = 0;
		GoRoundCourseList* pCourseList = 0;
		status = pGoRound_->getCourseList(&pCourseList);
		CHECK_QSTATUS();
		if (pCourseList && pCourseList->getCount() > 0)
			pCourse = pCourseList->getCourse(wstrCourse_);
		
		status = SyncUtil::createGoRoundData(pCourse, pDocument_, pData.get());
		CHECK_QSTATUS();
		
		if (!pData->isEmpty()) {
			SyncDialog* pSyncDialog = 0;
			status = pSyncDialogManager_->open(&pSyncDialog);
			CHECK_QSTATUS();
			pData->setCallback(pSyncDialog->getSyncManagerCallback());
			
			status = pSyncManager_->sync(pData.get());
			CHECK_QSTATUS();
			pData.release();
		}
	}
	
	return QSTATUS_SUCCESS;
}


/****************************************************************************
 *
 * NewMailCheckerCallback
 *
 */

qm::NewMailCheckerCallback::~NewMailCheckerCallback()
{
}
