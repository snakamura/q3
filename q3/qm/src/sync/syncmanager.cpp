/*
 * $Id$
 *
 * Copyright(C) 1998-2006 Satoshi Nakamura
 * All rights reserved.
 *
 */

#pragma warning(disable:4786)

#include <qmaccount.h>
#include <qmapplication.h>
#include <qmdocument.h>
#include <qmfilenames.h>
#include <qmfolder.h>
#include <qmjunk.h>
#include <qmmessage.h>
#include <qmmessageholder.h>
#include <qmrecents.h>
#include <qmsecurity.h>
#include <qmsession.h>
#include <qmsyncfilter.h>

#include <qsinit.h>
#include <qsstl.h>

#include <algorithm>
#include <functional>

#include <boost/bind.hpp>

#include "syncmanager.h"
#include "../model/term.h"
#include "../model/uri.h"
#include "../ui/resourceinc.h"

using namespace qm;
using namespace qs;


/****************************************************************************
 *
 * SyncItem
 *
 */

qm::SyncItem::SyncItem(Account* pAccount,
					   SubAccount* pSubAccount) :
	pAccount_(pAccount),
	pSubAccount_(pSubAccount)
{
}

qm::SyncItem::~SyncItem()
{
}

Account* qm::SyncItem::getAccount() const
{
	return pAccount_;
}

SubAccount* qm::SyncItem::getSubAccount() const
{
	return pSubAccount_;
}


/****************************************************************************
 *
 * ReceiveSyncItem
 *
 */

qm::ReceiveSyncItem::ReceiveSyncItem(Account* pAccount,
									 SubAccount* pSubAccount,
									 NormalFolder* pFolder,
									 std::auto_ptr<SyncFilterSet> pFilterSet,
									 unsigned int nFlags) :
	SyncItem(pAccount, pSubAccount),
	pFolder_(pFolder),
	pFilterSet_(pFilterSet),
	nFlags_(nFlags)
{
}

qm::ReceiveSyncItem::~ReceiveSyncItem()
{
}

const SyncFilterSet* qm::ReceiveSyncItem::getFilterSet() const
{
	return pFilterSet_.get();
}

bool qm::ReceiveSyncItem::isFlag(Flag flag) const
{
	return (nFlags_ & flag) != 0;
}

bool qm::ReceiveSyncItem::isSend() const
{
	return false;
}

NormalFolder* qm::ReceiveSyncItem::getFolder() const
{
	return pFolder_;
}


/****************************************************************************
 *
 * SendSyncItem
 *
 */

qm::SendSyncItem::SendSyncItem(Account* pAccount,
							   SubAccount* pSubAccount,
							   ConnectReceiveBeforeSend crbs,
							   const WCHAR* pwszMessageId) :
	SyncItem(pAccount, pSubAccount),
	pOutbox_(0),
	bConnectReceiveBeforeSend_(false)
{
	NormalFolder* pOutbox = static_cast<NormalFolder*>(
		pAccount->getFolderByBoxFlag(Folder::FLAG_OUTBOX));
	if (pOutbox && pOutbox->isFlag(Folder::FLAG_SYNCABLE))
		pOutbox_ = pOutbox;
	
	switch (crbs) {
	case CRBS_NONE:
		bConnectReceiveBeforeSend_ = pSubAccount->isConnectReceiveBeforeSend();
		break;
	case CRBS_TRUE:
		bConnectReceiveBeforeSend_ = true;
		break;
	case CRBS_FALSE:
		bConnectReceiveBeforeSend_ = false;
		break;
	default:
		assert(false);
		break;
	}
	
	if (pwszMessageId)
		wstrMessageId_ = allocWString(pwszMessageId);
}

qm::SendSyncItem::~SendSyncItem()
{
}

bool qm::SendSyncItem::isConnectReceiveBeforeSend() const
{
	return bConnectReceiveBeforeSend_;
}

const WCHAR* qm::SendSyncItem::getMessageId() const
{
	return wstrMessageId_.get();
}

bool qm::SendSyncItem::isSend() const
{
	return true;
}

NormalFolder* qm::SendSyncItem::getFolder() const
{
	return pOutbox_;
}


/****************************************************************************
 *
 * SyncDialup
 *
 */

qm::SyncDialup::SyncDialup(const WCHAR* pwszEntry,
						   unsigned int nFlags,
						   const WCHAR* pwszDialFrom,
						   unsigned int nDisconnectWait) :
	wstrEntry_(0),
	nFlags_(nFlags),
	wstrDialFrom_(0),
	nDisconnectWait_(nDisconnectWait)
{
	wstring_ptr wstrEntry;
	if (pwszEntry)
		wstrEntry = allocWString(pwszEntry);
	
	wstring_ptr wstrDialFrom;
	if (pwszDialFrom)
		wstrDialFrom = allocWString(pwszDialFrom);
	
	wstrEntry_ = wstrEntry;
	wstrDialFrom_ = wstrDialFrom;
}

qm::SyncDialup::~SyncDialup()
{
}

const WCHAR* qm::SyncDialup::getEntry() const
{
	return wstrEntry_.get();
}

unsigned int qm::SyncDialup::getFlags() const
{
	return nFlags_;
}

const WCHAR* qm::SyncDialup::getDialFrom() const
{
	return wstrDialFrom_.get();
}

unsigned int qm::SyncDialup::getDisconnectWait() const
{
	return nDisconnectWait_;
}


/****************************************************************************
 *
 * SyncData
 *
 */

qm::SyncData::SyncData(Document* pDocument,
					   Type type) :
	pDocument_(pDocument),
	type_(type),
	pCallback_(0)
{
}

qm::SyncData::~SyncData()
{
}

Document* qm::SyncData::getDocument() const
{
	return pDocument_;
}

SyncData::Type qm::SyncData::getType() const
{
	return type_;
}

const SyncDialup* qm::SyncData::getDialup() const
{
	return pDialup_.get();
}

SyncManagerCallback* qm::SyncData::getCallback() const
{
	return pCallback_;
}

void qm::SyncData::setDialup(std::auto_ptr<SyncDialup> pDialup)
{
	pDialup_ = pDialup;
}

void qm::SyncData::setCallback(SyncManagerCallback* pCallback)
{
	pCallback_ = pCallback;
}


/****************************************************************************
 *
 * StaticSyncData
 *
 */

qm::StaticSyncData::StaticSyncData(Document* pDocument,
								   Type type,
								   SyncManager* pManager) :
	SyncData(pDocument, type),
	pManager_(pManager),
	nSlot_(0)
{
}

qm::StaticSyncData::~StaticSyncData()
{
	std::for_each(listItem_.begin(), listItem_.end(),
		unary_compose_f_gx(
			deleter<SyncItem>(),
			std::select2nd<SlotItemList::value_type>()));
}

void qm::StaticSyncData::getItems(ItemListList* pList)
{
	assert(pList);
	
	for (unsigned int nSlot = 0; nSlot <= nSlot_; ++nSlot) {
		ItemList listItem;
		for (SlotItemList::iterator it = listItem_.begin(); it != listItem_.end(); ++it) {
			if ((*it).first == nSlot)
				listItem.push_back((*it).second);
		}
		if (!listItem.empty())
			pList->push_back(listItem);
	}
	listItem_.clear();
	nSlot_ = 0;
}

bool qm::StaticSyncData::isEmpty() const
{
	return listItem_.empty();
}

void qm::StaticSyncData::newSlot()
{
	if (!listItem_.empty() && listItem_.back().first == nSlot_)
		++nSlot_;
}

void qm::StaticSyncData::addFolder(Account* pAccount,
								   SubAccount* pSubAccount,
								   NormalFolder* pFolder,
								   const WCHAR* pwszFilterName,
								   unsigned int nFlags)
{
	SyncFilterManager* pManager = pManager_->getSyncFilterManager();
	std::auto_ptr<SyncFilterSet> pFilterSet(pManager->getFilterSet(pwszFilterName));
	std::auto_ptr<ReceiveSyncItem> pItem(new ReceiveSyncItem(
		pAccount, pSubAccount, pFolder, pFilterSet, nFlags));
	listItem_.push_back(std::make_pair(nSlot_, pItem.get()));
	pItem.release();
}

void qm::StaticSyncData::addFolders(Account* pAccount,
									SubAccount* pSubAccount,
									const Term& folder,
									const WCHAR* pwszFilterName)
{
	Account::FolderList listFolder;
	
	NormalFolder* pTrash = static_cast<NormalFolder*>(
		pAccount->getFolderByBoxFlag(Folder::FLAG_TRASHBOX));
	if (pTrash && pTrash->isFlag(Folder::FLAG_SYNCABLE))
		pTrash = 0;
	
	const Account::FolderList& l = pAccount->getFolders();
	for (Account::FolderList::const_iterator it = l.begin(); it != l.end(); ++it) {
		Folder* pFolder = *it;
		if (pFolder->getType() == Folder::TYPE_NORMAL &&
			!pFolder->isFlag(Folder::FLAG_NOSELECT) &&
			!pFolder->isHidden() &&
			pFolder->isFlag(Folder::FLAG_SYNCABLE) &&
			(!pTrash || !pTrash->isAncestorOf(pFolder))) {
			bool bAdd = true;
			if (folder.isSpecified()) {
				wstring_ptr wstrFolderName(pFolder->getFullName());
				bAdd = folder.match(wstrFolderName.get());
			}
			if (bAdd)
				listFolder.push_back(pFolder);
		}
	}
	
	std::sort(listFolder.begin(), listFolder.end(), FolderLess());
	for (Account::FolderList::const_iterator it = listFolder.begin(); it != listFolder.end(); ++it)
		addFolder(pAccount, pSubAccount, static_cast<NormalFolder*>(*it), pwszFilterName, 0);
}

void qm::StaticSyncData::addSend(Account* pAccount,
								 SubAccount* pSubAccount,
								 SendSyncItem::ConnectReceiveBeforeSend crbs,
								 const WCHAR* pwszMessageId)
{
	std::auto_ptr<SendSyncItem> pItem(new SendSyncItem(
		pAccount, pSubAccount, crbs, pwszMessageId));
	listItem_.push_back(std::make_pair(nSlot_, pItem.get()));
	pItem.release();
}


/****************************************************************************
 *
 * SyncManager
 *
 */

qm::SyncManager::SyncManager(Profile* pProfile) :
	pProfile_(pProfile),
	pSynchronizer_(InitThread::getInitThread().getSynchronizer()),
	nDialupConnectionCount_(0)
{
	pSyncFilterManager_.reset(new SyncFilterManager(
		Application::getApplication().getProfilePath(FileNames::SYNCFILTERS_XML).get()));
}

qm::SyncManager::~SyncManager()
{
	if (pSyncFilterManager_.get())
		dispose();
}

void qm::SyncManager::dispose()
{
	typedef std::vector<HANDLE> Handles;
	Handles handles;
	{
		Lock<CriticalSection> lock(cs_);
		
		for (ThreadList::size_type n = 0; n < listThread_.size(); ++n) {
			SyncThread* pThread = listThread_[n];
			handles.push_back(pThread->getHandle());
			pThread->setWaitMode();
		}
	}
	
	if (!handles.empty()) {
		::WaitForMultipleObjects(static_cast<DWORD>(handles.size()), &handles[0], TRUE, INFINITE);
		std::for_each(listThread_.begin(), listThread_.end(), deleter<SyncThread>());
		listThread_.clear();
	}
	
	std::for_each(listSyncingFolder_.begin(), listSyncingFolder_.end(),
		unary_compose_f_gx(
			deleter<Event>(),
			std::select2nd<SyncingFolderList::value_type>()));
	
	pProfile_ = 0;
	pSyncFilterManager_.reset(0);
}

bool qm::SyncManager::sync(std::auto_ptr<SyncData> pData)
{
	Lock<CriticalSection> lock(cs_);
	
	std::auto_ptr<SyncThread> pThread(new SyncThread(this, pData));
	if (!pThread->start())
		return false;
	listThread_.push_back(pThread.get());
	pThread.release();
	
	return true;
}

bool qm::SyncManager::isSyncing() const
{
	Lock<CriticalSection> lock(cs_);
	return !listThread_.empty();
}

SyncFilterManager* qm::SyncManager::getSyncFilterManager() const
{
	return pSyncFilterManager_.get();
}

void qm::SyncManager::addSyncManagerHandler(SyncManagerHandler* pHandler)
{
	listHandler_.push_back(pHandler);
}

void qm::SyncManager::removeSyncManagerHandler(SyncManagerHandler* pHandler)
{
	SyncManagerHandlerList::iterator it = std::remove(
		listHandler_.begin(), listHandler_.end(), pHandler);
	assert(it != listHandler_.end());
	listHandler_.erase(it, listHandler_.end());
}

void qm::SyncManager::fireStatusChanged() const
{
	SyncManagerEvent event;
	for (SyncManagerHandlerList::const_iterator it = listHandler_.begin(); it != listHandler_.end(); ++it)
		(*it)->statusChanged(event);
}

void qm::SyncManager::addError(SyncManagerCallback* pCallback,
							   unsigned int nId,
							   Account* pAccount,
							   SubAccount* pSubAccount,
							   NormalFolder* pFolder,
							   UINT nMessageId,
							   const WCHAR* pwszDescription)
{
	HINSTANCE hInst = Application::getApplication().getResourceHandle();
	wstring_ptr wstrMessage(loadString(hInst, nMessageId));
	const WCHAR* pwszDescriptions[] = {
		pwszDescription
	};
	SessionErrorInfo info(pAccount, pSubAccount, pFolder,
		wstrMessage.get(), 0, pwszDescriptions, pwszDescription ? 1 : 0);
	pCallback->addError(nId, info);
}

bool qm::SyncManager::syncData(SyncData* pData)
{
	Log log(InitThread::getInitThread().getLogger(), L"qm::SyncManager::syncData");
	
	SyncManagerCallback* pCallback = pData->getCallback();
	assert(pCallback);
	
	struct CallbackCaller
	{
		CallbackCaller(SyncManagerCallback* pCallback,
					   const SyncData* pData) :
			pCallback_(pCallback)
		{
			pCallback_->start(pData->getType());
		}
		
		~CallbackCaller()
		{
			pCallback_->end();
		}
		
		SyncManagerCallback* pCallback_;
	} caller(pCallback, pData);
	
	const SyncDialup* pDialup = pData->getDialup();
	if (pDialup) {
		if (!(pDialup->getFlags() & SyncDialup::FLAG_WHENEVERNOTCONNECTED) ||
			!RasConnection::isNetworkConnected()) {
			const WCHAR* pwszDialFrom = pDialup->getDialFrom();
			if (pwszDialFrom)
				RasConnection::setLocation(pwszDialFrom);
			
			const WCHAR* pwszEntry = pDialup->getEntry();
			wstring_ptr wstrEntry;
			if (!pwszEntry) {
				wstrEntry = pCallback->selectDialupEntry();
				if (!wstrEntry.get())
					return true;
				pwszEntry = wstrEntry.get();
			}
			assert(pwszEntry);
			
			RasConnectionCallbackImpl rasCallback(pDialup, pCallback);
			std::auto_ptr<RasConnection> pRasConnection(
				new RasConnection(&rasCallback));
			RasConnection::Result result = pRasConnection->connect(pwszEntry);
			if (result == RasConnection::RAS_FAIL)
				return false;
			pCallback->setMessage(-1, L"");
			if (result == RasConnection::RAS_CANCEL)
				return true;
		}
		::InterlockedIncrement(UNVOLATILE(LONG*)(&nDialupConnectionCount_));
	}
	
	struct DialupDisconnector
	{
		DialupDisconnector(const SyncDialup* pDialup,
						   volatile LONG& nDialupConnectionCount) :
			pDialup_(pDialup),
			nDialupConnectionCount_(nDialupConnectionCount)
		{
		}
		
		~DialupDisconnector()
		{
			if (!pDialup_)
				return;
			else if (::InterlockedDecrement(UNVOLATILE(LONG*)(&nDialupConnectionCount_)) != 0)
				return;
			else if (pDialup_->getFlags() & SyncDialup::FLAG_NOTDISCONNECT)
				return;
			
			std::auto_ptr<RasConnection> pRasConnection(
				RasConnection::getActiveConnection(0));
			if (pRasConnection.get())
				pRasConnection->disconnect(pDialup_->getDisconnectWait());
		}
		
		const SyncDialup* pDialup_;
		volatile LONG& nDialupConnectionCount_;
	} disconnector(pDialup, nDialupConnectionCount_);
	
	struct InternalOnline
	{
		InternalOnline(Document* pDocument,
					   Synchronizer* pSynchronizer) :
			pDocument_(pDocument),
			pSynchronizer_(pSynchronizer)
		{
			RunnableImpl runnable(pDocument_, true);
			pSynchronizer_->syncExec(&runnable);
		}
		
		~InternalOnline()
		{
			RunnableImpl runnable(pDocument_, false);
			pSynchronizer_->syncExec(&runnable);
		}
		
		struct RunnableImpl : public Runnable
		{
			RunnableImpl(Document* pDocument,
						 bool bIncrement) :
				pDocument_(pDocument),
				bIncrement_(bIncrement)
			{
			}
			
			virtual void run()
			{
				if (bIncrement_)
					pDocument_->incrementInternalOnline();
				else
					pDocument_->decrementInternalOnline();
			}
			
			Document* pDocument_;
			bool bIncrement_;
		};
		
		Document* pDocument_;
		Synchronizer* pSynchronizer_;
	} internalOnline(pData->getDocument(), pSynchronizer_);
	
	while (true) {
		SyncData::ItemListList listItemList;
		struct Deleter
		{
			Deleter(SyncData::ItemListList& l) :
				l_(l)
			{
			}
			
			~Deleter()
			{
				for (SyncData::ItemListList::iterator it = l_.begin(); it != l_.end(); ++it) {
					SyncData::ItemList& l = *it;
					std::for_each(l.begin(), l.end(), qs::deleter<SyncItem>());
				}
			}
			
			SyncData::ItemListList& l_;
		} deleter(listItemList);
		
		pData->getItems(&listItemList);
		if (listItemList.empty())
			break;
		
		if (listItemList.size() == 1) {
			syncSlotData(pData, listItemList[0]);
		}
		else {
			typedef std::vector<Thread*> ThreadList;
			ThreadList listThread;
			listThread.reserve(listItemList.size());
			
			struct Wait
			{
				typedef std::vector<Thread*> ThreadList;
				
				Wait(const ThreadList& l) :
					l_(l)
				{
				}
				
				~Wait()
				{
					for (ThreadList::const_iterator it = l_.begin(); it != l_.end(); ++it) {
						std::auto_ptr<Thread> pThread(*it);
						pThread->join();
					}
				}
				
				const ThreadList& l_;
			} wait(listThread);
			
			for (SyncData::ItemListList::size_type n = 0; n < listItemList.size() - 1; ++n) {
				std::auto_ptr<ParallelSyncThread> pThread(new ParallelSyncThread(this, pData, listItemList[n]));
				if (!pThread->start())
					break;
				listThread.push_back(pThread.release());
				
			}
			syncSlotData(pData, listItemList.back());
		}
	}
	
	JunkFilter* pJunkFilter = pData->getDocument()->getJunkFilter();
	if (pJunkFilter) {
		if (!pJunkFilter->save(false))
			log.error(L"Failed to save junk filter.");
	}
	
	return true;
}

void qm::SyncManager::syncSlotData(const SyncData* pData,
								   const SyncData::ItemList& listItem)
{
	SyncManagerCallback* pCallback = pData->getCallback();
	assert(pCallback);
	
	struct CallbackCaller
	{
		CallbackCaller(SyncManagerCallback* pCallback,
					   unsigned int nId,
					   SyncData::Type type) :
			pCallback_(pCallback),
			nId_(nId)
		{
			pCallback_->startThread(nId_, type);
		}
		
		~CallbackCaller()
		{
			pCallback_->endThread(nId_);
		}
		
		SyncManagerCallback* pCallback_;
		unsigned int nId_;
	} caller(pCallback, ::GetCurrentThreadId(), pData->getType());
	
	struct ReceiveSessionTerm
	{
		ReceiveSessionTerm() :
			bConnected_(false)
		{
		}
		
		~ReceiveSessionTerm()
		{
			clear();
		}
		
		ReceiveSession* get() const
		{
			return pSession_.get();
		}
		
		void reset(std::auto_ptr<ReceiveSession> pSession)
		{
			if (pSession_.get()) {
				if (bConnected_)
					pSession_->disconnect();
				pSession_->term();
			}
			pSession_ = pSession;
			bConnected_ = false;
		}
		
		void clear()
		{
			std::auto_ptr<ReceiveSession> pSession;
			reset(pSession);
		}
		
		void setConnected()
		{
			bConnected_ = true;
		}
		
		std::auto_ptr<ReceiveSession> pSession_;
		bool bConnected_;
	};
	
	unsigned int nId = ::GetCurrentThreadId();
	std::auto_ptr<Logger> pLogger;
	std::auto_ptr<ReceiveSessionCallback> pReceiveCallback;
	ReceiveSessionTerm session;
	SubAccount* pSubAccount = 0;
	typedef std::vector<SubAccount*> SubAccountList;
	SubAccountList listConnectFailed;
	for (SyncData::ItemList::const_iterator it = listItem.begin(); it != listItem.end(); ++it) {
		const SyncItem* pItem = *it;
		
		bool bSync = true;
		if (pItem->isSend())
			bSync = pItem->getFolder() != 0;
		
		if (bSync || (pItem->isSend() && static_cast<const SendSyncItem*>(pItem)->isConnectReceiveBeforeSend())) {
			if (pSubAccount != pItem->getSubAccount() ||
				(session.get() && !session.get()->isConnected())) {
				pSubAccount = 0;
				session.clear();
				pReceiveCallback.reset(0);
				pLogger.reset(0);
				
				if (std::find(listConnectFailed.begin(), listConnectFailed.end(), pItem->getSubAccount()) != listConnectFailed.end())
					continue;
				
				std::auto_ptr<ReceiveSession> pReceiveSession;
				if (!openReceiveSession(pData->getDocument(),
					pCallback, pItem, pData->getType(),
					&pReceiveSession, &pReceiveCallback, &pLogger))
					continue;
				session.reset(pReceiveSession);
				if (pCallback->isCanceled(nId, false))
					break;
				
				if (!session.get()->connect()) {
					listConnectFailed.push_back(pItem->getSubAccount());
					continue;
				}
				session.setConnected();
				if (pCallback->isCanceled(nId, false))
					break;
				
				if (!session.get()->applyOfflineJobs())
					continue;
				if (pCallback->isCanceled(nId, false))
					break;
				
				pSubAccount = pItem->getSubAccount();
			}
			if (bSync) {
				if (!syncFolder(pData->getDocument(), pCallback, pItem, session.get()))
					continue;
				if (pCallback->isCanceled(nId, false))
					break;
			}
		}
		
		if (pItem->isSend()) {
			if (!send(pData->getDocument(), pCallback, static_cast<const SendSyncItem*>(pItem)))
				continue;
			if (pCallback->isCanceled(nId, false))
				break;
		}
	}
}

bool qm::SyncManager::syncFolder(Document* pDocument,
								 SyncManagerCallback* pSyncManagerCallback,
								 const SyncItem* pItem,
								 ReceiveSession* pSession)
{
	assert(pSession);
	
	unsigned int nId = ::GetCurrentThreadId();
	
	const ReceiveSyncItem* pReceiveItem = pItem->isSend() ? 0 :
		static_cast<const ReceiveSyncItem*>(pItem);
	
	NormalFolder* pFolder = pItem->getFolder();
	if (!pFolder || !pFolder->isFlag(Folder::FLAG_SYNCABLE))
		return true;
	
	pSyncManagerCallback->setFolder(nId, pFolder);
	
	FolderWait wait(this, pFolder);
	
	if (!pFolder->loadMessageHolders())
		return false;
	
	unsigned int nSelectFlags = 0;
	if (pReceiveItem) {
		if (pReceiveItem->isFlag(ReceiveSyncItem::FLAG_EMPTY))
			nSelectFlags |= ReceiveSession::SELECTFLAG_EMPTY;
		if (pReceiveItem->isFlag(ReceiveSyncItem::FLAG_EXPUNGE))
			nSelectFlags |= ReceiveSession::SELECTFLAG_EXPUNGE;
	}
	if (!pSession->selectFolder(pFolder, nSelectFlags))
		return false;
	pFolder->setLastSyncTime(::GetTickCount());
	if (pSyncManagerCallback->isCanceled(nId, false))
		return true;
	
	if (!pSession->updateMessages())
		return false;
	if (pSyncManagerCallback->isCanceled(nId, false))
		return true;
	
	SubAccount* pSubAccount = pItem->getSubAccount();
	bool bApplyRules = (pSubAccount->getAutoApplyRules() & SubAccount::AUTOAPPLYRULES_EXISTING) != 0;
	unsigned int nLastId = -1;
	if (bApplyRules) {
		Lock<Account> lock(*pFolder->getAccount());
		if (!pFolder->loadMessageHolders())
			return false;
		unsigned int nCount = pFolder->getCount();
		if (nCount != 0)
			nLastId = pFolder->getMessage(nCount - 1)->getId();
	}
	
	const SyncFilterSet* pFilterSet = pReceiveItem ?
		pReceiveItem->getFilterSet() : 0;
	if (!pSession->downloadMessages(pFilterSet))
		return false;
	if (pSyncManagerCallback->isCanceled(nId, false))
		return true;
	
	if (bApplyRules && nLastId != -1) {
		MessagePtrList l;
		{
			Lock<Account> lock(*pFolder->getAccount());
			unsigned int nCount = pFolder->getCount();
			for (unsigned int n = 0; n < nCount; ++n) {
				MessageHolder* pmh = pFolder->getMessage(n);
				if (pmh->getId() > nLastId)
					break;
				l.push_back(MessagePtr(pmh));
			}
		}
		RuleManager* pRuleManager = pDocument->getRuleManager();
		RuleCallbackImpl callback(pSyncManagerCallback, nId);
		return pRuleManager->applyAuto(pFolder, &l, pDocument,
			pProfile_, RuleManager::AUTOFLAG_EXISTING, &callback);
	}
	
	Account* pAccount = pItem->getAccount();
	if (!pAccount->saveMessages(false)) {
		addError(pSyncManagerCallback, nId, pAccount, 0, 0, IDS_ERROR_SAVE, 0);
		return false;
	}
	
	if (!pSession->closeFolder())
		return false;
	
	return true;
}

bool qm::SyncManager::send(Document* pDocument,
						   SyncManagerCallback* pSyncManagerCallback,
						   const SendSyncItem* pItem)
{
	unsigned int nId = ::GetCurrentThreadId();
	Account* pAccount = pItem->getAccount();
	SubAccount* pSubAccount = pItem->getSubAccount();
	pSyncManagerCallback->setAccount(nId, pAccount, pSubAccount);
	
	NormalFolder* pOutbox = static_cast<NormalFolder*>(
		pAccount->getFolderByBoxFlag(Folder::FLAG_OUTBOX));
	if (!pOutbox) {
		addError(pSyncManagerCallback, nId, pAccount, 0, 0, IDS_ERROR_NOOUTBOX, 0);
		return false;
	}
	if (pOutbox->getFlags() & Folder::FLAG_BOX_MASK & ~Folder::FLAG_OUTBOX & ~Folder::FLAG_DRAFTBOX) {
		addError(pSyncManagerCallback, nId, pAccount, 0, 0, IDS_ERROR_OUTBOXHASEXTRAFLAGS, 0);
		return false;
	}
	if (!pOutbox->loadMessageHolders()) {
		addError(pSyncManagerCallback, nId, pAccount, 0, 0, IDS_ERROR_LOADOUTBOX, 0);
		return false;
	}
	
	NormalFolder* pSentbox = static_cast<NormalFolder*>(
		pAccount->getFolderByBoxFlag(Folder::FLAG_SENTBOX));
	if (!pSentbox) {
		addError(pSyncManagerCallback, nId, pAccount, 0, 0, IDS_ERROR_NOSENTBOX, 0);
		return false;
	}
	
	const WCHAR* pwszIdentity = pSubAccount->getIdentity();
	assert(pwszIdentity);
	
	FolderWait wait(this, pOutbox);
	
	MessagePtrList listMessagePtr;
	listMessagePtr.reserve(pOutbox->getCount());
	
	{
		Lock<Account> lock(*pOutbox->getAccount());
		
		const WCHAR* pwszMessageId = pItem->getMessageId();
		for (unsigned int n = 0; n < pOutbox->getCount(); ++n) {
			MessageHolder* pmh = pOutbox->getMessage(n);
			if (!pmh->isFlag(MessageHolder::FLAG_DRAFT) &&
				!pmh->isFlag(MessageHolder::FLAG_DELETED)) {
				bool bSend = true;
				
				if (pwszMessageId) {
					wstring_ptr wstrMessageId = pmh->getMessageId();
					bSend = wstrMessageId.get() && wcscmp(pwszMessageId, wstrMessageId.get()) == 0;
				}
				
				if (bSend) {
					Message msg;
					if (!pmh->getMessage(Account::GETMESSAGEFLAG_HEADER,
						L"X-QMAIL-SubAccount", SECURITYMODE_NONE, &msg)) {
						addError(pSyncManagerCallback, nId, pAccount, 0, 0, IDS_ERROR_GETMESSAGE, 0);
						return false;
					}
					
					if (*pwszIdentity) {
						UnstructuredParser subaccount;
						if (msg.getField(L"X-QMAIL-SubAccount", &subaccount) == Part::FIELD_EXIST) {
							SubAccount* p = pAccount->getSubAccount(subaccount.getValue());
							bSend = p && wcscmp(p->getIdentity(), pwszIdentity) == 0;
						}
						else {
							bSend = false;
						}
					}
				}
				
				if (bSend)
					listMessagePtr.push_back(MessagePtr(pmh));
				
				if (pSyncManagerCallback->isCanceled(nId, false))
					return true;
			}
		}
	}
	if (listMessagePtr.empty())
		return true;
	
	std::auto_ptr<Logger> pLogger;
	if (pSubAccount->isLog(Account::HOST_SEND))
		pLogger = pAccount->openLogger(Account::HOST_SEND);
	
	std::auto_ptr<SendSession> pSession(SendSessionFactory::getSession(
		pItem->getAccount()->getType(Account::HOST_SEND)));
	
	std::auto_ptr<SendSessionCallbackImpl> pCallback(
		new SendSessionCallbackImpl(pSyncManagerCallback));
	if (!pSession->init(pDocument, pAccount, pSubAccount,
		pProfile_, pLogger.get(), pCallback.get()))
		return false;
	
	struct SendSessionTerm
	{
		SendSessionTerm(SendSession* pSession) :
			pSession_(pSession),
			bConnected_(false)
		{
		}
		
		~SendSessionTerm()
		{
			if (bConnected_)
				pSession_->disconnect();
			pSession_->term();
		}
		
		void setConnected()
		{
			bConnected_ = true;
		}
		
		SendSession* pSession_;
		bool bConnected_;
	} term(pSession.get());
	
	if (pSyncManagerCallback->isCanceled(nId, false))
		return true;
	
	if (!pSession->connect())
		return false;
	term.setConnected();
	if (pSyncManagerCallback->isCanceled(nId, false))
		return true;
	
	pCallback->setRange(0, static_cast<unsigned int>(listMessagePtr.size()));
	
	MessageHolderList l;
	l.resize(1);
	
	for (MessagePtrList::size_type m = 0; m < listMessagePtr.size(); ++m) {
		if (pSyncManagerCallback->isCanceled(nId, false))
			return true;
		
		Message msg;
		bool bGet = false;
		{
			MessagePtrLock mpl(listMessagePtr[m]);
			if (mpl) {
				if (!mpl->getMessage(Account::GETMESSAGEFLAG_ALL, 0, SECURITYMODE_NONE, &msg)) {
					addError(pSyncManagerCallback, nId, pAccount, 0, 0, IDS_ERROR_GETMESSAGE, 0);
					return false;
				}
				bGet = true;
			}
		}
		if (bGet) {
			const WCHAR* pwszRemoveFields[] = {
				L"X-QMAIL-Account",
				L"X-QMAIL-SubAccount",
				L"X-QMAIL-Signature"
			};
			for (int n = 0; n < countof(pwszRemoveFields); ++n)
				msg.removeField(pwszRemoveFields[n]);
			if (!pSession->sendMessage(&msg))
				return false;
			
			MessagePtrLock mpl(listMessagePtr[m]);
			if (mpl) {
				l[0] = mpl;
				unsigned int nCopyFlags = Account::COPYFLAG_MOVE |
					Account::OPFLAG_ACTIVE | Account::OPFLAG_BACKGROUND;
				if (!pAccount->setMessagesFlags(l,
						MessageHolder::FLAG_SENT, MessageHolder::FLAG_SENT, 0) ||
					!pAccount->copyMessages(l, pOutbox, pSentbox, nCopyFlags, 0, 0, 0)) {
					addError(pSyncManagerCallback, nId, pAccount, 0, 0, IDS_ERROR_MOVETOSENTBOX, 0);
					return false;
				}
			}
		}
		
		pCallback->setPos(static_cast<unsigned int>(m + 1));
	}
	
	if (!pAccount->saveMessages(false)) {
		addError(pSyncManagerCallback, nId, pAccount, 0, 0, IDS_ERROR_SAVE, 0);
		return false;
	}
	
	return true;
}

bool qm::SyncManager::openReceiveSession(Document* pDocument,
										 SyncManagerCallback* pSyncManagerCallback,
										 const SyncItem* pItem,
										 SyncData::Type type,
										 std::auto_ptr<ReceiveSession>* ppSession,
										 std::auto_ptr<ReceiveSessionCallback>* ppCallback,
										 std::auto_ptr<Logger>* ppLogger)
{
	assert(ppSession);
	assert(ppCallback);
	assert(ppLogger);
	
	ppSession->reset(0);
	
	Account* pAccount = pItem->getAccount();
	SubAccount* pSubAccount = pItem->getSubAccount();
	
	pSyncManagerCallback->setAccount(::GetCurrentThreadId(), pAccount, pSubAccount);
	
	std::auto_ptr<Logger> pLogger;
	if (pSubAccount->isLog(Account::HOST_RECEIVE))
		pLogger = pAccount->openLogger(Account::HOST_RECEIVE);
	
	std::auto_ptr<ReceiveSession> pSession(ReceiveSessionFactory::getSession(
		pAccount->getType(Account::HOST_RECEIVE)));
	if (!pSession.get())
		return false;
	
	std::auto_ptr<ReceiveSessionCallbackImpl> pCallback(new ReceiveSessionCallbackImpl(
		pSyncManagerCallback, pDocument->getRecents(), isNotify(type)));
	if (!pSession->init(pDocument, pAccount, pSubAccount,
		pProfile_, pLogger.get(), pCallback.get()))
		return false;
	
	*ppSession = pSession;
	*ppCallback = pCallback;
	*ppLogger = pLogger;
	
	return true;
}

bool qm::SyncManager::isNotify(SyncData::Type type) const
{
	switch (pProfile_->getInt(L"Sync", L"Notify")) {
	case NOTIFY_NEVER:
		return false;
	case NOTIFY_AUTO:
		return type == SyncData::TYPE_AUTO;
	case NOTIFY_ALWAYS:
	default:
		return true;
	}
}


/****************************************************************************
 *
 * SyncManager::SyncThread
 *
 */

qm::SyncManager::SyncThread::SyncThread(SyncManager* pSyncManager,
										std::auto_ptr<SyncData> pData) :
	pSyncManager_(pSyncManager),
	pSyncData_(pData),
	bWaitMode_(false)
{
}

qm::SyncManager::SyncThread::~SyncThread()
{
}

void qm::SyncManager::SyncThread::setWaitMode()
{
	bWaitMode_ = true;
}

void qm::SyncManager::SyncThread::run()
{
	InitThread init(0);
	
	struct StatusChange
	{
		StatusChange(SyncManager* pSyncManager) :
			pSyncManager_(pSyncManager)
		{
			pSyncManager_->fireStatusChanged();
		}
		
		~StatusChange()
		{
			pSyncManager_->fireStatusChanged();
		}
		
		SyncManager* pSyncManager_;
	} statusChange(pSyncManager_);
	
	pSyncManager_->syncData(pSyncData_.get());
	
	Lock<CriticalSection> lock(pSyncManager_->cs_);
	
	if (!bWaitMode_) {
		SyncManager::ThreadList::iterator it = std::find(
			pSyncManager_->listThread_.begin(),
			pSyncManager_->listThread_.end(), this);
		assert(it != pSyncManager_->listThread_.end());
		
		pSyncManager_->listThread_.erase(it);
		delete this;
	}
}


/****************************************************************************
 *
 * SyncManager::ParallelSyncThread
 *
 */

qm::SyncManager::ParallelSyncThread::ParallelSyncThread(SyncManager* pSyncManager,
														const SyncData* pData,
														const SyncData::ItemList& listItem) :
	pSyncManager_(pSyncManager),
	pSyncData_(pData),
	listItem_(listItem)
{
}

qm::SyncManager::ParallelSyncThread::~ParallelSyncThread()
{
}

void qm::SyncManager::ParallelSyncThread::run()
{
	InitThread init(0);
	
	pSyncManager_->syncSlotData(pSyncData_, listItem_);
}


/****************************************************************************
 *
 * SyncManager::ReceiveSessionCallbackImpl
 *
 */

qm::SyncManager::ReceiveSessionCallbackImpl::ReceiveSessionCallbackImpl(SyncManagerCallback* pCallback,
																		Recents* pRecents,
																		bool bNotify) :
	pCallback_(pCallback),
	nId_(::GetCurrentThreadId()),
	pRecents_(pRecents),
	bNotify_(bNotify)
{
	assert(pCallback);
	assert(pRecents);
}

qm::SyncManager::ReceiveSessionCallbackImpl::~ReceiveSessionCallbackImpl()
{
}

PasswordState qm::SyncManager::ReceiveSessionCallbackImpl::getPassword(SubAccount* pSubAccount,
																	   Account::Host host,
																	   wstring_ptr* pwstrPassword)
{
	return pCallback_->getPassword(pSubAccount, host, pwstrPassword);
}

void qm::SyncManager::ReceiveSessionCallbackImpl::setPassword(SubAccount* pSubAccount,
															  Account::Host host,
															  const WCHAR* pwszPassword,
															  bool bPermanent)
{
	pCallback_->setPassword(pSubAccount, host, pwszPassword, bPermanent);
}

bool qm::SyncManager::ReceiveSessionCallbackImpl::isCanceled(bool bForce)
{
	return pCallback_->isCanceled(nId_, bForce);
}

void qm::SyncManager::ReceiveSessionCallbackImpl::setPos(size_t n)
{
	pCallback_->setPos(nId_, false, n);
}

void qm::SyncManager::ReceiveSessionCallbackImpl::setRange(size_t nMin,
														   size_t nMax)
{
	pCallback_->setRange(nId_, false, nMin, nMax);
}

void qm::SyncManager::ReceiveSessionCallbackImpl::setSubPos(size_t n)
{
	pCallback_->setPos(nId_, true, n);
}

void qm::SyncManager::ReceiveSessionCallbackImpl::setSubRange(size_t nMin,
															  size_t nMax)
{
	pCallback_->setRange(nId_, true, nMin, nMax);
}

void qm::SyncManager::ReceiveSessionCallbackImpl::setMessage(const WCHAR* pwszMessage)
{
	pCallback_->setMessage(nId_, pwszMessage);
}

void qm::SyncManager::ReceiveSessionCallbackImpl::addError(const SessionErrorInfo& info)
{
	pCallback_->addError(nId_, info);
}

void qm::SyncManager::ReceiveSessionCallbackImpl::notifyNewMessage(MessagePtr ptr)
{
	if (!bNotify_)
		return;
	
	std::auto_ptr<URI> pURI;
	{
		MessagePtrLock mpl(ptr);
		if (mpl && !mpl->getFolder()->isFlag(Folder::FLAG_IGNOREUNSEEN))
			pURI.reset(new URI(mpl));
	}
	if (pURI.get()) {
		pRecents_->add(pURI);
		pCallback_->notifyNewMessage(nId_);
	}
}


/****************************************************************************
 *
 * SyncManager::SendSessionCallbackImpl
 *
 */

qm::SyncManager::SendSessionCallbackImpl::SendSessionCallbackImpl(SyncManagerCallback* pCallback) :
	pCallback_(pCallback)
{
	nId_ = ::GetCurrentThreadId();
}

qm::SyncManager::SendSessionCallbackImpl::~SendSessionCallbackImpl()
{
}

PasswordState qm::SyncManager::SendSessionCallbackImpl::getPassword(SubAccount* pSubAccount,
																	Account::Host host,
																	wstring_ptr* pwstrPassword)
{
	return pCallback_->getPassword(pSubAccount, host, pwstrPassword);
}

void qm::SyncManager::SendSessionCallbackImpl::setPassword(SubAccount* pSubAccount,
														   Account::Host host,
														   const WCHAR* pwszPassword,
														   bool bPermanent)
{
	pCallback_->setPassword(pSubAccount, host, pwszPassword, bPermanent);
}

bool qm::SyncManager::SendSessionCallbackImpl::isCanceled(bool bForce)
{
	return pCallback_->isCanceled(nId_, bForce);
}

void qm::SyncManager::SendSessionCallbackImpl::setPos(size_t n)
{
	pCallback_->setPos(nId_, false, n);
}

void qm::SyncManager::SendSessionCallbackImpl::setRange(size_t nMin,
														size_t nMax)
{
	pCallback_->setRange(nId_, false, nMin, nMax);
}

void qm::SyncManager::SendSessionCallbackImpl::setSubPos(size_t n)
{
	pCallback_->setPos(nId_, true, n);
}

void qm::SyncManager::SendSessionCallbackImpl::setSubRange(size_t nMin,
														   size_t nMax)
{
	pCallback_->setRange(nId_, true, nMin, nMax);
}

void qm::SyncManager::SendSessionCallbackImpl::setMessage(const WCHAR* pwszMessage)
{
	pCallback_->setMessage(nId_, pwszMessage);
}

void qm::SyncManager::SendSessionCallbackImpl::addError(const SessionErrorInfo& info)
{
	pCallback_->addError(nId_, info);
}


/****************************************************************************
 *
 * SyncManager::RasConnectionCallbackImpl
 *
 */

qm::SyncManager::RasConnectionCallbackImpl::RasConnectionCallbackImpl(const SyncDialup* pDialup,
																	  SyncManagerCallback* pCallback) :
	pDialup_(pDialup),
	pCallback_(pCallback)
{
}

qm::SyncManager::RasConnectionCallbackImpl::~RasConnectionCallbackImpl()
{
}

bool qm::SyncManager::RasConnectionCallbackImpl::isCanceled()
{
	return pCallback_->isCanceled(-1, false);
}

bool qm::SyncManager::RasConnectionCallbackImpl::preConnect(RASDIALPARAMS* prdp)
{
	if (pDialup_->getFlags() & SyncDialup::FLAG_SHOWDIALOG)
		return pCallback_->showDialupDialog(prdp);
	else
		return true;
}

void qm::SyncManager::RasConnectionCallbackImpl::setMessage(const WCHAR* pwszMessage)
{
	pCallback_->setMessage(-1, pwszMessage);
}

void qm::SyncManager::RasConnectionCallbackImpl::error(const WCHAR* pwszMessage)
{
	SyncManager::addError(pCallback_, -1, 0, 0, 0, IDS_ERROR_DIALUP, pwszMessage);
}


/****************************************************************************
 *
 * SyncManager::RuleCallbackImpl
 *
 */

qm::SyncManager::RuleCallbackImpl::RuleCallbackImpl(SyncManagerCallback* pCallback,
													unsigned int nId) :
	pCallback_(pCallback),
	nId_(nId)
{
}

qm::SyncManager::RuleCallbackImpl::~RuleCallbackImpl()
{
}

bool qm::SyncManager::RuleCallbackImpl::isCanceled()
{
	return pCallback_->isCanceled(nId_, false);
}

void qm::SyncManager::RuleCallbackImpl::checkingMessages(Folder* pFolder)
{
	wstring_ptr wstrMessage(getMessage(IDS_MESSAGE_CHECKMESSAGES, pFolder));
	pCallback_->setMessage(nId_, wstrMessage.get());
}

void qm::SyncManager::RuleCallbackImpl::applyingRule(Folder* pFolder)
{
	wstring_ptr wstrMessage(getMessage(IDS_MESSAGE_APPLYRULE, pFolder));
	pCallback_->setMessage(nId_, wstrMessage.get());
}

void qm::SyncManager::RuleCallbackImpl::setRange(size_t nMin,
												 size_t nMax)
{
	pCallback_->setRange(nId_, false, nMin, nMax);
}

void qm::SyncManager::RuleCallbackImpl::setPos(size_t nPos)
{
	pCallback_->setPos(nId_, false, nPos);
}

wstring_ptr qm::SyncManager::RuleCallbackImpl::getMessage(UINT nId,
														  Folder* pFolder)
{
	HINSTANCE hInst = Application::getApplication().getResourceHandle();
	wstring_ptr wstrMessage(loadString(hInst, nId));
	wstring_ptr wstrName(pFolder->getFullName());
	return concat(wstrMessage.get(), L" : ", wstrName.get());
}


/****************************************************************************
 *
 * SyncManager::FolderWait
 *
 */

qm::SyncManager::FolderWait::FolderWait(SyncManager* pSyncManager,
										NormalFolder* pFolder) :
	pSyncManager_(pSyncManager),
	pFolder_(pFolder),
	pEvent_(0)
{
	assert(pFolder);
	
	bool bWait = false;
	{
		typedef SyncManager::SyncingFolderList List;
		List& l = pSyncManager_->listSyncingFolder_;
		
		Lock<CriticalSection> lock(pSyncManager_->cs_);
		List::iterator it = l.begin();
		while (it != l.end()) {
			if ((*it).first == pFolder)
				break;
			++it;
		}
		if (it == l.end()) {
			std::auto_ptr<Event> pEvent(new Event(false, false));
			l.push_back(std::make_pair(pFolder, pEvent.get()));
			pEvent_ = pEvent.release();
		}
		else {
			pEvent_ = (*it).second;
			bWait = true;
		}
	}
	if (bWait)
		pEvent_->wait();
	
#ifndef NDEBUG
	{
		Account* pAccount = pFolder_->getAccount();
		Lock<Account> lock(*pAccount);
		assert(pAccount->getLockCount() == 1);
	}
#endif
}

qm::SyncManager::FolderWait::~FolderWait()
{
#ifndef NDEBUG
	{
		Account* pAccount = pFolder_->getAccount();
		Lock<Account> lock(*pAccount);
		assert(pAccount->getLockCount() == 1);
	}
#endif
	pEvent_->set();
}


/****************************************************************************
 *
 * SyncManagerCallback
 *
 */

qm::SyncManagerCallback::~SyncManagerCallback()
{
}


/****************************************************************************
 *
 * SyncManagerHandler
 *
 */

qm::SyncManagerHandler::~SyncManagerHandler()
{
}


/****************************************************************************
 *
 * SyncManagerEvent
 *
 */

qm::SyncManagerEvent::SyncManagerEvent()
{
}

qm::SyncManagerEvent::~SyncManagerEvent()
{
}
