/*
 * $Id$
 *
 * Copyright(C) 1998-2004 Satoshi Nakamura
 * All rights reserved.
 *
 */

#include <qsinit.h>

#include <tchar.h>

#include "autocomplete.h"

using namespace qm;
using namespace qs;


/****************************************************************************
 *
 * AutoComplete
 *
 */

qm::AutoComplete::AutoComplete(HWND hwnd,
							   WindowBase* pParent,
							   AutoCompleteCallback* pCallback)
{
	new AutoCompleteEditSubclassWindow(hwnd, pParent, pCallback);
}

qm::AutoComplete::~AutoComplete()
{
}


/****************************************************************************
 *
 * AutoCompleteCallback
 *
 */

qm::AutoCompleteCallback::~AutoCompleteCallback()
{
}


/****************************************************************************
 *
 * AutoCompleteEditSubclassWindow
 *
 */

qm::AutoCompleteEditSubclassWindow::AutoCompleteEditSubclassWindow(HWND hwnd,
																   WindowBase* pParent,
																   AutoCompleteCallback* pCallback) :
	WindowBase(true),
	pParent_(pParent),
	pCallback_(pCallback),
	nId_(0),
	nTimerId_(0),
	pListWindow_(0)
{
	setWindowHandler(this, false);
	
	nId_ = Window(hwnd).getWindowLong(GWL_ID);
	
	subclassWindow(hwnd);
	
	pParent_->addCommandHandler(this);
}

qm::AutoCompleteEditSubclassWindow::~AutoCompleteEditSubclassWindow()
{
}

void qm::AutoCompleteEditSubclassWindow::fill(const WCHAR* pwszText)
{
	assert(pListWindow_);
	hideCandidates();
	
	sendMessage(EM_SETSEL, input_.first, input_.first + input_.second);
	W2T(pwszText, ptszText);
	sendMessage(EM_REPLACESEL, TRUE, reinterpret_cast<LPARAM>(ptszText));
}

bool qm::AutoCompleteEditSubclassWindow::preTranslateAccelerator(const MSG& msg)
{
	if (msg.message == WM_KEYDOWN && pListWindow_ && pListWindow_->isVisible()) {
		switch (msg.wParam) {
		case VK_UP:
			pListWindow_->select(AutoCompleteListWindow::SELECT_PREV);
			return true;
		case VK_DOWN:
			pListWindow_->select(AutoCompleteListWindow::SELECT_NEXT);
			return true;
		case VK_PRIOR:
			pListWindow_->select(AutoCompleteListWindow::SELECT_PREVPAGE);
			return true;
		case VK_NEXT:
			pListWindow_->select(AutoCompleteListWindow::SELECT_NEXTPAGE);
			return true;
		case VK_RETURN:
			pListWindow_->fill();
			return true;
		case VK_ESCAPE:
			hideCandidates();
			return true;
		default:
			break;
		}
	}
	return DefaultWindowHandler::preTranslateAccelerator(msg);
}

LRESULT qm::AutoCompleteEditSubclassWindow::windowProc(UINT uMsg,
													   WPARAM wParam,
													   LPARAM lParam)
{
	BEGIN_MESSAGE_HANDLER()
		HANDLE_DESTROY()
		HANDLE_KILLFOCUS()
		HANDLE_TIMER()
	END_MESSAGE_HANDLER()
	return DefaultWindowHandler::windowProc(uMsg, wParam, lParam);
}

LRESULT qm::AutoCompleteEditSubclassWindow::onDestroy()
{
	if (pListWindow_)
		pListWindow_->destroyWindow();
	
	pParent_->removeCommandHandler(this);
	
	return DefaultWindowHandler::onDestroy();
}

LRESULT qm::AutoCompleteEditSubclassWindow::onKillFocus(HWND hwnd)
{
	if (nTimerId_ != 0)
		killTimer(nTimerId_);
	
	hideCandidates();
	
	return DefaultWindowHandler::onKillFocus(hwnd);
}

LRESULT qm::AutoCompleteEditSubclassWindow::onTimer(UINT nId)
{
	if (nId == nTimerId_) {
		killTimer(nTimerId_);
		nTimerId_ = 0;
		
		showCandidates();
	}
	return DefaultWindowHandler::onTimer(nId);
}

LRESULT qm::AutoCompleteEditSubclassWindow::onCommand(WORD nCode,
													  WORD nId)
{
	BEGIN_COMMAND_HANDLER()
		HANDLE_COMMAND_ID_CODE(nId_, EN_CHANGE, onChange)
	END_COMMAND_HANDLER()
	return 1;
}

LRESULT qm::AutoCompleteEditSubclassWindow::onChange()
{
	hideCandidates();
	if (hasFocus())
		nTimerId_ = setTimer(TIMER_ID, TIMER_INTERVAL);
	return 1;
}

void qm::AutoCompleteEditSubclassWindow::showCandidates()
{
	DWORD dwStart = 0;
	DWORD dwEnd = 0;
	sendMessage(EM_GETSEL, reinterpret_cast<WPARAM>(&dwStart),
		reinterpret_cast<LPARAM>(&dwEnd));
	if (dwStart != dwEnd)
		return;
	
	wstring_ptr wstrText(getWindowText());
	if (!wstrText.get() || !*wstrText.get())
		return;
	
	input_ = pCallback_->getInput(wstrText.get(), dwStart);
	if (input_.second == 0)
		return;
	wstrText = allocWString(wstrText.get() + input_.first, input_.second);
	
	AutoCompleteCallback::CandidateList listCandidate;
	StringListFree<AutoCompleteCallback::CandidateList> free(listCandidate);
	pCallback_->getCandidates(wstrText.get(), &listCandidate);
	if (!listCandidate.empty())
		showCandidates(listCandidate, wstrText.get());
}

void qm::AutoCompleteEditSubclassWindow::showCandidates(AutoCompleteCallback::CandidateList& listCandidate,
														const WCHAR* pwszInput)
{
	RECT rect;
	getWindowRect(&rect);
	
	if (!pListWindow_) {
		std::auto_ptr<AutoCompleteListWindow> pListWindow(
			new AutoCompleteListWindow(this, getFont()));
#ifndef _WIN32_WCE
		DWORD dwExStyle = WS_EX_TOPMOST | WS_EX_TOOLWINDOW;
#else
		DWORD dwExStyle = WS_EX_TOPMOST | WS_EX_NOACTIVATE;
#endif
		if (!pListWindow->create(L"QmAutoCompleteListWindow", 0,
			WS_POPUP | WS_BORDER | WS_CLIPCHILDREN | WS_CLIPSIBLINGS | WS_VSCROLL,
			rect.left + 5, rect.bottom - 2, rect.right - rect.left - 10, 200,
			0, dwExStyle, 0, 0, 0))
			return;
		pListWindow_ = pListWindow.release();
	}
	pListWindow_->setWindowPos(0, rect.left + 5, rect.bottom - 2,
		rect.right - rect.left - 10, 200, SWP_NOZORDER | SWP_NOACTIVATE);
	pListWindow_->showCandidates(listCandidate, pwszInput);
}

void qm::AutoCompleteEditSubclassWindow::hideCandidates()
{
	if (pListWindow_)
		pListWindow_->showWindow(SW_HIDE);
}


/****************************************************************************
 *
 * AutoCompleteListWindow
 *
 */

qm::AutoCompleteListWindow::AutoCompleteListWindow(AutoCompleteEditSubclassWindow* pEditWindow,
												   HFONT hfont) :
	WindowBase(true),
	pEditWindow_(pEditWindow),
	nSelect_(0),
	nLineHeight_(20),
	hfont_(0),
	hfontBold_(0)
{
	LOGFONT lf;
	::GetObject(hfont, sizeof(lf), &lf);
	hfont_ = ::CreateFontIndirect(&lf);
	lf.lfWeight = FW_BOLD;
	hfontBold_ = ::CreateFontIndirect(&lf);
	
	ClientDeviceContext dc(getHandle());
	ObjectSelector<HFONT> selector(dc, hfont_);
	TEXTMETRIC tm;
	dc.getTextMetrics(&tm);
	nLineHeight_ = tm.tmHeight + tm.tmExternalLeading + 2;
	
	setWindowHandler(this, false);
}

qm::AutoCompleteListWindow::~AutoCompleteListWindow()
{
	std::for_each(listCandidate_.begin(), listCandidate_.end(), qs::string_free<WSTRING>());
}

void qm::AutoCompleteListWindow::showCandidates(CandidateList& listCandidate,
												const WCHAR* pwszInput)
{
	listCandidate_.swap(listCandidate);
	wstrInput_ = allocWString(pwszInput);
	nSelect_ = 0;
	
	setScrollPos(SB_VERT, 0);
	
	unsigned int nCount = listCandidate_.size();
	if (nCount > 10)
		nCount = 10;
	RECT rect;
	getWindowRect(&rect);
	setWindowPos(0, 0, 0, rect.right - rect.left, nLineHeight_*nCount + 2,
		SWP_NOMOVE | SWP_NOACTIVATE | SWP_NOZORDER);
	
	invalidate();
	updateScrollBar();
	
	showWindow(SW_SHOWNA);
}

void qm::AutoCompleteListWindow::select(Select select)
{
	if (listCandidate_.empty())
		return;
	
	RECT rect;
	getClientRect(&rect);
	int nLineInPage = (rect.bottom - rect.top)/nLineHeight_;
	
	switch (select) {
	case SELECT_PREV:
		if (nSelect_ == 0)
			nSelect_ = listCandidate_.size() - 1;
		else
			--nSelect_;
		break;
	case SELECT_NEXT:
		if (nSelect_ == static_cast<int>(listCandidate_.size()) - 1)
			nSelect_ = 0;
		else
			++nSelect_;
		break;
	case SELECT_PREVPAGE:
		nSelect_ -= nLineInPage;
		if (nSelect_ < 0)
			nSelect_ = 0;
		break;
	case SELECT_NEXTPAGE:
		nSelect_ += nLineInPage;
		if (nSelect_ >= static_cast<int>(listCandidate_.size()))
			nSelect_ = listCandidate_.size() - 1;
		break;
	default:
		break;
	}
	
	int nScrollPos = getScrollPos(SB_VERT);
	if (nSelect_ < nScrollPos)
		scroll(nSelect_);
	else if (nSelect_ >= nScrollPos + nLineInPage)
		scroll(nSelect_	 - nLineInPage + 1);
	else
		invalidate();
}

void qm::AutoCompleteListWindow::fill()
{
	if (0 <= nSelect_ && nSelect_ < static_cast<int>(listCandidate_.size()))
		pEditWindow_->fill(listCandidate_[nSelect_]);
}

LRESULT qm::AutoCompleteListWindow::windowProc(UINT uMsg,
											   WPARAM wParam,
											   LPARAM lParam)
{
	BEGIN_MESSAGE_HANDLER()
		HANDLE_ACTIVATE()
		HANDLE_CREATE()
		HANDLE_DESTROY()
		HANDLE_ERASEBKGND()
		HANDLE_LBUTTONUP()
#ifndef _WIN32_WCE
		HANDLE_MOUSEACTIVATE()
#endif
		HANDLE_MOUSEMOVE()
#if !defined _WIN32_WCE || _WIN32_WCE >= 211
		HANDLE_MOUSEWHEEL()
#endif
		HANDLE_PAINT()
		HANDLE_SIZE()
		HANDLE_VSCROLL()
#ifndef _WIN32_WCE
		HANDLE_WINDOWPOSCHANGING()
#endif
	END_MESSAGE_HANDLER()
	return DefaultWindowHandler::windowProc(uMsg, wParam, lParam);
}

LRESULT qm::AutoCompleteListWindow::onActivate(UINT nFlags,
											   HWND hwnd,
											   bool bMinimized)
{
	return 0;
}

LRESULT qm::AutoCompleteListWindow::onCreate(CREATESTRUCT* pCreateStruct)
{
	if (DefaultWindowHandler::onCreate(pCreateStruct) == -1)
		return -1;
	
	return 0;
}

LRESULT qm::AutoCompleteListWindow::onDestroy()
{
	::DeleteObject(hfont_);
	::DeleteObject(hfontBold_);
	
	return DefaultWindowHandler::onDestroy();
}

LRESULT qm::AutoCompleteListWindow::onEraseBkgnd(HDC hdc)
{
	return 0;
}

LRESULT qm::AutoCompleteListWindow::onLButtonUp(UINT nFlags,
												const POINT& pt)
{
	int nItem = getItemFromPoint(pt);
	if (0 <= nItem && nItem < static_cast<int>(listCandidate_.size()))
		pEditWindow_->fill(listCandidate_[nItem]);
	return DefaultWindowHandler::onLButtonUp(nFlags, pt);
}

#ifndef _WIN32_WCE
LRESULT qm::AutoCompleteListWindow::onMouseActivate(HWND hwnd,
													UINT nHitTest,
													UINT uMsg)
{
	return MA_NOACTIVATE;
}
#endif

LRESULT qm::AutoCompleteListWindow::onMouseMove(UINT nFlags,
												const POINT& pt)
{
	int nItem = getItemFromPoint(pt);
	if (0 <= nItem && nItem < static_cast<int>(listCandidate_.size()) && nItem != nSelect_) {
		nSelect_ = nItem;
		invalidate();
	}
	return DefaultWindowHandler::onMouseMove(nFlags, pt);
}

#if !defined _WIN32_WCE || _WIN32_WCE >= 211
LRESULT qm::AutoCompleteListWindow::onMouseWheel(UINT nFlags,
												 short nDelta,
												 const POINT& pt)
{
#ifdef _WIN32_WCE
#	define WHEEL_DELTA 120
#endif
	scroll(getScrollPos(SB_VERT) - nDelta/WHEEL_DELTA*3);
	
	POINT ptClient = pt;
	screenToClient(&ptClient);
	int nItem = getItemFromPoint(ptClient);
	if (0 <= nItem && nItem < static_cast<int>(listCandidate_.size()) && nItem != nSelect_) {
		nSelect_ = nItem;
		invalidate();
	}
	
	return 0;
}
#endif

LRESULT qm::AutoCompleteListWindow::onPaint()
{
	PaintDeviceContext dc(getHandle());
	
	ObjectSelector<HFONT> selector(dc, hfont_);
	
	RECT rect;
	getClientRect(&rect);
	
	int y = 0;
	for (int n = getScrollPos(SB_VERT); n < static_cast<int>(listCandidate_.size()) && y < rect.bottom; ++n) {
		RECT r = {
			0,
			y,
			rect.right,
			y + nLineHeight_
		};
		paintItem(&dc, n, r, n == nSelect_);
		y += nLineHeight_;
	}
	
	RECT r = {
		0,
		y,
		rect.right,
		rect.bottom
	};
	dc.fillSolidRect(r, ::GetSysColor(COLOR_WINDOW));
	
	return 0;
}

LRESULT qm::AutoCompleteListWindow::onSize(UINT nFlags,
										   int cx,
										   int cy)
{
	updateScrollBar();
	return DefaultWindowHandler::onSize(nFlags, cx, cy);
}

LRESULT qm::AutoCompleteListWindow::onVScroll(UINT nCode,
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
		scroll(nNewPos);
	
	return 0;
}

#ifndef _WIN32_WCE
LRESULT qm::AutoCompleteListWindow::onWindowPosChanging(WINDOWPOS* pWindowPos)
{
	pWindowPos->flags |= SWP_NOACTIVATE;
	return DefaultWindowHandler::onWindowPosChanging(pWindowPos);
}
#endif

int qm::AutoCompleteListWindow::getItemFromPoint(const POINT& pt) const
{
	return pt.y/nLineHeight_ + getScrollPos(SB_VERT);
}

void qm::AutoCompleteListWindow::paintItem(DeviceContext* pdc,
										   unsigned int n,
										   const RECT& rect,
										   bool bSelected)
{
	const WCHAR* pwsz = listCandidate_[n];
	
	if (bSelected) {
		pdc->setTextColor(::GetSysColor(COLOR_HIGHLIGHTTEXT));
		pdc->setBkColor(::GetSysColor(COLOR_HIGHLIGHT));
	}
	else {
		pdc->setTextColor(::GetSysColor(COLOR_WINDOWTEXT));
		pdc->setBkColor(::GetSysColor(COLOR_WINDOW));
	}
	pdc->fillSolidRect(rect, pdc->getBkColor());
	
	RECT r = {
		rect.left + 2,
		rect.top,
		rect.right,
		rect.bottom
	};
	
	size_t nInputLen = wcslen(wstrInput_.get());
	const WCHAR* pBegin = pwsz;
	const WCHAR* p = pwsz;
	while (*p) {
		if (_wcsnicmp(p, wstrInput_.get(), nInputLen) == 0) {
			paintText(pdc, pBegin, p - pBegin, hfont_, &r);
			paintText(pdc, p, nInputLen, hfontBold_, &r);
			p += nInputLen;
			pBegin = p;
		}
		else {
			++p;
		}
	}
	paintText(pdc, pBegin, p - pBegin, hfont_, &r);
}

void qm::AutoCompleteListWindow::updateScrollBar()
{
	RECT rect;
	getClientRect(&rect);
	
	unsigned int nCount = listCandidate_.size();
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
	setScrollInfo(SB_VERT, si);
}

void qm::AutoCompleteListWindow::scroll(int nPos)
{
	SCROLLINFO si = {
		sizeof(si),
		SIF_RANGE | SIF_POS | SIF_PAGE
	};
	getScrollInfo(SB_VERT, &si);
	
	if (nPos < 0)
		nPos = 0;
	else if (nPos > si.nMax - static_cast<int>(si.nPage) + 1)
		nPos = si.nMax - si.nPage + 1;
	
	if (nPos != si.nPos) {
		setScrollPos(SB_VERT, nPos);
		invalidate();
	}
}

void qm::AutoCompleteListWindow::paintText(DeviceContext* pdc,
										   const WCHAR* pwsz,
										   size_t nLen,
										   HFONT hfont,
										   RECT* pRect)
{
	if (nLen == 0)
		return;
	
	ObjectSelector<HFONT> selector(*pdc, hfont);
	pdc->extTextOut(pRect->left, pRect->top, ETO_CLIPPED, *pRect, pwsz, nLen, 0);
	
	SIZE size;
	pdc->getTextExtent(pwsz, nLen, &size);
	pRect->left += size.cx;
}
