/*
 * $Id$
 *
 * Copyright(C) 1998-2005 Satoshi Nakamura
 * All rights reserved.
 *
 */

#include <qsassert.h>
#include <qsstl.h>

#include <algorithm>

#include <openssl/crypto.h>

#include "lock.h"

using namespace qs;
using namespace qscrypto;


/****************************************************************************
 *
 * Lock
 *
 */

Locks qscrypto::Locks::locks__;

qscrypto::Locks::Locks()
{
	unsigned int nSize = CRYPTO_num_locks();
	listCriticalSection_.resize(nSize);
	for (unsigned int n = 0; n < nSize; ++n)
		listCriticalSection_[n] = new CriticalSection();
}

qscrypto::Locks::~Locks()
{
	std::for_each(listCriticalSection_.begin(),
		listCriticalSection_.end(), qs::deleter<CriticalSection>());
}

CriticalSection& qscrypto::Locks::get(unsigned int n)
{
	assert(n < listCriticalSection_.size());
	return *listCriticalSection_[n];
}

Locks& qscrypto::Locks::getInstance()
{
	return locks__;
}

extern "C" void qscrypto::lockCallback(int nMode,
									   int nType,
									   const char* pszFile,
									   int nLine)
{
	Locks& locks = Locks::getInstance();
	CriticalSection& cs = locks.get(nType);
	
	if (nMode & CRYPTO_LOCK)
		cs.lock();
	else
		cs.unlock();
}
