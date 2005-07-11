/*
 * $Id$
 *
 * Copyright(C) 1998-2005 Satoshi Nakamura
 * All rights reserved.
 *
 */

#pragma warning(disable:4786)

#include <qstimer.h>
#include <qswindow.h>

using namespace qs;


/****************************************************************************
 *
 * TimerImpl
 *
 */

class qs::TimerImpl : public DefaultWindowHandler
{
public:
	virtual LRESULT windowProc(UINT uMsg,
							   WPARAM wParam,
							   LPARAM lParam);

protected:
	LRESULT onTimer(UINT nId);

public:
	typedef std::vector<std::pair<unsigned int, TimerHandler*> > HandlerMap;

public:
	WindowBase* pWindow_;
	HandlerMap mapHandler_;
};

LRESULT qs::TimerImpl::windowProc(UINT uMsg,
								  WPARAM wParam,
								  LPARAM lParam)
{
	BEGIN_MESSAGE_HANDLER()
		HANDLE_TIMER()
	END_MESSAGE_HANDLER()
	return DefaultWindowHandler::windowProc(uMsg, wParam, lParam);
}

LRESULT qs::TimerImpl::onTimer(UINT nId)
{
	TimerImpl::HandlerMap::iterator it = std::find_if(
		mapHandler_.begin(), mapHandler_.end(),
		std::bind2nd(
			binary_compose_f_gx_hy(
				std::equal_to<unsigned int>(),
				std::select1st<HandlerMap::value_type>(),
				std::identity<unsigned int>()),
			nId));
	if (it != mapHandler_.end())
		(*it).second->timerTimeout(nId);
	
	return DefaultWindowHandler::onTimer(nId);
}


/****************************************************************************
 *
 * Timer
 *
 */

qs::Timer::Timer() :
	pImpl_(0)
{
	pImpl_ = new TimerImpl();
	pImpl_->pWindow_ = 0;
	
	std::auto_ptr<WindowBase> pWindow(new WindowBase(true));
	pWindow->setWindowHandler(pImpl_, false);
	if (!pWindow->create(L"QsTimerWindow", 0,
		WS_POPUP, 0, 0, 0, 0, 0, 0, 0, 0, 0))
		return;
	pImpl_->pWindow_ = pWindow.release();
}

qs::Timer::~Timer()
{
	if (pImpl_) {
		if (pImpl_->pWindow_)
			pImpl_->pWindow_->destroyWindow();
		delete pImpl_;
	}
}

unsigned int qs::Timer::setTimer(unsigned int nId,
								 unsigned int nTimeout,
								 TimerHandler* pHandler)
{
	assert(pHandler);
	
	TimerImpl::HandlerMap& m = pImpl_->mapHandler_;
	
	unsigned int nTimerId = pImpl_->pWindow_->setTimer(nId, nTimeout);
	if (nTimerId == 0)
		return 0;
	
	pImpl_->mapHandler_.push_back(std::make_pair(nTimerId, pHandler));
	
	return nTimerId;
}

void qs::Timer::killTimer(unsigned int nId)
{
	TimerImpl::HandlerMap& m = pImpl_->mapHandler_;
	TimerImpl::HandlerMap::iterator it = std::find_if(
		m.begin(), m.end(),
		std::bind2nd(
			binary_compose_f_gx_hy(
				std::equal_to<unsigned int>(),
				std::select1st<TimerImpl::HandlerMap::value_type>(),
				std::identity<unsigned int>()),
			nId));
	assert(it != m.end());
	m.erase(it);
	
	pImpl_->pWindow_->killTimer(nId);
}


/****************************************************************************
 *
 * TimerHandler
 *
 */

qs::TimerHandler::~TimerHandler()
{
}
