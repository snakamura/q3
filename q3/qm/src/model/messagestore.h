/*
 * $Id$
 *
 * Copyright(C) 1998-2004 Satoshi Nakamura
 * All rights reserved.
 *
 */

#ifndef __MESSAGESTORE_H__
#define __MESSAGESTORE_H__

#include <qmmessage.h>
#include <qmmessagecache.h>

#include <qs.h>
#include <qsclusterstorage.h>
#include <qsstring.h>


namespace qm {

class MessageStore;
	class SingleMessageStore;
	class MultiMessageStore;
class MessageStoreSalvageCallback;
class MessageStoreUtil;

class MessageCache;


/****************************************************************************
 *
 * MessageStore
 *
 */

class MessageStore
{
public:
	struct Refer {
		unsigned int nOffset_;
		unsigned int nLength_;
		MessageCacheKey key_;
	};

public:
	typedef std::vector<Refer> ReferList;

public:
	virtual ~MessageStore();

public:
	virtual bool close() = 0;
	virtual bool flush() = 0;
	virtual bool load(unsigned int nOffset,
					  unsigned int nLength,
					  Message* pMessage) = 0;
	virtual bool save(const CHAR* pszMessage,
					  const Message& header,
					  MessageCache* pMessageCache,
					  bool bIndexOnly,
					  unsigned int* pnOffset,
					  unsigned int* pnLength,
					  unsigned int* pnHeaderLength,
					  MessageCacheKey* pKey) = 0;
	virtual bool free(unsigned int nOffset,
					  unsigned int nLength,
					  MessageCacheKey key) = 0;
	virtual bool compact(unsigned int nOffset,
						 unsigned int nLength,
						 MessageCacheKey key,
						 MessageStore* pmsOld,
						 unsigned int* pnOffset,
						 MessageCacheKey* pKey) = 0;
	virtual bool freeUnused() = 0;
	virtual bool freeUnrefered(const ReferList& listRefer) = 0;
	virtual bool salvage(const ReferList& listRefer,
						 MessageStoreSalvageCallback* pCallback) = 0;
	virtual qs::malloc_ptr<unsigned char> readCache(MessageCacheKey key) = 0;
};


/****************************************************************************
 *
 * SingleMessageStore
 *
 */

class SingleMessageStore : public MessageStore
{
public:
	SingleMessageStore(const WCHAR* pwszPath,
					   unsigned int nBlockSize,
					   const WCHAR* pwszCachePath,
					   unsigned int nCacheBlockSize);
	virtual ~SingleMessageStore();

public:
	virtual bool close();
	virtual bool flush();
	virtual bool load(unsigned int nOffset,
					  unsigned int nLength,
					  Message* pMessage);
	virtual bool save(const CHAR* pszMessage,
					  const Message& header,
					  MessageCache* pMessageCache,
					  bool bIndexOnly,
					  unsigned int* pnOffset,
					  unsigned int* pnLength,
					  unsigned int* pnHeaderLength,
					  MessageCacheKey* pKey);
	virtual bool free(unsigned int nOffset,
					  unsigned int nLength,
					  MessageCacheKey key);
	virtual bool compact(unsigned int nOffset,
						 unsigned int nLength,
						 MessageCacheKey key,
						 MessageStore* pmsOld,
						 unsigned int* pnOffset,
						 MessageCacheKey* pKey);
	virtual bool freeUnused();
	virtual bool freeUnrefered(const ReferList& listRefer);
	virtual bool salvage(const ReferList& listRefer,
						 MessageStoreSalvageCallback* pCallback);
	virtual qs::malloc_ptr<unsigned char> readCache(MessageCacheKey key);

private:
	SingleMessageStore(const SingleMessageStore&);
	SingleMessageStore& operator=(const SingleMessageStore&);

private:
	struct SingleMessageStoreImpl* pImpl_;
};


/****************************************************************************
 *
 * MultiMessageStore
 *
 */

class MultiMessageStore : public MessageStore
{
public:
	MultiMessageStore(const WCHAR* pwszPath,
					  const WCHAR* pwszCachePath,
					  unsigned int nCacheBlockSize);
	virtual ~MultiMessageStore();

public:
	virtual bool close();
	virtual bool flush();
	virtual bool load(unsigned int nOffset,
					  unsigned int nLength,
					  Message* pMessage);
	virtual bool save(const CHAR* pszMessage,
					  const Message& header,
					  MessageCache* pMessageCache,
					  bool bIndexOnly,
					  unsigned int* pnOffset,
					  unsigned int* pnLength,
					  unsigned int* pnHeaderLength,
					  MessageCacheKey* pKey);
	virtual bool free(unsigned int nOffset,
					  unsigned int nLength,
					  MessageCacheKey key);
	virtual bool compact(unsigned int nOffset,
						 unsigned int nLength,
						 MessageCacheKey key,
						 MessageStore* pmsOld,
						 unsigned int* pnOffset,
						 MessageCacheKey* pKey);
	virtual bool freeUnused();
	virtual bool freeUnrefered(const ReferList& listRefer);
	virtual bool salvage(const ReferList& listRefer,
						 MessageStoreSalvageCallback* pCallback);
	virtual qs::malloc_ptr<unsigned char> readCache(MessageCacheKey key);

private:
	MultiMessageStore(const MultiMessageStore&);
	MultiMessageStore& operator=(const MultiMessageStore&);

private:
	struct MultiMessageStoreImpl* pImpl_;
};


/****************************************************************************
 *
 * MessageStoreSalvageCallback
 *
 */

class MessageStoreSalvageCallback
{
public:
	virtual ~MessageStoreSalvageCallback();

public:
	virtual bool salvage(const Message& msg) = 0;
};


/****************************************************************************
 *
 * MessageStoreUtil
 *
 */

class MessageStoreUtil
{
public:
	static bool freeUnrefered(qs::ClusterStorage* pStorage,
							  const MessageStore::ReferList& listRefer,
							  unsigned int nSeparatorSize);
	static bool freeUnreferedCache(qs::ClusterStorage* pCacheStorage,
								   const MessageStore::ReferList& listRefer);
};

}

#endif // __MESSAGESTORE_H__
