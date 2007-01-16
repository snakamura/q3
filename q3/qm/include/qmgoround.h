/*
 * $Id$
 *
 * Copyright(C) 1998-2007 Satoshi Nakamura
 * All rights reserved.
 *
 */

#ifndef __QMGOROUND_H__
#define __QMGOROUND_H__

#include <qm.h>

#include <qs.h>

#include <vector>


namespace qm {

class GoRound;

class GoRoundCourse;


/****************************************************************************
 *
 * GoRound
 *
 */

class GoRound
{
public:
	typedef std::vector<GoRoundCourse*> CourseList;

public:
	explicit GoRound(const WCHAR* pwszPath);
	~GoRound();

public:
	const CourseList& getCourses() const;
	const CourseList& getCourses(bool bReload) const;
	void setCourses(CourseList& listCourse);
	GoRoundCourse* getCourse(const WCHAR* pwszCourse) const;
	bool save() const;
	void clear();

private:
	GoRound(const GoRound&);
	GoRound& operator=(const GoRound&);

private:
	struct GoRoundImpl* pImpl_;
};

}

#endif // __QMGOROUND_H__
