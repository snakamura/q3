/*
 * $Id$
 *
 * Copyright(C) 1998-2003 Satoshi Nakamura
 * All rights reserved.
 *
 */

#include <qmsecurity.h>
#include <qmsession.h>

#include <qserror.h>
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

ReceiveSessionFactoryImpl::FactoryList::iterator qm::ReceiveSessionFactoryImpl::getIterator(
	const WCHAR* pwszName)
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

QSTATUS qm::ReceiveSessionFactory::getSession(const WCHAR* pwszName,
	ReceiveSession** ppReceiveSession)
{
	assert(ppReceiveSession);
	
	*ppReceiveSession = 0;
	
	ReceiveSessionFactoryImpl::FactoryList::iterator it =
		ReceiveSessionFactoryImpl::getIterator(pwszName);
	if (it == ReceiveSessionFactoryImpl::listFactory__.end())
		return QSTATUS_FAIL;
	
	return (*it).second->createSession(ppReceiveSession);
}

QSTATUS qm::ReceiveSessionFactory::getSession(const WCHAR* pwszName,
	std::auto_ptr<ReceiveSession>* papReceiveSession)
{
	assert(papReceiveSession);
	
	DECLARE_QSTATUS();
	
	ReceiveSession* p = 0;
	status = getSession(pwszName, &p);
	CHECK_QSTATUS();
	papReceiveSession->reset(p);
	
	return QSTATUS_SUCCESS;
}

QSTATUS qm::ReceiveSessionFactory::getUI(
	const WCHAR* pwszName, ReceiveSessionUI** ppUI)
{
	assert(ppUI);
	
	*ppUI = 0;
	
	ReceiveSessionFactoryImpl::FactoryList::iterator it =
		ReceiveSessionFactoryImpl::getIterator(pwszName);
	if (it == ReceiveSessionFactoryImpl::listFactory__.end())
		return QSTATUS_FAIL;
	
	return (*it).second->createUI(ppUI);
}

QSTATUS qm::ReceiveSessionFactory::getUI(const WCHAR* pwszName,
	std::auto_ptr<ReceiveSessionUI>* papUI)
{
	assert(papUI);
	
	DECLARE_QSTATUS();
	
	ReceiveSessionUI* p = 0;
	status = getUI(pwszName, &p);
	CHECK_QSTATUS();
	papUI->reset(p);
	
	return QSTATUS_SUCCESS;
}

QSTATUS qm::ReceiveSessionFactory::getNames(NameList* pList)
{
	assert(pList);
	
	DECLARE_QSTATUS();
	
	status = STLWrapper<NameList>(*pList).reserve(
		ReceiveSessionFactoryImpl::listFactory__.size());
	CHECK_QSTATUS();
	
	ReceiveSessionFactoryImpl::FactoryList::iterator it =
		ReceiveSessionFactoryImpl::listFactory__.begin();
	while (it != ReceiveSessionFactoryImpl::listFactory__.end()) {
		string_ptr<WSTRING> wstrName(allocWString((*it).first));
		if (!wstrName.get())
			return QSTATUS_OUTOFMEMORY;
		pList->push_back(wstrName.release());
		++it;
	}
	
	return QSTATUS_SUCCESS;
}

QSTATUS qm::ReceiveSessionFactory::regist(
	const WCHAR* pwszName, ReceiveSessionFactory* pFactory)
{
	assert(pwszName);
	assert(pFactory);
	
	return STLWrapper<ReceiveSessionFactoryImpl::FactoryList>(
		ReceiveSessionFactoryImpl::listFactory__).push_back(
			std::make_pair(pwszName, pFactory));
}

QSTATUS qm::ReceiveSessionFactory::unregist(const WCHAR* pwszName)
{
	assert(pwszName);
	
	ReceiveSessionFactoryImpl::FactoryList::iterator it =
		ReceiveSessionFactoryImpl::getIterator(pwszName);
	if (it != ReceiveSessionFactoryImpl::listFactory__.end())
		ReceiveSessionFactoryImpl::listFactory__.erase(it);
	
	return QSTATUS_SUCCESS;
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

SendSessionFactoryImpl::FactoryList::iterator qm::SendSessionFactoryImpl::getIterator(
	const WCHAR* pwszName)
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

QSTATUS qm::SendSessionFactory::getSession(const WCHAR* pwszName,
	SendSession** ppSendSession)
{
	assert(ppSendSession);
	
	*ppSendSession = 0;
	
	SendSessionFactoryImpl::FactoryList::iterator it =
		SendSessionFactoryImpl::getIterator(pwszName);
	if (it == SendSessionFactoryImpl::listFactory__.end())
		return QSTATUS_FAIL;
	
	return (*it).second->createSession(ppSendSession);
}

QSTATUS qm::SendSessionFactory::getSession(const WCHAR* pwszName,
	std::auto_ptr<SendSession>* papSendSession)
{
	assert(papSendSession);
	
	DECLARE_QSTATUS();
	
	SendSession* p = 0;
	status = getSession(pwszName, &p);
	CHECK_QSTATUS();
	papSendSession->reset(p);
	
	return QSTATUS_SUCCESS;
}

QSTATUS qm::SendSessionFactory::getUI(
	const WCHAR* pwszName, SendSessionUI** ppUI)
{
	assert(ppUI);
	
	*ppUI = 0;
	
	SendSessionFactoryImpl::FactoryList::iterator it =
		SendSessionFactoryImpl::getIterator(pwszName);
	if (it == SendSessionFactoryImpl::listFactory__.end())
		return QSTATUS_FAIL;
	
	return (*it).second->createUI(ppUI);
}

QSTATUS qm::SendSessionFactory::getUI(const WCHAR* pwszName,
	std::auto_ptr<SendSessionUI>* papUI)
{
	assert(papUI);
	
	DECLARE_QSTATUS();
	
	SendSessionUI* p = 0;
	status = getUI(pwszName, &p);
	CHECK_QSTATUS();
	papUI->reset(p);
	
	return QSTATUS_SUCCESS;
}

QSTATUS qm::SendSessionFactory::getNames(NameList* pList)
{
	assert(pList);
	
	DECLARE_QSTATUS();
	
	status = STLWrapper<NameList>(*pList).reserve(
		SendSessionFactoryImpl::listFactory__.size());
	CHECK_QSTATUS();
	
	SendSessionFactoryImpl::FactoryList::iterator it =
		SendSessionFactoryImpl::listFactory__.begin();
	while (it != SendSessionFactoryImpl::listFactory__.end()) {
		string_ptr<WSTRING> wstrName(allocWString((*it).first));
		if (!wstrName.get())
			return QSTATUS_OUTOFMEMORY;
		pList->push_back(wstrName.release());
		++it;
	}
	
	return QSTATUS_SUCCESS;
}

QSTATUS qm::SendSessionFactory::regist(
	const WCHAR* pwszName, SendSessionFactory* pFactory)
{
	assert(pwszName);
	assert(pFactory);
	
	return STLWrapper<SendSessionFactoryImpl::FactoryList>(
		SendSessionFactoryImpl::listFactory__).push_back(
			std::make_pair(pwszName, pFactory));
}

QSTATUS qm::SendSessionFactory::unregist(const WCHAR* pwszName)
{
	assert(pwszName);
	
	SendSessionFactoryImpl::FactoryList::iterator it =
		SendSessionFactoryImpl::getIterator(pwszName);
	if (it != SendSessionFactoryImpl::listFactory__.end())
		SendSessionFactoryImpl::listFactory__.erase(it);
	
	return QSTATUS_SUCCESS;
}


/****************************************************************************
 *
 * SessionErrorInfo
 *
 */

qm::SessionErrorInfo::SessionErrorInfo(Account* pAccount,
	SubAccount* pSubAccount, NormalFolder* pFolder,
	const WCHAR* pwszMessage, unsigned int nCode,
	const WCHAR* pwszDescriptions[], size_t nDescriptionCount) :
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

qm::DefaultSSLSocketCallback::DefaultSSLSocketCallback(
	SubAccount* pSubAccount, Account::Host host, const Security* pSecurity) :
	pSubAccount_(pSubAccount),
	host_(host),
	pSecurity_(pSecurity)
{
}

qm::DefaultSSLSocketCallback::~DefaultSSLSocketCallback()
{
}

QSTATUS qm::DefaultSSLSocketCallback::getCertStore(const Store** ppStore)
{
	assert(ppStore);
	*ppStore = pSecurity_->getCA();
	return QSTATUS_SUCCESS;
}
QSTATUS qm::DefaultSSLSocketCallback::checkCertificate(
	const Certificate& cert, bool bVerified)
{
	DECLARE_QSTATUS();
	
	if (!bVerified && !pSubAccount_->isAllowUnverifiedCertificate())
		return QSTATUS_FAIL;
	
	Name* p = 0;
	status = cert.getSubject(&p);
	CHECK_QSTATUS();
	std::auto_ptr<Name> pName(p);
	
	string_ptr<WSTRING> wstrCommonName;
	status = pName->getCommonName(&wstrCommonName);
	CHECK_QSTATUS();
	
	const WCHAR* pwszHost = pSubAccount_->getHost(host_);
	if (_wcsicmp(wstrCommonName.get(), pwszHost) != 0)
		return QSTATUS_FAIL;
	
	return QSTATUS_SUCCESS;
}
