/*
 * $Id$
 *
 * Copyright(C) 1998 Satoshi Nakamura
 * All rights reserved.
 *
 */

#include <qsstl.h>

#include <algorithm>

#include "layout.h"

using namespace qm;
using namespace qs;


/****************************************************************************
 *
 * LineLayout
 *
 */

qm::LineLayout::LineLayout() :
	nHeight_(0),
	nLineSpacing_(3)
{
}

qm::LineLayout::~LineLayout()
{
	std::for_each(listLine_.begin(), listLine_.end(), deleter<LineLayoutLine>());
}

unsigned int qm::LineLayout::getLineCount() const
{
	return static_cast<unsigned int>(listLine_.size());
}

LineLayoutLine* qm::LineLayout::getLine(unsigned int n) const
{
	assert(n < listLine_.size());
	return listLine_[n];
}

int qm::LineLayout::getHeight() const
{
	return nHeight_;
}

bool qm::LineLayout::create(WindowBase* pParent,
							const std::pair<HFONT, HFONT>& fonts,
							UINT* pnId,
							void* pParam) const
{
	for (LineList::const_iterator it = listLine_.begin(); it != listLine_.end(); ++it) {
		if (!(*it)->create(pParent, fonts, pnId, pParam))
			return false;
	}
	return true;
}

void qm::LineLayout::destroy() const
{
	for (LineList::const_iterator it = listLine_.begin(); it != listLine_.end(); ++it)
		(*it)->destroy();
}

void qm::LineLayout::layout(const RECT& rect,
							unsigned int nFontHeight)
{
	RECT r = rect;
	
	int nCount = 0;
	for (LineList::const_iterator it = listLine_.begin(); it != listLine_.end(); ++it)
		nCount += (*it)->getItemCount();
	
	LineLayoutLine::WidthMap mapWidth(LineLayoutLine::MAX_MINMAX*2, 0);
	std::fill_n(mapWidth.begin(), static_cast<int>(LineLayoutLine::MAX_MINMAX), -1);
	for (LineList::const_iterator it = listLine_.begin(); it != listLine_.end(); ++it) {
		const LineLayoutLine* pLine = *it;
		if (!pLine->isHidden()) {
			for (unsigned int n = 0; n < pLine->getItemCount(); ++n) {
				const LineLayoutItem* pItem = pLine->getItem(n);
				LineLayoutItem::Unit unit = pItem->getUnit();
				if (unit == LineLayoutItem::UNIT_MINMAX) {
					int nIndex = static_cast<int>(pItem->getWidth());
					unsigned int& nWidth = mapWidth[nIndex + LineLayoutLine::MAX_MINMAX];
					unsigned int nPreferredWidth = pItem->getPreferredWidth();
					if (nIndex < 0)
						nWidth = QSMIN(nWidth, nPreferredWidth);
					else
						nWidth = QSMAX(nWidth, nPreferredWidth);
				}
			}
		}
	}
	
	HDWP hdwp = Window::beginDeferWindowPos(nCount);
	
	int nTop = 0;
	for (LineList::const_iterator it = listLine_.begin(); it != listLine_.end(); ++it) {
		const LineLayoutLine* pLine = *it;
		unsigned int nHeight = 0;
		if (!pLine->isHidden()) {
			r.top = nTop == 0 ? 5 : nTop;
			r.bottom = r.top;
			hdwp = pLine->layout(hdwp, r, nFontHeight, mapWidth, &nHeight);
			r.bottom = r.top + nHeight;
			if (nHeight != 0)
				nTop = r.bottom + nLineSpacing_;
		}
		pLine->show(nHeight != 0);
	}
	if (nTop == 0)
		nHeight_ = 0;
	else
		nHeight_ = r.bottom + 5;
	
	Window::endDeferWindowPos(hdwp);
}

void qm::LineLayout::setFont(const std::pair<HFONT, HFONT>& fonts)
{
	for (LineList::const_iterator it = listLine_.begin(); it != listLine_.end(); ++it)
		(*it)->setFont(fonts);
}

void qm::LineLayout::setLineSpacing(unsigned int nLineSpacing)
{
	nLineSpacing_ = nLineSpacing;
}

void qm::LineLayout::addLine(std::auto_ptr<LineLayoutLine> pLine)
{
	listLine_.push_back(pLine.get());
	pLine.release();
}


/****************************************************************************
 *
 * LineLayoutLine
 *
 */

qm::LineLayoutLine::LineLayoutLine()
{
}

qm::LineLayoutLine::~LineLayoutLine()
{
	std::for_each(listItem_.begin(), listItem_.end(),
		deleter<LineLayoutItem>());
}

unsigned int qm::LineLayoutLine::getItemCount() const
{
	return static_cast<unsigned int>(listItem_.size());
}

LineLayoutItem* qm::LineLayoutLine::getItem(unsigned int n) const
{
	assert(n < listItem_.size());
	return listItem_[n];
}

bool qm::LineLayoutLine::create(WindowBase* pParent,
								const std::pair<HFONT, HFONT>& fonts,
								UINT* pnId,
								void* pParam) const
{
	for (ItemList::const_iterator it = listItem_.begin(); it != listItem_.end(); ++it) {
		if (!(*it)->create(pParent, fonts, (*pnId)++, pParam))
			return false;
	}
	return true;
}

void qm::LineLayoutLine::destroy() const
{
	for (ItemList::const_iterator it = listItem_.begin(); it != listItem_.end(); ++it)
		(*it)->destroy();
}

HDWP qm::LineLayoutLine::layout(HDWP hdwp,
								const RECT& rect,
								unsigned int nFontHeight,
								const WidthMap& mapWidth,
								unsigned int* pnHeight) const
{
	assert(pnHeight);
	assert(!isHidden());
	
	int nWidth = rect.right - rect.left -
		(static_cast<unsigned int>(listItem_.size()) - 1)*2 - 10;
	
	typedef std::vector<int> WidthList;
	WidthList listWidth;
	listWidth.resize(listItem_.size());
	
	int nFixedWidth = 0;
	int nPercentWidth = 0;
	int nNoWidthCount = 0;
	for (ItemList::size_type n = 0; n < listItem_.size(); ++n) {
		const LineLayoutItem* pItem = listItem_[n];
		double dItemWidth = pItem->getWidth();
		int nItemWidth = 0;
		switch (pItem->getUnit()) {
		case LineLayoutItem::UNIT_NONE:
			++nNoWidthCount;
			break;
		case LineLayoutItem::UNIT_PIXEL:
			nItemWidth = static_cast<int>(dItemWidth);
			nFixedWidth += nItemWidth;
			break;
		case LineLayoutItem::UNIT_PERCENT:
			nPercentWidth += static_cast<int>(dItemWidth);
			nItemWidth = -static_cast<int>(dItemWidth);
			break;
		case LineLayoutItem::UNIT_EM:
			nItemWidth = static_cast<int>(dItemWidth*nFontHeight);
			nFixedWidth += nItemWidth;
			break;
		case LineLayoutItem::UNIT_AUTO:
			nItemWidth = pItem->getPreferredWidth();
			nFixedWidth += nItemWidth;
			break;
		case LineLayoutItem::UNIT_MINMAX:
			nItemWidth = mapWidth[MAX_MINMAX + static_cast<int>(dItemWidth)];
			nFixedWidth += nItemWidth;
			break;
		default:
			assert(false);
			break;
		}
		listWidth[n] = nItemWidth;
	}
	
	if (nWidth > nFixedWidth) {
		for (WidthList::iterator it = listWidth.begin(); it != listWidth.end(); ++it) {
			if (*it < 0)
				*it = (nWidth - nFixedWidth)*(-*it)/100;
			else if (*it == 0 && nPercentWidth < 100)
				*it = (nWidth - nFixedWidth)*(100 - nPercentWidth)/100/nNoWidthCount;
		}
	}
	else {
		for (WidthList::iterator it = listWidth.begin(); it != listWidth.end(); ++it) {
			if (*it < 0)
				*it = 0;
		}
	}
	
	unsigned int nHeight = 0;
	for (ItemList::size_type n = 0; n < listItem_.size(); ++n) {
		unsigned int nItemHeight = listItem_[n]->getHeight(listWidth[n], nFontHeight);
		nHeight = QSMAX(nHeight, nItemHeight);
	}
	if (nHeight != 0) {
		RECT r = { 5, rect.top, 5, rect.top + nHeight };
		for (ItemList::size_type n = 0; n < listItem_.size(); ++n) {
			r.right = r.left + listWidth[n];
			hdwp = listItem_[n]->layout(hdwp, r, nFontHeight);
			r.left = r.right + 2;
		}
	}
	*pnHeight = nHeight;
	
	return hdwp;
}

void qm::LineLayoutLine::show(bool bShow) const
{
	for (ItemList::const_iterator it = listItem_.begin(); it != listItem_.end(); ++it)
		(*it)->show(bShow);
}

void qm::LineLayoutLine::setFont(const std::pair<HFONT, HFONT>& fonts)
{
	for (ItemList::const_iterator it = listItem_.begin(); it != listItem_.end(); ++it)
		(*it)->setFont(fonts);
}

void qm::LineLayoutLine::addItem(std::auto_ptr<LineLayoutItem> pItem)
{
	listItem_.push_back(pItem.get());
	pItem.release();
}

bool qm::LineLayoutLine::isHidden() const
{
	return false;
}


/****************************************************************************
 *
 * LineLayoutItem
 *
 */

qm::LineLayoutItem::LineLayoutItem() :
	dWidth_(0),
	unit_(UNIT_NONE)
{
}

qm::LineLayoutItem::~LineLayoutItem()
{
}

double qm::LineLayoutItem::getWidth() const
{
	return dWidth_;
}

LineLayoutItem::Unit qm::LineLayoutItem::getUnit() const
{
	return unit_;
}

void qm::LineLayoutItem::setWidth(double dWidth,
								  Unit unit)
{
	dWidth_ = dWidth;
	unit_ = unit;
}

unsigned int qm::LineLayoutItem::getPreferredWidth() const
{
	return 1;
}

bool qm::LineLayoutItem::parseWidth(const WCHAR* pwszWidth,
									double* pdWidth,
									Unit* pUnit)
{
	assert(pwszWidth);
	assert(pdWidth);
	assert(pUnit);
	
	if (wcscmp(pwszWidth, L"auto") == 0) {
		*pdWidth = 0;
		*pUnit = UNIT_AUTO;
	}
	else {
		WCHAR* pEnd = 0;
		double dWidth = wcstod(pwszWidth, &pEnd);
		Unit unit = UNIT_NONE;
		if (dWidth < 0)
			return false;
		if (wcscmp(pEnd, L"%") == 0) {
			unit = UNIT_PERCENT;
		}
		else if (wcscmp(pEnd, L"em") == 0) {
			unit = UNIT_EM;
		}
		else if (wcscmp(pEnd, L"min") == 0) {
			unit = UNIT_MINMAX;
			dWidth = -dWidth;
		}
		else if (wcscmp(pEnd, L"max") == 0) {
			unit = UNIT_MINMAX;
		}
		else if (!*pEnd || wcscmp(pEnd, L"px") == 0) {
			unit = UNIT_PIXEL;
		}
		else {
			return false;
		}
		
		if (unit == UNIT_MINMAX) {
			if (dWidth == 0 || dWidth > LineLayoutLine::MAX_MINMAX)
				return false;
		}
		
		*pdWidth = dWidth;
		*pUnit = unit;
	}
	
	return true;
}
