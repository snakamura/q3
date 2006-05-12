/*
 * $Id$
 *
 * Copyright(C) 1998-2005 Satoshi Nakamura
 * All rights reserved.
 *
 */

#pragma warning(disable:4786)

#include <qmprotocoldriver.h>
#include <qmsession.h>

#include <qsassert.h>
#include <qsstl.h>
#include <qsstring.h>

#include <algorithm>
#include <vector>

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

bool qm::ProtocolDriver::init()
{
	return true;
}

bool qm::ProtocolDriver::save(bool bForce)
{
	return true;
}

void qm::ProtocolDriver::setOffline(bool bOffline)
{
}

void qm::ProtocolDriver::setSubAccount(SubAccount* pSubAccount)
{
}

std::auto_ptr<NormalFolder> qm::ProtocolDriver::createFolder(const WCHAR* pwszName,
															 Folder* pParent)
{
	assert(false);
	return std::auto_ptr<NormalFolder>(0);
}

bool qm::ProtocolDriver::removeFolder(NormalFolder* pFolder)
{
	assert(false);
	return false;
}

bool qm::ProtocolDriver::renameFolder(NormalFolder* pFolder,
									  const WCHAR* pwszName)
{
	assert(false);
	return false;
}

bool qm::ProtocolDriver::moveFolder(NormalFolder* pFolder,
									NormalFolder* pParent,
									const WCHAR* pwszName)
{
	assert(false);
	return false;
}

bool qm::ProtocolDriver::createDefaultFolders(Account::FolderList* pList)
{
	return true;
}

bool qm::ProtocolDriver::getRemoteFolders(RemoteFolderList* pList)
{
	assert(false);
	return false;
}

std::pair<const WCHAR**, size_t> qm::ProtocolDriver::getFolderParamNames(Folder* pFolder)
{
	return std::pair<const WCHAR**, size_t>(0, 0);
}

void qm::ProtocolDriver::setDefaultFolderParams(NormalFolder* pFolder)
{
}

bool qm::ProtocolDriver::getMessage(MessageHolder* pmh,
									unsigned int nFlags,
									GetMessageCallback* pCallback)
{
	assert(false);
	return false;
}

bool qm::ProtocolDriver::setMessagesFlags(NormalFolder* pFolder,
										  const MessageHolderList& l,
										  unsigned int nFlags,
										  unsigned int nMask)
{
	assert(false);
	return false;
}

bool qm::ProtocolDriver::setMessagesLabel(NormalFolder* pFolder,
										  const MessageHolderList& l,
										  const WCHAR* pwszLabel)
{
	assert(false);
	return false;
}

bool qm::ProtocolDriver::appendMessage(NormalFolder* pFolder,
									   const CHAR* pszMessage,
									   size_t nLen,
									   unsigned int nFlags,
									   const WCHAR* pwszLabel)
{
	assert(false);
	return false;
}

bool qm::ProtocolDriver::removeMessages(NormalFolder* pFolder,
										const MessageHolderList& l)
{
	assert(false);
	return false;
}

bool qm::ProtocolDriver::copyMessages(const MessageHolderList& l,
									  NormalFolder* pFolderFrom,
									  NormalFolder* pFolderTo,
									  bool bMove)
{
	assert(false);
	return false;
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
