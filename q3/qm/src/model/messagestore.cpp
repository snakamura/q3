/*
 * $Id$
 *
 * Copyright(C) 1998-2004 Satoshi Nakamura
 * All rights reserved.
 *
 */

#include <qmfilenames.h>

#include <qsconv.h>
#include <qsosutil.h>
#include <qsthread.h>

#include <stdio.h>
#include <tchar.h>

#include "messagestore.h"
#include "messagecache.h"

#pragma warning(disable:4786)

using namespace qm;
using namespace qs;


/****************************************************************************
 *
 * MessageStore
 *
 */

qm::MessageStore::~MessageStore()
{
}


/****************************************************************************
 *
 * SingleMessageStoreImpl
 *
 */

struct qm::SingleMessageStoreImpl
{
	enum {
		SEPARATOR_SIZE	= 9
	};
	
	wstring_ptr wstrPath_;
	wstring_ptr wstrCachePath_;
	unsigned int nCacheBlockSize_;
	std::auto_ptr<ClusterStorage> pStorage_;
	std::auto_ptr<ClusterStorage> pCacheStorage_;
	CriticalSection cs_;
	
	static const unsigned char szUsedSeparator__[];
	static const unsigned char szUnusedSeparator__[];
};

const unsigned char qm::SingleMessageStoreImpl::szUsedSeparator__[] = "\n\nFrom -\n";
const unsigned char qm::SingleMessageStoreImpl::szUnusedSeparator__[] = "\n\nFrom *\n";


/****************************************************************************
 *
 * SingleMessageStore
 *
 */

qm::SingleMessageStore::SingleMessageStore(const WCHAR* pwszPath,
										   unsigned int nBlockSize,
										   const WCHAR* pwszCachePath,
										   unsigned int nCacheBlockSize)
{
	wstring_ptr wstrPath(allocWString(pwszPath));
	wstring_ptr wstrCachePath(allocWString(pwszCachePath));
	
	std::auto_ptr<ClusterStorage> pStorage(new ClusterStorage(pwszPath,
		L"msg", FileNames::BOX_EXT, FileNames::MAP_EXT, nBlockSize));
	std::auto_ptr<ClusterStorage> pCacheStorage(new ClusterStorage(pwszCachePath,
		L"cache", FileNames::BOX_EXT, FileNames::MAP_EXT, nCacheBlockSize));
	
	pImpl_ = new SingleMessageStoreImpl();
	pImpl_->wstrPath_ = wstrPath;
	pImpl_->wstrCachePath_ = wstrCachePath;
	pImpl_->nCacheBlockSize_ = nCacheBlockSize;
	pImpl_->pStorage_ = pStorage;
	pImpl_->pCacheStorage_ = pCacheStorage;
}

qm::SingleMessageStore::~SingleMessageStore()
{
	if (pImpl_) {
		close();
		
		delete pImpl_;
		pImpl_ = 0;
	}
}

bool qm::SingleMessageStore::close()
{
	Lock<CriticalSection> lock(pImpl_->cs_);
	return pImpl_->pStorage_->close() && pImpl_->pCacheStorage_->close();
}

bool qm::SingleMessageStore::flush()
{
	Lock<CriticalSection> lock(pImpl_->cs_);
	return pImpl_->pStorage_->close() && pImpl_->pCacheStorage_->close();
}

bool qm::SingleMessageStore::load(unsigned int nOffset,
								  unsigned int nLength,
								  Message* pMessage)
{
	assert(pMessage);
	
	Lock<CriticalSection> lock(pImpl_->cs_);
	
	unsigned int nLoad = nLength + SingleMessageStoreImpl::SEPARATOR_SIZE;
	malloc_ptr<unsigned char> pBuf(static_cast<unsigned char*>(malloc(nLoad)));
	if (!pBuf.get())
		return false;
	if (pImpl_->pStorage_->load(pBuf.get(), nOffset, nLoad) == -1)
		return false;
	unsigned char* p = pBuf.get() + SingleMessageStoreImpl::SEPARATOR_SIZE;
	
	return pMessage->create(reinterpret_cast<CHAR*>(p), nLength, Message::FLAG_NONE);
}

bool qm::SingleMessageStore::save(const CHAR* pszMessage,
								  const Message& header,
								  MessageCache* pMessageCache,
								  bool bIndexOnly,
								  unsigned int* pnOffset,
								  unsigned int* pnLength,
								  unsigned int* pnHeaderLength,
								  MessageCacheKey* pKey)
{
	assert(pnOffset);
	assert(pnLength);
	assert(pnHeaderLength);
	assert(pKey);
	
	const CHAR* pszHeader = header.getHeader();
	size_t nHeaderLen = strlen(pszHeader);
	const CHAR* pszBody = strstr(pszMessage, "\r\n\r\n");
	pszBody = pszBody ? pszBody + 4 : "";
	size_t nBodyLen = strlen(pszBody);
	
	if (!bIndexOnly) {
		*pnHeaderLength = nHeaderLen;
		*pnLength = nHeaderLen + nBodyLen + 2;
	}
	
	malloc_size_ptr<unsigned char> pData(MessageCache::createData(header));
	if (!pData.get())
		return false;
	
	const unsigned char* pMsg[] = {
		SingleMessageStoreImpl::szUsedSeparator__,
		reinterpret_cast<const unsigned char*>(pszHeader),
		reinterpret_cast<const unsigned char*>("\r\n"),
		reinterpret_cast<const unsigned char*>(pszBody),
		SingleMessageStoreImpl::szUnusedSeparator__
	};
	size_t nMsgLen[] = {
		SingleMessageStoreImpl::SEPARATOR_SIZE,
		nHeaderLen,
		2,
		nBodyLen,
		SingleMessageStoreImpl::SEPARATOR_SIZE
	};
	
	size_t nDataLen = pData.size();
	const unsigned char* pCache[] = {
		reinterpret_cast<const unsigned char*>(&nDataLen),
		pData.get(),
	};
	size_t nCacheLen[] = {
		sizeof(nDataLen),
		nDataLen,
	};
	
	Lock<CriticalSection> lock(pImpl_->cs_);
	if (!bIndexOnly) {
		*pnOffset = pImpl_->pStorage_->save(pMsg, nMsgLen, countof(pMsg));
		if (*pnOffset == -1)
			return false;
	}
	*pKey = pImpl_->pCacheStorage_->save(pCache, nCacheLen, countof(pCache));
	if (*pKey == -1)
		return false;
	
	return true;
}

bool qm::SingleMessageStore::free(unsigned int nOffset,
								  unsigned int nLength,
								  MessageCacheKey key)
{
	Lock<CriticalSection> lock(pImpl_->cs_);
	
	if (key != -1) {
		size_t nDataLen = 0;
		if (pImpl_->pCacheStorage_->load(reinterpret_cast<unsigned char*>(&nDataLen),
			key, sizeof(nDataLen)) != sizeof(nDataLen))
			return false;
		if (!pImpl_->pCacheStorage_->free(key, nDataLen + sizeof(nDataLen)))
			return false;
	}
	
	if (nOffset != -1) {
		if (!pImpl_->pStorage_->free(nOffset,
			nLength + SingleMessageStoreImpl::SEPARATOR_SIZE*2))
			return false;
	}
	
	return true;
}

bool qm::SingleMessageStore::compact(DataList* pListData,
									 MessageOperationCallback* pCallback)
{
	assert(pListData);
	assert(pCallback);
	
	pCallback->setCount(pListData->size());
	pCallback->show();
	
	Lock<CriticalSection> lock(pImpl_->cs_);
	
	std::auto_ptr<ClusterStorage> pCacheStorage(new ClusterStorage(
		pImpl_->wstrCachePath_.get(), L"compact",
		FileNames::BOX_EXT, FileNames::MAP_EXT, pImpl_->nCacheBlockSize_));
	ClusterStorage* pOldCacheStorage = pImpl_->pCacheStorage_.get();
	
	for (DataList::iterator it = pListData->begin(); it != pListData->end(); ++it) {
		Data& data = *it;
		
		size_t nDataLen = 0;
		unsigned int nLoad = pImpl_->pCacheStorage_->load(
			reinterpret_cast<unsigned char*>(&nDataLen), data.key_, sizeof(nDataLen));
		if (nLoad != sizeof(nDataLen))
			return false;
		data.key_ = pCacheStorage->compact(data.key_,
			nDataLen + sizeof(nDataLen), pOldCacheStorage);
		if (data.key_ == -1)
			return false;
		
		if (data.nOffset_ != -1) {
			size_t nLen = data.nLength_ + SingleMessageStoreImpl::SEPARATOR_SIZE*2;
			data.nOffset_ = pImpl_->pStorage_->compact(data.nOffset_, nLen, 0);
			if (data.nOffset_ == -1)
				return false;
		}
		
		pCallback->step(1);
	}
	
	if (!pImpl_->pCacheStorage_->close())
		return false;
	if (!pCacheStorage->rename(pImpl_->wstrCachePath_.get(), L"cache"))
		return false;
	pImpl_->pCacheStorage_ = pCacheStorage;
	
	MessageStoreUtil::freeUnrefered(pImpl_->pStorage_.get(),
		*pListData, SingleMessageStoreImpl::SEPARATOR_SIZE*2);
	
	return true;
}

bool qm::SingleMessageStore::salvage(const DataList& listData,
									 MessageStoreSalvageCallback* pCallback)
{
	// TODO
	return true;
}

bool qm::SingleMessageStore::check(MessageStoreCheckCallback* pCallback)
{
	std::auto_ptr<ClusterStorage> pCacheStorage(MessageStoreUtil::checkCache(
		pImpl_->pCacheStorage_.get(), pImpl_->wstrCachePath_.get(),
		pImpl_->nCacheBlockSize_, pCallback));
	if (!pCacheStorage.get())
		return false;
	
	pImpl_->pCacheStorage_ = pCacheStorage;
	
	return true;
}

bool qm::SingleMessageStore::freeUnused()
{
	return pImpl_->pStorage_->freeUnused() && pImpl_->pStorage_->flush();
}

malloc_ptr<unsigned char> qm::SingleMessageStore::readCache(MessageCacheKey key)
{
	Lock<CriticalSection> lock(pImpl_->cs_);
	
	const size_t nDefaultSize = 1024;
	malloc_ptr<unsigned char> p(static_cast<unsigned char*>(malloc(nDefaultSize)));
	if (!p.get())
		return 0;
	
	unsigned int nLoad = pImpl_->pCacheStorage_->load(p.get(), key, nDefaultSize);
	if (nLoad == -1 || nLoad < sizeof(size_t))
		return 0;
	
	size_t nSize = *reinterpret_cast<size_t*>(p.get());
	if (nSize + sizeof(nSize) > nDefaultSize) {
		unsigned char* p2 = static_cast<unsigned char*>(
			realloc(p.get(), nSize + sizeof(nSize)));
		if (!p2)
			return 0;
		p.release();
		p.reset(p2);
		
		if (pImpl_->pCacheStorage_->load(p.get(), key, nSize + sizeof(nSize)) == -1)
			return 0;
	}
	
	return p;
}


/****************************************************************************
 *
 * MultiMessageStoreImpl
 *
 */

struct qm::MultiMessageStoreImpl
{
public:
	bool init();
	unsigned int getOffset(bool bIncrement);
	wstring_ptr getPath(unsigned int nOffset) const;
	bool ensureDirectory(unsigned int nOffset) const;
	void freeUnrefered(const MessageStore::DataList& listData);

public:
	typedef std::vector<int> DirList;

public:
	wstring_ptr wstrPath_;
	wstring_ptr wstrCachePath_;
	unsigned int nCacheBlockSize_;
	std::auto_ptr<ClusterStorage> pCacheStorage_;
	unsigned int nOffset_;
	CriticalSection cs_;
	mutable DirList listDir_;
};

bool qm::MultiMessageStoreImpl::init()
{
	wstring_ptr wstrPath(concat(wstrPath_.get(), L"\\msg"));
	
	W2T(wstrPath.get(), ptszPath);
	
	DWORD dwAttributes = ::GetFileAttributes(ptszPath);
	if (dwAttributes == 0xffffffff) {
		if (!::CreateDirectory(ptszPath, 0))
			return false;
	}
	else if (!(dwAttributes & FILE_ATTRIBUTE_DIRECTORY)) {
		return false;
	}
	
	return true;
}

unsigned int qm::MultiMessageStoreImpl::getOffset(bool bIncrement)
{
	if (nOffset_ == -1) {
		unsigned int nDir = 0;
		
		wstring_ptr wstrDirFind(concat(wstrPath_.get(), L"\\msg\\*"));
		W2T(wstrDirFind.get(), ptszDirFind);
		
		WIN32_FIND_DATA fdDir;
		AutoFindHandle hFindDir(::FindFirstFile(ptszDirFind, &fdDir));
		if (hFindDir.get()) {
			do {
				if (fdDir.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY &&
					_tcscmp(fdDir.cFileName, _T(".")) != 0 &&
					_tcscmp(fdDir.cFileName, _T("..")) != 0) {
					unsigned int n = 0;
					_stscanf(fdDir.cFileName, _T("%08d"), &n);
					if (n > nDir)
						nDir = n;
				}
			} while (::FindNextFile(hFindDir.get(), &fdDir));
		}
		
		unsigned int nOffset = nDir*1000;
		
		WCHAR wszFind[32];
		swprintf(wszFind, L"\\msg\\%08d\\*.msg", nDir);
		wstring_ptr wstrFind(concat(wstrPath_.get(), wszFind));
		W2T(wstrFind.get(), ptszFind);
		
		WIN32_FIND_DATA fd;
		AutoFindHandle hFind(::FindFirstFile(ptszFind, &fd));
		if (hFind.get()) {
			do {
				unsigned int n = 0;
				_stscanf(fd.cFileName, _T("%08d.msg"), &n);
				if (n > nOffset)
					nOffset = n;
			} while (::FindNextFile(hFind.get(), &fd));
		}
		
		nOffset_ = nOffset;
	}
	
	if (bIncrement)
		++nOffset_;
	return nOffset_;
}

wstring_ptr qm::MultiMessageStoreImpl::getPath(unsigned int nOffset) const
{
	WCHAR wsz[64];
	swprintf(wsz, L"\\msg\\%08d\\%08d.msg", nOffset/1000, nOffset);
	return concat(wstrPath_.get(), wsz);
}

bool qm::MultiMessageStoreImpl::ensureDirectory(unsigned int nOffset) const
{
	unsigned int nIndex = nOffset/1000;
	
	if (nIndex >= listDir_.size() || !listDir_[nIndex]) {
		WCHAR wsz[64];
		swprintf(wsz, L"\\msg\\%08d", nIndex);
		
		wstring_ptr wstrPath(concat(wstrPath_.get(), wsz));
		W2T(wstrPath.get(), ptszPath);
		::CreateDirectory(ptszPath, 0);
		
		if (listDir_.size() <= nIndex)
			listDir_.resize(nIndex + 1);
		listDir_[nIndex] = 1;
	}
	
	return true;
}

void qm::MultiMessageStoreImpl::freeUnrefered(const MessageStore::DataList& listData)
{
	typedef std::vector<unsigned int> List;
	List l;
	l.resize(listData.size());
	std::transform(listData.begin(), listData.end(),
		l.begin(), mem_data_ref(&MessageStore::Data::nOffset_));
	std::sort(l.begin(), l.end());
	
	List::const_iterator it = l.begin();
	unsigned int nOffset = getOffset(false);
	for (unsigned int n = 0; n <= nOffset; ++n) {
		if (it != l.end() && *it == n) {
			++it;
		}
		else {
			wstring_ptr wstrPath(getPath(n));
			W2T(wstrPath.get(), ptszPath);
			if (::GetFileAttributes(ptszPath) != 0xffffffff)
				::DeleteFile(ptszPath);
		}
	}
}


/****************************************************************************
 *
 * MultiMessageStore
 *
 */

qm::MultiMessageStore::MultiMessageStore(const WCHAR* pwszPath,
										 const WCHAR* pwszCachePath,
										 unsigned int nCacheBlockSize)
{
	wstring_ptr wstrPath(allocWString(pwszPath));
	wstring_ptr wstrCachePath(allocWString(pwszCachePath));
	std::auto_ptr<ClusterStorage> pCacheStorage(new ClusterStorage(pwszCachePath,
		L"cache", FileNames::BOX_EXT, FileNames::MAP_EXT, nCacheBlockSize));
	
	pImpl_ = new MultiMessageStoreImpl();
	pImpl_->wstrPath_ = wstrPath;
	pImpl_->wstrCachePath_ = wstrCachePath;
	pImpl_->nCacheBlockSize_ = nCacheBlockSize;
	pImpl_->pCacheStorage_ = pCacheStorage;
	pImpl_->nOffset_ = -1;
	
	if (!pImpl_->init()) {
		// TODO
	}
}

qm::MultiMessageStore::~MultiMessageStore()
{
	if (pImpl_) {
		close();
		
		delete pImpl_;
		pImpl_ = 0;
	}
}

bool qm::MultiMessageStore::close()
{
	Lock<CriticalSection> lock(pImpl_->cs_);
	return pImpl_->pCacheStorage_->close();
}

bool qm::MultiMessageStore::flush()
{
	Lock<CriticalSection> lock(pImpl_->cs_);
	return pImpl_->pCacheStorage_->close();
}

bool qm::MultiMessageStore::load(unsigned int nOffset,
								 unsigned int nLength,
								 Message* pMessage)
{
	assert(pMessage);
	
	Lock<CriticalSection> lock(pImpl_->cs_);
	
	wstring_ptr wstrPath(pImpl_->getPath(nOffset));
	FileInputStream stream(wstrPath.get());
	if (!stream)
		return false;
	BufferedInputStream bufferedStream(&stream, false);
	
	malloc_ptr<unsigned char> pBuf(static_cast<unsigned char*>(malloc(nLength)));
	if (!pBuf.get())
		return false;
	
	size_t nRead = bufferedStream.read(pBuf.get(), nLength);
	if (nRead != nLength)
		return false;
	
	return pMessage->create(reinterpret_cast<CHAR*>(pBuf.get()), nRead, Message::FLAG_NONE);
}

bool qm::MultiMessageStore::save(const CHAR* pszMessage,
								 const Message& header,
								 MessageCache* pMessageCache,
								 bool bIndexOnly,
								 unsigned int* pnOffset,
								 unsigned int* pnLength,
								 unsigned int* pnHeaderLength,
								 MessageCacheKey* pKey)
{
	assert(pnOffset);
	assert(pnLength);
	assert(pnHeaderLength);
	assert(pKey);
	
	const CHAR* pszHeader = header.getHeader();
	size_t nHeaderLen = strlen(pszHeader);
	const CHAR* pszBody = strstr(pszMessage, "\r\n\r\n");
	pszBody = pszBody ? pszBody + 4 : "";
	size_t nBodyLen = strlen(pszBody);
	
	if (!bIndexOnly) {
		*pnHeaderLength = nHeaderLen;
		*pnLength = nHeaderLen + nBodyLen + 2;
	}
	
	malloc_size_ptr<unsigned char> pData(MessageCache::createData(header));
	if (!pData.get())
		return false;
	
	size_t nDataLen = pData.size();
	const unsigned char* pCache[] = {
		reinterpret_cast<const unsigned char*>(&nDataLen),
		pData.get(),
	};
	size_t nCacheLen[] = {
		sizeof(nDataLen),
		nDataLen,
	};
	
	Lock<CriticalSection> lock(pImpl_->cs_);
	
	if (!bIndexOnly) {
		*pnOffset = pImpl_->getOffset(true);
		if (!pImpl_->ensureDirectory(*pnOffset))
			return false;
		wstring_ptr wstrPath(pImpl_->getPath(*pnOffset));
		FileOutputStream stream(wstrPath.get());
		if (!stream)
			return false;
		BufferedOutputStream bufferedStream(&stream, false);
		if (bufferedStream.write(reinterpret_cast<const unsigned char*>(pszHeader), nHeaderLen) == -1 ||
			bufferedStream.write(reinterpret_cast<const unsigned char*>("\r\n"), 2) == -1 ||
			bufferedStream.write(reinterpret_cast<const unsigned char*>(pszBody), nBodyLen) == -1 ||
			!bufferedStream.close())
			return false;
	}
	*pKey = pImpl_->pCacheStorage_->save(pCache, nCacheLen, countof(pCache));
	if (*pKey == -1)
		return false;
	
	return true;
}

bool qm::MultiMessageStore::free(unsigned int nOffset,
								 unsigned int nLength,
								 MessageCacheKey key)
{
	Lock<CriticalSection> lock(pImpl_->cs_);
	
	if (key != -1) {
		size_t nDataLen = 0;
		unsigned int nLoad = pImpl_->pCacheStorage_->load(
			reinterpret_cast<unsigned char*>(&nDataLen), key, sizeof(nDataLen));
		if (nLoad != sizeof(nDataLen))
			return false;
		if (!pImpl_->pCacheStorage_->free(key, nDataLen + sizeof(nDataLen)))
			return false;
	}
	
	if (nOffset != -1) {
		wstring_ptr wstrPath(pImpl_->getPath(nOffset));
		W2T(wstrPath.get(), ptszPath);
		::DeleteFile(ptszPath);
	}
	
	return true;
}

bool qm::MultiMessageStore::compact(DataList* pListData,
									MessageOperationCallback* pCallback)
{
	assert(pListData);
	assert(pCallback);
	
	pCallback->setCount(pListData->size());
	pCallback->show();
	
	Lock<CriticalSection> lock(pImpl_->cs_);
	
	std::auto_ptr<ClusterStorage> pCacheStorage(new ClusterStorage(
		pImpl_->wstrCachePath_.get(), L"compact",
		FileNames::BOX_EXT, FileNames::MAP_EXT, pImpl_->nCacheBlockSize_));
	ClusterStorage* pOldCacheStorage = pImpl_->pCacheStorage_.get();
	
	for (DataList::iterator it = pListData->begin(); it != pListData->end(); ++it) {
		Data& data = *it;
		
		size_t nDataLen = 0;
		unsigned int nLoad = pImpl_->pCacheStorage_->load(
			reinterpret_cast<unsigned char*>(&nDataLen), data.key_, sizeof(nDataLen));
		if (nLoad != sizeof(nDataLen))
			return false;
		data.key_ = pCacheStorage->compact(data.key_,
			nDataLen + sizeof(nDataLen), pOldCacheStorage);
		if (data.key_ == -1)
			return false;
		
		pCallback->step(1);
	}
	
	if (!pImpl_->pCacheStorage_->close())
		return false;
	if (!pCacheStorage->rename(pImpl_->wstrCachePath_.get(), L"cache"))
		return false;
	pImpl_->pCacheStorage_ = pCacheStorage;
	
	pImpl_->freeUnrefered(*pListData);
	
	return true;
}

bool qm::MultiMessageStore::freeUnused()
{
	return true;
}

bool qm::MultiMessageStore::salvage(const DataList& listData,
									MessageStoreSalvageCallback* pCallback)
{
	typedef std::vector<unsigned int> List;
	List l;
	l.resize(listData.size());
	std::transform(listData.begin(), listData.end(),
		l.begin(), mem_data_ref(&Data::nOffset_));
	std::sort(l.begin(), l.end());
	
	List::const_iterator it = l.begin();
	
	unsigned int nOffset = pImpl_->getOffset(false);
	pCallback->setCount(nOffset);
	for (unsigned int n = 0; n <= nOffset; ++n) {
		if (it != l.end() && *it == n) {
			++it;
		}
		else {
			wstring_ptr wstrPath(pImpl_->getPath(n));
			W2T(wstrPath.get(), ptszPath);
			
			WIN32_FIND_DATA fd;
			HANDLE hFind = ::FindFirstFile(ptszPath, &fd);
			bool bFind = hFind != INVALID_HANDLE_VALUE;
			::FindClose(hFind);
			if (bFind) {
				Message msg;
				if (!load(n, fd.nFileSizeLow, &msg))
					return false;
				if (!pCallback->salvage(msg))
					return false;
				::DeleteFile(ptszPath);
			}
		}
		pCallback->step(1);
	}
	
	return true;
}

bool qm::MultiMessageStore::check(MessageStoreCheckCallback* pCallback)
{
	std::auto_ptr<ClusterStorage> pCacheStorage(MessageStoreUtil::checkCache(
		pImpl_->pCacheStorage_.get(), pImpl_->wstrCachePath_.get(),
		pImpl_->nCacheBlockSize_, pCallback));
	if (!pCacheStorage.get())
		return false;
	
	pImpl_->pCacheStorage_ = pCacheStorage;
	
	return true;
}

malloc_ptr<unsigned char> qm::MultiMessageStore::readCache(MessageCacheKey key)
{
	Lock<CriticalSection> lock(pImpl_->cs_);
	
	const size_t nDefaultSize = 1024;
	malloc_ptr<unsigned char> p(
		static_cast<unsigned char*>(malloc(nDefaultSize)));
	if (!p.get())
		return 0;
	unsigned int nLoad = pImpl_->pCacheStorage_->load(p.get(), key, nDefaultSize);
	if (nLoad == -1 || nLoad < sizeof(size_t))
		return 0;
	
	size_t nSize = *reinterpret_cast<size_t*>(p.get());
	if (nSize + sizeof(nSize) > nDefaultSize) {
		unsigned char* p2 = static_cast<unsigned char*>(
			realloc(p.get(), nSize + sizeof(nSize)));
		if (!p2)
			return 0;
		p.release();
		p.reset(p2);
		
		if (pImpl_->pCacheStorage_->load(p.get(), key, nSize + sizeof(nSize)) == -1)
			return 0;
	}
	
	return p;
}


/****************************************************************************
 *
 * MessageStoreSalvageCallback
 *
 */

qm::MessageStoreSalvageCallback::~MessageStoreSalvageCallback()
{
}


/****************************************************************************
 *
 * MessageStoreCheckCallback
 *
 */

qm::MessageStoreCheckCallback::~MessageStoreCheckCallback()
{
}


/****************************************************************************
 *
 * MessageStoreUtil
 *
 */

void qm::MessageStoreUtil::freeUnrefered(ClusterStorage* pStorage,
										 const MessageStore::DataList& listData,
										 unsigned int nSeparatorSize)
{
	ClusterStorage::ReferList l;
	l.reserve(listData.size());
	for (MessageStore::DataList::const_iterator it = listData.begin(); it != listData.end(); ++it) {
		ClusterStorage::Refer refer = {
			(*it).nOffset_,
			(*it).nLength_ + nSeparatorSize
		};
		l.push_back(refer);
	}
	
	pStorage->freeUnrefered(l);
}

std::auto_ptr<ClusterStorage> qm::MessageStoreUtil::checkCache(ClusterStorage* pStorage,
															   const WCHAR* pwszPath,
															   unsigned int nBlockSize,
															   MessageStoreCheckCallback* pCallback)
{
	assert(pCallback);
	
	if (!pStorage->close())
		return 0;
	
	std::auto_ptr<ClusterStorage> pCacheStorage(new ClusterStorage(
		pwszPath, L"check", FileNames::BOX_EXT, FileNames::MAP_EXT, nBlockSize));
	
	unsigned int nCount = pCallback->getCount();
	for (unsigned int n = 0; n < nCount; ++n) {
		Message header;
		if (pCallback->getHeader(n, &header)) {
			malloc_size_ptr<unsigned char> pData(MessageCache::createData(header));
			if (!pData.get())
				return 0;
			
			size_t nDataLen = pData.size();
			const unsigned char* pCache[] = {
				reinterpret_cast<const unsigned char*>(&nDataLen),
				pData.get(),
			};
			size_t nCacheLen[] = {
				sizeof(nDataLen),
				nDataLen,
			};
			
			MessageCacheKey key = pCacheStorage->save(pCache, nCacheLen, countof(pCache));
			if (key == -1)
				return 0;
			
			pCallback->setKey(n, key);
		}
		else {
			if (pCallback->isIgnoreError(n))
				pCallback->setKey(n, -1);
			else
				return 0;
		}
	}
	
	if (!pCacheStorage->rename(pwszPath, L"cache"))
		return 0;
	
	return pCacheStorage;
}
