/*
 * $Id$
 *
 * Copyright(C) 1998-2004 Satoshi Nakamura
 * All rights reserved.
 *
 */

#include <qmaccount.h>
#include <qmmessage.h>
#include <qmmessageholder.h>
#include <qmfolder.h>

#include <qsassert.h>
#include <qsmime.h>
#include <qsthread.h>
#include <qsutil.h>

#include <cstdio>

#include "messagecache.h"

using namespace qm;
using namespace qs;


namespace qm {
struct MessageHolderImpl;
}

/****************************************************************************
 *
 * MessageHolderBase
 *
 */

qm::MessageHolderBase::~MessageHolderBase()
{
}


/****************************************************************************
 *
 * MessageHolderImpl
 *
 */

struct qm::MessageHolderImpl
{
	static unsigned int hash(const WCHAR* pwsz);
};

unsigned int MessageHolderImpl::hash(const WCHAR* pwsz)
{
	assert(pwsz);
	
	unsigned int nHash = 0;
	
	while (*pwsz)
		nHash += static_cast<unsigned int>(*pwsz++);
	if (nHash == static_cast<unsigned int>(-1))
		nHash = 1;
	
	return nHash;
}


/****************************************************************************
 *
 * MessageHolder
 *
 */

qm::MessageHolder::MessageHolder(NormalFolder* pFolder,
								 const Init& init)
{
	nId_ = init.nId_;
	nFlags_ = init.nFlags_;
	nMessageIdHash_ = -1;
	nReferenceHash_ = -1;
	nDate_ = init.nDate_;
	nTime_ = init.nTime_;
	nSize_ = init.nSize_;
	messageCacheKey_ = init.nCacheKey_;
	messageBoxKey_.nOffset_ = init.nOffset_;
	messageBoxKey_.nLength_ = init.nLength_;
	messageBoxKey_.nHeaderLength_ = init.nHeaderLength_;
	pFolder_ = pFolder;
}

qm::MessageHolder::~MessageHolder()
{
}

void* qm::MessageHolder::operator new(size_t n)
{
	assert(n == sizeof(MessageHolder));
	return std::__sgi_alloc::allocate(n);
}

void qm::MessageHolder::operator delete(void* p)
{
	std::__sgi_alloc::deallocate(p, sizeof(MessageHolder));
}

unsigned int qm::MessageHolder::getId() const
{
	return nId_;
}

unsigned int qm::MessageHolder::getFlags() const
{
	Lock<Account> lock(*getAccount());
	return nFlags_;
}

wstring_ptr qm::MessageHolder::getFrom() const
{
	Lock<Account> lock(*getAccount());
	return getAccount()->getData(messageCacheKey_, ITEM_FROM);
}

wstring_ptr qm::MessageHolder::getTo() const
{
	Lock<Account> lock(*getAccount());
	return getAccount()->getData(messageCacheKey_, ITEM_TO);
}

wstring_ptr qm::MessageHolder::getFromTo() const
{
	if (isFlag(FLAG_SENT))
		return getTo();
	else
		return getFrom();
}

void qm::MessageHolder::getDate(Time* pTime) const
{
	assert(pTime);
	
	pTime->wYear = (nDate_ >> 16) & 0xffff;
	pTime->wMonth = (nDate_ >> 8) & 0xff;
	pTime->wDay = nDate_ & 0xff;
	pTime->wHour = (nTime_ >> 25) & 0x3f;
	pTime->wMinute = (nTime_ >> 19) & 0x3f;
	pTime->wSecond = (nTime_ >> 13) & 0x3f;
	int nTimeZone = ((nTime_ & 0x1000) ? -1 : 1)*
		(((nTime_ >> 6) & 0x3f)*100 + (nTime_ & 0x3f));
	pTime->setTimeZone(nTimeZone);
}

wstring_ptr qm::MessageHolder::getSubject() const
{
	Lock<Account> lock(*getAccount());
	return getAccount()->getData(messageCacheKey_, ITEM_SUBJECT);
}

unsigned int qm::MessageHolder::getSize() const
{
	return nSize_;
}

unsigned int qm::MessageHolder::getTextSize() const
{
	// TODO
	return nSize_;
}

NormalFolder* qm::MessageHolder::getFolder() const
{
	return pFolder_;
}

Account* qm::MessageHolder::getAccount() const
{
	return pFolder_->getAccount();
}

bool qm::MessageHolder::getMessage(unsigned int nFlags,
								   const WCHAR* pwszField,
								   Message* pMessage)
{
	int nMethod = nFlags & Account::GETMESSAGEFLAG_METHOD_MASK;
	switch (pMessage->getFlag()) {
	case Message::FLAG_EMPTY:
		break;
	case Message::FLAG_NONE:
		return true;
	case Message::FLAG_HEADERONLY:
		if (nMethod == Account::GETMESSAGEFLAG_HEADER)
			return true;
		break;
	case Message::FLAG_TEXTONLY:
		if (nMethod == Account::GETMESSAGEFLAG_HEADER ||
			nMethod == Account::GETMESSAGEFLAG_TEXT)
			return true;
		break;
	case Message::FLAG_HTMLONLY:
		if (nMethod == Account::GETMESSAGEFLAG_HEADER ||
			nMethod == Account::GETMESSAGEFLAG_TEXT ||
			nMethod == Account::GETMESSAGEFLAG_HTML)
			return true;
		break;
	case Message::FLAG_TEMPORARY:
		break;
	default:
		assert(false);
		return false;
	}
	
	return getAccount()->getMessage(this, nFlags, pMessage);
}

MessageHolder* qm::MessageHolder::getMessageHolder()
{
	return this;
}

bool qm::MessageHolder::isFlag(Flag flag) const
{
	Lock<Account> lock(*getAccount());
	return (nFlags_ & flag) != 0;
}

wstring_ptr qm::MessageHolder::getMessageId() const
{
	Lock<Account> lock(*getAccount());
	return getAccount()->getData(messageCacheKey_, ITEM_MESSAGEID);
}

unsigned int qm::MessageHolder::getMessageIdHash() const
{
	if (nMessageIdHash_ == -1) {
		Lock<Account> lock(*getAccount());
		if (nMessageIdHash_ == -1) {
			wstring_ptr wstrMessageId(getMessageId());
			if (wstrMessageId.get() && *wstrMessageId.get())
				nMessageIdHash_ = MessageHolderImpl::hash(wstrMessageId.get());
			else
				nMessageIdHash_ = 0;
		}
	}
	return nMessageIdHash_;
}

wstring_ptr qm::MessageHolder::getReference() const
{
	Lock<Account> lock(*getAccount());
	return getAccount()->getData(messageCacheKey_, ITEM_REFERENCE);
}

unsigned int qm::MessageHolder::getReferenceHash() const
{
	if (nReferenceHash_ == -1) {
		Lock<Account> lock(*getAccount());
		if (nReferenceHash_ == -1) {
			wstring_ptr wstrReference(getReference());
			if (wstrReference.get() && *wstrReference.get())
				nReferenceHash_ = MessageHolderImpl::hash(wstrReference.get());
			else
				nReferenceHash_ = 0;
		}
	}
	return nReferenceHash_;
}

MessageCacheKey qm::MessageHolder::getMessageCacheKey() const
{
	return messageCacheKey_;
}

const MessageHolder::MessageBoxKey& qm::MessageHolder::getMessageBoxKey() const
{
	return messageBoxKey_;
}

unsigned int qm::MessageHolder::getDate(const qs::Time& time)
{
	return time.wYear << 16 | time.wMonth << 8 | time.wDay;
}

unsigned int qm::MessageHolder::getTime(const qs::Time& time)
{
	int nTimeZone = time.getTimeZone();
	int nAbsTimeZone = nTimeZone > 0 ? nTimeZone : -nTimeZone;
	return time.wHour << 25 | time.wMinute << 19 | time.wSecond << 13 |
		(nTimeZone < 0 ? 1 : 0) << 12 | (nAbsTimeZone/100) << 6 | nAbsTimeZone%100;
}

void qm::MessageHolder::getInit(Init* pInit) const
{
	Lock<Account> lock(*getAccount());
	
	pInit->nId_ = nId_;
	pInit->nFlags_ = nFlags_;
	pInit->nDate_ = nDate_;
	pInit->nTime_ = nTime_;
	pInit->nSize_ = nSize_;
	pInit->nCacheKey_ = messageCacheKey_;
	pInit->nOffset_ = messageBoxKey_.nOffset_;
	pInit->nLength_ = messageBoxKey_.nLength_;
	pInit->nHeaderLength_ = messageBoxKey_.nHeaderLength_;
}

void qm::MessageHolder::setId(unsigned int nId)
{
	assert(getAccount()->isLocked());
	nId_ = nId;
}

void qm::MessageHolder::setFlags(unsigned int nFlags,
								 unsigned int nMask)
{
	Lock<Account> lock(*getAccount());
	
	unsigned int nOldFlags = nFlags_;
	
	nFlags_ &= ~nMask;
	nFlags_ |= nFlags & nMask;
	
	if (nOldFlags != nFlags_)
		getAccount()->fireMessageHolderChanged(this, nOldFlags, nFlags_);
}

void qm::MessageHolder::setFolder(NormalFolder* pFolder)
{
	assert(getAccount()->isLocked());
	pFolder_ = pFolder;
}

void qm::MessageHolder::destroy()
{
	assert(getAccount()->isLocked());
	getAccount()->fireMessageHolderDestroyed(this);
}

void qm::MessageHolder::setKeys(MessageCacheKey messageCacheKey,
								const MessageBoxKey& messageBoxKey)
{
	Lock<Account> lock(*getAccount());
	
	if (messageCacheKey != -1) {
		messageCacheKey_ = messageCacheKey;
		
		nMessageIdHash_ = static_cast<unsigned int>(-1);
		nReferenceHash_ = static_cast<unsigned int>(-1);
	}
	messageBoxKey_ = messageBoxKey;
}


/****************************************************************************
 *
 * AbstractMessageHolder
 *
 */

qm::AbstractMessageHolder::AbstractMessageHolder(NormalFolder* pFolder,
												 Message* pMessage,
												 unsigned int nId,
												 unsigned int nSize,
												 unsigned int nTextSize) :
	pFolder_(pFolder),
	pMessage_(pMessage),
	nId_(nId),
	nSize_(nSize),
	nTextSize_(nTextSize)
{
}

qm::AbstractMessageHolder::~AbstractMessageHolder()
{
}

unsigned int qm::AbstractMessageHolder::getId() const
{
	return nId_;
}

unsigned int qm::AbstractMessageHolder::getFlags() const
{
	// TODO
	return 0;
}

wstring_ptr qm::AbstractMessageHolder::getFrom() const
{
	return getAddress(L"From");
}

wstring_ptr qm::AbstractMessageHolder::getTo() const
{
	return getAddress(L"To");
}

wstring_ptr qm::AbstractMessageHolder::getFromTo() const
{
	return getFrom();
}

wstring_ptr qm::AbstractMessageHolder::getSubject() const
{
	UnstructuredParser subject;
	if (pMessage_->getField(L"Subject", &subject) == Part::FIELD_EXIST)
		return allocWString(subject.getValue());
	else
		return allocWString(L"");
}

void qm::AbstractMessageHolder::getDate(Time* pTime) const
{
	assert(pTime);
	
	DateParser date;
	if (pMessage_->getField(L"Date", &date) == Part::FIELD_EXIST)
		*pTime = date.getTime();
	else
		*pTime = Time::getCurrentTime();
}

unsigned int qm::AbstractMessageHolder::getSize() const
{
	return nSize_;
}

unsigned int qm::AbstractMessageHolder::getTextSize() const
{
	return nTextSize_;
}

NormalFolder* qm::AbstractMessageHolder::getFolder() const
{
	return pFolder_;
}

Account* qm::AbstractMessageHolder::getAccount() const
{
	return pFolder_->getAccount();
}

MessageHolder* qm::AbstractMessageHolder::getMessageHolder()
{
	return 0;
}

Message* qm::AbstractMessageHolder::getMessage() const
{
	return pMessage_;
}

wstring_ptr qm::AbstractMessageHolder::getAddress(const WCHAR* pwszName) const
{
	assert(pwszName);
	
	AddressListParser address(0);
	if (pMessage_->getField(pwszName, &address) == Part::FIELD_EXIST)
		return address.getNames();
	else
		return allocWString(L"");
}


/****************************************************************************
 *
 * MessageHolderHandler
 *
 */

qm::MessageHolderHandler::~MessageHolderHandler()
{
}


/****************************************************************************
 *
 * MessageHolderEvent
 *
 */

qm::MessageHolderEvent::MessageHolderEvent(MessageHolder* pmh) :
	pmh_(pmh),
	nOldFlags_(0),
	nNewFlags_(0)
{
}

qm::MessageHolderEvent::MessageHolderEvent(MessageHolder* pmh,
										   unsigned int nOldFlags,
										   unsigned int nNewFlags) :
	pmh_(pmh),
	nOldFlags_(nOldFlags),
	nNewFlags_(nNewFlags)
{
}

qm::MessageHolderEvent::~MessageHolderEvent()
{
}

MessageHolder* qm::MessageHolderEvent::getMessageHolder() const
{
	return pmh_;
}

unsigned int qm::MessageHolderEvent::getOldFlags() const
{
	return nOldFlags_;
}

unsigned int qm::MessageHolderEvent::getNewFlags() const
{
	return nNewFlags_;
}


/****************************************************************************
 *
 * MessagePtr
 *
 */

qm::MessagePtr::MessagePtr() :
	pFolder_(0),
	nId_(0),
	bLock_(false)
{
}

qm::MessagePtr::MessagePtr(MessageHolder* pmh) :
	pFolder_(0),
	nId_(0),
	bLock_(false)
{
	reset(pmh);
}

qm::MessagePtr::MessagePtr(const MessagePtr& ptr) :
	pFolder_(ptr.pFolder_),
	nId_(ptr.nId_),
	bLock_(false)
{
	assert(!ptr.bLock_);
	bLock_ = false;
}

qm::MessagePtr::~MessagePtr()
{
	assert(!bLock_);
}

qm::MessagePtr& qm::MessagePtr::operator=(const MessagePtr& ptr)
{
	assert(!bLock_);
	assert(!ptr.bLock_);
	
	if (&ptr != this) {
		pFolder_ = ptr.pFolder_;
		nId_ = ptr.nId_;
	}
	return *this;
}

NormalFolder* qm::MessagePtr::getFolder() const
{
	return pFolder_;
}

MessageHolder* qm::MessagePtr::lock() const
{
	assert(!bLock_);
	
	bLock_ = true;
	
	if (pFolder_) {
		pFolder_->getAccount()->lock();
		return pFolder_->getMessageHolderById(nId_);
	}
	else {
		return 0;
	}
}

void qm::MessagePtr::unlock() const
{
	assert(bLock_);
	
	bLock_ = false;
	
	if (pFolder_)
		pFolder_->getAccount()->unlock();
}

void qm::MessagePtr::reset(MessageHolder* pmh)
{
	bool bLock = bLock_;
	if (bLock)
		unlock();
	
	if (pmh) {
		pFolder_ = pmh->getFolder();
		nId_ = pmh->getId();
	}
	else {
		pFolder_ = 0;
		nId_ = 0;
	}
	
	if (bLock)
		lock();
}


/****************************************************************************
 *
 * MessagePtrLock
 *
 */

qm::MessagePtrLock::MessagePtrLock(const MessagePtr& ptr) :
	ptr_(ptr)
{
	pmh_ = ptr_.lock();
}

qm::MessagePtrLock::~MessagePtrLock()
{
	ptr_.unlock();
}

MessagePtrLock::operator qm::MessageHolder*() const
{
	return pmh_;
}

MessageHolder* qm::MessagePtrLock::operator->() const
{
	return pmh_;
}
