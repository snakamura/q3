/*
 * $Id$
 *
 * Copyright(C) 1998-2004 Satoshi Nakamura
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
	SmtpSendSession();
	virtual ~SmtpSendSession();

public:
	virtual bool init(qm::Document* pDocument,
					  qm::Account* pAccount,
					  qm::SubAccount* pSubAccount,
					  qs::Profile* pProfile,
					  qs::Logger* pLogger,
					  qm::SendSessionCallback* pCallback);
	virtual void term();
	virtual bool connect();
	virtual void disconnect();
	virtual bool sendMessage(qm::Message* pMessage);

private:
	void reportError();

private:
	SmtpSendSession(const SmtpSendSession&);
	SmtpSendSession& operator=(const SmtpSendSession&);

private:
	class CallbackImpl :
		public qs::SocketCallback,
		public qm::DefaultSSLSocketCallback,
		public SmtpCallback
	{
	public:
		CallbackImpl(qm::SubAccount* pSubAccount,
					 const qm::Security* pSecurity,
					 qm::SendSessionCallback* pSessionCallback);
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
		virtual bool getUserInfo(qs::wstring_ptr* pwstrUserName,
								 qs::wstring_ptr* pwstrPassword);
		virtual void setPassword(const WCHAR* pwszPassword);
		virtual qs::wstring_ptr getLocalHost();
		virtual qs::wstring_ptr getAuthMethods();
		
		virtual void authenticating();
		virtual void setRange(unsigned int nMin,
							  unsigned int nMax);
		virtual void setPos(unsigned int nPos);
	
	private:
		CallbackImpl(const CallbackImpl&);
		CallbackImpl& operator=(const CallbackImpl&);
	
	private:
		qm::SubAccount* pSubAccount_;
		qm::SendSessionCallback* pSessionCallback_;
	};

private:
	std::auto_ptr<Smtp> pSmtp_;
	std::auto_ptr<CallbackImpl> pCallback_;
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
	SmtpSendSessionUI();
	virtual ~SmtpSendSessionUI();

public:
	virtual const WCHAR* getClass();
	virtual qs::wstring_ptr getDisplayName();
	virtual short getDefaultPort(bool bSecure);
	virtual bool isSupported(Support support);
	virtual std::auto_ptr<qs::PropertyPage> createPropertyPage(qm::SubAccount* pSubAccount);

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
	virtual std::auto_ptr<qm::SendSession> createSession();
	virtual std::auto_ptr<qm::SendSessionUI> createUI();

private:
	SmtpSendSessionFactory(const SmtpSendSessionFactory&);
	SmtpSendSessionFactory& operator=(const SmtpSendSessionFactory&);

private:
	static SmtpSendSessionFactory factory__;
};

}

#endif // __SMTPSENDSESSION_H__
