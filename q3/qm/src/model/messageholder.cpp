/*
 * $Id$
 *
 * Copyright(C) 1998-2005 Satoshi Nakamura
 * All rights reserved.
 *
 */

#include <qmaccount.h>
#include <qmmessage.h>
#include <qmmessageholder.h>
#include <qmmessageindex.h>
#include <qmfolder.h>

#include <qsassert.h>
#include <qsmime.h>
#include <qsthread.h>
#include <qsutil.h>

#include <cstdio>

using namespace qm;
using namespace qs;


namespace qm {
struct MessageHolderImpl;
}


/****************************************************************************
 *
 * MessageDate
 *
 */

void qm::MessageDate::getTime(unsigned int nDate,
							  unsigned int nTime,
							  qs::Time* pTime)
{
	assert(pTime);
	
	pTime->wYear = (nDate >> 16) & 0xffff;
	pTime->wMonth = (nDate >> 8) & 0xff;
	pTime->wDay = nDate & 0xff;
	pTime->wHour = (nTime >> 25) & 0x3f;
	pTime->wMinute = (nTime >> 19) & 0x3f;
	pTime->wSecond = (nTime >> 13) & 0x3f;
	int nTimeZone = ((nTime & 0x1000) ? -1 : 1)*
		(((nTime >> 6) & 0x3f)*100 + (nTime & 0x3f));
	pTime->setTimeZone(nTimeZone);
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
	if (nHash == -1)
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
	date_ = MessageDate(init.nDate_, init.nTime_);
	nSize_ = init.nSize_;
	messageIndexKey_.nKey_ = init.nIndexKey_;
	messageIndexKey_.nLength_ = init.nIndexLength_;
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

bool qm::MessageHolder::getMessage(unsigned int nFlags,
								   const WCHAR* pwszField,
								   unsigned int nSecurityMode,
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
	
	return getAccount()->getMessage(this, nFlags, nSecurityMode, pMessage);
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

void qm::MessageHolder::getInit(Init* pInit) const
{
	Lock<Account> lock(*getAccount());
	
	pInit->nId_ = nId_;
	pInit->nFlags_ = nFlags_;
	pInit->nDate_ = date_.getDate();
	pInit->nTime_ = date_.getTime();
	pInit->nSize_ = nSize_;
	pInit->nIndexKey_ = messageIndexKey_.nKey_;
	pInit->nIndexLength_ = messageIndexKey_.nLength_;
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
	assert(getAccount()->isLocked());
	
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

void qm::MessageHolder::setKeys(const MessageIndexKey& messageIndexKey,
								const MessageBoxKey& messageBoxKey)
{
	Lock<Account> lock(*getAccount());
	
	if (messageIndexKey.nKey_ != -1) {
		messageIndexKey_ = messageIndexKey;
		
		nMessageIdHash_ = -1;
		nReferenceHash_ = -1;
	}
	messageBoxKey_ = messageBoxKey;
	
	getAccount()->fireMessageHolderChanged(this, nFlags_, nFlags_);
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

wstring_ptr qm::AbstractMessageHolder::getAddress(const WCHAR* pwszName) const
{
	assert(pwszName);
	
	AddressListParser address;
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
