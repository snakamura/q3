/*
 * $Id$
 *
 * Copyright(C) 1998-2004 Satoshi Nakamura
 * All rights reserved.
 *
 */

#include <qsconv.h>
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

qm::MessageCache::MessageCache(MessageStore* pMessageStore) :
	pMessageStore_(pMessageStore),
	nMaxSize_(-1),
	nSize_(0),
	pNewFirst_(0),
	pNewLast_(0),
	pLastGotten_(0)
{
	pNewFirst_ = new CacheItem(-1);
	pNewFirst_->pNewNext_ = pNewFirst_;
	pNewFirst_->pNewPrev_ = pNewFirst_;
	pNewLast_ = pNewFirst_;
}

qm::MessageCache::~MessageCache()
{
#if defined _WIN32_WCE && _MSC_VER == 1202 && defined MIPS
	for (ItemMap::const_iterator it = map_.begin(); it != map_.end(); ++it)
		delete (*it).second;
#else
	std::for_each(map_.begin(), map_.end(),
		unary_compose_f_gx(
			deleter<CacheItem>(),
			std::select2nd<ItemMap::value_type>()));
#endif
	delete pNewLast_;
}

wstring_ptr qm::MessageCache::getData(MessageCacheKey key,
									  MessageCacheItem item)
{
	Lock<CriticalSection> lock(cs_);
	
	wstring_ptr wstrData;
	
	CacheItem* pItem = 0;
	if (pLastGotten_ && pLastGotten_->getKey() == key)
		pItem = pLastGotten_;
	if (!pItem) {
		ItemMap::iterator it = map_.find(key);
		if (it != map_.end())
			pItem = (*it).second;
	}
	if (pItem) {
		wstrData = allocWString(pItem->getItem(item));
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
		malloc_ptr<unsigned char> pData(pMessageStore_->readCache(key));
		if (!pData.get())
			return 0;
		unsigned char* p = pData.get();
		
		p += sizeof(size_t);
		
		UTF8Converter converter;
		if (nMaxSize_ != 0) {
			unsigned char* pOrg = p;
			for (int n = 0; n < ITEM_MAX; ++n) {
				size_t nLen = 0;
				memcpy(&nLen, p, sizeof(nLen));
				for (int m = 0; m < sizeof(size_t); ++m)
					*p++ = 0;
				p += nLen;
			}
			size_t nDecode = p - pOrg;
			wxstring_size_ptr strDecoded(converter.decode(
				reinterpret_cast<CHAR*>(pOrg), &nDecode));
			const WCHAR* pw = strDecoded.get();
			const WCHAR* pwszItem[ITEM_MAX];
			for (int n = 0; n < ITEM_MAX; ++n) {
				while (*pw != L'\0')
					++pw;
				for (int m = 0; m < sizeof(size_t); ++m)
					++pw;
				pwszItem[n] = pw;
			}
			pItem = new CacheItem(key, strDecoded.release(), pwszItem);
			
			insert(pItem);
			
			if (nSize_ >= nMaxSize_)
				removeData(pNewLast_->pNewPrev_->getKey());
			else
				++nSize_;
			
			wstrData = allocWString(pItem->getItem(item));
		}
		else {
			for (int n = 0; n < item; ++n) {
				size_t nLen = 0;
				memcpy(&nLen, p, sizeof(nLen));
				p += sizeof(size_t) + nLen;
			}
			size_t nLen = 0;
			memcpy(&nLen, p, sizeof(nLen));
			p += sizeof(size_t);
			wxstring_size_ptr strDecoded(converter.decode(
				reinterpret_cast<CHAR*>(p), &nLen));
			if (!strDecoded.get())
				return 0;
			wstrData.reset(strDecoded.release());
		}
	}
	if (pItem)
		pLastGotten_ = pItem;
	
	return wstrData;
}

void qm::MessageCache::removeData(MessageCacheKey key)
{
	Lock<CriticalSection> lock(cs_);
	
	ItemMap::iterator it = map_.find(key);
	if (it != map_.end())
		remove(it);
}

malloc_size_ptr<unsigned char> qm::MessageCache::createData(const Message& msg)
{
	ByteOutputStream stream;
	
	const WCHAR* pwszFields[] = {
		L"From",
		L"To"
	};
	for (int n = 0; n < countof(pwszFields); ++n) {
		AddressListParser address(0);
		if (msg.getField(pwszFields[n], &address) == Part::FIELD_EXIST) {
			wstring_ptr wstrNames(address.getNames());
			if (!writeToStream(&stream, wstrNames.get()))
				return malloc_size_ptr<unsigned char>();
		}
		else {
			if (!writeToStream(&stream, 0))
				return malloc_size_ptr<unsigned char>();
		}
	}
	
	UnstructuredParser subject;
	if (msg.getField(L"Subject", &subject) == Part::FIELD_EXIST) {
		if (!writeToStream(&stream, subject.getValue()))
			return malloc_size_ptr<unsigned char>();
	}
	else {
		if (!writeToStream(&stream, 0))
			return malloc_size_ptr<unsigned char>();
	}
	
	MessageIdParser messageId;
	if (msg.getField(L"Message-Id", &messageId) == Part::FIELD_EXIST) {
		if (!writeToStream(&stream, messageId.getMessageId()))
			return malloc_size_ptr<unsigned char>();
	}
	else {
		if (!writeToStream(&stream, 0))
			return malloc_size_ptr<unsigned char>();
	}
	
	wstring_ptr wstrReference(PartUtil(msg).getReference());
	if (wstrReference.get()) {
		if (!writeToStream(&stream, wstrReference.get()))
			return malloc_size_ptr<unsigned char>();
	}
	else {
		if (!writeToStream(&stream, 0))
			return malloc_size_ptr<unsigned char>();
	}
	
	if (stream.write(reinterpret_cast<const unsigned char*>("\r\n"), 2) == -1)
		return malloc_size_ptr<unsigned char>();
	
	size_t nLen = stream.getLength();
	return malloc_size_ptr<unsigned char>(stream.releaseBuffer(), nLen);
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
	
#if defined _WIN32_WCE && _MSC_VER == 1202 && defined MIPS
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

bool qm::MessageCache::writeToStream(OutputStream* pStream,
									 const WCHAR* pwsz)
{
	assert(pStream);
	
	if (!pwsz)
		pwsz = L"";
	
	UTF8Converter converter;
	size_t nLen = wcslen(pwsz);
	xstring_size_ptr str(converter.encode(pwsz, &nLen));
	if (!str.get())
		return false;
	size_t nEncodedLen = str.size();
	if (pStream->write(reinterpret_cast<unsigned char*>(&nEncodedLen), sizeof(nEncodedLen)) == -1)
		return false;
	if (pStream->write(reinterpret_cast<unsigned char*>(str.get()), nEncodedLen) == -1)
		return false;
	
	return true;
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
	memset(pwszItem_, 0, sizeof(pwszItem_));
}

qm::CacheItem::CacheItem(MessageCacheKey key,
						 wxstring_ptr wstr,
						 const WCHAR* pwszItem[]) :
	key_(key),
	wstr_(wstr),
	pNewNext_(0),
	pNewPrev_(0)
{
	memcpy(pwszItem_, pwszItem, sizeof(pwszItem_));
}

qm::CacheItem::~CacheItem()
{
}

MessageCacheKey qm::CacheItem::getKey() const
{
	return key_;
}

const WCHAR* qm::CacheItem::getItem(MessageCacheItem item) const
{
	assert(item < ITEM_MAX);
	return pwszItem_[item];
}
