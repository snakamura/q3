/*
 * $Id$
 *
 * Copyright(C) 1998-2003 Satoshi Nakamura
 * All rights reserved.
 *
 */

#include <qsstl.h>

#include <openssl/err.h>

#include "util.h"

using namespace qs;


/****************************************************************************
 *
 * Util
 *
 */

QSTATUS qscrypto::Util::createBIOFromStream(
	InputStream* pStream, unsigned char** pp, size_t* pnLen)
{
	assert(pStream);
	assert(pp);
	assert(pnLen);
	
	DECLARE_QSTATUS();
	
	const size_t nOneSize = 2048;
	size_t nLen = 0;
	size_t nBufSize = 0;
	malloc_ptr<unsigned char> p;
	while (true) {
		if (nLen == nBufSize) {
			nBufSize += nOneSize;
			malloc_ptr<unsigned char> pNew(static_cast<unsigned char*>(
				realloc(p.get(), nBufSize)));
			if (!pNew.get())
				return QSTATUS_OUTOFMEMORY;
			p.release();
			p.reset(pNew.release());
		}
		
		size_t nRead = 0;
		status = pStream->read(p.get() + nLen, nBufSize - nLen, &nRead);
		CHECK_QSTATUS();
		if (nRead == -1)
			break;
		nLen += nRead;
	}
	
	*pp = p.release();
	*pnLen = nLen;
	
	return QSTATUS_SUCCESS;
}

void qscrypto::Util::logError(qs::Log& log, const WCHAR* pwszMessage)
{
	if (log.isErrorEnabled()) {
		char buf[256];
		ERR_error_string_n(ERR_get_error(), buf, sizeof(buf) - 1);
		const unsigned char* p = reinterpret_cast<const unsigned char*>(buf);
		log.error(pwszMessage, p, strlen(buf));
	}
}
