/*
 * $Id$
 *
 * Copyright(C) 1998-2004 Satoshi Nakamura
 * All rights reserved.
 *
 */

#ifndef __SYNCMANAGER_H__
#define __SYNCMANAGER_H__

#include <qm.h>
#include <qmsession.h>

#include <qs.h>
#include <qsprofile.h>
#include <qsras.h>
#include <qsregex.h>
#include <qsthread.h>

#include <vector>


namespace qm {

class SyncItem;
class SyncData;
class SyncManager;
class SyncManagerCallback;
class SyncManagerHandler;
class SyncManagerEvent;

class Account;
class Folder;
class SubAccount;
class NormalFolder;
class SyncFilterManager;
class SyncFilterSet;


/****************************************************************************
 *
 * SyncItem
 *
 */

class SyncItem
{
public:
	enum ConnectReceiveBeforeSend {
		CRBS_NONE,
		CRBS_TRUE,
		CRBS_FALSE
	};

public:
	SyncItem(Account* pAccount,
			 SubAccount* pSubAccount,
			 NormalFolder* pFolder,
			 const SyncFilterSet* pFilterSet,
			 unsigned int nSlot);
	SyncItem(Account* pAccount,
			 SubAccount* pSubAccount,
			 ConnectReceiveBeforeSend crbs,
			 unsigned int nSlot);
	~SyncItem();

public:
	Account* getAccount() const;
	SubAccount* getSubAccount() const;
	NormalFolder* getFolder() const;
	const SyncFilterSet* getFilterSet() const;
	bool isSend() const;
	bool isConnectReceiveBeforeSend() const;
	unsigned int getSlot() const;

private:
	Account* pAccount_;
	SubAccount* pSubAccount_;
	NormalFolder* pFolder_;
	const SyncFilterSet* pFilterSet_;
	bool bSend_;
	bool bConnectReceiveBeforeSend_;
	unsigned int nSlot_;
};


/****************************************************************************
 *
 * SyncDialup
 *
 */

class SyncDialup
{
public:
	enum {
		FLAG_SHOWDIALOG				= 0x01,
		FLAG_WHENEVERNOTCONNECTED	= 0x02,
		FLAG_NOTDISCONNECT			= 0x04
	};

public:
	SyncDialup(const WCHAR* pwszEntry,
			   unsigned int nFlags,
			   const WCHAR* pwszDialFrom,
			   unsigned int nDisconnectWait);
	~SyncDialup();

public:
	const WCHAR* getEntry() const;
	unsigned int getFlags() const;
	const WCHAR* getDialFrom() const;
	unsigned int getDisconnectWait() const;

private:
	SyncDialup(const SyncDialup&);
	SyncDialup& operator=(const SyncDialup&);

private:
	qs::wstring_ptr wstrEntry_;
	unsigned int nFlags_;
	qs::wstring_ptr wstrDialFrom_;
	unsigned int nDisconnectWait_;
};


/****************************************************************************
 *
 * SyncData
 *
 */

class SyncData
{
public:
	typedef std::vector<SyncItem> ItemList;

public:
	SyncData(SyncManager* pManager,
			 Document* pDocument,
			 HWND hwnd,
			 unsigned int nCallbackParam);
	~SyncData();

public:
	Document* getDocument() const;
	HWND getWindow() const;
	bool isEmpty() const;
	unsigned int getCallbackParam() const;
	const SyncDialup* getDialup() const;
	const ItemList& getItems() const;
	unsigned int getSlotCount() const;
	SyncManagerCallback* getCallback() const;
	void setCallback(SyncManagerCallback* pCallback);
	void setDialup(std::auto_ptr<SyncDialup> pDialup);
	void newSlot();
	void addFolder(Account* pAccount,
				   SubAccount* pSubAccount,
				   NormalFolder* pFolder,
				   const WCHAR* pwszFilterName);
	void addFolders(Account* pAccount,
					SubAccount* pSubAccount,
					const qs::RegexPattern* pFolderNamePattern,
					const WCHAR* pwszFilterName);
	void addSend(Account* pAccount,
				 SubAccount* pSubAccount,
				 SyncItem::ConnectReceiveBeforeSend crbs);

private:
	SyncData(const SyncData&);
	SyncData& operator=(const SyncData&);

private:
	SyncManager* pManager_;
	Document* pDocument_;
	HWND hwnd_;
	unsigned int nCallbackParam_;
	SyncManagerCallback* pCallback_;
	std::auto_ptr<SyncDialup> pDialup_;
	ItemList listItem_;
	unsigned int nSlot_;
};


/****************************************************************************
 *
 * SyncManager
 *
 */

class SyncManager
{
public:
	explicit SyncManager(qs::Profile* pProfile);
	~SyncManager();

public:
	void dispose();
	bool sync(std::auto_ptr<SyncData> pData);
	bool isSyncing() const;
	SyncFilterManager* getSyncFilterManager() const;
	void addSyncManagerHandler(SyncManagerHandler* pHandler);
	void removeSyncManagerHandler(SyncManagerHandler* pHandler);

public:
	void fireStatusChanged() const;

private:
	SyncManager(const SyncManager&);
	SyncManager& operator=(const SyncManager&);

private:
	bool syncData(const SyncData* pData);
	bool syncSlotData(const SyncData* pData,
					  unsigned int nSlot);
	bool syncFolder(SyncManagerCallback* pSyncManagerCallback,
					const SyncItem& item,
					ReceiveSession* pSession);
	bool send(Document* pDocument,
			  SyncManagerCallback* pSyncManagerCallback,
			  const SyncItem& item);
	bool openReceiveSession(Document* pDocument,
							HWND hwnd,
							SyncManagerCallback* pSyncManagerCallback,
							const SyncItem& item,
							std::auto_ptr<ReceiveSession>* ppSession,
							std::auto_ptr<ReceiveSessionCallback>* ppCallback,
							std::auto_ptr<qs::Logger>* ppLogger);

private:
	class SyncThread : public qs::Thread
	{
	public:
		SyncThread(SyncManager* pSyncManager,
				   std::auto_ptr<SyncData> pData);
		virtual ~SyncThread();
	
	public:
		void setWaitMode();
	
	public:
		virtual void run();
	
	private:
		SyncThread(const SyncThread&);
		SyncThread& operator=(const SyncThread&);
	
	private:
		SyncManager* pSyncManager_;
		std::auto_ptr<SyncData> pSyncData_;
		bool bWaitMode_;
	};
	friend class SyncThread;
	
	class ParallelSyncThread : public qs::Thread
	{
	public:
		ParallelSyncThread(SyncManager* pSyncManager,
						   const SyncData* pData,
						   unsigned int nSlot);
		virtual ~ParallelSyncThread();
	
	public:
		virtual void run();
	
	private:
		ParallelSyncThread(const ParallelSyncThread&);
		ParallelSyncThread& operator=(const ParallelSyncThread&);
	
	private:
		SyncManager* pSyncManager_;
		const SyncData* pSyncData_;
		unsigned int nSlot_;
	};
	friend class ParallelSyncThread;
	
	class ReceiveSessionCallbackImpl : public ReceiveSessionCallback
	{
	public:
		explicit ReceiveSessionCallbackImpl(SyncManagerCallback* pCallback);
		virtual ~ReceiveSessionCallbackImpl();
	
	public:
		virtual bool isCanceled(bool bForce);
		virtual void setPos(unsigned int n);
		virtual void setRange(unsigned int nMin,
							  unsigned int nMax);
		virtual void setSubPos(unsigned int n);
		virtual void setSubRange(unsigned int nMin,
								 unsigned int nMax);
		virtual void setMessage(const WCHAR* pwszMessage);
		virtual void addError(const SessionErrorInfo& info);
	
	public:
		virtual void notifyNewMessage();
	
	private:
		ReceiveSessionCallbackImpl(const ReceiveSessionCallbackImpl&);
		ReceiveSessionCallbackImpl& operator=(const ReceiveSessionCallbackImpl&);
	
	private:
		SyncManagerCallback* pCallback_;
		unsigned int nId_;
	};
	
	class SendSessionCallbackImpl : public SendSessionCallback
	{
	public:
		explicit SendSessionCallbackImpl(SyncManagerCallback* pCallback);
		virtual ~SendSessionCallbackImpl();
	
	public:
		virtual bool isCanceled(bool bForce);
		virtual void setPos(unsigned int n);
		virtual void setRange(unsigned int nMin,
							  unsigned int nMax);
		virtual void setSubPos(unsigned int n);
		virtual void setSubRange(unsigned int nMin,
								 unsigned int nMax);
		virtual void setMessage(const WCHAR* pwszMessage);
		virtual void addError(const SessionErrorInfo& info);
	
	private:
		SendSessionCallbackImpl(const SendSessionCallbackImpl&);
		SendSessionCallbackImpl& operator=(const SendSessionCallbackImpl&);
	
	private:
		SyncManagerCallback* pCallback_;
		unsigned int nId_;
	};
	
	class RasConnectionCallbackImpl : public qs::RasConnectionCallback
	{
	public:
		RasConnectionCallbackImpl(const SyncDialup* pDialup,
								  SyncManagerCallback* pCallback);
		virtual ~RasConnectionCallbackImpl();
	
	public:
		virtual bool isCanceled();
		virtual bool preConnect(RASDIALPARAMS* prdp);
		virtual void setMessage(const WCHAR* pwszMessage);
		virtual void error(const WCHAR* pwszMessage);
	
	private:
		RasConnectionCallbackImpl(const RasConnectionCallbackImpl&);
		RasConnectionCallbackImpl& operator=(const RasConnectionCallbackImpl&);
	
	private:
		const SyncDialup* pDialup_;
		SyncManagerCallback* pCallback_;
	};
	
	class FolderWait
	{
	public:
		FolderWait(SyncManager* pSyncManager,
				   NormalFolder* pFolder);
		~FolderWait();
	
	private:
		FolderWait(const FolderWait&);
		FolderWait& operator=(const FolderWait&);
	
	private:
		SyncManager* pSyncManager_;
		NormalFolder* pFolder_;
		qs::Event* pEvent_;
	};
	friend class FolderWait;

private:
	typedef std::vector<SyncThread*> ThreadList;
	typedef std::vector<std::pair<NormalFolder*, qs::Event*> > SyncingFolderList;
	typedef std::vector<SyncManagerHandler*> SyncManagerHandlerList;

private:
	qs::Profile* pProfile_;
	qs::Synchronizer* pSynchronizer_;
	std::auto_ptr<SyncFilterManager> pSyncFilterManager_;
	ThreadList listThread_;
	SyncingFolderList listSyncingFolder_;
	qs::CriticalSection cs_;
	SyncManagerHandlerList listHandler_;
};


/****************************************************************************
 *
 * SyncManagerCallback
 *
 */

class SyncManagerCallback
{
public:
	virtual ~SyncManagerCallback();

public:
	virtual void start(unsigned int nParam) = 0;
	virtual void end() = 0;
	virtual void startThread(unsigned int nId,
							 unsigned int nParam) = 0;
	virtual void endThread(unsigned int nId) = 0;
	virtual void setPos(unsigned int nId,
						bool bSub,
						unsigned int nPos) = 0;
	virtual void setRange(unsigned int nId,
						  bool bSub,
						  unsigned int nMin,
						  unsigned int nMax) = 0;
	virtual void setAccount(unsigned int nId,
							Account* pAccount,
							SubAccount* pSubAccount) = 0;
	virtual void setFolder(unsigned int nId,
						   Folder* pFolder) = 0;
	virtual void setMessage(unsigned int nId,
							const WCHAR* pwszMessage) = 0;
	virtual void addError(unsigned int nId,
						  const SessionErrorInfo& info) = 0;
	virtual bool isCanceled(unsigned int nId,
							bool bForce) = 0;
	virtual qs::wstring_ptr selectDialupEntry() = 0;
	virtual bool showDialupDialog(RASDIALPARAMS* prdp) = 0;
	virtual void notifyNewMessage(unsigned int nId) = 0;
};


/****************************************************************************
 *
 * SyncManagerHandler
 *
 */

class SyncManagerHandler
{
public:
	virtual ~SyncManagerHandler();

public:
	virtual void statusChanged(const SyncManagerEvent& event) = 0;
};


/****************************************************************************
 *
 * SyncManagerEvent
 *
 */

class SyncManagerEvent
{
public:
	SyncManagerEvent();
	~SyncManagerEvent();

private:
	SyncManagerEvent(const SyncManagerEvent&);
	SyncManagerEvent& operator=(const SyncManagerEvent&);
};

}

#endif // __SYNCMANAGER_H__
