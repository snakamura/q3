/*
 * $Id$
 *
 * Copyright(C) 1998-2003 Satoshi Nakamura
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

QSTATUS qs::Certificate::getInstance(Certificate** ppCertificate)
{
	return CryptoFactory::getFactory()->createCertificate(ppCertificate);
}


/****************************************************************************
 *
 * PrivateKey
 *
 */

qs::PrivateKey::~PrivateKey()
{
}

QSTATUS qs::PrivateKey::getInstance(PrivateKey** ppPrivateKey)
{
	return CryptoFactory::getFactory()->createPrivateKey(ppPrivateKey);
}


/****************************************************************************
 *
 * PublicKey
 *
 */

qs::PublicKey::~PublicKey()
{
}

QSTATUS qs::PublicKey::getInstance(PublicKey** ppPublicKey)
{
	return CryptoFactory::getFactory()->createPublicKey(ppPublicKey);
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

QSTATUS qs::Store::getInstance(Store** ppStore)
{
	return CryptoFactory::getFactory()->createStore(ppStore);
}


/****************************************************************************
 *
 * Cipher
 *
 */

qs::Cipher::~Cipher()
{
}

QSTATUS qs::Cipher::getInstance(const WCHAR* pwszName, Cipher** ppCipher)
{
	return CryptoFactory::getFactory()->createCipher(pwszName, ppCipher);
}


/****************************************************************************
 *
 * SMIMEUtility
 *
 */

qs::SMIMEUtility::~SMIMEUtility()
{
}

QSTATUS qs::SMIMEUtility::getInstance(SMIMEUtility** ppSMIMEUtility)
{
	return CryptoFactory::getFactory()->createSMIMEUtility(ppSMIMEUtility);
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

QSTATUS qs::CryptoFactory::regist(CryptoFactory* pFactory)
{
	assert(!CryptoFactoryImpl::pFactory__);
	CryptoFactoryImpl::pFactory__ = pFactory;
	return QSTATUS_SUCCESS;
}

QSTATUS qs::CryptoFactory::unregist(CryptoFactory* pFactory)
{
	assert(CryptoFactoryImpl::pFactory__ == pFactory);
	CryptoFactoryImpl::pFactory__ = 0;
	return QSTATUS_SUCCESS;
}
