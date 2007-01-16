/*
 * $Id$
 *
 * Copyright(C) 1998-2007 Satoshi Nakamura
 * All rights reserved.
 *
 */

#include <qmapplication.h>

#include <qshttp.h>
#include <qsinit.h>
#include <qsras.h>

#include "updatechecker.h"
#include "../ui/resourceinc.h"
#include "../ui/uiutil.h"

using namespace qm;
using namespace qs;


/****************************************************************************
 *
 * UpdateChecker
 *
 */

qm::UpdateChecker::UpdateChecker(SyncManager* pSyncManager,
								 Profile* pProfile) :
	pSyncManager_(pSyncManager),
	pProfile_(pProfile),
	bUpdated_(false),
	bAutoCheck_(false)
{
	wstring_ptr wstrNextCheck(pProfile_->getString(L"Global", L"NextUpdateCheck"));
	if (!timeNextCheck_.parse(wstrNextCheck.get()))
		timeNextCheck_ = Time::getCurrentTime();
	setAutoCheck(pProfile_->getInt(L"Global", L"AutoUpdateCheck") != 0);
}

qm::UpdateChecker::~UpdateChecker()
{
	setAutoCheck(false);
	
	if (pThread_.get())
		pThread_->stop();
}

UpdateChecker::Update qm::UpdateChecker::checkUpdate()
{
	if (!RasConnection::isNetworkConnected())
		return UPDATE_ERROR;
	
	malloc_size_ptr<unsigned char> p(HttpUtility::openURL(
		L"http://q3.snak.org/q3/version.cgi"));
	if (!p.get() || p.size() > 32)
		return UPDATE_ERROR;
	
	{
		Lock<CriticalSection> lock(cs_);
		timeNextCheck_ = Time::getCurrentTime().addDay(1);
	}
	
	wstring_ptr wstrVersion(Application::getApplication().getVersion(L' ', false));
	const WCHAR* pwszVersion = wcschr(wstrVersion.get(), L' ') + 1;
	*wcsrchr(wstrVersion.get(), L'.') = L'\0';
	
	wstring_ptr wstrNewVersion(mbs2wcs(reinterpret_cast<char*>(p.get()), p.size()));
	return wcscmp(wstrNewVersion.get(), pwszVersion) != 0 ?
		UPDATE_UPDATED : UPDATE_LATEST;
}

bool qm::UpdateChecker::isUpdated() const
{
	return bUpdated_;
}

void qm::UpdateChecker::setUpdated()
{
	bUpdated_ = true;
}

void qm::UpdateChecker::clearUpdated()
{
	bUpdated_ = false;
}

bool qm::UpdateChecker::isAutoCheck() const
{
	return bAutoCheck_;
}

void qm::UpdateChecker::setAutoCheck(bool bAutoCheck)
{
	if (bAutoCheck != bAutoCheck_) {
		if (bAutoCheck)
			pSyncManager_->addSyncManagerHandler(this);
		else
			pSyncManager_->removeSyncManagerHandler(this);
		
		bAutoCheck_ = bAutoCheck;
	}
}

void qm::UpdateChecker::save()
{
	Lock<CriticalSection> lock(cs_);
	
	pProfile_->setString(L"Global", L"NextUpdateCheck", timeNextCheck_.format().get());
	pProfile_->setInt(L"Global", L"AutoUpdateCheck", bAutoCheck_);
}

void qm::UpdateChecker::statusChanged(const SyncManagerEvent& event)
{
	if (isUpdated()/* || pSyncManager_->isSyncing()*/)
		return;
	
	Lock<CriticalSection> lock(cs_);
	
	if (pThread_.get()) {
		if (pThread_->isRunning())
			return;
		pThread_->join();
		pThread_.reset(0);
	}
	
	if (timeNextCheck_ > Time::getCurrentTime())
		return;
	
	std::auto_ptr<UpdateCheckThread> pThread(new UpdateCheckThread(this));
	if (!pThread->start())
		return;
	pThread_ = pThread;
}


/****************************************************************************
 *
 * UpdateCheckThread
 *
 */

qm::UpdateCheckThread::UpdateCheckThread(UpdateChecker* pUpdateChecker) :
	pUpdateChecker_(pUpdateChecker),
	bRunning_(true)
{
}

qm::UpdateCheckThread::~UpdateCheckThread()
{
}

bool qm::UpdateCheckThread::isRunning() const
{
	return bRunning_;
}

void qm::UpdateCheckThread::stop()
{
	join();
}

void qm::UpdateCheckThread::run()
{
	InitThread init(0);
	
	if (pUpdateChecker_->checkUpdate() == UpdateChecker::UPDATE_UPDATED)
		pUpdateChecker_->setUpdated();
	
	bRunning_ = false;
}
