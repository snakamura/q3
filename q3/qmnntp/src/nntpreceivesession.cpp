/*
 * $Id$
 *
 * Copyright(C) 1998-2008 Satoshi Nakamura
 *
 */

#pragma warning(disable:4786)

#include <qmdocument.h>
#include <qmjunk.h>
#include <qmmessage.h>
#include <qmsecurity.h>

#include <qsthread.h>

#include "lastid.h"
#include "main.h"
#include "nntperror.h"
#include "nntpreceivesession.h"
#include "resourceinc.h"
#include "ui.h"
#include "util.h"

using namespace qmnntp;
using namespace qm;
using namespace qs;


#define HANDLE_ERROR() HANDLE_ERROR_(0)
#define HANDLE_ERROR_SSL() HANDLE_ERROR_(pCallback_->getSSLErrorMessage().get())
#define HANDLE_ERROR_(s) \
	do { \
		Util::reportError(pNntp_.get(), pSessionCallback_, pAccount_, \
			pSubAccount_, pFolder_, 0, pCallback_->getErrorMessage(), s); \
		return false; \
	} while (false) \


/****************************************************************************
 *
 * NntpReceiveSession
 *
 */

qmnntp::NntpReceiveSession::NntpReceiveSession(LastIdManager* pLastIdManager) :
	pDocument_(0),
	pAccount_(0),
	pSubAccount_(0),
	pFolder_(0),
	pLogger_(0),
	pSessionCallback_(0),
	pLastIdManager_(pLastIdManager),
	pLastIdList_(0)
{
}

qmnntp::NntpReceiveSession::~NntpReceiveSession()
{
}

bool qmnntp::NntpReceiveSession::init(Document* pDocument,
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
		pDocument->getSecurity(), pSessionCallback_));
	
	return true;
}

void qmnntp::NntpReceiveSession::term()
{
	Log log(pLogger_, L"qmnntp::NntpReceiveSession");
	
	clearLastIds();
	if (pLastIdList_) {
		if (!pLastIdList_->save())
			log.error(L"Failed to save last id list.");
	}
}

bool qmnntp::NntpReceiveSession::connect()
{
	assert(!pNntp_.get());
	
	Log log(pLogger_, L"qmnntp::NntpReceiveSession");
	log.debug(L"Connecting to the server...");
	
	pNntp_.reset(new Nntp(pSubAccount_->getTimeout(), pCallback_.get(),
		pCallback_.get(), pCallback_.get(), pLogger_));
	if (!pNntp_->connect(pSubAccount_->getHost(Account::HOST_RECEIVE),
		pSubAccount_->getPort(Account::HOST_RECEIVE),
		pSubAccount_->getSecure(Account::HOST_RECEIVE) == SubAccount::SECURE_SSL))
		HANDLE_ERROR_SSL();
	
	log.debug(L"Connected to the server.");
	
	pLastIdList_ = pLastIdManager_->get(pAccount_);
	
	return true;
}

void qmnntp::NntpReceiveSession::disconnect()
{
	assert(pNntp_.get());
	
	Log log(pLogger_, L"qmnntp::NntpReceiveSession");
	log.debug(L"Disconnecting from the server...");
	pNntp_->disconnect();
	log.debug(L"Disconnected from the server.");
}

bool qmnntp::NntpReceiveSession::isConnected()
{
	return true;
}

bool qmnntp::NntpReceiveSession::selectFolder(NormalFolder* pFolder,
											  unsigned int nFlags)
{
	assert(pFolder);
	assert(nFlags == 0);
	
	pCallback_->setMessage(IDS_SELECTGROUP);
	
	wstring_ptr wstrGroup(pFolder->getFullName());
	if (!pNntp_->group(wstrGroup.get()))
		HANDLE_ERROR();
	
	pFolder_ = pFolder;
	
	return true;
}

bool qmnntp::NntpReceiveSession::closeFolder()
{
	assert(pFolder_);
	
	pCallback_->setRange(0, 0);
	pCallback_->setPos(0);
	
	pFolder_ = 0;
	
	return true;
}

bool qmnntp::NntpReceiveSession::updateMessages()
{
	return true;
}

bool qmnntp::NntpReceiveSession::downloadMessages(const SyncFilterSet* pSyncFilterSet)
{
	unsigned int nStart = pNntp_->getFirst();
	{
		unsigned int nLastId = pLastIdList_->getLastId(pNntp_->getGroup());
		if (nLastId != 0)
			nStart = nLastId + 1;
		
		Lock<Account> lock(*pAccount_);
		if (!pFolder_->loadMessageHolders())
			return false;
		unsigned int nCount = pFolder_->getCount();
		if (nCount != 0) {
			unsigned int nId = pFolder_->getMessage(nCount - 1)->getId();
			nStart = QSMAX(nStart, nId + 1);
		}
		else if (nLastId == 0) {
			unsigned int nInitialFetchCount = pSubAccount_->getPropertyInt(
				L"Nntp", L"InitialFetchCount");
			if (pNntp_->getLast() > nInitialFetchCount - 1)
				nStart = QSMAX(nStart, pNntp_->getLast() - nInitialFetchCount + 1);
		}
	}
	
	pCallback_->setMessage(IDS_DOWNLOADMESSAGES);
	pSessionCallback_->setRange(0, pNntp_->getLast());
	pSessionCallback_->setPos(nStart);
	
	bool bUseXOver = pSubAccount_->getPropertyInt(L"Nntp", L"UseXOVER") != 0;
	
	MacroVariableHolder globalVariable;
	
	MessagePtrList listDownloaded;
	
	if (bUseXOver) {
		unsigned int nStep = pSubAccount_->getPropertyInt(L"Nntp", L"XOVERStep");
		
		for (unsigned int n = nStart; n <= pNntp_->getLast(); n += nStep) {
			std::auto_ptr<MessagesData> pData;
			if (!pNntp_->getMessagesData(n, n + nStep - 1, &pData))
				HANDLE_ERROR();
			
			bool bDownload = false;
			for (size_t m = 0; m < pData->getCount(); ++m) {
				if (pSessionCallback_->isCanceled(false))
					return true;
				if (bDownload || (n + m) % 10 == 0)
					pSessionCallback_->setPos(n + m);
				bDownload = false;
				
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
				
				StringBuffer<STRING> buf;
				for (int f = 0; f < countof(fields); ++f) {
					if (*fields[f].pszValue_) {
						buf.append(fields[f].pszName_);
						buf.append(": ");
						buf.append(fields[f].pszValue_);
						buf.append("\r\n");
					}
				}
				buf.append("\r\n");
				
				const CHAR* pszMessage = buf.getCharArray();
				size_t nLen = buf.getLength();
				unsigned int nFlags = MessageHolder::FLAG_INDEXONLY;
				
				xstring_size_ptr strMessage;
				bool bIgnore = false;
				if (pSyncFilterSet) {
					Message msg;
					if (!msg.create(buf.getCharArray(), buf.getLength(), Message::FLAG_TEMPORARY))
						return false;
					
					State state = STATE_NONE;
					NntpSyncFilterCallback callback(pDocument_, pAccount_,
						pSubAccount_, pFolder_, &msg, item.nBytes_, pProfile_,
						&globalVariable, pNntp_.get(), item.nId_, &strMessage, &state);
					const SyncFilter* pFilter = pSyncFilterSet->getFilter(&callback);
					if (pFilter) {
						const SyncFilter::ActionList& listAction = pFilter->getActions();
						for (SyncFilter::ActionList::const_iterator it = listAction.begin(); it != listAction.end(); ++it) {
							const SyncFilterAction* pAction = *it;
							const WCHAR* pwszName = pAction->getName();
							if (wcscmp(pwszName, L"download") == 0) {
								if (state != STATE_ALL) {
									if (!pNntp_->getMessage(item.nId_,
										Nntp::GETMESSAGEFLAG_ARTICLE, &strMessage, item.nBytes_))
										HANDLE_ERROR();
								}
								if (strMessage.get()) {
									pszMessage = strMessage.get();
									nLen = strMessage.size();
									nFlags = 0;
								}
								bDownload = true;
							}
							else if (wcscmp(pwszName, L"ignore") == 0) {
								bIgnore = true;
							}
						}
					}
				}
				
				if (!bIgnore) {
					if (!storeMessage(pszMessage, nLen, item.nId_,
						nFlags, item.nBytes_, &listDownloaded))
						return false;
				}
				
				pLastIdList_->setLastId(pNntp_->getGroup(), item.nId_);
			}
		}
		pSessionCallback_->setPos(pNntp_->getLast());
	}
	else {
		for (unsigned int n = nStart; n <= pNntp_->getLast(); ++n) {
			if (pSessionCallback_->isCanceled(false))
				return true;
			pSessionCallback_->setPos(n);
			
			xstring_size_ptr strMessage;
			if (!pNntp_->getMessage(n, Nntp::GETMESSAGEFLAG_HEAD, &strMessage, -1))
				HANDLE_ERROR();
			
			if (strMessage.get()) {
				// TODO
				// Process sync filter ?
				
				if (!storeMessage(strMessage.get(), strMessage.size(),
					n, MessageHolder::FLAG_HEADERONLY, -1, &listDownloaded))
					return false;
				
				pLastIdList_->setLastId(pNntp_->getGroup(), n);
			}
		}
	}
	
	if (!pAccount_->saveMessages(false)) {
		Util::reportError(0, pSessionCallback_, pAccount_,
			pSubAccount_, pFolder_, NNTPERROR_SAVE, 0, 0);
		return false;
	}
	
	if (!listDownloaded.empty()) {
		bool bJunkFilter = pSubAccount_->isJunkFilterEnabled();
		bool bApplyRules = (pSubAccount_->getAutoApplyRules() & SubAccount::AUTOAPPLYRULES_NEW) != 0;
		
		if (bJunkFilter) {
			if (!applyJunkFilter(listDownloaded))
				return false;
		}
		
		if (bApplyRules || bJunkFilter) {
			if (!applyRules(&listDownloaded, bJunkFilter, !bApplyRules))
				Util::reportError(0, pSessionCallback_, pAccount_,
					pSubAccount_, pFolder_, NNTPERROR_APPLYRULES, 0, 0);
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
	}
	
	return true;
}

bool qmnntp::NntpReceiveSession::applyOfflineJobs()
{
	return downloadReservedMessages();
}

bool qmnntp::NntpReceiveSession::downloadReservedMessages()
{
	Account::FolderList l(pAccount_->getFolders());
	std::sort(l.begin(), l.end(), FolderLess());
	
	unsigned int nCount = 0;
	for (Account::FolderList::const_iterator it = l.begin(); it != l.end(); ++it) {
		Folder* pFolder = *it;
		if (pFolder->getType() == Folder::TYPE_NORMAL &&
			pFolder->getFlags() & Folder::FLAG_SYNCABLE)
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
			pFolder->getFlags() & Folder::FLAG_SYNCABLE) {
			if (!downloadReservedMessages(static_cast<NormalFolder*>(pFolder), &nPos))
				return false;
		}
	}
	return true;
}

bool qmnntp::NntpReceiveSession::downloadReservedMessages(NormalFolder* pFolder,
														  unsigned int* pnPos)
{
	assert(pFolder);
	assert(pnPos);
	
	if (pFolder->getDownloadCount() == 0)
		return true;
	
	wstring_ptr wstrGroup(pFolder->getFullName());
	if (!pNntp_->group(wstrGroup.get()))
		HANDLE_ERROR();
	
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
			
			xstring_size_ptr strMessage;
			if (!pNntp_->getMessage(mpl->getId(),
				Nntp::GETMESSAGEFLAG_ARTICLE, &strMessage, mpl->getSize()))
				HANDLE_ERROR();
			
			unsigned int nMask = MessageHolder::FLAG_DOWNLOAD | MessageHolder::FLAG_DOWNLOADTEXT;
			if (strMessage.get()) {
				if (!pAccount_->updateMessage(mpl, strMessage.get(), strMessage.size(), 0))
					return false;
				nMask |=  MessageHolder::FLAG_SEEN | MessageHolder::FLAG_PARTIAL_MASK;
			}
			mpl->setFlags(0, nMask);
		}
	}
	
	return true;
}

void qmnntp::NntpReceiveSession::clearLastIds()
{
	if (!pLastIdList_)
		return;
	
	typedef std::vector<const WCHAR*> NameList;
	NameList listRemove;
	
	Lock<LastIdList> lock(*pLastIdList_);
	
	const LastIdList::IdList& listId = pLastIdList_->getList();
	for (LastIdList::IdList::const_iterator it = listId.begin(); it != listId.end(); ++it) {
		if (!pAccount_->getFolder((*it).first))
			listRemove.push_back((*it).first);
	}
	
	for (NameList::const_iterator it = listRemove.begin(); it != listRemove.end(); ++it)
		pLastIdList_->removeLastId(*it);
}

bool qmnntp::NntpReceiveSession::storeMessage(const CHAR* pszMessage,
											  size_t nLen,
											  unsigned int nId,
											  unsigned int nFlags,
											  unsigned int nSize,
											  MessagePtrList* pListDownloaded)
{
	Lock<Account> lock(*pAccount_);
	
	unsigned int nStoreFlags = Account::OPFLAG_BACKGROUND;
	if (nFlags == MessageHolder::FLAG_INDEXONLY)
		nStoreFlags |= Account::STOREFLAG_INDEXONLY;
	MessageHolder* pmh = pAccount_->storeMessage(pFolder_,
		pszMessage, nLen, 0, nId, nFlags, 0, nSize, nStoreFlags, 0);
	if (!pmh)
		return false;
	
	pListDownloaded->push_back(MessagePtr(pmh));
	
	return true;
}

bool qmnntp::NntpReceiveSession::applyJunkFilter(const qm::MessagePtrList& l) const
{
	JunkFilter* pJunkFilter = pDocument_->getJunkFilter();
	if (!pJunkFilter)
		return true;
	
	if (pJunkFilter->getFlags() & JunkFilter::FLAG_AUTOLEARN) {
		pCallback_->setMessage(IDS_FILTERJUNK);
		pSessionCallback_->setRange(0, l.size());
		pSessionCallback_->setPos(0);
		
		pAccount_->prepareGetMessage(pFolder_);
		
		for (MessagePtrList::size_type n = 0; n < l.size(); ++n) {
			Message msg;
			bool bProcess = false;
			bool bSeen = false;
			{
				MessagePtrLock mpl(l[n]);
				if (mpl) {
					bSeen = pAccount_->isSeen(mpl);
					unsigned int nFlags = pJunkFilter->isScanAttachment() ?
						Account::GMF_ALL : Account::GMF_TEXT;
					bProcess = mpl->getMessage(nFlags, 0, SECURITYMODE_NONE, &msg);
				}
			}
			unsigned int nOperation = 0;
			if (bProcess) {
				if (bSeen) {
					nOperation = JunkFilter::OPERATION_ADDCLEAN;
				}
				else {
					float fScore = pJunkFilter->getScore(msg);
					if (fScore < 0)
						Util::reportError(0, pSessionCallback_, pAccount_,
							pSubAccount_, pFolder_, NNTPERROR_FILTERJUNK, 0, 0);
					else if (fScore > pJunkFilter->getThresholdScore())
						nOperation = JunkFilter::OPERATION_ADDJUNK;
					else
						nOperation = JunkFilter::OPERATION_ADDCLEAN;
				}
			}
			if (nOperation != 0) {
				if (!pJunkFilter->manage(msg, nOperation))
					Util::reportError(0, pSessionCallback_, pAccount_,
						pSubAccount_, pFolder_, NNTPERROR_MANAGEJUNK, 0, 0);
			}
			
			pSessionCallback_->setPos(n);
		}
	}
	
	return true;
}

bool qmnntp::NntpReceiveSession::applyRules(MessagePtrList* pList,
											bool bJunkFilter,
											bool bJunkFilterOnly)
{
	unsigned int nFlags = (bJunkFilter ? RuleManager::AUTOFLAG_JUNKFILTER : 0) |
		(bJunkFilterOnly ? RuleManager::AUTOFLAG_JUNKFILTERONLY : 0);
	RuleManager* pRuleManager = pDocument_->getRuleManager();
	DefaultReceiveSessionRuleCallback callback(pSessionCallback_);
	return pRuleManager->applyAuto(pFolder_, pList,
		pSubAccount_, pDocument_, pProfile_, nFlags, &callback);
}


/****************************************************************************
 *
 * NntpReceiveSession::CallbackImpl
 *
 */

qmnntp::NntpReceiveSession::CallbackImpl::CallbackImpl(SubAccount* pSubAccount,
													   const Security* pSecurity,
													   ReceiveSessionCallback* pSessionCallback) :
	DefaultCallback(pSubAccount, pSessionCallback, pSecurity),
	pSessionCallback_(pSessionCallback)
{
}

qmnntp::NntpReceiveSession::CallbackImpl::~CallbackImpl()
{
}

void qmnntp::NntpReceiveSession::CallbackImpl::setMessage(UINT nId)
{
	wstring_ptr wstrMessage(loadString(getResourceHandle(), nId));
	pSessionCallback_->setMessage(wstrMessage.get());
}

bool qmnntp::NntpReceiveSession::CallbackImpl::isCanceled(bool bForce) const
{
	return pSessionCallback_->isCanceled(bForce);
}

void qmnntp::NntpReceiveSession::CallbackImpl::initialize()
{
	setMessage(IDS_INITIALIZE);
}

void qmnntp::NntpReceiveSession::CallbackImpl::lookup()
{
	setMessage(IDS_LOOKUP);
}

void qmnntp::NntpReceiveSession::CallbackImpl::connecting()
{
	setMessage(IDS_CONNECTING);
}

void qmnntp::NntpReceiveSession::CallbackImpl::connected()
{
	setMessage(IDS_CONNECTED);
}

void qmnntp::NntpReceiveSession::CallbackImpl::authenticating()
{
	setMessage(IDS_AUTHENTICATING);
}

void qmnntp::NntpReceiveSession::CallbackImpl::setRange(size_t nMin,
														size_t nMax)
{
	pSessionCallback_->setSubRange(nMin, nMax);
}

void qmnntp::NntpReceiveSession::CallbackImpl::setPos(size_t nPos)
{
	pSessionCallback_->setSubPos(nPos);
}


/****************************************************************************
 *
 * NntpReceiveSessionUI
 *
 */

qmnntp::NntpReceiveSessionUI::NntpReceiveSessionUI()
{
}

qmnntp::NntpReceiveSessionUI::~NntpReceiveSessionUI()
{
}

const WCHAR* qmnntp::NntpReceiveSessionUI::getClass()
{
	return L"news";
}

wstring_ptr qmnntp::NntpReceiveSessionUI::getDisplayName()
{
	return loadString(getResourceHandle(), IDS_NNTP);
}

short qmnntp::NntpReceiveSessionUI::getDefaultPort(bool bSecure)
{
	return bSecure ? 563 : 119;
}

bool qmnntp::NntpReceiveSessionUI::isSupported(Support support)
{
	return support != SUPPORT_STARTTLS;
}

std::auto_ptr<PropertyPage> qmnntp::NntpReceiveSessionUI::createPropertyPage(SubAccount* pSubAccount)
{
	return std::auto_ptr<PropertyPage>(new ReceivePage(pSubAccount));
}

void qmnntp::NntpReceiveSessionUI::subscribe(Document* pDocument,
											 Account* pAccount,
											 Folder* pFolder,
											 PasswordCallback* pPasswordCallback,
											 HWND hwnd,
											 void* pParam)
{
	SubscribeDialog dialog(pDocument, pAccount, pPasswordCallback);
	if (dialog.doModal(hwnd) != IDOK)
		return;
	
	NormalFolder* pNewFolder = pAccount->createNormalFolder(
		dialog.getGroup(), 0, false, true);
	if (!pNewFolder) {
		messageBox(getResourceHandle(), IDS_ERROR_SUBSCRIBE, MB_OK | MB_ICONERROR, hwnd);
		return;
	}
}

bool qmnntp::NntpReceiveSessionUI::canSubscribe(Account* pAccount,
												Folder* pFolder)
{
	return true;
}

wstring_ptr qmnntp::NntpReceiveSessionUI::getSubscribeText()
{
	return loadString(getResourceHandle(), IDS_SUBSCRIBE);
}


/****************************************************************************
 *
 * NntpReceiveSessionFactory
 *
 */

NntpReceiveSessionFactory qmnntp::NntpReceiveSessionFactory::factory__;

qmnntp::NntpReceiveSessionFactory::NntpReceiveSessionFactory()
{
	pLastIdManager_.reset(new LastIdManager());
	
	registerFactory(L"nntp", this);
}

qmnntp::NntpReceiveSessionFactory::~NntpReceiveSessionFactory()
{
	unregisterFactory(L"nntp");
}

std::auto_ptr<ReceiveSession> qmnntp::NntpReceiveSessionFactory::createSession()
{
	return std::auto_ptr<ReceiveSession>(new NntpReceiveSession(pLastIdManager_.get()));
}

std::auto_ptr<ReceiveSessionUI> qmnntp::NntpReceiveSessionFactory::createUI()
{
	return std::auto_ptr<ReceiveSessionUI>(new NntpReceiveSessionUI());
}


/****************************************************************************
 *
 * NntpSyncFilterCallback
 *
 */

qmnntp::NntpSyncFilterCallback::NntpSyncFilterCallback(Document* pDocument,
													   Account* pAccount,
													   SubAccount* pSubAccount,
													   NormalFolder* pFolder,
													   Message* pMessage,
													   unsigned int nSize,
													   Profile* pProfile,
													   MacroVariableHolder* pGlobalVariable,
													   Nntp* pNntp,
													   unsigned int nMessage,
													   qs::xstring_size_ptr* pstrMessage,
													   NntpReceiveSession::State* pState) :
	pDocument_(pDocument),
	pAccount_(pAccount),
	pSubAccount_(pSubAccount),
	pFolder_(pFolder),
	pMessage_(pMessage),
	nSize_(nSize),
	pProfile_(pProfile),
	pGlobalVariable_(pGlobalVariable),
	pNntp_(pNntp),
	nMessage_(nMessage),
	pstrMessage_(pstrMessage),
	pState_(pState)
{
}

qmnntp::NntpSyncFilterCallback::~NntpSyncFilterCallback()
{
}

bool qmnntp::NntpSyncFilterCallback::getMessage(unsigned int nFlag)
{
	bool bDownload = false;
	switch (nFlag & Account::GMF_METHOD_MASK) {
	case Account::GMF_ALL:
	case Account::GMF_TEXT:
	case Account::GMF_HTML:
		bDownload = *pState_ != NntpReceiveSession::STATE_ALL;
		break;
	case Account::GMF_HEADER:
		bDownload = *pState_ == NntpReceiveSession::STATE_NONE;
		break;
	case Account::GMF_POSSIBLE:
		break;
	default:
		assert(false);
		return false;
	}
	
	if (bDownload) {
		xstring_size_ptr& str = *pstrMessage_;
		
		str.reset(0, -1);
		if (pNntp_->getMessage(nMessage_, Nntp::GETMESSAGEFLAG_HEAD, &str, nSize_))
			return false;
		
		if (!pMessage_->createHeader(str.get(), str.size()))
			return false;
	}
	
	return true;
}

const NormalFolder* qmnntp::NntpSyncFilterCallback::getFolder()
{
	return pFolder_;
}

std::auto_ptr<MacroContext> qmnntp::NntpSyncFilterCallback::getMacroContext()
{
	if (!pmh_.get())
		pmh_.reset(new NntpMessageHolder(this, pFolder_, pMessage_, nSize_));
	
	return std::auto_ptr<MacroContext>(new MacroContext(pmh_.get(),
		pMessage_, pAccount_, pSubAccount_, MessageHolderList(), pFolder_,
		pDocument_, 0, 0, pProfile_, 0, MacroContext::FLAG_NONE,
		SECURITYMODE_NONE, 0, pGlobalVariable_));
}


/****************************************************************************
 *
 * NntpMessageHolder
 *
 */

qmnntp::NntpMessageHolder::NntpMessageHolder(NntpSyncFilterCallback* pCallback,
											 NormalFolder* pFolder,
											 Message* pMessage,
											 unsigned int nSize) :
	AbstractMessageHolder(pFolder, pMessage, -1, nSize, nSize),
	pCallback_(pCallback)
{
}

qmnntp::NntpMessageHolder::~NntpMessageHolder()
{
}

bool qmnntp::NntpMessageHolder::getMessage(unsigned int nFlags,
										   const WCHAR* pwszField,
										   unsigned int nSecurityMode,
										   Message* pMessage)
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
		if (_wcsicmp(pwszField, pwszFields[n]) == 0)
			return true;
	}
	
	return pCallback_->getMessage(nFlags);
}
