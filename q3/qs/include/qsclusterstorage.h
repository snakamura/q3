/*
 * $Id$
 *
 * Copyright(C) 1998-2004 Satoshi Nakamura
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
	/**
	 * Create instance.
	 * Call operator! to check if file is opened or not.
	 *
	 * @param init [in] Initialize data.
	 * @exception std::bad_alloc Out of memory.
	 */
	explicit ClusterStorage(const Init& init);
	
	~ClusterStorage();

public:
	/**
	 * Check if storage is open or not.
	 *
	 * @return true if failed, false otherwise.
	 */
	bool operator!() const;

public:
	/**
	 * Close storage.
	 *
	 * @return true if success, false otherwise.
	 */
	bool close();
	
	/**
	 * Flush storage.
	 *
	 * @return true if success, false otherwise.
	 */
	bool flush();
	
	/**
	 * Load data.
	 *
	 * @param p [in] Buffer.
	 * @param nOffset [in] Offset.
	 * @param nLength [in] Length to load.
	 * @return Length read. -1 if error occured.
	 */
	unsigned int load(unsigned char* p,
					  unsigned int nOffset,
					  unsigned int nLength);
	
	/**
	 * Save data.
	 *
	 * @param p [in] Array of buffer.
	 * @param nLength [in] Array of buffer length.
	 * @param nCount [in] Size of array.
	 * @return Length written. -1 if error occured.
	 */
	unsigned int save(const unsigned char* p[],
					  unsigned int nLength[],
					  size_t nCount);
	
	/**
	 * Free storage.
	 *
	 * @param nOffset [in] Offset.
	 * @param nLength [in] Length.
	 * @return true if success, false otherwise.
	 */
	bool free(unsigned int nOffset,
			  unsigned int nLength);
	
	/**
	 * Compact storage.
	 *
	 * @param nOffset [in] Offset.
	 * @param nLength [in] Length.
	 * @param pmsOld [in] Old storage. Can be null.
	 *                    If null, data will be loaded from this storage.
	 * @return New offset. -1 if error occured.
	 */
	unsigned int compact(unsigned int nOffset,
						 unsigned int nLength,
						 ClusterStorage* pmsOld);
	
	/**
	 * Free unrefered storage.
	 *
	 * @param listRefer [in] List of where are referred.
	 * @return true if success, false otherwise.
	 * @exception std::bad_alloc Out of memory.
	 */
	bool freeUnrefered(const ReferList& listRefer);
	
	/**
	 * Free unused storage.
	 *
	 * @return true if success, false otherwise.
	 */
	bool freeUnused();

private:
	ClusterStorage(const ClusterStorage&);
	ClusterStorage& operator=(const ClusterStorage&);

private:
	struct ClusterStorageImpl* pImpl_;
};

}

#endif // __CLUSTERSTORAGE_H__
