/*
 * $Id$
 *
 * Copyright(C) 1998-2004 Satoshi Nakamura
 * All rights reserved.
 *
 */

#include <qmfilenames.h>
#include <qmsecurity.h>

#include <qsconv.h>

#include <tchar.h>

using namespace qm;
using namespace qs;


/****************************************************************************
 *
 * SecurityImpl
 *
 */

struct qm::SecurityImpl
{
	wstring_ptr wstrPath_;
	std::auto_ptr<Store> pStoreCA_;
	std::auto_ptr<SMIMEUtility> pSMIMEUtility_;
	
	static HINSTANCE hInst__;
};

HINSTANCE qm::SecurityImpl::hInst__ = 0;


/****************************************************************************
 *
 * Security
 *
 */

qm::Security::Security(const WCHAR* pwszPath) :
	pImpl_(0)
{
	if (SecurityImpl::hInst__) {
		pImpl_ = new SecurityImpl();
		pImpl_->wstrPath_ = concat(pwszPath, L"\\security");
		
		pImpl_->pStoreCA_ = Store::getInstance();
		wstring_ptr wstrCAPath(concat(pImpl_->wstrPath_.get(), L"\\", FileNames::CA_PEM));
		W2T(wstrCAPath.get(), ptszCAPath);
		if (::GetFileAttributes(ptszCAPath) != 0xffffffff) {
			if (!pImpl_->pStoreCA_->load(wstrCAPath.get(), Store::FILETYPE_PEM)) {
				// TODO
			}
		}
		
		pImpl_->pSMIMEUtility_ = SMIMEUtility::getInstance();
	}
}

qm::Security::~Security()
{
	delete pImpl_;
}

const Store* qm::Security::getCA() const
{
	return pImpl_ ? pImpl_->pStoreCA_.get() : 0;
}

const SMIMEUtility* qm::Security::getSMIMEUtility() const
{
	return pImpl_ ? pImpl_->pSMIMEUtility_.get() : 0;
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
	SecurityImpl::hInst__ = ::LoadLibrary(_T("qscrypto") SUFFIX _T(".dll"));
}

void qm::Security::term()
{
	if (SecurityImpl::hInst__) {
		::FreeLibrary(SecurityImpl::hInst__);
		SecurityImpl::hInst__ = 0;
	}
}

bool qm::Security::isEnabled()
{
	return SecurityImpl::hInst__ != 0;
}
