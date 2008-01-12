/*
 * $Id$
 *
 * Copyright(C) 1998-2008 Satoshi Nakamura
 *
 */

#ifndef __HTTP_H__
#define __HTTP_H__

#include <qsstring.h>

namespace qs {

class HttpUtil;

class InputStream;
class SocketBase;


/****************************************************************************
 *
 * HttpUtil
 *
 */

class HttpUtil
{
public:
	static wstring_ptr getBasicCredential(const WCHAR* pwszUserName,
										  const WCHAR* pwszPassword);
	static unsigned int parseResponse(const char* p);
	static xstring_ptr readLine(InputStream* pInputStream);
	static xstring_ptr readLine(SocketBase* pSocket);
	static bool readByte(SocketBase* pSocket,
						 unsigned char* p);
	static bool write(SocketBase* pSocket,
					  const unsigned char* p,
					  size_t nLen);
	static bool write(SocketBase* pSocket,
					  const WCHAR* p,
					  size_t nLen);
};

}

#endif // __HTTP_H__
