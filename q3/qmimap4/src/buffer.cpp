/*
 * $Id$
 *
 * Copyright(C) 1998-2006 Satoshi Nakamura
 * All rights reserved.
 *
 */

#pragma warning(disable:4786)

#include "buffer.h"
#include "error.h"
#include "imap4.h"

using namespace qmimap4;
using namespace qs;


/****************************************************************************
 *
 * Buffer
 *
 */

qmimap4::Buffer::Buffer(const CHAR* psz) :
	pSocket_(0),
	nError_(Imap4::IMAP4_ERROR_SUCCESS)
{
	if (psz) {
		if (!buf_.append(psz)) {
			// TODO
		}
	}
}

qmimap4::Buffer::Buffer(const CHAR* psz,
						SocketBase* pSocket) :
	pSocket_(pSocket),
	nError_(Imap4::IMAP4_ERROR_SUCCESS)
{
	if (psz) {
		if (!buf_.append(psz)) {
			// TODO
		}
	}
}

qmimap4::Buffer::~Buffer()
{
}

CHAR qmimap4::Buffer::get(size_t n,
						  Imap4Callback* pCallback,
						  size_t nStart)
{
	if (n >= buf_.getLength()) {
		if (!pSocket_)
			return '\0';
		if (!receive(n, pCallback, nStart))
			return '\0';
	}
	assert(buf_.getLength() >= n);
	
	return buf_.get(n);
}

size_t qmimap4::Buffer::find(CHAR c,
							 size_t n)
{
	const CHAR* p = 0;
	while (!p) {
		p = strchr(buf_.getCharArray() + n, c);
		if (!p) {
			if (!receive(buf_.getLength(), 0, 0))
				return -1;
		}
	}
	return p - buf_.getCharArray();
}

size_t qmimap4::Buffer::find(const CHAR* psz,
							 size_t n)
{
	const CHAR* p = 0;
	while (!p) {
		p = strstr(buf_.getCharArray() + n, psz);
		if (!p) {
			if (!receive(buf_.getLength(), 0, 0))
				return -1;
		}
	}
	return p - buf_.getCharArray();
}

string_ptr qmimap4::Buffer::substr(size_t nPos,
								   size_t nLen) const
{
	assert((nLen == -1 && buf_.getLength() >= nPos) ||
		buf_.getLength() >= nPos + nLen);
	
	const CHAR* p = buf_.getCharArray() + nPos;
	if (nLen == -1)
		nLen = strlen(p);
	return allocString(p, nLen);
}

size_t qmimap4::Buffer::free(size_t n)
{
	if (n < 1024*128 || n < (buf_.getLength() - n)*9)
		return n;
	
	buf_.remove(0, n);
	adjustTokens(-static_cast<ssize_t>(n));
	
	return 0;
}

bool qmimap4::Buffer::receive(size_t n,
							  Imap4Callback* pCallback,
							  size_t nStart)
{
	if (!pSocket_)
		IMAP4_ERROR(Imap4::IMAP4_ERROR_PARSE);
	
	if (pCallback)
		pCallback->setPos(0);
	
	const CHAR* pszOldBase = buf_.getCharArray();
	
	size_t nAllocSize = n - buf_.getLength() + Imap4::RECEIVE_BLOCK_SIZE;
	XStringBufferLock<XSTRING> lock(&buf_, nAllocSize);
	CHAR* pLock = lock.get();
	if (!pLock)
		return false;
	
	CHAR* p = pLock;
	do {
		int nSelect = pSocket_->select(Socket::SELECT_READ);
		if (nSelect == -1)
			IMAP4_ERROR_SOCKET(Imap4::IMAP4_ERROR_SELECTSOCKET);
		else if (nSelect == 0)
			IMAP4_ERROR(Imap4::IMAP4_ERROR_TIMEOUT);
		
		size_t nLen = pSocket_->recv(p, Imap4::RECEIVE_BLOCK_SIZE, 0);
		if (nLen == -1)
			IMAP4_ERROR_SOCKET(Imap4::IMAP4_ERROR_RECEIVE);
		else if (nLen == 0)
			IMAP4_ERROR(Imap4::IMAP4_ERROR_DISCONNECT);
		
		p += nLen;
		
		if (pCallback)
			pCallback->setPos((buf_.getLength() - nStart) + (p - pLock));
	} while (buf_.getLength() + (p - pLock) <= n);
	
	lock.unlock(p - pLock);
	
	const CHAR* pszNewBase = buf_.getCharArray();
	if (pszNewBase != pszOldBase)
		adjustTokens(pszNewBase - pszOldBase);
	
	return true;
}

void qmimap4::Buffer::adjustTokens(ssize_t nOffset)
{
	for (TokenList::iterator it = listToken_.begin(); it != listToken_.end(); ++it)
		(*it).first += nOffset;
}


/****************************************************************************
 *
 * TokenUtil
 *
 */


bool qmimap4::TokenUtil::isEqual(const std::pair<const CHAR*, size_t>& s,
								 const CHAR* psz)
{
	assert(s.first);
	assert(psz);
	
	const CHAR* p = s.first;
	for (size_t n = 0; n < s.second; ++n, ++p, ++psz) {
		if (*p != *psz)
			return false;
	}
	return *psz == '\0';
}

bool qmimap4::TokenUtil::isEqualIgnoreCase(const std::pair<const CHAR*, size_t>& s,
										   const CHAR* psz)
{
	assert(s.first);
	assert(psz);
	
	const CHAR* p = s.first;
	for (size_t n = 0; n < s.second; ++n, ++p, ++psz) {
		if (*p != *psz) {
			CHAR c1 = 'a' <= *p && *p <= 'z' ? *p - 'a' + 'A' : *p;
			CHAR c2 = 'a' <= *psz && *psz <= 'z' ? *psz - 'a' + 'A' : *psz;
			if (c1 != c2)
				return false;
		}
	}
	return *psz == '\0';
}

bool qmimap4::TokenUtil::string2number(const std::pair<const CHAR*, size_t>& s,
									   unsigned long* pn)
{
	assert(s.first);
	assert(pn);
	
	*pn = 0;
	
	// TODO
	// Take care of overflow.
	const CHAR* p = s.first;
	for (size_t n = 0; n < s.second; ++n, ++p) {
		if (*p < '0' || '9' < *p)
			return false;
		*pn = *pn*10 + *p - '0';
	}
	
	return true;
}
