/*
 * $Id$
 *
 * Copyright(C) 1998-2006 Satoshi Nakamura
 * All rights reserved.
 *
 */

#include <qsassert.h>
#include <qsconv.h>
#include <qslog.h>
#include <qssocket.h>
#include <qsstring.h>

#include "netresource.h"

using namespace qs;


/****************************************************************************
 *
 * Winsock
 *
 */

qs::Winsock::Winsock()
{
	WSADATA wsadata;
	WSAStartup(MAKEWORD(2, 0), &wsadata);
}

qs::Winsock::~Winsock()
{
	WSACleanup();
}


/****************************************************************************
 *
 * SocketBase
 *
 */

qs::SocketBase::~SocketBase()
{
}

wstring_ptr qs::SocketBase::getErrorDescription(Error error)
{
	struct {
		Error error_;
		UINT nId_;
	} errors[] = {
		{ SOCKET_ERROR_SOCKET,			IDS_ERROR_SOCKET_SOCKET			},
		{ SOCKET_ERROR_CLOSESOCKET,		IDS_ERROR_SOCKET_CLOSESOCKET	},
		{ SOCKET_ERROR_LOOKUPNAME,		IDS_ERROR_SOCKET_LOOKUPNAME		},
		{ SOCKET_ERROR_CONNECT,			IDS_ERROR_SOCKET_CONNECT		},
		{ SOCKET_ERROR_CONNECTTIMEOUT,	IDS_ERROR_SOCKET_CONNECTTIMEOUT	},
		{ SOCKET_ERROR_RECV,			IDS_ERROR_SOCKET_RECV			},
		{ SOCKET_ERROR_RECVTIMEOUT,		IDS_ERROR_SOCKET_RECVTIMEOUT	},
		{ SOCKET_ERROR_SEND,			IDS_ERROR_SOCKET_SEND			},
		{ SOCKET_ERROR_SENDTIMEOUT,		IDS_ERROR_SOCKET_SENDTIMEOUT	},
		{ SOCKET_ERROR_SELECT,			IDS_ERROR_SOCKET_SELECT			},
		{ SOCKET_ERROR_CANCEL,			IDS_ERROR_SOCKET_CANCEL			},
		{ SOCKET_ERROR_UNKNOWN,			IDS_ERROR_SOCKET_UNKNOWN		}
	};
	for (int n = 0; n < countof(errors); ++n) {
		if (error == errors[n].error_)
			return loadString(getResourceDllInstanceHandle(), errors[n].nId_);
	}
	return 0;
}


/****************************************************************************
 *
 * SocketImpl
 *
 */

struct qs::SocketImpl
{
	bool init(SOCKET socket,
			  long nTimeout,
			  SocketCallback* pSocketCallback,
			  Logger* pLogger);
	bool connect(const WCHAR* pwszHost,
				 short nPort);
	int select(int nSelect,
			   long nTimeout);
	
	SOCKET socket_;
	long nTimeout_;
	SocketCallback* pSocketCallback_;
	Logger* pLogger_;
	unsigned int nError_;
	bool bDebug_;
	
	SocketInputStream* pInputStream_;
	SocketOutputStream* pOutputStream_;
};

bool qs::SocketImpl::init(SOCKET socket,
						  long nTimeout,
						  SocketCallback* pSocketCallback,
						  Logger* pLogger)
{
	socket_ = socket;
	nTimeout_ = nTimeout;
	pSocketCallback_ = pSocketCallback;
	pLogger_ = pLogger;
	nError_ = Socket::SOCKET_ERROR_SUCCESS;
	bDebug_ = false;
	pInputStream_ = 0;
	pOutputStream_ = 0;
	
	if (!socket_) {
		if (pSocketCallback_)
			pSocketCallback_->initialize();
		
		SOCKET s = ::socket(PF_INET, SOCK_STREAM, 0);
		if (s == INVALID_SOCKET)
			return false;
		socket_ = s;
	}
	
	return true;
}

bool qs::SocketImpl::connect(const WCHAR* pwszHost,
							 short nPort)
{
	assert(pwszHost);
	
	Log log(pLogger_, L"qs::Socket");
	
	if (nPort != 1) {
		sockaddr_in sockAddr;
		::memset(&sockAddr, 0, sizeof(sockAddr));
		sockAddr.sin_family = AF_INET;
		sockAddr.sin_port = ::htons(nPort);
		
		string_ptr strHost(wcs2mbs(pwszHost));
		unsigned long nAddr = inet_addr(strHost.get());
		if (nAddr != INADDR_NONE) {
			sockAddr.sin_addr.s_addr = nAddr;
		}
		else {
			if (pSocketCallback_)
				pSocketCallback_->lookup();
			
			if (log.isDebugEnabled()) {
				wstring_ptr wstrLog(concat(L"Looking up host: ", pwszHost));
				log.debug(wstrLog.get());
			}
			
			hostent* phe = gethostbyname(strHost.get());
			if (!phe || !phe->h_addr_list[0])
				phe = gethostbyname(strHost.get());
			if (!phe || !phe->h_addr_list[0]) {
				nError_ = Socket::SOCKET_ERROR_LOOKUPNAME;
				return false;
			}
			::memcpy(&sockAddr.sin_addr.s_addr,
				phe->h_addr_list[0], phe->h_length);
		}
		
		if (pSocketCallback_)
			pSocketCallback_->connecting();
		
		struct BlockMode
		{
			BlockMode(SOCKET s, bool b) :
				s_(s),
				b_(b)
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
		} blockMode(socket_, pSocketCallback_ != 0);
		
		log.debug(L"Connecting...");
		
		if (::connect(socket_, reinterpret_cast<sockaddr*>(&sockAddr), sizeof(sockAddr))) {
			bool bConnect = false;
			nError_ = Socket::SOCKET_ERROR_SUCCESS;
			if (::WSAGetLastError() == WSAEWOULDBLOCK) {
				int n = 0;
				while (n < nTimeout_) {
					assert(pSocketCallback_);
					if (pSocketCallback_->isCanceled(false)) {
						log.debug(L"Connection canceled.");
						nError_ = Socket::SOCKET_ERROR_CANCEL;
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
					++n;
				}
				if (n == nTimeout_) {
					log.debug(L"Connection timeout.");
					nError_ = Socket::SOCKET_ERROR_CONNECTTIMEOUT;
				}
			}
			else {
				nError_ = Socket::SOCKET_ERROR_CONNECT;
			}
			if (!bConnect)
				return false;
		}
		if (pSocketCallback_)
			pSocketCallback_->connected();
		
		log.debug(L"Connected.");
	}
	else {
#ifndef _WIN32_WCE
		::AllocConsole();
#endif
		bDebug_ = true;
	}
	
	return true;
}

int qs::SocketImpl::select(int nSelect,
						   long nTimeout)
{
	int nSelected = 0;
	
	if (!bDebug_) {
		int nSelects[] = {
			Socket::SELECT_READ,
			Socket::SELECT_WRITE,
			Socket::SELECT_EXCEPT
		};
		fd_set fdset[3];
		for (int n = 0; n < 3; ++n) {
			FD_ZERO(&fdset[n]);
			if (nSelect & nSelects[n])
				FD_SET(socket_, &fdset[n]);
		}
		timeval tvTimeout = { nTimeout, 0 };
		int nRet = ::select(0,
			nSelect & Socket::SELECT_READ ? &fdset[0] : 0,
			nSelect & Socket::SELECT_WRITE ? &fdset[1] : 0,
			nSelect & Socket::SELECT_EXCEPT ? &fdset[2] : 0,
			&tvTimeout);
		if (nRet == SOCKET_ERROR) {
			nError_ = Socket::SOCKET_ERROR_SELECT;
			return -1;
		}
		else if (nRet != 0) {
			for (int n = 0; n < 3; ++n) {
				if (FD_ISSET(socket_, &fdset[n]))
					nSelected |= nSelects[n];
			}
		}
	}
	else {
		if (nSelect & Socket::SELECT_WRITE)
			nSelected = Socket::SELECT_WRITE;
		else if (nSelect & Socket::SELECT_READ)
			nSelected = Socket::SELECT_READ;
	}
	
	return nSelected;
}


/****************************************************************************
 *
 * Socket
 *
 */

qs::Socket::Socket(long nTimeout,
				   SocketCallback* pSocketCallback,
				   Logger* pLogger) :
	pImpl_(0)
{
	std::auto_ptr<SocketImpl> pImpl(new SocketImpl());
	if (!pImpl->init(0, nTimeout, pSocketCallback, pLogger))
		return;
	pImpl_ = pImpl.release();
}

qs::Socket::Socket(SOCKET socket,
				   long nTimeout,
				   SocketCallback* pSocketCallback,
				   Logger* pLogger) :
	pImpl_(0)
{
	std::auto_ptr<SocketImpl> pImpl(new SocketImpl());
	if (!pImpl->init(socket, nTimeout, pSocketCallback, pLogger))
		return;
	pImpl_ = pImpl.release();
}

qs::Socket::Socket(const WCHAR* pwszHost,
				   short nPort,
				   long nTimeout,
				   SocketCallback* pSocketCallback,
				   Logger* pLogger) :
	pImpl_(0)
{
	std::auto_ptr<SocketImpl> pImpl(new SocketImpl());
	if (!pImpl->init(0, nTimeout, pSocketCallback, pLogger))
		return;
	if (!pImpl->connect(pwszHost, nPort))
		return;
	pImpl_ = pImpl.release();
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

bool qs::Socket::operator!() const
{
	return pImpl_ == 0;
}

SOCKET qs::Socket::getSocket() const
{
	return pImpl_->socket_;
}

bool qs::Socket::connect(const WCHAR* pwszHost,
						 short nPort)
{
	return pImpl_->connect(pwszHost, nPort);
}

long qs::Socket::getTimeout() const
{
	return pImpl_->nTimeout_;
}

void qs::Socket::setTimeout(long nTimeout)
{
	pImpl_->nTimeout_ = nTimeout;
}

unsigned int qs::Socket::getLastError() const
{
	return pImpl_ ? pImpl_->nError_ : SOCKET_ERROR_UNKNOWN;
}

void qs::Socket::setLastError(unsigned int nError)
{
	pImpl_->nError_ = nError;
}

bool qs::Socket::close()
{
	Log log(pImpl_->pLogger_, L"qs::Socket");
	
	if (!pImpl_->bDebug_) {
		if (pImpl_->socket_) {
			log.debug(L"Closing socket...");
			
			SOCKET s = pImpl_->socket_;
			pImpl_->socket_ = 0;
			
			if (::closesocket(s)) {
				pImpl_->nError_ = SOCKET_ERROR_CLOSESOCKET;
				return false;
			}
			
			log.debug(L"Closed socket.");
		}
	}
	else {
#ifndef _WIN32_WCE
		::FreeConsole();
#endif
	}
	
	return true;
}

int qs::Socket::recv(char* p,
					 int nLen,
					 int nFlags)
{
	Log log(pImpl_->pLogger_, L"qs::Socket");
	
	int nRecv = -1;
	if (!pImpl_->bDebug_) {
		nRecv = ::recv(pImpl_->socket_, p, nLen, nFlags);
		if (nRecv == SOCKET_ERROR) {
			log.debug(L"Error occured while receiving.");
			pImpl_->nError_ = SOCKET_ERROR_RECV;
			return -1;
		}
		
		log.debug(L"Received data", reinterpret_cast<unsigned char*>(p), nRecv);
	}
	else {
#ifndef _WIN32_WCE
		DWORD dw = 0;
		if (!::ReadFile(::GetStdHandle(STD_INPUT_HANDLE), p, nLen, &dw, 0))
			return -1;
		nRecv = dw;
#endif
	}
	
	return nRecv;
}

int qs::Socket::send(const char* p,
					 int nLen,
					 int nFlags)
{
	Log log(pImpl_->pLogger_, L"qs::Socket");
	
	int nSent = -1;
	if (!pImpl_->bDebug_) {
		nSent = ::send(pImpl_->socket_, p, nLen, nFlags);
		if (nSent == SOCKET_ERROR) {
			log.debug(L"Error occured while sending.");
			pImpl_->nError_ = SOCKET_ERROR_SEND;
			return -1;
		}
		
		log.debug(L"Sent data", reinterpret_cast<const unsigned char*>(p), nSent);
	}
	else {
#ifndef _WIN32_WCE
		DWORD dw = 0;
		if (!::WriteFile(::GetStdHandle(STD_OUTPUT_HANDLE), p, nLen, &dw, 0))
			return -1;
		nSent = dw;
#endif
	}
	
	return nSent;
}

int qs::Socket::select(int nSelect)
{
	return select(nSelect, getTimeout());
}

int qs::Socket::select(int nSelect,
					   long nTimeout)
{
	Log log(pImpl_->pLogger_, L"qs::Socket");
	
	if (pImpl_->pSocketCallback_) {
		do {
			if (pImpl_->pSocketCallback_->isCanceled(true)) {
				log.debug(L"Canceled while selecting.");
				pImpl_->nError_ = SOCKET_ERROR_CANCEL;
				return -1;
			}
			
			int n = pImpl_->select(nSelect, nTimeout == 0 ? 0 : 1);
			if (n != 0)
				return n;
			
			--nTimeout;
		} while (nTimeout > 0);
		
		return 0;
	}
	else {
		return pImpl_->select(nSelect, nTimeout);
	}
}

InputStream* qs::Socket::getInputStream()
{
	if (!pImpl_->pInputStream_)
		pImpl_->pInputStream_ = new SocketInputStream(this);
	return pImpl_->pInputStream_;
}

OutputStream* qs::Socket::getOutputStream()
{
	if (!pImpl_->pOutputStream_)
		pImpl_->pOutputStream_ = new SocketOutputStream(this);
	return pImpl_->pOutputStream_;
}


/****************************************************************************
 *
 * ServerSocketImpl
 *
 */

struct qs::ServerSocketImpl
{
	bool bind(short nPort);
	bool listen(int nBackLog);
	
	SOCKET socket_;
};

bool qs::ServerSocketImpl::bind(short nPort)
{
	int b = 1;
	setsockopt(socket_, SOL_SOCKET, SO_REUSEADDR,
		reinterpret_cast<char*>(&b), sizeof(b));
	sockaddr_in sockAddr;
	memset(&sockAddr, 0, sizeof(sockAddr));
	sockAddr.sin_family = AF_INET;
	sockAddr.sin_port = htons(nPort);
	sockAddr.sin_addr.s_addr = htonl(INADDR_ANY);
	return ::bind(socket_, reinterpret_cast<sockaddr*>(&sockAddr), sizeof(sockAddr)) == 0;
}

bool qs::ServerSocketImpl::listen(int nBackLog)
{
	return ::listen(socket_, nBackLog) == 0;
}


/****************************************************************************
 *
 * ServerSocket
 *
 */

qs::ServerSocket::ServerSocket(short nPort,
							   int nBackLog) :
	pImpl_(0)
{
	std::auto_ptr<ServerSocketImpl> pImpl(new ServerSocketImpl());
	pImpl->socket_ = 0;
	
	SOCKET s = socket(PF_INET, SOCK_STREAM, 0);
	if (s == INVALID_SOCKET)
		return;
	pImpl->socket_ = s;
	
	if (!pImpl->bind(nPort))
		return;
	if (!pImpl->listen(nBackLog))
		return;
	
	pImpl_ = pImpl.release();
}

qs::ServerSocket::~ServerSocket()
{
	if (pImpl_) {
		close();
		
		delete pImpl_;
		pImpl_ = 0;
	}
}

bool qs::ServerSocket::operator!() const
{
	return pImpl_ == 0;
}

bool qs::ServerSocket::close()
{
	if (pImpl_->socket_) {
		if (::closesocket(pImpl_->socket_))
			return false;
	}
	return true;
}

SOCKET qs::ServerSocket::accept()
{
	return ::accept(pImpl_->socket_, 0, 0);
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

qs::SocketInputStream::SocketInputStream(SocketBase* pSocket) :
	pSocket_(pSocket)
{
}

qs::SocketInputStream::~SocketInputStream()
{
}

bool qs::SocketInputStream::close()
{
	return pSocket_->close();
}

size_t qs::SocketInputStream::read(unsigned char* p,
								   size_t nRead)
{
	assert(p);
	
	if (nRead == 0)
		return 0;
	
	size_t nSize = 0;
	
	while (nRead != 0) {
		long n = 0;
		while (n < pSocket_->getTimeout()) {
			// TODO
			// Check cancel
			int nSelect = pSocket_->select(Socket::SELECT_READ, 1);
			if (nSelect == -1)
				return -1;
			if (nSelect & Socket::SELECT_READ)
				break;
			++n;
		}
		if (n == pSocket_->getTimeout()) {
			pSocket_->setLastError(Socket::SOCKET_ERROR_RECVTIMEOUT);
			return nSize;
		}
		
		int nLen = pSocket_->recv(reinterpret_cast<char*>(p),
			static_cast<int>(nRead), 0);
		if (nLen == -1)
			return -1;
		else if (nLen == 0)
			break;
		
		nSize += nLen;
		nRead -= nLen;
		p += nLen;
	}
	
	return nSize;
}


/****************************************************************************
 *
 * SocketOutputStream
 *
 */

qs::SocketOutputStream::SocketOutputStream(SocketBase* pSocket) :
	pSocket_(pSocket)
{
}

SocketOutputStream::~SocketOutputStream()
{
}

bool SocketOutputStream::close()
{
	return pSocket_->close();
}

size_t SocketOutputStream::write(const unsigned char* p,
								 size_t nWrite)
{
	assert(p);
	
	if (nWrite == 0)
		return 0;
	
	size_t nSize = 0;
	
	while (nWrite != 0) {
		long n = 0;
		while (n < pSocket_->getTimeout()) {
			// TODO
			// Check cancel
			int nSelect = pSocket_->select(Socket::SELECT_WRITE, 1);
			if (nSelect == -1)
				return -1;
			if (nSelect & Socket::SELECT_WRITE)
				break;
			++n;
		}
		if (n == pSocket_->getTimeout()) {
			pSocket_->setLastError(Socket::SOCKET_ERROR_SENDTIMEOUT);
			return nSize;
		}
		
		int nLen = pSocket_->send(reinterpret_cast<const char*>(p),
			static_cast<int>(nWrite), 0);
		if (nLen == -1)
			return -1;
		nSize += nLen;
		nWrite -= nLen;
		p += nLen;
	}
	
	return nSize;
}

bool SocketOutputStream::flush()
{
	return true;
}
