/*
 * $Id$
 *
 * Copyright(C) 1998-2008 Satoshi Nakamura
 *
 */

#ifndef __QSTHREAD_H__
#define __QSTHREAD_H__

#include <qs.h>
#include <windows.h>

#ifdef _WIN32_WCE
#	define UNVOLATILE(type) const_cast<type>
#else
#	define UNVOLATILE(type)
#endif

#if !defined _WIN32_WCE && (!defined _WIN32_WINNT || WINVER <= 0x400) && (defined x86 || defined _X86_)
#pragma warning(push)
#pragma warning(disable:4035)
inline LONG WINAPI InterlockedCompareExchange_(LONG volatile* Destination,
											   LONG Exchange,
											   LONG Comperand) {
	__asm {
		mov eax, Comperand
		mov ecx, Destination
		mov edx, Exchange
		cmpxchg [ecx], edx
	}
}
#pragma warning(pop)
#	define InterlockedCompareExchange(ptr, newval, oldval) \
		InterlockedCompareExchange_(ptr, newval, oldval)
#	undef InterlockedCompareExchangePointer
#	define InterlockedCompareExchangePointer(ptr, newval, oldval) \
		InterlockedCompareExchange_((LPLONG)ptr, (LONG)newval, (LONG)oldval)
#elif defined _WIN32_WCE && _WIN32_WCE < 0x300
#	define InterlockedCompareExchange(ptr, newval, oldval) \
	((PVOID)InterlockedTestExchange((LPLONG)ptr, (LONG)oldval, (LONG)newval))  
#endif

namespace qs {

class Runnable;
class Thread;
template<class T> class ThreadLocal;
class CriticalSection;
class SpinLock;
class NoLock;
class ReadWriteLock;
class ReadWriteReadLock;
class ReadWriteWriteLock;
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

template<class T>
class ThreadLocal
{
public:
	ThreadLocal();
	~ThreadLocal();

public:
	T get() const;
	void set(const T& value);

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
	volatile mutable LONG nLock_;

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
 * ReadWriteLock
 *
 */

class QSEXPORTCLASS ReadWriteLock
{
public:
	ReadWriteLock();
	~ReadWriteLock();

public:
	void readLock() const;
	void writeLock() const;
	void unlock() const;

private:
	ReadWriteLock(const ReadWriteLock&);
	ReadWriteLock& operator=(const ReadWriteLock&);

private:
	HANDLE hMutexWrite_;
	HANDLE hEventRead_;
	volatile mutable LONG nReader_;
};


/****************************************************************************
 *
 * ReadWriteReadLock
 *
 */

class QSEXPORTCLASS ReadWriteReadLock
{
public:
	explicit ReadWriteReadLock(const ReadWriteLock& lock);
	~ReadWriteReadLock();

public:
	void lock() const;
	void unlock() const;

private:
	ReadWriteReadLock(const ReadWriteReadLock&);
	ReadWriteReadLock& operator=(const ReadWriteReadLock&);

private:
	const ReadWriteLock& lock_;
};


/****************************************************************************
 *
 * ReadWriteWriteLock
 *
 */

class QSEXPORTCLASS ReadWriteWriteLock
{
public:
	explicit ReadWriteWriteLock(const ReadWriteLock& lock);
	~ReadWriteWriteLock();

public:
	void lock() const;
	void unlock() const;

private:
	ReadWriteWriteLock(const ReadWriteWriteLock&);
	ReadWriteWriteLock& operator=(const ReadWriteWriteLock&);

private:
	const ReadWriteLock& lock_;
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
	Mutex(bool bOwner,
		  const WCHAR* pwszName,
		  bool* pbAlreadyExists);
	~Mutex();

public:
	bool acquire();
	bool release();

private:
	void init(bool bOwner,
			  const WCHAR* pwszName,
			  bool* pbAlreadyExists);

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
	void asyncExec(std::auto_ptr<Runnable> pRunnable);

private:
	Synchronizer(const Synchronizer&);
	Synchronizer& operator=(const Synchronizer&);

private:
	SynchronizerWindow* pWindow_;
};

}

#include <qsthread.inl>

#endif // __QSTHREAD_H__
