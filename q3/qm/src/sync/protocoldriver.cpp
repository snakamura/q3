/*
 * $Id: protocoldriver.cpp,v 1.1.1.1 2003/04/29 08:07:31 snakamura Exp $
 *
 * Copyright(C) 1998-2003 Satoshi Nakamura
 * All rights reserved.
 *
 */

#include <qmprotocoldriver.h>

#include <qsassert.h>
#include <qsstl.h>
#include <qsstring.h>

#include <algorithm>
#include <vector>

#pragma warning(disable:4786)

using namespace qm;
using namespace qs;


namespace qm {
struct ProtocolFactoryImpl;
}

/****************************************************************************
 *
 * ProtocolDriver
 *
 */

qm::ProtocolDriver::~ProtocolDriver()
{
}


/****************************************************************************
 *
 * ProtocolFactoryImpl
 *
 */

struct qm::ProtocolFactoryImpl
{
	typedef std::vector<std::pair<const WCHAR*, ProtocolFactory*> > FactoryList;
	
	static FactoryList::iterator getIterator(const WCHAR* pwszName);
	
	static FactoryList listFactory__;
};

qm::ProtocolFactoryImpl::FactoryList qm::ProtocolFactoryImpl::listFactory__;

qm::ProtocolFactoryImpl::FactoryList::iterator
	qm::ProtocolFactoryImpl::getIterator(const WCHAR* pwszName)
{
	return std::find_if(listFactory__.begin(), listFactory__.end(),
		std::bind2nd(
			binary_compose_f_gx_hy(
				string_equal<WCHAR>(),
				std::select1st<FactoryList::value_type>(),
				std::identity<const WCHAR*>()),
			pwszName));
}



/****************************************************************************
 *
 * ProtocolFactory
 *
 */

qm::ProtocolFactory::ProtocolFactory()
{
}

qm::ProtocolFactory::~ProtocolFactory()
{
}

QSTATUS qm::ProtocolFactory::getDriver(Account* pAccount,
	const WCHAR* pwszName, ProtocolDriver** ppProtocolDriver)
{
	assert(ppProtocolDriver);
	
	*ppProtocolDriver = 0;
	
	ProtocolFactoryImpl::FactoryList::iterator it =
		ProtocolFactoryImpl::getIterator(pwszName);
	if (it == ProtocolFactoryImpl::listFactory__.end())
		return QSTATUS_FAIL;
	
	return (*it).second->createDriver(pAccount, ppProtocolDriver);
}

QSTATUS qm::ProtocolFactory::getDriver(Account* pAccount,
	const WCHAR* pwszName, std::auto_ptr<ProtocolDriver>* papProtocolDriver)
{
	assert(papProtocolDriver);
	
	DECLARE_QSTATUS();
	
	ProtocolDriver* p = 0;
	status = getDriver(pAccount, pwszName, &p);
	CHECK_QSTATUS();
	papProtocolDriver->reset(p);
	
	return QSTATUS_SUCCESS;
}

QSTATUS qm::ProtocolFactory::regist(const WCHAR* pwszName,
	ProtocolFactory* pFactory)
{
	assert(pwszName);
	assert(pFactory);
	
	return STLWrapper<ProtocolFactoryImpl::FactoryList>(
		ProtocolFactoryImpl::listFactory__).push_back(
			std::make_pair(pwszName, pFactory));
}

QSTATUS qm::ProtocolFactory::unregist(const WCHAR* pwszName)
{
	assert(pwszName);
	
	ProtocolFactoryImpl::FactoryList::iterator it =
		ProtocolFactoryImpl::getIterator(pwszName);
	if (it != ProtocolFactoryImpl::listFactory__.end())
		ProtocolFactoryImpl::listFactory__.erase(it);
	
	return QSTATUS_SUCCESS;
}
