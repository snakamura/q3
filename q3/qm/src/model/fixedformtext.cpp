/*
 * $Id$
 *
 * Copyright(C) 1998-2004 Satoshi Nakamura
 * All rights reserved.
 *
 */

#include <qmapplication.h>
#include <qmfilenames.h>

#include <qsconv.h>
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

qm::FixedFormTextManager::FixedFormTextManager()
{
	load();
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

void qm::FixedFormTextManager::addText(std::auto_ptr<FixedFormText> pText)
{
	listText_.push_back(pText.get());
	pText.release();
}

bool qm::FixedFormTextManager::load()
{
	wstring_ptr wstrPath(Application::getApplication().getProfilePath(FileNames::TEXTS_XML));
	
	W2T(wstrPath.get(), ptszPath);
	if (::GetFileAttributes(ptszPath) != 0xffffffff) {
		XMLReader reader;
		FixedFormTextContentHandler handler(this);
		reader.setContentHandler(&handler);
		if (!reader.parse(wstrPath.get()))
			return false;
	}
	
	return true;
}


/****************************************************************************
 *
 * FixedFormText
 *
 */

qm::FixedFormText::FixedFormText(const WCHAR* pwszName)
{
	wstrName_ = allocWString(pwszName);
}

qm::FixedFormText::~FixedFormText()
{
}

const WCHAR* qm::FixedFormText::getName() const
{
	return wstrName_.get();
}

const WCHAR* qm::FixedFormText::getText() const
{
	return wstrText_.get();
}

void qm::FixedFormText::setText(wstring_ptr wstrText)
{
	wstrText_ = wstrText;
}


/****************************************************************************
 *
 * FixedFormTextContentHandler
 *
 */

qm::FixedFormTextContentHandler::FixedFormTextContentHandler(FixedFormTextManager* pManager) :
	pManager_(pManager),
	state_(STATE_ROOT),
	pText_(0)
{
}

qm::FixedFormTextContentHandler::~FixedFormTextContentHandler()
{
}

bool qm::FixedFormTextContentHandler::startElement(const WCHAR* pwszNamespaceURI,
												   const WCHAR* pwszLocalName,
												   const WCHAR* pwszQName,
												   const Attributes& attributes)
{
	if (wcscmp(pwszLocalName, L"texts") == 0) {
		if (state_ != STATE_ROOT)
			return false;
		if (attributes.getLength() != 0)
			return false;
		state_ = STATE_TEXTS;
	}
	else if (wcscmp(pwszLocalName, L"text") == 0) {
		if (state_ != STATE_TEXTS)
			return false;
		
		const WCHAR* pwszName = 0;
		for (int n = 0; n < attributes.getLength(); ++n) {
			const WCHAR* pwszAttrName = attributes.getLocalName(n);
			if (wcscmp(pwszAttrName, L"name") == 0)
				pwszName = attributes.getValue(n);
			else
				return false;
		}
		if (!pwszName)
			return false;
		
		assert(!pText_);
		std::auto_ptr<FixedFormText> pText(new FixedFormText(pwszName));
		pText_ = pText.get();
		pManager_->addText(pText);
		
		state_ = STATE_TEXT;
	}
	else {
		return false;
	}
	
	return true;
}

bool qm::FixedFormTextContentHandler::endElement(const WCHAR* pwszNamespaceURI,
												 const WCHAR* pwszLocalName,
												 const WCHAR* pwszQName)
{
	if (wcscmp(pwszLocalName, L"texts") == 0) {
		assert(state_ == STATE_TEXTS);
		state_ = STATE_ROOT;
	}
	else if (wcscmp(pwszLocalName, L"text") == 0) {
		assert(state_ == STATE_TEXT);
		
		assert(pText_);
		pText_->setText(buffer_.getString());
		pText_ = 0;
		
		state_ = STATE_TEXTS;
	}
	else {
		return false;
	}
	
	return true;
}

bool qm::FixedFormTextContentHandler::characters(const WCHAR* pwsz,
												 size_t nStart,
												 size_t nLength)
{
	if (state_ == STATE_TEXT) {
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
