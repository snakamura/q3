/*
 * $Id: qsclusterstorage.h,v 1.1.1.1 2003/04/29 08:07:34 snakamura Exp $
 *
 * Copyright(C) 1998-2003 Satoshi Nakamura
 * All rights reserved.
 *
 */

#ifndef __ClusterStorage_H__
#define __ClusterStorage_H__

#include <qs.h>


namespace qs {

/****************************************************************************
 *
 * ClusterStorage
 *
 */

class QSEXPORTCLASS ClusterStorage
{
public:
	struct Init
	{
		const WCHAR* pwszPath_;
		const WCHAR* pwszName_;
		const WCHAR* pwszBoxExt_;
		const WCHAR* pwszMapExt_;
		size_t nBlockSize_;
	};

public:
	ClusterStorage(const Init& init, QSTATUS* pstatus);
	~ClusterStorage();

public:
	QSTATUS close();
	QSTATUS flush();
	QSTATUS load(unsigned char* p,
		unsigned int nOffset, unsigned int* pnLength);
	QSTATUS save(const unsigned char* p[], unsigned int nLength[],
		size_t nCount, unsigned int* pnOffset);
	QSTATUS free(unsigned int nOffset, unsigned int nLength);
	QSTATUS compact(unsigned int nOffset, unsigned int nLength,
		ClusterStorage* pmsOld, unsigned int* pnOffset);
	QSTATUS freeUnused();

private:
	ClusterStorage(const ClusterStorage&);
	ClusterStorage& operator=(const ClusterStorage&);

private:
	struct ClusterStorageImpl* pImpl_;
};

}

#endif // __ClusterStorage_H__
