/*
 * $Id$
 *
 * Copyright(C) 1998-2008 Satoshi Nakamura
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

malloc_size_ptr<unsigned char> qscrypto::Util::loadFromStream(InputStream* pStream)
{
	assert(pStream);
	
	const size_t nInitialSize = 2048;
	size_t nLen = 0;
	size_t nBufSize = 0;
	malloc_ptr<unsigned char> p;
	while (true) {
		if (nLen == nBufSize) {
			nBufSize = nBufSize == 0 ? nInitialSize : nBufSize*2;
			malloc_ptr<unsigned char> pNew(static_cast<unsigned char*>(
				reallocate(p.get(), nBufSize)));
			if (!pNew.get())
				return malloc_size_ptr<unsigned char>();
			p.release();
			p.reset(pNew.release());
		}
		
		size_t nRead = pStream->read(p.get() + nLen, nBufSize - nLen);
		if (nRead == -1)
			return malloc_size_ptr<unsigned char>();
		if (nRead == 0)
			break;
		nLen += nRead;
	}
	
	return malloc_size_ptr<unsigned char>(p, nLen);
}

void qscrypto::Util::logError(Log& log,
							  const WCHAR* pwszMessage)
{
	if (log.isErrorEnabled()) {
		char buf[256];
		ERR_error_string_n(ERR_get_error(), buf, sizeof(buf) - 1);
		const unsigned char* p = reinterpret_cast<const unsigned char*>(buf);
		log.error(pwszMessage, p, strlen(buf));
	}
}
