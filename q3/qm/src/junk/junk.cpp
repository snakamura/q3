/*
 * $Id$
 *
 * Copyright(C) 1998-2004 Satoshi Nakamura
 * All rights reserved.
 *
 */

#include <qmjunk.h>

#include <qsassert.h>

using namespace qm;
using namespace qs;

namespace qm {
struct JunkFilterFactoryImpl;
}


/****************************************************************************
 *
 * JunkFilter
 *
 */

qm::JunkFilter::~JunkFilter()
{
}

std::auto_ptr<JunkFilter> qm::JunkFilter::getInstance(const WCHAR* pwszPath,
													  Profile* pProfile)
{
	JunkFilterFactory* pFactory = JunkFilterFactory::getFactory();
	if (!pFactory)
		return std::auto_ptr<JunkFilter>();
	return pFactory->createJunkFilter(pwszPath, pProfile);
}


/****************************************************************************
 *
 * JunkFilterFactoryImpl
 *
 */

struct qm::JunkFilterFactoryImpl
{
	static JunkFilterFactory* pFactory__;
};

JunkFilterFactory* qm::JunkFilterFactoryImpl::pFactory__ = 0;


/****************************************************************************
 *
 * JunkFilterFactory
 *
 */

qm::JunkFilterFactory::JunkFilterFactory()
{
}

qm::JunkFilterFactory::~JunkFilterFactory()
{
}

JunkFilterFactory* qm::JunkFilterFactory::getFactory()
{
	return JunkFilterFactoryImpl::pFactory__;
}

void qm::JunkFilterFactory::registerFactory(JunkFilterFactory* pFactory)
{
	assert(!JunkFilterFactoryImpl::pFactory__);
	JunkFilterFactoryImpl::pFactory__ = pFactory;
}

void qm::JunkFilterFactory::unregisterFactory(JunkFilterFactory* pFactory)
{
	assert(JunkFilterFactoryImpl::pFactory__ == pFactory);
	JunkFilterFactoryImpl::pFactory__ = 0;
}
