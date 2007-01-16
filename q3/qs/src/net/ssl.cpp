/*
 * $Id$
 *
 * Copyright(C) 1998-2007 Satoshi Nakamura
 * All rights reserved.
 *
 */

#include <qsassert.h>
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

void qs::SSLSocketFactory::registerFactory(SSLSocketFactory* pFactory)
{
	assert(!SSLSocketFactoryImpl::pFactory__);
	SSLSocketFactoryImpl::pFactory__ = pFactory;
}

void qs::SSLSocketFactory::unregisterFactory(SSLSocketFactory* pFactory)
{
	assert(SSLSocketFactoryImpl::pFactory__ == pFactory);
	SSLSocketFactoryImpl::pFactory__ = 0;
}
