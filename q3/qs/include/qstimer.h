/*
 * $Id$
 *
 * Copyright(C) 1998-2003 Satoshi Nakamura
 * All rights reserved.
 *
 */

#ifndef __QSTIMER_H__
#define __QSTIMER_H__

#include <qs.h>


namespace qs {

class Timer;
class TimerHandler;


/****************************************************************************
 *
 * Timer
 *
 */

class QSEXPORTCLASS Timer
{
public:
	Timer(QSTATUS* pstatus);
	~Timer();

public:
	QSTATUS setTimer(unsigned int* pnId,
		unsigned int nTimeout, TimerHandler* pHandler);
	QSTATUS killTimer(unsigned int nId);

private:
	Timer(const Timer&);
	Timer& operator=(const Timer&);

private:
	class TimerImpl* pImpl_;
};


/****************************************************************************
 *
 * TimerHandler
 *
 */

class QSEXPORTCLASS TimerHandler
{
public:
	virtual ~TimerHandler();

public:
	virtual QSTATUS timerTimeout(unsigned int nId) = 0;
};

}

#endif // __QSTIMER_H__
