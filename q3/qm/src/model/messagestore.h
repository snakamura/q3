/*
 * $Id$
 *
 * Copyright(C) 1998-2003 Satoshi Nakamura
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
	virtual qs::QSTATUS close() = 0;
	virtual qs::QSTATUS flush() = 0;
	virtual qs::QSTATUS load(unsigned int nOffset,
		unsigned int nLength, Message* pMessage) = 0;
	virtual qs::QSTATUS save(const CHAR* pszMessage,
		const Message& header, MessageCache* pMessageCache,
		bool bIndexOnly, unsigned int* pnOffset, unsigned int* pnLength,
		unsigned int* pnHeaderLength, MessageCacheKey* pKey) = 0;
	virtual qs::QSTATUS free(unsigned int nOffset,
		unsigned int nLength, MessageCacheKey key) = 0;
	virtual qs::QSTATUS compact(unsigned int nOffset, unsigned int nLength,
		MessageCacheKey key, MessageStore* pmsOld,
		unsigned int* pnOffset, MessageCacheKey* pKey) = 0;
	virtual qs::QSTATUS freeUnrefered(const ReferList& listRefer) = 0;
	virtual qs::QSTATUS freeUnused() = 0;
	virtual qs::QSTATUS readCache(MessageCacheKey key, unsigned char** ppBuf) = 0;
};


/****************************************************************************
 *
 * SingleMessageStore
 *
 */

class SingleMessageStore : public MessageStore
{
public:
	SingleMessageStore(const WCHAR* pwszPath, unsigned int nBlockSize,
		unsigned int nCacheBlockSize, qs::QSTATUS* pstatus);
	virtual ~SingleMessageStore();

public:
	virtual qs::QSTATUS close();
	virtual qs::QSTATUS flush();
	virtual qs::QSTATUS load(unsigned int nOffset,
		unsigned int nLength, Message* pMessage);
	virtual qs::QSTATUS save(const CHAR* pszMessage,
		const Message& header, MessageCache* pMessageCache,
		bool bIndexOnly, unsigned int* pnOffset, unsigned int* pnLength,
		unsigned int* pnHeaderLength, MessageCacheKey* pKey);
	virtual qs::QSTATUS free(unsigned int nOffset,
		unsigned int nLength, MessageCacheKey key);
	virtual qs::QSTATUS compact(unsigned int nOffset, unsigned int nLength,
		MessageCacheKey key, MessageStore* pmsOld,
		unsigned int* pnOffset, MessageCacheKey* pKey);
	virtual qs::QSTATUS freeUnrefered(const ReferList& listRefer);
	virtual qs::QSTATUS freeUnused();
	virtual qs::QSTATUS readCache(MessageCacheKey key, unsigned char** ppBuf);

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
		unsigned int nCacheBlockSize, qs::QSTATUS* pstatus);
	virtual ~MultiMessageStore();

public:
	virtual qs::QSTATUS close();
	virtual qs::QSTATUS flush();
	virtual qs::QSTATUS load(unsigned int nOffset,
		unsigned int nLength, Message* pMessage);
	virtual qs::QSTATUS save(const CHAR* pszMessage,
		const Message& header, MessageCache* pMessageCache,
		bool bIndexOnly, unsigned int* pnOffset, unsigned int* pnLength,
		unsigned int* pnHeaderLength, MessageCacheKey* pKey);
	virtual qs::QSTATUS free(unsigned int nOffset,
		unsigned int nLength, MessageCacheKey key);
	virtual qs::QSTATUS compact(unsigned int nOffset, unsigned int nLength,
		MessageCacheKey key, MessageStore* pmsOld,
		unsigned int* pnOffset, MessageCacheKey* pKey);
	virtual qs::QSTATUS freeUnrefered(const ReferList& listRefer);
	virtual qs::QSTATUS freeUnused();
	virtual qs::QSTATUS readCache(MessageCacheKey key, unsigned char** ppBuf);

private:
	MultiMessageStore(const MultiMessageStore&);
	MultiMessageStore& operator=(const MultiMessageStore&);

private:
	struct MultiMessageStoreImpl* pImpl_;
};


/****************************************************************************
 *
 * MessageStoreUtil
 *
 */

class MessageStoreUtil
{
public:
	static qs::QSTATUS freeUnrefered(qs::ClusterStorage* pStorage,
		const MessageStore::ReferList& listRefer, unsigned int nSeparatorSize);
	static qs::QSTATUS freeUnreferedCache(qs::ClusterStorage* pCacheStorage,
		const MessageStore::ReferList& listRefer);
};

}

#endif // __MESSAGESTORE_H__
