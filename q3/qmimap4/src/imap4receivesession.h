/*
 * $Id$
 *
 * Copyright(C) 1998-2004 Satoshi Nakamura
 * All rights reserved.
 *
 */

#ifndef __IMAP4RECEIVESESSION_H__
#define __IMAP4RECEIVESESSION_H__

#include <qmaccount.h>
#include <qmdocument.h>
#include <qmfolder.h>
#include <qmmacro.h>
#include <qmmessage.h>
#include <qmmessageholder.h>
#include <qmsession.h>
#include <qmsyncfilter.h>

#include <qs.h>
#include <qslog.h>

#include "imap4.h"
#include "util.h"

namespace qmimap4 {

class Imap4ReceiveSession;
class Imap4ReceiveSessionFactory;
class Imap4SyncFilterCallback;
class Imap4MessageHolder;

class ProcessHook;


/****************************************************************************
 *
 * Imap4ReceiveSession
 *
 */

class Imap4ReceiveSession : public qm::ReceiveSession
{
public:
	Imap4ReceiveSession();
	virtual ~Imap4ReceiveSession();

public:
	virtual bool init(qm::Document* pDocument,
					  qm::Account* pAccount,
					  qm::SubAccount* pSubAccount,
					  HWND hwnd,
					  qs::Profile* pProfile,
					  qs::Logger* pLogger,
					  qm::ReceiveSessionCallback* pCallback);
	virtual bool connect();
	virtual void disconnect();
	virtual bool selectFolder(qm::NormalFolder* pFolder);
	virtual bool closeFolder();
	virtual bool updateMessages();
	virtual bool downloadMessages(const qm::SyncFilterSet* pSyncFilterSet);
	virtual bool applyOfflineJobs();

private:
	bool downloadReservedMessages();
	bool downloadReservedMessages(qm::NormalFolder* pFolder);

private:
	bool processCapabilityResponse(ResponseCapability* pCapability);
	bool processContinueResponse(ResponseContinue* pContinue);
	bool processExistsResponse(ResponseExists* pExists);
	bool processExpungeResponse(ResponseExpunge* pExpunge);
	bool processFetchResponse(ResponseFetch* pFetch);
	bool processFlagsResponse(ResponseFlags* pFlags);
	bool processListResponse(ResponseList* pList);
	bool processNamespaceResponse(ResponseNamespace* pNamespace);
	bool processRecentResponse(ResponseRecent* pRecent);
	bool processSearchResponse(ResponseSearch* pSearch);
	bool processStateResponse(ResponseState* pState);
	bool processStatusResponse(ResponseStatus* pStatus);

private:
	void reportError();

private:
	Imap4ReceiveSession(const Imap4ReceiveSession&);
	Imap4ReceiveSession& operator=(const Imap4ReceiveSession&);

private:
	class CallbackImpl : public AbstractCallback
	{
	public:
		CallbackImpl(Imap4ReceiveSession* pSession,
					 qm::SubAccount* pSubAccount,
					 const qm::Security* pSecurity,
					 qm::ReceiveSessionCallback* pSessionCallback);
		virtual ~CallbackImpl();
	
	public:
		void setMessage(UINT nId);
	
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
		Imap4ReceiveSession* pSession_;
		qm::ReceiveSessionCallback* pSessionCallback_;
	};
	
	class Hook
	{
	public:
		Hook(Imap4ReceiveSession* pSession,
			 ProcessHook* pHook);
		~Hook();
	
	public:
		void unhook();
	
	private:
		Hook(const Hook&);
		Hook& operator=(const Hook&);
	
	private:
		Imap4ReceiveSession* pSession_;
	};

private:
	std::auto_ptr<Imap4> pImap4_;
	std::auto_ptr<CallbackImpl> pCallback_;
	qm::Document* pDocument_;
	qm::Account* pAccount_;
	qm::SubAccount* pSubAccount_;
	qm::NormalFolder* pFolder_;
	HWND hwnd_;
	qs::Profile* pProfile_;
	qs::Logger* pLogger_;
	qm::ReceiveSessionCallback* pSessionCallback_;
	ProcessHook* pProcessHook_;
	unsigned long nExists_;
	unsigned long nUidValidity_;
	bool bReadOnly_;
	unsigned long nUidStart_;
	unsigned long nIdStart_;

friend class CallbackImpl;
friend class Hook;
};


/****************************************************************************
 *
 * Imap4ReceiveSessionUI
 *
 */

class Imap4ReceiveSessionUI : public qm::ReceiveSessionUI
{
public:
	Imap4ReceiveSessionUI();
	virtual ~Imap4ReceiveSessionUI();

public:
	virtual const WCHAR* getClass();
	virtual qs::wstring_ptr getDisplayName();
	virtual short getDefaultPort();
	virtual std::auto_ptr<qs::PropertyPage> createPropertyPage(qm::SubAccount* pSubAccount);

private:
	Imap4ReceiveSessionUI(const Imap4ReceiveSessionUI&);
	Imap4ReceiveSessionUI& operator=(const Imap4ReceiveSessionUI&);
};


/****************************************************************************
 *
 * Imap4ReceiveSessionFactory
 *
 */

class Imap4ReceiveSessionFactory : public qm::ReceiveSessionFactory
{
private:
	Imap4ReceiveSessionFactory();

public:
	virtual ~Imap4ReceiveSessionFactory();

protected:
	virtual std::auto_ptr<qm::ReceiveSession> createSession();
	virtual std::auto_ptr<qm::ReceiveSessionUI> createUI();

private:
	Imap4ReceiveSessionFactory(const Imap4ReceiveSessionFactory&);
	Imap4ReceiveSessionFactory& operator=(const Imap4ReceiveSessionFactory&);

private:
	static Imap4ReceiveSessionFactory factory__;
};


/****************************************************************************
 *
 * Imap4SyncFilterCallback
 *
 */

class Imap4SyncFilterCallback : public qm::SyncFilterCallback
{
public:
	Imap4SyncFilterCallback(qm::Document* pDocument,
							qm::Account* pAccount,
							qm::NormalFolder* pFolder,
							qm::Message* pMessage,
							unsigned int nUid,
							unsigned int nSize,
							unsigned int nTextSize,
							HWND hwnd,
							qs::Profile* pProfile,
							qm::MacroVariableHolder* pGlobalVariable,
							Imap4ReceiveSession* pSession);
	virtual ~Imap4SyncFilterCallback();

public:
	bool getMessage(unsigned int nFlags);

public:
	virtual const qm::NormalFolder* getFolder();
	virtual std::auto_ptr<qm::MacroContext> getMacroContext();

private:
	Imap4SyncFilterCallback(const Imap4SyncFilterCallback&);
	Imap4SyncFilterCallback& operator=(const Imap4SyncFilterCallback&);

private:
	qm::Document* pDocument_;
	qm::Account* pAccount_;
	qm::NormalFolder* pFolder_;
	qm::Message* pMessage_;
	unsigned int nUid_;
	unsigned int nSize_;
	unsigned int nTextSize_;
	HWND hwnd_;
	qs::Profile* pProfile_;
	qm::MacroVariableHolder* pGlobalVariable_;
	Imap4ReceiveSession* pSession_;
	std::auto_ptr<Imap4MessageHolder> pmh_;
};


/****************************************************************************
 *
 * Imap4MessageHolder
 *
 */

class Imap4MessageHolder : public qm::AbstractMessageHolder
{
public:
	Imap4MessageHolder(Imap4SyncFilterCallback* pCallback,
					   qm::NormalFolder* pFolder,
					   qm::Message* pMessage,
					   unsigned int nId,
					   unsigned int nSize,
					   unsigned int nTextSize);
	virtual ~Imap4MessageHolder();

public:
	virtual bool getMessage(unsigned int nFlags,
							const WCHAR* pwszField,
							qm::Message* pMessage);

private:
	Imap4MessageHolder(const Imap4MessageHolder&);
	Imap4MessageHolder& operator=(const Imap4MessageHolder&);

private:
	Imap4SyncFilterCallback* pCallback_;
};

}

#endif // __IMAP4RECEIVESESSION_H__
