/*
 * $Id: splitterwindow.cpp,v 1.1.1.1 2003/04/29 08:07:37 snakamura Exp $
 *
 * Copyright(C) 1998-2003 Satoshi Nakamura
 * All rights reserved.
 *
 */

#include <qswindow.h>
#include <qserror.h>
#include <qsstl.h>
#include <qsnew.h>

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
	QSTATUS dragMouse(int x, int y);
	
public:
	SplitterWindow* pHandler_;
	int nColumn_;
	int nRow_;
	int nBorderWidth_;
	WindowList listWindow_;
	bool bDragging_;
};

QSTATUS qs::SplitterWindowImpl::dragMouse(int x, int y)
{
	assert((nColumn_ == 2 && nRow_ == 1) || (nColumn_ == 1 && nRow_ == 2));
	if (nColumn_ == 2)
		return pHandler_->setColumnWidth(0, x);
	else
		return pHandler_->setRowHeight(0, y);
}


/****************************************************************************
 *
 * SplitterWindow
 *
 */

qs::SplitterWindow::SplitterWindow(int nColumn, int nRow,
	bool bDeleteThis, QSTATUS* pstatus) :
	WindowBase(bDeleteThis, pstatus),
	DefaultWindowHandler(pstatus),
	pImpl_(0)
{
	assert((nColumn == 2 && nRow == 1) || (nColumn == 1 && nRow == 2));
	
	if (*pstatus != QSTATUS_SUCCESS)
		return;
	
	DECLARE_QSTATUS();
	
	status = newObject(&pImpl_);
	CHECK_QSTATUS_SET(pstatus);
	
	pImpl_->pHandler_ = this;
	pImpl_->nColumn_ = nColumn;
	pImpl_->nRow_ = nRow;
	pImpl_->nBorderWidth_ = ::GetSystemMetrics(nColumn == 2 ? SM_CXEDGE : SM_CYEDGE);
	pImpl_->bDragging_ = false;
	
	status = STLWrapper<SplitterWindowImpl::WindowList>
		(pImpl_->listWindow_).resize(pImpl_->nColumn_);
	CHECK_QSTATUS_SET(pstatus);
	SplitterWindowImpl::WindowList::iterator it = pImpl_->listWindow_.begin();
	while (it != pImpl_->listWindow_.end()) {
		status = STLWrapper<SplitterWindowImpl::WindowList::value_type>
			(*it).resize(pImpl_->nRow_);
		CHECK_QSTATUS_SET(pstatus);
		++it;
	}
	
	setWindowHandler(this, false);
}

qs::SplitterWindow::~SplitterWindow()
{
	delete pImpl_;
	pImpl_ = 0;
}

QSTATUS qs::SplitterWindow::add(int nColumn, int nRow, Window* pWindow)
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
	
	return QSTATUS_SUCCESS;
}

QSTATUS qs::SplitterWindow::setPane(int nColumn, int nRow, Window* pWindow)
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
	
	return QSTATUS_SUCCESS;
}

QSTATUS qs::SplitterWindow::showPane(int nColumn, int nRow, bool bShow)
{
	assert(0 <= nColumn && nColumn < pImpl_->nColumn_ && 0 <= nRow && nRow < pImpl_->nRow_);
	
	if (pImpl_->listWindow_[nColumn][nRow].second != bShow) {
		pImpl_->listWindow_[nColumn][nRow].second = bShow;
		pImpl_->listWindow_[nColumn][nRow].first->showWindow(bShow);
		RECT rect;
		getClientRect(&rect);
		sendMessage(WM_SIZE, 0, MAKELPARAM(rect.right, rect.bottom));
	}
	return QSTATUS_SUCCESS;
}

QSTATUS qs::SplitterWindow::isShowPane(int nColumn, int nRow, bool* pbShow)
{
	assert(0 <= nColumn && nColumn < pImpl_->nColumn_ && 0 <= nRow && nRow < pImpl_->nRow_);
	assert(pbShow);
	
	*pbShow = pImpl_->listWindow_[nColumn][nRow].second;
	
	return QSTATUS_SUCCESS;
}

QSTATUS qs::SplitterWindow::getColumnWidth(int nColumn, int* pnWidth)
{
	assert(0 <= nColumn && nColumn < pImpl_->nColumn_);
	assert(pnWidth);
	
	RECT rect;
	pImpl_->listWindow_[nColumn][0].first->getWindowRect(&rect);
	*pnWidth = rect.right - rect.left;
	
	return QSTATUS_SUCCESS;
}

QSTATUS qs::SplitterWindow::setColumnWidth(int nColumn, int nWidth)
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
	return QSTATUS_SUCCESS;
}

QSTATUS qs::SplitterWindow::getRowHeight(int nRow, int* pnHeight)
{
	assert(0 <= nRow && nRow < pImpl_->nRow_);
	assert(pnHeight);
	
	RECT rect;
	pImpl_->listWindow_[0][nRow].first->getWindowRect(&rect);
	*pnHeight = rect.bottom - rect.top;
	
	return QSTATUS_SUCCESS;
}

QSTATUS qs::SplitterWindow::setRowHeight(int nRow, int nHeight)
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
	return QSTATUS_SUCCESS;
}

QSTATUS qs::SplitterWindow::getWindowClass(WNDCLASS* pwc)
{
	DECLARE_QSTATUS();
	
	status = DefaultWindowHandler::getWindowClass(pwc);
	CHECK_QSTATUS();
	
	pwc->hbrBackground = reinterpret_cast<HBRUSH>(COLOR_3DFACE + 1);
#ifndef _WIN32_WCE
	assert(pImpl_->nColumn_ == 2 || pImpl_->nRow_ == 2);
	pwc->hCursor = ::LoadCursor(0, pImpl_->nColumn_ == 2 ? IDC_SIZEWE : IDC_SIZENS);
#endif
	
	return QSTATUS_SUCCESS;
}

LRESULT qs::SplitterWindow::windowProc(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	BEGIN_MESSAGE_HANDLER()
		HANDLE_LBUTTONDOWN()
		HANDLE_LBUTTONUP()
		HANDLE_MOUSEMOVE()
		HANDLE_SIZE()
	END_MESSAGE_HANDLER()
	return DefaultWindowHandler::windowProc(uMsg, wParam, lParam);
}

LRESULT qs::SplitterWindow::onLButtonDown(UINT nFlags, const POINT& pt)
{
	pImpl_->bDragging_ = true;
	setCapture();
	return DefaultWindowHandler::onLButtonDown(nFlags, pt);
}

LRESULT qs::SplitterWindow::onLButtonUp(UINT nFlags, const POINT& pt)
{
	if (pImpl_->bDragging_) {
		pImpl_->dragMouse(pt.x, pt.y);
		releaseCapture();
		pImpl_->bDragging_ = false;
	}
	return DefaultWindowHandler::onLButtonUp(nFlags, pt);
}

LRESULT qs::SplitterWindow::onMouseMove(UINT nFlags, const POINT& pt)
{
	if (pImpl_->bDragging_)
		pImpl_->dragMouse(pt.x, pt.y);
	return DefaultWindowHandler::onMouseMove(nFlags, pt);
}

LRESULT qs::SplitterWindow::onSize(UINT nFlags, int cx, int cy)
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
