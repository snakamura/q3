/*
 * $Id$
 *
 * Copyright(C) 1998-2003 Satoshi Nakamura
 * All rights reserved.
 *
 */

#include <qsdialog.h>
#include <qsinit.h>
#include <qsnew.h>
#include <qsthread.h>
#include <qswindow.h>

using namespace qs;


/****************************************************************************
 *
 * MessageLoopImpl
 *
 */

class qs::MessageLoopImpl
{
public:
	typedef std::vector<FrameWindow*> FrameList;

public:
	QSTATUS processIdle();

public:
	static MessageLoop* getMessageLoop();

public:
	FrameList listFrame_;

public:
	static ThreadLocal* pMessageLoop__;
	static class InitializerImpl : public Initializer
	{
	public:
		InitializerImpl();
		virtual ~InitializerImpl();
	
	public:
		virtual QSTATUS init();
		virtual QSTATUS term();
		virtual QSTATUS initThread();
		virtual QSTATUS termThread();
	} init__;
};

ThreadLocal* qs::MessageLoopImpl::pMessageLoop__;
MessageLoopImpl::InitializerImpl qs::MessageLoopImpl::init__;

QSTATUS qs::MessageLoopImpl::processIdle()
{
	DECLARE_QSTATUS();
	
	FrameList::const_iterator it = listFrame_.begin();
	while (it != listFrame_.end()) {
		status = (*it)->processIdle();
		CHECK_QSTATUS();
		++it;
	}
	
	return QSTATUS_SUCCESS;
}

MessageLoop* qs::MessageLoopImpl::getMessageLoop()
{
	void* pValue = 0;
	pMessageLoop__->get(&pValue);
	return static_cast<MessageLoop*>(pValue);
}


/****************************************************************************
 *
 * MessageLoopImpl::InitializerImpl
 *
 */

qs::MessageLoopImpl::InitializerImpl::InitializerImpl()
{
}

qs::MessageLoopImpl::InitializerImpl::~InitializerImpl()
{
}

QSTATUS qs::MessageLoopImpl::InitializerImpl::init()
{
	return newQsObject(&pMessageLoop__);
}

QSTATUS qs::MessageLoopImpl::InitializerImpl::term()
{
	delete pMessageLoop__;
	return QSTATUS_SUCCESS;
}

QSTATUS qs::MessageLoopImpl::InitializerImpl::initThread()
{
	DECLARE_QSTATUS();
	
	std::auto_ptr<MessageLoop> pMessageLoop;
	status = newQsObject(&pMessageLoop);
	CHECK_QSTATUS();
	status = pMessageLoop__->set(pMessageLoop.get());
	CHECK_QSTATUS();
	pMessageLoop.release();
	
	return QSTATUS_SUCCESS;
}

QSTATUS qs::MessageLoopImpl::InitializerImpl::termThread()
{
	void* p = 0;
	pMessageLoop__->get(&p);
	delete static_cast<MessageLoop*>(p);
	return QSTATUS_SUCCESS;
}


/****************************************************************************
 *
 * MessageLoop
 *
 */

qs::MessageLoop::MessageLoop(QSTATUS* pstatus) :
	pImpl_(0)
{
	DECLARE_QSTATUS();
	
	status = newObject(&pImpl_);
	CHECK_QSTATUS_SET(pstatus);
}

qs::MessageLoop::~MessageLoop()
{
	delete pImpl_;
}

QSTATUS qs::MessageLoop::run()
{
	DECLARE_QSTATUS();
	
	DWORD dwThreadId = ::GetCurrentThreadId();
	
	MSG msg;
	bool bIdle = true;
	while (true) {
		if (bIdle && !::PeekMessage(&msg, 0, 0, 0, PM_NOREMOVE)) {
			pImpl_->processIdle();
			bIdle = false;
		}
		
		do {
			if (!::GetMessage(&msg, 0, 0, 0))
				return QSTATUS_SUCCESS;
			
			switch (msg.message) {
			case WM_KEYDOWN:
			case WM_SYSKEYDOWN:
			case WM_LBUTTONDOWN:
			case WM_RBUTTONDOWN:
			case WM_MBUTTONDOWN:
			case WM_VSCROLL:
			case WM_HSCROLL:
			case WM_COMMAND:
				break;
#if !defined _WIN32_WCE || _WIN32_WCE >= 211
			case WM_MOUSEWHEEL:
				msg.hwnd = ::WindowFromPoint(msg.pt);
				assert(msg.hwnd);
				if (::GetWindowThreadProcessId(msg.hwnd, 0) != dwThreadId)
					continue;
				break;
#endif
			}
			
			bool bProcessed = false;
			status = DialogBase::processDialogMessage(&msg, &bProcessed);
			CHECK_QSTATUS();
			if (bProcessed)
				continue;
			status = PropertySheetBase::processDialogMessage(&msg, &bProcessed);
			CHECK_QSTATUS();
			if (bProcessed)
				continue;
			status = WindowBase::translateAccelerator(msg, &bProcessed);
			CHECK_QSTATUS();
			if (bProcessed)
				continue;
			
			::TranslateMessage(&msg);
			::DispatchMessage(&msg);
			
			if (msg.message != WM_PAINT && msg.message != 0x0118)
				bIdle = true;
		} while (::PeekMessage(&msg, 0, 0, 0, PM_NOREMOVE));
	}
	
	return QSTATUS_SUCCESS;
}

QSTATUS qs::MessageLoop::addFrame(FrameWindow* pFrameWindow)
{
	return STLWrapper<MessageLoopImpl::FrameList>(
		pImpl_->listFrame_).push_back(pFrameWindow);
}

QSTATUS qs::MessageLoop::removeFrame(FrameWindow* pFrameWindow)
{
	MessageLoopImpl::FrameList::iterator it = std::remove(
		pImpl_->listFrame_.begin(), pImpl_->listFrame_.end(), pFrameWindow);
	pImpl_->listFrame_.erase(it, pImpl_->listFrame_.end());
	return QSTATUS_SUCCESS;
}

MessageLoop& qs::MessageLoop::getMessageLoop()
{
	return *MessageLoopImpl::getMessageLoop();
}
