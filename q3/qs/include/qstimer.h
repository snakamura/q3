/*
 * $Id$
 *
 * Copyright(C) 1998-2005 Satoshi Nakamura
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
	/**
	 * Create instance.
	 *
	 * @exception std::bad_alloc Out of memory.
	 */
	Timer();
	
	~Timer();

public:
	/**
	 * Set timer.
	 *
	 * @param nId [in] Timer ID.
	 * @param nTimeout [in] Timeout in millisecond.
	 * @param pHandler [in] Handler which is callbacked when timer timeouts.
	 * @return Timer ID. -1 if fail.
	 * @exception std::bad_alloc Out of memory.
	 */
	unsigned int setTimer(unsigned int nId,
						  unsigned int nTimeout,
						  TimerHandler* pHandler);
	
	/**
	 * Kill timer.
	 *
	 * @param nId [in] Timer ID.
	 */
	void killTimer(unsigned int nId);

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
	/**
	 * This method is called when timer timeouts.
	 *
	 * @param nId [in] Timer ID.
	 */
	virtual void timerTimeout(unsigned int nId) = 0;
};

}

#endif // __QSTIMER_H__
