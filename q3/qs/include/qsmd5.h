/*
 * $Id$
 *
 * Copyright(C) 1998-2008 Satoshi Nakamura
 *
 */

#ifndef __QSMD5_H__
#define __QSMD5_H__

#include <qs.h>


namespace qs {

/****************************************************************************
 *
 * MD5
 *
 */

class QSEXPORTCLASS MD5
{
public:
	static void md5(const unsigned char* p,
					size_t nLen,
					unsigned char* pDigest);
	static void md5ToString(const unsigned char* p,
							size_t nLen,
							CHAR* pszDigest);
	static void hmac(const unsigned char* p,
					 size_t nLen,
					 const unsigned char* pKey,
					 size_t nKeyLen,
					 unsigned char* pDigest);
	static void hmacToString(const unsigned char* p,
							 size_t nLen,
							 const unsigned char* pKey,
							 size_t nKeyLen,
							 CHAR* pszDigest);
};

}

#endif // __QSMD5_H__
