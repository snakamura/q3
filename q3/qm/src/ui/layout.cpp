/*
 * $Id$
 *
 * Copyright(C) 1998 Satoshi Nakamura
 * All rights reserved.
 *
 */

#include <qserror.h>
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

qm::LineLayout::LineLayout(QSTATUS* pstatus) :
	nHeight_(0),
	nLineSpacing_(3)
{
	assert(pstatus);
	*pstatus = QSTATUS_SUCCESS;
}

qm::LineLayout::~LineLayout()
{
	std::for_each(listLine_.begin(), listLine_.end(),
		deleter<LineLayoutLine>());
}

unsigned int qm::LineLayout::getLineCount() const
{
	return listLine_.size();
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

QSTATUS qm::LineLayout::create(WindowBase* pParent,
	const std::pair<HFONT, HFONT>& fonts, UINT* pnId) const
{
	DECLARE_QSTATUS();
	
	LineList::const_iterator it = listLine_.begin();
	while (it != listLine_.end()) {
		status = (*it)->create(pParent, fonts, pnId);
		CHECK_QSTATUS();
		++it;
	}
	
	return QSTATUS_SUCCESS;
}

QSTATUS qm::LineLayout::destroy() const
{
	DECLARE_QSTATUS();
	
	LineList::const_iterator it = listLine_.begin();
	while (it != listLine_.end()) {
		status = (*it)->destroy();
		CHECK_QSTATUS();
		++it;
	}
	
	return QSTATUS_SUCCESS;
}

QSTATUS qm::LineLayout::layout(const RECT& rect, unsigned int nFontHeight)
{
	DECLARE_QSTATUS();
	
	RECT r = rect;
	
	int nTop = 0;
	LineList::const_iterator it = listLine_.begin();
	while (it != listLine_.end()) {
		unsigned int nHeight = (*it)->getHeight(nFontHeight);
		if (nHeight != 0) {
			if (nTop == 0)
				nTop = 5;
			r.top = nTop;
			r.bottom = nTop + nHeight;
			status = (*it)->layout(r, nFontHeight);
			CHECK_QSTATUS();
			nTop = r.bottom + nLineSpacing_;
		}
		status = (*it)->show(nHeight != 0);
		CHECK_QSTATUS();
		++it;
	}
	if (nTop == 0)
		nHeight_ = 0;
	else
		nHeight_ = r.bottom + 5;
	
	return QSTATUS_SUCCESS;
}

void qm::LineLayout::setLineSpacing(unsigned int nLineSpacing)
{
	nLineSpacing_ = nLineSpacing;
}

QSTATUS qm::LineLayout::addLine(LineLayoutLine* pLine)
{
	return STLWrapper<LineList>(listLine_).push_back(pLine);
}


/****************************************************************************
 *
 * LineLayoutLine
 *
 */

qm::LineLayoutLine::LineLayoutLine(QSTATUS* pstatus)
{
	assert(pstatus);
	*pstatus = QSTATUS_SUCCESS;
}

qm::LineLayoutLine::~LineLayoutLine()
{
	std::for_each(listItem_.begin(), listItem_.end(),
		deleter<LineLayoutItem>());
}

unsigned int qm::LineLayoutLine::getItemCount() const
{
	return listItem_.size();
}

LineLayoutItem* qm::LineLayoutLine::getItem(unsigned int n) const
{
	assert(n < listItem_.size());
	return listItem_[n];
}

unsigned int qm::LineLayoutLine::getHeight(unsigned int nFontHeight) const
{
	unsigned int nHeight = 0;
	
	if (!isHidden()) {
		ItemList::const_iterator itI = listItem_.begin();
		while (itI != listItem_.end()) {
			nHeight = QSMAX(nHeight, (*itI)->getHeight(nFontHeight));
			++itI;
		}
	}
	
	return nHeight;
}

QSTATUS qm::LineLayoutLine::create(WindowBase* pParent,
	const std::pair<HFONT, HFONT>& fonts, UINT* pnId) const
{
	DECLARE_QSTATUS();
	
	ItemList::const_iterator it = listItem_.begin();
	while (it != listItem_.end()) {
		status = (*it)->create(pParent, fonts, (*pnId)++);
		CHECK_QSTATUS();
		++it;
	}
	
	return QSTATUS_SUCCESS;
}

QSTATUS qm::LineLayoutLine::destroy() const
{
	DECLARE_QSTATUS();
	
	ItemList::const_iterator it = listItem_.begin();
	while (it != listItem_.end()) {
		status = (*it)->destroy();
		CHECK_QSTATUS();
		++it;
	}
	
	return QSTATUS_SUCCESS;
}

QSTATUS qm::LineLayoutLine::layout(const RECT& rect, unsigned int nFontHeight) const
{
	DECLARE_QSTATUS();
	
	int nWidth = rect.right - rect.left - (listItem_.size() - 1)*2 - 10;
	
	typedef std::vector<int> WidthList;
	WidthList listWidth;
	status = STLWrapper<WidthList>(listWidth).resize(listItem_.size());
	CHECK_QSTATUS();
	
	int nFixedWidth = 0;
	int nPercentWidth = 0;
	int nNoWidthCount = 0;
	for (ItemList::size_type n = 0; n < listItem_.size(); ++n) {
		double dItemWidth = listItem_[n]->getWidth();
		int nItemWidth = 0;
		switch (listItem_[n]->getUnit()) {
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
		default:
			assert(false);
			break;
		}
		listWidth[n] = nItemWidth;
	}
	
	if (nWidth > nFixedWidth) {
		WidthList::iterator it = listWidth.begin();
		while (it != listWidth.end()) {
			if (*it < 0)
				*it = (nWidth - nFixedWidth)*(-*it)/100;
			else if (*it == 0 && nPercentWidth < 100)
				*it = (nWidth - nFixedWidth)*(100 - nPercentWidth)/100/nNoWidthCount;
			++it;
		}
	}
	else {
		WidthList::iterator it = listWidth.begin();
		while (it != listWidth.end()) {
			if (*it < 0)
				*it = 0;
			++it;
		}
	}
	
	RECT r = { 5, rect.top, 5, rect.bottom };
	for (n = 0; n < listItem_.size(); ++n) {
		r.right = r.left + listWidth[n];
		status = listItem_[n]->layout(r, nFontHeight);
		CHECK_QSTATUS();
		r.left = r.right + 2;
	}
	
	return QSTATUS_SUCCESS;
}

QSTATUS qm::LineLayoutLine::show(bool bShow) const
{
	DECLARE_QSTATUS();
	
	ItemList::const_iterator it = listItem_.begin();
	while (it != listItem_.end()) {
		status = (*it)->show(bShow);
		CHECK_QSTATUS();
		++it;
	}
	
	return QSTATUS_SUCCESS;
}

QSTATUS qm::LineLayoutLine::addItem(LineLayoutItem* pItem)
{
	return STLWrapper<ItemList>(listItem_).push_back(pItem);
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

qm::LineLayoutItem::LineLayoutItem(QSTATUS* pstatus) :
	dWidth_(0),
	unit_(UNIT_NONE)
{
	assert(pstatus);
	*pstatus = QSTATUS_SUCCESS;
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

void qm::LineLayoutItem::setWidth(double dWidth, Unit unit)
{
	dWidth_ = dWidth;
	unit_ = unit;
}

QSTATUS qm::LineLayoutItem::setWidth(const WCHAR* pwszWidth)
{
	WCHAR* pEnd = 0;
	dWidth_ = wcstod(pwszWidth, &pEnd);
	if (dWidth_ < 0)
		return QSTATUS_FAIL;
	if (*pEnd == L'%') {
		if (*(pEnd + 1))
			return QSTATUS_FAIL;
		unit_ = UNIT_PERCENT;
	}
	else if (wcscmp(pEnd, L"em") == 0) {
		unit_ = UNIT_EM;
	}
	else if (!*pEnd || wcscmp(pEnd, L"px") == 0) {
		unit_ = UNIT_PIXEL;
	}
	else {
		return QSTATUS_FAIL;
	}
	
	return QSTATUS_SUCCESS;
}
