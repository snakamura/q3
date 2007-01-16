/*
 * $Id$
 *
 * Copyright(C) 1998-2007 Satoshi Nakamura
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
	typedef unsigned int Offset;

public:
	struct Refer
	{
		Offset nOffset_;
		unsigned int nLength_;
	};

public:
	typedef std::vector<Refer> ReferList;

public:
	ClusterStorage(const WCHAR* pwszPath,
				   const WCHAR* pwszName,
				   const WCHAR* pwszBoxExt,
				   const WCHAR* pwszMapExt,
				   unsigned int nBlockSize);
	
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
	 * Rename storage.
	 *
	 * @param pwszPath [in] New path.
	 * @param pwszName [in] New name.
	 * @return true if success, false otherwise.
	 */
	bool rename(const WCHAR* pwszPath,
				const WCHAR* pwszName);
	
	/**
	 * Load data.
	 *
	 * @param p [in] Buffer.
	 * @param nOffset [in] Offset.
	 * @param nLength [in] Length to load.
	 * @return Length read. -1 if error occured.
	 */
	size_t load(unsigned char* p,
				Offset nOffset,
				size_t nLength);
	
	/**
	 * Save data.
	 *
	 * @param p [in] Array of buffer.
	 * @param nLength [in] Array of buffer length.
	 * @param nCount [in] Size of array.
	 * @return Offset. -1 if error occured.
	 */
	Offset save(const unsigned char* p[],
				size_t nLength[],
				size_t nCount);
	
	/**
	 * Free storage.
	 *
	 * @param nOffset [in] Offset.
	 * @param nLength [in] Length.
	 * @return true if success, false otherwise.
	 */
	bool free(Offset nOffset,
			  size_t nLength);
	
	/**
	 * Compact storage.
	 *
	 * @param nOffset [in] Offset.
	 * @param nLength [in] Length.
	 * @param pcsOld [in] Old storage. Can be null.
	 *                    If null, data will be loaded from this storage.
	 * @return New offset. -1 if error occured.
	 */
	Offset compact(Offset nOffset,
				   size_t nLength,
				   ClusterStorage* pcsOld);
	
	/**
	 * Free unrefered storage.
	 *
	 * @param listRefer [in] List of where are referred.
	 * @exception std::bad_alloc Out of memory.
	 */
	void freeUnrefered(const ReferList& listRefer);
	
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
