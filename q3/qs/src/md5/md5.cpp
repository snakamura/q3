/*
 * $Id: md5.cpp,v 1.1.1.1 2003/04/29 08:07:35 snakamura Exp $
 *
 * Copyright(C) 1998-2003 Satoshi Nakamura
 * All rights reserved.
 *
 */

#include <qsmd5.h>

#include <stdio.h>

extern "C" {
#define PROTOTYPES 1
#include "global.h"
#include "md5.h"
}


/****************************************************************************
 *
 * MD5
 *
 */

void qs::MD5::md5(const unsigned char* p, size_t nLen, unsigned char* pDigest)
{
	MD5_CTX context;
	MD5Init(&context);
	MD5Update(&context, const_cast<unsigned char*>(p), nLen);
	MD5Final(pDigest, &context);
}

void qs::MD5::md5ToString(const unsigned char* p, size_t nLen, CHAR* pszDigest)
{
	unsigned char digest[16];
	md5(p, nLen, digest);
	CHAR* pDest = pszDigest;
	for (int n = 0; n < sizeof(digest); ++n, pDest += 2)
		sprintf(pDest, "%02x", digest[n] & 0xff);
	*pDest = '\0';
}

void qs::MD5::hmac(const unsigned char* p, size_t nLen,
	const unsigned char* pKey, size_t nKeyLen, unsigned char* pDigest)
{
	hmac_md5(const_cast<unsigned char*>(p), nLen,
		const_cast<unsigned char*>(pKey), nKeyLen, pDigest);
}

void qs::MD5::hmacToString(const unsigned char* p, size_t nLen,
	const unsigned char* pKey, size_t nKeyLen, CHAR* pszDigest)
{
	unsigned char digest[16];
	hmac(p, nLen, pKey, nKeyLen, digest);
	CHAR* pDest = pszDigest;
	for (int n = 0; n < sizeof(digest); ++n, pDest += 2)
		sprintf(pDest, "%02x", digest[n] & 0xff);
	*pDest = '\0';
}
