/*
 * $Id$
 *
 * Copyright(C) 1998-2005 Satoshi Nakamura
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

#include "uiutil.h"

#pragma warning(disable:4786)

namespace qs {

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
	ControllerMapBase();
	~ControllerMapBase();

public:
	bool initThread();
	void termThread();

public:
	void* getThis();
	void setThis(void* pThis);
	
	void* getController(HWND hwnd);
	void setController(HWND hwnd,
					   void* pController);
	void removeController(HWND hwnd);
	void* findController(HWND hwnd);

private:
	ControllerMapBase(const ControllerMapBase&);
	ControllerMapBase& operator=(const ControllerMapBase&);

private:
	std::auto_ptr<ThreadLocal> pThis_;
	std::auto_ptr<ThreadLocal> pMap_;
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
	ControllerMap();
	~ControllerMap();

public:
	bool initThread();
	void termThread();

public:
	Controller* getThis();
	void setThis(Controller* pThis);
	
	Controller* getController(HWND hwnd);
	void setController(HWND hwnd,
					   Controller* pController);
	void removeController(HWND hwnd);
	Controller* findController(HWND hwnd);

private:
	ControllerMap(const ControllerMap&);
	ControllerMap& operator=(const ControllerMap&);

private:
	ControllerMapBase base_;
};


#if defined _WIN32_WCE && !defined _WIN32_WCE_EMULATION
/****************************************************************************
 *
 * WindowDestroy
 *
 */

class WindowDestroy
{
public:
	WindowDestroy();
	~WindowDestroy();

public:
	void process(HWND hwnd);

public:
	static WindowDestroy* getWindowDestroy();

private:
	bool isMapped(HWND hwnd);
	void destroy(HWND hwnd);
	void remove(HWND hwnd);

private:
	typedef std::vector<HWND> WindowList;
	typedef std::vector<std::pair<HWND, WindowList> > DestroyList;

private:
	DestroyList listDestroy_;

private:
	static ThreadLocal* pWindowDestroy__;
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

friend class InitializerImpl;
};
#endif

}

#include "window.inl"

#pragma warning(default:4786)

#endif // __WINDOW_H__
