/*
 * $Id$
 *
 * Copyright(C) 1998-2004 Satoshi Nakamura
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
	explicit MessageCache(MessageStore* pMessageStore);
	~MessageCache();

public:
	qs::wstring_ptr getData(MessageCacheKey key,
							MessageCacheItem item);
	void removeData(MessageCacheKey key);

public:
	static qs::malloc_size_ptr<unsigned char> createData(const Message& msg);

private:
	void insert(CacheItem* pItem);
	void remove(ItemMap::iterator it);

private:
	static bool writeToStream(qs::OutputStream* pStream,
							  const WCHAR* pwsz);

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
	CacheItem(MessageCacheKey key,
			  qs::wxstring_ptr wstr,
			  const WCHAR* pwszItem[]);
	~CacheItem();

public:
	MessageCacheKey getKey() const;
	const WCHAR* getItem(MessageCacheItem item) const;

private:
	CacheItem(const CacheItem&);
	CacheItem& operator=(const CacheItem&);

private:
	MessageCacheKey key_;
	qs::wxstring_ptr wstr_;
	const WCHAR* pwszItem_[ITEM_MAX];
	CacheItem* pNewNext_;
	CacheItem* pNewPrev_;

	friend class MessageCache;
	friend class CacheItemIterator;
};

}

#endif // __MESSAGECACHE_H__
