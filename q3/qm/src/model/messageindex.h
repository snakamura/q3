/*
 * $Id$
 *
 * Copyright(C) 1998-2008 Satoshi Nakamura.
 *
 */

#ifndef __MESSAGEINDEX_H__
#define __MESSAGEINDEX_H__

#include <qmmessageindex.h>

#include <qsstl.h>
#include <qsstream.h>
#include <qsstring.h>
#include <qsthread.h>

#include <hash_map>


namespace qm {

class MessageIndex;
class MessageIndexItem;

class Message;
class MessageStore;


/****************************************************************************
 *
 * MessageIndex
 *
 */

class MessageIndex
{
public:
	typedef std::hash_map<unsigned int, MessageIndexItem*> ItemMap;

public:
	MessageIndex(MessageStore* pMessageStore,
				 size_t nMaxSize);
	~MessageIndex();

public:
	qs::wstring_ptr get(unsigned int nKey,
						unsigned int nLength,
						MessageIndexName name);
	void remove(unsigned int nKey);
	bool isPrepared(unsigned int nKey) const;
	void prepare(unsigned int nKey,
				 unsigned int nLength);
	qs::malloc_size_ptr<unsigned char> createReplacedIndex(unsigned int nKey,
														   unsigned int nLength,
														   MessageIndexName name,
														   const WCHAR* pwszValue);

public:
	static qs::malloc_size_ptr<unsigned char> createIndex(const Message& header,
														  const WCHAR* pwszLabel);

private:
	MessageIndexItem* getItem(unsigned int nKey) const;
	void insert(std::auto_ptr<MessageIndexItem> pItem);
	void remove(ItemMap::iterator it);

private:
	static void parseValues(WCHAR* p,
							size_t nLen,
							const WCHAR** ppwszValues);
	static bool writeToStream(qs::OutputStream* pStream,
							  const WCHAR* pwsz);

private:
	MessageIndex(const MessageIndex&);
	MessageIndex& operator=(const MessageIndex&);

private:
	MessageStore* pMessageStore_;
	size_t nMaxSize_;
	ItemMap map_;
	MessageIndexItem* pNewFirst_;
	MessageIndexItem* pNewLast_;
	MessageIndexItem* pLastGotten_;
};


/****************************************************************************
 *
 * MessageIndexItem
 *
 */

class MessageIndexItem
{
public:
	MessageIndexItem(unsigned int nKey);
	MessageIndexItem(unsigned int nKey,
					 qs::malloc_ptr<unsigned char> pData,
					 const WCHAR* pwszValues[]);
	~MessageIndexItem();

public:
	unsigned int getKey() const;
	const WCHAR* getValue(MessageIndexName name) const;

private:
	MessageIndexItem(const MessageIndexItem&);
	MessageIndexItem& operator=(const MessageIndexItem&);

private:
	unsigned int nKey_;
	qs::malloc_ptr<unsigned char> pData_;
	const WCHAR* pwszValues_[NAME_MAX];
	MessageIndexItem* pNewNext_;
	MessageIndexItem* pNewPrev_;
	
	friend class MessageIndex;
};

}

#endif // __MESSAGEINDEX_H__
