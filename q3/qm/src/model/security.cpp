/*
 * $Id: security.cpp,v 1.1.1.1 2003/04/29 08:07:31 snakamura Exp $
 *
 * Copyright(C) 1998-2003 Satoshi Nakamura
 * All rights reserved.
 *
 */

#include <qsconv.h>

#include <tchar.h>

#include "security.h"

using namespace qm;
using namespace qs;


/****************************************************************************
 *
 * Security
 *
 */

HINSTANCE qm::Security::hInst__ = 0;

qm::Security::Security(const WCHAR* pwszPath, QSTATUS* pstatus) :
	wstrPath_(0),
	pStoreCA_(0),
	pSMIMEUtility_(0)
{
	DECLARE_QSTATUS();
	
	if (hInst__) {
		wstrPath_ = concat(pwszPath, L"\\security");
		if (!wstrPath_) {
			*pstatus = QSTATUS_FAIL;
			return;
		}
		
		status = Store::getInstance(&pStoreCA_);
		CHECK_QSTATUS_SET(pstatus);
		string_ptr<WSTRING> wstrCAPath(
			concat(wstrPath_, L"\\ca.pem"));
		if (!wstrCAPath.get()) {
			*pstatus = QSTATUS_FAIL;
			return;
		}
		
		W2T_STATUS(wstrCAPath.get(), ptszCAPath);
		CHECK_QSTATUS_SET(pstatus);
		if (::GetFileAttributes(ptszCAPath) != 0xffffffff) {
			status = pStoreCA_->load(wstrCAPath.get(), Store::FILETYPE_PEM);
			CHECK_QSTATUS_SET(pstatus);
		}
		
		status = SMIMEUtility::getInstance(&pSMIMEUtility_);
		CHECK_QSTATUS_SET(pstatus);
	}
}

qm::Security::~Security()
{
	delete pSMIMEUtility_;
	delete pStoreCA_;
	freeWString(wstrPath_);
}

const Store* qm::Security::getCA() const
{
	return pStoreCA_;
}

const SMIMEUtility* qm::Security::getSMIMEUtility() const
{
	return pSMIMEUtility_;
}

QSTATUS qm::Security::getCertificate(
	const WCHAR* pwszName, Certificate** ppCertificate) const
{
	assert(pwszName);
	assert(ppCertificate);
	
	DECLARE_QSTATUS();
	
	ConcatW c[] = {
		{ wstrPath_,	-1	},
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
	hInst__ = ::LoadLibrary(_T("qscrypto") SUFFIX _T(".dll"));
}

void qm::Security::term()
{
	if (hInst__)
		::FreeLibrary(hInst__);
}

bool qm::Security::isEnabled()
{
	return hInst__ != 0;
}
