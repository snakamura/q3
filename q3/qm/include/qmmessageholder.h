/*
 * $Id$
 *
 * Copyright(C) 1998-2003 Satoshi Nakamura
 * All rights reserved.
 *
 */

#ifndef __QMMESSAGEHOLDER_H__
#define __QMMESSAGEHOLDER_H__

#include <qm.h>
#include <qmmessagecache.h>

#include <qs.h>
#include <qsstring.h>
#include <qsutil.h>

#include <vector>


namespace qm {

class MessageHolderBase;
	class MessageHolder;
	class AbstractMessageHolder;
class MessagePtr;
class MessagePtrLock;

class Account;
class NormalFolder;
class Message;


/****************************************************************************
 *
 * MessageHolderBase
 *
 */

class QMEXPORTCLASS MessageHolderBase
{
public:
	enum Flag {
		FLAG_SEEN			= 0x00000001,
		FLAG_REPLIED		= 0x00000002,
		FLAG_FORWARDED		= 0x00000004,
		FLAG_SENT			= 0x00000008,
		FLAG_DRAFT			= 0x00000010,
		FLAG_MARKED			= 0x00000020,
		FLAG_DELETED		= 0x00000040,
		FLAG_DOWNLOAD		= 0x00000080,
		FLAG_DOWNLOADTEXT	= 0x00000100,
		FLAG_TOME			= 0x00000200,
		FLAG_CCME			= 0x00000400,
		FLAG_USER1			= 0x00001000,
		FLAG_USER2			= 0x00002000,
		FLAG_USER3			= 0x00004000,
		FLAG_USER4			= 0x00008000,
		FLAG_USER_MASK		= 0x0000ffff,
		
		FLAG_MULTIPART		= 0x00010000,
		FLAG_TEXT			= 0x00020000,
		FLAG_NOTONSERVER	= 0x00040000,
		FLAG_LOCAL			= 0x00080000,
		FLAG_SYSTEM_MASK	= 0x000f0000,
		
		FLAG_INDEXONLY		= 0x01000000,
		FLAG_HEADERONLY		= 0x02000000,
		FLAG_TEXTONLY		= 0x04000000,
		FLAG_HTMLONLY		= 0x08000000,
		FLAG_PARTIAL_MASK	= 0x0f000000,
		
		FLAG_POPSDELETE		= 0x10000000,
		FLAG_ADDON_MASK		= 0xf0000000
	};

public:
	virtual ~MessageHolderBase();

public:
	virtual unsigned int getId() const = 0;
	virtual unsigned int getFlags() const = 0;
	virtual qs::QSTATUS getFrom(qs::WSTRING* pwstrFrom) const = 0;
	virtual qs::QSTATUS getTo(qs::WSTRING* pwstrTo) const = 0;
	virtual qs::QSTATUS getFromTo(qs::WSTRING* pwstrFromTo) const = 0;
	virtual qs::QSTATUS getSubject(qs::WSTRING* pwstrSubject) const = 0;
	virtual qs::QSTATUS getDate(qs::Time* pTime) const = 0;
	virtual unsigned int getSize() const = 0;
	virtual unsigned int getTextSize() const = 0;
	virtual NormalFolder* getFolder() const = 0;
	virtual Account* getAccount() const = 0;
	virtual qs::QSTATUS getMessage(unsigned int nFlags,
		const WCHAR* pwszField, Message* pMessage) = 0;
	virtual MessageHolder* getMessageHolder() = 0;
};


/****************************************************************************
 *
 * MessageHolder
 *
 */

class QMEXPORTCLASS MessageHolder : public MessageHolderBase
{
public:
	struct MessageBoxKey
	{
		unsigned int nOffset_;
		unsigned int nLength_;
		unsigned int nHeaderLength_;
	};
	
	struct Init
	{
		unsigned int nId_;
		unsigned int nFlags_;
		unsigned int nDate_;
		unsigned int nTime_;
		unsigned int nSize_;
		unsigned int nCacheKey_;
		unsigned int nOffset_;
		unsigned int nLength_;
		unsigned int nHeaderLength_;
	};

public:
	MessageHolder(NormalFolder* pFolder,
		const Init& init, qs::QSTATUS* pstatus);
	virtual ~MessageHolder();

public:
	void* operator new(size_t n);
	void operator delete(void* p);

public:
	virtual unsigned int getId() const;
	virtual unsigned int getFlags() const;
	virtual qs::QSTATUS getFrom(qs::WSTRING* pwstrFrom) const;
	virtual qs::QSTATUS getTo(qs::WSTRING* pwstrTo) const;
	virtual qs::QSTATUS getFromTo(qs::WSTRING* pwstrFromTo) const;
	virtual qs::QSTATUS getSubject(qs::WSTRING* pwstrSubject) const;
	virtual qs::QSTATUS getDate(qs::Time* pTime) const;
	virtual unsigned int getSize() const;
	virtual unsigned int getTextSize() const;
	virtual NormalFolder* getFolder() const;
	virtual Account* getAccount() const;
	virtual qs::QSTATUS getMessage(unsigned int nFlags,
		const WCHAR* pwszField, Message* pMessage);
	virtual MessageHolder* getMessageHolder();
	
public:
	bool isFlag(Flag flag) const;
	qs::QSTATUS getMessageId(qs::WSTRING* pwstrMessageId) const;
	unsigned int getMessageIdHash() const;
	qs::QSTATUS getReference(qs::WSTRING* pwstrReference) const;
	unsigned int getReferenceHash() const;
	MessageCacheKey getMessageCacheKey() const;
	const MessageBoxKey& getMessageBoxKey() const;

public:
	static unsigned int getDate(const qs::Time& time);
	static unsigned int getTime(const qs::Time& time);

// These methods are intended to be called from Folder class
public:
	void getInit(Init* pInit) const;
	void setId(unsigned int nId);
	void setFlags(unsigned int nFlags, unsigned int nMask);
	void setFolder(NormalFolder* pFolder);

// These methods are intended to be called from Account class
public:
	void setKeys(MessageCacheKey messageCacheKey,
		const MessageBoxKey& messageBoxKey);

private:
	MessageHolder(const MessageHolder&);
	MessageHolder& operator=(const MessageHolder&);

private:
	unsigned int nId_;
	volatile unsigned int nFlags_;
	volatile mutable unsigned int nMessageIdHash_;
	volatile mutable unsigned int nReferenceHash_;
	unsigned int nDate_;
	unsigned int nTime_;
	unsigned int nSize_;
	MessageCacheKey messageCacheKey_;
	MessageBoxKey messageBoxKey_;
	NormalFolder* pFolder_;
};

typedef std::vector<MessageHolder*> MessageHolderList;


/****************************************************************************
 *
 * AbstractMessageHolder
 *
 */

class QMEXPORTCLASS AbstractMessageHolder : public MessageHolderBase
{
protected:
	AbstractMessageHolder(NormalFolder* pFolder, Message* pMessage,
		unsigned int nId, unsigned int nSize,
		unsigned int nTextSize, qs::QSTATUS* pstatus);

public:
	virtual ~AbstractMessageHolder();

public:
	virtual unsigned int getId() const;
	virtual unsigned int getFlags() const;
	virtual qs::QSTATUS getFrom(qs::WSTRING* pwstrFrom) const;
	virtual qs::QSTATUS getTo(qs::WSTRING* pwstrTo) const;
	virtual qs::QSTATUS getFromTo(qs::WSTRING* pwstrFromTo) const;
	virtual qs::QSTATUS getSubject(qs::WSTRING* pwstrSubject) const;
	virtual qs::QSTATUS getDate(qs::Time* pTime) const;
	virtual unsigned int getSize() const;
	virtual unsigned int getTextSize() const;
	virtual NormalFolder* getFolder() const;
	virtual Account* getAccount() const;
	virtual MessageHolder* getMessageHolder();

protected:
	Message* getMessage() const;

private:
	qs::QSTATUS getAddress(const WCHAR* pwszName,
		qs::WSTRING* pwstrValue) const;

private:
	AbstractMessageHolder(const AbstractMessageHolder&);
	AbstractMessageHolder& operator=(const AbstractMessageHolder&);

private:
	NormalFolder* pFolder_;
	Message* pMessage_;
	unsigned int nId_;
	unsigned int nSize_;
	unsigned int nTextSize_;
};


/****************************************************************************
 *
 * MessagePtr
 *
 */

class QMEXPORTCLASS MessagePtr
{
public:
	MessagePtr();
	MessagePtr(MessageHolder* pmh);
	MessagePtr(const MessagePtr& ptr);
	~MessagePtr();

public:
	MessagePtr& operator=(const MessagePtr& ptr);

public:
	NormalFolder* getFolder() const;
	MessageHolder* lock() const;
	void unlock() const;
	void reset(MessageHolder* pmh);

private:
	NormalFolder* pFolder_;
	unsigned int nId_;
	mutable bool bLock_;
};

typedef std::vector<MessagePtr> MessagePtrList;


/****************************************************************************
 *
 * MessagePtrLock
 *
 */

class QMEXPORTCLASS MessagePtrLock
{
public:
	explicit MessagePtrLock(const MessagePtr& ptr);
	~MessagePtrLock();

public:
	operator MessageHolder*() const;
	MessageHolder* operator->() const;

private:
	MessagePtrLock(const MessagePtrLock&);
	MessagePtrLock& operator=(const MessagePtrLock&);

private:
	MessagePtr ptr_;
	MessageHolder* pmh_;
};

}

#endif // __QMMESSAGEHOLDER_H__
