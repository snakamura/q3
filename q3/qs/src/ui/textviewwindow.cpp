/*
 * $Id$
 *
 * Copyright(C) 1998-2004 Satoshi Nakamura
 * All rights reserved.
 *
 */

#if 0

#include <qstextviewwindow.h>
#include <qsdevicecontext.h>
#include <qsstream.h>
#include <qsconv.h>
#include <qsinit.h>

#pragma warning(disable:4786)

#include <vector>

#include <tchar.h>

#ifdef QS_KCONVERT
#	include <kctrl.h>
#endif

using namespace qs;


/****************************************************************************
 *
 * TextViewWindowImpl
 *
 */

struct qs::TextViewWindowImpl
{
	enum {
		BUFFER_SIZE	= 1024
	};
	
	enum {
		TIMER_LOAD			= 1000,
		TIMER_DRAGSCROLL	= 1001
	};
	
	enum {
		LOAD_INTERVAL		= 0
	};
	
	enum {
		DRAGSCROLL_BORDER	= 30,
		DRAGSCROLL_INTERVAL	= 50
	};
	
	enum Selection {
		SELECTION_NONE,
		SELECTION_PARTIAL,
		SELECTION_ALL
	};
	
	struct Line
	{
		WCHAR* pBegin_;
		WCHAR* pEnd_;
	};
	
	typedef std::vector<Line> LineList;
	
	QSTATUS appendText(const WCHAR* pwszText, size_t nLen);
	QSTATUS clearText();
	
	QSTATUS getTextFromPoint(const POINT& pt,
		bool bIncludeNewLine, const WCHAR** ppText) const;
	QSTATUS getTextFromPos(int nLine, int x,
		bool bIncludeNewLine, const WCHAR** ppText) const;
	QSTATUS getLine(const WCHAR* p, int* pnLine) const;
	QSTATUS getPosition(int nLine, const WCHAR* pPos, int* pnPos) const;
	
	QSTATUS getSelection(const Line& line, Selection* pSelection,
		const WCHAR** ppStart, const WCHAR** ppEnd);
	QSTATUS startSelection(const POINT& pt);
	QSTATUS updateSelection(const POINT& pt);
	QSTATUS clearSelection();
	
	QSTATUS invalidateText(const WCHAR* pBegin, const WCHAR* pEnd);
	QSTATUS invalidateLines(int nStartLine, int nEndLine);
	
	QSTATUS scrollHorizontal(int nPos);
	QSTATUS scrollVirtical(int nPos);
	QSTATUS updateScrollBar();
	
	QSTATUS showCaret();
	QSTATUS hideCaret();
	QSTATUS updateCaret(bool bScroll);
	
	QSTATUS getClientRectWithoutMargin(RECT* pRect) const;
	QSTATUS getLineHeight(int* pnHeight) const;
	QSTATUS getLineInWindow(int* pnLine) const;
	QSTATUS getAverageCharWidth(int* pnWidth) const;
	QSTATUS getFixedCharWidth(int* pnWidth) const;
	QSTATUS getMaxWidth(int* pnMaxWidth) const;
	QSTATUS getNewLineBitmap(HBITMAP* phbm) const;
	
	QSTATUS calcLine();
	QSTATUS recalcLine();
	QSTATUS getCharInLine(int* pnCharInLine) const;
	QSTATUS getNextTabStop(int n, int* pnNext) const;
	QSTATUS getFontCharWidth(WCHAR c, int* pnWidth) const;
	QSTATUS allocBuffer(size_t nSize);
	
	TextViewWindow* pThis_;
	
	HFONT hfont_;
	int nCharInLine_;
	int nLineSpacing_;
	bool bShowNewLine_;
	int nTabWidth_;
	WCHAR cTabChar_;
	int nMarginLeft_;
	int nMarginTop_;
	int nMarginRight_;
	int nMarginBottom_;
	COLORREF crForeground_;
	COLORREF crBackground_;
	bool bShowVerticalScrollBar_;
	bool bShowHorizontalScrollBar_;
	bool bShowCaret_;
	
	const WCHAR* pSelectionStart_;
	const WCHAR* pSelectionEnd_;
	POINT ptScroll_;
	unsigned int nTimerDragScroll_;
	const WCHAR* pCaret_;
	POINT ptCaret_;
	int nCaretPos_;
	
	Reader* pReader_;
	unsigned int nTimerLoad_;
	
	WCHAR* pBuf_;
	WCHAR* pBufEnd_;
	WCHAR* pCurrent_;
	
	LineList listLine_;
	
	bool bProportional_;
	mutable int nCalculatedCharInLine_;
	mutable int nLineHeight_;
	mutable int nLineInWindow_;
	mutable int nAverageCharWidth_;
	mutable int nFixedCharWidth_;
	mutable HBITMAP hbmNewLine_;
};

QSTATUS qs::TextViewWindowImpl::appendText(const WCHAR* pwszText, size_t nLen)
{
	assert(pwszText);
	assert(nLen != static_cast<size_t>(-1));
	
	DECLARE_QSTATUS();
	
	if (static_cast<size_t>(pBufEnd_ - pCurrent_) < nLen) {
		status = allocBuffer(nLen);
		CHECK_QSTATUS();
	}
	WCHAR* p = pCurrent_;
	memcpy(pCurrent_, pwszText, nLen*sizeof(WCHAR));
	pCurrent_ += nLen;
	
	status = calcLine();
	CHECK_QSTATUS();
	
	status = updateScrollBar();
	CHECK_QSTATUS();
	
	return QSTATUS_SUCCESS;
}

QSTATUS qs::TextViewWindowImpl::clearText()
{
	DECLARE_QSTATUS();
	
	pCurrent_ = pBuf_;
	listLine_.clear();
	
	pSelectionStart_ = 0;
	pSelectionEnd_ = 0;
	
	ptScroll_.x = 0;
	ptScroll_.y = 0;
	
	status = updateScrollBar();
	CHECK_QSTATUS();
	
	pThis_->invalidate();
	
	return QSTATUS_SUCCESS;
}

QSTATUS qs::TextViewWindowImpl::getTextFromPoint(
	const POINT& pt, bool bIncludeNewLine, const WCHAR** ppText) const
{
	assert(ppText);
	
	DECLARE_QSTATUS();
	
	int nLineHeight = 0;
	status = getLineHeight(&nLineHeight);
	CHECK_QSTATUS();
	
	int nLine = ptScroll_.y + (pt.y - nMarginTop_)/nLineHeight;
	if (nLine < 0) {
		*ppText = pBuf_;
	}
	else if (static_cast<LineList::size_type>(nLine) < listLine_.size()) {
		status = getTextFromPos(nLine, pt.x, bIncludeNewLine, ppText);
		CHECK_QSTATUS();
	}
	else {
		*ppText = pCurrent_;
	}
	
	return QSTATUS_SUCCESS;
}

QSTATUS qs::TextViewWindowImpl::getTextFromPos(
	int nLine, int x, bool bIncludeNewLine, const WCHAR** ppText) const
{
	assert(ppText);
	
	DECLARE_QSTATUS();
	
	const Line& line = listLine_[nLine];
	
	x += ptScroll_.x - nMarginLeft_;
	if (bProportional_) {
		// TODO
	}
	else {
		int nCharWidth = 0;
		status = getFixedCharWidth(&nCharWidth);
		
		int nCharIndex = x/nCharWidth;
		int n = 0;
		const WCHAR* p = line.pBegin_;
		const WCHAR* pEnd = line.pEnd_;
		if (!bIncludeNewLine && p != pEnd && *(pEnd - 1) == L'\n')
			--pEnd;
		while (p != pEnd) {
			int nCharWidth = 0;
			status = getFontCharWidth(*p, &nCharWidth);
			CHECK_QSTATUS();
			if (n + nCharWidth > nCharIndex)
				break;
			n += nCharWidth;
			++p;
		}
		*ppText = p;
	}
	
	return QSTATUS_SUCCESS;
}

QSTATUS qs::TextViewWindowImpl::getLine(const WCHAR* p, int* pnLine) const
{
	assert(pnLine);
	
	*pnLine = -1;
	
	// TODO
	// Change to use binary search?
	
	LineList::size_type n = ptScroll_.y;
	if (!listLine_.empty() && p < listLine_[n].pBegin_)
		n = 0;
	for (; n < listLine_.size(); ++n) {
		if (listLine_[n].pEnd_ > p) {
			*pnLine = n;
			break;
		}
	}
	if (*pnLine == -1)
		*pnLine = n - 1;
	
	return QSTATUS_SUCCESS;
}

QSTATUS qs::TextViewWindowImpl::getPosition(
	int nLine, const WCHAR* pPos, int* pnPos) const
{
	assert(0 <= nLine && static_cast<LineList::size_type>(nLine) < listLine_.size());
	assert(pPos);
	assert(pnPos);
	assert(listLine_[nLine].pBegin_ <= pPos &&
		pPos <= listLine_[nLine].pEnd_);
	
	DECLARE_QSTATUS();
	
	const Line& line = listLine_[nLine];
	if (bProportional_) {
		// TODO
	}
	else {
		int nFixedCharWidth = 0;
		status = getFixedCharWidth(&nFixedCharWidth);
		CHECK_QSTATUS();
		
		int nWidth = 0;
		const WCHAR* p = line.pBegin_;
		while (p < pPos) {
			if (*p == L'\t') {
				status = getNextTabStop(nWidth, &nWidth);
				CHECK_QSTATUS();
			}
			else {
				int nCharWidth = 0;
				status = getFontCharWidth(*p, &nCharWidth);
				CHECK_QSTATUS();
				nWidth += nCharWidth;
			}
			++p;
		}
		*pnPos = nWidth*nFixedCharWidth;
	}
	return QSTATUS_SUCCESS;
}

QSTATUS qs::TextViewWindowImpl::getSelection(const Line& line,
	Selection* pSelection, const WCHAR** ppStart, const WCHAR** ppEnd)
{
	assert(pSelection);
	assert(ppStart);
	assert(ppEnd);
	
	*pSelection = SELECTION_NONE;
	*ppStart = 0;
	*ppEnd = 0;
	
	const WCHAR* pStart = pSelectionStart_;
	const WCHAR* pEnd = pSelectionEnd_;
	if (pStart != pEnd) {
		if (pStart > pEnd)
			std::swap(pStart, pEnd);
		
		if (line.pBegin_ < pEnd) {
			if (pStart < line.pBegin_) {
				*ppStart = line.pBegin_;
				if (line.pEnd_ < pEnd) {
					*pSelection = SELECTION_ALL;
					*ppEnd = line.pEnd_;
				}
				else {
					*pSelection = SELECTION_PARTIAL;
					*ppEnd = pEnd;
				}
			}
			else if (pStart < line.pEnd_) {
				*pSelection = SELECTION_PARTIAL;
				*ppStart = pStart;
				if (line.pEnd_ < pEnd)
					*ppEnd = line.pEnd_;
				else
					*ppEnd = pEnd;
			}
		}
	}
	
	return QSTATUS_SUCCESS;
}

QSTATUS qs::TextViewWindowImpl::startSelection(const POINT& pt)
{
	if (pSelectionStart_ != pSelectionEnd_)
		invalidateText(pSelectionStart_, pSelectionEnd_);
	pSelectionStart_ = 0;
	pSelectionEnd_ = 0;
	
	const WCHAR* pText = 0;
	if (getTextFromPoint(pt, true, &pText) == QSTATUS_SUCCESS) {
		pSelectionStart_ = pText;
		pSelectionEnd_ = pText;
		
		if (bShowCaret_)
			pThis_->moveCaret(TextViewWindow::MOVECARET_POS,
				pText, false, TextViewWindow::SELECT_NONE);
	}
	
	return QSTATUS_SUCCESS;
}

QSTATUS qs::TextViewWindowImpl::updateSelection(const POINT& pt)
{
	const WCHAR* pText = 0;
	getTextFromPoint(pt, true, &pText);
	invalidateText(pSelectionEnd_, pText);
	pSelectionEnd_ = pText;
	
	if (bShowCaret_)
		pThis_->moveCaret(TextViewWindow::MOVECARET_POS,
			pText, false, TextViewWindow::SELECT_NONE);
	
	return QSTATUS_SUCCESS;
}

QSTATUS qs::TextViewWindowImpl::clearSelection()
{
	DECLARE_QSTATUS();
	
	if (pSelectionStart_ != pSelectionEnd_) {
		status = invalidateText(pSelectionStart_, pSelectionEnd_);
		CHECK_QSTATUS();
	}
	pSelectionStart_ = 0;
	pSelectionEnd_ = 0;
	
	return QSTATUS_SUCCESS;
}

QSTATUS qs::TextViewWindowImpl::invalidateText(
	const WCHAR* pBegin, const WCHAR* pEnd)
{
	DECLARE_QSTATUS();
	
	if (pBegin > pEnd)
		std::swap(pBegin, pEnd);
	
	int nStartLine = 0;
	status = getLine(pBegin, &nStartLine);
	
	int nEndLine = 0;
	status = getLine(pEnd, &nEndLine);
	
	return invalidateLines(nStartLine, nEndLine + 1);
}

QSTATUS qs::TextViewWindowImpl::invalidateLines(int nStartLine, int nEndLine)
{
	DECLARE_QSTATUS();
	
	RECT rect;
	pThis_->getClientRect(&rect);
	
	int nLineHeight = 0;
	status = getLineHeight(&nLineHeight);
	CHECK_QSTATUS();
	
	rect.top = nMarginTop_ + (nStartLine - ptScroll_.y)*nLineHeight;
	rect.bottom = nMarginTop_ + (nEndLine - ptScroll_.y)*nLineHeight;
	
	pThis_->invalidateRect(rect);
	
	return QSTATUS_SUCCESS;
}

QSTATUS qs::TextViewWindowImpl::scrollHorizontal(int nPos)
{
	DECLARE_QSTATUS();
	
	if (nPos != ptScroll_.x) {
		RECT rectClip;
		getClientRectWithoutMargin(&rectClip);
		pThis_->scrollWindow(ptScroll_.x - nPos, 0,
			&rectClip, &rectClip, 0, 0, SW_INVALIDATE);
		
		ptScroll_.x = nPos;
		updateScrollBar();
	}
	return QSTATUS_SUCCESS;
}

QSTATUS qs::TextViewWindowImpl::scrollVirtical(int nPos)
{
	DECLARE_QSTATUS();
	
	if (nPos != ptScroll_.y) {
		int nLineHeight = 0;
		status = getLineHeight(&nLineHeight);
		CHECK_QSTATUS();
		
		RECT rectClip;
		getClientRectWithoutMargin(&rectClip);
		pThis_->scrollWindow(0, (ptScroll_.y - nPos)*nLineHeight,
			&rectClip, &rectClip, 0, 0, SW_INVALIDATE);
		
		ptScroll_.y = nPos;
		updateScrollBar();
	}
	return QSTATUS_SUCCESS;
}

QSTATUS qs::TextViewWindowImpl::updateScrollBar()
{
	DECLARE_QSTATUS();
	
	if (bShowVerticalScrollBar_) {
		int nLineInWindow = 0;
		status = getLineInWindow(&nLineInWindow);
		CHECK_QSTATUS();
		
		SCROLLINFO si = {
			sizeof(si),
			SIF_PAGE | SIF_POS | SIF_RANGE | SIF_DISABLENOSCROLL,
			0,
			listLine_.size() - 1,
			nLineInWindow,
			ptScroll_.y,
			ptScroll_.y
		};
		pThis_->setScrollInfo(SB_VERT, si, true);
	}
	if (bShowHorizontalScrollBar_) {
		int nMaxWidth = 0;
		status = getMaxWidth(&nMaxWidth);
		CHECK_QSTATUS();
		
		RECT rect;
		getClientRectWithoutMargin(&rect);
		
		SCROLLINFO si = {
			sizeof(si),
			SIF_PAGE | SIF_POS | SIF_RANGE | SIF_DISABLENOSCROLL,
			0,
			nMaxWidth,
			rect.right - rect.left,
			ptScroll_.x,
			ptScroll_.x
		};
		pThis_->setScrollInfo(SB_HORZ, si, true);
	}
	
	return QSTATUS_SUCCESS;
}

QSTATUS qs::TextViewWindowImpl::showCaret()
{
	DECLARE_QSTATUS();
	
	int nLineHeight = 0;
	getLineHeight(&nLineHeight);
	CHECK_QSTATUS();
	pThis_->createCaret(2, nLineHeight);
	POINT pt = {
		ptCaret_.x + nMarginLeft_,
		ptCaret_.y*nLineHeight + nMarginTop_
	};
	pThis_->setCaretPos(pt);
	pThis_->showCaret();
	
	return QSTATUS_SUCCESS;
}

QSTATUS qs::TextViewWindowImpl::hideCaret()
{
	pThis_->hideCaret();
	pThis_->destroyCaret();
	
	return QSTATUS_SUCCESS;
}

QSTATUS qs::TextViewWindowImpl::updateCaret(bool bScroll)
{
	DECLARE_QSTATUS();
	
	int nLineHeight = 0;
	status = getLineHeight(&nLineHeight);
	CHECK_QSTATUS();
	
	int nLineInWindow = 0;
	status = getLineInWindow(&nLineInWindow);
	CHECK_QSTATUS();
	
	if (bScroll) {
		RECT rect;
		getClientRectWithoutMargin(&rect);
		
		if (ptCaret_.y < ptScroll_.y + 3)
			pThis_->scroll(TextViewWindow::SCROLL_VIRTICALPOS,
				ptCaret_.y - 3, false);
		else if (ptCaret_.y > ptScroll_.y + nLineInWindow - 3)
			pThis_->scroll(TextViewWindow::SCROLL_VIRTICALPOS,
				ptCaret_.y - nLineInWindow + 3, false);
		
		if (ptCaret_.x < ptScroll_.x + 20)
			pThis_->scroll(TextViewWindow::SCROLL_HORIZONTALPOS,
				ptCaret_.x - 20, false);
		else if (ptCaret_.x > ptScroll_.x + (rect.right - rect.left) - 20)
			pThis_->scroll(TextViewWindow::SCROLL_HORIZONTALPOS,
				ptCaret_.x - (rect.right - rect.left) + 20, false);
	}
	
	POINT ptCaret = {
		ptCaret_.x + nMarginLeft_ - ptScroll_.x,
		(ptCaret_.y - ptScroll_.y)*nLineHeight + nMarginTop_
	};
	pThis_->setCaretPos(ptCaret);
	
	return QSTATUS_SUCCESS;
}

QSTATUS qs::TextViewWindowImpl::getClientRectWithoutMargin(RECT* pRect) const
{
	assert(pRect);
	
	pThis_->getClientRect(pRect);
	pRect->left += nMarginLeft_;
	pRect->top += nMarginTop_;
	pRect->right -= nMarginRight_;
	pRect->bottom -= nMarginBottom_;
	
	return QSTATUS_SUCCESS;
}

QSTATUS qs::TextViewWindowImpl::getLineHeight(int* pnHeight) const
{
	assert(pnHeight);
	
	if (nLineHeight_ == 0) {
		DECLARE_QSTATUS();
		
		ClientDeviceContext dc(pThis_->getHandle(), &status);
		CHECK_QSTATUS();
		
		ObjectSelector<HFONT> selector(dc, hfont_);
		TEXTMETRIC tm;
		dc.getTextMetrics(&tm);
		nLineHeight_ = tm.tmHeight + tm.tmExternalLeading + nLineSpacing_;
	}
	*pnHeight = nLineHeight_;
	
	return QSTATUS_SUCCESS;
}

QSTATUS qs::TextViewWindowImpl::getLineInWindow(int* pnLine) const
{
	assert(pnLine);
	
	if (nLineInWindow_ == 0) {
		DECLARE_QSTATUS();
		
		int nLineHeight = 0;
		status = getLineHeight(&nLineHeight);
		CHECK_QSTATUS();
		
		RECT rect;
		getClientRectWithoutMargin(&rect);
		
		nLineInWindow_ = (rect.bottom - rect.top -
			nMarginTop_ - nMarginBottom_)/nLineHeight;
	}
	*pnLine = nLineInWindow_;
	
	return QSTATUS_SUCCESS;
}

QSTATUS qs::TextViewWindowImpl::getAverageCharWidth(int* pnWidth) const
{
	assert(pnWidth);
	
	if (nAverageCharWidth_ == 0) {
		DECLARE_QSTATUS();
		
		ClientDeviceContext dc(pThis_->getHandle(), &status);
		CHECK_QSTATUS();
		
		ObjectSelector<HFONT> selector(dc, hfont_);
		TEXTMETRIC tm;
		dc.getTextMetrics(&tm);
		
		nAverageCharWidth_ = tm.tmAveCharWidth + tm.tmOverhang;
	}
	*pnWidth = nAverageCharWidth_;
	
	return QSTATUS_SUCCESS;
}

QSTATUS qs::TextViewWindowImpl::getFixedCharWidth(int* pnWidth) const
{
	assert(pnWidth);
	
	if (nFixedCharWidth_ == 0) {
		DECLARE_QSTATUS();
		
		ClientDeviceContext dc(pThis_->getHandle(), &status);
		CHECK_QSTATUS();
		
		ObjectSelector<HFONT> selector(dc, hfont_);
		TEXTMETRIC tm;
		dc.getTextMetrics(&tm);
		
		nFixedCharWidth_ = tm.tmAveCharWidth + tm.tmOverhang;
	}
	*pnWidth = nFixedCharWidth_;
	
	return QSTATUS_SUCCESS;
}

QSTATUS qs::TextViewWindowImpl::getMaxWidth(int* pnMaxWidth) const
{
	assert(pnMaxWidth);
	
	DECLARE_QSTATUS();
	
	if (bProportional_) {
		// TODO
		*pnMaxWidth = 0;
	}
	else {
		int nCharInLine = 0;
		status = getCharInLine(&nCharInLine);
		CHECK_QSTATUS();
		
		int nFixedCharWidth = 0;
		status = getFixedCharWidth(&nFixedCharWidth);
		CHECK_QSTATUS();
		
		*pnMaxWidth = nCharInLine*nFixedCharWidth;
	}
	
	return QSTATUS_SUCCESS;
}

QSTATUS qs::TextViewWindowImpl::getNewLineBitmap(HBITMAP* phbm) const
{
	assert(phbm);
	
	if (!hbmNewLine_) {
		DECLARE_QSTATUS();
		
		ClientDeviceContext dc(pThis_->getHandle(), &status);
		CHECK_QSTATUS();
		CompatibleDeviceContext dcMem(dc, &status);
		CHECK_QSTATUS();
		
		int nWidth = 0;
		status = getFixedCharWidth(&nWidth);
		CHECK_QSTATUS();
		int nHeight = 0;
		status = getLineHeight(&nHeight);
		CHECK_QSTATUS();
		nHeight -= nLineSpacing_;
		
		GdiObject<HBITMAP> hbm(::CreateCompatibleBitmap(dc, nWidth, nHeight));
		if (!hbm.get())
			return QSTATUS_FAIL;
		ObjectSelector<HBITMAP> bitmapSelector(dcMem, hbm.get());
		
		GdiObject<HPEN> hpen(::CreatePen(PS_SOLID, 0, crForeground_));
		if (!hpen.get())
			return QSTATUS_FAIL;
		ObjectSelector<HPEN> penSelector(dcMem, hpen.get());
		
		RECT rect = { 0, 0, nWidth, nHeight };
		dcMem.fillSolidRect(rect, crBackground_);
		
		POINT pt[3];
		pt[0].x = nWidth/2;
		pt[0].y = 3;
		pt[1].x= pt[0].x;
		pt[1].y = nHeight - 2;
		dcMem.polyline(pt, 2);
		pt[0].x = pt[0].x - QSMAX(nWidth/4 + 1, 3);
		pt[0].y = pt[1].y - (pt[1].x - pt[0].x);
		dcMem.polyline(pt, 2);
		pt[0].x = pt[1].x + (pt[1].x - pt[0].x);
		dcMem.polyline(pt, 2);
		
		hbmNewLine_ = hbm.release();
	}
	*phbm = hbmNewLine_;
	
	return QSTATUS_SUCCESS;
}

QSTATUS qs::TextViewWindowImpl::calcLine()
{
	DECLARE_QSTATUS();
	
	if (bProportional_) {
		// TODO
	}
	else {
		int nCharInLine = 0;
		status = getCharInLine(&nCharInLine);
		CHECK_QSTATUS();
		
		WCHAR* p = pBuf_;
		if (!listLine_.empty()) {
			p = listLine_.back().pBegin_;
			listLine_.pop_back();
		}
		WCHAR* pBegin = p;
		int nChar = 0;
		while (p < pCurrent_) {
			int nCharNext = 0;
			if (*p == L'\t') {
				status = getNextTabStop(nChar, &nCharNext);
				CHECK_QSTATUS();
			}
			else if (*p == L'\n') {
				Line line = { pBegin, p + 1 };
				status = STLWrapper<LineList>(listLine_).push_back(line);
				CHECK_QSTATUS();
				pBegin = p + 1;
				nChar = 0;
			}
			else {
				status = getFontCharWidth(*p, &nCharNext);
				CHECK_QSTATUS();
				nCharNext += nChar;
			}
			if (nCharNext > nCharInLine) {
				Line line = { pBegin, p };
				status = STLWrapper<LineList>(listLine_).push_back(line);
				CHECK_QSTATUS();
				pBegin = p;
				nChar = 0;
				--p;
			}
			else {
				nChar = nCharNext;
			}
			++p;
		}
		
		Line line = { pBegin, p };
		status = STLWrapper<LineList>(listLine_).push_back(line);
		CHECK_QSTATUS();
	}
	
	return QSTATUS_SUCCESS;
}

QSTATUS qs::TextViewWindowImpl::recalcLine()
{
	listLine_.clear();
	return calcLine();
}

QSTATUS qs::TextViewWindowImpl::getCharInLine(int* pnCharInLine) const
{
	assert(pnCharInLine);
	
	if (nCalculatedCharInLine_ == 0) {
		if (nCharInLine_ == -1) {
			assert(!bProportional_);
			
			DECLARE_QSTATUS();
			
			RECT rect;
			getClientRectWithoutMargin(&rect);
			
			int nFixedCharWidth = 0;
			status = getFixedCharWidth(&nFixedCharWidth);
			CHECK_QSTATUS();
			
			nCalculatedCharInLine_ = (rect.right - rect.left)/nFixedCharWidth;
		}
		else {
			nCalculatedCharInLine_ = nCharInLine_;
		}
	}
	*pnCharInLine = nCalculatedCharInLine_;
	
	return QSTATUS_SUCCESS;
}

QSTATUS qs::TextViewWindowImpl::getNextTabStop(int n, int* pnNext) const
{
	assert(pnNext);
	*pnNext = (n/nTabWidth_ + 1)*nTabWidth_;
	return QSTATUS_SUCCESS;
}

QSTATUS qs::TextViewWindowImpl::getFontCharWidth(WCHAR c, int* pnWidth) const
{
	assert(pnWidth);
#ifdef QS_KCONVERT
	*pnWidth = ::is_hankaku(c) ? 1 : 2;
#else
	// TODO
	// Improve performance
	*pnWidth = ::WideCharToMultiByte(CP_ACP, 0, &c, 1, 0, 0, 0, 0);
#endif
	return QSTATUS_SUCCESS;
}

QSTATUS qs::TextViewWindowImpl::allocBuffer(size_t nSize)
{
	size_t nNewSize = 0;
	if (pBuf_ == pBufEnd_) {
		nNewSize = QSMAX(nSize, static_cast<size_t>(BUFFER_SIZE));
	}
	else {
		nNewSize = pBufEnd_ - pBuf_;
		nNewSize += QSMAX(nSize, nNewSize);
	}
	++nNewSize;
	
	WCHAR* pNew = static_cast<WCHAR*>(realloc(pBuf_, nNewSize*sizeof(WCHAR)));
	if (!pNew)
		return QSTATUS_OUTOFMEMORY;
	
	if (pNew != pBuf_) {
		LineList::iterator it = listLine_.begin();
		while (it != listLine_.end()) {
			(*it).pBegin_ = (*it).pBegin_ - pBuf_ + pNew;
			(*it).pEnd_ = (*it).pEnd_ - pBuf_ + pNew;
			++it;
		}
		if (pCaret_)
			pCaret_ = pCaret_ - pBuf_ + pNew;
		else
			pCaret_ = pNew;
	}
	
	pBufEnd_ = pNew + nNewSize;
	pCurrent_ = pNew + (pCurrent_ - pBuf_);
	pBuf_ = pNew;
	
	return QSTATUS_SUCCESS;
}


/****************************************************************************
 *
 * TextViewWindow
 *
 */

qs::TextViewWindow::TextViewWindow(bool bDeleteThis, QSTATUS* pstatus) :
	WindowBase(bDeleteThis, pstatus),
	DefaultWindowHandler(pstatus),
	pImpl_(0)
{
	if (*pstatus != QSTATUS_SUCCESS)
		return;
	
	DECLARE_QSTATUS();
	
	ClientDeviceContext dc(0, &status);
	CHECK_QSTATUS_SET(pstatus);
	LOGFONT lf;
	status = FontHelper::createLogFont(dc,
		Init::getInit().getDefaultFixedWidthFont(), 9, &lf);
	
	HFONT hfont = ::CreateFontIndirect(&lf);
	if (!hfont) {
		*pstatus = QSTATUS_FAIL;
		return;
	}
	
	status = newObject(&pImpl_);
	CHECK_QSTATUS_SET(pstatus);
	pImpl_->pThis_ = this;
	pImpl_->hfont_ = hfont;
	pImpl_->nCharInLine_ = -1;
	pImpl_->nLineSpacing_ = 0;
	pImpl_->bShowNewLine_ = false;
	pImpl_->nTabWidth_ = 4;
	pImpl_->cTabChar_ = L' ';
	pImpl_->nMarginLeft_ = 5;
	pImpl_->nMarginTop_ = 5;
	pImpl_->nMarginRight_ = 0;
	pImpl_->nMarginBottom_ = 0;
	pImpl_->crForeground_ = ::GetSysColor(COLOR_WINDOWTEXT);
	pImpl_->crBackground_ = ::GetSysColor(COLOR_WINDOW);
	pImpl_->bShowVerticalScrollBar_ = true;
	pImpl_->bShowHorizontalScrollBar_ = false;
	pImpl_->bShowCaret_ = false;
	pImpl_->pSelectionStart_ = 0;
	pImpl_->pSelectionEnd_ = 0;
	pImpl_->ptScroll_.x = 0;
	pImpl_->ptScroll_.y = 0;
	pImpl_->nTimerDragScroll_ = 0;
	pImpl_->pCaret_ = 0;
	pImpl_->ptCaret_.x = 0;
	pImpl_->ptCaret_.y = 0;
	pImpl_->nCaretPos_ = 0;
	pImpl_->pBuf_ = 0;
	pImpl_->pBufEnd_ = 0;
	pImpl_->pCurrent_ = 0;
	pImpl_->pReader_ = 0;
	pImpl_->nTimerLoad_ = 0;
	pImpl_->bProportional_ = false;
	pImpl_->nCalculatedCharInLine_ = 0;
	pImpl_->nLineHeight_ = 0;
	pImpl_->nLineInWindow_ = 0;
	pImpl_->nAverageCharWidth_ = 0;
	pImpl_->nFixedCharWidth_ = 0;
	pImpl_->hbmNewLine_ = 0;
	
	setWindowHandler(this, false);
}

qs::TextViewWindow::~TextViewWindow()
{
	if (pImpl_) {
		if (pImpl_->hfont_)
			::DeleteObject(pImpl_->hfont_);
		free(pImpl_->pBuf_);
		if (pImpl_->hbmNewLine_)
			::DeleteObject(pImpl_->hbmNewLine_);
		delete pImpl_;
		pImpl_ = 0;
	}
}

QSTATUS qs::TextViewWindow::getOption(Option* pOption)
{
	assert(pOption);
	
	if (pOption->nMask_ & Option::MASK_CHARINLINE)
		pOption->nCharInLine_ = pImpl_->nCharInLine_;
	if (pOption->nMask_ & Option::MASK_LINESPACING)
		pOption->nLineSpacing_ = pImpl_->nLineSpacing_;
	if (pOption->nMask_ & Option::MASK_NEWLINE)
		pOption->bShowNewLine_ = pImpl_->bShowNewLine_;
	if (pOption->nMask_ & Option::MASK_TAB) {
		pOption->nTabWidth_ = pImpl_->nTabWidth_;
		pOption->cTabChar_ = pImpl_->cTabChar_;
	}
	if (pOption->nMask_ & Option::MASK_MARGIN) {
		pOption->nMarginLeft_ = pImpl_->nMarginLeft_;
		pOption->nMarginTop_ = pImpl_->nMarginTop_;
		pOption->nMarginRight_ = pImpl_->nMarginRight_;
		pOption->nMarginBottom_ = pImpl_->nMarginBottom_;
	}
	if (pOption->nMask_ & Option::MASK_COLOR) {
		pOption->crForeground_ = pImpl_->crForeground_;
		pOption->crBackground_ = pImpl_->crBackground_;
	}
	if (pOption->nMask_ & Option::MASK_SCROLLBAR) {
		pOption->bShowVerticalScrollBar_ = pImpl_->bShowVerticalScrollBar_;
		pOption->bShowHorizontalScrollBar_ = pImpl_->bShowHorizontalScrollBar_;
	}
	if (pOption->nMask_ & Option::MASK_CARET)
		pOption->bShowCaret_ = pImpl_->bShowCaret_;
	
	return QSTATUS_SUCCESS;
}

QSTATUS qs::TextViewWindow::setOption(const Option& option)
{
	enum {
		UPDATE_RECALC	= 0x01,
		UPDATE_REDRAW	= 0x02
	};
	unsigned int nUpdate = 0;
	
	if (option.nMask_ & Option::MASK_FONT) {
		if (pImpl_->hfont_)
			::DeleteObject(pImpl_->hfont_);
		pImpl_->hfont_ = ::CreateFontIndirect(&option.logfont_);
		pImpl_->bProportional_ = (option.logfont_.lfPitchAndFamily & FIXED_PITCH) == 0;
		nUpdate |= UPDATE_RECALC | UPDATE_REDRAW;
	}
	if (option.nMask_ & Option::MASK_LINESPACING) {
		pImpl_->nLineSpacing_ = option.nLineSpacing_;
		nUpdate |= UPDATE_REDRAW;
	}
	if (option.nMask_ & Option::MASK_CHARINLINE) {
		pImpl_->nCharInLine_ = option.nCharInLine_;
		nUpdate |= UPDATE_RECALC | UPDATE_REDRAW;
	}
	if (option.nMask_ & Option::MASK_NEWLINE) {
		pImpl_->bShowNewLine_ = option.bShowNewLine_;
		nUpdate |= UPDATE_REDRAW;
	}
	if (option.nMask_ & Option::MASK_TAB) {
		if (option.nTabWidth_ != 0)
			pImpl_->nTabWidth_ = option.nTabWidth_;
		pImpl_->cTabChar_ = option.cTabChar_;
		nUpdate |= UPDATE_REDRAW;
	}
	if (option.nMask_ & Option::MASK_MARGIN) {
		pImpl_->nMarginLeft_ = option.nMarginLeft_;
		pImpl_->nMarginTop_ = option.nMarginTop_;
		pImpl_->nMarginRight_ = option.nMarginRight_;
		pImpl_->nMarginBottom_ = option.nMarginBottom_;
		nUpdate |= (pImpl_->nCharInLine_ == -1 ? UPDATE_RECALC : 0) | UPDATE_REDRAW;
	}
	if (option.nMask_ & Option::MASK_COLOR) {
		pImpl_->crForeground_ = option.crForeground_;
		pImpl_->crBackground_ = option.crBackground_;
		nUpdate |= UPDATE_REDRAW;
	}
	if (option.nMask_ & Option::MASK_SCROLLBAR) {
		pImpl_->bShowVerticalScrollBar_ = option.bShowVerticalScrollBar_;
		pImpl_->bShowHorizontalScrollBar_ = option.bShowHorizontalScrollBar_;
		nUpdate |= (pImpl_->nCharInLine_ == -1 ? UPDATE_RECALC : 0) | UPDATE_REDRAW;
	}
	if (option.nMask_ & Option::MASK_CARET) {
		if (!pImpl_->bShowCaret_ && option.bShowCaret_) {
			if (!pImpl_->listLine_.empty())
				pImpl_->pCaret_ = pImpl_->listLine_[pImpl_->ptScroll_.y].pBegin_;
			else
				pImpl_->pCaret_ = 0;
			pImpl_->ptCaret_.x = 0;
			pImpl_->ptCaret_.y = pImpl_->ptScroll_.y;
			pImpl_->nCaretPos_ = 0;
		}
		pImpl_->bShowCaret_ = option.bShowCaret_;
	}
	
	pImpl_->nLineHeight_ = 0;
	pImpl_->nLineInWindow_ = 0;
	pImpl_->nAverageCharWidth_ = 0;
	pImpl_->nFixedCharWidth_ = 0;
	if (pImpl_->hbmNewLine_) {
		::DeleteObject(pImpl_->hbmNewLine_);
		pImpl_->hbmNewLine_ = 0;
	}
	
	if (getHandle()) {
		if (nUpdate & UPDATE_RECALC)
			pImpl_->recalcLine();
		if (nUpdate & UPDATE_REDRAW)
			invalidate();
	}
	
	return QSTATUS_SUCCESS;
}

QSTATUS qs::TextViewWindow::getText(
	const WCHAR** ppwszText, size_t* pnLen) const
{
	assert(ppwszText);
	assert(pnLen);
	
	*ppwszText = pImpl_->pBuf_;
	*pnLen = pImpl_->pCurrent_ - pImpl_->pBuf_;
	
	return QSTATUS_SUCCESS;
}

QSTATUS qs::TextViewWindow::setText(const WCHAR* pwszText, size_t nLen)
{
	DECLARE_QSTATUS();
	
	status = pImpl_->clearText();
	CHECK_QSTATUS();
	
	status = pImpl_->appendText(pwszText, nLen);
	CHECK_QSTATUS();
	
	return QSTATUS_SUCCESS;
}

QSTATUS qs::TextViewWindow::appendText(const WCHAR* pwszText, size_t nLen)
{
	DECLARE_QSTATUS();
	
	status = pImpl_->appendText(pwszText, nLen);
	CHECK_QSTATUS();
	
	return QSTATUS_SUCCESS;
}

QSTATUS qs::TextViewWindow::clearText()
{
	DECLARE_QSTATUS();
	
	status = pImpl_->clearText();
	CHECK_QSTATUS();
	
	return QSTATUS_SUCCESS;
}

QSTATUS qs::TextViewWindow::loadText(Reader* pReader, bool bAsync)
{
	assert(pReader);
	
	DECLARE_QSTATUS();
	
	status = cancelLoad();
	CHECK_QSTATUS();
	
	assert(!pImpl_->pReader_);
	
	status = clearText();
	CHECK_QSTATUS();
	
	int nLineInWindow = 0;
	status = pImpl_->getLineInWindow(&nLineInWindow);
	CHECK_QSTATUS();
	
	WCHAR wsz[1024];
	size_t nRead = 0;
	while (nRead != static_cast<size_t>(-1) &&
		(!bAsync ||
		pImpl_->listLine_.size() <
			static_cast<TextViewWindowImpl::LineList::size_type>(nLineInWindow))) {
		status = pReader->read(wsz, 1024, &nRead);
		CHECK_QSTATUS();
		if (nRead != static_cast<size_t>(-1))
			appendText(wsz, nRead);
	}
	invalidate();
	
	if (nRead != -1) {
		pImpl_->pReader_ = pReader;
		pImpl_->nTimerLoad_ = setTimer(TextViewWindowImpl::TIMER_LOAD,
			TextViewWindowImpl::LOAD_INTERVAL);
	}
	else {
		delete pReader;
	}
	
	return QSTATUS_SUCCESS;
}

QSTATUS qs::TextViewWindow::cancelLoad()
{
	if (pImpl_->pReader_) {
		killTimer(pImpl_->nTimerLoad_);
		delete pImpl_->pReader_;
		pImpl_->pReader_ = 0;
	}
	
	return QSTATUS_SUCCESS;
}

QSTATUS qs::TextViewWindow::getSelectedText(
	const WCHAR** ppwszText, size_t* pnLen) const
{
	assert(ppwszText);
	assert(pnLen);
	
	if (pImpl_->pSelectionStart_ < pImpl_->pSelectionEnd_) {
		*ppwszText = pImpl_->pSelectionStart_;
		*pnLen = pImpl_->pSelectionEnd_ - pImpl_->pSelectionStart_;
	}
	else if (pImpl_->pSelectionStart_ > pImpl_->pSelectionEnd_) {
		*ppwszText = pImpl_->pSelectionEnd_;
		*pnLen = pImpl_->pSelectionStart_ - pImpl_->pSelectionEnd_;
	}
	else {
		*ppwszText = 0;
		*pnLen = 0;
	}
	
	return QSTATUS_SUCCESS;
}

QSTATUS qs::TextViewWindow::select(const WCHAR* pwszBegin, size_t nLen)
{
	assert(pImpl_->pBuf_ <= pwszBegin && pwszBegin < pImpl_->pCurrent_);
	assert(pwszBegin + nLen <= pImpl_->pCurrent_);
	
	if (pImpl_->pSelectionStart_ != pImpl_->pSelectionEnd_)
		pImpl_->invalidateText(pImpl_->pSelectionStart_, pImpl_->pSelectionEnd_);
	pImpl_->pSelectionStart_ = pwszBegin;
	pImpl_->pSelectionEnd_ = pwszBegin + nLen;
	
	return QSTATUS_SUCCESS;
}

QSTATUS qs::TextViewWindow::getScrollPos(int* pnPos)
{
	assert(pnPos);
	*pnPos = pImpl_->ptScroll_.y;
	return QSTATUS_SUCCESS;
}

QSTATUS qs::TextViewWindow::scroll(Scroll scroll, int nPos, bool bRepeat)
{
	DECLARE_QSTATUS();
	
	if (scroll & SCROLL_VIRTICAL_MASK) {
		int nLineInWindow = 0;
		status = pImpl_->getLineInWindow(&nLineInWindow);
		CHECK_QSTATUS_VALUE(0);
		
		int nEnd = pImpl_->listLine_.size() - nLineInWindow;
		if (nEnd < 0)
			nEnd = 0;
		
		int nNewPos = pImpl_->ptScroll_.y;
		switch (scroll) {
		case SCROLL_TOP:
			nNewPos = 0;
			break;
		case SCROLL_BOTTOM:
			nNewPos = nEnd;
			break;
		case SCROLL_LINEUP:
			nNewPos -= bRepeat ? 2 : 1;
			break;
		case SCROLL_LINEDOWN:
			nNewPos += bRepeat ? 2 : 1;
			break;
		case SCROLL_PAGEUP:
			nNewPos -= nLineInWindow;
			break;
		case SCROLL_PAGEDOWN:
			nNewPos += nLineInWindow;
			break;
		case SCROLL_VIRTICALPOS:
			nNewPos = nPos;
			break;
		}
		if (nNewPos < 0)
			nNewPos = 0;
		else if (nNewPos > nEnd)
			nNewPos = nEnd;
		
		status = pImpl_->scrollVirtical(nNewPos);
	}
	else if (scroll & SCROLL_HORIZONTAL_MASK) {
		int nCharWidth = 0;
		status = pImpl_->getAverageCharWidth(&nCharWidth);
		CHECK_QSTATUS();
		
		RECT rect;
		pImpl_->getClientRectWithoutMargin(&rect);
		int nPage = rect.right - rect.left;
		
		int nMaxWidth = 0;
		status = pImpl_->getMaxWidth(&nMaxWidth);
		CHECK_QSTATUS();
		
		int nEnd = nMaxWidth - nPage;
		
		int nNewPos = pImpl_->ptScroll_.x;
		switch (scroll) {
		case SCROLL_LEFT:
			nNewPos = 0;
			break;
		case SCROLL_RIGHT:
			nNewPos = nEnd;
			break;
		case SCROLL_CHARLEFT:
			nNewPos -= (bRepeat ? 2 : 1)*nCharWidth;
			break;
		case SCROLL_CHARRIGHT:
			nNewPos += (bRepeat ? 2 : 1)*nCharWidth;
			break;
		case SCROLL_PAGELEFT:
			nNewPos -= nPage;
			break;
		case SCROLL_PAGERIGHT:
			nNewPos += nPage;
			break;
		case SCROLL_HORIZONTALPOS:
			nNewPos = nPos;
			break;
		}
		if (nNewPos < 0)
			nNewPos = 0;
		else if (nNewPos > nEnd)
			nNewPos = nEnd;
		
		status = pImpl_->scrollHorizontal(nNewPos);
	}
	
	pImpl_->updateCaret(false);
	
	return status;
}

QSTATUS qs::TextViewWindow::moveCaret(MoveCaret moveCaret,
	const WCHAR* pText, bool bRepeat, Select select)
{
	if (!pImpl_->listLine_.empty()) {
		DECLARE_QSTATUS();
		
		const TextViewWindowImpl::Line& line =
			pImpl_->listLine_[pImpl_->ptCaret_.y];
		
		int nLineInWindow = 0;
		status = pImpl_->getLineInWindow(&nLineInWindow);
		CHECK_QSTATUS();
		
		int nLines = pImpl_->listLine_.size();
		int nPos = 0;
		const WCHAR* pCaret = pImpl_->pCaret_;
		
		switch (moveCaret) {
		case MOVECARET_CHARLEFT:
			if (pImpl_->pCaret_ != pImpl_->pBuf_) {
				--pImpl_->pCaret_;
				if (bRepeat && pImpl_->pCaret_ >= line.pBegin_)
					--pImpl_->pCaret_;
				if (pImpl_->pCaret_ < line.pBegin_) {
					--pImpl_->ptCaret_.y;
					status = pImpl_->getPosition(pImpl_->ptCaret_.y,
						pImpl_->pCaret_, &nPos);
					CHECK_QSTATUS();
					pImpl_->ptCaret_.x = nPos;
					pImpl_->nCaretPos_ = nPos;
				}
				else {
					status = pImpl_->getPosition(pImpl_->ptCaret_.y,
						pImpl_->pCaret_, &nPos);
					CHECK_QSTATUS();
					pImpl_->ptCaret_.x = nPos;
					pImpl_->nCaretPos_ = nPos;
				}
			}
			break;
		case MOVECARET_CHARRIGHT:
			if (pImpl_->pCaret_ != pImpl_->pCurrent_) {
				++pImpl_->pCaret_;
				if (bRepeat && pImpl_->pCaret_ != line.pEnd_)
					++pImpl_->pCaret_;
				if (pImpl_->pCaret_ == line.pEnd_ &&
					pImpl_->ptCaret_.y != nLines - 1) {
					++pImpl_->ptCaret_.y;
					pImpl_->ptCaret_.x = 0;
					pImpl_->nCaretPos_ = 0;
				}
				else {
					status = pImpl_->getPosition(pImpl_->ptCaret_.y,
						pImpl_->pCaret_, &nPos);
					CHECK_QSTATUS();
					pImpl_->ptCaret_.x = nPos;
					pImpl_->nCaretPos_ = nPos;
				}
			}
			break;
		case MOVECARET_LINESTART:
			pImpl_->pCaret_ = line.pBegin_;
			pImpl_->ptCaret_.x = 0;
			pImpl_->nCaretPos_ = 0;
			break;
		case MOVECARET_LINEEND:
			pImpl_->pCaret_ = line.pEnd_ - 1;
			status = pImpl_->getPosition(pImpl_->ptCaret_.y,
				line.pEnd_ - 1, &nPos);
			CHECK_QSTATUS();
			pImpl_->ptCaret_.x = nPos;
			pImpl_->nCaretPos_ = nPos;
			break;
		case MOVECARET_LINEUP:
			if (pImpl_->ptCaret_.y != 0) {
				--pImpl_->ptCaret_.y;
				if (bRepeat && pImpl_->ptCaret_.y != 0)
					--pImpl_->ptCaret_.y;
				status = pImpl_->getTextFromPos(pImpl_->ptCaret_.y,
					pImpl_->nCaretPos_, false, &pImpl_->pCaret_);
				CHECK_QSTATUS();
				status = pImpl_->getPosition(pImpl_->ptCaret_.y,
					pImpl_->pCaret_, &nPos);
				CHECK_QSTATUS();
				pImpl_->ptCaret_.x = nPos;
			}
			break;
		case MOVECARET_LINEDOWN:
			if (pImpl_->ptCaret_.y != nLines - 1) {
				++pImpl_->ptCaret_.y;
				if (bRepeat && pImpl_->ptCaret_.y != nLines - 1)
					++pImpl_->ptCaret_.y;
				status = pImpl_->getTextFromPos(pImpl_->ptCaret_.y,
					pImpl_->nCaretPos_, false, &pImpl_->pCaret_);
				CHECK_QSTATUS();
				status = pImpl_->getPosition(pImpl_->ptCaret_.y,
					pImpl_->pCaret_, &nPos);
				CHECK_QSTATUS();
				pImpl_->ptCaret_.x = nPos;
			}
			break;
		case MOVECARET_PAGEUP:
			if (pImpl_->ptCaret_.y != 0) {
				pImpl_->ptCaret_.y -= nLineInWindow - 1;
				if (pImpl_->ptCaret_.y < 0)
					pImpl_->ptCaret_.y = 0;
				status = pImpl_->getTextFromPos(pImpl_->ptCaret_.y,
					pImpl_->nCaretPos_, false, &pImpl_->pCaret_);
				CHECK_QSTATUS();
				status = pImpl_->getPosition(pImpl_->ptCaret_.y,
					pImpl_->pCaret_, &nPos);
				CHECK_QSTATUS();
				pImpl_->ptCaret_.x = nPos;
			}
			break;
		case MOVECARET_PAGEDOWN:
			if (pImpl_->ptCaret_.y != nLines - 1) {
				pImpl_->ptCaret_.y += nLineInWindow - 1;
				if (pImpl_->ptCaret_.y > nLines - 1)
					pImpl_->ptCaret_.y = nLines - 1;
				status = pImpl_->getTextFromPos(pImpl_->ptCaret_.y,
					pImpl_->nCaretPos_, false, &pImpl_->pCaret_);
				CHECK_QSTATUS();
				status = pImpl_->getPosition(pImpl_->ptCaret_.y,
					pImpl_->pCaret_, &nPos);
				CHECK_QSTATUS();
				pImpl_->ptCaret_.x = nPos;
			}
			break;
		case MOVECARET_DOCSTART:
			pImpl_->pCaret_ = pImpl_->pBuf_;
			pImpl_->ptCaret_.x = 0;
			pImpl_->ptCaret_.y = 0;
			pImpl_->nCaretPos_ = 0;
			break;
		case MOVECARET_DOCEND:
			pImpl_->pCaret_ = pImpl_->pCurrent_;
			pImpl_->ptCaret_.y = nLines - 1;
			status = pImpl_->getPosition(pImpl_->ptCaret_.y,
				pImpl_->pCaret_, &nPos);
			CHECK_QSTATUS();
			pImpl_->ptCaret_.x = nPos;
			pImpl_->nCaretPos_ = 0;
			break;
		case MOVECARET_POS:
			{
				int nLine = 0;
				pImpl_->pCaret_ = pText;
				status = pImpl_->getLine(pText, &nLine);
				CHECK_QSTATUS();
				pImpl_->ptCaret_.y = nLine;
				status = pImpl_->getPosition(pImpl_->ptCaret_.y,
					pImpl_->pCaret_, &nPos);
				CHECK_QSTATUS();
				pImpl_->ptCaret_.x = nPos;
				pImpl_->nCaretPos_ = nPos;
			}
			break;
		}
		
		if (select == SELECT_SELECT) {
			if (pImpl_->pSelectionStart_ == pImpl_->pSelectionEnd_)
				pImpl_->pSelectionStart_ = pCaret;
			pImpl_->pSelectionEnd_ = pImpl_->pCaret_;
			pImpl_->invalidateText(pCaret, pImpl_->pCaret_);
		}
		else if (select == SELECT_CLEAR) {
			pImpl_->clearSelection();
		}
		
		pImpl_->updateCaret(true);
	}
	return QSTATUS_SUCCESS;
}

QSTATUS qs::TextViewWindow::getWindowClass(WNDCLASS* pwc)
{
	DECLARE_QSTATUS();
	
	status = DefaultWindowHandler::getWindowClass(pwc);
	CHECK_QSTATUS();
	
#if !defined _WIN32_WCE || _WIN32_WCE >= 211
	pwc->hCursor = ::LoadCursor(0, IDC_IBEAM);
#endif // _WIN32_WCE
	
	return QSTATUS_SUCCESS;
}

QSTATUS qs::TextViewWindow::preCreateWindow(CREATESTRUCT* pCreateStruct)
{
	DECLARE_QSTATUS();
	
	status = DefaultWindowHandler::preCreateWindow(pCreateStruct);
	CHECK_QSTATUS();
	
	if (pImpl_->bShowVerticalScrollBar_)
		pCreateStruct->style |= WS_VSCROLL;
	if (pImpl_->bShowHorizontalScrollBar_)
		pCreateStruct->style |= WS_HSCROLL;
	
	return QSTATUS_SUCCESS;
}

LRESULT qs::TextViewWindow::windowProc(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	BEGIN_MESSAGE_HANDLER()
		HANDLE_CREATE()
		HANDLE_ERASEBKGND()
		HANDLE_HSCROLL()
		HANDLE_KEYDOWN()
		HANDLE_KILLFOCUS()
		HANDLE_LBUTTONDOWN()
		HANDLE_LBUTTONUP()
		HANDLE_MOUSEMOVE()
		HANDLE_MOUSEWHEEL()
		HANDLE_PAINT()
		HANDLE_SETFOCUS()
		HANDLE_SIZE()
		HANDLE_TIMER()
		HANDLE_VSCROLL();
	END_MESSAGE_HANDLER()
	return DefaultWindowHandler::windowProc(uMsg, wParam, lParam);
}

LRESULT qs::TextViewWindow::onCreate(CREATESTRUCT* pCreateStruct)
{
	if (DefaultWindowHandler::onCreate(pCreateStruct) == -1)
		return -1;
	
	return 0;
}

LRESULT qs::TextViewWindow::onEraseBkgnd(HDC hdc)
{
	return 1;
}

LRESULT qs::TextViewWindow::onHScroll(UINT nCode, UINT nPos, HWND hwnd)
{
	bool bScroll = true;
	Scroll s;
	switch (nCode) {
	case SB_LEFT:
		s = SCROLL_LEFT;
		break;
	case SB_RIGHT:
		s = SCROLL_RIGHT;
		break;
	case SB_LINELEFT:
		s = SCROLL_CHARLEFT;
		break;
	case SB_LINERIGHT:
		s = SCROLL_CHARRIGHT;
		break;
	case SB_PAGELEFT:
		s = SCROLL_PAGELEFT;
		break;
	case SB_PAGERIGHT:
		s = SCROLL_PAGERIGHT;
		break;
	case SB_THUMBPOSITION:
	case SB_THUMBTRACK:
		s = SCROLL_HORIZONTALPOS;
		break;
	default:
		bScroll = false;
		break;
	}
	
	if (bScroll)
		scroll(s, nPos, 0);
	
	return DefaultWindowHandler::onHScroll(nCode, nPos, hwnd);
}

LRESULT qs::TextViewWindow::onKeyDown(UINT nKey, UINT nRepeat, UINT nFlags)
{
	if (pImpl_->bShowCaret_) {
		MoveCaret mc = static_cast<MoveCaret>(-1);
		switch (nKey) {
		case VK_LEFT:
			mc = MOVECARET_CHARLEFT;
			break;
		case VK_RIGHT:
			mc = MOVECARET_CHARRIGHT;
			break;
		case VK_UP:
			mc = MOVECARET_LINEUP;
			break;
		case VK_DOWN:
			mc = MOVECARET_LINEDOWN;
			break;
		case VK_PRIOR:
			mc = MOVECARET_PAGEUP;
			break;
		case VK_NEXT:
			mc = MOVECARET_PAGEDOWN;
			break;
		case VK_HOME:
			if (::GetKeyState(VK_CONTROL) < 0)
				mc = MOVECARET_DOCSTART;
			else
				mc = MOVECARET_LINESTART;
			break;
		case VK_END:
			if (::GetKeyState(VK_CONTROL) < 0)
				mc = MOVECARET_DOCEND;
			else
				mc = MOVECARET_LINEEND;
			break;
		default:
			break;
		}
		if (mc != -1)
			moveCaret(mc, 0, (nFlags & 0x40000000) != 0,
				::GetKeyState(VK_SHIFT) < 0 ? SELECT_SELECT : SELECT_CLEAR);
	}
	else {
		Scroll s = static_cast<Scroll>(0);
		switch (nKey) {
		case VK_LEFT:
			s = SCROLL_CHARLEFT;
			break;
		case VK_RIGHT:
			s = SCROLL_CHARRIGHT;
			break;
		case VK_UP:
			s = SCROLL_LINEUP;
			break;
		case VK_DOWN:
			s = SCROLL_LINEDOWN;
			break;
		case VK_PRIOR:
			s = SCROLL_PAGEUP;
			break;
		case VK_NEXT:
			s = SCROLL_PAGEDOWN;
			break;
		case VK_HOME:
			s = SCROLL_TOP;
			break;
		case VK_END:
			s = SCROLL_BOTTOM;
			break;
		default:
			break;
		}
		if (s != 0)
			scroll(s, 0, (nFlags & 0x40000000) != 0);
	}
	return DefaultWindowHandler::onKeyDown(nKey, nRepeat, nFlags);
}

LRESULT qs::TextViewWindow::onKillFocus(HWND hwnd)
{
	if (pImpl_->bShowCaret_)
		pImpl_->hideCaret();
	
	return DefaultWindowHandler::onKillFocus(hwnd);
}

LRESULT qs::TextViewWindow::onLButtonDown(UINT nFlags, const POINT& pt)
{
	setFocus();
	
	pImpl_->startSelection(pt);
	setCapture();
	
#ifndef _WIN32_WCE
	pImpl_->nTimerDragScroll_ = setTimer(
		TextViewWindowImpl::TIMER_DRAGSCROLL, TextViewWindowImpl::DRAGSCROLL_INTERVAL);
#endif
	
	return DefaultWindowHandler::onLButtonDown(nFlags, pt);
}

LRESULT qs::TextViewWindow::onLButtonUp(UINT nFlags, const POINT& pt)
{
	if (getCapture()) {
#ifndef _WIN32_WCE
		killTimer(pImpl_->nTimerDragScroll_);
		pImpl_->nTimerDragScroll_ = 0;
#endif
		
		releaseCapture();
		
		pImpl_->updateSelection(pt);
	}
	
	return DefaultWindowHandler::onLButtonUp(nFlags, pt);
}

LRESULT qs::TextViewWindow::onMouseMove(UINT nFlags, const POINT& pt)
{
	if (getCapture())
		pImpl_->updateSelection(pt);
	
	return DefaultWindowHandler::onMouseMove(nFlags, pt);
}

#if !defined _WIN32_WCE || _WIN32_WCE >= 211
LRESULT qs::TextViewWindow::onMouseWheel(
	UINT nFlags, short nDelta, const POINT& pt)
{
#ifdef _WIN32_WCE
#	define WHEEL_DELTA 120
#endif
	scroll(SCROLL_VIRTICALPOS, pImpl_->ptScroll_.y - nDelta*3/WHEEL_DELTA, false);
	
	return DefaultWindowHandler::onMouseWheel(nFlags, nDelta, pt);
}
#endif

LRESULT qs::TextViewWindow::onPaint()
{
	DECLARE_QSTATUS();
	
	PaintDeviceContext dc(getHandle(), &status);
	CHECK_QSTATUS_VALUE(1);
	
	ObjectSelector<HFONT> fontSelector(dc, pImpl_->hfont_);
	ObjectSelector<HBRUSH> brushSelector(dc,
		static_cast<HBRUSH>(::GetStockObject(WHITE_BRUSH)));
	dc.setTextColor(pImpl_->crForeground_);
	dc.setBkColor(pImpl_->crBackground_);
	
	CompatibleDeviceContext dcNewLine(dc, &status);
	CHECK_QSTATUS_VALUE(1);
	HBITMAP hbmNewLine = 0;
	status = pImpl_->getNewLineBitmap(&hbmNewLine);
	CHECK_QSTATUS_VALUE(1);
	ObjectSelector<HBITMAP> bitmapSelector(dcNewLine, hbmNewLine);
	
	RECT rectClip;
	dc.getClipBox(&rectClip);
	
	RECT rect;
	getClientRect(&rect);
	rect.left += pImpl_->nMarginLeft_;
	rect.right -= pImpl_->nMarginRight_;
	int nBottom = rect.bottom;
	
	int nLineHeight = 0;
	status = pImpl_->getLineHeight(&nLineHeight);
	CHECK_QSTATUS_VALUE(0);
	int nLineInWindow = 0;
	status = pImpl_->getLineInWindow(&nLineInWindow);
	CHECK_QSTATUS_VALUE(0);
	int nFixedCharWidth = 0;
	status = pImpl_->getFixedCharWidth(&nFixedCharWidth);
	CHECK_QSTATUS_VALUE(0);
	
	rect.bottom = pImpl_->nMarginTop_;
	dc.fillSolidRect(rect, pImpl_->crBackground_);
	
	int nStartLine = pImpl_->ptScroll_.y;
	int nEndLine = QSMIN(static_cast<int>(pImpl_->listLine_.size()),
		nStartLine + nLineInWindow + 2);
	
	int nCharInLine = 0;
	if (pImpl_->bProportional_) {
		// TODO
	}
	else {
		status = pImpl_->getCharInLine(&nCharInLine);
		CHECK_QSTATUS_VALUE(0);
	}
	string_ptr<WSTRING> wstrText(allocWString(nCharInLine + 1));
	if (!wstrText.get())
		return 0;
	
	POINT pt = {
		pImpl_->nMarginLeft_ - pImpl_->ptScroll_.x,
		pImpl_->nMarginTop_
	};
	int n = nStartLine;
	while (n < nEndLine) {
		rect.top = pt.y;
		rect.bottom = rect.top + nLineHeight;
		RECT rectIntersect;
		if (::IntersectRect(&rectIntersect, &rect, &rectClip)) {
			const TextViewWindowImpl::Line& line = pImpl_->listLine_[n];
			WCHAR* pBegin = line.pBegin_;
			WCHAR* pEnd = line.pEnd_;
			if (pBegin != pEnd && *(pEnd - 1) == L'\n')
				--pEnd;
			
			WCHAR* p = wstrText.get();
			int nPos = 0;
			while (pBegin < pEnd) {
				WCHAR c = *pBegin;
				if (c == L'\t') {
					int nNewPos = 0;
					status = pImpl_->getNextTabStop(nPos, &nNewPos);
					CHECK_QSTATUS();
					int nWidth = nNewPos - nPos;
					*p = pImpl_->cTabChar_;
					while (--nWidth != 0)
						*(++p) = L' ';
					nPos = nNewPos;
				}
				else {
					*p = c;
					int nWidth = 0;
					status = pImpl_->getFontCharWidth(c, &nWidth);
					CHECK_QSTATUS();
					nPos += nWidth;
				}
				++pBegin;
				++p;
			}
			
			dc.extTextOut(pt.x, pt.y + pImpl_->nLineSpacing_,
				ETO_CLIPPED | ETO_OPAQUE, rect, wstrText.get(),
				p - wstrText.get(), 0);
			
			if (pImpl_->bShowNewLine_ && pEnd != line.pEnd_) {
				dc.bitBlt(pImpl_->nMarginLeft_ + nPos*nFixedCharWidth - pImpl_->ptScroll_.x,
					pt.y, nFixedCharWidth, nLineHeight, dcNewLine, 0, 0, SRCCOPY);
			}
			
			TextViewWindowImpl::Selection selection =
				TextViewWindowImpl::SELECTION_NONE;
			const WCHAR* pSelectionStart = 0;
			const WCHAR* pSelectionEnd = 0;
			status = pImpl_->getSelection(line, &selection,
				&pSelectionStart, &pSelectionEnd);
			if (status == QSTATUS_SUCCESS &&
				selection != TextViewWindowImpl::SELECTION_NONE) {
				assert(pSelectionStart <= pSelectionEnd);
				int nStart = 0;
				pImpl_->getPosition(n, pSelectionStart, &nStart);
				int nEnd = 0;
				pImpl_->getPosition(n, pSelectionEnd, &nEnd);
				dc.patBlt(nStart + pImpl_->nMarginLeft_ - pImpl_->ptScroll_.x,
					rect.top, nEnd - nStart, rect.bottom - rect.top, PATINVERT);
			}
		}
		
		pt.y += nLineHeight;
		++n;
	}
	rect.top = QSMIN(static_cast<int>(pt.y),
		nBottom - pImpl_->nMarginBottom_);
	rect.bottom = nBottom;
	dc.fillSolidRect(rect, pImpl_->crBackground_);
	
	rect.top = 0;
	rect.bottom = nBottom;
	rect.left = rect.right;
	rect.right += pImpl_->nMarginRight_;
	dc.fillSolidRect(rect, pImpl_->crBackground_);
	rect.left = 0;
	rect.right = pImpl_->nMarginLeft_;
	dc.fillSolidRect(rect, pImpl_->crBackground_);
	
	return 0;
}

LRESULT qs::TextViewWindow::onSetFocus(HWND hwnd)
{
	if (pImpl_->bShowCaret_)
		pImpl_->showCaret();
	
	return DefaultWindowHandler::onSetFocus(hwnd);
}

LRESULT qs::TextViewWindow::onSize(UINT nFlags, int cx, int cy)
{
	pImpl_->nLineInWindow_ = 0;
	
	int nOldCharInLine = 0;
	pImpl_->getCharInLine(&nOldCharInLine);
	pImpl_->nCalculatedCharInLine_ = 0;
	int nNewCharInLine = 0;
	pImpl_->getCharInLine(&nNewCharInLine);
	if (nNewCharInLine != nOldCharInLine) {
		pImpl_->recalcLine();
		invalidate();
	}
	
	pImpl_->updateScrollBar();
	
	return DefaultWindowHandler::onSize(nFlags, cx, cy);
}

LRESULT qs::TextViewWindow::onTimer(UINT nId)
{
	if (nId == pImpl_->nTimerDragScroll_) {
#ifndef _WIN32_WCE
		POINT pt;
		::GetCursorPos(&pt);
		screenToClient(&pt);
		
		RECT rect;
		pImpl_->getClientRectWithoutMargin(&rect);
		
		bool bScroll = false;
		if (pt.y < TextViewWindowImpl::DRAGSCROLL_BORDER) {
			scroll(SCROLL_LINEUP, 0, pt.y < 0);
			bScroll = true;
		}
		else if (pt.y > rect.bottom - TextViewWindowImpl::DRAGSCROLL_BORDER) {
			scroll(SCROLL_LINEDOWN, 0, pt.y > rect.bottom);
			bScroll = true;
		}
		if (pt.x < TextViewWindowImpl::DRAGSCROLL_BORDER) {
			scroll(SCROLL_CHARLEFT, 0, pt.x < 0);
			bScroll = true;
		}
		else if (pt.x > rect.right - TextViewWindowImpl::DRAGSCROLL_BORDER) {
			scroll(SCROLL_CHARRIGHT, 0, pt.x > rect.right);
			bScroll = true;
		}
		
		if (bScroll)
			pImpl_->updateSelection(pt);
		return 0;
#endif
	}
	else if (pImpl_->nTimerLoad_) {
		assert(pImpl_->pReader_);
		
		bool bError = false;
		WCHAR wsz[1024];
		size_t nRead = 0;
		int n = 0;
		while (nRead != static_cast<size_t>(-1) && n < 5) {
			if (pImpl_->pReader_->read(wsz, 1024, &nRead) != QSTATUS_SUCCESS) {
				bError = true;
				break;
			}
			if (nRead != static_cast<size_t>(-1))
				appendText(wsz, nRead);
			++n;
		}
		
		if (bError || nRead == static_cast<size_t>(-1)) {
			delete pImpl_->pReader_;
			pImpl_->pReader_ = 0;
			killTimer(pImpl_->nTimerLoad_);
		}
	}
	
	return DefaultWindowHandler::onTimer(nId);
}

LRESULT qs::TextViewWindow::onVScroll(UINT nCode, UINT nPos, HWND hwnd)
{
	bool bScroll = true;
	Scroll s;
	switch (nCode) {
	case SB_LEFT:
		s = SCROLL_TOP;
		break;
	case SB_RIGHT:
		s = SCROLL_BOTTOM;
		break;
	case SB_LINELEFT:
		s = SCROLL_LINEUP;
		break;
	case SB_LINERIGHT:
		s = SCROLL_LINEDOWN;
		break;
	case SB_PAGELEFT:
		s = SCROLL_PAGEUP;
		break;
	case SB_PAGERIGHT:
		s = SCROLL_PAGEDOWN;
		break;
	case SB_THUMBPOSITION:
	case SB_THUMBTRACK:
		s = SCROLL_VIRTICALPOS;
		break;
	default:
		bScroll = false;
		break;
	}
	
	if (bScroll)
		scroll(s, nPos, 0);
	
	return DefaultWindowHandler::onVScroll(nCode, nPos, hwnd);
}

#endif // 0
