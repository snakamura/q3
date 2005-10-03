/*
 * $Id$
 *
 * Copyright(C) 1998-2005 Satoshi Nakamura
 * All rights reserved.
 *
 */

#ifndef __QMMESSAGEHOLDER_INL__
#define __QMMESSAGEHOLDER_INL__

#include <qmaccount.h>
#include <qmfolder.h>
#include <qmmessageindex.h>

#include <qsthread.h>


/****************************************************************************
 *
 * MessageDate
 *
 */

inline qm::MessageDate::MessageDate() :
	nDate_(0),
	nTime_(0)
{
}

inline qm::MessageDate::MessageDate(unsigned int nDate,
									unsigned int nTime) :
	nDate_(nDate),
	nTime_(nTime)
{
}

inline qm::MessageDate::MessageDate(const MessageDate& date) :
	nDate_(date.nDate_),
	nTime_(date.nTime_)
{
}

inline qm::MessageDate::~MessageDate()
{
}

inline qm::MessageDate& qm::MessageDate::operator=(const MessageDate& date)
{
	if (&date != this) {
		nDate_ = date.nDate_;
		nTime_ = date.nTime_;
	}
	return *this;
}

inline unsigned int qm::MessageDate::getDate() const
{
	return nDate_;
}

inline unsigned int qm::MessageDate::getTime() const
{
	return nTime_;
}

inline void qm::MessageDate::getTime(qs::Time* pTime) const
{
	getTime(nDate_, nTime_, pTime);
}

inline unsigned int qm::MessageDate::getDate(const qs::Time& time)
{
	return time.wYear << 16 | time.wMonth << 8 | time.wDay;
}

inline unsigned int qm::MessageDate::getTime(const qs::Time& time)
{
	int nTimeZone = time.getTimeZone();
	int nAbsTimeZone = nTimeZone > 0 ? nTimeZone : -nTimeZone;
	return time.wHour << 25 | time.wMinute << 19 | time.wSecond << 13 |
		(nTimeZone < 0 ? 1 : 0) << 12 | (nAbsTimeZone/100) << 6 | nAbsTimeZone%100;
}


/****************************************************************************
 *
 * MessageHolder
 *
 */

inline unsigned int qm::MessageHolder::getId() const
{
	return nId_;
}

inline unsigned int qm::MessageHolder::getFlags() const
{
	qs::Lock<Account> lock(*getAccount());
	return nFlags_;
}

inline qs::wstring_ptr qm::MessageHolder::getFrom() const
{
	qs::Lock<Account> lock(*getAccount());
	return getAccount()->getIndex(messageIndexKey_.nKey_,
		messageIndexKey_.nLength_, NAME_FROM);
}

inline qs::wstring_ptr qm::MessageHolder::getTo() const
{
	qs::Lock<Account> lock(*getAccount());
	return getAccount()->getIndex(messageIndexKey_.nKey_,
		messageIndexKey_.nLength_, NAME_TO);
}

inline qs::wstring_ptr qm::MessageHolder::getFromTo() const
{
	if (isFlag(FLAG_SENT))
		return getTo();
	else
		return getFrom();
}

inline void qm::MessageHolder::getDate(qs::Time* pTime) const
{
	date_.getTime(pTime);
}

inline qs::wstring_ptr qm::MessageHolder::getSubject() const
{
	qs::Lock<Account> lock(*getAccount());
	return getAccount()->getIndex(messageIndexKey_.nKey_,
		messageIndexKey_.nLength_, NAME_SUBJECT);
}

inline unsigned int qm::MessageHolder::getSize() const
{
	return nSize_;
}

inline unsigned int qm::MessageHolder::getTextSize() const
{
	// TODO
	return nSize_;
}

inline qm::NormalFolder* qm::MessageHolder::getFolder() const
{
	return pFolder_;
}

inline qm::Account* qm::MessageHolder::getAccount() const
{
	return pFolder_->getAccount();
}

inline qm::MessageHolder* qm::MessageHolder::getMessageHolder()
{
	return this;
}

inline bool qm::MessageHolder::isFlag(Flag flag) const
{
	qs::Lock<Account> lock(*getAccount());
	return (nFlags_ & flag) != 0;
}

inline bool qm::MessageHolder::isSeen() const
{
	Account* pAccount = getAccount();
	qs::Lock<Account> lock(*pAccount);
	return pAccount->isSeen(this);
}

inline qs::wstring_ptr qm::MessageHolder::getMessageId() const
{
	qs::Lock<Account> lock(*getAccount());
	return getAccount()->getIndex(messageIndexKey_.nKey_,
		messageIndexKey_.nLength_, NAME_MESSAGEID);
}

inline qs::wstring_ptr qm::MessageHolder::getReference() const
{
	qs::Lock<Account> lock(*getAccount());
	return getAccount()->getIndex(messageIndexKey_.nKey_,
		messageIndexKey_.nLength_, NAME_REFERENCE);
}

inline qs::wstring_ptr qm::MessageHolder::getLabel() const
{
	qs::Lock<Account> lock(*getAccount());
	return getAccount()->getIndex(messageIndexKey_.nKey_,
		messageIndexKey_.nLength_, NAME_LABEL);
}

inline const qm::MessageHolder::MessageIndexKey& qm::MessageHolder::getMessageIndexKey() const
{
	return messageIndexKey_;
}

inline const qm::MessageHolder::MessageBoxKey& qm::MessageHolder::getMessageBoxKey() const
{
	return messageBoxKey_;
}

inline qm::MessageDate qm::MessageHolder::getDate() const
{
	return date_;
}


/****************************************************************************
 *
 * AbstractMessageHolder
 *
 */

inline unsigned int qm::AbstractMessageHolder::getId() const
{
	return nId_;
}

inline unsigned int qm::AbstractMessageHolder::getFlags() const
{
	// TODO
	return 0;
}

inline qs::wstring_ptr qm::AbstractMessageHolder::getFrom() const
{
	return getAddress(L"From");
}

inline qs::wstring_ptr qm::AbstractMessageHolder::getTo() const
{
	return getAddress(L"To");
}

inline qs::wstring_ptr qm::AbstractMessageHolder::getFromTo() const
{
	return getFrom();
}

inline unsigned int qm::AbstractMessageHolder::getSize() const
{
	return nSize_;
}

inline unsigned int qm::AbstractMessageHolder::getTextSize() const
{
	return nTextSize_;
}

inline qm::NormalFolder* qm::AbstractMessageHolder::getFolder() const
{
	return pFolder_;
}

inline qm::Account* qm::AbstractMessageHolder::getAccount() const
{
	return pFolder_->getAccount();
}

inline qm::MessageHolder* qm::AbstractMessageHolder::getMessageHolder()
{
	return 0;
}

inline qm::Message* qm::AbstractMessageHolder::getMessage() const
{
	return pMessage_;
}


/****************************************************************************
 *
 * MessagePtrLock
 *
 */

inline qm::MessagePtrLock::MessagePtrLock(const MessagePtr& ptr) :
	ptr_(ptr)
{
	pmh_ = ptr_.lock();
}

inline qm::MessagePtrLock::~MessagePtrLock()
{
	ptr_.unlock();
}

inline qm::MessagePtrLock::operator qm::MessageHolder*() const
{
	return pmh_;
}

inline qm::MessageHolder* qm::MessagePtrLock::operator->() const
{
	return pmh_;
}

#endif // __QMMESSAGEHOLDER_INL__
