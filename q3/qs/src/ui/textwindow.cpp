/*
 * $Id$
 *
 * Copyright(C) 1998-2004 Satoshi Nakamura
 * All rights reserved.
 *
 */

#include <qsconv.h>
#include <qsdevicecontext.h>
#include <qsinit.h>
#include <qsprofile.h>
#include <qsregex.h>
#include <qstextutil.h>
#include <qstextwindow.h>
#include <qstheme.h>
#include <qsuiutil.h>
#include <qsutil.h>

#include <algorithm>

#include <tchar.h>
#ifndef _WIN32_WCE
#	include <tmschema.h>
#endif

#include "resourceinc.h"
#include "textwindow.h"

#pragma warning(disable:4786)

using namespace qs;


/****************************************************************************
 *
 * TextWindowImpl
 *
 */

class qs::TextWindowImpl : public TextModelHandler
{
public:
	enum {
		ID_RULER			= 1001
	};
	
	enum {
		RULER_HEIGHT		= 10
	};
	
	enum {
		TIMER_DRAGSCROLL	= 1000,
		DRAGSCROLL_BORDER	= 30,
		DRAGSCROLL_DELAY	= 300,
		DRAGSCROLL_INTERVAL	= 50
	};
	
	enum InsertTextFlag {
		INSERTTEXTFLAG_NORMAL,
		INSERTTEXTFLAG_UNDO,
		INSERTTEXTFLAG_REDO
	};
	
	enum CharType {
		CHARTYPE_NONE,
		CHARTYPE_SPACE,
		CHARTYPE_NEWLINE,
		CHARTYPE_ASCII,
		CHARTYPE_MARK,
		CHARTYPE_OTHER
	};
	
	enum {
		MAX_QUOTE = 100
	};

public:
	struct LinkItem
	{
		size_t nOffset_;
		size_t nLength_;
		int nLeft_;
		int nRight_;
	};
	
	struct LinkItems
	{
		size_t nCount_;
		LinkItem items_[1];
	};
	
	struct PhysicalLine
	{
		size_t nLogicalLine_;
		size_t nPosition_;
		size_t nLength_;
		COLORREF cr_;
		unsigned char nQuoteDepth_;
		unsigned char nQuoteLength_;
		LinkItems items_;
	};
	
	struct ScrollPos
	{
		int nPos_;
		int nLine_;
	};
	
	struct Caret
	{
		unsigned int nLine_;
		unsigned int nChar_;
		int nPos_;
		int nOldPos_;
	};
	
	struct Selection
	{
		unsigned int nStartLine_;
		unsigned int nStartChar_;
		unsigned int nEndLine_;
		unsigned int nEndChar_;
	};
	
	class PhysicalLinePtr
	{
	public:
		PhysicalLinePtr(PhysicalLine* p);
		~PhysicalLinePtr();
	
	public:
		PhysicalLine* operator->() const;
	
	public:
		PhysicalLine* get() const;
		PhysicalLine* release();
	
	private:
		PhysicalLine* p_;
	};
	
	struct PhysicalLineComp :
		public std::binary_function<PhysicalLine*, PhysicalLine*, bool>
	{
		bool operator()(const PhysicalLine* pLhs, const PhysicalLine* pRhs) const;
	};
	
	struct LogicalLinkItem
	{
		size_t nOffset_;
		size_t nLength_;
	};

public:
	typedef std::vector<PhysicalLine*> LineList;
	typedef std::vector<int> Extent;
	typedef std::vector<LinkItem> LinkItemList;
	typedef std::vector<LogicalLinkItem> LogicalLinkItemList;
	typedef std::vector<WSTRING> URLSchemaList;

public:
	unsigned int getLineHeight() const;
	unsigned int getLineInWindow() const;
	unsigned int getNextTabStop(unsigned int n) const;
	unsigned int getAverageCharWidth() const;
	
	bool updateBitmaps();
	
	void getClientRectWithoutMargin(RECT* pRect) const;
	
	unsigned int getCharFromPos(unsigned int nLine,
								unsigned int nPos) const;
	int getPosFromChar(unsigned int nLine,
					   unsigned int nChar) const;
	std::pair<unsigned int, unsigned int> getPositionFromPoint(const POINT& pt) const;
	int getLineFromPos(int nY) const;
	
	std::pair<unsigned int, unsigned int> getPhysicalLine(unsigned int nLogicalLine,
														  unsigned int nChar) const;
	
	void getSelection(unsigned int* pnStartLine,
					  unsigned int* pnStartChar,
					  unsigned int* pnEndLine,
					  unsigned int* pnEndChar,
					  bool* pbReverse) const;
	void expandSelection(unsigned int nStartLine,
						 unsigned int nStartChar,
						 unsigned int nEndLine,
						 unsigned int nEndChar);
	void startSelection(const POINT& pt,
						bool bScroll);
	void updateSelection(const POINT& pt,
						 bool bScroll);
	void clearSelection();
	std::pair<unsigned int, unsigned int> getSelection(unsigned int nLine) const;
	
	void calcLines(unsigned int nStartLine,
				   unsigned int nOldEndLine,
				   unsigned int nNewEndLine);
	void recalcLines();
	
	int paintBlock(DeviceContext* pdc,
				   const POINT& pt,
				   const RECT& rect,
				   int x,
				   const WCHAR* pBegin,
				   const WCHAR* pEnd,
				   DeviceContext* pdcTab,
				   const SIZE& sizeTab) const;
	COLORREF getLineColor(const TextModel::Line& line) const;
	std::pair<unsigned char, unsigned char> getLineQuoteDepth(const TextModel::Line& line) const;
	unsigned int getQuoteWidth() const;
	
	void scrollHorizontal(int nPos);
	void scrollVertical(int nLine);
	void updateScrollBar();
	
	void showCaret();
	void hideCaret();
	void updateCaret(bool bScroll);
	void updateCaret(bool bScroll,
					 const RECT& rectMargin);
	
	void invalidate(unsigned int nStartLine,
					unsigned int nStartChar,
					unsigned int nEndLine,
					unsigned int nEndChar);
	
	bool insertText(const WCHAR* pwsz,
					size_t nLen,
					InsertTextFlag flag);
	bool deleteText(TextWindow::DeleteTextFlag flag);
	
	size_t getReformQuoteLength(const WCHAR* pwszLine,
								size_t nLen) const;
	
	const LinkItem* getLinkItem(unsigned int nLine,
								unsigned int nChar);
	std::pair<int, const LinkItem*> getLinkItemFromPoint(const POINT& pt) const;
	bool openLink(const POINT& pt);
	wstring_ptr getURL(int nLine,
					   const LinkItem* pLinkItem) const;

public:
	virtual void textUpdated(const TextModelEvent& event);
	virtual void textSet(const TextModelEvent& event);

public:
	static PhysicalLine* allocLine(size_t nLogicalLine,
								   size_t nPosition,
								   size_t nLength,
								   COLORREF cr,
								   unsigned char nQuoteDepth,
								   unsigned char nQuoteLength,
								   LinkItem* pLinkItems,
								   size_t nLinkCount);
	static void freeLine(PhysicalLine* pLine);
	static CharType getCharType(WCHAR c);

private:
	bool getTextExtent(const DeviceContext& dc,
					   const WCHAR* pwszString,
					   int nCount,
					   int nMaxExtent,
					   int* pnFit,
					   int* pnDx,
					   SIZE* pSize) const;
	void getLineExtent(unsigned int nLine,
					   Extent* pExtent,
					   bool* pbNewLine) const;
	void getLineExtent(const DeviceContext& dc,
					   const WCHAR* pwsz,
					   size_t nLength,
					   Extent* pExtent,
					   bool* pbNewLine) const;
	void getLinks(const WCHAR* pwsz,
				  size_t nLen,
				  LogicalLinkItemList* pList) const;
	void convertLogicalLinksToPhysicalLinks(const LogicalLinkItemList& listLogicalLinkItem,
											size_t nOffset,
											size_t nLength,
											LinkItemList* pListPhysicalLink) const;
	void fillPhysicalLinks(LinkItemList* pList,
						   const DeviceContext& dc,
						   const WCHAR* pwsz,
						   size_t nLength,
						   unsigned char nQuoteDepth) const;

public:
	TextWindow* pThis_;
	TextModel* pTextModel_;
	LineList listLine_;
	std::auto_ptr<TextWindowUndoManager> pUndoManager_;
	TextWindowLinkHandler* pLinkHandler_;
	TextWindowRuler* pRuler_;
	
	COLORREF crForeground_;
	COLORREF crBackground_;
	unsigned int nLineSpacing_;
	unsigned int nCharInLine_;
	unsigned int nTabWidth_;
	int nMarginTop_;
	int nMarginBottom_;
	int nMarginLeft_;
	int nMarginRight_;
	bool bShowNewLine_;
	bool bShowTab_;
	bool bShowVerticalScrollBar_;
	bool bShowHorizontalScrollBar_;
	bool bShowCaret_;
	bool bShowRuler_;
	wstring_ptr wstrQuote_[2];
	COLORREF crQuote_[2];
	bool bLineQuote_;
	unsigned int nReformLineLength_;
	wstring_ptr wstrReformQuote_;
	URLSchemaList listURLSchema_;
	COLORREF crLink_;
	
	HFONT hfont_;
	bool bAdjustExtent_;
	HBITMAP hbmNewLine_;
	HBITMAP hbmTab_;
	ScrollPos scrollPos_;
	Caret caret_;
	Selection selection_;
	unsigned int nTimerDragScroll_;
	POINT ptLastButtonDown_;
	mutable int nLastWindowWidth_;
	HIMC hImc_;
	bool bAtok_;
	HCURSOR hCursorLink_;
#ifndef _WIN32_WCE
	std::auto_ptr<Theme> pTheme_;
#endif
	
	mutable unsigned int nLineHeight_;
	mutable unsigned int nLineInWindow_;
	mutable unsigned int nAverageCharWidth_;
	mutable bool bHorizontalScrollable_;
	mutable Extent extent_;
	mutable unsigned int nExtentLine_;
	mutable bool bExtentNewLine_;
	mutable int nDx_[1024];
};

unsigned int qs::TextWindowImpl::getLineHeight() const
{
	if (nLineHeight_ == 0) {
		ClientDeviceContext dc(pThis_->getHandle());
		ObjectSelector<HFONT> fontSelector(dc, hfont_);
		TEXTMETRIC tm;
		dc.getTextMetrics(&tm);
		nLineHeight_ = tm.tmHeight + tm.tmExternalLeading + nLineSpacing_;
	}
	return nLineHeight_;
}

unsigned int qs::TextWindowImpl::getLineInWindow() const
{
	if (nLineInWindow_ == 0) {
		unsigned int nLineHeight = getLineHeight();
		
		RECT rect;
		getClientRectWithoutMargin(&rect);
		
		if (rect.bottom - rect.top > static_cast<int>(nLineHeight))
			nLineInWindow_ = (rect.bottom - rect.top)/nLineHeight;
		else
			nLineInWindow_ = 1;
	}
	return nLineInWindow_;
}

unsigned int qs::TextWindowImpl::getNextTabStop(unsigned int n) const
{
	unsigned int nTabWidth = nTabWidth_*getAverageCharWidth();
	return (n/nTabWidth + 1)*nTabWidth;
}

unsigned int qs::TextWindowImpl::getAverageCharWidth() const
{
	if (nAverageCharWidth_ == 0) {
		ClientDeviceContext dc(pThis_->getHandle());
		ObjectSelector<HFONT> fontSelector(dc, hfont_);
		TEXTMETRIC tm;
		dc.getTextMetrics(&tm);
		nAverageCharWidth_ = tm.tmAveCharWidth + tm.tmOverhang;
	}
	return nAverageCharWidth_;
}

bool qs::TextWindowImpl::updateBitmaps()
{
	if (hbmNewLine_) {
		::DeleteObject(hbmNewLine_);
		hbmNewLine_ = 0;
	}
	if (hbmTab_) {
		::DeleteObject(hbmTab_);
		hbmTab_ = 0;
	}
	
	ClientDeviceContext dc(pThis_->getHandle());
	if (!dc)
		return false;
	ObjectSelector<HFONT> fontSelector(dc, hfont_);
	
	CompatibleDeviceContext dcMem(dc);
	if (!dcMem)
		return false;
	
	int nWidth = getAverageCharWidth();
	int nHeight = getLineHeight();
	RECT rect = { 0, 0, nWidth, nHeight };
	POINT pt[3];
	
	GdiObject<HPEN> hpen(::CreatePen(PS_SOLID, 0, crForeground_));
	if (!hpen.get())
		return false;
	ObjectSelector<HPEN> penSelector(dcMem, hpen.get());
	
	GdiObject<HBITMAP> hbmNewLine(
		::CreateCompatibleBitmap(dc, nWidth, nHeight));
	if (!hbmNewLine.get())
		return false;
	ObjectSelector<HBITMAP> selectorNewLine(dcMem, hbmNewLine.get());
	
	dcMem.fillSolidRect(rect, crBackground_);
	
	int nArcWidth = QSMIN(QSMAX(nWidth/4 + 1, 3), nWidth - nWidth/2 - 1);
	pt[0].x = nWidth/2;
	pt[0].y = 3;
	pt[1].x= pt[0].x;
	pt[1].y = nHeight - nLineSpacing_ - 2;
	dcMem.polyline(pt, 2);
	pt[0].x = pt[0].x - nArcWidth;
	pt[0].y = pt[1].y - nArcWidth;
	pt[2].x = pt[1].x + nArcWidth + 1;
	pt[2].y = pt[1].y - nArcWidth - 1;
	dcMem.polyline(pt, 3);
	
	GdiObject<HBITMAP> hbmTab(
		::CreateCompatibleBitmap(dc, nWidth, nHeight));
	if (!hbmTab.get())
		return false;
	ObjectSelector<HBITMAP> selectorTab(dcMem, hbmTab.get());
	
	dcMem.fillSolidRect(rect, crBackground_);
	
	pt[0].x = 1;
	pt[0].y = (nHeight - nLineSpacing_)/2 - 1;
	pt[1].x = 3;
	pt[1].y = (nHeight - nLineSpacing_)/2;
	pt[2].x = 0;
	pt[2].y = (nHeight - nLineSpacing_)/2 + 2;
	dcMem.polyline(pt, 3);
	
	hbmNewLine_ = hbmNewLine.release();
	hbmTab_ = hbmTab.release();
	
	return true;
}

void qs::TextWindowImpl::getClientRectWithoutMargin(RECT* pRect) const
{
	assert(pRect);
	
	pThis_->getClientRect(pRect);
	pRect->left += nMarginLeft_;
	pRect->top += nMarginTop_;
	pRect->right -= nMarginRight_;
	pRect->bottom -= nMarginBottom_;
	
	if (pRect->left > pRect->right)
		pRect->right = pRect->left;
	if (pRect->top > pRect->bottom)
		pRect->bottom = pRect->top;
}

unsigned int qs::TextWindowImpl::getCharFromPos(unsigned int nLine,
												unsigned int nPos) const
{
	unsigned int nChar = 0;
	
	unsigned int nQuoteWidth = listLine_[nLine]->nQuoteDepth_*getQuoteWidth();
	if (nPos > nQuoteWidth) {
		nPos -= nQuoteWidth;
		
		if (nLine != nExtentLine_) {
			getLineExtent(nLine, &extent_, &bExtentNewLine_);
			nExtentLine_ = nLine;
		}
		
		while (nChar < extent_.size() - (bExtentNewLine_ ? 1 : 0)) {
			unsigned int nSep = extent_[nChar] -
				(extent_[nChar] - (nChar == 0 ? 0 : extent_[nChar - 1]))/2;
			if (nSep >= nPos)
				break;
			++nChar;
		}
	}
	
	return nChar;
}

int qs::TextWindowImpl::getPosFromChar(unsigned int nLine,
									   unsigned int nChar) const
{
	int nPos = listLine_[nLine]->nQuoteDepth_*getQuoteWidth();
	
	if (nChar != 0) {
		if (nLine != nExtentLine_) {
			getLineExtent(nLine, &extent_, &bExtentNewLine_);
			nExtentLine_ = nLine;
		}
		if (!extent_.empty())
			nPos += extent_[nChar - 1];
	}
	
	return nPos;
}

std::pair<unsigned int, unsigned int> qs::TextWindowImpl::getPositionFromPoint(const POINT& pt) const
{
	int nLine = getLineFromPos(pt.y);
	unsigned int nChar = 0;
	if (nLine < 0) {
		nLine = 0;
		nChar = 0;
	}
	else if (static_cast<LineList::size_type>(nLine) < listLine_.size()) {
		int nPos = scrollPos_.nPos_ + (pt.x - nMarginLeft_);
		if (nPos < 0)
			nChar = 0;
		else
			nChar = getCharFromPos(nLine, nPos);
	}
	else {
		nLine = listLine_.size() - 1;
		nChar = listLine_.empty() ? 0 : listLine_.back()->nLength_;
	}
	
	return std::make_pair(nLine, nChar);
}

int qs::TextWindowImpl::getLineFromPos(int nY) const
{
	return scrollPos_.nLine_ + (nY < nMarginTop_ ? 0 : (nY - nMarginTop_)/getLineHeight());
}

std::pair<unsigned int, unsigned int> qs::TextWindowImpl::getPhysicalLine(unsigned int nLogicalLine,
																		  unsigned int nChar) const
{
	assert(!listLine_.empty());
	
	PhysicalLine line = { nLogicalLine, nChar, 0 };
	LineList::const_iterator it = std::lower_bound(
		listLine_.begin(), listLine_.end(), &line, PhysicalLineComp());
	if (it == listLine_.end() || nChar != (*it)->nPosition_ - (*it)->nQuoteLength_)
		--it;
	
#ifndef NDEBUG
	LineList::const_iterator itD = listLine_.begin();
	while (itD != listLine_.end()) {
		if ((*itD)->nLogicalLine_ > nLogicalLine) {
			break;
		}
		else if ((*itD)->nLogicalLine_ == nLogicalLine) {
			if ((*itD)->nPosition_ - (*itD)->nQuoteLength_ > nChar)
				break;
		}
		++itD;
	}
	--itD;
	assert(it == itD);
#endif
	
	return std::make_pair(it - listLine_.begin(), nChar - (*it)->nPosition_);
}

void qs::TextWindowImpl::getSelection(unsigned int* pnStartLine,
									  unsigned int* pnStartChar,
									  unsigned int* pnEndLine,
									  unsigned int* pnEndChar,
									  bool* pbReverse) const
{
	assert((pnStartLine && pnStartChar) || (!pnStartLine && !pnStartChar));
	assert((pnEndLine && pnEndChar) || (!pnEndLine && !pnEndChar));
	
	bool bReverse = selection_.nStartLine_ > selection_.nEndLine_ ||
		(selection_.nStartLine_ == selection_.nEndLine_ &&
			selection_.nStartChar_ > selection_.nEndChar_);
	if (bReverse) {
		if (pnStartLine)
			*pnStartLine = selection_.nEndLine_;
		if (pnStartChar)
			*pnStartChar = selection_.nEndChar_;
		if (pnEndLine)
			*pnEndLine = selection_.nStartLine_;
		if (pnEndChar)
			*pnEndChar = selection_.nStartChar_;
	}
	else {
		if (pnStartLine)
			*pnStartLine = selection_.nStartLine_;
		if (pnStartChar)
			*pnStartChar = selection_.nStartChar_;
		if (pnEndLine)
			*pnEndLine = selection_.nEndLine_;
		if (pnEndChar)
			*pnEndChar = selection_.nEndChar_;
	}
	
	if (pbReverse)
		*pbReverse = bReverse;
}

void qs::TextWindowImpl::expandSelection(unsigned int nStartLine,
										 unsigned int nStartChar,
										 unsigned int nEndLine,
										 unsigned int nEndChar)
{
	if (!pThis_->isSelected()) {
		selection_.nStartLine_ = nStartLine;
		selection_.nStartChar_ = nStartChar;
	}
	selection_.nEndLine_ = nEndLine;
	selection_.nEndChar_ = nEndChar;
	
	invalidate(nStartLine, nStartChar, nEndLine, nEndChar);
}

void qs::TextWindowImpl::startSelection(const POINT& pt,
										bool bScroll)
{
	clearSelection();
	
	std::pair<unsigned int, unsigned int> pos(getPositionFromPoint(pt));
	selection_.nStartLine_ = pos.first;
	selection_.nStartChar_ = pos.second;
	selection_.nEndLine_ = pos.first;
	selection_.nEndChar_ = pos.second;
	
	pThis_->moveCaret(TextWindow::MOVECARET_POS, pos.first,
		pos.second, false, TextWindow::SELECT_NONE, bScroll);
}

void qs::TextWindowImpl::updateSelection(const POINT& pt,
										 bool bScroll)
{
	std::pair<unsigned int, unsigned int> pos(getPositionFromPoint(pt));
	invalidate(selection_.nEndLine_, selection_.nEndChar_, pos.first, pos.second);
	selection_.nEndLine_ = pos.first;
	selection_.nEndChar_ = pos.second;
	
	pThis_->moveCaret(TextWindow::MOVECARET_POS, pos.first,
		pos.second, false, TextWindow::SELECT_NONE, bScroll);
}

void qs::TextWindowImpl::clearSelection()
{
	if (pThis_->isSelected()) {
		invalidate(selection_.nStartLine_, selection_.nStartChar_,
			selection_.nEndLine_, selection_.nEndChar_);
		
		selection_.nStartLine_ = 0;
		selection_.nStartChar_ = 0;
		selection_.nEndLine_ = 0;
		selection_.nEndChar_ = 0;
	}
}

std::pair<unsigned int, unsigned int> qs::TextWindowImpl::getSelection(unsigned int nLine) const
{
	if (!pThis_->isSelected())
		return std::pair<unsigned int, unsigned int>(0, 0);
	
	Selection s = selection_;
	if (s.nStartLine_ > s.nEndLine_) {
		std::swap(s.nStartLine_, s.nEndLine_);
		std::swap(s.nStartChar_, s.nEndChar_);
	}
	else if (s.nStartLine_ == s.nEndLine_ &&
		s.nStartChar_ > s.nEndChar_) {
		std::swap(s.nStartChar_, s.nEndChar_);
	}
	
	if (nLine < s.nStartLine_ || s.nEndLine_ < nLine)
		return std::pair<unsigned int, unsigned int>(0, 0);
	else if (nLine == s.nStartLine_ && nLine == s.nEndLine_)
		return std::pair<unsigned int, unsigned int>(
			s.nStartChar_, s.nEndChar_);
	else if (nLine == s.nStartLine_)
		return std::pair<unsigned int, unsigned int>(
			s.nStartChar_, listLine_[nLine]->nLength_);
	else if (nLine == s.nEndLine_)
		return std::pair<unsigned int, unsigned int>(0, s.nEndChar_);
	else
		return std::pair<unsigned int, unsigned int>(
			0, listLine_[nLine]->nLength_);
}

void qs::TextWindowImpl::calcLines(unsigned int nStartLine,
								   unsigned int nOldEndLine,
								   unsigned int nNewEndLine)
{
	ClientDeviceContext dc(pThis_->getHandle());
	ObjectSelector<HFONT> fontSelector(dc, hfont_);
	
	unsigned int nAverageCharWidth = getAverageCharWidth();
	
	unsigned int nWidth = 0;
	if (nCharInLine_ != 0) {
		nWidth = nCharInLine_*nAverageCharWidth;
	}
	else {
		RECT rect;
		getClientRectWithoutMargin(&rect);
		nWidth = rect.right > rect.left ? rect.right - rect.left : 0;
	}
	
	LineList listLine;
	LineList* pListLine = &listLine;
	if (nStartLine == -1) {
		std::for_each(listLine_.begin(), listLine_.end(), &TextWindowImpl::freeLine);
		listLine_.clear();
		pListLine = &listLine_;
	}
	
	LogicalLinkItemList listLogicalLinkItem;
	LinkItemList listPhysicalLinkItem;
	
	unsigned int nStart = nStartLine == -1 ? 0 : nStartLine;
	unsigned int nEnd = nStartLine == -1 ?
		pTextModel_->getLineCount() : nNewEndLine + 1;
	
	for (size_t n = nStart; n < nEnd; ++n) {
		TextModel::Line line = pTextModel_->getLine(n);
		
		COLORREF cr = getLineColor(line);
		std::pair<unsigned char, unsigned char> quote(0, 0);
		if (pThis_->isLineQuote())
			quote = getLineQuoteDepth(line);
		unsigned char nQuoteDepth = quote.first;
		unsigned char nQuoteLength = quote.second;
		
		if (line.getLength() - nQuoteLength == 0) {
			PhysicalLinePtr ptr(allocLine(n, 0, 0, cr, nQuoteDepth, nQuoteLength, 0, 0));
			pListLine->push_back(ptr.get());
			ptr.release();
		}
		else {
			getLinks(line.getText(), line.getLength(), &listLogicalLinkItem);
			
			const WCHAR* pBegin = line.getText() + nQuoteLength;
			const WCHAR* pEnd = line.getText() + line.getLength();
			unsigned int nQuoteMargin = nQuoteDepth*getQuoteWidth();
			unsigned int nFormatWidth = nWidth;
			unsigned int nLineWidth = nQuoteMargin;
			if (nLineWidth + 100 > nFormatWidth)
				nFormatWidth = nLineWidth + 100;
			const WCHAR* p = pBegin;
			const WCHAR* pLine = pBegin;
			while (p != pEnd) {
				while (p != pEnd && *p != L'\t' && *p != L'\n')
					++p;
				
				int nFit = 0;
				bool bFull = false;
				do {
					SIZE size;
					getTextExtent(dc, pBegin, p - pBegin,
						nFormatWidth - nLineWidth, &nFit, 0, &size);
					if (nFit != p - pBegin || p == pEnd ||
						static_cast<unsigned int>(size.cx) == nFormatWidth - nLineWidth) {
						size_t nOffset = pLine - line.getText();
						size_t nLength = pBegin + nFit - pLine;
						convertLogicalLinksToPhysicalLinks(listLogicalLinkItem,
							nOffset, nLength, &listPhysicalLinkItem);
						fillPhysicalLinks(&listPhysicalLinkItem, dc,
							line.getText() + nOffset, nLength, nQuoteDepth);
						PhysicalLinePtr ptr(allocLine(n, nOffset, nLength, cr,
							nQuoteDepth, nQuoteLength, &listPhysicalLinkItem[0],
							listPhysicalLinkItem.size()));
						pListLine->push_back(ptr.get());
						ptr.release();
						
						bFull = static_cast<unsigned int>(size.cx) == nFormatWidth - nLineWidth;
						pBegin += nFit;
						pLine = pBegin;
						nLineWidth = nQuoteMargin;
					}
					else {
						nLineWidth += size.cx;
						pBegin = p;
					}
				} while (p != pBegin);
				
				if (*p == L'\n') {
					bool bWrap = nLineWidth + nAverageCharWidth > nFormatWidth;
					size_t nOffset = pLine - line.getText();
					size_t nLength = p - pLine;
					convertLogicalLinksToPhysicalLinks(listLogicalLinkItem,
						nOffset, nLength, &listPhysicalLinkItem);
					fillPhysicalLinks(&listPhysicalLinkItem, dc,
						line.getText() + nOffset, nLength, nQuoteDepth);
					PhysicalLinePtr ptr(allocLine(n, nOffset, nLength, cr, nQuoteDepth,
						nQuoteLength, &listPhysicalLinkItem[0], listPhysicalLinkItem.size()));
					if (!bWrap)
						++ptr->nLength_;
					pListLine->push_back(ptr.get());
					ptr.release();
					if (bWrap) {
						size_t nOffset = p - line.getText();
						size_t nLength = 1;
						convertLogicalLinksToPhysicalLinks(listLogicalLinkItem,
							nOffset, nLength, &listPhysicalLinkItem);
						fillPhysicalLinks(&listPhysicalLinkItem, dc,
							line.getText() + nOffset, nLength, nQuoteDepth);
						PhysicalLinePtr ptr(allocLine(n, nOffset, nLength, cr, nQuoteDepth,
							nQuoteLength, &listPhysicalLinkItem[0], listPhysicalLinkItem.size()));
						pListLine->push_back(ptr.get());
						ptr.release();
					}
					
					++p;
					pLine = p;
					pBegin = p;
					nLineWidth = nQuoteMargin;
				}
				else if (*p == L'\t') {
					++p;
					unsigned int nNextTabStop = getNextTabStop(nLineWidth);
					if (nNextTabStop >= nFormatWidth) {
						size_t nOffset = pLine - line.getText();
						size_t nLength = p - pLine;
						convertLogicalLinksToPhysicalLinks(listLogicalLinkItem,
							nOffset, nLength, &listPhysicalLinkItem);
						fillPhysicalLinks(&listPhysicalLinkItem, dc,
							line.getText() + nOffset, nLength, nQuoteDepth);
						PhysicalLinePtr ptr(allocLine(n, nOffset, nLength, cr, nQuoteDepth,
							nQuoteLength, &listPhysicalLinkItem[0], listPhysicalLinkItem.size()));
						pListLine->push_back(ptr.get());
						ptr.release();
						
						bFull = true;
						pLine = p;
						nLineWidth = nQuoteMargin;
					}
					else {
						nLineWidth = nNextTabStop;
						
						if (p == pEnd) {
							size_t nOffset = pLine - line.getText();
							size_t nLength = p - pLine;
							convertLogicalLinksToPhysicalLinks(listLogicalLinkItem,
								nOffset, nLength, &listPhysicalLinkItem);
							fillPhysicalLinks(&listPhysicalLinkItem, dc,
								line.getText() + nOffset, nLength, nQuoteDepth);
							PhysicalLinePtr ptr(allocLine(n, nOffset, nLength, cr, nQuoteDepth,
								nQuoteLength, &listPhysicalLinkItem[0], listPhysicalLinkItem.size()));
							pListLine->push_back(ptr.get());
							ptr.release();
						}
					}
					pBegin = p;
				}
				
				if (p == pEnd && bFull &&
					n == pTextModel_->getLineCount() - 1) {
					PhysicalLinePtr ptr(allocLine(n, p - line.getText(),
						0, cr, nQuoteDepth, nQuoteLength, 0, 0));
					pListLine->push_back(ptr.get());
					ptr.release();
				}
			}
		}
	}
	
	if (nStartLine != -1) {
		std::pair<unsigned int, unsigned int> start = getPhysicalLine(nStartLine, 0);
		assert(start.first == 0 || listLine_[start.first - 1]->nLogicalLine_ != nStartLine);
		std::pair<unsigned int, unsigned int> end = getPhysicalLine(nOldEndLine, 0);
		while (end.first + 1 < listLine_.size() &&
			listLine_[end.first]->nLogicalLine_ == listLine_[end.first + 1]->nLogicalLine_)
			++end.first;
		++end.first;
		
		std::for_each(listLine_.begin() + start.first,
			listLine_.begin() + end.first, &TextWindowImpl::freeLine);
		
		if (end.first - start.first < listLine.size()) {
			LineList::size_type nSize = end.first - start.first;
			LineList::iterator it = std::copy(listLine.begin(),
				listLine.begin() + nSize, listLine_.begin() + start.first);
			listLine_.insert(it, listLine.begin() + nSize, listLine.end());
		}
		else {
			LineList::iterator it = std::copy(listLine.begin(),
				listLine.end(), listLine_.begin() + start.first);
			listLine_.erase(it, listLine_.begin() + end.first);
		}
		
		if (nOldEndLine != nNewEndLine) {
			for (LineList::iterator it = listLine_.begin() + start.first + listLine.size(); it != listLine_.end(); ++it)
				(*it)->nLogicalLine_ += nNewEndLine - nOldEndLine;
		}
		
		if (end.first - start.first == listLine.size())
			invalidate(start.first, 0, end.first, 0);
		else
			invalidate(start.first, 0, -1, 0);
	}
	else {
		pThis_->invalidate();
	}
}

void qs::TextWindowImpl::recalcLines()
{
	calcLines(-1, -1, -1);
}

int qs::TextWindowImpl::paintBlock(DeviceContext* pdc,
								   const POINT& pt,
								   const RECT& rect,
								   int x,
								   const WCHAR* pBegin,
								   const WCHAR* pEnd,
								   DeviceContext* pdcTab,
								   const SIZE& sizeTab) const
{
	RECT r = rect;
	const WCHAR* p = pBegin;
	while (p != pEnd) {
		while (p != pEnd && *p != L'\t')
			++p;
		if (p != pBegin) {
			size_t nLen = p - pBegin;
			SIZE size;
			int* pnDx = 0;
			if (bAdjustExtent_) {
				getTextExtent(*pdc, pBegin, nLen, 0, 0, nDx_, &size);
				for (size_t n = nLen; n > 0; --n)
					nDx_[n] -= nDx_[n - 1];
				pnDx = nDx_;
			}
			else {
				getTextExtent(*pdc, pBegin, nLen, 0, 0, 0, &size);
			}
			
			r.left = pt.x + x;
			r.right = r.left + size.cx;
			
			pdc->extTextOut(pt.x + x, pt.y + nLineSpacing_,
				ETO_CLIPPED | ETO_OPAQUE, r, pBegin, nLen, pnDx);
			
			x += size.cx;
		}
		if (p != pEnd) {
			assert(*p == L'\t');
			unsigned int nLeft = pt.x + x;
			if (bShowTab_) {
				pdc->bitBlt(pt.x + x, pt.y, sizeTab.cx,
					sizeTab.cy, *pdcTab, 0, 0, SRCCOPY);
				nLeft += sizeTab.cx;
			}
			unsigned int nNext = getNextTabStop(x);
			if (nLeft < pt.x + nNext) {
				RECT rectTab = {
					nLeft,
					pt.y,
					pt.x + nNext,
					pt.y + sizeTab.cy
				};
				pdc->fillSolidRect(rectTab, crBackground_);
			}
			x = nNext;
			++p;
			pBegin = p;
		}
	}
	
	return x;
}

COLORREF qs::TextWindowImpl::getLineColor(const TextModel::Line& line) const
{
	const WCHAR* p = line.getText();
	size_t n = 0;
	while (n < line.getLength() &&
		(*p == L' ' || *p == L'\t' || *p == 0x3000)) {
		++p;
		++n;
	}
	if (n != line.getLength()) {
		int m = 0;
		while (m < countof(wstrQuote_) && !wcschr(wstrQuote_[m].get(), *p))
			++m;
		if (m != countof(wstrQuote_))
			return crQuote_[m];
	}
	return crForeground_;
}

std::pair<unsigned char, unsigned char> qs::TextWindowImpl::getLineQuoteDepth(const TextModel::Line& line) const
{
	std::pair<unsigned char, unsigned char> quote(0, 0);
	
	const WCHAR* p = line.getText();
	size_t nLength = line.getLength();
	if (nLength != 0 && wcschr(wstrQuote_[0].get(), *p)) {
		size_t nMax = QSMIN(nLength, MAX_QUOTE);
		const WCHAR* pLastQuote = p;
		for (size_t n = 0; n < nMax; ++n, ++p) {
			if (wcschr(wstrQuote_[0].get(), *p)) {
				++quote.first;
				pLastQuote = p;
			}
			else if (*p != L' ') {
				break;
			}
		}
		quote.second = pLastQuote - line.getText() + 1;
	}
	
	return quote;
}

unsigned int qs::TextWindowImpl::getQuoteWidth() const
{
	return getAverageCharWidth()*2;
}

void qs::TextWindowImpl::scrollHorizontal(int nPos)
{
	if (nPos != scrollPos_.nPos_) {
		RECT rectClip;
		getClientRectWithoutMargin(&rectClip);
		pThis_->scrollWindow(scrollPos_.nPos_ - nPos, 0,
			&rectClip, &rectClip, 0, 0, SW_INVALIDATE);
		
		if (bShowRuler_)
			pRuler_->setWindowPos(0, -nPos, 0, 0, 0,
				SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE);
		
		scrollPos_.nPos_ = nPos;
		updateScrollBar();
	}
}

void qs::TextWindowImpl::scrollVertical(int nLine)
{
	if (nLine != scrollPos_.nLine_) {
		int nLineHeight = getLineHeight();
		RECT rectClip;
		getClientRectWithoutMargin(&rectClip);
		pThis_->scrollWindow(0, (scrollPos_.nLine_ - nLine)*nLineHeight,
			&rectClip, &rectClip, 0, 0, SW_INVALIDATE);
		
		scrollPos_.nLine_ = nLine;
		updateScrollBar();
	}
}

void qs::TextWindowImpl::updateScrollBar()
{
	if (bShowVerticalScrollBar_) {
		SCROLLINFO si = {
			sizeof(si),
			SIF_PAGE | SIF_POS | SIF_RANGE
		};
		pThis_->getScrollInfo(SB_VERT, &si);
		
		unsigned int nPage = getLineInWindow();
		if (nPage > listLine_.size())
			nPage = listLine_.size();
		if (si.nMin != 0 || si.nMax != static_cast<int>(listLine_.size() - 1) ||
			si.nPage != nPage || si.nPos != scrollPos_.nLine_) {
			si.fMask = SIF_PAGE | SIF_POS | SIF_RANGE | SIF_DISABLENOSCROLL;
			si.nMin = 0;
			si.nMax = listLine_.size() - 1;
			si.nPage = nPage;
			si.nPos = scrollPos_.nLine_;
			pThis_->setScrollInfo(SB_VERT, si, true);
		}
	}
	
	RECT rect;
	getClientRectWithoutMargin(&rect);
	
	int nWidth = 0;
	if (nCharInLine_ != 0)
		nWidth = nCharInLine_*getAverageCharWidth();
	else
		nWidth = rect.right - rect.left;
	if (bShowHorizontalScrollBar_) {
		unsigned int nPage = rect.right - rect.left;
		if (nPage > static_cast<unsigned int>(nWidth))
			nPage = nWidth;
		SCROLLINFO si = {
			sizeof(si),
			SIF_PAGE | SIF_POS | SIF_RANGE
		};
		pThis_->getScrollInfo(SB_HORZ, &si);
		if (si.nMin != 0 || si.nMax != nWidth - 1 ||
			si.nPage != nPage || si.nPos != scrollPos_.nPos_) {
			si.fMask = SIF_PAGE | SIF_POS | SIF_RANGE | SIF_DISABLENOSCROLL;
			si.nMin = 0;
			si.nMax = nWidth - 1;
			si.nPage = rect.right - rect.left;
			si.nPos = scrollPos_.nPos_;
			pThis_->setScrollInfo(SB_HORZ, si, true);
		}
	}
	bHorizontalScrollable_ = nWidth > rect.right - rect.left;
}

void qs::TextWindowImpl::showCaret()
{
	int nLineHeight = getLineHeight();
	pThis_->createCaret(2, nLineHeight);
	POINT pt = {
		caret_.nPos_ + nMarginLeft_ - scrollPos_.nPos_,
		(caret_.nLine_ - scrollPos_.nLine_)*nLineHeight + nMarginTop_
	};
	pThis_->setCaretPos(pt);
	pThis_->showCaret();
}

void qs::TextWindowImpl::hideCaret()
{
	pThis_->hideCaret();
	pThis_->destroyCaret();
}

void qs::TextWindowImpl::updateCaret(bool bScroll)
{
	updateCaret(bScroll, Rect(-1, -1, -1, -1));
}

void qs::TextWindowImpl::updateCaret(bool bScroll,
									 const RECT& rectMargin)
{
	int nMarginTop = rectMargin.top != -1 ? rectMargin.top : 3;
	int nMarginBottom = rectMargin.bottom != -1 ? rectMargin.bottom : 3;
	int nMarginLeft = rectMargin.left != -1 ? rectMargin.left : 20;
	int nMarginRight = rectMargin.right != -1 ? rectMargin.right : 20;
	
	unsigned int nLineHeight = getLineHeight();
	unsigned int nLineInWindow = getLineInWindow();
	
	if (bScroll) {
		RECT rect;
		getClientRectWithoutMargin(&rect);
		
		if (static_cast<int>(caret_.nLine_) < scrollPos_.nLine_ + nMarginTop)
			pThis_->scroll(TextWindow::SCROLL_VERTICALPOS,
				caret_.nLine_ - 3, false);
		else if (caret_.nLine_ > scrollPos_.nLine_ + nLineInWindow - nMarginBottom)
			pThis_->scroll(TextWindow::SCROLL_VERTICALPOS,
				caret_.nLine_ - nLineInWindow + 3, false);
		else
			bScroll = false;
		
		if (caret_.nPos_ < scrollPos_.nPos_ + nMarginLeft)
			pThis_->scroll(TextWindow::SCROLL_HORIZONTALPOS,
				caret_.nPos_ - 20, false);
		else if (caret_.nPos_ > scrollPos_.nPos_ + (rect.right - rect.left) - nMarginRight)
			pThis_->scroll(TextWindow::SCROLL_HORIZONTALPOS,
				caret_.nPos_ - (rect.right - rect.left) + 20, false);
		else
			bScroll = false;
	}
	
	if (bShowCaret_ && !bScroll) {
		POINT pt = {
			caret_.nPos_ + nMarginLeft_ - scrollPos_.nPos_,
			(caret_.nLine_ - scrollPos_.nLine_)*nLineHeight + nMarginTop_
		};
		pThis_->setCaretPos(pt);
		
		pt.y += nLineSpacing_;
		COMPOSITIONFORM cf = { 0 };
		cf.dwStyle = CFS_POINT;
		cf.ptCurrentPos = pt;
		::ImmSetCompositionWindow(hImc_, &cf);
	}
	
	if (bShowRuler_)
		pRuler_->update();
}

void qs::TextWindowImpl::invalidate(unsigned int nStartLine,
									unsigned int nStartChar,
									unsigned int nEndLine,
									unsigned int nEndChar)
{
	if (nStartLine > nEndLine)
		std::swap(nStartLine, nEndLine);
	
	RECT rect;
	pThis_->getClientRect(&rect);
	unsigned int nLineHeight = getLineHeight();
	rect.top = nMarginTop_ + (nStartLine - scrollPos_.nLine_)*nLineHeight;
	if (nEndLine != -1)
		rect.bottom = nMarginTop_ + (nEndLine + 1 - scrollPos_.nLine_)*nLineHeight;
	
	pThis_->invalidateRect(rect);
}

bool qs::TextWindowImpl::insertText(const WCHAR* pwsz,
									size_t nLen,
									InsertTextFlag flag)
{
	if (pTextModel_->isEditable()) {
		if (nLen == static_cast<size_t>(-1))
			nLen = wcslen(pwsz);
		
		unsigned int nStartLine = 0;
		unsigned int nStartChar = 0;
		unsigned int nEndLine = -1;
		unsigned int nEndChar = -1;
		bool bReverse = true;
		
		wstring_ptr wstrSelected;
		
		bool bSelected = pThis_->isSelected();
		if (bSelected) {
			getSelection(&nStartLine, &nStartChar,
				&nEndLine, &nEndChar, &bReverse);
			wstrSelected = pThis_->getSelectedText();
			clearSelection();
		}
		else {
			nStartLine = caret_.nLine_;
			nStartChar = caret_.nChar_;
		}
		
		unsigned int nLine = 0;
		unsigned int nChar = 0;
		const PhysicalLine* pStart = listLine_[nStartLine];
		if (bSelected) {
			const PhysicalLine* pEnd = listLine_[nEndLine];
			pTextModel_->update(pStart->nLogicalLine_,
				pStart->nPosition_ + nStartChar, pEnd->nLogicalLine_,
				pEnd->nPosition_ + nEndChar, pwsz, nLen, &nLine, &nChar);
		}
		else {
			pTextModel_->update(pStart->nLogicalLine_,
				pStart->nPosition_ + nStartChar, -1, -1,
				pwsz, nLen, &nLine, &nChar);
		}
		nExtentLine_ = -1;
		
		std::pair<unsigned int, unsigned int> line = getPhysicalLine(nLine, nChar);
		caret_.nLine_ = line.first;
		caret_.nChar_ = line.second;
		caret_.nPos_ = getPosFromChar(caret_.nLine_, caret_.nChar_);
		caret_.nOldPos_ = caret_.nPos_;
		updateCaret(true);
		
		switch (flag) {
		case INSERTTEXTFLAG_NORMAL:
		case INSERTTEXTFLAG_REDO:
			pUndoManager_->pushUndoItem(nStartLine, nStartChar,
				line.first, line.second, bReverse ? nStartLine : nEndLine,
				bReverse ? nStartChar : nEndChar, wstrSelected,
				flag == INSERTTEXTFLAG_NORMAL);
			break;
		case INSERTTEXTFLAG_UNDO:
			pUndoManager_->pushRedoItem(nStartLine, nStartChar,
				line.first, line.second, bReverse ? nStartLine : nEndLine,
				bReverse ? nStartChar : nEndChar, wstrSelected);
			break;
		default:
			assert(false);
			break;
		}
	}
	
	return true;
}

bool qs::TextWindowImpl::deleteText(TextWindow::DeleteTextFlag flag)
{
	if (pTextModel_->isEditable()) {
		if (!pThis_->isSelected()) {
			TextWindow::MoveCaret mc;
			switch (flag) {
			case TextWindow::DELETETEXTFLAG_DELETECHAR:
				mc = TextWindow::MOVECARET_CHARRIGHT;
				break;
			case TextWindow::DELETETEXTFLAG_DELETEBACKWARDCHAR:
				mc = TextWindow::MOVECARET_CHARLEFT;
				break;
			case TextWindow::DELETETEXTFLAG_DELETEWORD:
				mc = TextWindow::MOVECARET_WORDRIGHT;
				break;
			case TextWindow::DELETETEXTFLAG_DELETEBACKWARDWORD:
				mc = TextWindow::MOVECARET_WORDLEFT;
				break;
			default:
				assert(false);
				break;
			}
			
			pThis_->moveCaret(mc, 0, 0, false, TextWindow::SELECT_SELECT, true);
			if (pThis_->isSelected()) {
				std::swap(selection_.nStartLine_, selection_.nEndLine_);
				std::swap(selection_.nStartChar_, selection_.nEndChar_);
			}
		}
		
		if (!insertText(L"", 0, INSERTTEXTFLAG_NORMAL))
			return false;
	}
	
	return true;
}

size_t qs::TextWindowImpl::getReformQuoteLength(const WCHAR* pwszLine,
												size_t nLen) const
{
	const WCHAR* p = pwszLine;
	for (size_t n = 0; n < nLen; ++n, ++p) {
		if (*p != L' ' && *p != L'\t' && !wcschr(wstrReformQuote_.get(), *p))
			break;
	}
	return p - pwszLine;
}

const TextWindowImpl::LinkItem* qs::TextWindowImpl::getLinkItem(unsigned int nLine,
																unsigned int nChar)
{
	const PhysicalLine* pLine = listLine_[nLine];
	if (pLine->items_.nCount_ != 0) {
		for (size_t n = 0; n < pLine->items_.nCount_; ++n) {
			const LinkItem& item = pLine->items_.items_[n];
			if (item.nOffset_ <= nChar && nChar <= item.nOffset_ + item.nLength_)
				return &item;
		}
	}
	return 0;
}

std::pair<int, const TextWindowImpl::LinkItem*> qs::TextWindowImpl::getLinkItemFromPoint(const POINT& pt) const
{
	std::pair<int, const LinkItem*> i(-1, 0);
	
	int nLine = getLineFromPos(pt.y);
	if (0 <= nLine && static_cast<LineList::size_type>(nLine) < listLine_.size()) {
		const PhysicalLine* pLine = listLine_[nLine];
		if (pLine->items_.nCount_ != 0) {
			int nPos = scrollPos_.nPos_ + (pt.x - nMarginLeft_);
			for (size_t n = 0; n < pLine->items_.nCount_ && !i.second; ++n) {
				const LinkItem& item = pLine->items_.items_[n];
				if (item.nLeft_ < nPos && nPos <= item.nRight_) {
					i.first = nLine;
					i.second = &item;
				}
			}
		}
	}
	return i;
}

bool qs::TextWindowImpl::openLink(const POINT& pt)
{
	if (pLinkHandler_) {
		std::pair<int, const LinkItem*> item(getLinkItemFromPoint(pt));
		if (item.second) {
			wstring_ptr wstrURL(getURL(item.first, item.second));
			if (!pLinkHandler_->openLink(wstrURL.get()))
				return false;
		}
	}
	
	return true;
}

wstring_ptr qs::TextWindowImpl::getURL(int nLine,
									   const LinkItem* pLinkItem) const
{
	assert(0 <= nLine && nLine < static_cast<int>(listLine_.size()));
	assert(pLinkItem);
	
	const PhysicalLine* pLine = listLine_[nLine];
	
	TextModel::Line line = pTextModel_->getLine(pLine->nLogicalLine_);
	const WCHAR* pBegin = line.getText() + pLine->nPosition_ + pLinkItem->nOffset_;
	const WCHAR* pEnd = pBegin + pLinkItem->nLength_;
	if (pLinkItem->nOffset_ == 0 && pLine->nPosition_ != 0) {
		while (pBegin >= line.getText() && TextUtil::isURLChar(*pBegin))
			--pBegin;
		++pBegin;
	}
	
	if (pLinkItem->nOffset_ + pLinkItem->nLength_ == pLine->nLength_) {
		while (pEnd - line.getText() < static_cast<int>(line.getLength()) &&
			TextUtil::isURLChar(*pEnd))
			++pEnd;
	}
	
	wstring_ptr wstrURL(allocWString(pBegin, pEnd - pBegin));
	
	URLSchemaList::const_iterator it = listURLSchema_.begin();
	while (it != listURLSchema_.end()) {
		if (wcsncmp(*it, wstrURL.get(), wcslen(*it)) == 0)
			break;
		++it;
	}
	if (it == listURLSchema_.end())
		wstrURL = concat(L"mailto:", wstrURL.get());
	
	return wstrURL;
}

void qs::TextWindowImpl::textUpdated(const TextModelEvent& event)
{
	calcLines(event.getStartLine(),
		event.getOldEndLine(), event.getNewEndLine());
	updateScrollBar();
}

void qs::TextWindowImpl::textSet(const TextModelEvent& event)
{
	recalcLines();
	caret_.nLine_ = 0;
	caret_.nChar_ = 0;
	caret_.nPos_ = 0;
	caret_.nOldPos_ = 0;
	clearSelection();
	pUndoManager_->clear();
	scrollHorizontal(0);
	scrollVertical(0);
	updateScrollBar();
	updateCaret(true);
}

TextWindowImpl::PhysicalLine* qs::TextWindowImpl::allocLine(size_t nLogicalLine,
															size_t nPosition,
															size_t nLength,
															COLORREF cr,
															unsigned char nQuoteDepth,
															unsigned char nQuoteLength,
															LinkItem* pLinkItems,
															size_t nLinkCount)
{
	size_t nSize = sizeof(PhysicalLine) -
		sizeof(LinkItem) + sizeof(LinkItem)*nLinkCount;
	
	PhysicalLine* pLine = static_cast<PhysicalLine*>(
		std::__sgi_alloc::allocate(nSize));
	pLine->nLogicalLine_ = nLogicalLine;
	pLine->nPosition_ = nPosition;
	pLine->nLength_ = nLength;
	pLine->cr_ = cr;
	pLine->nQuoteDepth_ = nQuoteDepth;
	pLine->nQuoteLength_ = nQuoteLength;
	pLine->items_.nCount_ = nLinkCount;
	if (nLinkCount != 0)
		memcpy(pLine->items_.items_, pLinkItems, nLinkCount*sizeof(LinkItem));
	return pLine;
}

void qs::TextWindowImpl::freeLine(PhysicalLine* pLine)
{
	size_t nSize = sizeof(PhysicalLine) -
		sizeof(LinkItem) + sizeof(LinkItem)*pLine->items_.nCount_;
	std::__sgi_alloc::deallocate(pLine, sizeof(nSize));
}

TextWindowImpl::CharType qs::TextWindowImpl::getCharType(WCHAR c)
{
	const WCHAR wszMark[] = L"!\"#$%&\'()*+,-.:;<=>?[\\]^_`{}~/";
	
	if (c == L' ' || c == L'\t')
		return CHARTYPE_SPACE;
	else if (c == L'\n')
		return CHARTYPE_NEWLINE;
	else if (c > 0x7f)
		return CHARTYPE_OTHER;
	else if (wcschr(wszMark, c))
		return CHARTYPE_MARK;
	else
		return CHARTYPE_ASCII;
}

bool qs::TextWindowImpl::getTextExtent(const DeviceContext& dc,
									   const WCHAR* pwszString,
									   int nCount,
									   int nMaxExtent,
									   int* pnFit,
									   int* pnDx,
									   SIZE* pSize) const
{
	if (nCount == 0) {
		if (pnFit)
			*pnFit = 0;
		pSize->cx = 0;
		pSize->cy = 0;
		return true;
	}
	
	if (!pnDx && bAdjustExtent_)
		pnDx = nDx_;
	
	int nExtent = nMaxExtent == 0 ? 0 : bAdjustExtent_ ? nMaxExtent*2 : nMaxExtent;
	if (!dc.getTextExtentEx(pwszString, nCount, nExtent, pnFit, pnDx, pSize))
		return false;
	
	if (bAdjustExtent_) {
		int nLen = pnFit ? *pnFit : nCount;
		if (nLen != 0) {
			int nCharWidth = getAverageCharWidth();
			int nPrev = 0;
			int n = 0;
			while (n < nLen) {
				int nNext = pnDx[n];
				int nWidth = pnDx[n] - nPrev <= nCharWidth*3/2 ?
					nCharWidth : nCharWidth*2;
				pnDx[n] = (n == 0 ? 0 : pnDx[n - 1]) + nWidth;
				if (nMaxExtent != 0 && pnDx[n] > nMaxExtent)
					break;
				nPrev = nNext;
				++n;
			}
			pSize->cx = pnDx[n - 1];
			if (nMaxExtent != 0) {
				assert(pnFit);
				*pnFit = n;
			}
		}
	}
	
	return true;
}

void qs::TextWindowImpl::getLineExtent(unsigned int nLine,
									   Extent* pExtent,
									   bool* pbNewLine) const
{
	ClientDeviceContext dc(pThis_->getHandle());
	ObjectSelector<HFONT> fontSelector(dc, hfont_);
	
	const PhysicalLine* pPhysicalLine = listLine_[nLine];
	const TextModel::Line& logicalLine = pTextModel_->getLine(
		pPhysicalLine->nLogicalLine_);
	getLineExtent(dc, logicalLine.getText() + pPhysicalLine->nPosition_,
		pPhysicalLine->nLength_, pExtent, pbNewLine);
}

void qs::TextWindowImpl::getLineExtent(const DeviceContext& dc,
									   const WCHAR* pwsz,
									   size_t nLength,
									   Extent* pExtent,
									   bool* pbNewLine) const
{
	assert(pwsz);
	assert(pExtent);
	assert(pbNewLine);
	
	pExtent->resize(nLength);
	
	bool bNewLine = nLength != 0 && *(pwsz + nLength - 1) == L'\n';
	const WCHAR* pBegin = pwsz;
	const WCHAR* pEnd = pBegin + nLength - (bNewLine ? 1 : 0);
	const WCHAR* p = pBegin;
	while (p <= pEnd) {
		if (p == pEnd || *p == L'\t') {
			if (p != pBegin) {
				SIZE size;
				getTextExtent(dc, pBegin, p - pBegin, 0, 0,
					&(*pExtent)[pBegin - pwsz], &size);
				if (pBegin != pwsz) {
					int nOffset = (*pExtent)[pBegin - pwsz - 1];
					for (Extent::size_type n = pBegin - pwsz; n < static_cast<Extent::size_type>(p - pwsz); ++n)
						(*pExtent)[n] += nOffset;
				}
			}
			if (p != pEnd) {
				assert(*p == L'\t');
				(*pExtent)[p - pwsz] = getNextTabStop(
					p != pwsz ? (*pExtent)[p - pwsz - 1] : 0);
				pBegin = p + 1;
			}
		}
		++p;
	}
	if (bNewLine)
		(*pExtent)[nLength - 1] = (nLength == 1 ? 0 : (*pExtent)[nLength - 2]) + getAverageCharWidth();
	*pbNewLine = bNewLine;
}

void qs::TextWindowImpl::getLinks(const WCHAR* pwsz,
								  size_t nLen,
								  LogicalLinkItemList* pList) const
{
	assert(pwsz);
	assert(pList);
	
	pList->clear();
	
	const WCHAR* p = pwsz;
	while (true) {
		std::pair<size_t, size_t> link(TextUtil::findURL(p,
			nLen, &listURLSchema_[0], listURLSchema_.size()));
		if (link.first == -1)
			break;
		
		LogicalLinkItem item = {
			p - pwsz + link.first,
			link.second
		};
		pList->push_back(item);
		
		p += link.first + link.second;
		nLen -= link.first + link.second;
	}
}

void qs::TextWindowImpl::convertLogicalLinksToPhysicalLinks(const LogicalLinkItemList& listLogicalLinkItem,
															size_t nOffset,
															size_t nLength,
															LinkItemList* pListPhysicalLink) const
{
	assert(pListPhysicalLink);
	
	pListPhysicalLink->clear();
	
	for (LogicalLinkItemList::const_iterator it = listLogicalLinkItem.begin(); it != listLogicalLinkItem.end(); ++it) {
		LinkItem item = { -1, 0 };
		
		if ((*it).nOffset_ < nOffset) {
			if ((*it).nOffset_ + (*it).nLength_ < nOffset) {
				;
			}
			else if ((*it).nOffset_ + (*it).nLength_ < nOffset + nLength) {
				item.nOffset_ = 0;
				item.nLength_ = (*it).nOffset_ + (*it).nLength_ - nOffset;
			}
			else {
				item.nOffset_ = 0;
				item.nLength_ = nLength;
			}
		}
		else if ((*it).nOffset_ < nOffset + nLength) {
			if ((*it).nOffset_ + (*it).nLength_ < nOffset) {
				assert(false);
			}
			else if ((*it).nOffset_ + (*it).nLength_ < nOffset + nLength) {
				item.nOffset_ = (*it).nOffset_ - nOffset;
				item.nLength_ = (*it).nLength_;
			}
			else {
				item.nOffset_ = (*it).nOffset_ - nOffset;
				item.nLength_ = nLength - ((*it).nOffset_ - nOffset);
			}
		}
		else {
			if ((*it).nOffset_ + (*it).nLength_ < nOffset) {
				assert(false);
			}
			else if ((*it).nOffset_ + (*it).nLength_ < nOffset + nLength) {
				assert(false);
			}
			else {
				;
			}
		}
		
		if (item.nOffset_ != -1 && item.nLength_ != 0)
			pListPhysicalLink->push_back(item);
	}
}

void qs::TextWindowImpl::fillPhysicalLinks(LinkItemList* pList,
										   const DeviceContext& dc,
										   const WCHAR* pwsz,
										   size_t nLength,
										   unsigned char nQuoteDepth) const
{
	assert(pList);
	assert(pwsz);
	
	if (pList->empty() || nLength == 0)
		return;
	
	getLineExtent(dc, pwsz, nLength, &extent_, &bExtentNewLine_);
	nExtentLine_ = -1;
	
	unsigned int nQuoteWidth = nQuoteDepth*getQuoteWidth();
	
	for (LinkItemList::iterator it = pList->begin(); it != pList->end(); ++it) {
		assert((*it).nOffset_ + (*it).nLength_ <= extent_.size());
		(*it).nLeft_ = ((*it).nOffset_ != 0 ? extent_[(*it).nOffset_ - 1] : 0) + nQuoteWidth;
		(*it).nRight_ = extent_[(*it).nOffset_ + (*it).nLength_ - 1] + nQuoteWidth;
	}
}


/****************************************************************************
 *
 * TextWindowImpl::PhysicalLinePtr
 *
 */

qs::TextWindowImpl::PhysicalLinePtr::PhysicalLinePtr(PhysicalLine* p) :
	p_(p)
{
}

qs::TextWindowImpl::PhysicalLinePtr::~PhysicalLinePtr()
{
	if (p_)
		TextWindowImpl::freeLine(p_);
}

TextWindowImpl::PhysicalLine* qs::TextWindowImpl::PhysicalLinePtr::operator->() const
{
	return p_;
}

TextWindowImpl::PhysicalLine* qs::TextWindowImpl::PhysicalLinePtr::get() const
{
	return p_;
}

TextWindowImpl::PhysicalLine* qs::TextWindowImpl::PhysicalLinePtr::release()
{
	PhysicalLine* p = p_;
	p_ = 0;
	return p;
}


/****************************************************************************
 *
 * TextWindowImpl::PhysicalLineComp
 *
 */

bool qs::TextWindowImpl::PhysicalLineComp::operator()(const PhysicalLine* pLhs,
													  const PhysicalLine* pRhs) const
{
	if (pLhs->nLogicalLine_ < pRhs->nLogicalLine_)
		return true;
	else if (pRhs->nLogicalLine_ < pLhs->nLogicalLine_)
		return false;
	else if (pLhs->nPosition_ < pRhs->nPosition_)
		return true;
	else if (pRhs->nPosition_ < pLhs->nPosition_)
		return false;
	else
		return false;
}


/****************************************************************************
 *
 * TextWindow
 *
 */

qs::TextWindow::TextWindow(TextModel* pTextModel,
						   Profile* pProfile,
						   const WCHAR* pwszSection,
						   bool bDeleteThis) :
	WindowBase(bDeleteThis),
	pImpl_(0)
{
	std::auto_ptr<TextWindowUndoManager> pUndoManager(new TextWindowUndoManager());
	
	int n = 0;
	struct InitColor
	{
		const WCHAR* pwszKey_;
		const WCHAR* pwszDefault_;
		COLORREF cr_;
	} initColors[] = {
		{ L"ForegroundColor",	L"000000",	0 },
		{ L"BackgroundColor",	L"ffffff",	0 },
		{ L"QuoteColor1",		L"008000",	0 },
		{ L"QuoteColor2",		L"000080",	0 },
		{ L"LinkColor",			L"0000ff",	0 }
	};
	for (n = 0; n < countof(initColors); ++n) {
		wstring_ptr wstr(pProfile->getString(pwszSection,
			initColors[n].pwszKey_, initColors[n].pwszDefault_));
		Color color(wstr.get());
		if (color.getColor() != 0xffffffff)
			initColors[n].cr_ = color.getColor();
	}
	
	int nUseSystemColor = pProfile->getInt(pwszSection, L"UseSystemColor", 1);
	if (nUseSystemColor) {
		initColors[0].cr_ = ::GetSysColor(COLOR_WINDOWTEXT);
		initColors[1].cr_ = ::GetSysColor(COLOR_WINDOW);
	}
	
	struct InitNumber
	{
		const WCHAR* pwszKey_;
		int nDefault_;
		int nValue_;
	} initNumbers[] = {
		{ L"LineSpecing",				2,	0 },
		{ L"CharInLine",				0,	0 },
		{ L"TabWidth",					4,	0 },
		{ L"MarginTop",					10,	0 },
		{ L"MarginBottom",				10,	0 },
		{ L"MarginLeft",				10,	0 },
		{ L"MarginRight",				10,	0 },
		{ L"ShowNewLine",				0,	0 },
		{ L"ShowTab",					0,	0 },
		{ L"ShowVerticalScrollBar",		1,	0 },
		{ L"ShowHorizontalScrollBar",	0,	0 },
		{ L"ShowCaret",					0,	0 },
		{ L"ShowRuler",					0,	0 },
		{ L"ReformLineLength",			74,	0 },
		{ L"AdjustExtent",				0,	0 },
		{ L"LineQuote",					1,	0 }
	};
	for (n = 0; n < countof(initNumbers); ++n)
		initNumbers[n].nValue_ = pProfile->getInt(pwszSection,
			initNumbers[n].pwszKey_, initNumbers[n].nDefault_);
	
	struct InitString
	{
		const WCHAR* pwszKey_;
		const WCHAR* pwszDefault_;
		WSTRING wstrValue_;
	} initStrings[] = {
		{ L"Quote1",		L">"	},
		{ L"Quote2",		L"#"	},
		{ L"ReformQuote",	L">|#"	}
	};
	for (n = 0; n < countof(initStrings); ++n)
		initStrings[n].wstrValue_ = pProfile->getString(pwszSection,
			initStrings[n].pwszKey_, initStrings[n].pwszDefault_).release();
	
	HFONT hfont = UIUtil::createFontFromProfile(pProfile, pwszSection, true);
	GdiObject<HFONT> font(hfont);
	
	LOGFONT lf;
	::GetObject(font.get(), sizeof(lf), &lf);
	bool bAdjustExtent = lf.lfPitchAndFamily & FIXED_PITCH &&
		(initNumbers[14].nValue_ != 0 || lf.lfWeight != FW_NORMAL);
	
	TextWindowImpl::URLSchemaList listURLSchema;
	int nClickableURL = pProfile->getInt(pwszSection, L"ClickableURL", 1);
	if (nClickableURL) {
		wstring_ptr wstrSchemas(pProfile->getString(pwszSection,
			L"URLSchemas", L"http https ftp file mailto"));
		
		WCHAR* p = wcstok(wstrSchemas.get(), L" ");
		while (p) {
			wstring_ptr wstr(allocWString(p));
			listURLSchema.push_back(wstr.get());
			wstr.release();
			p = wcstok(0, L" ");
		}
	}
	
	pImpl_ = new TextWindowImpl();
	pImpl_->pThis_ = this;
	pImpl_->pTextModel_ = pTextModel;
	pImpl_->pUndoManager_ = pUndoManager;
	pImpl_->pLinkHandler_ = 0;
	pImpl_->pRuler_ = 0;
	pImpl_->crForeground_ = initColors[0].cr_;
	pImpl_->crBackground_ = initColors[1].cr_;
	pImpl_->nLineSpacing_ = initNumbers[0].nValue_;
	pImpl_->nCharInLine_ = initNumbers[1].nValue_;
	pImpl_->nTabWidth_ = initNumbers[2].nValue_;
	pImpl_->nMarginTop_ = initNumbers[3].nValue_;
	pImpl_->nMarginBottom_ = initNumbers[4].nValue_;
	pImpl_->nMarginLeft_ = initNumbers[5].nValue_;
	pImpl_->nMarginRight_ = initNumbers[6].nValue_;
	pImpl_->bShowNewLine_ = initNumbers[7].nValue_ != 0;
	pImpl_->bShowTab_ = initNumbers[8].nValue_ != 0;
	pImpl_->bShowVerticalScrollBar_ = initNumbers[9].nValue_ != 0;
	pImpl_->bShowHorizontalScrollBar_ = initNumbers[10].nValue_ != 0;
	pImpl_->bShowCaret_ = initNumbers[11].nValue_ != 0;
	pImpl_->bShowRuler_ = initNumbers[12].nValue_ != 0;
	for (n = 0; n < countof(pImpl_->wstrQuote_); ++n) {
		pImpl_->wstrQuote_[n].reset(initStrings[n].wstrValue_);
		pImpl_->crQuote_[n] = initColors[n + 2].cr_;
	}
	pImpl_->bLineQuote_ = initNumbers[15].nValue_ != 0;
	pImpl_->nReformLineLength_ = initNumbers[13].nValue_;
	pImpl_->wstrReformQuote_.reset(initStrings[2].wstrValue_);
	pImpl_->listURLSchema_.swap(listURLSchema);
	pImpl_->crLink_ = initColors[4].cr_;
	pImpl_->hfont_ = font.release();
	pImpl_->bAdjustExtent_ = bAdjustExtent;
	pImpl_->hbmNewLine_ = 0;
	pImpl_->hbmTab_ = 0;
	pImpl_->scrollPos_.nPos_ = 0;
	pImpl_->scrollPos_.nLine_ = 0;
	pImpl_->caret_.nLine_ = 0;
	pImpl_->caret_.nChar_ = 0;
	pImpl_->caret_.nPos_ = 0;
	pImpl_->caret_.nOldPos_ = 0;
	pImpl_->selection_.nStartLine_ = 0;
	pImpl_->selection_.nStartChar_ = 0;
	pImpl_->selection_.nEndLine_ = 0;
	pImpl_->selection_.nEndChar_ = 0;
	pImpl_->nTimerDragScroll_ = 0;
	pImpl_->ptLastButtonDown_.x = -1;
	pImpl_->ptLastButtonDown_.y = -1;
	pImpl_->nLastWindowWidth_ = 0;
	pImpl_->hImc_ = 0;
	pImpl_->bAtok_ = false;
	pImpl_->hCursorLink_ = ::LoadCursor(getDllInstanceHandle(),
		MAKEINTRESOURCE(IDC_LINK));
	pImpl_->nLineHeight_ = 0;
	pImpl_->nLineInWindow_ = 0;
	pImpl_->nAverageCharWidth_ = 0;
	pImpl_->bHorizontalScrollable_ = false;
	pImpl_->nExtentLine_ = -1;
	pImpl_->bExtentNewLine_ = false;
	
	setWindowHandler(this, false);
}

qs::TextWindow::~TextWindow()
{
	if (pImpl_) {
		std::for_each(pImpl_->listURLSchema_.begin(),
			pImpl_->listURLSchema_.end(), string_free<WSTRING>());
		std::for_each(pImpl_->listLine_.begin(),
			pImpl_->listLine_.end(), &TextWindowImpl::freeLine);
		delete pImpl_;
	}
}

TextModel* qs::TextWindow::getTextModel() const
{
	return pImpl_->pTextModel_;
}

void qs::TextWindow::setTextModel(TextModel* pTextModel)
{
	assert(pTextModel);
	assert(!pImpl_->pTextModel_);
	pImpl_->pTextModel_ = pTextModel;
}

bool qs::TextWindow::insertText(const WCHAR* pwszText,
								size_t nLen)
{
	return pImpl_->insertText(pwszText, nLen,
		TextWindowImpl::INSERTTEXTFLAG_NORMAL);
}

bool qs::TextWindow::deleteText(DeleteTextFlag flag)
{
	return pImpl_->deleteText(flag);
}

bool qs::TextWindow::isSelected() const
{
	return pImpl_->selection_.nStartLine_ != pImpl_->selection_.nEndLine_ ||
		pImpl_->selection_.nStartChar_ != pImpl_->selection_.nEndChar_;
}

wstring_ptr qs::TextWindow::getSelectedText() const
{
	unsigned int nStartLine = 0;
	unsigned int nStartChar = 0;
	unsigned int nEndLine = 0;
	unsigned int nEndChar = 0;
	pImpl_->getSelection(&nStartLine, &nStartChar, &nEndLine, &nEndChar, 0);
	
	StringBuffer<WSTRING> buf;
	
	for (unsigned int n = nStartLine; n <= nEndLine; ++n) {
		const TextWindowImpl::PhysicalLine* pLine = pImpl_->listLine_[n];
		TextModel::Line l = pImpl_->pTextModel_->getLine(pLine->nLogicalLine_);
		unsigned int nStart = n == nStartLine ? nStartChar : 0;
		unsigned int nEnd = n == nEndLine ? nEndChar : pLine->nLength_;
		if (nStart == 0 && pLine->nPosition_ == pLine->nQuoteLength_)
			buf.append(l.getText(), pLine->nQuoteLength_);
		buf.append(l.getText() + pLine->nPosition_ + nStart, nEnd - nStart);
	}
	
	return buf.getString();
}

bool qs::TextWindow::selectAll()
{
	moveCaret(MOVECARET_DOCSTART, 0, 0, false, SELECT_CLEAR, false);
	moveCaret(MOVECARET_DOCEND, 0, 0, false, SELECT_SELECT, true);
	return true;
}

bool qs::TextWindow::canSelectAll() const
{
	return pImpl_->pTextModel_->getLineCount() != 0;
}

bool qs::TextWindow::cut()
{
	if (!pImpl_->pTextModel_->isEditable())
		return false;
	
	if (!copy())
		return false;
	if (!pImpl_->insertText(L"", 0, TextWindowImpl::INSERTTEXTFLAG_NORMAL))
		return false;
	
	return true;
}

bool qs::TextWindow::canCut() const
{
	return pImpl_->pTextModel_->isEditable() && isSelected();
}

bool qs::TextWindow::copy()
{
	wstring_ptr wstrText(getSelectedText());
	if (!wstrText.get())
		return false;
	return Clipboard::setText(getHandle(), wstrText.get());
}

bool qs::TextWindow::canCopy() const
{
	return isSelected();
}

bool qs::TextWindow::paste()
{
	if (!pImpl_->pTextModel_->isEditable())
		return false;
	
	wstring_ptr wstrText(Clipboard::getText(getHandle()));
	if (wstrText.get()) {
		if (!pImpl_->insertText(wstrText.get(), -1,
			TextWindowImpl::INSERTTEXTFLAG_NORMAL))
			return false;
	}
	
	return true;
}

bool qs::TextWindow::canPaste() const
{
	if (pImpl_->pTextModel_->isEditable())
		return Clipboard::isFormatAvailable(Clipboard::CF_QSTEXT);
	else
		return false;
}

bool qs::TextWindow::undo()
{
	if (!pImpl_->pTextModel_->isEditable())
		return false;
	
	if (!pImpl_->pUndoManager_->hasUndoItem())
		return false;
	
	std::auto_ptr<TextWindowUndoManager::Item> pItem(
		pImpl_->pUndoManager_->popUndoItem());
	pImpl_->clearSelection();
	pImpl_->expandSelection(pItem->getStartLine(), pItem->getStartChar(),
		pItem->getEndLine(), pItem->getEndChar());
	moveCaret(TextWindow::MOVECARET_POS, pItem->getEndLine(),
		pItem->getEndChar(), false, TextWindow::SELECT_NONE, false);
	if (!pImpl_->insertText(pItem->getText(), -1,
		TextWindowImpl::INSERTTEXTFLAG_UNDO))
		return false;
	
	moveCaret(TextWindow::MOVECARET_POS, pItem->getCaretLine(),
		 pItem->getCaretChar(), false, TextWindow::SELECT_NONE, true);
	
	return true;
}

bool qs::TextWindow::canUndo() const
{
	return pImpl_->pUndoManager_->hasUndoItem();
}

bool qs::TextWindow::redo()
{
	if (!pImpl_->pTextModel_->isEditable())
		return false;
	
	if (!pImpl_->pUndoManager_->hasRedoItem())
		return false;
	
	std::auto_ptr<TextWindowUndoManager::Item> pItem(
		pImpl_->pUndoManager_->popRedoItem());
	pImpl_->clearSelection();
	pImpl_->expandSelection(pItem->getStartLine(), pItem->getStartChar(),
		pItem->getEndLine(), pItem->getEndChar());
	if (!pImpl_->insertText(pItem->getText(), -1,
		TextWindowImpl::INSERTTEXTFLAG_REDO))
		return false;
	
	moveCaret(TextWindow::MOVECARET_POS,pItem->getCaretLine(),
		 pItem->getCaretChar(), false, TextWindow::SELECT_NONE, true);
	
	return true;
}

bool qs::TextWindow::canRedo() const
{
	return pImpl_->pUndoManager_->hasRedoItem();
}

bool qs::TextWindow::find(const WCHAR* pwszFind,
						  unsigned int nFlags)
{
	return replace(pwszFind, 0, nFlags);
}

bool qs::TextWindow::replace(const WCHAR* pwszFind,
							 const WCHAR* pwszReplace,
							 unsigned int nFlags)
{
	assert(pwszFind);
	
	if (*pwszFind == L'\0')
		return false;
	
	bool bFound = false;
	
	// TODO
	// Treat FLAG_REFORMED
	
	unsigned int nLine = -1;
	unsigned int nChar = -1;
	if (isSelected()) {
		if (nFlags & FIND_PREVIOUS)
			pImpl_->getSelection(&nLine, &nChar, 0, 0, 0);
		else
			pImpl_->getSelection(0, 0, &nLine, &nChar, 0);
	}
	else if (isShowCaret()) {
		nLine = pImpl_->caret_.nLine_;
		nChar = pImpl_->caret_.nChar_;
	}
	
	bool bRegex = (nFlags & FIND_REGEX) != 0;
	std::auto_ptr<RegexPattern> pPattern;
	if (bRegex) {
		pPattern = RegexCompiler().compile(pwszFind);
		if (!pPattern.get())
			return false;
	}
	RegexRangeList listRange;
	
	std::pair<unsigned int, unsigned int> start(0, 0);
	std::pair<unsigned int, unsigned int> end(0, 0);
	
	size_t nLen = wcslen(pwszFind);
	if (nFlags & FIND_PREVIOUS) {
		if (nLine == -1)
			nLine = pImpl_->listLine_.size() - 1;
		if (nChar == -1)
			nChar = pImpl_->listLine_[nLine]->nLength_;
		
		unsigned int nLogicalLine = pImpl_->listLine_[nLine]->nLogicalLine_;
		unsigned int nLogicalChar = pImpl_->listLine_[nLine]->nPosition_ + nChar;
		if (bRegex) {
			while (nLogicalLine != -1) {
				TextModel::Line line = pImpl_->pTextModel_->getLine(nLogicalLine);
				const WCHAR* pStart = 0;
				const WCHAR* pEnd = 0;
				pPattern->search(line.getText(), line.getLength(),
					line.getText() + (nLogicalChar == -1 ? line.getLength() : nLogicalChar),
					true, &pStart, &pEnd, &listRange);
				if (pStart) {
					nLogicalChar = pStart - line.getText();
					nLen = pEnd - pStart;
					break;
				}
				nLogicalChar = -1;
				--nLogicalLine;
			}
		}
		else {
			unsigned int nBMFlags = BMFindString<WSTRING>::FLAG_REVERSE |
				((nFlags & FIND_MATCHCASE) == 0 ?
					BMFindString<WSTRING>::FLAG_IGNORECASE : 0);
			BMFindString<WSTRING> bmfs(pwszFind, nLen, nBMFlags);
			
			while (nLogicalLine != -1) {
				TextModel::Line line = pImpl_->pTextModel_->getLine(nLogicalLine);
				const WCHAR* p = bmfs.find(line.getText(),
					nLogicalChar == -1 ? line.getLength() : nLogicalChar);
				if (p) {
					nLogicalChar = p - line.getText();
					break;
				}
				nLogicalChar = -1;
				--nLogicalLine;
			}
		}
		if (nLogicalLine != -1) {
			start = pImpl_->getPhysicalLine(nLogicalLine, nLogicalChar + nLen);
			end = pImpl_->getPhysicalLine(nLogicalLine, nLogicalChar);
			bFound = true;
		}
	}
	else {
		if (nLine == -1)
			nLine = 0;
		if (nChar == -1)
			nChar = 0;
		
		unsigned int nLogicalLine = pImpl_->listLine_[nLine]->nLogicalLine_;
		unsigned int nLogicalChar = pImpl_->listLine_[nLine]->nPosition_ + nChar;
		if (bRegex) {
			while (nLogicalLine < pImpl_->pTextModel_->getLineCount()) {
				TextModel::Line line = pImpl_->pTextModel_->getLine(nLogicalLine);
				const WCHAR* pStart = 0;
				const WCHAR* pEnd = 0;
				pPattern->search(line.getText() + nLogicalChar,
					line.getLength() - nLogicalChar, line.getText() + nLogicalChar,
					false, &pStart, &pEnd, &listRange);
				if (pStart) {
					nLogicalChar = pStart - line.getText();
					nLen = pEnd - pStart;
					break;
				}
				nLogicalChar = 0;
				++nLogicalLine;
			}
		}
		else {
			unsigned int nBMFlags = (nFlags & FIND_MATCHCASE) == 0 ?
				BMFindString<WSTRING>::FLAG_IGNORECASE : 0;
			BMFindString<WSTRING> bmfs(pwszFind, nLen, nBMFlags);
			
			while (nLogicalLine < pImpl_->pTextModel_->getLineCount()) {
				TextModel::Line line = pImpl_->pTextModel_->getLine(nLogicalLine);
				const WCHAR* p = bmfs.find(line.getText() + nLogicalChar,
					line.getLength() - nLogicalChar);
				if (p) {
					nLogicalChar = p - line.getText();
					break;
				}
				nLogicalChar = 0;
				++nLogicalLine;
			}
		}
		if (nLogicalLine != pImpl_->pTextModel_->getLineCount()) {
			start = pImpl_->getPhysicalLine(nLogicalLine, nLogicalChar);
			end = pImpl_->getPhysicalLine(nLogicalLine, nLogicalChar + nLen);
			bFound = true;
		}
	}
	
	if (bFound) {
		moveCaret(MOVECARET_POS, start.first,
			start.second, false, SELECT_CLEAR, false);
		moveCaret(MOVECARET_POS, end.first,
			end.second, false, SELECT_SELECT, true);
		
		if (pwszReplace) {
			if (bRegex) {
				wstring_ptr wstrReplace(listRange.getReplace(pwszReplace));
				if (!insertText(wstrReplace.get(), -1))
					return false;
			}
			else {
				if (!insertText(pwszReplace, -1))
					return false;
			}
		}
	}
	
	return bFound;
}

void qs::TextWindow::reform()
{
	size_t nStart = 0;
	size_t nEnd = 0;
	
	if (isSelected()) {
		unsigned int nStartLine = 0;
		unsigned int nStartChar = 0;
		unsigned int nEndLine = 0;
		unsigned int nEndChar = 0;
		pImpl_->getSelection(&nStartLine, &nStartChar, &nEndLine, &nEndChar, 0);
		
		nStart = pImpl_->listLine_[nStartLine]->nLogicalLine_;
		nEnd = pImpl_->listLine_[nEndLine]->nLogicalLine_;
	}
	else {
		TextWindowImpl::PhysicalLine& line =
			*pImpl_->listLine_[pImpl_->caret_.nLine_];
		nStart = line.nLogicalLine_;
		
		TextModel::Line firstLine = pImpl_->pTextModel_->getLine(nStart);
		const WCHAR* pwszQuote = firstLine.getText();
		size_t nFirstQuoteLen = pImpl_->getReformQuoteLength(
			pwszQuote, firstLine.getLength());
		
		nEnd = nStart;
		bool bCaretEnd = pImpl_->caret_.nChar_ == line.nLength_ - 1;
		while (nEnd < pImpl_->pTextModel_->getLineCount()) {
			TextModel::Line line = pImpl_->pTextModel_->getLine(nEnd);
			size_t nQuoteLen = pImpl_->getReformQuoteLength(
				line.getText(), line.getLength());
			if (nQuoteLen != nFirstQuoteLen ||
				wcsncmp(pwszQuote, line.getText(), nQuoteLen) != 0) {
				assert(nEnd != nStart);
				--nEnd;
				break;
			}
			if (line.getLength() != 0 && *(line.getText() + nQuoteLen) == L'\n') {
				if (nEnd != nStart)
					--nEnd;
				break;
			}
			
			if (!bCaretEnd) {
				size_t nLen = 0;
				const WCHAR* p = line.getText();
				for (size_t n = 0; n < line.getLength(); ++n, ++p) {
					if (*p == L'\n')
						;
					else if (*p == L'\t')
						nLen = (nLen/pImpl_->nTabWidth_ + 1)*pImpl_->nTabWidth_;
					else if (TextUtil::isHalfWidth(*p))
						nLen += 1;
					else
						nLen += 2;
				}
				if (nLen < pImpl_->nReformLineLength_ - 6 ||
					nLen > pImpl_->nReformLineLength_ + 6)
					break;
			}
			else {
				bCaretEnd = false;
			}
			++nEnd;
		}
		if (nEnd == pImpl_->pTextModel_->getLineCount())
			--nEnd;
	}
	
	wstring_ptr wstrQuote;
	TextModel::Line firstLine = pImpl_->pTextModel_->getLine(nStart);
	size_t nQuoteLen = pImpl_->getReformQuoteLength(
		firstLine.getText(), firstLine.getLength());
	if (nQuoteLen != 0)
		wstrQuote = allocWString(firstLine.getText(), nQuoteLen);
	
	StringBuffer<WSTRING> buf;
	bool bAscii = false;
	for (size_t n = nStart; n <= nEnd; ++n) {
		TextModel::Line line = pImpl_->pTextModel_->getLine(n);
		const WCHAR* p = line.getText();
		size_t nQuoteLen = pImpl_->getReformQuoteLength(p, line.getLength());
		size_t nLen = line.getLength() - nQuoteLen;
		if (nLen != 0) {
			p += nQuoteLen;
			if (bAscii && *p < 0x80)
				buf.append(L' ');
			for (--nLen; nLen > 0; --nLen, ++p)
				buf.append(*p);
			if (*p != L'\n')
				buf.append(*p);
			bAscii = *p < 0x80;
		}
		else {
			bAscii = false;
		}
	}
	if (buf.getLength() == 0)
		return;
	buf.append(L'\n');
	
	wxstring_ptr wstrText(TextUtil::fold(buf.getCharArray(),
		buf.getLength(), pImpl_->nReformLineLength_, wstrQuote.get(),
		nQuoteLen, pImpl_->nTabWidth_));
	
	std::pair<unsigned int, unsigned int> l;
	l = pImpl_->getPhysicalLine(nStart, 0);
	pImpl_->selection_.nStartLine_ = l.first;
	pImpl_->selection_.nStartChar_ = l.second;
	TextModel::Line line = pImpl_->pTextModel_->getLine(nEnd);
	if (line.getLength() != 0 &&
		*(line.getText() + line.getLength() - 1) == L'\n') {
		l = pImpl_->getPhysicalLine(nEnd + 1, 0);
		pImpl_->selection_.nEndLine_ = l.first;
		pImpl_->selection_.nEndChar_ = l.second;
	}
	else {
		l = pImpl_->getPhysicalLine(nEnd,
			pImpl_->pTextModel_->getLine(nEnd).getLength());
		pImpl_->selection_.nEndLine_ = l.first;
		pImpl_->selection_.nEndChar_ = l.second;
	}
	
	pImpl_->insertText(wstrText.get(), -1,
		TextWindowImpl::INSERTTEXTFLAG_NORMAL);
	
	moveCaret(MOVECARET_CHARLEFT, 0, 0, false, SELECT_NONE, true);
}

void qs::TextWindow::scroll(Scroll scroll,
							int nPos,
							bool bRepeat)
{
	if (scroll & SCROLL_VERTICAL_MASK) {
		unsigned int nLineInWindow = pImpl_->getLineInWindow();
		
		int nEnd = pImpl_->listLine_.size() - nLineInWindow;
		if (nEnd < 0)
			nEnd = 0;
		
		int nNewPos = pImpl_->scrollPos_.nLine_;
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
			nNewPos -= nLineInWindow - 1;
			break;
		case SCROLL_PAGEDOWN:
			nNewPos += nLineInWindow - 1;
			break;
		case SCROLL_VERTICALPOS:
			nNewPos = nPos;
			break;
		}
		if (nNewPos < 0)
			nNewPos = 0;
		else if (nNewPos > nEnd)
			nNewPos = nEnd;
		
		pImpl_->scrollVertical(nNewPos);
	}
	else if (scroll & SCROLL_HORIZONTAL_MASK) {
		int nCharWidth = pImpl_->getAverageCharWidth();
		
		RECT rect;
		pImpl_->getClientRectWithoutMargin(&rect);
		int nPage = rect.right - rect.left;
		
		int nMaxWidth = (pImpl_->nCharInLine_ == 0 ?
			nPage : pImpl_->nCharInLine_*nCharWidth) - 1;
		
		int nEnd = nMaxWidth - nPage;
		if (nEnd < 0)
			nEnd = 0;
		
		int nNewPos = pImpl_->scrollPos_.nPos_;
		switch (scroll) {
		case SCROLL_LEFT:
			nNewPos = 0;
			break;
		case SCROLL_RIGHT:
			nNewPos = nEnd;
			break;
		case SCROLL_CHARLEFT:
			nNewPos -= (bRepeat ? 2 : 1)*nCharWidth*4;
			break;
		case SCROLL_CHARRIGHT:
			nNewPos += (bRepeat ? 2 : 1)*nCharWidth*4;
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
		
		pImpl_->scrollHorizontal(nNewPos);
	}
	
	pImpl_->updateCaret(false);
}

void qs::TextWindow::moveCaret(MoveCaret moveCaret,
							   unsigned int nLine,
							   unsigned int nChar,
							   bool bRepeat,
							   Select select,
							   bool bScroll)
{
	if (!pImpl_->listLine_.empty()) {
		TextWindowImpl::Caret caret = pImpl_->caret_;
		
		const TextWindowImpl::PhysicalLine* pLine =
			pImpl_->listLine_[pImpl_->caret_.nLine_];
		
		unsigned int nLineCount = pImpl_->listLine_.size();
		unsigned int nLineInWindow = pImpl_->getLineInWindow();
		
		RECT rectMargin = { -1, -1, -1, -1 };
		
		switch (moveCaret) {
		case MOVECARET_CHARLEFT:
			if (pImpl_->caret_.nLine_ != 0 || pImpl_->caret_.nChar_ != 0) {
				--pImpl_->caret_.nChar_;
				if (static_cast<int>(pImpl_->caret_.nChar_) < 0 &&
					pImpl_->caret_.nLine_ != 0) {
					--pImpl_->caret_.nLine_;
					pImpl_->caret_.nChar_ =
						pImpl_->listLine_[pImpl_->caret_.nLine_]->nLength_ - 1;
				}
				else {
					rectMargin.top = 0;
				}
				rectMargin.bottom = 0;
				pImpl_->caret_.nPos_ = pImpl_->getPosFromChar(
					pImpl_->caret_.nLine_, pImpl_->caret_.nChar_);
				pImpl_->caret_.nOldPos_ = pImpl_->caret_.nPos_;
			}
			break;
		case MOVECARET_CHARRIGHT:
			if (pImpl_->caret_.nLine_ != nLineCount - 1 ||
				pImpl_->caret_.nChar_ != pLine->nLength_) {
				++pImpl_->caret_.nChar_;
				if (pImpl_->caret_.nChar_ == pLine->nLength_ &&
					pImpl_->caret_.nLine_ != nLineCount - 1) {
					++pImpl_->caret_.nLine_;
					pImpl_->caret_.nChar_ = 0;
				}
				else {
					rectMargin.bottom = 0;
				}
				rectMargin.top = 0;
				pImpl_->caret_.nPos_ = pImpl_->getPosFromChar(
					pImpl_->caret_.nLine_, pImpl_->caret_.nChar_);
				pImpl_->caret_.nOldPos_ = pImpl_->caret_.nPos_;
			}
			break;
		case MOVECARET_WORDLEFT:
			if (pImpl_->caret_.nLine_ != 0 || pImpl_->caret_.nChar_ != 0) {
				unsigned int nLine = pImpl_->caret_.nLine_;
				unsigned int nChar = pImpl_->caret_.nChar_;
				TextWindowImpl::CharType firstType = TextWindowImpl::CHARTYPE_NONE;
				bool bBreak = false;
				while (!bBreak) {
					const TextWindowImpl::PhysicalLine* pLine =
						pImpl_->listLine_[nLine];
					TextModel::Line line = pImpl_->pTextModel_->getLine(
						pLine->nLogicalLine_);
					if (nChar == -1)
						nChar = pLine->nLength_ + 1;
					for (unsigned int n = nChar; n > 0 && !bBreak; --n) {
						if (n == 1 && pLine->nPosition_ == 0) {
							bBreak = true;
							nChar = 0;
							break;
						}
						WCHAR c = *(line.getText() + pLine->nPosition_ + n - 2);
						TextWindowImpl::CharType type = TextWindowImpl::getCharType(c);
						if (firstType == TextWindowImpl::CHARTYPE_NONE) {
							firstType = type;
						}
						else {
							switch (type) {
							case TextWindowImpl::CHARTYPE_SPACE:
							case TextWindowImpl::CHARTYPE_MARK:
								bBreak = firstType != TextWindowImpl::CHARTYPE_SPACE &&
									firstType != TextWindowImpl::CHARTYPE_MARK;
								break;
							case TextWindowImpl::CHARTYPE_NEWLINE:
								bBreak = true;
								break;
							case TextWindowImpl::CHARTYPE_ASCII:
							case TextWindowImpl::CHARTYPE_OTHER:
								if (firstType == TextWindowImpl::CHARTYPE_SPACE ||
									firstType == TextWindowImpl::CHARTYPE_MARK)
									firstType = type;
								else
									bBreak = type != firstType;
								break;
							default:
								assert(false);
								break;
							}
							if (bBreak)
								nChar = n - 1;
						}
					}
					if (bBreak) {
						break;
					}
					else if (nLine == 0) {
						nChar = 0;
						break;
					}
					--nLine;
					nChar = -1;
				}
				pImpl_->caret_.nLine_ = nLine;
				pImpl_->caret_.nChar_ = nChar;
				pImpl_->caret_.nPos_ = pImpl_->getPosFromChar(
					pImpl_->caret_.nLine_, pImpl_->caret_.nChar_);
				pImpl_->caret_.nOldPos_ = pImpl_->caret_.nPos_;
			}
			break;
		case MOVECARET_WORDRIGHT:
			if (pImpl_->caret_.nLine_ != nLineCount - 1 ||
				pImpl_->caret_.nChar_ != pLine->nLength_) {
				unsigned int nLine = pImpl_->caret_.nLine_;
				unsigned int nChar = pImpl_->caret_.nChar_;
				TextWindowImpl::CharType firstType = TextWindowImpl::CHARTYPE_NONE;
				bool bBreak = false;
				while (nLine < pImpl_->listLine_.size()) {
					const TextWindowImpl::PhysicalLine* pLine =
						pImpl_->listLine_[nLine];
					TextModel::Line line = pImpl_->pTextModel_->getLine(
						pLine->nLogicalLine_);
					for (unsigned int n = nChar; n < pLine->nLength_ && !bBreak; ++n) {
						WCHAR c = *(line.getText() + pLine->nPosition_ + n);
						TextWindowImpl::CharType type = TextWindowImpl::getCharType(c);
						if (firstType == TextWindowImpl::CHARTYPE_NONE) {
							firstType = type;
						}
						else {
							switch (type) {
							case TextWindowImpl::CHARTYPE_SPACE:
							case TextWindowImpl::CHARTYPE_MARK:
								firstType = type;
								break;
							case TextWindowImpl::CHARTYPE_NEWLINE:
								bBreak = true;
								break;
							case TextWindowImpl::CHARTYPE_ASCII:
							case TextWindowImpl::CHARTYPE_OTHER:
								bBreak = type != firstType;
								break;
							default:
								assert(false);
								break;
							}
							if (bBreak)
								nChar = n;
						}
					}
					if (bBreak)
						break;
					++nLine;
					nChar = 0;
				}
				if (nLine < pImpl_->listLine_.size()) {
					pImpl_->caret_.nLine_ = nLine;
					pImpl_->caret_.nChar_ = nChar;
				}
				else {
					pImpl_->caret_.nLine_ = pImpl_->listLine_.size() - 1;
					pImpl_->caret_.nChar_ = pImpl_->listLine_[pImpl_->caret_.nLine_]->nLength_;
				}
				pImpl_->caret_.nPos_ = pImpl_->getPosFromChar(
					pImpl_->caret_.nLine_, pImpl_->caret_.nChar_);
				pImpl_->caret_.nOldPos_ = pImpl_->caret_.nPos_;
			}
			break;
		case MOVECARET_LINESTART:
			pImpl_->caret_.nChar_ = 0;
			pImpl_->caret_.nPos_ = pImpl_->getPosFromChar(
				pImpl_->caret_.nLine_, pImpl_->caret_.nChar_);
			pImpl_->caret_.nOldPos_ = pImpl_->caret_.nPos_;
			rectMargin.top = 0;
			rectMargin.bottom = 0;
			break;
		case MOVECARET_LINEEND:
			pImpl_->caret_.nChar_ = pLine->nLength_ -
				(pImpl_->caret_.nLine_ == nLineCount - 1 ? 0 : 1);
			pImpl_->caret_.nPos_ = pImpl_->getPosFromChar(
				pImpl_->caret_.nLine_, pImpl_->caret_.nChar_);
			pImpl_->caret_.nOldPos_ = pImpl_->caret_.nPos_;
			rectMargin.top = 0;
			rectMargin.bottom = 0;
			break;
		case MOVECARET_LINEUP:
			if (pImpl_->caret_.nLine_ != 0) {
				--pImpl_->caret_.nLine_;
				if (bRepeat && pImpl_->caret_.nLine_ != 0)
					--pImpl_->caret_.nLine_;
				pImpl_->caret_.nChar_ = pImpl_->getCharFromPos(
					pImpl_->caret_.nLine_, pImpl_->caret_.nOldPos_);
				pImpl_->caret_.nPos_ = pImpl_->getPosFromChar(
					pImpl_->caret_.nLine_, pImpl_->caret_.nChar_);
			}
			rectMargin.bottom = 0;
			break;
		case MOVECARET_LINEDOWN:
			if (pImpl_->caret_.nLine_ != nLineCount - 1) {
				++pImpl_->caret_.nLine_;
				if (bRepeat && pImpl_->caret_.nLine_ != nLineCount - 1)
					++pImpl_->caret_.nLine_;
				pImpl_->caret_.nChar_ = pImpl_->getCharFromPos(
					pImpl_->caret_.nLine_, pImpl_->caret_.nOldPos_);
				pImpl_->caret_.nPos_ = pImpl_->getPosFromChar(
					pImpl_->caret_.nLine_, pImpl_->caret_.nChar_);
			}
			rectMargin.top = 0;
			break;
		case MOVECARET_PAGEUP:
			if (pImpl_->caret_.nLine_ != 0) {
				pImpl_->caret_.nLine_ -= nLineInWindow - 1;
				if (static_cast<int>(pImpl_->caret_.nLine_) < 0)
					pImpl_->caret_.nLine_ = 0;
				pImpl_->caret_.nChar_ = pImpl_->getCharFromPos(
					pImpl_->caret_.nLine_, pImpl_->caret_.nOldPos_);
				pImpl_->caret_.nPos_ = pImpl_->getPosFromChar(
					pImpl_->caret_.nLine_, pImpl_->caret_.nChar_);
			}
			break;
		case MOVECARET_PAGEDOWN:
			if (pImpl_->caret_.nLine_ != nLineCount - 1) {
				pImpl_->caret_.nLine_ += nLineInWindow - 1;
				if (pImpl_->caret_.nLine_ > nLineCount - 1)
					pImpl_->caret_.nLine_ = nLineCount - 1;
				pImpl_->caret_.nChar_ = pImpl_->getCharFromPos(
					pImpl_->caret_.nLine_, pImpl_->caret_.nOldPos_);
				pImpl_->caret_.nPos_ = pImpl_->getPosFromChar(
					pImpl_->caret_.nLine_, pImpl_->caret_.nChar_);
			}
			break;
		case MOVECARET_DOCSTART:
			pImpl_->caret_.nLine_ = 0;
			pImpl_->caret_.nChar_ = 0;
			pImpl_->caret_.nPos_ = pImpl_->getPosFromChar(
				pImpl_->caret_.nLine_, pImpl_->caret_.nChar_);
			pImpl_->caret_.nOldPos_ = pImpl_->caret_.nPos_;
			break;
		case MOVECARET_DOCEND:
			pImpl_->caret_.nLine_ = nLineCount - 1;
			pImpl_->caret_.nChar_ =
				pImpl_->listLine_[pImpl_->caret_.nLine_]->nLength_;
			pImpl_->caret_.nPos_ = pImpl_->getPosFromChar(
				pImpl_->caret_.nLine_, pImpl_->caret_.nChar_);
			pImpl_->caret_.nOldPos_ = pImpl_->caret_.nPos_;
			break;
		case MOVECARET_POS:
			pImpl_->caret_.nLine_ = nLine;
			pImpl_->caret_.nChar_ = nChar;
			pImpl_->caret_.nPos_ = pImpl_->getPosFromChar(
				pImpl_->caret_.nLine_, pImpl_->caret_.nChar_);
			pImpl_->caret_.nOldPos_ = pImpl_->caret_.nPos_;
			break;
		default:
			assert(false);
			break;
		}
		
		if (select == SELECT_SELECT)
			pImpl_->expandSelection(caret.nLine_, caret.nChar_,
				pImpl_->caret_.nLine_, pImpl_->caret_.nChar_);
		else if (select == SELECT_CLEAR)
			pImpl_->clearSelection();
		
		pImpl_->updateCaret(bScroll, rectMargin);
	}
}

void qs::TextWindow::deselectAll()
{
	pImpl_->clearSelection();
}

bool qs::TextWindow::openLink()
{
	if (pImpl_->pLinkHandler_) {
		wstring_ptr wstrURL;
		if (isSelected()) {
			wstrURL = getSelectedText();
			
			const WCHAR* pSrc = wstrURL.get();
			WCHAR* pDst = wstrURL.get();
			while (*pSrc) {
				if (*pSrc != L'\n')
					*pDst++ = *pSrc;
				++pSrc;
			}
			*pDst = L'\0';
			
			if (!pImpl_->pLinkHandler_->openLink(wstrURL.get()))
				return false;
		}
		else {
			const TextWindowImpl::LinkItem* pLinkItem =
				pImpl_->getLinkItem(pImpl_->caret_.nLine_, pImpl_->caret_.nChar_);
			if (pLinkItem) {
				wstrURL = pImpl_->getURL(pImpl_->caret_.nLine_, pLinkItem);
				if (!pImpl_->pLinkHandler_->openLink(wstrURL.get()))
					return false;
			}
		}
	}
	
	return true;
}

TextWindowLinkHandler* qs::TextWindow::getLinkHandler() const
{
	return pImpl_->pLinkHandler_;
}

void qs::TextWindow::setLinkHandler(TextWindowLinkHandler* pLinkHandler)
{
	pImpl_->pLinkHandler_ = pLinkHandler;
}

COLORREF qs::TextWindow::getForegroundColor() const
{
	return pImpl_->crForeground_;
}

void qs::TextWindow::setForegroundColor(COLORREF cr)
{
	pImpl_->crForeground_ = cr;
	invalidate();
}

COLORREF qs::TextWindow::getBackgroundColor() const
{
	return pImpl_->crBackground_;
}

void qs::TextWindow::setBackgroundColor(COLORREF cr)
{
	pImpl_->crBackground_ = cr;
	invalidate();
}

unsigned int qs::TextWindow::getLineSpacing() const
{
	return pImpl_->nLineSpacing_;
}

void qs::TextWindow::setLineSpacing(unsigned int nLineSpacing)
{
	pImpl_->nLineSpacing_ = nLineSpacing;
	invalidate();
}

unsigned int qs::TextWindow::getCharInLine() const
{
	return pImpl_->nCharInLine_;
}

void qs::TextWindow::setCharInLine(unsigned int nCharInLine)
{
	pImpl_->nCharInLine_ = nCharInLine;
	pImpl_->recalcLines();
}

unsigned int qs::TextWindow::getTabWidth() const
{
	return pImpl_->nTabWidth_;
}

void qs::TextWindow::setTabWidth(unsigned int nTabWidth)
{
	pImpl_->nTabWidth_ = nTabWidth;
	pImpl_->recalcLines();
}

void qs::TextWindow::getMargin(unsigned int* pnTop,
							   unsigned int* pnBottom,
							   unsigned int* pnLeft,
							   unsigned int* pnRight) const
{
	*pnTop = pImpl_->nMarginTop_;
	*pnBottom = pImpl_->nMarginBottom_;
	*pnLeft = pImpl_->nMarginLeft_;
	*pnRight = pImpl_->nMarginRight_;
}

void qs::TextWindow::setMargin(unsigned int nTop,
							   unsigned int nBottom,
							   unsigned int nLeft,
							   unsigned int nRight)
{
	pImpl_->nMarginTop_ = nTop;
	pImpl_->nMarginBottom_ = nBottom;
	pImpl_->nMarginLeft_ = nLeft;
	pImpl_->nMarginRight_ = nRight;
	pImpl_->recalcLines();
}

bool qs::TextWindow::isShowNewLine() const
{
	return pImpl_->bShowNewLine_;
}

void qs::TextWindow::setShowNewLine(bool bShowNewLine)
{
	pImpl_->bShowNewLine_ = bShowNewLine;
	invalidate();
}

bool qs::TextWindow::isShowTab() const
{
	return pImpl_->bShowTab_;
}

void qs::TextWindow::setShowTab(bool bShowTab)
{
	pImpl_->bShowTab_ = bShowTab;
	invalidate();
}

bool qs::TextWindow::isShowScrollBar(bool bHorizontal) const
{
	return bHorizontal ? pImpl_->bShowHorizontalScrollBar_ :
		pImpl_->bShowVerticalScrollBar_;
}

void qs::TextWindow::setShowScrollBar(bool bHorizontal,
									  bool bShowScrollBar)
{
	if (bHorizontal)
		pImpl_->bShowHorizontalScrollBar_ = bShowScrollBar;
	else
		pImpl_->bShowVerticalScrollBar_ = bShowScrollBar;
	pImpl_->recalcLines();
}

bool qs::TextWindow::isShowCaret() const
{
	return pImpl_->bShowCaret_;
}

void qs::TextWindow::setShowCaret(bool bShowCaret)
{
	if (pImpl_->bShowCaret_ != bShowCaret) {
		pImpl_->bShowCaret_ = bShowCaret;
		if (hasFocus()) {
			if (bShowCaret) {
				pImpl_->caret_.nLine_ = pImpl_->scrollPos_.nLine_;
				pImpl_->caret_.nChar_ = 0;
				pImpl_->caret_.nPos_ = 0;
				pImpl_->caret_.nOldPos_ = 0;
				
				pImpl_->showCaret();
				pImpl_->updateCaret(false);
			}
			else {
				pImpl_->hideCaret();
			}
		}
	}
}

const WCHAR* qs::TextWindow::getQuote(unsigned int n) const
{
	assert(n < 2);
	return pImpl_->wstrQuote_[n].get();
}

void qs::TextWindow::setQuote(unsigned int n,
							  const WCHAR* pwszQuote)
{
	assert(n < 2);
	assert(pwszQuote);
	pImpl_->wstrQuote_[n] = allocWString(pwszQuote);
	invalidate();
}

COLORREF qs::TextWindow::getQuoteColor(unsigned int n) const
{
	assert(n < 2);
	return pImpl_->crQuote_[n];
}

void qs::TextWindow::setQuoteColor(unsigned int n,
								   COLORREF cr)
{
	assert(n < 2);
	pImpl_->crQuote_[n] = cr;
	invalidate();
}

bool qs::TextWindow::isLineQuote() const
{
	return pImpl_->bLineQuote_ && !pImpl_->pTextModel_->isEditable();
}

void qs::TextWindow::setLineQuote(bool bLineQuote)
{
	pImpl_->bLineQuote_ = bLineQuote;
	pImpl_->recalcLines();
}

unsigned int qs::TextWindow::getReformLineLength() const
{
	return pImpl_->nReformLineLength_;
}

void qs::TextWindow::setReformLineLength(unsigned int nReformLineLength)
{
	pImpl_->nReformLineLength_ = nReformLineLength;
}

const WCHAR* qs::TextWindow::getReformQuote() const
{
	return pImpl_->wstrReformQuote_.get();
}

void qs::TextWindow::setReformQuote(const WCHAR* pwszReformQuote)
{
	assert(pwszReformQuote);
	pImpl_->wstrReformQuote_ = allocWString(pwszReformQuote);
}

void qs::TextWindow::getWindowClass(WNDCLASS* pwc)
{
	DefaultWindowHandler::getWindowClass(pwc);
	
#if !defined _WIN32_WCE || _WIN32_WCE >= 211
	pwc->hCursor = ::LoadCursor(0, IDC_IBEAM);
#endif // _WIN32_WCE
}

bool qs::TextWindow::preCreateWindow(CREATESTRUCT* pCreateStruct)
{
	if (!DefaultWindowHandler::preCreateWindow(pCreateStruct))
		return false;
	
	if (pImpl_->bShowVerticalScrollBar_)
		pCreateStruct->style |= WS_VSCROLL;
	if (pImpl_->bShowHorizontalScrollBar_)
		pCreateStruct->style |= WS_HSCROLL;
	
	return true;
}

LRESULT qs::TextWindow::windowProc(UINT uMsg,
								   WPARAM wParam,
								   LPARAM lParam)
{
	BEGIN_MESSAGE_HANDLER()
		HANDLE_CHAR()
		HANDLE_COPY()
		HANDLE_CREATE()
		HANDLE_CUT()
		HANDLE_DESTROY()
		HANDLE_ERASEBKGND()
		HANDLE_HSCROLL()
		HANDLE_IME_CHAR()
		HANDLE_IME_COMPOSITION()
		HANDLE_IME_ENDCOMPOSITION()
		HANDLE_IME_STARTCOMPOSITION()
		HANDLE_KEYDOWN()
		HANDLE_KILLFOCUS()
		HANDLE_LBUTTONDBLCLK()
		HANDLE_LBUTTONDOWN()
		HANDLE_LBUTTONUP()
		HANDLE_MOUSEMOVE()
		HANDLE_MOUSEWHEEL()
#ifndef _WIN32_WCE
		HANDLE_NCPAINT()
#endif
		HANDLE_PAINT()
		HANDLE_PASTE()
		HANDLE_SETCURSOR()
		HANDLE_SETFOCUS()
		HANDLE_SIZE()
#ifndef _WIN32_WCE
		HANDLE_THEMECHANGED()
#endif
		HANDLE_TIMER()
		HANDLE_VSCROLL();
	END_MESSAGE_HANDLER()
	return DefaultWindowHandler::windowProc(uMsg, wParam, lParam);
}

LRESULT qs::TextWindow::onChar(UINT nChar,
							   UINT nRepeat,
							   UINT nFlags)
{
	WCHAR c = nChar;
	
#ifndef UNICODE
	if (c > 0x7f) {
		wstring_ptr wstr(mbs2wcs(reinterpret_cast<char*>(&nChar), 1));
		c = *wstr.get();
	}
#endif
	
	if (c == L'\r')
		c = L'\n';
	
	if (c == L'\t' || c == L'\n' || c == L' ' || (c >= 0x20 && c != 0x7f))
		pImpl_->insertText(&c, 1, TextWindowImpl::INSERTTEXTFLAG_NORMAL);
	
	return 0;
}

LRESULT qs::TextWindow::onCopy()
{
	copy();
	return 0;
}

LRESULT qs::TextWindow::onCreate(CREATESTRUCT* pCreateStruct)
{
	assert(pImpl_->pTextModel_);
	
	if (DefaultWindowHandler::onCreate(pCreateStruct) == -1)
		return -1;
	
#ifndef _WIN32_WCE
	pImpl_->pTheme_.reset(new Theme(getHandle(), L"Edit"));
#endif
	
	pImpl_->pRuler_ = new TextWindowRuler(pImpl_);
	if (!pImpl_->pRuler_->create(L"QsTextWindowRuler",
		0, WS_CHILD, 0, 0, 0, 0, getHandle(), 0, 0,
		TextWindowImpl::ID_RULER, 0))
		return -1;
	if (pImpl_->bShowRuler_)
		pImpl_->pRuler_->showWindow(SW_SHOW);
	
	pImpl_->pTextModel_->addTextModelHandler(pImpl_);
	pImpl_->hImc_ = ::ImmGetContext(getHandle());
	
#ifdef _WIN32_WCE
	Registry reg(HKEY_LOCAL_MACHINE,
		L"System\\CurrentControlSet\\Control\\Layouts\\e0010411");
	if (reg) {
		wstring_ptr wstrIme;
		if (reg.getValue(L"Ime File", &wstrIme))
			pImpl_->bAtok_ = wstrIme.get() && wcsstr(wstrIme.get(), L"atok");
	}
#endif
	
	if (!pImpl_->updateBitmaps())
		return -1;
	
	pImpl_->updateCaret(false);
	pImpl_->updateScrollBar();
	
	return 0;
}

LRESULT qs::TextWindow::onCut()
{
	cut();
	return 0;
}

LRESULT qs::TextWindow::onDestroy()
{
	::ImmReleaseContext(getHandle(), pImpl_->hImc_);
	
	::DeleteObject(pImpl_->hfont_);
	pImpl_->hfont_ = 0;
	
	::DeleteObject(pImpl_->hbmNewLine_);
	pImpl_->hbmNewLine_ = 0;
	
	::DeleteObject(pImpl_->hbmTab_);
	pImpl_->hbmTab_ = 0;
	
	pImpl_->pTextModel_->removeTextModelHandler(pImpl_);
	
#ifndef _WIN32_WCE
	pImpl_->pTheme_.reset(0);
#endif
	
	return DefaultWindowHandler::onDestroy();
}

LRESULT qs::TextWindow::onEraseBkgnd(HDC hdc)
{
	return 1;
}

LRESULT qs::TextWindow::onHScroll(UINT nCode,
								  UINT nPos,
								  HWND hwnd)
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

LRESULT qs::TextWindow::onImeChar(UINT nChar,
								  UINT nRepeat,
								  UINT nFlags)
{
	return 0;
}

LRESULT qs::TextWindow::onImeComposition(UINT nChar,
										 UINT nFlags)
{
	if (pImpl_->pTextModel_->isEditable() && nFlags & GCS_RESULTSTR) {
		int nLen = ::ImmGetCompositionString(pImpl_->hImc_, GCS_RESULTSTR, 0, 0);
		tstring_ptr tstr(allocTString(nLen/sizeof(TCHAR) + 1));
		::ImmGetCompositionString(pImpl_->hImc_, GCS_RESULTSTR, tstr.get(), nLen);
		*(tstr.get() + nLen/sizeof(TCHAR)) = _T('\0');
		T2W(tstr.get(), pwsz);
		pImpl_->insertText(pwsz, -1, TextWindowImpl::INSERTTEXTFLAG_NORMAL);
		return 0;
	}
	
	return DefaultWindowHandler::onImeComposition(nChar, nFlags);
}

LRESULT qs::TextWindow::onImeEndComposition()
{
	return DefaultWindowHandler::onImeEndComposition();
}

LRESULT qs::TextWindow::onImeStartComposition()
{
	if (pImpl_->pTextModel_->isEditable()) {
		LOGFONT lf;
		::GetObject(pImpl_->hfont_, sizeof(lf), &lf);
		::ImmSetCompositionFont(pImpl_->hImc_, &lf);
	}
	return DefaultWindowHandler::onImeStartComposition();
}

LRESULT qs::TextWindow::onKeyDown(UINT nKey,
								  UINT nRepeat,
								  UINT nFlags)
{
	if (nKey == VK_BACK) {
		DeleteTextFlag flag = DELETETEXTFLAG_DELETEBACKWARDCHAR;
		if (::GetKeyState(VK_CONTROL) < 0 &&
			(!pImpl_->bAtok_ || !::ImmGetOpenStatus(pImpl_->hImc_)))
			flag = DELETETEXTFLAG_DELETEBACKWARDWORD;
		pImpl_->deleteText(flag);
		return 0;
	}
	else if (nKey == VK_DELETE) {
		pImpl_->deleteText(::GetKeyState(VK_CONTROL) < 0 ?
			DELETETEXTFLAG_DELETEWORD : DELETETEXTFLAG_DELETECHAR);
		return 0;
	}
	
	if (pImpl_->bShowCaret_) {
		bool bMoveCaret = true;
		MoveCaret mc;
		switch (nKey) {
		case VK_LEFT:
			mc = ::GetKeyState(VK_CONTROL) < 0 ?
				MOVECARET_WORDLEFT : MOVECARET_CHARLEFT;
			break;
		case VK_RIGHT:
			mc = ::GetKeyState(VK_CONTROL) < 0 ?
				MOVECARET_WORDRIGHT : MOVECARET_CHARRIGHT;
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
			bMoveCaret= false;
			break;
		}
		if (bMoveCaret)
			moveCaret(mc, 0, 0, (nFlags & 0x40000000) != 0,
				::GetKeyState(VK_SHIFT) < 0 ? SELECT_SELECT : SELECT_CLEAR, true);
	}
	else {
		bool bScroll = true;
		Scroll s;
		switch (nKey) {
		case VK_LEFT:
			if (pImpl_->bHorizontalScrollable_)
				s = SCROLL_CHARLEFT;
			else
				s = SCROLL_PAGEUP;
			break;
		case VK_RIGHT:
			if (pImpl_->bHorizontalScrollable_)
				s = SCROLL_CHARRIGHT;
			else
				s = SCROLL_PAGEDOWN;
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
			bScroll = false;
			break;
		}
		if (bScroll)
			scroll(s, 0, (nFlags & 0x40000000) != 0);
	}
	
	return DefaultWindowHandler::onKeyDown(nKey, nRepeat, nFlags);
}

LRESULT qs::TextWindow::onKillFocus(HWND hwnd)
{
	if (pImpl_->bShowCaret_)
		pImpl_->hideCaret();
	return DefaultWindowHandler::onKillFocus(hwnd);
}

LRESULT qs::TextWindow::onLButtonDblClk(UINT nFlags,
										const POINT& pt)
{
	if (pImpl_->pTextModel_->isEditable())
		pImpl_->openLink(pt);
	return DefaultWindowHandler::onLButtonDblClk(nFlags, pt);
}

LRESULT qs::TextWindow::onLButtonDown(UINT nFlags,
									  const POINT& pt)
{
	setFocus();
	
	pImpl_->startSelection(pt, false);
	pImpl_->ptLastButtonDown_ = pt;
	setCapture();
	
	pImpl_->nTimerDragScroll_ = setTimer(TextWindowImpl::TIMER_DRAGSCROLL,
		TextWindowImpl::DRAGSCROLL_DELAY);
	
	return DefaultWindowHandler::onLButtonDown(nFlags, pt);
}

LRESULT qs::TextWindow::onLButtonUp(UINT nFlags,
									const POINT& pt)
{
	if (getCapture()) {
		killTimer(pImpl_->nTimerDragScroll_);
		pImpl_->nTimerDragScroll_ = 0;
		
		releaseCapture();
		
		pImpl_->updateSelection(pt, false);
		
		if (!pImpl_->pTextModel_->isEditable()) {
			int cx = ::GetSystemMetrics(SM_CXDOUBLECLK);
			int cy = ::GetSystemMetrics(SM_CYDOUBLECLK);
			if (pImpl_->ptLastButtonDown_.x - cx/2 <= pt.x &&
				pt.x <= pImpl_->ptLastButtonDown_.x + cx/2 &&
				pImpl_->ptLastButtonDown_.y - cy/2 <= pt.y &&
				pt.y <= pImpl_->ptLastButtonDown_.y + cy/2)
				pImpl_->openLink(pt);
		}
		
		pImpl_->ptLastButtonDown_.x = -1;
		pImpl_->ptLastButtonDown_.y = -1;
	}
	
	return DefaultWindowHandler::onLButtonUp(nFlags, pt);
}

LRESULT qs::TextWindow::onMouseMove(UINT nFlags,
									const POINT& pt)
{
	if (getCapture()) {
		if (pt.x != pImpl_->ptLastButtonDown_.x ||
			pt.y != pImpl_->ptLastButtonDown_.y)
			pImpl_->updateSelection(pt, true);
	}
	
	return DefaultWindowHandler::onMouseMove(nFlags, pt);
}

#if !defined _WIN32_WCE || _WIN32_WCE >= 211
LRESULT qs::TextWindow::onMouseWheel(UINT nFlags,
									 short nDelta,
									 const POINT& pt)
{
#ifdef _WIN32_WCE
#	define WHEEL_DELTA 120
#endif
	scroll(SCROLL_VERTICALPOS, pImpl_->scrollPos_.nLine_ - nDelta*3/WHEEL_DELTA, false);
	
	return DefaultWindowHandler::onMouseWheel(nFlags, nDelta, pt);
}
#endif

#ifndef _WIN32_WCE
LRESULT qs::TextWindow::onNcPaint(HRGN hrgn)
{
	DefaultWindowHandler::onNcPaint(hrgn);
	
	if (getWindowLong(GWL_EXSTYLE) & WS_EX_CLIENTEDGE && pImpl_->pTheme_->isActive())
		UIUtil::drawThemeBorder(pImpl_->pTheme_.get(), getHandle(), EP_EDITTEXT, 0, pImpl_->crBackground_);
	
	return 0;
}
#endif

LRESULT qs::TextWindow::onPaint()
{
	PaintDeviceContext dc(getHandle());
	
	GdiObject<HPEN> hpenLink(::CreatePen(PS_SOLID, 1, pImpl_->crLink_));
	ObjectSelector<HPEN> penSelector(dc, hpenLink.get());
	GdiObject<HPEN> hpenQuote(::CreatePen(PS_SOLID, 2, pImpl_->crQuote_[0]));
	
	ObjectSelector<HFONT> fontSelector(dc, pImpl_->hfont_);
	ObjectSelector<HBRUSH> brushSelector(dc,
		static_cast<HBRUSH>(::GetStockObject(WHITE_BRUSH)));
//	dc.setTextColor(pImpl_->crForeground_);
	dc.setBkColor(pImpl_->crBackground_);
	
	CompatibleDeviceContext dcNewLine(dc);
	ObjectSelector<HBITMAP> bitmapSelectorNewLine(
		dcNewLine, pImpl_->hbmNewLine_);
	
	CompatibleDeviceContext dcTab(dc);
	ObjectSelector<HBITMAP> bitmapSelectorTab(
		dcTab, pImpl_->hbmTab_);
	
	RECT rectClip;
	dc.getClipBox(&rectClip);
	
	RECT rect;
	getClientRect(&rect);
	rect.left += pImpl_->nMarginLeft_;
	rect.right -= pImpl_->nMarginRight_;
	int nBottom = rect.bottom;
	
	unsigned int nAverageCharWidth = pImpl_->getAverageCharWidth();
	unsigned int nLineHeight = pImpl_->getLineHeight();
	unsigned int nLineInWindow = pImpl_->getLineInWindow();
	
	SIZE sizeTab = { nAverageCharWidth, nLineHeight };
	POINT ptLink[2];
	unsigned int nQuoteWidth = pImpl_->getQuoteWidth();
	
	rect.bottom = pImpl_->nMarginTop_;
	dc.fillSolidRect(rect, pImpl_->crBackground_);
	
	unsigned int nStartLine = pImpl_->scrollPos_.nLine_;
	unsigned int nEndLine = QSMIN(pImpl_->listLine_.size(),
		nStartLine + nLineInWindow + 2);
	
	POINT pt = {
		pImpl_->nMarginLeft_ - pImpl_->scrollPos_.nPos_,
		pImpl_->nMarginTop_
	};
	unsigned int n = nStartLine;
	while (n < nEndLine) {
		rect.top = pt.y;
		rect.bottom = rect.top + nLineHeight;
		RECT rectIntersect;
		if (::IntersectRect(&rectIntersect, &rect, &rectClip)) {
			const TextWindowImpl::PhysicalLine* pPhysicalLine =
				pImpl_->listLine_[n];
			const TextModel::Line& logicalLine =
				pImpl_->pTextModel_->getLine(pPhysicalLine->nLogicalLine_);
			
			COLORREF cr = pPhysicalLine->cr_;
			
			const WCHAR* pBegin = logicalLine.getText() + pPhysicalLine->nPosition_;
			const WCHAR* pEnd = pBegin + pPhysicalLine->nLength_;
			bool bNewLine = pBegin != pEnd && *(pEnd - 1) == L'\n';
			if (bNewLine)
				--pEnd;
			
			unsigned int nAllQuoteWidth = pPhysicalLine->nQuoteDepth_*nQuoteWidth;
			if (nAllQuoteWidth != 0) {
				RECT r = {
					pt.x,
					rect.top,
					pt.x + nAllQuoteWidth,
					rect.bottom
				};
				dc.fillSolidRect(r, pImpl_->crBackground_);
				
				ObjectSelector<HPEN> penSelector(dc, hpenQuote.get());
				for (size_t n = 0; n < pPhysicalLine->nQuoteDepth_; ++n) {
					POINT ptQuote[] = {
						{ pt.x + nQuoteWidth*n + nQuoteWidth/2,	rect.top },
						{ pt.x + nQuoteWidth*n + nQuoteWidth/2,	rect.bottom }
					};
					dc.polyline(ptQuote, countof(ptQuote));
				}
			}
			
			unsigned int x = nAllQuoteWidth;
			if (pBegin == pEnd) {
				RECT r = {
					pt.x + x,
					rect.top,
					rect.right,
					rect.bottom
				};
				dc.fillSolidRect(r, pImpl_->crBackground_);
			}
			else {
				size_t nLinkCount = pPhysicalLine->items_.nCount_;
				if (nLinkCount == 0) {
					dc.setTextColor(cr);
					x = pImpl_->paintBlock(&dc, pt, rect,
						x, pBegin, pEnd, &dcTab, sizeTab);
				}
				else {
					ptLink[0].y = rect.bottom - 1;
					ptLink[1].y = rect.bottom - 1;
					
					size_t nOffset = 0;
					for (size_t n = 0; n < nLinkCount; ++n) {
						const TextWindowImpl::LinkItem& item =
							pPhysicalLine->items_.items_[n];
						
						dc.setTextColor(cr);
						x = pImpl_->paintBlock(&dc, pt, rect, x, pBegin + nOffset,
							pBegin + item.nOffset_, &dcTab, sizeTab);
						ptLink[0].x = pt.x + x;
						
						dc.setTextColor(pImpl_->crLink_);
						x = pImpl_->paintBlock(&dc, pt,
							rect, x, pBegin + item.nOffset_,
							pBegin + item.nOffset_ + item.nLength_,
							&dcTab, sizeTab);
						ptLink[1].x = pt.x + x;
						
						dc.polyline(ptLink, countof(ptLink));
						
						nOffset = item.nOffset_ + item.nLength_;
					}
					dc.setTextColor(cr);
					x = pImpl_->paintBlock(&dc, pt, rect,
						x, pBegin + nOffset, pEnd, &dcTab, sizeTab);
				}
				RECT r = {
					pt.x + x,
					rect.top,
					rect.right,
					rect.bottom
				};
				dc.extTextOut(pt.x + x, pt.y + pImpl_->nLineSpacing_,
					ETO_CLIPPED | ETO_OPAQUE, r, L"", 0, 0);
			}
			
			if (pImpl_->bShowNewLine_ && bNewLine)
				dc.bitBlt(pt.x + x, pt.y, nAverageCharWidth,
					nLineHeight, dcNewLine, 0, 0, SRCCOPY);
			
			std::pair<unsigned int, unsigned int> s = pImpl_->getSelection(n);
			if (s.first != s.second) {
				assert(s.first < s.second);
				int nStartPos = pImpl_->getPosFromChar(n, s.first);
				int nEndPos = pImpl_->getPosFromChar(n, s.second);
				dc.patBlt(nStartPos + pImpl_->nMarginLeft_ - pImpl_->scrollPos_.nPos_,
					rect.top, nEndPos - nStartPos, rect.bottom - rect.top, PATINVERT);
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

LRESULT qs::TextWindow::onPaste()
{
	paste();
	return 0;
}

LRESULT qs::TextWindow::onSetCursor(HWND hwnd,
									UINT nHitTest,
									UINT nMessage)
{
	if (nHitTest == HTCLIENT) {
		POINT pt;
		::GetCursorPos(&pt);
		screenToClient(&pt);
		
		std::pair<int, const TextWindowImpl::LinkItem*> item(
			pImpl_->getLinkItemFromPoint(pt));
		if (item.second) {
			::SetCursor(pImpl_->hCursorLink_);
			return 0;
		}
	}
	return DefaultWindowHandler::onSetCursor(hwnd, nHitTest, nMessage);
}

LRESULT qs::TextWindow::onSetFocus(HWND hwnd)
{
	if (pImpl_->bShowCaret_) {
		pImpl_->showCaret();
		pImpl_->updateCaret(false);
	}
	
	return DefaultWindowHandler::onSetFocus(hwnd);
}

LRESULT qs::TextWindow::onSize(UINT nFlags,
							   int cx,
							   int cy)
{
	if (pImpl_->bShowRuler_) {
		unsigned int nCharInLine = getCharInLine();
		int nWidth = 0;
		if (nCharInLine == 0)
			nWidth = cx;
		else
			nWidth = pImpl_->nMarginLeft_ +
				pImpl_->getAverageCharWidth()*nCharInLine;
		pImpl_->pRuler_->setWindowPos(0, 0, 0, nWidth,
			TextWindowImpl::RULER_HEIGHT, SWP_NOZORDER);
	}
	
	pImpl_->nLineInWindow_ = 0;
	
	if (pImpl_->nCharInLine_ == 0 && cx != pImpl_->nLastWindowWidth_)
		pImpl_->recalcLines();
	pImpl_->nLastWindowWidth_ = cx;
	
	pImpl_->updateScrollBar();
	invalidate();
	
	return DefaultWindowHandler::onSize(nFlags, cx, cy);
}

#ifndef _WIN32_WCE
LRESULT qs::TextWindow::onThemeChanged()
{
#ifndef _WIN32_WCE
	pImpl_->pTheme_.reset(new Theme(getHandle(), L"Edit"));
#endif
	return 0;
}
#endif

LRESULT qs::TextWindow::onTimer(UINT nId)
{
	if (nId == pImpl_->nTimerDragScroll_) {
		killTimer(pImpl_->nTimerDragScroll_);
		pImpl_->nTimerDragScroll_ = 0;
		
		POINT pt;
#ifdef _WIN32_WCE
		DWORD dwPos = ::GetMessagePos();
		pt.x = static_cast<int>(dwPos & 0x0000ffff);
		pt.y = static_cast<int>(dwPos & 0xffff0000) >> 16;
#else
		::GetCursorPos(&pt);
		screenToClient(&pt);
#endif
		
		RECT rect;
		pImpl_->getClientRectWithoutMargin(&rect);
		
		bool bScroll = false;
		if (pt.y < TextWindowImpl::DRAGSCROLL_BORDER) {
			scroll(SCROLL_LINEUP, 0, pt.y < 0);
			bScroll = true;
		}
		else if (pt.y > rect.bottom - TextWindowImpl::DRAGSCROLL_BORDER) {
			scroll(SCROLL_LINEDOWN, 0, pt.y > rect.bottom);
			bScroll = true;
		}
		if (pt.x < TextWindowImpl::DRAGSCROLL_BORDER) {
			scroll(SCROLL_CHARLEFT, 0, pt.x < 0);
			bScroll = true;
		}
		else if (pt.x > rect.right - TextWindowImpl::DRAGSCROLL_BORDER) {
			scroll(SCROLL_CHARRIGHT, 0, pt.x > rect.right);
			bScroll = true;
		}
		
		if (bScroll)
			pImpl_->updateSelection(pt, true);
		
		pImpl_->nTimerDragScroll_ = setTimer(TextWindowImpl::TIMER_DRAGSCROLL,
			TextWindowImpl::DRAGSCROLL_INTERVAL);
		
		return 0;
	}
	
	return DefaultWindowHandler::onTimer(nId);
}

LRESULT qs::TextWindow::onVScroll(UINT nCode,
								  UINT nPos,
								  HWND hwnd)
{
	bool bScroll = true;
	Scroll s;
	switch (nCode) {
	case SB_TOP:
		s = SCROLL_TOP;
		break;
	case SB_BOTTOM:
		s = SCROLL_BOTTOM;
		break;
	case SB_LINEUP:
		s = SCROLL_LINEUP;
		break;
	case SB_LINEDOWN:
		s = SCROLL_LINEDOWN;
		break;
	case SB_PAGEUP:
		s = SCROLL_PAGEUP;
		break;
	case SB_PAGEDOWN:
		s = SCROLL_PAGEDOWN;
		break;
	case SB_THUMBPOSITION:
	case SB_THUMBTRACK:
		s = SCROLL_VERTICALPOS;
		break;
	default:
		bScroll = false;
		break;
	}
	if (bScroll)
		scroll(s, nPos, false);
	
	return DefaultWindowHandler::onVScroll(nCode, nPos, hwnd);
}


/****************************************************************************
 *
 * TextWindowLinkHandler
 *
 */

qs::TextWindowLinkHandler::~TextWindowLinkHandler()
{
}


/****************************************************************************
 *
 * TextWindowUndoManager
 *
 */

qs::TextWindowUndoManager::TextWindowUndoManager()
{
}

qs::TextWindowUndoManager::~TextWindowUndoManager()
{
	clear();
}

void qs::TextWindowUndoManager::pushUndoItem(unsigned int nStartLine,
											 unsigned int nStartChar,
											 unsigned int nEndLine,
											 unsigned int nEndChar,
											 unsigned int nCaretLine,
											 unsigned int nCaretChar,
											 wstring_ptr wstrText,
											 bool bClearRedo)
{
	std::auto_ptr<Item> pItem(new Item(nStartLine, nStartChar,
		nEndLine, nEndChar, nCaretLine, nCaretChar, wstrText));
	listUndo_.push_back(pItem.get());
	pItem.release();
	
	if (bClearRedo)
		clearRedoItems();
}

TextWindowUndoManager::Item* qs::TextWindowUndoManager::popUndoItem()
{
	assert(!listUndo_.empty());
	Item* pItem = listUndo_.back();
	listUndo_.pop_back();
	return pItem;
}

bool qs::TextWindowUndoManager::hasUndoItem() const
{
	return !listUndo_.empty();
}

void qs::TextWindowUndoManager::pushRedoItem(unsigned int nStartLine,
											 unsigned int nStartChar,
											 unsigned int nEndLine,
											 unsigned int nEndChar,
											 unsigned int nCaretLine,
											 unsigned int nCaretChar,
											 wstring_ptr wstrText)
{
	std::auto_ptr<Item> pItem(new Item(nStartLine, nStartChar,
		nEndLine, nEndChar, nCaretLine, nCaretChar, wstrText));
	listRedo_.push_back(pItem.get());
	pItem.release();
}

TextWindowUndoManager::Item* qs::TextWindowUndoManager::popRedoItem()
{
	assert(!listRedo_.empty());
	Item* pItem = listRedo_.back();
	listRedo_.pop_back();
	return pItem;
}

bool qs::TextWindowUndoManager::hasRedoItem() const
{
	return !listRedo_.empty();
}

void qs::TextWindowUndoManager::clearRedoItems()
{
	std::for_each(listRedo_.begin(), listRedo_.end(), deleter<Item>());
	listRedo_.clear();
}

void qs::TextWindowUndoManager::clear()
{
	ItemList* pLists[] = {
		&listUndo_,
		&listRedo_
	};
	for (int n = 0; n < countof(pLists); ++n) {
		std::for_each(pLists[n]->begin(), pLists[n]->end(), deleter<Item>());
		pLists[n]->clear();
	}
}


/****************************************************************************
 *
 * TextWindowUndoManager::Item
 *
 */

qs::TextWindowUndoManager::Item::Item(unsigned int nStartLine,
									  unsigned int nStartChar,
									  unsigned int nEndLine,
									  unsigned int nEndChar,
									  unsigned int nCaretLine,
									  unsigned int nCaretChar,
									  wstring_ptr wstrText) :
	nStartLine_(nStartLine),
	nStartChar_(nStartChar),
	nEndLine_(nEndLine),
	nEndChar_(nEndChar),
	nCaretLine_(nCaretLine),
	nCaretChar_(nCaretChar),
	wstrText_(wstrText)
{
}

qs::TextWindowUndoManager::Item::~Item()
{
}

unsigned int qs::TextWindowUndoManager::Item::getStartLine() const
{
	return nStartLine_;
}

unsigned int qs::TextWindowUndoManager::Item::getStartChar() const
{
	return nStartChar_;
}

unsigned int qs::TextWindowUndoManager::Item::getEndLine() const
{
	return nEndLine_;
}

unsigned int qs::TextWindowUndoManager::Item::getEndChar() const
{
	return nEndChar_;
}

unsigned int qs::TextWindowUndoManager::Item::getCaretLine() const
{
	return nCaretLine_;
}

unsigned int qs::TextWindowUndoManager::Item::getCaretChar() const
{
	return nCaretChar_;
}

const WCHAR* qs::TextWindowUndoManager::Item::getText() const
{
	return wstrText_.get() ? wstrText_.get() : L"";
}


/****************************************************************************
 *
 * TextWindowRuler
 *
 */

qs::TextWindowRuler::TextWindowRuler(TextWindowImpl* pImpl) :
	WindowBase(true),
	pImpl_(pImpl),
	nPos_(-1)
{
	setWindowHandler(this, false);
}

qs::TextWindowRuler::~TextWindowRuler()
{
}

void qs::TextWindowRuler::update()
{
	RECT rect;
	getClientRect(&rect);
	int nRulerHeight = rect.bottom - rect.top;
	
	int nCharWidth = pImpl_->getAverageCharWidth();
	
	if (nPos_ != -1) {
		RECT r = {
			nPos_,
			0,
			nPos_ + nCharWidth + 1,
			nRulerHeight
		};
		invalidateRect(r);
	}
	
	nPos_ = pImpl_->nMarginLeft_ + pImpl_->caret_.nPos_;
	
	RECT r = {
		nPos_,
		0,
		nPos_ + nCharWidth + 1,
		nRulerHeight
	};
	invalidateRect(r);
}

LRESULT qs::TextWindowRuler::windowProc(UINT uMsg,
										WPARAM wParam,
										LPARAM lParam)
{
	BEGIN_MESSAGE_HANDLER()
		HANDLE_PAINT()
	END_MESSAGE_HANDLER()
	return DefaultWindowHandler::windowProc(uMsg, wParam, lParam);
}

LRESULT qs::TextWindowRuler::onEraseBkgnd(HDC hdc)
{
	return 1;
}

LRESULT qs::TextWindowRuler::onPaint()
{
	PaintDeviceContext dc(getHandle());
	if (!dc)
		return 1;
	
	RECT rect;
	getClientRect(&rect);
	
	dc.fillSolidRect(rect, pImpl_->crBackground_);
	
	int nRulerHeight = rect.bottom - rect.top;
	
	int nCharWidth = pImpl_->getAverageCharWidth();
	int nCharInLine = pImpl_->nCharInLine_;
	if (nCharInLine == 0)
		nCharInLine = 1000;
	
	GdiObject<HPEN> hpen(::CreatePen(PS_SOLID, 0, pImpl_->crForeground_));
	ObjectSelector<HPEN> penSelector(dc, hpen.get());
	int nPos = pImpl_->nMarginLeft_;
	for (int n = 0; n < nCharInLine && nPos < rect.right; ++n) {
		int nHeight = 3;
		if (n % 10 == 0)
			nHeight = nRulerHeight;
		else if (n % 5 == 0)
			nHeight = nRulerHeight - 3;
		POINT pt[2];
		pt[0].x = nPos;
		pt[0].y = nRulerHeight - nHeight;
		pt[1].x = nPos;
		pt[1].y = nRulerHeight;
		nPos += nCharWidth;
		dc.polyline(pt, sizeof(pt)/sizeof(pt[0]));
	}
	POINT pt[2];
	pt[0].x = pImpl_->nMarginLeft_;
	pt[0].y = nRulerHeight - 1;
	pt[1].x = nPos;
	pt[1].y = nRulerHeight - 1;
	dc.polyline(pt, sizeof(pt)/sizeof(pt[0]));
	
	RECT rectPos = {
		pImpl_->nMarginLeft_ + pImpl_->caret_.nPos_,
		0,
		pImpl_->nMarginLeft_ + pImpl_->caret_.nPos_ + nCharWidth + 1,
		nRulerHeight
	};
	dc.fillSolidRect(rectPos, pImpl_->crForeground_);
	
	return 0;
}
