/*
 * $Id$
 *
 * Copyright(C) 1998-2004 Satoshi Nakamura
 * All rights reserved.
 *
 */

#include <qmsecurity.h>
#include <qmsession.h>

#include <qsstl.h>

#pragma warning(disable:4786)

using namespace qm;
using namespace qs;


namespace qm {
struct ReceiveSessionFactoryImpl;
struct SendSessionFactoryImpl;
}


/****************************************************************************
 *
 * SessionCallback
 *
 */

qm::SessionCallback::~SessionCallback()
{
}


/****************************************************************************
 *
 * ReceiveSession
 *
 */

qm::ReceiveSession::~ReceiveSession()
{
}


/****************************************************************************
 *
 * ReceiveSessionCallback
 *
 */

qm::ReceiveSessionCallback::~ReceiveSessionCallback()
{
}


/****************************************************************************
 *
 * ReceiveSessionUI
 *
 */

qm::ReceiveSessionUI::~ReceiveSessionUI()
{
}


/****************************************************************************
 *
 * ReceiveSessionFactoryImpl
 *
 */

struct qm::ReceiveSessionFactoryImpl
{
	typedef std::vector<std::pair<const WCHAR*, ReceiveSessionFactory*> > FactoryList;
	
	static FactoryList::iterator getIterator(const WCHAR* pwszName);
	
	static FactoryList listFactory__;
};

ReceiveSessionFactoryImpl::FactoryList qm::ReceiveSessionFactoryImpl::listFactory__;

ReceiveSessionFactoryImpl::FactoryList::iterator qm::ReceiveSessionFactoryImpl::getIterator(const WCHAR* pwszName)
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
 * ReceiveSessionFactory
 *
 */

qm::ReceiveSessionFactory::ReceiveSessionFactory()
{
}

qm::ReceiveSessionFactory::~ReceiveSessionFactory()
{
}

std::auto_ptr<ReceiveSession> qm::ReceiveSessionFactory::getSession(const WCHAR* pwszName)
{
	ReceiveSessionFactoryImpl::FactoryList::iterator it =
		ReceiveSessionFactoryImpl::getIterator(pwszName);
	if (it == ReceiveSessionFactoryImpl::listFactory__.end())
		return 0;
	return (*it).second->createSession();
}

std::auto_ptr<ReceiveSessionUI> qm::ReceiveSessionFactory::getUI(const WCHAR* pwszName)
{
	ReceiveSessionFactoryImpl::FactoryList::iterator it =
		ReceiveSessionFactoryImpl::getIterator(pwszName);
	if (it == ReceiveSessionFactoryImpl::listFactory__.end())
		return 0;
	return (*it).second->createUI();
}

void qm::ReceiveSessionFactory::getNames(NameList* pList)
{
	assert(pList);
	
	typedef ReceiveSessionFactoryImpl::FactoryList List;
	const List& l = ReceiveSessionFactoryImpl::listFactory__;
	pList->reserve(l.size());
	for (List::const_iterator it = l.begin(); it != l.end(); ++it) {
		wstring_ptr wstrName(allocWString((*it).first));
		pList->push_back(wstrName.release());
	}
}

void qm::ReceiveSessionFactory::registerFactory(const WCHAR* pwszName,
												ReceiveSessionFactory* pFactory)
{
	assert(pwszName);
	assert(pFactory);
	
	ReceiveSessionFactoryImpl::listFactory__.push_back(std::make_pair(pwszName, pFactory));
}

void qm::ReceiveSessionFactory::unregisterFactory(const WCHAR* pwszName)
{
	assert(pwszName);
	
	ReceiveSessionFactoryImpl::FactoryList::iterator it =
		ReceiveSessionFactoryImpl::getIterator(pwszName);
	if (it != ReceiveSessionFactoryImpl::listFactory__.end())
		ReceiveSessionFactoryImpl::listFactory__.erase(it);
}


/****************************************************************************
 *
 * SendSession
 *
 */

qm::SendSession::~SendSession()
{
}


/****************************************************************************
 *
 * SendSessionCallback
 *
 */

qm::SendSessionCallback::~SendSessionCallback()
{
}


/****************************************************************************
 *
 * SendSessionUI
 *
 */

qm::SendSessionUI::~SendSessionUI()
{
}


/****************************************************************************
 *
 * SendSessionFactoryImpl
 *
 */

struct qm::SendSessionFactoryImpl
{
	typedef std::vector<std::pair<const WCHAR*, SendSessionFactory*> > FactoryList;
	
	static FactoryList::iterator getIterator(const WCHAR* pwszName);
	
	static FactoryList listFactory__;
};

SendSessionFactoryImpl::FactoryList qm::SendSessionFactoryImpl::listFactory__;

SendSessionFactoryImpl::FactoryList::iterator qm::SendSessionFactoryImpl::getIterator(const WCHAR* pwszName)
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
 * SendSessionFactory
 *
 */

qm::SendSessionFactory::SendSessionFactory()
{
}

qm::SendSessionFactory::~SendSessionFactory()
{
}

std::auto_ptr<SendSession> qm::SendSessionFactory::getSession(const WCHAR* pwszName)
{
	SendSessionFactoryImpl::FactoryList::iterator it =
		SendSessionFactoryImpl::getIterator(pwszName);
	if (it == SendSessionFactoryImpl::listFactory__.end())
		return 0;
	return (*it).second->createSession();
}

std::auto_ptr<SendSessionUI> qm::SendSessionFactory::getUI(const WCHAR* pwszName)
{
	SendSessionFactoryImpl::FactoryList::iterator it =
		SendSessionFactoryImpl::getIterator(pwszName);
	if (it == SendSessionFactoryImpl::listFactory__.end())
		return 0;
	return (*it).second->createUI();
}

void qm::SendSessionFactory::getNames(NameList* pList)
{
	assert(pList);
	
	typedef SendSessionFactoryImpl::FactoryList List;
	const List& l = SendSessionFactoryImpl::listFactory__;
	pList->reserve(l.size());
	for (List::const_iterator it = l.begin(); it != l.end(); ++it) {
		wstring_ptr wstrName(allocWString((*it).first));
		pList->push_back(wstrName.release());
	}
}

void qm::SendSessionFactory::registerFactory(const WCHAR* pwszName,
											 SendSessionFactory* pFactory)
{
	assert(pwszName);
	assert(pFactory);
	
	SendSessionFactoryImpl::listFactory__.push_back(std::make_pair(pwszName, pFactory));
}

void qm::SendSessionFactory::unregisterFactory(const WCHAR* pwszName)
{
	assert(pwszName);
	
	SendSessionFactoryImpl::FactoryList::iterator it =
		SendSessionFactoryImpl::getIterator(pwszName);
	if (it != SendSessionFactoryImpl::listFactory__.end())
		SendSessionFactoryImpl::listFactory__.erase(it);
}


/****************************************************************************
 *
 * SessionErrorInfo
 *
 */

qm::SessionErrorInfo::SessionErrorInfo(Account* pAccount,
									   SubAccount* pSubAccount,
									   NormalFolder* pFolder,
									   const WCHAR* pwszMessage,
									   unsigned int nCode,
									   const WCHAR* pwszDescriptions[],
									   size_t nDescriptionCount) :
	pAccount_(pAccount),
	pSubAccount_(pSubAccount),
	pFolder_(pFolder),
	pwszMessage_(pwszMessage),
	nCode_(nCode),
	ppwszDescription_(pwszDescriptions),
	nDescriptionCount_(nDescriptionCount)
{
}

qm::SessionErrorInfo::~SessionErrorInfo()
{
}

Account* qm::SessionErrorInfo::getAccount() const
{
	return pAccount_;
}

SubAccount* qm::SessionErrorInfo::getSubAccount() const
{
	return pSubAccount_;
}

NormalFolder* qm::SessionErrorInfo::getFolder() const
{
	return pFolder_;
}

const WCHAR* qm::SessionErrorInfo::getMessage() const
{
	return pwszMessage_;
}

unsigned int qm::SessionErrorInfo::getCode() const
{
	return nCode_;
}

const WCHAR* qm::SessionErrorInfo::getDescription(size_t n) const
{
	assert(n < nDescriptionCount_);
	return ppwszDescription_[n];
}

size_t qm::SessionErrorInfo::getDescriptionCount() const
{
	return nDescriptionCount_;
}


/****************************************************************************
 *
 * DefaultSSLSocketCallback
 *
 */

qm::DefaultSSLSocketCallback::DefaultSSLSocketCallback(SubAccount* pSubAccount,
													   Account::Host host,
													   const Security* pSecurity) :
	pSubAccount_(pSubAccount),
	host_(host),
	pSecurity_(pSecurity)
{
}

qm::DefaultSSLSocketCallback::~DefaultSSLSocketCallback()
{
}

const Store* qm::DefaultSSLSocketCallback::getCertStore()
{
	return pSecurity_->getCA();
}

bool qm::DefaultSSLSocketCallback::checkCertificate(const Certificate& cert,
													bool bVerified)
{
	unsigned int nSslOption = pSubAccount_->getSslOption();
	
	if (!bVerified && (nSslOption & SubAccount::SSLOPTION_ALLOWUNVERIFIEDCERTIFICATE) == 0)
		return false;
	
	std::auto_ptr<Name> pName(cert.getSubject());
	wstring_ptr wstrCommonName(pName->getCommonName());
	if ((nSslOption & SubAccount::SSLOPTION_ALLOWDIFFERENTHOST) == 0 &&
		_wcsicmp(wstrCommonName.get(), pSubAccount_->getHost(host_)) != 0)
		return false;
	
	return true;
}
