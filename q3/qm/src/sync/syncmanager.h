/*
 * $Id$
 *
 * Copyright(C) 1998-2008 Satoshi Nakamura
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

class SyncDataItem;
	class SyncItem;
		class ReceiveSyncItem;
		class SendSyncItem;
	class ApplyRulesSyncDataItem;
class SyncData;
	class StaticSyncData;
class SyncManager;
class SyncManagerCallback;
class SyncManagerHandler;
class SyncManagerEvent;

class Account;
class Folder;
class SubAccount;
class NormalFolder;
class Recents;
class Term;
class SyncFilterManager;
class SyncFilterSet;


/****************************************************************************
 *
 * SyncDataItem
 *
 */

class SyncDataItem
{
public:
	enum Type {
		TYPE_RECEIVE,
		TYPE_SEND,
		TYPE_APPLYRULES
	};

protected:
	SyncDataItem(Type type,
				 Account* pAccount);

public:
	virtual ~SyncDataItem();

public:
	Type getType() const;
	Account* getAccount() const;

public:
	virtual const SyncItem* getSyncItem() const = 0;

private:
	SyncDataItem(const SyncDataItem&);
	SyncDataItem& operator=(const SyncDataItem&);

private:
	Type type_;
	Account* pAccount_;
};


/****************************************************************************
 *
 * SyncItem
 *
 */

class SyncItem : public SyncDataItem
{
protected:
	SyncItem(Type type,
			 Account* pAccount,
			 SubAccount* pSubAccount);

public:
	virtual ~SyncItem();

public:
	SubAccount* getSubAccount() const;

public:
	virtual NormalFolder* getSyncFolder() const = 0;
	virtual unsigned int getSelectFlags() const = 0;
	virtual const SyncFilterSet* getSyncFilterSet() const = 0;

private:
	SyncItem(const SyncItem&);
	SyncItem& operator=(const SyncItem&);

private:
	SubAccount* pSubAccount_;
};


/****************************************************************************
 *
 * ReceiveSyncItem
 *
 */

class ReceiveSyncItem : public SyncItem
{
public:
	enum Flag {
		FLAG_EMPTY		= 0x01,
		FLAG_EXPUNGE	= 0x02
	};

public:
	ReceiveSyncItem(Account* pAccount,
					SubAccount* pSubAccount,
					NormalFolder* pFolder,
					std::auto_ptr<SyncFilterSet> pFilterSet,
					unsigned int nFlags);
	virtual ~ReceiveSyncItem();

public:
	virtual const SyncItem* getSyncItem() const;

public:
	virtual NormalFolder* getSyncFolder() const;
	virtual unsigned int getSelectFlags() const;
	virtual const SyncFilterSet* getSyncFilterSet() const;

private:
	ReceiveSyncItem(const ReceiveSyncItem&);
	ReceiveSyncItem& operator=(const ReceiveSyncItem&);

private:
	NormalFolder* pFolder_;
	std::auto_ptr<SyncFilterSet> pFilterSet_;
	unsigned int nFlags_;
};


/****************************************************************************
 *
 * SendSyncItem
 *
 */

class SendSyncItem : public SyncItem
{
public:
	SendSyncItem(Account* pAccount,
				 SubAccount* pSubAccount,
				 const WCHAR* pwszMessageId);
	virtual ~SendSyncItem();

public:
	const WCHAR* getMessageId() const;

public:
	virtual const SyncItem* getSyncItem() const;

public:
	virtual NormalFolder* getSyncFolder() const;
	virtual unsigned int getSelectFlags() const;
	virtual const SyncFilterSet* getSyncFilterSet() const;

private:
	SendSyncItem(const SendSyncItem&);
	SendSyncItem& operator=(const SendSyncItem&);

private:
	NormalFolder* pOutbox_;
	qs::wstring_ptr wstrMessageId_;
};


/****************************************************************************
 *
 * ApplyRulesSyncDataItem
 *
 */

class ApplyRulesSyncDataItem : public SyncDataItem
{
public:
	ApplyRulesSyncDataItem(Account* pAccount,
						   Folder* pFolder);
	virtual ~ApplyRulesSyncDataItem();

public:
	Folder* getFolder() const;

public:
	virtual const SyncItem* getSyncItem() const;

private:
	ApplyRulesSyncDataItem(const ApplyRulesSyncDataItem&);
	ApplyRulesSyncDataItem& operator=(const ApplyRulesSyncDataItem&);

private:
	Folder* pFolder_;
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
	enum Type {
		TYPE_MANUAL,
		TYPE_AUTO,
		TYPE_ACTIVE
	};

public:
	typedef std::vector<SyncDataItem*> ItemList;
	typedef std::vector<ItemList> ItemListList;

public:
	SyncData(Document* pDocument,
			 Type type);
	virtual ~SyncData();

public:
	Document* getDocument() const;
	Type getType() const;
	const SyncDialup* getDialup() const;
	SyncManagerCallback* getCallback() const;

public:
	virtual void getItems(ItemListList* pList) = 0;

public:
	void setDialup(std::auto_ptr<SyncDialup> pDialup);
	void setCallback(SyncManagerCallback* pCallback);

private:
	SyncData(const SyncData&);
	SyncData& operator=(const SyncData&);

private:
	Document* pDocument_;
	Type type_;
	SyncManagerCallback* pCallback_;
	std::auto_ptr<SyncDialup> pDialup_;
};


/****************************************************************************
 *
 * StaticSyncData
 *
 */

class StaticSyncData : public SyncData
{
public:
	StaticSyncData(Document* pDocument,
				   Type type,
				   SyncManager* pManager);
	virtual ~StaticSyncData();

public:
	virtual void getItems(ItemListList* pList);

public:
	bool isEmpty() const;
	void newSlot();
	void addReceiveFolder(Account* pAccount,
						  SubAccount* pSubAccount,
						  NormalFolder* pFolder,
						  const WCHAR* pwszFilterName,
						  unsigned int nFlags);
	void addReceiveFolders(Account* pAccount,
						   SubAccount* pSubAccount,
						   const Term& folder,
						   const WCHAR* pwszFilterName);
	void addSend(Account* pAccount,
				 SubAccount* pSubAccount,
				 const WCHAR* pwszMessageId);
	void addApplyRulesFolder(Account* pAccount,
							 Folder* pFolder);
	void addApplyRulesFolders(Account* pAccount,
							  const Term& folder);

private:
	StaticSyncData(const StaticSyncData&);
	StaticSyncData& operator=(const StaticSyncData&);

private:
	typedef std::vector<std::pair<unsigned int, SyncDataItem*> > SlotItemList;

private:
	SyncManager* pManager_;
	SlotItemList listItem_;
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
	enum Notify {
		NOTIFY_ALWAYS,
		NOTIFY_NEVER,
		NOTIFY_AUTO
	};

public:
	SyncManager(SyncFilterManager* pSyncFilterManager,
				qs::Profile* pProfile);
	~SyncManager();

public:
	void dispose();
	bool sync(std::auto_ptr<SyncData> pData);
	bool isSyncing() const;
	SyncFilterManager* getSyncFilterManager() const;
	void addSyncManagerHandler(SyncManagerHandler* pHandler);
	void removeSyncManagerHandler(SyncManagerHandler* pHandler);

public:
	void fireStatusChanged(bool bStart);

public:
	static void addError(SyncManagerCallback* pCallback,
						 unsigned int nId,
						 Account* pAccount,
						 SubAccount* pSubAccount,
						 NormalFolder* pFolder,
						 UINT nMessageId,
						 const WCHAR* pwszDescription);

private:
	bool syncData(SyncData* pData);
	void syncSlotData(const SyncData* pData,
					  const SyncData::ItemList& listItem);
	bool syncFolder(unsigned int nId,
					Document* pDocument,
					SyncManagerCallback* pSyncManagerCallback,
					const SyncItem* pItem,
					ReceiveSession* pSession);
	bool send(unsigned int nId,
			  Document* pDocument,
			  SyncManagerCallback* pSyncManagerCallback,
			  const SendSyncItem* pItem);
	bool applyRules(unsigned int nId,
					Document* pDocument,
					SyncManagerCallback* pSyncManagerCallback,
					const ApplyRulesSyncDataItem* pItem);
	bool openReceiveSession(unsigned int nId,
							Document* pDocument,
							SyncManagerCallback* pSyncManagerCallback,
							const SyncItem* pItem,
							SyncData::Type type,
							std::auto_ptr<ReceiveSession>* ppSession,
							std::auto_ptr<ReceiveSessionCallback>* ppCallback,
							std::auto_ptr<qs::Logger>* ppLogger);

private:
	bool isNotify(SyncData::Type type) const;

private:
	SyncManager(const SyncManager&);
	SyncManager& operator=(const SyncManager&);

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
						   const SyncData::ItemList& listItem);
		virtual ~ParallelSyncThread();
	
	public:
		virtual void run();
	
	private:
		ParallelSyncThread(const ParallelSyncThread&);
		ParallelSyncThread& operator=(const ParallelSyncThread&);
	
	private:
		SyncManager* pSyncManager_;
		const SyncData* pSyncData_;
		const SyncData::ItemList& listItem_;
	};
	friend class ParallelSyncThread;
	
	class ReceiveSessionCallbackImpl : public ReceiveSessionCallback
	{
	public:
		explicit ReceiveSessionCallbackImpl(SyncManagerCallback* pCallback,
											unsigned int nId,
											Document* pDocument,
											qs::Profile* pProfile,
											bool bNotify);
		virtual ~ReceiveSessionCallbackImpl();
	
	public:
		virtual PasswordState getPassword(SubAccount* pSubAccount,
										  Account::Host host,
										  qs::wstring_ptr* pwstrPassword);
		virtual void setPassword(SubAccount* pSubAccount,
								 Account::Host host,
								 const WCHAR* pwszPassword,
								 bool bPermanent);
		
	public:
		virtual void addError(const SessionErrorInfo& info);
	
	public:
		virtual bool isCanceled(bool bForce);
		virtual void setPos(size_t n);
		virtual void setRange(size_t nMin,
							  size_t nMax);
		virtual void setSubPos(size_t n);
		virtual void setSubRange(size_t nMin,
								 size_t nMax);
		virtual void setMessage(const WCHAR* pwszMessage);
	
	public:
		virtual void notifyNewMessage(MessagePtr ptr);
	
	private:
		ReceiveSessionCallbackImpl(const ReceiveSessionCallbackImpl&);
		ReceiveSessionCallbackImpl& operator=(const ReceiveSessionCallbackImpl&);
	
	private:
		SyncManagerCallback* pCallback_;
		unsigned int nId_;
		Document* pDocument_;
		qs::Profile* pProfile_;
		bool bNotify_;
	};
	
	class SendSessionCallbackImpl : public SendSessionCallback
	{
	public:
		SendSessionCallbackImpl(SyncManagerCallback* pCallback,
								unsigned int nId);
		virtual ~SendSessionCallbackImpl();
	
	public:
		virtual PasswordState getPassword(SubAccount* pSubAccount,
										  Account::Host host,
										  qs::wstring_ptr* pwstrPassword);
		virtual void setPassword(SubAccount* pSubAccount,
								 Account::Host host,
								 const WCHAR* pwszPassword,
								 bool bPermanent);
	
	public:
		virtual void addError(const SessionErrorInfo& info);
	
	public:
		virtual bool isCanceled(bool bForce);
		virtual void setPos(size_t n);
		virtual void setRange(size_t nMin,
							  size_t nMax);
		virtual void setSubPos(size_t n);
		virtual void setSubRange(size_t nMin,
								 size_t nMax);
		virtual void setMessage(const WCHAR* pwszMessage);
	
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
	
	class RuleCallbackImpl : public RuleCallback
	{
	public:
		RuleCallbackImpl(SyncManagerCallback* pCallback,
						 unsigned int nId);
		virtual ~RuleCallbackImpl();
	
	public:
		virtual bool isCanceled();
		virtual void checkingMessages(Folder* pFolder);
		virtual void applyingRule(Folder* pFolder);
		virtual void setRange(size_t nMin,
							  size_t nMax);
		virtual void setPos(size_t nPos);
	
	private:
		qs::wstring_ptr getMessage(UINT nId,
								   Folder* pFolder);
	
	private:
		RuleCallbackImpl(const RuleCallbackImpl&);
		RuleCallbackImpl& operator=(const RuleCallbackImpl&);
	
	private:
		SyncManagerCallback* pCallback_;
		unsigned int nId_;
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
	SyncFilterManager* pSyncFilterManager_;
	
	ThreadList listThread_;
	SyncingFolderList listSyncingFolder_;
	qs::CriticalSection cs_;
	
	LONG nDialupConnectionCount_;
	bool bDialup_;
	qs::CriticalSection csDialup_;
	
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
	virtual void start(SyncData::Type type) = 0;
	virtual void end() = 0;
	virtual void startThread(unsigned int nId,
							 SyncData::Type type) = 0;
	virtual void endThread(unsigned int nId) = 0;
	virtual void setPos(unsigned int nId,
						bool bSub,
						size_t nPos) = 0;
	virtual void setRange(unsigned int nId,
						  bool bSub,
						  size_t nMin,
						  size_t nMax) = 0;
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
	virtual PasswordState getPassword(SubAccount* pSubAccount,
									  Account::Host host,
									  qs::wstring_ptr* pwstrPassword) = 0;
	virtual void setPassword(SubAccount* pSubAccount,
							 Account::Host host,
							 const WCHAR* pwszPassword,
							 bool bPermanent) = 0;
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
	enum Status {
		STATUS_START,
		STATUS_END
	};

public:
	SyncManagerEvent(SyncManager* pSyncManager,
					 Status status);
	~SyncManagerEvent();

public:
	SyncManager* getSyncManager() const;
	Status getStatus() const;

private:
	SyncManagerEvent(const SyncManagerEvent&);
	SyncManagerEvent& operator=(const SyncManagerEvent&);

private:
	SyncManager* pSyncManager_;
	Status status_;
};

}

#endif // __SYNCMANAGER_H__
