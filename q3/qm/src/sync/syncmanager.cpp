/*
 * $Id$
 *
 * Copyright(C) 1998-2003 Satoshi Nakamura
 * All rights reserved.
 *
 */

#include <qmaccount.h>
#include <qmapplication.h>
#include <qmdocument.h>
#include <qmfolder.h>
#include <qmmessage.h>
#include <qmmessageholder.h>
#include <qmsession.h>
#include <qmsyncfilter.h>

#include <qserror.h>
#include <qsinit.h>
#include <qsnew.h>
#include <qsstl.h>

#include <algorithm>
#include <functional>

#include "syncmanager.h"
#include "../ui/resourceinc.h"

#pragma warning(disable:4786)

using namespace qm;
using namespace qs;


/****************************************************************************
 *
 * SyncItem
 *
 */

qm::SyncItem::SyncItem(Account* pAccount, SubAccount* pSubAccount,
	NormalFolder* pFolder, const SyncFilterSet* pFilterSet, unsigned int nSlot) :
	pAccount_(pAccount),
	pSubAccount_(pSubAccount),
	pFolder_(pFolder),
	pFilterSet_(pFilterSet),
	bSend_(false),
	bConnectReceiveBeforeSend_(false),
	nSlot_(nSlot)
{
}

qm::SyncItem::SyncItem(Account* pAccount, SubAccount* pSubAccount,
	ConnectReceiveBeforeSend crbs, unsigned int nSlot) :
	pAccount_(pAccount),
	pSubAccount_(pSubAccount),
	pFolder_(0),
	pFilterSet_(0),
	bSend_(true),
	bConnectReceiveBeforeSend_(false),
	nSlot_(nSlot)
{
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

NormalFolder* qm::SyncItem::getFolder() const
{
	return pFolder_;
}

const SyncFilterSet* qm::SyncItem::getFilterSet() const
{
	return pFilterSet_;
}

bool qm::SyncItem::isSend() const
{
	return bSend_;
}

bool qm::SyncItem::isConnectReceiveBeforeSend() const
{
	return bConnectReceiveBeforeSend_;
}

unsigned int qm::SyncItem::getSlot() const
{
	return nSlot_;
}


/****************************************************************************
 *
 * SyncDialup
 *
 */

qm::SyncDialup::SyncDialup(const WCHAR* pwszEntry, unsigned int nFlags,
	const WCHAR* pwszDialFrom, unsigned int nDisconnectWait, QSTATUS* pstatus) :
	wstrEntry_(0),
	nFlags_(nFlags),
	wstrDialFrom_(0),
	nDisconnectWait_(nDisconnectWait)
{
	DECLARE_QSTATUS();
	
	string_ptr<WSTRING> wstrEntry;
	if (pwszEntry) {
		wstrEntry.reset(allocWString(pwszEntry));
		if (!wstrEntry.get()) {
			*pstatus = QSTATUS_OUTOFMEMORY;
			return;
		}
	}
	
	string_ptr<WSTRING> wstrDialFrom;
	if (pwszDialFrom) {
		wstrDialFrom.reset(allocWString(pwszDialFrom));
		if (!wstrDialFrom.get()) {
			*pstatus = QSTATUS_OUTOFMEMORY;
			return;
		}
	}
	
	wstrEntry_ = wstrEntry.release();
	wstrDialFrom_ = wstrDialFrom.release();
}

qm::SyncDialup::~SyncDialup()
{
	freeWString(wstrEntry_);
	freeWString(wstrDialFrom_);
}

const WCHAR* qm::SyncDialup::getEntry() const
{
	return wstrEntry_;
}

unsigned int qm::SyncDialup::getFlags() const
{
	return nFlags_;
}

const WCHAR* qm::SyncDialup::getDialFrom() const
{
	return wstrDialFrom_;
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

qm::SyncData::SyncData(SyncManager* pManager, Document* pDocument,
	HWND hwnd, unsigned int nCallbackParam, QSTATUS* pstatus) :
	pManager_(pManager),
	pDocument_(pDocument),
	hwnd_(hwnd),
	nCallbackParam_(nCallbackParam),
	pCallback_(0),
	pDialup_(0),
	nSlot_(0)
{
	assert(pstatus);
	*pstatus = QSTATUS_SUCCESS;
}

qm::SyncData::~SyncData()
{
	delete pDialup_;
}

Document* qm::SyncData::getDocument() const
{
	return pDocument_;
}

HWND qm::SyncData::getWindow() const
{
	return hwnd_;
}

bool qm::SyncData::isEmpty() const
{
	return listItem_.empty();
}

unsigned int qm::SyncData::getCallbackParam() const
{
	return nCallbackParam_;
}

const SyncDialup* qm::SyncData::getDialup() const
{
	return pDialup_;
}

const SyncData::ItemList& qm::SyncData::getItems() const
{
	return listItem_;
}

unsigned int qm::SyncData::getSlotCount() const
{
	return listItem_.empty() ? 0 : listItem_.back().getSlot();
}

SyncManagerCallback* qm::SyncData::getCallback() const
{
	return pCallback_;
}

void qm::SyncData::setCallback(SyncManagerCallback* pCallback)
{
	pCallback_ = pCallback;
}

void qm::SyncData::setDialup(SyncDialup* pDialup)
{
	pDialup_ = pDialup;
}

void qm::SyncData::newSlot()
{
	if (!listItem_.empty() && listItem_.back().getSlot() == nSlot_)
		++nSlot_;
}

QSTATUS qm::SyncData::addFolder(Account* pAccount, SubAccount* pSubAccount,
	NormalFolder* pFolder, const WCHAR* pwszFilterName)
{
	DECLARE_QSTATUS();
	
	const SyncFilterSet* pFilterSet = 0;
	status = pManager_->getSyncFilterManager()->getFilterSet(
		pAccount, pwszFilterName, &pFilterSet);
	status = STLWrapper<ItemList>(listItem_).push_back(
		SyncItem(pAccount, pSubAccount, pFolder, pFilterSet, nSlot_));
	CHECK_QSTATUS();
	
	return QSTATUS_SUCCESS;
}

QSTATUS qm::SyncData::addFolders(Account* pAccount, SubAccount* pSubAccount,
	const qs::RegexPattern* pFolderNamePattern, const WCHAR* pwszFilterName)
{
	DECLARE_QSTATUS();
	
	Account::FolderList listFolder;
	
	const Account::FolderList& l = pAccount->getFolders();
	Account::FolderList::const_iterator it = l.begin();
	while (it != l.end()) {
		Folder* pFolder = *it;
		if (pFolder->getType() == Folder::TYPE_NORMAL &&
			!pFolder->isFlag(Folder::FLAG_NOSELECT) &&
			!pFolder->isHidden() &&
			pFolder->isFlag(Folder::FLAG_SYNCABLE)) {
			bool bAdd = true;
			if (pFolderNamePattern) {
				string_ptr<WSTRING> wstrFolderName;
				status = pFolder->getFullName(&wstrFolderName);
				CHECK_QSTATUS();
				status = pFolderNamePattern->match(wstrFolderName.get(), &bAdd);
				CHECK_QSTATUS();
			}
			if (bAdd) {
				status = STLWrapper<Account::FolderList>(listFolder).push_back(pFolder);
				CHECK_QSTATUS();
			}
		}
		++it;
	}
	
	std::sort(listFolder.begin(), listFolder.end(), FolderLess());
	it = listFolder.begin();
	while (it != listFolder.end()) {
		status = addFolder(pAccount, pSubAccount,
			static_cast<NormalFolder*>(*it), pwszFilterName);
		CHECK_QSTATUS();
		++it;
	}
	
	
	return QSTATUS_SUCCESS;
}

QSTATUS qm::SyncData::addSend(Account* pAccount,
	SubAccount* pSubAccount, SyncItem::ConnectReceiveBeforeSend crbs)
{
	return STLWrapper<ItemList>(listItem_).push_back(
		SyncItem(pAccount, pSubAccount, crbs, nSlot_));
}


/****************************************************************************
 *
 * SyncManager
 *
 */

qm::SyncManager::SyncManager(Profile* pProfile, QSTATUS* pstatus) :
	pProfile_(pProfile),
	pSynchronizer_(InitThread::getInitThread().getSynchronizer()),
	pSyncFilterManager_(0)
{
	assert(pstatus);
	
	*pstatus = QSTATUS_SUCCESS;
	
	DECLARE_QSTATUS();
	
	status = newQsObject(&pSyncFilterManager_);
	CHECK_QSTATUS_SET(pstatus);
	
	status = STLWrapper<ThreadList>(listThread_).reserve(THREAD_MAX);
	CHECK_QSTATUS_SET(pstatus);
}

qm::SyncManager::~SyncManager()
{
	if (pSyncFilterManager_)
		dispose();
}

QSTATUS qm::SyncManager::dispose()
{
	DWORD dwCount = 0;
	HANDLE handles[THREAD_MAX];
	{
		Lock<CriticalSection> lock(cs_);
		dwCount = listThread_.size();
		
		for (ThreadList::size_type n = 0; n < listThread_.size(); ++n) {
			SyncThread* pThread = listThread_[n];
			handles[n] = pThread->getHandle();
			pThread->setWaitMode();
		}
	}
	
	if (dwCount != 0) {
		::WaitForMultipleObjects(dwCount, handles, TRUE, INFINITE);
		
		std::for_each(listThread_.begin(),
			listThread_.end(), deleter<SyncThread>());
		listThread_.clear();
	}
	
	std::for_each(listSyncingFolder_.begin(), listSyncingFolder_.end(),
		unary_compose_f_gx(
			deleter<Event>(),
			std::select2nd<SyncingFolderList::value_type>()));
	
	delete pSyncFilterManager_;
	
	pProfile_ = 0;
	pSyncFilterManager_ = 0;
	
	return QSTATUS_SUCCESS;
}

QSTATUS qm::SyncManager::sync(SyncData* pData)
{
	DECLARE_QSTATUS();
	
	Lock<CriticalSection> lock(cs_);
	
	if (listThread_.size() >= THREAD_MAX)
		return QSTATUS_FAIL;
	
	std::auto_ptr<SyncThread> pThread;
	status = newQsObject(this, pData, &pThread);
	CHECK_QSTATUS();
	
	status = pThread->start();
	CHECK_QSTATUS();
	
	listThread_.push_back(pThread.release());
	
	return QSTATUS_SUCCESS;
}

bool qm::SyncManager::isSyncing() const
{
	Lock<CriticalSection> lock(cs_);
	return !listThread_.empty();
}

SyncFilterManager* qm::SyncManager::getSyncFilterManager() const
{
	return pSyncFilterManager_;
}

QSTATUS qm::SyncManager::addSyncManagerHandler(SyncManagerHandler* pHandler)
{
	return STLWrapper<SyncManagerHandlerList>(listHandler_).push_back(pHandler);
}

QSTATUS qm::SyncManager::removeSyncManagerHandler(SyncManagerHandler* pHandler)
{
	SyncManagerHandlerList::iterator it = std::remove(
		listHandler_.begin(), listHandler_.end(), pHandler);
	assert(it != listHandler_.end());
	listHandler_.erase(it, listHandler_.end());
	return QSTATUS_SUCCESS;
}

QSTATUS qm::SyncManager::fireStatusChanged() const
{
	DECLARE_QSTATUS();
	
	SyncManagerEvent event;
	SyncManagerHandlerList::const_iterator it = listHandler_.begin();
	while (it != listHandler_.end()) {
		status = (*it)->statusChanged(event);
		CHECK_QSTATUS();
		++it;
	}
	
	return QSTATUS_SUCCESS;
}

QSTATUS qm::SyncManager::syncData(const SyncData* pData)
{
	DECLARE_QSTATUS();
	
	SyncManagerCallback* pCallback = pData->getCallback();
	assert(pCallback);
	
	struct CallbackCaller
	{
		CallbackCaller(SyncManagerCallback* pCallback,
			const SyncData* pData, QSTATUS* pstatus) :
			pCallback_(pCallback)
		{
			*pstatus = pCallback_->start(pData->getCallbackParam());
		}
		
		~CallbackCaller()
		{
			pCallback_->end();
		}
		
		SyncManagerCallback* pCallback_;
	} caller(pCallback, pData, &status);
	CHECK_QSTATUS();
	
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
		
		status = newQsObject(pDialup->getDisconnectWait(),
			&rasCallback, &pRasConnection);
		CHECK_QSTATUS();
		
		const WCHAR* pwszEntry = pDialup->getEntry();
		string_ptr<WSTRING> wstrEntry;
		if (!pwszEntry) {
			status = pCallback->selectDialupEntry(&wstrEntry);
			CHECK_QSTATUS();
			if (!wstrEntry.get())
				return QSTATUS_SUCCESS;
			pwszEntry = wstrEntry.get();
		}
		assert(pwszEntry);
		
		RasConnection::Result result;
		status = pRasConnection->connect(pwszEntry, &result);
		CHECK_QSTATUS();
		status = pCallback->setMessage(-1, L"");
		CHECK_QSTATUS();
		if (result == RasConnection::RAS_CANCEL)
			return QSTATUS_SUCCESS;
	}
	
	struct DialupDisconnector
	{
		DialupDisconnector(RasConnection* pRasConnection) :
			pRasConnection_(pRasConnection)
		{
		}
		
		~DialupDisconnector()
		{
			if (pRasConnection_) {
				RasConnection::Result result;
				pRasConnection_->disconnect(true, &result);
			}
		}
		
		RasConnection* pRasConnection_;
	} disconnector(!pDialup || pDialup->getFlags() & SyncDialup::FLAG_NOTDISCONNECT ?
		0 : pRasConnection.get());
	
	struct InternalOnline
	{
		InternalOnline(Document* pDocument, Synchronizer* pSynchronizer) :
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
			RunnableImpl(Document* pDocument, bool bIncrement) :
				pDocument_(pDocument),
				bIncrement_(bIncrement)
			{
			}
			
			virtual unsigned int run()
			{
				if (bIncrement_)
					pDocument_->incrementInternalOnline();
				else
					pDocument_->decrementInternalOnline();
				return 0;
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
		status = STLWrapper<ThreadList>(listThread).resize(nSlot);
		CHECK_QSTATUS();
		
		struct Wait
		{
			typedef std::vector<Thread*> ThreadList;
			Wait(const ThreadList& l) : l_(l) {}
			~Wait()
			{
				ThreadList::const_iterator it = l_.begin();
				while (it != l_.end()) {
					std::auto_ptr<Thread> pThread(*it);
					pThread->join();
					++it;
				}
			}
			const ThreadList& l_;
		} wait(listThread);
		
		unsigned int n = 0;
		for (n = 0; n < nSlot; ++n) {
			std::auto_ptr<ParallelSyncThread> pThread;
			status = newQsObject(this, pData, n, &pThread);
			CHECK_QSTATUS();
			status = pThread->start();
			CHECK_QSTATUS();
			listThread[n] = pThread.release();
		}
		status = syncSlotData(pData, nSlot);
		CHECK_QSTATUS();
	}
	else {
		status = syncSlotData(pData, 0);
		CHECK_QSTATUS();
	}
	
	return QSTATUS_SUCCESS;
}

QSTATUS qm::SyncManager::syncSlotData(const SyncData* pData, unsigned int nSlot)
{
	DECLARE_QSTATUS();
	
	SyncManagerCallback* pCallback = pData->getCallback();
	assert(pCallback);
	
	struct CallbackCaller
	{
		CallbackCaller(SyncManagerCallback* pCallback,
			unsigned int nId, unsigned int nParam, QSTATUS* pstatus) :
			pCallback_(pCallback),
			nId_(nId),
			nParam_(nParam)
		{
			*pstatus = pCallback_->startThread(nId_, nParam_);
		}
		
		~CallbackCaller()
		{
			pCallback_->endThread(nId_);
		}
		
		SyncManagerCallback* pCallback_;
		unsigned int nId_;
		unsigned int nParam_;
	} caller(pCallback, ::GetCurrentThreadId(), pData->getCallbackParam(), &status);
	CHECK_QSTATUS();
	
	std::auto_ptr<Logger> pLogger;
	std::auto_ptr<ReceiveSession> pReceiveSession;
	std::auto_ptr<ReceiveSessionCallback> pReceiveCallback;
	SubAccount* pSubAccount = 0;
	const SyncData::ItemList& l = pData->getItems();
	SyncData::ItemList::const_iterator it = l.begin();
	while (it != l.end()) {
		const SyncItem& item = *it;
		if (item.getSlot() == nSlot) {
			bool bSync = true;
			if (item.isSend()) {
				Account* pAccount = item.getAccount();
				Folder* pFolder = pAccount->getFolderByFlag(Folder::FLAG_OUTBOX);
				bSync = pFolder &&
					pFolder->getType() == Folder::TYPE_NORMAL &&
					pFolder->isFlag(Folder::FLAG_SYNCABLE);
			}
			
			if (bSync || item.isConnectReceiveBeforeSend()) {
				if (pSubAccount != item.getSubAccount()) {
					if (pReceiveSession.get()) {
						status = pReceiveSession->disconnect();
						CHECK_QSTATUS();
					}
					ReceiveSession* p = 0;
					ReceiveSessionCallback* pc = 0;
					Logger* pl = 0;
					status = openReceiveSession(pData->getDocument(),
						pData->getWindow(), pCallback, item, &p, &pc, &pl);
					CHECK_QSTATUS();
					pReceiveSession.reset(p);
					pReceiveCallback.reset(pc);
					pLogger.reset(pl);
					pSubAccount = item.getSubAccount();
				}
				if (bSync) {
					status = syncFolder(pCallback, item, pReceiveSession.get());
					CHECK_QSTATUS();
				}
			}
			
			if (item.isSend()) {
				status = send(pData->getDocument(), pCallback, item);
				CHECK_QSTATUS();
			}
		}
		++it;
	}
	if (pReceiveSession.get()) {
		status = pReceiveSession->disconnect();
		CHECK_QSTATUS();
	}
	
	return QSTATUS_SUCCESS;
}

QSTATUS qm::SyncManager::syncFolder(SyncManagerCallback* pSyncManagerCallback,
	const SyncItem& item, ReceiveSession* pSession)
{
	assert(pSession);
	
	DECLARE_QSTATUS();
	
	NormalFolder* pFolder = 0;
	if (item.isSend()) {
		Account* pAccount = item.getAccount();
		Folder* pOutbox = pAccount->getFolderByFlag(Folder::FLAG_OUTBOX);
		if (pOutbox && pOutbox->getType() == Folder::TYPE_NORMAL)
			pFolder = static_cast<NormalFolder*>(pOutbox);
	}
	else {
		pFolder = item.getFolder();
	}
	if (!pFolder || !pFolder->isFlag(Folder::FLAG_SYNCABLE))
		return QSTATUS_SUCCESS;
	
	status = pSyncManagerCallback->setFolder(
		::GetCurrentThreadId(), pFolder);
	CHECK_QSTATUS();
	
	FolderWait wait(this, pFolder, &status);
	CHECK_QSTATUS();
	
	status = pFolder->loadMessageHolders();
	CHECK_QSTATUS();
	
	status = pSession->selectFolder(pFolder);
	CHECK_QSTATUS();
	pFolder->setLastSyncTime(::GetTickCount());
	
	status = pSession->updateMessages();
	CHECK_QSTATUS();
	
	status = pSession->downloadMessages(item.getFilterSet());
	CHECK_QSTATUS();
	
	status = pFolder->getAccount()->flushMessageStore();
	CHECK_QSTATUS();
	
	status = pFolder->saveMessageHolders();
	CHECK_QSTATUS();
	
	status = pSession->closeFolder();
	CHECK_QSTATUS();
	
	return QSTATUS_SUCCESS;
}

QSTATUS qm::SyncManager::send(Document* pDocument,
	SyncManagerCallback* pSyncManagerCallback, const SyncItem& item)
{
	assert(item.isSend());
	
	DECLARE_QSTATUS();
	
	unsigned int nId = ::GetCurrentThreadId();
	status = pSyncManagerCallback->setAccount(nId,
		item.getAccount(), item.getSubAccount());
	CHECK_QSTATUS();
	
	Account* pAccount = item.getAccount();
	SubAccount* pSubAccount = item.getSubAccount();
	Folder* pFolder = 0;
	pFolder = pAccount->getFolderByFlag(Folder::FLAG_OUTBOX);
	if (!pFolder || pFolder->getType() != Folder::TYPE_NORMAL)
		return QSTATUS_FAIL;
	NormalFolder* pOutbox = static_cast<NormalFolder*>(pFolder);
	status = pOutbox->loadMessageHolders();
	CHECK_QSTATUS();
	
	pFolder = pAccount->getFolderByFlag(Folder::FLAG_SENTBOX);
	if (!pFolder || pFolder->getType() != Folder::TYPE_NORMAL)
		return QSTATUS_FAIL;
	NormalFolder* pSentbox = static_cast<NormalFolder*>(pFolder);
	
	const WCHAR* pwszIdentity = pSubAccount->getIdentity();
	assert(pwszIdentity);
	
	MessagePtrList listMessagePtr;
	status = STLWrapper<MessagePtrList>(
		listMessagePtr).reserve(pOutbox->getCount());
	CHECK_QSTATUS();
	
	{
		Lock<Account> lock(*pOutbox->getAccount());
		
		for (unsigned int n = 0; n < pOutbox->getCount(); ++n) {
			MessageHolder* pmh = pOutbox->getMessage(n);
			if (!pmh->isFlag(MessageHolder::FLAG_DRAFT) &&
				!pmh->isFlag(MessageHolder::FLAG_DELETED)) {
				Message msg(&status);
				CHECK_QSTATUS();
				status = pmh->getMessage(
					Account::GETMESSAGEFLAG_HEADER,
					L"X-QMAIL-SubAccount", &msg);
				CHECK_QSTATUS();
				
				bool bSend = false;
				if (*pwszIdentity) {
					UnstructuredParser subaccount(&status);
					CHECK_QSTATUS();
					Part::Field field = Part::FIELD_ERROR;
					status = msg.getField(L"X-QMAIL-SubAccount", &subaccount, &field);
					CHECK_QSTATUS();
					if (field == Part::FIELD_EXIST) {
						SubAccount* p = pAccount->getSubAccount(subaccount.getValue());
						if (p && wcscmp(p->getIdentity(), pwszIdentity) == 0)
							bSend = true;
					}
				}
				else {
					bSend = true;
				}
				
				if (bSend)
					listMessagePtr.push_back(MessagePtr(pmh));
			}
		}
	}
	if (listMessagePtr.empty())
		return QSTATUS_SUCCESS;
	
	FolderWait wait(this, item.getFolder(), &status);
	CHECK_QSTATUS();
	
	std::auto_ptr<Logger> pLogger;
	if (pSubAccount->isLog(Account::HOST_SEND)) {
		Logger* p = 0;
		status = pAccount->openLogger(Account::HOST_SEND, &p);
		CHECK_QSTATUS();
		pLogger.reset(p);
	}
	
	std::auto_ptr<SendSession> pSession;
	status = SendSessionFactory::getSession(
		item.getAccount()->getType(Account::HOST_SEND), &pSession);
	CHECK_QSTATUS();
	
	std::auto_ptr<SendSessionCallbackImpl> pCallback;
	status = newObject(pSyncManagerCallback, &pCallback);
	CHECK_QSTATUS();
	status = pSession->init(pDocument, pAccount, pSubAccount,
		pProfile_, pLogger.get(), pCallback.get());
	CHECK_QSTATUS();
	status = pSession->connect();
	CHECK_QSTATUS();
	
	status = pCallback->setRange(0, listMessagePtr.size());
	CHECK_QSTATUS();
	
	MessageHolderList l;
	status = STLWrapper<MessageHolderList>(l).resize(1);
	CHECK_QSTATUS();
	
	MessagePtrList::size_type m = 0;
	while (m < listMessagePtr.size()) {
		if (pCallback->isCanceled(false))
			return QSTATUS_SUCCESS;
		
		MessagePtrLock mpl(listMessagePtr[m]);
		if (mpl) {
			Message msg(&status);
			CHECK_QSTATUS();
			status = mpl->getMessage(
				Account::GETMESSAGEFLAG_ALL, 0, &msg);
			CHECK_QSTATUS();
			const WCHAR* pwszRemoveFields[] = {
				L"X-QMAIL-Account",
				L"X-QMAIL-SubAccount",
				L"X-QMAIL-Signature"
			};
			for (int n = 0; n < countof(pwszRemoveFields); ++n) {
				status = msg.removeField(pwszRemoveFields[n]);
				CHECK_QSTATUS();
			}
			status = pSession->sendMessage(&msg);
			CHECK_QSTATUS();
			
			l[0] = mpl;
//			status = pOutbox->setMessagesFlags(l,
//				MessageHolder::FLAG_SENT, MessageHolder::FLAG_SENT);
//			CHECK_QSTATUS();
//			status = pOutbox->copyMessages(l, pSentbox, true, 0);
//			CHECK_QSTATUS();
			status = pAccount->setMessagesFlags(l,
				MessageHolder::FLAG_SENT, MessageHolder::FLAG_SENT);
			CHECK_QSTATUS();
			status = pAccount->copyMessages(l, pSentbox, true, 0);
			CHECK_QSTATUS();
			
			status = pCallback->setPos(m + 1);
			CHECK_QSTATUS();
		}
		
		++m;
	}
	
	status = pSession->disconnect();
	CHECK_QSTATUS();
	
	return QSTATUS_SUCCESS;
}

QSTATUS qm::SyncManager::openReceiveSession(Document* pDocument,
	HWND hwnd, SyncManagerCallback* pSyncManagerCallback,
	const SyncItem& item, ReceiveSession** ppSession,
	ReceiveSessionCallback** ppCallback, Logger** ppLogger)
{
	assert(ppSession);
	assert(ppCallback);
	assert(ppLogger);
	
	DECLARE_QSTATUS();
	
	*ppSession = 0;
	
	Account* pAccount = item.getAccount();
	SubAccount* pSubAccount = item.getSubAccount();
	
	status = pSyncManagerCallback->setAccount(
		::GetCurrentThreadId(), pAccount, pSubAccount);
	CHECK_QSTATUS();
	
	std::auto_ptr<Logger> pLogger;
	if (pSubAccount->isLog(Account::HOST_RECEIVE)) {
		Logger* p = 0;
		status = pAccount->openLogger(Account::HOST_RECEIVE, &p);
		CHECK_QSTATUS();
		pLogger.reset(p);
	}
	
	std::auto_ptr<ReceiveSession> pSession;
	status = ReceiveSessionFactory::getSession(
		pAccount->getType(Account::HOST_RECEIVE), &pSession);
	CHECK_QSTATUS();
	
	std::auto_ptr<ReceiveSessionCallbackImpl> pCallback;
	status = newObject(pSyncManagerCallback, &pCallback);
	CHECK_QSTATUS();
	status = pSession->init(pDocument, pAccount, pSubAccount,
		hwnd, pProfile_, pLogger.get(), pCallback.get());
	CHECK_QSTATUS();
	
	status = pSession->connect();
	CHECK_QSTATUS();
	
	status = pSession->applyOfflineJobs();
	CHECK_QSTATUS();
	
	*ppSession = pSession.release();
	*ppCallback = pCallback.release();
	*ppLogger = pLogger.release();
	
	return QSTATUS_SUCCESS;
}


/****************************************************************************
 *
 * SyncManager::SyncThread
 *
 */

qm::SyncManager::SyncThread::SyncThread(SyncManager* pSyncManager,
	const SyncData* pData, QSTATUS* pstatus) :
	Thread(pstatus),
	pSyncManager_(pSyncManager),
	pSyncData_(pData),
	bWaitMode_(false)
{
	if (*pstatus != QSTATUS_SUCCESS)
		return;
}

qm::SyncManager::SyncThread::~SyncThread()
{
	delete pSyncData_;
}

void qm::SyncManager::SyncThread::setWaitMode()
{
	bWaitMode_ = true;
}

unsigned int qm::SyncManager::SyncThread::run()
{
	DECLARE_QSTATUS();
	
	InitThread init(0, &status);
	CHECK_QSTATUS_VALUE(-1);
	
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
	
	pSyncManager_->syncData(pSyncData_);
	
	Lock<CriticalSection> lock(pSyncManager_->cs_);
	
	if (!bWaitMode_) {
		SyncManager::ThreadList::iterator it = std::find(
			pSyncManager_->listThread_.begin(),
			pSyncManager_->listThread_.end(), this);
		assert(it != pSyncManager_->listThread_.end());
		
		pSyncManager_->listThread_.erase(it);
		delete this;
	}
	
	return 0;
}


/****************************************************************************
 *
 * SyncManager::ParallelSyncThread
 *
 */

qm::SyncManager::ParallelSyncThread::ParallelSyncThread(
	SyncManager* pSyncManager, const SyncData* pData,
	unsigned int nSlot, qs::QSTATUS* pstatus) :
	Thread(pstatus),
	pSyncManager_(pSyncManager),
	pSyncData_(pData),
	nSlot_(nSlot)
{
	if (*pstatus != QSTATUS_SUCCESS)
		return;
}

qm::SyncManager::ParallelSyncThread::~ParallelSyncThread()
{
}

unsigned int qm::SyncManager::ParallelSyncThread::run()
{
	DECLARE_QSTATUS();
	
	InitThread init(0, &status);
	CHECK_QSTATUS_VALUE(-1);
	
	pSyncManager_->syncSlotData(pSyncData_, nSlot_);
	
	return 0;
}


/****************************************************************************
 *
 * SyncManager::ReceiveSessionCallbackImpl
 *
 */

qm::SyncManager::ReceiveSessionCallbackImpl::ReceiveSessionCallbackImpl(
	SyncManagerCallback* pCallback) :
	pCallback_(pCallback)
{
	nId_ = ::GetCurrentThreadId();
}

qm::SyncManager::ReceiveSessionCallbackImpl::~ReceiveSessionCallbackImpl()
{
}

bool qm::SyncManager::ReceiveSessionCallbackImpl::isCanceled(bool bForce)
{
	return pCallback_->isCanceled(nId_, bForce);
}

QSTATUS qm::SyncManager::ReceiveSessionCallbackImpl::setPos(unsigned int n)
{
	return pCallback_->setPos(nId_, false, n);
}

QSTATUS qm::SyncManager::ReceiveSessionCallbackImpl::setRange(
	unsigned int nMin, unsigned int nMax)
{
	return pCallback_->setRange(nId_, false, nMin, nMax);
}

QSTATUS qm::SyncManager::ReceiveSessionCallbackImpl::setSubPos(unsigned int n)
{
	return pCallback_->setPos(nId_, true, n);
}

QSTATUS qm::SyncManager::ReceiveSessionCallbackImpl::setSubRange(
	unsigned int nMin, unsigned int nMax)
{
	return pCallback_->setRange(nId_, true, nMin, nMax);
}

QSTATUS qm::SyncManager::ReceiveSessionCallbackImpl::setMessage(
	const WCHAR* pwszMessage)
{
	return pCallback_->setMessage(nId_, pwszMessage);
}

QSTATUS qm::SyncManager::ReceiveSessionCallbackImpl::addError(
	const SessionErrorInfo& info)
{
	return pCallback_->addError(nId_, info);
}

QSTATUS qm::SyncManager::ReceiveSessionCallbackImpl::notifyNewMessage()
{
	return pCallback_->notifyNewMessage(nId_);
}


/****************************************************************************
 *
 * SyncManager::SendSessionCallbackImpl
 *
 */

qm::SyncManager::SendSessionCallbackImpl::SendSessionCallbackImpl(
	SyncManagerCallback* pCallback) :
	pCallback_(pCallback)
{
	nId_ = ::GetCurrentThreadId();
}

qm::SyncManager::SendSessionCallbackImpl::~SendSessionCallbackImpl()
{
}

bool qm::SyncManager::SendSessionCallbackImpl::isCanceled(bool bForce)
{
	return pCallback_->isCanceled(nId_, bForce);
}

QSTATUS qm::SyncManager::SendSessionCallbackImpl::setPos(unsigned int n)
{
	return pCallback_->setPos(nId_, false, n);
}

QSTATUS qm::SyncManager::SendSessionCallbackImpl::setRange(
	unsigned int nMin, unsigned int nMax)
{
	return pCallback_->setRange(nId_, false, nMin, nMax);
}

QSTATUS qm::SyncManager::SendSessionCallbackImpl::setSubPos(unsigned int n)
{
	return pCallback_->setPos(nId_, true, n);
}

QSTATUS qm::SyncManager::SendSessionCallbackImpl::setSubRange(
	unsigned int nMin, unsigned int nMax)
{
	return pCallback_->setRange(nId_, true, nMin, nMax);
}

QSTATUS qm::SyncManager::SendSessionCallbackImpl::setMessage(
	const WCHAR* pwszMessage)
{
	return pCallback_->setMessage(nId_, pwszMessage);
}

QSTATUS qm::SyncManager::SendSessionCallbackImpl::addError(
	const SessionErrorInfo& info)
{
	return pCallback_->addError(nId_, info);
}


/****************************************************************************
 *
 * SyncManager::RasConnectionCallbackImpl
 *
 */

qm::SyncManager::RasConnectionCallbackImpl::RasConnectionCallbackImpl(
	const SyncDialup* pDialup, SyncManagerCallback* pCallback) :
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

QSTATUS qm::SyncManager::RasConnectionCallbackImpl::preConnect(
	RASDIALPARAMS* prdp, bool* pbCancel)
{
	DECLARE_QSTATUS();
	
	if (pDialup_->getFlags() & SyncDialup::FLAG_SHOWDIALOG) {
		status = pCallback_->showDialupDialog(prdp, pbCancel);
		CHECK_QSTATUS();
	}
	
	return QSTATUS_SUCCESS;
}

QSTATUS qm::SyncManager::RasConnectionCallbackImpl::setMessage(
	const WCHAR* pwszMessage)
{
	return pCallback_->setMessage(-1, pwszMessage);
}

QSTATUS qm::SyncManager::RasConnectionCallbackImpl::error(const WCHAR* pwszMessage)
{
	DECLARE_QSTATUS();
	
	string_ptr<WSTRING> wstrMessage;
	status = loadString(Application::getApplication().getResourceHandle(),
		IDS_ERROR_DIALUP, &wstrMessage);
	CHECK_QSTATUS();
	
	SessionErrorInfo info(0, 0, 0, wstrMessage.get(), 0, &pwszMessage, 1);
	return pCallback_->addError(-1, info);
}


/****************************************************************************
 *
 * SyncManager::FolderWait
 *
 */

qm::SyncManager::FolderWait::FolderWait(SyncManager* pSyncManager,
	NormalFolder* pFolder, QSTATUS* pstatus) :
	pSyncManager_(pSyncManager),
	pFolder_(pFolder),
	pEvent_(0)
{
	DECLARE_QSTATUS();
	
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
			std::auto_ptr<Event> p;
			status = newQsObject(false, false, &p);
			CHECK_QSTATUS_SET(pstatus);
			status = STLWrapper<List>(l).push_back(
				std::make_pair(pFolder, p.get()));
			CHECK_QSTATUS_SET(pstatus);
			pEvent_ = p.release();
		}
		else {
			pEvent_ = (*it).second;
			bWait = true;
		}
	}
	if (bWait) {
		status = pEvent_->wait();
		CHECK_QSTATUS_SET(pstatus);
	}
}

qm::SyncManager::FolderWait::~FolderWait()
{
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
