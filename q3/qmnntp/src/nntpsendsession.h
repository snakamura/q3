/*
 * $Id$
 *
 * Copyright(C) 1998-2003 Satoshi Nakamura
 * All rights reserved.
 *
 */

#ifndef __NNTPSENDSESSION_H__
#define __NNTPSENDSESSION_H__

#include <qmsession.h>

#include <qssocket.h>

#include "nntp.h"


namespace qmnntp {

/****************************************************************************
 *
 * NntpSendSession
 *
 */

class NntpSendSession : public qm::SendSession
{
public:
	NntpSendSession(qs::QSTATUS* pstatus);
	virtual ~NntpSendSession();

public:
	virtual qs::QSTATUS init(qm::Account* pAccount,
		qm::SubAccount* pSubAccount, qs::Profile* pProfile,
		qs::Logger* pLogger, qm::SendSessionCallback* pCallback);
	virtual qs::QSTATUS connect();
	virtual qs::QSTATUS disconnect();
	virtual qs::QSTATUS sendMessage(qm::Message* pMessage);

private:
	NntpSendSession(const NntpSendSession&);
	NntpSendSession& operator=(const NntpSendSession&);

private:
	class CallbackImpl : public qs::SocketCallback, public NntpCallback
	{
	public:
		CallbackImpl(qm::SubAccount* pSubAccount,
			qm::SendSessionCallback* pSessionCallback, qs::QSTATUS* pstatus);
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
		qm::SendSessionCallback* pSessionCallback_;
	};

private:
	Nntp* pNntp_;
	CallbackImpl* pCallback_;
	qm::Account* pAccount_;
	qm::SubAccount* pSubAccount_;
	qs::Logger* pLogger_;
	qm::SendSessionCallback* pSessionCallback_;
};


/****************************************************************************
 *
 * NntpSendSessionUI
 *
 */

class NntpSendSessionUI : public qm::SendSessionUI
{
public:
	NntpSendSessionUI(qs::QSTATUS* pstatus);
	virtual ~NntpSendSessionUI();

public:
	virtual const WCHAR* getClass();
	virtual qs::QSTATUS getDisplayName(qs::WSTRING* pwstrName);
	virtual short getDefaultPort();
	virtual qs::QSTATUS createPropertyPage(
		qm::SubAccount* pSubAccount, qs::PropertyPage** ppPage);

private:
	NntpSendSessionUI(const NntpSendSessionUI&);
	NntpSendSessionUI& operator=(const NntpSendSessionUI&);
};


/****************************************************************************
 *
 * NntpSendSessionFactory
 *
 */

class NntpSendSessionFactory : public qm::SendSessionFactory
{
private:
	NntpSendSessionFactory();

public:
	virtual ~NntpSendSessionFactory();

public:
	virtual qs::QSTATUS createSession(qm::SendSession** ppSendSession);
	virtual qs::QSTATUS createUI(qm::SendSessionUI** ppUI);

private:
	NntpSendSessionFactory(const NntpSendSessionFactory&);
	NntpSendSessionFactory& operator=(const NntpSendSessionFactory&);

private:
	static NntpSendSessionFactory factory__;
};

}

#endif // __NNTPSENDSESSION_H__
