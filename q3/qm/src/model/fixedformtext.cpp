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
#include <qsfile.h>
#include <qsosutil.h>
#include <qsstl.h>
#include <qsstream.h>

#include <algorithm>

#include "fixedformtext.h"

using namespace qm;
using namespace qs;


/****************************************************************************
 *
 * FixedFormTextManager
 *
 */

qm::FixedFormTextManager::FixedFormTextManager() :
	helper_(Application::getApplication().getProfilePath(FileNames::TEXTS_XML).get())
{
}

qm::FixedFormTextManager::~FixedFormTextManager()
{
	clear();
}

const FixedFormTextManager::TextList& qm::FixedFormTextManager::getTexts()
{
	return getTexts(true);
}

const FixedFormTextManager::TextList& qm::FixedFormTextManager::getTexts(bool bReload)
{
	if (bReload)
		load();
	return listText_;
}

void qm::FixedFormTextManager::setTexts(TextList& listText)
{
	clear();
	listText_.swap(listText);
}

void qm::FixedFormTextManager::addText(std::auto_ptr<FixedFormText> pText)
{
	listText_.push_back(pText.get());
	pText.release();
}

void qm::FixedFormTextManager::clear()
{
	std::for_each(listText_.begin(),
		listText_.end(), deleter<FixedFormText>());
	listText_.clear();
}

bool qm::FixedFormTextManager::save() const
{
	return helper_.save(this);
}

bool qm::FixedFormTextManager::load()
{
	FixedFormTextContentHandler handler(this);
	return helper_.load(this, &handler);
}


/****************************************************************************
 *
 * FixedFormText
 *
 */

qm::FixedFormText::FixedFormText()
{
	wstrName_ = allocWString(L"");
	wstrText_ = allocWString(L"");
}

qm::FixedFormText::FixedFormText(const WCHAR* pwszName)
{
	wstrName_ = allocWString(pwszName);
}

qm::FixedFormText::FixedFormText(const FixedFormText& text)
{
	wstrName_ = allocWString(text.wstrName_.get());
	wstrText_ = allocWString(text.wstrText_.get());
}

qm::FixedFormText::~FixedFormText()
{
}

const WCHAR* qm::FixedFormText::getName() const
{
	return wstrName_.get();
}

void qm::FixedFormText::setName(const WCHAR* pwszName)
{
	wstrName_ = allocWString(pwszName);
}

const WCHAR* qm::FixedFormText::getText() const
{
	return wstrText_.get();
}

void qm::FixedFormText::setText(const WCHAR* pwszText)
{
	wstrText_ = allocWString(pwszText);
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
		pText_->setText(buffer_.getCharArray());
		buffer_.remove();
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


/****************************************************************************
 *
 * FixedFormTextWriter
 *
 */

qm::FixedFormTextWriter::FixedFormTextWriter(Writer* pWriter) :
	handler_(pWriter)
{
}

qm::FixedFormTextWriter::~FixedFormTextWriter()
{
}

bool qm::FixedFormTextWriter::write(const FixedFormTextManager* pManager)
{
	if (!handler_.startDocument())
		return false;
	if (!handler_.startElement(0, 0, L"texts", DefaultAttributes()))
		return false;
	
	const FixedFormTextManager::TextList& l =
		const_cast<FixedFormTextManager*>(pManager)->getTexts(false);
	for (FixedFormTextManager::TextList::const_iterator it = l.begin(); it != l.end(); ++it) {
		const FixedFormText* pText = *it;
		if (!write(pText))
			return false;
	}
	
	if (!handler_.endElement(0, 0, L"texts"))
		return false;
	if (!handler_.endDocument())
		return false;
	
	return true;
}

bool qm::FixedFormTextWriter::write(const FixedFormText* pText)
{
	SimpleAttributes attrs(L"name", pText->getName());
	if (!handler_.startElement(0, 0, L"text", attrs))
		return false;
	
	const WCHAR* pwszText = pText->getText();
	if (!handler_.characters(pwszText, 0, wcslen(pwszText)))
		return false;
	
	if (!handler_.endElement(0, 0, L"text"))
		return false;
	
	return true;
}
