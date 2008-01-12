/*
 * $Id$
 *
 * Copyright(C) 1998-2008 Satoshi Nakamura
 *
 */

#ifndef __QMCONNECTION_H__
#define __QMCONNECTION_H__

#include <qmsession.h>


namespace qm {

class Connection;
class ConnectionCallback;
class ConnectionUI;
class ConnectionFactory;


/****************************************************************************
 *
 * Connection
 *
 */

class QMEXPORTCLASS Connection
{
public:
	virtual ~Connection();

public:
	virtual bool connect(const WCHAR* pwszHost,
						 short nPort,
						 SubAccount::Secure secure) = 0;
	virtual void disconnect() = 0;
	virtual bool setProperty(const WCHAR* pwszName,
							 const WCHAR* pwszValue);
};


/****************************************************************************
 *
 * ConnectionCallback
 *
 */

class QMEXPORTCLASS ConnectionCallback : public ErrorCallback
{
public:
	virtual ~ConnectionCallback();

public:
	virtual bool getUserInfo(qs::wstring_ptr* pwstrUserName,
							 qs::wstring_ptr* pwstrPassword) = 0;
	virtual void setPassword(const WCHAR* pwszPassword) = 0;
	
	virtual void authenticating() = 0;
};


/****************************************************************************
 *
 * ConnectionUI
 *
 */

class QMEXPORTCLASS ConnectionUI
{
public:
	virtual ~ConnectionUI();

public:
	virtual qs::wstring_ptr getDisplayName() = 0;
	virtual short getDefaultPort(bool bSecure) = 0;
};


/****************************************************************************
 *
 * ConnectionFactory
 *
 */

class QMEXPORTCLASS ConnectionFactory
{
public:
	typedef std::vector<qs::WSTRING> NameList;

protected:
	ConnectionFactory();
	~ConnectionFactory();

public:
	static std::auto_ptr<Connection> getConnection(const WCHAR* pwszName,
												   long nTimeout,
												   qs::SocketCallback* pSocketCallback,
												   qs::SSLSocketCallback* pSSLSocketCallback,
												   ConnectionCallback* pConnectionCallback,
												   qs::Logger* pLogger);
	static std::auto_ptr<ConnectionUI> getUI(const WCHAR* pwszName);
	static void getNames(NameList* pList);

protected:
	virtual std::auto_ptr<Connection> createConnection(long nTimeout,
													   qs::SocketCallback* pSocketCallback,
													   qs::SSLSocketCallback* pSSLSocketCallback,
													   ConnectionCallback* pConnectionCallback,
													   qs::Logger* pLogger) = 0;
	virtual std::auto_ptr<ConnectionUI> createUI() = 0;

protected:
	static void registerFactory(const WCHAR* pwszName,
								ConnectionFactory* pFactory);
	static void unregisterFactory(const WCHAR* pwszName);

private:
	ConnectionFactory(const ConnectionFactory&);
	ConnectionFactory& operator=(const ConnectionFactory&);
};

}

#endif // __QMCONNECTION_H__
