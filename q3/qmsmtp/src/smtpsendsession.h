/*
 * $Id$
 *
 * Copyright(C) 1998-2003 Satoshi Nakamura
 * All rights reserved.
 *
 */

#ifndef __SMTPSENDSESSION_H__
#define __SMTPSENDSESSION_H__

#include <qmsession.h>

#include <qslog.h>
#include <qssocket.h>

#include "smtp.h"


namespace qmsmtp {

class SmtpSendSession;
class SmtpSendSessionFactory;


/****************************************************************************
 *
 * SmtpSendSession
 *
 */

class SmtpSendSession : public qm::SendSession
{
public:
	SmtpSendSession(qs::QSTATUS* pstatus);
	virtual ~SmtpSendSession();

public:
	virtual qs::QSTATUS init(qm::Account* pAccount,
		qm::SubAccount* pSubAccount, qs::Profile* pProfile,
		qs::Logger* pLogger, qm::SendSessionCallback* pCallback);
	virtual qs::QSTATUS connect();
	virtual qs::QSTATUS disconnect();
	virtual qs::QSTATUS sendMessage(qm::Message* pMessage);

private:
	qs::QSTATUS reportError();

private:
	SmtpSendSession(const SmtpSendSession&);
	SmtpSendSession& operator=(const SmtpSendSession&);

private:
	class CallbackImpl : public qs::SocketCallback, public SmtpCallback
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
		virtual qs::QSTATUS getLocalHost(qs::WSTRING* pwstrLocalHost);
		virtual qs::QSTATUS getAuthMethods(qs::WSTRING* pwstrAuthMethods);
		
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
	Smtp* pSmtp_;
	CallbackImpl* pCallback_;
	qm::Account* pAccount_;
	qm::SubAccount* pSubAccount_;
	qs::Logger* pLogger_;
	qm::SendSessionCallback* pSessionCallback_;
};


/****************************************************************************
 *
 * SmtpSendSessionUI
 *
 */

class SmtpSendSessionUI : public qm::SendSessionUI
{
public:
	SmtpSendSessionUI(qs::QSTATUS* pstatus);
	virtual ~SmtpSendSessionUI();

public:
	virtual qs::QSTATUS createPropertyPage(
		qm::SubAccount* pSubAccount, qs::PropertyPage** ppPage);
	virtual qs::QSTATUS getDisplayName(qs::WSTRING* pwstrName);
	virtual short getDefaultPort();

private:
	SmtpSendSessionUI(const SmtpSendSessionUI&);
	SmtpSendSessionUI& operator=(const SmtpSendSessionUI&);
};


/****************************************************************************
 *
 * SmtpSendSessionFactory
 *
 */

class SmtpSendSessionFactory : public qm::SendSessionFactory
{
private:
	SmtpSendSessionFactory();

public:
	virtual ~SmtpSendSessionFactory();

public:
	virtual qs::QSTATUS createSession(qm::SendSession** ppSendSession);
	virtual qs::QSTATUS createUI(qm::SendSessionUI** ppUI);

private:
	SmtpSendSessionFactory(const SmtpSendSessionFactory&);
	SmtpSendSessionFactory& operator=(const SmtpSendSessionFactory&);

private:
	static SmtpSendSessionFactory factory__;
};

}

#endif // __SMTPSENDSESSION_H__
