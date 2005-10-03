/*
 * $Id$
 *
 * Copyright(C) 1998-2005 Satoshi Nakamura
 * All rights reserved.
 *
 */

#pragma warning(disable:4786)

#include <qmaccount.h>
#include <qmfolder.h>
#include <qmmessage.h>
#include <qmmessageholder.h>
#include <qmrule.h>
#include <qmsecurity.h>

#include <qsconv.h>
#include <qsstream.h>

#include <algorithm>

#include "main.h"
#include "pop3.h"
#include "pop3error.h"
#include "pop3receivesession.h"
#include "resourceinc.h"
#include "ui.h"
#include "uid.h"
#include "util.h"

using namespace qmpop3;
using namespace qm;
using namespace qs;


#define HANDLE_ERROR() \
	do { \
		Util::reportError(pPop3_.get(), pSessionCallback_, \
			pAccount_, pSubAccount_, pFolder_, 0); \
		return false; \
	} while (false) \


/****************************************************************************
 *
 * Pop3ReceiveSession
 *
 */

qmpop3::Pop3ReceiveSession::Pop3ReceiveSession() :
	pDocument_(0),
	pAccount_(0),
	pSubAccount_(0),
	pFolder_(0),
	pLogger_(0),
	pSessionCallback_(0),
	bReservedDownload_(false),
	bCacheAll_(false),
	nStart_(0)
{
}

qmpop3::Pop3ReceiveSession::~Pop3ReceiveSession()
{
	std::for_each(listUID_.begin(), listUID_.end(), string_free<WSTRING>());
}

bool qmpop3::Pop3ReceiveSession::init(Document* pDocument,
									  Account* pAccount,
									  SubAccount* pSubAccount,
									  Profile* pProfile,
									  Logger* pLogger,
									  ReceiveSessionCallback* pCallback)
{
	assert(pDocument);
	assert(pAccount);
	assert(pSubAccount);
	assert(pProfile);
	assert(pCallback);
	
	pDocument_ = pDocument;
	pAccount_ = pAccount;
	pSubAccount_ = pSubAccount;
	pProfile_ = pProfile;
	pLogger_ = pLogger;
	pSessionCallback_ = pCallback;
	pCallback_.reset(new CallbackImpl(pSubAccount_,
		pDocument_->getSecurity(), pSessionCallback_));
	
	return true;
}

void qmpop3::Pop3ReceiveSession::term()
{
}

bool qmpop3::Pop3ReceiveSession::connect()
{
	assert(!pPop3_.get());
	
	Log log(pLogger_, L"qmpop3::Pop3ReceiveSession");
	log.debug(L"Connecting to the server...");
	
	pPop3_.reset(new Pop3(pSubAccount_->getTimeout(), pCallback_.get(),
		pCallback_.get(), pCallback_.get(), pLogger_));
	
	bool bApop = pSubAccount_->getProperty(L"Pop3", L"Apop", 0) != 0;
	Pop3::Secure secure = Util::getSecure(pSubAccount_);
	if (!pPop3_->connect(pSubAccount_->getHost(Account::HOST_RECEIVE),
		pSubAccount_->getPort(Account::HOST_RECEIVE), bApop, secure))
		HANDLE_ERROR();
	
	log.debug(L"Connected to the server.");
	
	return true;
}

void qmpop3::Pop3ReceiveSession::disconnect()
{
	assert(pPop3_.get());
	
	Log log(pLogger_, L"qmpop3::Pop3ReceiveSession");
	log.debug(L"Disconnecting from the server...");
	
	pPop3_->disconnect();
	
	log.debug(L"Disconnected from the server.");
}

bool qmpop3::Pop3ReceiveSession::isConnected()
{
	return true;
}

bool qmpop3::Pop3ReceiveSession::selectFolder(NormalFolder* pFolder,
											  unsigned int nFlags)
{
	assert(pFolder);
	assert(nFlags == 0);
	
	if (!prepare() || !downloadReservedMessages())
		return false;
	
	pFolder_ = pFolder;
	
	return true;
}

bool qmpop3::Pop3ReceiveSession::closeFolder()
{
	assert(pFolder_);
	
	pFolder_ = 0;
	
	return true;
}

bool qmpop3::Pop3ReceiveSession::updateMessages()
{
	return true;
}

bool qmpop3::Pop3ReceiveSession::downloadMessages(const SyncFilterSet* pSyncFilterSet)
{
	UIDSaver saver(this, pLogger_, pUIDList_.get());
	
	unsigned int nCount = pPop3_->getMessageCount();
	
	pCallback_->setMessage(IDS_DOWNLOADMESSAGES);
	pSessionCallback_->setRange(0, nCount);
	pSessionCallback_->setPos(nStart_);
	
	bool bHandleStatus = pSubAccount_->getProperty(L"Pop3", L"HandleStatus", 0) != 0;
	bool bSkipDuplicatedUID = pSubAccount_->getProperty(L"Pop3", L"SkipDuplicatedUID", 0) != 0;
	
	Time time(Time::getCurrentTime());
	UID::Date date = {
		time.wYear,
		time.wMonth,
		time.wDay
	};
	
	MacroVariableHolder globalVariable;
	
	DeleteList listDelete;
	
	MessagePtrList listDownloaded;
	
	for (unsigned int n = nStart_; n < nCount; ++n) {
		if (pSessionCallback_->isCanceled(false))
			return true;
		pSessionCallback_->setPos(n + 1);
		
		const WCHAR* pwszUID = 0;
		wstring_ptr wstrUID;
		unsigned int nSize = 0;
		if (bCacheAll_) {
			pwszUID = listUID_[n];
			nSize = listSize_[n];
		}
		else {
			if (!pPop3_->getUid(n, &wstrUID))
				HANDLE_ERROR();
			pwszUID = wstrUID.get();
			if (!pPop3_->getMessageSize(n, &nSize))
				HANDLE_ERROR();
		}
		
		if (bSkipDuplicatedUID && pOldUIDList_.get()) {
			unsigned int nIndex = pOldUIDList_->getIndex(pwszUID);
			if (nIndex != -1) {
				std::auto_ptr<UID> pUID(pOldUIDList_->remove(nIndex));
				pUIDList_->add(pUID);
				continue;
			}
		}
		
		xstring_size_ptr strMessage;
		Message msg;
		State state = STATE_NONE;
		unsigned int nMaxLine = 0xffffffff;
		bool bIgnore = false;
		if (pSyncFilterSet) {
			Pop3SyncFilterCallback callback(pDocument_, pAccount_,
				pFolder_, &msg, nSize, pProfile_, &globalVariable,
				pPop3_.get(), n, &strMessage, &state);
			const SyncFilter* pFilter = pSyncFilterSet->getFilter(&callback);
			if (pFilter) {
				const SyncFilter::ActionList& listAction = pFilter->getActions();
				for (SyncFilter::ActionList::const_iterator it = listAction.begin(); it != listAction.end(); ++it) {
					const SyncFilterAction* pAction = *it;
					const WCHAR* pwszName = pAction->getName();
					if (wcscmp(pwszName, L"download") == 0) {
						const WCHAR* pwszLine = pAction->getParam(L"line");
						if (pwszLine) {
							WCHAR* pEnd = 0;
							long nLine = wcstol(pwszLine, &pEnd, 10);
							if (!*pEnd)
								nMaxLine = nLine;
						}
					}
					else if (wcscmp(pwszName, L"ignore") == 0) {
						bIgnore = true;
					}
					else if (wcscmp(pwszName, L"delete") == 0) {
						listDelete.add(n);
					}
				}
			}
		}
		if (!bIgnore && state != STATE_ALL && (state != STATE_HEADER || nMaxLine != 0)) {
			strMessage.reset(0, -1);
			if (!pPop3_->getMessage(n, nMaxLine, &strMessage, nSize))
				HANDLE_ERROR();
			
			if (!msg.createHeader(strMessage.get(), strMessage.size()))
				return false;
		}
		
		bool bPartial = bIgnore || (nMaxLine != 0xffffffff && nSize > strMessage.size());
		
		if (!bIgnore) {
			if (!setUid(&msg, pwszUID))
				return false;
			if (!setSubAccount(&msg, pSubAccount_))
				return false;
			
			unsigned int nFlags = (bPartial ? MessageHolder::FLAG_HEADERONLY : 0);
			
			if (bHandleStatus) {
				UnstructuredParser status;
				if (msg.getField(L"Status", &status) == Part::FIELD_EXIST &&
					wcscmp(status.getValue(), L"RO") == 0)
					nFlags |= MessageHolder::FLAG_SEEN;
			}
			
			if (pSubAccount_->isSelf(msg))
				nFlags |= MessageHolder::FLAG_SEEN | MessageHolder::FLAG_SENT;
			
			{
				Lock<Account> lock(*pAccount_);
				
				MessageHolder* pmh = pAccount_->storeMessage(pFolder_, strMessage.get(),
					strMessage.size(), &msg, -1, nFlags, 0, nSize, false);
				if (!pmh)
					return false;
				
				listDownloaded.push_back(MessagePtr(pmh));
			}
		}
		
		unsigned int nUIDFlags = bPartial ? UID::FLAG_PARTIAL : UID::FLAG_NONE;
		std::auto_ptr<UID> pUID(new UID(pwszUID, nUIDFlags, date));
		pUIDList_->add(pUID);
	}
	
	const Account::FolderList& listFolder = pAccount_->getFolders();
	for (Account::FolderList::const_iterator it = listFolder.begin(); it != listFolder.end(); ++it) {
		if ((*it)->getType() == Folder::TYPE_NORMAL) {
			NormalFolder* pFolder = static_cast<NormalFolder*>(*it);
			Lock<Account> lock(*pAccount_);
			if (pFolder->getDeletedCount() != 0) {
				if (!pFolder->loadMessageHolders())
					return false;
				
				for (unsigned int n = 0; n < pFolder->getCount(); ++n) {
					MessageHolder* pmh = pFolder->getMessage(n);
					if (pmh->isFlag(MessageHolder::FLAG_DELETED)) {
						Message msg;
						if (!pmh->getMessage(Account::GETMESSAGEFLAG_HEADER,
							0, SECURITYMODE_NONE, &msg))
							return false;
						
						if (isSameIdentity(msg, pSubAccount_)) {
							size_t nIndex = -1;
							UnstructuredParser uid;
							if (msg.getField(L"X-UIDL", &uid) == Part::FIELD_EXIST) {
								nIndex = pUIDList_->getIndex(uid.getValue());
								if (nIndex != -1)
									listDelete.add(nIndex, MessagePtr(pmh));
							}
							if (nIndex == -1)
								pmh->setFlags(0, MessageHolder::FLAG_DELETED);
						}
					}
				}
			}
		}
	}
	
	bool bDeleteOnServer = pSubAccount_->getProperty(L"Pop3", L"DeleteOnServer", 0) != 0;
	int nDeleteBefore = pSubAccount_->getProperty(L"Pop3", L"DeleteBefore", 0);
	
	if (bDeleteOnServer || nDeleteBefore != 0) {
		for (unsigned int n = 0; n < pUIDList_->getCount(); ++n) {
			UID* pUID = pUIDList_->getUID(n);
			
			bool bDelete = false;
			if (bDeleteOnServer) {
				bDelete = !(pUID->getFlags() & UID::FLAG_PARTIAL);
			}
			else if (nDeleteBefore != 0) {
				const UID::Date& date = pUID->getDate();
				Time t(date.nYear_, date.nMonth_, 0, date.nDay_, 0, 0, 0, 0, 0);
				t.addDay(nDeleteBefore);
				bDelete = t < time;
			}
			
			if (bDelete)
				listDelete.add(n);
		}
	}
	
	const DeleteList::List& l = listDelete.getList();
	if (!l.empty()) {
		UIDList::IndexList listIndex;
		listIndex.reserve(l.size());
		for (DeleteList::List::size_type m = 0; m < l.size(); ++m) {
			if (l[m].first)
				listIndex.push_back(static_cast<unsigned int>(m));
		}
		
		if (listIndex.size()) {
			bool bDeleteLocal = pSubAccount_->getProperty(L"Pop3", L"DeleteLocal", 0) != 0;
			
			pCallback_->setMessage(IDS_DELETEMESSAGE);
			pSessionCallback_->setRange(0, listIndex.size());
			pSessionCallback_->setPos(0);
			
			int nPos = 0;
			for (DeleteList::List::size_type n = 0; n < l.size(); ++n) {
				if (l[n].first) {
					pSessionCallback_->setPos(++nPos);
					
					if (!pPop3_->deleteMessage(static_cast<unsigned int>(n)))
						HANDLE_ERROR();
					
					MessagePtrLock mpl(l[n].second);
					if (mpl) {
						mpl->setFlags(0, MessageHolder::FLAG_DELETED);
						if (bDeleteLocal)
							pAccount_->removeMessages(MessageHolderList(1, mpl),
								pFolder_, false, 0, 0);
					}
				}
			}
			
			pUIDList_->remove(listIndex);
		}
	}
	
	bool bApplyRules = pSubAccount_->isAutoApplyRules();
	bool bJunkFilter = pSubAccount_->isJunkFilterEnabled();
	if (bApplyRules || bJunkFilter) {
		if (!applyRules(&listDownloaded, bJunkFilter, !bApplyRules))
			Util::reportError(0, pSessionCallback_, pAccount_,
				pSubAccount_, pFolder_, POP3ERROR_APPLYRULES);
	}
	for (MessagePtrList::const_iterator it = listDownloaded.begin(); it != listDownloaded.end(); ++it) {
		bool bNotify = false;
		{
			MessagePtrLock mpl(*it);
			bNotify = mpl && !pAccount_->isSeen(mpl);
		}
		if (bNotify)
			pSessionCallback_->notifyNewMessage(*it);
	}
	
	return true;
}

bool qmpop3::Pop3ReceiveSession::applyOfflineJobs()
{
	return true;
}

bool qmpop3::Pop3ReceiveSession::prepare()
{
	assert(!bReservedDownload_);
	assert(!bCacheAll_);
	assert(nStart_ == 0);
	assert(!pUIDList_.get());
	assert(!pOldUIDList_.get());
	assert(listUID_.empty());
	assert(listSize_.empty());
	
	std::auto_ptr<UIDList> pUIDList(loadUIDList());
	if (!pUIDList.get())
		return false;
	
	pCallback_->setMessage(IDS_CHECKNEWMESSAGE);
	
	const Account::FolderList& listFolder = pAccount_->getFolders();
	for (Account::FolderList::const_iterator it = listFolder.begin();
		it != listFolder.end() && !bReservedDownload_; ++it) {
		Folder* pFolder = *it;
		if (pFolder->getType() == Folder::TYPE_NORMAL &&
			static_cast<NormalFolder*>(pFolder)->getDownloadCount() != 0)
			bReservedDownload_ = true;
	}
	
	unsigned int nGetAll = pSubAccount_->getProperty(L"Pop3", L"GetAll", 20);
	unsigned int nCount = pPop3_->getMessageCount();
	unsigned int nUIDCount = pUIDList->getCount();
	if (nUIDCount == 0 ||
		nCount < nUIDCount ||
		(nCount != nUIDCount && nCount/(nCount - nUIDCount) > nGetAll) ||
		bReservedDownload_) {
		bCacheAll_ = true;
	}
	else {
		wstring_ptr wstrUID;
		if (!pPop3_->getUid(nUIDCount - 1, &wstrUID))
			HANDLE_ERROR();
		UID* pUID = pUIDList->getUID(nUIDCount - 1);
		if (wcscmp(pUID->getUID(), wstrUID.get()) == 0)
			nStart_ = nUIDCount;
		else
			bCacheAll_ = true;
	}
	
	std::auto_ptr<UIDList> pNewUIDList;
	
	if (bCacheAll_) {
		if (!pPop3_->getUids(&listUID_))
			HANDLE_ERROR();
		if (!pPop3_->getMessageSizes(&listSize_))
			HANDLE_ERROR();
		
		if (nUIDCount != 0 && listUID_.size() >= nUIDCount &&
			wcscmp(pUIDList->getUID(nUIDCount - 1)->getUID(),
				listUID_[nUIDCount - 1]) == 0) {
			nStart_ = nUIDCount;
		}
		else {
			pNewUIDList.reset(new UIDList());
			pNewUIDList->setModified(true);
			
			unsigned int nIndex = -1;
			Pop3::UidList::size_type n = 0;
			while (n < listUID_.size()) {
				const WCHAR* pwszUID = listUID_[n];
				nIndex = pUIDList->getIndex(pwszUID, nIndex + 1);
				if (nIndex == -1)
					break;
				
				std::auto_ptr<UID> pUID(pUIDList->remove(nIndex));
				pNewUIDList->add(pUID);
				
				++n;
			}
			nStart_ = static_cast<unsigned int>(n);
		}
	}
	
	if (pNewUIDList.get()) {
		pUIDList_ = pNewUIDList;
		pOldUIDList_ = pUIDList;
	}
	else {
		pUIDList_ = pUIDList;
	}
	
	return true;
}

bool qmpop3::Pop3ReceiveSession::downloadReservedMessages()
{
	if (!bReservedDownload_)
		return true;
	
	assert(bCacheAll_);
	
	Account::FolderList l(pAccount_->getFolders());
	std::sort(l.begin(), l.end(), FolderLess());
	
	unsigned int nCount = 0;
	for (Account::FolderList::const_iterator it = l.begin(); it != l.end(); ++it) {
		Folder* pFolder = *it;
		if (pFolder->getType() == Folder::TYPE_NORMAL)
			nCount += static_cast<NormalFolder*>(pFolder)->getDownloadCount();
	}
	if (nCount == 0)
		return true;
	
	pCallback_->setMessage(IDS_DOWNLOADRESERVEDMESSAGES);
	pSessionCallback_->setRange(0, nCount);
	
	unsigned int nPos = 0;
	for (Account::FolderList::const_iterator it = l.begin(); it != l.end(); ++it) {
		Folder* pFolder = *it;
		if (pFolder->getType() == Folder::TYPE_NORMAL &&
			static_cast<NormalFolder*>(pFolder)->getDownloadCount() != 0) {
			if (!downloadReservedMessages(static_cast<NormalFolder*>(pFolder), &nPos))
				return false;
		}
	}
	
	return true;
}

bool qmpop3::Pop3ReceiveSession::downloadReservedMessages(NormalFolder* pFolder,
														  unsigned int* pnPos)
{
	assert(pFolder);
	assert(pFolder->getAccount() == pAccount_);
	assert(pFolder->getDownloadCount() != 0);
	assert(pnPos);
	assert(bCacheAll_);
	
	Time time(Time::getCurrentTime());
	
	typedef std::vector<MessagePtr> List;
	List l;
	
	{
		Lock<Account> lock(*pAccount_);
		
		if (!pFolder->loadMessageHolders())
			return false;
		
		l.reserve(pFolder->getDownloadCount());
		
		for (unsigned int n = 0; n < pFolder->getCount(); ++n) {
			MessageHolder* pmh = pFolder->getMessage(n);
			if (pmh->isFlag(MessageHolder::FLAG_DOWNLOAD) ||
				pmh->isFlag(MessageHolder::FLAG_DOWNLOADTEXT))
				l.push_back(MessagePtr(pmh));
		}
	}
	
	for (List::const_iterator it = l.begin(); it != l.end(); ++it) {
		MessagePtrLock mpl(*it);
		if (mpl) {
			if (pSessionCallback_->isCanceled(false))
				return true;
			pSessionCallback_->setPos(++(*pnPos));
			
			Message msg;
			if (!mpl->getMessage(Account::GETMESSAGEFLAG_HEADER,
				0, SECURITYMODE_NONE, &msg))
				return false;
			
			if (isSameIdentity(msg, pSubAccount_)) {
				UnstructuredParser uid;
				if (msg.getField(L"X-UIDL", &uid) == Part::FIELD_EXIST) {
					unsigned int nIndex = pUIDList_->getIndex(uid.getValue());
					if (nIndex != -1) {
						xstring_size_ptr strMessage;
						if (!pPop3_->getMessage(nIndex, 0xffffffff, &strMessage, listSize_[nIndex]))
							HANDLE_ERROR();
						
						if (!setUid(&msg, uid.getValue()))
							return false;
						if (!setSubAccount(&msg, pSubAccount_))
							return false;
						
						if (!pAccount_->updateMessage(mpl, strMessage.get(), strMessage.size(), &msg))
							return false;
						
						UID* pUID = pUIDList_->getUID(nIndex);
						pUID->update(UID::FLAG_NONE, time.wYear, time.wMonth, time.wDay);
					}
				}
				mpl->setFlags(0,
					MessageHolder::FLAG_SEEN |
					MessageHolder::FLAG_DOWNLOAD |
					MessageHolder::FLAG_DOWNLOADTEXT |
					MessageHolder::FLAG_PARTIAL_MASK);
			}
		}
	}
	
	return true;
}

bool qmpop3::Pop3ReceiveSession::applyRules(MessagePtrList* pList,
											bool bJunkFilter,
											bool bJunkFilterOnly)
{
	RuleManager* pRuleManager = pDocument_->getRuleManager();
	DefaultReceiveSessionRuleCallback callback(pSessionCallback_);
	return pRuleManager->apply(pFolder_, pList, pDocument_,
		pProfile_, bJunkFilter, bJunkFilterOnly, &callback);
}

std::auto_ptr<UIDList> qmpop3::Pop3ReceiveSession::loadUIDList() const
{
	wstring_ptr wstrPath(getUIDListPath());
	
	std::auto_ptr<UIDList> pUIDList(new UIDList());
	if (!pUIDList->load(wstrPath.get()))
		return std::auto_ptr<UIDList>(0);
	
	return pUIDList;
}

bool qmpop3::Pop3ReceiveSession::saveUIDList(const UIDList* pUIDList) const
{
	assert(pUIDList);
	
	if (pUIDList->isModified()) {
		wstring_ptr wstrPath(getUIDListPath());
		if (!pUIDList->save(wstrPath.get()))
			return false;
	}
	
	return true;
}

wstring_ptr qmpop3::Pop3ReceiveSession::getUIDListPath() const
{
	const WCHAR* pwszIdentity = pSubAccount_->getIdentity();
	const ConcatW c[] = {
		{ pAccount_->getPath(),			-1	},
		{ L"\\uidl",					-1	},
		{ *pwszIdentity ? L"_" : L"",	-1	},
		{ pwszIdentity,					-1	},
		{ L".xml",						-1	}
	};
	return concat(c, countof(c));
}

bool qmpop3::Pop3ReceiveSession::isSameIdentity(const Message& msg,
												SubAccount* pSubAccount)
{
	const WCHAR* pwszIdentity = pSubAccount->getIdentity();
	if (!*pwszIdentity)
		return true;
	
	UnstructuredParser subaccount;
	if (msg.getField(L"X-QMAIL-SubAccount", &subaccount) != Part::FIELD_EXIST)
		return false;
	
	Account* pAccount = pSubAccount->getAccount();
	SubAccount* pMessageSubAccount = pAccount->getSubAccount(subaccount.getValue());
	return pMessageSubAccount && wcscmp(pMessageSubAccount->getIdentity(), pwszIdentity) == 0;
}

bool qmpop3::Pop3ReceiveSession::setUid(Message* pMessage,
										const WCHAR* pwszUid)
{
	UnstructuredParser uid(pwszUid, L"utf-8");
	return pMessage->replaceField(L"X-UIDL", uid);
}

bool qmpop3::Pop3ReceiveSession::setSubAccount(Message* pMessage,
											   SubAccount* pSubAccount)
{
	const WCHAR* pwszIdentity = pSubAccount->getIdentity();
	if (*pwszIdentity) {
		UnstructuredParser subaccount(pSubAccount->getName(), L"utf-8");
		if (!pMessage->replaceField(L"X-QMAIL-SubAccount", subaccount))
			return false;
	}
	else {
		pMessage->removeField(L"X-QMAIL-SubAccount");
	}
	return true;
}


/****************************************************************************
 *
 * Pop3ReceiveSession::CallbackImpl
 *
 */

qmpop3::Pop3ReceiveSession::CallbackImpl::CallbackImpl(SubAccount* pSubAccount,
													   const Security* pSecurity,
													   ReceiveSessionCallback* pSessionCallback) :
	DefaultSSLSocketCallback(pSubAccount, Account::HOST_RECEIVE, pSecurity),
	pSubAccount_(pSubAccount),
	pSessionCallback_(pSessionCallback),
	state_(PASSWORDSTATE_ONETIME)
{
}

qmpop3::Pop3ReceiveSession::CallbackImpl::~CallbackImpl()
{
}

void qmpop3::Pop3ReceiveSession::CallbackImpl::setMessage(UINT nId)
{
	wstring_ptr wstrMessage(loadString(getResourceHandle(), nId));
	pSessionCallback_->setMessage(wstrMessage.get());
}

bool qmpop3::Pop3ReceiveSession::CallbackImpl::isCanceled(bool bForce) const
{
	return pSessionCallback_->isCanceled(bForce);
}

void qmpop3::Pop3ReceiveSession::CallbackImpl::initialize()
{
	setMessage(IDS_INITIALIZE);
}

void qmpop3::Pop3ReceiveSession::CallbackImpl::lookup()
{
	setMessage(IDS_LOOKUP);
}

void qmpop3::Pop3ReceiveSession::CallbackImpl::connecting()
{
	setMessage(IDS_CONNECTING);
}

void qmpop3::Pop3ReceiveSession::CallbackImpl::connected()
{
	setMessage(IDS_CONNECTED);
}

bool qmpop3::Pop3ReceiveSession::CallbackImpl::getUserInfo(wstring_ptr* pwstrUserName,
														   wstring_ptr* pwstrPassword)
{
	state_ = Util::getUserInfo(pSubAccount_, Account::HOST_RECEIVE,
		pSessionCallback_, pwstrUserName, pwstrPassword);
	return state_ != PASSWORDSTATE_NONE;
}

void qmpop3::Pop3ReceiveSession::CallbackImpl::setPassword(const WCHAR* pwszPassword)
{
	Util::setPassword(pSubAccount_, Account::HOST_RECEIVE,
		state_, pSessionCallback_, pwszPassword);
}

void qmpop3::Pop3ReceiveSession::CallbackImpl::authenticating()
{
	setMessage(IDS_AUTHENTICATING);
}

void qmpop3::Pop3ReceiveSession::CallbackImpl::setRange(size_t nMin,
														size_t nMax)
{
	pSessionCallback_->setSubRange(nMin, nMax);
}

void qmpop3::Pop3ReceiveSession::CallbackImpl::setPos(size_t nPos)
{
	pSessionCallback_->setSubPos(nPos);
}


/****************************************************************************
 *
 * Pop3ReceiveSession::UIDSaver
 *
 */

qmpop3::Pop3ReceiveSession::UIDSaver::UIDSaver(Pop3ReceiveSession* pSession,
											   Logger* pLogger,
											   UIDList* pUIDList) :
	pSession_(pSession),
	pLogger_(pLogger),
	pUIDList_(pUIDList)
{
}

qmpop3::Pop3ReceiveSession::UIDSaver::~UIDSaver()
{
	if (pUIDList_) {
		if (!pSession_->saveUIDList(pUIDList_)) {
			Log log(pLogger_, L"qmpop3::Pop3ReceiveSession");
			log.error(L"Failed to save uid list.");
		}
	}
}

	
/****************************************************************************
 *
 * Pop3ReceiveSessionUI
 *
 */

qmpop3::Pop3ReceiveSessionUI::Pop3ReceiveSessionUI()
{
}

qmpop3::Pop3ReceiveSessionUI::~Pop3ReceiveSessionUI()
{
}

const WCHAR* qmpop3::Pop3ReceiveSessionUI::getClass()
{
	return L"mail";
}

wstring_ptr qmpop3::Pop3ReceiveSessionUI::getDisplayName()
{
	return loadString(getResourceHandle(), IDS_POP3);
}

short qmpop3::Pop3ReceiveSessionUI::getDefaultPort(bool bSecure)
{
	return bSecure ? 995 : 110;
}

bool qmpop3::Pop3ReceiveSessionUI::isSupported(Support support)
{
	return true;
}

std::auto_ptr<PropertyPage> qmpop3::Pop3ReceiveSessionUI::createPropertyPage(SubAccount* pSubAccount)
{
	return std::auto_ptr<PropertyPage>(new ReceivePage(pSubAccount));
}


/****************************************************************************
 *
 * Pop3ReceiveSessionFactory
 *
 */

Pop3ReceiveSessionFactory qmpop3::Pop3ReceiveSessionFactory::factory__;

qmpop3::Pop3ReceiveSessionFactory::Pop3ReceiveSessionFactory()
{
	registerFactory(L"pop3", this);
}

qmpop3::Pop3ReceiveSessionFactory::~Pop3ReceiveSessionFactory()
{
	unregisterFactory(L"pop3");
}

std::auto_ptr<ReceiveSession> qmpop3::Pop3ReceiveSessionFactory::createSession()
{
	return std::auto_ptr<ReceiveSession>(new Pop3ReceiveSession());
}

std::auto_ptr<ReceiveSessionUI> qmpop3::Pop3ReceiveSessionFactory::createUI()
{
	return std::auto_ptr<ReceiveSessionUI>(new Pop3ReceiveSessionUI());
}


/****************************************************************************
 *
 * Pop3SyncFilterCallback
 *
 */

qmpop3::Pop3SyncFilterCallback::Pop3SyncFilterCallback(Document* pDocument,
													   Account* pAccount,
													   NormalFolder* pFolder,
													   Message* pMessage,
													   unsigned int nSize,
													   Profile* pProfile,
													   MacroVariableHolder* pGlobalVariable,
													   Pop3* pPop3,
													   unsigned int nMessage,
													   xstring_size_ptr* pstrMessage,
													   Pop3ReceiveSession::State* pState) :
	pDocument_(pDocument),
	pAccount_(pAccount),
	pFolder_(pFolder),
	pMessage_(pMessage),
	nSize_(nSize),
	pProfile_(pProfile),
	pGlobalVariable_(pGlobalVariable),
	pPop3_(pPop3),
	nMessage_(nMessage),
	pstrMessage_(pstrMessage),
	pState_(pState)
{
}

qmpop3::Pop3SyncFilterCallback::~Pop3SyncFilterCallback()
{
}

bool qmpop3::Pop3SyncFilterCallback::getMessage(unsigned int nFlag)
{
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
		return false;
	}
	
	if (bDownload) {
		xstring_size_ptr& str = *pstrMessage_;
		
		str.reset(0, -1);
		if (!pPop3_->getMessage(nMessage_, nMaxLine, &str, nSize_))
			return false;
		
		if (!pMessage_->createHeader(str.get(), str.size()))
			return false;
	}
	
	return true;
}

const NormalFolder* qmpop3::Pop3SyncFilterCallback::getFolder()
{
	return pFolder_;
}

std::auto_ptr<MacroContext> qmpop3::Pop3SyncFilterCallback::getMacroContext()
{
	if (!pmh_.get())
		pmh_.reset(new Pop3MessageHolder(this, pFolder_, pMessage_, nSize_));
	
	return std::auto_ptr<MacroContext>(new MacroContext(
		pmh_.get(), pMessage_, MessageHolderList(), pAccount_,
		pDocument_, 0, pProfile_, 0, MacroContext::FLAG_NONE,
		SECURITYMODE_NONE, 0, pGlobalVariable_));
}


/****************************************************************************
 *
 * Pop3MessageHolder
 *
 */

qmpop3::Pop3MessageHolder::Pop3MessageHolder(Pop3SyncFilterCallback* pCallback,
											 NormalFolder* pFolder,
											 Message* pMessage,
											 unsigned int nSize) :
	AbstractMessageHolder(pFolder, pMessage, -1, nSize, nSize),
	pCallback_(pCallback)
{
}

qmpop3::Pop3MessageHolder::~Pop3MessageHolder()
{
}

wstring_ptr qmpop3::Pop3MessageHolder::getFrom() const
{
	if (!getMessage(Account::GETMESSAGEFLAG_HEADER))
		return allocWString(L"");
	return AbstractMessageHolder::getFrom();
}

wstring_ptr qmpop3::Pop3MessageHolder::getTo() const
{
	if (!getMessage(Account::GETMESSAGEFLAG_HEADER))
		return allocWString(L"");
	return AbstractMessageHolder::getTo();
}

wstring_ptr qmpop3::Pop3MessageHolder::getFromTo() const
{
	return getFrom();
}

wstring_ptr qmpop3::Pop3MessageHolder::getSubject() const
{
	if (!getMessage(Account::GETMESSAGEFLAG_HEADER))
		return allocWString(L"");
	return AbstractMessageHolder::getSubject();
}

void qmpop3::Pop3MessageHolder::getDate(Time* pTime) const
{
	if (!getMessage(Account::GETMESSAGEFLAG_HEADER))
		*pTime = Time::getCurrentTime();
	AbstractMessageHolder::getDate(pTime);
}

bool qmpop3::Pop3MessageHolder::getMessage(unsigned int nFlags,
										   const WCHAR* pwszField,
										   unsigned int nSecurityMode,
										   Message* pMessage)
{
	assert(pMessage == AbstractMessageHolder::getMessage());
	return getMessage(nFlags);
}

bool qmpop3::Pop3MessageHolder::getMessage(unsigned int nFlags) const
{
	return pCallback_->getMessage(nFlags);
}


/****************************************************************************
 *
 * DeleteList
 *
 */

qmpop3::DeleteList::DeleteList()
{
}

qmpop3::DeleteList::~DeleteList()
{
}

const DeleteList::List qmpop3::DeleteList::getList() const
{
	return list_;
}

void qmpop3::DeleteList::add(size_t n)
{
	add(n, MessagePtr());
}

void qmpop3::DeleteList::add(size_t n,
							 const MessagePtr& ptr)
{
	resize(n + 1);
	list_[n].first = true;
	list_[n].second = ptr;
}

void qmpop3::DeleteList::resize(size_t n)
{
	if (list_.size() < n)
		list_.resize(n);
}
