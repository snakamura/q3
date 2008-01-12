/*
 * $Id$
 *
 * Copyright(C) 1998-2008 Satoshi Nakamura
 *
 */

#ifndef __QMACCOUNT_INL__
#define __QMACCOUNT_INL__


/****************************************************************************
 *
 * AccountLock
 *
 */

inline qm::AccountLock::AccountLock() :
	pAccount_(0)
{
}

inline qm::AccountLock::AccountLock(Account* pAccount) :
	pAccount_(pAccount)
{
	if (pAccount_)
		pAccount_->lock();
}

inline qm::AccountLock::AccountLock(const AccountLock& lock) :
	pAccount_(lock.pAccount_)
{
	if (pAccount_)
		pAccount_->lock();
}

inline qm::AccountLock::~AccountLock()
{
	if (pAccount_)
		pAccount_->unlock();
}

inline qm::Account* qm::AccountLock::get() const
{
	return pAccount_;
}

inline void qm::AccountLock::set(Account* pAccount)
{
	assert(!pAccount_);
	assert(pAccount);
	pAccount_ = pAccount;
	pAccount_->lock();
}

#endif // __QMACCOUNT_INL__
