/*
 * $Id$
 *
 * Copyright(C) 1998-2004 Satoshi Nakamura
 * All rights reserved.
 *
 */

#ifndef __RSSSENDSESSION_H__
#define __RSSSENDSESSION_H__

#include <qmsession.h>


namespace qmrss {

/****************************************************************************
 *
 * RssSendSession
 *
 */

class RssSendSession : public qm::SendSession
{
public:
	RssSendSession();
	virtual ~RssSendSession();

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
	RssSendSession(const RssSendSession&);
	RssSendSession& operator=(const RssSendSession&);
};


/****************************************************************************
 *
 * RssSendSessionUI
 *
 */

class RssSendSessionUI : public qm::SendSessionUI
{
public:
	RssSendSessionUI();
	virtual ~RssSendSessionUI();

public:
	virtual const WCHAR* getClass();
	virtual qs::wstring_ptr getDisplayName();
	virtual short getDefaultPort();
	virtual std::auto_ptr<qs::PropertyPage> createPropertyPage(qm::SubAccount* pSubAccount);

private:
	RssSendSessionUI(const RssSendSessionUI&);
	RssSendSessionUI& operator=(const RssSendSessionUI&);
};


/****************************************************************************
 *
 * RssSendSessionFactory
 *
 */

class RssSendSessionFactory : public qm::SendSessionFactory
{
private:
	RssSendSessionFactory();

public:
	virtual ~RssSendSessionFactory();

public:
	virtual std::auto_ptr<qm::SendSession> createSession();
	virtual std::auto_ptr<qm::SendSessionUI> createUI();

private:
	RssSendSessionFactory(const RssSendSessionFactory&);
	RssSendSessionFactory& operator=(const RssSendSessionFactory&);

private:
	static RssSendSessionFactory factory__;
};

}

#endif // __RSSSENDSESSION_H__
