/*
 * $Id$
 *
 * Copyright(C) 1998-2003 Satoshi Nakamura
 * All rights reserved.
 *
 */

#ifndef __QSSSL_H__
#define __QSSSL_H__

#include <qs.h>
#include <qssocket.h>


namespace qs {

class SSLSocket;
class SSLSocketCallback;
class SSLSocketFactory;

class Certificate;
class Store;


/****************************************************************************
 *
 * SSLSocket
 *
 */

class QSEXPORTCLASS SSLSocket : public SocketBase
{
public:
	virtual ~SSLSocket();
};


/****************************************************************************
 *
 * SSLSocketCallback
 *
 */

class QSEXPORTCLASS SSLSocketCallback
{
public:
	virtual ~SSLSocketCallback();

public:
	virtual QSTATUS getCertStore(const Store** ppStore) = 0;
	virtual QSTATUS checkCertificate(const Certificate& cert, bool bVerified) = 0;
};


/****************************************************************************
 *
 * SSLSocketFactory
 *
 */

class QSEXPORTCLASS SSLSocketFactory
{
protected:
	SSLSocketFactory();

public:
	virtual ~SSLSocketFactory();

public:
	virtual QSTATUS createSSLSocket(Socket* pSocket, bool bDeleteSocket,
		SSLSocketCallback* pCallback, Logger* ppLogger, SSLSocket** ppSSLSocket) = 0;

public:
	static SSLSocketFactory* getFactory();

protected:
	static QSTATUS regist(SSLSocketFactory* pFactory);
	static QSTATUS unregist(SSLSocketFactory* pFactory);

private:
	SSLSocketFactory(const SSLSocketFactory&);
	SSLSocketFactory& operator=(const SSLSocketFactory&);
};

}

#endif // __QSSSL_H__
