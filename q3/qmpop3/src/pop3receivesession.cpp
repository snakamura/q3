/*
 * $Id$
 *
 * Copyright(C) 1998-2003 Satoshi Nakamura
 * All rights reserved.
 *
 */

#include <qmaccount.h>
#include <qmfolder.h>
#include <qmmessage.h>
#include <qmmessageholder.h>

#include <qsconv.h>
#include <qserror.h>
#include <qsnew.h>
#include <qsstream.h>

#include <algorithm>

#include "main.h"
#include "pop3.h"
#include "pop3receivesession.h"
#include "resourceinc.h"
#include "ui.h"
#include "uid.h"
#include "util.h"

#pragma warning(disable:4786)

using namespace qmpop3;
using namespace qm;
using namespace qs;


#define CHECK_QSTATUS_ERROR() \
	if (status != QSTATUS_SUCCESS) { \
		Util::reportError(pPop3_, pSessionCallback_, pAccount_, pSubAccount_); \
		return status; \
	} \


/****************************************************************************
 *
 * Pop3ReceiveSession
 *
 */

qmpop3::Pop3ReceiveSession::Pop3ReceiveSession(QSTATUS* pstatus) :
	pPop3_(0),
	pCallback_(0),
	pDocument_(0),
	pAccount_(0),
	pSubAccount_(0),
	pFolder_(0),
	hwnd_(0),
	pLogger_(0),
	pSessionCallback_(0),
	bReservedDownload_(false),
	bCacheAll_(false),
	nStart_(0),
	pUIDList_(0)
{
	assert(pstatus);
	
	*pstatus = QSTATUS_SUCCESS;
}

qmpop3::Pop3ReceiveSession::~Pop3ReceiveSession()
{
	delete pPop3_;
	delete pCallback_;
	delete pUIDList_;
	std::for_each(listUID_.begin(), listUID_.end(), string_free<WSTRING>());
}

QSTATUS qmpop3::Pop3ReceiveSession::init(Document* pDocument,
	Account* pAccount, SubAccount* pSubAccount, HWND hwnd,
	Profile* pProfile, Logger* pLogger, ReceiveSessionCallback* pCallback)
{
	assert(pDocument);
	assert(pAccount);
	assert(pSubAccount);
	assert(hwnd);
	assert(pProfile);
	assert(pCallback);
	
	DECLARE_QSTATUS();
	
	pDocument_ = pDocument;
	pAccount_ = pAccount;
	pSubAccount_ = pSubAccount;
	hwnd_ = hwnd;
	pProfile_ = pProfile;
	pLogger_ = pLogger;
	pSessionCallback_ = pCallback;
	
	status = newQsObject(pSubAccount_, pSessionCallback_, &pCallback_);
	CHECK_QSTATUS();
	
	return QSTATUS_SUCCESS;
}

QSTATUS qmpop3::Pop3ReceiveSession::connect()
{
	assert(!pPop3_);
	
	DECLARE_QSTATUS();
	
	Log log(pLogger_, L"qmpop3::Pop3ReceiveSession");
	status = log.debug(L"Connecting to the server...");
	CHECK_QSTATUS();
	
	Pop3::Option option = {
		pSubAccount_->getTimeout(),
		pCallback_,
		pCallback_,
		pLogger_
	};
	status = newQsObject(option, &pPop3_);
	CHECK_QSTATUS();
	
	int nApop = 0;
	status = pSubAccount_->getProperty(L"Pop3", L"Apop", 0, &nApop);
	CHECK_QSTATUS();
	status = pPop3_->connect(pSubAccount_->getHost(Account::HOST_RECEIVE),
		pSubAccount_->getPort(Account::HOST_RECEIVE),
		nApop != 0, pSubAccount_->isSsl(Account::HOST_RECEIVE));
	CHECK_QSTATUS_ERROR();
	
	status = log.debug(L"Connected to the server.");
	CHECK_QSTATUS();
	
	return QSTATUS_SUCCESS;
}

QSTATUS qmpop3::Pop3ReceiveSession::disconnect()
{
	assert(pPop3_);
	
	DECLARE_QSTATUS();
	
	Log log(pLogger_, L"qmpop3::Pop3ReceiveSession");
	status = log.debug(L"Disconnecting from the server...");
	CHECK_QSTATUS();
	
	status = pPop3_->disconnect();
	CHECK_QSTATUS_ERROR();
	
	status = log.debug(L"Disconnected from the server.");
	CHECK_QSTATUS();
	
	return QSTATUS_SUCCESS;
}

QSTATUS qmpop3::Pop3ReceiveSession::selectFolder(NormalFolder* pFolder)
{
	assert(pFolder);
	
	DECLARE_QSTATUS();
	
	status = prepare();
	CHECK_QSTATUS();
	
	status = downloadReservedMessages();
	CHECK_QSTATUS();
	
	pFolder_ = pFolder;
	
	return QSTATUS_SUCCESS;
}

QSTATUS qmpop3::Pop3ReceiveSession::closeFolder()
{
	assert(pFolder_);
	
	DECLARE_QSTATUS();
	
	pFolder_ = 0;
	
	if (pUIDList_) {
		status = saveUIDList(pUIDList_);
		CHECK_QSTATUS();
	}
	
	return QSTATUS_SUCCESS;
}

QSTATUS qmpop3::Pop3ReceiveSession::updateMessages()
{
	return QSTATUS_SUCCESS;
}

QSTATUS qmpop3::Pop3ReceiveSession::downloadMessages(
	const SyncFilterSet* pSyncFilterSet)
{
	DECLARE_QSTATUS();
	
	unsigned int nCount = pPop3_->getMessageCount();
	status = pCallback_->setMessage(IDS_DOWNLOADMESSAGES);
	CHECK_QSTATUS();
	status = pSessionCallback_->setRange(0, nCount);
	CHECK_QSTATUS();
	status = pSessionCallback_->setPos(nStart_);
	CHECK_QSTATUS();
	
	const WCHAR* pwszIdentity = pSubAccount_->getIdentity();
	UnstructuredParser subaccount(pSubAccount_->getName(), L"utf-8", &status);
	CHECK_QSTATUS();
	
	Time time(Time::getCurrentTime());
	UID::Date date = {
		time.wYear,
		time.wMonth,
		time.wDay
	};
	
	MacroVariableHolder globalVariable(&status);
	CHECK_QSTATUS();
	
	for (unsigned int n = nStart_; n < nCount; ++n) {
		if (pSessionCallback_->isCanceled(false))
			return QSTATUS_SUCCESS;
		status = pSessionCallback_->setPos(n + 1);
		CHECK_QSTATUS();
		
		unsigned int nSize = 0;
		if (bCacheAll_) {
			nSize = listSize_[n];
		}
		else {
			status = pPop3_->getMessageSize(n, &nSize);
			CHECK_QSTATUS_ERROR();
		}
		
		string_ptr<STRING> strMessage;
		Message msg(&status);
		CHECK_QSTATUS();
		State state = STATE_NONE;
		unsigned int nGetSize = 0;
		unsigned int nMaxLine = 0xffffffff;
		if (pSyncFilterSet) {
			const SyncFilter* pFilter = 0;
			Pop3SyncFilterCallback callback(pDocument_, pAccount_,
				pFolder_, &msg, nSize, hwnd_, pProfile_, &globalVariable,
				pPop3_, n, strMessage.getThis(), &state, &nGetSize);
			status = pSyncFilterSet->getFilter(&callback, &pFilter);
			CHECK_QSTATUS();
			if (pFilter) {
				const SyncFilterAction* pAction = pFilter->getAction();
				if (wcscmp(pAction->getName(), L"download") == 0) {
					const WCHAR* pwszLine = pAction->getParam(L"line");
					if (pwszLine) {
						WCHAR* pEnd = 0;
						long nLine = wcstol(pwszLine, &pEnd, 10);
						if (!*pEnd)
							nMaxLine = nLine;
					}
				}
				// TODO
				// Delete action?
			}
		}
		if (state != STATE_ALL && (state != STATE_HEADER || nMaxLine != 0)) {
			strMessage.reset(0);
			nGetSize = nSize;
			status = pPop3_->getMessage(n, nMaxLine, &strMessage, &nGetSize);
			CHECK_QSTATUS_ERROR();
			
			size_t nLen = static_cast<size_t>(-1);
			CHAR* p = strstr(strMessage.get(), "\r\n\r\n");
			if (p)
				nLen = p - strMessage.get() + 4;
			status = msg.create(strMessage.get(), nLen,
				Message::FLAG_HEADERONLY);
			CHECK_QSTATUS();
		}
		
		bool bPartial = nMaxLine != 0xffffffff && nSize > nGetSize;
		
		const WCHAR* pwszUID = 0;
		string_ptr<WSTRING> wstrUID;
		if (bCacheAll_) {
			pwszUID = listUID_[n];
		}
		else {
			status = pPop3_->getUid(n, &wstrUID);
			CHECK_QSTATUS_ERROR();
			pwszUID = wstrUID.get();
		}
		
		UnstructuredParser uid(pwszUID, L"utf-8", &status);
		CHECK_QSTATUS();
		status = msg.replaceField(L"X-UIDL", uid);
		CHECK_QSTATUS();
		
		if (*pwszIdentity) {
			status = msg.replaceField(L"X-QMAIL-SubAccount", subaccount);
			CHECK_QSTATUS();
		}
		else {
			status = msg.removeField(L"X-QMAIL-SubAccount");
			CHECK_QSTATUS();
		}
		
		bool bSelf = false;
		status = pSubAccount_->isSelf(msg, &bSelf);
		CHECK_QSTATUS();
		unsigned int nFlags = (bSelf ?
			MessageHolder::FLAG_SEEN | MessageHolder::FLAG_SENT : 0) |
			(bPartial ? MessageHolder::FLAG_HEADERONLY : 0);
		
		Lock<Folder> lock(*pFolder_);
		
		MessageHolder* pmh = 0;
		status = pAccount_->storeMessage(pFolder_, strMessage.get(),
			&msg, static_cast<unsigned int>(-1), nFlags, nSize, false, &pmh);
		CHECK_QSTATUS();
		
		unsigned int nUIDFlags = bPartial ? UID::FLAG_PARTIAL : UID::FLAG_NONE;
		std::auto_ptr<UID> pUID;
		status = newQsObject(pwszUID, nUIDFlags, date, &pUID);
		CHECK_QSTATUS();
		status = pUIDList_->add(pUID.get());
		CHECK_QSTATUS();
		pUID.release();
		
		status = pSessionCallback_->notifyNewMessage();
		CHECK_QSTATUS();
	}
	
	int nDeleteOnServer = 0;
	status = pSubAccount_->getProperty(
		L"Pop3", L"DeleteOnServer", 0, &nDeleteOnServer);
	CHECK_QSTATUS();
	int nDeleteBefore = 0;
	status = pSubAccount_->getProperty(
		L"Pop3", L"DeleteBefore", 0, &nDeleteBefore);
	CHECK_QSTATUS();
	
	// TODO
	// Treat messages reserved to be deleted
	if (nDeleteOnServer || nDeleteBefore != 0) {
		status = pCallback_->setMessage(IDS_DELETEMESSAGE);
		CHECK_QSTATUS();
		
		UIDList::IndexList listDelete;
		status = STLWrapper<UIDList::IndexList>(
			listDelete).reserve(pUIDList_->getCount());
		CHECK_QSTATUS();
		
		for (n = 0; n < pUIDList_->getCount(); ++n) {
			UID* pUID = pUIDList_->getUID(n);
			
			bool bDelete = false;
			if (nDeleteOnServer) {
				bDelete = !(pUID->getFlags() & UID::FLAG_PARTIAL);
			}
			else if (nDeleteBefore != 0) {
				const UID::Date& date = pUID->getDate();
				Time t(date.nYear_, date.nMonth_, 0, date.nDay_, 0, 0, 0, 0, 0);
				t.addDay(nDeleteBefore);
				bDelete = t < time;
			}
			
			if (bDelete)
				listDelete.push_back(n);
		}
		
		status = pSessionCallback_->setRange(0, listDelete.size());
		CHECK_QSTATUS();
		status = pSessionCallback_->setPos(0);
		CHECK_QSTATUS();
		
		for (n = 0; n < listDelete.size(); ++n) {
			status = pSessionCallback_->setPos(n + 1);
			CHECK_QSTATUS();
			
			status = pPop3_->deleteMessage(listDelete[n]);
			CHECK_QSTATUS_ERROR();
		}
		
		pUIDList_->remove(listDelete);
	}
	
	return QSTATUS_SUCCESS;
}

QSTATUS qmpop3::Pop3ReceiveSession::applyOfflineJobs()
{
	return QSTATUS_SUCCESS;
}

QSTATUS qmpop3::Pop3ReceiveSession::prepare()
{
	assert(!bReservedDownload_);
	assert(!bCacheAll_);
	assert(nStart_ == 0);
	assert(!pUIDList_);
	assert(listUID_.empty());
	assert(listSize_.empty());
	
	DECLARE_QSTATUS();
	
	std::auto_ptr<UIDList> pUIDList;
	status = loadUIDList(&pUIDList);
	CHECK_QSTATUS();
	
	status = pCallback_->setMessage(IDS_CHECKNEWMESSAGE);
	CHECK_QSTATUS();
	
	const Account::FolderList& listFolder = pAccount_->getFolders();
	Account::FolderList::const_iterator it = listFolder.begin();
	while (it != listFolder.end() && !bReservedDownload_) {
		Folder* pFolder = *it;
		if (pFolder->getType() == Folder::TYPE_NORMAL &&
			static_cast<NormalFolder*>(pFolder)->getDownloadCount() != 0)
			bReservedDownload_ = true;
		++it;
	}
	
	unsigned int nGetAll = 0;
	status = pSubAccount_->getProperty(L"Pop3", L"GetAll",
		20, reinterpret_cast<int*>(&nGetAll));
	CHECK_QSTATUS();
	
	unsigned int nCount = pPop3_->getMessageCount();
	unsigned int nUIDCount = pUIDList->getCount();
	if (nUIDCount == 0 ||
		nCount < nUIDCount ||
		(nCount != nUIDCount && nCount/(nCount - nUIDCount) > nGetAll) ||
		bReservedDownload_) {
		bCacheAll_ = true;
	}
	else {
		string_ptr<WSTRING> wstrUID;
		status = pPop3_->getUid(nUIDCount - 1, &wstrUID);
		CHECK_QSTATUS_ERROR();
		UID* pUID = pUIDList->getUID(nUIDCount - 1);
		if (wcscmp(pUID->getUID(), wstrUID.get()) == 0)
			nStart_ = nUIDCount;
		else
			bCacheAll_ = true;
	}
	
	std::auto_ptr<UIDList> pNewUIDList;
	
	if (bCacheAll_) {
		status = pPop3_->getUids(&listUID_);
		CHECK_QSTATUS_ERROR();
		
		status = pPop3_->getMessageSizes(&listSize_);
		CHECK_QSTATUS_ERROR();
		
		if (nUIDCount != 0 && listUID_.size() >= nUIDCount &&
			wcscmp(pUIDList->getUID(nUIDCount - 1)->getUID(),
				listUID_[nUIDCount - 1]) == 0) {
			nStart_ = nUIDCount;
		}
		else {
			status = newQsObject(&pNewUIDList);
			CHECK_QSTATUS();
			pNewUIDList->setModified(true);
			
			size_t nIndex = -1;
			Pop3::UidList::size_type n = 0;
			while (n < listUID_.size()) {
				const WCHAR* pwszUID = listUID_[n];
				nIndex = pUIDList->getIndex(pwszUID, nIndex + 1);
				if (nIndex == -1)
					break;
				
				std::auto_ptr<UID> pUID(pUIDList->remove(nIndex));
				status = pNewUIDList->add(pUID.get());
				CHECK_QSTATUS();
				pUID.release();
				
				++n;
			}
			nStart_ = n;
		}
	}
	
	if (pNewUIDList.get())
		pUIDList_ = pNewUIDList.release();
	else
		pUIDList_ = pUIDList.release();
	
	return QSTATUS_SUCCESS;
}

QSTATUS qmpop3::Pop3ReceiveSession::downloadReservedMessages()
{
	DECLARE_QSTATUS();
	
	if (bReservedDownload_) {
		assert(bCacheAll_);
		
		const Account::FolderList& l = pAccount_->getFolders();
		Account::FolderList::const_iterator it = l.begin();
		while (it != l.end()) {
			Folder* pFolder = *it;
			if (pFolder->getType() == Folder::TYPE_NORMAL) {
				status = downloadReservedMessages(
					static_cast<NormalFolder*>(pFolder));
				CHECK_QSTATUS();
			}
			++it;
		}
	}
	
	return QSTATUS_SUCCESS;
}

QSTATUS qmpop3::Pop3ReceiveSession::downloadReservedMessages(
	NormalFolder* pFolder)
{
	assert(pFolder);
	assert(bCacheAll_);
	
	DECLARE_QSTATUS();
	
	Time time(Time::getCurrentTime());
	
	typedef std::vector<MessagePtr> List;
	List l;
	
	{
		Lock<Folder> lock(*pFolder);
		
		status = pFolder->loadMessageHolders();
		CHECK_QSTATUS();
		
		status = STLWrapper<List>(l).reserve(pFolder->getDownloadCount());
		CHECK_QSTATUS();
		
		for (unsigned int n = 0; n < pFolder->getCount(); ++n) {
			MessageHolder* pmh = pFolder->getMessage(n);
			if (pmh->isFlag(MessageHolder::FLAG_DOWNLOAD) ||
				pmh->isFlag(MessageHolder::FLAG_DOWNLOADTEXT))
				l.push_back(MessagePtr(pmh));
		}
	}
	
	List::const_iterator it = l.begin();
	while (it != l.end()) {
		MessagePtrLock mpl(*it);
		if (mpl) {
			Message msg(&status);
			CHECK_QSTATUS();
			status = mpl->getMessage(Account::GETMESSAGEFLAG_HEADER, 0, &msg);
			CHECK_QSTATUS();
			UnstructuredParser uidl(&status);
			CHECK_QSTATUS();
			Part::Field f;
			status = msg.getField(L"X-UIDL", &uidl, &f);
			CHECK_QSTATUS();
			if (f == Part::FIELD_EXIST) {
				size_t nIndex = pUIDList_->getIndex(uidl.getValue());
				if (nIndex != -1) {
					string_ptr<STRING> strMessage;
					unsigned int nSize = listSize_[nIndex];
					status = pPop3_->getMessage(nIndex,
						0xffffffff, &strMessage, &nSize);
					CHECK_QSTATUS_ERROR();
					status = pAccount_->updateMessage(mpl, strMessage.get());
					CHECK_QSTATUS();
					
					UID* pUID = pUIDList_->getUID(nIndex);
					pUID->update(UID::FLAG_NONE,
						time.wYear, time.wMonth, time.wDay);
				}
			}
			mpl->setFlags(0,
				MessageHolder::FLAG_SEEN |
				MessageHolder::FLAG_DOWNLOAD |
				MessageHolder::FLAG_DOWNLOADTEXT |
				MessageHolder::FLAG_PARTIAL_MASK);
		}
		++it;
	}
	
	return QSTATUS_SUCCESS;
}

QSTATUS qmpop3::Pop3ReceiveSession::loadUIDList(
	std::auto_ptr<UIDList>* papUIDList) const
{
	DECLARE_QSTATUS();
	
	const ConcatW c[] = {
		{ pAccount_->getPath(),			-1	},
		{ L"\\",						-1	},
		{ pSubAccount_->getIdentity(),	-1	},
		{ L".uidl",						-1	}
	};
	string_ptr<WSTRING> wstrPath(concat(c, countof(c)));
	if (!wstrPath.get())
		return QSTATUS_OUTOFMEMORY;
	
	std::auto_ptr<UIDList> pUIDList;
	status = newQsObject(&pUIDList);
	CHECK_QSTATUS();
	status = pUIDList->load(wstrPath.get());
	CHECK_QSTATUS();
	
	papUIDList->reset(pUIDList.release());
	
	return QSTATUS_SUCCESS;
}

QSTATUS qmpop3::Pop3ReceiveSession::saveUIDList(const UIDList* pUIDList) const
{
	assert(pUIDList);
	
	DECLARE_QSTATUS();
	
	if (pUIDList->isModified()) {
		const ConcatW c[] = {
			{ pAccount_->getPath(),			-1	},
			{ L"\\",						-1	},
			{ pSubAccount_->getIdentity(),	-1	},
			{ L".uidl",						-1	}
		};
		string_ptr<WSTRING> wstrPath(concat(c, countof(c)));
		if (!wstrPath.get())
			return QSTATUS_OUTOFMEMORY;
		
		status = pUIDList->save(wstrPath.get());
		CHECK_QSTATUS();
	}
	
	return QSTATUS_SUCCESS;
}


/****************************************************************************
 *
 * Pop3ReceiveSession::CallbackImpl
 *
 */

qmpop3::Pop3ReceiveSession::CallbackImpl::CallbackImpl(SubAccount* pSubAccount,
	ReceiveSessionCallback* pSessionCallback, qs::QSTATUS* pstatus) :
	pSubAccount_(pSubAccount),
	pSessionCallback_(pSessionCallback)
{
	assert(pstatus);
	*pstatus = QSTATUS_SUCCESS;
}

qmpop3::Pop3ReceiveSession::CallbackImpl::~CallbackImpl()
{
}

QSTATUS qmpop3::Pop3ReceiveSession::CallbackImpl::setMessage(UINT nId)
{
	DECLARE_QSTATUS();
	
	string_ptr<WSTRING> wstrMessage;
	status = loadString(getResourceHandle(), nId, &wstrMessage);
	CHECK_QSTATUS();
	
	return pSessionCallback_->setMessage(wstrMessage.get());
}

bool qmpop3::Pop3ReceiveSession::CallbackImpl::isCanceled(bool bForce) const
{
	return pSessionCallback_->isCanceled(bForce);
}

QSTATUS qmpop3::Pop3ReceiveSession::CallbackImpl::initialize()
{
	return setMessage(IDS_INITIALIZE);
}

QSTATUS qmpop3::Pop3ReceiveSession::CallbackImpl::lookup()
{
	return setMessage(IDS_LOOKUP);
}

QSTATUS qmpop3::Pop3ReceiveSession::CallbackImpl::connecting()
{
	return setMessage(IDS_CONNECTING);
}

QSTATUS qmpop3::Pop3ReceiveSession::CallbackImpl::connected()
{
	return setMessage(IDS_CONNECTED);
}

QSTATUS qmpop3::Pop3ReceiveSession::CallbackImpl::getUserInfo(
	WSTRING* pwstrUserName, WSTRING* pwstrPassword)
{
	assert(pwstrUserName);
	assert(pwstrPassword);
	
	DECLARE_QSTATUS();
	
	string_ptr<WSTRING> wstrUserName(
		allocWString(pSubAccount_->getUserName(Account::HOST_RECEIVE)));
	if (!wstrUserName.get())
		return QSTATUS_OUTOFMEMORY;
	string_ptr<WSTRING> wstrPassword(
		allocWString(pSubAccount_->getPassword(Account::HOST_RECEIVE)));
	if (!wstrPassword.get())
		return QSTATUS_OUTOFMEMORY;
	
	*pwstrUserName = wstrUserName.release();
	*pwstrPassword = wstrPassword.release();
	
	return QSTATUS_SUCCESS;
}

QSTATUS qmpop3::Pop3ReceiveSession::CallbackImpl::setPassword(
	const WCHAR* pwszPassword)
{
	// TODO
	return QSTATUS_SUCCESS;
}

QSTATUS qmpop3::Pop3ReceiveSession::CallbackImpl::authenticating()
{
	return setMessage(IDS_AUTHENTICATING);
}

QSTATUS qmpop3::Pop3ReceiveSession::CallbackImpl::setRange(
	unsigned int nMin, unsigned int nMax)
{
	return pSessionCallback_->setSubRange(nMin, nMax);
}

QSTATUS qmpop3::Pop3ReceiveSession::CallbackImpl::setPos(
	unsigned int nPos)
{
	return pSessionCallback_->setSubPos(nPos);
}


/****************************************************************************
 *
 * Pop3ReceiveSessionUI
 *
 */

qmpop3::Pop3ReceiveSessionUI::Pop3ReceiveSessionUI(QSTATUS* pstatus)
{
}

qmpop3::Pop3ReceiveSessionUI::~Pop3ReceiveSessionUI()
{
}

const WCHAR* qmpop3::Pop3ReceiveSessionUI::getClass()
{
	return L"mail";
}

QSTATUS qmpop3::Pop3ReceiveSessionUI::getDisplayName(WSTRING* pwstrName)
{
	assert(pwstrName);
	return loadString(getResourceHandle(), IDS_POP3, pwstrName);
}

short qmpop3::Pop3ReceiveSessionUI::getDefaultPort()
{
	return 110;
}

QSTATUS qmpop3::Pop3ReceiveSessionUI::createPropertyPage(
	SubAccount* pSubAccount, PropertyPage** ppPage)
{
	assert(ppPage);
	
	DECLARE_QSTATUS();
	
	*ppPage = 0;
	
	std::auto_ptr<ReceivePage> pPage;
	status = newQsObject(pSubAccount, &pPage);
	CHECK_QSTATUS();
	
	*ppPage = pPage.release();
	
	return QSTATUS_SUCCESS;
}


/****************************************************************************
 *
 * Pop3ReceiveSessionFactory
 *
 */

Pop3ReceiveSessionFactory qmpop3::Pop3ReceiveSessionFactory::factory__;

qmpop3::Pop3ReceiveSessionFactory::Pop3ReceiveSessionFactory()
{
	regist(L"pop3", this);
}

qmpop3::Pop3ReceiveSessionFactory::~Pop3ReceiveSessionFactory()
{
	unregist(L"pop3");
}

QSTATUS qmpop3::Pop3ReceiveSessionFactory::createSession(
	ReceiveSession** ppReceiveSession)
{
	assert(ppReceiveSession);
	
	DECLARE_QSTATUS();
	
	Pop3ReceiveSession* pSession = 0;
	status = newQsObject(&pSession);
	CHECK_QSTATUS();
	
	*ppReceiveSession = pSession;
	
	return QSTATUS_SUCCESS;
}

QSTATUS qmpop3::Pop3ReceiveSessionFactory::createUI(ReceiveSessionUI** ppUI)
{
	assert(ppUI);
	
	DECLARE_QSTATUS();
	
	Pop3ReceiveSessionUI* pUI = 0;
	status = newQsObject(&pUI);
	CHECK_QSTATUS();
	
	*ppUI = pUI;
	
	return QSTATUS_SUCCESS;
}


/****************************************************************************
 *
 * Pop3SyncFilterCallback
 *
 */

qmpop3::Pop3SyncFilterCallback::Pop3SyncFilterCallback(
	Document* pDocument, Account* pAccount, NormalFolder* pFolder,
	Message* pMessage, unsigned int nSize, HWND hwnd, Profile* pProfile,
	MacroVariableHolder* pGlobalVariable, Pop3* pPop3,
	unsigned int nMessage, qs::string_ptr<qs::STRING>* pstrMessage,
	Pop3ReceiveSession::State* pState, unsigned int* pnGetSize) :
	pDocument_(pDocument),
	pAccount_(pAccount),
	pFolder_(pFolder),
	pMessage_(pMessage),
	nSize_(nSize),
	hwnd_(hwnd),
	pProfile_(pProfile),
	pGlobalVariable_(pGlobalVariable),
	pPop3_(pPop3),
	nMessage_(nMessage),
	pstrMessage_(pstrMessage),
	pState_(pState),
	pnGetSize_(pnGetSize),
	pmh_(0)
{
}

qmpop3::Pop3SyncFilterCallback::~Pop3SyncFilterCallback()
{
	delete pmh_;
}

QSTATUS qmpop3::Pop3SyncFilterCallback::getMessage(unsigned int nFlag)
{
	DECLARE_QSTATUS();
	
	bool bDownload = false;
	unsigned int nMaxLine = 0xffffffff;
	switch (nFlag & Account::GETMESSAGEFLAG_METHOD_MASK) {
	case Account::GETMESSAGEFLAG_ALL:
	case Account::GETMESSAGEFLAG_TEXT:
	case Account::GETMESSAGEFLAG_HTML:
		bDownload = *pState_ != Pop3ReceiveSession::STATE_ALL;
		break;
	case Account::GETMESSAGEFLAG_HEADER:
		bDownload = *pState_ == Pop3ReceiveSession::STATE_NONE;
		nMaxLine = 0;
		break;
	case Account::GETMESSAGEFLAG_POSSIBLE:
		break;
	default:
		assert(false);
		return QSTATUS_FAIL;
	}
	
	if (bDownload) {
		string_ptr<STRING>& str = *pstrMessage_;
		
		str.reset(0);
		*pnGetSize_ = nSize_;
		status = pPop3_->getMessage(nMessage_, nMaxLine, &str, pnGetSize_);
		CHECK_QSTATUS();
		
		size_t nLen = static_cast<size_t>(-1);
		CHAR* p = strstr(str.get(), "\r\n\r\n");
		if (p)
			nLen = p - str.get() + 4;
		status = pMessage_->create(str.get(), nLen, Message::FLAG_HEADERONLY);
		CHECK_QSTATUS();
	}
	
	return QSTATUS_SUCCESS;
}

const NormalFolder* qmpop3::Pop3SyncFilterCallback::getFolder()
{
	return pFolder_;
}

QSTATUS qmpop3::Pop3SyncFilterCallback::getMacroContext(
	MacroContext** ppContext)
{
	assert(ppContext);
	
	DECLARE_QSTATUS();
	
	if (!pmh_) {
		status = newQsObject(this, pFolder_, pMessage_, nSize_, &pmh_);
		CHECK_QSTATUS();
	}
	MacroContext::Init init = {
		pmh_,
		pMessage_,
		pAccount_,
		pDocument_,
		hwnd_,
		pProfile_,
		false,
		0,
		pGlobalVariable_
	};
	std::auto_ptr<MacroContext> pContext;
	status = newQsObject(init, &pContext);
	CHECK_QSTATUS();
	
	*ppContext = pContext.release();
	
	return QSTATUS_SUCCESS;
}


/****************************************************************************
 *
 * Pop3MessageHolder
 *
 */

qmpop3::Pop3MessageHolder::Pop3MessageHolder(
	Pop3SyncFilterCallback* pCallback, NormalFolder* pFolder,
	Message* pMessage, unsigned int nSize, QSTATUS* pstatus) :
	AbstractMessageHolder(pFolder, pMessage, -1, nSize, nSize, pstatus),
	pCallback_(pCallback)
{
	assert(pstatus);
	*pstatus = QSTATUS_SUCCESS;
}

qmpop3::Pop3MessageHolder::~Pop3MessageHolder()
{
}

QSTATUS qmpop3::Pop3MessageHolder::getFrom(WSTRING* pwstrFrom) const
{
	DECLARE_QSTATUS();
	
	status = getMessage(Account::GETMESSAGEFLAG_HEADER);
	CHECK_QSTATUS();
	
	return AbstractMessageHolder::getFrom(pwstrFrom);
}

QSTATUS qmpop3::Pop3MessageHolder::getTo(WSTRING* pwstrTo) const
{
	DECLARE_QSTATUS();
	
	status = getMessage(Account::GETMESSAGEFLAG_HEADER);
	CHECK_QSTATUS();
	
	return AbstractMessageHolder::getTo(pwstrTo);
}

QSTATUS qmpop3::Pop3MessageHolder::getFromTo(WSTRING* pwstrFromTo) const
{
	return getFrom(pwstrFromTo);
}

QSTATUS qmpop3::Pop3MessageHolder::getSubject(WSTRING* pwstrSubject) const
{
	DECLARE_QSTATUS();
	
	status = getMessage(Account::GETMESSAGEFLAG_HEADER);
	CHECK_QSTATUS();
	
	return AbstractMessageHolder::getSubject(pwstrSubject);
}

QSTATUS qmpop3::Pop3MessageHolder::getDate(Time* pTime) const
{
	DECLARE_QSTATUS();
	
	status = getMessage(Account::GETMESSAGEFLAG_HEADER);
	CHECK_QSTATUS();
	
	return AbstractMessageHolder::getDate(pTime);
}

QSTATUS qmpop3::Pop3MessageHolder::getMessage(unsigned int nFlags,
	const WCHAR* pwszField, Message* pMessage)
{
	assert(pMessage == AbstractMessageHolder::getMessage());
	return getMessage(nFlags);
}

QSTATUS qmpop3::Pop3MessageHolder::getMessage(unsigned int nFlags) const
{
	return pCallback_->getMessage(nFlags);
}
