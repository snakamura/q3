/*
 * $Id: socket.h,v 1.1.1.1 2003/04/29 08:07:35 snakamura Exp $
 *
 * Copyright(C) 1998-2003 Satoshi Nakamura
 * All rights reserved.
 *
 */

#ifndef __SOCKET_H__
#define __SOCKET_H__

#include <qssocket.h>
#include <qsstream.h>


namespace qs {

/****************************************************************************
 *
 * SocketInputStream
 *
 */

class SocketInputStream : public InputStream
{
public:
	SocketInputStream(Socket* pSocket, QSTATUS* pstatus);
	virtual ~SocketInputStream();

public:
	virtual QSTATUS close();
	virtual QSTATUS read(unsigned char* p, size_t nRead, size_t* pnRead);

private:
	SocketInputStream(const SocketInputStream&);
	SocketInputStream& operator=(const SocketInputStream&);

private:
	Socket* pSocket_;
};


/****************************************************************************
 *
 * SocketOutputStream
 *
 */

class SocketOutputStream : public OutputStream
{
public:
	SocketOutputStream(Socket* pSocket, QSTATUS* pstatus);
	virtual ~SocketOutputStream();

public:
	virtual QSTATUS close();
	virtual QSTATUS write(const unsigned char* p, size_t nWrite);
	virtual QSTATUS flush();

private:
	SocketOutputStream(const SocketOutputStream&);
	SocketOutputStream& operator=(const SocketOutputStream&);

private:
	Socket* pSocket_;
};

}

#endif // __SOCKET_H__
