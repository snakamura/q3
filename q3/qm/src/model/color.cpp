/*
 * $Id: color.cpp,v 1.2 2003/05/31 15:50:36 snakamura Exp $
 *
 * Copyright(C) 1998-2003 Satoshi Nakamura
 * All rights reserved.
 *
 */

#include <qmaccount.h>
#include <qmextensions.h>
#include <qmfolder.h>
#include <qmmacro.h>

#include <qsconv.h>
#include <qserror.h>
#include <qsnew.h>
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

qm::ColorManager::ColorManager(const WCHAR* pwszPath, QSTATUS* pstatus)
{
	DECLARE_QSTATUS();
	
	status = load(pwszPath);
	CHECK_QSTATUS_SET(pstatus);
}

qm::ColorManager::~ColorManager()
{
	std::for_each(listColorSet_.begin(),
		listColorSet_.end(), deleter<ColorSet>());
}

QSTATUS qm::ColorManager::getColorSet(
	Folder* pFolder, const ColorSet** ppSet) const
{
	assert(pFolder);
	assert(ppSet);
	
	DECLARE_QSTATUS();
	
	*ppSet = 0;
	
	ColorSetList::const_iterator it = listColorSet_.begin();
	while (it != listColorSet_.end() && !*ppSet) {
		bool bMatch = false;
		status = (*it)->match(pFolder, &bMatch);
		CHECK_QSTATUS();
		if (bMatch)
			*ppSet = *it;
		++it;
	}
	
	return QSTATUS_SUCCESS;
}

QSTATUS qm::ColorManager::addColorSet(ColorSet* pSet)
{
	return STLWrapper<ColorSetList>(listColorSet_).push_back(pSet);
}

QSTATUS qm::ColorManager::load(const WCHAR* pwszPath)
{
	DECLARE_QSTATUS();
	
	string_ptr<WSTRING> wstrPath(concat(
		pwszPath, L"\\", Extensions::COLORS));
	if (!wstrPath.get())
		return QSTATUS_OUTOFMEMORY;
	
	W2T(wstrPath.get(), ptszPath);
	if (::GetFileAttributes(ptszPath) != 0xffffffff) {
		XMLReader reader(&status);
		CHECK_QSTATUS();
		ColorContentHandler handler(this, &status);
		CHECK_QSTATUS();
		reader.setContentHandler(&handler);
		status = reader.parse(wstrPath.get());
		CHECK_QSTATUS();
	}
	
	return QSTATUS_SUCCESS;
}


/****************************************************************************
 *
 * ColorSet
 *
 */

qm::ColorSet::ColorSet(RegexPattern* pAccountName,
	RegexPattern* pFolderName, QSTATUS* pstatus) :
	pAccountName_(pAccountName),
	pFolderName_(pFolderName)
{
}

qm::ColorSet::~ColorSet()
{
	delete pAccountName_;
	delete pFolderName_;
	std::for_each(listEntry_.begin(),
		listEntry_.end(), deleter<ColorEntry>());
}

QSTATUS qm::ColorSet::match(Folder* pFolder, bool* pbMatch) const
{
	assert(pFolder);
	assert(pbMatch);
	
	DECLARE_QSTATUS();
	
	*pbMatch = false;
	
	if (pAccountName_) {
		status = pAccountName_->match(pFolder->getAccount()->getName(), pbMatch);
		CHECK_QSTATUS();
		if (!*pbMatch)
			return QSTATUS_SUCCESS;
	}
	
	if (pFolderName_) {
		string_ptr<WSTRING> wstrFullName;
		status = pFolder->getFullName(&wstrFullName);
		CHECK_QSTATUS();
		status = pFolderName_->match(wstrFullName.get(), pbMatch);
		CHECK_QSTATUS();
	}
	else {
		*pbMatch = true;
	}
	
	return QSTATUS_SUCCESS;
}

QSTATUS qm::ColorSet::getColor(MacroContext* pContext, COLORREF* pcr) const
{
	assert(pContext);
	assert(pcr);
	
	DECLARE_QSTATUS();
	
	*pcr = 0xff000000;
	
	EntryList::const_iterator it = listEntry_.begin();
	while (it != listEntry_.end()) {
		bool bMatch = false;
		status = (*it)->match(pContext, &bMatch);
		CHECK_QSTATUS();
		if (bMatch) {
			*pcr = (*it)->getColor();
			break;
		}
		++it;
	}
	
	return QSTATUS_SUCCESS;
}

QSTATUS qm::ColorSet::addEntry(ColorEntry* pEntry)
{
	return STLWrapper<EntryList>(listEntry_).push_back(pEntry);
}


/****************************************************************************
 *
 * ColorEntry
 *
 */

qm::ColorEntry::ColorEntry(Macro* pMacro,
	const WCHAR* pwszColor, QSTATUS* pstatus) :
	pMacro_(0),
	cr_(0)
{
	assert(pMacro);
	assert(pwszColor);
	
	DECLARE_QSTATUS();
	
	Color color(pwszColor, &status);
	CHECK_QSTATUS_SET(pstatus);
	
	pMacro_ = pMacro;
	cr_ = color.getColor();
}

qm::ColorEntry::~ColorEntry()
{
	delete pMacro_;
}

QSTATUS qm::ColorEntry::match(MacroContext* pContext, bool* pbMatch) const
{
	assert(pContext);
	assert(pContext->getMessageHolder());
	assert(pbMatch);
	
	DECLARE_QSTATUS();
	
	*pbMatch = false;
	
	MacroValuePtr pValue;
	status = pMacro_->value(pContext, &pValue);
	CHECK_QSTATUS();
	*pbMatch = pValue->boolean();
	
	return QSTATUS_SUCCESS;
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

qm::ColorContentHandler::ColorContentHandler(
	ColorManager* pManager, QSTATUS* pstatus) :
	DefaultHandler(pstatus),
	pManager_(pManager),
	state_(STATE_ROOT),
	pColorSet_(0),
	pMacro_(0),
	pBuffer_(0),
	pParser_(0)
{
	DECLARE_QSTATUS();
	
	status = newQsObject(&pBuffer_);
	CHECK_QSTATUS_SET(pstatus);
	status = newQsObject(MacroParser::TYPE_COLOR, &pParser_);
	CHECK_QSTATUS_SET(pstatus);
}

qm::ColorContentHandler::~ColorContentHandler()
{
	delete pParser_;
	delete pBuffer_;
	delete pMacro_;
}

QSTATUS qm::ColorContentHandler::startElement(
	const WCHAR* pwszNamespaceURI, const WCHAR* pwszLocalName,
	const WCHAR* pwszQName, const Attributes& attributes)
{
	DECLARE_QSTATUS();
	
	if (wcscmp(pwszLocalName, L"colors") == 0) {
		if (state_ != STATE_ROOT)
			return QSTATUS_FAIL;
		
		if (attributes.getLength() != 0)
			return QSTATUS_FAIL;
		
		state_ = STATE_COLORS;
	}
	else if (wcscmp(pwszLocalName, L"colorSet") == 0) {
		if (state_ != STATE_COLORS)
			return QSTATUS_FAIL;
		
		const WCHAR* pwszAccount = 0;
		const WCHAR* pwszFolder = 0;
		for (int n = 0; n < attributes.getLength(); ++n) {
			const WCHAR* pwszAttrName = attributes.getLocalName(n);
			if (wcscmp(pwszAttrName, L"account") == 0)
				pwszAccount = attributes.getValue(n);
			else if (wcscmp(pwszAttrName, L"folder") == 0)
				pwszFolder = attributes.getValue(n);
			else
				return QSTATUS_FAIL;
		}
		
		assert(!pColorSet_);
		
		std::auto_ptr<RegexPattern> pAccountName;
		if (pwszAccount) {
			RegexCompiler compiler;
			RegexPattern* p = 0;
			status = compiler.compile(pwszAccount, &p);
			CHECK_QSTATUS();
			pAccountName.reset(p);
		}
		
		std::auto_ptr<RegexPattern> pFolderName;
		if (pwszFolder) {
			RegexCompiler compiler;
			RegexPattern* p = 0;
			status = compiler.compile(pwszFolder, &p);
			CHECK_QSTATUS();
			pFolderName.reset(p);
		}
		
		std::auto_ptr<ColorSet> pColorSet;
		status = newQsObject(pAccountName.get(), pFolderName.get(), &pColorSet);
		CHECK_QSTATUS();
		pAccountName.release();
		pFolderName.release();
		status = pManager_->addColorSet(pColorSet.get());
		CHECK_QSTATUS();
		pColorSet_ = pColorSet.release();
		
		state_ = STATE_COLORSET;
	}
	else if (wcscmp(pwszLocalName, L"color") == 0) {
		if (state_ != STATE_COLORSET)
			return QSTATUS_FAIL;
		
		const WCHAR* pwszMatch = 0;
		for (int n = 0; n < attributes.getLength(); ++n) {
			const WCHAR* pwszAttrName = attributes.getLocalName(n);
			if (wcscmp(pwszAttrName, L"match") == 0)
				pwszMatch = attributes.getValue(n);
			else
				return QSTATUS_FAIL;
		}
		if (!pwszMatch)
			return QSTATUS_FAIL;
		
		assert(!pMacro_);
		status = pParser_->parse(pwszMatch, &pMacro_);
		CHECK_QSTATUS();
		
		state_ = STATE_COLOR;
	}
	else {
		return QSTATUS_FAIL;
	}
	
	return QSTATUS_SUCCESS;
}

QSTATUS qm::ColorContentHandler::endElement(const WCHAR* pwszNamespaceURI,
	const WCHAR* pwszLocalName, const WCHAR* pwszQName)
{
	DECLARE_QSTATUS();
	
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
		
		std::auto_ptr<ColorEntry> pEntry;
		status = newQsObject(pMacro_, pBuffer_->getCharArray(), &pEntry);
		CHECK_QSTATUS();
		pMacro_ = 0;
		pBuffer_->remove();
		
		assert(pColorSet_);
		status = pColorSet_->addEntry(pEntry.get());
		CHECK_QSTATUS();
		pEntry.release();
		
		state_ = STATE_COLORSET;
	}
	else {
		return QSTATUS_FAIL;
	}
	
	return QSTATUS_SUCCESS;
}

QSTATUS qm::ColorContentHandler::characters(
	const WCHAR* pwsz, size_t nStart, size_t nLength)
{
	DECLARE_QSTATUS();
	
	if (state_ == STATE_COLOR) {
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
