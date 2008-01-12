/*
 * $Id$
 *
 * Copyright(C) 1998-2008 Satoshi Nakamura
 *
 */

#ifndef __MESSAGESTORE_H__
#define __MESSAGESTORE_H__

#include <qmmessage.h>
#include <qmmessageindex.h>
#include <qmmessageoperation.h>

#include <qs.h>
#include <qsclusterstorage.h>
#include <qsstring.h>


namespace qm {

class MessageStore;
	class SingleMessageStore;
	class MultiMessageStore;
class MessageStoreSalvageCallback;
class MessageStoreCheckCallback;
class MessageStoreUtil;


/****************************************************************************
 *
 * MessageStore
 *
 */

class MessageStore
{
public:
	struct Data
	{
		unsigned int nOffset_;
		unsigned int nLength_;
		unsigned int nIndexKey_;
		unsigned int nIndexLength_;
	};

public:
	typedef std::vector<Data> DataList;

public:
	virtual ~MessageStore();

public:
	virtual bool close() = 0;
	virtual bool flush() = 0;
	virtual bool load(unsigned int nOffset,
					  unsigned int nLength,
					  Message* pMessage) = 0;
	virtual bool save(const Message& header,
					  const CHAR* pszBody,
					  size_t nBodyLen,
					  const WCHAR* pwszLabel,
					  bool bIndexOnly,
					  unsigned int* pnOffset,
					  unsigned int* pnLength,
					  unsigned int* pnHeaderLength,
					  unsigned int* pnIndexKey,
					  unsigned int* pnIndexLength) = 0;
	virtual bool saveDecoded(unsigned int nOffset,
							 const Message& msg) = 0;
	virtual bool updateIndex(unsigned int nOldIndexKey,
							 unsigned int nOldIndexLength,
							 const unsigned char* pIndex,
							 unsigned int nIndexLength,
							 unsigned int* pnIndexKey) = 0;
	virtual bool free(unsigned int nOffset,
					  unsigned int nLength,
					  unsigned int nIndexKey,
					  unsigned int nIndexLength) = 0;
	virtual bool compact(DataList* pListData,
						 MessageOperationCallback* pCallback) = 0;
	virtual bool salvage(const DataList& listData,
						 MessageStoreSalvageCallback* pCallback) = 0;
	virtual bool isSalvageSupported() const = 0;
	virtual bool check(MessageStoreCheckCallback* pCallback) = 0;
	virtual bool freeUnused() = 0;
	virtual qs::malloc_ptr<unsigned char> readIndex(unsigned int nKey,
													unsigned int nLength) = 0;

public:
	bool save(const CHAR* pszMessage,
			  size_t nLen,
			  const Message* pHeader,
			  const WCHAR* pwszLabel,
			  bool bIndexOnly,
			  unsigned int* pnOffset,
			  unsigned int* pnLength,
			  unsigned int* pnHeaderLength,
			  unsigned int* pnIndexKey,
			  unsigned int* pnIndexLength);
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
					   const WCHAR* pwszIndexPath,
					   unsigned int nIndexBlockSize);
	virtual ~SingleMessageStore();

public:
	virtual bool close();
	virtual bool flush();
	virtual bool load(unsigned int nOffset,
					  unsigned int nLength,
					  Message* pMessage);
	virtual bool save(const Message& header,
					  const CHAR* pszBody,
					  size_t nBodyLen,
					  const WCHAR* pwszLabel,
					  bool bIndexOnly,
					  unsigned int* pnOffset,
					  unsigned int* pnLength,
					  unsigned int* pnHeaderLength,
					  unsigned int* pnIndexKey,
					  unsigned int* pnIndexLength);
	virtual bool saveDecoded(unsigned int nOffset,
							 const Message& msg);
	virtual bool updateIndex(unsigned int nOldIndexKey,
							 unsigned int nOldIndexLength,
							 const unsigned char* pIndex,
							 unsigned int nIndexLength,
							 unsigned int* pnIndexKey);
	virtual bool free(unsigned int nOffset,
					  unsigned int nLength,
					  unsigned int nIndexKey,
					  unsigned int nIndexLength);
	virtual bool compact(DataList* pListData,
						 MessageOperationCallback* pCallback);
	virtual bool salvage(const DataList& listData,
						 MessageStoreSalvageCallback* pCallback);
	virtual bool isSalvageSupported() const;
	virtual bool check(MessageStoreCheckCallback* pCallback);
	virtual bool freeUnused();
	virtual qs::malloc_ptr<unsigned char> readIndex(unsigned int nKey,
													unsigned int nLength);

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
					  const WCHAR* pwszIndexPath,
					  unsigned int nIndexBlockSize);
	virtual ~MultiMessageStore();

public:
	virtual bool close();
	virtual bool flush();
	virtual bool load(unsigned int nOffset,
					  unsigned int nLength,
					  Message* pMessage);
	virtual bool save(const Message& header,
					  const CHAR* pszBody,
					  size_t nBodyLen,
					  const WCHAR* pwszLabel,
					  bool bIndexOnly,
					  unsigned int* pnOffset,
					  unsigned int* pnLength,
					  unsigned int* pnHeaderLength,
					  unsigned int* pnIndexKey,
					  unsigned int* pnIndexLength);
	virtual bool saveDecoded(unsigned int nOffset,
							 const Message& msg);
	virtual bool updateIndex(unsigned int nOldIndexKey,
							 unsigned int nOldIndexLength,
							 const unsigned char* pIndex,
							 unsigned int nIndexLength,
							 unsigned int* pnIndexKey);
	virtual bool free(unsigned int nOffset,
					  unsigned int nLength,
					  unsigned int nIndexKey,
					  unsigned int nIndexLength);
	virtual bool compact(DataList* pListData,
						 MessageOperationCallback* pCallback);
	virtual bool salvage(const DataList& listData,
						 MessageStoreSalvageCallback* pCallback);
	virtual bool isSalvageSupported() const;
	virtual bool check(MessageStoreCheckCallback* pCallback);
	virtual bool freeUnused();
	virtual qs::malloc_ptr<unsigned char> readIndex(unsigned int nKey,
													unsigned int nLength);

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
	virtual void setCount(size_t nCount) = 0;
	virtual void step(size_t nStep) = 0;
	virtual bool salvage(const Message& msg) = 0;
};


/****************************************************************************
 *
 * MessageStoreCheckCallback
 *
 */

class MessageStoreCheckCallback
{
public:
	virtual ~MessageStoreCheckCallback();

public:
	virtual unsigned int getCount() = 0;
	virtual bool getHeader(unsigned int n,
						   Message* pMessage) = 0;
	virtual qs::wstring_ptr getLabel(unsigned int n) = 0;
	virtual void setKey(unsigned int n,
						unsigned int nKey,
						unsigned int nLength) = 0;
	virtual bool isIgnoreError(unsigned int n) = 0;
};


/****************************************************************************
 *
 * MessageStoreUtil
 *
 */

class MessageStoreUtil
{
public:
	static void freeUnrefered(qs::ClusterStorage* pStorage,
							  const MessageStore::DataList& listData,
							  unsigned int nSeparatorSize);
	static std::auto_ptr<qs::ClusterStorage> checkIndex(qs::ClusterStorage* pStorage,
														const WCHAR* pwszPath,
														unsigned int nBlockSize,
														MessageStoreCheckCallback* pCallback);
	static qs::malloc_ptr<unsigned char> readIndex(qs::ClusterStorage* pStorage,
												   unsigned int nKey,
												   unsigned int nLength);
	static bool updateIndex(qs::ClusterStorage* pStorage,
							unsigned int nOldIndexKey,
							unsigned int nOldIndexLength,
							const unsigned char* pIndex,
							unsigned int nIndexLength,
							unsigned int* pnIndexKey);
};

}

#endif // __MESSAGESTORE_H__
