/*
 * $Id$
 *
 * Copyright(C) 1998-2003 Satoshi Nakamura
 * All rights reserved.
 *
 */

#ifndef __QMACCOUNT_H__
#define __QMACCOUNT_H__

#include <qm.h>
#include <qmfolder.h>
#include <qmmessagecache.h>

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
class FolderListChangedEvent;

class Folder;
	class NormalFolder;
class MessageHolder;
class Message;
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
		GETMESSAGEFLAG_FORCEONLINE	= 0x20
	};
	
	enum Support {
		SUPPORT_REMOTEFOLDER		= 0x01,
		SUPPORT_LOCALFOLDERDOWNLOAD	= 0x02
	};

public:
	typedef std::vector<SubAccount*> SubAccountList;
	typedef std::vector<Folder*> FolderList;

public:
	Account(const WCHAR* pwszPath,
		const Security* pSecurity, qs::QSTATUS* pstatus);
	~Account();

public:
	const WCHAR* getName() const;
	const WCHAR* getPath() const;
	const WCHAR* getClass() const;
	const WCHAR* getType(Host host) const;
	bool isSupport(Support support) const;
	
	qs::QSTATUS getProperty(const WCHAR* pwszSection,
		const WCHAR* pwszKey, int nDefault, int* pnValue) const;
	qs::QSTATUS getProperty(const WCHAR* pwszSection, const WCHAR* pwszName,
		const WCHAR* pwszDefault, qs::WSTRING* pwstrValue) const;
	
	SubAccount* getSubAccount(const WCHAR* pwszName) const;
	SubAccount* getSubAccountByIdentity(const WCHAR* pwszIdentity) const;
	const SubAccountList& getSubAccounts() const;
	qs::QSTATUS addSubAccount(SubAccount* pSubAccount);
	qs::QSTATUS removeSubAccount(SubAccount* pSubAccount);
	qs::QSTATUS renameSubAccount(
		SubAccount* pSubAccount, const WCHAR* pwszName);
	SubAccount* getCurrentSubAccount() const;
	void setCurrentSubAccount(SubAccount* pSubAccount);
	
	qs::QSTATUS getFolder(const WCHAR* pwszName, Folder** ppFolder) const;
	Folder* getFolder(Folder* pParent, const WCHAR* pwszName) const;
	Folder* getFolderById(unsigned int nId) const;
	Folder* getFolderByFlag(unsigned int nFlag) const;
	const FolderList& getFolders() const;
	qs::QSTATUS createNormalFolder(const WCHAR* pwszName,
		Folder* pParent, bool bRemote);
	qs::QSTATUS createQueryFolder(const WCHAR* pwszName,
		Folder* pParent, const WCHAR* pwszMacro);
	qs::QSTATUS removeFolder(Folder* pFolder);
	qs::QSTATUS renameFolder(Folder* pFolder, const WCHAR* pwszName);
	qs::QSTATUS showFolder(Folder* pFolder, bool bShow);
	qs::QSTATUS updateFolders();
	
	qs::QSTATUS setOffline(bool bOffline);
	qs::QSTATUS setForceOnline(bool bOnline);
	qs::QSTATUS compact();
	qs::QSTATUS save() const;
	qs::QSTATUS flushMessageStore() const;
	qs::QSTATUS importMessage(NormalFolder* pFolder,
		const CHAR* pszMessage, unsigned int nFlags);
	
	qs::QSTATUS addAccountHandler(AccountHandler* pHandler);
	qs::QSTATUS removeAccountHandler(AccountHandler* pHandler);

// These methods are intended to be called from Document class
public:
	qs::QSTATUS remove();
	qs::QSTATUS rename(const WCHAR* pwszName);

// These methods are intended to be called from NormalFolder class
public:
	qs::QSTATUS getData(MessageCacheKey key,
		MessageCacheItem item, qs::WSTRING* pwstrData) const;
	qs::QSTATUS getMessage(MessageHolder* pmh,
		unsigned int nFlags, Message* pMessage) const;
	qs::QSTATUS setMessagesFlags(NormalFolder* pFolder,
		const Folder::MessageHolderList& l,
		unsigned int nFlags, unsigned int nMask) const;
	qs::QSTATUS appendMessage(NormalFolder* pFolder, const CHAR* pszMessage,
		const Message& msgHeader, unsigned int nFlags, unsigned int nSize);
	qs::QSTATUS removeMessages(NormalFolder* pFolder,
		const Folder::MessageHolderList& l, bool bDirect) const;
	qs::QSTATUS copyMessages(const Folder::MessageHolderList& l,
		NormalFolder* pFolderFrom, NormalFolder* pFolderTo, bool bMove) const;
	qs::QSTATUS clearDeletedMessages(NormalFolder* pFolder) const;

// These methods are intended to be called from ReceiveSession class
public:
	ProtocolDriver* getProtocolDriver() const;
	qs::QSTATUS storeMessage(NormalFolder* pFolder, const CHAR* pszMessage,
		const Message* pHeader, unsigned int nId, unsigned int nFlags,
		unsigned int nSize, bool bIndexOnly, MessageHolder** ppmh);
	qs::QSTATUS unstoreMessage(MessageHolder* pmh);
	qs::QSTATUS cloneMessage(MessageHolder* pmh,
		NormalFolder* pFolderTo, MessageHolder** ppmh);
	qs::QSTATUS updateMessage(MessageHolder* pmh, const CHAR* pszMessage);

// These methods are intended to be called from ProtocolDriver class
public:
	unsigned int generateFolderId() const;

// These methods are intended to be called from SyncManager class
public:
	qs::QSTATUS openLogger(Host host, qs::Logger** ppLogger) const;

private:
	Account(const Account&);
	Account& operator=(const Account&);

private:
	class AccountImpl* pImpl_;
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

public:
	SubAccount(Account* pAccount, qs::Profile* pProfile,
		const WCHAR* pwszName, qs::QSTATUS* pstatus);
	~SubAccount();

public:
	Account* getAccount() const;
	const WCHAR* getName() const;
	qs::QSTATUS getDisplayName(qs::WSTRING* pwstrName) const;
	
	const WCHAR* getIdentity() const;
	qs::QSTATUS setIdentity(const WCHAR* pwszIdentity);
	const WCHAR* getSenderName() const;
	qs::QSTATUS setSenderName(const WCHAR* pwszName);
	const WCHAR* getSenderAddress() const;
	qs::QSTATUS setSenderAddress(const WCHAR* pwszAddress);
	qs::QSTATUS getMyAddress(qs::WSTRING* pwstrAddress) const;
	qs::QSTATUS setMyAddress(const WCHAR* pwszAddress);
	bool isMyAddress(const WCHAR* pwszMailbox, const WCHAR* pwszHost) const;
	bool isMyAddress(const qs::AddressListParser& address) const;
	
	const WCHAR* getHost(Account::Host host) const;
	qs::QSTATUS setHost(Account::Host host, const WCHAR* pwszHost);
	short getPort(Account::Host host) const;
	void setPort(Account::Host host, short nPort);
	const WCHAR* getUserName(Account::Host host) const;
	qs::QSTATUS setUserName(Account::Host host, const WCHAR* pwszUserName);
	const WCHAR* getPassword(Account::Host host) const;
	qs::QSTATUS setPassword(Account::Host host, const WCHAR* pwszPassword);
	bool isSsl(Account::Host host) const;
	void setSsl(Account::Host host, bool bSsl);
	bool isLog(Account::Host host) const;
	void setLog(Account::Host host, bool bLog);
	
	long getTimeout() const;
	void setTimeout(long nTimeout);
	bool isConnectReceiveBeforeSend() const;
	void setConnectReceiveBeforeSend(bool bConnectReceiveBeforeSend);
	bool isTreatAsSent() const;
	void setTreatAsSent(bool bTreatAsSent);
	bool isAllowUnverifiedCertificate() const;
	void setAllowUnverifiedCertificate(bool bAllow) const;
	
	DialupType getDialupType() const;
	void setDialupType(DialupType type);
	const WCHAR* getDialupEntry() const;
	qs::QSTATUS setDialupEntry(const WCHAR* pwszEntry);
	bool isDialupShowDialog() const;
	void setDialupShowDialog(bool bShow);
	unsigned int getDialupDisconnectWait() const;
	void setDialupDisconnectWait(unsigned int nWait);
	
	qs::PrivateKey* getPrivateKey() const;
	qs::Certificate* getCertificate() const;
	
	qs::QSTATUS getProperty(const WCHAR* pwszSection,
		const WCHAR* pwszKey, int nDefault, int* pnValue) const;
	qs::QSTATUS setProperty(const WCHAR* pwszSection,
		const WCHAR* pwszKey, int nValue);
	qs::QSTATUS getProperty(const WCHAR* pwszSection, const WCHAR* pwszKey,
		const WCHAR* pwszDefault, qs::WSTRING* pwstrValue) const;
	qs::QSTATUS setProperty(const WCHAR* pwszSection,
		const WCHAR* pwszKey, const WCHAR* pwszValue);
	
	const WCHAR* getSyncFilterName() const;
	qs::QSTATUS setSyncFilterName(const WCHAR* pwszName);
	
	qs::QSTATUS isSelf(const Message& msg, bool* pbSelf) const;
	
	qs::QSTATUS save() const;

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
	virtual qs::QSTATUS folderListChanged(const FolderListChangedEvent& event) = 0;
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
		TYPE_SHOW,
		TYPE_HIDE
	};

public:
	FolderListChangedEvent(Account* pAccount, Type type, Folder* pFolder);
	~FolderListChangedEvent();

public:
	Account* getAccount() const;
	Type getType() const;
	Folder* getFolder() const;

private:
	FolderListChangedEvent(const FolderListChangedEvent&);
	FolderListChangedEvent& operator=(const FolderListChangedEvent&);

private:
	Account* pAccount_;
	Type type_;
	Folder* pFolder_;
};

}

#endif // __QMACCOUNT_H__
