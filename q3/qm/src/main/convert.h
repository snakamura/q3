/*
 * $Id$
 *
 * Copyright(C) 1998-2004 Satoshi Nakamura
 * All rights reserved.
 *
 */

#ifndef __CONVERT_H__
#define __CONVERT_H__

#include <qm.h>

#include <qsclusterstorage.h>


namespace qm {

struct Convert
{
	static bool convert(const WCHAR* pwszMailFolder);
	static bool check(const WCHAR* pwszMailFolder);
	static bool convertAccount(const WCHAR* pwszDir);
	static bool convertMessages(const WCHAR* pwszPath,
								qs::ClusterStorage* pCacheStorage,
								qs::ClusterStorage* pIndexStorage);
	static void clearAccount(const WCHAR* pwszDir);
	static void deleteFiles(const WCHAR* pwszDir,
							const WCHAR* pwszFilter);
};

}

#endif // __CONVERT_H__
