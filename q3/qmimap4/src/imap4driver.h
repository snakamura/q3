/*
 * $Id$
 *
 * Copyright(C) 1998-2004 Satoshi Nakamura
 * All rights reserved.
 *
 */

#ifndef __IMAP4DRIVER_H__
#define __IMAP4DRIVER_H__

#include <qmprotocoldriver.h>

#include <qs.h>
#include <qssocket.h>
#include <qsthread.h>

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
				const qm::Security* pSecurity);
	virtual ~Imap4Driver();

public:
	virtual bool init();
	virtual bool save();
	virtual bool isSupport(qm::Account::Support support);
	virtual void setOffline(bool bOffline);
	
	virtual std::auto_ptr<qm::NormalFolder> createFolder(qm::SubAccount* pSubAccount,
														 const WCHAR* pwszName,
														 qm::Folder* pParent);
	virtual bool removeFolder(qm::SubAccount* pSubAccount,
							  qm::NormalFolder* pFolder);
	virtual bool renameFolder(qm::SubAccount* pSubAccount,
							  qm::NormalFolder* pFolder,
							  const WCHAR* pwszName);
	virtual bool createDefaultFolders(qm::Account::FolderList* pList);
	virtual bool getRemoteFolders(qm::SubAccount* pSubAccount,
								  RemoteFolderList* pList);
	
	virtual bool getMessage(qm::SubAccount* pSubAccount,
							qm::MessageHolder* pmh,
							unsigned int nFlags,
							qs::xstring_ptr* pstrMessage,
							qm::Message::Flag* pFlag,
							bool* pbMadeSeen);
	virtual bool setMessagesFlags(qm::SubAccount* pSubAccount,
								  qm::NormalFolder* pFolder,
								  const qm::MessageHolderList& l,
								  unsigned int nFlags,
								  unsigned int nMask);
	virtual bool appendMessage(qm::SubAccount* pSubAccount,
							   qm::NormalFolder* pFolder,
							   const CHAR* pszMessage,
							   unsigned int nFlags);
	virtual bool removeMessages(qm::SubAccount* pSubAccount,
								qm::NormalFolder* pFolder,
								const qm::MessageHolderList& l);
	virtual bool copyMessages(qm::SubAccount* pSubAccount,
							  const qm::MessageHolderList& l,
							  qm::NormalFolder* pFolderFrom,
							  qm::NormalFolder* pFolderTo,
							  bool bMove);
	virtual bool clearDeletedMessages(qm::SubAccount* pSubAccount,
									  qm::NormalFolder* pFolder);

public:
	OfflineJobManager* getOfflineJobManager() const;
	bool search(qm::SubAccount* pSubAccount,
				qm::NormalFolder* pFolder,
				const WCHAR* pwszCondition,
				const WCHAR* pwszCharset,
				bool bUseCharset,
				qm::MessageHolderList* pList);

private:
	bool prepareSessionCache(qm::SubAccount* pSubAccount);
	bool setFlags(Imap4* pImap4,
				  const Range& range,
				  qm::NormalFolder* pFolder,
				  const qm::MessageHolderList& l,
				  unsigned int nFlags,
				  unsigned int nMask);

private:
	Imap4Driver(const Imap4Driver&);
	Imap4Driver& operator=(const Imap4Driver&);

private:
	class ProcessHook
	{
	public:
		virtual ~ProcessHook();
	
	public:
		virtual bool processFetchResponse(ResponseFetch* pFetch);
		virtual bool processListResponse(ResponseList* pList);
		virtual bool processExpungeResponse(ResponseExpunge* pExpunge);
		virtual bool processSearchResponse(ResponseSearch* pSearch);
	};
	
	class CallbackImpl : public AbstractCallback
	{
	public:
		CallbackImpl(qm::SubAccount* pSubAccount,
					 const qm::Security* pSecurity);
		virtual ~CallbackImpl();
	
	public:
		void setProcessHook(ProcessHook* pProcessHook);
	
	public:
		virtual bool isCanceled(bool bForce) const;
		virtual void initialize();
		virtual void lookup();
		virtual void connecting();
		virtual void connected();
	
	public:
		virtual void authenticating();
		virtual void setRange(unsigned int nMin,
							  unsigned int nMax);
		virtual void setPos(unsigned int nPos);
		virtual bool response(Response* pResponse);
	
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
		virtual ~FlagProcessHook();
	
	public:
		virtual bool processFetchResponse(ResponseFetch* pFetch);
	
	private:
		qm::NormalFolder* pFolder_;
	};
	
	class Hook
	{
	public:
		Hook(CallbackImpl* pCallback,
			 ProcessHook* pProcessHook);
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
	std::auto_ptr<SessionCache> pSessionCache_;
	std::auto_ptr<CallbackImpl> pCallback_;
	std::auto_ptr<OfflineJobManager> pOfflineJobManager_;
	bool bOffline_;
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
	virtual std::auto_ptr<qm::ProtocolDriver> createDriver(qm::Account* pAccount,
														   const qm::Security* pSecurity);

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
	FolderUtil(qm::Account* pAccount);
	~FolderUtil();

public:
	bool isRootFolderSpecified() const;
	const WCHAR* getRootFolder() const;
	void getFolderData(const WCHAR* pwszName,
					   WCHAR cSeparator,
					   unsigned int nAttributes,
					   qs::wstring_ptr* pwstrName,
					   unsigned int* pnFlags) const;

public:
	static void saveSpecialFolders(qm::Account* pAccount);

private:
	FolderUtil(const FolderUtil&);
	FolderUtil& operator=(const FolderUtil&);

private:
	qs::wstring_ptr wstrRootFolder_;
	qs::wstring_ptr wstrSpecialFolders_[4];
};


/****************************************************************************
 *
 * FolderListGetter
 *
 */

class FolderListGetter
{
public:
	FolderListGetter(qm::Account* pAccount,
					 qm::SubAccount* pSubAccount,
					 const qm::Security* pSecurity);
	~FolderListGetter();

public:
	bool update();
	void getFolders(Imap4Driver::RemoteFolderList* pList);

private:
	bool connect();
	bool listNamespaces();
	bool listFolders();
	qm::Folder* getFolder(const WCHAR* pwszName,
						  WCHAR cSeparator,
						  unsigned int nFlags,
						  unsigned int* pnId);

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
	
	struct FolderDataLess : public std::binary_function<FolderData, FolderData, bool>
	{
		bool operator()(const FolderData& lhs,
						const FolderData& rhs) const;
	};

private:
	typedef std::vector<std::pair<qs::WSTRING, WCHAR> > NamespaceList;
	typedef std::vector<FolderData> FolderDataList;
	typedef Imap4Driver::RemoteFolderList FolderList;

private:
	class CallbackImpl : public AbstractCallback
	{
	public:
		CallbackImpl(FolderListGetter* pGetter,
					 const qm::Security* pSecurity);
		virtual ~CallbackImpl();
	
	public:
		void setNamespaceList(NamespaceList* pListNamespace);
		void setFolderDataList(FolderDataList* pListFolderData);
	
	public:
		virtual bool isCanceled(bool bForce) const;
		virtual void initialize();
		virtual void lookup();
		virtual void connecting();
		virtual void connected();
	
	public:
		virtual void authenticating();
		virtual void setRange(unsigned int nMin,
							  unsigned int nMax);
		virtual void setPos(unsigned int nPos);
		virtual bool response(Response* pResponse);
	
	private:
		bool processNamespace(ResponseNamespace* pNamespace);
		bool processList(ResponseList* pList);
	
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
	std::auto_ptr<FolderUtil> pFolderUtil_;
	std::auto_ptr<Imap4> pImap4_;
	std::auto_ptr<CallbackImpl> pCallback_;
	std::auto_ptr<qs::Logger> pLogger_;
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
	unsigned int nLastUsedTime_;
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
	SessionCache(qm::Account* pAccount,
				 qm::SubAccount* pSubAccount,
				 AbstractCallback* pCallback,
				 size_t nMaxSession);
	~SessionCache();

public:
	qm::SubAccount* getSubAccount() const;
	bool getSession(qm::NormalFolder* pFolder,
					Session* pSession);
	void releaseSession(Session session);

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
	unsigned int nForceDisconnect_;
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
	SessionCacher(SessionCache* pCache,
				  qm::NormalFolder* pFolder);
	~SessionCacher();

public:
	Imap4* get() const;
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
