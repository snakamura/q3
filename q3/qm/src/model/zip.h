/*
 * $Id$
 *
 * Copyright(C) 1998-2006 Satoshi Nakamura
 * All rights reserved.
 */

#ifndef __ZIP_H__
#define __ZIP_H__

#ifdef QMZIP

#include <qm.h>

#include <qsstring.h>

#include <vector>


namespace qm {

/****************************************************************************
 *
 * ZipFile
 *
 */

class ZipFile
{
public:
	typedef std::vector<const WCHAR*> PathList;

public:
	ZipFile(const WCHAR* pwszFileName,
			const PathList& listPath,
			const WCHAR* pwszTempDir);
	~ZipFile();

public:
	const WCHAR* getPath() const;

private:
	static bool zip(const WCHAR* pwszPath,
					const PathList& listPath);

private:
	ZipFile(const ZipFile&);
	ZipFile& operator=(const ZipFile&);

private:
	qs::wstring_ptr wstrDir_;
	qs::wstring_ptr wstrPath_;
};

}

#endif // QMZIP

#endif // __ZIP_H__
