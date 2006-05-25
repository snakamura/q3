/*
 * $Id$
 *
 * Copyright(C) 1998-2006 Satoshi Nakamura
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
qs::ControllerMap<Controller>::ControllerMap()
{
}

template<class Controller>
qs::ControllerMap<Controller>::~ControllerMap()
{
}

template<class Controller>
bool qs::ControllerMap<Controller>::initThread()
{
	return base_.initThread();
}

template<class Controller>
void qs::ControllerMap<Controller>::termThread()
{
	base_.termThread();
}

template<class Controller>
Controller* qs::ControllerMap<Controller>::getThis()
{
	return static_cast<Controller*>(base_.getThis());
}

template<class Controller>
void qs::ControllerMap<Controller>::setThis(Controller* pThis)
{
	base_.setThis(pThis);
}

template<class Controller>
Controller* qs::ControllerMap<Controller>::getController(HWND hwnd)
{
	return static_cast<Controller*>(base_.getController(hwnd));
}

template<class Controller>
void qs::ControllerMap<Controller>::setController(HWND hwnd,
												  Controller* pController)
{
	base_.setController(hwnd, pController);
}

template<class Controller>
void qs::ControllerMap<Controller>::removeController(HWND hwnd)
{
	base_.removeController(hwnd);
}

template<class Controller>
Controller* qs::ControllerMap<Controller>::findController(HWND hwnd)
{
	assert(hwnd);
	
	Controller* pThis = getController(hwnd);
	if (!pThis) {
		pThis = getThis();
		if (pThis) {
			assert(!pThis->getHandle());
			pThis->setHandle(hwnd);
			setController(hwnd, pThis);
			setThis(0);
		}
	}
	return pThis;
}

#endif // __WINDOW_INL__
