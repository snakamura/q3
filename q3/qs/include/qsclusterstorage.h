/*
 * $Id$
 *
 * Copyright(C) 1998-2003 Satoshi Nakamura
 * All rights reserved.
 *
 */

#ifndef __CLUSTERSTORAGE_H__
#define __CLUSTERSTORAGE_H__

#include <qs.h>

#include <vector>


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
	
	struct Refer
	{
		unsigned int nOffset_;
		unsigned int nLength_;
	};

public:
	typedef std::vector<Refer> ReferList;

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
	QSTATUS freeUnrefered(const ReferList& listRefer);
	QSTATUS freeUnused();

private:
	ClusterStorage(const ClusterStorage&);
	ClusterStorage& operator=(const ClusterStorage&);

private:
	struct ClusterStorageImpl* pImpl_;
};

}

#endif // __CLUSTERSTORAGE_H__
