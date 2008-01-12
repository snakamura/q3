/*
 * $Id$
 *
 * Copyright(C) 1998-2008 Satoshi Nakamura
 *
 */

#ifndef __SSL_H__
#define __SSL_H__

#include <qsssl.h>

#include <openssl/ssl.h>


namespace qscrypto {

/****************************************************************************
 *
 * SSLSocketImpl
 *
 */

class SSLSocketImpl : public qs::SSLSocket
{
public:
	SSLSocketImpl(qs::Socket* pSocket,
				  bool bDeleteSocket,
				  qs::SSLSocketCallback* pCallback,
				  qs::Logger* pLogger);
	virtual ~SSLSocketImpl();

public:
	bool operator!() const;

public:
	virtual long getTimeout() const;
	virtual void setTimeout(long nTimeout);
	virtual unsigned int getLastError() const;
	virtual void setLastError(unsigned int nError);
	virtual bool close();
	virtual int recv(char* p,
					 int nLen,
					 int nFlags);
	virtual int send(const char* p,
					 int nLen,
					 int nFlags);
	virtual int select(int nSelect);
	virtual int select(int nSelect,
					   long nTimeout);
	virtual qs::InputStream* getInputStream();
	virtual qs::OutputStream* getOutputStream();

private:
	bool connect(qs::Socket* pSocket);

private:
	SSLSocketImpl(const SSLSocketImpl&);
	SSLSocketImpl& operator=(const SSLSocketImpl&);

private:
	qs::Socket* pSocket_;
	bool bDeleteSocket_;
	qs::SSLSocketCallback* pCallback_;
	qs::Logger* pLogger_;
	qs::SocketInputStream* pInputStream_;
	qs::SocketOutputStream* pOutputStream_;
	SSL_CTX* pCTX_;
	SSL* pSSL_;
	unsigned int nError_;
};


/****************************************************************************
 *
 * SSLSocketFactoryImpl
 *
 */

class SSLSocketFactoryImpl : public qs::SSLSocketFactory
{
public:
	SSLSocketFactoryImpl();
	virtual ~SSLSocketFactoryImpl();

public:
	virtual std::auto_ptr<qs::SSLSocket> createSSLSocket(qs::Socket* pSocket,
														 bool bDeleteSocket,
														 qs::SSLSocketCallback* pCallback,
														 qs::Logger* pLogger);

private:
	SSLSocketFactoryImpl(const SSLSocketFactoryImpl&);
	SSLSocketFactoryImpl& operator=(const SSLSocketFactoryImpl&);

private:
	static SSLSocketFactoryImpl factory__;
};

}

#endif // __SSL_H__
