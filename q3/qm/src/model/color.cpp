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
#include <qsfile.h>
#include <qsstream.h>
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
	clear();
}

const ColorManager::ColorSetList& qm::ColorManager::getColorSets()
{
	return listColorSet_;
}

void qm::ColorManager::setColorSets(ColorSetList& listColorSet)
{
	clear();
	listColorSet_.swap(listColorSet);
}

std::auto_ptr<ColorList> qm::ColorManager::getColorList(Folder* pFolder) const
{
	assert(pFolder);
	
	ColorList::List listColor;
	
	for (ColorSetList::const_iterator it = listColorSet_.begin(); it != listColorSet_.end(); ++it) {
		const ColorSet* pSet = *it;
		if (pSet->match(pFolder)) {
			const ColorSet::ColorList& l = pSet->getColors();
			std::copy(l.begin(), l.end(), std::back_inserter(listColor));
		}
	}
	
	return std::auto_ptr<ColorList>(new ColorList(listColor));
}

bool qm::ColorManager::save() const
{
	wstring_ptr wstrPath(Application::getApplication().getProfilePath(FileNames::COLORS_XML));
	
	TemporaryFileRenamer renamer(wstrPath.get());
	
	FileOutputStream os(renamer.getPath());
	if (!os)
		return false;
	OutputStreamWriter writer(&os, false, L"utf-8");
	if (!writer)
		return false;
	BufferedWriter bufferedWriter(&writer, false);
	
	ColorWriter colorWriter(&bufferedWriter);
	if (!colorWriter.write(this))
		return false;
	
	if (!bufferedWriter.close())
		return false;
	
	if (!renamer.rename())
		return false;
	
	return true;
}

void qm::ColorManager::addColorSet(std::auto_ptr<ColorSet> pSet)
{
	listColorSet_.push_back(pSet.get());
	pSet.release();
}

bool qm::ColorManager::load()
{
	clear();
	
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

void qm::ColorManager::clear()
{
	std::for_each(listColorSet_.begin(), listColorSet_.end(), deleter<ColorSet>());
	listColorSet_.clear();
}


/****************************************************************************
 *
 * ColorSet
 *
 */

qm::ColorSet::ColorSet()
{
}

qm::ColorSet::ColorSet(const WCHAR* pwszAccount,
					   std::auto_ptr<RegexPattern> pAccount,
					   const WCHAR* pwszFolder,
					   std::auto_ptr<RegexPattern> pFolder) :
	pAccount_(pAccount),
	pFolder_(pFolder)
{
	if (pwszAccount)
		wstrAccount_ = allocWString(pwszAccount);
	if (pwszFolder)
		wstrFolder_ = allocWString(pwszFolder);
}

qm::ColorSet::ColorSet(const ColorSet& colorset)
{
	RegexCompiler compiler;
	if (colorset.wstrAccount_.get()) {
		wstrAccount_ = allocWString(colorset.wstrAccount_.get());
		pAccount_ = compiler.compile(wstrAccount_.get());
		assert(pAccount_.get());
	}
	if (colorset.wstrFolder_.get()) {
		wstrFolder_ = allocWString(colorset.wstrFolder_.get());
		pFolder_ = compiler.compile(wstrFolder_.get());
		assert(pFolder_.get());
	}
	
	for (ColorSet::ColorList::const_iterator it = colorset.listColor_.begin(); it != colorset.listColor_.end(); ++it)
		listColor_.push_back(new ColorEntry(**it));
}

qm::ColorSet::~ColorSet()
{
	clear();
}

const WCHAR* qm::ColorSet::getAccount() const
{
	return wstrAccount_.get();
}

void qm::ColorSet::setAccount(const WCHAR* pwszAccount,
							  std::auto_ptr<RegexPattern> pAccount)
{
	assert((pwszAccount && pAccount.get()) || (!pwszAccount && !pAccount.get()));
	if (pwszAccount)
		wstrAccount_ = allocWString(pwszAccount);
	else
		wstrAccount_.reset(0);
	pAccount_ = pAccount;
}

const WCHAR* qm::ColorSet::getFolder() const
{
	return wstrFolder_.get();
}

void qm::ColorSet::setFolder(const WCHAR* pwszFolder,
							 std::auto_ptr<RegexPattern> pFolder)
{
	assert((pwszFolder && pFolder.get()) || (!pwszFolder && !pFolder.get()));
	if (pwszFolder)
		wstrFolder_ = allocWString(pwszFolder);
	else
		wstrFolder_.reset(0);
	pFolder_ = pFolder;
}

const ColorSet::ColorList& qm::ColorSet::getColors() const
{
	return listColor_;
}

void qm::ColorSet::setColors(ColorList& listColor)
{
	clear();
	listColor_.swap(listColor);
}

bool qm::ColorSet::match(Folder* pFolder) const
{
	assert(pFolder);
	
	if (pAccount_.get() && !pAccount_->match(pFolder->getAccount()->getName()))
		return false;
	
	if (pFolder_.get()) {
		wstring_ptr wstrFullName(pFolder->getFullName());
		if (!pFolder_->match(wstrFullName.get()))
			return false;
	}
	
	return true;
}

void qm::ColorSet::addEntry(std::auto_ptr<ColorEntry> pEntry)
{
	listColor_.push_back(pEntry.get());
	pEntry.release();
}

void qm::ColorSet::clear()
{
	std::for_each(listColor_.begin(), listColor_.end(), deleter<ColorEntry>());
	listColor_.clear();
}


/****************************************************************************
 *
 * ColorEntry
 *
 */

qm::ColorEntry::ColorEntry() :
	cr_(RGB(0, 0, 0))
{
}

qm::ColorEntry::ColorEntry(std::auto_ptr<Macro> pCondition,
						   COLORREF cr) :
	pCondition_(pCondition),
	cr_(cr)
{
}

qm::ColorEntry::ColorEntry(const ColorEntry& color)
{
	wstring_ptr wstrCondition(color.pCondition_->getString());
	pCondition_ = MacroParser(MacroParser::TYPE_COLOR).parse(wstrCondition.get());
	cr_ = color.cr_;
}

qm::ColorEntry::~ColorEntry()
{
}

const Macro* qm::ColorEntry::getCondition() const
{
	return pCondition_.get();
}

void qm::ColorEntry::setCondition(std::auto_ptr<Macro> pCondition)
{
	pCondition_ = pCondition;
}

bool qm::ColorEntry::match(MacroContext* pContext) const
{
	assert(pContext);
	assert(pContext->getMessageHolder());
	
	MacroValuePtr pValue(pCondition_->value(pContext));
	return pValue.get() && pValue->boolean();
}

COLORREF qm::ColorEntry::getColor() const
{
	return cr_;
}

void qm::ColorEntry::setColor(COLORREF cr)
{
	cr_ = cr;
}


/****************************************************************************
 *
 * ColorList
 *
 */

qm::ColorList::ColorList(List& list)
{
	list_.swap(list);
}

qm::ColorList::~ColorList()
{
}

COLORREF qm::ColorList::getColor(MacroContext* pContext) const
{
	assert(pContext);
	
	for (List::const_iterator it = list_.begin(); it != list_.end(); ++it) {
		const ColorEntry* pEntry = *it;
		if (pEntry->match(pContext))
			return pEntry->getColor();
	}
	
	return 0xff000000;
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
		
		std::auto_ptr<RegexPattern> pAccount;
		if (pwszAccount) {
			pAccount = compiler.compile(pwszAccount);
			if (!pAccount.get())
				return false;
		}
		
		std::auto_ptr<RegexPattern> pFolder;
		if (pwszFolder) {
			pFolder = compiler.compile(pwszFolder);
			if (!pFolder.get())
				return false;
		}
		
		std::auto_ptr<ColorSet> pColorSet(new ColorSet(
			pwszAccount, pAccount, pwszFolder, pFolder));
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
		
		assert(!pCondition_.get());
		pCondition_ = parser_.parse(pwszMatch);
		if (!pCondition_.get())
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
		std::auto_ptr<ColorEntry> pEntry(new ColorEntry(pCondition_, color.getColor()));
		pCondition_.reset(0);
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


/****************************************************************************
 *
 * ColorWriter
 *
 */

qm::ColorWriter::ColorWriter(Writer* pWriter) :
	handler_(pWriter)
{
}

qm::ColorWriter::~ColorWriter()
{
}

bool qm::ColorWriter::write(const ColorManager* pManager)
{
	if (!handler_.startDocument())
		return false;
	if (!handler_.startElement(0, 0, L"colors", DefaultAttributes()))
		return false;
	
	const ColorManager::ColorSetList& listColorSet =
		const_cast<ColorManager*>(pManager)->getColorSets();
	for (ColorManager::ColorSetList::const_iterator it = listColorSet.begin(); it != listColorSet.end(); ++it) {
		const ColorSet* pColorSet = *it;
		if (!write(pColorSet))
			return false;
	}
	
	if (!handler_.endElement(0, 0, L"colors"))
		return false;
	if (!handler_.endDocument())
		return false;
	
	return true;
}

bool qm::ColorWriter::write(const ColorSet* pColorSet)
{
	const WCHAR* pwszAccount = pColorSet->getAccount();
	const WCHAR* pwszFolder = pColorSet->getFolder();
	const SimpleAttributes::Item items[] = {
		{ L"account",	pwszAccount,	pwszAccount == 0	},
		{ L"folder",	pwszFolder,		pwszFolder == 0		}
	};
	SimpleAttributes attrs(items, countof(items));
	if (!handler_.startElement(0, 0, L"colorSet", attrs))
		return false;
	
	const ColorSet::ColorList& listColor = pColorSet->getColors();
	for (ColorSet::ColorList::const_iterator it = listColor.begin(); it != listColor.end(); ++it) {
		const ColorEntry* pColor = *it;
		if (!write(pColor))
			return false;
	}
	
	if (!handler_.endElement(0, 0, L"colorSet"))
		return false;
	
	return true;
}

bool qm::ColorWriter::write(const ColorEntry* pColor)
{
	wstring_ptr wstrCondition(pColor->getCondition()->getString());
	SimpleAttributes attrs(L"match", wstrCondition.get());
	if (!handler_.startElement(0, 0, L"color", attrs))
		return false;
	
	wstring_ptr wstrColor(Color(pColor->getColor()).getString());
	if (!handler_.characters(wstrColor.get(), 0, wcslen(wstrColor.get())))
		return false;
	
	if (!handler_.endElement(0, 0, L"color"))
		return false;
	
	return true;
}
