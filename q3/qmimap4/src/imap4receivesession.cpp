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

#include <qserror.h>
#include <qsnew.h>
#include <qsthread.h>

#include <algorithm>
#include <cstdio>
#include <functional>

#include "imap4.h"
#include "imap4driver.h"
#include "imap4receivesession.h"
#include "main.h"
#include "offlinejob.h"
#include "option.h"
#include "processhook.h"
#include "resourceinc.h"
#include "ui.h"

#pragma warning(disable:4786)

using namespace qmimap4;
using namespace qm;
using namespace qs;

#define CHECK_QSTATUS_ERROR() \
	if (status != QSTATUS_SUCCESS) { \
		reportError(); \
		return status; \
	} \


namespace qmimap4 {
class MessageData;
typedef std::vector<MessageData> MessageDataList;
}

/****************************************************************************
 *
 * MessageData
 *
 */

class qmimap4::MessageData
{
public:
	enum Type {
		TYPE_HEADER,
		TYPE_TEXT,
		TYPE_HTML,
		TYPE_ALL
	};

public:
	MessageData(MessageHolder* pmh, Type type,
		FetchDataBodyStructure* pBodyStructure);
	MessageData(const MessageData& data);
	~MessageData();

public:
	MessageData& operator=(const MessageData& data);

public:
	const MessagePtr& getMessagePtr() const;
	unsigned long getId() const;
	Type getType() const;
	FetchDataBodyStructure* getBodyStructure() const;
	void setBodyStructure(FetchDataBodyStructure* pBodyStructure);

private:
	MessagePtr ptr_;
	unsigned long nId_;
	Type type_;
	FetchDataBodyStructure* pBodyStructure_;
};

qmimap4::MessageData::MessageData(MessageHolder* pmh,
	Type type, FetchDataBodyStructure* pBodyStructure) :
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


/****************************************************************************
 *
 * Imap4ReceiveSession
 *
 */

qmimap4::Imap4ReceiveSession::Imap4ReceiveSession(QSTATUS* pstatus) :
	pImap4_(0),
	pCallback_(0),
	pDocument_(0),
	pAccount_(0),
	pSubAccount_(0),
	pFolder_(0),
	hwnd_(0),
	pLogger_(0),
	pSessionCallback_(0),
	pProcessHook_(0),
	nExists_(0),
	nUidValidity_(0),
	bReadOnly_(false),
	nUidStart_(0),
	nIdStart_(0)
{
	assert(pstatus);
	*pstatus = QSTATUS_SUCCESS;
}

qmimap4::Imap4ReceiveSession::~Imap4ReceiveSession()
{
	delete pImap4_;
	delete pCallback_;
}

QSTATUS qmimap4::Imap4ReceiveSession::init(Document* pDocument,
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
	
	status = newQsObject(this, pSubAccount_,
		pDocument->getSecurity(), pSessionCallback_, &pCallback_);
	CHECK_QSTATUS();
	
	return QSTATUS_SUCCESS;
}

QSTATUS qmimap4::Imap4ReceiveSession::connect()
{
	assert(!pImap4_);
	
	DECLARE_QSTATUS();
	
	Log log(pLogger_, L"qmimap4::Imap4ReceiveSession");
	status = log.debug(L"Connecting to the server...");
	CHECK_QSTATUS();
	
	Imap4::Option option = {
		pSubAccount_->getTimeout(),
		pCallback_,
		pCallback_,
		pCallback_,
		pLogger_
	};
	status = newQsObject(option, &pImap4_);
	CHECK_QSTATUS();
	
	Imap4::Ssl ssl = Imap4::SSL_NONE;
	status = Util::getSsl(pSubAccount_, &ssl);
	CHECK_QSTATUS();
	status = pImap4_->connect(pSubAccount_->getHost(Account::HOST_RECEIVE),
		pSubAccount_->getPort(Account::HOST_RECEIVE), ssl);
	CHECK_QSTATUS_ERROR();
	
	status = log.debug(L"Connected to the server.");
	CHECK_QSTATUS();
	
	return QSTATUS_SUCCESS;
}

QSTATUS qmimap4::Imap4ReceiveSession::disconnect()
{
	assert(pImap4_);
	
	DECLARE_QSTATUS();
	
	Log log(pLogger_, L"qmimap4::Imap4ReceiveSession");
	status = log.debug(L"Disconnecting from the server...");
	CHECK_QSTATUS();
	
	status = pImap4_->disconnect();
	CHECK_QSTATUS_ERROR();
	
	status = log.debug(L"Disconnected from the server.");
	CHECK_QSTATUS();
	
	return QSTATUS_SUCCESS;
}

QSTATUS qmimap4::Imap4ReceiveSession::selectFolder(NormalFolder* pFolder)
{
	assert(pFolder);
	
	DECLARE_QSTATUS();
	
	status = pCallback_->setMessage(IDS_SELECTFOLDER);
	CHECK_QSTATUS();
	
	string_ptr<WSTRING> wstrName;
	status = Util::getFolderName(pFolder, &wstrName);
	CHECK_QSTATUS();
	
	pFolder_ = 0;
	
	status = pImap4_->select(wstrName.get());
	CHECK_QSTATUS_ERROR();
	
	pFolder_ = pFolder;
	
	status = pSessionCallback_->setRange(0, nExists_);
	CHECK_QSTATUS();
	status = pSessionCallback_->setPos(0);
	CHECK_QSTATUS();
	
	return QSTATUS_SUCCESS;
}

QSTATUS qmimap4::Imap4ReceiveSession::closeFolder()
{
	assert(pFolder_);
	
	DECLARE_QSTATUS();
	
	status = pCallback_->setRange(0, 0);
	CHECK_QSTATUS();
	status = pCallback_->setPos(0);
	CHECK_QSTATUS();
	
	int nCloseFolder = 0;
	status = pSubAccount_->getProperty(
		L"Imap4", L"CloseFolder", 0, &nCloseFolder);
	CHECK_QSTATUS();
	if (nCloseFolder) {
		status = pCallback_->setMessage(IDS_CLOSEFOLDER);
		CHECK_QSTATUS();
		
		status = pImap4_->close();
		CHECK_QSTATUS_ERROR();
	}
	
	pFolder_ = 0;
	
	return QSTATUS_SUCCESS;
}

QSTATUS qmimap4::Imap4ReceiveSession::updateMessages()
{
	assert(pFolder_);
	
	DECLARE_QSTATUS();
	
	status = pCallback_->setMessage(IDS_UPDATEMESSAGES);
	CHECK_QSTATUS();
	
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
		status = pFolder_->setValidity(nUidValidity_);
		CHECK_QSTATUS();
	}
	
	if (nUidStart_ != 0) {
		struct UpdateFlagsProcessHook : public ProcessHook
		{
			typedef NormalFolder::FlagList FlagList;
			
			UpdateFlagsProcessHook(unsigned int nUidStart,
				ReceiveSessionCallback* pSessionCallback) :
				nUidStart_(nUidStart),
				pSessionCallback_(pSessionCallback),
				nLastId_(0)
			{
			}
			
			virtual QSTATUS processFetchResponse(
				ResponseFetch* pFetch, bool* pbProcessed)
			{
				DECLARE_QSTATUS();
				
				STLWrapper<FlagList> wrapper(listFlag_);
				
				unsigned int nUid = 0;
				unsigned int nFlags = 0;
				
				int nCount = 0;
				
				const ResponseFetch::FetchDataList& l =
					pFetch->getFetchDataList();
				ResponseFetch::FetchDataList::const_iterator it = l.begin();
				while (it != l.end()) {
					switch ((*it)->getType()) {
					case FetchData::TYPE_FLAGS:
						nFlags = Util::getMessageFlagsFromImap4Flags(
							static_cast<FetchDataFlags*>(*it)->getSystemFlags(),
							static_cast<FetchDataFlags*>(*it)->getCustomFlags());
						++nCount;
						break;
					case FetchData::TYPE_UID:
						nUid = static_cast<FetchDataUid*>(*it)->getUid();
						++nCount;
						break;
					default:
						break;
					}
					++it;
				}
				
				if (nCount == 2 && nUid <= nUidStart_) {
					status = wrapper.push_back(
						FlagList::value_type(nUid, nFlags));
					CHECK_QSTATUS();
					status = pSessionCallback_->setPos(pFetch->getNumber());
					CHECK_QSTATUS();
					nLastId_ = pFetch->getNumber();
				}
				
				return QSTATUS_SUCCESS;
			}
			
			unsigned int nUidStart_;
			ReceiveSessionCallback* pSessionCallback_;
			FlagList listFlag_;
			unsigned int nLastId_;
		} hook(nUidStart_, pSessionCallback_);
		
		if (nExists_ != 0) {
			Hook h(this, &hook);
			ContinuousRange range(1, nUidStart_, true, &status);
			CHECK_QSTATUS();
			status = pImap4_->getFlags(range);
			CHECK_QSTATUS_ERROR();
		}
		
		bool bClear = false;
		status = pFolder_->updateMessageFlags(hook.listFlag_, &bClear);
		CHECK_QSTATUS();
		if (bClear)
			nUidStart_ = 0;
		else
			nIdStart_ = hook.nLastId_;
	}
	
	status = pSessionCallback_->setPos(nIdStart_);
	CHECK_QSTATUS();
	
	++nUidStart_;
	
	return QSTATUS_SUCCESS;
}

QSTATUS qmimap4::Imap4ReceiveSession::downloadMessages(
	const SyncFilterSet* pSyncFilterSet)
{
	DECLARE_QSTATUS();
	
	int nFetchCount = 0;
	status = pSubAccount_->getProperty(L"Imap4",
		L"FetchCount", 100, &nFetchCount);
	CHECK_QSTATUS();
	
	int nOption = 0;
	status = pSubAccount_->getProperty(L"Imap4", L"Option", 0xff, &nOption);
	CHECK_QSTATUS();
	
	string_ptr<WSTRING> wstrAdditionalFields;
	status = pSubAccount_->getProperty(L"Imap4",
		L"AdditionalFields", 0, &wstrAdditionalFields);
	CHECK_QSTATUS();
	string_ptr<STRING> strAdditionalFields(wcs2mbs(wstrAdditionalFields.get()));
	if (!strAdditionalFields.get())
		return QSTATUS_OUTOFMEMORY;
	
	typedef std::vector<unsigned long> UidList;
	UidList listMakeSeen;
	UidList listMakeDeleted;
	
	typedef std::vector<FetchDataBodyStructure*> BodyStructureList;
	BodyStructureList listBodyStructure;
	container_deleter<BodyStructureList> deleter(listBodyStructure);
	
	MessageDataList listMessageData;
	status = STLWrapper<MessageDataList>(listMessageData).reserve(
		nExists_ - nIdStart_);
	CHECK_QSTATUS();
	
	struct GetMessageDataProcessHook : public ProcessHook
	{
		typedef std::vector<unsigned long> UidList;
		typedef std::vector<FetchDataBodyStructure*> BodyStructureList;
		
		GetMessageDataProcessHook(Document* pDocument, Account* pAccount,
			SubAccount* pSubAccount, NormalFolder* pFolder, HWND hwnd,
			Profile* pProfile, ReceiveSessionCallback* pSessionCallback,
			const SyncFilterSet* pFilterSet, unsigned int nOption,
			unsigned long nUidStart, MessageDataList& listMessageData,
			UidList& listMakeSeen, UidList& listMakeDeleted,
			BodyStructureList& listBodyStructure, Imap4* pImap4,
			MacroVariableHolder* pGlobalVariable, Imap4ReceiveSession* pSession) :
			pDocument_(pDocument),
			pAccount_(pAccount),
			pSubAccount_(pSubAccount),
			pFolder_(pFolder),
			hwnd_(hwnd),
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
		
		virtual QSTATUS processFetchResponse(
			ResponseFetch* pFetch, bool* pbProcessed)
		{
			DECLARE_QSTATUS();
			
			unsigned long nUid = 0;
			unsigned int nFlags = 0;
			unsigned long nSize = static_cast<unsigned long>(-1);
			FetchDataBodyStructure* pBodyStructure = 0;
			string_ptr<STRING> strEnvelope;
			const CHAR* pszHeader = 0;
			
			int nCount = 0;
			
			const ResponseFetch::FetchDataList& l = pFetch->getFetchDataList();
			ResponseFetch::FetchDataList::const_iterator it = l.begin();
			while (it != l.end()) {
				switch ((*it)->getType()) {
				case FetchData::TYPE_UID:
					nUid = static_cast<FetchDataUid*>(*it)->getUid();
					++nCount;
					break;
				case FetchData::TYPE_ENVELOPE:
					if (nOption_ & OPTION_USEENVELOPE) {
						status = Util::getMessageFromEnvelope(
							static_cast<FetchDataEnvelope*>(*it), &strEnvelope);
						CHECK_QSTATUS();
						++nCount;
					}
					break;
				case FetchData::TYPE_FLAGS:
					nFlags = Util::getMessageFlagsFromImap4Flags(
						static_cast<FetchDataFlags*>(*it)->getSystemFlags(),
						static_cast<FetchDataFlags*>(*it)->getCustomFlags());
					++nCount;
					break;
				case FetchData::TYPE_BODY:
					pszHeader = static_cast<FetchDataBody*>(*it)->getContent();
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
				++it;
			}
			
			const int nNeedCount = 4 + (nOption_ & OPTION_USEENVELOPE ? 1 : 0) +
				(nOption_ & OPTION_USEBODYSTRUCTUREALWAYS ? 1 : 0);
			if (nCount == nNeedCount && nUid >= nUidStart_) {
				status = pSessionCallback_->setPos(pFetch->getNumber());
				CHECK_QSTATUS();
				
				StringBuffer<STRING> message(&status);
				CHECK_QSTATUS();
				if (strEnvelope.get()) {
					status = message.append(strEnvelope.get());
					CHECK_QSTATUS();
				}
				if (pBodyStructure) {
					string_ptr<STRING> strBodyStructure;
					status = Util::getHeaderFromBodyStructure(
						pBodyStructure, &strBodyStructure);
					CHECK_QSTATUS();
					status = message.append(strBodyStructure.get());
					CHECK_QSTATUS();
				}
				status = message.append(pszHeader);
				CHECK_QSTATUS();
				
				Message msg(message.getCharArray(), message.getLength(),
					Message::FLAG_TEMPORARY, &status);
				CHECK_QSTATUS();
				
				bool bSelf = false;
				status = pSubAccount_->isSelf(msg, &bSelf);
				CHECK_QSTATUS();
				if (bSelf) {
					bool bSeen = nFlags & MessageHolder::FLAG_SEEN;
					nFlags |= MessageHolder::FLAG_SEEN | MessageHolder::FLAG_SENT;
					if (!bSeen) {
						status = STLWrapper<UidList>(listMakeSeen_).push_back(nUid);
						CHECK_QSTATUS();
					}
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
						pFolder_, &msg, nUid, nSize, nTextSize, hwnd_,
						pProfile_, pGlobalVariable_, pSession_);
					status = pFilterSet_->getFilter(&callback, &pFilter);
					CHECK_QSTATUS();
					if (pFilter) {
						const SyncFilter::ActionList& listAction = pFilter->getActions();
						SyncFilter::ActionList::const_iterator it = listAction.begin();
						while (it != listAction.end()) {
							const SyncFilterAction* pAction = *it;
							if (wcscmp(pAction->getName(), L"download") == 0) {
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
							else if (wcscmp(pAction->getName(), L"delete") == 0) {
								nFlags |= MessageHolder::FLAG_DELETED;
								status = STLWrapper<UidList>(
									listMakeDeleted_).push_back(nUid);
								CHECK_QSTATUS();
							}
							++it;
						}
					}
				}
				
				if (download == DOWNLOAD_TEXT || download == DOWNLOAD_HTML) {
					bool bAll = false;
					if (pBodyStructure) {
						bAll = !Util::hasAttachmentPart(pBodyStructure);
					}
					else {
						PartUtil util(msg);
						if (!util.isMultipart()) {
							bool bAttachment = false;
							status = util.isAttachment(&bAttachment);
							CHECK_QSTATUS();
							if (!bAttachment)
								bAll = true;
						}
					}
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
				
				MessageHolder* pmh = 0;
				status = pAccount_->storeMessage(pFolder_,
					message.getCharArray(), &msg, nUid, nFlags, nSize, true, &pmh);
				CHECK_QSTATUS();
				assert(pmh);
				
				switch (download) {
				case DOWNLOAD_ALL:
					status = STLWrapper<MessageDataList>(listMessageData_).push_back(
						MessageData(pmh, MessageData::TYPE_ALL, 0));
					CHECK_QSTATUS();
					break;
				case DOWNLOAD_TEXT:
				case DOWNLOAD_HTML:
					{
						MessageData::Type type = download == DOWNLOAD_TEXT ?
							MessageData::TYPE_TEXT : MessageData::TYPE_HTML;
						status = STLWrapper<MessageDataList>(listMessageData_).push_back(
							MessageData(pmh, type, pBodyStructure));
						CHECK_QSTATUS();
						if (pBodyStructure) {
							status = STLWrapper<BodyStructureList>(
								listBodyStructure_).push_back(pBodyStructure);
							CHECK_QSTATUS();
							pFetch->detach(pBodyStructure);
						}
					}
					break;
				case DOWNLOAD_HEADER:
					status = STLWrapper<MessageDataList>(listMessageData_).push_back(
						MessageData(pmh, MessageData::TYPE_HEADER, 0));
					break;
				case DOWNLOAD_NONE:
					break;
				default:
					assert(false);
					break;
				}
				
				if ((nFlags & MessageHolder::FLAG_SEEN) == 0) {
					status = pSessionCallback_->notifyNewMessage();
					CHECK_QSTATUS();
				}
				
				*pbProcessed = true;
			}
			
			return QSTATUS_SUCCESS;
		}
		
		Document* pDocument_;
		Account* pAccount_;
		SubAccount* pSubAccount_;
		NormalFolder* pFolder_;
		HWND hwnd_;
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
		status = pCallback_->setMessage(IDS_DOWNLOADMESSAGESDATA);
		CHECK_QSTATUS();
		
		MacroVariableHolder globalVariable(&status);
		CHECK_QSTATUS();
		GetMessageDataProcessHook hook(pDocument_, pAccount_, pSubAccount_,
			pFolder_, hwnd_, pProfile_, pSessionCallback_, pSyncFilterSet,
			nOption, nUidStart_, listMessageData, listMakeSeen, listMakeDeleted,
			listBodyStructure, pImap4_, &globalVariable, this);
		Hook h(this, &hook);
		for (unsigned int nId = nIdStart_ + 1; nId <= nExists_; nId += nFetchCount) {
			ContinuousRange range(nId,
				QSMIN(static_cast<unsigned long>(nId + nFetchCount - 1), nExists_),
				false, &status);
			CHECK_QSTATUS();
			status = pImap4_->getMessageData(range, (nOption & OPTION_USEENVELOPE) == 0,
				(nOption & OPTION_USEBODYSTRUCTUREALWAYS) != 0, strAdditionalFields.get());
			CHECK_QSTATUS_ERROR();
		}
	}
	
	if (!listMakeSeen.empty() || !listMakeDeleted.empty()) {
		status = pCallback_->setMessage(IDS_SETFLAGS);
		CHECK_QSTATUS();
		
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
				MultipleRange range(&(*items[n].p_)[0],
					items[n].p_->size(), true, &status);
				CHECK_QSTATUS();
				Flags flags(items[n].flag_, &status);
				CHECK_QSTATUS();
				Flags mask(items[n].flag_, &status);
				CHECK_QSTATUS();
				status = pImap4_->setFlags(range, flags, mask);
				CHECK_QSTATUS_ERROR();
			}
		}
	}
	
	if (!listMessageData.empty()) {
		status = pCallback_->setMessage(IDS_DOWNLOADMESSAGES);
		CHECK_QSTATUS();
		
		UidList listAllUid;
		UidList listHeaderUid;
		MessageDataList listPartial;
		MessageDataList::const_iterator it = listMessageData.begin();
		while (it != listMessageData.end()) {
			switch ((*it).getType()) {
			case MessageData::TYPE_HEADER:
				status = STLWrapper<UidList>(listHeaderUid).push_back((*it).getId());
				CHECK_QSTATUS();
				break;
			case MessageData::TYPE_TEXT:
			case MessageData::TYPE_HTML:
				status = STLWrapper<MessageDataList>(listPartial).push_back(*it);
				CHECK_QSTATUS();
				break;
			case MessageData::TYPE_ALL:
				status = STLWrapper<UidList>(listAllUid).push_back((*it).getId());
				CHECK_QSTATUS();
				break;
			default:
				break;
			}
			++it;
		}
		
		unsigned int nPos = 0;
		status = pSessionCallback_->setRange(0,
			listHeaderUid.size() + listAllUid.size() +
			listPartial.size()*(nOption & OPTION_USEBODYSTRUCTUREALWAYS ? 1 : 2));
		CHECK_QSTATUS();
		status = pSessionCallback_->setPos(0);
		CHECK_QSTATUS();
		
		if ((nOption & OPTION_USEBODYSTRUCTUREALWAYS) == 0 && !listPartial.empty()) {
			class BodyStructureProcessHook : public AbstractBodyStructureProcessHook
			{
			public:
				typedef std::vector<FetchDataBodyStructure*> BodyStructureList;
			
			public:
				BodyStructureProcessHook(MessageDataList& listMessageData,
					BodyStructureList& listBodyStructure,
					ReceiveSessionCallback* pSessionCallback, unsigned int* pnPos) :
					listMessageData_(listMessageData),
					listBodyStructure_(listBodyStructure),
					pSessionCallback_(pSessionCallback),
					pnPos_(pnPos)
				{
				}
				
			protected:
				virtual qs::QSTATUS setBodyStructure(unsigned long nUid,
					FetchDataBodyStructure* pBodyStructure, bool* pbSet)
				{
					DECLARE_QSTATUS();
					
					MessageDataList::iterator it = std::find_if(
						listMessageData_.begin(), listMessageData_.end(),
						std::bind2nd(
							binary_compose_f_gx_hy(
								std::equal_to<unsigned long>(),
								std::mem_fun_ref(&MessageData::getId),
								std::identity<unsigned long>()),
							nUid));
					if (it != listMessageData_.end()) {
						status = STLWrapper<BodyStructureList>(
							listBodyStructure_).push_back(pBodyStructure);
						CHECK_QSTATUS();
						(*it).setBodyStructure(pBodyStructure);
						*pbSet = true;
					}
					
					return QSTATUS_SUCCESS;
				}
				
				virtual qs::QSTATUS processed()
				{
					++(*pnPos_);
					return pSessionCallback_->setPos(*pnPos_);
				}
			
			private:
				MessageDataList& listMessageData_;
				BodyStructureList& listBodyStructure_;
				ReceiveSessionCallback* pSessionCallback_;
				unsigned int* pnPos_;
			} hook(listPartial, listBodyStructure, pSessionCallback_, &nPos);
			Hook h(this, &hook);
			
			UidList listPartialUid;
			status = STLWrapper<UidList>(listPartialUid).resize(listPartial.size());
			CHECK_QSTATUS();
			std::transform(listPartial.begin(), listPartial.end(),
				listPartialUid.begin(), std::mem_fun_ref(&MessageData::getId));
			MultipleRange range(&listPartialUid[0], listPartialUid.size(), true, &status);
			CHECK_QSTATUS();
			status = pImap4_->getBodyStructure(range);
			CHECK_QSTATUS_ERROR();
			
			bool bMoved = false;
			MessageDataList::iterator it = listPartial.begin();
			while (it != listPartial.end()) {
				FetchDataBodyStructure* pBodyStructure = (*it).getBodyStructure();
				if (!pBodyStructure ||
					!Util::hasAttachmentPart(pBodyStructure)) {
					status = STLWrapper<UidList>(listAllUid).push_back((*it).getId());
					CHECK_QSTATUS();
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
				bool bHeader, unsigned int* pnPos) :
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
				MessageDataList::const_iterator m = std::find_if(
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
						return QSTATUS_SUCCESS;
				}
				
				it_ = m;
				
				return (*it_).getMessagePtr();
			}
			
			virtual QSTATUS processed()
			{
				++(*pnPos_);
				return pSessionCallback_->setPos(*pnPos_);
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
			
			MultipleRange range(&listAllUid[0], listAllUid.size(), true, &status);
			CHECK_QSTATUS();
			status = pImap4_->getMessage(range, true);
			CHECK_QSTATUS_ERROR();
		}
		
		if (!listPartial.empty()) {
			class PartialMessageProcessHook : public AbstractPartialMessageProcessHook
			{
			public:
				PartialMessageProcessHook(Account* pAccount,
					const MessageDataList& listMessageData,
					FetchDataBodyStructure* pBodyStructure,
					const PartList& listPart, unsigned int nPartCount, bool bAll) :
					pAccount_(pAccount),
					listMessageData_(listMessageData),
					it_(listMessageData_.begin()),
					pBodyStructure_(pBodyStructure),
					listPart_(listPart),
					nPartCount_(nPartCount),
					bAll_(bAll)
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
				
				virtual qm::MessagePtr getMessagePtr(unsigned long nUid)
				{
					MessageDataList::const_iterator m = std::find_if(
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
							return QSTATUS_SUCCESS;
					}
					
					it_ = m;
					
					return (*it_).getMessagePtr();
				}
				
				virtual qs::QSTATUS processed()
				{
					return QSTATUS_SUCCESS;
				}
			
			private:
				Account* pAccount_;
				const MessageDataList& listMessageData_;
				MessageDataList::const_iterator it_;
				FetchDataBodyStructure* pBodyStructure_;
				const PartList& listPart_;
				unsigned int nPartCount_;
				bool bAll_;
			};
			
			MessageDataList::iterator it = listPartial.begin();
			while (it != listPartial.end()) {
				Util::PartList listPart;
				Util::PartListDeleter deleter(listPart);
				unsigned int nPath = 0;
				status = Util::getPartsFromBodyStructure(
					(*it).getBodyStructure(), &nPath, &listPart);
				CHECK_QSTATUS();
				
				if (!listPart.empty()) {
					string_ptr<STRING> strArg;
					unsigned int nPartCount = 0;
					bool bAll = false;
					status = Util::getFetchArgFromPartList(listPart,
						(*it).getType() == MessageData::TYPE_HTML ?
							Util::FETCHARG_HTML : Util::FETCHARG_TEXT,
						true, (nOption & OPTION_TRUSTBODYSTRUCTURE) == 0,
						&strArg, &nPartCount, &bAll);
					CHECK_QSTATUS();
					
					PartialMessageProcessHook hook(pAccount_, listMessageData,
						(*it).getBodyStructure(), listPart, nPartCount, bAll);
					Hook h(this, &hook);
					
					SingleRange range((*it).getId(), true, &status);
					CHECK_QSTATUS();
					status = pImap4_->fetch(range, strArg.get());
					CHECK_QSTATUS_ERROR();
				}
				
				status = pSessionCallback_->setPos(nPos);
				CHECK_QSTATUS();
				++nPos;
				
				++it;
			}
		}
		
		if (!listHeaderUid.empty()) {
			MessageProcessHook hook(pAccount_,
				listMessageData, pSessionCallback_, true, &nPos);
			Hook h(this, &hook);
			
			MultipleRange range(&listHeaderUid[0], listHeaderUid.size(), true, &status);
			CHECK_QSTATUS();
			status = pImap4_->getHeader(range, true);
			CHECK_QSTATUS_ERROR();
		}
	}
	
	return QSTATUS_SUCCESS;
}

QSTATUS qmimap4::Imap4ReceiveSession::applyOfflineJobs()
{
	DECLARE_QSTATUS();
	
	status = pCallback_->setMessage(IDS_APPLYOFFLINEJOBS);
	CHECK_QSTATUS();
	
	Imap4Driver* pDriver = static_cast<Imap4Driver*>(
		pAccount_->getProtocolDriver());
	OfflineJobManager* pManager = pDriver->getOfflineJobManager();
	status = pManager->apply(pAccount_, pImap4_, pSessionCallback_);
	CHECK_QSTATUS();
	
	status = downloadReservedMessages();
	CHECK_QSTATUS();
	
	return QSTATUS_SUCCESS;
}

QSTATUS qmimap4::Imap4ReceiveSession::downloadReservedMessages()
{
	DECLARE_QSTATUS();
	
	const Account::FolderList& listFolder = pAccount_->getFolders();
	Account::FolderList::const_iterator it = listFolder.begin();
	while (it != listFolder.end()) {
		Folder* pFolder = *it;
		if (pFolder->getType() == Folder::TYPE_NORMAL &&
			!(pFolder->getFlags() & Folder::FLAG_LOCAL) &&
			pFolder->getFlags() & Folder::FLAG_SYNCABLE) {
			status = downloadReservedMessages(
				static_cast<NormalFolder*>(pFolder));
			CHECK_QSTATUS();
		}
		++it;
	}
	
	return QSTATUS_SUCCESS;
}

QSTATUS qmimap4::Imap4ReceiveSession::downloadReservedMessages(
	NormalFolder* pFolder)
{
	DECLARE_QSTATUS();
	
	if (pFolder->getDownloadCount() != 0) {
		int nOption = 0;
		status = pSubAccount_->getProperty(L"Imap4", L"Option", 0xff, &nOption);
		CHECK_QSTATUS();
		
		Lock<Account> lock(*pAccount_);
		
		status = pFolder->loadMessageHolders();
		CHECK_QSTATUS();
		
		typedef std::vector<unsigned long> UidList;
		UidList listAll;
		UidList listText;
		
		for (unsigned int n = 0; n < pFolder->getCount(); ++n) {
			MessageHolder* pmh = pFolder->getMessage(n);
			unsigned int nFlags = pmh->getFlags();
			if (nFlags & MessageHolder::FLAG_DOWNLOAD) {
				status = STLWrapper<UidList>(listAll).push_back(pmh->getId());
				CHECK_QSTATUS();
			}
			else if (nFlags & MessageHolder::FLAG_DOWNLOADTEXT) {
				status = STLWrapper<UidList>(listText).push_back(pmh->getId());
				CHECK_QSTATUS();
			}
		}
		
		if (!listAll.empty() || !listText.empty()) {
			string_ptr<WSTRING> wstrName;
			status = Util::getFolderName(pFolder, &wstrName);
			CHECK_QSTATUS();
			status = pImap4_->select(wstrName.get());
			CHECK_QSTATUS_ERROR();
		}
		
		if (!listAll.empty()) {
			class MessageProcessHook : public AbstractMessageProcessHook
			{
			public:
				typedef std::vector<unsigned long> UidList;
			
			public:
				MessageProcessHook(NormalFolder* pFolder, const UidList& listUid) :
					pFolder_(pFolder),
					listUid_(listUid)
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
					MessageHolder* pmh = 0;
					UidList::const_iterator it = std::lower_bound(
						listUid_.begin(), listUid_.end(), nUid);
					if (it != listUid_.end() && *it == nUid)
						pmh = pFolder_->getMessageById(nUid);
					return MessagePtr(pmh);
				}
				
				virtual qs::QSTATUS processed()
				{
					return QSTATUS_SUCCESS;
				}
			
			private:
				NormalFolder* pFolder_;
				const UidList& listUid_;
			} hook(pFolder, listAll);
			
			// TODO
			// Callback progress
			
			Hook h(this, &hook);
			
			MultipleRange range(&listAll[0], listAll.size(), true, &status);
			CHECK_QSTATUS();
			status = pImap4_->getMessage(range, true);
			CHECK_QSTATUS_ERROR();
		}
		
		if (!listText.empty()) {
			typedef std::vector<FetchDataBodyStructure*> BodyStructureList;
			BodyStructureList listBodyStructure;
			container_deleter<BodyStructureList> deleter(listBodyStructure);
			status = STLWrapper<BodyStructureList>(
				listBodyStructure).resize(listText.size());
			CHECK_QSTATUS();
			
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
				virtual qs::QSTATUS setBodyStructure(unsigned long nUid,
					FetchDataBodyStructure* pBodyStructure, bool* pbSet)
				{
					UidList::const_iterator it = std::lower_bound(
						listUid_.begin(), listUid_.end(), nUid);
					if (it != listUid_.end() && *it == nUid) {
						listBodyStructure_[it - listUid_.begin()] = pBodyStructure;
						*pbSet = true;
					}
					return QSTATUS_SUCCESS;
				}
				
				virtual qs::QSTATUS processed()
				{
					return QSTATUS_SUCCESS;
				}
			
			private:
				const UidList& listUid_;
				BodyStructureList& listBodyStructure_;
			} hook(listText, listBodyStructure);
			Hook h(this, &hook);
			
			// TODO
			// Callback progress
			
			MultipleRange range(&listText[0], listText.size(), true, &status);
			CHECK_QSTATUS();
			status = pImap4_->getBodyStructure(range);
			CHECK_QSTATUS_ERROR();
			
			class PartialMessageProcessHook : public AbstractPartialMessageProcessHook
			{
			public:
				typedef std::vector<unsigned long> UidList;
			
			public:
				PartialMessageProcessHook(NormalFolder* pFolder,
					const UidList& listUid, FetchDataBodyStructure* pBodyStructure,
					const PartList& listPart, unsigned int nPartCount, bool bAll) :
					pFolder_(pFolder),
					listUid_(listUid),
					pBodyStructure_(pBodyStructure),
					listPart_(listPart),
					nPartCount_(nPartCount),
					bAll_(bAll)
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
					MessageHolder* pmh = 0;
					UidList::const_iterator it = std::lower_bound(
						listUid_.begin(), listUid_.end(), nUid);
					if (it != listUid_.end() && *it == nUid)
						pmh = pFolder_->getMessageById(nUid);
					return MessagePtr(pmh);
				}
				
				virtual qs::QSTATUS processed()
				{
					return QSTATUS_SUCCESS;
				}
			
			private:
				NormalFolder* pFolder_;
				const UidList& listUid_;
				FetchDataBodyStructure* pBodyStructure_;
				const PartList& listPart_;
				unsigned int nPartCount_;
				bool bAll_;
			};
			for (UidList::size_type n = 0; n < listText.size(); ++n) {
				if (listBodyStructure[n]) {
					Util::PartList listPart;
					Util::PartListDeleter deleter(listPart);
					unsigned int nPath = 0;
					status = Util::getPartsFromBodyStructure(
						listBodyStructure[n], &nPath, &listPart);
					CHECK_QSTATUS();
					
					if (!listPart.empty()) {
						string_ptr<STRING> strArg;
						unsigned int nPartCount = 0;
						bool bAll = false;
						status = Util::getFetchArgFromPartList(
							listPart, Util::FETCHARG_TEXT, true,
							(nOption & OPTION_TRUSTBODYSTRUCTURE) == 0,
							&strArg, &nPartCount, &bAll);
						CHECK_QSTATUS();
						
						PartialMessageProcessHook hook(pFolder, listText,
							listBodyStructure[n], listPart, nPartCount, bAll);
						Hook h(this, &hook);
						
						SingleRange range(listText[n], true, &status);
						CHECK_QSTATUS();
						status = pImap4_->fetch(range, strArg.get());
						CHECK_QSTATUS_ERROR();
					}
				}
			}
		}
	}
	
	return QSTATUS_SUCCESS;
}

QSTATUS qmimap4::Imap4ReceiveSession::processCapabilityResponse(
	ResponseCapability* pCapability)
{
	// TODO
	return QSTATUS_SUCCESS;
}

QSTATUS qmimap4::Imap4ReceiveSession::processContinueResponse(
	ResponseContinue* pContinue)
{
	// TODO
	return QSTATUS_SUCCESS;
}

QSTATUS qmimap4::Imap4ReceiveSession::processExistsResponse(
	ResponseExists* pExists)
{
	nExists_ = pExists->getExists();
	return QSTATUS_SUCCESS;
}

QSTATUS qmimap4::Imap4ReceiveSession::processExpungeResponse(
	ResponseExpunge* pExpunge)
{
	// TODO
	return QSTATUS_SUCCESS;
}

QSTATUS qmimap4::Imap4ReceiveSession::processFetchResponse(
	ResponseFetch* pFetch)
{
	DECLARE_QSTATUS();
	
	bool bProcessed = false;
	if (pProcessHook_) {
		status = pProcessHook_->processFetchResponse(
			pFetch, &bProcessed);
		CHECK_QSTATUS();
	}
	if (!bProcessed) {
		// TODO
	}
	return QSTATUS_SUCCESS;
}

QSTATUS qmimap4::Imap4ReceiveSession::processFlagsResponse(
	ResponseFlags* pFlags)
{
	// TODO
	return QSTATUS_SUCCESS;
}

QSTATUS qmimap4::Imap4ReceiveSession::processListResponse(
	ResponseList* pList)
{
	// TODO
	return QSTATUS_SUCCESS;
}

QSTATUS qmimap4::Imap4ReceiveSession::processNamespaceResponse(
	ResponseNamespace* pNamespace)
{
	// TODO
	return QSTATUS_SUCCESS;
}

QSTATUS qmimap4::Imap4ReceiveSession::processRecentResponse(
	ResponseRecent* pRecent)
{
	return QSTATUS_SUCCESS;
}

QSTATUS qmimap4::Imap4ReceiveSession::processSearchResponse(
	ResponseSearch* pSearch)
{
	// TODO
	return QSTATUS_SUCCESS;
}

QSTATUS qmimap4::Imap4ReceiveSession::processStateResponse(
	ResponseState* pState)
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
		return QSTATUS_FAIL;
	}
	
	// TODO
	return QSTATUS_SUCCESS;
}

QSTATUS qmimap4::Imap4ReceiveSession::processStatusResponse(
	ResponseStatus* pStatus)
{
	// TODO
	return QSTATUS_SUCCESS;
}

QSTATUS qmimap4::Imap4ReceiveSession::reportError()
{
	assert(pImap4_);
	
	DECLARE_QSTATUS();
	
	struct
	{
		unsigned int nError_;
		UINT nId_;
	} maps[][23] = {
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
		},
		{
			{ Socket::SOCKET_ERROR_SOCKET,			IDS_ERROR_SOCKET_SOCKET			},
			{ Socket::SOCKET_ERROR_CLOSESOCKET,		IDS_ERROR_SOCKET_CLOSESOCKET	},
			{ Socket::SOCKET_ERROR_LOOKUPNAME,		IDS_ERROR_SOCKET_LOOKUPNAME		},
			{ Socket::SOCKET_ERROR_CONNECT,			IDS_ERROR_SOCKET_CONNECT		},
			{ Socket::SOCKET_ERROR_CONNECTTIMEOUT,	IDS_ERROR_SOCKET_CONNECTTIMEOUT	},
			{ Socket::SOCKET_ERROR_RECV,			IDS_ERROR_SOCKET_RECV			},
			{ Socket::SOCKET_ERROR_RECVTIMEOUT,		IDS_ERROR_SOCKET_RECVTIMEOUT	},
			{ Socket::SOCKET_ERROR_SEND,			IDS_ERROR_SOCKET_SEND			},
			{ Socket::SOCKET_ERROR_SENDTIMEOUT,		IDS_ERROR_SOCKET_SENDTIMEOUT	},
			{ Socket::SOCKET_ERROR_CANCEL,			IDS_ERROR_SOCKET_CANCEL			},
			{ Socket::SOCKET_ERROR_UNKNOWN,			IDS_ERROR_SOCKET_UNKNOWN		}
		}
	};
	
	unsigned int nError = pImap4_->getLastError();
	unsigned int nMasks[] = {
		Imap4::IMAP4_ERROR_MASK_HIGHLEVEL,
		Imap4::IMAP4_ERROR_MASK_LOWLEVEL,
		Socket::SOCKET_ERROR_MASK_SOCKET
	};
	string_ptr<WSTRING> wstrDescriptions[countof(maps)];
	for (int n = 0; n < countof(maps); ++n) {
		for (int m = 0; m < countof(maps[n]) && !wstrDescriptions[n].get(); ++m) {
			if (maps[n][m].nError_ != 0 &&
				(nError & nMasks[n]) == maps[n][m].nError_) {
				status = loadString(getResourceHandle(),
					maps[n][m].nId_, &wstrDescriptions[n]);
				CHECK_QSTATUS();
			}
		}
	}
	
	string_ptr<WSTRING> wstrMessage;
	status = loadString(getResourceHandle(), IDS_ERROR_MESSAGE, &wstrMessage);
	CHECK_QSTATUS();
	
	const WCHAR* pwszDescription[] = {
		wstrDescriptions[0].get(),
		wstrDescriptions[1].get(),
		wstrDescriptions[2].get(),
		pImap4_->getLastErrorResponse()
	};
	SessionErrorInfo info(pAccount_, pSubAccount_, 0, wstrMessage.get(),
		nError, pwszDescription, countof(pwszDescription));
	status = pSessionCallback_->addError(info);
	CHECK_QSTATUS();
	
	return QSTATUS_SUCCESS;
}


/****************************************************************************
 *
 * Imap4ReceiveSession::CallbackImpl
 *
 */

qmimap4::Imap4ReceiveSession::CallbackImpl::CallbackImpl(
	Imap4ReceiveSession* pSession, SubAccount* pSubAccount,
	const Security* pSecurity, ReceiveSessionCallback* pSessionCallback,
	QSTATUS* pstatus) :
	AbstractCallback(pSubAccount, pSecurity, pstatus),
	pSession_(pSession),
	pSessionCallback_(pSessionCallback)
{
}

qmimap4::Imap4ReceiveSession::CallbackImpl::~CallbackImpl()
{
}

QSTATUS qmimap4::Imap4ReceiveSession::CallbackImpl::setMessage(UINT nId)
{
	DECLARE_QSTATUS();
	
	string_ptr<WSTRING> wstrMessage;
	status = loadString(getResourceHandle(), nId, &wstrMessage);
	CHECK_QSTATUS();
	
	return pSessionCallback_->setMessage(wstrMessage.get());
}

bool qmimap4::Imap4ReceiveSession::CallbackImpl::isCanceled(bool bForce) const
{
	return pSessionCallback_->isCanceled(bForce);
}

QSTATUS qmimap4::Imap4ReceiveSession::CallbackImpl::initialize()
{
	return setMessage(IDS_INITIALIZE);
}

QSTATUS qmimap4::Imap4ReceiveSession::CallbackImpl::lookup()
{
	return setMessage(IDS_LOOKUP);
}

QSTATUS qmimap4::Imap4ReceiveSession::CallbackImpl::connecting()
{
	return setMessage(IDS_CONNECTING);
}

QSTATUS qmimap4::Imap4ReceiveSession::CallbackImpl::connected()
{
	return setMessage(IDS_CONNECTED);
}


QSTATUS qmimap4::Imap4ReceiveSession::CallbackImpl::authenticating()
{
	return setMessage(IDS_AUTHENTICATING);
}

QSTATUS qmimap4::Imap4ReceiveSession::CallbackImpl::setRange(
	unsigned int nMin, unsigned int nMax)
{
	return pSessionCallback_->setSubRange(nMin, nMax);
}

QSTATUS qmimap4::Imap4ReceiveSession::CallbackImpl::setPos(unsigned int nPos)
{
	return pSessionCallback_->setSubPos(nPos);
}

QSTATUS qmimap4::Imap4ReceiveSession::CallbackImpl::response(Response* pResponse)
{
	DECLARE_QSTATUS();
	
#define MAP_RESPONSE(type, name) \
	case Response::type: \
		status = pSession_->process##name##Response( \
			static_cast<Response##name*>(pResponse)); \
		CHECK_QSTATUS(); \
		break \
	
	switch (pResponse->getType()) {
	MAP_RESPONSE(TYPE_CAPABILITY, Capability);
	MAP_RESPONSE(TYPE_CONTINUE, Continue);
	MAP_RESPONSE(TYPE_EXISTS, Exists);
	MAP_RESPONSE(TYPE_EXPUNGE, Expunge);
	MAP_RESPONSE(TYPE_FETCH, Fetch);
	MAP_RESPONSE(TYPE_FLAGS, Flags);
	MAP_RESPONSE(TYPE_LIST, List);
	MAP_RESPONSE(TYPE_NAMESPACE, Namespace);
	MAP_RESPONSE(TYPE_RECENT, Recent);
	MAP_RESPONSE(TYPE_SEARCH, Search);
	MAP_RESPONSE(TYPE_STATE, State);
	MAP_RESPONSE(TYPE_STATUS, Status);
	default:
		assert(false);
		return QSTATUS_FAIL;
	}
	
	return QSTATUS_SUCCESS;
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

qmimap4::Imap4ReceiveSessionUI::Imap4ReceiveSessionUI(QSTATUS* pstatus)
{
}

qmimap4::Imap4ReceiveSessionUI::~Imap4ReceiveSessionUI()
{
}

const WCHAR* qmimap4::Imap4ReceiveSessionUI::getClass()
{
	return L"mail";
}

QSTATUS qmimap4::Imap4ReceiveSessionUI::getDisplayName(WSTRING* pwstrName)
{
	assert(pwstrName);
	return loadString(getResourceHandle(), IDS_IMAP4, pwstrName);
}

short qmimap4::Imap4ReceiveSessionUI::getDefaultPort()
{
	return 143;
}

QSTATUS qmimap4::Imap4ReceiveSessionUI::createPropertyPage(
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
 * Imap4ReceiveSessionFactory
 *
 */

Imap4ReceiveSessionFactory qmimap4::Imap4ReceiveSessionFactory::factory__;

qmimap4::Imap4ReceiveSessionFactory::Imap4ReceiveSessionFactory()
{
	regist(L"imap4", this);
}

qmimap4::Imap4ReceiveSessionFactory::~Imap4ReceiveSessionFactory()
{
	unregist(L"imap4");
}

QSTATUS qmimap4::Imap4ReceiveSessionFactory::createSession(
	ReceiveSession** ppReceiveSession)
{
	assert(ppReceiveSession);
	
	DECLARE_QSTATUS();
	
	Imap4ReceiveSession* pSession = 0;
	status = newQsObject(&pSession);
	CHECK_QSTATUS();
	
	*ppReceiveSession = pSession;
	
	return QSTATUS_SUCCESS;
}

QSTATUS qmimap4::Imap4ReceiveSessionFactory::createUI(ReceiveSessionUI** ppUI)
{
	assert(ppUI);
	
	DECLARE_QSTATUS();
	
	Imap4ReceiveSessionUI* pUI = 0;
	status = newQsObject(&pUI);
	CHECK_QSTATUS();
	
	*ppUI = pUI;
	
	return QSTATUS_SUCCESS;
}


/****************************************************************************
 *
 * Imap4SyncFilterCallback
 *
 */

qmimap4::Imap4SyncFilterCallback::Imap4SyncFilterCallback(
	Document* pDocument, Account* pAccount,
	NormalFolder* pFolder, Message* pMessage, unsigned int nUid,
	unsigned int nSize, unsigned int nTextSize, HWND hwnd, Profile* pProfile,
	MacroVariableHolder* pGlobalVariable, Imap4ReceiveSession* pSession) :
	pDocument_(pDocument),
	pAccount_(pAccount),
	pFolder_(pFolder),
	pMessage_(pMessage),
	nUid_(nUid),
	nSize_(nSize),
	nTextSize_(nTextSize),
	hwnd_(hwnd),
	pProfile_(pProfile),
	pGlobalVariable_(pGlobalVariable),
	pSession_(pSession),
	pmh_(0)
{
}

qmimap4::Imap4SyncFilterCallback::~Imap4SyncFilterCallback()
{
	delete pmh_;
}

QSTATUS qmimap4::Imap4SyncFilterCallback::getMessage(unsigned int nFlags)
{
	// TODO
	
	return QSTATUS_SUCCESS;
}

const NormalFolder* qmimap4::Imap4SyncFilterCallback::getFolder()
{
	return pFolder_;
}

QSTATUS qmimap4::Imap4SyncFilterCallback::getMacroContext(MacroContext** ppContext)
{
	assert(ppContext);
	
	DECLARE_QSTATUS();
	
	if (!pmh_) {
		status = newQsObject(this, pFolder_, pMessage_,
			nUid_, nSize_, nTextSize_, &pmh_);
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
		pGlobalVariable_,
	};
	std::auto_ptr<MacroContext> pContext;
	status = newQsObject(init, &pContext);
	CHECK_QSTATUS();
	
	*ppContext = pContext.release();
	
	return QSTATUS_SUCCESS;
}


/****************************************************************************
 *
 * Imap4MessageHolder
 *
 */

qmimap4::Imap4MessageHolder::Imap4MessageHolder(
	Imap4SyncFilterCallback* pCallback, NormalFolder* pFolder,
	Message* pMessage, unsigned int nId, unsigned int nSize,
	unsigned int nTextSize, QSTATUS* pstatus) :
	AbstractMessageHolder(pFolder, pMessage, nId, nSize, nTextSize, pstatus),
	pCallback_(pCallback)
{
	assert(pstatus);
	*pstatus = QSTATUS_SUCCESS;
}

qmimap4::Imap4MessageHolder::~Imap4MessageHolder()
{
}

QSTATUS qmimap4::Imap4MessageHolder::getMessage(unsigned int nFlags,
	const WCHAR* pwszField, Message* pMessage)
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
		if (wcsicmp(pwszField, pwszFields[n]) == 0)
			return QSTATUS_SUCCESS;
	}
	
	return pCallback_->getMessage(nFlags);
}
