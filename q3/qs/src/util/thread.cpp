/*
 * $Id$
 *
 * Copyright(C) 1998-2006 Satoshi Nakamura
 * All rights reserved.
 *
 */

#include <qs.h>
#include <qsassert.h>
#include <qsconv.h>
#include <qsstring.h>
#include <qsthread.h>

#ifndef _WIN32_WCE
#	include <process.h>
#endif
#include <tchar.h>

#include "thread.h"

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
#elif defined _WIN32_WCE && _WIN32_WCE < 300
#	define InterlockedCompareExchange(ptr, newval, oldval) \
	((PVOID)InterlockedTestExchange((LPLONG)ptr, (LONG)oldval, (LONG)newval))  
#endif

using namespace qs;


/****************************************************************************
 *
 * Runnable
 *
 */

qs::Runnable::~Runnable()
{
}


/****************************************************************************
 *
 * ThreadImpl
 *
 */

struct qs::ThreadImpl
{
	HANDLE hThread_;
	Runnable* pRunnable_;
};


/****************************************************************************
 *
 * Thread
 *
 */

namespace qs {
#ifdef _WIN32_WCE
DWORD WINAPI threadProc(LPVOID pParam);
#else
unsigned int __stdcall threadProc(void* pParam);
#endif
}

#ifdef _WIN32_WCE
DWORD WINAPI qs::threadProc(LPVOID pParam)
#else
unsigned int __stdcall qs::threadProc(void* pParam)
#endif
{
	assert(pParam);
	Runnable* pRunnable = static_cast<Runnable*>(pParam);
	pRunnable->run();
	return 0;
}

qs::Thread::Thread() :
	pImpl_(0)
{
	init(this);
}

qs::Thread::Thread(Runnable* pRunnable) :
	pImpl_(0)
{
	init(pRunnable);
}

qs::Thread::~Thread()
{
	if (pImpl_->hThread_)
		::CloseHandle(pImpl_->hThread_);
	delete pImpl_;
	pImpl_ = 0;
}

bool qs::Thread::start()
{
	assert(pImpl_);
	
#ifdef _WIN32_WCE
	DWORD dwId = 0;
	pImpl_->hThread_ = ::CreateThread(0, 0, threadProc,
		pImpl_->pRunnable_, 0, &dwId);
#else
	unsigned int nId = 0;
	pImpl_->hThread_ = reinterpret_cast<HANDLE>(_beginthreadex(
		0, 0, threadProc, pImpl_->pRunnable_, 0, &nId));
#endif
	return pImpl_->hThread_ != 0;
}

bool qs::Thread::join()
{
	return join(0);
}

bool qs::Thread::join(DWORD dwWait)
{
	assert(pImpl_);
	
	if (!pImpl_->hThread_)
		return false;
	
	if (dwWait == 0)
		dwWait = INFINITE;
	return ::WaitForSingleObject(pImpl_->hThread_, dwWait) != WAIT_FAILED;
}

HANDLE qs::Thread::getHandle() const
{
	assert(pImpl_);
	return pImpl_->hThread_;
}

void qs::Thread::run()
{
	if (pImpl_->pRunnable_)
		pImpl_->pRunnable_->run();
}

void qs::Thread::init(Runnable* pRunnable)
{
	assert(!pImpl_);
	
	pImpl_ = new ThreadImpl();
	pImpl_->hThread_ = 0;
	pImpl_->pRunnable_ = pRunnable;
}


/****************************************************************************
 *
 * ThreadLocal
 *
 */

qs::ThreadLocal::ThreadLocal() :
	dwTls_(0xffffffff)
{
	dwTls_ = ::TlsAlloc();
}

qs::ThreadLocal::~ThreadLocal()
{
	::TlsFree(dwTls_);
}

void* qs::ThreadLocal::get() const
{
	return ::TlsGetValue(dwTls_);
}

void qs::ThreadLocal::set(void* pValue)
{
	::TlsSetValue(dwTls_, pValue);
}


/****************************************************************************
 *
 * SpinLock
 *
 */

unsigned int qs::SpinLock::nMax__ = LOW_MAX;
unsigned int qs::SpinLock::nLast__ = 0;

void qs::SpinLock::lock() const
{
	if (::InterlockedExchange(UNVOLATILE(LONG*)(&nLock_), 1) != 0) {
		unsigned int nSpinMax = nMax__;
		unsigned int nSpinLast = nLast__;
		unsigned int nJunk = 17;
		unsigned int n = 0;
		for (n = 0; n < nSpinMax; ++n) {
			if (n < nSpinLast/2 || nLock_) {
				nJunk *= nJunk;
				nJunk *= nJunk;
				nJunk *= nJunk;
				nJunk *= nJunk;
			}
			else {
				if (::InterlockedExchange(UNVOLATILE(LONG*)(&nLock_), 1) == 0) {
					nLast__ = n;
					nMax__ = HIGH_MAX;
					return;
				}
			}
		}
		
		for (n = 0; ; ++n) {
			int nSec = n + 6;
			if (nSec > 27)
				nSec = 27;
			if (::InterlockedExchange(UNVOLATILE(LONG*)(&nLock_), 1) == 0)
				break;
			::Sleep(nSec);
		}
	}
}


/****************************************************************************
 *
 * ReadWriteLock
 *
 */

qs::ReadWriteLock::ReadWriteLock() :
	hMutexWrite_(0),
	hEventRead_(0),
	nReader_(0)
{
	hMutexWrite_ = ::CreateMutex(0, FALSE, 0);
	hEventRead_ = ::CreateEvent(0, TRUE, FALSE, 0);
}

qs::ReadWriteLock::~ReadWriteLock()
{
	::CloseHandle(hMutexWrite_);
	::CloseHandle(hEventRead_);
}

void qs::ReadWriteLock::readLock() const
{
	::WaitForSingleObject(hMutexWrite_, INFINITE);
	::InterlockedIncrement(UNVOLATILE(LONG*)(&nReader_));
	::ResetEvent(hEventRead_);
	::ReleaseMutex(hMutexWrite_);
}

void qs::ReadWriteLock::writeLock() const
{
	::WaitForSingleObject(hMutexWrite_, INFINITE);
	if (InterlockedCompareExchange(UNVOLATILE(LONG*)(&nReader_), 0, 0) != 0)
		::WaitForSingleObject(hEventRead_, INFINITE);
}

void qs::ReadWriteLock::unlock() const
{
	if (!::ReleaseMutex(hMutexWrite_) && ::GetLastError() == ERROR_NOT_OWNER) {
		if (::InterlockedDecrement(UNVOLATILE(LONG*)(&nReader_)) == 0)
			::SetEvent(hEventRead_);
	}
}


/****************************************************************************
 *
 * Event
 *
 */

qs::Event::Event() :
	hEvent_(0)
{
	hEvent_ = ::CreateEvent(0, FALSE, FALSE, 0);
}

qs::Event::Event(bool bManual, bool bInitial)
{
	hEvent_ = ::CreateEvent(0, bManual, bInitial, 0);
}

qs::Event::Event(bool bManual,
				 bool bInitial,
				 const WCHAR* pwszName)
{
	tstring_ptr tstrName(wcs2tcs(pwszName));
	hEvent_ = ::CreateEvent(0, bManual, bInitial, tstrName.get());
}

qs::Event::~Event()
{
	if (hEvent_)
		::CloseHandle(hEvent_);
}

bool qs::Event::set()
{
	return ::SetEvent(hEvent_) != 0;
}

bool qs::Event::reset()
{
	return ::ResetEvent(hEvent_) != 0;
}

bool qs::Event::pulse()
{
	return ::PulseEvent(hEvent_) != 0;
}

bool qs::Event::wait()
{
	return wait(INFINITE);
}

bool qs::Event::wait(unsigned int nMillisecond)
{
	return ::WaitForSingleObject(hEvent_, nMillisecond) != WAIT_FAILED;
}

HANDLE qs::Event::getHandle() const
{
	return hEvent_;
}


/****************************************************************************
 *
 * Mutex
 *
 */

qs::Mutex::Mutex() :
	hMutex_(0)
{
	init(false, 0, 0);
}

qs::Mutex::Mutex(bool bOwner) :
	hMutex_(0)
{
	init(bOwner, 0, 0);
}

qs::Mutex::Mutex(bool bOwner,
				 const WCHAR* pwszName) :
	hMutex_(0)
{
	init(bOwner, pwszName, 0);
}

qs::Mutex::Mutex(bool bOwner,
				 const WCHAR* pwszName,
				 bool* pbAlreadyExists) :
	hMutex_(0)
{
	init(bOwner, pwszName, pbAlreadyExists);
}

qs::Mutex::~Mutex()
{
	if (hMutex_)
		::CloseHandle(hMutex_);
}

bool qs::Mutex::acquire()
{
	assert(hMutex_);
	return ::WaitForSingleObject(hMutex_, INFINITE) != WAIT_FAILED;
}

bool qs::Mutex::release()
{
	assert(hMutex_);
	return ::ReleaseMutex(hMutex_) != 0;
}

void qs::Mutex::init(bool bOwner,
					 const WCHAR* pwszName,
					 bool* pbAlreadyExists)
{
	if (pwszName) {
		W2T(pwszName, ptszName);
		hMutex_ = ::CreateMutex(0, bOwner, ptszName);
		if (pbAlreadyExists)
			*pbAlreadyExists = ::GetLastError() == ERROR_ALREADY_EXISTS;
	}
	else {
		hMutex_ = ::CreateMutex(0, bOwner, 0);
	}
}


/****************************************************************************
 *
 * Synchronizer
 *
 */

qs::Synchronizer::Synchronizer() :
	pWindow_(0)
{
	std::auto_ptr<SynchronizerWindow> pWindow(new SynchronizerWindow());
	if (!pWindow->create(L"QsSynchronizerWindow", 0, WS_POPUP,
		0, 0, 0, 0, 0, 0, 0, 0, 0))
		return;
	pWindow_ = pWindow.release();
}

qs::Synchronizer::~Synchronizer()
{
	pWindow_->destroyWindow();
}

void qs::Synchronizer::syncExec(Runnable* pRunnable)
{
	pWindow_->sendMessage(SynchronizerWindow::WM_SYNCEXEC,
		0, reinterpret_cast<LPARAM>(pRunnable));
}


/****************************************************************************
 *
 * SynchronizerWindow
 *
 */

qs::SynchronizerWindow::SynchronizerWindow() :
	WindowBase(true)
{
	setWindowHandler(this, false);
}

qs::SynchronizerWindow::~SynchronizerWindow()
{
}

LRESULT qs::SynchronizerWindow::windowProc(UINT uMsg,
										   WPARAM wParam,
										   LPARAM lParam)
{
	BEGIN_MESSAGE_HANDLER()
		HANDLE_MESSAGE(WM_SYNCEXEC, onSyncExec)
	END_MESSAGE_HANDLER()
	return DefaultWindowHandler::windowProc(uMsg, wParam, lParam);
}

LRESULT qs::SynchronizerWindow::onSyncExec(WPARAM wParam,
										   LPARAM lParam)
{
	Runnable* pRunnable = reinterpret_cast<Runnable*>(lParam);
	QTRY {
		pRunnable->run();
	}
	QCATCH_ALL() {
		;
	}
	return 0;
}
