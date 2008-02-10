/*
 * $Id$
 *
 * Copyright(C) 1998-2008 Satoshi Nakamura
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
	Winsock();
	~Winsock();

private:
	Winsock(const Winsock&);
	Winsock& operator=(const Winsock&);
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
		SOCKET_ERROR_SELECT			= 0x0a000000,
		SOCKET_ERROR_CANCEL			= 0x0b000000,
		SOCKET_ERROR_UNKNOWN		= 0x0c000000,
		SOCKET_ERROR_MASK_SOCKET	= 0xff000000
	};
	
	enum Select {
		SELECT_READ		= 0x01,
		SELECT_WRITE	= 0x02,
		SELECT_EXCEPT	= 0x04,
	};

protected:
	SocketBase();

public:
	virtual ~SocketBase();

public:
	virtual long getTimeout() const = 0;
	virtual void setTimeout(long nTimeout) = 0;
	virtual bool close() = 0;
	virtual int recv(char* p,
				int nLen,
				int nFlags) = 0;
	virtual int send(const char* p,
					 int nLen,
					 int nFlags) = 0;
	virtual int select(int nSelect) = 0;
	virtual int select(int nSelect,
					   long nTimeout) = 0;
	virtual InputStream* getInputStream() = 0;
	virtual OutputStream* getOutputStream() = 0;

public:
	Error getLastError() const;
	void setLastError(Error error);

public:
	static wstring_ptr getErrorDescription(Error error);

private:
	Error error_;
};


/****************************************************************************
 *
 * Socket
 *
 */

class QSEXPORTCLASS Socket : public SocketBase
{
public:
	Socket(long nTimeout,
		   SocketCallback* pSocketCallback,
		   Logger* pLogger);
	Socket(SOCKET socket,
		   long nTimeout,
		   SocketCallback* pSocketCallback,
		   Logger* pLogger);
	Socket(const WCHAR* pwszHost,
		   short nPort,
		   long nTimeout,
		   SocketCallback* pSocketCallback,
		   Logger* pLogger);
	~Socket();

public:
	bool operator!() const;

public:
	SOCKET getSocket() const;
	bool connect(const WCHAR* pwszHost,
				 short nPort);

public:
	virtual long getTimeout() const;
	virtual void setTimeout(long nTimeout);
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
	virtual InputStream* getInputStream();
	virtual OutputStream* getOutputStream();

private:
	Socket(const Socket&);
	Socket& operator=(const Socket&);

private:
	class SocketImpl* pImpl_;
};


/****************************************************************************
 *
 * ServerSocket
 *
 */

class QSEXPORTCLASS ServerSocket
{
public:
	ServerSocket(short nPort,
				 int nBackLog);
	~ServerSocket();

public:
	bool operator!() const;

public:
	bool close();
	SOCKET accept();

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
	virtual void initialize() = 0;
	virtual void lookup() = 0;
	virtual void connecting() = 0;
	virtual void connected() = 0;
};


/****************************************************************************
 *
 * DefaultSocketCallback
 *
 */

class QSEXPORTCLASS DefaultSocketCallback : public SocketCallback
{
public:
	DefaultSocketCallback();
	virtual ~DefaultSocketCallback();

public:
	virtual bool isCanceled(bool bForce) const;
	virtual void initialize();
	virtual void lookup();
	virtual void connecting();
	virtual void connected();
};


/****************************************************************************
 *
 * SocketInputStream
 *
 */

class QSEXPORTCLASS SocketInputStream : public InputStream
{
public:
	explicit SocketInputStream(SocketBase* pSocket);
	virtual ~SocketInputStream();

public:
	virtual bool close();
	virtual size_t read(unsigned char* p,
						size_t nRead);

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
	explicit SocketOutputStream(SocketBase* pSocket);
	virtual ~SocketOutputStream();

public:
	virtual bool close();
	virtual size_t write(const unsigned char* p, size_t nWrite);
	virtual bool flush();

private:
	SocketOutputStream(const SocketOutputStream&);
	SocketOutputStream& operator=(const SocketOutputStream&);

private:
	SocketBase* pSocket_;
};

}

#endif // __QSSOCKET_H__
