/*
 * $Id$
 *
 * Copyright(C) 1998-2003 Satoshi Nakamura
 * All rights reserved.
 *
 */

#include <qslog.h>
#include <qsnew.h>

#include "crypto.h"
#include "ssl.h"

using namespace qs;
using namespace qscrypto;


/****************************************************************************
 *
 * SSLSocketImpl
 *
 */

qscrypto::SSLSocketImpl::SSLSocketImpl(Socket* pSocket, bool bDeleteSocket,
	SSLSocketCallback* pCallback, Logger* pLogger, QSTATUS* pstatus) :
	pSocket_(0),
	bDeleteSocket_(bDeleteSocket),
	pCallback_(pCallback),
	pLogger_(pLogger),
	pInputStream_(0),
	pOutputStream_(0),
	pCTX_(0),
	pSSL_(0)
{
	DECLARE_QSTATUS();
	
	status = connect(pSocket);
	CHECK_QSTATUS_SET(pstatus);
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

QSTATUS qscrypto::SSLSocketImpl::close()
{
	DECLARE_QSTATUS();
	
	Log log(pLogger_, L"qscrypto::SSLSocketImpl");
	
	status = log.debug(L"Shuting down SSL connection...");
	CHECK_QSTATUS();
	
	SSL_shutdown(pSSL_);
	
	status = log.debug(L"SSL connection shut down.");
	CHECK_QSTATUS();
	
	if (bDeleteSocket_ && pSocket_) {
		status = pSocket_->close();
		CHECK_QSTATUS();
	}
	
	return QSTATUS_SUCCESS;
}

QSTATUS qscrypto::SSLSocketImpl::recv(char* p, int* pnLen, int nFlags)
{
	assert(p);
	assert(pnLen);
	
	DECLARE_QSTATUS();
	
	Log log(pLogger_, L"qscrypto::SSLSocketImpl");
	
	*pnLen = SSL_read(pSSL_, p, *pnLen);
	if (*pnLen < 0) {
		status = log.debug(L"Error occured while receiving.");
		CHECK_QSTATUS();
		return QSTATUS_FAIL;
	}
	
	status = log.debug(L"Received data",
		reinterpret_cast<unsigned char*>(p), *pnLen);
	CHECK_QSTATUS();
	
	return QSTATUS_SUCCESS;
}

QSTATUS qscrypto::SSLSocketImpl::send(const char* p, int* pnLen, int nFlags)
{
	assert(p);
	assert(pnLen);
	
	DECLARE_QSTATUS();
	
	Log log(pLogger_, L"qscrypto::SSLSocketImpl");
	
	*pnLen = SSL_write(pSSL_, p, *pnLen);
	if (*pnLen < 0) {
		status = log.debug(L"Error occured while sending.");
		CHECK_QSTATUS();
		return QSTATUS_FAIL;
	}
	
	status = log.debug(L"Sent data",
		reinterpret_cast<const unsigned char*>(p), *pnLen);
	CHECK_QSTATUS();
	
	return QSTATUS_SUCCESS;
}

QSTATUS qscrypto::SSLSocketImpl::select(int* pnSelect)
{
	return select(pnSelect, getTimeout());
}

QSTATUS qscrypto::SSLSocketImpl::select(int* pnSelect, long nTimeout)
{
	assert(pnSelect);
	
	DECLARE_QSTATUS();
	
	if (*pnSelect & SELECT_READ && SSL_pending(pSSL_) > 0) {
		*pnSelect = SELECT_READ;
	}
	else {
		status = pSocket_->select(pnSelect, nTimeout);
		CHECK_QSTATUS();
	}
	
	return QSTATUS_SUCCESS;
}

QSTATUS qscrypto::SSLSocketImpl::getInputStream(InputStream** ppStream)
{
	DECLARE_QSTATUS();
	
	if (!pInputStream_) {
		status = newQsObject(this, &pInputStream_);
		CHECK_QSTATUS();
	}
	*ppStream = pInputStream_;
	
	return QSTATUS_SUCCESS;
}

QSTATUS qscrypto::SSLSocketImpl::getOutputStream(OutputStream** ppStream)
{
	DECLARE_QSTATUS();
	
	if (!pOutputStream_) {
		status = newQsObject(this, &pOutputStream_);
		CHECK_QSTATUS();
	}
	*ppStream = pOutputStream_;
	
	return QSTATUS_SUCCESS;
}

QSTATUS qscrypto::SSLSocketImpl::connect(Socket* pSocket)
{
	DECLARE_QSTATUS();
	
	Log log(pLogger_, L"qscrypto::SSLSocketImpl");
	
	status = log.debug(L"Initializing SSL context...");
	CHECK_QSTATUS();
	
	pCTX_ = SSL_CTX_new(SSLv23_method());
	if (!pCTX_)
		return QSTATUS_FAIL;
	
	status = log.debug(L"Installing trusted CAs...");
	CHECK_QSTATUS();
	
	const Store* pStore = 0;
	status = pCallback_->getCertStore(&pStore);
	CHECK_QSTATUS();
	if (pStore)
		SSL_CTX_set_cert_store(pCTX_,
			static_cast<const StoreImpl*>(pStore)->getStore());
	
	status = log.debug(L"Initializing SSL...");
	CHECK_QSTATUS();
	
	pSSL_ = SSL_new(pCTX_);
	if (!pSSL_)
		return QSTATUS_FAIL;
	
	SSL_set_fd(pSSL_, pSocket->getSocket());
	
	status = log.debug(L"Starting SSL handshake...");
	CHECK_QSTATUS();
	
	if (SSL_connect(pSSL_) <= 0)
		return QSTATUS_FAIL;
	
	status = log.debug(L"SSL handshake completed.");
	CHECK_QSTATUS();
	
	bool bVerified = SSL_get_verify_result(pSSL_) == X509_V_OK;
	if (log.isDebugEnabled()) {
		if (bVerified) {
			status = log.debug(L"Server certificate is verified.");
			CHECK_QSTATUS();
		}
		else {
			status = log.debug(L"Failed to verify server certificate.");
			CHECK_QSTATUS();
		}
	}
	
	X509* pX509 = SSL_get_peer_certificate(pSSL_);
	CertificateImpl cert(pX509);
	status = pCallback_->checkCertificate(cert, bVerified);
	if (status != QSTATUS_SUCCESS) {
		status = log.debug(L"Failed to check server certificate");
		CHECK_QSTATUS();
		return QSTATUS_FAIL;
	}
	
	return QSTATUS_SUCCESS;
}


/****************************************************************************
 *
 * SSLSocketFactoryImpl
 *
 */

SSLSocketFactoryImpl qscrypto::SSLSocketFactoryImpl::factory__;

qscrypto::SSLSocketFactoryImpl::SSLSocketFactoryImpl()
{
	regist(this);
}

qscrypto::SSLSocketFactoryImpl::~SSLSocketFactoryImpl()
{
	unregist(this);
}

QSTATUS qscrypto::SSLSocketFactoryImpl::createSSLSocket(
	Socket* pSocket, bool bDeleteSocket, SSLSocketCallback* pCallback,
	Logger* pLogger, SSLSocket** ppSSLSocket)
{
	assert(pSocket);
	assert(pCallback);
	assert(ppSSLSocket);
	
	DECLARE_QSTATUS();
	
	*ppSSLSocket = 0;
	
	std::auto_ptr<SSLSocketImpl> pSSLSocket;
	status = newQsObject(pSocket, bDeleteSocket, pCallback, pLogger, &pSSLSocket);
	CHECK_QSTATUS();
	
	*ppSSLSocket = pSSLSocket.release();
	
	return QSTATUS_SUCCESS;
}
