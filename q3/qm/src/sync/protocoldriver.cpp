/*
 * $Id$
 *
 * Copyright(C) 1998-2005 Satoshi Nakamura
 * All rights reserved.
 *
 */

#include <qmprotocoldriver.h>
#include <qmsession.h>

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
 * ProtocolDriver::GetMessageCallback
 *
 */

qm::ProtocolDriver::GetMessageCallback::~GetMessageCallback()
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
	static std::auto_ptr<PasswordCallback> pPasswordCallback_;
};

qm::ProtocolFactoryImpl::FactoryList qm::ProtocolFactoryImpl::listFactory__;
std::auto_ptr<PasswordCallback> qm::ProtocolFactoryImpl::pPasswordCallback_;

ProtocolFactoryImpl::FactoryList::iterator qm::ProtocolFactoryImpl::getIterator(const WCHAR* pwszName)
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

std::auto_ptr<ProtocolDriver> qm::ProtocolFactory::getDriver(Account* pAccount,
															 const Security* pSecurity)
{
	const WCHAR* pwszName = pAccount->getType(Account::HOST_RECEIVE);
	ProtocolFactoryImpl::FactoryList::iterator it =
		ProtocolFactoryImpl::getIterator(pwszName);
	if (it == ProtocolFactoryImpl::listFactory__.end())
		return std::auto_ptr<ProtocolDriver>(0);
	
	return (*it).second->createDriver(pAccount,
		ProtocolFactoryImpl::pPasswordCallback_.get(), pSecurity);
}

void qm::ProtocolFactory::setPasswordCallback(std::auto_ptr<PasswordCallback> pPasswordCallback)
{
	ProtocolFactoryImpl::pPasswordCallback_ = pPasswordCallback;
}

void qm::ProtocolFactory::registerFactory(const WCHAR* pwszName,
										  ProtocolFactory* pFactory)
{
	assert(pwszName);
	assert(pFactory);
	
	ProtocolFactoryImpl::listFactory__.push_back(std::make_pair(pwszName, pFactory));
}

void qm::ProtocolFactory::unregisterFactory(const WCHAR* pwszName)
{
	assert(pwszName);
	
	ProtocolFactoryImpl::FactoryList::iterator it =
		ProtocolFactoryImpl::getIterator(pwszName);
	if (it != ProtocolFactoryImpl::listFactory__.end())
		ProtocolFactoryImpl::listFactory__.erase(it);
}
