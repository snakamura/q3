/*
 * $Id$
 *
 * Copyright(C) 1998-2004 Satoshi Nakamura
 * All rights reserved.
 *
 */

#include <qslog.h>

#include "crypto.h"
#include "ssl.h"

using namespace qs;
using namespace qscrypto;


/****************************************************************************
 *
 * SSLSocketImpl
 *
 */

qscrypto::SSLSocketImpl::SSLSocketImpl(Socket* pSocket,
									   bool bDeleteSocket,
									   SSLSocketCallback* pCallback,
									   Logger* pLogger) :
	pSocket_(0),
	bDeleteSocket_(bDeleteSocket),
	pCallback_(pCallback),
	pLogger_(pLogger),
	pInputStream_(0),
	pOutputStream_(0),
	pCTX_(0),
	pSSL_(0)
{
	if (!connect(pSocket))
		return;
	pSocket_ = pSocket;
}

qscrypto::SSLSocketImpl::~SSLSocketImpl()
{
	close();
	
	delete pInputStream_;
	delete pOutputStream_;
	if (pSSL_)
		SSL_free(pSSL_);
	if (pCTX_) {
		pCTX_->cert_store = 0;
		SSL_CTX_free(pCTX_);
	}
	if (bDeleteSocket_)
		delete pSocket_;
}

bool qscrypto::SSLSocketImpl::operator!() const
{
	return pSocket_ == 0;
}

long qscrypto::SSLSocketImpl::getTimeout() const
{
	return pSocket_->getTimeout();
}

unsigned int qscrypto::SSLSocketImpl::getLastError() const
{
	// TODO
	return 0;
}

void qscrypto::SSLSocketImpl::setLastError(unsigned int nError)
{
	// TODO
}

bool qscrypto::SSLSocketImpl::close()
{
	Log log(pLogger_, L"qscrypto::SSLSocketImpl");
	
	log.debug(L"Shuting down SSL connection...");
	
	SSL_shutdown(pSSL_);
	
	log.debug(L"SSL connection shut down.");
	
	if (bDeleteSocket_ && pSocket_) {
		if (!pSocket_->close())
			return false;
	}
	
	return true;
}

int qscrypto::SSLSocketImpl::recv(char* p,
								  int nLen,
								  int nFlags)
{
	assert(p);
	
	Log log(pLogger_, L"qscrypto::SSLSocketImpl");
	
	int nRead = SSL_read(pSSL_, p, nLen);
	if (nRead < 0) {
		log.debug(L"Error occured while receiving.");
		return -1;
	}
	
	log.debug(L"Received data", reinterpret_cast<unsigned char*>(p), nRead);
	
	return nRead;
}

int qscrypto::SSLSocketImpl::send(const char* p,
								  int nLen,
								  int nFlags)
{
	assert(p);
	
	Log log(pLogger_, L"qscrypto::SSLSocketImpl");
	
	int nWritten = SSL_write(pSSL_, p, nLen);
	if (nWritten < 0) {
		log.debug(L"Error occured while sending.");
		return -1;
	}
	
	log.debug(L"Sent data", reinterpret_cast<const unsigned char*>(p), nWritten);
	
	return nWritten;
}

int qscrypto::SSLSocketImpl::select(int nSelect)
{
	return select(nSelect, getTimeout());
}

int qscrypto::SSLSocketImpl::select(int nSelect,
									long nTimeout)
{
	if (nSelect & SELECT_READ && SSL_pending(pSSL_) > 0)
		return SELECT_READ;
	else
		return pSocket_->select(nSelect, nTimeout);
}

InputStream* qscrypto::SSLSocketImpl::getInputStream()
{
	if (!pInputStream_)
		pInputStream_ = new SocketInputStream(this);
	return pInputStream_;
}

OutputStream* qscrypto::SSLSocketImpl::getOutputStream()
{
	if (!pOutputStream_)
		pOutputStream_ = new SocketOutputStream(this);
	return pOutputStream_;
}

bool qscrypto::SSLSocketImpl::connect(Socket* pSocket)
{
	Log log(pLogger_, L"qscrypto::SSLSocketImpl");
	
	log.debug(L"Initializing SSL context...");
	
	pCTX_ = SSL_CTX_new(SSLv23_method());
	if (!pCTX_)
		return false;
	
	log.debug(L"Installing trusted CAs...");
	
	const Store* pStore = pCallback_->getCertStore();
	if (pStore)
		SSL_CTX_set_cert_store(pCTX_,
			static_cast<const StoreImpl*>(pStore)->getStore());
	
	log.debug(L"Initializing SSL...");
	
	pSSL_ = SSL_new(pCTX_);
	if (!pSSL_)
		return false;
	
	SSL_set_fd(pSSL_, pSocket->getSocket());
	
	log.debug(L"Starting SSL handshake...");
	
	if (SSL_connect(pSSL_) <= 0)
		return false;
	
	log.debug(L"SSL handshake completed.");
	
	bool bVerified = SSL_get_verify_result(pSSL_) == X509_V_OK;
	if (log.isDebugEnabled()) {
		if (bVerified)
			log.debug(L"Server certificate is verified.");
		else
			log.debug(L"Failed to verify server certificate.");
	}
	
	X509* pX509 = SSL_get_peer_certificate(pSSL_);
	CertificateImpl cert(pX509);
	if (log.isDebugEnabled()) {
		wstring_ptr wstrCert(cert.getText());
		log.debug(wstrCert.get());
	}
	
	if (!pCallback_->checkCertificate(cert, bVerified)) {
		log.debug(L"Failed to check server certificate");
		return false;
	}
	
	return true;
}


/****************************************************************************
 *
 * SSLSocketFactoryImpl
 *
 */

SSLSocketFactoryImpl qscrypto::SSLSocketFactoryImpl::factory__;

qscrypto::SSLSocketFactoryImpl::SSLSocketFactoryImpl()
{
	registerFactory(this);
}

qscrypto::SSLSocketFactoryImpl::~SSLSocketFactoryImpl()
{
	unregisterFactory(this);
}

std::auto_ptr<SSLSocket> qscrypto::SSLSocketFactoryImpl::createSSLSocket(Socket* pSocket,
																		 bool bDeleteSocket,
																		 SSLSocketCallback* pCallback,
																		 Logger* pLogger)
{
	std::auto_ptr<SSLSocketImpl> pSSLSocket(new SSLSocketImpl(
		pSocket, bDeleteSocket, pCallback, pLogger));
	if (!*pSSLSocket.get())
		return 0;
	return pSSLSocket;
}
