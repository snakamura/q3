/*
 * $Id$
 *
 * Copyright(C) 1998-2003 Satoshi Nakamura
 * All rights reserved.
 *
 */

#include <qsnew.h>
#include <qstimer.h>
#include <qswindow.h>

#pragma warning(disable:4786)

using namespace qs;


/****************************************************************************
 *
 * TimerImpl
 *
 */

class qs::TimerImpl : public DefaultWindowHandler
{
public:
	TimerImpl(QSTATUS* pstatus);
	virtual ~TimerImpl();

public:
	virtual LRESULT windowProc(UINT uMsg, WPARAM wParam, LPARAM lParam);

protected:
	LRESULT onTimer(UINT nId);

private:
	TimerImpl(const TimerImpl&);
	TimerImpl& operator=(const TimerImpl&);

public:
	typedef std::vector<std::pair<unsigned int, TimerHandler*> > HandlerMap;

public:
	WindowBase* pWindow_;
	HandlerMap mapHandler_;
};

qs::TimerImpl::TimerImpl(QSTATUS* pstatus) :
	DefaultWindowHandler(pstatus)
{
	if (*pstatus != QSTATUS_SUCCESS)
		return;
}

qs::TimerImpl::~TimerImpl()
{
}

LRESULT qs::TimerImpl::windowProc(UINT uMsg, WPARAM wParam, LPARAM lParam)
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

qs::Timer::Timer(QSTATUS* pstatus) :
	pImpl_(0)
{
	assert(pstatus);
	
	DECLARE_QSTATUS();
	
	*pstatus = QSTATUS_SUCCESS;
	
	status = newQsObject(&pImpl_);
	CHECK_QSTATUS_SET(pstatus);
	pImpl_->pWindow_ = 0;
	
	std::auto_ptr<WindowBase> pWindow;
	status = newQsObject(true, &pWindow);
	CHECK_QSTATUS_SET(pstatus);
	status = pWindow->setWindowHandler(pImpl_, false);
	CHECK_QSTATUS_SET(pstatus);
	status = pWindow->create(L"QsTimerWindow", 0, WS_POPUP,
		0, 0, 0, 0, 0, 0, 0, 0, 0);
	CHECK_QSTATUS_SET(pstatus);
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

QSTATUS qs::Timer::setTimer(unsigned int* pnId,
	unsigned int nTimeout, TimerHandler* pHandler)
{
	assert(pnId);
	assert(pHandler);
	
	DECLARE_QSTATUS();
	
	TimerImpl::HandlerMap& m = pImpl_->mapHandler_;
	status = STLWrapper<TimerImpl::HandlerMap>(m).reserve(m.size() + 1);
	CHECK_QSTATUS();
	
	*pnId = pImpl_->pWindow_->setTimer(*pnId, nTimeout);
	if (*pnId == 0)
		return QSTATUS_FAIL;
	
	m.push_back(std::make_pair(*pnId, pHandler));
	
	return QSTATUS_SUCCESS;
}

QSTATUS qs::Timer::killTimer(unsigned int nId)
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
	if (it == m.end())
		return QSTATUS_FAIL;
	m.erase(it);
	
	pImpl_->pWindow_->killTimer(nId);
	
	return QSTATUS_SUCCESS;
}


/****************************************************************************
 *
 * TimerHandler
 *
 */

qs::TimerHandler::~TimerHandler()
{
}
