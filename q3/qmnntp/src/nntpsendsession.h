/*
 * $Id$
 *
 * Copyright(C) 1998-2004 Satoshi Nakamura
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
	NntpSendSession();
	virtual ~NntpSendSession();

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
	NntpSendSession(const NntpSendSession&);
	NntpSendSession& operator=(const NntpSendSession&);

private:
	class CallbackImpl :
		public qs::SocketCallback,
		public qm::DefaultSSLSocketCallback,
		public NntpCallback
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
	};

private:
	std::auto_ptr<Nntp> pNntp_;
	std::auto_ptr<CallbackImpl> pCallback_;
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
	NntpSendSessionUI();
	virtual ~NntpSendSessionUI();

public:
	virtual const WCHAR* getClass();
	virtual qs::wstring_ptr getDisplayName();
	virtual short getDefaultPort(bool bSecure);
	virtual bool isSupported(Support support);
	virtual std::auto_ptr<qs::PropertyPage> createPropertyPage(qm::SubAccount* pSubAccount);

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
	virtual std::auto_ptr<qm::SendSession> createSession();
	virtual std::auto_ptr<qm::SendSessionUI> createUI();

private:
	NntpSendSessionFactory(const NntpSendSessionFactory&);
	NntpSendSessionFactory& operator=(const NntpSendSessionFactory&);

private:
	static NntpSendSessionFactory factory__;
};

}

#endif // __NNTPSENDSESSION_H__
