/*
 * $Id$
 *
 * Copyright(C) 1998-2003 Satoshi Nakamura
 * All rights reserved.
 *
 */

#ifndef __QSSOCKET_H__
#define __QSSOCKET_H__

#include <qs.h>
#include <qsstream.h>

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
 * SocketBase
 *
 */

class QSEXPORTCLASS SocketBase
{
public:
	enum Error {
		SOCKET_ERROR_SUCCESS		= 0x00000000,
		SOCKET_ERROR_SOCKET			= 0x01000000,
		SOCKET_ERROR_CLOSESOCKET	= 0x02000000,
		SOCKET_ERROR_LOOKUPNAME		= 0x03000000,
		SOCKET_ERROR_CONNECT		= 0x04000000,
		SOCKET_ERROR_CONNECTTIMEOUT	= 0x05000000,
		SOCKET_ERROR_RECV			= 0x06000000,
		SOCKET_ERROR_RECVTIMEOUT	= 0x07000000,
		SOCKET_ERROR_SEND			= 0x08000000,
		SOCKET_ERROR_SENDTIMEOUT	= 0x09000000,
		SOCKET_ERROR_CANCEL			= 0x0a000000,
		SOCKET_ERROR_UNKNOWN		= 0x0b000000,
		SOCKET_ERROR_MASK_SOCKET	= 0xff000000
	};
	
	enum Select {
		SELECT_READ		= 0x01,
		SELECT_WRITE	= 0x02,
		SELECT_EXCEPT	= 0x04,
	};

public:
	virtual ~SocketBase();

public:
	virtual long getTimeout() const = 0;
	virtual unsigned int getLastError() const = 0;
	virtual void setLastError(unsigned int nError) = 0;
	virtual QSTATUS close() = 0;
	virtual QSTATUS recv(char* p, int* pnLen, int nFlags) = 0;
	virtual QSTATUS send(const char* p, int* pnLen, int nFlags) = 0;
	virtual QSTATUS select(int* pnSelect) = 0;
	virtual QSTATUS select(int* pnSelect, long nTimeout) = 0;
	virtual QSTATUS getInputStream(InputStream** ppStream) = 0;
	virtual QSTATUS getOutputStream(OutputStream** ppStream) = 0;
};


/****************************************************************************
 *
 * Socket
 *
 */

class QSEXPORTCLASS Socket : public SocketBase
{
public:
	struct Option
	{
		long nTimeout_;
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
	QSTATUS connect(const WCHAR* pwszHost, short nPort);

public:
	virtual long getTimeout() const;
	virtual unsigned int getLastError() const;
	virtual void setLastError(unsigned int nError);
	virtual QSTATUS close();
	virtual QSTATUS recv(char* p, int* pnLen, int nFlags);
	virtual QSTATUS send(const char* p, int* pnLen, int nFlags);
	virtual QSTATUS select(int* pnSelect);
	virtual QSTATUS select(int* pnSelect, long nTimeout);
	virtual QSTATUS getInputStream(InputStream** ppStream);
	virtual QSTATUS getOutputStream(OutputStream** ppStream);

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


/****************************************************************************
 *
 * SocketInputStream
 *
 */

class QSEXPORTCLASS SocketInputStream : public InputStream
{
public:
	SocketInputStream(SocketBase* pSocket, QSTATUS* pstatus);
	virtual ~SocketInputStream();

public:
	virtual QSTATUS close();
	virtual QSTATUS read(unsigned char* p, size_t nRead, size_t* pnRead);

private:
	SocketInputStream(const SocketInputStream&);
	SocketInputStream& operator=(const SocketInputStream&);

private:
	SocketBase* pSocket_;
};


/****************************************************************************
 *
 * SocketOutputStream
 *
 */

class QSEXPORTCLASS SocketOutputStream : public OutputStream
{
public:
	SocketOutputStream(SocketBase* pSocket, QSTATUS* pstatus);
	virtual ~SocketOutputStream();

public:
	virtual QSTATUS close();
	virtual QSTATUS write(const unsigned char* p, size_t nWrite);
	virtual QSTATUS flush();

private:
	SocketOutputStream(const SocketOutputStream&);
	SocketOutputStream& operator=(const SocketOutputStream&);

private:
	SocketBase* pSocket_;
};

}

#endif // __QSSOCKET_H__
