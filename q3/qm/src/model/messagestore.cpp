/*
 * $Id$
 *
 * Copyright(C) 1998-2003 Satoshi Nakamura
 * All rights reserved.
 *
 */

#include <qmextensions.h>

#include <qsclusterstorage.h>
#include <qsconv.h>
#include <qserror.h>
#include <qsnew.h>
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
	
	WSTRING wstrPath_;
	ClusterStorage* pStorage_;
	ClusterStorage* pCacheStorage_;
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
	unsigned int nBlockSize, unsigned int nCacheBlockSize, QSTATUS* pstatus)
{
	assert(pstatus);
	
	*pstatus = QSTATUS_SUCCESS;
	
	DECLARE_QSTATUS();
	
	string_ptr<WSTRING> wstrPath(allocWString(pwszPath));
	if (!wstrPath.get()) {
		*pstatus = QSTATUS_OUTOFMEMORY;
		return;
	}
	
	ClusterStorage::Init initMsg = {
		pwszPath,
		L"msg",
		Extensions::BOX,
		Extensions::MAP,
		nBlockSize
	};
	std::auto_ptr<ClusterStorage> pStorage;
	status = newQsObject(initMsg, &pStorage);
	CHECK_QSTATUS_SET(pstatus);
	
	ClusterStorage::Init initCache = {
		pwszPath,
		L"cache",
		Extensions::BOX,
		Extensions::MAP,
		nCacheBlockSize
	};
	std::auto_ptr<ClusterStorage> pCacheStorage;
	status = newQsObject(initCache, &pCacheStorage);
	CHECK_QSTATUS_SET(pstatus);
	
	status = newObject(&pImpl_);
	CHECK_QSTATUS_SET(pstatus);
	pImpl_->wstrPath_ = wstrPath.release();
	pImpl_->pStorage_ = pStorage.release();
	pImpl_->pCacheStorage_ = pCacheStorage.release();
}

qm::SingleMessageStore::~SingleMessageStore()
{
	if (pImpl_) {
		close();
		
		freeWString(pImpl_->wstrPath_);
		delete pImpl_->pStorage_;
		delete pImpl_->pCacheStorage_;
		delete pImpl_;
		pImpl_ = 0;
	}
}

QSTATUS qm::SingleMessageStore::close()
{
	Lock<CriticalSection> lock(pImpl_->cs_);
	
	QSTATUS status1 = pImpl_->pStorage_->close();
	QSTATUS status2 = pImpl_->pCacheStorage_->close();
	
	return status1 != QSTATUS_SUCCESS ? status1 :
		status2 != QSTATUS_SUCCESS ? status2 : QSTATUS_SUCCESS;
}

QSTATUS qm::SingleMessageStore::flush()
{
	DECLARE_QSTATUS();
	
	Lock<CriticalSection> lock(pImpl_->cs_);
	
	status = pImpl_->pStorage_->flush();
	CHECK_QSTATUS();
	status = pImpl_->pCacheStorage_->flush();
	CHECK_QSTATUS();
	
	return QSTATUS_SUCCESS;
}

QSTATUS qm::SingleMessageStore::load(unsigned int nOffset,
	unsigned int nLength, Message* pMessage)
{
	assert(pMessage);
	
	DECLARE_QSTATUS();
	
	Lock<CriticalSection> lock(pImpl_->cs_);
	
	unsigned int nLoad = nLength + SingleMessageStoreImpl::SEPARATOR_SIZE;
	malloc_ptr<unsigned char> pBuf(static_cast<unsigned char*>(malloc(nLoad)));
	if (!pBuf.get())
		return QSTATUS_OUTOFMEMORY;
	status = pImpl_->pStorage_->load(pBuf.get(), nOffset, &nLoad);
	CHECK_QSTATUS();
	unsigned char* p = pBuf.get() + SingleMessageStoreImpl::SEPARATOR_SIZE;
	
	status = pMessage->create(reinterpret_cast<CHAR*>(p),
		nLength, Message::FLAG_NONE);
	CHECK_QSTATUS();
	
	return QSTATUS_SUCCESS;
}

QSTATUS qm::SingleMessageStore::save(const CHAR* pszMessage,
	const Message& header, MessageCache* pMessageCache, bool bIndexOnly,
	unsigned int* pnOffset, unsigned int* pnLength,
	unsigned int* pnHeaderLength, MessageCacheKey* pKey)
{
	assert(pnOffset);
	assert(pnLength);
	assert(pnHeaderLength);
	assert(pKey);
	
	DECLARE_QSTATUS();
	
	const CHAR* pszHeader = header.getHeader();
	size_t nHeaderLen = strlen(pszHeader);
	const CHAR* pszBody = strstr(pszMessage, "\r\n\r\n");
	pszBody = pszBody ? pszBody + 4 : "";
	size_t nBodyLen = strlen(pszBody);
	
	if (!bIndexOnly) {
		*pnHeaderLength = nHeaderLen;
		*pnLength = nHeaderLen + nBodyLen + 2;
	}
	
	unsigned char* pData = 0;
	size_t nDataLen = 0;
	status = pMessageCache->createData(header, &pData, &nDataLen);
	CHECK_QSTATUS();
	malloc_ptr<unsigned char> pDelete(pData);
	
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
	
	const unsigned char* pCache[] = {
		reinterpret_cast<const unsigned char*>(&nDataLen),
		pData,
	};
	size_t nCacheLen[] = {
		sizeof(nDataLen),
		nDataLen,
	};
	
	Lock<CriticalSection> lock(pImpl_->cs_);
	if (!bIndexOnly) {
		status = pImpl_->pStorage_->save(pMsg, nMsgLen, countof(pMsg), pnOffset);
		CHECK_QSTATUS();
	}
	status = pImpl_->pCacheStorage_->save(pCache, nCacheLen, countof(pCache), pKey);
	CHECK_QSTATUS();
	
	return QSTATUS_SUCCESS;
}

QSTATUS qm::SingleMessageStore::free(unsigned int nOffset,
	unsigned int nLength, MessageCacheKey key)
{
	DECLARE_QSTATUS();
	
	Lock<CriticalSection> lock(pImpl_->cs_);
	
	size_t nDataLen = 0;
	unsigned int nLoad = sizeof(nDataLen);
	status = pImpl_->pCacheStorage_->load(
		reinterpret_cast<unsigned char*>(&nDataLen), key, &nLoad);
	CHECK_QSTATUS();
	if (nLoad != sizeof(nDataLen))
		return QSTATUS_FAIL;
	status = pImpl_->pCacheStorage_->free(key, nDataLen);
	CHECK_QSTATUS();
	
	status = pImpl_->pStorage_->free(nOffset,
		nLength + SingleMessageStoreImpl::SEPARATOR_SIZE*2);
	CHECK_QSTATUS();
	
	return QSTATUS_SUCCESS;
}

QSTATUS qm::SingleMessageStore::compact(unsigned int nOffset,
	unsigned int nLength, MessageCacheKey key, MessageStore* pmsOld,
	unsigned int* pnOffset, MessageCacheKey* pKey)
{
	assert(pnOffset);
	assert(pKey);
	
	DECLARE_QSTATUS();
	
	Lock<CriticalSection> lock(pImpl_->cs_);
	
	ClusterStorage* pCacheStorage = !pmsOld ? 0 :
		static_cast<SingleMessageStore*>(pmsOld)->pImpl_->pCacheStorage_;
	size_t nDataLen = 0;
	unsigned int nLoad = sizeof(nDataLen);
	status = pImpl_->pCacheStorage_->load(
		reinterpret_cast<unsigned char*>(&nDataLen), key, &nLoad);
	CHECK_QSTATUS();
	if (nLoad != sizeof(nDataLen))
		return QSTATUS_FAIL;
	status = pImpl_->pCacheStorage_->compact(key, nDataLen, pCacheStorage, pKey);
	CHECK_QSTATUS();
	
	if (nOffset != -1) {
		ClusterStorage* pStorage = !pmsOld ? 0 :
			static_cast<SingleMessageStore*>(pmsOld)->pImpl_->pStorage_;
		status = pImpl_->pStorage_->compact(nOffset, nLength, pStorage, pnOffset);
		CHECK_QSTATUS();
	}
	else {
		*pnOffset = nOffset;
	}
	
	return QSTATUS_SUCCESS;
}

QSTATUS qm::SingleMessageStore::freeUnused()
{
	DECLARE_QSTATUS();
	
	Lock<CriticalSection> lock(pImpl_->cs_);
	
	status = pImpl_->pCacheStorage_->freeUnused();
	CHECK_QSTATUS();
	
	status = pImpl_->pStorage_->freeUnused();
	CHECK_QSTATUS();
	
	return QSTATUS_SUCCESS;
}

QSTATUS qm::SingleMessageStore::readCache(
	MessageCacheKey key, unsigned char** ppBuf)
{
	assert(ppBuf);
	
	DECLARE_QSTATUS();
	
	*ppBuf = 0;
	
	Lock<CriticalSection> lock(pImpl_->cs_);
	
	const size_t nDefaultSize = 1024;
	malloc_ptr<unsigned char> p(
		static_cast<unsigned char*>(malloc(nDefaultSize)));
	if (!p.get())
		return QSTATUS_OUTOFMEMORY;
	unsigned int nLoad = nDefaultSize;
	status = pImpl_->pCacheStorage_->load(p.get(), key, &nLoad);
	CHECK_QSTATUS();
	if (nLoad == -1 || nLoad < sizeof(size_t))
		return QSTATUS_FAIL;
	
	size_t nSize = *reinterpret_cast<size_t*>(p.get());
	if (nSize + sizeof(size_t) > nDefaultSize) {
		unsigned char* p2 = static_cast<unsigned char*>(
			realloc(p.get(), nSize + sizeof(size_t)));
		if (!p2)
			return QSTATUS_OUTOFMEMORY;
		p.release();
		p.reset(p2);
		
		nLoad = nSize + sizeof(size_t);
		status = pImpl_->pCacheStorage_->load(p.get(), key, &nLoad);
		CHECK_QSTATUS();
	}
	
	*ppBuf = p.release();
	
	return QSTATUS_SUCCESS;
}


/****************************************************************************
 *
 * MultiMessageStoreImpl
 *
 */

struct qm::MultiMessageStoreImpl
{
public:
	QSTATUS init();
	QSTATUS getOffset(unsigned int* pnOffset);
	QSTATUS getPath(unsigned int nOffset, WSTRING* pwstrPath) const;
	QSTATUS ensureDirectory(unsigned int nOffset) const;

public:
	typedef std::vector<bool> DirList;

public:
	WSTRING wstrPath_;
	ClusterStorage* pCacheStorage_;
	unsigned int nOffset_;
	CriticalSection cs_;
	mutable DirList listDir_;
};

QSTATUS qm::MultiMessageStoreImpl::init()
{
	DECLARE_QSTATUS();
	
	string_ptr<WSTRING> wstrPath(concat(wstrPath_, L"\\msg"));
	if (!wstrPath.get())
		return QSTATUS_OUTOFMEMORY;
	
	W2T(wstrPath.get(), ptszPath);
	
	DWORD dwAttributes = ::GetFileAttributes(ptszPath);
	if (dwAttributes == 0xffffffff) {
		if (!::CreateDirectory(ptszPath, 0))
			return QSTATUS_FAIL;
	}
	else if (!(dwAttributes & FILE_ATTRIBUTE_DIRECTORY)) {
		return QSTATUS_FAIL;
	}
	
	return QSTATUS_SUCCESS;
}

QSTATUS qm::MultiMessageStoreImpl::getOffset(unsigned int *pnOffset)
{
	assert(pnOffset);
	
	DECLARE_QSTATUS();
	
	if (nOffset_ == -1) {
		unsigned int nDir = 0;
		
		string_ptr<WSTRING> wstrDirFind(concat(wstrPath_, L"\\msg\\*"));
		if (!wstrDirFind.get())
			return QSTATUS_OUTOFMEMORY;
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
		string_ptr<WSTRING> wstrFind(concat(wstrPath_, wszFind));
		if (!wstrFind.get())
			return QSTATUS_OUTOFMEMORY;
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
	
	*pnOffset = ++nOffset_;
	
	return QSTATUS_SUCCESS;
}

QSTATUS qm::MultiMessageStoreImpl::getPath(
	unsigned int nOffset, WSTRING* pwstrPath) const
{
	assert(pwstrPath);
	
	WCHAR wsz[64];
	swprintf(wsz, L"\\msg\\%08d\\%08d.msg", nOffset/1000, nOffset);
	*pwstrPath = concat(wstrPath_, wsz);
	
	return *pwstrPath ? QSTATUS_SUCCESS : QSTATUS_OUTOFMEMORY;
}

QSTATUS qm::MultiMessageStoreImpl::ensureDirectory(unsigned int nOffset) const
{
	DECLARE_QSTATUS();
	
	unsigned int nIndex = nOffset/1000;
	
	if (nIndex >= listDir_.size() || !listDir_[nIndex]) {
		WCHAR wsz[64];
		swprintf(wsz, L"\\msg\\%08d", nIndex);
		
		string_ptr<WSTRING> wstrPath(concat(wstrPath_, wsz));
		if (!wstrPath.get())
			return QSTATUS_OUTOFMEMORY;
		
		W2T(wstrPath.get(), ptszPath);
		::CreateDirectory(ptszPath, 0);
		
		if (listDir_.size() <= nIndex) {
			status = STLWrapper<DirList>(listDir_).resize(nIndex + 1);
			CHECK_QSTATUS();
		}
		listDir_[nIndex] = true;
	}
	
	return QSTATUS_SUCCESS;
}


/****************************************************************************
 *
 * MultiMessageStore
 *
 */

qm::MultiMessageStore::MultiMessageStore(const WCHAR* pwszPath,
	unsigned int nCacheBlockSize, QSTATUS* pstatus)
{
	assert(pstatus);
	
	*pstatus = QSTATUS_SUCCESS;
	
	DECLARE_QSTATUS();
	
	string_ptr<WSTRING> wstrPath(allocWString(pwszPath));
	if (!wstrPath.get()) {
		*pstatus = QSTATUS_OUTOFMEMORY;
		return;
	}
	
	ClusterStorage::Init initCache = {
		pwszPath,
		L"cache",
		Extensions::BOX,
		Extensions::MAP,
		nCacheBlockSize
	};
	std::auto_ptr<ClusterStorage> pCacheStorage;
	status = newQsObject(initCache, &pCacheStorage);
	CHECK_QSTATUS_SET(pstatus);
	
	status = newObject(&pImpl_);
	CHECK_QSTATUS_SET(pstatus);
	pImpl_->wstrPath_ = wstrPath.release();
	pImpl_->pCacheStorage_ = pCacheStorage.release();
	pImpl_->nOffset_ = -1;
	
	status = pImpl_->init();
	CHECK_QSTATUS_SET(pstatus);
}

qm::MultiMessageStore::~MultiMessageStore()
{
	if (pImpl_) {
		close();
		
		freeWString(pImpl_->wstrPath_);
		delete pImpl_->pCacheStorage_;
		delete pImpl_;
		pImpl_ = 0;
	}
}

QSTATUS qm::MultiMessageStore::close()
{
	DECLARE_QSTATUS();
	
	Lock<CriticalSection> lock(pImpl_->cs_);
	
	status = pImpl_->pCacheStorage_->close();
	CHECK_QSTATUS();
	
	return QSTATUS_SUCCESS;
}

QSTATUS qm::MultiMessageStore::flush()
{
	DECLARE_QSTATUS();
	
	Lock<CriticalSection> lock(pImpl_->cs_);
	
	status = pImpl_->pCacheStorage_->flush();
	CHECK_QSTATUS();
	
	return QSTATUS_SUCCESS;
}

QSTATUS qm::MultiMessageStore::load(unsigned int nOffset,
	unsigned int nLength, Message* pMessage)
{
	assert(pMessage);
	
	DECLARE_QSTATUS();
	
	Lock<CriticalSection> lock(pImpl_->cs_);
	
	string_ptr<WSTRING> wstrPath;
	status = pImpl_->getPath(nOffset, &wstrPath);
	CHECK_QSTATUS();
	FileInputStream stream(wstrPath.get(), &status);
	CHECK_QSTATUS();
	BufferedInputStream bufferedStream(&stream, false, &status);
	CHECK_QSTATUS();
	
	malloc_ptr<unsigned char> pBuf(static_cast<unsigned char*>(malloc(nLength)));
	if (!pBuf.get())
		return QSTATUS_OUTOFMEMORY;
	
	size_t nRead = 0;
	status = bufferedStream.read(pBuf.get(), nLength, &nRead);
	CHECK_QSTATUS();
	if (nRead != nLength)
		return QSTATUS_FAIL;
	
	status = pMessage->create(reinterpret_cast<CHAR*>(pBuf.get()),
		nRead, Message::FLAG_NONE);
	CHECK_QSTATUS();
	
	return QSTATUS_SUCCESS;
}

QSTATUS qm::MultiMessageStore::save(const CHAR* pszMessage,
	const Message& header, MessageCache* pMessageCache, bool bIndexOnly,
	unsigned int* pnOffset, unsigned int* pnLength,
	unsigned int* pnHeaderLength, MessageCacheKey* pKey)
{
	assert(pnOffset);
	assert(pnLength);
	assert(pnHeaderLength);
	assert(pKey);
	
	DECLARE_QSTATUS();
	
	const CHAR* pszHeader = header.getHeader();
	size_t nHeaderLen = strlen(pszHeader);
	const CHAR* pszBody = strstr(pszMessage, "\r\n\r\n");
	pszBody = pszBody ? pszBody + 4 : "";
	size_t nBodyLen = strlen(pszBody);
	
	if (!bIndexOnly) {
		*pnHeaderLength = nHeaderLen;
		*pnLength = nHeaderLen + nBodyLen + 2;
	}
	
	unsigned char* pData = 0;
	size_t nDataLen = 0;
	status = pMessageCache->createData(header, &pData, &nDataLen);
	CHECK_QSTATUS();
	malloc_ptr<unsigned char> pDelete(pData);
	
	const unsigned char* pCache[] = {
		reinterpret_cast<const unsigned char*>(&nDataLen),
		pData,
	};
	size_t nCacheLen[] = {
		sizeof(nDataLen),
		nDataLen,
	};
	
	Lock<CriticalSection> lock(pImpl_->cs_);
	
	if (!bIndexOnly) {
		status = pImpl_->getOffset(pnOffset);
		CHECK_QSTATUS();
		status = pImpl_->ensureDirectory(*pnOffset);
		CHECK_QSTATUS();
		string_ptr<WSTRING> wstrPath;
		status = pImpl_->getPath(*pnOffset, &wstrPath);
		CHECK_QSTATUS();
		FileOutputStream stream(wstrPath.get(), &status);
		CHECK_QSTATUS();
		BufferedOutputStream bufferedStream(&stream, false, &status);
		CHECK_QSTATUS();
		status = bufferedStream.write(
			reinterpret_cast<const unsigned char*>(pszHeader), nHeaderLen);
		CHECK_QSTATUS();
		status = bufferedStream.write(
			reinterpret_cast<const unsigned char*>("\r\n"), 2);
		CHECK_QSTATUS();
		status = bufferedStream.write(
			reinterpret_cast<const unsigned char*>(pszBody), nBodyLen);
		CHECK_QSTATUS();
		status = bufferedStream.close();
		CHECK_QSTATUS();
	}
	status = pImpl_->pCacheStorage_->save(pCache, nCacheLen, countof(pCache), pKey);
	CHECK_QSTATUS();
	
	return QSTATUS_SUCCESS;
}

QSTATUS qm::MultiMessageStore::free(unsigned int nOffset,
	unsigned int nLength, MessageCacheKey key)
{
	DECLARE_QSTATUS();
	
	Lock<CriticalSection> lock(pImpl_->cs_);
	
	size_t nDataLen = 0;
	unsigned int nLoad = sizeof(nDataLen);
	status = pImpl_->pCacheStorage_->load(
		reinterpret_cast<unsigned char*>(&nDataLen), key, &nLoad);
	CHECK_QSTATUS();
	if (nLoad != sizeof(nDataLen))
		return QSTATUS_FAIL;
	status = pImpl_->pCacheStorage_->free(key, nDataLen);
	CHECK_QSTATUS();
	
	string_ptr<WSTRING> wstrPath;
	status = pImpl_->getPath(nOffset, &wstrPath);
	CHECK_QSTATUS();
	W2T(wstrPath.get(), ptszPath);
	::DeleteFile(ptszPath);
	
	return QSTATUS_SUCCESS;
}

QSTATUS qm::MultiMessageStore::compact(unsigned int nOffset,
	unsigned int nLength, MessageCacheKey key, MessageStore* pmsOld,
	unsigned int* pnOffset, MessageCacheKey* pKey)
{
	assert(pnOffset);
	assert(pKey);
	
	DECLARE_QSTATUS();
	
	Lock<CriticalSection> lock(pImpl_->cs_);
	
	ClusterStorage* pCacheStorage = !pmsOld ? 0 :
		static_cast<MultiMessageStore*>(pmsOld)->pImpl_->pCacheStorage_;
	size_t nDataLen = 0;
	unsigned int nLoad = sizeof(nDataLen);
	status = pImpl_->pCacheStorage_->load(
		reinterpret_cast<unsigned char*>(&nDataLen), key, &nLoad);
	CHECK_QSTATUS();
	if (nLoad != sizeof(nDataLen))
		return QSTATUS_FAIL;
	status = pImpl_->pCacheStorage_->compact(key, nDataLen, pCacheStorage, pKey);
	CHECK_QSTATUS();
	
	*pnOffset = nOffset;
	
	return QSTATUS_SUCCESS;
}

QSTATUS qm::MultiMessageStore::freeUnused()
{
	DECLARE_QSTATUS();
	
	Lock<CriticalSection> lock(pImpl_->cs_);
	
	status = pImpl_->pCacheStorage_->freeUnused();
	CHECK_QSTATUS();
	
	return QSTATUS_SUCCESS;
}

QSTATUS qm::MultiMessageStore::readCache(
	MessageCacheKey key, unsigned char** ppBuf)
{
	assert(ppBuf);
	
	DECLARE_QSTATUS();
	
	*ppBuf = 0;
	
	Lock<CriticalSection> lock(pImpl_->cs_);
	
	const size_t nDefaultSize = 1024;
	malloc_ptr<unsigned char> p(
		static_cast<unsigned char*>(malloc(nDefaultSize)));
	if (!p.get())
		return QSTATUS_OUTOFMEMORY;
	unsigned int nLoad = nDefaultSize;
	status = pImpl_->pCacheStorage_->load(p.get(), key, &nLoad);
	CHECK_QSTATUS();
	if (nLoad == -1 || nLoad < sizeof(size_t))
		return QSTATUS_FAIL;
	
	size_t nSize = *reinterpret_cast<size_t*>(p.get());
	if (nSize + sizeof(size_t) > nDefaultSize) {
		unsigned char* p2 = static_cast<unsigned char*>(
			realloc(p.get(), nSize + sizeof(size_t)));
		if (!p2)
			return QSTATUS_OUTOFMEMORY;
		p.release();
		p.reset(p2);
		
		nLoad = nSize + sizeof(size_t);
		status = pImpl_->pCacheStorage_->load(p.get(), key, &nLoad);
		CHECK_QSTATUS();
	}
	
	*ppBuf = p.release();
	
	return QSTATUS_SUCCESS;
}
