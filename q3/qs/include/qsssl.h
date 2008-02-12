/*
 * $Id$
 *
 * Copyright(C) 1998-2008 Satoshi Nakamura
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
	/**
	 * Get certificate store.
	 *
	 * @return Certificate store. null if not exists.
	 */
	virtual const Store* getCertStore() = 0;
	
	/**
	 * Check server certificate.
	 *
	 * @param cert [in] Server certificate.
	 * @param bVerified [in] true if certificate was verified, false otherwise.
	 * @param pwszVerifyError [in] An error message why verification failed.
	 *                             null if verification successed.
	 * @return true if ok, flase otherwise.
	 */
	virtual bool checkCertificate(const Certificate& cert,
								  bool bVerified,
								  const WCHAR* pwszVerifyError) = 0;
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
	/**
	 * Create SSLSocket.
	 *
	 * @param pSocket [in] Base socket.
	 * @param bDeleteSocket [in] true if delete the base socket when this socket is deleted.
	 * @param pCallback [in] Callback.
	 * @param ppLogger [out] Logger.
	 * @exception std::bad_alloc Out of memory.
	 */
	virtual std::auto_ptr<SSLSocket> createSSLSocket(Socket* pSocket,
													 bool bDeleteSocket,
													 SSLSocketCallback* pCallback,
													 Logger* ppLogger) = 0;

public:
	/**
	 * Get factory.
	 *
	 * @return Factory. null if no factory was registered.
	 */
	static SSLSocketFactory* getFactory();

protected:
	/**
	 * Register factory.
	 *
	 * @param pFactory [in] Factory.
	 */
	static void registerFactory(SSLSocketFactory* pFactory);
	
	/**
	 * Unregister factory.
	 *
	 * @param pFactory [in] Factory.
	 */
	static void unregisterFactory(SSLSocketFactory* pFactory);

private:
	SSLSocketFactory(const SSLSocketFactory&);
	SSLSocketFactory& operator=(const SSLSocketFactory&);
};

}

#endif // __QSSSL_H__
