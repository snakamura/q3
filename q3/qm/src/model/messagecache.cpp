/*
 * $Id$
 *
 * Copyright(C) 1998-2003 Satoshi Nakamura
 * All rights reserved.
 *
 */

#include <qsconv.h>
#include <qserror.h>
#include <qsnew.h>
#include <qsstream.h>

#include <algorithm>

#include "messagecache.h"
#include "messagestore.h"

#pragma warning(disable:4786)

using namespace qm;
using namespace qs;


/****************************************************************************
 *
 * MessageCache
 *
 */

qm::MessageCache::MessageCache(
	MessageStore* pMessageStore, QSTATUS* pstatus) :
	pMessageStore_(pMessageStore),
	nMaxSize_(-1),
	nSize_(0),
	pNewFirst_(0),
	pNewLast_(0),
	pLastGotten_(0)
{
	assert(pstatus);
	*pstatus = QSTATUS_SUCCESS;
	
	DECLARE_QSTATUS();
	
	status = newObject(-1, &pNewFirst_);
	CHECK_QSTATUS_SET(pstatus);
	pNewFirst_->pNewNext_ = pNewFirst_;
	pNewFirst_->pNewPrev_ = pNewFirst_;
	pNewLast_ = pNewFirst_;
}

qm::MessageCache::~MessageCache()
{
#if _MSC_VER == 1202 && defined MIPS
	ItemMap::const_iterator it = map_.begin();
	while (it != map_.end()) {
		delete (*it).second;
		++it;
	}
#else
	std::for_each(map_.begin(), map_.end(),
		unary_compose_f_gx(
			deleter<CacheItem>(),
			std::select2nd<ItemMap::value_type>()));
#endif
	delete pNewLast_;
}

QSTATUS qm::MessageCache::createData(const Message& msg,
	unsigned char** ppData, size_t* pnLen)
{
	assert(ppData);
	assert(pnLen);
	
	DECLARE_QSTATUS();
	
	ByteOutputStream stream(&status);
	CHECK_QSTATUS();
	
	Part::Field field;
	
	const WCHAR* pwszFields[] = {
		L"From",
		L"To"
	};
	for (int n = 0; n < countof(pwszFields); ++n) {
		AddressListParser address(0, &status);
		CHECK_QSTATUS();
		status = msg.getField(pwszFields[n], &address, &field);
		CHECK_QSTATUS();
		if (field == Part::FIELD_EXIST) {
			string_ptr<WSTRING> wstrNames;
			status = address.getNames(&wstrNames);
			CHECK_QSTATUS();
			status = writeToStream(&stream, wstrNames.get());
			CHECK_QSTATUS();
		}
		else {
			status = writeToStream(&stream, 0);
			CHECK_QSTATUS();
		}
	}
	
	UnstructuredParser subject(&status);
	CHECK_QSTATUS();
	status = msg.getField(L"Subject", &subject, &field);
	CHECK_QSTATUS();
	if (field == Part::FIELD_EXIST) {
		status = writeToStream(&stream, subject.getValue());
		CHECK_QSTATUS();
	}
	else {
		status = writeToStream(&stream, 0);
		CHECK_QSTATUS();
	}
	
	MessageIdParser messageId(&status);
	CHECK_QSTATUS();
	status = msg.getField(L"Message-Id", &messageId, &field);
	CHECK_QSTATUS();
	if (field == Part::FIELD_EXIST) {
		status = writeToStream(&stream, messageId.getMessageId());
		CHECK_QSTATUS();
	}
	else {
		status = writeToStream(&stream, 0);
		CHECK_QSTATUS();
	}
	
	string_ptr<WSTRING> wstrReference;
	status = PartUtil(msg).getReference(&wstrReference);
	CHECK_QSTATUS();
	if (field == Part::FIELD_EXIST) {
		status = writeToStream(&stream, wstrReference.get());
		CHECK_QSTATUS();
	}
	else {
		status = writeToStream(&stream, 0);
		CHECK_QSTATUS();
	}
	
	status = stream.write(reinterpret_cast<const unsigned char*>("\r\n"), 2);
	CHECK_QSTATUS();
	
	*pnLen = stream.getLength();
	*ppData = stream.releaseBuffer();
	
	return QSTATUS_SUCCESS;
}

QSTATUS qm::MessageCache::getData(MessageCacheKey key,
	MessageCacheItem item, WSTRING* pwstrData)
{
	assert(pwstrData);
	
	DECLARE_QSTATUS();
	
	Lock<CriticalSection> lock(cs_);
	
	CacheItem* pItem = 0;
	if (pLastGotten_ && pLastGotten_->getKey() == key)
		pItem = pLastGotten_;
	if (!pItem) {
		ItemMap::iterator it = map_.find(key);
		if (it != map_.end())
			pItem = (*it).second;
	}
	if (pItem) {
		*pwstrData = allocWString(pItem->getItem(item));
		if (!*pwstrData)
			return QSTATUS_OUTOFMEMORY;
		
		if (pItem != pNewFirst_) {
			CacheItem* pNext = pItem->pNewNext_;
			CacheItem* pPrev = pItem->pNewPrev_;
			pNext->pNewPrev_ = pPrev;
			pPrev->pNewNext_ = pNext;
			
			CacheItem* pFirst = pNewFirst_;
			pNewFirst_ = pItem;
			pItem->pNewPrev_ = pNewLast_;
			pItem->pNewNext_ = pFirst;
			pFirst->pNewPrev_ = pItem;
		}
	}
	else {
		unsigned char* p = 0;
		status = pMessageStore_->readCache(key, &p);
		CHECK_QSTATUS();
		malloc_ptr<unsigned char> pFree(p);
		
		p += sizeof(size_t);
		
		union {
			char buf_[sizeof(size_t)];
			size_t n_;
		} x;
		UTF8Converter converter(&status);
		CHECK_QSTATUS();
		if (nMaxSize_ != 0) {
			int n = 0;
			string_ptr<WSTRING> items[ITEM_MAX];
			for (n = 0; n < ITEM_MAX; ++n) {
//				size_t nLen = *reinterpret_cast<size_t*>(p);
				memcpy(x.buf_, p, sizeof(size_t));
				size_t nLen = x.n_;
				p += sizeof(size_t);
				size_t nDecodedLen = 0;
				status = converter.decode(reinterpret_cast<CHAR*>(p),
					&nLen, &items[n], &nDecodedLen);
				CHECK_QSTATUS();
				p += nLen;
			}
			WSTRING i[ITEM_MAX];
			for (n = 0; n < ITEM_MAX; ++n)
				i[n] = items[n].get();
			status = newObject(key, i, &pItem);
			CHECK_QSTATUS();
			for (n = 0; n < ITEM_MAX; ++n)
				items[n].release();
			
			insert(pItem);
			
			if (nSize_ >= nMaxSize_) {
				std::auto_ptr<CacheItem> pRemove(pNewLast_->pNewPrev_);
				ItemMap::iterator it = map_.find(pRemove->getKey());
				assert(it != map_.end());
				remove(it);
			}
			else {
				++nSize_;
			}
			
			*pwstrData = allocWString(pItem->getItem(item));
		}
		else {
			for (int n = 0; n < item; ++n) {
//				size_t nLen = *reinterpret_cast<size_t*>(p);
				memcpy(x.buf_, p, sizeof(size_t));
				size_t nLen = x.n_;
				p += sizeof(size_t) + nLen;
			}
//			size_t nLen = *reinterpret_cast<size_t*>(p);
			memcpy(x.buf_, p, sizeof(size_t));
			size_t nLen = x.n_;
			p += sizeof(size_t);
			size_t nDecodedLen = 0;
			status = converter.decode(reinterpret_cast<CHAR*>(p),
				&nLen, pwstrData, &nDecodedLen);
			CHECK_QSTATUS();
		}
	}
	if (pItem)
		pLastGotten_ = pItem;
	
	return QSTATUS_SUCCESS;
}

void qm::MessageCache::removeData(MessageCacheKey key)
{
	ItemMap::iterator it = map_.find(key);
	if (it != map_.end())
		remove(it);
}

QSTATUS qm::MessageCache::writeToStream(
	OutputStream* pStream, const WCHAR* pwsz)
{
	assert(pStream);
	
	DECLARE_QSTATUS();
	
	if (!pwsz)
		pwsz = L"";
	
	UTF8Converter converter(&status);
	CHECK_QSTATUS();
	size_t nLen = wcslen(pwsz);
	string_ptr<STRING> str;
	size_t nEncodedLen = 0;
	status = converter.encode(pwsz, &nLen, &str, &nEncodedLen);
	CHECK_QSTATUS();
	status = pStream->write(reinterpret_cast<unsigned char*>(&nEncodedLen),
		sizeof(nEncodedLen));
	CHECK_QSTATUS();
	status = pStream->write(reinterpret_cast<unsigned char*>(str.get()),
		nEncodedLen);
	CHECK_QSTATUS();
	
	return QSTATUS_SUCCESS;
}

void qm::MessageCache::insert(CacheItem* pItem)
{
	map_.insert(std::make_pair(pItem->getKey(), pItem));
	
	pNewFirst_->pNewPrev_ = pItem;
	pItem->pNewNext_ = pNewFirst_;
	pNewFirst_ = pItem;
}

void qm::MessageCache::remove(ItemMap::iterator it)
{
	assert(it != map_.end());
	
	CacheItem* pItem = (*it).second;
	
#if _MSC_VER == 1202 && defined MIPS
	map_.erase((*it).first);
#else
	map_.erase(it);
#endif
	
	if (pItem == pNewFirst_)
		pNewFirst_ = pItem->pNewNext_;
	else
		pItem->pNewPrev_->pNewNext_ = pItem->pNewNext_;
	pItem->pNewNext_->pNewPrev_ = pItem->pNewPrev_;
	
	if (pItem == pLastGotten_)
		pLastGotten_ = 0;
	
	delete pItem;
}


/****************************************************************************
 *
 * CacheItem
 *
 */

qm::CacheItem::CacheItem(MessageCacheKey key) :
	key_(key),
	pNewNext_(0),
	pNewPrev_(0)
{
	memset(items_, 0, sizeof(items_));
}

qm::CacheItem::CacheItem(MessageCacheKey key, const WSTRING* pItems) :
	key_(key),
	pNewNext_(0),
	pNewPrev_(0)
{
	memcpy(items_, pItems, ITEM_MAX*sizeof(WSTRING));
}

qm::CacheItem::~CacheItem()
{
	for (int n = 0; n < countof(items_); ++n)
		freeWString(items_[n]);
}

MessageCacheKey qm::CacheItem::getKey() const
{
	return key_;
}

const WCHAR* qm::CacheItem::getItem(MessageCacheItem item) const
{
	assert(item < ITEM_MAX);
	return items_[item];
}
