/*
 * $Id$
 *
 * Copyright(C) 1998-2004 Satoshi Nakamura
 * All rights reserved.
 *
 */

#ifndef __QMACCOUNT_H__
#define __QMACCOUNT_H__

#include <qm.h>
#include <qmfolder.h>
#include <qmmessagecache.h>
#include <qmmessageoperation.h>

#include <qs.h>
#include <qscrypto.h>
#include <qslog.h>
#include <qsmime.h>
#include <qsprofile.h>
#include <qsstream.h>
#include <qsstring.h>

#include <vector>


namespace qm {

class Account;
class SubAccount;
class AccountHandler;
	class DefaultAccountHandler;
class AccountEvent;
class FolderListChangedEvent;
class AccountCheckCallback;

class Message;
class MessageHolder;
class MessageOperationCallback;
class ProtocolDriver;
class Security;


/****************************************************************************
 *
 * Account
 *
 */

class QMEXPORTCLASS Account
{
public:
	enum Host {
		HOST_SEND,
		HOST_RECEIVE,
		HOST_SIZE
	};
	
	enum ImportFlag {
		IMPORTFLAG_NORMALFLAGS	= 0x00,
		IMPORTFLAG_IGNOREFLAGS	= 0x01,
		IMPORTFLAG_QMAIL20FLAGS	= 0x02,
	};
	
	enum GetMessageFlag {
		GETMESSAGEFLAG_ALL			= 0x01,
		GETMESSAGEFLAG_HEADER		= 0x02,
		GETMESSAGEFLAG_TEXT			= 0x03,
		GETMESSAGEFLAG_HTML			= 0x04,
		GETMESSAGEFLAG_POSSIBLE		= 0x05,
		GETMESSAGEFLAG_METHOD_MASK	= 0x0f,
		
		GETMESSAGEFLAG_MAKESEEN		= 0x10,
		GETMESSAGEFLAG_NOFALLBACK	= 0x20,
		GETMESSAGEFLAG_NOSECURITY	= 0x40
	};
	
	enum Support {
		SUPPORT_REMOTEFOLDER			= 0x01,
		SUPPORT_LOCALFOLDERSYNC			= 0x02,
		SUPPORT_LOCALFOLDERGETMESSAGE	= 0x04,
		SUPPORT_LOCALFOLDERDOWNLOAD		= 0x08,
		SUPPORT_EXTERNALLINK			= 0x10
	};

public:
	typedef std::vector<SubAccount*> SubAccountList;
	typedef std::vector<Folder*> FolderList;

public:
	Account(const WCHAR* pwszPath,
			const Security* pSecurity);
	~Account();

public:
	const WCHAR* getName() const;
	const WCHAR* getPath() const;
	const WCHAR* getClass() const;
	const WCHAR* getType(Host host) const;
	bool isSupport(Support support) const;
	const WCHAR* getMessageStorePath() const;
	bool isMultiMessageStore() const;
	
	int getProperty(const WCHAR* pwszSection,
					const WCHAR* pwszKey,
					int nDefault) const;
	void setProperty(const WCHAR* pwszSection,
					 const WCHAR* pwszKey,
					 int nValue);
	qs::wstring_ptr getProperty(const WCHAR* pwszSection,
								const WCHAR* pwszName,
								const WCHAR* pwszDefault) const;
	void setProperty(const WCHAR* pwszSection,
					 const WCHAR* pwszName,
					 const WCHAR* pwszValue);
	
	SubAccount* getSubAccount(const WCHAR* pwszName) const;
	SubAccount* getSubAccountByIdentity(const WCHAR* pwszIdentity) const;
	const SubAccountList& getSubAccounts() const;
	void addSubAccount(std::auto_ptr<SubAccount> pSubAccount);
	void removeSubAccount(SubAccount* pSubAccount);
	bool renameSubAccount(SubAccount* pSubAccount,
						  const WCHAR* pwszName);
	SubAccount* getCurrentSubAccount() const;
	void setCurrentSubAccount(SubAccount* pSubAccount);
	
	Folder* getFolder(const WCHAR* pwszName) const;
	Folder* getFolder(Folder* pParent,
					  const WCHAR* pwszName) const;
	Folder* getFolderById(unsigned int nId) const;
	Folder* getFolderByFlag(unsigned int nFlag) const;
	const FolderList& getFolders() const;
	NormalFolder* createNormalFolder(const WCHAR* pwszName,
									 Folder* pParent,
									 bool bRemote);
	QueryFolder* createQueryFolder(const WCHAR* pwszName,
								   Folder* pParent,
								   const WCHAR* pwszDriver,
								   const WCHAR* pwszCondition,
								   const WCHAR* pwszTargetFolder,
								   bool bRecursive);
	bool removeFolder(Folder* pFolder);
	bool renameFolder(Folder* pFolder,
					  const WCHAR* pwszName);
	void setFolderFlags(Folder* pFolder,
						unsigned int nFlags,
						unsigned int nMask);
	bool updateFolders();
	std::pair<const WCHAR**, size_t> getFolderParamNames() const;
	
	void setOffline(bool bOffline);
	bool compact(MessageOperationCallback* pCallback);
	bool salvage(NormalFolder* pFolder,
				 MessageOperationCallback* pCallback);
	bool check(AccountCheckCallback* pCallback);
	bool save() const;
	bool flushMessageStore() const;
	bool importMessage(NormalFolder* pFolder,
					   const CHAR* pszMessage,
					   unsigned int nFlags);
	
	bool appendMessage(NormalFolder* pFolder,
					   const Message& msg,
					   unsigned int nFlags);
	bool removeMessages(const MessageHolderList& l,
						bool bDirect,
						MessageOperationCallback* pCallback);
	bool copyMessages(const MessageHolderList& l,
					  NormalFolder* pFolderTo,
					  bool bMove,
					  MessageOperationCallback* pCallback);
	bool setMessagesFlags(const MessageHolderList& l,
						  unsigned int nFlags,
						  unsigned int nMask);
	bool deleteMessagesCache(const MessageHolderList& l);
	bool clearDeletedMessages(NormalFolder* pFolder);
	
	void addAccountHandler(AccountHandler* pHandler);
	void removeAccountHandler(AccountHandler* pHandler);
	
	void addMessageHolderHandler(MessageHolderHandler* pHandler);
	void removeMessageHolderHandler(MessageHolderHandler* pHandler);
	
	void lock() const;
	void unlock() const;
#ifndef NDEBUG
	bool isLocked() const;
	unsigned int getLockCount() const;
#endif

// These methods are intended to be called from Document class
public:
	void deletePermanent(bool bDeleteContent);

// These methods are intended to be called from MessageHolder class
public:
	qs::wstring_ptr getData(MessageCacheKey key,
							MessageCacheItem item) const;
	bool getMessage(MessageHolder* pmh,
					unsigned int nFlags,
					Message* pMessage);
	void fireMessageHolderChanged(MessageHolder* pmh,
								  unsigned int nOldFlags,
								  unsigned int nNewFlags);
	void fireMessageHolderDestroyed(MessageHolder* pmh);

// These methods are intended to be called from ReceiveSession class
public:
	ProtocolDriver* getProtocolDriver() const;
	MessageHolder* storeMessage(NormalFolder* pFolder,
								const CHAR* pszMessage,
								const Message* pHeader,
								unsigned int nId,
								unsigned int nFlags,
								unsigned int nSize,
								bool bIndexOnly);
	bool unstoreMessage(MessageHolder* pmh);
	MessageHolder* cloneMessage(MessageHolder* pmh,
								NormalFolder* pFolderTo);
	bool updateMessage(MessageHolder* pmh,
					   const CHAR* pszMessage);

// These methods are intended to be called from ProtocolDriver class
public:
	unsigned int generateFolderId() const;

// These methods are intended to be called from SyncManager class
public:
	std::auto_ptr<qs::Logger> openLogger(Host host) const;

private:
	Account(const Account&);
	Account& operator=(const Account&);

private:
	class AccountImpl* pImpl_;
};


/****************************************************************************
 *
 * AccountLock
 *
 */

class AccountLock
{
public:
	AccountLock();
	AccountLock(Account* pAccount);
	~AccountLock();

public:
	Account* get() const;
	void set(Account* pAccount);

private:
	AccountLock& operator=(const AccountLock&);

private:
	Account* pAccount_;
};


/****************************************************************************
 *
 * SubAccount
 *
 */

class QMEXPORTCLASS SubAccount
{
public:
	enum DialupType {
		DIALUPTYPE_NEVER,
		DIALUPTYPE_WHENEVERNOTCONNECTED,
		DIALUPTYPE_CONNECT
	};
	
	enum SSLOption {
		SSLOPTION_ALLOWUNVERIFIEDCERTIFICATE	= 0x01,
		SSLOPTION_ALLOWDIFFERENTHOST			= 0x02
	};

public:
	SubAccount(Account* pAccount,
			   std::auto_ptr<qs::Profile> pProfile,
			   const WCHAR* pwszName);
	~SubAccount();

public:
	Account* getAccount() const;
	const WCHAR* getName() const;
	
	const WCHAR* getIdentity() const;
	void setIdentity(const WCHAR* pwszIdentity);
	const WCHAR* getSenderName() const;
	void setSenderName(const WCHAR* pwszName);
	const WCHAR* getSenderAddress() const;
	void setSenderAddress(const WCHAR* pwszAddress);
	qs::wstring_ptr getMyAddress() const;
	void setMyAddress(const WCHAR* pwszAddress);
	bool isMyAddress(const WCHAR* pwszMailbox,
					 const WCHAR* pwszHost) const;
	bool isMyAddress(const qs::AddressListParser& address) const;
	
	const WCHAR* getHost(Account::Host host) const;
	void setHost(Account::Host host,
				 const WCHAR* pwszHost);
	short getPort(Account::Host host) const;
	void setPort(Account::Host host,
				 short nPort);
	const WCHAR* getUserName(Account::Host host) const;
	void setUserName(Account::Host host,
					 const WCHAR* pwszUserName);
	const WCHAR* getPassword(Account::Host host) const;
	void setPassword(Account::Host host,
					 const WCHAR* pwszPassword);
	bool isSsl(Account::Host host) const;
	void setSsl(Account::Host host,
				bool bSsl);
	bool isLog(Account::Host host) const;
	void setLog(Account::Host host,
				bool bLog);
	
	long getTimeout() const;
	void setTimeout(long nTimeout);
	bool isConnectReceiveBeforeSend() const;
	void setConnectReceiveBeforeSend(bool bConnectReceiveBeforeSend);
	bool isTreatAsSent() const;
	void setTreatAsSent(bool bTreatAsSent);
	bool isAddMessageId() const;
	void setAddMessageId(bool bAddMessageId);
	unsigned int getSslOption() const;
	void setSslOption(unsigned int nSslOption);
	
	DialupType getDialupType() const;
	void setDialupType(DialupType type);
	const WCHAR* getDialupEntry() const;
	void setDialupEntry(const WCHAR* pwszEntry);
	bool isDialupShowDialog() const;
	void setDialupShowDialog(bool bShow);
	unsigned int getDialupDisconnectWait() const;
	void setDialupDisconnectWait(unsigned int nWait);
	
	qs::PrivateKey* getPrivateKey() const;
	qs::Certificate* getCertificate() const;
	
	int getProperty(const WCHAR* pwszSection,
					const WCHAR* pwszKey,
					int nDefault) const;
	void setProperty(const WCHAR* pwszSection,
					 const WCHAR* pwszKey,
					 int nValue);
	qs::wstring_ptr getProperty(const WCHAR* pwszSection,
								const WCHAR* pwszKey,
								const WCHAR* pwszDefault) const;
	void setProperty(const WCHAR* pwszSection,
					 const WCHAR* pwszKey,
					 const WCHAR* pwszValue);
	
	const WCHAR* getSyncFilterName() const;
	void setSyncFilterName(const WCHAR* pwszName);
	
	bool isSelf(const Message& msg) const;
	
	bool save() const;

// These methods are intended to be call from Account class.
public:
	bool setName(const WCHAR* pwszName);
	bool deletePermanent();

private:
	SubAccount(const SubAccount&);
	SubAccount& operator=(const SubAccount&);

private:
	struct SubAccountImpl* pImpl_;
};


/****************************************************************************
 *
 * AccountHandler
 *
 */

class AccountHandler
{
public:
	virtual ~AccountHandler();

public:
	virtual void subAccountListChanged(const AccountEvent& event) = 0;
	virtual void folderListChanged(const FolderListChangedEvent& event) = 0;
	virtual void accountDestroyed(const AccountEvent& event) = 0;
};


/****************************************************************************
 *
 * DefaultAccountHandler
 *
 */

class DefaultAccountHandler : public AccountHandler
{
public:
	DefaultAccountHandler();
	virtual ~DefaultAccountHandler();

public:
	virtual void subAccountListChanged(const AccountEvent& event);
	virtual void folderListChanged(const FolderListChangedEvent& event);
	virtual void accountDestroyed(const AccountEvent& event);
};


/****************************************************************************
 *
 * AccountEvent
 *
 */

class AccountEvent
{
public:
	AccountEvent(Account* pAccount);
	~AccountEvent();

public:
	Account* getAccount() const;

private:
	AccountEvent(const AccountEvent&);
	AccountEvent& operator=(const AccountEvent&);

private:
	Account* pAccount_;
};


/****************************************************************************
 *
 * FolderListChangedEvent
 *
 */

class FolderListChangedEvent
{
public:
	enum Type {
		TYPE_ALL,
		TYPE_ADD,
		TYPE_REMOVE,
		TYPE_RENAME,
		TYPE_FLAGS
	};

public:
	FolderListChangedEvent(Account* pAccount,
						   Type type,
						   Folder* pFolder);
	FolderListChangedEvent(Account* pAccount,
						   Type type,
						   Folder* pFolder,
						   unsigned int nOldFlags,
						   unsigned int nNewFlags);
	~FolderListChangedEvent();

public:
	Account* getAccount() const;
	Type getType() const;
	Folder* getFolder() const;
	unsigned int getOldFlags() const;
	unsigned int getNewFlags() const;

private:
	FolderListChangedEvent(const FolderListChangedEvent&);
	FolderListChangedEvent& operator=(const FolderListChangedEvent&);

private:
	Account* pAccount_;
	Type type_;
	Folder* pFolder_;
	unsigned int nOldFlags_;
	unsigned int nNewFlags_;
};


/****************************************************************************
 *
 * AccountCheckCallback
 *
 */

class QMEXPORTCLASS AccountCheckCallback : public MessageOperationCallback
{
public:
	enum Ignore {
		IGNORE_FALSE,
		IGNORE_TRUE,
		IGNORE_ALL
	};

public:
	virtual ~AccountCheckCallback();

public:
	virtual Ignore isIgnoreError(MessageHolder* pmh) = 0;
};

}

#include <qmaccount.inl>

#endif // __QMACCOUNT_H__
