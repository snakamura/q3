/*
 * $Id$
 *
 * Copyright(C) 1998-2004 Satoshi Nakamura
 * All rights reserved.
 *
 */

#include <qmscript.h>

using namespace qm;

namespace qm {
struct ScriptFactoryImpl;
}


/****************************************************************************
 *
 * Script
 *
 */

qm::Script::~Script()
{
}


/****************************************************************************
 *
 * ScriptFactoryImpl
 *
 */

struct qm::ScriptFactoryImpl
{
	static ScriptFactory* pFactory__;
};

ScriptFactory* qm::ScriptFactoryImpl::pFactory__ = 0;


/****************************************************************************
 *
 * ScriptFactory
 *
 */

qm::ScriptFactory::ScriptFactory()
{
}

qm::ScriptFactory::~ScriptFactory()
{
}

ScriptFactory* qm::ScriptFactory::getFactory()
{
	return ScriptFactoryImpl::pFactory__;
}

void qm::ScriptFactory::registerFactory(ScriptFactory* pFactory)
{
	assert(!ScriptFactoryImpl::pFactory__);
	ScriptFactoryImpl::pFactory__ = pFactory;
}

void qm::ScriptFactory::unregisterFactory(ScriptFactory* pFactory)
{
	assert(ScriptFactoryImpl::pFactory__ == pFactory);
	ScriptFactoryImpl::pFactory__ = 0;
}
