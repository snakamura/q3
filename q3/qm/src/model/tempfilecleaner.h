/*
 * $Id: tempfilecleaner.h,v 1.1.1.1 2003/04/29 08:07:31 snakamura Exp $
 *
 * Copyright(C) 1998-2003 Satoshi Nakamura
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
	TempFileCleaner(qs::QSTATUS* pstatus);
	~TempFileCleaner();

public:
	qs::QSTATUS add(const WCHAR* pwszPath);
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
