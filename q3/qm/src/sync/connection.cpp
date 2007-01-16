/*
 * $Id$
 *
 * Copyright(C) 1998-2007 Satoshi Nakamura
 * All rights reserved.
 *
 */

#pragma warning(disable:4786)

#include <qmconnection.h>

using namespace qm;
using namespace qs;


namespace qm {
struct ConnectionFactoryImpl;
}


/****************************************************************************
 *
 * Connection
 *
 */

qm::Connection::~Connection()
{
}

bool qm::Connection::setProperty(const WCHAR* pwszName,
								 const WCHAR* pwszValue)
{
	return true;
}


/****************************************************************************
 *
 * ConnectionCallback
 *
 */

qm::ConnectionCallback::~ConnectionCallback()
{
}


/****************************************************************************
 *
 * ConnectionUI
 *
 */

qm::ConnectionUI::~ConnectionUI()
{
}


/****************************************************************************
 *
 * ConnectionFactoryImpl
 *
 */

struct qm::ConnectionFactoryImpl
{
	typedef std::vector<std::pair<const WCHAR*, ConnectionFactory*> > FactoryList;
	
	static FactoryList::iterator getIterator(const WCHAR* pwszName);
	
	static FactoryList listFactory__;
};

ConnectionFactoryImpl::FactoryList qm::ConnectionFactoryImpl::listFactory__;

ConnectionFactoryImpl::FactoryList::iterator qm::ConnectionFactoryImpl::getIterator(const WCHAR* pwszName)
{
	return std::find_if(ConnectionFactoryImpl::listFactory__.begin(),
		ConnectionFactoryImpl::listFactory__.end(),
		std::bind2nd(
			binary_compose_f_gx_hy(
				string_equal<WCHAR>(),
				std::select1st<ConnectionFactoryImpl::FactoryList::value_type>(),
				std::identity<const WCHAR*>()),
			pwszName));
}


/****************************************************************************
 *
 * ConnectionFactory
 *
 */

qm::ConnectionFactory::ConnectionFactory()
{
}

qm::ConnectionFactory::~ConnectionFactory()
{
}

std::auto_ptr<Connection> qm::ConnectionFactory::getConnection(const WCHAR* pwszName,
															   long nTimeout,
															   SocketCallback* pSocketCallback,
															   SSLSocketCallback* pSSLSocketCallback,
															   ConnectionCallback* pConnectionCallback,
															   Logger* pLogger)
{
	ConnectionFactoryImpl::FactoryList::const_iterator it =
		ConnectionFactoryImpl::getIterator(pwszName);
	if (it == ConnectionFactoryImpl::listFactory__.end())
		return std::auto_ptr<Connection>();
	return (*it).second->createConnection(nTimeout, pSocketCallback,
		pSSLSocketCallback, pConnectionCallback, pLogger);
}

std::auto_ptr<ConnectionUI> qm::ConnectionFactory::getUI(const WCHAR* pwszName)
{
	ConnectionFactoryImpl::FactoryList::const_iterator it =
		ConnectionFactoryImpl::getIterator(pwszName);
	if (it == ConnectionFactoryImpl::listFactory__.end())
		return std::auto_ptr<ConnectionUI>();
	return (*it).second->createUI();
}

void qm::ConnectionFactory::getNames(NameList* pList)
{
	assert(pList);
	
	typedef ConnectionFactoryImpl::FactoryList List;
	const List& l = ConnectionFactoryImpl::listFactory__;
	pList->reserve(l.size());
	for (List::const_iterator it = l.begin(); it != l.end(); ++it) {
		wstring_ptr wstrName(allocWString((*it).first));
		pList->push_back(wstrName.release());
	}
}

void qm::ConnectionFactory::registerFactory(const WCHAR* pwszName,
											ConnectionFactory* pFactory)
{
	assert(pwszName);
	assert(pFactory);
	
	ConnectionFactoryImpl::listFactory__.push_back(std::make_pair(pwszName, pFactory));
}

void qm::ConnectionFactory::unregisterFactory(const WCHAR* pwszName)
{
	assert(pwszName);
	
	ConnectionFactoryImpl::FactoryList::iterator it =
		ConnectionFactoryImpl::getIterator(pwszName);
	if (it != ConnectionFactoryImpl::listFactory__.end())
		ConnectionFactoryImpl::listFactory__.erase(it);
}
