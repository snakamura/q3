/*
 * $Id$
 *
 * Copyright(C) 1998-2005 Satoshi Nakamura
 * All rights reserved.
 *
 */

#pragma warning(disable:4786)

#include <qmapplication.h>
#include <qmsecurity.h>
#include <qmsession.h>

#include <qsstl.h>

#include "../ui/resourceinc.h"

using namespace qm;
using namespace qs;


namespace qm {
struct ReceiveSessionFactoryImpl;
struct SendSessionFactoryImpl;
}


/****************************************************************************
 *
 * PasswordCallback
 *
 */

qm::PasswordCallback::~PasswordCallback()
{
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

void qm::ReceiveSessionUI::subscribe(Document* pDocument,
									 Account* pAccount,
									 Folder* pFolder,
									 HWND hwnd)
{
}

bool qm::ReceiveSessionUI::canSubscribe(Account* pAccount,
										Folder* pFolder)
{
	return false;
}

wstring_ptr qm::ReceiveSessionUI::getSubscribeText()
{
	return 0;
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
		return std::auto_ptr<ReceiveSession>(0);
	return (*it).second->createSession();
}

std::auto_ptr<ReceiveSessionUI> qm::ReceiveSessionFactory::getUI(const WCHAR* pwszName)
{
	ReceiveSessionFactoryImpl::FactoryList::iterator it =
		ReceiveSessionFactoryImpl::getIterator(pwszName);
	if (it == ReceiveSessionFactoryImpl::listFactory__.end())
		return std::auto_ptr<ReceiveSessionUI>(0);
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

void qm::ReceiveSessionFactory::getClasses(NameList* pList)
{
	assert(pList);
	
	typedef ReceiveSessionFactoryImpl::FactoryList List;
	const List& l = ReceiveSessionFactoryImpl::listFactory__;
	pList->reserve(l.size());
	for (List::const_iterator it = l.begin(); it != l.end(); ++it) {
		std::auto_ptr<ReceiveSessionUI> pUI((*it).second->createUI());
		wstring_ptr wstrClass(allocWString(pUI->getClass()));
		if (std::find_if(pList->begin(), pList->end(),
			std::bind2nd(string_equal<WCHAR>(), wstrClass.get())) == pList->end())
			pList->push_back(wstrClass.release());
	}
	
	std::sort(pList->begin(), pList->end(), string_less<WCHAR>());
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
		return std::auto_ptr<SendSession>(0);
	return (*it).second->createSession();
}

std::auto_ptr<SendSessionUI> qm::SendSessionFactory::getUI(const WCHAR* pwszName)
{
	SendSessionFactoryImpl::FactoryList::iterator it =
		SendSessionFactoryImpl::getIterator(pwszName);
	if (it == SendSessionFactoryImpl::listFactory__.end())
		return std::auto_ptr<SendSessionUI>(0);
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
 * AbstractSSLSocketCallback
 *
 */

qm::AbstractSSLSocketCallback::AbstractSSLSocketCallback(const Security* pSecurity) :
	pSecurity_(pSecurity)
{
}

qm::AbstractSSLSocketCallback::~AbstractSSLSocketCallback()
{
}

const Store* qm::AbstractSSLSocketCallback::getCertStore()
{
	return pSecurity_->getCA();
}

bool qm::AbstractSSLSocketCallback::checkCertificate(const Certificate& cert,
													 bool bVerified)
{
	unsigned int nSslOption = getOption();
	
	if (!bVerified && (nSslOption & SubAccount::SSLOPTION_ALLOWUNVERIFIEDCERTIFICATE) == 0)
		return false;
	
	if ((nSslOption & SubAccount::SSLOPTION_ALLOWDIFFERENTHOST) == 0) {
		const WCHAR* pwszHost = getHost();
		
		std::auto_ptr<GeneralNames> pSubjectAltName(cert.getSubjectAltNames());
		int nAltNameCount = pSubjectAltName.get() ? pSubjectAltName->getCount() : 0;
		if (nAltNameCount != 0) {
			int n = 0;
			for (n = 0; n < nAltNameCount; ++n) {
				std::auto_ptr<GeneralName> pName(pSubjectAltName->getGeneralName(n));
				if (pName->getType() == GeneralName::TYPE_DNS &&
					checkHostName(pwszHost, pName->getValue().get()))
					break;
			}
			if (n == nAltNameCount)
				return false;
		}
		else {
			std::auto_ptr<Name> pSubject(cert.getSubject());
			if (!pSubject.get() ||
				!checkHostName(pwszHost, pSubject->getCommonName().get()))
				return false;
		}
	}
	
	return true;
}

bool qm::AbstractSSLSocketCallback::checkHostName(const WCHAR* pwszHostName,
												  const WCHAR* pwszCertName)
{
	while (pwszCertName && pwszHostName) {
		const WCHAR* pCert = wcschr(pwszCertName, L'.');
		const WCHAR* pHost = wcschr(pwszHostName, L'.');
		if (pCert) {
			if (pHost) {
				if (pCert - pwszCertName != 1 || *pwszCertName != L'*') {
					if (pCert - pwszCertName != pHost - pwszHostName ||
						wcsncmp(pwszCertName, pwszHostName, pCert - pwszCertName) != 0)
						return false;
				}
			}
			else {
				return false;
			}
		}
		else {
			if (pHost) {
				return false;
			}
			else {
				if (wcscmp(pwszCertName, pwszHostName) != 0)
					return false;
				else
					break;
			}
		}
		
		assert(pCert && pHost);
		pwszCertName = pCert + 1;
		pwszHostName = pHost + 1;
	}
	return true;
}


/****************************************************************************
 *
 * DefaultSSLSocketCallback
 *
 */

qm::DefaultSSLSocketCallback::DefaultSSLSocketCallback(SubAccount* pSubAccount,
													   Account::Host host,
													   const Security* pSecurity) :
	AbstractSSLSocketCallback(pSecurity),
	pSubAccount_(pSubAccount),
	host_(host)
{
}

qm::DefaultSSLSocketCallback::~DefaultSSLSocketCallback()
{
}

unsigned int qm::DefaultSSLSocketCallback::getOption()
{
	return pSubAccount_->getSslOption();
}

const WCHAR* qm::DefaultSSLSocketCallback::getHost()
{
	return pSubAccount_->getHost(host_);
}


/****************************************************************************
 *
 * DefaultReceiveSessionRuleCallback
 *
 */

qm::DefaultReceiveSessionRuleCallback::DefaultReceiveSessionRuleCallback(ReceiveSessionCallback* pCallback) :
	pCallback_(pCallback)
{
}

qm::DefaultReceiveSessionRuleCallback::~DefaultReceiveSessionRuleCallback()
{
}

bool qm::DefaultReceiveSessionRuleCallback::isCanceled()
{
	return pCallback_->isCanceled(false);
}

void qm::DefaultReceiveSessionRuleCallback::checkingMessages(Folder* pFolder)
{
	wstring_ptr wstrMessage(getMessage(IDS_MESSAGE_CHECKMESSAGES, pFolder));
	pCallback_->setMessage(wstrMessage.get());
}

void qm::DefaultReceiveSessionRuleCallback::applyingRule(Folder* pFolder)
{
	wstring_ptr wstrMessage(getMessage(IDS_MESSAGE_APPLYRULE, pFolder));
	pCallback_->setMessage(wstrMessage.get());
}

void qm::DefaultReceiveSessionRuleCallback::setRange(size_t nMin,
													 size_t nMax)
{
	pCallback_->setRange(nMin, nMax);
}

void qm::DefaultReceiveSessionRuleCallback::setPos(size_t nPos)
{
	pCallback_->setPos(nPos);
}

wstring_ptr qm::DefaultReceiveSessionRuleCallback::getMessage(UINT nId,
															  Folder* pFolder)
{
	HINSTANCE hInst = Application::getApplication().getResourceHandle();
	wstring_ptr wstrMessage(loadString(hInst, nId));
	wstring_ptr wstrName(pFolder->getFullName());
	return concat(wstrMessage.get(), L" : ", wstrName.get());
}
