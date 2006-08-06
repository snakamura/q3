/*
 * $Id$
 *
 * Copyright(C) 1998-2006 Satoshi Nakamura
 * All rights reserved.
 *
 */

#include <qmdocument.h>
#include <qmsyncfilter.h>

#include <boost/bind.hpp>

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
	bSyncing_(false),
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

void qm::SyncQueue::pushFolder(NormalFolder* pFolder)
{
	wstring_ptr wstrFolder(Util::formatFolder(pFolder));
	
	bool bSyncing = false;
	{
		Lock<CriticalSection> lock(cs_);
		listFolderName_.push_back(wstrFolder.get());
		wstrFolder.release();
		bSyncing = bSyncing_;
	}
	
	if (!bSyncing)
		pWindow_->postMessage(WM_SYNCQUEUE_SYNC);
}

void qm::SyncQueue::pushFolders(const Account::NormalFolderList& listFolder)
{
	std::for_each(listFolder.begin(), listFolder.end(),
		boost::bind(&SyncQueue::pushFolder, this, _1));
}

void qm::SyncQueue::sync()
{
	NormalFolder* pFolder = 0;
	{
		Lock<CriticalSection> lock(cs_);
		for (FolderNameList::const_iterator it = listFolderName_.begin(); it != listFolderName_.end(); ++it) {
			Folder* p = Util::getAccountOrFolder(pDocument_, *it).second;
			if (p && p->getType() == Folder::TYPE_NORMAL)
				pFolder = static_cast<NormalFolder*>(p);
		}
	}
	if (!pFolder)
		return;
	
	{
		Lock<CriticalSection> lock(cs_);
		if (bSyncing_)
			return;
		bSyncing_ = true;
	}
	
	std::auto_ptr<SyncData> pData(new DynamicSyncData(pDocument_, this));
	SyncUtil::syncData(pSyncManager_, pSyncDialogManager_,
		pFolder->getAccount()->getCurrentSubAccount(), pData);
}

void qm::SyncQueue::clear()
{
	std::for_each(listFolderName_.begin(), listFolderName_.end(), string_free<WSTRING>());
	listFolderName_.clear();
}

void qm::SyncQueue::getFolders(Account::NormalFolderList* pList)
{
	assert(pList);
	
	Lock<CriticalSection> lock(cs_);
	
	assert(bSyncing_);
	
	pList->reserve(listFolderName_.size());
	for (FolderNameList::const_iterator it = listFolderName_.begin(); it != listFolderName_.end(); ++it) {
		Folder* p = Util::getAccountOrFolder(pDocument_, *it).second;
		if (p && p->getType() == Folder::TYPE_NORMAL)
			pList->push_back(static_cast<NormalFolder*>(p));
	}
	
	std::sort(pList->begin(), pList->end());
	pList->erase(std::unique(pList->begin(), pList->end()), pList->end());
	
	clear();
	
	bSyncing_ = !pList->empty();
}


/****************************************************************************
 *
 * SyncQueue::DynamicSyncData
 *
 */

SyncQueue::DynamicSyncData::DynamicSyncData(Document* pDocument,
											SyncQueue* pSyncQueue) :
	SyncData(pDocument, TYPE_ACTIVE),
	pSyncQueue_(pSyncQueue)
{
}

SyncQueue::DynamicSyncData::~DynamicSyncData()
{
}

void SyncQueue::DynamicSyncData::getItems(ItemListList* pList)
{
	assert(pList);
	
	Account::NormalFolderList l;
	pSyncQueue_->getFolders(&l);
	if (l.empty())
		return;
	
	SyncFilterManager* pSyncFilterManager = pSyncQueue_->pSyncManager_->getSyncFilterManager();
	
	ItemList listItem;
	listItem.reserve(l.size());
	for (Account::NormalFolderList::const_iterator it = l.begin(); it != l.end(); ++it) {
		NormalFolder* pFolder = *it;
		Account* pAccount = pFolder->getAccount();
		SubAccount* pSubAccount = pAccount->getCurrentSubAccount();
		std::auto_ptr<SyncFilterSet> pSyncFilterSet(
			pSyncFilterManager->getFilterSet(pSubAccount->getSyncFilterName()));
		std::auto_ptr<ReceiveSyncItem> pItem(new ReceiveSyncItem(
			pAccount, pSubAccount, pFolder, pSyncFilterSet, 0));
		listItem.push_back(pItem.release());
	}
	pList->push_back(listItem);
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
		HANDLE_MESSAGE(WM_SYNCQUEUE_SYNC, onSync)
	END_MESSAGE_HANDLER()
	return DefaultWindowHandler::windowProc(uMsg, wParam, lParam);
}

LRESULT qm::SyncQueue::WindowHandler::onSync(WPARAM wParam,
											 LPARAM lParam)
{
	pSyncQueue_->sync();
	return 0;
}
