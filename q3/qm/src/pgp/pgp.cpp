/*
 * $Id$
 *
 * Copyright(C) 1998-2007 Satoshi Nakamura
 * All rights reserved.
 *
 */

#include <qmpgp.h>

#include <qsassert.h>

using namespace qm;
using namespace qs;

namespace qm {
struct PGPFactoryImpl;
}


/****************************************************************************
 *
 * PGPUtility
 *
 */

qm::PGPUtility::~PGPUtility()
{
}

std::auto_ptr<PGPUtility> qm::PGPUtility::getInstance(Profile* pProfile)
{
	return PGPFactory::getFactory()->createPGPUtility(pProfile);
}


/****************************************************************************
 *
 * PGPFactoryImpl
 *
 */

struct qm::PGPFactoryImpl
{
	static PGPFactory* pFactory__;
};

PGPFactory* qm::PGPFactoryImpl::pFactory__ = 0;


/****************************************************************************
 *
 * PGPFactory
 *
 */

qm::PGPFactory::PGPFactory()
{
}

qm::PGPFactory::~PGPFactory()
{
}

PGPFactory* qm::PGPFactory::getFactory()
{
	return PGPFactoryImpl::pFactory__;
}

void qm::PGPFactory::registerFactory(PGPFactory* pFactory)
{
	assert(!PGPFactoryImpl::pFactory__);
	PGPFactoryImpl::pFactory__ = pFactory;
}

void qm::PGPFactory::unregisterFactory(PGPFactory* pFactory)
{
	assert(PGPFactoryImpl::pFactory__ == pFactory);
	PGPFactoryImpl::pFactory__ = 0;
}
