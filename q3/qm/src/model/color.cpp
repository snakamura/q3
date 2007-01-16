/*
 * $Id$
 *
 * Copyright(C) 1998-2007 Satoshi Nakamura
 * All rights reserved.
 *
 */

#pragma warning(disable:4786)

#include <qmaccount.h>
#include <qmfolder.h>
#include <qmmacro.h>

#include <qsconv.h>
#include <qsfile.h>
#include <qsstream.h>
#include <qsutil.h>

#include <algorithm>

#include <boost/bind.hpp>

#include "color.h"

using namespace qm;
using namespace qs;


/****************************************************************************
 *
 * ColorManager
 *
 */

qm::ColorManager::ColorManager(const WCHAR* pwszPath) :
	helper_(pwszPath)
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
	fireColorSetsChanged();
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
	return helper_.save(this);
}

void qm::ColorManager::addColorManagerHandler(ColorManagerHandler* pHandler)
{
	listHandler_.push_back(pHandler);
}

void qm::ColorManager::removeColorManagerHandler(ColorManagerHandler* pHandler)
{
	HandlerList::iterator it = std::remove(listHandler_.begin(), listHandler_.end(), pHandler);
	listHandler_.erase(it, listHandler_.end());
}

void qm::ColorManager::addColorSet(std::auto_ptr<ColorSet> pSet)
{
	listColorSet_.push_back(pSet.get());
	pSet.release();
}

void qm::ColorManager::clear()
{
	std::for_each(listColorSet_.begin(), listColorSet_.end(), deleter<ColorSet>());
	listColorSet_.clear();
}

bool qm::ColorManager::load()
{
	ColorContentHandler handler(this);
	return helper_.load(this, &handler);
}

void qm::ColorManager::fireColorSetsChanged()
{
	ColorManagerEvent event(this);
	for (HandlerList::const_iterator it = listHandler_.begin(); it != listHandler_.end(); ++it)
		(*it)->colorSetsChanged(event);
}


/****************************************************************************
 *
 * ColorManagerHandler
 *
 */

qm::ColorManagerHandler::~ColorManagerHandler()
{
}


/****************************************************************************
 *
 * ColorManagerEvent
 *
 */

qm::ColorManagerEvent::ColorManagerEvent(ColorManager* pColorManager) :
	pColorManager_(pColorManager)
{
}

qm::ColorManagerEvent::~ColorManagerEvent()
{
}

ColorManager* qm::ColorManagerEvent::getColorManager() const
{
	return pColorManager_;
}


/****************************************************************************
 *
 * ColorSet
 *
 */

qm::ColorSet::ColorSet()
{
}

qm::ColorSet::ColorSet(Term& account,
					   Term& folder)
{
	account_.assign(account);
	folder_.assign(folder);
}

qm::ColorSet::ColorSet(const ColorSet& colorset) :
	account_(colorset.account_),
	folder_(colorset.folder_)
{
	for (ColorSet::ColorList::const_iterator it = colorset.listColor_.begin(); it != colorset.listColor_.end(); ++it)
		listColor_.push_back(new ColorEntry(**it));
}

qm::ColorSet::~ColorSet()
{
	clear();
}

const WCHAR* qm::ColorSet::getAccount() const
{
	return account_.getValue();
}

void qm::ColorSet::setAccount(Term& account)
{
	account_.assign(account);
}

const WCHAR* qm::ColorSet::getFolder() const
{
	return folder_.getValue();
}

void qm::ColorSet::setFolder(Term& folder)
{
	folder_.assign(folder);
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
	
	if (!account_.match(pFolder->getAccount()->getName()))
		return false;
	
	if (folder_.isSpecified()) {
		wstring_ptr wstrFullName(pFolder->getFullName());
		if (!folder_.match(wstrFullName.get()))
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
	crForeground_(0xff000000),
	crBackground_(0xff000000),
	nFontStyle_(FONTSTYLE_NONE)
{
}

qm::ColorEntry::ColorEntry(std::auto_ptr<Macro> pCondition,
						   COLORREF crForeground,
						   COLORREF crBackground,
						   unsigned int nFontStyle,
						   const WCHAR* pwszDescription) :
	pCondition_(pCondition),
	crForeground_(crForeground),
	crBackground_(crBackground),
	nFontStyle_(nFontStyle)
{
	if (pwszDescription)
		wstrDescription_ = allocWString(pwszDescription);
}

qm::ColorEntry::ColorEntry(const ColorEntry& color) :
	crForeground_(color.crForeground_),
	crBackground_(color.crBackground_),
	nFontStyle_(color.nFontStyle_)
{
	wstring_ptr wstrCondition(color.pCondition_->getString());
	pCondition_ = MacroParser().parse(wstrCondition.get());
	if (color.wstrDescription_.get())
		wstrDescription_ = allocWString(color.wstrDescription_.get());
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

COLORREF qm::ColorEntry::getForeground() const
{
	return crForeground_;
}

void qm::ColorEntry::setForeground(COLORREF cr)
{
	crForeground_ = cr;
}

COLORREF qm::ColorEntry::getBackground() const
{
	return crBackground_;
}

void qm::ColorEntry::setBackground(COLORREF cr)
{
	crBackground_ = cr;
}

unsigned int qm::ColorEntry::getFontStyle() const
{
	return nFontStyle_;
}

void qm::ColorEntry::setFontStyle(unsigned int nFontStyle)
{
	nFontStyle_ = nFontStyle;
}

const WCHAR* qm::ColorEntry::getDescription() const
{
	return wstrDescription_.get();
}

void qm::ColorEntry::setDescription(const WCHAR* pwszDescription)
{
	if (pwszDescription && *pwszDescription)
		wstrDescription_ = allocWString(pwszDescription);
	else
		wstrDescription_.reset(0);
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

ColorList::Color qm::ColorList::getColor(MacroContext* pContext) const
{
	assert(pContext);
	
	Color color = { 0xff000000, 0xff000000, ColorEntry::FONTSTYLE_NONE };
	for (List::const_iterator it = list_.begin(); it != list_.end(); ++it) {
		const ColorEntry* pEntry = *it;
		if (((color.crForeground_ == 0xff000000 && pEntry->getForeground() != 0xff000000) ||
			(color.crBackground_ == 0xff000000 && pEntry->getBackground() != 0xff000000) ||
			(color.nFontStyle_ == ColorEntry::FONTSTYLE_NONE && pEntry->getFontStyle() != ColorEntry::FONTSTYLE_NONE)) &&
			pEntry->match(pContext)) {
			if (color.crForeground_ == 0xff000000)
				color.crForeground_ = pEntry->getForeground();
			if (color.crBackground_ == 0xff000000)
				color.crBackground_ = pEntry->getBackground();
			if (color.nFontStyle_ == ColorEntry::FONTSTYLE_NONE)
				color.nFontStyle_ = pEntry->getFontStyle();
			
			if (color.crForeground_ != 0xff000000 &&
				color.crBackground_ != 0xff000000 &&
				color.nFontStyle_ != ColorEntry::FONTSTYLE_NONE)
				break;
		}
	}
	return color;
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
	crForeground_(0xff000000),
	crBackground_(0xff000000),
	nFontStyle_(ColorEntry::FONTSTYLE_NONE)
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
		
		Term account;
		if (pwszAccount && !account.setValue(pwszAccount))
			return false;
		
		Term folder;
		if (pwszFolder && !folder.setValue(pwszFolder))
			return false;
		
		std::auto_ptr<ColorSet> pColorSet(new ColorSet(account, folder));
		pColorSet_ = pColorSet.get();
		pManager_->addColorSet(pColorSet);
		
		state_ = STATE_COLORSET;
	}
	else if (wcscmp(pwszLocalName, L"color") == 0) {
		if (state_ != STATE_COLORSET)
			return false;
		
		const WCHAR* pwszMatch = 0;
		const WCHAR* pwszDescription = 0;
		for (int n = 0; n < attributes.getLength(); ++n) {
			const WCHAR* pwszAttrName = attributes.getLocalName(n);
			if (wcscmp(pwszAttrName, L"match") == 0)
				pwszMatch = attributes.getValue(n);
			else if (wcscmp(pwszAttrName, L"description") == 0)
				pwszDescription = attributes.getValue(n);
			else
				return false;
		}
		if (!pwszMatch)
			return false;
		
		assert(!pCondition_.get());
		pCondition_ = MacroParser().parse(pwszMatch);
		if (!pCondition_.get())
			return false;
		
		if (pwszDescription)
			wstrDescription_ = allocWString(pwszDescription);
		
		crForeground_ = 0xff000000;
		crBackground_ = 0xff000000;
		nFontStyle_ = ColorEntry::FONTSTYLE_NONE;
		
		state_ = STATE_COLOR;
	}
	else if (wcscmp(pwszLocalName, L"foreground") == 0 ||
		wcscmp(pwszLocalName, L"background") == 0 ||
		wcscmp(pwszLocalName, L"style") == 0) {
		if (state_ != STATE_COLOR)
			return false;
		state_ = STATE_VALUE;
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
		
		if (crForeground_ == 0xff000000 && buffer_.getLength() != 0) {
			Color color(buffer_.getCharArray());
			if (color.getColor() != 0xffffffff)
				crForeground_ = color.getColor();
		}
		
		std::auto_ptr<ColorEntry> pEntry(new ColorEntry(pCondition_,
			crForeground_, crBackground_, nFontStyle_, wstrDescription_.get()));
		pCondition_.reset(0);
		buffer_.remove();
		wstrDescription_.reset(0);
		
		assert(pColorSet_);
		pColorSet_->addEntry(pEntry);
		
		state_ = STATE_COLORSET;
	}
	else if (wcscmp(pwszLocalName, L"foreground") == 0 ||
		wcscmp(pwszLocalName, L"background") == 0) {
		assert(state_ == STATE_VALUE);
		
		Color color(buffer_.getCharArray());
		if (color.getColor() == 0xffffffff)
			return false;
		if (wcscmp(pwszLocalName, L"foreground") == 0)
			crForeground_ = color.getColor();
		else
			crBackground_ = color.getColor();
		buffer_.remove();
		
		state_ = STATE_COLOR;
	}
	else if (wcscmp(pwszLocalName, L"style") == 0) {
		assert(state_ == STATE_VALUE);
		
		wstring_ptr wstrStyle(buffer_.getString());
		const WCHAR* p = wcstok(wstrStyle.get(), L" ");
		while (p) {
			if (wcscmp(p, L"regular") == 0)
				nFontStyle_ |= ColorEntry::FONTSTYLE_REGULAR;
			else if (wcscmp(p, L"bold") == 0)
				nFontStyle_ |= ColorEntry::FONTSTYLE_BOLD;
			else if (wcscmp(p, L"italic") == 0)
				nFontStyle_ |= ColorEntry::FONTSTYLE_ITALIC;
			p = wcstok(0, L" ");
		}
		
		state_ = STATE_COLOR;
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
		const WCHAR* p = pwsz + nStart;
		for (size_t n = 0; n < nLength; ++n, ++p) {
			if (*p != L' ' && *p != L'\t' && *p != '\n') {
				buffer_.append(pwsz + nStart, nLength);
				break;
			}
		}
	}
	else if (state_ == STATE_VALUE) {
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

qm::ColorWriter::ColorWriter(Writer* pWriter,
							 const WCHAR* pwszEncoding) :
	handler_(pWriter, pwszEncoding)
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
	const WCHAR* pwszDescription = pColor->getDescription();
	const SimpleAttributes::Item items[] = {
		{ L"match",			wstrCondition.get(),	false					},
		{ L"description",	pwszDescription,		!pwszDescription		}
	};
	SimpleAttributes attrs(items, countof(items));
	if (!handler_.startElement(0, 0, L"color", attrs))
		return false;
	
	struct {
		COLORREF cr_;
		const WCHAR* pwszName_;
	} colors[] = {
		{ pColor->getForeground(), L"foreground"	},
		{ pColor->getBackground(), L"background"	}
	};
	for (int n = 0; n < countof(colors); ++n) {
		if (colors[n].cr_ != 0xff000000) {
			wstring_ptr wstrColor(Color(colors[n].cr_).getString());
			if (!handler_.startElement(0, 0, colors[n].pwszName_, DefaultAttributes()) ||
				!handler_.characters(wstrColor.get(), 0, wcslen(wstrColor.get())) ||
				!handler_.endElement(0, 0, colors[n].pwszName_))
				return false;
		}
	}
	
	unsigned int nFontStyle = pColor->getFontStyle();
	if (nFontStyle != ColorEntry::FONTSTYLE_NONE) {
		struct {
			ColorEntry::FontStyle style_;
			const WCHAR* pwsz_;
		} styles[] = {
			{ ColorEntry::FONTSTYLE_REGULAR,	L"regular"	},
			{ ColorEntry::FONTSTYLE_BOLD,		L"bold"		},
			{ ColorEntry::FONTSTYLE_ITALIC,		L"italic"	}
		};
		StringBuffer<WSTRING> buf;
		for (int n = 0; n < countof(styles); ++n) {
			if (nFontStyle & styles[n].style_) {
				if (buf.getLength() != 0)
					buf.append(L' ');
				buf.append(styles[n].pwsz_);
			}
		}
		if (!handler_.startElement(0, 0, L"style", DefaultAttributes()) ||
			!handler_.characters(buf.getCharArray(), 0, buf.getLength()) ||
			!handler_.endElement(0, 0, L"style"))
			return false;
	}
	
	if (!handler_.endElement(0, 0, L"color"))
		return false;
	
	return true;
}
