/*
 * $Id: window.h,v 1.1.1.1 2003/04/29 08:07:37 snakamura Exp $
 *
 * Copyright(C) 1998-2003 Satoshi Nakamura
 * All rights reserved.
 *
 */

#ifndef __WINDOW_H__
#define __WINDOW_H__

#include <qs.h>
#include <qsthread.h>
#include <qsstl.h>

#include <hash_map>

#include <windows.h>

#pragma warning(disable:4786)

namespace qs {

/****************************************************************************
 *
 * hash_hwnd
 *
 */

struct hash_hwnd
{
	size_t operator()(HWND hwnd) const;
};

inline size_t qs::hash_hwnd::operator()(HWND hwnd) const
{
	return reinterpret_cast<int>(hwnd)/8;
}


/****************************************************************************
 *
 * ControllerMapBase
 *
 */

class ControllerMapBase
{
private:
	typedef std::hash_map<HWND, void*, hash_hwnd> Map;

public:
	ControllerMapBase(QSTATUS* pstatus);
	~ControllerMapBase();

public:
	QSTATUS initThread();
	QSTATUS termThread();

public:
	QSTATUS getThis(void** ppThis);
	QSTATUS setThis(void* pThis);
	
	QSTATUS getController(HWND hwnd, void** ppController);
	QSTATUS setController(HWND hwnd, void* pController);
	QSTATUS removeController(HWND hwnd);
	QSTATUS findController(HWND hwnd, void** ppController);

private:
	ControllerMapBase(const ControllerMapBase&);
	ControllerMapBase& operator=(const ControllerMapBase&);

private:
	ThreadLocal* pThis_;
	ThreadLocal* pMap_;
};


/****************************************************************************
 *
 * ControllerMap
 *
 */

template<class Controller>
class ControllerMap
{
public:
	ControllerMap(QSTATUS* pstatus);
	~ControllerMap();

public:
	QSTATUS initThread();
	QSTATUS termThread();

public:
	QSTATUS getThis(Controller** ppThis);
	QSTATUS setThis(Controller* pThis);
	
	QSTATUS getController(HWND hwnd, Controller** ppController);
	QSTATUS setController(HWND hwnd, Controller* pController);
	QSTATUS removeController(HWND hwnd);
	QSTATUS findController(HWND hwnd, Controller** ppController);

private:
	ControllerMap(const ControllerMap&);
	ControllerMap& operator=(const ControllerMap&);

private:
	ControllerMapBase base_;
};

}

#include "window.inl"

#pragma warning(default:4786)

#endif // __WINDOW_H__
