/*
 * $Id$
 *
 * Copyright(C) 1998-2005 Satoshi Nakamura
 * All rights reserved.
 *
 */

#include <qswindow.h>
#include <qsstl.h>

#pragma warning(disable:4786)

#include <vector>

using namespace qs;


/****************************************************************************
 *
 * SplitterWindowImpl
 *
 */

struct qs::SplitterWindowImpl
{
public:
	enum {
		MIN_SIZE		= 10
	};
	
public:
	typedef std::vector<std::vector<std::pair<Window*, bool> > > WindowList;

public:
	void dragMouse(int x,
				   int y);
	
public:
	SplitterWindow* pThis_;
	int nColumn_;
	int nRow_;
	int nBorderWidth_;
	SplitterWindowHandler* pHandler_;
	WindowList listWindow_;
	bool bDragging_;
};

void qs::SplitterWindowImpl::dragMouse(int x,
									   int y)
{
	assert((nColumn_ == 2 && nRow_ == 1) || (nColumn_ == 1 && nRow_ == 2));
	if (nColumn_ == 2)
		pThis_->setColumnWidth(0, x);
	else
		pThis_->setRowHeight(0, y);
}


/****************************************************************************
 *
 * SplitterWindow
 *
 */

qs::SplitterWindow::SplitterWindow(int nColumn,
								   int nRow,
								   bool bDeleteThis,
								   SplitterWindowHandler* pHandler) :
	WindowBase(bDeleteThis),
	pImpl_(0)
{
	assert((nColumn == 2 && nRow == 1) || (nColumn == 1 && nRow == 2));
	
	pImpl_ = new SplitterWindowImpl();
	
	pImpl_->pThis_ = this;
	pImpl_->nColumn_ = nColumn;
	pImpl_->nRow_ = nRow;
	pImpl_->nBorderWidth_ = ::GetSystemMetrics(nColumn == 2 ? SM_CXEDGE : SM_CYEDGE);
	pImpl_->pHandler_ = pHandler;
	pImpl_->bDragging_ = false;
	
	pImpl_->listWindow_.resize(pImpl_->nColumn_);
	SplitterWindowImpl::WindowList::iterator it = pImpl_->listWindow_.begin();
	while (it != pImpl_->listWindow_.end()) {
		(*it).resize(pImpl_->nRow_);
		++it;
	}
	
	setWindowHandler(this, false);
}

qs::SplitterWindow::~SplitterWindow()
{
	delete pImpl_;
	pImpl_ = 0;
}

void qs::SplitterWindow::add(int nColumn,
							 int nRow,
							 Window* pWindow)
{
	assert(0 <= nColumn && nColumn < pImpl_->nColumn_ && 0 <= nRow && nRow < pImpl_->nRow_);
	
	pImpl_->listWindow_[nColumn][nRow] = std::make_pair(pWindow, true);
	pWindow->setParent(getHandle());
	
	RECT rectClient;
	getClientRect(&rectClient);
	int nWidth = (rectClient.right - pImpl_->nBorderWidth_*(pImpl_->nColumn_ - 1))/pImpl_->nColumn_;
	int nHeight = (rectClient.bottom - pImpl_->nBorderWidth_*(pImpl_->nRow_ - 1))/pImpl_->nRow_;
	pWindow->setWindowPos(0, (nWidth + pImpl_->nBorderWidth_)*nColumn,
		(nHeight + pImpl_->nBorderWidth_)*nRow, nWidth, nHeight,
		SWP_NOZORDER | SWP_NOACTIVATE);
}

void qs::SplitterWindow::setPane(int nColumn,
								 int nRow,
								 Window* pWindow)
{
	assert(0 <= nColumn && nColumn < pImpl_->nColumn_ && 0 <= nRow && nRow < pImpl_->nRow_);
	
	Window* pWindowOrg = pImpl_->listWindow_[nColumn][nRow].first;
	pImpl_->listWindow_[nColumn][nRow].first = pWindow;
	pWindow->setParent(getHandle());
	
	RECT rect;
	getWindowRect(&rect);
	RECT rectOrg;
	pWindowOrg->getWindowRect(&rectOrg);
	pWindow->setWindowPos(0, rectOrg.left - rect.left, rectOrg.top - rect.top,
		rectOrg.right - rectOrg.left, rectOrg.bottom - rectOrg.top,
		SWP_NOZORDER | SWP_NOACTIVATE);
}

void qs::SplitterWindow::showPane(int nColumn,
								  int nRow,
								  bool bShow)
{
	assert(0 <= nColumn && nColumn < pImpl_->nColumn_ && 0 <= nRow && nRow < pImpl_->nRow_);
	
	if (pImpl_->listWindow_[nColumn][nRow].second != bShow) {
		pImpl_->listWindow_[nColumn][nRow].second = bShow;
		pImpl_->listWindow_[nColumn][nRow].first->showWindow(bShow);
		RECT rect;
		getClientRect(&rect);
		sendMessage(WM_SIZE, 0, MAKELPARAM(rect.right, rect.bottom));
	}
}

bool qs::SplitterWindow::isShowPane(int nColumn,
									int nRow)
{
	assert(0 <= nColumn && nColumn < pImpl_->nColumn_ && 0 <= nRow && nRow < pImpl_->nRow_);
	return pImpl_->listWindow_[nColumn][nRow].second;
}

int qs::SplitterWindow::getColumnWidth(int nColumn)
{
	assert(0 <= nColumn && nColumn < pImpl_->nColumn_);
	
	RECT rect;
	pImpl_->listWindow_[nColumn][0].first->getWindowRect(&rect);
	return rect.right - rect.left;
}

void qs::SplitterWindow::setColumnWidth(int nColumn,
										int nWidth)
{
	assert(pImpl_->nColumn_);
	assert(nColumn == 0);
	
	SplitterWindowImpl::WindowList& l = pImpl_->listWindow_;
	
	RECT rectClient;
	getClientRect(&rectClient);
	
	if (l[1][0].second) {
		if (nWidth < SplitterWindowImpl::MIN_SIZE)
			nWidth = SplitterWindowImpl::MIN_SIZE;
		else if (nWidth + SplitterWindowImpl::MIN_SIZE > rectClient.right)
			nWidth = rectClient.right - SplitterWindowImpl::MIN_SIZE;
	}
	else {
		nWidth = rectClient.right;
	}
	
	for (int nRow = 0; nRow < pImpl_->nRow_; ++nRow) {
		RECT rect;
		assert(l[nColumn][nRow].first);
		l[nColumn][nRow].first->getWindowRect(&rect);
		l[nColumn][nRow].first->setWindowPos(0, 0, 0, nWidth, rect.bottom - rect.top,
			SWP_NOZORDER | SWP_NOACTIVATE | SWP_NOMOVE);
		
		RECT rectWindow;
		getWindowRect(&rectWindow);
		assert(l[1][nRow].first);
		l[1][nRow].first->getWindowRect(&rect);
		if (l[0][nRow].second) {
			l[1][nRow].first->setWindowPos(0, nWidth + pImpl_->nBorderWidth_,
				rect.top - rectWindow.top, rectClient.right - (nWidth + pImpl_->nBorderWidth_),
				rect.bottom - rect.top, SWP_NOZORDER | SWP_NOACTIVATE);
		}
		else {
			l[1][nRow].first->setWindowPos(0, 0, rect.top - rectWindow.top,
				rectClient.right, rect.bottom - rect.top, SWP_NOZORDER | SWP_NOACTIVATE);
		}
	}
	
	if (pImpl_->pHandler_)
		pImpl_->pHandler_->sizeChanged(SplitterWindowEvent(this));
}

int qs::SplitterWindow::getRowHeight(int nRow)
{
	assert(0 <= nRow && nRow < pImpl_->nRow_);
	
	RECT rect;
	pImpl_->listWindow_[0][nRow].first->getWindowRect(&rect);
	return rect.bottom - rect.top;
}

void qs::SplitterWindow::setRowHeight(int nRow,
									  int nHeight)
{
	assert(pImpl_->nRow_ == 2);
	assert(nRow == 0);
	
	SplitterWindowImpl::WindowList& l = pImpl_->listWindow_;
	
	RECT rectClient;
	getClientRect(&rectClient);
	
	if (l[0][1].second) {
		if (nHeight < SplitterWindowImpl::MIN_SIZE)
			nHeight = SplitterWindowImpl::MIN_SIZE;
		else if (nHeight + SplitterWindowImpl::MIN_SIZE > rectClient.bottom)
			nHeight = rectClient.bottom - SplitterWindowImpl::MIN_SIZE;
	}
	else {
		nHeight = rectClient.bottom;
	}
	
	for (int nColumn = 0; nColumn < pImpl_->nColumn_; ++nColumn) {
		RECT rect;
		assert(l[nColumn][nRow].first);
		l[nColumn][nRow].first->getWindowRect(&rect);
		l[nColumn][nRow].first->setWindowPos(0, 0, 0,
			rect.right - rect.left, nHeight, SWP_NOZORDER | SWP_NOACTIVATE | SWP_NOMOVE);
		
		RECT rectWindow;
		getWindowRect(&rectWindow);
		assert(l[nColumn][1].first);
		l[nColumn][1].first->getWindowRect(&rect);
		if (l[nColumn][0].second) {
			l[nColumn][1].first->setWindowPos(0, rect.left - rectWindow.left,
				nHeight + pImpl_->nBorderWidth_, rect.right - rect.left,
				rectClient.bottom - (nHeight + pImpl_->nBorderWidth_),
				SWP_NOZORDER | SWP_NOACTIVATE);
		}
		else {
			l[nColumn][1].first->setWindowPos(0, rect.left - rectWindow.left,
				0, rect.right - rect.left, rectClient.bottom, SWP_NOZORDER | SWP_NOACTIVATE);
		}
	}
	
	if (pImpl_->pHandler_)
		pImpl_->pHandler_->sizeChanged(SplitterWindowEvent(this));
}

void qs::SplitterWindow::getWindowClass(WNDCLASS* pwc)
{
	DefaultWindowHandler::getWindowClass(pwc);
	
	pwc->hbrBackground = reinterpret_cast<HBRUSH>(COLOR_3DFACE + 1);
#ifndef _WIN32_WCE
	assert(pImpl_->nColumn_ == 2 || pImpl_->nRow_ == 2);
	pwc->hCursor = ::LoadCursor(0, pImpl_->nColumn_ == 2 ? IDC_SIZEWE : IDC_SIZENS);
#endif
}

LRESULT qs::SplitterWindow::windowProc(UINT uMsg,
									   WPARAM wParam,
									   LPARAM lParam)
{
	BEGIN_MESSAGE_HANDLER()
		HANDLE_LBUTTONDOWN()
		HANDLE_LBUTTONUP()
		HANDLE_MOUSEMOVE()
		HANDLE_SIZE()
	END_MESSAGE_HANDLER()
	return DefaultWindowHandler::windowProc(uMsg, wParam, lParam);
}

LRESULT qs::SplitterWindow::onLButtonDown(UINT nFlags,
										  const POINT& pt)
{
	pImpl_->bDragging_ = true;
	setCapture();
	return DefaultWindowHandler::onLButtonDown(nFlags, pt);
}

LRESULT qs::SplitterWindow::onLButtonUp(UINT nFlags,
										const POINT& pt)
{
	if (pImpl_->bDragging_) {
		pImpl_->dragMouse(pt.x, pt.y);
		releaseCapture();
		pImpl_->bDragging_ = false;
	}
	return DefaultWindowHandler::onLButtonUp(nFlags, pt);
}

LRESULT qs::SplitterWindow::onMouseMove(UINT nFlags,
										const POINT& pt)
{
	if (pImpl_->bDragging_)
		pImpl_->dragMouse(pt.x, pt.y);
	return DefaultWindowHandler::onMouseMove(nFlags, pt);
}

LRESULT qs::SplitterWindow::onSize(UINT nFlags,
								   int cx,
								   int cy)
{
	SplitterWindowImpl::WindowList& l = pImpl_->listWindow_;
	if (l[0][0].first) {
		if (pImpl_->nColumn_ == 2) {
			RECT rect;
			for (int n = 0; n < pImpl_->nColumn_; ++n) {
				l[n][0].first->getWindowRect(&rect);
				l[n][0].first->setWindowPos(0, 0, 0, rect.right - rect.left, cy,
					SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE);
			}
			
			l[0][0].first->getWindowRect(&rect);
			if (cx < rect.right - rect.left)
				setColumnWidth(0, cx);
			else
				setColumnWidth(0, rect.right - rect.left);
		}
		else if (pImpl_->nRow_ == 2) {
			RECT rect;
			for (int n = 0; n < pImpl_->nRow_; ++n) {
				l[0][n].first->getWindowRect(&rect);
				l[0][n].first->setWindowPos(0, 0, 0, cx, rect.bottom - rect.top,
					SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE);
			}
			
			l[0][0].first->getWindowRect(&rect);
			if (cy < rect.bottom - rect.top)
				setRowHeight(0, cy);
			else
				setRowHeight(0, rect.bottom - rect.top);
		}
		else {
			assert(false);
		}
	}
	return DefaultWindowHandler::onSize(nFlags, cx, cy);
}


/****************************************************************************
 *
 * SplitterWindowHandler
 *
 */

qs::SplitterWindowHandler::~SplitterWindowHandler()
{
}


/****************************************************************************
 *
 * SplitterWindowEvent
 *
 */

qs::SplitterWindowEvent::SplitterWindowEvent(SplitterWindow* pSplitterWindow) :
	pSplitterWindow_(pSplitterWindow)
{
}

qs::SplitterWindowEvent::~SplitterWindowEvent()
{
}

SplitterWindow* qs::SplitterWindowEvent::getSplitterWindow() const
{
	return pSplitterWindow_;
}
