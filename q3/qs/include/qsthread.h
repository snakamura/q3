/*
 * $Id$
 *
 * Copyright(C) 1998-2004 Satoshi Nakamura
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

public:
	virtual void run() = 0;
};


/****************************************************************************
 *
 * Thread
 *
 */

class QSEXPORTCLASS Thread : public Runnable
{
public:
	Thread();
	explicit Thread(Runnable* pRunnable);
	virtual ~Thread();

public:
	bool start();
	bool join();
	bool join(DWORD dwWait);
	HANDLE getHandle() const;

public:
	virtual void run();

private:
	void init(Runnable* pRunnable);

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
	ThreadLocal();
	~ThreadLocal();

public:
	void* get() const;
	void set(void* pValue);

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
class Lock
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
	Event();
	Event(bool bManual,
		  bool bInitial);
	Event(bool bManual,
		  bool bInitial,
		  const WCHAR* pwszName);
	~Event();

public:
	bool set();
	bool reset();
	bool pulse();
	bool wait();
	bool wait(unsigned int nMillisecond);
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
	Mutex();
	Mutex(bool bOwner);
	Mutex(bool bOwner,
		  const WCHAR* pwszName);
	~Mutex();

public:
	bool acquire();
	bool release();

private:
	void init(bool bOwner,
			  const WCHAR* pwszName);

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
	Synchronizer();
	~Synchronizer();

public:
	void syncExec(Runnable* pRunnable);

private:
	Synchronizer(const Synchronizer&);
	Synchronizer& operator=(const Synchronizer&);

private:
	SynchronizerWindow* pWindow_;
};

}

#include <qsthread.inl>

#endif // __QSTHREAD_H__
