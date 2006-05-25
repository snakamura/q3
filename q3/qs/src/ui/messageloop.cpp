/*
 * $Id$
 *
 * Copyright(C) 1998-2006 Satoshi Nakamura
 * All rights reserved.
 *
 */

#include <qsdialog.h>
#include <qsinit.h>
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
	void processIdle();

public:
	static bool isIdleMessage(const MSG& msg);
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
		virtual bool init();
		virtual void term();
		virtual bool initThread();
		virtual void termThread();
	} init__;
};

ThreadLocal* qs::MessageLoopImpl::pMessageLoop__;
MessageLoopImpl::InitializerImpl qs::MessageLoopImpl::init__;

void qs::MessageLoopImpl::processIdle()
{
	FrameList::const_iterator it = listFrame_.begin();
	while (it != listFrame_.end()) {
		(*it)->processIdle();
		++it;
	}
}

bool qs::MessageLoopImpl::isIdleMessage(const MSG& msg)
{
	if (msg.message == WM_PAINT ||
		msg.message == 0x0118)
		return false;
	else if (msg.message == WM_KEYDOWN && LOWORD(msg.lParam) > 0)
		return false;
	else
		return true;
}

MessageLoop* qs::MessageLoopImpl::getMessageLoop()
{
	return static_cast<MessageLoop*>(pMessageLoop__->get());
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

bool qs::MessageLoopImpl::InitializerImpl::init()
{
	pMessageLoop__ = new ThreadLocal();
	return true;
}

void qs::MessageLoopImpl::InitializerImpl::term()
{
	delete pMessageLoop__;
}

bool qs::MessageLoopImpl::InitializerImpl::initThread()
{
	pMessageLoop__->set(new MessageLoop());
	return true;
}

void qs::MessageLoopImpl::InitializerImpl::termThread()
{
	delete static_cast<MessageLoop*>(pMessageLoop__->get());
}


/****************************************************************************
 *
 * MessageLoop
 *
 */

qs::MessageLoop::MessageLoop() :
	pImpl_(0)
{
	pImpl_ = new MessageLoopImpl();
}

qs::MessageLoop::~MessageLoop()
{
	delete pImpl_;
}

void qs::MessageLoop::run()
{
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
				return;
			
			bool bProcess = true;
#if !defined _WIN32_WCE || _WIN32_WCE >= 211
			if (msg.message == WM_MOUSEWHEEL) {
				msg.hwnd = ::WindowFromPoint(msg.pt);
				bProcess = msg.hwnd &&
					::GetWindowThreadProcessId(msg.hwnd, 0) == dwThreadId;
			}
#endif
			if (bProcess) {
				if (DialogBase::processDialogMessage(&msg))
					continue;
				if (PropertySheetBase::processDialogMessage(&msg))
					continue;
				if (WindowBase::translateAccelerator(msg))
					continue;
				
				::TranslateMessage(&msg);
				::DispatchMessage(&msg);
			}
			
			if (!bIdle && MessageLoopImpl::isIdleMessage(msg))
				bIdle = true;
		} while (::PeekMessage(&msg, 0, 0, 0, PM_NOREMOVE));
	}
}

void qs::MessageLoop::addFrame(FrameWindow* pFrameWindow)
{
	pImpl_->listFrame_.push_back(pFrameWindow);
}

void qs::MessageLoop::removeFrame(FrameWindow* pFrameWindow)
{
	MessageLoopImpl::FrameList::iterator it = std::remove(
		pImpl_->listFrame_.begin(), pImpl_->listFrame_.end(), pFrameWindow);
	pImpl_->listFrame_.erase(it, pImpl_->listFrame_.end());
}

MessageLoop& qs::MessageLoop::getMessageLoop()
{
	return *MessageLoopImpl::getMessageLoop();
}
