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
#include <qmfolder.h>
#include <qmmacro.h>

#include <qsconv.h>
#include <qsutil.h>

#include <algorithm>

#include "color.h"

using namespace qm;
using namespace qs;

/****************************************************************************
 *
 * ColorManager
 *
 */

qm::ColorManager::ColorManager()
{
	load();
}

qm::ColorManager::~ColorManager()
{
	std::for_each(listColorSet_.begin(),
		listColorSet_.end(), deleter<ColorSet>());
}

const ColorSet* qm::ColorManager::getColorSet(Folder* pFolder) const
{
	assert(pFolder);
	
	for (ColorSetList::const_iterator it = listColorSet_.begin(); it != listColorSet_.end(); ++it) {
		const ColorSet* pColor = *it;
		if (pColor->match(pFolder))
			return pColor;
	}
	
	return 0;
}

void qm::ColorManager::addColorSet(std::auto_ptr<ColorSet> pSet)
{
	listColorSet_.push_back(pSet.get());
	pSet.release();
}

bool qm::ColorManager::load()
{
	wstring_ptr wstrPath(
		Application::getApplication().getProfilePath(FileNames::COLORS_XML));
	
	W2T(wstrPath.get(), ptszPath);
	if (::GetFileAttributes(ptszPath) != 0xffffffff) {
		XMLReader reader;
		ColorContentHandler handler(this);
		reader.setContentHandler(&handler);
		if (!reader.parse(wstrPath.get()))
			return false;
	}
	
	return true;
}


/****************************************************************************
 *
 * ColorSet
 *
 */

qm::ColorSet::ColorSet(std::auto_ptr<RegexPattern> pAccountName,
					   std::auto_ptr<RegexPattern> pFolderName) :
	pAccountName_(pAccountName),
	pFolderName_(pFolderName)
{
}

qm::ColorSet::~ColorSet()
{
	std::for_each(listEntry_.begin(),
		listEntry_.end(), deleter<ColorEntry>());
}

bool qm::ColorSet::match(Folder* pFolder) const
{
	assert(pFolder);
	
	if (pAccountName_.get() &&
		!pAccountName_->match(pFolder->getAccount()->getName()))
		return false;
	
	if (pFolderName_.get()) {
		wstring_ptr wstrFullName(pFolder->getFullName());
		if (!pFolderName_->match(wstrFullName.get()))
			return false;
	}
	
	return true;
}

COLORREF qm::ColorSet::getColor(MacroContext* pContext) const
{
	assert(pContext);
	
	for (EntryList::const_iterator it = listEntry_.begin(); it != listEntry_.end(); ++it) {
		const ColorEntry* pEntry = *it;
		if (pEntry->match(pContext))
			return pEntry->getColor();
	}
	
	return 0xff000000;
}

void qm::ColorSet::addEntry(std::auto_ptr<ColorEntry> pEntry)
{
	listEntry_.push_back(pEntry.get());
	pEntry.release();
}


/****************************************************************************
 *
 * ColorEntry
 *
 */

qm::ColorEntry::ColorEntry(std::auto_ptr<Macro> pMacro,
						   COLORREF cr) :
	pMacro_(pMacro),
	cr_(cr)
{
}

qm::ColorEntry::~ColorEntry()
{
}

bool qm::ColorEntry::match(MacroContext* pContext) const
{
	assert(pContext);
	assert(pContext->getMessageHolder());
	
	MacroValuePtr pValue(pMacro_->value(pContext));
	return pValue.get() && pValue->boolean();
}

COLORREF qm::ColorEntry::getColor() const
{
	return cr_;
}


/****************************************************************************
 *
 * ColorContentHandler
 *
 */

qm::ColorContentHandler::ColorContentHandler(ColorManager* pManager) :
	pManager_(pManager),
	state_(STATE_ROOT),
	pColorSet_(0),
	parser_(MacroParser::TYPE_COLOR)
{
}

qm::ColorContentHandler::~ColorContentHandler()
{
}

bool qm::ColorContentHandler::startElement(const WCHAR* pwszNamespaceURI,
										   const WCHAR* pwszLocalName,
										   const WCHAR* pwszQName,
										   const Attributes& attributes)
{
	if (wcscmp(pwszLocalName, L"colors") == 0) {
		if (state_ != STATE_ROOT)
			return false;
		if (attributes.getLength() != 0)
			return false;
		state_ = STATE_COLORS;
	}
	else if (wcscmp(pwszLocalName, L"colorSet") == 0) {
		if (state_ != STATE_COLORS)
			return false;
		
		const WCHAR* pwszAccount = 0;
		const WCHAR* pwszFolder = 0;
		for (int n = 0; n < attributes.getLength(); ++n) {
			const WCHAR* pwszAttrName = attributes.getLocalName(n);
			if (wcscmp(pwszAttrName, L"account") == 0)
				pwszAccount = attributes.getValue(n);
			else if (wcscmp(pwszAttrName, L"folder") == 0)
				pwszFolder = attributes.getValue(n);
			else
				return false;
		}
		
		assert(!pColorSet_);
		
		RegexCompiler compiler;
		
		std::auto_ptr<RegexPattern> pAccountName;
		if (pwszAccount) {
			pAccountName = compiler.compile(pwszAccount);
			if (!pAccountName.get())
				return false;
		}
		
		std::auto_ptr<RegexPattern> pFolderName;
		if (pwszFolder) {
			pFolderName = compiler.compile(pwszFolder);
			if (!pFolderName.get())
				return false;
		}
		
		std::auto_ptr<ColorSet> pColorSet(new ColorSet(pAccountName, pFolderName));
		pColorSet_ = pColorSet.get();
		pManager_->addColorSet(pColorSet);
		
		state_ = STATE_COLORSET;
	}
	else if (wcscmp(pwszLocalName, L"color") == 0) {
		if (state_ != STATE_COLORSET)
			return false;
		
		const WCHAR* pwszMatch = 0;
		for (int n = 0; n < attributes.getLength(); ++n) {
			const WCHAR* pwszAttrName = attributes.getLocalName(n);
			if (wcscmp(pwszAttrName, L"match") == 0)
				pwszMatch = attributes.getValue(n);
			else
				return false;
		}
		if (!pwszMatch)
			return false;
		
		assert(!pMacro_.get());
		pMacro_ = parser_.parse(pwszMatch);
		if (!pMacro_.get())
			return false;
		
		state_ = STATE_COLOR;
	}
	else {
		return false;
	}
	
	return true;
}

bool qm::ColorContentHandler::endElement(const WCHAR* pwszNamespaceURI,
										 const WCHAR* pwszLocalName,
										 const WCHAR* pwszQName)
{
	if (wcscmp(pwszLocalName, L"colors") == 0) {
		assert(state_ == STATE_COLORS);
		state_ = STATE_ROOT;
	}
	else if (wcscmp(pwszLocalName, L"colorSet") == 0) {
		assert(state_ = STATE_COLORSET);
		pColorSet_ = 0;
		state_ = STATE_COLORS;
	}
	else if (wcscmp(pwszLocalName, L"color") == 0) {
		assert(state_ == STATE_COLOR);
		
		Color color(buffer_.getCharArray());
		if (color.getColor() == 0xffffffff)
			return false;
		std::auto_ptr<ColorEntry> pEntry(new ColorEntry(pMacro_, color.getColor()));
		pMacro_.reset(0);
		buffer_.remove();
		
		assert(pColorSet_);
		pColorSet_->addEntry(pEntry);
		
		state_ = STATE_COLORSET;
	}
	else {
		return false;
	}
	
	return true;
}

bool qm::ColorContentHandler::characters(const WCHAR* pwsz,
										 size_t nStart,
										 size_t nLength)
{
	if (state_ == STATE_COLOR) {
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
