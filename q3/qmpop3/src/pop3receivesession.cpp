/*
 * $Id$
 *
 * Copyright(C) 1998-2004 Satoshi Nakamura
 * All rights reserved.
 *
 */

#include <qmaccount.h>
#include <qmfolder.h>
#include <qmmessage.h>
#include <qmmessageholder.h>
#include <qmsecurity.h>

#include <qsconv.h>
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


#define HANDLE_ERROR() \
	do { \
		Util::reportError(pPop3_.get(), pSessionCallback_, pAccount_, pSubAccount_); \
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
	hwnd_(0),
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
									  HWND hwnd,
									  Profile* pProfile,
									  Logger* pLogger,
									  ReceiveSessionCallback* pCallback)
{
	assert(pDocument);
	assert(pAccount);
	assert(pSubAccount);
	assert(hwnd);
	assert(pProfile);
	assert(pCallback);
	
	pDocument_ = pDocument;
	pAccount_ = pAccount;
	pSubAccount_ = pSubAccount;
	hwnd_ = hwnd;
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
	Pop3::Ssl ssl = Util::getSsl(pSubAccount_);
	if (!pPop3_->connect(pSubAccount_->getHost(Account::HOST_RECEIVE),
		pSubAccount_->getPort(Account::HOST_RECEIVE), bApop, ssl))
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
											  bool bExpunge)
{
	assert(pFolder);
	assert(!bExpunge);
	
	if (!prepare() ||
		!downloadReservedMessages())
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
	
	const WCHAR* pwszIdentity = pSubAccount_->getIdentity();
	UnstructuredParser subaccount(pSubAccount_->getName(), L"utf-8");
	
	Time time(Time::getCurrentTime());
	UID::Date date = {
		time.wYear,
		time.wMonth,
		time.wDay
	};
	
	MacroVariableHolder globalVariable;
	
	DeleteList listDelete;
	
	for (unsigned int n = nStart_; n < nCount; ++n) {
		if (pSessionCallback_->isCanceled(false))
			return true;
		pSessionCallback_->setPos(n + 1);
		
		unsigned int nSize = 0;
		if (bCacheAll_) {
			nSize = listSize_[n];
		}
		else {
			if (!pPop3_->getMessageSize(n, &nSize))
				HANDLE_ERROR();
		}
		
		xstring_ptr strMessage;
		Message msg;
		State state = STATE_NONE;
		unsigned int nGetSize = 0;
		unsigned int nMaxLine = 0xffffffff;
		bool bIgnore = false;
		if (pSyncFilterSet) {
			Pop3SyncFilterCallback callback(pDocument_, pAccount_,
				pFolder_, &msg, nSize, hwnd_, pProfile_, &globalVariable,
				pPop3_.get(), n, strMessage.getThis(), &state, &nGetSize);
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
			strMessage.reset(0);
			nGetSize = nSize;
			if (!pPop3_->getMessage(n, nMaxLine, &strMessage, &nGetSize))
				HANDLE_ERROR();
			
			size_t nLen = -1;
			CHAR* p = strstr(strMessage.get(), "\r\n\r\n");
			if (p)
				nLen = p - strMessage.get() + 4;
			if (!msg.create(strMessage.get(), nLen, Message::FLAG_HEADERONLY))
				return false;
		}
		
		bool bPartial = bIgnore || (nMaxLine != 0xffffffff && nSize > nGetSize);
		
		const WCHAR* pwszUID = 0;
		wstring_ptr wstrUID;
		if (bCacheAll_) {
			pwszUID = listUID_[n];
		}
		else {
			if (!pPop3_->getUid(n, &wstrUID))
				HANDLE_ERROR();
			pwszUID = wstrUID.get();
		}
		
		if (!bIgnore) {
			UnstructuredParser uid(pwszUID, L"utf-8");
			if (!msg.replaceField(L"X-UIDL", uid))
				return false;
			
			if (*pwszIdentity) {
				if (!msg.replaceField(L"X-QMAIL-SubAccount", subaccount))
					return false;
			}
			else {
				msg.removeField(L"X-QMAIL-SubAccount");
			}
			
			unsigned int nFlags = (bPartial ? MessageHolder::FLAG_HEADERONLY : 0);
			
			if (bHandleStatus) {
				UnstructuredParser status;
				if (msg.getField(L"Status", &status) == Part::FIELD_EXIST &&
					wcscmp(status.getValue(), L"RO") == 0)
					nFlags |= MessageHolder::FLAG_SEEN;
			}
			
			if (pSubAccount_->isSelf(msg))
				nFlags |= MessageHolder::FLAG_SEEN | MessageHolder::FLAG_SENT;
			
			Lock<Account> lock(*pAccount_);
			
			MessageHolder* pmh = pAccount_->storeMessage(pFolder_,
				strMessage.get(), &msg, -1, nFlags, nSize, false);
			if (!pmh)
				return false;
			
			if ((nFlags & MessageHolder::FLAG_SEEN) == 0)
				pSessionCallback_->notifyNewMessage(pmh);
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
						if (!pmh->getMessage(Account::GETMESSAGEFLAG_HEADER, L"X-UIDL", &msg))
							return false;
						
						bool bSkip = false;
						if (*pwszIdentity) {
							UnstructuredParser subaccount;
							if (msg.getField(L"X-QMAIL-SubAccount", &subaccount) == Part::FIELD_EXIST) {
								SubAccount* pSubAccount = pAccount_->getSubAccount(subaccount.getValue());
								bSkip = !pSubAccount || wcscmp(pSubAccount->getIdentity(), pwszIdentity) != 0;
							}
							else {
								bSkip = true;
							}
						}
						
						if (!bSkip) {
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
		for (size_t n = 0; n < pUIDList_->getCount(); ++n) {
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
				listIndex.push_back(m);
		}
		
		if (listIndex.size()) {
			pCallback_->setMessage(IDS_DELETEMESSAGE);
			pSessionCallback_->setRange(0, listIndex.size());
			pSessionCallback_->setPos(0);
			
			int nPos = 0;
			for (DeleteList::List::size_type n = 0; n < l.size(); ++n) {
				if (l[n].first) {
					pSessionCallback_->setPos(++nPos);
					
					if (!pPop3_->deleteMessage(n))
						HANDLE_ERROR();
					
					MessagePtrLock mpl(l[n].second);
					if (mpl)
						mpl->setFlags(0, MessageHolder::FLAG_DELETED);
				}
			}
			
			pUIDList_->remove(listIndex);
		}
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
			
			size_t nIndex = -1;
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
			nStart_ = n;
		}
	}
	
	if (pNewUIDList.get())
		pUIDList_ = pNewUIDList;
	else
		pUIDList_ = pUIDList;
	
	return true;
}

bool qmpop3::Pop3ReceiveSession::downloadReservedMessages()
{
	if (bReservedDownload_) {
		assert(bCacheAll_);
		
		const Account::FolderList& l = pAccount_->getFolders();
		for (Account::FolderList::const_iterator it = l.begin(); it != l.end(); ++it) {
			Folder* pFolder = *it;
			if (pFolder->getType() == Folder::TYPE_NORMAL) {
				if (!downloadReservedMessages(static_cast<NormalFolder*>(pFolder)))
					return false;
			}
		}
	}
	
	return true;
}

bool qmpop3::Pop3ReceiveSession::downloadReservedMessages(NormalFolder* pFolder)
{
	assert(pFolder);
	assert(pFolder->getAccount() == pAccount_);
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
			Message msg;
			if (!mpl->getMessage(Account::GETMESSAGEFLAG_HEADER, 0, &msg))
				return false;
			UnstructuredParser uidl;
			if (msg.getField(L"X-UIDL", &uidl) == Part::FIELD_EXIST) {
				size_t nIndex = pUIDList_->getIndex(uidl.getValue());
				if (nIndex != -1) {
					xstring_ptr strMessage;
					unsigned int nSize = listSize_[nIndex];
					if (!pPop3_->getMessage(nIndex, 0xffffffff, &strMessage, &nSize))
						HANDLE_ERROR();
					if (!pAccount_->updateMessage(mpl, strMessage.get()))
						return false;
					
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
	}
	
	return true;
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
	pSessionCallback_(pSessionCallback)
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
	assert(pwstrUserName);
	assert(pwstrPassword);
	
	*pwstrUserName = allocWString(pSubAccount_->getUserName(Account::HOST_RECEIVE));
	*pwstrPassword = allocWString(pSubAccount_->getPassword(Account::HOST_RECEIVE));
	
	return true;
}

void qmpop3::Pop3ReceiveSession::CallbackImpl::setPassword(const WCHAR* pwszPassword)
{
	// TODO
}

void qmpop3::Pop3ReceiveSession::CallbackImpl::authenticating()
{
	setMessage(IDS_AUTHENTICATING);
}

void qmpop3::Pop3ReceiveSession::CallbackImpl::setRange(unsigned int nMin,
														unsigned int nMax)
{
	pSessionCallback_->setSubRange(nMin, nMax);
}

void qmpop3::Pop3ReceiveSession::CallbackImpl::setPos(unsigned int nPos)
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

short qmpop3::Pop3ReceiveSessionUI::getDefaultPort()
{
	return 110;
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
													   HWND hwnd,
													   Profile* pProfile,
													   MacroVariableHolder* pGlobalVariable,
													   Pop3* pPop3,
													   unsigned int nMessage,
													   xstring_ptr* pstrMessage,
													   Pop3ReceiveSession::State* pState,
													   unsigned int* pnGetSize) :
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
	pnGetSize_(pnGetSize)
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
		xstring_ptr& str = *pstrMessage_;
		
		str.reset(0);
		*pnGetSize_ = nSize_;
		if (!pPop3_->getMessage(nMessage_, nMaxLine, &str, pnGetSize_))
			return false;
		
		size_t nLen = -1;
		CHAR* p = strstr(str.get(), "\r\n\r\n");
		if (p)
			nLen = p - str.get() + 4;
		if (!pMessage_->create(str.get(), nLen, Message::FLAG_HEADERONLY))
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
	
	return std::auto_ptr<MacroContext>(new MacroContext(pmh_.get(),
		pMessage_, MessageHolderList(), pAccount_, pDocument_,
		hwnd_, pProfile_, false, false, 0, pGlobalVariable_));
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
