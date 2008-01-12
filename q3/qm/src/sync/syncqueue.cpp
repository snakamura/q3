/*
 * $Id$
 *
 * Copyright(C) 1998-2008 Satoshi Nakamura
 *
 */

#pragma warning(disable:4786)

#include <qmdocument.h>
#include <qmsyncfilter.h>

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

void qm::SyncQueue::pushFolder(NormalFolder* pFolder,
							   bool bCancelable)
{
	wstring_ptr wstrFolder(Util::formatFolder(pFolder));
	
	bool bSyncing = false;
	{
		Lock<CriticalSection> lock(cs_);
		
		if (bCancelable) {
			for (FolderList::iterator it = listFolder_.begin(); it != listFolder_.end(); ) {
				if ((*it).second) {
					freeWString((*it).first);
					it = listFolder_.erase(it);
				}
				else {
					++it;
				}
			}
		}
		
		listFolder_.push_back(std::make_pair(wstrFolder.get(), bCancelable));
		wstrFolder.release();
		
		bSyncing = isSyncing(pFolder->getAccount());
	}
	
	if (!bSyncing)
		pWindow_->postMessage(WM_SYNCQUEUE_SYNC);
}

void qm::SyncQueue::sync()
{
	Lock<CriticalSection> lock(cs_);
	
	AccountList listAccount;
	for (FolderList::iterator it = listFolder_.begin(); it != listFolder_.end(); ) {
		Folder* pFolder = Util::getAccountOrFolder(pDocument_, (*it).first).second;
		if (pFolder && pFolder->getType() == Folder::TYPE_NORMAL) {
			listAccount.push_back(pFolder->getAccount());
			++it;
		}
		else {
			freeWString((*it).first);
			it = listFolder_.erase(it);
		}
	}
	if (listAccount.empty())
		return;
	
	std::sort(listAccount.begin(), listAccount.end());
	listAccount.erase(std::unique(listAccount.begin(), listAccount.end()), listAccount.end());
	
	for (AccountList::const_iterator it = listAccount.begin(); it != listAccount.end(); ++it) {
		Account* pAccount = *it;
		if (!isSyncing(pAccount)) {
			std::auto_ptr<SyncData> pData(new DynamicSyncData(this, pDocument_, pAccount));
			SyncUtil::syncData(pSyncManager_, pSyncDialogManager_,
				pAccount->getCurrentSubAccount(), pData);
			startSyncing(pAccount);
		}
	}
}

void qm::SyncQueue::clear()
{
	std::for_each(listFolder_.begin(), listFolder_.end(),
		boost::bind(&freeWString, boost::bind(&FolderList::value_type::first, _1)));
	listFolder_.clear();
}

void qm::SyncQueue::getFolders(Account* pAccount,
							   Account::NormalFolderList* pList)
{
	assert(pList);
	
	Lock<CriticalSection> lock(cs_);
	
	assert(isSyncing(pAccount));
	
	pList->reserve(listFolder_.size());
	for (FolderList::iterator it = listFolder_.begin(); it != listFolder_.end(); ) {
		bool bRemove = true;
		Folder* pFolder = Util::getAccountOrFolder(pDocument_, (*it).first).second;
		if (pFolder && pFolder->getType() == Folder::TYPE_NORMAL) {
			if (pFolder->getAccount() == pAccount)
				pList->push_back(static_cast<NormalFolder*>(pFolder));
			else
				bRemove = false;
		}
		if (bRemove) {
			freeWString((*it).first);
			it = listFolder_.erase(it);
		}
		else {
			++it;
		}
	}
	
	std::sort(pList->begin(), pList->end());
	pList->erase(std::unique(pList->begin(), pList->end()), pList->end());
	
	if (pList->empty())
		endSyncing(pAccount);
}

bool qm::SyncQueue::isSyncing(Account* pAccount) const
{
	AccountList::const_iterator it = std::find(listSyncingAccount_.begin(),
		listSyncingAccount_.end(), pAccount);
	return it != listSyncingAccount_.end();
}

void qm::SyncQueue::startSyncing(Account* pAccount)
{
	assert(!isSyncing(pAccount));
	listSyncingAccount_.push_back(pAccount);
}

void qm::SyncQueue::endSyncing(Account* pAccount)
{
	assert(isSyncing(pAccount));
	listSyncingAccount_.erase(std::remove(listSyncingAccount_.begin(),
		listSyncingAccount_.end(), pAccount), listSyncingAccount_.end());
}


/****************************************************************************
 *
 * SyncQueue::DynamicSyncData
 *
 */

SyncQueue::DynamicSyncData::DynamicSyncData(SyncQueue* pSyncQueue,
											Document* pDocument,
											Account* pAccount) :
	SyncData(pDocument, TYPE_ACTIVE),
	pSyncQueue_(pSyncQueue),
	pAccount_(pAccount)
{
	assert(pSyncQueue);
	assert(pAccount);
}

SyncQueue::DynamicSyncData::~DynamicSyncData()
{
}

void SyncQueue::DynamicSyncData::getItems(ItemListList* pList)
{
	assert(pList);
	
	Account::NormalFolderList l;
	pSyncQueue_->getFolders(pAccount_, &l);
	if (l.empty())
		return;
	
	SyncFilterManager* pSyncFilterManager = pSyncQueue_->pSyncManager_->getSyncFilterManager();
	
	ItemList listItem;
	listItem.reserve(l.size());
	for (Account::NormalFolderList::const_iterator it = l.begin(); it != l.end(); ++it) {
		NormalFolder* pFolder = *it;
		assert(pFolder->getAccount() == pAccount_);
		SubAccount* pSubAccount = pAccount_->getCurrentSubAccount();
		std::auto_ptr<SyncFilterSet> pSyncFilterSet(
			pSyncFilterManager->getFilterSet(pSubAccount->getSyncFilterName()));
		std::auto_ptr<ReceiveSyncItem> pItem(new ReceiveSyncItem(
			pAccount_, pSubAccount, pFolder, pSyncFilterSet, 0));
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
