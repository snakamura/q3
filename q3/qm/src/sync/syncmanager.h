/*
 * $Id: syncmanager.h,v 1.3 2003/05/29 08:15:50 snakamura Exp $
 *
 * Copyright(C) 1998-2003 Satoshi Nakamura
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
	SyncItem(Account* pAccount, SubAccount* pSubAccount,
		NormalFolder* pFolder, const SyncFilterSet* pFilterSet,
		unsigned int nSlot);
	SyncItem(Account* pAccount, SubAccount* pSubAccount, unsigned int nSlot);
	~SyncItem();

public:
	Account* getAccount() const;
	SubAccount* getSubAccount() const;
	NormalFolder* getFolder() const;
	const SyncFilterSet* getFilterSet() const;
	bool isSend() const;
	unsigned int getSlot() const;

private:
	Account* pAccount_;
	SubAccount* pSubAccount_;
	NormalFolder* pFolder_;
	const SyncFilterSet* pFilterSet_;
	bool bSend_;
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
		FLAG_WHENEVERNOTCONNECTED	= 0x02
	};

public:
	SyncDialup(const WCHAR* pwszEntry, unsigned int nFlags,
		const WCHAR* pwszDialFrom, unsigned int nDisconnectWait,
		qs::QSTATUS* pstatus);
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
	qs::WSTRING wstrEntry_;
	unsigned int nFlags_;
	qs::WSTRING wstrDialFrom_;
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
	SyncData(SyncManager* pManager, Document* pDocument,
		HWND hwnd, qs::QSTATUS* pstatus);
	~SyncData();

public:
	Document* getDocument() const;
	HWND getWindow() const;
	bool isEmpty() const;
	const SyncDialup* getDialup() const;
	const ItemList& getItems() const;
	unsigned int getSlotCount() const;
	SyncManagerCallback* getCallback() const;
	void setCallback(SyncManagerCallback* pCallback);
	void setDialup(SyncDialup* pDialup);
	void newSlot();
	qs::QSTATUS addFolder(Account* pAccount, SubAccount* pSubAccount,
		NormalFolder* pFolder, const WCHAR* pwszFilterName);
	qs::QSTATUS addFolders(Account* pAccount, SubAccount* pSubAccount,
		const qs::RegexPattern* pFolderNamePattern, const WCHAR* pwszFilterName);
	qs::QSTATUS addSend(Account* pAccount, SubAccount* pSubAccount);

private:
	SyncData(const SyncData&);
	SyncData& operator=(const SyncData&);

private:
	SyncManager* pManager_;
	Document* pDocument_;
	HWND hwnd_;
	SyncManagerCallback* pCallback_;
	SyncDialup* pDialup_;
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
	enum {
		THREAD_MAX	= 16
	};

public:
	SyncManager(qs::Profile* pProfile, qs::QSTATUS* pstatus);
	~SyncManager();

public:
	qs::QSTATUS sync(SyncData* pData);
	bool isSyncing() const;
	SyncFilterManager* getSyncFilterManager() const;

private:
	SyncManager(const SyncManager&);
	SyncManager& operator=(const SyncManager&);

private:
	qs::QSTATUS syncData(const SyncData* pData);
	qs::QSTATUS syncSlotData(const SyncData* pData, unsigned int nSlot);
	qs::QSTATUS syncFolder(SyncManagerCallback* pSyncManagerCallback,
		const SyncItem& item, ReceiveSession* pSession);
	qs::QSTATUS send(SyncManagerCallback* pSyncManagerCallback,
		const SyncItem& item);
	qs::QSTATUS openReceiveSession(Document* pDocument, HWND hwnd,
		SyncManagerCallback* pSyncManagerCallback, const SyncItem& item,
		ReceiveSession** ppSession, ReceiveSessionCallback** ppCallback,
		qs::Logger** ppLogger);

private:
	class SyncThread : public qs::Thread
	{
	public:
		SyncThread(SyncManager* pSyncManager,
			const SyncData* pData, qs::QSTATUS* pstatus);
		virtual ~SyncThread();
	
	public:
		virtual unsigned int run();
	
	private:
		SyncThread(const SyncThread&);
		SyncThread& operator=(const SyncThread&);
	
	private:
		SyncManager* pSyncManager_;
		const SyncData* pSyncData_;
	};
	
	class ParallelSyncThread : public qs::Thread
	{
	public:
		ParallelSyncThread(SyncManager* pSyncManager, const SyncData* pData,
			unsigned int nSlot, qs::QSTATUS* pstatus);
		virtual ~ParallelSyncThread();
	
	public:
		virtual unsigned int run();
	
	private:
		ParallelSyncThread(const ParallelSyncThread&);
		ParallelSyncThread& operator=(const ParallelSyncThread&);
	
	private:
		SyncManager* pSyncManager_;
		const SyncData* pSyncData_;
		unsigned int nSlot_;
	};
	
	class WaitThread : public qs::Thread
	{
	public:
		WaitThread(SyncManager* pSyncManager, qs::QSTATUS* pstatus);
		virtual ~WaitThread();
	
	public:
		qs::QSTATUS stop();
	
	public:
		virtual unsigned int run();
	
	private:
		WaitThread(const WaitThread&);
		WaitThread& operator=(const WaitThread&);
	
	private:
		SyncManager* pSyncManager_;
		bool bStop_;
	};
	
	class ReceiveSessionCallbackImpl : public ReceiveSessionCallback
	{
	public:
		ReceiveSessionCallbackImpl(SyncManagerCallback* pCallback);
		virtual ~ReceiveSessionCallbackImpl();
	
	public:
		virtual bool isCanceled(bool bForce);
		virtual qs::QSTATUS setPos(unsigned int n);
		virtual qs::QSTATUS setRange(unsigned int nMin, unsigned int nMax);
		virtual qs::QSTATUS setSubPos(unsigned int n);
		virtual qs::QSTATUS setSubRange(unsigned int nMin, unsigned int nMax);
		virtual qs::QSTATUS setMessage(const WCHAR* pwszMessage);
		virtual qs::QSTATUS addError(const SessionErrorInfo& info);
	
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
		SendSessionCallbackImpl(SyncManagerCallback* pCallback);
		virtual ~SendSessionCallbackImpl();
	
	public:
		virtual bool isCanceled(bool bForce);
		virtual qs::QSTATUS setPos(unsigned int n);
		virtual qs::QSTATUS setRange(unsigned int nMin, unsigned int nMax);
		virtual qs::QSTATUS setSubPos(unsigned int n);
		virtual qs::QSTATUS setSubRange(unsigned int nMin, unsigned int nMax);
		virtual qs::QSTATUS setMessage(const WCHAR* pwszMessage);
		virtual qs::QSTATUS addError(const SessionErrorInfo& info);
	
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
		RasConnectionCallbackImpl(SyncManagerCallback* pCallback);
		virtual ~RasConnectionCallbackImpl();
	
	public:
		virtual bool isCanceled();
		virtual qs::QSTATUS preConnect(RASDIALPARAMS* prdp, bool* pbCancel);
		virtual qs::QSTATUS stateChanged(State state);
		virtual qs::QSTATUS error(const WCHAR* pwszMessage);
	
	private:
		RasConnectionCallbackImpl(const RasConnectionCallbackImpl&);
		RasConnectionCallbackImpl& operator=(const RasConnectionCallbackImpl&);
	
	private:
		SyncManagerCallback* pCallback_;
	};
	
	class FolderWait
	{
	public:
		FolderWait(SyncManager* pSyncManager,
			NormalFolder* pFolder, qs::QSTATUS* pstatus);
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

private:
	qs::Profile* pProfile_;
	SyncFilterManager* pSyncFilterManager_;
	WaitThread* pWaitThread_;
	ThreadList listThread_;
	qs::Event* pEvent_;
	SyncingFolderList listSyncingFolder_;
	qs::CriticalSection cs_;

friend class SyncThread;
friend class ParallelSyncThread;
friend class WaitThread;
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
	virtual qs::QSTATUS start() = 0;
	virtual void end() = 0;
	virtual qs::QSTATUS startThread(unsigned int nId) = 0;
	virtual void endThread(unsigned int nId) = 0;
	virtual qs::QSTATUS setPos(unsigned int nId,
		bool bSub, unsigned int nPos) = 0;
	virtual qs::QSTATUS setRange(unsigned int nId, bool bSub,
		unsigned int nMin, unsigned int nMax) = 0;
	virtual qs::QSTATUS setAccount(unsigned int nId,
		Account* pAccount, SubAccount* pSubAccount) = 0;
	virtual qs::QSTATUS setFolder(unsigned int nId, Folder* pFolder) = 0;
	virtual qs::QSTATUS setMessage(
		unsigned int nId, const WCHAR* pwszMessage) = 0;
	virtual qs::QSTATUS addError(unsigned int nId,
		const SessionErrorInfo& info) = 0;
	virtual bool isCanceled(unsigned int nId, bool bForce) = 0;
};

}

#endif // __SYNCMANAGER_H__
