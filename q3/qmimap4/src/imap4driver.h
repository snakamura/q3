/*
 * $Id$
 *
 * Copyright(C) 1998-2003 Satoshi Nakamura
 * All rights reserved.
 *
 */

#ifndef __IMAP4DRIVER_H__
#define __IMAP4DRIVER_H__

#include <qmprotocoldriver.h>

#include <qs.h>
#include <qssocket.h>

#include "imap4.h"
#include "util.h"

namespace qmimap4 {

class Imap4Driver;
class Imap4Factory;
class FolderUtil;
class FolderListGetter;
class SessionCache;
class SessionCacher;

class OfflineJobManager;


/****************************************************************************
 *
 * Imap4Driver
 *
 */

class Imap4Driver : public qm::ProtocolDriver
{
public:
	Imap4Driver(qm::Account* pAccount,
		const qm::Security* pSecurity, qs::QSTATUS* pstatus);
	virtual ~Imap4Driver();

public:
	virtual bool isSupport(qm::Account::Support support);
	virtual qs::QSTATUS setOffline(bool bOffline);
	virtual qs::QSTATUS setForceOnline(bool bOnline);
	virtual qs::QSTATUS save();
	
	virtual qs::QSTATUS createFolder(qm::SubAccount* pSubAccount,
		const WCHAR* pwszName, qm::Folder* pParent,
		qm::NormalFolder** ppFolder);
	virtual qs::QSTATUS createDefaultFolders(
		qm::Folder*** pppFolder, size_t* pnCount);
	virtual qs::QSTATUS getRemoteFolders(qm::SubAccount* pSubAccount,
		std::pair<qm::Folder*, bool>** pFolder, size_t* pnCount);
	
	virtual qs::QSTATUS getMessage(qm::SubAccount* pSubAccount,
		qm::MessageHolder* pmh, unsigned int nFlags,
		qm::Message* pMessage, bool* pbGet, bool* pbMadeSeen);
	virtual qs::QSTATUS setMessagesFlags(qm::SubAccount* pSubAccount,
		qm::NormalFolder* pFolder, const qm::Folder::MessageHolderList& l,
		unsigned int nFlags, unsigned int nMask);
	virtual qs::QSTATUS appendMessage(qm::SubAccount* pSubAccount,
		qm::NormalFolder* pFolder, const CHAR* pszMessage, unsigned int nFlags);
	virtual qs::QSTATUS removeMessages(qm::SubAccount* pSubAccount,
		qm::NormalFolder* pFolder, const qm::Folder::MessageHolderList& l);
	virtual qs::QSTATUS copyMessages(qm::SubAccount* pSubAccount,
		const qm::Folder::MessageHolderList& l, qm::NormalFolder* pFolderFrom,
		qm::NormalFolder* pFolderTo, bool bMove);
	virtual qs::QSTATUS clearDeletedMessages(
		qm::SubAccount* pSubAccount, qm::NormalFolder* pFolder);

public:
	OfflineJobManager* getOfflineJobManager() const;

private:
	qs::QSTATUS prepareSessionCache(qm::SubAccount* pSubAccount);
	qs::QSTATUS setFlags(Imap4* pImap4, const Range& range,
		qm::NormalFolder* pFolder, const qm::Folder::MessageHolderList& l,
		unsigned int nFlags, unsigned int nMask);

private:
	Imap4Driver(const Imap4Driver&);
	Imap4Driver& operator=(const Imap4Driver&);

private:
	class ProcessHook
	{
	public:
		virtual ~ProcessHook();
	
	public:
		virtual qs::QSTATUS processFetchResponse(ResponseFetch* pFetch);
		virtual qs::QSTATUS processListResponse(ResponseList* pList);
		virtual qs::QSTATUS processExpungeResponse(ResponseExpunge* pExpunge);
	};
	
	class CallbackImpl : public AbstractCallback
	{
	public:
		CallbackImpl(qm::SubAccount* pSubAccount,
			const qm::Security* pSecurity, qs::QSTATUS* pstatus);
		virtual ~CallbackImpl();
	
	public:
		void setProcessHook(ProcessHook* pProcessHook);
	
	public:
		virtual bool isCanceled(bool bForce) const;
		virtual qs::QSTATUS initialize();
		virtual qs::QSTATUS lookup();
		virtual qs::QSTATUS connecting();
		virtual qs::QSTATUS connected();
	
	public:
		virtual qs::QSTATUS authenticating();
		virtual qs::QSTATUS setRange(unsigned int nMin, unsigned int nMax);
		virtual qs::QSTATUS setPos(unsigned int nPos);
		virtual qs::QSTATUS response(Response* pResponse);
	
	private:
		CallbackImpl(const CallbackImpl&);
		CallbackImpl& operator=(const CallbackImpl&);
	
	private:
		ProcessHook* pProcessHook_;
	};
	
	class FlagProcessHook : public ProcessHook
	{
	public:
		FlagProcessHook(qm::NormalFolder* pFolder);
		virtual qs::QSTATUS processFetchResponse(ResponseFetch* pFetch);
	
	private:
		qm::NormalFolder* pFolder_;
	};
	
	class Hook
	{
	public:
		Hook(CallbackImpl* pCallback, ProcessHook* pProcessHook);
		~Hook();
	
	private:
		Hook(const Hook&);
		Hook& operator=(const Hook&);
	
	private:
		CallbackImpl* pCallback_;
	};

private:
	qm::Account* pAccount_;
	const qm::Security* pSecurity_;
	SessionCache* pSessionCache_;
	CallbackImpl* pCallback_;
	OfflineJobManager* pOfflineJobManager_;
	bool bOffline_;
	unsigned int nForceOnline_;
	qs::CriticalSection cs_;
};


/****************************************************************************
 *
 * Imap4Factory
 *
 */

class Imap4Factory : public qm::ProtocolFactory
{
private:
	Imap4Factory();

public:
	virtual ~Imap4Factory();

protected:
	virtual qs::QSTATUS createDriver(qm::Account* pAccount,
		const qm::Security* pSecurity, qm::ProtocolDriver** ppProtocolDriver);

private:
	Imap4Factory(const Imap4Factory&);
	Imap4Factory& operator=(const Imap4Factory&);

private:
	static Imap4Factory factory__;
};


/****************************************************************************
 *
 * FolderUtil
 *
 */

class FolderUtil
{
public:
	FolderUtil(qm::Account* pAccount, qs::QSTATUS* pstatus);
	~FolderUtil();

public:
	bool isRootFolderSpecified() const;
	const WCHAR* getRootFolder() const;
	qs::QSTATUS getFolderData(const WCHAR* pwszName,
		WCHAR cSeparator, unsigned int nAttributes,
		qs::WSTRING* pwstrName, unsigned int* pnFlags) const;

private:
	FolderUtil(const FolderUtil&);
	FolderUtil& operator=(const FolderUtil&);

private:
	qs::WSTRING wstrRootFolder_;
	qs::WSTRING wstrSpecialFolders_[4];
};


/****************************************************************************
 *
 * FolderListGetter
 *
 */

class FolderListGetter
{
public:
	FolderListGetter(qm::Account* pAccount, qm::SubAccount* pSubAccount,
		const qm::Security* pSecurity, qs::QSTATUS* pstatus);
	~FolderListGetter();

public:
	qs::QSTATUS getFolders(std::pair<qm::Folder*, bool>** ppFolder, size_t* pnCount);

private:
	qs::QSTATUS connect();
	qs::QSTATUS listNamespaces();
	qs::QSTATUS listFolders();
	qs::QSTATUS getFolder(const WCHAR* pwszName, WCHAR cSeparator,
		unsigned int nFlags, unsigned int* pnId, qm::Folder** ppFolder);

private:
	FolderListGetter(const FolderListGetter&);
	FolderListGetter& operator=(const FolderListGetter&);

private:
	struct FolderData
	{
		qs::WSTRING wstrMailbox_;
		WCHAR cSeparator_;
		unsigned int nFlags_;
	};
	
	struct FolderDataLess :
		public std::binary_function<FolderData, FolderData, bool>
	{
		bool operator()(const FolderData& lhs, const FolderData& rhs) const;
	};

private:
	typedef std::vector<std::pair<qs::WSTRING, WCHAR> > NamespaceList;
	typedef std::vector<FolderData> FolderDataList;
	typedef std::vector<std::pair<qm::Folder*, bool> > FolderList;

private:
	class CallbackImpl : public AbstractCallback
	{
	public:
		CallbackImpl(FolderListGetter* pGetter,
			const qm::Security* pSecurity, qs::QSTATUS* pstatus);
		virtual ~CallbackImpl();
	
	public:
		void setNamespaceList(NamespaceList* pListNamespace);
		void setFolderDataList(FolderDataList* pListFolderData);
	
	public:
		virtual bool isCanceled(bool bForce) const;
		virtual qs::QSTATUS initialize();
		virtual qs::QSTATUS lookup();
		virtual qs::QSTATUS connecting();
		virtual qs::QSTATUS connected();
	
	public:
		virtual qs::QSTATUS authenticating();
		virtual qs::QSTATUS setRange(unsigned int nMin, unsigned int nMax);
		virtual qs::QSTATUS setPos(unsigned int nPos);
		virtual qs::QSTATUS response(Response* pResponse);
	
	private:
		qs::QSTATUS processNamespace(ResponseNamespace* pNamespace);
		qs::QSTATUS processList(ResponseList* pList);
	
	private:
		CallbackImpl(const CallbackImpl&);
		CallbackImpl& operator=(const CallbackImpl&);
	
	private:
		FolderListGetter* pGetter_;
		NamespaceList* pListNamespace_;
		FolderDataList* pListFolderData_;
	};
	friend class CallbackImpl;

private:
	qm::Account* pAccount_;
	qm::SubAccount* pSubAccount_;
	const qm::Security* pSecurity_;
	FolderUtil* pFolderUtil_;
	Imap4* pImap4_;
	CallbackImpl* pCallback_;
	qs::Logger* pLogger_;
	NamespaceList listNamespace_;
	FolderDataList listFolderData_;
	FolderList listFolder_;
};


/****************************************************************************
 *
 * Session
 *
 */

struct Session
{
	qm::NormalFolder* pFolder_;
	Imap4* pImap4_;
	qs::Logger* pLogger_;
	unsigned int nLastSelectedTime_;
};


/****************************************************************************
 *
 * SessionCache
 *
 */

class SessionCache
{
public:
	SessionCache(qm::Account* pAccount, qm::SubAccount* pSubAccount,
		AbstractCallback* pCallback, size_t nMaxSession, qs::QSTATUS* pstatus);
	~SessionCache();

public:
	qm::SubAccount* getSubAccount() const;
	qs::QSTATUS getSession(qm::NormalFolder* pFolder, Session* pSession);
	void releaseSession(const Session& session);

private:
	bool isNeedSelect(qm::NormalFolder* pFolder,
		unsigned int nLastSelectedTime);

private:
	SessionCache(const SessionCache&);
	SessionCache& operator=(const SessionCache&);

private:
	typedef std::vector<Session> SessionList;

private:
	qm::Account* pAccount_;
	qm::SubAccount* pSubAccount_;
	AbstractCallback* pCallback_;
	size_t nMaxSession_;
	bool bReselect_;
	SessionList listSession_;
};


/****************************************************************************
 *
 * SessionCacher
 *
 */

class SessionCacher
{
public:
	SessionCacher(SessionCache* pCache, qm::NormalFolder* pFolder,
		Imap4** ppImap4, qs::QSTATUS* pstatus);
	~SessionCacher();

public:
	void release();

private:
	SessionCacher(const SessionCacher&);
	SessionCacher& operator=(const SessionCacher&);

private:
	SessionCache* pCache_;
	Session session_;
};

}

#endif // __IMAP4DRIVER_H__
