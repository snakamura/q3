/*
 * $Id$
 *
 * Copyright(C) 1998-2005 Satoshi Nakamura
 * All rights reserved.
 *
 */

#ifndef __UTIL_H__
#define __UTIL_H__

#include <qsstring.h>

namespace qmpgp {

/****************************************************************************
 *
 * Util
 *
 */

class Util
{
public:
	static qs::wstring_ptr writeTemporaryFile(const CHAR* psz,
											  size_t nLen);
};


/****************************************************************************
 *
 * FileDeleter
 *
 */

class FileDeleter
{
public:
	FileDeleter(const WCHAR* pwszPath);
	~FileDeleter();

private:
	FileDeleter(const FileDeleter&);
	FileDeleter& operator=(const FileDeleter&);

private:
	const WCHAR* pwszPath_;
};

}

#endif // __UTIL_H__
