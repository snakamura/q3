/*
 * $Id$
 *
 * Copyright(C) 1998-2004 Satoshi Nakamura
 * All rights reserved.
 *
 */

#include <qmaccount.h>
#include <qmapplication.h>
#include <qmfilenames.h>

#include <qsassert.h>
#include <qsconv.h>
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

qm::SignatureManager::SignatureManager()
{
	SYSTEMTIME st;
	::GetSystemTime(&st);
	::SystemTimeToFileTime(&st, &ft_);
}

qm::SignatureManager::~SignatureManager()
{
	clear();
}

void qm::SignatureManager::getSignatures(Account* pAccount,
										 SignatureList* pList)
{
	assert(pAccount);
	assert(pList);
	
	if (!load())
		return;
	
	for (SignatureList::const_iterator it = listSignature_.begin(); it != listSignature_.end(); ++it) {
		Signature* pSignature = *it;
		if (pSignature->match(pAccount))
			pList->push_back(pSignature);
	}
}

const Signature* qm::SignatureManager::getSignature(Account* pAccount,
													const WCHAR* pwszName)
{
	assert(pAccount);
	
	if (!load())
		return 0;
	
	for (SignatureList::const_iterator it = listSignature_.begin(); it != listSignature_.end(); ++it) {
		Signature* pSignature = *it;
		if (pSignature->match(pAccount)) {
			if ((!pSignature->getName() && !pwszName) ||
				wcscmp((*it)->getName(), pwszName) == 0)
				return pSignature;
		}
	}
	
	return 0;
}

const Signature* qm::SignatureManager::getDefaultSignature(Account* pAccount)
{
	assert(pAccount);
	
	if (!load())
		return 0;
	
	for (SignatureList::const_iterator it = listSignature_.begin(); it != listSignature_.end(); ++it) {
		Signature* pSignature = *it;
		if (pSignature->match(pAccount)) {
			if ((*it)->isDefault())
				return pSignature;
		}
	}
	
	return 0;
}

void qm::SignatureManager::addSignature(std::auto_ptr<Signature> pSignature)
{
	listSignature_.push_back(pSignature.get());
	pSignature.release();
}

bool qm::SignatureManager::load()
{
	wstring_ptr wstrPath(Application::getApplication().getProfilePath(FileNames::SIGNATURES_XML));
	
	W2T(wstrPath.get(), ptszPath);
	AutoHandle hFile(::CreateFile(ptszPath, GENERIC_READ, 0, 0,
		OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0));
	if (hFile.get()) {
		FILETIME ft;
		::GetFileTime(hFile.get(), 0, 0, &ft);
		hFile.close();
		
		if (::CompareFileTime(&ft, &ft_) != 0) {
			clear();
			
			XMLReader reader;
			SignatureContentHandler handler(this);
			reader.setContentHandler(&handler);
			if (!reader.parse(wstrPath.get()))
				return false;
			
			ft_ = ft;
		}
	}
	else {
		clear();
	}
	
	return true;
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

qm::Signature::Signature(std::auto_ptr<RegexPattern> pAccountName,
						 wstring_ptr wstrName,
						 bool bDefault,
						 wstring_ptr wstrSignature) :
	pAccountName_(pAccountName),
	wstrName_(wstrName),
	bDefault_(bDefault),
	wstrSignature_(wstrSignature)
{
	assert(wstrName_.get());
	assert(wstrSignature_.get());
}

qm::Signature::~Signature()
{
}

bool qm::Signature::match(Account* pAccount) const
{
	assert(pAccount);
	
	if (pAccountName_.get())
		return pAccountName_->match(pAccount->getName());
	else
		return true;
}

const WCHAR* qm::Signature::getName() const
{
	return wstrName_.get();
}

bool qm::Signature::isDefault() const
{
	return bDefault_;
}

const WCHAR* qm::Signature::getSignature() const
{
	return wstrSignature_.get();
}


/****************************************************************************
 *
 * SignatureContentHandler
 *
 */

qm::SignatureContentHandler::SignatureContentHandler(SignatureManager* pManager) :
	pManager_(pManager),
	state_(STATE_ROOT),
	bDefault_(false)
{
}

qm::SignatureContentHandler::~SignatureContentHandler()
{
}

bool qm::SignatureContentHandler::startElement(const WCHAR* pwszNamespaceURI,
											   const WCHAR* pwszLocalName,
											   const WCHAR* pwszQName,
											   const Attributes& attributes)
{
	if (wcscmp(pwszLocalName, L"signatures") == 0) {
		if (state_ != STATE_ROOT)
			return false;
		if (attributes.getLength() != 0)
			return false;
		state_ = STATE_SIGNATURES;
	}
	else if (wcscmp(pwszLocalName, L"signature") == 0) {
		if (state_ != STATE_SIGNATURES)
			return false;
		
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
				return false;
		}
		
		assert(!wstrName_.get());
		wstrName_ = allocWString(pwszName);
		
		assert(!pAccountName_.get());
		if (pwszAccount) {
			pAccountName_ = RegexCompiler().compile(pwszAccount);
			if (!pAccountName_.get())
				return false;
		}
		
		state_ = STATE_SIGNATURE;
	}
	else {
		return false;
	}
	
	return true;
}

bool qm::SignatureContentHandler::endElement(const WCHAR* pwszNamespaceURI,
											 const WCHAR* pwszLocalName,
											 const WCHAR* pwszQName)
{
	if (wcscmp(pwszLocalName, L"signatures") == 0) {
		assert(state_ == STATE_SIGNATURES);
		state_ = STATE_ROOT;
	}
	else if (wcscmp(pwszLocalName, L"signature") == 0) {
		assert(state_ == STATE_SIGNATURE);
		
		wstring_ptr wstrSignature(buffer_.getString());
		std::auto_ptr<Signature> pSignature(new Signature(
			pAccountName_, wstrName_, bDefault_, wstrSignature));
		
		pManager_->addSignature(pSignature);
		
		state_ = STATE_SIGNATURES;
	}
	else {
		return false;
	}
	
	return true;
}

bool qm::SignatureContentHandler::characters(const WCHAR* pwsz,
											 size_t nStart,
											 size_t nLength)
{
	if (state_ == STATE_SIGNATURE) {
		buffer_.append(pwsz + nStart, nLength);
	}
	else {
		const WCHAR* p = pwsz + nStart;
		for (size_t n = 0; n < nLength; ++n, ++p) {
			if (*p != L' ' && *p != L'\t' && *p != '\n')
				return false;
		}
	}
	
	return true;
}
