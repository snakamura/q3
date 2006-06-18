/*
 * $Id$
 *
 * Copyright(C) 1998-2006 Satoshi Nakamura
 * All rights reserved.
 *
 */

#ifndef __QMACCOUNT_H__
#define __QMACCOUNT_H__

#include <qm.h>
#include <qmfolder.h>
#include <qmmessageindex.h>
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
class AccountManager;
class AccountManagerHandler;
class AccountManagerEvent;

class Message;
class MessageHolder;
class MessageHolderHandler;
class MessageOperationCallback;
class PasswordManager;
class ProtocolDriver;
class Security;
class JunkFilter;
class UndoItemList;
class URI;


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
		GETMESSAGEFLAG_FALLBACK		= 0x20
	};
	
	enum OpFlag {
		OPFLAG_NONE			= 0x0000,
		OPFLAG_ACTIVE		= 0x0100,
		OPFLAG_BACKGROUND	= 0x0200,
		
		OPFLAG_MASK			= 0x0f00
	};
	
	enum RemoveFlag {
		REMOVEFLAG_NONE			= 0x00,
		REMOVEFLAG_DIRECT		= 0x01
	};
	
	enum CopyFlag {
		COPYFLAG_NONE		= 0x00,
		COPYFLAG_MOVE		= 0x01,
		COPYFLAG_MANAGEJUNK	= 0x02
	};
	
	enum StoreFlag {
		STOREFLAG_NONE			= 0x00,
		STOREFLAG_INDEXONLY		= 0x01
	};
	
	enum ResultFlag {
		RESULTFLAG_NONE			= 0x00,
		RESULTFLAG_DESTROYED	= 0x01,
		RESULTFLAG_MOVED		= 0x02,
		
		RESULTFLAG_ALL			= 0x0f
	};
	
	enum Support {
		SUPPORT_REMOTEFOLDER			= 0x01,
		SUPPORT_LOCALFOLDERSYNC			= 0x02,
		SUPPORT_LOCALFOLDERGETMESSAGE	= 0x04,
		SUPPORT_LOCALFOLDERDOWNLOAD		= 0x08,
		SUPPORT_EXTERNALLINK			= 0x10,
		SUPPORT_DELETEDMESSAGE			= 0x20,
		SUPPORT_JUNKFILTER				= 0x40
	};

public:
	typedef std::vector<SubAccount*> SubAccountList;
	typedef std::vector<Folder*> FolderList;
	typedef std::vector<NormalFolder*> NormalFolderList;

public:
	Account(const WCHAR* pwszPath,
			const Security* pSecurity,
			PasswordManager* pPasswordManager,
			JunkFilter* pJunkFilter);
	~Account();

public:
	const WCHAR* getName() const;
	const WCHAR* getPath() const;
	const WCHAR* getClass() const;
	const WCHAR* getType(Host host) const;
	bool isSupport(Support support) const;
	const WCHAR* getMessageStorePath() const;
	bool isMultiMessageStore() const;
	bool isStoreDecodedMessage() const;
	void setStoreDecodedMessage(bool bStore) const;
	
	int getPropertyInt(const WCHAR* pwszSection,
					   const WCHAR* pwszKey) const;
	int getPropertyInt(const WCHAR* pwszSection,
					   const WCHAR* pwszKey,
					   int nDefault) const;
	void setPropertyInt(const WCHAR* pwszSection,
						const WCHAR* pwszKey,
						int nValue);
	qs::wstring_ptr getPropertyString(const WCHAR* pwszSection,
									  const WCHAR* pwszKey) const;
	qs::wstring_ptr getPropertyString(const WCHAR* pwszSection,
									  const WCHAR* pwszKey,
									  const WCHAR* pwszDefault) const;
	void setPropertyString(const WCHAR* pwszSection,
						   const WCHAR* pwszKey,
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
	Folder* getFolderByBoxFlag(unsigned int nBoxFlag) const;
	Folder* getFolderByParam(const WCHAR* pwszName,
							 const WCHAR* pwszValue) const;
	const FolderList& getFolders() const;
	void getNormalFolders(const WCHAR* pwszName,
						  bool bRecursive,
						  NormalFolderList* pList) const;
	NormalFolder* createNormalFolder(const WCHAR* pwszName,
									 Folder* pParent,
									 bool bRemote,
									 bool bSyncable);
	QueryFolder* createQueryFolder(const WCHAR* pwszName,
								   Folder* pParent,
								   const WCHAR* pwszDriver,
								   const WCHAR* pwszCondition,
								   const WCHAR* pwszTargetFolder,
								   bool bRecursive);
	bool removeFolder(Folder* pFolder);
	bool renameFolder(Folder* pFolder,
					  const WCHAR* pwszName);
	bool moveFolder(Folder* pFolder,
					Folder* pParent,
					const WCHAR* pwszName);
	void setFolderFlags(Folder* pFolder,
						unsigned int nFlags,
						unsigned int nMask);
	bool updateFolders();
	std::pair<const WCHAR**, size_t> getFolderParamNames(Folder* pFolder) const;
	bool isRemoteMessageFolder(const NormalFolder* pFolder);
	
	void setOffline(bool bOffline);
	bool compact(MessageOperationCallback* pCallback);
	bool salvage(NormalFolder* pFolder,
				 MessageOperationCallback* pCallback);
	bool check(AccountCheckCallback* pCallback);
	bool save(bool bForce) const;
	bool saveMessages(bool bForce) const;
	bool flushMessageStore() const;
	bool importMessage(NormalFolder* pFolder,
					   const CHAR* pszMessage,
					   size_t nLen,
					   unsigned int nFlags);
	
	bool appendMessage(NormalFolder* pFolder,
					   const Message& msg,
					   unsigned int nFlags,
					   const WCHAR* pwszLabel,
					   unsigned int nAppendFlags,
					   UndoItemList* pUndoItemList,
					   MessagePtr* pptr);
	bool removeMessages(const MessageHolderList& l,
						Folder* pFolder,
						unsigned int nRemoveFlags,
						MessageOperationCallback* pCallback,
						UndoItemList* pUndoItemList,
						unsigned int* pnResultFlags);
	bool copyMessages(const MessageHolderList& l,
					  Folder* pFolderFrom,
					  NormalFolder* pFolderTo,
					  unsigned int nCopyFlags,
					  MessageOperationCallback* pCallback,
					  UndoItemList* pUndoItemList,
					  unsigned int* pnResultFlags);
	bool setMessagesFlags(const MessageHolderList& l,
						  unsigned int nFlags,
						  unsigned int nMask,
						  UndoItemList* pUndoItemList);
	bool setMessagesLabel(const MessageHolderList& l,
						  const WCHAR* pwszLabel,
						  UndoItemList* pUndoItemList);
	bool deleteMessagesCache(const MessageHolderList& l);
	
	bool isSeen(const MessageHolder* pmh) const;
	bool isSeen(unsigned int nFlags) const;
	
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
	qs::wstring_ptr getIndex(unsigned int nKey,
							 unsigned int nLength,
							 MessageIndexName name) const;
	bool getMessage(MessageHolder* pmh,
					unsigned int nFlags,
					unsigned int nSecurityMode,
					Message* pMessage);
	bool setLabel(MessageHolder* pmh,
				  const WCHAR* pwszLabel);
	void fireMessageHolderFlagsChanged(MessageHolder* pmh,
									   unsigned int nOldFlags,
									   unsigned int nNewFlags);
	void fireMessageHolderKeysChanged(MessageHolder* pmh);
	void fireMessageHolderDestroyed(MessageHolder* pmh);

// These methods are intended to be called from ReceiveSession class
public:
	ProtocolDriver* getProtocolDriver() const;
	MessageHolder* storeMessage(NormalFolder* pFolder,
								const CHAR* pszMessage,
								size_t nLen,
								const Message* pHeader,
								unsigned int nId,
								unsigned int nFlags,
								const WCHAR* pwszLabel,
								unsigned int nSize,
								unsigned int nStoreFlags,
								unsigned int* pnResultFlags);
	bool unstoreMessages(const MessageHolderList& l,
						 MessageOperationCallback* pCallback);
	MessageHolder* cloneMessage(MessageHolder* pmh,
								NormalFolder* pFolderTo);
	bool updateMessage(MessageHolder* pmh,
					   const CHAR* pszMessage,
					   size_t nLen,
					   const Message* pHeader);
	bool prepareGetMessage(NormalFolder* pFolder);

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
	
	enum Secure {
		SECURE_NONE,
		SECURE_SSL,
		SECURE_STARTTLS
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
	bool isSelf(const WCHAR* pwszMailbox,
				const WCHAR* pwszHost) const;
	bool isSelf(const qs::AddressListParser& address) const;
	
	const WCHAR* getHost(Account::Host host) const;
	void setHost(Account::Host host,
				 const WCHAR* pwszHost);
	short getPort(Account::Host host) const;
	void setPort(Account::Host host,
				 short nPort);
	const WCHAR* getUserName(Account::Host host) const;
	void setUserName(Account::Host host,
					 const WCHAR* pwszUserName);
	Secure getSecure(Account::Host host) const;
	void setSecure(Account::Host host,
				   Secure secure);
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
	bool isAutoApplyRules() const;
	void setAutoApplyRules(bool bAutoApplyRules);
	const WCHAR* getTransferEncodingFor8Bit();
	void setTransferEncodingFor8Bit(const WCHAR* pwszTransferEncodingFor8Bit);
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
	
	std::auto_ptr<qs::PrivateKey> getPrivateKey(PasswordManager* pPasswordManager) const;
	std::auto_ptr<qs::Certificate> getCertificate(PasswordManager* pPasswordManager) const;
	
	int getPropertyInt(const WCHAR* pwszSection,
					   const WCHAR* pwszKey) const;
	int getPropertyInt(const WCHAR* pwszSection,
					   const WCHAR* pwszKey,
					   int nDefault) const;
	void setPropertyInt(const WCHAR* pwszSection,
						const WCHAR* pwszKey,
						int nValue);
	qs::wstring_ptr getPropertyString(const WCHAR* pwszSection,
									  const WCHAR* pwszKey) const;
	qs::wstring_ptr getPropertyString(const WCHAR* pwszSection,
									  const WCHAR* pwszKey,
									  const WCHAR* pwszDefault) const;
	void setPropertyString(const WCHAR* pwszSection,
						   const WCHAR* pwszKey,
						   const WCHAR* pwszValue);
	
	const WCHAR* getSyncFilterName() const;
	void setSyncFilterName(const WCHAR* pwszName);
	
	bool isJunkFilterEnabled() const;
	void setJunkFilterEnabled(bool bEnabled);
	
	bool isSelf(const Message& msg) const;
	
	bool save(bool bForce) const;

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
	virtual void currentSubAccountChanged(const AccountEvent& event) = 0;
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
	virtual void currentSubAccountChanged(const AccountEvent& event);
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


/****************************************************************************
 *
 * AccountManager
 *
 */

class QMEXPORTCLASS AccountManager
{
public:
	typedef std::vector<Account*> AccountList;

public:
	virtual ~AccountManager();

public:
	virtual Account* getAccount(const WCHAR* pwszName) const = 0;
	virtual const AccountList& getAccounts() const = 0;
	virtual bool hasAccount(const WCHAR* pwszName) const = 0;
	virtual void addAccount(std::auto_ptr<Account> pAccount) = 0;
	virtual void removeAccount(Account* pAccount) = 0;
	virtual bool renameAccount(Account* pAccount,
							   const WCHAR* pwszName) = 0;
	virtual Folder* getFolder(Account* pAccount,
							  const WCHAR* pwszName) const = 0;
	virtual MessagePtr getMessage(const URI& uri) const = 0;
	virtual void addAccountManagerHandler(AccountManagerHandler* pHandler) = 0;
	virtual void removeAccountManagerHandler(AccountManagerHandler* pHandler) = 0;
};


/****************************************************************************
 *
 * AccountManagerHandler
 *
 */

class QMEXPORTCLASS AccountManagerHandler
{
public:
	virtual ~AccountManagerHandler();

public:
	virtual void accountListChanged(const AccountManagerEvent& event) = 0;
};


/****************************************************************************
 *
 * AccountManagerEvent
 *
 */

class QMEXPORTCLASS AccountManagerEvent
{
public:
	enum Type {
		TYPE_ALL,
		TYPE_ADD,
		TYPE_REMOVE
	};

public:
	AccountManagerEvent(AccountManager* pAccountManager,
						Type type,
						Account* pAccount);
	~AccountManagerEvent();

public:
	AccountManager* getAccountManager() const;
	Type getType() const;
	Account* getAccount() const;

private:
	AccountManagerEvent(const AccountManagerEvent&);
	AccountManagerEvent& operator=(const AccountManagerEvent&);

private:
	AccountManager* pAccountManager_;
	Type type_;
	Account* pAccount_;
};

}

#include <qmaccount.inl>

#endif // __QMACCOUNT_H__
