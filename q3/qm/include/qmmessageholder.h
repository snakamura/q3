/*
 * $Id$
 *
 * Copyright(C) 1998-2008 Satoshi Nakamura
 *
 */

#ifndef __QMMESSAGEHOLDER_H__
#define __QMMESSAGEHOLDER_H__

#include <qm.h>

#include <qs.h>
#include <qsstring.h>
#include <qsutil.h>

#include <vector>


namespace qm {

class MessageDate;
class MessageHolderBase;
	class MessageHolder;
	class AbstractMessageHolder;
class MessageHolderHandler;
class MessageHolderEvent;
class MessagePtr;
class MessagePtrLock;

class Account;
class NormalFolder;
class Message;


/****************************************************************************
 *
 * MessageDate
 *
 */

class QMEXPORTCLASS MessageDate
{
public:
	MessageDate();
	MessageDate(unsigned int nDate,
				unsigned int nTime);
	MessageDate(const MessageDate& date);
	~MessageDate();

public:
	MessageDate& operator=(const MessageDate& date);

public:
	unsigned int getDate() const;
	unsigned int getTime() const;
	void getTime(qs::Time* pTime) const;

public:
	static unsigned int getDate(const qs::Time& time);
	static unsigned int getTime(const qs::Time& time);
	static void getTime(unsigned int nDate,
						unsigned int nTime,
						qs::Time* pTime);
private:
	unsigned int nDate_;
	unsigned int nTime_;
};


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
		FLAG_ENVELOPED		= 0x00040000,
		FLAG_LOCAL			= 0x00080000,
		FLAG_LABEL			= 0x80000000,
		FLAG_SYSTEM_MASK	= 0x800f0000,
		
		FLAG_INDEXONLY		= 0x01000000,
		FLAG_HEADERONLY		= 0x02000000,
		FLAG_TEXTONLY		= 0x04000000,
		FLAG_HTMLONLY		= 0x08000000,
		FLAG_PARTIAL_MASK	= 0x0f000000,
		
		FLAG_POPSDELETE		= 0x10000000,
		FLAG_ADDON_MASK		= 0x70000000
	};

public:
	virtual ~MessageHolderBase();

public:
	virtual unsigned int getId() const = 0;
	virtual unsigned int getFlags() const = 0;
	virtual qs::wstring_ptr getFrom() const = 0;
	virtual qs::wstring_ptr getTo() const = 0;
	virtual qs::wstring_ptr getFromTo() const = 0;
	virtual qs::wstring_ptr getSubject() const = 0;
	virtual void getDate(qs::Time* pTime) const = 0;
	virtual unsigned int getSize() const = 0;
	virtual unsigned int getTextSize() const = 0;
	virtual NormalFolder* getFolder() const = 0;
	virtual Account* getAccount() const = 0;
	virtual bool getMessage(unsigned int nFlags,
							const WCHAR* pwszField,
							unsigned int nSecurityMode,
							Message* pMessage) = 0;
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
	struct MessageIndexKey
	{
		unsigned int nKey_;
		unsigned int nLength_;
	};
	
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
		unsigned int nIndexKey_;
		unsigned int nIndexLength_;
		unsigned int nOffset_;
		unsigned int nLength_;
		unsigned int nHeaderLength_;
	};

public:
	MessageHolder(NormalFolder* pFolder,
				  const Init& init);
	virtual ~MessageHolder();

public:
	void* operator new(size_t n);
	void operator delete(void* p);

public:
	virtual unsigned int getId() const;
	virtual unsigned int getFlags() const;
	virtual qs::wstring_ptr getFrom() const;
	virtual qs::wstring_ptr getTo() const;
	virtual qs::wstring_ptr getFromTo() const;
	virtual qs::wstring_ptr getSubject() const;
	virtual void getDate(qs::Time* pTime) const;
	virtual unsigned int getSize() const;
	virtual unsigned int getTextSize() const;
	virtual NormalFolder* getFolder() const;
	virtual Account* getAccount() const;
	virtual bool getMessage(unsigned int nFlags,
							const WCHAR* pwszField,
							unsigned int nSecurityMode,
							Message* pMessage);
	virtual MessageHolder* getMessageHolder();
	
public:
	bool isFlag(Flag flag) const;
	bool isSeen() const;
	qs::wstring_ptr getMessageId() const;
	unsigned int getMessageIdHash() const;
	qs::wstring_ptr getReference() const;
	unsigned int getReferenceHash() const;
	qs::wstring_ptr getLabel() const;
	const MessageIndexKey& getMessageIndexKey() const;
	const MessageBoxKey& getMessageBoxKey() const;
	MessageDate getDate() const;

// These methods are intended to be called from Folder class
public:
	void getInit(Init* pInit) const;
	void setId(unsigned int nId);
	void setFlags(unsigned int nFlags,
				  unsigned int nMask);
	bool setLabel(const WCHAR* pwszLabel);
	void setFolder(NormalFolder* pFolder);
	void destroy();

// These methods are intended to be called from Account class
public:
	void setKeys(const MessageIndexKey& messageIndexKey,
				 const MessageBoxKey& messageBoxKey);
	void setIndexKey(const MessageIndexKey& messageIndexKey);

private:
	void setKeys(const MessageIndexKey* pMessageIndexKey,
				 const MessageBoxKey* pMessageBoxKey);

private:
	MessageHolder(const MessageHolder&);
	MessageHolder& operator=(const MessageHolder&);

private:
	unsigned int nId_;
	volatile unsigned int nFlags_;
	volatile mutable unsigned int nMessageIdHash_;
	volatile mutable unsigned int nReferenceHash_;
	MessageDate date_;
	unsigned int nSize_;
	MessageIndexKey messageIndexKey_;
	MessageBoxKey messageBoxKey_;
	NormalFolder* pFolder_;
};


/****************************************************************************
 *
 * AbstractMessageHolder
 *
 */

class QMEXPORTCLASS AbstractMessageHolder : public MessageHolderBase
{
protected:
	AbstractMessageHolder(NormalFolder* pFolder,
						  Message* pMessage,
						  unsigned int nId,
						  unsigned int nSize,
						  unsigned int nTextSize);

public:
	virtual ~AbstractMessageHolder();

public:
	virtual unsigned int getId() const;
	virtual unsigned int getFlags() const;
	virtual qs::wstring_ptr getFrom() const;
	virtual qs::wstring_ptr getTo() const;
	virtual qs::wstring_ptr getFromTo() const;
	virtual qs::wstring_ptr getSubject() const;
	virtual void getDate(qs::Time* pTime) const;
	virtual unsigned int getSize() const;
	virtual unsigned int getTextSize() const;
	virtual NormalFolder* getFolder() const;
	virtual Account* getAccount() const;
	virtual MessageHolder* getMessageHolder();

protected:
	Message* getMessage() const;

private:
	qs::wstring_ptr getAddress(const WCHAR* pwszName) const;

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
 * MessageHolderHandler
 *
 */

class MessageHolderHandler
{
public:
	virtual ~MessageHolderHandler();

public:
	virtual void messageHolderFlagsChanged(const MessageHolderEvent& event) = 0;
	virtual void messageHolderKeysChanged(const MessageHolderEvent& event) = 0;
	virtual void messageHolderDestroyed(const MessageHolderEvent& event) = 0;
};


/****************************************************************************
 *
 * DefaultMessageHolderHandler
 *
 */

class DefaultMessageHolderHandler : public MessageHolderHandler
{
public:
	DefaultMessageHolderHandler();
	virtual ~DefaultMessageHolderHandler();

public:
	virtual void messageHolderFlagsChanged(const MessageHolderEvent& event);
	virtual void messageHolderKeysChanged(const MessageHolderEvent& event);
	virtual void messageHolderDestroyed(const MessageHolderEvent& event);
};


/****************************************************************************
 *
 * MessageHolderEvent
 *
 */

class MessageHolderEvent
{
public:
	MessageHolderEvent(MessageHolder* pmh);
	MessageHolderEvent(MessageHolder* pmh,
					   unsigned int nOldFlags,
					   unsigned int nNewFlags);
	~MessageHolderEvent();

public:
	MessageHolder* getMessageHolder() const;
	unsigned int getOldFlags() const;
	unsigned int getNewFlags() const;

private:
	MessageHolderEvent(const MessageHolderEvent&);
	MessageHolderEvent& operator=(const MessageHolderEvent&);

private:
	MessageHolder* pmh_;
	unsigned int nOldFlags_;
	unsigned int nNewFlags_;
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
	bool operator!() const;
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

#include <qmmessageholder.inl>

#endif // __QMMESSAGEHOLDER_H__
