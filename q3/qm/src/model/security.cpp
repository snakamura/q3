/*
 * $Id$
 *
 * Copyright(C) 1998-2003 Satoshi Nakamura
 * All rights reserved.
 *
 */

#include <qmfilenames.h>
#include <qmsecurity.h>

#include <qsconv.h>
#include <qsnew.h>

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
	qs::WSTRING wstrPath_;
	qs::Store* pStoreCA_;
	qs::SMIMEUtility* pSMIMEUtility_;
	
	static HINSTANCE hInst__;
};

HINSTANCE qm::SecurityImpl::hInst__ = 0;


/****************************************************************************
 *
 * Security
 *
 */

qm::Security::Security(const WCHAR* pwszPath, QSTATUS* pstatus) :
	pImpl_(0)
{
	DECLARE_QSTATUS();
	
	if (SecurityImpl::hInst__) {
		status = newObject(&pImpl_);
		CHECK_QSTATUS_SET(pstatus);
		pImpl_->wstrPath_ = 0;
		pImpl_->pStoreCA_ = 0;
		pImpl_->pSMIMEUtility_ = 0;
		
		pImpl_->wstrPath_ = concat(pwszPath, L"\\security");
		if (!pImpl_->wstrPath_) {
			*pstatus = QSTATUS_FAIL;
			return;
		}
		
		status = Store::getInstance(&pImpl_->pStoreCA_);
		CHECK_QSTATUS_SET(pstatus);
		string_ptr<WSTRING> wstrCAPath(concat(
			pImpl_->wstrPath_, L"\\", FileNames::CA_PEM));
		if (!wstrCAPath.get()) {
			*pstatus = QSTATUS_FAIL;
			return;
		}
		W2T_STATUS(wstrCAPath.get(), ptszCAPath);
		CHECK_QSTATUS_SET(pstatus);
		if (::GetFileAttributes(ptszCAPath) != 0xffffffff) {
			status = pImpl_->pStoreCA_->load(
				wstrCAPath.get(), Store::FILETYPE_PEM);
			CHECK_QSTATUS_SET(pstatus);
		}
		
		status = SMIMEUtility::getInstance(&pImpl_->pSMIMEUtility_);
		CHECK_QSTATUS_SET(pstatus);
	}
}

qm::Security::~Security()
{
	if (pImpl_) {
		delete pImpl_->pSMIMEUtility_;
		delete pImpl_->pStoreCA_;
		freeWString(pImpl_->wstrPath_);
		delete pImpl_;
	}
}

const Store* qm::Security::getCA() const
{
	return pImpl_ ? pImpl_->pStoreCA_ : 0;
}

const SMIMEUtility* qm::Security::getSMIMEUtility() const
{
	return pImpl_ ? pImpl_->pSMIMEUtility_ : 0;
}

QSTATUS qm::Security::getCertificate(
	const WCHAR* pwszName, Certificate** ppCertificate) const
{
	assert(pwszName);
	assert(ppCertificate);
	
	DECLARE_QSTATUS();
	
	ConcatW c[] = {
		{ pImpl_->wstrPath_,	-1	},
		{ L"\\",		1	},
		{ pwszName,		-1	},
		{ L".pem",		-1	}
	};
	string_ptr<WSTRING> wstrPath(concat(c, countof(c)));
	if (!wstrPath.get())
		return QSTATUS_OUTOFMEMORY;
	
	std::auto_ptr<Certificate> pCertificate;
	status = CryptoUtil<Certificate>::getInstance(&pCertificate);
	CHECK_QSTATUS();
	status = pCertificate->load(wstrPath.get(),
		Certificate::FILETYPE_PEM, 0);
	CHECK_QSTATUS();
	
	*ppCertificate = pCertificate.release();
	
	return QSTATUS_SUCCESS;
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
