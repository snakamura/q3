/*
 * $Id: qmgoround.h,v 1.1.1.1 2003/04/29 08:07:31 snakamura Exp $
 *
 * Copyright(C) 1998-2003 Satoshi Nakamura
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
	GoRound(const WCHAR* pwszPath, qs::QSTATUS* pstatus);
	~GoRound();

public:
	qs::QSTATUS getCourseList(GoRoundCourseList** ppCourseList);

private:
	GoRound(const GoRound&);
	GoRound& operator=(const GoRound&);

private:
	struct GoRoundImpl* pImpl_;
};

}

#endif // __QMGOROUND_H__
