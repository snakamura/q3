/*
 * $Id$
 *
 * Copyright(C) 1998-2006 Satoshi Nakamura
 * All rights reserved.
 *
 */

#pragma warning(disable:4786)

#include <qmfilenames.h>

#include <qsconv.h>
#include <qsfile.h>
#include <qsosutil.h>
#include <qsthread.h>

#include <stdio.h>
#include <tchar.h>

#include "messageindex.h"
#include "messagestore.h"

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

bool qm::MessageStore::save(const CHAR* pszMessage,
							size_t nLen,
							const Message* pHeader,
							const WCHAR* pwszLabel,
							bool bIndexOnly,
							unsigned int* pnOffset,
							unsigned int* pnLength,
							unsigned int* pnHeaderLength,
							unsigned int* pnIndexKey,
							unsigned int* pnIndexLength)
{
	assert(pszMessage);
	assert(pnOffset);
	assert(pnLength);
	assert(pnHeaderLength);
	assert(pnIndexKey);
	assert(pnIndexLength);
	
	if (nLen == -1)
		nLen = strlen(pszMessage);
	
	Message header;
	
	const CHAR* pBody = Part::getBody(pszMessage, nLen);
	size_t nBodyLen = 0;
	if (pBody)
		nBodyLen = nLen - (pBody - pszMessage);
	else
		pBody = "";
	
	if (!pHeader) {
		if (!header.createHeader(pszMessage, nLen))
			return false;
		pHeader = &header;
	}
	
	return save(*pHeader, pBody, nBodyLen, pwszLabel, bIndexOnly,
		pnOffset, pnLength, pnHeaderLength, pnIndexKey, pnIndexLength);
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
	wstring_ptr wstrIndexPath_;
	unsigned int nIndexBlockSize_;
	std::auto_ptr<ClusterStorage> pStorage_;
	std::auto_ptr<ClusterStorage> pIndexStorage_;
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
										   const WCHAR* pwszIndexPath,
										   unsigned int nIndexBlockSize)
{
	wstring_ptr wstrPath(allocWString(pwszPath));
	wstring_ptr wstrIndexPath(allocWString(pwszIndexPath));
	
	std::auto_ptr<ClusterStorage> pStorage(new ClusterStorage(pwszPath,
		FileNames::MSG, FileNames::BOX_EXT, FileNames::MAP_EXT, nBlockSize));
	std::auto_ptr<ClusterStorage> pIndexStorage(new ClusterStorage(pwszIndexPath,
		FileNames::INDEX, FileNames::BOX_EXT, FileNames::MAP_EXT, nIndexBlockSize));
	
	pImpl_ = new SingleMessageStoreImpl();
	pImpl_->wstrPath_ = wstrPath;
	pImpl_->wstrIndexPath_ = wstrIndexPath;
	pImpl_->nIndexBlockSize_ = nIndexBlockSize;
	pImpl_->pStorage_ = pStorage;
	pImpl_->pIndexStorage_ = pIndexStorage;
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
	return pImpl_->pStorage_->close() && pImpl_->pIndexStorage_->close();
}

bool qm::SingleMessageStore::flush()
{
	Lock<CriticalSection> lock(pImpl_->cs_);
	return pImpl_->pStorage_->close() && pImpl_->pIndexStorage_->close();
}

bool qm::SingleMessageStore::load(unsigned int nOffset,
								  unsigned int nLength,
								  Message* pMessage)
{
	assert(pMessage);
	
	Lock<CriticalSection> lock(pImpl_->cs_);
	
	unsigned int nLoad = nLength + SingleMessageStoreImpl::SEPARATOR_SIZE;
	malloc_ptr<unsigned char> pBuf(static_cast<unsigned char*>(allocate(nLoad)));
	if (!pBuf.get())
		return false;
	if (pImpl_->pStorage_->load(pBuf.get(), nOffset, nLoad) == -1)
		return false;
	unsigned char* p = pBuf.get() + SingleMessageStoreImpl::SEPARATOR_SIZE;
	
	return pMessage->create(reinterpret_cast<CHAR*>(p), nLength, Message::FLAG_NONE);
}

bool qm::SingleMessageStore::save(const Message& header,
								  const CHAR* pszBody,
								  size_t nBodyLen,
								  const WCHAR* pwszLabel,
								  bool bIndexOnly,
								  unsigned int* pnOffset,
								  unsigned int* pnLength,
								  unsigned int* pnHeaderLength,
								  unsigned int* pnIndexKey,
								  unsigned int* pnIndexLength)
{
	assert(pnOffset);
	assert(pnLength);
	assert(pnHeaderLength);
	assert(pnIndexKey);
	assert(pnIndexLength);
	
	const CHAR* pszHeader = header.getHeader();
	size_t nHeaderLen = strlen(pszHeader);
	
	if (!bIndexOnly) {
		*pnHeaderLength = static_cast<unsigned int>(nHeaderLen);
		*pnLength = static_cast<unsigned int>(nHeaderLen + nBodyLen + 2);
	}
	
	malloc_size_ptr<unsigned char> pIndex(MessageIndex::createIndex(header, pwszLabel));
	if (!pIndex.get())
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
	
	Lock<CriticalSection> lock(pImpl_->cs_);
	
	if (!bIndexOnly) {
		*pnOffset = pImpl_->pStorage_->save(pMsg, nMsgLen, countof(pMsg));
		if (*pnOffset == -1)
			return false;
	}
	
	const unsigned char* p = pIndex.get();
	size_t nIndexLength = pIndex.size();
	*pnIndexKey = pImpl_->pIndexStorage_->save(&p, &nIndexLength, 1);
	if (*pnIndexKey == -1)
		return false;
	*pnIndexLength = static_cast<unsigned int>(nIndexLength);
	
	return true;
}

bool qm::SingleMessageStore::saveDecoded(unsigned int nOffset,
										 const Message& msg)
{
	return false;
}

bool qm::SingleMessageStore::updateIndex(unsigned int nOldIndexKey,
										 unsigned int nOldIndexLength,
										 const unsigned char* pIndex,
										 unsigned int nIndexLength,
										 unsigned int* pnIndexKey)
{
	return MessageStoreUtil::updateIndex(pImpl_->pIndexStorage_.get(),
		nOldIndexKey, nOldIndexLength, pIndex, nIndexLength, pnIndexKey);
}

bool qm::SingleMessageStore::free(unsigned int nOffset,
								  unsigned int nLength,
								  unsigned int nIndexKey,
								  unsigned int nIndexLength)
{
	Lock<CriticalSection> lock(pImpl_->cs_);
	
	if (nIndexKey != -1) {
		if (!pImpl_->pIndexStorage_->free(nIndexKey, nIndexLength))
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
	
	pCallback->setCancelable(false);
	pCallback->setCount(static_cast<int>(pListData->size()));
	pCallback->show();
	
	Lock<CriticalSection> lock(pImpl_->cs_);
	
	std::auto_ptr<ClusterStorage> pIndexStorage(new ClusterStorage(
		pImpl_->wstrIndexPath_.get(), FileNames::COMPACT,
		FileNames::BOX_EXT, FileNames::MAP_EXT, pImpl_->nIndexBlockSize_));
	ClusterStorage* pOldIndexStorage = pImpl_->pIndexStorage_.get();
	
	for (DataList::iterator it = pListData->begin(); it != pListData->end(); ++it) {
		Data& data = *it;
		
		data.nIndexKey_ = pIndexStorage->compact(
			data.nIndexKey_, data.nIndexLength_, pOldIndexStorage);
		if (data.nIndexKey_ == -1)
			return false;
		
		if (data.nOffset_ != -1) {
			size_t nLen = data.nLength_ + SingleMessageStoreImpl::SEPARATOR_SIZE*2;
			data.nOffset_ = pImpl_->pStorage_->compact(data.nOffset_, nLen, 0);
			if (data.nOffset_ == -1)
				return false;
		}
		
		pCallback->step(1);
	}
	
	if (!pImpl_->pIndexStorage_->close() ||
		!pIndexStorage->rename(pImpl_->wstrIndexPath_.get(), FileNames::INDEX))
		return false;
	pImpl_->pIndexStorage_ = pIndexStorage;
	
	MessageStoreUtil::freeUnrefered(pImpl_->pStorage_.get(),
		*pListData, SingleMessageStoreImpl::SEPARATOR_SIZE*2);
	
	return true;
}

bool qm::SingleMessageStore::salvage(const DataList& listData,
									 MessageStoreSalvageCallback* pCallback)
{
	return false;
}

bool qm::SingleMessageStore::check(MessageStoreCheckCallback* pCallback)
{
	std::auto_ptr<ClusterStorage> pIndexStorage(MessageStoreUtil::checkIndex(
		pImpl_->pIndexStorage_.get(), pImpl_->wstrIndexPath_.get(),
		pImpl_->nIndexBlockSize_, pCallback));
	if (!pIndexStorage.get())
		return false;
	
	pImpl_->pIndexStorage_ = pIndexStorage;
	
	return true;
}

bool qm::SingleMessageStore::freeUnused()
{
	return pImpl_->pStorage_->freeUnused() && pImpl_->pStorage_->flush();
}

malloc_ptr<unsigned char> qm::SingleMessageStore::readIndex(unsigned int nKey,
															unsigned int nLength)
{
	return MessageStoreUtil::readIndex(pImpl_->pIndexStorage_.get(), nKey, nLength);
}


/****************************************************************************
 *
 * MultiMessageStoreImpl
 *
 */

struct qm::MultiMessageStoreImpl
{
public:
	enum {
		FILES_IN_DIRECTORY = 1000
	};

public:
	typedef std::vector<int> DirList;

public:
	bool init();
	unsigned int getOffset(bool bIncrement);
	wstring_ptr getPath(unsigned int nOffset,
						bool bEncoded) const;
	void deleteFiles(unsigned int nOffset) const;
	bool ensureDirectory(unsigned int nOffset) const;
	void deleteEmptyDirectories(unsigned int nMaxOffset) const;
	void freeUnrefered(const MessageStore::DataList& listData,
					   unsigned int nMaxOffset,
					   MessageOperationCallback* pCallback);

public:
	wstring_ptr wstrPath_;
	wstring_ptr wstrIndexPath_;
	unsigned int nIndexBlockSize_;
	std::auto_ptr<ClusterStorage> pIndexStorage_;
	unsigned int nOffset_;
	CriticalSection cs_;
	mutable DirList listDir_;
};

bool qm::MultiMessageStoreImpl::init()
{
	wstring_ptr wstrPath(concat(wstrPath_.get(), L"\\msg"));
	if (!File::isDirectoryExisting(wstrPath.get())) {
		if (!File::createDirectory(wstrPath.get()))
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
		
		unsigned int nOffset = nDir*FILES_IN_DIRECTORY;
		
		WCHAR wszFind[32];
		_snwprintf(wszFind, countof(wszFind), L"\\msg\\%08d\\*.msg", nDir);
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

wstring_ptr qm::MultiMessageStoreImpl::getPath(unsigned int nOffset,
											   bool bEncoded) const
{
	WCHAR wsz[64];
	_snwprintf(wsz, countof(wsz), L"\\msg\\%08d\\%08d.%s",
		nOffset/FILES_IN_DIRECTORY, nOffset, bEncoded ? L"d.msg" : L"msg");
	return concat(wstrPath_.get(), wsz);
}

void qm::MultiMessageStoreImpl::deleteFiles(unsigned int nOffset) const
{
	bool b[] = { false, true };
	for (int n = 0; n < countof(b); ++n) {
		wstring_ptr wstrPath(getPath(nOffset, b[n]));
		W2T(wstrPath.get(), ptszPath);
		::DeleteFile(ptszPath);
	}
}

bool qm::MultiMessageStoreImpl::ensureDirectory(unsigned int nOffset) const
{
	unsigned int nIndex = nOffset/FILES_IN_DIRECTORY;
	
	if (nIndex >= listDir_.size() || !listDir_[nIndex]) {
		WCHAR wsz[64];
		_snwprintf(wsz, countof(wsz), L"\\msg\\%08d", nIndex);
		
		wstring_ptr wstrPath(concat(wstrPath_.get(), wsz));
		W2T(wstrPath.get(), ptszPath);
		::CreateDirectory(ptszPath, 0);
		
		if (listDir_.size() <= nIndex)
			listDir_.resize(nIndex + 1);
		listDir_[nIndex] = 1;
	}
	
	return true;
}

void qm::MultiMessageStoreImpl::deleteEmptyDirectories(unsigned int nMaxOffset) const
{
	unsigned int nMax = nMaxOffset/FILES_IN_DIRECTORY;
	for (unsigned int n = 0; n < nMax; ++n) {
		WCHAR wsz[64];
		_snwprintf(wsz, countof(wsz), L"\\msg\\%08d", n);
		wstring_ptr wstrPath(concat(wstrPath_.get(), wsz));
		if (File::isDirectoryEmpty(wstrPath.get()))
			File::removeDirectory(wstrPath.get());
		
		if (n < listDir_.size())
			listDir_[n] = 0;
	}
}

void qm::MultiMessageStoreImpl::freeUnrefered(const MessageStore::DataList& listData,
											  unsigned int nMaxOffset,
											  MessageOperationCallback* pCallback)
{
	assert(pCallback);
	
	typedef std::vector<unsigned int> List;
	List l;
	l.resize(listData.size());
	std::transform(listData.begin(), listData.end(),
		l.begin(), mem_data_ref(&MessageStore::Data::nOffset_));
	std::sort(l.begin(), l.end());
	
	List::const_iterator it = l.begin();
	for (unsigned int n = 0; n <= nMaxOffset; ++n) {
		if (it != l.end() && *it == n)
			++it;
		else
			deleteFiles(n);
		
		pCallback->step(1);
	}
	
	deleteEmptyDirectories(nMaxOffset);
}


/****************************************************************************
 *
 * MultiMessageStore
 *
 */

qm::MultiMessageStore::MultiMessageStore(const WCHAR* pwszPath,
										 const WCHAR* pwszIndexPath,
										 unsigned int nIndexBlockSize)
{
	wstring_ptr wstrPath(allocWString(pwszPath));
	wstring_ptr wstrIndexPath(allocWString(pwszIndexPath));
	std::auto_ptr<ClusterStorage> pIndexStorage(new ClusterStorage(pwszIndexPath,
		FileNames::INDEX, FileNames::BOX_EXT, FileNames::MAP_EXT, nIndexBlockSize));
	
	pImpl_ = new MultiMessageStoreImpl();
	pImpl_->wstrPath_ = wstrPath;
	pImpl_->wstrIndexPath_ = wstrIndexPath;
	pImpl_->nIndexBlockSize_ = nIndexBlockSize;
	pImpl_->pIndexStorage_ = pIndexStorage;
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
	return pImpl_->pIndexStorage_->close();
}

bool qm::MultiMessageStore::flush()
{
	Lock<CriticalSection> lock(pImpl_->cs_);
	return pImpl_->pIndexStorage_->close();
}

bool qm::MultiMessageStore::load(unsigned int nOffset,
								 unsigned int nLength,
								 Message* pMessage)
{
	assert(pMessage);
	
	Lock<CriticalSection> lock(pImpl_->cs_);
	
	wstring_ptr wstrPath(pImpl_->getPath(nOffset, false));
	FileInputStream stream(wstrPath.get());
	if (!stream)
		return false;
	
	malloc_ptr<unsigned char> pBuf(static_cast<unsigned char*>(allocate(nLength)));
	if (!pBuf.get())
		return false;
	
	size_t nRead = stream.read(pBuf.get(), nLength);
	if (nRead != nLength)
		return false;
	
	return pMessage->create(reinterpret_cast<CHAR*>(pBuf.get()), nRead, Message::FLAG_NONE);
}

bool qm::MultiMessageStore::save(const Message& header,
								 const CHAR* pszBody,
								 size_t nBodyLen,
								 const WCHAR* pwszLabel,
								 bool bIndexOnly,
								 unsigned int* pnOffset,
								 unsigned int* pnLength,
								 unsigned int* pnHeaderLength,
								 unsigned int* pnIndexKey,
								 unsigned int* pnIndexLength)
{
	assert(pnOffset);
	assert(pnLength);
	assert(pnHeaderLength);
	assert(pnIndexKey);
	assert(pnIndexLength);
	
	const CHAR* pszHeader = header.getHeader();
	size_t nHeaderLen = strlen(pszHeader);
	
	if (!bIndexOnly) {
		*pnHeaderLength = static_cast<unsigned int>(nHeaderLen);
		*pnLength = static_cast<unsigned int>(nHeaderLen + nBodyLen + 2);
	}
	
	malloc_size_ptr<unsigned char> pIndex(MessageIndex::createIndex(header, pwszLabel));
	if (!pIndex.get())
		return false;
	
	Lock<CriticalSection> lock(pImpl_->cs_);
	
	if (!bIndexOnly) {
		*pnOffset = pImpl_->getOffset(true);
		if (!pImpl_->ensureDirectory(*pnOffset))
			return false;
		wstring_ptr wstrPath(pImpl_->getPath(*pnOffset, false));
		FileOutputStream stream(wstrPath.get());
		if (!stream)
			return false;
		if (stream.write(reinterpret_cast<const unsigned char*>(pszHeader), nHeaderLen) == -1 ||
			stream.write(reinterpret_cast<const unsigned char*>("\r\n"), 2) == -1 ||
			stream.write(reinterpret_cast<const unsigned char*>(pszBody), nBodyLen) == -1 ||
			!stream.close())
			return false;
	}
	
	const unsigned char* p = pIndex.get();
	size_t nIndexLength = pIndex.size();
	*pnIndexKey = pImpl_->pIndexStorage_->save(&p, &nIndexLength, 1);
	if (*pnIndexKey == -1)
		return false;
	*pnIndexLength = static_cast<unsigned int>(nIndexLength);
	
	return true;
}

bool qm::MultiMessageStore::saveDecoded(unsigned int nOffset,
										const Message& msg)
{
	assert(nOffset != -1);
	
	Lock<CriticalSection> lock(pImpl_->cs_);
	
	wstring_ptr wstrPath(pImpl_->getPath(nOffset, true));
	if (File::isFileExisting(wstrPath.get()))
		return true;
	
	xstring_size_ptr strContent(msg.getContent());
	if (!strContent.get())
		return false;
	
	FileOutputStream stream(wstrPath.get());
	if (!stream)
		return false;
	
	if (stream.write(reinterpret_cast<const unsigned char*>(strContent.get()), strContent.size()) == -1 ||
		!stream.close())
		return false;
	
	return true;
}

bool qm::MultiMessageStore::updateIndex(unsigned int nOldIndexKey,
										unsigned int nOldIndexLength,
										const unsigned char* pIndex,
										unsigned int nIndexLength,
										unsigned int* pnIndexKey)
{
	return MessageStoreUtil::updateIndex(pImpl_->pIndexStorage_.get(),
		nOldIndexKey, nOldIndexLength, pIndex, nIndexLength, pnIndexKey);
}

bool qm::MultiMessageStore::free(unsigned int nOffset,
								 unsigned int nLength,
								 unsigned int nIndexKey,
								 unsigned int nIndexLength)
{
	Lock<CriticalSection> lock(pImpl_->cs_);
	
	if (nIndexKey != -1) {
		if (!pImpl_->pIndexStorage_->free(nIndexKey, nIndexLength))
			return false;
	}
	
	if (nOffset != -1)
		pImpl_->deleteFiles(nOffset);
	
	return true;
}

bool qm::MultiMessageStore::compact(DataList* pListData,
									MessageOperationCallback* pCallback)
{
	assert(pListData);
	assert(pCallback);
	
	unsigned int nOffset = pImpl_->getOffset(false);
	
	pCallback->setCancelable(false);
	pCallback->setCount(pListData->size() + nOffset);
	pCallback->show();
	
	Lock<CriticalSection> lock(pImpl_->cs_);
	
	std::auto_ptr<ClusterStorage> pIndexStorage(new ClusterStorage(
		pImpl_->wstrIndexPath_.get(), FileNames::COMPACT,
		FileNames::BOX_EXT, FileNames::MAP_EXT, pImpl_->nIndexBlockSize_));
	ClusterStorage* pOldIndexStorage = pImpl_->pIndexStorage_.get();
	
	for (DataList::iterator it = pListData->begin(); it != pListData->end(); ++it) {
		Data& data = *it;
		
		data.nIndexKey_ = pIndexStorage->compact(data.nIndexKey_,
			data.nIndexLength_, pOldIndexStorage);
		if (data.nIndexKey_ == -1)
			return false;
		
		pCallback->step(1);
	}
	
	if (!pImpl_->pIndexStorage_->close() ||
		!pIndexStorage->rename(pImpl_->wstrIndexPath_.get(), FileNames::INDEX))
		return false;
	pImpl_->pIndexStorage_ = pIndexStorage;
	
	pImpl_->freeUnrefered(*pListData, nOffset, pCallback);
	
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
			wstring_ptr wstrPath(pImpl_->getPath(n, false));
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
				pImpl_->deleteFiles(n);
			}
		}
		pCallback->step(1);
	}
	
	return true;
}

bool qm::MultiMessageStore::check(MessageStoreCheckCallback* pCallback)
{
	std::auto_ptr<ClusterStorage> pIndexStorage(MessageStoreUtil::checkIndex(
		pImpl_->pIndexStorage_.get(), pImpl_->wstrIndexPath_.get(),
		pImpl_->nIndexBlockSize_, pCallback));
	if (!pIndexStorage.get())
		return false;
	
	pImpl_->pIndexStorage_ = pIndexStorage;
	
	return true;
}

malloc_ptr<unsigned char> qm::MultiMessageStore::readIndex(unsigned int nKey,
														   unsigned int nLength)
{
	return MessageStoreUtil::readIndex(pImpl_->pIndexStorage_.get(), nKey, nLength);
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

std::auto_ptr<ClusterStorage> qm::MessageStoreUtil::checkIndex(ClusterStorage* pStorage,
															   const WCHAR* pwszPath,
															   unsigned int nBlockSize,
															   MessageStoreCheckCallback* pCallback)
{
	assert(pCallback);
	
	std::auto_ptr<ClusterStorage> pIndexStorage(new ClusterStorage(
		pwszPath, FileNames::CHECK, FileNames::BOX_EXT, FileNames::MAP_EXT, nBlockSize));
	
	unsigned int nCount = pCallback->getCount();
	for (unsigned int n = 0; n < nCount; ++n) {
		Message header;
		if (pCallback->getHeader(n, &header)) {
			malloc_size_ptr<unsigned char> pIndex(MessageIndex::createIndex(
				header, pCallback->getLabel(n).get()));
			if (!pIndex.get())
				return std::auto_ptr<ClusterStorage>(0);
			
			const unsigned char* p = pIndex.get();
			size_t nLength = pIndex.size();
			unsigned int nKey = pIndexStorage->save(&p, &nLength, 1);
			if (nKey == -1)
				return std::auto_ptr<ClusterStorage>(0);
			
			pCallback->setKey(n, nKey, static_cast<unsigned int>(nLength));
		}
		else {
			if (pCallback->isIgnoreError(n))
				pCallback->setKey(n, -1, -1);
			else
				return std::auto_ptr<ClusterStorage>(0);
		}
	}
	
	if (!pStorage->close() ||
		!pIndexStorage->rename(pwszPath, FileNames::INDEX))
		return std::auto_ptr<ClusterStorage>(0);
	
	return pIndexStorage;
}

malloc_ptr<unsigned char> qm::MessageStoreUtil::readIndex(ClusterStorage* pStorage,
														  unsigned int nKey,
														  unsigned int nLength)
{
	malloc_ptr<unsigned char> p(static_cast<unsigned char*>(allocate(nLength)));
	if (!p.get())
		return malloc_ptr<unsigned char>(0);
	
	size_t nLoad = pStorage->load(p.get(), nKey, nLength);
	if (nLoad != nLength)
		return malloc_ptr<unsigned char>(0);
	
	return p;
}

bool qm::MessageStoreUtil::updateIndex(ClusterStorage* pStorage,
									   unsigned int nOldIndexKey,
									   unsigned int nOldIndexLength,
									   const unsigned char* pIndex,
									   unsigned int nIndexLength,
									   unsigned int* pnIndexKey)
{
	assert(pStorage);
	assert(pIndex);
	assert(pnIndexKey);
	
	size_t nLength = nIndexLength;
	*pnIndexKey = pStorage->save(&pIndex, &nLength, 1);
	if (*pnIndexKey == -1)
		return false;
	
	if (!pStorage->free(nOldIndexKey, nOldIndexLength))
		return false;
	
	return true;
}
