/*
 * $Id$
 *
 * Copyright(C) 1998-2004 Satoshi Nakamura
 * All rights reserved.
 *
 */

#ifndef __TEMPFILECLEANER_H__
#define __TEMPFILECLEANER_H__

#include <qm.h>

#include <qs.h>
#include <qsstring.h>

#include <vector>


namespace qm {

class TempFileCleaner;
class TempFileCleanerCallback;


/****************************************************************************
 *
 * TempFileCleaner
 *
 */

class TempFileCleaner
{
public:
	TempFileCleaner();
	~TempFileCleaner();

public:
	void add(const WCHAR* pwszPath);
	void clean(TempFileCleanerCallback* pCallback);

private:
	TempFileCleaner(const TempFileCleaner&);
	TempFileCleaner& operator=(const TempFileCleaner&);

private:
	typedef std::vector<std::pair<qs::TSTRING, FILETIME> > List;

private:
	List list_;
};


/****************************************************************************
 *
 * TempFileCleanerCallback
 *
 */

class TempFileCleanerCallback
{
public:
	virtual ~TempFileCleanerCallback();

public:
	virtual bool confirmDelete(const WCHAR* pwszPath) = 0;
};

}

#endif // __TEMPFILECLEANER_H__
