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
#include <qmjunk.h>
#include <qmmessage.h>
#include <qmmessageholder.h>

#include <qsthread.h>

#include <algorithm>
#include <cstdio>
#include <functional>

#include "imap4.h"
#include "imap4driver.h"
#include "imap4error.h"
#include "imap4receivesession.h"
#include "main.h"
#include "offlinejob.h"
#include "option.h"
#include "processhook.h"
#include "resourceinc.h"
#include "ui.h"

using namespace qmimap4;
using namespace qm;
using namespace qs;

#define HANDLE_ERROR() \
	do { \
		reportError(pImap4_.get(), 0); \
		return false; \
	} while (false) \


/****************************************************************************
 *
 * Imap4ReceiveSession
 *
 */

qmimap4::Imap4ReceiveSession::Imap4ReceiveSession() :
	pDocument_(0),
	pAccount_(0),
	pSubAccount_(0),
	pFolder_(0),
	pLogger_(0),
	pSessionCallback_(0),
	pProcessHook_(0),
	nExists_(0),
	nUidValidity_(0),
	bReadOnly_(false),
	nUidStart_(0),
	nIdStart_(0)
{
}

qmimap4::Imap4ReceiveSession::~Imap4ReceiveSession()
{
}

bool qmimap4::Imap4ReceiveSession::init(Document* pDocument,
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
	
	pCallback_.reset(new CallbackImpl(this, pSubAccount_,
		pDocument->getSecurity(), pSessionCallback_));
	
	return true;
}

void qmimap4::Imap4ReceiveSession::term()
{
}

bool qmimap4::Imap4ReceiveSession::connect()
{
	assert(!pImap4_.get());
	
	Log log(pLogger_, L"qmimap4::Imap4ReceiveSession");
	log.debug(L"Connecting to the server...");
	
	pImap4_.reset(new Imap4(pSubAccount_->getTimeout(), pCallback_.get(),
		pCallback_.get(), pCallback_.get(), pLogger_));
	
	Imap4::Secure secure = Util::getSecure(pSubAccount_);
	if (!pImap4_->connect(pSubAccount_->getHost(Account::HOST_RECEIVE),
		pSubAccount_->getPort(Account::HOST_RECEIVE), secure))
		HANDLE_ERROR();
	
	log.debug(L"Connected to the server.");
	
	return true;
}

void qmimap4::Imap4ReceiveSession::disconnect()
{
	assert(pImap4_.get());
	
	Log log(pLogger_, L"qmimap4::Imap4ReceiveSession");
	log.debug(L"Disconnecting from the server...");
	
	pImap4_->disconnect();
	
	log.debug(L"Disconnected from the server.");
}

bool qmimap4::Imap4ReceiveSession::isConnected()
{
	return pImap4_->checkConnection();
}

bool qmimap4::Imap4ReceiveSession::selectFolder(NormalFolder* pFolder,
												unsigned int nFlags)
{
	assert(pFolder);
	
	pCallback_->setMessage(IDS_SELECTFOLDER);
	
	wstring_ptr wstrName(Util::getFolderName(pFolder));
	
	pFolder_ = 0;
	
	if (!pImap4_->select(wstrName.get()))
		HANDLE_ERROR();
	
	if (nExists_ != 0) {
		if (nFlags & SELECTFLAG_EMPTY) {
			ContinuousRange range(1, nExists_, false);
			Flags flags(Imap4::FLAG_DELETED);
			Flags mask(Imap4::FLAG_DELETED);
			if (!pImap4_->setFlags(range, flags, mask))
				HANDLE_ERROR();
		}
		if (nFlags & SELECTFLAG_EXPUNGE) {
			if (!pImap4_->close() || !pImap4_->select(wstrName.get()))
				HANDLE_ERROR();
		}
	}
	
	pFolder_ = pFolder;
	
	return true;
}

bool qmimap4::Imap4ReceiveSession::closeFolder()
{
	assert(pFolder_);
	
	pCallback_->setRange(0, 0);
	pCallback_->setPos(0);
	
	if (pSubAccount_->getPropertyInt(L"Imap4", L"CloseFolder")) {
		pCallback_->setMessage(IDS_CLOSEFOLDER);
		if (!pImap4_->close())
			HANDLE_ERROR();
		
		Lock<Account> lock(*pAccount_);
		if (pFolder_->getDeletedCount() != 0) {
			const MessageHolderList& l = pFolder_->getMessages();
			NormalFolder::MessageInfoList listMessageInfo;
			listMessageInfo.reserve(l.size());
			for (MessageHolderList::const_iterator it = l.begin(); it != l.end(); ++it) {
				MessageHolder* pmh = *it;
				if (!pmh->isFlag(MessageHolder::FLAG_DELETED)) {
					NormalFolder::MessageInfo info = {
						pmh->getId(),
						pmh->getFlags(),
						0
					};
					listMessageInfo.push_back(info);
				}
			}
			if (!pFolder_->updateMessageInfos(listMessageInfo, false, 0))
				return false;
		}
	}
	
	pFolder_ = 0;
	
	return true;
}

bool qmimap4::Imap4ReceiveSession::updateMessages()
{
	assert(pFolder_);
	
	unsigned int nDownloadCount = pFolder_->getDownloadCount();
	pCallback_->setMessage(IDS_DOWNLOADRESERVEDMESSAGES);
	pSessionCallback_->setRange(0, nDownloadCount);
	if (!downloadReservedMessages(pFolder_))
		return false;
	
	pCallback_->setMessage(IDS_UPDATEMESSAGES);
	pSessionCallback_->setRange(0, nExists_);
	pSessionCallback_->setPos(0);
	
	nUidStart_ = 0;
	nIdStart_ = 0;
	if (nUidValidity_ == pFolder_->getValidity()) {
		Lock<Account> lock(*pAccount_);
		unsigned int n = pFolder_->getCount();
		if (n != 0) {
			MessageHolder* pmh = pFolder_->getMessage(n - 1);
			nUidStart_ = pmh->getId();
		}
	}
	else {
		if (!pFolder_->setValidity(nUidValidity_))
			return false;
	}
	
	if (nUidStart_ != 0) {
		struct UpdateFlagsProcessHook : public ProcessHook
		{
			typedef NormalFolder::MessageInfoList MessageInfoList;
			
			UpdateFlagsProcessHook(unsigned int nUidStart,
								   ReceiveSessionCallback* pSessionCallback) :
				nUidStart_(nUidStart),
				pSessionCallback_(pSessionCallback),
				nLastId_(0)
			{
			}
			
			virtual ~UpdateFlagsProcessHook()
			{
				std::for_each(listMessageInfo_.begin(), listMessageInfo_.end(),
					unary_compose_f_gx(qs::string_free<WSTRING>(),
						mem_data_ref(&NormalFolder::MessageInfo::wstrLabel_)));
			}
			
			virtual Result processFetchResponse(ResponseFetch* pFetch)
			{
				unsigned int nUid = pFetch->getUid();
				unsigned int nFlags = 0;
				wstring_ptr wstrLabel;
				
				int nCount = 0;
				
				const ResponseFetch::FetchDataList& l = pFetch->getFetchDataList();
				for (ResponseFetch::FetchDataList::const_iterator it = l.begin(); it != l.end(); ++it) {
					switch ((*it)->getType()) {
					case FetchData::TYPE_FLAGS:
						nFlags = Util::getMessageFlagsFromImap4Flags(
							static_cast<FetchDataFlags*>(*it)->getSystemFlags(),
							static_cast<FetchDataFlags*>(*it)->getCustomFlags());
						wstrLabel = Util::getLabelFromImap4Flags(
							static_cast<FetchDataFlags*>(*it)->getCustomFlags());
						++nCount;
						break;
					default:
						break;
					}
				}
				
				if (nCount == 1 && nUid != -1 && nUid <= nUidStart_) {
					NormalFolder::MessageInfo info = {
						nUid,
						nFlags,
						wstrLabel.get()
					};
					listMessageInfo_.push_back(info);
					wstrLabel.release();
					
					unsigned int nPos = pFetch->getNumber();
					if (nPos % 10 == 0)
						pSessionCallback_->setPos(nPos);
					
					nLastId_ = pFetch->getNumber();
				}
				
				return RESULT_PROCESSED;
			}
			
			unsigned int nUidStart_;
			ReceiveSessionCallback* pSessionCallback_;
			MessageInfoList listMessageInfo_;
			unsigned int nLastId_;
		} hook(nUidStart_, pSessionCallback_);
		
		if (nExists_ != 0) {
			Hook h(this, &hook);
			ContinuousRange range(1, nUidStart_, true);
			if (!pImap4_->getFlags(range)) {
				// Because some servers return NO response when I try
				// getting flags of UID that doesn't exist, I ignore this.
				if ((pImap4_->getLastError() & Imap4::IMAP4_ERROR_MASK_LOWLEVEL) != Imap4::IMAP4_ERROR_RESPONSE)
					HANDLE_ERROR();
			}
			pSessionCallback_->setPos(hook.nLastId_);
		}
		
		bool bClear = false;
		if (!pFolder_->updateMessageInfos(hook.listMessageInfo_, true, &bClear))
			return false;
		if (bClear)
			nUidStart_ = 0;
		else
			nIdStart_ = hook.nLastId_;
	}
	
	pSessionCallback_->setPos(nIdStart_);
	
	++nUidStart_;
	
	return true;
}

bool qmimap4::Imap4ReceiveSession::downloadMessages(const SyncFilterSet* pSyncFilterSet)
{
	int nFetchCount = pSubAccount_->getPropertyInt(L"Imap4", L"FetchCount");
	int nOption = pSubAccount_->getPropertyInt(L"Imap4", L"Option");
	
	wstring_ptr wstrAdditionalFields(pSubAccount_->getPropertyString(L"Imap4", L"AdditionalFields"));
	string_ptr strAdditionalFields(wcs2mbs(wstrAdditionalFields.get()));
	
	typedef std::vector<unsigned long> UidList;
	UidList listMakeSeen;
	UidList listMakeDeleted;
	
	typedef std::vector<FetchDataBodyStructure*> BodyStructureList;
	BodyStructureList listBodyStructure;
	container_deleter<BodyStructureList> deleter(listBodyStructure);
	
	MessageDataList listMessageData;
	listMessageData.reserve(nExists_ - nIdStart_);
	
	struct GetMessageDataProcessHook : public ProcessHook
	{
		typedef std::vector<unsigned long> UidList;
		typedef std::vector<FetchDataBodyStructure*> BodyStructureList;
		
		GetMessageDataProcessHook(Document* pDocument,
								  Account* pAccount,
								  SubAccount* pSubAccount,
								  NormalFolder* pFolder,
								  Profile* pProfile,
								  ReceiveSessionCallback* pSessionCallback,
								  const SyncFilterSet* pFilterSet,
								  unsigned int nOption,
								  unsigned long nUidStart,
								  MessageDataList& listMessageData,
								  UidList& listMakeSeen,
								  UidList& listMakeDeleted,
								  BodyStructureList& listBodyStructure,
								  Imap4* pImap4,
								  MacroVariableHolder* pGlobalVariable,
								  Imap4ReceiveSession* pSession) :
			pDocument_(pDocument),
			pAccount_(pAccount),
			pSubAccount_(pSubAccount),
			pFolder_(pFolder),
			pProfile_(pProfile),
			pSessionCallback_(pSessionCallback),
			pFilterSet_(pFilterSet),
			nOption_(nOption),
			nUidStart_(nUidStart),
			listMessageData_(listMessageData),
			listMakeSeen_(listMakeSeen),
			listMakeDeleted_(listMakeDeleted),
			listBodyStructure_(listBodyStructure),
			pImap4_(pImap4),
			pGlobalVariable_(pGlobalVariable),
			pSession_(pSession)
		{
		}
		
		virtual Result processFetchResponse(ResponseFetch* pFetch)
		{
			unsigned long nUid = pFetch->getUid();
			unsigned int nFlags = 0;
			wstring_ptr wstrLabel;
			unsigned long nSize = -1;
			FetchDataBodyStructure* pBodyStructure = 0;
			string_ptr strEnvelope;
			std::pair<const CHAR*, size_t> header;
			
			int nCount = 0;
			
			const ResponseFetch::FetchDataList& l = pFetch->getFetchDataList();
			for (ResponseFetch::FetchDataList::const_iterator it = l.begin(); it != l.end(); ++it) {
				switch ((*it)->getType()) {
				case FetchData::TYPE_ENVELOPE:
					if (nOption_ & OPTION_USEENVELOPE) {
						strEnvelope = Util::getMessageFromEnvelope(
							static_cast<FetchDataEnvelope*>(*it));
						if (!strEnvelope.get())
							return RESULT_ERROR;
						++nCount;
					}
					break;
				case FetchData::TYPE_FLAGS:
					nFlags = Util::getMessageFlagsFromImap4Flags(
						static_cast<FetchDataFlags*>(*it)->getSystemFlags(),
						static_cast<FetchDataFlags*>(*it)->getCustomFlags());
					wstrLabel = Util::getLabelFromImap4Flags(
						static_cast<FetchDataFlags*>(*it)->getCustomFlags());
					++nCount;
					break;
				case FetchData::TYPE_BODY:
					header = static_cast<FetchDataBody*>(*it)->getContent().get();
					++nCount;
					break;
				case FetchData::TYPE_SIZE:
					nSize = static_cast<FetchDataSize*>(*it)->getSize();
					++nCount;
					break;
				case FetchData::TYPE_BODYSTRUCTURE:
					if (nOption_ & OPTION_USEBODYSTRUCTUREALWAYS) {
						pBodyStructure = static_cast<FetchDataBodyStructure*>(*it);
						++nCount;
					}
					break;
				default:
					break;
				}
			}
			
			const int nNeedCount = 3 + (nOption_ & OPTION_USEENVELOPE ? 1 : 0) +
				(nOption_ & OPTION_USEBODYSTRUCTUREALWAYS ? 1 : 0);
			if (nCount != nNeedCount || nUid == -1 || nUid < nUidStart_)
				return RESULT_UNPROCESSED;
			
			unsigned int nPos = pFetch->getNumber();
			if (nPos % 10 == 0)
				pSessionCallback_->setPos(nPos);
			
			XStringBuffer<XSTRING> buf;
			if (strEnvelope.get()) {
				if (!buf.append(strEnvelope.get()))
					return RESULT_ERROR;
			}
			if (pBodyStructure) {
				// Content-Type's parameter of multipart and Content-Disposition is
				// extension data of BODYSTRUCTURE. Thus these data may not be returned.
				// To get these headers always, don't get them from BODYSTURCTURE response,
				// use BODY[HEADER (Content-Type Content-Disposition)] instead.
				string_ptr strBodyStructure(Util::getHeaderFromBodyStructure(pBodyStructure, false));
				if (!buf.append(strBodyStructure.get()))
					return RESULT_ERROR;
			}
			if (!buf.append(header.first, header.second))
				return RESULT_ERROR;
			
			Message msg;
			if (!msg.create(buf.getCharArray(),
				buf.getLength(), Message::FLAG_TEMPORARY))
				return RESULT_ERROR;
			
			bool bSelf = pSubAccount_->isSelf(msg);
			if (bSelf) {
				bool bSeen = nFlags & MessageHolder::FLAG_SEEN;
				nFlags |= MessageHolder::FLAG_SEEN | MessageHolder::FLAG_SENT;
				if (!bSeen)
					listMakeSeen_.push_back(nUid);
			}
			
			unsigned long nTextSize = pBodyStructure ?
				Util::getTextSizeFromBodyStructure(pBodyStructure) : nSize;
			
			enum {
				DOWNLOAD_NONE,
				DOWNLOAD_HEADER,
				DOWNLOAD_TEXT,
				DOWNLOAD_HTML,
				DOWNLOAD_ALL
			} download = DOWNLOAD_NONE;
			
			const SyncFilter* pFilter = 0;
			if (pFilterSet_) {
				Imap4SyncFilterCallback callback(pDocument_, pAccount_,
					pFolder_, &msg, nUid, nSize, nTextSize,
					pProfile_, pGlobalVariable_, pSession_);
				pFilter = pFilterSet_->getFilter(&callback);
				if (pFilter) {
					const SyncFilter::ActionList& listAction = pFilter->getActions();
					for (SyncFilter::ActionList::const_iterator it = listAction.begin(); it != listAction.end(); ++it) {
						const SyncFilterAction* pAction = *it;
						const WCHAR* pwszName = pAction->getName();
						if (wcscmp(pwszName, L"download") == 0) {
							const WCHAR* pwszType = pAction->getParam(L"type");
							if (wcscmp(pwszType, L"all") == 0)
								download = DOWNLOAD_ALL;
							else if (wcscmp(pwszType, L"text") == 0)
								download = DOWNLOAD_TEXT;
							else if (wcscmp(pwszType, L"html") == 0)
								download = DOWNLOAD_HTML;
							else if (wcscmp(pwszType, L"header") == 0)
								download = DOWNLOAD_HEADER;
						}
						else if (wcscmp(pwszName, L"delete") == 0) {
							nFlags |= MessageHolder::FLAG_DELETED;
							listMakeDeleted_.push_back(nUid);
						}
					}
				}
			}
			
			if (download == DOWNLOAD_TEXT || download == DOWNLOAD_HTML) {
				bool bAll = false;
				if (pBodyStructure)
					bAll = !Util::hasAttachmentPart(pBodyStructure);
				else
					bAll = !msg.isMultipart() && !msg.isAttachment();
				if (bAll)
					download = DOWNLOAD_ALL;
			}
			
			switch (download) {
			case DOWNLOAD_ALL:
			case DOWNLOAD_HTML:
				nFlags |= MessageHolder::FLAG_DOWNLOAD;
				break;
			case DOWNLOAD_TEXT:
				nFlags |= MessageHolder::FLAG_DOWNLOADTEXT;
				break;
			case DOWNLOAD_HEADER:
			case DOWNLOAD_NONE:
				break;
			default:
				assert(false);
				break;
			}
			nFlags |= MessageHolder::FLAG_INDEXONLY;
			
			Lock<Account> lock(*pAccount_);
			
			MessageHolder* pmh = pAccount_->storeMessage(pFolder_, buf.getCharArray(),
				buf.getLength(), &msg, nUid, nFlags, wstrLabel.get(), nSize, true);
			if (!pmh)
				return RESULT_ERROR;
			
			switch (download) {
			case DOWNLOAD_ALL:
				listMessageData_.push_back(MessageData(pmh, MessageData::TYPE_ALL, 0));
				break;
			case DOWNLOAD_TEXT:
			case DOWNLOAD_HTML:
				{
					MessageData::Type type = download == DOWNLOAD_TEXT ?
						MessageData::TYPE_TEXT : MessageData::TYPE_HTML;
					listMessageData_.push_back(MessageData(pmh, type, pBodyStructure));
					if (pBodyStructure) {
						listBodyStructure_.push_back(pBodyStructure);
						pFetch->detach(pBodyStructure);
					}
				}
				break;
			case DOWNLOAD_HEADER:
				listMessageData_.push_back(MessageData(pmh, MessageData::TYPE_HEADER, 0));
				break;
			case DOWNLOAD_NONE:
				listMessageData_.push_back(MessageData(pmh, MessageData::TYPE_NONE, 0));
				break;
			default:
				assert(false);
				break;
			}
			
			return RESULT_PROCESSED;
		}
		
		Document* pDocument_;
		Account* pAccount_;
		SubAccount* pSubAccount_;
		NormalFolder* pFolder_;
		Profile* pProfile_;
		ReceiveSessionCallback* pSessionCallback_;
		const SyncFilterSet* pFilterSet_;
		unsigned int nOption_;
		unsigned long nUidStart_;
		MessageDataList& listMessageData_;
		UidList& listMakeSeen_;
		UidList& listMakeDeleted_;
		BodyStructureList& listBodyStructure_;
		Imap4* pImap4_;
		MacroVariableHolder* pGlobalVariable_;
		Imap4ReceiveSession* pSession_;
	};
	
	if (nIdStart_ < nExists_) {
		pCallback_->setMessage(IDS_DOWNLOADMESSAGESDATA);
		
		MacroVariableHolder globalVariable;
		GetMessageDataProcessHook hook(pDocument_, pAccount_, pSubAccount_,
			pFolder_, pProfile_, pSessionCallback_, pSyncFilterSet,
			nOption, nUidStart_, listMessageData, listMakeSeen, listMakeDeleted,
			listBodyStructure, pImap4_.get(), &globalVariable, this);
		Hook h(this, &hook);
		for (unsigned int nId = nIdStart_ + 1; nId <= nExists_; nId += nFetchCount) {
			unsigned long nEnd = QSMIN(static_cast<unsigned long>(nId + nFetchCount - 1), nExists_);
			ContinuousRange range(nId, nEnd, false);
			if (!pImap4_->getMessageData(range, (nOption & OPTION_USEENVELOPE) == 0,
				(nOption & OPTION_USEBODYSTRUCTUREALWAYS) != 0, strAdditionalFields.get()))
				HANDLE_ERROR();
		}
		pSessionCallback_->setPos(nExists_);
	}
	
	if (!listMakeSeen.empty() || !listMakeDeleted.empty()) {
		pCallback_->setMessage(IDS_SETFLAGS);
		
		struct
		{
			const UidList* p_;
			Imap4::Flag flag_;
		} items[] = {
			{ &listMakeSeen,	Imap4::FLAG_SEEN	},
			{ &listMakeDeleted,	Imap4::FLAG_DELETED	}
		};
		for (int n = 0; n < countof(items); ++n) {
			if (!items[n].p_->empty()) {
				MultipleRange range(&(*items[n].p_)[0], items[n].p_->size(), true);
				Flags flags(items[n].flag_);
				Flags mask(items[n].flag_);
				if (!pImap4_->setFlags(range, flags, mask))
					HANDLE_ERROR();
			}
		}
	}
	
	pCallback_->setMessage(IDS_DOWNLOADMESSAGES);
	
	UidList listAllUid;
	UidList listHeaderUid;
	MessageDataList listPartial;
	for (MessageDataList::const_iterator it = listMessageData.begin(); it != listMessageData.end(); ++it) {
		switch ((*it).getType()) {
		case MessageData::TYPE_HEADER:
			listHeaderUid.push_back((*it).getId());
			break;
		case MessageData::TYPE_TEXT:
		case MessageData::TYPE_HTML:
			listPartial.push_back(*it);
			break;
		case MessageData::TYPE_ALL:
			listAllUid.push_back((*it).getId());
			break;
		default:
			break;
		}
	}
	
	unsigned int nPos = 0;
	pSessionCallback_->setRange(0,
		listHeaderUid.size() + listAllUid.size() +
		listPartial.size()*(nOption & OPTION_USEBODYSTRUCTUREALWAYS ? 1 : 2));
	pSessionCallback_->setPos(0);
	
	if ((nOption & OPTION_USEBODYSTRUCTUREALWAYS) == 0 && !listPartial.empty()) {
		class BodyStructureProcessHook : public AbstractBodyStructureProcessHook
		{
		public:
			typedef std::vector<FetchDataBodyStructure*> BodyStructureList;
		
		public:
			BodyStructureProcessHook(MessageDataList& listMessageData,
									 BodyStructureList& listBodyStructure,
									 ReceiveSessionCallback* pSessionCallback,
									 unsigned int* pnPos) :
				listMessageData_(listMessageData),
				listBodyStructure_(listBodyStructure),
				pSessionCallback_(pSessionCallback),
				pnPos_(pnPos)
			{
			}
			
		protected:
			virtual bool setBodyStructure(unsigned long nUid,
										  FetchDataBodyStructure* pBodyStructure,
										  bool* pbSet)
			{
				Imap4ReceiveSession::MessageDataList::iterator it = std::find_if(
					listMessageData_.begin(), listMessageData_.end(),
					std::bind2nd(
						binary_compose_f_gx_hy(
							std::equal_to<unsigned long>(),
							std::mem_fun_ref(&MessageData::getId),
							std::identity<unsigned long>()),
						nUid));
				if (it != listMessageData_.end()) {
					listBodyStructure_.push_back(pBodyStructure);
					(*it).setBodyStructure(pBodyStructure);
					*pbSet = true;
				}
				
				return true;
			}
			
			virtual void processed()
			{
				++(*pnPos_);
				pSessionCallback_->setPos(*pnPos_);
			}
		
		private:
			MessageDataList& listMessageData_;
			BodyStructureList& listBodyStructure_;
			ReceiveSessionCallback* pSessionCallback_;
			unsigned int* pnPos_;
		} hook(listPartial, listBodyStructure, pSessionCallback_, &nPos);
		Hook h(this, &hook);
		
		UidList listPartialUid;
		listPartialUid.resize(listPartial.size());
		std::transform(listPartial.begin(), listPartial.end(),
			listPartialUid.begin(), std::mem_fun_ref(&MessageData::getId));
		MultipleRange range(&listPartialUid[0], listPartialUid.size(), true);
		if (!pImap4_->getBodyStructure(range))
			HANDLE_ERROR();
		
		bool bMoved = false;
		for (MessageDataList::iterator it = listPartial.begin(); it != listPartial.end(); ) {
			FetchDataBodyStructure* pBodyStructure = (*it).getBodyStructure();
			if (!pBodyStructure || !Util::hasAttachmentPart(pBodyStructure)) {
				listAllUid.push_back((*it).getId());
				it = listPartial.erase(it);
				bMoved = true;
			}
			else {
				++it;
			}
		}
		if (bMoved)
			std::sort(listAllUid.begin(), listAllUid.end());
	}
	
	class MessageProcessHook : public AbstractMessageProcessHook
	{
	public:
		MessageProcessHook(Account* pAccount,
						   const MessageDataList& listMessageData,
						   ReceiveSessionCallback* pSessionCallback,
						   bool bHeader,
						   unsigned int* pnPos) :
			pAccount_(pAccount),
			listMessageData_(listMessageData),
			it_(listMessageData_.begin()),
			pSessionCallback_(pSessionCallback),
			bHeader_(bHeader),
			pnPos_(pnPos)
		{
		}
	
	protected:
		virtual Account* getAccount()
		{
			return pAccount_;
		}
		
		virtual bool isHeader()
		{
			return bHeader_;
		}
		
		virtual bool isMakeUnseen()
		{
			return false;
		}
		
		virtual MessagePtr getMessagePtr(unsigned long nUid)
		{
			Imap4ReceiveSession::MessageDataList::const_iterator m = std::find_if(
				it_, listMessageData_.end(),
				std::bind2nd(
					binary_compose_f_gx_hy(
						std::equal_to<unsigned long>(),
						std::mem_fun_ref(&MessageData::getId),
						std::identity<unsigned long>()),
					nUid));
			if (m == listMessageData_.end()) {
				m = std::find_if(listMessageData_.begin(), it_,
					std::bind2nd(
						binary_compose_f_gx_hy(
							std::equal_to<unsigned long>(),
							std::mem_fun_ref(&MessageData::getId),
							std::identity<unsigned long>()),
						nUid));
				if (m == it_)
					return 0;
			}
			
			it_ = m;
			
			return (*it_).getMessagePtr();
		}
		
		virtual void processed()
		{
			++(*pnPos_);
			pSessionCallback_->setPos(*pnPos_);
		}
	
	private:
		Account* pAccount_;
		const MessageDataList& listMessageData_;
		MessageDataList::const_iterator it_;
		ReceiveSessionCallback* pSessionCallback_;
		bool bHeader_;
		unsigned int* pnPos_;
	};
	
	if (!listAllUid.empty()) {
		MessageProcessHook hook(pAccount_,
			listMessageData, pSessionCallback_, false, &nPos);
		Hook h(this, &hook);
		
		MultipleRange range(&listAllUid[0], listAllUid.size(), true);
		if (!pImap4_->getMessage(range, true))
			HANDLE_ERROR();
	}
	
	if (!listPartial.empty()) {
		class PartialMessageProcessHook : public AbstractPartialMessageProcessHook
		{
		public:
			PartialMessageProcessHook(Account* pAccount,
									  const MessageDataList& listMessageData,
									  FetchDataBodyStructure* pBodyStructure,
									  const PartList& listPart,
									  unsigned int nPartCount,
									  bool bAll,
									  unsigned int nOption) :
				pAccount_(pAccount),
				listMessageData_(listMessageData),
				it_(listMessageData_.begin()),
				pBodyStructure_(pBodyStructure),
				listPart_(listPart),
				nPartCount_(nPartCount),
				bAll_(bAll),
				nOption_(nOption)
			{
			}
			
		protected:
			virtual qm::Account* getAccount()
			{
				return pAccount_;
			}
			
			virtual bool isAll()
			{
				return bAll_;
			}
			
			virtual const PartList& getPartList()
			{
				return listPart_;
			}
			
			virtual unsigned int getPartCount()
			{
				return nPartCount_;
			}
			
			virtual bool isMakeUnseen()
			{
				return false;
			}
			
			virtual MessagePtr getMessagePtr(unsigned long nUid)
			{
				Imap4ReceiveSession::MessageDataList::const_iterator m = std::find_if(
					it_, listMessageData_.end(),
					std::bind2nd(
						binary_compose_f_gx_hy(
							std::equal_to<unsigned long>(),
							std::mem_fun_ref(&MessageData::getId),
							std::identity<unsigned long>()),
						nUid));
				if (m == listMessageData_.end()) {
					m = std::find_if(listMessageData_.begin(), it_,
						std::bind2nd(
							binary_compose_f_gx_hy(
								std::equal_to<unsigned long>(),
								std::mem_fun_ref(&MessageData::getId),
								std::identity<unsigned long>()),
							nUid));
					if (m == it_)
						return 0;
				}
				
				it_ = m;
				
				return (*it_).getMessagePtr();
			}
			
			virtual unsigned int getOption()
			{
				return nOption_;
			}
			
			virtual void processed()
			{
			}
		
		private:
			Account* pAccount_;
			const MessageDataList& listMessageData_;
			MessageDataList::const_iterator it_;
			FetchDataBodyStructure* pBodyStructure_;
			const PartList& listPart_;
			unsigned int nPartCount_;
			bool bAll_;
			unsigned int nOption_;
		};
		
		for (MessageDataList::iterator it = listPartial.begin(); it != listPartial.end(); ++it) {
			Util::PartList listPart;
			Util::PartListDeleter deleter(listPart);
			unsigned int nPath = 0;
			Util::getPartsFromBodyStructure((*it).getBodyStructure(), &nPath, &listPart);
			
			if (!listPart.empty()) {
				string_ptr strArg;
				unsigned int nPartCount = 0;
				bool bAll = false;
				Util::getFetchArgFromPartList(listPart,
					(*it).getType() == MessageData::TYPE_HTML ?
						Util::FETCHARG_HTML : Util::FETCHARG_TEXT,
					true, (nOption & OPTION_TRUSTBODYSTRUCTURE) == 0,
					&strArg, &nPartCount, &bAll);
				
				PartialMessageProcessHook hook(pAccount_, listMessageData,
					(*it).getBodyStructure(), listPart, nPartCount, bAll, nOption);
				Hook h(this, &hook);
				
				SingleRange range((*it).getId(), true);
				if (!pImap4_->fetch(range, strArg.get()))
					HANDLE_ERROR();
			}
			
			pSessionCallback_->setPos(nPos);
			++nPos;
		}
	}
	
	if (!listHeaderUid.empty()) {
		MessageProcessHook hook(pAccount_,
			listMessageData, pSessionCallback_, true, &nPos);
		Hook h(this, &hook);
		
		MultipleRange range(&listHeaderUid[0], listHeaderUid.size(), true);
		if (!pImap4_->getHeader(range, true))
			HANDLE_ERROR();
	}
	
	if (!listMessageData.empty()) {
		bool bJunkFilter = pSubAccount_->isJunkFilterEnabled();
		bool bApplyRules = pSubAccount_->isAutoApplyRules();
		
		if (bJunkFilter) {
			if (!applyJunkFilter(listMessageData))
				return false;
			bJunkFilter = pFolder_->isFlag(Folder::FLAG_INBOX);
		}
		
		if (bApplyRules || bJunkFilter) {
			if (!applyRules(listMessageData, bJunkFilter, !bApplyRules))
				reportError(0, IMAP4ERROR_APPLYRULES);
		}
		
		bool bNotifyNewMessage = !pFolder_->isFlag(Folder::FLAG_OUTBOX) &&
			!pFolder_->isFlag(Folder::FLAG_SENTBOX) &&
			!pFolder_->isFlag(Folder::FLAG_TRASHBOX) &&
			!pFolder_->isFlag(Folder::FLAG_DRAFTBOX) &&
			!pFolder_->isFlag(Folder::FLAG_JUNKBOX);
		if (bNotifyNewMessage) {
			for (MessageDataList::const_iterator it = listMessageData.begin(); it != listMessageData.end(); ++it) {
				bool bNotify = false;
				{
					MessagePtrLock mpl((*it).getMessagePtr());
					bNotify = mpl && !pAccount_->isSeen(mpl);
				}
				if (bNotify)
					pSessionCallback_->notifyNewMessage((*it).getMessagePtr());
			}
		}
	}
	
	return true;
}

bool qmimap4::Imap4ReceiveSession::applyOfflineJobs()
{
	pCallback_->setMessage(IDS_APPLYOFFLINEJOBS);
	
	Imap4Driver* pDriver = static_cast<Imap4Driver*>(pAccount_->getProtocolDriver());
	OfflineJobManager* pManager = pDriver->getOfflineJobManager();
	if (!pManager->apply(pAccount_, pImap4_.get(), pSessionCallback_))
		return false;
	
	return true;
}

bool qmimap4::Imap4ReceiveSession::downloadReservedMessages(NormalFolder* pFolder)
{
	assert(pFolder);
	
	if (pFolder->getDownloadCount() == 0)
		return true;
	
	unsigned int nOption = pSubAccount_->getPropertyInt(L"Imap4", L"Option");
	
	typedef std::vector<unsigned long> UidList;
	UidList listAll;
	UidList listText;
	
	{
		Lock<Account> lock(*pAccount_);
		
		if (!pFolder->loadMessageHolders())
			return false;
		
		for (unsigned int n = 0; n < pFolder->getCount(); ++n) {
			MessageHolder* pmh = pFolder->getMessage(n);
			unsigned int nFlags = pmh->getFlags();
			if (nFlags & MessageHolder::FLAG_DOWNLOAD)
				listAll.push_back(pmh->getId());
			else if (nFlags & MessageHolder::FLAG_DOWNLOADTEXT)
				listText.push_back(pmh->getId());
		}
	}
	
	if (listAll.empty() && listText.empty())
		return true;
	
	wstring_ptr wstrName(Util::getFolderName(pFolder));
	if (!pImap4_->select(wstrName.get()))
		HANDLE_ERROR();
	
	unsigned int nPos = 0;
	if (!listAll.empty()) {
		class MessageProcessHook : public AbstractMessageProcessHook
		{
		public:
			typedef std::vector<unsigned long> UidList;
		
		public:
			MessageProcessHook(NormalFolder* pFolder,
							   const UidList& listUid,
							   ReceiveSessionCallback* pSessionCallback,
							   unsigned int* pnPos) :
				pFolder_(pFolder),
				listUid_(listUid),
				pSessionCallback_(pSessionCallback),
				pnPos_(pnPos)
			{
			}
		
		protected:
			virtual Account* getAccount()
			{
				return pFolder_->getAccount();
			}
			
			virtual bool isHeader()
			{
				return false;
			}
			
			virtual bool isMakeUnseen()
			{
				return true;
			}
			
			virtual MessagePtr getMessagePtr(unsigned long nUid)
			{
				UidList::const_iterator it = std::lower_bound(
					listUid_.begin(), listUid_.end(), nUid);
				if (it == listUid_.end() || *it != nUid)
					return MessagePtr();
				return pFolder_->getMessageById(nUid);
			}
			
			virtual void processed()
			{
				pSessionCallback_->setPos(++(*pnPos_));
			}
		
		private:
			NormalFolder* pFolder_;
			const UidList& listUid_;
			ReceiveSessionCallback* pSessionCallback_;
			unsigned int* pnPos_;
		} hook(pFolder, listAll, pSessionCallback_, &nPos);
		
		Hook h(this, &hook);
		
		MultipleRange range(&listAll[0], listAll.size(), true);
		if (!pImap4_->getMessage(range, true))
			HANDLE_ERROR();
	}
	
	if (!listText.empty()) {
		typedef std::vector<FetchDataBodyStructure*> BodyStructureList;
		BodyStructureList listBodyStructure;
		container_deleter<BodyStructureList> deleter(listBodyStructure);
		listBodyStructure.resize(listText.size());
		
		class BodyStructureProcessHook : public AbstractBodyStructureProcessHook
		{
		public:
			typedef std::vector<unsigned long> UidList;
			typedef std::vector<FetchDataBodyStructure*> BodyStructureList;
		
		public:
			BodyStructureProcessHook(const UidList& listUid,
									 BodyStructureList& listBodyStructure) :
				listUid_(listUid),
				listBodyStructure_(listBodyStructure)
			{
			}
		
		protected:
			virtual bool setBodyStructure(unsigned long nUid,
										  FetchDataBodyStructure* pBodyStructure,
										  bool* pbSet)
			{
				UidList::const_iterator it = std::lower_bound(
					listUid_.begin(), listUid_.end(), nUid);
				if (it != listUid_.end() && *it == nUid) {
					listBodyStructure_[it - listUid_.begin()] = pBodyStructure;
					*pbSet = true;
				}
				return true;
			}
			
			virtual void processed()
			{
			}
		
		private:
			const UidList& listUid_;
			BodyStructureList& listBodyStructure_;
		} hook(listText, listBodyStructure);
		Hook h(this, &hook);
		
		MultipleRange range(&listText[0], listText.size(), true);
		if (!pImap4_->getBodyStructure(range))
			HANDLE_ERROR();
		
		class PartialMessageProcessHook : public AbstractPartialMessageProcessHook
		{
		public:
			typedef std::vector<unsigned long> UidList;
		
		public:
			PartialMessageProcessHook(NormalFolder* pFolder,
									  const UidList& listUid,
									  FetchDataBodyStructure* pBodyStructure,
									  const PartList& listPart,
									  unsigned int nPartCount,
									  bool bAll,
									  unsigned int nOption) :
				pFolder_(pFolder),
				listUid_(listUid),
				pBodyStructure_(pBodyStructure),
				listPart_(listPart),
				nPartCount_(nPartCount),
				bAll_(bAll),
				nOption_(nOption)
			{
			}
			
		protected:
			virtual qm::Account* getAccount()
			{
				return pFolder_->getAccount();
			}
			
			virtual bool isAll()
			{
				return bAll_;
			}
			
			virtual const PartList& getPartList()
			{
				return listPart_;
			}
			
			virtual unsigned int getPartCount()
			{
				return nPartCount_;
			}
			
			virtual bool isMakeUnseen()
			{
				return true;
			}
			
			virtual qm::MessagePtr getMessagePtr(unsigned long nUid)
			{
				UidList::const_iterator it = std::lower_bound(
					listUid_.begin(), listUid_.end(), nUid);
				if (it == listUid_.end() || *it != nUid)
					return MessagePtr();
				return pFolder_->getMessageById(nUid);
			}
			
			virtual unsigned int getOption()
			{
				return nOption_;
			}
			
			virtual void processed()
			{
			}
		
		private:
			NormalFolder* pFolder_;
			const UidList& listUid_;
			FetchDataBodyStructure* pBodyStructure_;
			const PartList& listPart_;
			unsigned int nPartCount_;
			bool bAll_;
			unsigned int nOption_;
		};
		for (UidList::size_type n = 0; n < listText.size(); ++n) {
			if (listBodyStructure[n]) {
				if (pSessionCallback_->isCanceled(false))
					return true;
				pSessionCallback_->setPos(++nPos);
				
				Util::PartList listPart;
				Util::PartListDeleter deleter(listPart);
				unsigned int nPath = 0;
				Util::getPartsFromBodyStructure(listBodyStructure[n], &nPath, &listPart);
				
				if (!listPart.empty()) {
					string_ptr strArg;
					unsigned int nPartCount = 0;
					bool bAll = false;
					Util::getFetchArgFromPartList(listPart, Util::FETCHARG_TEXT,
						true, (nOption & OPTION_TRUSTBODYSTRUCTURE) == 0,
						&strArg, &nPartCount, &bAll);
					
					PartialMessageProcessHook hook(pFolder, listText,
						listBodyStructure[n], listPart, nPartCount, bAll, nOption);
					Hook h(this, &hook);
					
					SingleRange range(listText[n], true);
					if (!pImap4_->fetch(range, strArg.get()))
						HANDLE_ERROR();
				}
			}
		}
	}
	
	return true;
}

bool qmimap4::Imap4ReceiveSession::applyJunkFilter(const MessageDataList& l)
{
	JunkFilter* pJunkFilter = pDocument_->getJunkFilter();
	if (!pJunkFilter)
		return true;
	
	if (pFolder_->isFlag(Folder::FLAG_INBOX) &&
		pJunkFilter->getFlags() & JunkFilter::FLAG_AUTOLEARN) {
		pCallback_->setMessage(IDS_FILTERJUNK);
		pSessionCallback_->setRange(0, l.size());
		pSessionCallback_->setPos(0);
		
		pAccount_->prepareGetMessage(pFolder_);
		
		for (MessageDataList::size_type n = 0; n < l.size(); ++n) {
			Message msg;
			bool bProcess = false;
			bool bSeen = false;
			{
				MessagePtrLock mpl(l[n].getMessagePtr());
				if (mpl && !mpl->isFlag(MessageHolder::FLAG_DELETED)) {
					bSeen = pAccount_->isSeen(mpl);
					bProcess = mpl->getMessage(Account::GETMESSAGEFLAG_TEXT,
						0, SECURITYMODE_NONE, &msg);
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
						reportError(0, IMAP4ERROR_FILTERJUNK);
					else if (fScore > pJunkFilter->getThresholdScore())
						nOperation = JunkFilter::OPERATION_ADDJUNK;
					else
						nOperation = JunkFilter::OPERATION_ADDCLEAN;
				}
			}
			if (nOperation != 0) {
				if (!pJunkFilter->manage(msg, nOperation))
					reportError(0, IMAP4ERROR_MANAGEJUNK);
			}
			
			pSessionCallback_->setPos(n);
		}
	}
	else if (pFolder_->isFlag(Folder::FLAG_JUNKBOX) &&
		pJunkFilter->getFlags() & JunkFilter::FLAG_AUTOLEARN) {
		pCallback_->setMessage(IDS_MANAGEJUNK);
		pSessionCallback_->setRange(0, l.size());
		pSessionCallback_->setPos(0);
		
		pAccount_->prepareGetMessage(pFolder_);
		
		for (MessageDataList::size_type n = 0; n < l.size(); ++n) {
			Message msg;
			unsigned int nOperation = 0;
			{
				MessagePtrLock mpl(l[n].getMessagePtr());
				if (mpl && !mpl->isFlag(MessageHolder::FLAG_DELETED)) {
					wstring_ptr wstrId(mpl->getMessageId());
					if (pJunkFilter->getStatus(wstrId.get()) != JunkFilter::STATUS_JUNK) {
						if (mpl->getMessage(Account::GETMESSAGEFLAG_TEXT, 0, SECURITYMODE_NONE, &msg))
							nOperation = JunkFilter::OPERATION_ADDJUNK;
					}
				}
			}
			if (nOperation != 0) {
				if (!pJunkFilter->manage(msg, nOperation))
					reportError(0, IMAP4ERROR_MANAGEJUNK);
			}
			
			pSessionCallback_->setPos(n);
		}
	}
	
	return true;
}

bool qmimap4::Imap4ReceiveSession::applyRules(const MessageDataList& l,
											  bool bJunkFilter,
											  bool bJunkFilterOnly)
{
	MessagePtrList listMessagePtr;
	listMessagePtr.resize(l.size());
	std::transform(l.begin(), l.end(), listMessagePtr.begin(),
		std::mem_fun_ref(&MessageData::getMessagePtr));
	
	RuleManager* pRuleManager = pDocument_->getRuleManager();
	DefaultReceiveSessionRuleCallback callback(pSessionCallback_);
	return pRuleManager->apply(pFolder_, &listMessagePtr, pDocument_,
		pProfile_, bJunkFilter, bJunkFilterOnly, &callback);
}

bool qmimap4::Imap4ReceiveSession::processCapabilityResponse(ResponseCapability* pCapability)
{
	// TODO
	return true;
}

bool qmimap4::Imap4ReceiveSession::processContinueResponse(ResponseContinue* pContinue)
{
	// TODO
	return true;
}

bool qmimap4::Imap4ReceiveSession::processExistsResponse(ResponseExists* pExists)
{
	nExists_ = pExists->getExists();
	return true;
}

bool qmimap4::Imap4ReceiveSession::processExpungeResponse(ResponseExpunge* pExpunge)
{
	// TODO
	return true;
}

bool qmimap4::Imap4ReceiveSession::processFetchResponse(ResponseFetch* pFetch)
{
	ProcessHook::Result result = ProcessHook::RESULT_UNPROCESSED;
	if (pProcessHook_) {
		result = pProcessHook_->processFetchResponse(pFetch);
		if (result == ProcessHook::RESULT_ERROR)
			return false;
	}
	if (result == ProcessHook::RESULT_UNPROCESSED) {
		// TODO
	}
	return true;
}

bool qmimap4::Imap4ReceiveSession::processFlagsResponse(ResponseFlags* pFlags)
{
	// TODO
	return true;
}

bool qmimap4::Imap4ReceiveSession::processListResponse(ResponseList* pList)
{
	// TODO
	return true;
}

bool qmimap4::Imap4ReceiveSession::processNamespaceResponse(ResponseNamespace* pNamespace)
{
	// TODO
	return true;
}

bool qmimap4::Imap4ReceiveSession::processRecentResponse(ResponseRecent* pRecent)
{
	return true;
}

bool qmimap4::Imap4ReceiveSession::processSearchResponse(ResponseSearch* pSearch)
{
	// TODO
	return true;
}

bool qmimap4::Imap4ReceiveSession::processStateResponse(ResponseState* pState)
{
	switch (pState->getFlag()) {
	case ResponseState::FLAG_OK:
		{
			State* p = pState->getState();
			switch (p->getCode()) {
			case State::CODE_NONE:
				break;
			case State::CODE_ALERT:
				break;
			case State::CODE_NEWNAME:
				break;
			case State::CODE_PARSE:
				break;
			case State::CODE_PERMANENTFLAGS:
				break;
			case State::CODE_READONLY:
				bReadOnly_ = true;
				break;
			case State::CODE_READWRITE:
				bReadOnly_ = false;
				break;
			case State::CODE_TRYCREATE:
				break;
			case State::CODE_UIDVALIDITY:
				nUidValidity_ = p->getArgNumber();
				break;
			case State::CODE_UNSEEN:
				break;
			case State::CODE_UIDNEXT:
				break;
			case State::CODE_OTHER:
				break;
			default:
				assert(false);
				return false;
			}
		}
		break;
	default:
		break;
	}
	
	return true;
}

bool qmimap4::Imap4ReceiveSession::processStatusResponse(ResponseStatus* pStatus)
{
	// TODO
	return true;
}

void qmimap4::Imap4ReceiveSession::reportError(Imap4* pImap4,
											   unsigned int nImap4Error)
{
	struct
	{
		unsigned int nError_;
		UINT nId_;
	} maps[][23] = {
		{
			{ IMAP4ERROR_APPLYRULES,	IDS_ERROR_APPLYRULES	},
			{ IMAP4ERROR_MANAGEJUNK,	IDS_ERROR_MANAGEJUNK	},
			{ IMAP4ERROR_FILTERJUNK,	IDS_ERROR_FILTERJUNK	}
		},
		{
			{ Imap4::IMAP4_ERROR_GREETING,		IDS_ERROR_GREETING		},
			{ Imap4::IMAP4_ERROR_LOGIN,			IDS_ERROR_LOGIN			},
			{ Imap4::IMAP4_ERROR_CAPABILITY,	IDS_ERROR_CAPABILITY	},
			{ Imap4::IMAP4_ERROR_FETCH,			IDS_ERROR_FETCH			},
			{ Imap4::IMAP4_ERROR_STORE,			IDS_ERROR_STORE			},
			{ Imap4::IMAP4_ERROR_SELECT,		IDS_ERROR_SELECT		},
			{ Imap4::IMAP4_ERROR_LSUB,			IDS_ERROR_LSUB			},
			{ Imap4::IMAP4_ERROR_LIST,			IDS_ERROR_LIST			},
			{ Imap4::IMAP4_ERROR_COPY,			IDS_ERROR_COPY			},
			{ Imap4::IMAP4_ERROR_APPEND,		IDS_ERROR_APPEND		},
			{ Imap4::IMAP4_ERROR_NOOP,			IDS_ERROR_NOOP			},
			{ Imap4::IMAP4_ERROR_CREATE,		IDS_ERROR_CREATE		},
			{ Imap4::IMAP4_ERROR_DELETE,		IDS_ERROR_DELETE		},
			{ Imap4::IMAP4_ERROR_RENAME,		IDS_ERROR_RENAME		},
			{ Imap4::IMAP4_ERROR_SUBSCRIBE,		IDS_ERROR_SUBSCRIBE		},
			{ Imap4::IMAP4_ERROR_UNSUBSCRIBE,	IDS_ERROR_UNSUBSCRIBE	},
			{ Imap4::IMAP4_ERROR_CLOSE,			IDS_ERROR_CLOSE			},
			{ Imap4::IMAP4_ERROR_EXPUNGE,		IDS_ERROR_EXPUNGE		},
			{ Imap4::IMAP4_ERROR_AUTHENTICATE,	IDS_ERROR_AUTHENTICATE	},
			{ Imap4::IMAP4_ERROR_SEARCH,		IDS_ERROR_SEARCH		},
			{ Imap4::IMAP4_ERROR_NAMESPACE,		IDS_ERROR_NAMESPACE		},
			{ Imap4::IMAP4_ERROR_LOGOUT,		IDS_ERROR_LOGOUT		},
			{ Imap4::IMAP4_ERROR_STARTTLS,		IDS_ERROR_STARTTLS		}
		},
		{
			{ Imap4::IMAP4_ERROR_INITIALIZE,	IDS_ERROR_INITIALIZE	},
			{ Imap4::IMAP4_ERROR_CONNECT,		IDS_ERROR_CONNECT		},
			{ Imap4::IMAP4_ERROR_SELECTSOCKET,	IDS_ERROR_SELECTSOCKET	},
			{ Imap4::IMAP4_ERROR_TIMEOUT,		IDS_ERROR_TIMEOUT		},
			{ Imap4::IMAP4_ERROR_DISCONNECT,	IDS_ERROR_DISCONNECT	},
			{ Imap4::IMAP4_ERROR_RECEIVE,		IDS_ERROR_RECEIVE		},
			{ Imap4::IMAP4_ERROR_PARSE,			IDS_ERROR_PARSE			},
			{ Imap4::IMAP4_ERROR_OTHER,			IDS_ERROR_OTHER			},
			{ Imap4::IMAP4_ERROR_INVALIDSOCKET,	IDS_ERROR_INVALIDSOCKET	},
			{ Imap4::IMAP4_ERROR_SEND,			IDS_ERROR_SEND			},
			{ Imap4::IMAP4_ERROR_RESPONSE,		IDS_ERROR_RESPONSE		},
			{ Imap4::IMAP4_ERROR_SSL,			IDS_ERROR_SSL			}
		}
	};
	
	unsigned int nError = (pImap4 ? pImap4->getLastError() : 0) | nImap4Error;
	unsigned int nMasks[] = {
		IMAP4ERROR_MASK,
		Imap4::IMAP4_ERROR_MASK_HIGHLEVEL,
		Imap4::IMAP4_ERROR_MASK_LOWLEVEL
	};
	wstring_ptr wstrDescriptions[countof(maps)];
	for (int n = 0; n < countof(maps); ++n) {
		for (int m = 0; m < countof(maps[n]) && !wstrDescriptions[n].get(); ++m) {
			if (maps[n][m].nError_ != 0 && (nError & nMasks[n]) == maps[n][m].nError_)
				wstrDescriptions[n] = loadString(getResourceHandle(), maps[n][m].nId_);
		}
	}
	
	wstring_ptr wstrMessage(loadString(getResourceHandle(), IDS_ERROR_MESSAGE));
	wstring_ptr wstrSocketDescription(SocketBase::getErrorDescription(
		static_cast<SocketBase::Error>(nError & SocketBase::SOCKET_ERROR_MASK_SOCKET)));
	
	const WCHAR* pwszDescription[] = {
		wstrDescriptions[0].get(),
		wstrDescriptions[1].get(),
		wstrDescriptions[2].get(),
		wstrSocketDescription.get(),
		pImap4 ? pImap4->getLastErrorResponse() : 0
	};
	SessionErrorInfo info(pAccount_, pSubAccount_, pFolder_, wstrMessage.get(),
		nError, pwszDescription, countof(pwszDescription));
	pSessionCallback_->addError(info);
}


/****************************************************************************
 *
 * Imap4ReceiveSession::CallbackImpl
 *
 */

qmimap4::Imap4ReceiveSession::CallbackImpl::CallbackImpl(Imap4ReceiveSession* pSession,
														 SubAccount* pSubAccount,
														 const Security* pSecurity,
														 ReceiveSessionCallback* pSessionCallback) :
	AbstractCallback(pSubAccount, pSessionCallback, pSecurity),
	pSession_(pSession),
	pSessionCallback_(pSessionCallback)
{
}

qmimap4::Imap4ReceiveSession::CallbackImpl::~CallbackImpl()
{
}

void qmimap4::Imap4ReceiveSession::CallbackImpl::setMessage(UINT nId)
{
	wstring_ptr wstrMessage(loadString(getResourceHandle(), nId));
	pSessionCallback_->setMessage(wstrMessage.get());
}

bool qmimap4::Imap4ReceiveSession::CallbackImpl::isCanceled(bool bForce) const
{
	return pSessionCallback_->isCanceled(bForce);
}

void qmimap4::Imap4ReceiveSession::CallbackImpl::initialize()
{
	setMessage(IDS_INITIALIZE);
}

void qmimap4::Imap4ReceiveSession::CallbackImpl::lookup()
{
	setMessage(IDS_LOOKUP);
}

void qmimap4::Imap4ReceiveSession::CallbackImpl::connecting()
{
	setMessage(IDS_CONNECTING);
}

void qmimap4::Imap4ReceiveSession::CallbackImpl::connected()
{
	setMessage(IDS_CONNECTED);
}


void qmimap4::Imap4ReceiveSession::CallbackImpl::authenticating()
{
	setMessage(IDS_AUTHENTICATING);
}

void qmimap4::Imap4ReceiveSession::CallbackImpl::setRange(size_t nMin,
														  size_t nMax)
{
	pSessionCallback_->setSubRange(nMin, nMax);
}

void qmimap4::Imap4ReceiveSession::CallbackImpl::setPos(size_t nPos)
{
	pSessionCallback_->setSubPos(nPos);
}

bool qmimap4::Imap4ReceiveSession::CallbackImpl::response(Response* pResponse)
{
#define BEGIN_PROCESS_RESPONSE() \
	switch (pResponse->getType()) { \

#define END_PROCESS_RESPONSE() \
	default: \
		assert(false); \
		return false; \
	} \

#define PROCESS_RESPONSE(type, name) \
	case Response::type: \
		if (!pSession_->process##name##Response( \
			static_cast<Response##name*>(pResponse))) \
			return false; \
		break; \
	
	BEGIN_PROCESS_RESPONSE()
		PROCESS_RESPONSE(TYPE_CAPABILITY, Capability)
		PROCESS_RESPONSE(TYPE_CONTINUE, Continue)
		PROCESS_RESPONSE(TYPE_EXISTS, Exists)
		PROCESS_RESPONSE(TYPE_EXPUNGE, Expunge)
		PROCESS_RESPONSE(TYPE_FETCH, Fetch)
		PROCESS_RESPONSE(TYPE_FLAGS, Flags)
		PROCESS_RESPONSE(TYPE_LIST, List)
		PROCESS_RESPONSE(TYPE_NAMESPACE, Namespace)
		PROCESS_RESPONSE(TYPE_RECENT, Recent)
		PROCESS_RESPONSE(TYPE_SEARCH, Search)
		PROCESS_RESPONSE(TYPE_STATE, State)
		PROCESS_RESPONSE(TYPE_STATUS, Status)
	END_PROCESS_RESPONSE()
	
	return true;
}


/****************************************************************************
 *
 * Imap4ReceiveSession::Hook
 *
 */

Imap4ReceiveSession::Hook::Hook(Imap4ReceiveSession* pSession, ProcessHook* pHook) :
	pSession_(pSession)
{
	pSession_->pProcessHook_ = pHook;
}

Imap4ReceiveSession::Hook::~Hook()
{
	unhook();
}

void Imap4ReceiveSession::Hook::unhook()
{
	pSession_->pProcessHook_ = 0;
}


/****************************************************************************
 *
 * Imap4ReceiveSessionUI
 *
 */

qmimap4::Imap4ReceiveSessionUI::Imap4ReceiveSessionUI()
{
}

qmimap4::Imap4ReceiveSessionUI::~Imap4ReceiveSessionUI()
{
}

const WCHAR* qmimap4::Imap4ReceiveSessionUI::getClass()
{
	return L"mail";
}

wstring_ptr qmimap4::Imap4ReceiveSessionUI::getDisplayName()
{
	return loadString(getResourceHandle(), IDS_IMAP4);
}

short qmimap4::Imap4ReceiveSessionUI::getDefaultPort(bool bSecure)
{
	return bSecure ? 993 : 143;
}

bool qmimap4::Imap4ReceiveSessionUI::isSupported(Support support)
{
	return true;
}

std::auto_ptr<PropertyPage> qmimap4::Imap4ReceiveSessionUI::createPropertyPage(SubAccount* pSubAccount)
{
	return std::auto_ptr<PropertyPage>(new ReceivePage(pSubAccount));
}


/****************************************************************************
 *
 * Imap4ReceiveSessionFactory
 *
 */

Imap4ReceiveSessionFactory qmimap4::Imap4ReceiveSessionFactory::factory__;

qmimap4::Imap4ReceiveSessionFactory::Imap4ReceiveSessionFactory()
{
	registerFactory(L"imap4", this);
}

qmimap4::Imap4ReceiveSessionFactory::~Imap4ReceiveSessionFactory()
{
	unregisterFactory(L"imap4");
}

std::auto_ptr<ReceiveSession> qmimap4::Imap4ReceiveSessionFactory::createSession()
{
	return std::auto_ptr<ReceiveSession>(new Imap4ReceiveSession());
}

std::auto_ptr<ReceiveSessionUI> qmimap4::Imap4ReceiveSessionFactory::createUI()
{
	return std::auto_ptr<ReceiveSessionUI>(new Imap4ReceiveSessionUI());
}


/****************************************************************************
 *
 * Imap4SyncFilterCallback
 *
 */

qmimap4::Imap4SyncFilterCallback::Imap4SyncFilterCallback(Document* pDocument,
														  Account* pAccount,
														  NormalFolder* pFolder,
														  Message* pMessage,
														  unsigned int nUid,
														  unsigned int nSize,
														  unsigned int nTextSize,
														  Profile* pProfile,
														  MacroVariableHolder* pGlobalVariable,
														  Imap4ReceiveSession* pSession) :
	pDocument_(pDocument),
	pAccount_(pAccount),
	pFolder_(pFolder),
	pMessage_(pMessage),
	nUid_(nUid),
	nSize_(nSize),
	nTextSize_(nTextSize),
	pProfile_(pProfile),
	pGlobalVariable_(pGlobalVariable),
	pSession_(pSession)
{
}

qmimap4::Imap4SyncFilterCallback::~Imap4SyncFilterCallback()
{
}

bool qmimap4::Imap4SyncFilterCallback::getMessage(unsigned int nFlags)
{
	// TODO
	
	return true;
}

const NormalFolder* qmimap4::Imap4SyncFilterCallback::getFolder()
{
	return pFolder_;
}

std::auto_ptr<MacroContext> qmimap4::Imap4SyncFilterCallback::getMacroContext()
{
	if (!pmh_.get())
		pmh_.reset(new Imap4MessageHolder(this, pFolder_,
			pMessage_, nUid_, nSize_, nTextSize_));
	
	return std::auto_ptr<MacroContext>(new MacroContext(pmh_.get(),
		pMessage_, pAccount_, MessageHolderList(), pFolder_,
		pDocument_, 0, pProfile_, 0, MacroContext::FLAG_NONE,
		SECURITYMODE_NONE, 0, pGlobalVariable_));
}


/****************************************************************************
 *
 * Imap4MessageHolder
 *
 */

qmimap4::Imap4MessageHolder::Imap4MessageHolder(Imap4SyncFilterCallback* pCallback,
												NormalFolder* pFolder,
												Message* pMessage,
												unsigned int nId,
												unsigned int nSize,
												unsigned int nTextSize) :
	AbstractMessageHolder(pFolder, pMessage, nId, nSize, nTextSize),
	pCallback_(pCallback)
{
}

qmimap4::Imap4MessageHolder::~Imap4MessageHolder()
{
}

bool qmimap4::Imap4MessageHolder::getMessage(unsigned int nFlags,
											 const WCHAR* pwszField,
											 unsigned int nSecurityMode,
											 Message* pMessage)
{
	assert(pMessage == AbstractMessageHolder::getMessage());
	
	const WCHAR* pwszFields[] = {
		L"To",
		L"Cc",
		L"Bcc",
		L"From",
		L"Reply-To",
		L"Sender",
		L"Date",
		L"Subject",
		L"Message-Id",
		L"In-Reply-To"
	};
	
	for (int n = 0; n < countof(pwszFields); ++n) {
		if (_wcsicmp(pwszField, pwszFields[n]) == 0)
			return true;
	}
	
	return pCallback_->getMessage(nFlags);
}


/****************************************************************************
 *
 * MessageData
 *
 */

qmimap4::MessageData::MessageData(MessageHolder* pmh,
								  Type type,
								  FetchDataBodyStructure* pBodyStructure) :
	ptr_(pmh),
	nId_(pmh->getId()),
	type_(type),
	pBodyStructure_(pBodyStructure)
{
}

qmimap4::MessageData::MessageData(const MessageData& data) :
	ptr_(data.ptr_),
	nId_(data.nId_),
	type_(data.type_),
	pBodyStructure_(data.pBodyStructure_)
{
}

qmimap4::MessageData::~MessageData()
{
}

MessageData& qmimap4::MessageData::operator=(const MessageData& data)
{
	if (&data != this) {
		ptr_ = data.ptr_;
		nId_ = data.nId_;
		type_ = data.type_;
		pBodyStructure_ = data.pBodyStructure_;
	}
	return *this;
}

const MessagePtr& qmimap4::MessageData::getMessagePtr() const
{
	return ptr_;
}

unsigned long qmimap4::MessageData::getId() const
{
	return nId_;
}

MessageData::Type qmimap4::MessageData::getType() const
{
	return type_;
}

FetchDataBodyStructure* qmimap4::MessageData::getBodyStructure() const
{
	return pBodyStructure_;
}

void qmimap4::MessageData::setBodyStructure(FetchDataBodyStructure* pBodyStructure)
{
	assert(pBodyStructure);
	assert(!pBodyStructure_);
	assert(type_ == TYPE_TEXT);
	pBodyStructure_ = pBodyStructure;
}
