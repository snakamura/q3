/*
 * $Id$
 *
 * Copyright(C) 1998-2003 Satoshi Nakamura
 * All rights reserved.
 *
 */

#include <qmaccount.h>
#include <qmmessage.h>
#include <qmmessageholder.h>
#include <qmfolder.h>

#include <qserror.h>
#include <qsassert.h>
#include <qsmime.h>
#include <qsutil.h>
#include <qsthread.h>

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
	const Init& init, QSTATUS* pstatus)
{
	assert(pstatus);
	
	*pstatus = QSTATUS_SUCCESS;
	
	nId_ = init.nId_;
	nFlags_ = init.nFlags_;
	nMessageIdHash_ = static_cast<unsigned int>(-1);
	nReferenceHash_ = static_cast<unsigned int>(-1);
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
	Lock<Folder> lock(*pFolder_);
	return nFlags_;
}

QSTATUS qm::MessageHolder::getFrom(WSTRING* pwstrFrom) const
{
	Lock<Folder> lock(*pFolder_);
	return pFolder_->getData(messageCacheKey_, ITEM_FROM, pwstrFrom);
}

QSTATUS qm::MessageHolder::getTo(WSTRING* pwstrTo) const
{
	Lock<Folder> lock(*pFolder_);
	return pFolder_->getData(messageCacheKey_, ITEM_TO, pwstrTo);
}

QSTATUS qm::MessageHolder::getFromTo(WSTRING* pwstrFromTo) const
{
	if (isFlag(FLAG_SENT))
		return getTo(pwstrFromTo);
	else
		return getFrom(pwstrFromTo);
}

QSTATUS qm::MessageHolder::getDate(Time* pTime) const
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
	
	return QSTATUS_SUCCESS;
}

QSTATUS qm::MessageHolder::getSubject(WSTRING* pwstrSubject) const
{
	Lock<Folder> lock(*pFolder_);
	return pFolder_->getData(messageCacheKey_, ITEM_SUBJECT, pwstrSubject);
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

QSTATUS qm::MessageHolder::getMessage(unsigned int nFlags,
	const WCHAR* pwszField, Message* pMessage)
{
	int nMethod = nFlags & Account::GETMESSAGEFLAG_METHOD_MASK;
	switch (pMessage->getFlag()) {
	case Message::FLAG_EMPTY:
		break;
	case Message::FLAG_NONE:
		return QSTATUS_SUCCESS;
	case Message::FLAG_HEADERONLY:
		if (nMethod == Account::GETMESSAGEFLAG_HEADER)
			return QSTATUS_SUCCESS;
		break;
	case Message::FLAG_TEXTONLY:
		if (nMethod == Account::GETMESSAGEFLAG_HEADER ||
			nMethod == Account::GETMESSAGEFLAG_TEXT)
			return QSTATUS_SUCCESS;
		break;
	case Message::FLAG_HTMLONLY:
		if (nMethod == Account::GETMESSAGEFLAG_HEADER ||
			nMethod == Account::GETMESSAGEFLAG_TEXT ||
			nMethod == Account::GETMESSAGEFLAG_HTML)
			return QSTATUS_SUCCESS;
		break;
	case Message::FLAG_TEMPORARY:
		break;
	default:
		assert(false);
		return QSTATUS_FAIL;
	}
	
	return pFolder_->getMessage(this, nFlags, pMessage);
}

MessageHolder* qm::MessageHolder::getMessageHolder()
{
	return this;
}

bool qm::MessageHolder::isFlag(Flag flag) const
{
	Lock<Folder> lock(*pFolder_);
	return (nFlags_ & flag) != 0;
}

QSTATUS qm::MessageHolder::getMessageId(WSTRING* pwstrMessageId) const
{
	Lock<Folder> lock(*pFolder_);
	return pFolder_->getData(messageCacheKey_, ITEM_MESSAGEID, pwstrMessageId);
}

unsigned int qm::MessageHolder::getMessageIdHash() const
{
	if (nMessageIdHash_ == static_cast<unsigned int>(-1)) {
		Lock<Folder> lock(*pFolder_);
		if (nMessageIdHash_ == static_cast<unsigned int>(-1)) {
			DECLARE_QSTATUS();
			string_ptr<WSTRING> wstrMessageId;
			status = getMessageId(&wstrMessageId);
			// TODO
			if (status == QSTATUS_SUCCESS)
				nMessageIdHash_ = MessageHolderImpl::hash(wstrMessageId.get());
			else
				nMessageIdHash_ = 0;
		}
	}
	return nMessageIdHash_;
}

QSTATUS qm::MessageHolder::getReference(WSTRING* pwstrReference) const
{
	Lock<Folder> lock(*pFolder_);
	return pFolder_->getData(messageCacheKey_, ITEM_REFERENCE, pwstrReference);
}

unsigned int qm::MessageHolder::getReferenceHash() const
{
	if (nReferenceHash_ == static_cast<unsigned int>(-1)) {
		Lock<Folder> lock(*pFolder_);
		if (nReferenceHash_ == static_cast<unsigned int>(-1)) {
			DECLARE_QSTATUS();
			string_ptr<WSTRING> wstrReference;
			status = getReference(&wstrReference);
			// TODO
			if (status == QSTATUS_SUCCESS)
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
	Lock<Folder> lock(*pFolder_);
	
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
	assert(pFolder_->isLocked());
	nId_ = nId;
}

void qm::MessageHolder::setFlags(unsigned int nFlags, unsigned int nMask)
{
	Lock<Folder> lock(*pFolder_);
	
	unsigned int nOldFlags = nFlags_;
	
	nFlags_ &= ~nMask;
	nFlags_ |= nFlags & nMask;
	
	if (nOldFlags != nFlags_)
		pFolder_->fireMessageFlagChanged(this, nOldFlags, nFlags_);
}

void qm::MessageHolder::setFolder(NormalFolder* pFolder)
{
	assert(pFolder_->isLocked());
	assert(pFolder->isLocked());
	pFolder_ = pFolder;
}

void qm::MessageHolder::setKeys(MessageCacheKey messageCacheKey,
	const MessageBoxKey& messageBoxKey)
{
	Lock<Folder> lock(*pFolder_);
	
	messageCacheKey_ = messageCacheKey;
	messageBoxKey_ = messageBoxKey;
	
	nMessageIdHash_ = static_cast<unsigned int>(-1);
	nReferenceHash_ = static_cast<unsigned int>(-1);
}


/****************************************************************************
 *
 * AbstractMessageHolder
 *
 */

qm::AbstractMessageHolder::AbstractMessageHolder(NormalFolder* pFolder,
	Message* pMessage, unsigned int nId, unsigned int nSize,
	unsigned int nTextSize, QSTATUS* pstatus) :
	pFolder_(pFolder),
	pMessage_(pMessage),
	nId_(nId),
	nSize_(nSize),
	nTextSize_(nTextSize)
{
	assert(pstatus);
	*pstatus = QSTATUS_SUCCESS;
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

QSTATUS qm::AbstractMessageHolder::getFrom(WSTRING* pwstrFrom) const
{
	return getAddress(L"From", pwstrFrom);
}

QSTATUS qm::AbstractMessageHolder::getTo(WSTRING* pwstrTo) const
{
	return getAddress(L"To", pwstrTo);
}

QSTATUS qm::AbstractMessageHolder::getFromTo(WSTRING* pwstrFromTo) const
{
	return getFrom(pwstrFromTo);
}

QSTATUS qm::AbstractMessageHolder::getSubject(WSTRING* pwstrSubject) const
{
	assert(pwstrSubject);
	
	DECLARE_QSTATUS();
	
	UnstructuredParser subject(&status);
	CHECK_QSTATUS();
	Part::Field f;
	status = pMessage_->getField(L"Subject", &subject, &f);
	CHECK_QSTATUS();
	if (f == Part::FIELD_EXIST)
		*pwstrSubject = allocWString(subject.getValue());
	else
		*pwstrSubject = allocWString(L"");
	if (!*pwstrSubject)
		return QSTATUS_OUTOFMEMORY;
	
	return QSTATUS_SUCCESS;
}

QSTATUS qm::AbstractMessageHolder::getDate(Time* pTime) const
{
	assert(pTime);
	
	DECLARE_QSTATUS();
	
	DateParser date(&status);
	CHECK_QSTATUS();
	Part::Field f;
	status = pMessage_->getField(L"Date", &date, &f);
	CHECK_QSTATUS();
	if (f == Part::FIELD_EXIST)
		*pTime = date.getTime();
	else
		*pTime = Time::getCurrentTime();
	
	return QSTATUS_SUCCESS;
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

MessageHolder* qm::AbstractMessageHolder::getMessageHolder()
{
	return 0;
}

Message* qm::AbstractMessageHolder::getMessage() const
{
	return pMessage_;
}

QSTATUS qm::AbstractMessageHolder::getAddress(
	const WCHAR* pwszName, WSTRING* pwstrValue) const
{
	assert(pwszName);
	assert(pwstrValue);
	
	DECLARE_QSTATUS();
	
	AddressListParser address(0, &status);
	CHECK_QSTATUS();
	Part::Field f;
	status = pMessage_->getField(pwszName, &address, &f);
	CHECK_QSTATUS();
	if (f == Part::FIELD_EXIST) {
		status = address.getNames(pwstrValue);
		CHECK_QSTATUS();
	}
	else {
		*pwstrValue = allocWString(L"");
		if (!*pwstrValue)
			return QSTATUS_OUTOFMEMORY;
	}
	
	return QSTATUS_SUCCESS;
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

Folder* qm::MessagePtr::getFolder() const
{
	return pFolder_;
}

MessageHolder* qm::MessagePtr::lock() const
{
	assert(!bLock_);
	
	bLock_ = true;
	
	if (pFolder_) {
		pFolder_->lock();
		return pFolder_->getMessageById(nId_);
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
		pFolder_->unlock();
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
