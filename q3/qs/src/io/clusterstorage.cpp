/*
 * $Id$
 *
 * Copyright(C) 1998-2003 Satoshi Nakamura
 * All rights reserved.
 *
 */

#include <qsclusterstorage.h>
#include <qsconv.h>
#include <qserror.h>
#include <qsfile.h>
#include <qsnew.h>
#include <qsstl.h>
#include <qsstream.h>

#include <numeric>
#include <vector>

using namespace qs;


/****************************************************************************
 *
 * ClusterStorageImpl
 *
 */

struct qs::ClusterStorageImpl
{
	enum {
		CLUSTER_SIZE		= 128,
		SEARCHBEGIN_SIZE	= 64,
		BYTE_SIZE			= 8
	};
	
	typedef std::vector<unsigned char> Map;
	typedef std::vector<Map::size_type> SearchBegin;
	
	QSTATUS loadMap();
	QSTATUS saveMap() const;
	QSTATUS reopen();
	QSTATUS getFreeOffset(unsigned int nSize, unsigned int* pnOffset);
	QSTATUS getFreeOffset(unsigned int nSize,
		unsigned int nCurrentOffset, unsigned int* pnOffset);
	Map::size_type getSearchBegin(unsigned int nSize) const;
	void setSearchBegin(unsigned int nSize, Map::size_type nBegin, bool bFree);
	
	WSTRING wstrPath_;
	WSTRING wstrBoxExt_;
	WSTRING wstrMapExt_;
	size_t nBlockSize_;
	File* pFile_;
	Map map_;
	SearchBegin searchBegin_;
};

QSTATUS qs::ClusterStorageImpl::loadMap()
{
	DECLARE_QSTATUS();
	
	string_ptr<WSTRING> wstrPath(concat(wstrPath_, wstrMapExt_));
	if (!wstrPath.get())
		return QSTATUS_OUTOFMEMORY;
	
	W2T(wstrPath.get(), ptszPath);
	if (::GetFileAttributes(ptszPath) != 0xffffffff) {
		FileInputStream fileStream(wstrPath.get(), &status);
		CHECK_QSTATUS();
		
		BufferedInputStream stream(&fileStream, false, &status);
		CHECK_QSTATUS();
		
		STLWrapper<Map> wrapper(map_);
		size_t nRead = 0;
		while (true) {
			unsigned char c = 0;
			status = stream.read(&c, sizeof(c), &nRead);
			CHECK_QSTATUS();
			if (nRead == -1)
				break;
			status = wrapper.push_back(c);
			CHECK_QSTATUS();
		}
	}
	
	return QSTATUS_SUCCESS;
}

QSTATUS qs::ClusterStorageImpl::saveMap() const
{
	DECLARE_QSTATUS();
	
	string_ptr<WSTRING> wstrPath(concat(wstrPath_, wstrMapExt_));
	if (!wstrPath.get())
		return QSTATUS_OUTOFMEMORY;
	
	TemporaryFileRenamer renamer(wstrPath.get(), &status);
	CHECK_QSTATUS();
	
	FileOutputStream fileStream(renamer.getPath(), &status);
	CHECK_QSTATUS();
	BufferedOutputStream stream(&fileStream, false, &status);
	CHECK_QSTATUS();
	
	Map::const_iterator it = map_.begin();
	while (it != map_.end()) {
		unsigned char c = *it;
		status = stream.write(&c, sizeof(c));
		CHECK_QSTATUS();
		++it;
	}
	
	status = stream.close();
	CHECK_QSTATUS();
	
	status = renamer.rename();
	CHECK_QSTATUS();
	
	return QSTATUS_SUCCESS;
}

QSTATUS qs::ClusterStorageImpl::reopen()
{
	DECLARE_QSTATUS();
	
	if (pFile_)
		return QSTATUS_SUCCESS;
	
	string_ptr<WSTRING> wstrPath(concat(wstrPath_, wstrBoxExt_));
	if (!wstrPath.get())
		return QSTATUS_OUTOFMEMORY;
	
	if (nBlockSize_ == static_cast<size_t>(0xffffffff)) {
		BinaryFile* pFile = 0;
		status = newQsObject(wstrPath.get(),
			BinaryFile::MODE_READ | BinaryFile::MODE_WRITE, 256, &pFile);
		CHECK_QSTATUS();
		pFile_ = pFile;
	}
	else {
		DividedFile* pFile = 0;
		status = newQsObject(wstrPath.get(), nBlockSize_,
			BinaryFile::MODE_READ | BinaryFile::MODE_WRITE, 256, &pFile);
		CHECK_QSTATUS();
		pFile_ = pFile;
	}
	
	return QSTATUS_SUCCESS;
}

QSTATUS qs::ClusterStorageImpl::getFreeOffset(
	unsigned int nSize, unsigned int* pnOffset)
{
	return getFreeOffset(nSize, 0xffffffff, pnOffset);
}

QSTATUS qs::ClusterStorageImpl::getFreeOffset(unsigned int nSize,
	unsigned int nCurrentOffset, unsigned int* pnOffset)
{
	assert(pnOffset);
	
	*pnOffset = 0xffffffff;
	
	DECLARE_QSTATUS();
	
	nSize = nSize + (CLUSTER_SIZE - nSize%CLUSTER_SIZE);
	assert(nSize%CLUSTER_SIZE == 0);
	nSize /= CLUSTER_SIZE;
	
	if (nCurrentOffset != 0xffffffff)
		nCurrentOffset /= BYTE_SIZE;
	
	unsigned int nBegin = 0;
	unsigned int nBeginBit = 0;
	unsigned int nEnd = 0;
	unsigned int nEndBit = 0;
	unsigned int nFindSize = 0;
	Map::iterator it = map_.begin();
	Map::size_type nSearchBegin = getSearchBegin(nSize);
	if (nSearchBegin == 0xffffffff)
		it = map_.end() - 1;
	else if (nSearchBegin < map_.size())
		it += nSearchBegin;
	for (; it != map_.end() && nFindSize < nSize; ++it) {
		if (static_cast<unsigned int>(it - map_.begin()) >= nCurrentOffset)
			return QSTATUS_SUCCESS;
		
		if (*it == 0xff) {
			if (nFindSize != 0)
				setSearchBegin(nFindSize, nBegin, false);
			nFindSize = 0;
		}
		else {
			BYTE b = 1;
			for (unsigned int n = 0; n < BYTE_SIZE && nFindSize < nSize; ++n, b <<= 1) {
				if (!(*it & b)) {
					if (nFindSize == 0) {
						nBegin = it - map_.begin();
						nBeginBit = n;
					}
					++nFindSize;
					if (nFindSize >= nSize) {
						nEnd = it - map_.begin();
						nEndBit = n + 1;
					}
				}
				else {
					if (nFindSize != 0)
						setSearchBegin(nFindSize, nBegin, false);
					nFindSize = 0;
				}
			}
		}
	}
	if (nFindSize != 0) {
		if (nFindSize < nSize) {
			nEnd = map_.size() - 1;
			nEndBit = BYTE_SIZE;
		}
		for (unsigned int n = nBegin; n <= nEnd; ++n) {
			BYTE bMask = 0;
			unsigned int nFirstBit = n == nBegin ? nBeginBit : 0;
			unsigned int nLastBit = n == nEnd ? nEndBit : BYTE_SIZE;
			if (nFirstBit == 0 && nLastBit == BYTE_SIZE) {
				bMask = 0xff;
			}
			else {
				BYTE b = 1;
				b <<= nFirstBit;
				for (unsigned int m = nFirstBit; m < nLastBit; ++m, b <<= 1)
					bMask |= b;
			}
			map_[n] |= bMask;
		}
	}
	else {
		nBegin = map_.size();
		nBeginBit = 0;
	}
	if (nFindSize < nSize) {
		STLWrapper<Map> wrapper(map_);
		for (unsigned int n = 0; n < (nSize - nFindSize)/BYTE_SIZE; ++n) {
			status = wrapper.push_back(0xff);
			CHECK_QSTATUS();
		}
		if ((nSize - nFindSize)%BYTE_SIZE != 0) {
			BYTE bData = 0;
			BYTE b = 1;
			for (unsigned int m = 0; m < (nSize - nFindSize)%BYTE_SIZE; ++m, b <<= 1)
				bData |= b;
			status = wrapper.push_back(bData);
			CHECK_QSTATUS();
		}
		status = pFile_->setPosition(
			(map_.size()*BYTE_SIZE + (nSize - nFindSize)%BYTE_SIZE)*CLUSTER_SIZE,
			File::SEEKORIGIN_BEGIN);
		CHECK_QSTATUS();
		status = pFile_->setEndOfFile();
		CHECK_QSTATUS();
		setSearchBegin(nSize, 0xffffffff, false);
	}
	else {
		setSearchBegin(nSize, nBegin, false);
	}
	*pnOffset = nBegin*BYTE_SIZE + nBeginBit;
	
	return QSTATUS_SUCCESS;
}

ClusterStorageImpl::Map::size_type qs::ClusterStorageImpl::getSearchBegin(
	unsigned int nSize) const
{
	assert(searchBegin_.size() == SEARCHBEGIN_SIZE);
	
	if (nSize >= searchBegin_.size())
		nSize = searchBegin_.size() - 1;
	SearchBegin::size_type n = nSize;
	while (n > 0 && searchBegin_[n] == 0)
		--n;
	return searchBegin_[n];
}

void qs::ClusterStorageImpl::setSearchBegin(
	unsigned int nSize, Map::size_type nBegin, bool bFree)
{
	assert(searchBegin_.size() == SEARCHBEGIN_SIZE);
	
	if (nSize >= searchBegin_.size())
		nSize = searchBegin_.size() - 1;
	
	if (nBegin == 0xffffffff) {
		for (unsigned int n = nSize + 1; n < searchBegin_.size(); ++n) {
			if (searchBegin_[n] == 0)
				break;
			searchBegin_[n] = nBegin;
		}
	}
	
	if (bFree) {
		while (nSize > 0) {
			Map::size_type& n = searchBegin_[nSize];
			if (n == 0)
				;
			else if (n <= nBegin)
				break;
			else
				n = nBegin;
			--nSize;
		}
	}
	else {
		while (nSize > 0) {
			Map::size_type& n = searchBegin_[nSize];
			if (n != 0 && n <= nBegin)
				break;
			n = nBegin;
			--nSize;
		}
	}
}


/****************************************************************************
 *
 * ClusterStorage
 *
 */

qs::ClusterStorage::ClusterStorage(const Init& init, QSTATUS* pstatus)
{
	assert(pstatus);
	
	*pstatus = QSTATUS_SUCCESS;
	
	DECLARE_QSTATUS();
	
	string_ptr<WSTRING> wstrPath(
		concat(init.pwszPath_, L"\\", init.pwszName_));
	if (!wstrPath.get()) {
		*pstatus = QSTATUS_OUTOFMEMORY;
		return;
	}
	
	string_ptr<WSTRING> wstrBoxExt(allocWString(init.pwszBoxExt_));
	if (!wstrBoxExt.get()) {
		*pstatus = QSTATUS_OUTOFMEMORY;
		return;
	}
	
	string_ptr<WSTRING> wstrMapExt(allocWString(init.pwszMapExt_));
	if (!wstrMapExt.get()) {
		*pstatus = QSTATUS_OUTOFMEMORY;
		return;
	}
	
	status = newObject(&pImpl_);
	CHECK_QSTATUS_SET(pstatus);
	pImpl_->wstrPath_ = wstrPath.release();
	pImpl_->wstrBoxExt_ = wstrBoxExt.release();
	pImpl_->wstrMapExt_ = wstrMapExt.release();
	pImpl_->nBlockSize_ = init.nBlockSize_;
	pImpl_->pFile_ = 0;
	
	status = STLWrapper<ClusterStorageImpl::SearchBegin>(
		pImpl_->searchBegin_).resize(ClusterStorageImpl::SEARCHBEGIN_SIZE);
	CHECK_QSTATUS_SET(pstatus);
	
	status = pImpl_->loadMap();
	CHECK_QSTATUS_SET(pstatus);
}

qs::ClusterStorage::~ClusterStorage()
{
	if (pImpl_) {
		freeWString(pImpl_->wstrPath_);
		freeWString(pImpl_->wstrBoxExt_);
		freeWString(pImpl_->wstrMapExt_);
		delete pImpl_->pFile_;
		delete pImpl_;
		pImpl_ = 0;
	}
}

QSTATUS qs::ClusterStorage::close()
{
	DECLARE_QSTATUS();
	
	status = flush();
	CHECK_QSTATUS();
	
	if (pImpl_->pFile_) {
		status = pImpl_->pFile_->close();
		CHECK_QSTATUS();
		delete pImpl_->pFile_;
		pImpl_->pFile_ = 0;
	}
	
	return QSTATUS_SUCCESS;
}

QSTATUS qs::ClusterStorage::flush()
{
	DECLARE_QSTATUS();
	
	status = pImpl_->saveMap();
	CHECK_QSTATUS();
	
	if (pImpl_->pFile_) {
		status = pImpl_->pFile_->flush();
		CHECK_QSTATUS();
	}
	
	return QSTATUS_SUCCESS;
}

QSTATUS qs::ClusterStorage::load(unsigned char* p,
	unsigned int nOffset, unsigned int* pnLength)
{
	assert(p);
	assert(pnLength);
	assert(*pnLength != 0);
	
	DECLARE_QSTATUS();
	
	status = pImpl_->reopen();
	CHECK_QSTATUS();
	
	status = pImpl_->pFile_->setPosition(
		nOffset*ClusterStorageImpl::CLUSTER_SIZE,
		File::SEEKORIGIN_BEGIN);
	CHECK_QSTATUS();
	
	size_t nRead = 0;
	status = pImpl_->pFile_->read(p, *pnLength, &nRead);
	CHECK_QSTATUS();
	*pnLength = nRead;
	
	return QSTATUS_SUCCESS;
}

QSTATUS qs::ClusterStorage::save(const unsigned char* p[],
	unsigned int nLength[], size_t nCount, unsigned int* pnOffset)
{
	assert(p);
	assert(pnOffset);
	
	*pnOffset = 0xffffffff;
	
	DECLARE_QSTATUS();
	
	size_t n = 0;
	
	status = pImpl_->reopen();
	CHECK_QSTATUS();
	
	unsigned int nAllLength = std::accumulate(&nLength[0], &nLength[nCount], 0);
	unsigned int nOffset = 0;
	status = pImpl_->getFreeOffset(nAllLength, &nOffset);
	CHECK_QSTATUS();
	
	struct Free
	{
		Free(ClusterStorage* pms, unsigned int nOffset, unsigned int nLength) :
			pms_(pms), nOffset_(nOffset), nLength_(nLength) {}
		~Free()
		{
			if (pms_)
				pms_->free(nOffset_, nLength_);
		}
		void release() { pms_ = 0; }
		
		ClusterStorage* pms_;
		unsigned int nOffset_;
		unsigned int nLength_;
	} free(this, nOffset, nAllLength);
	
	File* pFile = pImpl_->pFile_;
	
	status = pFile->setPosition(nOffset*ClusterStorageImpl::CLUSTER_SIZE,
		File::SEEKORIGIN_BEGIN);
	CHECK_QSTATUS();
	
	for (n = 0; n < nCount; ++n) {
		status = pFile->write(p[n], nLength[n]);
		CHECK_QSTATUS();
	}
	status = pFile->flush();
	CHECK_QSTATUS();
	
	free.release();
	*pnOffset = nOffset;
	
	return QSTATUS_SUCCESS;
}

QSTATUS qs::ClusterStorage::free(unsigned int nOffset, unsigned int nLength)
{
	DECLARE_QSTATUS();
	
	status = pImpl_->reopen();
	CHECK_QSTATUS();
	
	nLength = nLength + (ClusterStorageImpl::CLUSTER_SIZE -
		nLength%ClusterStorageImpl::CLUSTER_SIZE);
	assert(nLength%ClusterStorageImpl::CLUSTER_SIZE == 0);
	nLength /= ClusterStorageImpl::CLUSTER_SIZE;
	
	unsigned int nBegin = nOffset/ClusterStorageImpl::BYTE_SIZE;
	unsigned int nBeginBit = nOffset%ClusterStorageImpl::BYTE_SIZE;
	unsigned int nEnd = (nOffset + nLength)/ClusterStorageImpl::BYTE_SIZE;
	unsigned int nEndBit = (nOffset + nLength)%ClusterStorageImpl::BYTE_SIZE;
	if (nEndBit == 0) {
		--nEnd;
		nEndBit = ClusterStorageImpl::BYTE_SIZE;
	}
	assert(pImpl_->map_.size() > nEnd);
	for (unsigned int n = nBegin; n <= nEnd; ++n) {
		BYTE bMask = 0;
		unsigned int nFirstBit = n == nBegin ? nBeginBit : 0;
		unsigned int nLastBit = n == nEnd ? nEndBit : ClusterStorageImpl::BYTE_SIZE;
		if (nFirstBit != 0) {
			BYTE b = 1;
			for (unsigned int m = 0; m < nFirstBit; ++m, b <<= 1)
				bMask |= b;
		}
		if (nLastBit != ClusterStorageImpl::BYTE_SIZE) {
			BYTE b = 1 << nLastBit;
			for (unsigned int m = nLastBit; m < ClusterStorageImpl::BYTE_SIZE; ++m, b <<= 1)
				bMask |= b;
		}
		pImpl_->map_[n] &= bMask;
	}
	
	pImpl_->setSearchBegin(nLength, nBegin, true);
	
	return QSTATUS_SUCCESS;
}

QSTATUS qs::ClusterStorage::compact(unsigned int nOffset,
	unsigned int nLength, ClusterStorage* pmsOld, unsigned int* pnOffset)
{
	assert(pnOffset);
	
	*pnOffset = 0xffffffff;
	
	DECLARE_QSTATUS();
	
	status = pImpl_->reopen();
	CHECK_QSTATUS();
	
	unsigned int nOffsetNew = 0;
	status = pImpl_->getFreeOffset(nLength,
		pmsOld ? 0xffffffff : nOffset, &nOffsetNew);
	CHECK_QSTATUS();
	if (nOffsetNew != 0xffffffff) {
		size_t nLen = nLength;
		nLen += ClusterStorageImpl::CLUSTER_SIZE -
			nLen%ClusterStorageImpl::CLUSTER_SIZE;
		assert(nLen%ClusterStorageImpl::CLUSTER_SIZE == 0);
		
		malloc_ptr<unsigned char> p(
			static_cast<unsigned char*>(malloc(nLen + 1)));
		File* pFile = pmsOld ? pmsOld->pImpl_->pFile_ : pImpl_->pFile_;
		status = pImpl_->pFile_->setPosition(
			nOffset*ClusterStorageImpl::CLUSTER_SIZE,
			File::SEEKORIGIN_BEGIN);
		CHECK_QSTATUS();
		size_t nRead = 0;
		status = pImpl_->pFile_->read(p.get(), nLen, &nRead);
		CHECK_QSTATUS();
		if (nRead != nLen)
			return QSTATUS_FAIL;
		*(p.get() + nLen) = '\0';
		status = pImpl_->pFile_->setPosition(
			nOffsetNew*ClusterStorageImpl::CLUSTER_SIZE,
			File::SEEKORIGIN_BEGIN);
		CHECK_QSTATUS();
		status = pImpl_->pFile_->write(p.get(), nLen);
		CHECK_QSTATUS();
		status = pImpl_->pFile_->flush();
		if (!pmsOld)
			free(nOffset, nLength);
	}
	else {
		nOffsetNew = nOffset;
	}
	*pnOffset = nOffsetNew;
	
	return QSTATUS_SUCCESS;
}

QSTATUS qs::ClusterStorage::freeUnrefered(const ReferList& listRefer)
{
	DECLARE_QSTATUS();
	
	status = pImpl_->reopen();
	CHECK_QSTATUS();
	
	ClusterStorageImpl::Map m;
	status = STLWrapper<ClusterStorageImpl::Map>(m).resize(pImpl_->map_.size());
	CHECK_QSTATUS();
	
	ReferList::const_iterator it = listRefer.begin();
	while (it != listRefer.end()) {
		unsigned int nOffset = (*it).nOffset_;
		unsigned int nLength = (*it).nLength_ +
			(ClusterStorageImpl::CLUSTER_SIZE -
				(*it).nLength_%ClusterStorageImpl::CLUSTER_SIZE);
		assert(nLength%ClusterStorageImpl::CLUSTER_SIZE == 0);
		nLength /= ClusterStorageImpl::CLUSTER_SIZE;
		
		unsigned int nBegin = nOffset/ClusterStorageImpl::BYTE_SIZE;
		unsigned int nBeginBit = nOffset%ClusterStorageImpl::BYTE_SIZE;
		unsigned int nEnd = (nOffset + nLength)/ClusterStorageImpl::BYTE_SIZE;
		unsigned int nEndBit = (nOffset + nLength)%ClusterStorageImpl::BYTE_SIZE;
		if (nEndBit == 0) {
			--nEnd;
			nEndBit = ClusterStorageImpl::BYTE_SIZE;
		}
		assert(m.size() > nEnd);
		for (unsigned int n = nBegin; n <= nEnd; ++n) {
			BYTE bMask = ~0;
			unsigned int nFirstBit = n == nBegin ? nBeginBit : 0;
			unsigned int nLastBit = n == nEnd ? nEndBit : ClusterStorageImpl::BYTE_SIZE;
			if (nFirstBit != 0) {
				BYTE b = 1;
				for (unsigned int m = 0; m < nFirstBit; ++m, b <<= 1)
					bMask &= ~b;
			}
			if (nLastBit != ClusterStorageImpl::BYTE_SIZE) {
				BYTE b = 1 << nLastBit;
				for (unsigned int m = nLastBit; m < ClusterStorageImpl::BYTE_SIZE; ++m, b <<= 1)
					bMask &= ~b;
			}
			m[n] |= bMask;
		}
		
		++it;
	}
	std::copy(m.begin(), m.end(), pImpl_->map_.begin());
	std::fill(pImpl_->searchBegin_.begin(), pImpl_->searchBegin_.end(), 0);
	
	return QSTATUS_SUCCESS;
}

QSTATUS qs::ClusterStorage::freeUnused()
{
	DECLARE_QSTATUS();
	
	status = pImpl_->reopen();
	CHECK_QSTATUS();
	
	ClusterStorageImpl::Map& m = pImpl_->map_;
	ClusterStorageImpl::Map::reverse_iterator it = m.rbegin();
	while (it != m.rend() && *it == 0)
		++it;
	m.erase(it.base(), m.end());
	status = pImpl_->pFile_->setPosition(m.size()*
		ClusterStorageImpl::BYTE_SIZE*ClusterStorageImpl::CLUSTER_SIZE,
		File::SEEKORIGIN_BEGIN);
	CHECK_QSTATUS();
	status = pImpl_->pFile_->setEndOfFile();
	CHECK_QSTATUS();
	
	return QSTATUS_SUCCESS;
}
