/*
 * $Id$
 *
 * Copyright(C) 1998-2003 Satoshi Nakamura
 * All rights reserved.
 *
 */

#include <qmdocument.h>
#include <qmmessage.h>

#include <qsnew.h>
#include <qsthread.h>

#include "lastid.h"
#include "main.h"
#include "nntpreceivesession.h"
#include "resourceinc.h"
#include "ui.h"
#include "util.h"

using namespace qmnntp;
using namespace qm;
using namespace qs;


#define CHECK_QSTATUS_ERROR() \
	if (status != QSTATUS_SUCCESS) { \
		Util::reportError(pNntp_, pSessionCallback_, pAccount_, pSubAccount_); \
		return status; \
	} \


/****************************************************************************
 *
 * NntpReceiveSession
 *
 */

qmnntp::NntpReceiveSession::NntpReceiveSession(QSTATUS* pstatus) :
	pNntp_(0),
	pCallback_(0),
	pDocument_(0),
	pAccount_(0),
	pSubAccount_(0),
	pFolder_(0),
	hwnd_(0),
	pLogger_(0),
	pSessionCallback_(0),
	pLastIdList_(0)
{
}

qmnntp::NntpReceiveSession::~NntpReceiveSession()
{
	delete pNntp_;
	delete pCallback_;
	delete pLastIdList_;
}

QSTATUS qmnntp::NntpReceiveSession::init(Document* pDocument,
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
	
	status = newQsObject(pSubAccount_, pDocument->getSecurity(),
		pSessionCallback_, &pCallback_);
	CHECK_QSTATUS();
	
	return QSTATUS_SUCCESS;
}

QSTATUS qmnntp::NntpReceiveSession::connect()
{
	assert(!pNntp_);
	
	DECLARE_QSTATUS();
	
	Log log(pLogger_, L"qmnntp::NntpReceiveSession");
	status = log.debug(L"Connecting to the server...");
	CHECK_QSTATUS();
	
	Nntp::Option option = {
		pSubAccount_->getTimeout(),
		pCallback_,
		pCallback_,
		pCallback_,
		pLogger_
	};
	status = newQsObject(option, &pNntp_);
	CHECK_QSTATUS();
	
	status = pNntp_->connect(pSubAccount_->getHost(Account::HOST_RECEIVE),
		pSubAccount_->getPort(Account::HOST_RECEIVE),
		pSubAccount_->isSsl(Account::HOST_RECEIVE));
	CHECK_QSTATUS_ERROR();
	
	status = log.debug(L"Connected to the server.");
	CHECK_QSTATUS();
	
	string_ptr<WSTRING> wstrPath(concat(pAccount_->getPath(), L"\\lastid.xml"));
	if (!wstrPath.get())
		return QSTATUS_OUTOFMEMORY;
	status = newQsObject(wstrPath.get(), &pLastIdList_);
	CHECK_QSTATUS();
	
	return QSTATUS_SUCCESS;
}

QSTATUS qmnntp::NntpReceiveSession::disconnect()
{
	assert(pNntp_);
	
	DECLARE_QSTATUS();
	
	if (pLastIdList_->isModified()) {
		status = pLastIdList_->save();
		CHECK_QSTATUS();
	}
	
	Log log(pLogger_, L"qmnntp::NntpReceiveSession");
	status = log.debug(L"Disconnecting from the server...");
	CHECK_QSTATUS();
	
	status = pNntp_->disconnect();
	CHECK_QSTATUS_ERROR();
	
	status = log.debug(L"Disconnected from the server.");
	CHECK_QSTATUS();
	
	return QSTATUS_SUCCESS;
}

QSTATUS qmnntp::NntpReceiveSession::selectFolder(NormalFolder* pFolder)
{
	DECLARE_QSTATUS();
	
	status = pCallback_->setMessage(IDS_SELECTGROUP);
	CHECK_QSTATUS();
	
	string_ptr<WSTRING> wstrGroup;
	status = pFolder->getFullName(&wstrGroup);
	CHECK_QSTATUS();
	
	status = pNntp_->group(wstrGroup.get());
	CHECK_QSTATUS();
	
	pFolder_ = pFolder;
	
	return QSTATUS_SUCCESS;
}

QSTATUS qmnntp::NntpReceiveSession::closeFolder()
{
	assert(pFolder_);
	
	DECLARE_QSTATUS();
	
	status = pCallback_->setRange(0, 0);
	CHECK_QSTATUS();
	status = pCallback_->setPos(0);
	CHECK_QSTATUS();
	
	pFolder_ = 0;
	
	return QSTATUS_SUCCESS;
}

QSTATUS qmnntp::NntpReceiveSession::updateMessages()
{
	return QSTATUS_SUCCESS;
}

QSTATUS qmnntp::NntpReceiveSession::downloadMessages(
	const SyncFilterSet* pSyncFilterSet)
{
	DECLARE_QSTATUS();
	
	unsigned int nStart = pNntp_->getFirst();
	{
		unsigned int nLastId = pLastIdList_->getLastId(pNntp_->getGroup());
		if (nLastId != 0)
			nStart = nLastId + 1;
		
		Lock<Account> lock(*pAccount_);
		status = pFolder_->loadMessageHolders();
		CHECK_QSTATUS();
		unsigned int nCount = pFolder_->getCount();
		if (nCount != 0) {
			unsigned int nId = pFolder_->getMessage(nCount - 1)->getId();
			nStart = QSMAX(nStart, nId + 1);
		}
		else if (nLastId == 0) {
			unsigned int nInitialFetchCount = 0;
			status = pSubAccount_->getProperty(L"Nntp", L"InitialFetchCount",
				300, reinterpret_cast<int*>(&nInitialFetchCount));
			CHECK_QSTATUS();
			if (pNntp_->getLast() > nInitialFetchCount - 1)
				nStart = QSMAX(nStart, pNntp_->getLast() - nInitialFetchCount + 1);
		}
	}
	
	status = pCallback_->setMessage(IDS_DOWNLOADMESSAGES);
	CHECK_QSTATUS();
	status = pSessionCallback_->setRange(0, pNntp_->getLast());
	CHECK_QSTATUS();
	status = pSessionCallback_->setPos(nStart);
	CHECK_QSTATUS();
	
	int nUseXOver = 1;
	status = pSubAccount_->getProperty(L"Nntp", L"UseXOVER", 1, &nUseXOver);
	CHECK_QSTATUS();
	
	MacroVariableHolder globalVariable(&status);
	CHECK_QSTATUS();
	
	if (nUseXOver) {
		unsigned int nStep = 0;
		status = pSubAccount_->getProperty(L"Nntp", L"XOVERStep",
			100, reinterpret_cast<int*>(&nStep));
		CHECK_QSTATUS();
		
		for (unsigned int n = nStart; n <= pNntp_->getLast(); n += nStep) {
			MessagesData* p = 0;
			status = pNntp_->getMessagesData(n, n + nStep - 1, &p);
			CHECK_QSTATUS_ERROR();
			std::auto_ptr<MessagesData> pData(p);
			
			for (size_t m = 0; m < pData->getCount(); ++m) {
				if (pSessionCallback_->isCanceled(false))
					return QSTATUS_SUCCESS;
				status = pSessionCallback_->setPos(n + m);
				CHECK_QSTATUS();
				
				const MessagesData::Item& item = pData->getItem(m);
				
				struct {
					const CHAR* pszName_;
					const CHAR* pszValue_;
				} fields[] = {
					{ "Subject",	item.pszSubject_	},
					{ "From",		item.pszFrom_		},
					{ "Date",		item.pszDate_		},
					{ "Message-Id",	item.pszMessageId_	},
					{ "References",	item.pszReferences_	}
				};
				
				StringBuffer<STRING> buf(&status);
				CHECK_QSTATUS();
				for (int f = 0; f < countof(fields); ++f) {
					if (*fields[f].pszValue_) {
						status = buf.append(fields[f].pszName_);
						CHECK_QSTATUS();
						status = buf.append(": ");
						CHECK_QSTATUS();
						status = buf.append(fields[f].pszValue_);
						CHECK_QSTATUS();
						status = buf.append("\r\n");
						CHECK_QSTATUS();
					}
				}
				status = buf.append("\r\n");
				CHECK_QSTATUS();
				
				const CHAR* pszMessage = buf.getCharArray();
				unsigned int nFlags = MessageHolder::FLAG_INDEXONLY;
				
				string_ptr<STRING> strMessage;
				if (pSyncFilterSet) {
					bool bDownload = false;
					Message msg(buf.getCharArray(), buf.getLength(),
						Message::FLAG_TEMPORARY, &status);
					CHECK_QSTATUS();
					State state = STATE_NONE;
					const SyncFilter* pFilter = 0;
					NntpSyncFilterCallback callback(pDocument_, pAccount_,
						pFolder_, &msg, item.nBytes_, hwnd_, pProfile_, &globalVariable,
						pNntp_, item.nId_, strMessage.getThis(), &state);
					status = pSyncFilterSet->getFilter(&callback, &pFilter);
					CHECK_QSTATUS();
					if (pFilter) {
						const SyncFilter::ActionList& listAction = pFilter->getActions();
						SyncFilter::ActionList::const_iterator it = listAction.begin();
						while (it != listAction.end()) {
							const SyncFilterAction* pAction = *it;
							if (wcscmp(pAction->getName(), L"download") == 0) {
								if (state != STATE_ALL) {
									status = pNntp_->getMessage(item.nId_,
										Nntp::GETMESSAGEFLAG_ARTICLE, &strMessage);
									CHECK_QSTATUS_ERROR();
								}
								if (strMessage.get()) {
									pszMessage = strMessage.get();
									nFlags = 0;
								}
							}
							++it;
						}
					}
				}
				
				{
					Lock<Account> lock(*pAccount_);
					
					MessageHolder* pmh = 0;
					status = pAccount_->storeMessage(pFolder_, pszMessage,
						0, item.nId_, nFlags, item.nBytes_,
						nFlags == MessageHolder::FLAG_INDEXONLY, &pmh);
					CHECK_QSTATUS();
				}
				
				if ((nFlags & MessageHolder::FLAG_SEEN) == 0) {
					status = pSessionCallback_->notifyNewMessage();
					CHECK_QSTATUS();
				}
				
				status = pLastIdList_->setLastId(pNntp_->getGroup(), item.nId_);
				CHECK_QSTATUS();
			}
		}
	}
	else {
		for (unsigned int n = nStart; n <= pNntp_->getLast(); ++n) {
			if (pSessionCallback_->isCanceled(false))
				return QSTATUS_SUCCESS;
			status = pSessionCallback_->setPos(n);
			CHECK_QSTATUS();
			
			string_ptr<STRING> strMessage;
			status = pNntp_->getMessage(n,
				Nntp::GETMESSAGEFLAG_HEAD, &strMessage);
			CHECK_QSTATUS_ERROR();
			
			if (strMessage.get()) {
				// TODO
				// Process sync filter ?
				
				Lock<Account> lock(*pAccount_);
				
				MessageHolder* pmh = 0;
				status = pAccount_->storeMessage(pFolder_, strMessage.get(),
					0, n, MessageHolder::FLAG_INDEXONLY, -1, true, &pmh);
				CHECK_QSTATUS();
				
				status = pSessionCallback_->notifyNewMessage();
				CHECK_QSTATUS();
				
				status = pLastIdList_->setLastId(pNntp_->getGroup(), n);
				CHECK_QSTATUS();
			}
		}
	}
	
	return QSTATUS_SUCCESS;
}

QSTATUS qmnntp::NntpReceiveSession::applyOfflineJobs()
{
	return downloadReservedMessages();
}

QSTATUS qmnntp::NntpReceiveSession::downloadReservedMessages()
{
	DECLARE_QSTATUS();
	
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
	
	return QSTATUS_SUCCESS;
}

QSTATUS qmnntp::NntpReceiveSession::downloadReservedMessages(
	NormalFolder* pFolder)
{
	assert(pFolder);
	
	DECLARE_QSTATUS();
	
	if (pFolder->getDownloadCount() == 0)
		return QSTATUS_SUCCESS;
	
	string_ptr<WSTRING> wstrGroup;
	status = pFolder->getFullName(&wstrGroup);
	CHECK_QSTATUS();
	status = pNntp_->group(wstrGroup.get());
	CHECK_QSTATUS_ERROR();
	
	typedef std::vector<MessagePtr> List;
	List l;
	
	{
		Lock<Account> lock(*pAccount_);
		
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
			string_ptr<STRING> strMessage;
			status = pNntp_->getMessage(mpl->getId(),
				Nntp::GETMESSAGEFLAG_ARTICLE, &strMessage);
			CHECK_QSTATUS();
			
			unsigned int nMask = MessageHolder::FLAG_DOWNLOAD |
				MessageHolder::FLAG_DOWNLOADTEXT;
			if (strMessage.get()) {
				status = pAccount_->updateMessage(mpl, strMessage.get());
				CHECK_QSTATUS();
				nMask |=  MessageHolder::FLAG_SEEN |
					MessageHolder::FLAG_PARTIAL_MASK;
			}
			mpl->setFlags(0, nMask);
		}
		++it;
	}
	
	return QSTATUS_SUCCESS;
}


/****************************************************************************
 *
 * NntpReceiveSession::CallbackImpl
 *
 */

qmnntp::NntpReceiveSession::CallbackImpl::CallbackImpl(
	SubAccount* pSubAccount, const Security* pSecurity,
	ReceiveSessionCallback* pSessionCallback, QSTATUS* pstatus) :
	AbstractCallback(pSubAccount, pSecurity, pstatus),
	pSessionCallback_(pSessionCallback)
{
}

qmnntp::NntpReceiveSession::CallbackImpl::~CallbackImpl()
{
}

QSTATUS qmnntp::NntpReceiveSession::CallbackImpl::setMessage(UINT nId)
{
	DECLARE_QSTATUS();
	
	string_ptr<WSTRING> wstrMessage;
	status = loadString(getResourceHandle(), nId, &wstrMessage);
	CHECK_QSTATUS();
	
	return pSessionCallback_->setMessage(wstrMessage.get());
}

bool qmnntp::NntpReceiveSession::CallbackImpl::isCanceled(bool bForce) const
{
	return pSessionCallback_->isCanceled(bForce);
}

QSTATUS qmnntp::NntpReceiveSession::CallbackImpl::initialize()
{
	return setMessage(IDS_INITIALIZE);
}

QSTATUS qmnntp::NntpReceiveSession::CallbackImpl::lookup()
{
	return setMessage(IDS_LOOKUP);
}

QSTATUS qmnntp::NntpReceiveSession::CallbackImpl::connecting()
{
	return setMessage(IDS_CONNECTING);
}

QSTATUS qmnntp::NntpReceiveSession::CallbackImpl::connected()
{
	return setMessage(IDS_CONNECTED);
}

QSTATUS qmnntp::NntpReceiveSession::CallbackImpl::authenticating()
{
	return setMessage(IDS_AUTHENTICATING);
}

QSTATUS qmnntp::NntpReceiveSession::CallbackImpl::setRange(
	unsigned int nMin, unsigned int nMax)
{
	return pSessionCallback_->setSubRange(nMin, nMax);
}

QSTATUS qmnntp::NntpReceiveSession::CallbackImpl::setPos(unsigned int nPos)
{
	return pSessionCallback_->setSubPos(nPos);
}


/****************************************************************************
 *
 * NntpReceiveSessionUI
 *
 */

qmnntp::NntpReceiveSessionUI::NntpReceiveSessionUI(QSTATUS* pstatus)
{
}

qmnntp::NntpReceiveSessionUI::~NntpReceiveSessionUI()
{
}

const WCHAR* qmnntp::NntpReceiveSessionUI::getClass()
{
	return L"news";
}

QSTATUS qmnntp::NntpReceiveSessionUI::getDisplayName(WSTRING* pwstrName)
{
	assert(pwstrName);
	return loadString(getResourceHandle(), IDS_NNTP, pwstrName);
}

short qmnntp::NntpReceiveSessionUI::getDefaultPort()
{
	return 119;
}

QSTATUS qmnntp::NntpReceiveSessionUI::createPropertyPage(
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
 * NntpReceiveSessionFactory
 *
 */

NntpReceiveSessionFactory qmnntp::NntpReceiveSessionFactory::factory__;

qmnntp::NntpReceiveSessionFactory::NntpReceiveSessionFactory()
{
	regist(L"nntp", this);
}

qmnntp::NntpReceiveSessionFactory::~NntpReceiveSessionFactory()
{
	unregist(L"nntp");
}

QSTATUS qmnntp::NntpReceiveSessionFactory::createSession(
	ReceiveSession** ppReceiveSession)
{
	assert(ppReceiveSession);
	
	DECLARE_QSTATUS();
	
	NntpReceiveSession* pSession = 0;
	status = newQsObject(&pSession);
	CHECK_QSTATUS();
	
	*ppReceiveSession = pSession;
	
	return QSTATUS_SUCCESS;
}

QSTATUS qmnntp::NntpReceiveSessionFactory::createUI(ReceiveSessionUI** ppUI)
{
	assert(ppUI);
	
	DECLARE_QSTATUS();
	
	NntpReceiveSessionUI* pUI = 0;
	status = newQsObject(&pUI);
	CHECK_QSTATUS();
	
	*ppUI = pUI;
	
	return QSTATUS_SUCCESS;
}


/****************************************************************************
 *
 * NntpSyncFilterCallback
 *
 */

qmnntp::NntpSyncFilterCallback::NntpSyncFilterCallback(
	Document* pDocument, Account* pAccount, NormalFolder* pFolder,
	Message* pMessage, unsigned int nSize, HWND hwnd, Profile* pProfile,
	MacroVariableHolder* pGlobalVariable, Nntp* pNntp,
	unsigned int nMessage, qs::string_ptr<qs::STRING>* pstrMessage,
	NntpReceiveSession::State* pState) :
	pDocument_(pDocument),
	pAccount_(pAccount),
	pFolder_(pFolder),
	pMessage_(pMessage),
	nSize_(nSize),
	hwnd_(hwnd),
	pProfile_(pProfile),
	pGlobalVariable_(pGlobalVariable),
	pNntp_(pNntp),
	nMessage_(nMessage),
	pstrMessage_(pstrMessage),
	pState_(pState),
	pmh_(0)
{
}

qmnntp::NntpSyncFilterCallback::~NntpSyncFilterCallback()
{
	delete pmh_;
}

QSTATUS qmnntp::NntpSyncFilterCallback::getMessage(unsigned int nFlag)
{
	DECLARE_QSTATUS();
	
	bool bDownload = false;
	unsigned int nMaxLine = 0xffffffff;
	switch (nFlag & Account::GETMESSAGEFLAG_METHOD_MASK) {
	case Account::GETMESSAGEFLAG_ALL:
	case Account::GETMESSAGEFLAG_TEXT:
	case Account::GETMESSAGEFLAG_HTML:
		bDownload = *pState_ != NntpReceiveSession::STATE_ALL;
		break;
	case Account::GETMESSAGEFLAG_HEADER:
		bDownload = *pState_ == NntpReceiveSession::STATE_NONE;
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
		status = pNntp_->getMessage(nMessage_,
			Nntp::GETMESSAGEFLAG_HEAD, &str);
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

const NormalFolder* qmnntp::NntpSyncFilterCallback::getFolder()
{
	return pFolder_;
}

QSTATUS qmnntp::NntpSyncFilterCallback::getMacroContext(
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
 * NntpMessageHolder
 *
 */

qmnntp::NntpMessageHolder::NntpMessageHolder(
	NntpSyncFilterCallback* pCallback, NormalFolder* pFolder,
	Message* pMessage, unsigned int nSize, QSTATUS* pstatus) :
	AbstractMessageHolder(pFolder, pMessage, -1, nSize, nSize, pstatus),
	pCallback_(pCallback)
{
	assert(pstatus);
	*pstatus = QSTATUS_SUCCESS;
}

qmnntp::NntpMessageHolder::~NntpMessageHolder()
{
}

QSTATUS qmnntp::NntpMessageHolder::getMessage(unsigned int nFlags,
	const WCHAR* pwszField, Message* pMessage)
{
	assert(pMessage == AbstractMessageHolder::getMessage());
	
	const WCHAR* pwszFields[] = {
		L"Subject",
		L"From",
		L"Date",
		L"Message-Id",
		L"References"
	};
	
	for (int n = 0; n < countof(pwszFields); ++n) {
		if (wcsicmp(pwszField, pwszFields[n]) == 0)
			return QSTATUS_SUCCESS;
	}
	
	return pCallback_->getMessage(nFlags);
}
