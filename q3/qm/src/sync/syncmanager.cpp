/*
 * $Id$
 *
 * Copyright(C) 1998-2003 Satoshi Nakamura
 * All rights reserved.
 *
 */

#include <qmaccount.h>
#include <qmapplication.h>
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
#include "../ui/resource.h"

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
	nSlot_(nSlot)
{
}

qm::SyncItem::SyncItem(Account* pAccount,
	SubAccount* pSubAccount, unsigned int nSlot) :
	pAccount_(pAccount),
	pSubAccount_(pSubAccount),
	pFolder_(0),
	pFilterSet_(0),
	bSend_(true),
	nSlot_(nSlot)
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
	HWND hwnd, QSTATUS* pstatus) :
	pManager_(pManager),
	pDocument_(pDocument),
	hwnd_(hwnd),
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
	
	const Account::FolderList& l = pAccount->getFolders();
	Account::FolderList::const_iterator it = l.begin();
	while (it != l.end()) {
		Folder* pFolder = *it;
		if (pFolder->getType() == Folder::TYPE_NORMAL &&
			!pFolder->isFlag(Folder::FLAG_NOSELECT) &&
			pFolder->isFlag(Folder::FLAG_SYNCABLE)) {
			// TODO
			// Check if this is sync folder or not.
			
			bool bAdd = true;
			if (pFolderNamePattern) {
				string_ptr<WSTRING> wstrFolderName;
				status = pFolder->getFullName(&wstrFolderName);
				CHECK_QSTATUS();
				status = pFolderNamePattern->match(wstrFolderName.get(), &bAdd);
				CHECK_QSTATUS();
			}
			if (bAdd) {
				status = addFolder(pAccount, pSubAccount,
					static_cast<NormalFolder*>(pFolder), pwszFilterName);
				CHECK_QSTATUS();
			}
		}
		++it;
	}
	
	return QSTATUS_SUCCESS;
}

QSTATUS qm::SyncData::addSend(Account* pAccount, SubAccount* pSubAccount)
{
	return STLWrapper<ItemList>(listItem_).push_back(
		SyncItem(pAccount, pSubAccount, nSlot_));
}


/****************************************************************************
 *
 * SyncManager
 *
 */

qm::SyncManager::SyncManager(Profile* pProfile, QSTATUS* pstatus) :
	pProfile_(pProfile),
	pSyncFilterManager_(0),
	pWaitThread_(0),
	pEvent_(0)
{
	assert(pstatus);
	
	*pstatus = QSTATUS_SUCCESS;
	
	DECLARE_QSTATUS();
	
	status = newQsObject(&pSyncFilterManager_);
	CHECK_QSTATUS_SET(pstatus);
	
	status = STLWrapper<ThreadList>(listThread_).reserve(THREAD_MAX);
	CHECK_QSTATUS_SET(pstatus);
	
	status = newQsObject(&pEvent_);
	CHECK_QSTATUS_SET(pstatus);
}

qm::SyncManager::~SyncManager()
{
	if (pWaitThread_) {
		pWaitThread_->stop();
		delete pWaitThread_;
	}
	
	std::for_each(listSyncingFolder_.begin(), listSyncingFolder_.end(),
		unary_compose_f_gx(
			deleter<Event>(),
			std::select2nd<SyncingFolderList::value_type>()));
	delete pEvent_;
	
	delete pSyncFilterManager_;
}

QSTATUS qm::SyncManager::sync(SyncData* pData)
{
	DECLARE_QSTATUS();
	
	Lock<CriticalSection> lock(cs_);
	
	if (!pWaitThread_) {
		status = newQsObject(this, &pWaitThread_);
		CHECK_QSTATUS();
		status = pWaitThread_->start();
		CHECK_QSTATUS();
	}
	
	if (listThread_.size() >= THREAD_MAX)
		return QSTATUS_FAIL;
	
	std::auto_ptr<SyncThread> pThread;
	status = newQsObject(this, pData, &pThread);
	CHECK_QSTATUS();
	
	status = pThread->start();
	CHECK_QSTATUS();
	
	listThread_.push_back(pThread.release());
	
	pEvent_->set();
	
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

QSTATUS qm::SyncManager::syncData(const SyncData* pData)
{
	DECLARE_QSTATUS();
	
	SyncManagerCallback* pCallback = pData->getCallback();
	assert(pCallback);
	
	struct CallbackCaller
	{
		CallbackCaller(SyncManagerCallback* pCallback, QSTATUS* pstatus) :
			pCallback_(pCallback)
		{
			*pstatus = pCallback_->start();
		}
		
		~CallbackCaller()
		{
			pCallback_->end();
		}
		
		SyncManagerCallback* pCallback_;
	} caller(pCallback, &status);
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
	} disconnector(pRasConnection.get());
	
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
			unsigned int nId, QSTATUS* pstatus) :
			pCallback_(pCallback),
			nId_(nId)
		{
			*pstatus = pCallback_->startThread(nId_);
		}
		
		~CallbackCaller()
		{
			pCallback_->endThread(nId_);
		}
		
		SyncManagerCallback* pCallback_;
		unsigned int nId_;
	} caller(pCallback, ::GetCurrentThreadId(), &status);
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
			
			if (bSync) {
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
				status = syncFolder(pCallback, item, pReceiveSession.get());
				CHECK_QSTATUS();
			}
			
			if (item.isSend()) {
				status = send(pCallback, item);
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
	
	status = pSession->updateMessages();
	CHECK_QSTATUS();
	
	status = pSession->downloadMessages(item.getFilterSet());
	CHECK_QSTATUS();
	
	status = pSession->closeFolder();
	CHECK_QSTATUS();
	
	status = pFolder->saveMessageHolders();
	CHECK_QSTATUS();
	
	return QSTATUS_SUCCESS;
}

QSTATUS qm::SyncManager::send(SyncManagerCallback* pSyncManagerCallback,
	const SyncItem& item)
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
		Lock<Folder> lock(*pOutbox);
		
		for (unsigned int n = 0; n < pOutbox->getCount(); ++n) {
			MessageHolder* pmh = pOutbox->getMessage(n);
			if (!pmh->isFlag(MessageHolder::FLAG_DRAFT) &&
				!pmh->isFlag(MessageHolder::FLAG_DELETED)) {
				Message msg(&status);
				CHECK_QSTATUS();
				status = pmh->getMessage(Account::GETMESSAGEFLAG_HEADER,
					L"X-QMAIL-Identity", &msg);
				CHECK_QSTATUS();
				
				bool bSend = true;
				if (*pwszIdentity) {
					SimpleParser identity(0, &status);
					CHECK_QSTATUS();
					Part::Field field = Part::FIELD_ERROR;
					status = msg.getField(L"X-QMAIL-Identity", &identity, &field);
					CHECK_QSTATUS();
					bSend = field == Part::FIELD_EXIST &&
						wcscmp(identity.getValue(), pwszIdentity) == 0;
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
	status = pSession->init(pAccount, pSubAccount,
		pProfile_, pLogger.get(), pCallback.get());
	CHECK_QSTATUS();
	status = pSession->connect();
	CHECK_QSTATUS();
	
	status = pCallback->setRange(0, listMessagePtr.size());
	CHECK_QSTATUS();
	
	Folder::MessageHolderList l;
	status = STLWrapper<Folder::MessageHolderList>(l).resize(1);
	CHECK_QSTATUS();
	
	MessagePtrList::size_type m = 0;
	while (m < listMessagePtr.size()) {
		if (pCallback->isCanceled(false))
			return QSTATUS_SUCCESS;
		
		MessagePtrLock mpl(listMessagePtr[m]);
		if (mpl) {
			Message msg(&status);
			CHECK_QSTATUS();
			status = mpl->getMessage(Account::GETMESSAGEFLAG_ALL, 0, &msg);
			CHECK_QSTATUS();
			const WCHAR* pwszRemoveFields[] = {
				L"X-QMAIL-Account",
				L"X-QMAIL-Identity",
				L"X-QMAIL-Signature"
			};
			for (int n = 0; n < countof(pwszRemoveFields); ++n) {
				status = msg.removeField(pwszRemoveFields[n]);
				CHECK_QSTATUS();
			}
			status = pSession->sendMessage(&msg);
			CHECK_QSTATUS();
			
			l[0] = mpl;
			status = pOutbox->setMessagesFlags(l,
				MessageHolder::FLAG_SENT, MessageHolder::FLAG_SENT);
			CHECK_QSTATUS();
			status = pOutbox->copyMessages(l, pSentbox, true);
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
	pSyncData_(pData)
{
	if (*pstatus != QSTATUS_SUCCESS)
		return;
}

qm::SyncManager::SyncThread::~SyncThread()
{
	delete pSyncData_;
}

unsigned int qm::SyncManager::SyncThread::run()
{
	DECLARE_QSTATUS();
	
	InitThread init(&status);
	CHECK_QSTATUS_VALUE(-1);
	
	pSyncManager_->syncData(pSyncData_);
	
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
	
	InitThread init(&status);
	CHECK_QSTATUS_VALUE(-1);
	
	pSyncManager_->syncSlotData(pSyncData_, nSlot_);
	
	return 0;
}


/****************************************************************************
 *
 * SyncManager::WaitThread
 *
 */

qm::SyncManager::WaitThread::WaitThread(
	SyncManager* pSyncManager, QSTATUS* pstatus) :
	Thread(pstatus),
	pSyncManager_(pSyncManager),
	bStop_(false)
{
	if (*pstatus != QSTATUS_SUCCESS)
		return;
}

qm::SyncManager::WaitThread::~WaitThread()
{
}

QSTATUS qm::SyncManager::WaitThread::stop()
{
	DECLARE_QSTATUS();
	
	bStop_ = true;
	
	status = pSyncManager_->pEvent_->set();
	CHECK_QSTATUS();
	
	status = join();
	CHECK_QSTATUS();
	
	return QSTATUS_SUCCESS;
}

unsigned int qm::SyncManager::WaitThread::run()
{
	DECLARE_QSTATUS();
	
	InitThread init(&status);
	CHECK_QSTATUS_VALUE(-1);
	
	typedef SyncManager::ThreadList List;
	List& l = pSyncManager_->listThread_;
	
	while (true) {
		DWORD dwCount = 0;
		HANDLE handles[THREAD_MAX];
		{
			Lock<CriticalSection> lock(pSyncManager_->cs_);
			dwCount = l.size() + 1;
			handles[0] = pSyncManager_->pEvent_->getHandle();
			std::transform(l.begin(), l.end(), handles + 1, 
				std::mem_fun(SyncThread::getHandle));
		}
		
		HANDLE handle = 0;
		DWORD dw = ::WaitForMultipleObjects(
			dwCount, handles, FALSE, INFINITE);
		if (WAIT_OBJECT_0 <= dw && dw < WAIT_OBJECT_0 + dwCount) {
			handle = handles[dw - WAIT_OBJECT_0];
		}
		else if (WAIT_ABANDONED_0 <= dw && dw < WAIT_ABANDONED_0 + dwCount) {
			handle = handles[dw - WAIT_ABANDONED_0];
		}
		else {
			// TODO
		}
		
		if (handle == pSyncManager_->pEvent_->getHandle()) {
			if (bStop_) {
				// TODO
				// Discard all threads
				return 0;
			}
		}
		else {
			Lock<CriticalSection> lock(pSyncManager_->cs_);
			
			List::iterator it = l.begin();
			while (it != l.end() && (*it)->getHandle() != handle)
				++it;
			assert(it != l.end());
			
			SyncThread* pThread = *it;
			l.erase(it);
			delete pThread;
		}
	}
	
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
