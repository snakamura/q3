/*
 * $Id$
 *
 * Copyright(C) 1998-2005 Satoshi Nakamura
 * All rights reserved.
 *
 */

#pragma warning(disable:4786)

#include <qmaccount.h>
#include <qmapplication.h>
#include <qmdocument.h>
#include <qmfilenames.h>
#include <qmfolder.h>
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

#include "syncmanager.h"
#include "../model/uri.h"
#include "../ui/resourceinc.h"

using namespace qm;
using namespace qs;


/****************************************************************************
 *
 * SyncItem
 *
 */

qm::SyncItem::SyncItem(unsigned int nSlot,
					   Account* pAccount,
					   SubAccount* pSubAccount) :
	nSlot_(nSlot),
	pAccount_(pAccount),
	pSubAccount_(pSubAccount)
{
}

qm::SyncItem::~SyncItem()
{
}

unsigned int qm::SyncItem::getSlot() const
{
	return nSlot_;
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

qm::ReceiveSyncItem::ReceiveSyncItem(unsigned int nSlot,
									 Account* pAccount,
									 SubAccount* pSubAccount,
									 NormalFolder* pFolder,
									 std::auto_ptr<SyncFilterSet> pFilterSet,
									 unsigned int nFlags) :
	SyncItem(nSlot, pAccount, pSubAccount),
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

qm::SendSyncItem::SendSyncItem(unsigned int nSlot,
							   Account* pAccount,
							   SubAccount* pSubAccount,
							   ConnectReceiveBeforeSend crbs,
							   const WCHAR* pwszMessageId) :
	SyncItem(nSlot, pAccount, pSubAccount),
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

qm::SyncData::SyncData(SyncManager* pManager,
					   Document* pDocument,
					   bool bAuto,
					   unsigned int nCallbackParam) :
	pManager_(pManager),
	pDocument_(pDocument),
	bAuto_(bAuto),
	nCallbackParam_(nCallbackParam),
	pCallback_(0),
	nSlot_(0)
{
}

qm::SyncData::~SyncData()
{
	std::for_each(listItem_.begin(), listItem_.end(), deleter<SyncItem>());
}

Document* qm::SyncData::getDocument() const
{
	return pDocument_;
}

bool qm::SyncData::isAuto() const
{
	return bAuto_;
}

unsigned int qm::SyncData::getCallbackParam() const
{
	return nCallbackParam_;
}

const SyncDialup* qm::SyncData::getDialup() const
{
	return pDialup_.get();
}

bool qm::SyncData::isEmpty() const
{
	return listItem_.empty();
}

const SyncData::ItemList& qm::SyncData::getItems() const
{
	return listItem_;
}

unsigned int qm::SyncData::getSlotCount() const
{
	return listItem_.empty() ? 0 : listItem_.back()->getSlot();
}

SyncManagerCallback* qm::SyncData::getCallback() const
{
	return pCallback_;
}

void qm::SyncData::setCallback(SyncManagerCallback* pCallback)
{
	pCallback_ = pCallback;
}

void qm::SyncData::setDialup(std::auto_ptr<SyncDialup> pDialup)
{
	pDialup_ = pDialup;
}

void qm::SyncData::newSlot()
{
	if (!listItem_.empty() && listItem_.back()->getSlot() == nSlot_)
		++nSlot_;
}

void qm::SyncData::addFolder(Account* pAccount,
							 SubAccount* pSubAccount,
							 NormalFolder* pFolder,
							 const WCHAR* pwszFilterName,
							 unsigned int nFlags)
{
	SyncFilterManager* pManager = pManager_->getSyncFilterManager();
	std::auto_ptr<SyncFilterSet> pFilterSet(pManager->getFilterSet(pwszFilterName));
	std::auto_ptr<ReceiveSyncItem> pItem(new ReceiveSyncItem(
		nSlot_, pAccount, pSubAccount, pFolder, pFilterSet, nFlags));
	listItem_.push_back(pItem.get());
	pItem.release();
}

void qm::SyncData::addFolders(Account* pAccount,
							  SubAccount* pSubAccount,
							  const RegexPattern* pFolderNamePattern,
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
			if (pFolderNamePattern) {
				wstring_ptr wstrFolderName(pFolder->getFullName());
				bAdd = pFolderNamePattern->match(wstrFolderName.get());
			}
			if (bAdd)
				listFolder.push_back(pFolder);
		}
	}
	
	std::sort(listFolder.begin(), listFolder.end(), FolderLess());
	for (Account::FolderList::const_iterator it = listFolder.begin(); it != listFolder.end(); ++it)
		addFolder(pAccount, pSubAccount, static_cast<NormalFolder*>(*it), pwszFilterName, 0);
}

void qm::SyncData::addSend(Account* pAccount,
						   SubAccount* pSubAccount,
						   SendSyncItem::ConnectReceiveBeforeSend crbs,
						   const WCHAR* pwszMessageId)
{
	std::auto_ptr<SendSyncItem> pItem(new SendSyncItem(
		nSlot_, pAccount, pSubAccount, crbs, pwszMessageId));
	listItem_.push_back(pItem.get());
	pItem.release();
}


/****************************************************************************
 *
 * SyncManager
 *
 */

qm::SyncManager::SyncManager(Profile* pProfile) :
	pProfile_(pProfile),
	pSynchronizer_(InitThread::getInitThread().getSynchronizer())
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
		::WaitForMultipleObjects(handles.size(), &handles[0], TRUE, INFINITE);
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

bool qm::SyncManager::syncData(const SyncData* pData)
{
	SyncManagerCallback* pCallback = pData->getCallback();
	assert(pCallback);
	
	struct CallbackCaller
	{
		CallbackCaller(SyncManagerCallback* pCallback,
					   const SyncData* pData) :
			pCallback_(pCallback)
		{
			pCallback_->start(pData->getCallbackParam());
		}
		
		~CallbackCaller()
		{
			pCallback_->end();
		}
		
		SyncManagerCallback* pCallback_;
	} caller(pCallback, pData);
	
	const SyncDialup* pDialup = pData->getDialup();
	if (pDialup && pDialup->getFlags() & SyncDialup::FLAG_WHENEVERNOTCONNECTED &&
		RasConnection::isNetworkConnected())
		pDialup = 0;
	
	RasConnectionCallbackImpl rasCallback(pDialup, pCallback);
	std::auto_ptr<RasConnection> pRasConnection;
	if (pDialup) {
		const WCHAR* pwszDialFrom = pDialup->getDialFrom();
		if (pwszDialFrom)
			RasConnection::setLocation(pwszDialFrom);
		
		pRasConnection.reset(new RasConnection(&rasCallback));
		
		const WCHAR* pwszEntry = pDialup->getEntry();
		wstring_ptr wstrEntry;
		if (!pwszEntry) {
			wstrEntry = pCallback->selectDialupEntry();
			if (!wstrEntry.get())
				return true;
			pwszEntry = wstrEntry.get();
		}
		assert(pwszEntry);
		
		RasConnection::Result result = pRasConnection->connect(pwszEntry);
		if (result == RasConnection::RAS_FAIL)
			return false;
		pCallback->setMessage(-1, L"");
		if (result == RasConnection::RAS_CANCEL)
			return true;
	}
	
	struct DialupDisconnector
	{
		DialupDisconnector(RasConnection* pRasConnection,
						   unsigned int nWait) :
			pRasConnection_(pRasConnection),
			nWait_(nWait)
		{
		}
		
		~DialupDisconnector()
		{
			if (pRasConnection_)
				pRasConnection_->disconnect(nWait_);
		}
		
		RasConnection* pRasConnection_;
		unsigned int nWait_;
	} disconnector(!pDialup || pDialup->getFlags() & SyncDialup::FLAG_NOTDISCONNECT ? 0 : pRasConnection.get(),
		pDialup ? pDialup->getDisconnectWait() : 0);
	
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
	
	unsigned int nSlot = pData->getSlotCount();
	if (nSlot > 0) {
		typedef std::vector<Thread*> ThreadList;
		ThreadList listThread;
		listThread.reserve(nSlot);
		
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
		
		for (unsigned int n = 0; n < nSlot; ++n) {
			std::auto_ptr<ParallelSyncThread> pThread(new ParallelSyncThread(this, pData, n));
			if (!pThread->start())
				break;
			listThread.push_back(pThread.release());
		}
		syncSlotData(pData, nSlot);
	}
	else {
		syncSlotData(pData, 0);
	}
	
	return true;
}

void qm::SyncManager::syncSlotData(const SyncData* pData,
								   unsigned int nSlot)
{
	SyncManagerCallback* pCallback = pData->getCallback();
	assert(pCallback);
	
	struct CallbackCaller
	{
		CallbackCaller(SyncManagerCallback* pCallback,
					   unsigned int nId,
					   unsigned int nParam) :
			pCallback_(pCallback),
			nId_(nId),
			nParam_(nParam)
		{
			pCallback_->startThread(nId_, nParam_);
		}
		
		~CallbackCaller()
		{
			pCallback_->endThread(nId_);
		}
		
		SyncManagerCallback* pCallback_;
		unsigned int nId_;
		unsigned int nParam_;
	} caller(pCallback, ::GetCurrentThreadId(), pData->getCallbackParam());
	
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
	
	
	std::auto_ptr<Logger> pLogger;
	std::auto_ptr<ReceiveSessionCallback> pReceiveCallback;
	ReceiveSessionTerm session;
	SubAccount* pSubAccount = 0;
	typedef std::vector<SubAccount*> SubAccountList;
	SubAccountList listConnectFailed;
	const SyncData::ItemList& l = pData->getItems();
	for (SyncData::ItemList::const_iterator it = l.begin(); it != l.end(); ++it) {
		const SyncItem* pItem = *it;
		if (pItem->getSlot() == nSlot) {
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
						pCallback, pItem, pData->isAuto(),
						&pReceiveSession, &pReceiveCallback, &pLogger))
						continue;
					session.reset(pReceiveSession);
					if (pCallback->isCanceled(pItem->getSlot(), false))
						break;
					
					if (!session.get()->connect()) {
						listConnectFailed.push_back(pItem->getSubAccount());
						continue;
					}
					session.setConnected();
					if (pCallback->isCanceled(pItem->getSlot(), false))
						break;
					
					if (!session.get()->applyOfflineJobs())
						continue;
					if (pCallback->isCanceled(pItem->getSlot(), false))
						break;
					
					pSubAccount = pItem->getSubAccount();
				}
				if (bSync) {
					if (!syncFolder(pCallback, pItem, session.get()))
						continue;
					if (pCallback->isCanceled(pItem->getSlot(), false))
						break;
				}
			}
			
			if (pItem->isSend()) {
				if (!send(pData->getDocument(), pCallback, static_cast<const SendSyncItem*>(pItem)))
					continue;
				if (pCallback->isCanceled(pItem->getSlot(), false))
					break;
			}
		}
	}
}

bool qm::SyncManager::syncFolder(SyncManagerCallback* pSyncManagerCallback,
								 const SyncItem* pItem,
								 ReceiveSession* pSession)
{
	assert(pSession);
	
	const ReceiveSyncItem* pReceiveItem = pItem->isSend() ? 0 :
		static_cast<const ReceiveSyncItem*>(pItem);
	
	NormalFolder* pFolder = pItem->getFolder();
	if (!pFolder || !pFolder->isFlag(Folder::FLAG_SYNCABLE))
		return true;
	
	pSyncManagerCallback->setFolder(::GetCurrentThreadId(), pFolder);
	
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
	if (pSyncManagerCallback->isCanceled(pItem->getSlot(), false))
		return true;
	
	if (!pSession->updateMessages())
		return false;
	if (pSyncManagerCallback->isCanceled(pItem->getSlot(), false))
		return true;
	
	const SyncFilterSet* pFilterSet = pReceiveItem ?
		pReceiveItem->getFilterSet() : 0;
	if (!pSession->downloadMessages(pFilterSet))
		return false;
	if (pSyncManagerCallback->isCanceled(pItem->getSlot(), false))
		return true;
	
	if (!pFolder->getAccount()->saveMessages())
		return false;
	
	if (!pSession->closeFolder())
		return false;
	
	return true;
}

bool qm::SyncManager::send(Document* pDocument,
						   SyncManagerCallback* pSyncManagerCallback,
						   const SendSyncItem* pItem)
{
	unsigned int nId = ::GetCurrentThreadId();
	pSyncManagerCallback->setAccount(nId, pItem->getAccount(), pItem->getSubAccount());
	
	Account* pAccount = pItem->getAccount();
	SubAccount* pSubAccount = pItem->getSubAccount();
	NormalFolder* pOutbox = static_cast<NormalFolder*>(
		pAccount->getFolderByBoxFlag(Folder::FLAG_OUTBOX));
	if (!pOutbox)
		return false;
	if (!pOutbox->loadMessageHolders())
		return false;
	
	NormalFolder* pSentbox = static_cast<NormalFolder*>(
		pAccount->getFolderByBoxFlag(Folder::FLAG_SENTBOX));
	if (!pSentbox)
		return false;
	
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
						L"X-QMAIL-SubAccount", SECURITYMODE_NONE, &msg))
						return false;
					
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
				
				if (pSyncManagerCallback->isCanceled(pItem->getSlot(), false))
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
	
	if (pSyncManagerCallback->isCanceled(pItem->getSlot(), false))
		return true;
	
	if (!pSession->connect())
		return false;
	term.setConnected();
	if (pSyncManagerCallback->isCanceled(pItem->getSlot(), false))
		return true;
	
	pCallback->setRange(0, listMessagePtr.size());
	
	MessageHolderList l;
	l.resize(1);
	
	for (MessagePtrList::size_type m = 0; m < listMessagePtr.size(); ++m) {
		if (pSyncManagerCallback->isCanceled(pItem->getSlot(), false))
			return true;
		
		Message msg;
		bool bGet = false;
		{
			MessagePtrLock mpl(listMessagePtr[m]);
			if (mpl) {
				if (!mpl->getMessage(Account::GETMESSAGEFLAG_ALL, 0, SECURITYMODE_NONE, &msg))
					return false;
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
				if (!pAccount->setMessagesFlags(l,
					MessageHolder::FLAG_SENT, MessageHolder::FLAG_SENT, 0) ||
					!pAccount->copyMessages(l, pOutbox, pSentbox, Account::COPYFLAG_MOVE, 0, 0))
					return false;
			}
		}
		
		pCallback->setPos(m + 1);
	}
	
	if (!pAccount->saveMessages())
		return false;
	
	return true;
}

bool qm::SyncManager::openReceiveSession(Document* pDocument,
										 SyncManagerCallback* pSyncManagerCallback,
										 const SyncItem* pItem,
										 bool bAuto,
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
		pSyncManagerCallback, pDocument->getRecents(), bAuto));
	if (!pSession->init(pDocument, pAccount, pSubAccount,
		pProfile_, pLogger.get(), pCallback.get()))
		return false;
	
	*ppSession = pSession;
	*ppCallback = pCallback;
	*ppLogger = pLogger;
	
	return true;
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
														unsigned int nSlot) :
	pSyncManager_(pSyncManager),
	pSyncData_(pData),
	nSlot_(nSlot)
{
}

qm::SyncManager::ParallelSyncThread::~ParallelSyncThread()
{
}

void qm::SyncManager::ParallelSyncThread::run()
{
	InitThread init(0);
	
	pSyncManager_->syncSlotData(pSyncData_, nSlot_);
}


/****************************************************************************
 *
 * SyncManager::ReceiveSessionCallbackImpl
 *
 */

qm::SyncManager::ReceiveSessionCallbackImpl::ReceiveSessionCallbackImpl(SyncManagerCallback* pCallback,
																		Recents* pRecents,
																		bool bAuto) :
	pCallback_(pCallback),
	pRecents_(pRecents),
	bAuto_(bAuto)
{
	assert(pCallback);
	assert(pRecents);
	
	nId_ = ::GetCurrentThreadId();
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

void qm::SyncManager::ReceiveSessionCallbackImpl::setPos(unsigned int n)
{
	pCallback_->setPos(nId_, false, n);
}

void qm::SyncManager::ReceiveSessionCallbackImpl::setRange(unsigned int nMin,
														   unsigned int nMax)
{
	pCallback_->setRange(nId_, false, nMin, nMax);
}

void qm::SyncManager::ReceiveSessionCallbackImpl::setSubPos(unsigned int n)
{
	pCallback_->setPos(nId_, true, n);
}

void qm::SyncManager::ReceiveSessionCallbackImpl::setSubRange(unsigned int nMin,
															  unsigned int nMax)
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
	pCallback_->notifyNewMessage(nId_);
	
	std::auto_ptr<URI> pURI;
	{
		MessagePtrLock mpl(ptr);
		if (mpl)
			pURI.reset(new URI(mpl));
	}
	if (pURI.get())
		pRecents_->add(pURI, bAuto_);
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

void qm::SyncManager::SendSessionCallbackImpl::setPos(unsigned int n)
{
	pCallback_->setPos(nId_, false, n);
}

void qm::SyncManager::SendSessionCallbackImpl::setRange(unsigned int nMin,
														unsigned int nMax)
{
	pCallback_->setRange(nId_, false, nMin, nMax);
}

void qm::SyncManager::SendSessionCallbackImpl::setSubPos(unsigned int n)
{
	pCallback_->setPos(nId_, true, n);
}

void qm::SyncManager::SendSessionCallbackImpl::setSubRange(unsigned int nMin,
														   unsigned int nMax)
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
	HINSTANCE hInst = Application::getApplication().getResourceHandle();
	wstring_ptr wstrMessage(loadString(hInst, IDS_ERROR_DIALUP));
	SessionErrorInfo info(0, 0, 0, wstrMessage.get(), 0, &pwszMessage, 1);
	pCallback_->addError(-1, info);
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
