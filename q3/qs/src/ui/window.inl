/*
 * $Id$
 *
 * Copyright(C) 1998-2003 Satoshi Nakamura
 * All rights reserved.
 *
 */

#ifndef __WINDOW_INL__
#define __WINDOW_INL__


/****************************************************************************
 *
 * ControllerMap
 *
 */

template<class Controller>
qs::ControllerMap<Controller>::ControllerMap(QSTATUS* pstatus) :
	base_(pstatus)
{
}

template<class Controller>
qs::ControllerMap<Controller>::~ControllerMap()
{
}

template<class Controller>
qs::QSTATUS qs::ControllerMap<Controller>::initThread()
{
	return base_.initThread();
}

template<class Controller>
qs::QSTATUS qs::ControllerMap<Controller>::termThread()
{
	return base_.termThread();
}

template<class Controller>
qs::QSTATUS qs::ControllerMap<Controller>::getThis(Controller** ppThis)
{
	return base_.getThis(reinterpret_cast<void**>(ppThis));
}

template<class Controller>
qs::QSTATUS qs::ControllerMap<Controller>::setThis(Controller* pThis)
{
	return base_.setThis(pThis);
}

template<class Controller>
qs::QSTATUS qs::ControllerMap<Controller>::getController(HWND hwnd, Controller** ppController)
{
	return base_.getController(hwnd, reinterpret_cast<void**>(ppController));
}

template<class Controller>
qs::QSTATUS qs::ControllerMap<Controller>::setController(HWND hwnd, Controller* pController)
{
	return base_.setController(hwnd, pController);
}

template<class Controller>
qs::QSTATUS qs::ControllerMap<Controller>::removeController(HWND hwnd)
{
	return base_.removeController(hwnd);
}

template<class Controller>
qs::QSTATUS qs::ControllerMap<Controller>::findController(HWND hwnd, Controller** ppController)
{
	assert(hwnd);
	assert(ppController);
	
	DECLARE_QSTATUS();
	
	Controller* pThis = 0;
	status = getController(hwnd, &pThis);
	CHECK_QSTATUS_VALUE(0);
	if (!pThis) {
		status = getThis(&pThis);
		CHECK_QSTATUS_VALUE(0);
		if (pThis) {
			assert(!pThis->getHandle());
			pThis->setHandle(hwnd);
			status = setController(hwnd, pThis);
			CHECK_QSTATUS_VALUE(0);
			status = setThis(0);
			CHECK_QSTATUS_VALUE(0);
		}
	}
	*ppController = pThis;
	
	return QSTATUS_SUCCESS;
}

#endif // __WINDOW_INL__
