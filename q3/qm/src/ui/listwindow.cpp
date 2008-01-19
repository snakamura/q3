/*
 * $Id$
 *
 * Copyright(C) 1998-2008 Satoshi Nakamura
 *
 */

#pragma warning(disable:4786)

#include <qmaccount.h>
#include <qmfilenames.h>
#include <qmfolder.h>
#include <qmlistwindow.h>
#include <qmmacro.h>
#include <qmmessageholder.h>

#include <qsaccelerator.h>
#include <qsconv.h>
#include <qsdevicecontext.h>
#include <qsdragdrop.h>
#include <qsmenu.h>
#include <qsosutil.h>
#include <qsprofile.h>
#include <qsstl.h>
#include <qsstream.h>
#include <qsuiutil.h>

#include <algorithm>
#include <vector>

#ifndef _WIN32_WCE
#	include <tmschema.h>
#endif

#include "listwindow.h"
#include "messageframewindow.h"
#include "resourceinc.h"
#include "syncdialog.h"
#include "syncutil.h"
#include "uimanager.h"
#include "../action/action.h"
#include "../main/main.h"
#include "../model/dataobject.h"
#include "../uimodel/viewmodel.h"

using namespace qm;
using namespace qs;


/****************************************************************************
 *
 * ListWindowImpl
 *
 */

struct qm::ListWindowImpl :
	public NotifyHandler,
	public ViewModelManagerHandler,
	public ViewModelHandler,
	public DragGestureHandler,
	public DragSourceHandler,
	public DropTargetHandler
{
public:
	enum {
		ID_HEADERCOLUMN		= 1000,
#ifdef QMTOOLTIP
		ID_TOOLTIP			= 1001
#endif
	};
	enum {
		LINE_SPACING		= 4,
		HORIZONTAL_BORDER	= 2,
		INDENT				= 15
	};
	enum {
		WM_VIEWMODEL_ITEMADDED		= WM_APP + 1101,
		WM_VIEWMODEL_ITEMREMOVED	= WM_APP + 1102,
		WM_VIEWMODEL_ITEMCHANGED	= WM_APP + 1103
	};

public:
	bool createHeaderColumn();
	void layoutChildren();
	void layoutChildren(int cx, int cy);
	
	void getRect(RECT* pRect);
	void paintMessage(const PaintInfo& pi);
	void invalidateLine(unsigned int nLine);
	void invalidateSelected();
	void ensureVisible(unsigned int nLine);
	void scrollVertical(int nPos);
	void scrollHorizontal(int nPos);
	void updateScrollBar(bool bVertical);
	unsigned int getLineFromPoint(const POINT& pt) const;
	unsigned int getColumnFromPoint(const POINT& pt) const;
	void getRect(unsigned int nLine,
				 unsigned int nColumn,
				 RECT* pRect);
	void reloadProfiles(bool bInitialize);
	void updateLineHeight();
	COLORREF getItemForeground(const ViewModelItem* pItem) const;
	COLORREF getItemBackground(const ViewModelItem* pItem) const;
	COLORREF getColor(int nIndex) const;
#ifdef QMTOOLTIP
	void relayToolTipEvent(UINT uMsg,
						   WPARAM wParam,
						   LPARAM lParam);
	void hideToolTip();
#endif
	
	void postRefreshMessage(UINT uMsg,
							unsigned int nItem);
	void handleRefreshMessage();
	void postInvalidateMessage(UINT uMsg,
							   unsigned int nItem);
	void handleInvalidateMessage(unsigned int nItem);

public:
	virtual LRESULT onNotify(NMHDR* pnmhdr,
							 bool* pbHandled);

public:
	virtual void viewModelSelected(const ViewModelManagerEvent& event);

public:
	virtual void itemAdded(const ViewModelEvent& event);
	virtual void itemRemoved(const ViewModelEvent& event);
	virtual void itemChanged(const ViewModelEvent& event);
	virtual void itemStateChanged(const ViewModelEvent& event);
	virtual void itemAttentionPaid(const ViewModelEvent& event);
	virtual void updated(const ViewModelEvent& event);
	virtual void sorted(const ViewModelEvent& event);
	virtual void colorChanged(const ViewModelEvent& event);
	virtual void columnChanged(const ViewModelEvent& event);
	virtual void destroyed(const ViewModelEvent& event);

public:
	virtual void dragGestureRecognized(const DragGestureEvent& event);

public:
	virtual void dragDropEnd(const DragSourceDropEvent& event);

public:
	virtual void dragEnter(const DropTargetDragEvent& event);
	virtual void dragOver(const DropTargetDragEvent& event);
	virtual void dragExit(const DropTargetEvent& event);
	virtual void drop(const DropTargetDropEvent& event);

private:
#ifdef QMTOOLTIP
	LRESULT onShow(NMHDR* pnmhdr,
				   bool* pbHandled);
#endif

private:
	HIMAGELIST createDragImage(const POINT& ptCursor,
							   POINT* pptHotspot);

private:
	static int getMessageImage(MessageHolder* pmh,
							   unsigned int nFlags);

public:
	ListWindow* pThis_;
	
	Profile* pProfile_;
	MessageFrameWindowManager* pMessageFrameWindowManager_;
	MenuManager* pMenuManager_;
	std::auto_ptr<Accelerator> pAccelerator_;
	AccountManager* pAccountManager_;
	const URIResolver* pURIResolver_;
	ViewModelManager* pViewModelManager_;
	SyncManager* pSyncManager_;
	SyncDialogManager* pSyncDialogManager_;
	
	HFONT hfont_;
	HFONT hfontBold_;
	bool bUseSystemColor_;
	COLORREF crForeground_;
	COLORREF crBackground_;
	COLORREF crSelectedForeground_;
	COLORREF crSelectedBackground_;
	COLORREF crSelectedNotFocusBackground_;
	bool bSingleClickOpen_;
	bool bEllipsis_;
	int nLineHeight_;
	ListHeaderColumn* pHeaderColumn_;
	HIMAGELIST hImageList_;
	HIMAGELIST hImageListData_;
	HPEN hpenThreadLine_;
	HPEN hpenFocusedThreadLine_;
	
#ifdef QMTOOLTIP
	HWND hwndToolTip_;
	unsigned int nToolTipLine_;
	unsigned int nToolTipColumn_;
	int nToolTipIndent_;
#endif
	
	std::auto_ptr<DragGestureRecognizer> pDragGestureRecognizer_;
	std::auto_ptr<DropTarget> pDropTarget_;
	bool bCanDrop_;
	
	volatile LONG nRefreshing_;
	volatile LONG nInvalidating_;
};

bool qm::ListWindowImpl::createHeaderColumn()
{
	std::auto_ptr<ListHeaderColumn> pHeaderColumn(
		new ListHeaderColumn(pThis_, pProfile_));
	
	RECT rect;
	pThis_->getClientRect(&rect);
	if (!pHeaderColumn->create(L"QmListHeaderColumn", 0,
		WS_CHILD, 0, 0, rect.right - rect.left, 0,
		pThis_->getHandle(), 0, 0, ID_HEADERCOLUMN, 0))
		return false;
	pHeaderColumn_ = pHeaderColumn.release();
	pHeaderColumn_->setFont(hfont_, false);
	
	return true;
}

void qm::ListWindowImpl::layoutChildren()
{
	RECT rect;
	pThis_->getClientRect(&rect);
	layoutChildren(rect.right - rect.left, rect.bottom - rect.top);
}

void qm::ListWindowImpl::layoutChildren(int cx,
										int cy)
{
	if (pHeaderColumn_)
		pHeaderColumn_->setWindowPos(0, 0, 0, cx + pThis_->getScrollPos(SB_HORZ),
			pHeaderColumn_->getPreferredHeight(cx, cy), SWP_NOMOVE | SWP_NOZORDER);
	updateScrollBar(true);
	updateScrollBar(false);
	pThis_->invalidate();
}

void qm::ListWindowImpl::getRect(RECT* pRect)
{
	assert(pRect);
	
	pThis_->getClientRect(pRect);
	pRect->top += pHeaderColumn_->getHeight();
}

void qm::ListWindowImpl::paintMessage(const PaintInfo& pi)
{
	DeviceContext* pdc = pi.getDeviceContext();
	ViewModel* pViewModel = pi.getViewModel();
	const ViewModelItem* pItem = pi.getItem();
	MessageHolder* pmh = pItem->getMessageHolder();
	const RECT& rect = pi.getRect();
	
	bool bHasFocus = pThis_->hasFocus();
	bool bSelected = (pItem->getFlags() & ViewModelItem::FLAG_SELECTED) != 0;
	
	COLORREF crBackground = getItemBackground(pItem);
	if (bSelected) {
		pdc->setTextColor(getColor(bHasFocus ? COLOR_HIGHLIGHTTEXT : COLOR_WINDOWTEXT));
		pdc->setBkColor(getColor(bHasFocus ? COLOR_HIGHLIGHT : COLOR_INACTIVEBORDER));
	}
	else {
		COLORREF crText = getItemForeground(pItem);
		pdc->setTextColor(crText);
		pdc->setBkColor(crBackground);
	}
	ObjectSelector<HFONT> fontSelector(*pdc, pItem->isBold() ? hfontBold_ : 0);
	
	RECT r = { rect.left, rect.top, rect.left, rect.bottom };
#if defined _WIN32_WCE && _WIN32_WCE >= 0x300 && defined _WIN32_WCE_PSPC
	RECT rectFocus = { 0, rect.top, 0, rect.bottom };
#else
	RECT rectFocus = { -1, rect.top, 0, rect.bottom };
#endif
	
	unsigned int nLevel = 0;
	if ((pViewModel->getSort() & ViewModel::SORT_THREAD_MASK) == ViewModel::SORT_THREAD)
		nLevel = pItem->getLevel();
	
	int nThreadLeft = -1;
	int nThreadRight = -1;
	const ViewColumnList& listColumn = pViewModel->getColumns();
	for (ViewColumnList::const_iterator it = listColumn.begin(); it != listColumn.end(); ++it) {
		ViewColumn* pColumn = *it;
		
		r.right += pColumn->getWidth();
		
		int nOffset = 0;
		if (pColumn->isFlag(ViewColumn::FLAG_INDENT) &&
			((pViewModel->getSort() & ViewModel::SORT_THREAD_MASK) == ViewModel::SORT_THREAD)) {
			nOffset = nLevel*INDENT;
			if (pColumn->isFlag(ViewColumn::FLAG_LINE)) {
				nThreadLeft = r.left;
				nThreadRight = r.right;
			}
		}
		
		ViewColumn::Type type = pColumn->getType();
		if (!pColumn->isFlag(ViewColumn::FLAG_ICON)) {
			wstring_ptr wstrText(pColumn->getText(pViewModel, pItem));
			size_t nLen = wcslen(wstrText.get());
			POINT pt = { r.left + HORIZONTAL_BORDER, r.top + LINE_SPACING/2 };
			if (pColumn->isFlag(ViewColumn::FLAG_RIGHTALIGN)) {
				SIZE size;
				pdc->getTextExtent(wstrText.get(), static_cast<int>(nLen), &size);
				int nX = r.right - size.cx - HORIZONTAL_BORDER;
				if (nX > pt.x)
					pt.x = nX;
			}
			else {
				pt.x += nOffset;
			}
			
			if (bEllipsis_)
				pdc->extTextOutEllipsis(pt.x, pt.y, r.right - pt.x - HORIZONTAL_BORDER,
					ETO_CLIPPED | ETO_OPAQUE, r, wstrText.get(), static_cast<UINT>(nLen));
			else
				pdc->extTextOut(pt.x, pt.y, ETO_CLIPPED | ETO_OPAQUE,
					r, wstrText.get(), static_cast<UINT>(nLen), 0);
			
			if (rectFocus.left == -1)
				rectFocus.left = r.left;
			rectFocus.right = r.right;
		}
		else {
			unsigned int nValue = pColumn->getNumber(pViewModel, pItem);
			
#if defined _WIN32_WCE && _WIN32_WCE >= 0x300 && defined _WIN32_WCE_PSPC
			COLORREF crBk = bSelected ? 0xff000000 : pItem->getBackground();
#else
			COLORREF crBk = pItem->getBackground();
#endif
			if (crBk == 0xff000000) {
#if defined _WIN32_WCE && _WIN32_WCE >= 0x300 && defined _WIN32_WCE_PSPC
				UINT nBkColorId = bSelected ? pThis_->hasFocus() ?
					COLOR_HIGHLIGHT : COLOR_INACTIVEBORDER : COLOR_WINDOW;
#else
				UINT nBkColorId = COLOR_WINDOW;
#endif
				crBk = getColor(nBkColorId);
			}
			COLORREF crBkOld = pdc->getBkColor();
			pdc->fillSolidRect(r, crBk);
			pdc->setBkColor(crBkOld);
			
#if defined _WIN32_WCE && _WIN32_WCE >= 0x300 && defined _WIN32_WCE_PSPC
			UINT nFlags = bSelected ? ILD_TRANSPARENT : ILD_NORMAL;
#else
			UINT nFlags = bSelected ? ILD_SELECTED : ILD_NORMAL;
#endif
			int y = r.top + (r.bottom - r.top - 16)/2;
			ImageList_Draw(hImageList_, getMessageImage(pmh, nValue),
				pdc->getHandle(), r.left + 7, y, nFlags);
			if (nValue & MessageHolder::FLAG_MULTIPART)
				ImageList_Draw(hImageListData_, 0, pdc->getHandle(),
					r.left, y, nFlags);
			if (nValue & MessageHolder::FLAG_REPLIED)
				ImageList_Draw(hImageListData_, 1, pdc->getHandle(),
					r.left + 15, y, nFlags);
			else if (nValue & MessageHolder::FLAG_FORWARDED)
				ImageList_Draw(hImageListData_, 2, pdc->getHandle(),
					r.left + 15, y, nFlags);
			if (nValue & MessageHolder::FLAG_DOWNLOAD ||
				nValue & MessageHolder::FLAG_DOWNLOADTEXT)
				ImageList_Draw(hImageListData_, 3, pdc->getHandle(),
					r.left + 7, y, nFlags);
			if (nValue & MessageHolder::FLAG_DELETED)
				ImageList_Draw(hImageListData_, 4, pdc->getHandle(),
					r.left + 7, y, nFlags);
			if (nValue & MessageHolder::FLAG_MARKED)
				ImageList_Draw(hImageListData_, 5, pdc->getHandle(),
					r.left + 15, y, nFlags);
			
#if defined _WIN32_WCE && _WIN32_WCE >= 0x300 && defined _WIN32_WCE_PSPC
			rectFocus.right = r.right;
#endif
		}
		
		r.left = r.right;
	}
	r.right = rect.right;
	pdc->fillSolidRect(r, crBackground);
	
	if (((pViewModel->getSort() & ViewModel::SORT_THREAD_MASK) == ViewModel::SORT_THREAD) &&
		nThreadLeft != -1) {
		if (nLevel > 0) {
			typedef std::vector<int> Line;
			Line line(nLevel, 0);
			for (unsigned int n = pi.getIndex() + 1; n < pViewModel->getCount(); ++n) {
				const ViewModelItem* pNextItem = pViewModel->getItem(n);
				unsigned int nNextLevel = pNextItem->getLevel();
				if (nNextLevel == 0) {
					break;
				}
				else if (nNextLevel <= nLevel) {
					line[nNextLevel - 1] = 1;
					nLevel = nNextLevel;
				}
			}
			
			HPEN hpen = (bSelected && bHasFocus) ?
				hpenFocusedThreadLine_ : hpenThreadLine_;
			ObjectSelector<HPEN> selector(*pdc, hpen);
			int nOffset = 7;
			POINT points[4];
			for (Line::iterator it = line.begin(); it != line.end(); ++it) {
				int nPoint = 2;
				if (*it || it + 1 == line.end()) {
					points[0].x = nThreadLeft + nOffset;
					points[0].y = rect.top;
					points[1].x = points[0].x;
					points[1].y = *it ? rect.bottom : (rect.top + rect.bottom)/2;
					if (it + 1 == line.end()) {
						if (*it) {
							points[nPoint].x = points[1].x;
							points[nPoint].y = (rect.top + rect.bottom)/2;
							++nPoint;
						}
						points[nPoint].x = points[nPoint - 1].x + 5;
						points[nPoint].y = points[nPoint - 1].y;
						++nPoint;
					}
					pdc->polyline(points, nPoint);
				}
				nOffset += INDENT;
				if (nThreadLeft + nOffset > nThreadRight)
					break;
			}
		}
	}
	
#if !defined _WIN32_WCE || _WIN32_WCE < 0x300 || !defined _WIN32_WCE_PSPC
	if ((pItem->getFlags() & ViewModelItem::FLAG_FOCUSED) && bHasFocus) {
		pdc->setTextColor(COLOR_WINDOWTEXT);
		pdc->drawFocusRect(rectFocus);
	}
#endif
}

void qm::ListWindowImpl::invalidateLine(unsigned int nLine)
{
	RECT rect;
	getRect(&rect);
	
	int nPos = pThis_->getScrollPos(SB_VERT);
	if (static_cast<int>(nLine) < nPos ||
		nPos + (rect.bottom - rect.top)/nLineHeight_ + 1 < static_cast<int>(nLine))
		return;
	rect.top += (nLine - nPos)*nLineHeight_;
	rect.bottom = rect.top + nLineHeight_;
	
	pThis_->invalidateRect(rect);
}

void qm::ListWindowImpl::invalidateSelected()
{
	ViewModel* pViewModel = pViewModelManager_->getCurrentViewModel();
	if (pViewModel) {
		Lock<ViewModel> lock(*pViewModel);
		
		RECT rect;
		getRect(&rect);
		int nTop = rect.top;
		
		SCROLLINFO si = {
			sizeof(si),
			SIF_PAGE | SIF_POS
		};
		pThis_->getScrollInfo(SB_VERT, &si);
		
		unsigned int nCount = pViewModel->getCount();
		unsigned int nFocused = pViewModel->getFocused();
		for (unsigned int n = si.nPos; n < nCount && n <= si.nPos + si.nPage; ++n) {
			if (pViewModel->isSelected(n) || n == nFocused) {
				rect.top = nTop + (n - si.nPos)*nLineHeight_;
				rect.bottom = rect.top + nLineHeight_;
				pThis_->invalidateRect(rect);
			}
		}
	}
}

void qm::ListWindowImpl::ensureVisible(unsigned int nLine)
{
	SCROLLINFO si = {
		sizeof(si),
		SIF_POS | SIF_PAGE | SIF_RANGE
	};
	pThis_->getScrollInfo(SB_VERT, &si);
	if (si.nMax - si.nMin < static_cast<int>(si.nPage))
		return;
	
	if (nLine < static_cast<unsigned int>(si.nPos))
		scrollVertical(nLine);
	else if (nLine >= static_cast<unsigned int>(si.nPos + si.nPage) - 1)
		scrollVertical(nLine + 1 - si.nPage);
}

void qm::ListWindowImpl::scrollVertical(int nPos)
{
	SCROLLINFO si = {
		sizeof(si),
		SIF_RANGE | SIF_POS | SIF_PAGE
	};
	pThis_->getScrollInfo(SB_VERT, &si);
	
	if (nPos < 0)
		nPos = 0;
	else if (nPos > si.nMax - static_cast<int>(si.nPage) + 1)
		nPos = si.nMax - si.nPage + 1;
	
	if (nPos == si.nPos)
		return;
	
	RECT rect;
	getRect(&rect);
	
	int nOldPos = si.nPos;
	pThis_->setScrollPos(SB_VERT, nPos);
	pThis_->scrollWindow(0, (nOldPos - nPos)*nLineHeight_,
		&rect, &rect, 0, 0, SW_INVALIDATE);
	
#ifdef QMTOOLTIP
	hideToolTip();
#endif
}

void qm::ListWindowImpl::scrollHorizontal(int nPos)
{
	SCROLLINFO si = {
		sizeof(si),
		SIF_RANGE | SIF_POS | SIF_PAGE
	};
	pThis_->getScrollInfo(SB_HORZ, &si);
	
	if (nPos < 0)
		nPos = 0;
	else if (nPos > si.nMax - static_cast<int>(si.nPage) + 1)
		nPos = si.nMax - si.nPage;
	
	if (nPos == si.nPos)
		return;
	
	RECT rect;
	pThis_->getClientRect(&rect);
	
	int nOldPos = si.nPos;
	pThis_->setScrollPos(SB_HORZ, nPos);
	pThis_->scrollWindow(nOldPos - nPos, 0,
		&rect, &rect, 0, 0, SW_INVALIDATE/* | SW_SCROLLCHILDREN*/);
	
	pHeaderColumn_->setWindowPos(0, -nPos, 0,
		(rect.right - rect.left) + nPos, pHeaderColumn_->getHeight(),
		SWP_NOZORDER | SWP_NOACTIVATE);
	
#ifdef QMTOOLTIP
	hideToolTip();
#endif
}

void qm::ListWindowImpl::updateScrollBar(bool bVertical)
{
	RECT rect;
	getRect(&rect);
	
	if (bVertical) {
		ViewModel* pViewModel = pViewModelManager_->getCurrentViewModel();
		unsigned int nCount = 0;
		if (pViewModel) {
			Lock<ViewModel> lock(*pViewModel);
			nCount = pViewModel->getCount();
		}
		
		unsigned int nPage = (rect.bottom - rect.top)/nLineHeight_;
		if (nPage > nCount)
			nPage = nCount;
		SCROLLINFO si = {
			sizeof(si),
			SIF_PAGE | SIF_RANGE,
			0,
			nCount == 0 ? 0 : nCount - 1,
			nPage,
			0,
		};
		pThis_->setScrollInfo(SB_VERT, si);
	}
	else {
		unsigned int nWidth = pHeaderColumn_->getWidth();
		SCROLLINFO si = {
			sizeof(si),
			SIF_PAGE | SIF_RANGE,
			0,
			nWidth,
			rect.right - rect.left,
			0,
		};
		pThis_->setScrollInfo(SB_HORZ, si);
		
		pHeaderColumn_->setWindowPos(0, -pThis_->getScrollPos(SB_HORZ),
			0, 0, 0, SWP_NOZORDER | SWP_NOSIZE | SWP_NOACTIVATE);
	}
}

unsigned int qm::ListWindowImpl::getLineFromPoint(const POINT& pt) const
{
	ViewModel* pViewModel = pViewModelManager_->getCurrentViewModel();
	assert(pViewModel);
	assert(pViewModel->isLocked());
	
	unsigned int nLine = (pt.y - pHeaderColumn_->getHeight())/
		nLineHeight_ + pThis_->getScrollPos(SB_VERT);
	return nLine < pViewModel->getCount() ? nLine : -1;
}

unsigned int qm::ListWindowImpl::getColumnFromPoint(const POINT& pt) const
{
	ViewModel* pViewModel = pViewModelManager_->getCurrentViewModel();
	assert(pViewModel);
	
	int nX = pt.x + pThis_->getScrollPos(SB_HORZ);
	
	const ViewColumnList& listColumn = pViewModel->getColumns();
	for (unsigned int n = 0; n < listColumn.size(); ++n) {
		int nWidth = listColumn[n]->getWidth();
		if (nX < nWidth)
			return n;
		nX -= nWidth;
	}
	return -1;
}

void qm::ListWindowImpl::getRect(unsigned int nLine,
								 unsigned int nColumn,
								 RECT* pRect)
{
	assert(pRect);
	
	ViewModel* pViewModel = pViewModelManager_->getCurrentViewModel();
	assert(pViewModel);
	
	pRect->top = pHeaderColumn_->getHeight() +
		(nLine - pThis_->getScrollPos(SB_VERT))*nLineHeight_;
	pRect->bottom = pRect->top + nLineHeight_;
	
	pRect->left = -pThis_->getScrollPos(SB_HORZ);
	for (unsigned int n = 0; n < nColumn; ++n)
		pRect->left += pViewModel->getColumn(n).getWidth();
	pRect->right = pRect->left + pViewModel->getColumn(nColumn).getWidth();
}

void qm::ListWindowImpl::reloadProfiles(bool bInitialize)
{
	bSingleClickOpen_ = pProfile_->getInt(L"ListWindow", L"SingleClickOpen") != 0;
	bEllipsis_ = pProfile_->getInt(L"ListWindow", L"Ellipsis") != 0;
	
	HFONT hfont = qs::UIUtil::createFontFromProfile(pProfile_,
		L"ListWindow", qs::UIUtil::DEFAULTFONT_UI);
	if (!bInitialize) {
		assert(hfont_);
		pHeaderColumn_->setFont(hfont);
		::DeleteObject(hfont_);
	}
	hfont_ = hfont;
	
	LOGFONT lf;
	::GetObject(hfont, sizeof(lf), &lf);
	lf.lfWeight = FW_BOLD;
	HFONT hfontBold = ::CreateFontIndirect(&lf);
	if (!bInitialize) {
		assert(hfontBold_);
		::DeleteObject(hfontBold_);
	}
	hfontBold_ = hfontBold;
	
	if (!bInitialize) {
		updateLineHeight();
		layoutChildren();
	}
	
	bUseSystemColor_ = pProfile_->getInt(L"ListWindow", L"UseSystemColor") != 0;
	if (!bUseSystemColor_) {
		struct {
			const WCHAR* pwszKey_;
			int nIndex_;
			COLORREF* pcr_;
		} colors[] = {
			{ L"ForegroundColor",					COLOR_WINDOWTEXT,		&crForeground_					},
			{ L"BackgroundColor",					COLOR_WINDOW,			&crBackground_					},
			{ L"SelectedForegroundColor",			COLOR_HIGHLIGHTTEXT,	&crSelectedForeground_			},
			{ L"SelectedBackgroundColor",			COLOR_HIGHLIGHT,		&crSelectedBackground_			},
			{ L"SelectedNotFocusBackgroundColor",	COLOR_INACTIVEBORDER,	&crSelectedNotFocusBackground_	}
		};
		for (int n = 0; n < countof(colors); ++n) {
			wstring_ptr wstr(pProfile_->getString(L"ListWindow", colors[n].pwszKey_));
			Color color(wstr.get());
			if (color.getColor() != 0xffffffff)
				*colors[n].pcr_ = color.getColor();
			else
				*colors[n].pcr_ = ::GetSysColor(colors[n].nIndex_);
		}
	}
}

void qm::ListWindowImpl::updateLineHeight()
{
	ClientDeviceContext dc(pThis_->getHandle());
	ObjectSelector<HFONT> selector(dc, hfont_);
	TEXTMETRIC tm;
	dc.getTextMetrics(&tm);
	nLineHeight_ = tm.tmHeight +
		tm.tmExternalLeading + ListWindowImpl::LINE_SPACING;
}

COLORREF qm::ListWindowImpl::getItemForeground(const ViewModelItem* pItem) const
{
	COLORREF cr = pItem->getForeground();
	return cr != 0xff000000 ? cr : getColor(COLOR_WINDOWTEXT);
}

COLORREF qm::ListWindowImpl::getItemBackground(const ViewModelItem* pItem) const
{
	COLORREF cr = pItem->getBackground();
	return cr != 0xff000000 ? cr : getColor(COLOR_WINDOW);
}

COLORREF qm::ListWindowImpl::getColor(int nIndex) const
{
	if (!bUseSystemColor_) {
		switch (nIndex) {
		case COLOR_WINDOWTEXT:
			return crForeground_;
		case COLOR_WINDOW:
			return crBackground_;
		case COLOR_HIGHLIGHTTEXT:
			return crSelectedForeground_;
		case COLOR_HIGHLIGHT:
			return crSelectedBackground_;
		case COLOR_INACTIVEBORDER:
			return crSelectedNotFocusBackground_;
		default:
			break;
		}
	}
	return ::GetSysColor(nIndex);
}

#ifdef QMTOOLTIP
void qm::ListWindowImpl::relayToolTipEvent(UINT uMsg,
										   WPARAM wParam,
										   LPARAM lParam)
{
	POINT pt = {
		GET_X_LPARAM(lParam),
		GET_Y_LPARAM(lParam)
	};
	
	Window toolTip(hwndToolTip_);
	
	wstring_ptr wstrText;
	bool bUpdate = false;
	
	ViewModel* pViewModel = pViewModelManager_->getCurrentViewModel();
	if (pViewModel) {
		Lock<ViewModel> lock(*pViewModel);
		unsigned int nLine = getLineFromPoint(pt);
		unsigned int nColumn = getColumnFromPoint(pt);
		if (nLine != nToolTipLine_ || nColumn != nToolTipColumn_) {
			nToolTipIndent_ = 0;
			if (nLine != -1 && nColumn != -1) {
				const ViewColumn& column = pViewModel->getColumn(nColumn);
				const ViewModelItem* pItem = pViewModel->getItem(nLine);
				if (!column.isFlag(ViewColumn::FLAG_ICON)) {
					wstring_ptr wstr(column.getText(pViewModel, pItem));
					
					HFONT hfont = pItem->isBold() ? hfontBold_ : hfont_;
					ClientDeviceContext dc(pThis_->getHandle());
					ObjectSelector<HFONT> selector(dc, hfont);
					SIZE size;
					dc.getTextExtent(wstr.get(), static_cast<int>(wcslen(wstr.get())), &size);
					
					int nIndent = 0;
					if ((pViewModel->getSort() & ViewModel::SORT_THREAD_MASK) == ViewModel::SORT_THREAD &&
						column.isFlag(ViewColumn::FLAG_INDENT))
						nIndent = pItem->getLevel()*INDENT;
					int nWidth = size.cx + HORIZONTAL_BORDER*2 + nIndent;
					
					if (nWidth > static_cast<int>(column.getWidth())) {
						wstrText = wstr;
						toolTip.sendMessage(TTM_SETTIPTEXTCOLOR, getItemForeground(pItem));
						toolTip.sendMessage(TTM_SETTIPBKCOLOR, getItemBackground(pItem));
						toolTip.setFont(hfont);
						nToolTipIndent_ = nIndent;
					}
				}
			}
			nToolTipLine_ = nLine;
			nToolTipColumn_ = nColumn;
			bUpdate = true;
		}
	}
	else {
		bUpdate = nToolTipLine_ != -1 || nToolTipColumn_ != -1;
		nToolTipLine_ = -1;
		nToolTipColumn_ = -1;
		nToolTipIndent_ = 0;
	}
	
	if (bUpdate) {
		toolTip.sendMessage(TTM_POP);
		toolTip.showWindow(SW_HIDE);
		
		W2T(wstrText.get(), ptszText);
		TOOLINFO ti = {
			sizeof(ti),
			0,
			pThis_->getHandle(),
			ID_TOOLTIP,
			{ 0, 0, 0, 0 },
			0,
			const_cast<LPTSTR>(ptszText)
		};
		toolTip.sendMessage(TTM_UPDATETIPTEXT,
			0, reinterpret_cast<LPARAM>(&ti));
	}
	
	MSG msg = {
		pThis_->getHandle(),
		uMsg,
		wParam,
		lParam
	};
	toolTip.sendMessage(TTM_RELAYEVENT, 0, reinterpret_cast<LPARAM>(&msg));
}

void qm::ListWindowImpl::hideToolTip()
{
	Window(hwndToolTip_).sendMessage(TTM_POP);
	nToolTipLine_ = -1;
	nToolTipColumn_ = -1;
	nToolTipIndent_ = 0;
}
#endif

void qm::ListWindowImpl::postRefreshMessage(UINT uMsg,
											unsigned int nItem)
{
	if (InterlockedCompareExchange(UNVOLATILE(LONG*)(&nRefreshing_), 1, 0))
		return;
	if (!pThis_->postMessage(uMsg, nItem, 0))
		InterlockedExchange(UNVOLATILE(LONG*)(&nRefreshing_), 0);
}

void qm::ListWindowImpl::handleRefreshMessage()
{
	InterlockedExchange(UNVOLATILE(LONG*)(&nRefreshing_), 0);
	pThis_->refresh();
}

void qm::ListWindowImpl::postInvalidateMessage(UINT uMsg,
											   unsigned int nItem)
{
	if (InterlockedIncrement(UNVOLATILE(LONG*)(&nInvalidating_)) > 10) {
		InterlockedDecrement(UNVOLATILE(LONG*)(&nInvalidating_));
		return;
	}
	if (!pThis_->postMessage(uMsg, nItem, 0))
		InterlockedDecrement(UNVOLATILE(LONG*)(&nInvalidating_));
}

void qm::ListWindowImpl::handleInvalidateMessage(unsigned int nItem)
{
	if (InterlockedDecrement(UNVOLATILE(LONG*)(&nInvalidating_)) >= 9)
		pThis_->invalidate();
	else
		invalidateLine(nItem);
}

LRESULT qm::ListWindowImpl::onNotify(NMHDR* pnmhdr,
									 bool* pbHandled)
{
#ifdef QMTOOLTIP
	BEGIN_NOTIFY_HANDLER()
		HANDLE_NOTIFY(TTN_SHOW, ID_TOOLTIP, onShow)
	END_NOTIFY_HANDLER()
#endif
	return NotifyHandler::onNotify(pnmhdr, pbHandled);
}

void qm::ListWindowImpl::viewModelSelected(const ViewModelManagerEvent& event)
{
	ViewModel* pOldViewModel = event.getOldViewModel();
	if (pOldViewModel) {
		pOldViewModel->setScroll(pThis_->getScrollPos(SB_HORZ),
			pThis_->getScrollPos(SB_VERT));
		pOldViewModel->removeViewModelHandler(this);
	}
	
	ViewModel* pNewViewModel = event.getNewViewModel();
	if (pNewViewModel)
		pNewViewModel->addViewModelHandler(this);
	
	pHeaderColumn_->setViewModel(pNewViewModel);
	
	pThis_->refresh();
	
	if (pNewViewModel) {
		std::pair<unsigned int, unsigned int> scroll(pNewViewModel->getScroll());
		if (scroll.first != -1)
			scrollHorizontal(scroll.first);
		if (scroll.second != -1)
			scrollVertical(scroll.second);
		else
			ensureVisible(pNewViewModel->getFocused());
	}
}

void qm::ListWindowImpl::itemAdded(const ViewModelEvent& event)
{
	postRefreshMessage(WM_VIEWMODEL_ITEMADDED, event.getItem());
}

void qm::ListWindowImpl::itemRemoved(const ViewModelEvent& event)
{
	postRefreshMessage(WM_VIEWMODEL_ITEMREMOVED, event.getItem());
}

void qm::ListWindowImpl::itemChanged(const ViewModelEvent& event)
{
	postInvalidateMessage(WM_VIEWMODEL_ITEMCHANGED, event.getItem());
}

void qm::ListWindowImpl::itemStateChanged(const ViewModelEvent& event)
{
	assert(event.getViewModel() == pViewModelManager_->getCurrentViewModel());
	invalidateLine(event.getItem());
}

void qm::ListWindowImpl::itemAttentionPaid(const ViewModelEvent& event)
{
	assert(event.getViewModel() == pViewModelManager_->getCurrentViewModel());
	ensureVisible(event.getItem());
}

void qm::ListWindowImpl::updated(const ViewModelEvent& event)
{
	assert(event.getViewModel() == pViewModelManager_->getCurrentViewModel());
	pThis_->refresh();
}

void qm::ListWindowImpl::sorted(const ViewModelEvent& event)
{
	assert(event.getViewModel() == pViewModelManager_->getCurrentViewModel());
	ensureVisible(event.getViewModel()->getFocused());
	pThis_->invalidate();
}

void qm::ListWindowImpl::colorChanged(const ViewModelEvent& event)
{
	assert(event.getViewModel() == pViewModelManager_->getCurrentViewModel());
	pThis_->invalidate();
}

void qm::ListWindowImpl::columnChanged(const ViewModelEvent& event)
{
	assert(event.getViewModel() == pViewModelManager_->getCurrentViewModel());
	pHeaderColumn_->setViewModel(pViewModelManager_->getCurrentViewModel());
	pThis_->refresh();
}

void qm::ListWindowImpl::destroyed(const ViewModelEvent& event)
{
	assert(false);
}

void qm::ListWindowImpl::dragGestureRecognized(const DragGestureEvent& event)
{
	ViewModel* pViewModel = pViewModelManager_->getCurrentViewModel();
	if (!pViewModel)
		return;
	
	Lock<ViewModel> lock(*pViewModel);
	
	if (getLineFromPoint(event.getPoint()) == -1)
		return;
	
	MessageHolderList l;
	pViewModel->getSelection(&l);
	if (l.empty())
		return;
	
	POINT ptHotspot;
	HIMAGELIST hImageList = createDragImage(event.getPoint(), &ptHotspot);
	if (!hImageList)
		return;
	
	std::auto_ptr<MessageDataObject> p(new MessageDataObject(pAccountManager_,
		pURIResolver_, pViewModel->getFolder(), l, MessageDataObject::FLAG_NONE));
	p->AddRef();
	ComPtr<IDataObject> pDataObject(p.release());
	
	ImageList_BeginDrag(hImageList, 0, ptHotspot.x, ptHotspot.y);
	
	DragSource source;
	source.setDragSourceHandler(this);
	source.startDrag(event, pDataObject.get(), DROPEFFECT_COPY | DROPEFFECT_MOVE);
	
	ImageList_EndDrag();
	ImageList_Destroy(hImageList);
}

void qm::ListWindowImpl::dragDropEnd(const DragSourceDropEvent& event)
{
}

void qm::ListWindowImpl::dragEnter(const DropTargetDragEvent& event)
{
	IDataObject* pDataObject = event.getDataObject();
	
#ifndef _WIN32_WCE
	FORMATETC fe = {
		CF_HDROP,
		0,
		DVASPECT_CONTENT,
		-1,
		TYMED_HGLOBAL
	};
	StgMedium stm;
	if (pDataObject->GetData(&fe, &stm) == S_OK) {
		if (stm.tymed == TYMED_HGLOBAL) {
			HDROP hDrop = reinterpret_cast<HDROP>(stm.hGlobal);
			UINT nCount = ::DragQueryFile(hDrop, 0xffffffff, 0, 0);
			for (UINT n = 0; n < nCount && !bCanDrop_; ++n) {
				TCHAR tszPath[MAX_PATH];
				::DragQueryFile(hDrop, n, tszPath, countof(tszPath));
				T2W(tszPath, pwszPath);
				bCanDrop_ = File::isFileExisting(pwszPath);
			}
		}
	}
#endif
	
	event.setEffect(bCanDrop_ ? DROPEFFECT_COPY : DROPEFFECT_NONE);
	
	POINT pt = event.getPoint();
	pThis_->screenToClient(&pt);
	ImageList_DragEnter(pThis_->getHandle(), pt.x, pt.y);
}

void qm::ListWindowImpl::dragOver(const DropTargetDragEvent& event)
{
	event.setEffect(bCanDrop_ ? DROPEFFECT_COPY : DROPEFFECT_NONE);
	
	POINT pt = event.getPoint();
	pThis_->screenToClient(&pt);
	ImageList_DragMove(pt.x, pt.y);
}

void qm::ListWindowImpl::dragExit(const DropTargetEvent& event)
{
	bCanDrop_ = false;
	
	ImageList_DragLeave(pThis_->getHandle());
}

void qm::ListWindowImpl::drop(const DropTargetDropEvent& event)
{
	ViewModel* pViewModel = pViewModelManager_->getCurrentViewModel();
	if (pViewModel) {
		Folder* pFolder = pViewModel->getFolder();
		if (pFolder->getType() == Folder::TYPE_NORMAL) {
			IDataObject* pDataObject = event.getDataObject();
			
#ifndef _WIN32_WCE
			FORMATETC fe = {
				CF_HDROP,
				0,
				DVASPECT_CONTENT,
				-1,
				TYMED_HGLOBAL
			};
			StgMedium stm;
			if (pDataObject->GetData(&fe, &stm) == S_OK) {
				if (stm.tymed == TYMED_HGLOBAL) {
					HDROP hDrop = reinterpret_cast<HDROP>(stm.hGlobal);
					int nCount = ::DragQueryFile(hDrop, 0xffffffff, 0, 0);
					
					FileImportAction::PathList listPath;
					listPath.reserve(nCount);
					StringListFree<FileImportAction::PathList> free(listPath);
					
					for (int n = 0; n < nCount; ++n) {
						TCHAR tszPath[MAX_PATH];
						::DragQueryFile(hDrop, n, tszPath, countof(tszPath));
						wstring_ptr wstrPath(tcs2wcs(tszPath));
						if (File::isFileExisting(wstrPath.get()))
							listPath.push_back(wstrPath.release());
					}
					
					NormalFolder* pNormalFolder = static_cast<NormalFolder*>(pFolder);
					if (event.getKeyState() & MK_ALT) {
						if (!FileImportAction::importShowDialog(pNormalFolder,
							listPath, pProfile_, pThis_->getParentFrame(), 0, 0)) {
							messageBox(getResourceHandle(), IDS_ERROR_IMPORT,
								MB_OK | MB_ICONERROR, pThis_->getParentFrame());
						}
					}
					else {
						if (!FileImportAction::import(pNormalFolder,
							listPath, false, 0, Account::IMPORTFLAG_NORMALFLAGS,
							pThis_->getParentFrame(), 0, 0)) {
							messageBox(getResourceHandle(), IDS_ERROR_IMPORT,
								MB_OK | MB_ICONERROR, pThis_->getParentFrame());
						}
					}
				}
			}
#endif
		}
	}
	
	ImageList_DragLeave(pThis_->getHandle());
}

#ifdef QMTOOLTIP
LRESULT qm::ListWindowImpl::onShow(NMHDR* pnmhdr,
								   bool* pbHandled)
{
	*pbHandled = true;
	
	if (nToolTipLine_ == -1 || nToolTipColumn_ == -1)
		return 0;
	
	RECT rect;
	getRect(nToolTipLine_, nToolTipColumn_, &rect);
	rect.top += LINE_SPACING/2;
	rect.bottom -= LINE_SPACING/2;
	rect.left += HORIZONTAL_BORDER + nToolTipIndent_;
	pThis_->clientToScreen(&rect);
	
	Window toolTip(hwndToolTip_);
	toolTip.sendMessage(TTM_ADJUSTRECT,
		TRUE, reinterpret_cast<LPARAM>(&rect));
	toolTip.setWindowPos(HWND_TOP, rect.left,
		rect.top, 0, 0, SWP_NOSIZE | SWP_NOACTIVATE);
	
	return 1;
}
#endif

HIMAGELIST qm::ListWindowImpl::createDragImage(const POINT& ptCursor,
											   POINT* pptHotspot)
{
	assert(pptHotspot);
	
#if defined _WIN32_WCE && _WIN32_WCE < 0x500
	const UINT nFlags = ILC_COLOR | ILC_MASK;
#else
	const UINT nFlags = ILC_COLOR32 | ILC_MASK;
#endif
	BOOL bImage = TRUE;
#ifndef _WIN32_WCE
	if (!::SystemParametersInfo(SPI_GETDRAGFULLWINDOWS, 0, &bImage, 0))
		bImage = FALSE;
#endif
	if (bImage) {
		ViewModel* pViewModel = pViewModelManager_->getCurrentViewModel();
		assert(pViewModel);
		
		unsigned int nWidth = pHeaderColumn_->getWidth();
		
		SCROLLINFO si = {
			sizeof(si),
			SIF_PAGE | SIF_POS
		};
		pThis_->getScrollInfo(SB_VERT, &si);
		
		unsigned int nStart = -1;
		unsigned int nEnd = -1;
		unsigned int nCount = pViewModel->getCount();
		for (unsigned int n = si.nPos; n < nCount && n <= si.nPos + si.nPage; ++n) {
			if (pViewModel->isSelected(n)) {
				if (nStart == -1)
					nStart = n;
				nEnd = n;
			}
		}
		if (nStart == -1)
			return 0;
		unsigned int nHeight = nLineHeight_*(nEnd - nStart + 1);
		
		ClientDeviceContext dc(pThis_->getHandle());
		GdiObject<HBITMAP> hbm(::CreateCompatibleBitmap(dc.getHandle(), nWidth, nHeight));
		CompatibleDeviceContext dcMem(dc.getHandle());
		{
			ObjectSelector<HFONT> fontSelector(dcMem, hfont_);
			ObjectSelector<HBITMAP> selector(dcMem, hbm.get());
			RECT rect = {
				0,
				0,
				nWidth,
				nHeight
			};
			dcMem.fillSolidRect(Rect(0, 0, nWidth, nHeight), getColor(COLOR_WINDOW));
			
			int nTop = 0;
			for (unsigned int n = nStart; n <= nEnd; ++n) {
				if (pViewModel->isSelected(n)) {
					RECT r = {
						0,
						nTop,
						nWidth,
						nTop + nLineHeight_
					};
					PaintInfo pi(&dcMem, pViewModel, n, r);
					paintMessage(pi);
				}
				nTop += nLineHeight_;
			}
		}
		
		HIMAGELIST hImageList = ImageList_Create(nWidth, nHeight, nFlags, 1, 0);
		ImageList_AddMasked(hImageList, hbm.get(), getColor(COLOR_WINDOW));
		
		pptHotspot->x = ptCursor.x + pThis_->getScrollPos(SB_HORZ);
		pptHotspot->y = ptCursor.y - pHeaderColumn_->getHeight() - (nStart - si.nPos)*nLineHeight_;
		
		return hImageList;
	}
	else {
		ClientDeviceContext dc(pThis_->getHandle());
		GdiObject<HBITMAP> hbm(::CreateCompatibleBitmap(dc.getHandle(), 1, 1));
		CompatibleDeviceContext dcMem(dc.getHandle());
		{
			ObjectSelector<HBITMAP> selector(dcMem, hbm.get());
			dcMem.fillSolidRect(Rect(0, 0, 1, 1), getColor(COLOR_WINDOW));
		}
		
		HIMAGELIST hImageList = ImageList_Create(1, 1, nFlags, 1, 0);
		ImageList_AddMasked(hImageList, hbm.get(), getColor(COLOR_WINDOW));
		
		pptHotspot->x = 0;
		pptHotspot->y = 0;
		
		return hImageList;
	}
}

int qm::ListWindowImpl::getMessageImage(MessageHolder* pmh,
										unsigned int nFlags)
{
	int nImage = 0;
	
	NormalFolder* pFolder = pmh->getFolder();
	if (pFolder && pFolder->isFlag(NormalFolder::FLAG_OUTBOX))
		nImage = nFlags & MessageHolder::FLAG_DRAFT ? 4 : 1;
	else if (nFlags & MessageHolder::FLAG_SENT)
		nImage = 2;
	else if (pmh->getAccount()->isSeen(nFlags))
		nImage = 3;
	
	if (nFlags & MessageHolder::FLAG_PARTIAL_MASK)
		nImage += 5;
	
	return nImage;
}


/****************************************************************************
 *
 * ListWindow
 *
 */

qm::ListWindow::ListWindow(ViewModelManager* pViewModelManager,
						   Profile* pProfile,
						   MessageFrameWindowManager* pMessageFrameWindowManager) :
	WindowBase(true),
	pImpl_(0)
{
	pImpl_ = new ListWindowImpl();
	pImpl_->pThis_ = this;
	pImpl_->pProfile_ = pProfile;
	pImpl_->pMessageFrameWindowManager_ = pMessageFrameWindowManager;
	pImpl_->pMenuManager_ = 0;
	pImpl_->pAccountManager_ = 0;
	pImpl_->pURIResolver_ = 0;
	pImpl_->pViewModelManager_ = pViewModelManager;
	pImpl_->hfont_ = 0;
	pImpl_->hfontBold_ = 0;
	pImpl_->bUseSystemColor_ = true;
	pImpl_->crForeground_ = RGB(0, 0, 0);
	pImpl_->crBackground_ = RGB(255, 255, 255);
	pImpl_->crSelectedForeground_ = RGB(0, 0, 0);
	pImpl_->crSelectedBackground_ = RGB(255, 255, 255);
	pImpl_->crSelectedNotFocusBackground_ = RGB(255, 255, 255);
	pImpl_->bSingleClickOpen_ = false;
	pImpl_->bEllipsis_ = true;
	pImpl_->nLineHeight_ = 0;
	pImpl_->pHeaderColumn_ = 0;
	pImpl_->hImageList_ = 0;
	pImpl_->hImageListData_ = 0;
	pImpl_->hpenThreadLine_ = 0;
	pImpl_->hpenFocusedThreadLine_ = 0;
#ifdef QMTOOLTIP
	pImpl_->hwndToolTip_ = 0;
	pImpl_->nToolTipLine_ = -1;
	pImpl_->nToolTipColumn_ = -1;
	pImpl_->nToolTipIndent_ = 0;
#endif
	pImpl_->bCanDrop_ = false;
	pImpl_->nRefreshing_ = 0;
	pImpl_->nInvalidating_ = 0;
	
	pImpl_->reloadProfiles(true);
	
	pImpl_->pViewModelManager_->addViewModelManagerHandler(pImpl_);
	
	setWindowHandler(this, false);
}

qm::ListWindow::~ListWindow()
{
	delete pImpl_;
}

void qm::ListWindow::refresh()
{
	pImpl_->updateScrollBar(true);
	pImpl_->updateScrollBar(false);
	
	invalidate();
	
#ifdef QMTOOLTIP
	pImpl_->hideToolTip();
#endif
}

void qm::ListWindow::moveSelection(MoveSelection m,
								   bool bShift,
								   bool bCtrl)
{
	ViewModel* pViewModel = pImpl_->pViewModelManager_->getCurrentViewModel();
	if (!pViewModel)
		return;
	
	Lock<ViewModel> lock(*pViewModel);
	
	unsigned int nCount = pViewModel->getCount();
	if (nCount == 0)
		return;
	
	unsigned int nLine = pViewModel->getFocused();
	unsigned int nNewLine = nLine;
	
	switch (m) {
	case MOVESELECTION_LINEUP:
		if (nNewLine != 0)
			--nNewLine;
		break;
	case MOVESELECTION_LINEDOWN:
		if (nNewLine != nCount - 1)
			++nNewLine;
		break;
	case MOVESELECTION_PAGEUP:
		{
			SCROLLINFO si = { sizeof(si), SIF_POS | SIF_PAGE };
			getScrollInfo(SB_VERT, &si);
			unsigned int nTop = si.nPos;
			if (nNewLine <= nTop)
				nNewLine = nNewLine > si.nPage ? nNewLine - si.nPage : 0;
			else
				nNewLine = nTop;
		}
		break;
	case MOVESELECTION_PAGEDOWN:
		{
			SCROLLINFO si = { sizeof(si), SIF_POS | SIF_PAGE };
			getScrollInfo(SB_VERT, &si);
			unsigned int nBottom = si.nPos + si.nPage - 1;
			if (nNewLine >= nBottom)
				nNewLine = nNewLine + si.nPage >= nCount ?
					nCount - 1 : nNewLine + si.nPage;
			else
				nNewLine = nBottom;
		}
		break;
	case MOVESELECTION_TOP:
		nNewLine = 0;
		break;
	case MOVESELECTION_BOTTOM:
		nNewLine = nCount - 1;
		break;
	case MOVESELECTION_CURRENT:
		if (bShift) {
			pViewModel->setSelection(nNewLine, pViewModel->getLastSelection());
		}
		else {
			if (pViewModel->isSelected(nNewLine)) {
				if (bCtrl)
					pViewModel->removeSelection(nNewLine);
			}
			else {
				pViewModel->addSelection(nNewLine);
			}
			if (bCtrl)
				pViewModel->setLastSelection(nNewLine);
		}
		break;
	default:
		assert(false);
		break;
	}
	
	if (nNewLine != nLine) {
		pViewModel->setFocused(nNewLine, true);
		if (bShift) {
			pViewModel->setSelection(nNewLine, pViewModel->getLastSelection());
		}
		else {
			if (!bCtrl) {
				pViewModel->setSelection(nNewLine);
				pViewModel->setLastSelection(nNewLine);
			}
		}
		pImpl_->ensureVisible(nNewLine);
	}
}

bool qm::ListWindow::isShowHeaderColumn() const
{
	return pImpl_->pHeaderColumn_->isShow();
}

void qm::ListWindow::setShowHeaderColumn(bool bShow)
{
	if (bShow != pImpl_->pHeaderColumn_->isShow()) {
		pImpl_->pHeaderColumn_->setShow(bShow);
		pImpl_->layoutChildren();
	}
}

void qm::ListWindow::reloadProfiles()
{
	pImpl_->reloadProfiles(false);
}

void qm::ListWindow::save() const
{
	ViewModel* pViewModel = pImpl_->pViewModelManager_->getCurrentViewModel();
	if (pViewModel)
		pViewModel->setScroll(getScrollPos(SB_HORZ), getScrollPos(SB_VERT));
	
	pImpl_->pHeaderColumn_->save();
}

bool qm::ListWindow::preCreateWindow(CREATESTRUCT* pCreateStruct)
{
	if (!DefaultWindowHandler::preCreateWindow(pCreateStruct))
		return false;
	pCreateStruct->style |= WS_HSCROLL | WS_VSCROLL;
	return true;
}

Accelerator* qm::ListWindow::getAccelerator()
{
	return pImpl_->pAccelerator_.get();
}

LRESULT qm::ListWindow::windowProc(UINT uMsg,
								   WPARAM wParam,
								   LPARAM lParam)
{
#ifdef QMTOOLTIP
	if (WM_MOUSEFIRST <= uMsg && uMsg <= WM_MOUSELAST)
		pImpl_->relayToolTipEvent(uMsg, wParam, lParam);
#endif
	
	BEGIN_MESSAGE_HANDLER()
		HANDLE_CONTEXTMENU()
		HANDLE_CREATE()
		HANDLE_DESTROY()
		HANDLE_ERASEBKGND()
		HANDLE_HSCROLL()
		HANDLE_KEYDOWN()
		HANDLE_KILLFOCUS()
		HANDLE_LBUTTONDBLCLK()
		HANDLE_LBUTTONDOWN()
		HANDLE_LBUTTONUP()
#if !defined _WIN32_WCE || _WIN32_WCE >= 0x211
		HANDLE_MOUSEWHEEL()
#endif
		HANDLE_PAINT()
		HANDLE_RBUTTONDOWN()
		HANDLE_RBUTTONUP()
		HANDLE_SETFOCUS()
		HANDLE_SIZE()
		HANDLE_VSCROLL()
		HANDLE_MESSAGE(ListWindowImpl::WM_VIEWMODEL_ITEMADDED, onViewModelItemAdded)
		HANDLE_MESSAGE(ListWindowImpl::WM_VIEWMODEL_ITEMREMOVED, onViewModelItemRemoved)
		HANDLE_MESSAGE(ListWindowImpl::WM_VIEWMODEL_ITEMCHANGED, onViewModelItemChanged)
	END_MESSAGE_HANDLER()
	return DefaultWindowHandler::windowProc(uMsg, wParam, lParam);
}

LRESULT qm::ListWindow::onContextMenu(HWND hwnd,
									  const POINT& pt)
{
	setFocus();
	
	POINT ptMenu = { 5, 5 };
	if (pt.x == -1 && pt.y == -1) {
		ViewModel* pViewModel = pImpl_->pViewModelManager_->getCurrentViewModel();
		if (pViewModel) {
			Lock<ViewModel> lock(*pViewModel);
			unsigned int nItem = pViewModel->getFocused();
			if (nItem < pViewModel->getCount()) {
				RECT rect;
				pImpl_->getRect(&rect);
				ptMenu.y = rect.top + (nItem - getScrollPos(SB_VERT))*pImpl_->nLineHeight_ + 5;
			}
		}
		clientToScreen(&ptMenu);
	}
	else {
		ptMenu = pt;
	}
	
	HMENU hmenu = pImpl_->pMenuManager_->getMenu(L"list", false, false);
	if (hmenu) {
		UINT nFlags = TPM_LEFTALIGN | TPM_TOPALIGN;
#ifndef _WIN32_WCE
		nFlags |= TPM_LEFTBUTTON | TPM_RIGHTBUTTON;
#endif
		::TrackPopupMenu(hmenu, nFlags, ptMenu.x, ptMenu.y, 0, getParentFrame(), 0);
	}
	
	return 0;
}

LRESULT qm::ListWindow::onCreate(CREATESTRUCT* pCreateStruct)
{
	if (DefaultWindowHandler::onCreate(pCreateStruct) == -1)
		return -1;
	
	ListWindowCreateContext* pContext =
		static_cast<ListWindowCreateContext*>(pCreateStruct->lpCreateParams);
	pImpl_->pAccountManager_ = pContext->pAccountManager_;
	pImpl_->pURIResolver_ = pContext->pURIResolver_;
	pImpl_->pMenuManager_ = pContext->pUIManager_->getMenuManager();
	pImpl_->pSyncManager_ = pContext->pSyncManager_;
	pImpl_->pSyncDialogManager_ = pContext->pSyncDialogManager_;
	
	CustomAcceleratorFactory acceleratorFactory;
	pImpl_->pAccelerator_ = pContext->pUIManager_->getKeyMap()->createAccelerator(
		&acceleratorFactory, L"ListWindow");
	if (!pImpl_->pAccelerator_.get())
		return -1;
	
	pImpl_->updateLineHeight();
	
	if (!pImpl_->createHeaderColumn())
		return -1;
	
	pImpl_->hImageList_ = UIUtil::createImageListFromFile(
		FileNames::LIST_BMP, 16, CLR_DEFAULT);
	if (!pImpl_->hImageList_)
		return -1;
#ifdef _WIN32_WCE_PSPC
	ImageList_SetBkColor(pImpl_->hImageList_, CLR_NONE);
#endif
	pImpl_->hImageListData_ = UIUtil::createImageListFromFile(
		FileNames::LISTDATA_BMP, 8, RGB(255, 255, 255));
	if (!pImpl_->hImageListData_)
		return -1;
	
	pImpl_->hpenThreadLine_ = ::CreatePen(PS_SOLID, 1, RGB(0, 0x80, 0));
	if (!pImpl_->hpenThreadLine_)
		return -1;
	pImpl_->hpenFocusedThreadLine_ = ::CreatePen(PS_SOLID, 1, RGB(255, 255, 255));
	if (!pImpl_->hpenFocusedThreadLine_)
		return -1;
	
#ifdef QMTOOLTIP
	DWORD dwToolTipExStyle = 0;
#if _WIN32_WINNT >= 0x500
	if (Version::isWindowsXPOrLater())
		dwToolTipExStyle = WS_EX_TRANSPARENT;
#endif
	pImpl_->hwndToolTip_ = ::CreateWindowEx(dwToolTipExStyle, TOOLTIPS_CLASS,
		0, TTS_NOPREFIX, 0, 0, 0, 0, 0, 0, getInstanceHandle(), 0);
	if (!pImpl_->hwndToolTip_)
		return -1;
	TOOLINFO ti = {
		sizeof(ti),
		TTF_TRANSPARENT,
		getHandle(),
		ListWindowImpl::ID_TOOLTIP,
		{ 0, 0, 0, 0 },
		0,
		LPSTR_TEXTCALLBACK
	};
	Window toolTip(pImpl_->hwndToolTip_);
	toolTip.sendMessage(TTM_ADDTOOL, 0, reinterpret_cast<LPARAM>(&ti));
	int nInitialDelay = static_cast<int>(toolTip.sendMessage(TTM_GETDELAYTIME, TTDT_RESHOW));
	toolTip.sendMessage(TTM_SETDELAYTIME, TTDT_INITIAL, MAKELONG(nInitialDelay, 0));
#endif
	
	pImpl_->pDragGestureRecognizer_.reset(new DragGestureRecognizer(getHandle()));
	pImpl_->pDragGestureRecognizer_->setDragGestureHandler(pImpl_);
	pImpl_->pDropTarget_.reset(new DropTarget(getHandle()));
	pImpl_->pDropTarget_->setDropTargetHandler(pImpl_);
	
	refresh();
	
	addNotifyHandler(pImpl_);
	
	return 0;
}

LRESULT qm::ListWindow::onDestroy()
{
	removeNotifyHandler(pImpl_);
	
	if (pImpl_->hfont_) {
		::DeleteObject(pImpl_->hfont_);
		pImpl_->hfont_ = 0;
	}
	
	if (pImpl_->hfontBold_) {
		::DeleteObject(pImpl_->hfontBold_);
		pImpl_->hfontBold_ = 0;
	}
	
	if (pImpl_->hImageList_) {
		ImageList_Destroy(pImpl_->hImageList_);
		pImpl_->hImageList_ = 0;
	}
	if (pImpl_->hImageListData_) {
		ImageList_Destroy(pImpl_->hImageListData_);
		pImpl_->hImageListData_ = 0;
	}
	
	if (pImpl_->hpenThreadLine_) {
		::DeleteObject(pImpl_->hpenThreadLine_);
		pImpl_->hpenThreadLine_ = 0;
	}
	if (pImpl_->hpenFocusedThreadLine_) {
		::DeleteObject(pImpl_->hpenFocusedThreadLine_);
		pImpl_->hpenFocusedThreadLine_ = 0;
	}
	
	pImpl_->pViewModelManager_->removeViewModelManagerHandler(pImpl_);
	
	pImpl_->pDragGestureRecognizer_.reset(0);
	pImpl_->pDropTarget_.reset(0);
	
	return DefaultWindowHandler::onDestroy();
}

LRESULT qm::ListWindow::onEraseBkgnd(HDC hdc)
{
	return 1;
}

LRESULT qm::ListWindow::onHScroll(UINT nCode,
								  UINT nPos,
								  HWND hwnd)
{
	SCROLLINFO si = {
		sizeof(si),
		SIF_RANGE | SIF_POS | SIF_PAGE
	};
	getScrollInfo(SB_HORZ, &si);
	
	int nNewPos = si.nPos;
	bool bScroll = true;
	switch (nCode) {
	case SB_LINELEFT:
		nNewPos -= 10;
		break;
	case SB_LINERIGHT:
		nNewPos += 10;
		break;
	case SB_PAGELEFT:
		nNewPos -= si.nPage;
		break;
	case SB_PAGERIGHT:
		nNewPos += si.nPage;
		break;
	case SB_LEFT:
		nNewPos = 0;
		break;
	case SB_RIGHT:
		nNewPos = si.nMax;
		break;
	case SB_THUMBPOSITION:
	case SB_THUMBTRACK:
		nNewPos = nPos;
		break;
	default:
		bScroll = false;
		break;
	}
	if (bScroll)
		pImpl_->scrollHorizontal(nNewPos);
	
	return 0;
}

LRESULT qm::ListWindow::onKeyDown(UINT nKey,
								  UINT nRepeat,
								  UINT nFlags)
{
	switch (nKey) {
	case VK_UP:
		moveSelection(MOVESELECTION_LINEUP,
			::GetKeyState(VK_SHIFT) < 0, ::GetKeyState(VK_CONTROL) < 0);
		break;
	case VK_DOWN:
		moveSelection(MOVESELECTION_LINEDOWN,
			::GetKeyState(VK_SHIFT) < 0, ::GetKeyState(VK_CONTROL) < 0);
		break;
	case VK_LEFT:
		moveSelection(MOVESELECTION_PAGEUP,
			::GetKeyState(VK_SHIFT) < 0, ::GetKeyState(VK_CONTROL) < 0);
		break;
	case VK_RIGHT:
		moveSelection(MOVESELECTION_PAGEDOWN,
			::GetKeyState(VK_SHIFT) < 0, ::GetKeyState(VK_CONTROL) < 0);
		break;
	case VK_PRIOR:
		moveSelection(MOVESELECTION_PAGEUP,
			::GetKeyState(VK_SHIFT) < 0, ::GetKeyState(VK_CONTROL) < 0);
		break;
	case VK_NEXT:
		moveSelection(MOVESELECTION_PAGEDOWN,
			::GetKeyState(VK_SHIFT) < 0, ::GetKeyState(VK_CONTROL) < 0);
		break;
	case VK_HOME:
		moveSelection(MOVESELECTION_TOP,
			::GetKeyState(VK_SHIFT) < 0, ::GetKeyState(VK_CONTROL) < 0);
		break;
	case VK_END:
		moveSelection(MOVESELECTION_BOTTOM,
			::GetKeyState(VK_SHIFT) < 0, ::GetKeyState(VK_CONTROL) < 0);
		break;
	case VK_SPACE:
		moveSelection(MOVESELECTION_CURRENT,
			::GetKeyState(VK_SHIFT) < 0, ::GetKeyState(VK_CONTROL) < 0);
		break;
	case VK_RETURN:
		{
			ViewModel* pViewModel = pImpl_->pViewModelManager_->getCurrentViewModel();
			if (pViewModel) {
				Lock<ViewModel> lock(*pViewModel);
				unsigned int nItem = pViewModel->getFocused();
				if (nItem < pViewModel->getCount()) {
					if (!pImpl_->pMessageFrameWindowManager_->open(
						pViewModel, pViewModel->getMessageHolder(nItem)))
						messageBox(getResourceHandle(), IDS_ERROR_OPENMESSAGE,
							MB_OK | MB_ICONERROR, getParentFrame());
				}
			}
		}
		break;
	default:
		break;
	}
	
	return 0;
}

LRESULT qm::ListWindow::onKillFocus(HWND hwnd)
{
	pImpl_->invalidateSelected();
	return 0;
}

LRESULT qm::ListWindow::onLButtonDblClk(UINT nFlags,
										const POINT& pt)
{
	if (!pImpl_->bSingleClickOpen_) {
		ViewModel* pViewModel = pImpl_->pViewModelManager_->getCurrentViewModel();
		if (pViewModel) {
			Lock<ViewModel> lock(*pViewModel);
			
			unsigned int nLine = pImpl_->getLineFromPoint(pt);
			if (nLine != -1) {
				if (!pImpl_->pMessageFrameWindowManager_->open(
					pViewModel, pViewModel->getMessageHolder(nLine)))
					messageBox(getResourceHandle(), IDS_ERROR_OPENMESSAGE,
						MB_OK | MB_ICONERROR, getParentFrame());
			}
		}
	}
	
	return 0;
}

LRESULT qm::ListWindow::onLButtonDown(UINT nFlags,
									  const POINT& pt)
{
	setFocus();
	
#if defined _WIN32_WCE && _WIN32_WCE >= 0x300 && defined _WIN32_WCE_PSPC
	bool bTapAndHold = false;
#endif
	
	ViewModel* pViewModel = pImpl_->pViewModelManager_->getCurrentViewModel();
	if (pViewModel) {
		Lock<ViewModel> lock(*pViewModel);
		
		unsigned int nLine = pImpl_->getLineFromPoint(pt);
		if (nLine != -1) {
			bool bSelected = pViewModel->isSelected(nLine);
			
			pViewModel->setFocused(nLine, false);
			
			if (!(nFlags & MK_CONTROL) && !bSelected)
				pViewModel->clearSelection();
			
			if (nFlags & MK_SHIFT) {
				unsigned int nLast = pViewModel->getLastSelection();
				pViewModel->setSelection(nLast, nLine);
			}
			else {
				if (nFlags & MK_CONTROL) {
					if (pViewModel->isSelected(nLine))
						pViewModel->removeSelection(nLine);
					else
						pViewModel->addSelection(nLine);
				}
				else {
					if (!bSelected)
						pViewModel->addSelection(nLine);
				}
				pViewModel->setLastSelection(nLine);
			}
			
			pImpl_->ensureVisible(nLine);
			
#if defined _WIN32_WCE && _WIN32_WCE >= 0x300 && defined _WIN32_WCE_PSPC
			if (tapAndHold(pt))
				return 0;
			bTapAndHold = true;
#endif
			
			if (pImpl_->bSingleClickOpen_) {
				if (!pImpl_->pMessageFrameWindowManager_->open(
					pViewModel, pViewModel->getMessageHolder(nLine)))
					messageBox(getResourceHandle(), IDS_ERROR_OPENMESSAGE,
						MB_OK | MB_ICONERROR, getParentFrame());
			}
		}
	}
	
#if defined _WIN32_WCE && _WIN32_WCE >= 0x300 && defined _WIN32_WCE_PSPC
	if (!bTapAndHold) {
		if (tapAndHold(pt))
			return 0;
	}
#endif
	
	return 0;
}

LRESULT qm::ListWindow::onLButtonUp(UINT nFlags,
									const POINT& pt)
{
	ViewModel* pViewModel = pImpl_->pViewModelManager_->getCurrentViewModel();
	if (pViewModel) {
		Lock<ViewModel> lock(*pViewModel);
		
		unsigned int nLine = pImpl_->getLineFromPoint(pt);
		if (nLine != -1) {
			bool bSelected = pViewModel->isSelected(nLine);
			
			if (bSelected && !(nFlags & MK_SHIFT) && !(nFlags & MK_CONTROL))
				pViewModel->setSelection(nLine);
		}
	}
	
	return 0;
}

#if !defined _WIN32_WCE || _WIN32_WCE >= 0x211
LRESULT qm::ListWindow::onMouseWheel(UINT nFlags,
									 short nDelta,
									 const POINT& pt)
{
#ifdef _WIN32_WCE
#	define WHEEL_DELTA 120
#endif
	pImpl_->scrollVertical(getScrollPos(SB_VERT) - nDelta/WHEEL_DELTA*3);
	
	return 0;
}
#endif

LRESULT qm::ListWindow::onPaint()
{
	PaintDeviceContext dc(getHandle());
	
	ObjectSelector<HFONT> fontSelector(dc, pImpl_->hfont_);
	
	ViewModel* pViewModel = pImpl_->pViewModelManager_->getCurrentViewModel();
	if (pViewModel) {
		Lock<ViewModel> lock(*pViewModel);
		
		RECT rectClip;
		dc.getClipBox(&rectClip);
		
		RECT rect;
		pImpl_->getRect(&rect);
		int nBottom = rect.bottom;
		rect.bottom = rect.top + pImpl_->nLineHeight_;
		
		SCROLLINFO si = {
			sizeof(si),
			SIF_PAGE | SIF_POS
		};
		getScrollInfo(SB_VERT, &si);
		
		int nHorzPos = getScrollPos(SB_HORZ);
		
		unsigned int nCount = pViewModel->getCount();
		for (unsigned int n = si.nPos; n < nCount && n <= si.nPos + si.nPage; ++n) {
			RECT rectTemp;
			if (::IntersectRect(&rectTemp, &rect, &rectClip)) {
				RECT r = {
					rect.left - nHorzPos,
					rect.top,
					rect.right,
					rect.bottom
				};
				PaintInfo pi(&dc, pViewModel, n, r);
				pImpl_->paintMessage(pi);
			}
			rect.top = rect.bottom;
			rect.bottom += pImpl_->nLineHeight_;
		}
		
		rect.bottom = nBottom;
		dc.fillSolidRect(rect, pImpl_->getColor(COLOR_WINDOW));
	}
	else {
		RECT rect;
		getClientRect(&rect);
		dc.fillSolidRect(rect, pImpl_->getColor(COLOR_WINDOW));
	}
	
	return 0;
}

LRESULT qm::ListWindow::onRButtonDown(UINT nFlags,
									  const POINT& pt)
{
	setFocus();
	
	ViewModel* pViewModel = pImpl_->pViewModelManager_->getCurrentViewModel();
	if (pViewModel) {
		Lock<ViewModel> lock(*pViewModel);
		
		unsigned int nLine = pImpl_->getLineFromPoint(pt);
		if (nLine != -1) {
			bool bSelected = pViewModel->isSelected(nLine);
			
			if ((nFlags & MK_SHIFT) == 0 && (nFlags & MK_CONTROL) == 0) {
				if (!bSelected)
					pViewModel->setSelection(nLine);
				
				pViewModel->setFocused(nLine, false);
				pViewModel->setLastSelection(nLine);
				pImpl_->ensureVisible(nLine);
			}
		}
	}
	
	return 0;
}

LRESULT qm::ListWindow::onRButtonUp(UINT nFlags,
									const POINT& pt)
{
	ViewModel* pViewModel = pImpl_->pViewModelManager_->getCurrentViewModel();
	if (pViewModel) {
		Lock<ViewModel> lock(*pViewModel);
		
		unsigned int nLine = pImpl_->getLineFromPoint(pt);
		if (nLine != -1) {
			bool bSelected = pViewModel->isSelected(nLine);
			
			if (!bSelected && nFlags & MK_SHIFT)
				pViewModel->setSelection(nLine);
		}
	}
	
	return DefaultWindowHandler::onRButtonUp(nFlags, pt);
}

LRESULT qm::ListWindow::onSetFocus(HWND hwnd)
{
	pImpl_->invalidateSelected();
	return 0;
}

LRESULT qm::ListWindow::onSize(UINT nFlags,
							   int cx,
							   int cy)
{
	if (pImpl_->pHeaderColumn_)
		pImpl_->layoutChildren(cx, cy);
	
#ifdef QMTOOLTIP
	if (pImpl_->hwndToolTip_) {
		TOOLINFO ti = {
			sizeof(ti),
			0,
			getHandle(),
			ListWindowImpl::ID_TOOLTIP,
		};
		pImpl_->getRect(&ti.rect);
		Window(pImpl_->hwndToolTip_).sendMessage(
			TTM_NEWTOOLRECT, 0, reinterpret_cast<LPARAM>(&ti));
	}
#endif
	
	return DefaultWindowHandler::onSize(nFlags, cx, cy);
}

LRESULT qm::ListWindow::onVScroll(UINT nCode,
								  UINT nPos,
								  HWND hwnd)
{
	SCROLLINFO si = {
		sizeof(si),
		SIF_RANGE | SIF_POS | SIF_PAGE
	};
	getScrollInfo(SB_VERT, &si);
	
	int nNewPos = si.nPos;
	bool bScroll = true;
	switch (nCode) {
	case SB_LINEUP:
		--nNewPos;
		break;
	case SB_LINEDOWN:
		++nNewPos;
		break;
	case SB_PAGEUP:
		nNewPos -= si.nPage;
		break;
	case SB_PAGEDOWN:
		nNewPos += si.nPage;
		break;
	case SB_TOP:
		nNewPos = 0;
		break;
	case SB_BOTTOM:
		nNewPos = si.nMax;
		break;
	case SB_THUMBPOSITION:
	case SB_THUMBTRACK:
		nNewPos = nPos;
		break;
	default:
		bScroll = false;
		break;
	}
	if (bScroll)
		pImpl_->scrollVertical(nNewPos);
	
	return 0;
}

LRESULT qm::ListWindow::onViewModelItemAdded(WPARAM wParam,
											 LPARAM lParam)
{
	pImpl_->handleRefreshMessage();
	return 0;
}

LRESULT qm::ListWindow::onViewModelItemRemoved(WPARAM wParam,
											   LPARAM lParam)
{
	pImpl_->handleRefreshMessage();
	return 0;
}

LRESULT qm::ListWindow::onViewModelItemChanged(WPARAM wParam,
											   LPARAM lParam)
{
	pImpl_->handleInvalidateMessage(static_cast<unsigned int>(wParam));
	return 0;
}

bool qm::ListWindow::isShow() const
{
	return (getStyle() & WS_VISIBLE) != 0;
}

bool qm::ListWindow::isActive() const
{
	return hasFocus();
}

void qm::ListWindow::setActive()
{
	setFocus();
}


/****************************************************************************
 *
 * ListHeaderColumnImpl
 *
 */

struct qm::ListHeaderColumnImpl :
	public NotifyHandler,
	public DefaultViewModelHandler
{
public:
	void updateSortIcon();

public:
	virtual LRESULT onNotify(NMHDR* pnmhdr,
							 bool* pbHandled);

private:
	LRESULT onItemChanging(NMHDR* pnmhdr,
						   bool* pbHandled);
	LRESULT onItemClick(NMHDR* pnmhdr,
						bool* pbHandled);

public:
	virtual void sorted(const ViewModelEvent& event);

public:
	ListHeaderColumn* pThis_;
	ListWindow* pListWindow_;
	UINT nId_;
	Profile* pProfile_;
	ViewModel* pViewModel_;
	
	bool bShow_;
};

void qm::ListHeaderColumnImpl::updateSortIcon()
{
	assert(pViewModel_);
	
	HDITEM item = { HDI_IMAGE };
	for (int n = 0; n < Header_GetItemCount(pThis_->getHandle()); ++n)
		Header_SetItem(pThis_->getHandle(), n, &item);
	
	unsigned int nSort = pViewModel_->getSort();
	item.iImage = (nSort & ViewModel::SORT_DIRECTION_MASK) == ViewModel::SORT_ASCENDING ? 2 : 1;
	Header_SetItem(pThis_->getHandle(), nSort & ViewModel::SORT_INDEX_MASK, &item);
}

LRESULT qm::ListHeaderColumnImpl::onNotify(NMHDR* pnmhdr,
										   bool* pbHandled)
{
	BEGIN_NOTIFY_HANDLER()
		HANDLE_NOTIFY(HDN_ITEMCHANGING, nId_, onItemChanging)
		HANDLE_NOTIFY(HDN_ITEMCLICK, nId_, onItemClick)
	END_NOTIFY_HANDLER()
	return NotifyHandler::onNotify(pnmhdr, pbHandled);
}

LRESULT qm::ListHeaderColumnImpl::onItemChanging(NMHDR* pnmhdr,
												 bool* pbHandled)
{
	NMHEADER* pHeader = reinterpret_cast<NMHEADER*>(pnmhdr);
	if (pHeader->pitem->mask & HDI_WIDTH) {
		pViewModel_->getColumn(pHeader->iItem).setWidth(pHeader->pitem->cxy);
		pListWindow_->refresh();
	}
	
	*pbHandled = true;
	
	return 0;
}

LRESULT qm::ListHeaderColumnImpl::onItemClick(NMHDR* pnmhdr,
											  bool* pbHandled)
{
	NMHEADER* pHeader = reinterpret_cast<NMHEADER*>(pnmhdr);
	
	unsigned int nColumn = pHeader->iItem;
	unsigned int nSort = pViewModel_->getSort();
	if ((nSort & ViewModel::SORT_INDEX_MASK) == nColumn) {
		bool bAscending = (nSort & ViewModel::SORT_DIRECTION_MASK) ==
			ViewModel::SORT_ASCENDING;
		pViewModel_->setSort(bAscending ? ViewModel::SORT_DESCENDING :
			ViewModel::SORT_ASCENDING, ViewModel::SORT_DIRECTION_MASK);
	}
	else {
		pViewModel_->setSort(nColumn | ViewModel::SORT_ASCENDING,
			ViewModel::SORT_INDEX_MASK | ViewModel::SORT_DIRECTION_MASK);
	}
	return 0;
}

void qm::ListHeaderColumnImpl::sorted(const ViewModelEvent& event)
{
	assert(event.getViewModel() == pViewModel_);
	updateSortIcon();
}


/****************************************************************************
 *
 * ListHeaderColumn
 *
 */

qm::ListHeaderColumn::ListHeaderColumn(ListWindow* pListWindow,
									   Profile* pProfile) :
	WindowBase(true),
	pImpl_(0)
{
	pImpl_ = new ListHeaderColumnImpl();
	pImpl_->pThis_ = this;
	pImpl_->pListWindow_ = pListWindow;
	pImpl_->nId_ = 0;
	pImpl_->pProfile_ = pProfile;
	pImpl_->pViewModel_ = 0;
	pImpl_->bShow_ = pProfile->getInt(L"ListWindow", L"ShowHeaderColumn") != 0;
	
	setWindowHandler(this, false);
	
	pListWindow->addNotifyHandler(pImpl_);
}

qm::ListHeaderColumn::~ListHeaderColumn()
{
	delete pImpl_;
}

int qm::ListHeaderColumn::getWidth() const
{
	int nWidth = 0;
	if (pImpl_->pViewModel_) {
		for (unsigned int n = 0; n < pImpl_->pViewModel_->getColumnCount(); ++n)
			nWidth += pImpl_->pViewModel_->getColumn(n).getWidth();
	}
	return nWidth;
}

int qm::ListHeaderColumn::getHeight() const
{
	if (getStyle() & WS_VISIBLE) {
		RECT rect;
		getWindowRect(&rect);
		return rect.bottom - rect.top;
	}
	else {
		return 0;
	}
}

int qm::ListHeaderColumn::getPreferredHeight(int nWidth,
											 int nHeight) const
{
	RECT rect = { 0, 0, nWidth, nHeight };
	WINDOWPOS wp;
	HDLAYOUT layout = {
		&rect,
		&wp
	};
	Header_Layout(getHandle(), &layout);
	return wp.cy;
}

void qm::ListHeaderColumn::setViewModel(ViewModel* pViewModel)
{
	for (int n = Header_GetItemCount(getHandle()) - 1; n >= 0; --n)
		Header_DeleteItem(getHandle(), n);
	if (pImpl_->pViewModel_)
		pImpl_->pViewModel_->removeViewModelHandler(pImpl_);
	
	if (pViewModel) {
		for (unsigned int nColumn = 0; nColumn < pViewModel->getColumnCount(); ++nColumn) {
			ViewColumn& column = pViewModel->getColumn(nColumn);
			W2T(column.getTitle(), ptszTitle);
			HDITEM item = {
				HDI_TEXT | HDI_WIDTH | HDI_FORMAT | HDI_IMAGE,
				column.getWidth(),
				const_cast<LPTSTR>(ptszTitle),
				0,
				0,
				HDF_STRING | HDF_IMAGE | (column.isFlag(ViewColumn::FLAG_RIGHTALIGN) ? HDF_RIGHT : HDF_LEFT),
				0,
				0
			};
			Header_InsertItem(getHandle(), nColumn, &item);
		}
		if (pImpl_->bShow_)
			showWindow(SW_SHOW);
	}
	else {
		showWindow(SW_HIDE);
	}
	
	pImpl_->pViewModel_ = pViewModel;
	
	if (pImpl_->pViewModel_) {
		pImpl_->pViewModel_->addViewModelHandler(pImpl_);
		pImpl_->updateSortIcon();
	}
}

bool qm::ListHeaderColumn::isShow() const
{
	return pImpl_->bShow_;
}

void qm::ListHeaderColumn::setShow(bool bShow)
{
	if (bShow) {
		if (pImpl_->pViewModel_)
			showWindow(SW_SHOW);
	}
	else {
		showWindow(SW_HIDE);
	}
	pImpl_->bShow_ = bShow;
}

void qm::ListHeaderColumn::save() const
{
	pImpl_->pProfile_->setInt(L"ListWindow", L"ShowHeaderColumn", pImpl_->bShow_ ? 1 : 0);
}

wstring_ptr qm::ListHeaderColumn::getSuperClass()
{
	return allocWString(WC_HEADERW);
}

bool qm::ListHeaderColumn::preCreateWindow(CREATESTRUCT* pCreateStruct)
{
	pCreateStruct->style |= HDS_FULLDRAG | HDS_BUTTONS | HDS_HORZ;
	return DefaultWindowHandler::preCreateWindow(pCreateStruct);
}

LRESULT qm::ListHeaderColumn::windowProc(UINT uMsg,
										 WPARAM wParam,
										 LPARAM lParam)
{
	BEGIN_MESSAGE_HANDLER()
		HANDLE_CREATE()
		HANDLE_DESTROY()
	END_MESSAGE_HANDLER()
	return DefaultWindowHandler::windowProc(uMsg, wParam, lParam);
}

LRESULT qm::ListHeaderColumn::onCreate(CREATESTRUCT* pCreateStruct)
{
	if (DefaultWindowHandler::onCreate(pCreateStruct) == -1)
		return -1;
	
	pImpl_->nId_ = getId();
	
	HIMAGELIST hImageList = ImageList_LoadImage(getResourceHandle(),
		MAKEINTRESOURCE(IDB_HEADER), 16, 0, CLR_DEFAULT, IMAGE_BITMAP, 0);
	Header_SetImageList(getHandle(), hImageList);
	
	return 0;
}

LRESULT qm::ListHeaderColumn::onDestroy()
{
	HIMAGELIST hImageList = Header_SetImageList(getHandle(), 0);
	ImageList_Destroy(hImageList);
	
	pImpl_->pListWindow_->removeNotifyHandler(pImpl_);
	
	return DefaultWindowHandler::onDestroy();
}
