/*
 * $Id$
 *
 * Copyright(C) 1998-2003 Satoshi Nakamura
 * All rights reserved.
 *
 */

#include <qsassert.h>
#include <qserror.h>
#include <qsssl.h>

using namespace qs;


namespace qs {
struct SSLSocketFactoryImpl;
}


/****************************************************************************
 *
 * SSLSocket
 *
 */

qs::SSLSocket::~SSLSocket()
{
}


/****************************************************************************
 *
 * SSLSocketCallback
 *
 */

qs::SSLSocketCallback::~SSLSocketCallback()
{
}


/****************************************************************************
 *
 * SSLSocketFactoryImpl
 *
 */

struct qs::SSLSocketFactoryImpl
{
	static SSLSocketFactory* pFactory__;
};

SSLSocketFactory* qs::SSLSocketFactoryImpl::pFactory__ = 0;


/****************************************************************************
 *
 * SSLSocketFactory
 *
 */

qs::SSLSocketFactory::SSLSocketFactory()
{
}

qs::SSLSocketFactory::~SSLSocketFactory()
{
}

SSLSocketFactory* qs::SSLSocketFactory::getFactory()
{
	return SSLSocketFactoryImpl::pFactory__;
}

QSTATUS qs::SSLSocketFactory::regist(SSLSocketFactory* pFactory)
{
	assert(!SSLSocketFactoryImpl::pFactory__);
	SSLSocketFactoryImpl::pFactory__ = pFactory;
	return QSTATUS_SUCCESS;
}

QSTATUS qs::SSLSocketFactory::unregist(SSLSocketFactory* pFactory)
{
	assert(SSLSocketFactoryImpl::pFactory__ == pFactory);
	SSLSocketFactoryImpl::pFactory__ = 0;
	return QSTATUS_SUCCESS;
}
