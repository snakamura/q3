/*
 * $Id$
 *
 * Copyright(C) 1998-2003 Satoshi Nakamura
 * All rights reserved.
 *
 */

#include <qmapplication.h>
#include <qmextensions.h>

#include <qsconv.h>
#include <qserror.h>
#include <qsnew.h>
#include <qsstl.h>

#include <algorithm>

#include "fixedformtext.h"

using namespace qm;
using namespace qs;


/****************************************************************************
 *
 * FixedFormTextManager
 *
 */

qm::FixedFormTextManager::FixedFormTextManager(QSTATUS* pstatus)
{
	DECLARE_QSTATUS();
	
	status = load();
	CHECK_QSTATUS_SET(pstatus);
}

qm::FixedFormTextManager::~FixedFormTextManager()
{
	std::for_each(listText_.begin(),
		listText_.end(), deleter<FixedFormText>());
}

const FixedFormTextManager::TextList& qm::FixedFormTextManager::getTextList() const
{
	return listText_;
}

QSTATUS qm::FixedFormTextManager::addText(FixedFormText* pText)
{
	return STLWrapper<TextList>(listText_).push_back(pText);
}

QSTATUS qm::FixedFormTextManager::load()
{
	DECLARE_QSTATUS();
	
	string_ptr<WSTRING> wstrPath;
	status = Application::getApplication().getProfilePath(
		Extensions::TEXTS, &wstrPath);
	CHECK_QSTATUS();
	
	W2T(wstrPath.get(), ptszPath);
	if (::GetFileAttributes(ptszPath) != 0xffffffff) {
		XMLReader reader(&status);
		CHECK_QSTATUS();
		FixedFormTextContentHandler handler(this, &status);
		CHECK_QSTATUS();
		reader.setContentHandler(&handler);
		status = reader.parse(wstrPath.get());
		CHECK_QSTATUS();
	}
	
	return QSTATUS_SUCCESS;
}


/****************************************************************************
 *
 * FixedFormText
 *
 */

qm::FixedFormText::FixedFormText(const WCHAR* pwszName, qs::QSTATUS* pstatus) :
	wstrName_(0),
	wstrText_(0)
{
	assert(pwszName);
	
	wstrName_ = allocWString(pwszName);
	if (!wstrName_) {
		*pstatus = QSTATUS_OUTOFMEMORY;
		return;
	}
}

qm::FixedFormText::~FixedFormText()
{
	freeWString(wstrName_);
	freeWString(wstrText_);
}

const WCHAR* qm::FixedFormText::getName() const
{
	return wstrName_;
}

const WCHAR* qm::FixedFormText::getText() const
{
	return wstrText_;
}

void qm::FixedFormText::setText(qs::WSTRING wstrText)
{
	wstrText_ = wstrText;
}


/****************************************************************************
 *
 * FixedFormTextContentHandler
 *
 */

qm::FixedFormTextContentHandler::FixedFormTextContentHandler(
	FixedFormTextManager* pManager, QSTATUS* pstatus) :
	DefaultHandler(pstatus),
	pManager_(pManager),
	state_(STATE_ROOT),
	pText_(0),
	pBuffer_(0)
{
	DECLARE_QSTATUS();
	
	status = newQsObject(&pBuffer_);
	CHECK_QSTATUS_SET(pstatus);
}

qm::FixedFormTextContentHandler::~FixedFormTextContentHandler()
{
	delete pBuffer_;
}

QSTATUS qm::FixedFormTextContentHandler::startElement(
	const WCHAR* pwszNamespaceURI, const WCHAR* pwszLocalName,
	const WCHAR* pwszQName, const qs::Attributes& attributes)
{
	DECLARE_QSTATUS();
	
	if (wcscmp(pwszLocalName, L"texts") == 0) {
		if (state_ != STATE_ROOT)
			return QSTATUS_FAIL;
		
		if (attributes.getLength() != 0)
			return QSTATUS_FAIL;
		
		state_ = STATE_TEXTS;
	}
	else if (wcscmp(pwszLocalName, L"text") == 0) {
		if (state_ != STATE_TEXTS)
			return QSTATUS_FAIL;
		
		const WCHAR* pwszName = 0;
		for (int n = 0; n < attributes.getLength(); ++n) {
			const WCHAR* pwszAttrName = attributes.getLocalName(n);
			if (wcscmp(pwszAttrName, L"name") == 0)
				pwszName = attributes.getValue(n);
			else
				return QSTATUS_FAIL;
		}
		if (!pwszName)
			return QSTATUS_FAIL;
		
		assert(!pText_);
		std::auto_ptr<FixedFormText> pText;
		status = newQsObject(pwszName, &pText);
		CHECK_QSTATUS();
		status = pManager_->addText(pText.get());
		CHECK_QSTATUS();
		pText_ = pText.release();
		
		state_ = STATE_TEXT;
	}
	else {
		return QSTATUS_FAIL;
	}
	
	return QSTATUS_SUCCESS;
}

QSTATUS qm::FixedFormTextContentHandler::endElement(
	const WCHAR* pwszNamespaceURI, const WCHAR* pwszLocalName,
	const WCHAR* pwszQName)
{
	DECLARE_QSTATUS();
	
	if (wcscmp(pwszLocalName, L"texts") == 0) {
		assert(state_ == STATE_TEXTS);
		state_ = STATE_ROOT;
	}
	else if (wcscmp(pwszLocalName, L"text") == 0) {
		assert(state_ == STATE_TEXT);
		
		assert(pText_);
		pText_->setText(pBuffer_->getString());
		pText_ = 0;
		
		state_ = STATE_TEXTS;
	}
	else {
		return QSTATUS_FAIL;
	}
	
	return QSTATUS_SUCCESS;
}

QSTATUS qm::FixedFormTextContentHandler::characters(
	const WCHAR* pwsz, size_t nStart, size_t nLength)
{
	DECLARE_QSTATUS();
	
	if (state_ == STATE_TEXT) {
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
