/*
 * $Id$
 *
 * Copyright(C) 1998-2005 Satoshi Nakamura
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
				qm::PasswordCallback* pPasswordCallback,
				const qm::Security* pSecurity);
	virtual ~Imap4Driver();

public:
	virtual bool init();
	virtual bool save(bool bForce);
	virtual bool isSupport(qm::Account::Support support);
	virtual void setOffline(bool bOffline);
	virtual void setSubAccount(qm::SubAccount* pSubAccount);
	
	virtual std::auto_ptr<qm::NormalFolder> createFolder(const WCHAR* pwszName,
														 qm::Folder* pParent);
	virtual bool removeFolder(qm::NormalFolder* pFolder);
	virtual bool renameFolder(qm::NormalFolder* pFolder,
							  const WCHAR* pwszName);
	virtual bool moveFolder(qm::NormalFolder* pFolder,
							qm::NormalFolder* pParent,
							const WCHAR* pwszName);
	virtual bool createDefaultFolders(qm::Account::FolderList* pList);
	virtual bool getRemoteFolders(RemoteFolderList* pList);
	virtual std::pair<const WCHAR**, size_t> getFolderParamNames();
	virtual void setDefaultFolderParams(qm::NormalFolder* pFolder);
	
	virtual bool getMessage(qm::MessageHolder* pmh,
							unsigned int nFlags,
							GetMessageCallback* pCallback);
	virtual bool setMessagesFlags(qm::NormalFolder* pFolder,
								  const qm::MessageHolderList& l,
								  unsigned int nFlags,
								  unsigned int nMask);
	virtual bool setMessagesLabel(qm::NormalFolder* pFolder,
								  const qm::MessageHolderList& l,
								  const WCHAR* pwszLabel);
	virtual bool appendMessage(qm::NormalFolder* pFolder,
							   const CHAR* pszMessage,
							   size_t nLen,
							   unsigned int nFlags,
							   const WCHAR* pwszLabel);
	virtual bool removeMessages(qm::NormalFolder* pFolder,
								const qm::MessageHolderList& l);
	virtual bool copyMessages(const qm::MessageHolderList& l,
							  qm::NormalFolder* pFolderFrom,
							  qm::NormalFolder* pFolderTo,
							  bool bMove);

public:
	OfflineJobManager* getOfflineJobManager() const;
	bool search(qm::NormalFolder* pFolder,
				const WCHAR* pwszCondition,
				const WCHAR* pwszCharset,
				bool bUseCharset,
				qm::MessageHolderList* pList);

private:
	bool prepareSessionCache(bool bClear);
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
					 qm::PasswordCallback* pPasswordCallback,
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
		virtual void setRange(size_t nMin,
							  size_t nMax);
		virtual void setPos(size_t nPos);
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
	qm::PasswordCallback* pPasswordCallback_;
	const qm::Security* pSecurity_;
	std::auto_ptr<CallbackImpl> pCallback_;
	std::auto_ptr<SessionCache> pSessionCache_;
	std::auto_ptr<OfflineJobManager> pOfflineJobManager_;
	qm::SubAccount* pSubAccount_;
	bool bOffline_;
	qs::CriticalSection cs_;

private:
	static const unsigned int nSupport__;
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
														   qm::PasswordCallback* pPasswordCallback,
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
	WCHAR getRootFolderSeparator() const;
	void setRootFolderSeparator(WCHAR c);
	void getFolderData(const WCHAR* pwszName,
					   WCHAR cSeparator,
					   unsigned int nAttributes,
					   qs::wstring_ptr* pwstrName,
					   unsigned int* pnFlags) const;
	void save() const;

public:
	static void saveSpecialFolders(qm::Account* pAccount);

private:
	FolderUtil(const FolderUtil&);
	FolderUtil& operator=(const FolderUtil&);

private:
	qm::Account* pAccount_;
	qs::wstring_ptr wstrRootFolder_;
	WCHAR cRootFolderSeparator_;
	qs::wstring_ptr wstrSpecialFolders_[5];
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
					 qm::PasswordCallback* pPasswordCallback,
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
	
	struct FolderInfo
	{
		qm::Folder* pFolder_;
		bool bNew_;
		qs::WSTRING wstrFullName_;
	};
	
	struct FolderInfoLess : public std::binary_function<FolderInfo, FolderInfo, bool>
	{
		bool operator()(const FolderInfo& lhs,
						const FolderInfo& rhs) const;
	};

private:
	typedef std::vector<std::pair<qs::WSTRING, WCHAR> > NamespaceList;
	typedef std::vector<FolderData> FolderDataList;
	typedef std::vector<FolderInfo> FolderInfoList;

private:
	class CallbackImpl : public AbstractCallback
	{
	public:
		CallbackImpl(FolderListGetter* pGetter,
					 qm::PasswordCallback* pPasswordCallback,
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
		virtual void setRange(size_t nMin,
							  size_t nMax);
		virtual void setPos(size_t nPos);
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
	qm::PasswordCallback* pPasswordCallback_;
	const qm::Security* pSecurity_;
	std::auto_ptr<FolderUtil> pFolderUtil_;
	std::auto_ptr<Imap4> pImap4_;
	std::auto_ptr<CallbackImpl> pCallback_;
	std::auto_ptr<qs::Logger> pLogger_;
	NamespaceList listNamespace_;
	FolderDataList listFolderData_;
	FolderInfoList listFolderInfo_;
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
					Session* pSession,
					bool* pbNew);
	void releaseSession(Session session);

private:
	bool getSessionWithoutSelect(qm::NormalFolder* pFolder,
								 std::auto_ptr<qs::Logger>* ppLogger,
								 std::auto_ptr<Imap4>* ppImap4,
								 unsigned int* pnLastSelectedTime,
								 bool* pbNew);
	bool isNeedSelect(qm::NormalFolder* pFolder,
					  unsigned int nLastSelectedTime);
	bool isForceDisconnect(unsigned int nLastUsedTime) const;

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
	bool isNew() const;
	void release();
	bool retry();

private:
	void init();
	bool create();
	void destroy();

private:
	SessionCacher(const SessionCacher&);
	SessionCacher& operator=(const SessionCacher&);

private:
	SessionCache* pCache_;
	qm::NormalFolder* pFolder_;
	Session session_;
	bool bNew_;
};

}

#endif // __IMAP4DRIVER_H__
