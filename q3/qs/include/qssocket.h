/*
 * $Id: qssocket.h,v 1.1.1.1 2003/04/29 08:07:35 snakamura Exp $
 *
 * Copyright(C) 1998-2003 Satoshi Nakamura
 * All rights reserved.
 *
 */

#ifndef __QSSOCKET_H__
#define __QSSOCKET_H__

#include <qs.h>

#include <winsock.h>


namespace qs {

class Winsock;
class Socket;
class SocketCallback;

class InputStream;
class Logger;
class OutputStream;


/****************************************************************************
 *
 * Winsock
 *
 */

class QSEXPORTCLASS Winsock
{
public:
	Winsock(QSTATUS* pstatus);
	~Winsock();

private:
	Winsock(const Winsock&);
	Winsock& operator=(const Winsock&);

private:
	QSTATUS status_;
};


/****************************************************************************
 *
 * Socket
 *
 */

class QSEXPORTCLASS Socket
{
public:
	enum Error {
		SOCKET_ERROR_SUCCESS		= 0x00000000,
		SOCKET_ERROR_SOCKET			= 0x01000000,
		SOCKET_ERROR_CLOSESOCKET	= 0x02000000,
		SOCKET_ERROR_SETSSL			= 0x03000000,
		SOCKET_ERROR_LOOKUPNAME		= 0x04000000,
		SOCKET_ERROR_CONNECT		= 0x05000000,
		SOCKET_ERROR_CONNECTTIMEOUT	= 0x06000000,
		SOCKET_ERROR_RECV			= 0x07000000,
		SOCKET_ERROR_RECVTIMEOUT	= 0x08000000,
		SOCKET_ERROR_SEND			= 0x09000000,
		SOCKET_ERROR_SENDTIMEOUT	= 0x0a000000,
		SOCKET_ERROR_CANCEL			= 0x0f000000,
		SOCKET_ERROR_UNKNOWN		= 0x10000000,
		SOCKET_ERROR_MASK_SOCKET	= 0xff000000
	};
	
	enum Select {
		SELECT_READ		= 0x01,
		SELECT_WRITE	= 0x02,
		SELECT_EXCEPT	= 0x04,
	};

public:
	struct Option
	{
		long nTimeout_;
		bool bSsl_;
		SocketCallback* pSocketCallback_;
		Logger* pLogger_;
	};

public:
	Socket(const Option& option, QSTATUS* pstatus);
	Socket(SOCKET socket, const Option& option, QSTATUS* pstatus);
	Socket(const WCHAR* pwszHost, short nPort,
		const Option& option, QSTATUS* pstatus);
	~Socket();

public:
	SOCKET getSocket() const;
	long getTimeout() const;
	QSTATUS connect(const WCHAR* pwszHost, short nPort);
	QSTATUS close();
	QSTATUS recv(char* p, int* pnLen, int nFlags);
	QSTATUS send(const char* p, int* pnLen, int nFlags);
	QSTATUS select(int* pnSelect);
	QSTATUS select(int* pnSelect, long nTimeout);
	QSTATUS getInputStream(InputStream** ppStream);
	QSTATUS getOutputStream(OutputStream** ppStream);
	Error getLastError() const;
	void setLastError(Error error);

private:
	Socket(const Socket&);
	Socket& operator=(const Socket&);

private:
	struct SocketImpl* pImpl_;
};


/****************************************************************************
 *
 * ServerSocket
 *
 */

class QSEXPORTCLASS ServerSocket
{
public:
	ServerSocket(short nPort, int nBackLog, QSTATUS* pstatus);
	~ServerSocket();

public:
	QSTATUS close();
	QSTATUS accept(SOCKET* pSocket);

private:
	ServerSocket(const ServerSocket&);
	ServerSocket& operator=(const ServerSocket&);

private:
	struct ServerSocketImpl* pImpl_;
};


/****************************************************************************
 *
 * SocketCallback
 *
 */

class QSEXPORTCLASS SocketCallback
{
public:
	virtual ~SocketCallback();

public:
	virtual bool isCanceled(bool bForce) const = 0;
	virtual QSTATUS initialize() = 0;
	virtual QSTATUS lookup() = 0;
	virtual QSTATUS connecting() = 0;
	virtual QSTATUS connected() = 0;
};

}

#endif // __QSSOCKET_H__
