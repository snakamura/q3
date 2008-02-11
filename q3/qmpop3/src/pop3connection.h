/*
 * $Id$
 *
 * Copyright(C) 1998-2008 Satoshi Nakamura
 *
 */

#ifndef __POP3CONNECTION_H__
#define __POP3CONNECTION_H__

#include <qmconnection.h>

#include "pop3.h"


namespace qmpop3 {

class Pop3Connection;
class Pop3ConnectionFactory;


/****************************************************************************
 *
 * Pop3Connection
 *
 */

class Pop3Connection : public qm::Connection
{
public:
	Pop3Connection(long nTimeout,
				   qs::SocketCallback* pSocketCallback,
				   qs::SSLSocketCallback* pSSLSocketCallback,
				   qm::ConnectionCallback* pConnectionCallback,
				   qs::Logger* pLogger);
	virtual ~Pop3Connection();

public:
	virtual bool connect(const WCHAR* pwszHost,
						 short nPort,
						 qm::SubAccount::Secure secure);
	virtual void disconnect();
	virtual bool setProperty(const WCHAR* pwszName,
							 const WCHAR* pwszValue);

private:
	Pop3Connection(const Pop3Connection&);
	Pop3Connection& operator=(const Pop3Connection&);

private:
	class CallbackImpl :
		public qs::DefaultFilterSocketCallback,
		public Pop3Callback
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
		
		virtual void authenticating();
		virtual void setRange(size_t nMin,
							  size_t nMax);
		virtual void setPos(size_t nPos);
	
	private:
		qm::ConnectionCallback* pConnectionCallback_;
	};

private:
	std::auto_ptr<CallbackImpl> pCallback_;
	std::auto_ptr<Pop3> pPop3_;
	bool bApop_;
};


/****************************************************************************
 *
 * Pop3ConnectionUI
 *
 */

class Pop3ConnectionUI : public qm::ConnectionUI
{
public:
	Pop3ConnectionUI();
	virtual ~Pop3ConnectionUI();

public:
	virtual qs::wstring_ptr getDisplayName();
	virtual short getDefaultPort(bool bSecure);

private:
	Pop3ConnectionUI(const Pop3ConnectionUI&);
	Pop3ConnectionUI& operator=(const Pop3ConnectionUI&);
};


/****************************************************************************
 *
 * Pop3ConnectionFactory
 *
 */

class Pop3ConnectionFactory : public qm::ConnectionFactory
{
private:
	Pop3ConnectionFactory();

public:
	~Pop3ConnectionFactory();

protected:
	virtual std::auto_ptr<qm::Connection> createConnection(long nTimeout,
														   qs::SocketCallback* pSocketCallback,
														   qs::SSLSocketCallback* pSSLSocketCallback,
														   qm::ConnectionCallback* pConnectionCallback,
														   qs::Logger* pLogger);
	virtual std::auto_ptr<qm::ConnectionUI> createUI();

private:
	Pop3ConnectionFactory(const Pop3ConnectionFactory&);
	Pop3ConnectionFactory& operator=(const Pop3ConnectionFactory&);

private:
	static Pop3ConnectionFactory factory__;
};

}

#endif // __POP3CONNECTION_H__
