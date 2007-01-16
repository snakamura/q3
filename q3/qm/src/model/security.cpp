/*
 * $Id$
 *
 * Copyright(C) 1998-2007 Satoshi Nakamura
 * All rights reserved.
 *
 */

#include <qmfilenames.h>
#include <qmpassword.h>
#include <qmpgp.h>
#include <qmsecurity.h>

#include <qsconv.h>
#include <qsfile.h>
#include <qsinit.h>
#include <qslog.h>
#include <qsthread.h>

#include <tchar.h>

#include "security.h"

using namespace qm;
using namespace qs;


/****************************************************************************
 *
 * SecurityImpl
 *
 */

struct qm::SecurityImpl
{
	std::auto_ptr<Store> loadCA();
	
	wstring_ptr wstrPath_;
	Profile* pProfile_;
	std::auto_ptr<Store> pStoreCA_;
	std::auto_ptr<SMIMEUtility> pSMIMEUtility_;
	std::auto_ptr<PGPUtility> pPGPUtility_;
	CriticalSection cs_;
	
	static HINSTANCE hInstCrypto__;
	static HINSTANCE hInstPGP__;
};

HINSTANCE qm::SecurityImpl::hInstCrypto__ = 0;
HINSTANCE qm::SecurityImpl::hInstPGP__ = 0;

std::auto_ptr<Store> qm::SecurityImpl::loadCA()
{
	Log log(InitThread::getInitThread().getLogger(), L"qm::SecurityImpl");
	
	std::auto_ptr<Store> pStoreCA(Store::getInstance());
	if (pProfile_->getInt(L"Security", L"LoadSystemStore")) {
		if (!pStoreCA->loadSystem())
			log.warn(L"Failed to load certificates from the system store.");
	}
	
	wstring_ptr wstrCAPath(concat(wstrPath_.get(), L"\\", FileNames::CA_PEM));
	if (File::isFileExisting(wstrCAPath.get())) {
		if (!pStoreCA->load(wstrCAPath.get(), Store::FILETYPE_PEM))
			log.warn(L"Failed to load certificates from ca.pem.");
	}
	
	return pStoreCA;
}


/****************************************************************************
 *
 * Security
 *
 */

qm::Security::Security(const WCHAR* pwszPath,
					   Profile* pProfile) :
	pImpl_(0)
{
	pImpl_ = new SecurityImpl();
	pImpl_->wstrPath_ = concat(pwszPath, L"\\security");
	pImpl_->pProfile_ = pProfile;
}

qm::Security::~Security()
{
	delete pImpl_;
}

const Store* qm::Security::getCA() const
{
#if 1
	Lock<CriticalSection> lock(pImpl_->cs_);
	
	if (!pImpl_->pStoreCA_.get())
		pImpl_->pStoreCA_ = pImpl_->loadCA();
	
	return pImpl_->pStoreCA_.get();
#else
	return pImpl_->loadCA().release();
#endif
}

const SMIMEUtility* qm::Security::getSMIMEUtility() const
{
	Lock<CriticalSection> lock(pImpl_->cs_);
	
	if (!pImpl_->pSMIMEUtility_.get()) {
		if (SecurityImpl::hInstCrypto__)
			pImpl_->pSMIMEUtility_ = SMIMEUtility::getInstance();
	}
	
	return pImpl_->pSMIMEUtility_.get();
}

std::auto_ptr<Certificate> qm::Security::getCertificate(const WCHAR* pwszName) const
{
	assert(pwszName);
	
	ConcatW c[] = {
		{ pImpl_->wstrPath_.get(),	-1	},
		{ L"\\",					1	},
		{ pwszName,					-1	},
		{ L".pem",					-1	}
	};
	wstring_ptr wstrPath(concat(c, countof(c)));
	
	std::auto_ptr<Certificate> pCertificate(Certificate::getInstance());
	if (!pCertificate->load(wstrPath.get(), Certificate::FILETYPE_PEM, 0))
		return std::auto_ptr<Certificate>(0);
	
	return pCertificate;
}

const PGPUtility* qm::Security::getPGPUtility() const
{
	Lock<CriticalSection> lock(pImpl_->cs_);
	
	if (!pImpl_->pPGPUtility_.get()) {
		if (SecurityImpl::hInstPGP__)
			pImpl_->pPGPUtility_ = PGPUtility::getInstance(pImpl_->pProfile_);
	}
	
	return pImpl_->pPGPUtility_.get();
}

void qm::Security::reload()
{
	Lock<CriticalSection> lock(pImpl_->cs_);
	
	pImpl_->pStoreCA_.reset(0);
}

void qm::Security::init()
{
#ifdef NDEBUG
#	ifdef UNICODE
#		define SUFFIX _T("u")
#	else
#		define SUFFIX _T("")
#	endif
#else
#	ifdef UNICODE
#		define SUFFIX _T("ud")
#	else
#		define SUFFIX _T("d")
#	endif
#endif
	SecurityImpl::hInstCrypto__ = ::LoadLibrary(_T("qscrypto") SUFFIX _T(".dll"));
	SecurityImpl::hInstPGP__ = ::LoadLibrary(_T("qmpgp") SUFFIX _T(".dll"));
}

void qm::Security::term()
{
	if (SecurityImpl::hInstCrypto__) {
		::FreeLibrary(SecurityImpl::hInstCrypto__);
		SecurityImpl::hInstCrypto__ = 0;
	}
	if (SecurityImpl::hInstPGP__) {
		::FreeLibrary(SecurityImpl::hInstPGP__);
		SecurityImpl::hInstPGP__ = 0;
	}
}

bool qm::Security::isSSLEnabled()
{
	return SecurityImpl::hInstCrypto__ != 0;
}

bool qm::Security::isSMIMEEnabled()
{
	return SecurityImpl::hInstCrypto__ != 0;
}

bool qm::Security::isPGPEnabled()
{
	return SecurityImpl::hInstPGP__ != 0;
}


/****************************************************************************
 *
 * FileCryptoPasswordCallback
 *
 */

qm::FileCryptoPasswordCallback::FileCryptoPasswordCallback(PasswordManager* pPasswordManager,
														   const WCHAR* pwszPath) :
														   pPasswordManager_(pPasswordManager),
														   pwszPath_(pwszPath),
														   state_(PASSWORDSTATE_ONETIME)
{
}

qm::FileCryptoPasswordCallback::~FileCryptoPasswordCallback()
{
}

void qm::FileCryptoPasswordCallback::save()
{
	if (state_ == PASSWORDSTATE_SESSION || state_ == PASSWORDSTATE_SAVE) {
		FilePasswordCondition condition(pwszPath_);
		pPasswordManager_->setPassword(condition,
			wstrPassword_.get(), state_ == PASSWORDSTATE_SAVE);
	}
}

wstring_ptr qm::FileCryptoPasswordCallback::getPassword()
{
	FilePasswordCondition condition(pwszPath_);
	wstring_ptr wstrPassword = pPasswordManager_->getPassword(condition, false, &state_);
	if (!wstrPassword.get())
		return 0;
	
	wstrPassword_ = allocWString(wstrPassword.get());
	
	return wstrPassword;
}
