/*
 * $Id$
 *
 * Copyright(C) 1998-2003 Satoshi Nakamura
 * All rights reserved.
 *
 */

#ifndef __QSTHREAD_H__
#define __QSTHREAD_H__

#include <qs.h>
#include <windows.h>

namespace qs {

class Runnable;
class Thread;
class ThreadLocal;
class CriticalSection;
class SpinLock;
class NoLock;
template<class Object> class Lock;
class Event;
class Synchronizer;

class SynchronizerWindow;


/****************************************************************************
 *
 * Runnable
 *
 */

class QSEXPORTCLASS Runnable
{
public:
	virtual ~Runnable();
	virtual unsigned int run() = 0;
};


/****************************************************************************
 *
 * Thread
 *
 */

class QSEXPORTCLASS Thread : public Runnable
{
public:
	explicit Thread(QSTATUS* pstatus);
	Thread(Runnable* pRunnable, QSTATUS* pstatus);
	virtual ~Thread();

public:
	QSTATUS start();
	QSTATUS join();
	QSTATUS join(DWORD dwWait);
	HANDLE getHandle() const;

public:
	virtual unsigned int run();

private:
	QSTATUS init(Runnable* pRunnable);

private:
	Thread(const Thread&);
	Thread& operator=(const Thread&);

private:
	struct ThreadImpl* pImpl_;
};


/****************************************************************************
 *
 * ThreadLocal
 *
 */

class ThreadLocal
{
public:
	explicit ThreadLocal(QSTATUS* pstatus);
	~ThreadLocal();

public:
	QSTATUS get(void** ppValue) const;
	QSTATUS set(void* pValue);

private:
	ThreadLocal(const ThreadLocal&);
	ThreadLocal operator=(const ThreadLocal&);

private:
	DWORD dwTls_;
};


/****************************************************************************
 *
 * CriticalSection
 *
 */


class QSEXPORTCLASS CriticalSection
{
public:
	CriticalSection();
	~CriticalSection();

public:
	void lock() const;
	void unlock() const;

private:
	CriticalSection(const CriticalSection&);
	CriticalSection& operator=(const CriticalSection&);

private:
	mutable CRITICAL_SECTION cs_;
};


/****************************************************************************
 *
 * SpinLock
 *
 */

class QSEXPORTCLASS SpinLock
{
private:
	enum {
		LOW_MAX		= 30,
		HIGH_MAX	= 1000
	};

public:
	SpinLock();
	~SpinLock();

public:
	void lock() const;
	void unlock() const;

private:
	SpinLock(const SpinLock&);
	SpinLock& operator=(const SpinLock&);

private:
	volatile mutable unsigned long nLock_;

private:
	static unsigned int nMax__;
	static unsigned int nLast__;
};


/****************************************************************************
 *
 * NoLock
 *
 */

class QSEXPORTCLASS NoLock
{
public:
	NoLock();
	~NoLock();

public:
	void lock() const;
	void unlock() const;

private:
	NoLock(const NoLock&);
	NoLock& operator=(const NoLock&);
};


/****************************************************************************
 *
 * Lock
 *
 */

template<class Object>
class QSEXPORTCLASS Lock
{
public:
	explicit Lock(const Object& o);
	~Lock();

private:
	Lock(const Lock&);
	Lock& operator=(const Lock&);

private:
	const Object& o_;
};


/****************************************************************************
 *
 * Event
 *
 */

class QSEXPORTCLASS Event
{
public:
	Event(QSTATUS* pstatus);
	Event(bool bManual, bool bInitial, QSTATUS* pstatus);
	Event(bool bManual, bool bInitial, const WCHAR* pwszName, QSTATUS* pstatus);
	~Event();

public:
	QSTATUS set();
	QSTATUS reset();
	QSTATUS pulse();
	QSTATUS wait();
	QSTATUS wait(unsigned int nMillisecond);
	HANDLE getHandle() const;

private:
	Event(const Event&);
	Event& operator=(const Event&);

private:
	HANDLE hEvent_;
};


/****************************************************************************
 *
 * Mutex
 *
 */

class QSEXPORTCLASS Mutex
{
public:
	Mutex(QSTATUS* pstatus);
	Mutex(bool bOwner, QSTATUS* pstatus);
	Mutex(bool bOwner, const WCHAR* pwszName, QSTATUS* pstatus);
	~Mutex();

public:
	QSTATUS acquire();
	QSTATUS release();

private:
	QSTATUS init(bool bOwner, const WCHAR* pwszName);

private:
	Mutex(const Mutex&);
	Mutex& operator=(const Mutex&);

private:
	HANDLE hMutex_;
};


/****************************************************************************
 *
 * Synchronizer
 *
 */

class QSEXPORTCLASS Synchronizer
{
public:
	explicit Synchronizer(QSTATUS* pstatus);
	~Synchronizer();

public:
	QSTATUS syncExec(Runnable* pRunnable);

private:
	Synchronizer(const Synchronizer&);
	Synchronizer& operator=(const Synchronizer&);

private:
	SynchronizerWindow* pWindow_;
};

}

#include <qsthread.inl>

#endif // __QSTHREAD_H__
