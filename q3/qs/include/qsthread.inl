/*
 * $Id$
 *
 * Copyright(C) 1998-2003 Satoshi Nakamura
 * All rights reserved.
 *
 */

#ifndef __QSTHREAD_INL__
#define __QSTHREAD_INL__


/****************************************************************************
 *
 * CriticalSection
 *
 */


inline qs::CriticalSection::CriticalSection()
{
	::InitializeCriticalSection(&cs_);
}

inline qs::CriticalSection::~CriticalSection()
{
	::DeleteCriticalSection(&cs_);
}

inline void qs::CriticalSection::lock() const
{
	::EnterCriticalSection(&cs_);
}

inline void qs::CriticalSection::unlock() const
{
	::LeaveCriticalSection(&cs_);
}


/****************************************************************************
 *
 * SpinLock
 *
 */

inline qs::SpinLock::SpinLock() :
	nLock_(0)
{
}

inline qs::SpinLock::~SpinLock()
{
}

inline void qs::SpinLock::unlock() const
{
	nLock_ = 0;
}


/****************************************************************************
 *
 * NoLock
 *
 */

inline qs::NoLock::NoLock()
{
}

inline qs::NoLock::~NoLock()
{
}

inline void qs::NoLock::lock() const
{
}

inline void qs::NoLock::unlock() const
{
}


/****************************************************************************
 *
 * Lock
 *
 */

template<class Object>
qs::Lock<Object>::Lock(const Object& o) :
	o_(o)
{
	o_.lock();
}

template<class Object>
qs::Lock<Object>::~Lock()
{
	o_.unlock();
}

#endif // __QSTHREAD_INL__
