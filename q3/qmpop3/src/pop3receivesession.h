/*
 * $Id$
 *
 * Copyright(C) 1998-2003 Satoshi Nakamura
 * All rights reserved.
 *
 */

#ifndef __POP3RECEIVESESSION_H__
#define __POP3RECEIVESESSION_H__

#include <qmaccount.h>
#include <qmdocument.h>
#include <qmfolder.h>
#include <qmmacro.h>
#include <qmmessage.h>
#include <qmmessageholder.h>
#include <qmsecurity.h>
#include <qmsession.h>
#include <qmsyncfilter.h>

#include <qs.h>
#include <qslog.h>

#include <vector>

#include "pop3.h"


namespace qmpop3 {

class Pop3ReceiveSession;
class Pop3ReceiveSessionFactory;
class Pop3SyncFilterCallback;
class Pop3MessageHolder;

class UIDList;


/****************************************************************************
 *
 * Pop3ReceiveSession
 *
 */

class Pop3ReceiveSession : public qm::ReceiveSession
{
public:
	enum State {
		STATE_NONE,
		STATE_HEADER,
		STATE_ALL
	};

public:
	Pop3ReceiveSession(qs::QSTATUS* pstatus);
	virtual ~Pop3ReceiveSession();

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
	qs::QSTATUS prepare();
	qs::QSTATUS downloadReservedMessages();
	qs::QSTATUS downloadReservedMessages(qm::NormalFolder* pFolder);
	qs::QSTATUS loadUIDList(std::auto_ptr<UIDList>* papUIDList) const;
	qs::QSTATUS saveUIDList(const UIDList* pUIDList) const;

private:
	Pop3ReceiveSession(const Pop3ReceiveSession&);
	Pop3ReceiveSession& operator=(const Pop3ReceiveSession&);

private:
	class CallbackImpl :
		public qs::SocketCallback,
		public qm::DefaultSSLSocketCallback,
		public Pop3Callback
	{
	public:
		CallbackImpl(qm::SubAccount* pSubAccount, const qm::Security* pSecurity,
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
		virtual qs::QSTATUS getUserInfo(qs::WSTRING* pwstrUserName,
			qs::WSTRING* pwstrPassword);
		virtual qs::QSTATUS setPassword(const WCHAR* pwszPassword);
		virtual qs::QSTATUS authenticating();
		virtual qs::QSTATUS setRange(unsigned int nMin, unsigned int nMax);
		virtual qs::QSTATUS setPos(unsigned int nPos);
	
	private:
		CallbackImpl(const CallbackImpl&);
		CallbackImpl& operator=(const CallbackImpl&);
	
	private:
		qm::SubAccount* pSubAccount_;
		qm::ReceiveSessionCallback* pSessionCallback_;
	};

private:
	Pop3* pPop3_;
	CallbackImpl* pCallback_;
	qm::Document* pDocument_;
	qm::Account* pAccount_;
	qm::SubAccount* pSubAccount_;
	qm::NormalFolder* pFolder_;
	HWND hwnd_;
	qs::Profile* pProfile_;
	qs::Logger* pLogger_;
	qm::ReceiveSessionCallback* pSessionCallback_;
	bool bReservedDownload_;
	bool bCacheAll_;
	unsigned int nStart_;
	UIDList* pUIDList_;
	Pop3::UidList listUID_;
	Pop3::MessageSizeList listSize_;
};


/****************************************************************************
 *
 * Pop3ReceiveSessionUI
 *
 */

class Pop3ReceiveSessionUI : public qm::ReceiveSessionUI
{
public:
	Pop3ReceiveSessionUI(qs::QSTATUS* pstatus);
	virtual ~Pop3ReceiveSessionUI();

public:
	virtual const WCHAR* getClass();
	virtual qs::QSTATUS getDisplayName(qs::WSTRING* pwstrName);
	virtual short getDefaultPort();
	virtual qs::QSTATUS createPropertyPage(
		qm::SubAccount* pSubAccount, qs::PropertyPage** ppPage);

private:
	Pop3ReceiveSessionUI(const Pop3ReceiveSessionUI&);
	Pop3ReceiveSessionUI& operator=(const Pop3ReceiveSessionUI&);
};


/****************************************************************************
 *
 * Pop3ReceiveSessionFactory
 *
 */

class Pop3ReceiveSessionFactory : public qm::ReceiveSessionFactory
{
private:
	Pop3ReceiveSessionFactory();

public:
	virtual ~Pop3ReceiveSessionFactory();

protected:
	virtual qs::QSTATUS createSession(qm::ReceiveSession** ppReceiveSession);
	virtual qs::QSTATUS createUI(qm::ReceiveSessionUI** ppUI);

private:
	Pop3ReceiveSessionFactory(const Pop3ReceiveSessionFactory&);
	Pop3ReceiveSessionFactory& operator=(const Pop3ReceiveSessionFactory&);

private:
	static Pop3ReceiveSessionFactory factory__;
};


/****************************************************************************
 *
 * Pop3SyncFilterCallback
 *
 */

class Pop3SyncFilterCallback : public qm::SyncFilterCallback
{
public:
	Pop3SyncFilterCallback(qm::Document* pDocument, qm::Account* pAccount,
		qm::NormalFolder* pFolder, qm::Message* pMessage,
		unsigned int nSize, HWND hwnd, qs::Profile* pProfile,
		qm::MacroVariableHolder* pGlobalVariable, Pop3* pPop3,
		unsigned int nMessage, qs::string_ptr<qs::STRING>* pstrMessage,
		Pop3ReceiveSession::State* pState, unsigned int* pnGetSize);
	virtual ~Pop3SyncFilterCallback();

public:
	qs::QSTATUS getMessage(unsigned int nFlag);

public:
	virtual const qm::NormalFolder* getFolder();
	virtual qs::QSTATUS getMacroContext(qm::MacroContext** ppContext);

private:
	Pop3SyncFilterCallback(const Pop3SyncFilterCallback&);
	Pop3SyncFilterCallback& operator=(const Pop3SyncFilterCallback&);

private:
	qm::Document* pDocument_;
	qm::Account* pAccount_;
	qm::NormalFolder* pFolder_;
	qm::Message* pMessage_;
	unsigned int nSize_;
	HWND hwnd_;
	qs::Profile* pProfile_;
	qm::MacroVariableHolder* pGlobalVariable_;
	Pop3* pPop3_;
	unsigned int nMessage_;
	qs::string_ptr<qs::STRING>* pstrMessage_;
	Pop3ReceiveSession::State* pState_;
	unsigned int* pnGetSize_;
	Pop3MessageHolder* pmh_;
};


/****************************************************************************
 *
 * Pop3MessageHolder
 *
 */

class Pop3MessageHolder : public qm::AbstractMessageHolder
{
public:
	Pop3MessageHolder(Pop3SyncFilterCallback* pCallback,
		qm::NormalFolder* pFolder, qm::Message* pMessage,
		unsigned int nSize, qs::QSTATUS* pstatus);
	virtual ~Pop3MessageHolder();

public:
	virtual qs::QSTATUS getFrom(qs::WSTRING* pwstrFrom) const;
	virtual qs::QSTATUS getTo(qs::WSTRING* pwstrTo) const;
	virtual qs::QSTATUS getFromTo(qs::WSTRING* pwstrFromTo) const;
	virtual qs::QSTATUS getSubject(qs::WSTRING* pwstrSubject) const;
	virtual qs::QSTATUS getDate(qs::Time* pTime) const;
	virtual qs::QSTATUS getMessage(unsigned int nFlags,
		const WCHAR* pwszField, qm::Message* pMessage);

private:
	qs::QSTATUS getMessage(unsigned int nFlags) const;

private:
	Pop3MessageHolder(const Pop3MessageHolder&);
	Pop3MessageHolder& operator=(const Pop3MessageHolder&);

private:
	Pop3SyncFilterCallback* pCallback_;
};

}

#endif // __POP3RECEIVESESSION_H__
