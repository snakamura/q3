/*
 * $Id$
 *
 * Copyright(C) 1998-2008 Satoshi Nakamura
 *
 */

#include <qsconv.h>
#include <qslog.h>

#include <openssl/err.h>

#include "crypto.h"
#include "ssl.h"
#include "util.h"

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

void qscrypto::SSLSocketImpl::setTimeout(long nTimeout)
{
	pSocket_->setTimeout(nTimeout);
}

bool qscrypto::SSLSocketImpl::close()
{
	if (!pSocket_)
		return true;
	
	assert(pSSL_);
	
	Log log(pLogger_, L"qscrypto::SSLSocketImpl");
	
	log.debug(L"Shuting down SSL connection...");
	
	SSL_shutdown(pSSL_);
	
	log.debug(L"SSL connection shut down.");
	
	if (bDeleteSocket_) {
		if (!pSocket_->close()) {
			setLastError(pSocket_->getLastError());
			return false;
		}
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
		Util::logError(log, L"Error occurred while receiving.");
		setLastError(SOCKET_ERROR_RECV);
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
		Util::logError(log, L"Error occurred while sending.");
		setLastError(SOCKET_ERROR_SEND);
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
	if (nSelect & SELECT_READ && SSL_pending(pSSL_) > 0) {
		return SELECT_READ;
	}
	else {
		int n = pSocket_->select(nSelect, nTimeout);
		if (n == -1)
			setLastError(pSocket_->getLastError());
		return n;
	}
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
	if (!pCTX_) {
		Util::logError(log, L"Error occurred while creating SSL context.");
		setLastError(SOCKET_ERROR_CONNECT);
		return false;
	}
	
	log.debug(L"Installing trusted CAs...");
	
	const Store* pStore = pCallback_->getCertStore();
	if (pStore)
		SSL_CTX_set_cert_store(pCTX_,
			static_cast<const StoreImpl*>(pStore)->getStore());
	
	log.debug(L"Initializing SSL...");
	
	pSSL_ = SSL_new(pCTX_);
	if (!pSSL_) {
		Util::logError(log, L"Error occurred while create SSL.");
		setLastError(SOCKET_ERROR_CONNECT);
		return false;
	}
	
	SSL_set_fd(pSSL_, static_cast<int>(pSocket->getSocket()));
	
	log.debug(L"Starting SSL handshake...");
	
	if (SSL_connect(pSSL_) <= 0) {
		Util::logError(log, L"Error occurred while connecting.");
		setLastError(SOCKET_ERROR_CONNECT);
		return false;
	}
	
	log.debug(L"SSL handshake completed.");
	
	long nVerify = SSL_get_verify_result(pSSL_);
	bool bVerified = nVerify == X509_V_OK;
	wstring_ptr wstrVerifyError;
	if (bVerified) {
		log.debug(L"Server certificate is verified.");
	}
	else {
		const char* p = X509_verify_cert_error_string(nVerify);
		log.warn(L"Failed to verify server certificate:",
			reinterpret_cast<const unsigned char*>(p), strlen(p));
		wstrVerifyError = mbs2wcs(p);
	}
	
	X509Ptr pX509(SSL_get_peer_certificate(pSSL_));
	CertificateImpl cert(pX509.get(), false);
	if (log.isDebugEnabled()) {
		STACK_OF(X509)* pStackCert = SSL_get_peer_cert_chain(pSSL_);
		if (pStackCert) {
			StringBuffer<WSTRING> buf(L"Peer certificate chain:\n");
			for (int n = 0; n < sk_X509_num(pStackCert); ++n) {
				X509* p = sk_X509_value(pStackCert, n);
				wstring_ptr wstrCert(CertificateImpl(p, false).getText());
				buf.append(wstrCert.get());
			}
			log.debug(buf.getCharArray());
		}
	}
	
	if (!pCallback_->checkCertificate(cert, bVerified, wstrVerifyError.get())) {
		log.error(L"Failed to check server certificate");
		setLastError(SOCKET_ERROR_CONNECT);
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
		return std::auto_ptr<SSLSocket>(0);
	return pSSLSocket;
}
