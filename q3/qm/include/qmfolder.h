/*
 * $Id$
 *
 * Copyright(C) 1998-2003 Satoshi Nakamura
 * All rights reserved.
 *
 */

#ifndef __QMFOLDER_H__
#define __QMFOLDER_H__

#include <qm.h>
#include <qmmessagecache.h>
#include <qmmessageholder.h>

#include <qs.h>
#include <qsstring.h>

#include <vector>


namespace qm {

class Folder;
	class NormalFolder;
	class QueryFolder;
class FolderLess;
class FolderHandler;
class FolderEvent;
class MessageEvent;

class Account;
class MessageHolder;
class Message;


/****************************************************************************
 *
 * Folder
 *
 */

class QMEXPORTCLASS Folder
{
public:
	enum Type {
		TYPE_NORMAL,
		TYPE_QUERY
	};
	
	enum Flag {
		FLAG_NOSELECT		= 0x00000001,
		FLAG_NOINFERIORS	= 0x00000002,
		FLAG_CUSTOMFLAGS	= 0x00000004,
		FLAG_NORENAME		= 0x00000008,
		FLAG_LOCAL			= 0x00000010,
		FLAG_HIDE			= 0x00000020,
		FLAG_CHILDOFROOT	= 0x00000040,
		
		FLAG_INBOX			= 0x00000100,
		FLAG_OUTBOX			= 0x00000200,
		FLAG_SENTBOX		= 0x00000400,
		FLAG_TRASHBOX		= 0x00000800,
		FLAG_DRAFTBOX		= 0x00001000,
		FLAG_BOX_MASK		= 0x0000ff00,
		
		FLAG_SYNCABLE		= 0x00010000,
		FLAG_SYNCWHENOPEN	= 0x00020000,
		FLAG_CACHEWHENREAD	= 0x00040000,
		FLAG_SYNC_MASK		= 0x00ff0000,
		
		FLAG_USER_MASK		= 0x00060020
	};

public:
	struct Init
	{
		unsigned int nId_;
		const WCHAR* pwszName_;
		WCHAR cSeparator_;
		unsigned int nFlags_;
		unsigned int nCount_;
		unsigned int nUnseenCount_;
		Folder* pParentFolder_;
		Account* pAccount_;
	};

public:
	typedef std::vector<MessageHolder*> MessageHolderList;

public:
	Folder(const Init& init, qs::QSTATUS* pstatus);
	virtual ~Folder();

public:
	unsigned int getId() const;
	const WCHAR* getName() const;
	qs::QSTATUS getDisplayName(qs::WSTRING* pwstrName) const;
	qs::QSTATUS getFullName(qs::WSTRING* pwstrName) const;
	WCHAR getSeparator() const;
	unsigned int getFlags() const;
	bool isFlag(Flag flag) const;
	void setFlags(unsigned int nFlags, unsigned int nMask);
	Folder* getParentFolder() const;
	bool isAncestorOf(const Folder* pFolder) const;
	bool isHidden() const;
	unsigned int getLevel() const;
	Account* getAccount() const;
	
	qs::QSTATUS setMessagesFlags(const MessagePtrList& l,
		unsigned int nFlags, unsigned int nMask);
	qs::QSTATUS copyMessages(const MessagePtrList& l,
		NormalFolder* pFolder, bool bMove);
	qs::QSTATUS removeMessages(const MessagePtrList& l, bool bDirect);
	
	qs::QSTATUS addFolderHandler(FolderHandler* pHandler);
	qs::QSTATUS removeFolderHandler(FolderHandler* pHandler);

public:
	void lock() const;
	void unlock() const;
#ifndef NDEBUG
	bool isLocked() const;
#endif

public:
	struct FolderImpl* getImpl() const;

public:
	virtual Type getType() const = 0;
	virtual unsigned int getCount() const = 0;
	virtual unsigned int getUnseenCount() const = 0;
	virtual qs::QSTATUS getSize(unsigned int* pnSize) = 0;
	virtual MessageHolder* getMessage(unsigned int n) const = 0;
	virtual MessageHolder* getMessageById(unsigned int nId) const = 0;
	virtual qs::QSTATUS setMessagesFlags(const MessageHolderList& l,
		unsigned int nFlags, unsigned int nMask) = 0;
	virtual qs::QSTATUS copyMessages(const MessageHolderList& l,
		NormalFolder* pFolder, bool bMove) = 0;
	virtual qs::QSTATUS removeMessages(
		const MessageHolderList& l, bool bDirect) = 0;
	virtual qs::QSTATUS loadMessageHolders() = 0;
	virtual qs::QSTATUS saveMessageHolders() = 0;
	virtual qs::QSTATUS deletePermanent() = 0;

private:
	Folder(const Folder&);
	Folder& operator=(const Folder&);

private:
	struct FolderImpl* pImpl_;
};


/****************************************************************************
 *
 * NormalFolder
 *
 */

class QMEXPORTCLASS NormalFolder : public Folder
{
public:
	struct Init : public Folder::Init
	{
		unsigned int nValidity_;
		unsigned int nDownloadCount_;
	};

public:
	typedef std::vector<std::pair<unsigned int, unsigned int> > FlagList;

public:
	NormalFolder(const Init& init, qs::QSTATUS* pstatus);
	virtual ~NormalFolder();

public:
	unsigned int getValidity() const;
	qs::QSTATUS setValidity(unsigned int nValidity);
	unsigned int getDownloadCount() const;
	qs::QSTATUS getMessageById(unsigned int nId, MessageHolder** ppmh);
	qs::QSTATUS updateMessageFlags(const FlagList& listFlag, bool* pbClear);
	qs::QSTATUS appendMessage(const Message& msg, unsigned int nFlags);
	qs::QSTATUS removeAllMessages();
	qs::QSTATUS clearDeletedMessages();

public:
	virtual Type getType() const;
	virtual unsigned int getCount() const;
	virtual unsigned int getUnseenCount() const;
	virtual qs::QSTATUS getSize(unsigned int* pnSize);
	virtual MessageHolder* getMessage(unsigned int n) const;
	virtual MessageHolder* getMessageById(unsigned int nId) const;
	virtual qs::QSTATUS setMessagesFlags(const MessageHolderList& l,
		unsigned int nFlags, unsigned int nMask);
	virtual qs::QSTATUS copyMessages(const MessageHolderList& l,
		NormalFolder* pFolder, bool bMove);
	virtual qs::QSTATUS removeMessages(
		const MessageHolderList& l, bool bDirect);
	virtual qs::QSTATUS loadMessageHolders();
	virtual qs::QSTATUS saveMessageHolders();
	virtual qs::QSTATUS deletePermanent();

// These methods are intended to be called from Account class
public:
	qs::QSTATUS generateId(unsigned int* pnId);
	qs::QSTATUS appendMessage(MessageHolder* pmh);
	qs::QSTATUS removeMessage(MessageHolder* pmh);
	qs::QSTATUS deleteMessages(const MessageHolderList& l);
	qs::QSTATUS deleteAllMessages();
	qs::QSTATUS moveMessages(const MessageHolderList& l, NormalFolder* pFolder);

// These methods are intended to be called from MessageHolder class
public:
	qs::QSTATUS getData(MessageCacheKey key,
		MessageCacheItem item, qs::WSTRING* pwstrData) const;
	qs::QSTATUS getMessage(MessageHolder* pmh,
		unsigned int nFlags, Message* pMessage) const;
	qs::QSTATUS fireMessageFlagChanged(MessageHolder* pmh,
		unsigned int nOldFlags, unsigned int nNewFlags);

private:
	NormalFolder(const NormalFolder&);
	NormalFolder& operator=(const NormalFolder&);

private:
	struct NormalFolderImpl* pImpl_;
};


/****************************************************************************
 *
 * QueryFolder
 *
 */

class QMEXPORTCLASS QueryFolder : public Folder
{
public:
	struct Init : public Folder::Init
	{
		const WCHAR* pwszMacro_;
	};

public:
	QueryFolder(const Init& init, qs::QSTATUS* pstatus);
	virtual ~QueryFolder();

public:
	const WCHAR* getMacro() const;

public:
	virtual Type getType() const;
	virtual unsigned int getCount() const;
	virtual unsigned int getUnseenCount() const;
	virtual qs::QSTATUS getSize(unsigned int* pnSize);
	virtual MessageHolder* getMessage(unsigned int n) const;
	virtual MessageHolder* getMessageById(unsigned int nId) const;
	virtual qs::QSTATUS setMessagesFlags(const MessageHolderList& l,
		unsigned int nFlags, unsigned int nMask);
	virtual qs::QSTATUS copyMessages(const MessageHolderList& l,
		NormalFolder* pFolder, bool bMove);
	virtual qs::QSTATUS removeMessages(
		const MessageHolderList& l, bool bDirect);
	virtual qs::QSTATUS loadMessageHolders();
	virtual qs::QSTATUS saveMessageHolders();
	virtual qs::QSTATUS deletePermanent();

private:
	QueryFolder(const QueryFolder&);
	QueryFolder& operator=(const QueryFolder&);

private:
	qs::WSTRING wstrMacro_;
};


/****************************************************************************
 *
 * FolderLess
 *
 */

class FolderLess : public std::binary_function<Folder*, Folder*, bool>
{
public:
	bool operator()(const Folder* pFolderLhs, const Folder* pFolderRhs) const;

public:
	static int compare(const Folder* pFolderLhs, const Folder* pFolderRhs);

private:
	typedef std::vector<const Folder*> FolderPath;

private:
	static int compareSingle(const Folder* pFolderLhs, const Folder* pFolderRhs);
	static qs::QSTATUS getFolderPath(const Folder* pFolder, FolderPath* pPath);
};


/****************************************************************************
 *
 * FolderHandler
 *
 */

class FolderHandler
{
public:
	virtual ~FolderHandler();

public:
	virtual qs::QSTATUS messageAdded(const FolderEvent& event) = 0;
	virtual qs::QSTATUS messageRemoved(const FolderEvent& event) = 0;
	virtual qs::QSTATUS messageChanged(const MessageEvent& event) = 0;
};


/****************************************************************************
 *
 * FolderEvent
 *
 */

class FolderEvent
{
public:
	FolderEvent(Folder* pFolder, MessageHolder* pmh);
	~FolderEvent();

public:
	Folder* getFolder() const;
	MessageHolder* getMessageHolder() const;

private:
	FolderEvent(const FolderEvent&);
	FolderEvent& operator=(const FolderEvent&);

private:
	Folder* pFolder_;
	MessageHolder* pmh_;
};


/****************************************************************************
 *
 * MessageEvent
 *
 */

class MessageEvent
{
public:
	MessageEvent(NormalFolder* pFolder, MessageHolder* pmh,
		unsigned int nOldFlags, unsigned int nNewFlags);
	~MessageEvent();

public:
	NormalFolder* getFolder() const;
	MessageHolder* getMessageHolder() const;
	unsigned int getOldFlags() const;
	unsigned int getNewFlags() const;

private:
	MessageEvent(const MessageEvent&);
	MessageEvent& operator=(const MessageEvent&);

private:
	NormalFolder* pFolder_;
	MessageHolder* pmh_;
	unsigned int nOldFlags_;
	unsigned int nNewFlags_;
};

}

#endif // __QMFOLDER_H__
