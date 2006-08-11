/*
 * $Id$
 *
 * Copyright(C) 1998-2006 Satoshi Nakamura
 * All rights reserved.
 *
 */

#include <qmapplication.h>

#include <qshttp.h>
#include <qsinit.h>

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
	bAutoCheck_(false)
{
	wstring_ptr wstrLastCheck(pProfile_->getString(L"Global", L"LastUpdateCheck"));
	if (!timeLastCheck_.parse(wstrLastCheck.get())) {
		timeLastCheck_ = Time::getCurrentTime();
		timeLastCheck_.addDay(-1);
	}
	setAutoCheck(pProfile_->getInt(L"Global", L"AutoUpdateCheck") != 0);
}

qm::UpdateChecker::~UpdateChecker()
{
	setAutoCheck(false);
	
	if (pThread_.get())
		pThread_->stop();
}

bool qm::UpdateChecker::checkUpdate(HWND hwnd,
									bool bNotifyNoUpdate)
{
	malloc_size_ptr<unsigned char> p(HttpUtility::openURL(
		L"http://q3.snak.org/q3/snapshot"));
	if (!p.get() || p.size() > 32)
		return false;
	
	{
		Lock<CriticalSection> lock(cs_);
		timeLastCheck_ = Time::getCurrentTime();
	}
	
	const Application& app = Application::getApplication();
	HINSTANCE hInst = app.getResourceHandle();
	
	wstring_ptr wstrVersion(app.getVersion(L' ', false));
	const WCHAR* pwszVersion = wcschr(wstrVersion.get(), L' ') + 1;
	*wcsrchr(wstrVersion.get(), L'.') = L'\0';
	
	wstring_ptr wstrNewVersion(mbs2wcs(reinterpret_cast<char*>(p.get()), p.size()));
	if (wcscmp(wstrNewVersion.get(), pwszVersion) != 0) {
		if (messageBox(hInst, IDS_CONFIRM_UPDATE, MB_YESNO, hwnd) == IDYES)
			UIUtil::openURL(L"http://q3.snak.org/download/", hwnd);
	}
	else {
		if (bNotifyNoUpdate)
			messageBox(hInst, IDS_MESSAGE_UPDATED, hwnd);
	}
	
	return true;
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
	wstring_ptr wstrLastCheck(timeLastCheck_.format());
	pProfile_->setString(L"Global", L"LastUpdateCheck", wstrLastCheck.get());
	pProfile_->setInt(L"Global", L"AutoUpdateCheck", bAutoCheck_);
}

void qm::UpdateChecker::statusChanged(const SyncManagerEvent& event)
{
	if (pSyncManager_->isSyncing())
		return;
	
	if (pThread_.get()) {
		if (pThread_->isRunning())
			return;
		pThread_->join();
		pThread_.reset(0);
	}

	{
		Lock<CriticalSection> lock(cs_);
		if (Time(timeLastCheck_).addDay(1) > Time::getCurrentTime())
			return;
	}
	
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
	Lock<CriticalSection> lock(cs_);
	return bRunning_;
}

void qm::UpdateCheckThread::stop()
{
	if (isRunning()) {
		// TODO
	}
	join();
}

void qm::UpdateCheckThread::run()
{
	InitThread init(0);
	
	pUpdateChecker_->checkUpdate(::GetDesktopWindow(), false);
	
	// TODO
	
	Lock<CriticalSection> lock(cs_);
	bRunning_ = false;
}
