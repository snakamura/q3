/*
 * $Id$
 *
 * Copyright(C) 1998-2006 Satoshi Nakamura
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
	typedef UINT_PTR Id;

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
	 * @return true if success, false otherwise.
	 * @exception std::bad_alloc Out of memory.
	 */
	bool setTimer(Id nId,
				  unsigned int nTimeout,
				  TimerHandler* pHandler);
	
	/**
	 * Kill timer.
	 *
	 * @param nId [in] Timer ID.
	 */
	void killTimer(Id nId);

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
	virtual void timerTimeout(Timer::Id nId) = 0;
};

}

#endif // __QSTIMER_H__
