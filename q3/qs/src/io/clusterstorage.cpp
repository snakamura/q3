/*
 * $Id$
 *
 * Copyright(C) 1998-2004 Satoshi Nakamura
 * All rights reserved.
 *
 */

#include <qsclusterstorage.h>
#include <qsconv.h>
#include <qsfile.h>
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
	
	bool loadMap();
	bool saveMap() const;
	bool adjustMapSize();
	bool reopen();
	unsigned int getFreeOffset(unsigned int nSize);
	unsigned int getFreeOffset(unsigned int nSize,
							   unsigned int nCurrentOffset);
	Map::size_type getSearchBegin(unsigned int nSize) const;
	void setSearchBegin(unsigned int nSize,
						Map::size_type nBegin,
						bool bFree);
	
	wstring_ptr wstrPath_;
	wstring_ptr wstrBoxExt_;
	wstring_ptr wstrMapExt_;
	size_t nBlockSize_;
	std::auto_ptr<File> pFile_;
	Map map_;
	SearchBegin searchBegin_;
	mutable bool bModified_;
};

bool qs::ClusterStorageImpl::loadMap()
{
	wstring_ptr wstrPath(concat(wstrPath_.get(), wstrMapExt_.get()));
	
	W2T(wstrPath.get(), ptszPath);
	if (::GetFileAttributes(ptszPath) != 0xffffffff) {
		FileInputStream fileStream(wstrPath.get());
		if (!fileStream)
			return false;
		
		BufferedInputStream stream(&fileStream, false);
		
		while (true) {
			unsigned char c = 0;
			size_t nRead = stream.read(&c, sizeof(c));
			if (nRead == -1)
				return false;
			else if (nRead == 0)
				break;
			map_.push_back(c);
		}
	}
	
	return true;
}

bool qs::ClusterStorageImpl::saveMap() const
{
	if (!bModified_)
		return true;
	
	wstring_ptr wstrPath(concat(wstrPath_.get(), wstrMapExt_.get()));
	
	TemporaryFileRenamer renamer(wstrPath.get());
	
	FileOutputStream fileStream(renamer.getPath());
	if (!fileStream)
		return false;
	BufferedOutputStream stream(&fileStream, false);
	
	for (Map::const_iterator it = map_.begin(); it != map_.end(); ++it) {
		unsigned char c = *it;
		if (stream.write(&c, sizeof(c)) != sizeof(c))
			return false;
	}
	
	if (!stream.close())
		return false;
	
	if (!renamer.rename())
		return false;
	
	bModified_ = false;
	
	return true;
}

bool qs::ClusterStorageImpl::adjustMapSize()
{
	if (!reopen())
		return false;
	
	const size_t nBlockSize = CLUSTER_SIZE*BYTE_SIZE;
	size_t nSize = pFile_->getSize();
	size_t nBlock = nSize/nBlockSize + (nSize%nBlockSize == 0 ? 0 : 1);
	if (map_.size() < nBlock) {
		map_.resize(nBlock, 0xff);
		bModified_ = true;
	}
	
	return true;
}

bool qs::ClusterStorageImpl::reopen()
{
	if (pFile_.get())
		return true;
	
	wstring_ptr wstrPath(concat(wstrPath_.get(), wstrBoxExt_.get()));
	
	if (nBlockSize_ == static_cast<size_t>(0xffffffff)) {
		std::auto_ptr<BinaryFile> pFile(new BinaryFile(wstrPath.get(),
			BinaryFile::MODE_READ | BinaryFile::MODE_WRITE, 256));
		if (!*pFile.get())
			return false;
		pFile_.reset(pFile.release());
	}
	else {
		std::auto_ptr<DividedFile> pFile(new DividedFile(wstrPath.get(),
			nBlockSize_, BinaryFile::MODE_READ | BinaryFile::MODE_WRITE, 256));
		pFile_.reset(pFile.release());
	}
	
	return true;
}

unsigned int qs::ClusterStorageImpl::getFreeOffset(unsigned int nSize)
{
	return getFreeOffset(nSize, 0xffffffff);
}

unsigned int qs::ClusterStorageImpl::getFreeOffset(unsigned int nSize,
												   unsigned int nCurrentOffset)
{
	bModified_ = true;
	
	nSize = nSize + (CLUSTER_SIZE - nSize%CLUSTER_SIZE);
	assert(nSize%CLUSTER_SIZE == 0);
	nSize /= CLUSTER_SIZE;
	
	unsigned int nCurrentBlock = nCurrentOffset != 0xffffffff ?
		nCurrentOffset / BYTE_SIZE : 0xffffffff;
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
		if (static_cast<unsigned int>(it - map_.begin()) >= nCurrentBlock)
			return nCurrentOffset;
		
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
		for (unsigned int n = 0; n < (nSize - nFindSize)/BYTE_SIZE; ++n)
			map_.push_back(0xff);
		if ((nSize - nFindSize)%BYTE_SIZE != 0) {
			BYTE bData = 0;
			BYTE b = 1;
			for (unsigned int m = 0; m < (nSize - nFindSize)%BYTE_SIZE; ++m, b <<= 1)
				bData |= b;
			map_.push_back(bData);
		}
		if (pFile_->setPosition(map_.size()*BYTE_SIZE*CLUSTER_SIZE, File::SEEKORIGIN_BEGIN) == -1)
			return -1;
		if (!pFile_->setEndOfFile())
			return -1;
		setSearchBegin(nSize, 0xffffffff, false);
	}
	else {
		setSearchBegin(nSize, nBegin, false);
	}
	
	return nBegin*BYTE_SIZE + nBeginBit;
}

ClusterStorageImpl::Map::size_type qs::ClusterStorageImpl::getSearchBegin(unsigned int nSize) const
{
	assert(searchBegin_.size() == SEARCHBEGIN_SIZE);
	
	if (nSize >= searchBegin_.size())
		nSize = searchBegin_.size() - 1;
	SearchBegin::size_type n = nSize;
	while (n > 0 && searchBegin_[n] == 0)
		--n;
	return searchBegin_[n];
}

void qs::ClusterStorageImpl::setSearchBegin(unsigned int nSize,
											Map::size_type nBegin,
											bool bFree)
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

qs::ClusterStorage::ClusterStorage(const Init& init)
{
	wstring_ptr wstrPath(concat(init.pwszPath_, L"\\", init.pwszName_));
	wstring_ptr wstrBoxExt(allocWString(init.pwszBoxExt_));
	wstring_ptr wstrMapExt(allocWString(init.pwszMapExt_));
	ClusterStorageImpl::SearchBegin searchBegin(
		ClusterStorageImpl::SEARCHBEGIN_SIZE);
	
	std::auto_ptr<ClusterStorageImpl> pImpl(new ClusterStorageImpl());
	pImpl->wstrPath_ = wstrPath;
	pImpl->wstrBoxExt_ = wstrBoxExt;
	pImpl->wstrMapExt_ = wstrMapExt;
	pImpl->nBlockSize_ = init.nBlockSize_;
	pImpl->searchBegin_.swap(searchBegin);
	pImpl->bModified_ = false;
	
	if (!pImpl->loadMap() || !pImpl->adjustMapSize())
		return;
	
	pImpl_ = pImpl.release();
}

qs::ClusterStorage::~ClusterStorage()
{
	delete pImpl_;
	pImpl_ = 0;
}

bool qs::ClusterStorage::operator!() const
{
	return pImpl_ == 0;
}

bool qs::ClusterStorage::close()
{
	if (!flush())
		return false;
	
	if (pImpl_->pFile_.get()) {
		/// TODO
		if (!pImpl_->pFile_->close())
			return false;
		pImpl_->pFile_.reset(0);
	}
	
	return true;
}

bool qs::ClusterStorage::flush()
{
	if (!pImpl_->saveMap())
		return false;
	
	if (pImpl_->pFile_.get()) {
		if (!pImpl_->pFile_->flush())
			return false;
	}
	
	return true;
}

unsigned int qs::ClusterStorage::load(unsigned char* p,
									  unsigned int nOffset,
									  unsigned int nLength)
{
	assert(p);
	assert(nLength != 0);
	
	if (!pImpl_->reopen())
		return -1;
	
	if (pImpl_->pFile_->setPosition(
		nOffset*ClusterStorageImpl::CLUSTER_SIZE,
		File::SEEKORIGIN_BEGIN) == -1)
		return -1;
	
	return pImpl_->pFile_->read(p, nLength);
}

unsigned int qs::ClusterStorage::save(const unsigned char* p[],
									  unsigned int nLength[],
									  size_t nCount)
{
	assert(p);
	
	size_t n = 0;
	
	if (!pImpl_->reopen())
		return -1;
	
	unsigned int nAllLength = std::accumulate(&nLength[0], &nLength[nCount], 0);
	unsigned int nOffset = pImpl_->getFreeOffset(nAllLength);
	if (nOffset == -1)
		return -1;
	
	struct Free
	{
		Free(ClusterStorage* pms,
			 unsigned int nOffset,
			 unsigned int nLength) :
			pms_(pms),
			nOffset_(nOffset),
			nLength_(nLength)
		{
		}
		
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
	
	File* pFile = pImpl_->pFile_.get();
	
	if (pFile->setPosition(nOffset*ClusterStorageImpl::CLUSTER_SIZE,
		File::SEEKORIGIN_BEGIN) == -1)
		return -1;
	
	for (n = 0; n < nCount; ++n) {
		if (pFile->write(p[n], nLength[n]) == -1)
			return -1;
	}
	if (!pFile->flush())
		return -1;
	
	free.release();
	
	return nOffset;
}

bool qs::ClusterStorage::free(unsigned int nOffset,
							  unsigned int nLength)
{
	if (!pImpl_->reopen())
		return false;
	
	pImpl_->bModified_ = true;
	
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
	
	return true;
}

unsigned int qs::ClusterStorage::compact(unsigned int nOffset,
										 unsigned int nLength,
										 ClusterStorage* pmsOld)
{
	if (!pImpl_->reopen())
		return -1;
	
	unsigned int nOffsetNew = pImpl_->getFreeOffset(
		nLength, pmsOld ? 0xffffffff : nOffset);
	if (nOffsetNew == -1)
		return -1;
	if (nOffsetNew != nOffset) {
		size_t nLen = nLength;
		nLen += ClusterStorageImpl::CLUSTER_SIZE -
			nLen%ClusterStorageImpl::CLUSTER_SIZE;
		assert(nLen%ClusterStorageImpl::CLUSTER_SIZE == 0);
		
		malloc_ptr<unsigned char> p(
			static_cast<unsigned char*>(malloc(nLen + 1)));
		if (!p.get())
			return -1;
		File* pFile = pmsOld ? pmsOld->pImpl_->pFile_.get() : pImpl_->pFile_.get();
		if (pImpl_->pFile_->setPosition(
			nOffset*ClusterStorageImpl::CLUSTER_SIZE,
			File::SEEKORIGIN_BEGIN) == -1)
			return -1;
		size_t nRead = pImpl_->pFile_->read(p.get(), nLen);
		if (nRead != nLen)
			return -1;
		*(p.get() + nLen) = '\0';
		if (pFile->setPosition(nOffsetNew*ClusterStorageImpl::CLUSTER_SIZE,
			File::SEEKORIGIN_BEGIN) == -1)
			return -1;
		if (pFile->write(p.get(), nLen) != nLen)
			return -1;
		if (!pImpl_->pFile_->flush())
			return false;
		if (!pmsOld)
			free(nOffset, nLength);
	}
	
	return nOffsetNew;
}

bool qs::ClusterStorage::freeUnrefered(const ReferList& listRefer)
{
	pImpl_->bModified_ = true;
	
	ClusterStorageImpl::Map m;
	m.resize(pImpl_->map_.size());
	
	for (ReferList::const_iterator it = listRefer.begin(); it != listRefer.end(); ++it) {
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
			assert((m[n] & bMask) == 0);
			m[n] |= bMask;
		}
	}
	std::copy(m.begin(), m.end(), pImpl_->map_.begin());
	std::fill(pImpl_->searchBegin_.begin(), pImpl_->searchBegin_.end(), 0);
	
	return true;
}

bool qs::ClusterStorage::freeUnused()
{
	if (!pImpl_->reopen())
		return false;
	
	pImpl_->bModified_ = true;
	
	ClusterStorageImpl::Map& m = pImpl_->map_;
	ClusterStorageImpl::Map::reverse_iterator it = m.rbegin();
	while (it != m.rend() && *it == 0)
		++it;
	m.erase(it.base(), m.end());
	if (pImpl_->pFile_->setPosition(m.size()*
		ClusterStorageImpl::BYTE_SIZE*ClusterStorageImpl::CLUSTER_SIZE,
		File::SEEKORIGIN_BEGIN) == -1)
		return false;
	if (!pImpl_->pFile_->setEndOfFile())
		return false;
	
	return true;
}
