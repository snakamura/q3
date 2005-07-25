/*
 * $Id$
 *
 * Copyright(C) 1998-2005 Satoshi Nakamura
 * All rights reserved.
 *
 */

#include <qsclusterstorage.h>
#include <qsconv.h>
#include <qsfile.h>
#include <qsosutil.h>
#include <qsstl.h>
#include <qsstream.h>

#include <numeric>
#include <vector>

#include <tchar.h>

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
	bool saveMap();
	bool adjustMapSize();
	bool reopen();
	size_t getFreeOffset(size_t nSize);
	size_t getFreeOffset(size_t nSize,
						 size_t nCurrentOffset);
	Map::size_type getSearchBegin(size_t nSize) const;
	void setSearchBegin(size_t nSize,
						Map::size_type nBegin,
						bool bFree);
	void clearSearchBegin();
#ifndef NDEBUG
	bool checkWorkMap() const;
#endif
	
	wstring_ptr wstrPath_;
	wstring_ptr wstrBoxExt_;
	wstring_ptr wstrMapExt_;
	size_t nBlockSize_;
	std::auto_ptr<File> pFile_;
	Map map_;
	Map mapWork_;
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
	else {
		bModified_ = true;
	}
	
	mapWork_ = map_;
	
	return true;
}

bool qs::ClusterStorageImpl::saveMap()
{
	if (!bModified_)
		return true;
	
	assert(checkWorkMap());
	
	wstring_ptr wstrPath(concat(wstrPath_.get(), wstrMapExt_.get()));
	
	TemporaryFileRenamer renamer(wstrPath.get());
	
	FileOutputStream fileStream(renamer.getPath());
	if (!fileStream)
		return false;
	BufferedOutputStream stream(&fileStream, false);
	
	for (Map::const_iterator it = mapWork_.begin(); it != mapWork_.end(); ++it) {
		unsigned char c = *it;
		if (stream.write(&c, sizeof(c)) != sizeof(c))
			return false;
	}
	
	if (!stream.close())
		return false;
	
	if (!renamer.rename())
		return false;
	
	map_ = mapWork_;
	clearSearchBegin();
	bModified_ = false;
	
	return true;
}

bool qs::ClusterStorageImpl::adjustMapSize()
{
	if (!reopen())
		return false;
	
	const size_t nBlockSize = CLUSTER_SIZE*BYTE_SIZE;
	size_t nSize = pFile_->getSize();
	if (nSize == -1)
		return false;
	size_t nBlock = nSize/nBlockSize + (nSize%nBlockSize == 0 ? 0 : 1);
	if (map_.size() < nBlock) {
		map_.resize(nBlock, 0xff);
		mapWork_ = map_;
		bModified_ = true;
	}
	
	return true;
}

bool qs::ClusterStorageImpl::reopen()
{
	if (pFile_.get())
		return true;
	
	wstring_ptr wstrPath(concat(wstrPath_.get(), wstrBoxExt_.get()));
	
	if (nBlockSize_ == -1) {
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

size_t qs::ClusterStorageImpl::getFreeOffset(size_t nSize)
{
	return getFreeOffset(nSize, -1);
}

size_t qs::ClusterStorageImpl::getFreeOffset(size_t nSize,
											 size_t nCurrentOffset)
{
	assert(checkWorkMap());
	
	bModified_ = true;
	
	nSize = nSize + (CLUSTER_SIZE - nSize%CLUSTER_SIZE);
	assert(nSize%CLUSTER_SIZE == 0);
	nSize /= CLUSTER_SIZE;
	
	size_t nCurrentBlock = nCurrentOffset != -1 ? nCurrentOffset / BYTE_SIZE : -1;
	size_t nBegin = 0;
	unsigned char nBeginBit = 0;
	size_t nEnd = 0;
	unsigned char nEndBit = 0;
	size_t nFindSize = 0;
	Map::iterator it = map_.begin();
	Map::size_type nSearchBegin = getSearchBegin(nSize);
	if (nSearchBegin == -1)
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
			for (unsigned char n = 0; n < BYTE_SIZE && nFindSize < nSize; ++n, b <<= 1) {
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
		for (size_t n = nBegin; n <= nEnd; ++n) {
			BYTE bMask = 0;
			unsigned char nFirstBit = n == nBegin ? nBeginBit : 0;
			unsigned char nLastBit = n == nEnd ? nEndBit : BYTE_SIZE;
			if (nFirstBit == 0 && nLastBit == BYTE_SIZE) {
				bMask = 0xff;
			}
			else {
				BYTE b = 1;
				b <<= nFirstBit;
				for (unsigned char m = nFirstBit; m < nLastBit; ++m, b <<= 1)
					bMask |= b;
			}
			map_[n] |= bMask;
			mapWork_[n] |= bMask;
		}
	}
	else {
		nBegin = map_.size();
		nBeginBit = 0;
	}
	if (nFindSize < nSize) {
		for (size_t n = 0; n < (nSize - nFindSize)/BYTE_SIZE; ++n) {
			map_.push_back(0xff);
			mapWork_.push_back(0xff);
		}
		if ((nSize - nFindSize)%BYTE_SIZE != 0) {
			BYTE bData = 0;
			BYTE b = 1;
			for (size_t m = 0; m < (nSize - nFindSize)%BYTE_SIZE; ++m, b <<= 1)
				bData |= b;
			map_.push_back(bData);
			mapWork_.push_back(bData);
		}
		if (pFile_->setPosition(map_.size()*BYTE_SIZE*CLUSTER_SIZE, File::SEEKORIGIN_BEGIN) == -1)
			return -1;
		if (!pFile_->setEndOfFile())
			return -1;
		setSearchBegin(nSize, -1, false);
	}
	else {
		setSearchBegin(nSize, nBegin, false);
	}
	
	return nBegin*BYTE_SIZE + nBeginBit;
}

ClusterStorageImpl::Map::size_type qs::ClusterStorageImpl::getSearchBegin(size_t nSize) const
{
	assert(searchBegin_.size() == SEARCHBEGIN_SIZE);
	
	if (nSize >= searchBegin_.size())
		nSize = searchBegin_.size() - 1;
	SearchBegin::size_type n = nSize;
	while (n > 0 && searchBegin_[n] == 0)
		--n;
	return searchBegin_[n];
}

void qs::ClusterStorageImpl::setSearchBegin(size_t nSize,
											Map::size_type nBegin,
											bool bFree)
{
	assert(searchBegin_.size() == SEARCHBEGIN_SIZE);
	
	if (nSize >= searchBegin_.size())
		nSize = searchBegin_.size() - 1;
	
	if (nBegin == -1) {
		for (size_t n = nSize + 1; n < searchBegin_.size(); ++n) {
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

void qs::ClusterStorageImpl::clearSearchBegin()
{
	std::fill(searchBegin_.begin(), searchBegin_.end(), 0);
}

#ifndef NDEBUG
bool qs::ClusterStorageImpl::checkWorkMap() const
{
	assert(map_.size() == mapWork_.size());
	for (Map::size_type n = 0; n < map_.size(); ++n)
		assert(map_[n] >= mapWork_[n]);
	return true;
}
#endif


/****************************************************************************
 *
 * ClusterStorage
 *
 */

qs::ClusterStorage::ClusterStorage(const WCHAR* pwszPath,
								   const WCHAR* pwszName,
								   const WCHAR* pwszBoxExt,
								   const WCHAR* pwszMapExt,
								   size_t nBlockSize)
{
	wstring_ptr wstrPath(concat(pwszPath, L"\\", pwszName));
	wstring_ptr wstrBoxExt(allocWString(pwszBoxExt));
	wstring_ptr wstrMapExt(allocWString(pwszMapExt));
	ClusterStorageImpl::SearchBegin searchBegin(
		ClusterStorageImpl::SEARCHBEGIN_SIZE);
	
	std::auto_ptr<ClusterStorageImpl> pImpl(new ClusterStorageImpl());
	pImpl->wstrPath_ = wstrPath;
	pImpl->wstrBoxExt_ = wstrBoxExt;
	pImpl->wstrMapExt_ = wstrMapExt;
	pImpl->nBlockSize_ = nBlockSize;
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

bool qs::ClusterStorage::rename(const WCHAR* pwszPath,
								const WCHAR* pwszName)
{
	if (!close())
		return false;
	
	wstring_ptr wstrPath(concat(pwszPath, L"\\", pwszName));
	
	const WCHAR* pwszExt[] = {
		pImpl_->wstrBoxExt_.get(),
		pImpl_->wstrMapExt_.get()
	};
	if (pImpl_->nBlockSize_ != -1) {
		{
			wstring_ptr wstrFind(concat(wstrPath.get(), L"*", pImpl_->wstrBoxExt_.get()));
			W2T(wstrFind.get(), ptszFind);
			WIN32_FIND_DATA fd;
			AutoFindHandle hFind(::FindFirstFile(ptszFind, &fd));
			if (hFind.get()) {
				W2T(pwszPath, ptszPath);
				do {
					tstring_ptr tstrDeletePath(concat(ptszPath, _T("\\"), fd.cFileName));
					if (!::DeleteFile(tstrDeletePath.get()))
						return false;
				} while (::FindNextFile(hFind.get(), &fd));
			}
		}
		for (int n = 0; ; ++n) {
			WCHAR wsz[16];
			swprintf(wsz, L"%03u", n);
			wstring_ptr wstrOldPath(concat(pImpl_->wstrPath_.get(), wsz, pImpl_->wstrBoxExt_.get()));
			W2T(wstrOldPath.get(), ptszOldPath);
			if (::GetFileAttributes(ptszOldPath) == 0xffffffff)
				break;
			
			wstring_ptr wstrNewPath(concat(wstrPath.get(), wsz, pImpl_->wstrBoxExt_.get()));
			W2T(wstrNewPath.get(), ptszNewPath);
			
			if (!::MoveFile(ptszOldPath, ptszNewPath))
				return false;
		}
		
		pwszExt[0] = 0;
	}
	for (int n = 0; n < countof(pwszExt); ++n) {
		if (pwszExt[n]) {
			wstring_ptr wstrOldPath(concat(pImpl_->wstrPath_.get(), pwszExt[n]));
			wstring_ptr wstrNewPath(concat(wstrPath.get(), pwszExt[n]));
			
			W2T(wstrOldPath.get(), ptszOldPath);
			W2T(wstrNewPath.get(), ptszNewPath);
			
			if (!::DeleteFile(ptszNewPath) || !::MoveFile(ptszOldPath, ptszNewPath))
				return false;
		}
	}
	
	pImpl_->wstrPath_ = wstrPath;
	
	return true;
}

size_t qs::ClusterStorage::load(unsigned char* p,
								size_t nOffset,
								size_t nLength)
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

size_t qs::ClusterStorage::save(const unsigned char* p[],
								size_t nLength[],
								size_t nCount)
{
	assert(p);
	
	if (!pImpl_->reopen())
		return -1;
	
	size_t nAllLength = std::accumulate(&nLength[0],
		&nLength[nCount], static_cast<size_t>(0));
	size_t nOffset = pImpl_->getFreeOffset(nAllLength);
	if (nOffset == -1)
		return -1;
	
	struct Free
	{
		Free(ClusterStorage* pcs,
			 size_t nOffset,
			 size_t nLength) :
			pcs_(pcs),
			nOffset_(nOffset),
			nLength_(nLength)
		{
		}
		
		~Free()
		{
			if (pcs_)
				pcs_->free(nOffset_, nLength_);
		}
		
		void release() { pcs_ = 0; }
		
		ClusterStorage* pcs_;
		size_t nOffset_;
		size_t nLength_;
	} free(this, nOffset, nAllLength);
	
	File* pFile = pImpl_->pFile_.get();
	
	if (pFile->setPosition(nOffset*ClusterStorageImpl::CLUSTER_SIZE,
		File::SEEKORIGIN_BEGIN) == -1)
		return -1;
	
	for (size_t n = 0; n < nCount; ++n) {
		if (pFile->write(p[n], nLength[n]) == -1)
			return -1;
	}
	if (!pFile->flush())
		return -1;
	
	free.release();
	
	return nOffset;
}

bool qs::ClusterStorage::free(size_t nOffset,
							  size_t nLength)
{
	assert(pImpl_->checkWorkMap());
	
	pImpl_->bModified_ = true;
	
	size_t nBlock = nLength + (ClusterStorageImpl::CLUSTER_SIZE -
		nLength%ClusterStorageImpl::CLUSTER_SIZE);
	assert(nBlock%ClusterStorageImpl::CLUSTER_SIZE == 0);
	nBlock /= ClusterStorageImpl::CLUSTER_SIZE;
	
	size_t nBegin = nOffset/ClusterStorageImpl::BYTE_SIZE;
	unsigned char nBeginBit = static_cast<unsigned char>(
		nOffset%ClusterStorageImpl::BYTE_SIZE);
	size_t nEnd = (nOffset + nBlock)/ClusterStorageImpl::BYTE_SIZE;
	unsigned char nEndBit = static_cast<unsigned char>(
		(nOffset + nBlock)%ClusterStorageImpl::BYTE_SIZE);
	if (nEndBit == 0) {
		--nEnd;
		nEndBit = ClusterStorageImpl::BYTE_SIZE;
	}
	assert(pImpl_->mapWork_.size() > nEnd);
	for (size_t n = nBegin; n <= nEnd; ++n) {
		BYTE bMask = 0;
		unsigned char nFirstBit = n == nBegin ? nBeginBit : 0;
		unsigned char nLastBit = n == nEnd ? nEndBit : ClusterStorageImpl::BYTE_SIZE;
		if (nFirstBit != 0) {
			BYTE b = 1;
			for (unsigned char m = 0; m < nFirstBit; ++m, b <<= 1)
				bMask |= b;
		}
		if (nLastBit != ClusterStorageImpl::BYTE_SIZE) {
			BYTE b = 1 << nLastBit;
			for (unsigned char m = nLastBit; m < ClusterStorageImpl::BYTE_SIZE; ++m, b <<= 1)
				bMask |= b;
		}
		pImpl_->mapWork_[n] &= bMask;
	}
	
#ifndef NDEBUG
	if (pImpl_->reopen()) {
		File* pFile = pImpl_->pFile_.get();
		if (pFile->setPosition(nOffset*ClusterStorageImpl::CLUSTER_SIZE,
							   File::SEEKORIGIN_BEGIN) != -1) {
			for (size_t n = 0; n < nLength; ++n) {
				unsigned char c = 0;
				if (pFile->write(&c, 1) == -1)
					break;
			}
			pFile->flush();
		}
	}
#endif
	
	return true;
}

size_t qs::ClusterStorage::compact(size_t nOffset,
								   size_t nLength,
								   ClusterStorage* pcsOld)
{
	assert(pImpl_->checkWorkMap());
	
	if (!pImpl_->reopen())
		return -1;
	
	size_t nOffsetNew = pImpl_->getFreeOffset(nLength, pcsOld ? -1 : nOffset);
	if (nOffsetNew == -1)
		return -1;
	if (pcsOld || nOffsetNew != nOffset) {
		size_t nLen = nLength;
		nLen += ClusterStorageImpl::CLUSTER_SIZE -
			nLen%ClusterStorageImpl::CLUSTER_SIZE;
		assert(nLen%ClusterStorageImpl::CLUSTER_SIZE == 0);
		
		malloc_ptr<unsigned char> p(
			static_cast<unsigned char*>(malloc(nLen + 1)));
		if (!p.get())
			return -1;
		File* pFile = pImpl_->pFile_.get();
		if (pcsOld) {
			if (!pcsOld->pImpl_->reopen())
				return false;
			pFile = pcsOld->pImpl_->pFile_.get();
		}
		if (pFile->setPosition(nOffset*ClusterStorageImpl::CLUSTER_SIZE, File::SEEKORIGIN_BEGIN) == -1)
			return -1;
		size_t nRead = pFile->read(p.get(), nLen);
		if (nRead != nLen)
			return -1;
		*(p.get() + nLen) = '\0';
		if (pImpl_->pFile_->setPosition(nOffsetNew*ClusterStorageImpl::CLUSTER_SIZE,
			File::SEEKORIGIN_BEGIN) == -1)
			return -1;
		if (pImpl_->pFile_->write(p.get(), nLen) != nLen)
			return -1;
		if (!pcsOld) {
			if (!pImpl_->pFile_->flush())
				return false;
			free(nOffset, nLength);
		}
	}
	
	return nOffsetNew;
}

void qs::ClusterStorage::freeUnrefered(const ReferList& listRefer)
{
	assert(pImpl_->checkWorkMap());
	
	pImpl_->bModified_ = true;
	
	ClusterStorageImpl::Map m;
	m.resize(pImpl_->mapWork_.size());
	
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
	pImpl_->mapWork_ = m;
}

bool qs::ClusterStorage::freeUnused()
{
	assert(pImpl_->checkWorkMap());
	
	if (!pImpl_->reopen())
		return false;
	
	pImpl_->bModified_ = true;
	
	ClusterStorageImpl::Map& m = pImpl_->map_;
	ClusterStorageImpl::Map::reverse_iterator it = m.rbegin();
	while (it != m.rend() && *it == 0)
		++it;
	m.erase(it.base(), m.end());
#ifndef NDEBUG
	for (ClusterStorageImpl::Map::size_type n = m.size(); n < pImpl_->mapWork_.size(); ++n)
		assert(pImpl_->mapWork_[n] == 0);
#endif
	pImpl_->mapWork_.resize(m.size());
	if (pImpl_->pFile_->setPosition(m.size()*
		ClusterStorageImpl::BYTE_SIZE*ClusterStorageImpl::CLUSTER_SIZE,
		File::SEEKORIGIN_BEGIN) == -1)
		return false;
	if (!pImpl_->pFile_->setEndOfFile())
		return false;
	
	return true;
}
