/*
 * $Id: socket.cpp,v 1.1.1.1 2003/04/29 08:07:35 snakamura Exp $
 *
 * Copyright(C) 1998-2003 Satoshi Nakamura
 * All rights reserved.
 *
 */

#include <qsassert.h>
#include <qsconv.h>
#include <qserror.h>
#include <qslog.h>
#include <qsnew.h>
#include <qssocket.h>
#include <qsstring.h>

#ifdef _WIN32_WCE
#	include <sslsock.h>
#endif

#include "socket.h"

using namespace qs;


/****************************************************************************
 *
 * Winsock
 *
 */

qs::Winsock::Winsock(QSTATUS* pstatus) :
	status_(QSTATUS_SUCCESS)
{
	assert(pstatus);
	
	WSADATA wsadata;
	if (WSAStartup(MAKEWORD(2, 0), &wsadata))
		status_ = QSTATUS_FAIL;
	
	*pstatus = status_;
}

qs::Winsock::~Winsock()
{
	if (status_ == QSTATUS_SUCCESS)
		WSACleanup();
}


/****************************************************************************
 *
 * SocketImpl
 *
 */

int CALLBACK sslValidateCertProc(DWORD, LPVOID, DWORD, LPBLOB, DWORD);

struct qs::SocketImpl
{
	QSTATUS init(SOCKET socket, const Socket::Option& option);
	QSTATUS connect(const WCHAR* pwszHost, short nPort);
	
	SOCKET socket_;
	long nTimeout_;
	bool bSsl_;
	SocketCallback* pSocketCallback_;
	Logger* pLogger_;
	Socket::Error error_;
	bool bDebug_;
	
	SocketInputStream* pInputStream_;
	SocketOutputStream* pOutputStream_;
};

QSTATUS qs::SocketImpl::init(SOCKET socket, const Socket::Option& option)
{
	DECLARE_QSTATUS();
	
	socket_ = socket;
	nTimeout_ = option.nTimeout_;
	bSsl_ = option.bSsl_;
	pSocketCallback_ = option.pSocketCallback_;
	pLogger_ = option.pLogger_;
	error_ = Socket::SOCKET_ERROR_SUCCESS;
	bDebug_ = false;
	pInputStream_ = 0;
	pOutputStream_ = 0;
	
	if (!socket_) {
		if (pSocketCallback_) {
			status = pSocketCallback_->initialize();
			CHECK_QSTATUS();
		}
		
		SOCKET s = ::socket(PF_INET, SOCK_STREAM, 0);
		if (s == INVALID_SOCKET)
			return QSTATUS_FAIL;
		socket_ = s;
	}
	
	return QSTATUS_SUCCESS;
}

QSTATUS qs::SocketImpl::connect(const WCHAR* pwszHost, short nPort)
{
	assert(pwszHost);
	
	DECLARE_QSTATUS();
	
	Log log(pLogger_, L"qs::Socket");
	
	if (nPort != 1) {
#ifdef _WIN32_WCE
		if (bSsl_) {
			DWORD dwOpt = SO_SEC_SSL;
			if (::setsockopt(socket_, SOL_SOCKET, SO_SECURE,
				reinterpret_cast<const char*>(&dwOpt), sizeof(dwOpt))) {
				error_ = Socket::SOCKET_ERROR_SETSSL;
				return QSTATUS_FAIL;
			}
			DWORD dwOut = 0;
			DWORD dwSize = 0;
			SSLVALIDATECERTHOOK svch = { sslValidateCertProc, 0 };
			if (::WSAIoctl(socket_, SO_SSL_SET_VALIDATE_CERT_HOOK, &svch,
				sizeof(svch), &dwOut, sizeof(dwOut), &dwSize, 0, 0)) {
				error_ = Socket::SOCKET_ERROR_SETSSL;
				return QSTATUS_FAIL;
			}
		}
#endif
		
		sockaddr_in sockAddr;
		::memset(&sockAddr, 0, sizeof(sockAddr));
		sockAddr.sin_family = AF_INET;
		sockAddr.sin_port = ::htons(nPort);
		
		string_ptr<STRING> strHost(wcs2mbs(pwszHost));
		if (!strHost.get())
			return QSTATUS_OUTOFMEMORY;
		unsigned long nAddr = inet_addr(strHost.get());
		if (nAddr != INADDR_NONE) {
			sockAddr.sin_addr.s_addr = nAddr;
		}
		else {
			if (pSocketCallback_)
				pSocketCallback_->lookup();
			
			if (log.isDebugEnabled()) {
				string_ptr<WSTRING> wstrLog(
					concat(L"Looking up host: ", pwszHost));
				if (!wstrLog.get())
					return QSTATUS_OUTOFMEMORY;
				status = log.debug(wstrLog.get());
				CHECK_QSTATUS();
			}
			
			hostent* phe = gethostbyname(strHost.get());
			if (!phe || !phe->h_addr_list[0])
				phe = gethostbyname(strHost.get());
			if (!phe || !phe->h_addr_list[0]) {
				error_ = Socket::SOCKET_ERROR_LOOKUPNAME;
				return QSTATUS_FAIL;
			}
			::memcpy(&sockAddr.sin_addr.s_addr,
				phe->h_addr_list[0], phe->h_length);
		}
		
		if (pSocketCallback_)
			pSocketCallback_->connecting();
		
		struct BlockMode
		{
			BlockMode(SOCKET s, bool b) : s_(s), b_(b)
			{
				if (b_) {
					unsigned long lBlock = 1;
					::ioctlsocket(s_, FIONBIO, &lBlock);
				}
			}
			~BlockMode()
			{
				if (b_) {
					unsigned long lBlock = 0;
					::ioctlsocket(s_, FIONBIO, &lBlock);
				}
			}
			
			SOCKET s_;
			bool b_;
		} blockMode(socket_, pSocketCallback_ && !bSsl_);
		
		status = log.debug(L"Connecting...");
		CHECK_QSTATUS();
		
		if (::connect(socket_, reinterpret_cast<sockaddr*>(&sockAddr),
			sizeof(sockAddr))) {
			bool bConnect = false;
			error_ = Socket::SOCKET_ERROR_SUCCESS;
			if (::WSAGetLastError() == WSAEWOULDBLOCK) {
				for (int n = 0; n < nTimeout_; ++n) {
					assert(pSocketCallback_);
					if (pSocketCallback_->isCanceled(false)) {
						status = log.debug(L"Connection canceled.");
						CHECK_QSTATUS();
						error_ = Socket::SOCKET_ERROR_CANCEL;
						break;
					}
					fd_set fdset;
					FD_ZERO(&fdset);
					FD_SET(socket_, &fdset);
					timeval tvTimeout = { 1, 0 };
					if (::select(0, 0, &fdset, 0, &tvTimeout) == 1) {
						bConnect = true;
						break;
					}
				}
				if (n == nTimeout_) {
					status = log.debug(L"Connection timeout.");
					CHECK_QSTATUS();
					error_ = Socket::SOCKET_ERROR_CONNECTTIMEOUT;
				}
			}
			else {
				error_ = Socket::SOCKET_ERROR_CONNECT;
			}
			if (!bConnect)
				return QSTATUS_FAIL;
		}
		if (pSocketCallback_)
			pSocketCallback_->connected();
		
		status = log.debug(L"Connected.");
		CHECK_QSTATUS();
	}
	else {
#ifndef _WIN32_WCE
		::AllocConsole();
#endif
		bDebug_ = true;
	}
	
	return QSTATUS_SUCCESS;
}

#ifdef _WIN32_WCE
int CALLBACK sslValidateCertProc(DWORD dwType, LPVOID pvArg,
	DWORD dwChainLen, LPBLOB pCertChain, DWORD dwFalgs)
{
	return SSL_ERR_OKAY;
}
#endif


/****************************************************************************
 *
 * Socket
 *
 */

qs::Socket::Socket(const Option& option, QSTATUS* pstatus) :
	pImpl_(0)
{
	assert(pstatus);
	
	*pstatus = QSTATUS_SUCCESS;
	
	DECLARE_QSTATUS();
	
	status = newObject(&pImpl_);
	CHECK_QSTATUS_SET(pstatus);
	status = pImpl_->init(0, option);
	CHECK_QSTATUS_SET(pstatus);
}

qs::Socket::Socket(SOCKET socket, const Option& option, QSTATUS* pstatus) :
	pImpl_(0)
{
	assert(pstatus);
	
	*pstatus = QSTATUS_SUCCESS;
	
	DECLARE_QSTATUS();
	
	status = newObject(&pImpl_);
	CHECK_QSTATUS_SET(pstatus);
	status = pImpl_->init(socket, option);
	CHECK_QSTATUS_SET(pstatus);
}

qs::Socket::Socket(const WCHAR* pwszHost, short nPort,
	const Option& option, QSTATUS* pstatus) :
	pImpl_(0)
{
	assert(pstatus);
	
	*pstatus = QSTATUS_SUCCESS;
	
	DECLARE_QSTATUS();
	
	status = newObject(&pImpl_);
	CHECK_QSTATUS_SET(pstatus);
	status = pImpl_->init(0, option);
	CHECK_QSTATUS_SET(pstatus);
	
	status = pImpl_->connect(pwszHost, nPort);
	CHECK_QSTATUS_SET(pstatus);
}

qs::Socket::~Socket()
{
	if (pImpl_) {
		close();
		
		delete pImpl_->pInputStream_;
		delete pImpl_->pOutputStream_;
		
		delete pImpl_;
		pImpl_ = 0;
	}
}

SOCKET qs::Socket::getSocket() const
{
	return pImpl_->socket_;
}

long qs::Socket::getTimeout() const
{
	return pImpl_->nTimeout_;
}

QSTATUS qs::Socket::connect(const WCHAR* pwszHost, short nPort)
{
	return pImpl_->connect(pwszHost, nPort);
}

QSTATUS qs::Socket::close()
{
	DECLARE_QSTATUS();
	
	Log log(pImpl_->pLogger_, L"qs::Socket");
	
	if (!pImpl_->bDebug_) {
		if (pImpl_->socket_) {
			status = log.debug(L"Closing socket...");
			CHECK_QSTATUS();
			
			if (::closesocket(pImpl_->socket_)) {
				pImpl_->error_ = SOCKET_ERROR_CLOSESOCKET;
				return QSTATUS_FAIL;
			}
			
			status = log.debug(L"Closed socket.");
			CHECK_QSTATUS();
		}
	}
	else {
#ifndef _WIN32_WCE
		::FreeConsole();
#endif
	}
	return QSTATUS_SUCCESS;
}

QSTATUS qs::Socket::recv(char* p, int* pnLen, int nFlags)
{
	DECLARE_QSTATUS();
	
	Log log(pImpl_->pLogger_, L"qs::Socket");
	
	if (!pImpl_->bDebug_) {
		*pnLen = ::recv(pImpl_->socket_, p, *pnLen, nFlags);
		if (*pnLen == SOCKET_ERROR) {
			status = log.debug(L"Error occured while receiving.");
			CHECK_QSTATUS();
			pImpl_->error_ = SOCKET_ERROR_RECV;
			return QSTATUS_FAIL;
		}
		
		status = log.debug(L"Received data",
			reinterpret_cast<unsigned char*>(p), *pnLen);
		CHECK_QSTATUS();
	}
	else {
#ifndef _WIN32_WCE
		DWORD dw = 0;
		if (!::ReadFile(::GetStdHandle(STD_INPUT_HANDLE), p, *pnLen, &dw, 0))
			return QSTATUS_FAIL;
		*pnLen = dw;
#endif
	}
	return QSTATUS_SUCCESS;
}

QSTATUS qs::Socket::send(const char* p, int* pnLen, int nFlags)
{
	DECLARE_QSTATUS();
	
	Log log(pImpl_->pLogger_, L"qs::Socket");
	
	if (!pImpl_->bDebug_) {
		*pnLen = ::send(pImpl_->socket_, p, *pnLen, nFlags);
		if (*pnLen == SOCKET_ERROR) {
			status = log.debug(L"Error occured while sending.");
			CHECK_QSTATUS();
			pImpl_->error_ = SOCKET_ERROR_SEND;
			return QSTATUS_FAIL;
		}
		
		status = log.debug(L"Sent data",
			reinterpret_cast<const unsigned char*>(p), *pnLen);
		CHECK_QSTATUS();
	}
	else {
#ifndef _WIN32_WCE
		DWORD dw = 0;
		if (!::WriteFile(::GetStdHandle(STD_OUTPUT_HANDLE), p, *pnLen, &dw, 0))
			return QSTATUS_FAIL;
		*pnLen = dw;
#endif
	}
	return QSTATUS_SUCCESS;
}

QSTATUS qs::Socket::select(int* pnSelect)
{
	return select(pnSelect, getTimeout());
}

QSTATUS qs::Socket::select(int* pnSelect, long nTimeout)
{
	if (!pImpl_->bDebug_) {
		int nSelects[] = { SELECT_READ, SELECT_WRITE, SELECT_EXCEPT };
		fd_set fdset[3];
		for (int n = 0; n < 3; ++n) {
			FD_ZERO(&fdset[n]);
			if (*pnSelect & nSelects[n])
				FD_SET(pImpl_->socket_, &fdset[n]);
		}
		timeval tvTimeout = { nTimeout, 0 };
		int nRet = ::select(pImpl_->socket_,
			*pnSelect & SELECT_READ ? &fdset[0] : 0,
			*pnSelect & SELECT_WRITE ? &fdset[1] : 0,
			*pnSelect & SELECT_EXCEPT ? &fdset[2] : 0,
			&tvTimeout);
		*pnSelect = 0;
		if (nRet == SOCKET_ERROR) {
			return QSTATUS_FAIL;
		}
		else if (nRet != 0) {
			for (int n = 0; n < 3; ++n) {
				if (FD_ISSET(pImpl_->socket_, &fdset[n]))
					*pnSelect |= nSelects[n];
			}
		}
	}
	else {
		if (*pnSelect & SELECT_WRITE)
			*pnSelect = SELECT_WRITE;
		else if (*pnSelect & SELECT_READ)
			*pnSelect = SELECT_READ;
	}
	return QSTATUS_SUCCESS;
}

QSTATUS qs::Socket::getInputStream(InputStream** ppStream)
{
	DECLARE_QSTATUS();
	
	if (!pImpl_->pInputStream_) {
		status = newQsObject(this, &pImpl_->pInputStream_);
		CHECK_QSTATUS();
	}
	*ppStream = pImpl_->pInputStream_;
	
	return QSTATUS_SUCCESS;
}

QSTATUS qs::Socket::getOutputStream(OutputStream** ppStream)
{
	DECLARE_QSTATUS();
	
	if (!pImpl_->pOutputStream_) {
		status = newQsObject(this, &pImpl_->pOutputStream_);
		CHECK_QSTATUS();
	}
	*ppStream = pImpl_->pOutputStream_;
	
	return QSTATUS_SUCCESS;
}

qs::Socket::Error qs::Socket::getLastError() const
{
	return pImpl_ ? pImpl_->error_ : SOCKET_ERROR_UNKNOWN;
}

void qs::Socket::setLastError(Error error)
{
	pImpl_->error_ = error;
}


/****************************************************************************
 *
 * ServerSocketImpl
 *
 */

struct qs::ServerSocketImpl
{
	QSTATUS bind(short nPort);
	QSTATUS listen(int nBackLog);
	
	SOCKET socket_;
};

QSTATUS qs::ServerSocketImpl::bind(short nPort)
{
	int b = 1;
	setsockopt(socket_, SOL_SOCKET, SO_REUSEADDR,
		reinterpret_cast<char*>(&b), sizeof(b));
	sockaddr_in sockAddr;
	memset(&sockAddr, 0, sizeof(sockAddr));
	sockAddr.sin_family = AF_INET;
	sockAddr.sin_port = htons(nPort);
	sockAddr.sin_addr.s_addr = htonl(INADDR_ANY);
	return ::bind(socket_, reinterpret_cast<sockaddr*>(&sockAddr),
		sizeof(sockAddr)) == 0 ? QSTATUS_SUCCESS : QSTATUS_FAIL;
}

QSTATUS qs::ServerSocketImpl::listen(int nBackLog)
{
	return ::listen(socket_, nBackLog) == 0 ? QSTATUS_SUCCESS : QSTATUS_FAIL;
}


/****************************************************************************
 *
 * ServerSocket
 *
 */

qs::ServerSocket::ServerSocket(short nPort, int nBackLog, QSTATUS* pstatus) :
	pImpl_(0)
{
	assert(pstatus);
	
	*pstatus = QSTATUS_SUCCESS;
	
	DECLARE_QSTATUS();
	
	status = newObject(&pImpl_);
	CHECK_QSTATUS_SET(pstatus);
	pImpl_->socket_ = 0;
	
	SOCKET s = socket(PF_INET, SOCK_STREAM, 0);
	if (s == INVALID_SOCKET) {
		*pstatus = QSTATUS_FAIL;
		return;
	}
	pImpl_->socket_ = s;
	
	status = pImpl_->bind(nPort);
	CHECK_QSTATUS_SET(pstatus);
	
	status = pImpl_->listen(nBackLog);
	CHECK_QSTATUS_SET(pstatus);
}

qs::ServerSocket::~ServerSocket()
{
	if (pImpl_) {
		close();
		
		delete pImpl_;
		pImpl_ = 0;
	}
}

QSTATUS qs::ServerSocket::close()
{
	if (pImpl_->socket_) {
		if (::closesocket(pImpl_->socket_))
			return QSTATUS_FAIL;
	}
	return QSTATUS_SUCCESS;
}

QSTATUS qs::ServerSocket::accept(SOCKET* pSocket)
{
	assert(pSocket);
	*pSocket = ::accept(pImpl_->socket_, 0, 0);
	return *pSocket != INVALID_SOCKET ? QSTATUS_SUCCESS : QSTATUS_FAIL;
}


/****************************************************************************
 *
 * SocketCallback
 *
 */

qs::SocketCallback::~SocketCallback()
{
}


/****************************************************************************
 *
 * SocketInputStream
 *
 */

qs::SocketInputStream::SocketInputStream(Socket* pSocket, QSTATUS* pstatus) :
	pSocket_(pSocket)
{
	assert(pstatus);
	*pstatus = QSTATUS_SUCCESS;
}

qs::SocketInputStream::~SocketInputStream()
{
}

QSTATUS qs::SocketInputStream::close()
{
	return pSocket_->close();
}

QSTATUS qs::SocketInputStream::read(
	unsigned char* p, size_t nRead, size_t* pnRead)
{
	assert(p);
	assert(pnRead);
	
	*pnRead = 0;
	
	if (nRead == 0)
		return QSTATUS_SUCCESS;
	
	DECLARE_QSTATUS();
	
	while (nRead != 0) {
		for (long n = 0; n < pSocket_->getTimeout(); ++n) {
			// TODO
			// Check cancel
			int nSelect = Socket::SELECT_READ;
			status = pSocket_->select(&nSelect, 1);
			CHECK_QSTATUS();
			if (nSelect & Socket::SELECT_READ)
				break;
		}
		if (n == pSocket_->getTimeout()) {
			pSocket_->setLastError(Socket::SOCKET_ERROR_RECVTIMEOUT);
			return QSTATUS_SUCCESS;
		}
		
		int nLen = nRead;
		status = pSocket_->recv(reinterpret_cast<char*>(p), &nLen, 0);
		CHECK_QSTATUS();
		if (nLen == 0) {
			if (*pnRead == 0)
				*pnRead = -1;
			return QSTATUS_SUCCESS;
		}
		else {
			*pnRead += nLen;
			nRead -= nLen;
			p += nLen;
		}
	}
	return QSTATUS_SUCCESS;
}


/****************************************************************************
 *
 * SocketOutputStream
 *
 */

qs::SocketOutputStream::SocketOutputStream(Socket* pSocket, QSTATUS* pstatus) :
	pSocket_(pSocket)
{
	assert(pstatus);
	*pstatus = QSTATUS_SUCCESS;
}

SocketOutputStream::~SocketOutputStream()
{
}

QSTATUS SocketOutputStream::close()
{
	return pSocket_->close();
}

QSTATUS SocketOutputStream::write(const unsigned char* p, size_t nWrite)
{
	assert(p);
	
	if (nWrite == 0)
		return QSTATUS_SUCCESS;
	
	DECLARE_QSTATUS();
	
	while (nWrite != 0) {
		for (long n = 0; n < pSocket_->getTimeout(); ++n) {
			// TODO
			// Check cancel
			int nSelect = Socket::SELECT_WRITE;
			status = pSocket_->select(&nSelect, 1);
			CHECK_QSTATUS();
			if (nSelect & Socket::SELECT_WRITE)
				break;
		}
		if (n == pSocket_->getTimeout()) {
			pSocket_->setLastError(Socket::SOCKET_ERROR_SENDTIMEOUT);
			return QSTATUS_SUCCESS;
		}
		
		int nLen = nWrite;
		status = pSocket_->send(reinterpret_cast<const char*>(p), &nLen, 0);
		CHECK_QSTATUS();
		nWrite -= nLen;
		p += nLen;
	}
	
	return QSTATUS_SUCCESS;
}

QSTATUS SocketOutputStream::flush()
{
	return QSTATUS_SUCCESS;
}
