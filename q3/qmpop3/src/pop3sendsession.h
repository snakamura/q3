/*
 * $Id$
 *
 * Copyright(C) 1998-2004 Satoshi Nakamura
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
	Pop3SendSession();
	virtual ~Pop3SendSession();

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
	Pop3SendSession(const Pop3SendSession&);
	Pop3SendSession& operator=(const Pop3SendSession&);

private:
	class CallbackImpl :
		public qs::SocketCallback,
		public qm::DefaultSSLSocketCallback,
		public Pop3Callback
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
		qm::PasswordState state_;
	};

private:
	std::auto_ptr<Pop3> pPop3_;
	std::auto_ptr<CallbackImpl> pCallback_;
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
	Pop3SendSessionUI();
	virtual ~Pop3SendSessionUI();

public:
	virtual const WCHAR* getClass();
	virtual qs::wstring_ptr getDisplayName();
	virtual short getDefaultPort(bool bSecure);
	virtual bool isSupported(Support support);
	virtual std::auto_ptr<qs::PropertyPage> createPropertyPage(qm::SubAccount* pSubAccount);

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
	virtual std::auto_ptr<qm::SendSession> createSession();
	virtual std::auto_ptr<qm::SendSessionUI> createUI();

private:
	Pop3SendSessionFactory(const Pop3SendSessionFactory&);
	Pop3SendSessionFactory& operator=(const Pop3SendSessionFactory&);

private:
	static Pop3SendSessionFactory factory__;
};

}

#endif // __POP3SENDSESSION_H__
