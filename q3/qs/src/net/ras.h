/*
 * $Id$
 *
 * Copyright(C) 1998-2008 Satoshi Nakamura
 *
 */

#ifndef __RAS_H__
#define __RAS_H__

#include <qsras.h>
#include <qswindow.h>


namespace qs {

/****************************************************************************
 *
 * RasWindow
 *
 */

class RasWindow : public WindowBase, public DefaultWindowHandler
{
public:
	enum {
		TIMER_RAS	= 1000,
		TIMEOUT		= 500
	};

public:
	RasWindow(RasConnection* pConnection,
			  RasConnectionCallback* pCallback);
	virtual ~RasWindow();

public:
	bool isEnd() const;

public:
	virtual LRESULT windowProc(UINT uMsg,
							   WPARAM wParam,
							   LPARAM lParam);

protected:
	LRESULT onTimer(UINT_PTR nId);
	LRESULT onRasDialEvent(WPARAM wParam,
						   LPARAM lParam);

private:
	void end(bool bDisconnect);

private:
	RasWindow(const RasWindow&);
	RasWindow& operator=(const RasWindow&);

private:
	RasConnection* pConnection_;
	RasConnectionCallback* pCallback_;
	bool bTimer_;
	bool bEnd_;
	bool bCanceled_;

private:
	static const UINT nRasDialEventMessage__;
};

}

#endif // __RAS_H__
