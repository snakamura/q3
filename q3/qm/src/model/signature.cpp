/*
 * $Id$
 *
 * Copyright(C) 1998-2003 Satoshi Nakamura
 * All rights reserved.
 *
 */

#include <qmaccount.h>
#include <qmapplication.h>
#include <qmextensions.h>

#include <qsassert.h>
#include <qsconv.h>
#include <qserror.h>
#include <qsnew.h>
#include <qsosutil.h>

#include <algorithm>

#include "signature.h"

using namespace qm;
using namespace qs;


/****************************************************************************
 *
 * SignatureManager
 *
 */

qm::SignatureManager::SignatureManager(QSTATUS* pstatus)
{
	SYSTEMTIME st;
	::GetSystemTime(&st);
	::SystemTimeToFileTime(&st, &ft_);
}

qm::SignatureManager::~SignatureManager()
{
	clear();
}

QSTATUS qm::SignatureManager::getSignatures(
	Account* pAccount, SignatureList* pList)
{
	assert(pAccount);
	assert(pList);
	
	DECLARE_QSTATUS();
	
	status = load();
	CHECK_QSTATUS();
	
	SignatureList::const_iterator it = listSignature_.begin();
	while (it != listSignature_.end()) {
		bool bMatch = false;
		status = (*it)->match(pAccount, &bMatch);
		CHECK_QSTATUS();
		if (bMatch) {
			status = STLWrapper<SignatureList>(*pList).push_back(*it);
			CHECK_QSTATUS();
		}
		++it;
	}
	
	return QSTATUS_SUCCESS;
}

QSTATUS qm::SignatureManager::getSignature(Account* pAccount,
	const WCHAR* pwszName, const Signature** ppSignature)
{
	assert(pAccount);
	assert(ppSignature);
	
	DECLARE_QSTATUS();
	
	*ppSignature = 0;
	
	status = load();
	CHECK_QSTATUS();
	
	SignatureList::const_iterator it = listSignature_.begin();
	while (it != listSignature_.end() && !*ppSignature) {
		bool bMatch = false;
		status = (*it)->match(pAccount, &bMatch);
		CHECK_QSTATUS();
		if (bMatch) {
			if ((!(*it)->getName() && !pwszName) ||
				wcscmp((*it)->getName(), pwszName) == 0)
				*ppSignature = *it;
		}
		++it;
	}
	
	return QSTATUS_SUCCESS;
}

QSTATUS qm::SignatureManager::getDefaultSignature(
	Account* pAccount, const Signature** ppSignature)
{
	assert(pAccount);
	assert(ppSignature);
	
	DECLARE_QSTATUS();
	
	*ppSignature = 0;
	
	status = load();
	CHECK_QSTATUS();
	
	SignatureList::const_iterator it = listSignature_.begin();
	while (it != listSignature_.end() && !*ppSignature) {
		bool bMatch = false;
		status = (*it)->match(pAccount, &bMatch);
		CHECK_QSTATUS();
		if (bMatch) {
			if ((*it)->isDefault())
				*ppSignature = *it;
		}
		++it;
	}
	
	return QSTATUS_SUCCESS;
}

QSTATUS qm::SignatureManager::addSignature(Signature* pSignature)
{
	return STLWrapper<SignatureList>(listSignature_).push_back(pSignature);
}

QSTATUS qm::SignatureManager::load()
{
	DECLARE_QSTATUS();
	
	string_ptr<WSTRING> wstrPath;
	status = Application::getApplication().getProfilePath(
		Extensions::SIGNATURES, &wstrPath);
	CHECK_QSTATUS();
	
	W2T(wstrPath.get(), ptszPath);
	AutoHandle hFile(::CreateFile(ptszPath, GENERIC_READ, 0, 0,
		OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0));
	if (hFile.get()) {
		FILETIME ft;
		::GetFileTime(hFile.get(), 0, 0, &ft);
		hFile.close();
		
		if (::CompareFileTime(&ft, &ft_) != 0) {
			clear();
			
			XMLReader reader(&status);
			CHECK_QSTATUS();
			SignatureContentHandler handler(this, &status);
			CHECK_QSTATUS();
			reader.setContentHandler(&handler);
			status = reader.parse(wstrPath.get());
			CHECK_QSTATUS();
			
			ft_ = ft;
		}
	}
	else {
		clear();
	}
	
	return QSTATUS_SUCCESS;
}

void qm::SignatureManager::clear()
{
	std::for_each(listSignature_.begin(),
		listSignature_.end(), deleter<Signature>());
	listSignature_.clear();
}


/****************************************************************************
 *
 * Signature
 *
 */

qm::Signature::Signature(RegexPattern* pAccountName, WSTRING wstrName,
	bool bDefault, WSTRING wstrSignature, QSTATUS* pstatus) :
	pAccountName_(pAccountName),
	wstrName_(wstrName),
	bDefault_(bDefault),
	wstrSignature_(wstrSignature)
{
	assert(wstrName);
	assert(wstrSignature);
}

qm::Signature::~Signature()
{
	delete pAccountName_;
	freeWString(wstrName_);
	freeWString(wstrSignature_);
}

QSTATUS qm::Signature::match(Account* pAccount, bool* pbMatch) const
{
	assert(pAccount);
	assert(pbMatch);
	
	DECLARE_QSTATUS();
	
	if (pAccountName_) {
		status = pAccountName_->match(pAccount->getName(), pbMatch);
		CHECK_QSTATUS();
	}
	else {
		*pbMatch = true;
	}
	
	return QSTATUS_SUCCESS;
}

const WCHAR* qm::Signature::getName() const
{
	return wstrName_;
}

bool qm::Signature::isDefault() const
{
	return bDefault_;
}

const WCHAR* qm::Signature::getSignature() const
{
	return wstrSignature_;
}


/****************************************************************************
 *
 * SignatureContentHandler
 *
 */

qm::SignatureContentHandler::SignatureContentHandler(
	SignatureManager* pManager, QSTATUS* pstatus) :
	DefaultHandler(pstatus),
	pManager_(pManager),
	state_(STATE_ROOT),
	pAccountName_(0),
	wstrName_(0),
	bDefault_(false),
	pBuffer_(0)
{
	DECLARE_QSTATUS();
	
	status = newQsObject(&pBuffer_);
	CHECK_QSTATUS_SET(pstatus);
}

qm::SignatureContentHandler::~SignatureContentHandler()
{
	freeWString(wstrName_);
	delete pAccountName_;
	delete pBuffer_;
}

QSTATUS qm::SignatureContentHandler::startElement(
	const WCHAR* pwszNamespaceURI, const WCHAR* pwszLocalName,
	const WCHAR* pwszQName, const Attributes& attributes)
{
	DECLARE_QSTATUS();
	
	if (wcscmp(pwszLocalName, L"signatures") == 0) {
		if (state_ != STATE_ROOT)
			return QSTATUS_FAIL;
		
		if (attributes.getLength() != 0)
			return QSTATUS_FAIL;
		
		state_ = STATE_SIGNATURES;
	}
	else if (wcscmp(pwszLocalName, L"signature") == 0) {
		if (state_ != STATE_SIGNATURES)
			return QSTATUS_FAIL;
		
		bDefault_ = false;
		const WCHAR* pwszName = 0;
		const WCHAR* pwszAccount = 0;
		for (int n = 0; n < attributes.getLength(); ++n) {
			const WCHAR* pwszAttrName = attributes.getLocalName(n);
			if (wcscmp(pwszAttrName, L"name") == 0)
				pwszName = attributes.getValue(n);
			else if (wcscmp(pwszAttrName, L"account") == 0)
				pwszAccount = attributes.getValue(n);
			else if (wcscmp(pwszAttrName, L"default") == 0)
				bDefault_ = wcscmp(attributes.getValue(n), L"true") == 0;
			else
				return QSTATUS_FAIL;
		}
		
		assert(!wstrName_);
		wstrName_ = allocWString(pwszName);
		if (!wstrName_)
			return QSTATUS_OUTOFMEMORY;
		
		assert(!pAccountName_);
		if (pwszAccount) {
			RegexCompiler compiler;
			RegexPattern* p = 0;
			status = compiler.compile(pwszAccount, &p);
			CHECK_QSTATUS();
			pAccountName_ = p;
		}
		
		state_ = STATE_SIGNATURE;
	}
	else {
		return QSTATUS_FAIL;
	}
	
	return QSTATUS_SUCCESS;
}

QSTATUS qm::SignatureContentHandler::endElement(
	const WCHAR* pwszNamespaceURI, const WCHAR* pwszLocalName,
	const WCHAR* pwszQName)
{
	DECLARE_QSTATUS();
	
	if (wcscmp(pwszLocalName, L"signatures") == 0) {
		assert(state_ == STATE_SIGNATURES);
		state_ = STATE_ROOT;
	}
	else if (wcscmp(pwszLocalName, L"signature") == 0) {
		assert(state_ == STATE_SIGNATURE);
		
		string_ptr<WSTRING> wstrSignature(pBuffer_->getString());
		std::auto_ptr<Signature> pSignature;
		status = newQsObject(pAccountName_, wstrName_,
			bDefault_, wstrSignature.get(), &pSignature);
		CHECK_QSTATUS();
		wstrName_ = 0;
		wstrSignature.release();
		pAccountName_ = 0;
		
		status = pManager_->addSignature(pSignature.get());
		CHECK_QSTATUS();
		pSignature.release();
		
		state_ = STATE_SIGNATURES;
	}
	else {
		return QSTATUS_FAIL;
	}
	
	return QSTATUS_SUCCESS;
}

QSTATUS qm::SignatureContentHandler::characters(
	const WCHAR* pwsz, size_t nStart, size_t nLength)
{
	DECLARE_QSTATUS();
	
	if (state_ == STATE_SIGNATURE) {
		status = pBuffer_->append(pwsz + nStart, nLength);
		CHECK_QSTATUS();
	}
	else {
		const WCHAR* p = pwsz + nStart;
		for (size_t n = 0; n < nLength; ++n, ++p) {
			if (*p != L' ' && *p != L'\t' && *p != '\n')
				return QSTATUS_FAIL;
		}
	}
	
	return QSTATUS_SUCCESS;
}
