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
#include <qsfile.h>
#include <qsosutil.h>
#include <qsstream.h>

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

const SignatureManager::SignatureList& qm::SignatureManager::getSignatures()
{
	load();
	return listSignature_;
}

void qm::SignatureManager::setSignatures(SignatureList& listSignature)
{
	clear();
	listSignature_.swap(listSignature);
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

bool qm::SignatureManager::save() const
{
	wstring_ptr wstrPath(Application::getApplication().getProfilePath(FileNames::SIGNATURES_XML));
	
	TemporaryFileRenamer renamer(wstrPath.get());
	
	FileOutputStream os(renamer.getPath());
	if (!os)
		return false;
	OutputStreamWriter writer(&os, false, L"utf-8");
	if (!writer)
		return false;
	BufferedWriter bufferedWriter(&writer, false);
	
	SignatureWriter signatureWriter(&bufferedWriter);
	if (!signatureWriter.write(listSignature_))
		return false;
	
	if (!bufferedWriter.close())
		return false;
	
	if (!renamer.rename())
		return false;
	
	return true;
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
		
		SYSTEMTIME st;
		::GetSystemTime(&st);
		::SystemTimeToFileTime(&st, &ft_);
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

qm::Signature::Signature() :
	bDefault_(false)
{
	wstrName_ = allocWString(L"");
	wstrSignature_ = allocWString(L"");
}

qm::Signature::Signature(const WCHAR* pwszAccount,
						 std::auto_ptr<RegexPattern> pAccount,
						 const WCHAR* pwszName,
						 bool bDefault,
						 const WCHAR* pwszSignature) :
	pAccount_(pAccount),
	bDefault_(bDefault)
{
	assert(pwszName);
	assert(pwszSignature);
	
	if (pwszAccount)
		wstrAccount_ = allocWString(pwszAccount);
	wstrName_ = allocWString(pwszName);
	wstrSignature_ = allocWString(pwszSignature);
}

qm::Signature::Signature(const Signature& signature) :
	bDefault_(signature.bDefault_)
{
	if (signature.wstrAccount_.get()) {
		wstrAccount_ = allocWString(signature.wstrAccount_.get());
		pAccount_ = RegexCompiler().compile(wstrAccount_.get());
		assert(pAccount_.get());
	}
	wstrName_ = allocWString(signature.wstrName_.get());
	wstrSignature_ = allocWString(signature.wstrSignature_.get());
}

qm::Signature::~Signature()
{
}

const WCHAR* qm::Signature::getAccount() const
{
	return wstrAccount_.get();
}

void qm::Signature::setAccount(const WCHAR* pwszAccount,
							   std::auto_ptr<RegexPattern> pAccount)
{
	assert((pwszAccount && pAccount.get()) || (!pwszAccount && !pAccount.get()));
	
	if (pwszAccount)
		wstrAccount_ = allocWString(pwszAccount);
	else
		wstrAccount_.reset(0);
	pAccount_ = pAccount;
}

bool qm::Signature::match(Account* pAccount) const
{
	assert(pAccount);
	
	if (pAccount_.get())
		return pAccount_->match(pAccount->getName());
	else
		return true;
}

const WCHAR* qm::Signature::getName() const
{
	return wstrName_.get();
}

void qm::Signature::setName(const WCHAR* pwszName)
{
	wstrName_ = allocWString(pwszName);
}

bool qm::Signature::isDefault() const
{
	return bDefault_;
}

void qm::Signature::setDefault(bool bDefault)
{
	bDefault_ = bDefault;
}

const WCHAR* qm::Signature::getSignature() const
{
	return wstrSignature_.get();
}

void qm::Signature::setSignature(const WCHAR* pwszSignature)
{
	wstrSignature_ = allocWString(pwszSignature);
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
		
		assert(!wstrAccount_.get());
		assert(!pAccount_.get());
		if (pwszAccount) {
			pAccount_ = RegexCompiler().compile(pwszAccount);
			if (!pAccount_.get())
				return false;
			wstrAccount_ = allocWString(pwszAccount);
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
		
		std::auto_ptr<Signature> pSignature(new Signature(wstrAccount_.get(),
			pAccount_, wstrName_.get(), bDefault_, buffer_.getCharArray()));
		
		wstrAccount_.reset(0);
		wstrName_.reset(0);
		buffer_.remove();
		
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


/****************************************************************************
 *
 * SignatureWriter
 *
 */

qm::SignatureWriter::SignatureWriter(Writer* pWriter) :
	handler_(pWriter)
{
}

qm::SignatureWriter::~SignatureWriter()
{
}

bool qm::SignatureWriter::write(const SignatureManager::SignatureList& listSignature)
{
	if (!handler_.startDocument())
		return false;
	if (!handler_.startElement(0, 0, L"signatures", DefaultAttributes()))
		return false;
	
	for (SignatureManager::SignatureList::const_iterator it = listSignature.begin(); it != listSignature.end(); ++it) {
		const Signature* pSignature = *it;
		if (!write(pSignature))
			return false;
	}
	
	if (!handler_.endElement(0, 0, L"signatures"))
		return false;
	if (!handler_.endDocument())
		return false;
	
	return true;
}

bool qm::SignatureWriter::write(const Signature* pSignature)
{
	const WCHAR* pwszAccount = pSignature->getAccount();
	bool bDefault = pSignature->isDefault();
	const SimpleAttributes::Item items[] = {
		{ L"name",		pSignature->getName()								},
		{ L"account",	pwszAccount,					pwszAccount == 0	},
		{ L"default",	bDefault ? L"true" : L"false",	!bDefault			}
	};
	SimpleAttributes attrs(items, countof(items));
	if (!handler_.startElement(0, 0, L"signature", attrs))
		return false;
	
	const WCHAR* pwszSignature = pSignature->getSignature();
	if (!handler_.characters(pwszSignature, 0, wcslen(pwszSignature)))
		return false;
	
	if (!handler_.endElement(0, 0, L"signature"))
		return false;
	
	return true;
}
