/*
 * $Id$
 *
 * Copyright(C) 1998-2003 Satoshi Nakamura
 * All rights reserved.
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
	SSLSocketImpl(qs::Socket* pSocket, bool bDeleteSocket,
		qs::SSLSocketCallback* pCallback,
		qs::Logger* pLogger, qs::QSTATUS* pstatus);
	virtual ~SSLSocketImpl();

public:
	virtual long getTimeout() const;
	virtual unsigned int getLastError() const;
	virtual void setLastError(unsigned int nError);
	virtual qs::QSTATUS close();
	virtual qs::QSTATUS recv(char* p, int* pnLen, int nFlags);
	virtual qs::QSTATUS send(const char* p, int* pnLen, int nFlags);
	virtual qs::QSTATUS select(int* pnSelect);
	virtual qs::QSTATUS select(int* pnSelect, long nTimeout);
	virtual qs::QSTATUS getInputStream(qs::InputStream** ppStream);
	virtual qs::QSTATUS getOutputStream(qs::OutputStream** ppStream);

private:
	qs::QSTATUS connect(qs::Socket* pSocket);

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
	virtual qs::QSTATUS createSSLSocket(qs::Socket* pSocket,
		bool bDeleteSocket, qs::SSLSocketCallback* pCallback,
		qs::Logger* pLogger, qs::SSLSocket** ppSSLSocket);

private:
	SSLSocketFactoryImpl(const SSLSocketFactoryImpl&);
	SSLSocketFactoryImpl& operator=(const SSLSocketFactoryImpl&);

private:
	static SSLSocketFactoryImpl factory__;
};

}

#endif // __SSL_H__
