/*
 * $Id$
 *
 * Copyright(C) 1998-2003 Satoshi Nakamura
 * All rights reserved.
 *
 */

#ifndef __MESSAGECACHE_H__
#define __MESSAGECACHE_H__

#include <qmmessagecache.h>

#include <qs.h>
#include <qsstream.h>
#include <qsstring.h>
#include <qsthread.h>

#include <hash_map>


namespace qm {

class MessageCache;
class CacheItem;
class CacheItemIterator;

class Message;
class MessageStore;


/****************************************************************************
 *
 * MessageCache
 *
 */

class MessageCache
{
public:
	typedef std::hash_map<MessageCacheKey, CacheItem*> ItemMap;

public:
	MessageCache(MessageStore* pMessageStore, qs::QSTATUS* pstatus);
	~MessageCache();

public:
	qs::QSTATUS getData(MessageCacheKey key,
		MessageCacheItem item, qs::WSTRING* pwstrData);
	void removeData(MessageCacheKey key);

public:
	static qs::QSTATUS createData(const Message& msg,
		unsigned char** ppData, size_t* pnLen);

private:
	void insert(CacheItem* pItem);
	void remove(ItemMap::iterator it);

private:
	static qs::QSTATUS writeToStream(qs::OutputStream* pStream, const WCHAR* pwsz);

private:
	MessageCache(const MessageCache&);
	MessageCache& operator=(const MessageCache&);

private:
	MessageStore* pMessageStore_;
	size_t nMaxSize_;
	size_t nSize_;
	ItemMap map_;
	CacheItem* pNewFirst_;
	CacheItem* pNewLast_;
	CacheItem* pLastGotten_;
	qs::CriticalSection cs_;
};


/****************************************************************************
 *
 * CacheItem
 *
 */

class CacheItem
{
public:
	CacheItem(MessageCacheKey key);
	CacheItem(MessageCacheKey key, const qs::WSTRING* pItems);
	~CacheItem();

public:
	MessageCacheKey getKey() const;
	const WCHAR* getItem(MessageCacheItem item) const;

private:
	CacheItem(const CacheItem&);
	CacheItem& operator=(const CacheItem&);

private:
	MessageCacheKey key_;
	qs::WSTRING items_[ITEM_MAX];
	CacheItem* pNewNext_;
	CacheItem* pNewPrev_;

	friend class MessageCache;
	friend class CacheItemIterator;
};

}

#endif // __MESSAGECACHE_H__
