/*
 * $Id$
 *
 * Copyright(C) 1998-2003 Satoshi Nakamura
 * All rights reserved.
 *
 */

#include <qs.h>
#include <qsassert.h>
#include <qsconv.h>
#include <qserror.h>
#include <qsnew.h>
#include <qsstring.h>
#include <qsthread.h>

#ifndef _WIN32_WCE
#	include <process.h>
#endif
#include <tchar.h>

#include "thread.h"

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
	return pRunnable->run();
}

qs::Thread::Thread(QSTATUS* pstatus) :
	pImpl_(0)
{
	assert(pstatus);
	*pstatus = init(this);
}

qs::Thread::Thread(Runnable* pRunnable, QSTATUS* pstatus) :
	pImpl_(0)
{
	assert(pstatus);
	*pstatus = init(pRunnable);
}

qs::Thread::~Thread()
{
	if (pImpl_->hThread_)
		::CloseHandle(pImpl_->hThread_);
	delete pImpl_;
	pImpl_ = 0;
}

QSTATUS qs::Thread::start()
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
	if (!pImpl_->hThread_)
		return QSTATUS_FAIL;
	return QSTATUS_SUCCESS;
}

QSTATUS qs::Thread::join()
{
	return join(0);
}

QSTATUS qs::Thread::join(DWORD dwWait)
{
	assert(pImpl_);
	
	if (!pImpl_->hThread_)
		return QSTATUS_FAIL;
	
	if (dwWait == 0)
		dwWait = INFINITE;
	if (::WaitForSingleObject(pImpl_->hThread_, dwWait) == WAIT_FAILED)
		return QSTATUS_FAIL;
	return QSTATUS_SUCCESS;
}

HANDLE qs::Thread::getHandle() const
{
	assert(pImpl_);
	return pImpl_->hThread_;
}

unsigned int qs::Thread::run()
{
	if (pImpl_->pRunnable_)
		return pImpl_->pRunnable_->run();
	else
		return 0;
}

QSTATUS qs::Thread::init(Runnable* pRunnable)
{
	assert(!pImpl_);
	
	DECLARE_QSTATUS();
	
	status = newObject(&pImpl_);
	CHECK_QSTATUS();
	assert(pImpl_);
	
	pImpl_->hThread_ = 0;
	pImpl_->pRunnable_ = pRunnable;
	
	return QSTATUS_SUCCESS;
}


/****************************************************************************
 *
 * ThreadLocal
 *
 */

qs::ThreadLocal::ThreadLocal(QSTATUS* pstatus) :
	dwTls_(0xffffffff)
{
	assert(pstatus);
	
	dwTls_ = ::TlsAlloc();
	*pstatus = dwTls_ != 0xffffffff ? QSTATUS_SUCCESS : QSTATUS_FAIL;
}

qs::ThreadLocal::~ThreadLocal()
{
	::TlsFree(dwTls_);
}

QSTATUS qs::ThreadLocal::get(void** ppValue) const
{
	assert(ppValue);
	
	*ppValue = ::TlsGetValue(dwTls_);
	if (!*ppValue && ::GetLastError() != NO_ERROR)
		return QSTATUS_FAIL;
	return QSTATUS_SUCCESS;
}

QSTATUS qs::ThreadLocal::set(void* pValue)
{
	if (!::TlsSetValue(dwTls_, pValue))
		return QSTATUS_FAIL;
	return QSTATUS_SUCCESS;
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
	if (::InterlockedExchange(reinterpret_cast<long*>(const_cast<unsigned long*>(&nLock_)), 1) != 0) {
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
				if (::InterlockedExchange(reinterpret_cast<long*>(const_cast<unsigned long*>(&nLock_)), 1) == 0) {
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
			if (::InterlockedExchange(reinterpret_cast<long*>(const_cast<unsigned long*>(&nLock_)), 1) == 0)
				break;
			::Sleep(nSec);
		}
	}
}


/****************************************************************************
 *
 * Event
 *
 */

qs::Event::Event(QSTATUS* pstatus) :
	hEvent_(0)
{
	assert(pstatus);
	
	hEvent_ = ::CreateEvent(0, FALSE, FALSE, 0);
	*pstatus = hEvent_ ? QSTATUS_SUCCESS : QSTATUS_FAIL;
}

qs::Event::Event(bool bManual, bool bInitial, QSTATUS* pstatus)
{
	assert(pstatus);
	
	hEvent_ = ::CreateEvent(0, bManual, bInitial, 0);
	*pstatus = hEvent_ ? QSTATUS_SUCCESS : QSTATUS_FAIL;
}

qs::Event::Event(bool bManual, bool bInitial, const WCHAR* pwszName, QSTATUS* pstatus)
{
	assert(pstatus);
	
	string_ptr<TSTRING> tstrName(wcs2tcs(pwszName));
	if (!tstrName.get()) {
		*pstatus = QSTATUS_OUTOFMEMORY;
		return;
	}
	
	hEvent_ = ::CreateEvent(0, bManual, bInitial, tstrName.get());
	*pstatus = hEvent_ ? QSTATUS_SUCCESS : QSTATUS_FAIL;
}

qs::Event::~Event()
{
	if (hEvent_)
		::CloseHandle(hEvent_);
}

QSTATUS qs::Event::set()
{
	return ::SetEvent(hEvent_) ? QSTATUS_SUCCESS : QSTATUS_FAIL;
}

QSTATUS qs::Event::reset()
{
	return ::ResetEvent(hEvent_) ? QSTATUS_SUCCESS : QSTATUS_FAIL;
}

QSTATUS qs::Event::pulse()
{
	return ::PulseEvent(hEvent_) ? QSTATUS_SUCCESS : QSTATUS_FAIL;
}

QSTATUS qs::Event::wait()
{
	return wait(INFINITE);
}

QSTATUS qs::Event::wait(unsigned int nMillisecond)
{
	return ::WaitForSingleObject(hEvent_, nMillisecond) != WAIT_FAILED ?
		QSTATUS_SUCCESS : QSTATUS_FAIL;
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

qs::Mutex::Mutex(QSTATUS* pstatus) :
	hMutex_(0)
{
	*pstatus = init(false, 0);
}

qs::Mutex::Mutex(bool bOwner, QSTATUS* pstatus) :
	hMutex_(0)
{
	*pstatus = init(bOwner, 0);
}

qs::Mutex::Mutex(bool bOwner, const WCHAR* pwszName, QSTATUS* pstatus) :
	hMutex_(0)
{
	*pstatus = init(bOwner, pwszName);
}

qs::Mutex::~Mutex()
{
	if (hMutex_)
		::CloseHandle(hMutex_);
}

QSTATUS qs::Mutex::acquire()
{
	assert(hMutex_);
	return ::WaitForSingleObject(hMutex_, INFINITE) != WAIT_FAILED ?
		QSTATUS_SUCCESS : QSTATUS_FAIL;
}

QSTATUS qs::Mutex::release()
{
	assert(hMutex_);
	return ::ReleaseMutex(hMutex_) ? QSTATUS_SUCCESS : QSTATUS_FAIL;
}

QSTATUS qs::Mutex::init(bool bOwner, const WCHAR* pwszName)
{
	DECLARE_QSTATUS();
	
	if (pwszName) {
		W2T(pwszName, ptszName);
		hMutex_ = ::CreateMutex(0, bOwner, ptszName);
	}
	else {
		hMutex_ = ::CreateMutex(0, bOwner, 0);
	}
	
	return hMutex_ ? QSTATUS_SUCCESS : QSTATUS_FAIL;
}


/****************************************************************************
 *
 * Synchronizer
 *
 */

qs::Synchronizer::Synchronizer(QSTATUS* pstatus) :
	pWindow_(0)
{
	DECLARE_QSTATUS();
	
	std::auto_ptr<SynchronizerWindow> pWindow;
	status = newQsObject(&pWindow);
	CHECK_QSTATUS_SET(pstatus);
	status = pWindow->create(L"QsSynchronizerWindow", 0, WS_POPUP,
		0, 0, 0, 0, 0, 0, 0, 0, 0);
	pWindow_ = pWindow.release();
}

qs::Synchronizer::~Synchronizer()
{
	pWindow_->destroyWindow();
}

QSTATUS qs::Synchronizer::syncExec(Runnable* pRunnable)
{
	DECLARE_QSTATUS();
	
	status = pWindow_->sendMessage(SynchronizerWindow::WM_SYNCEXEC,
		0, reinterpret_cast<LPARAM>(pRunnable));
	CHECK_QSTATUS();
	
	return QSTATUS_SUCCESS;
}


/****************************************************************************
 *
 * SynchronizerWindow
 *
 */

qs::SynchronizerWindow::SynchronizerWindow(QSTATUS* pstatus) :
	WindowBase(true, pstatus),
	DefaultWindowHandler(pstatus)
{
	setWindowHandler(this, false);
}

qs::SynchronizerWindow::~SynchronizerWindow()
{
}

LRESULT qs::SynchronizerWindow::windowProc(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	BEGIN_MESSAGE_HANDLER()
		HANDLE_MESSAGE(WM_SYNCEXEC, onSyncExec)
	END_MESSAGE_HANDLER()
	return DefaultWindowHandler::windowProc(uMsg, wParam, lParam);
}

LRESULT qs::SynchronizerWindow::onSyncExec(WPARAM wParam, LPARAM lParam)
{
	Runnable* pRunnable = reinterpret_cast<Runnable*>(lParam);
	pRunnable->run();
	return 0;
}
