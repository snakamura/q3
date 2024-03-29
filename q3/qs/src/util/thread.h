/*
 * $Id$
 *
 * Copyright(C) 1998-2008 Satoshi Nakamura
 *
 */

#ifndef __THREAD_H__
#define __THREAD_H__

#include <qswindow.h>

namespace qs {

/****************************************************************************
 *
 * SynchronizerWindow
 *
 */

class SynchronizerWindow :
	public WindowBase,
	public DefaultWindowHandler
{
public:
	enum {
		WM_SYNCEXEC	= WM_APP + 1001,
		WM_ASYNCEXEC
	};

public:
	SynchronizerWindow();
	virtual ~SynchronizerWindow();

public:
	virtual LRESULT windowProc(UINT uMsg,
							   WPARAM wParam,
							   LPARAM lParam);

protected:
	LRESULT onSyncExec(WPARAM wParam,
					   LPARAM lParam);
	LRESULT onAsyncExec(WPARAM wParam,
						LPARAM lParam);

private:
	SynchronizerWindow(const SynchronizerWindow&);
	SynchronizerWindow& operator=(const SynchronizerWindow&);
};

}

#endif // __THREAD_H__
