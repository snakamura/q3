/*
 * $Id$
 *
 * Copyright(C) 1998-2004 Satoshi Nakamura
 * All rights reserved.
 *
 */

#include <qscrypto.h>

using namespace qs;

namespace qs {
struct CryptoFactoryImpl;
}


/****************************************************************************
 *
 * Name
 *
 */

qs::Name::~Name()
{
}


/****************************************************************************
 *
 * Certificate
 *
 */

qs::Certificate::~Certificate()
{
}

std::auto_ptr<Certificate> qs::Certificate::getInstance()
{
	return CryptoFactory::getFactory()->createCertificate();
}


/****************************************************************************
 *
 * PrivateKey
 *
 */

qs::PrivateKey::~PrivateKey()
{
}

std::auto_ptr<PrivateKey> qs::PrivateKey::getInstance()
{
	return CryptoFactory::getFactory()->createPrivateKey();
}


/****************************************************************************
 *
 * PublicKey
 *
 */

qs::PublicKey::~PublicKey()
{
}

std::auto_ptr<PublicKey> qs::PublicKey::getInstance()
{
	return CryptoFactory::getFactory()->createPublicKey();
}


/****************************************************************************
 *
 * PasswordCallback
 *
 */

qs::PasswordCallback::~PasswordCallback()
{
}


/****************************************************************************
 *
 * Store
 *
 */

qs::Store::~Store()
{
}

std::auto_ptr<Store> qs::Store::getInstance()
{
	return CryptoFactory::getFactory()->createStore();
}


/****************************************************************************
 *
 * Cipher
 *
 */

qs::Cipher::~Cipher()
{
}

std::auto_ptr<Cipher> qs::Cipher::getInstance(const WCHAR* pwszName)
{
	return CryptoFactory::getFactory()->createCipher(pwszName);
}


/****************************************************************************
 *
 * SMIMEUtility
 *
 */

qs::SMIMEUtility::~SMIMEUtility()
{
}

std::auto_ptr<SMIMEUtility> qs::SMIMEUtility::getInstance()
{
	return CryptoFactory::getFactory()->createSMIMEUtility();
}


/****************************************************************************
 *
 * SMIMECallback
 *
 */

qs::SMIMECallback::~SMIMECallback()
{
}


/****************************************************************************
 *
 * CryptoFactoryImpl
 *
 */

struct qs::CryptoFactoryImpl
{
	static CryptoFactory* pFactory__;
};

CryptoFactory* qs::CryptoFactoryImpl::pFactory__ = 0;


/****************************************************************************
 *
 * CryptoFactory
 *
 */

qs::CryptoFactory::CryptoFactory()
{
}

qs::CryptoFactory::~CryptoFactory()
{
}

CryptoFactory* qs::CryptoFactory::getFactory()
{
	return CryptoFactoryImpl::pFactory__;
}

void qs::CryptoFactory::registerFactory(CryptoFactory* pFactory)
{
	assert(!CryptoFactoryImpl::pFactory__);
	CryptoFactoryImpl::pFactory__ = pFactory;
}

void qs::CryptoFactory::unregisterFactory(CryptoFactory* pFactory)
{
	assert(CryptoFactoryImpl::pFactory__ == pFactory);
	CryptoFactoryImpl::pFactory__ = 0;
}
