/*
 * $Id$
 *
 * Copyright(C) 1998-2005 Satoshi Nakamura
 * All rights reserved.
 *
 */

#ifndef __LOCK_H__
#define __LOCK_H__

#include <qsthread.h>

#include <vector>


namespace qscrypto {

/****************************************************************************
 *
 * Locks
 *
 */

class Locks
{
private:
	Locks();

public:
	~Locks();

public:
	qs::CriticalSection& get(unsigned int n);

public:
	static Locks& getInstance();

private:
	Locks(const Locks&);
	Locks& operator=(const Locks&);

private:
	typedef std::vector<qs::CriticalSection*> CriticalSectionList;

private:
	CriticalSectionList listCriticalSection_;

private:
	static Locks locks__;
};

extern "C" void lockCallback(int nMode,
							 int nType,
							 const char* pszFile,
							 int nLine);

}

#endif __LOCK_H__
