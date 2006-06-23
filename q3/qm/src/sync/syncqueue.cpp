/*
 * $Id$
 *
 * Copyright(C) 1998-2006 Satoshi Nakamura
 * All rights reserved.
 *
 */

#include <qmdocument.h>

#include <boost/bind.hpp>
#include <boost/bind/make_adaptable.hpp>

#include "syncmanager.h"
#include "syncqueue.h"
#include "../ui/syncutil.h"
#include "../util/util.h"

using namespace qm;
using namespace qs;


/****************************************************************************
 *
 * SyncQueue
 *
 */

qm::SyncQueue::SyncQueue(SyncManager* pSyncManager,
						 Document* pDocument,
						 SyncDialogManager* pSyncDialogManager) :
	pSyncManager_(pSyncManager),
	pDocument_(pDocument),
	pSyncDialogManager_(pSyncDialogManager),
	pWindow_(0)
{
	std::auto_ptr<WindowBase> pWindow(new WindowBase(true));
	pWindow->setWindowHandler(new WindowHandler(this), true);
	if (!pWindow->create(L"QmSyncQueueWindow", 0,
		WS_POPUP, 0, 0, 0, 0, 0, 0, 0, 0, 0))
		return;
	pWindow_ = pWindow.release();
}

qm::SyncQueue::~SyncQueue()
{
	pWindow_->destroyWindow();
	clear();
}

bool qm::SyncQueue::pushFolder(NormalFolder* pFolder)
{
	wstring_ptr wstrFolder(Util::formatFolder(pFolder));
	
	{
		Lock<CriticalSection> lock(cs_);
		listFolderName_.push_back(wstrFolder.get());
		wstrFolder.release();
	}
	
	pWindow_->postMessage(WM_SYNCQUEUE_SYNC);
	
	return true;
}

bool qm::SyncQueue::pushFolders(const Account::NormalFolderList& listFolder)
{
	Account::NormalFolderList::const_iterator it = std::find_if(
		listFolder.begin(), listFolder.end(),
		std::not1(boost::make_adaptable<bool, NormalFolder*>(
			boost::bind(&SyncQueue::pushFolder, this, _1))));
	return it == listFolder.end();
}

void qm::SyncQueue::sync()
{
	Account::NormalFolderList l;
	{
		Lock<CriticalSection> lock(cs_);
		l.reserve(listFolderName_.size());
		for (FolderNameList::const_iterator it = listFolderName_.begin(); it != listFolderName_.end(); ++it) {
			Folder* pFolder = Util::getAccountOrFolder(pDocument_, *it).second;
			if (pFolder && pFolder->getType() == Folder::TYPE_NORMAL)
				l.push_back(static_cast<NormalFolder*>(pFolder));
		}
		clear();
	}
	if (l.empty())
		return;
	
	std::sort(l.begin(), l.end());
	l.erase(std::unique(l.begin(), l.end()), l.end());
	
	SyncUtil::syncFolders(pSyncManager_, pDocument_,
		pSyncDialogManager_, SyncData::TYPE_ACTIVE, l, 0);
}

void qm::SyncQueue::clear()
{
	std::for_each(listFolderName_.begin(), listFolderName_.end(), string_free<WSTRING>());
	listFolderName_.clear();
}


/****************************************************************************
 *
 * SyncQueue::WindowHandler
 *
 */

qm::SyncQueue::WindowHandler::WindowHandler(SyncQueue* pSyncQueue) :
	pSyncQueue_(pSyncQueue)
{
}

LRESULT qm::SyncQueue::WindowHandler::windowProc(UINT uMsg,
												 WPARAM wParam,
												 LPARAM lParam)
{
	BEGIN_MESSAGE_HANDLER()
		HANDLE_TIMER()
		HANDLE_MESSAGE(WM_SYNCQUEUE_SYNC, onSync)
	END_MESSAGE_HANDLER()
	return DefaultWindowHandler::windowProc(uMsg, wParam, lParam);
}

LRESULT qm::SyncQueue::WindowHandler::onTimer(UINT_PTR nId)
{
	if (nId == TIMER_ID) {
		getWindowBase()->killTimer(TIMER_ID);
		pSyncQueue_->sync();
	}
	return DefaultWindowHandler::onTimer(nId);
}

LRESULT qm::SyncQueue::WindowHandler::onSync(WPARAM wParam,
											 LPARAM lParam)
{
	getWindowBase()->setTimer(TIMER_ID, TIMER_DELAY);
	return 0;
}
