/*
 * $Id$
 *
 * Copyright(C) 1998-2003 Satoshi Nakamura
 * All rights reserved.
 *
 */

#ifndef __NNTPRECEIVESESSION_H__
#define __NNTPRECEIVESESSION_H__

#include <qmmacro.h>
#include <qmsession.h>
#include <qmsyncfilter.h>

#include <qssocket.h>

#include "nntp.h"
#include "util.h"


namespace qmnntp {

class NntpReceiveSession;
class NntpReceiveSessionUI;
class NntpReceiveSessionFactory;
class NntpSyncFilterCallback;
class NntpMessageHolder;

class LastIdList;


/****************************************************************************
 *
 * NntpReceiveSession
 *
 */

class NntpReceiveSession : public qm::ReceiveSession
{
public:
	enum State {
		STATE_NONE,
		STATE_HEADER,
		STATE_ALL
	};

public:
	NntpReceiveSession(qs::QSTATUS* pstatus);
	virtual ~NntpReceiveSession();

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
	NntpReceiveSession(const NntpReceiveSession&);
	NntpReceiveSession& operator=(const NntpReceiveSession&);

private:
	class CallbackImpl : public AbstractCallback
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
		virtual qs::QSTATUS authenticating();
		virtual qs::QSTATUS setRange(unsigned int nMin, unsigned int nMax);
		virtual qs::QSTATUS setPos(unsigned int nPos);
	
	private:
		CallbackImpl(const CallbackImpl&);
		CallbackImpl& operator=(const CallbackImpl&);
	
	private:
		qm::ReceiveSessionCallback* pSessionCallback_;
	};

private:
	Nntp* pNntp_;
	CallbackImpl* pCallback_;
	qm::Document* pDocument_;
	qm::Account* pAccount_;
	qm::SubAccount* pSubAccount_;
	qm::NormalFolder* pFolder_;
	HWND hwnd_;
	qs::Profile* pProfile_;
	qs::Logger* pLogger_;
	qm::ReceiveSessionCallback* pSessionCallback_;
	LastIdList* pLastIdList_;
};


/****************************************************************************
 *
 * NntpReceiveSessionUI
 *
 */

class NntpReceiveSessionUI : public qm::ReceiveSessionUI
{
public:
	NntpReceiveSessionUI(qs::QSTATUS* pstatus);
	virtual ~NntpReceiveSessionUI();

public:
	virtual const WCHAR* getClass();
	virtual qs::QSTATUS getDisplayName(qs::WSTRING* pwstrName);
	virtual short getDefaultPort();
	virtual qs::QSTATUS createPropertyPage(
		qm::SubAccount* pSubAccount, qs::PropertyPage** ppPage);

private:
	NntpReceiveSessionUI(const NntpReceiveSessionUI&);
	NntpReceiveSessionUI& operator=(const NntpReceiveSessionUI&);
};


/****************************************************************************
 *
 * NntpReceiveSessionFactory
 *
 */

class NntpReceiveSessionFactory : public qm::ReceiveSessionFactory
{
private:
	NntpReceiveSessionFactory();

public:
	virtual ~NntpReceiveSessionFactory();

protected:
	virtual qs::QSTATUS createSession(qm::ReceiveSession** ppReceiveSession);
	virtual qs::QSTATUS createUI(qm::ReceiveSessionUI** ppUI);

private:
	NntpReceiveSessionFactory(const NntpReceiveSessionFactory&);
	NntpReceiveSessionFactory& operator=(const NntpReceiveSessionFactory&);

private:
	static NntpReceiveSessionFactory factory__;
};


/****************************************************************************
 *
 * NntpSyncFilterCallback
 *
 */

class NntpSyncFilterCallback : public qm::SyncFilterCallback
{
public:
	NntpSyncFilterCallback(qm::Document* pDocument, qm::Account* pAccount,
		qm::NormalFolder* pFolder, qm::Message* pMessage,
		unsigned int nSize, HWND hwnd, qs::Profile* pProfile,
		qm::MacroVariableHolder* pGlobalVariable, Nntp* pNntp,
		unsigned int nMessage, qs::string_ptr<qs::STRING>* pstrMessage,
		NntpReceiveSession::State* pState);
	virtual ~NntpSyncFilterCallback();

public:
	qs::QSTATUS getMessage(unsigned int nFlag);

public:
	virtual const qm::NormalFolder* getFolder();
	virtual qs::QSTATUS getMacroContext(qm::MacroContext** ppContext);

private:
	NntpSyncFilterCallback(const NntpSyncFilterCallback&);
	NntpSyncFilterCallback& operator=(const NntpSyncFilterCallback&);

private:
	qm::Document* pDocument_;
	qm::Account* pAccount_;
	qm::NormalFolder* pFolder_;
	qm::Message* pMessage_;
	unsigned int nSize_;
	HWND hwnd_;
	qs::Profile* pProfile_;
	qm::MacroVariableHolder* pGlobalVariable_;
	Nntp* pNntp_;
	unsigned int nMessage_;
	qs::string_ptr<qs::STRING>* pstrMessage_;
	NntpReceiveSession::State* pState_;
	unsigned int* pnGetSize_;
	NntpMessageHolder* pmh_;
};


/****************************************************************************
 *
 * NntpMessageHolder
 *
 */

class NntpMessageHolder : public qm::AbstractMessageHolder
{
public:
	NntpMessageHolder(NntpSyncFilterCallback* pCallback,
		qm::NormalFolder* pFolder, qm::Message* pMessage,
		unsigned int nSize, qs::QSTATUS* pstatus);
	virtual ~NntpMessageHolder();

public:
	virtual qs::QSTATUS getMessage(unsigned int nFlags,
		const WCHAR* pwszField, qm::Message* pMessage);

private:
	NntpMessageHolder(const NntpMessageHolder&);
	NntpMessageHolder& operator=(const NntpMessageHolder&);

private:
	NntpSyncFilterCallback* pCallback_;
};

}

#endif // __NNTPRECEIVESESSION_H__
