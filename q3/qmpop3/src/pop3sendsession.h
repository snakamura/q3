/*
 * $Id$
 *
 * Copyright(C) 1998-2003 Satoshi Nakamura
 * All rights reserved.
 *
 */

#ifndef __POP3SENDSESSION_H__
#define __POP3SENDSESSION_H__

#include <qm.h>
#include <qmsecurity.h>
#include <qmsession.h>

#include <qs.h>
#include <qssocket.h>

#include "pop3.h"


namespace qmpop3 {

/****************************************************************************
 *
 * Pop3SendSession
 *
 */

class Pop3SendSession : public qm::SendSession
{
public:
	Pop3SendSession(qs::QSTATUS* pstatus);
	virtual ~Pop3SendSession();

public:
	virtual qs::QSTATUS init(qm::Document* pDocument, qm::Account* pAccount,
		qm::SubAccount* pSubAccount, qs::Profile* pProfile,
		qs::Logger* pLogger, qm::SendSessionCallback* pCallback);
	virtual qs::QSTATUS connect();
	virtual qs::QSTATUS disconnect();
	virtual qs::QSTATUS sendMessage(qm::Message* pMessage);

private:
	Pop3SendSession(const Pop3SendSession&);
	Pop3SendSession& operator=(const Pop3SendSession&);

private:
	class CallbackImpl :
		public qs::SocketCallback,
		public qs::SSLSocketCallback,
		public Pop3Callback
	{
	public:
		CallbackImpl(qm::SubAccount* pSubAccount, const qm::Security* pSecurity,
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
		virtual qs::QSTATUS getCertStore(const qs::Store** ppStore);
		virtual qs::QSTATUS checkCertificate(
			const qs::Certificate& cert, bool bVerified);
	
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
		const qm::Security* pSecurity_;
		qm::SendSessionCallback* pSessionCallback_;
	};

private:
	Pop3* pPop3_;
	CallbackImpl* pCallback_;
	qm::Account* pAccount_;
	qm::SubAccount* pSubAccount_;
	qs::Logger* pLogger_;
	qm::SendSessionCallback* pSessionCallback_;
};


/****************************************************************************
 *
 * Pop3SendSessionUI
 *
 */

class Pop3SendSessionUI : public qm::SendSessionUI
{
public:
	Pop3SendSessionUI(qs::QSTATUS* pstatus);
	virtual ~Pop3SendSessionUI();

public:
	virtual const WCHAR* getClass();
	virtual qs::QSTATUS getDisplayName(qs::WSTRING* pwstrName);
	virtual short getDefaultPort();
	virtual qs::QSTATUS createPropertyPage(
		qm::SubAccount* pSubAccount, qs::PropertyPage** ppPage);

private:
	Pop3SendSessionUI(const Pop3SendSessionUI&);
	Pop3SendSessionUI& operator=(const Pop3SendSessionUI&);
};


/****************************************************************************
 *
 * Pop3SendSessionFactory
 *
 */

class Pop3SendSessionFactory : public qm::SendSessionFactory
{
private:
	Pop3SendSessionFactory();

public:
	virtual ~Pop3SendSessionFactory();

public:
	virtual qs::QSTATUS createSession(qm::SendSession** ppSendSession);
	virtual qs::QSTATUS createUI(qm::SendSessionUI** ppUI);

private:
	Pop3SendSessionFactory(const Pop3SendSessionFactory&);
	Pop3SendSessionFactory& operator=(const Pop3SendSessionFactory&);

private:
	static Pop3SendSessionFactory factory__;
};

}

#endif // __POP3SENDSESSION_H__
