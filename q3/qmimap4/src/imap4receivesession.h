/*
 * $Id$
 *
 * Copyright(C) 1998-2003 Satoshi Nakamura
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
	Imap4ReceiveSession(qs::QSTATUS* pstatus);
	virtual ~Imap4ReceiveSession();

public:
	virtual qs::QSTATUS init(qm::Document* pDocument, qm::Account* pAccount,
		qm::SubAccount* pSubAccount, HWND hwnd, qs::Profile* pProfile,
		qs::Logger* pLogger, qm::ReceiveSessionCallback* pCallback);
	virtual qs::QSTATUS connect();
	virtual qs::QSTATUS disconnect();
	virtual qs::QSTATUS selectFolder(qm::NormalFolder* pFolder);
	virtual qs::QSTATUS closeFolder();
	virtual qs::QSTATUS updateMessages();
	virtual qs::QSTATUS downloadMessages(
		const qm::SyncFilterSet* pSyncFilterSet);
	virtual qs::QSTATUS applyOfflineJobs();

private:
	qs::QSTATUS downloadReservedMessages();
	qs::QSTATUS downloadReservedMessages(qm::NormalFolder* pFolder);

private:
	qs::QSTATUS processCapabilityResponse(ResponseCapability* pCapability);
	qs::QSTATUS processContinueResponse(ResponseContinue* pContinue);
	qs::QSTATUS processExistsResponse(ResponseExists* pExists);
	qs::QSTATUS processExpungeResponse(ResponseExpunge* pExpunge);
	qs::QSTATUS processFetchResponse(ResponseFetch* pFetch);
	qs::QSTATUS processFlagsResponse(ResponseFlags* pFlags);
	qs::QSTATUS processListResponse(ResponseList* pList);
	qs::QSTATUS processNamespaceResponse(ResponseNamespace* pNamespace);
	qs::QSTATUS processRecentResponse(ResponseRecent* pRecent);
	qs::QSTATUS processSearchResponse(ResponseSearch* pSearch);
	qs::QSTATUS processStateResponse(ResponseState* pState);
	qs::QSTATUS processStatusResponse(ResponseStatus* pStatus);

private:
	qs::QSTATUS reportError();

private:
	Imap4ReceiveSession(const Imap4ReceiveSession&);
	Imap4ReceiveSession& operator=(const Imap4ReceiveSession&);

private:
	class CallbackImpl : public AbstractCallback
	{
	public:
		CallbackImpl(Imap4ReceiveSession* pSession, qm::SubAccount* pSubAccount,
			qm::ReceiveSessionCallback* pSessionCallback, qs::QSTATUS* pstatus);
		virtual ~CallbackImpl();
	
	public:
		qs::QSTATUS setMessage(UINT nId);
	
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
		Imap4ReceiveSession* pSession_;
		qm::ReceiveSessionCallback* pSessionCallback_;
	};
	
	class Hook
	{
	public:
		Hook(Imap4ReceiveSession* pSession, ProcessHook* pHook);
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
	Imap4* pImap4_;
	CallbackImpl* pCallback_;
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
	Imap4ReceiveSessionUI(qs::QSTATUS* pstatus);
	virtual ~Imap4ReceiveSessionUI();

public:
	virtual const WCHAR* getClass();
	virtual qs::QSTATUS getDisplayName(qs::WSTRING* pwstrName);
	virtual short getDefaultPort();
	virtual qs::QSTATUS createPropertyPage(
		qm::SubAccount* pSubAccount, qs::PropertyPage** ppPage);

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
	virtual qs::QSTATUS createSession(qm::ReceiveSession** ppReceiveSession);
	virtual qs::QSTATUS createUI(qm::ReceiveSessionUI** ppUI);

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
	Imap4SyncFilterCallback(qm::Document* pDocument, qm::Account* pAccount,
		qm::NormalFolder* pFolder, qm::Message* pMessage, unsigned int nUid,
		unsigned int nSize, unsigned int nTextSize, HWND hwnd,
		qs::Profile* pProfile, qm::MacroVariableHolder* pGlobalVariable,
		Imap4ReceiveSession* pSession);
	virtual ~Imap4SyncFilterCallback();

public:
	qs::QSTATUS getMessage(unsigned int nFlags);

public:
	virtual const qm::NormalFolder* getFolder();
	virtual qs::QSTATUS getMacroContext(qm::MacroContext** ppContext);

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
	Imap4MessageHolder* pmh_;
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
		qm::NormalFolder* pFolder, qm::Message* pMessage,
		unsigned int nId, unsigned int nSize,
		unsigned int nTextSize, qs::QSTATUS* pstatus);
	virtual ~Imap4MessageHolder();

public:
	virtual qs::QSTATUS getMessage(unsigned int nFlags,
		const WCHAR* pwszField, qm::Message* pMessage);

private:
	Imap4MessageHolder(const Imap4MessageHolder&);
	Imap4MessageHolder& operator=(const Imap4MessageHolder&);

private:
	Imap4SyncFilterCallback* pCallback_;
};

}

#endif // __IMAP4RECEIVESESSION_H__
