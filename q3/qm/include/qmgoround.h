/*
 * $Id$
 *
 * Copyright(C) 1998-2004 Satoshi Nakamura
 * All rights reserved.
 *
 */

#ifndef __QMGOROUND_H__
#define __QMGOROUND_H__

#include <qm.h>

#include <qs.h>


namespace qm {

class GoRound;

class GoRoundCourseList;


/****************************************************************************
 *
 * GoRound
 *
 */

class GoRound
{
public:
	GoRound();
	~GoRound();

public:
	const GoRoundCourseList* getCourseList();

private:
	GoRound(const GoRound&);
	GoRound& operator=(const GoRound&);

private:
	struct GoRoundImpl* pImpl_;
};

}

#endif // __QMGOROUND_H__
