/*
 * $Id$
 *
 * Copyright(C) 1998-2004 Satoshi Nakamura
 * All rights reserved.
 *
 */

#ifndef __QMFOLDER_H__
#define __QMFOLDER_H__

#include <qm.h>
#include <qmmessagecache.h>
#include <qmmessageholder.h>

#include <qs.h>
#include <qsprofile.h>
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
class Document;
class Message;
class MessageHolder;
class MessageOperationCallback;


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
		FLAG_IGNOREUNSEEN	= 0x00000080,
		
		FLAG_INBOX			= 0x00000100,
		FLAG_OUTBOX			= 0x00000200,
		FLAG_SENTBOX		= 0x00000400,
		FLAG_TRASHBOX		= 0x00000800,
		FLAG_DRAFTBOX		= 0x00001000,
		FLAG_SEARCHBOX		= 0x00002000,
		FLAG_BOX_MASK		= 0x0000ff00,
		
		FLAG_SYNCABLE		= 0x00010000,
		FLAG_SYNCWHENOPEN	= 0x00020000,
		FLAG_CACHEWHENREAD	= 0x00040000,
		FLAG_SYNC_MASK		= 0x00ff0000,
		
		FLAG_USER_MASK		= 0x000600a0
	};

public:
	typedef std::vector<std::pair<qs::WSTRING, qs::WSTRING> > ParamList;

public:
	Folder(unsigned int nId,
		   const WCHAR* pwszName,
		   WCHAR cSeparator,
		   unsigned int nFlags,
		   Folder* pParentFolder,
		   Account* pAccount);
	virtual ~Folder();

public:
	unsigned int getId() const;
	const WCHAR* getName() const;
	qs::wstring_ptr getDisplayName() const;
	qs::wstring_ptr getFullName() const;
	WCHAR getSeparator() const;
	unsigned int getFlags() const;
	bool isFlag(Flag flag) const;
	const ParamList& getParams() const;
	void setParams(ParamList& listParam);
	const WCHAR* getParam(const WCHAR* pwszName) const;
	void setParam(const WCHAR* pwszName,
				  const WCHAR* pwszValue);
	void removeParam(const WCHAR* pwszName);
	Folder* getParentFolder() const;
	bool isAncestorOf(const Folder* pFolder) const;
	bool isHidden() const;
	unsigned int getLevel() const;
	Account* getAccount() const;
	
	void addFolderHandler(FolderHandler* pHandler);
	void removeFolderHandler(FolderHandler* pHandler);

// These methods are intended to be called from Account class.
public:
	void setName(const WCHAR* pwszName);
	void setFlags(unsigned int nFlags,
				  unsigned int nMask);

// These methods are intended to be called from impl classes.
public:
	struct FolderImpl* getImpl() const;

public:
	virtual Type getType() const = 0;
	virtual unsigned int getCount() const = 0;
	virtual unsigned int getUnseenCount() const = 0;
	virtual unsigned int getSize() = 0;
	virtual unsigned int getBoxSize() = 0;
	virtual MessageHolder* getMessage(unsigned int n) const = 0;
	virtual const MessageHolderList& getMessages() = 0;
	virtual bool loadMessageHolders() = 0;
	virtual bool saveMessageHolders() = 0;
	virtual bool deletePermanent() = 0;

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
	typedef std::vector<std::pair<unsigned int, unsigned int> > FlagList;

public:
	NormalFolder(unsigned int nId,
				 const WCHAR* pwszName,
				 WCHAR cSeparator,
				 unsigned int nFlags,
				 unsigned int nCount,
				 unsigned int nUnseenCount,
				 unsigned int nValidity,
				 unsigned int nDownloadCount,
				 unsigned int nDeletedCount,
				 Folder* pParentFolder,
				 Account* pAccount);
	virtual ~NormalFolder();

public:
	unsigned int getValidity() const;
	bool setValidity(unsigned int nValidity);
	unsigned int getDownloadCount() const;
	unsigned int getDeletedCount() const;
	unsigned int getLastSyncTime() const;
	void setLastSyncTime(unsigned int nTime);
	MessagePtr getMessageById(unsigned int nId);
	MessageHolder* getMessageHolderById(unsigned int nId) const;
	bool updateMessageFlags(const FlagList& listFlag,
							bool* pbClear);

public:
	virtual Type getType() const;
	virtual unsigned int getCount() const;
	virtual unsigned int getUnseenCount() const;
	virtual unsigned int getSize();
	virtual unsigned getBoxSize();
	virtual MessageHolder* getMessage(unsigned int n) const;
	virtual const MessageHolderList& getMessages();
	virtual bool loadMessageHolders();
	virtual bool saveMessageHolders();
	virtual bool deletePermanent();

// These methods are intended to be called from Account class
public:
	unsigned int generateId();
	bool appendMessage(std::auto_ptr<MessageHolder> pmh);
	void removeMessage(MessageHolder* pmh);
	bool moveMessages(const MessageHolderList& l,
					  NormalFolder* pFolder);

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
	QueryFolder(unsigned int nId,
				const WCHAR* pwszName,
				WCHAR cSeparator,
				unsigned int nFlags,
				unsigned int nCount,
				unsigned int nUseenCount,
				const WCHAR* pwszDriver,
				const WCHAR* pwszCondition,
				const WCHAR* pwszTargetFolder,
				bool bRecursive,
				Folder* pParentFolder,
				Account* pAccount);
	virtual ~QueryFolder();

public:
	const WCHAR* getDriver() const;
	const WCHAR* getCondition() const;
	const WCHAR* getTargetFolder() const;
	bool isRecursive() const;
	void set(const WCHAR* pwszDriver,
			 const WCHAR* pwszCondition,
			 const WCHAR* pwszTargetFolder,
			 bool bRecursive);
	bool search(Document* pDocument,
				HWND hwnd,
				qs::Profile* pProfile,
				bool bDecryptVerify);

public:
	virtual Type getType() const;
	virtual unsigned int getCount() const;
	virtual unsigned int getUnseenCount() const;
	virtual unsigned int getSize();
	virtual unsigned int getBoxSize();
	virtual MessageHolder* getMessage(unsigned int n) const;
	virtual const MessageHolderList& getMessages();
	virtual bool loadMessageHolders();
	virtual bool saveMessageHolders();
	virtual bool deletePermanent();

private:
	QueryFolder(const QueryFolder&);
	QueryFolder& operator=(const QueryFolder&);

private:
	struct QueryFolderImpl* pImpl_;
};


/****************************************************************************
 *
 * FolderLess
 *
 */

class FolderLess : public std::binary_function<Folder*, Folder*, bool>
{
public:
	bool operator()(const Folder* pFolderLhs,
					const Folder* pFolderRhs) const;

public:
	static int compare(const Folder* pFolderLhs,
					   const Folder* pFolderRhs);

private:
	typedef std::vector<const Folder*> FolderPath;

private:
	static int compareSingle(const Folder* pFolderLhs,
							 const Folder* pFolderRhs);
	static void getFolderPath(const Folder* pFolder,
							  FolderPath* pPath);
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
	virtual void messageAdded(const FolderEvent& event) = 0;
	virtual void messageRemoved(const FolderEvent& event) = 0;
	virtual void messageRefreshed(const FolderEvent& event) = 0;
	virtual void unseenCountChanged(const FolderEvent& event) = 0;
	virtual void folderDestroyed(const FolderEvent& event) = 0;
};


/****************************************************************************
 *
 * FolderEvent
 *
 */

class FolderEvent
{
public:
	FolderEvent(Folder* pFolder,
				MessageHolder* pmh);
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

}

#endif // __QMFOLDER_H__
