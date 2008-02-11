/*
 * $Id$
 *
 * Copyright(C) 1998-2008 Satoshi Nakamura
 *
 */

#ifndef __IMAP4CONNECTION_H__
#define __IMAP4CONNECTION_H__

#include <qmconnection.h>

#include "imap4.h"


namespace qmimap4 {

class Imap4Connection;
class Imap4ConnectionFactory;


/****************************************************************************
 *
 * Imap4Connection
 *
 */

class Imap4Connection : public qm::Connection
{
public:
	Imap4Connection(long nTimeout,
					qs::SocketCallback* pSocketCallback,
					qs::SSLSocketCallback* pSSLSocketCallback,
					qm::ConnectionCallback* pConnectionCallback,
					qs::Logger* pLogger);
	virtual ~Imap4Connection();

public:
	virtual bool connect(const WCHAR* pwszHost,
						 short nPort,
						 qm::SubAccount::Secure secure);
	virtual void disconnect();

private:
	Imap4Connection(const Imap4Connection&);
	Imap4Connection& operator=(const Imap4Connection&);

private:
	class CallbackImpl :
		public qs::DefaultFilterSocketCallback,
		public Imap4Callback
	{
	public:
		CallbackImpl(qs::SocketCallback* pSocketCallback,
					 qm::ConnectionCallback* pConnectionCallback);
		~CallbackImpl();
	
	public:
		qm::ConnectionCallback* getConnectionCallback() const;
	
	public:
		virtual bool getUserInfo(qs::wstring_ptr* pwstrUserName,
								 qs::wstring_ptr* pwstrPassword);
		virtual void setPassword(const WCHAR* pwszPassword);
		virtual qs::wstring_ptr getAuthMethods();
		
		virtual void authenticating();
		virtual void setRange(size_t nMin,
							  size_t nMax);
		virtual void setPos(size_t nPos);
		
		virtual bool response(Response* pResponse);
	
	private:
		qm::ConnectionCallback* pConnectionCallback_;
	};

private:
	std::auto_ptr<CallbackImpl> pCallback_;
	std::auto_ptr<Imap4> pImap4_;
};


/****************************************************************************
 *
 * Imap4ConnectionUI
 *
 */

class Imap4ConnectionUI : public qm::ConnectionUI
{
public:
	Imap4ConnectionUI();
	virtual ~Imap4ConnectionUI();

public:
	virtual qs::wstring_ptr getDisplayName();
	virtual short getDefaultPort(bool bSecure);

private:
	Imap4ConnectionUI(const Imap4ConnectionUI&);
	Imap4ConnectionUI& operator=(const Imap4ConnectionUI&);
};


/****************************************************************************
 *
 * Imap4ConnectionFactory
 *
 */

class Imap4ConnectionFactory : public qm::ConnectionFactory
{
private:
	Imap4ConnectionFactory();

public:
	~Imap4ConnectionFactory();

protected:
	virtual std::auto_ptr<qm::Connection> createConnection(long nTimeout,
														   qs::SocketCallback* pSocketCallback,
														   qs::SSLSocketCallback* pSSLSocketCallback,
														   qm::ConnectionCallback* pConnectionCallback,
														   qs::Logger* pLogger);
	virtual std::auto_ptr<qm::ConnectionUI> createUI();

private:
	Imap4ConnectionFactory(const Imap4ConnectionFactory&);
	Imap4ConnectionFactory& operator=(const Imap4ConnectionFactory&);

private:
	static Imap4ConnectionFactory factory__;
};

}

#endif // __IMAP4CONNECTION_H__
