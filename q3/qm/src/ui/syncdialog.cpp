/*
 * $Id$
 *
 * Copyright(C) 1998-2003 Satoshi Nakamura
 * All rights reserved.
 *
 */

#include <qmaccount.h>
#include <qmapplication.h>
#include <qmfolder.h>

#include <qsconv.h>
#include <qsinit.h>
#include <qsnew.h>

#include <algorithm>

#include <tchar.h>

#include "resourceinc.h"
#include "syncdialog.h"

#pragma warning(disable:4786)

using namespace qm;
using namespace qs;

#ifdef _WIN32_WCE
#	define DEFAULT_GUI_FONT SYSTEM_FONT
#endif


/****************************************************************************
 *
 * SyncDialogManager
 *
 */

qm::SyncDialogManager::SyncDialogManager(Profile* pProfile, QSTATUS* pstatus) :
	pProfile_(pProfile),
	pThread_(0)
{
}

qm::SyncDialogManager::~SyncDialogManager()
{
	if (pThread_) {
		pThread_->stop();
		delete pThread_;
	}
}

QSTATUS qm::SyncDialogManager::open(SyncDialog** ppSyncDialog)
{
	assert(ppSyncDialog);
	
	DECLARE_QSTATUS();
	
	*ppSyncDialog = 0;
	
	if (!pThread_) {
		std::auto_ptr<SyncDialogThread> pThread;
		status = newQsObject(pProfile_, &pThread);
		CHECK_QSTATUS();
		status = pThread->start();
		CHECK_QSTATUS();
		pThread_ = pThread.release();
	}
	*ppSyncDialog = pThread_->getDialog();
	if (!*ppSyncDialog)
		return QSTATUS_FAIL;
	
	return QSTATUS_SUCCESS;
}


/****************************************************************************
 *
 * SyncDialog
 *
 */

qm::SyncDialog::SyncDialog(Profile* pProfile, QSTATUS* pstatus) :
	Dialog(Application::getApplication().getResourceHandle(), IDD_SYNC, false, pstatus),
	DefaultDialogHandler(pstatus),
	DefaultCommandHandler(pstatus),
	pProfile_(pProfile),
	pStatusWindow_(0),
	bShowError_(false)
{
	if (*pstatus != QSTATUS_SUCCESS)
		return;
	
	DECLARE_QSTATUS();
	
	status = addCommandHandler(this);
	CHECK_QSTATUS_SET(pstatus);
	
	setDialogHandler(this, false);
}

qm::SyncDialog::~SyncDialog()
{
}

SyncManagerCallback* qm::SyncDialog::getSyncManagerCallback() const
{
	return pStatusWindow_;
}

void qm::SyncDialog::show()
{
	if (!isVisible()) {
		bShowError_ = false;
		layout();
		showWindow();
	}
	setForegroundWindow();
}

void qm::SyncDialog::hide()
{
	sendDlgItemMessage(IDC_ERROR, EM_SETSEL, 0, -1);
	sendDlgItemMessage(IDC_ERROR, EM_REPLACESEL, FALSE,
		reinterpret_cast<LPARAM>(_T("")));
	
	bool bForeground = Window::getForegroundWindow() == getHandle();
	showWindow(SW_HIDE);
	if (bForeground)
		getMainWindow()->setForegroundWindow();
}

QSTATUS qm::SyncDialog::addError(const WCHAR* pwszError)
{
	assert(pwszError);
	
	DECLARE_QSTATUS();
	
	W2T(pwszError, ptszError);
	
	Lock<CriticalSection> lock(csError_);
	
	sendDlgItemMessage(IDC_ERROR, EM_SETSEL, -1, -1);
	sendDlgItemMessage(IDC_ERROR, EM_REPLACESEL, FALSE,
		reinterpret_cast<LPARAM>(ptszError));
	
	if (!bShowError_) {
		bShowError_ = true;
		layout();
	}
	
	return QSTATUS_SUCCESS;
}

bool qm::SyncDialog::hasError() const
{
	return Window(getDlgItem(IDC_ERROR)).getWindowTextLength() != 0;
}

INT_PTR qm::SyncDialog::dialogProc(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	BEGIN_DIALOG_HANDLER()
		HANDLE_CLOSE()
		HANDLE_DESTROY()
		HANDLE_INITDIALOG()
		HANDLE_SIZE()
	END_DIALOG_HANDLER()
	return DefaultDialogHandler::dialogProc(uMsg, wParam, lParam);
}

LRESULT qm::SyncDialog::onCommand(WORD nCode, WORD nId)
{
	BEGIN_COMMAND_HANDLER()
		HANDLE_COMMAND_ID(IDCANCEL, onCancel)
	END_COMMAND_HANDLER()
	return 1;
}

LRESULT qm::SyncDialog::onClose()
{
	destroyWindow();
	return 0;
}

LRESULT qm::SyncDialog::onDestroy()
{
#ifndef _WIN32_WCE
	RECT rect;
	getWindowRect(&rect);
	pProfile_->setInt(L"SyncDialog", L"X", rect.left);
	pProfile_->setInt(L"SyncDialog", L"Y", rect.top);
	pProfile_->setInt(L"SyncDialog", L"Width", rect.right - rect.left);
	pProfile_->setInt(L"SyncDialog", L"Height", rect.bottom - rect.top);
#endif
	
	pStatusWindow_->unsubclassWindow();
	removeCommandHandler(this);
	
	::PostQuitMessage(0);
	
	return 0;
}

LRESULT qm::SyncDialog::onInitDialog(HWND hwndFocus, LPARAM lParam)
{
	DECLARE_QSTATUS();
	
	status = newQsObject(this, &pStatusWindow_);
	CHECK_QSTATUS_VALUE(TRUE);
	status = pStatusWindow_->create(L"QmSyncStatus", 0,
		WS_VISIBLE | WS_CHILD | WS_VSCROLL,
		0, 0, 0, 0, getHandle(), WS_EX_STATICEDGE, 0, IDC_SYNCSTATUS, 0);
	CHECK_QSTATUS_VALUE(TRUE);
	
#ifdef _WIN32_WCE
	int x = 0;
	int y = 0;
	RECT rectWorkArea;
	::SystemParametersInfo(SPI_GETWORKAREA, 0, &rectWorkArea, 0);
	int nWidth = rectWorkArea.right - rectWorkArea.left;
	int nHeight = rectWorkArea.bottom - rectWorkArea.top;
#else
	int x = 0;
	status = pProfile_->getInt(L"SyncDialog", L"X", 0, &x);
	CHECK_QSTATUS_VALUE(TRUE);
	int y = 0;
	status = pProfile_->getInt(L"SyncDialog", L"Y", 0, &y);
	CHECK_QSTATUS_VALUE(TRUE);
	int nWidth = 300;
	status = pProfile_->getInt(L"SyncDialog", L"Width", 300, &nWidth);
	CHECK_QSTATUS_VALUE(TRUE);
	int nHeight = 200;
	status = pProfile_->getInt(L"SyncDialog", L"Height", 200, &nHeight);
	CHECK_QSTATUS_VALUE(TRUE);
#endif
	setWindowPos(0, x, y, nWidth, nHeight, SWP_NOZORDER);
	
	return TRUE;
}

LRESULT qm::SyncDialog::onSize(UINT nFlags, int cx, int cy)
{
	layout(cx, cy);
	return DefaultDialogHandler::onSize(nFlags, cx, cy);
}

LRESULT qm::SyncDialog::onCancel()
{
	hide();
	return 0;
}

void qm::SyncDialog::layout()
{
	RECT rect;
	getClientRect(&rect);
	layout(rect.right - rect.left, rect.bottom - rect.top);
}

void qm::SyncDialog::layout(int cx, int cy)
{
	Window hide(getDlgItem(IDCANCEL));
	RECT rectHide;
	hide.getWindowRect(&rectHide);
	int nHideWidth = rectHide.right - rectHide.left;
#if defined _WIN32_WCE && _WIN32_WCE >= 300 && defined _WIN32_WCE_PSPC
	int nHideHeight = rectHide.bottom - rectHide.top;
	
	int nErrorHeight = bShowError_ ? (cy - nHideHeight - 25)/2 : 0;
	Window error(getDlgItem(IDC_ERROR));
	error.setWindowPos(0, 5,
		cy - nErrorHeight - nHideHeight - 10, cx - 10, nErrorHeight,
		SWP_NOZORDER | SWP_NOACTIVATE);
	error.showWindow(bShowError_);
	
	pStatusWindow_->setWindowPos(0, 5, 5, cx - 10,
		cy - nErrorHeight - nHideHeight - (bShowError_ ? 20 : 15),
		SWP_NOZORDER | SWP_NOACTIVATE);
	
	hide.setWindowPos(0, cx - nHideWidth - 5, cy - nHideHeight - 5, 0, 0,
		SWP_NOZORDER | SWP_NOSIZE | SWP_NOACTIVATE);
#else
	int nErrorHeight = bShowError_ ? (cy - 20)/2 : 0;
	Window error(getDlgItem(IDC_ERROR));
	error.setWindowPos(0, 5,
		cy - nErrorHeight - 5, cx - nHideWidth - 15, nErrorHeight,
		SWP_NOZORDER | SWP_NOACTIVATE);
	error.showWindow(bShowError_);
	
	pStatusWindow_->setWindowPos(0, 5, 5, cx - nHideWidth - 15,
		cy - nErrorHeight - (bShowError_ ? 15 : 10),
		SWP_NOZORDER | SWP_NOACTIVATE);
	
	hide.setWindowPos(0, cx - nHideWidth - 5, 5, 0, 0,
		SWP_NOZORDER | SWP_NOSIZE | SWP_NOACTIVATE);

#ifndef _WIN32_WCE
	Window sizeGrip(getDlgItem(IDC_SIZEGRIP));
	RECT rectSizeGrip;
	sizeGrip.getWindowRect(&rectSizeGrip);
	sizeGrip.setWindowPos(0,
		cx - (rectSizeGrip.right - rectSizeGrip.left),
		cy - (rectSizeGrip.bottom - rectSizeGrip.top),
		0, 0, SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE);
#endif
#endif
}


/****************************************************************************
 *
 * SyncStatusWindow
 *
 */

qm::SyncStatusWindow::SyncStatusWindow(SyncDialog* pSyncDialog, QSTATUS* pstatus) :
	WindowBase(true, pstatus),
	DefaultWindowHandler(pstatus),
	pSyncDialog_(pSyncDialog),
	nFontHeight_(0),
	wstrFinished_(0),
	wstrCancel_(0),
	nCancelWidth_(0)
{
	DECLARE_QSTATUS();
	
	HINSTANCE hInst = Application::getApplication().getResourceHandle();
	status = loadString(hInst, IDS_SYNCMSG_FINISHED, &wstrFinished_);
	CHECK_QSTATUS_SET(pstatus);
	status = loadString(hInst, IDS_CANCEL, &wstrCancel_);
	CHECK_QSTATUS_SET(pstatus);
	
	setWindowHandler(this, false);
}

qm::SyncStatusWindow::~SyncStatusWindow()
{
	freeWString(wstrFinished_);
	freeWString(wstrCancel_);
}

QSTATUS qm::SyncStatusWindow::getWindowClass(WNDCLASS* pwc)
{
	DECLARE_QSTATUS();
	
	status = DefaultWindowHandler::getWindowClass(pwc);
	pwc->hbrBackground = reinterpret_cast<HBRUSH>(COLOR_BTNFACE + 1);
	
	return QSTATUS_SUCCESS;
}

LRESULT qm::SyncStatusWindow::windowProc(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	BEGIN_MESSAGE_HANDLER()
		HANDLE_CREATE()
		HANDLE_LBUTTONDOWN()
		HANDLE_PAINT()
		HANDLE_SIZE()
		HANDLE_VSCROLL()
	END_MESSAGE_HANDLER()
	return DefaultWindowHandler::windowProc(uMsg, wParam, lParam);
}

QSTATUS qm::SyncStatusWindow::start()
{
	return QSTATUS_SUCCESS;
}

void qm::SyncStatusWindow::end()
{
}

QSTATUS qm::SyncStatusWindow::startThread(unsigned int nId)
{
	DECLARE_QSTATUS();
	
	pSyncDialog_->show();
	
	std::auto_ptr<Item> pItem;
	status = newQsObject(nId, &pItem);
	CHECK_QSTATUS();
	
	{
		Lock<CriticalSection> lock(cs_);
		status = STLWrapper<ItemList>(listItem_).push_back(pItem.get());
		CHECK_QSTATUS();
		pItem.release();
	}
	
	invalidate(false);
	updateScrollBar();
	
	return QSTATUS_SUCCESS;
}

void qm::SyncStatusWindow::endThread(unsigned int nId)
{
	bool bEmpty = false;
	{
		Lock<CriticalSection> lock(cs_);
		ItemList::iterator it = getItem(nId);
		delete *it;
		listItem_.erase(it);
		bEmpty = listItem_.empty();
	}
	
	if (bEmpty && !pSyncDialog_->hasError())
		pSyncDialog_->hide();
	
	invalidate(false);
	updateScrollBar();
	
	invalidate(false);
}

QSTATUS qm::SyncStatusWindow::setPos(
	unsigned int nId, bool bSub, unsigned int nPos)
{
	Lock<CriticalSection> lock(cs_);
	ItemList::iterator it = getItem(nId);
	(*it)->setPos(bSub, nPos);
	invalidate(false);
	return QSTATUS_SUCCESS;
}

QSTATUS qm::SyncStatusWindow::setRange(unsigned int nId,
	bool bSub, unsigned int nMin, unsigned int nMax)
{
	Lock<CriticalSection> lock(cs_);
	ItemList::iterator it = getItem(nId);
	(*it)->setRange(bSub, nMin, nMax);
	invalidate(false);
	return QSTATUS_SUCCESS;
}

QSTATUS qm::SyncStatusWindow::setAccount(unsigned int nId,
	Account* pAccount, SubAccount* pSubAccount)
{
	DECLARE_QSTATUS();
	
	Lock<CriticalSection> lock(cs_);
	ItemList::iterator it = getItem(nId);
	status = (*it)->setAccount(pAccount, pSubAccount);
	CHECK_QSTATUS();
	invalidate(false);
	
	return QSTATUS_SUCCESS;
}

QSTATUS qm::SyncStatusWindow::setFolder(
	unsigned int nId, Folder* pFolder)
{
	DECLARE_QSTATUS();
	
	Lock<CriticalSection> lock(cs_);
	ItemList::iterator it = getItem(nId);
	status = (*it)->setFolder(pFolder);
	CHECK_QSTATUS();
	invalidate(false);
	
	return QSTATUS_SUCCESS;
}

QSTATUS qm::SyncStatusWindow::setMessage(
	unsigned int nId, const WCHAR* pwszMessage)
{
	DECLARE_QSTATUS();
	
	if (nId == -1) {
		// TODO
	}
	else {
		Lock<CriticalSection> lock(cs_);
		ItemList::iterator it = getItem(nId);
		status = (*it)->setMessage(pwszMessage);
		CHECK_QSTATUS();
		invalidate(false);
	}
	
	return QSTATUS_SUCCESS;
}

QSTATUS qm::SyncStatusWindow::addError(
	unsigned int nId, const SessionErrorInfo& info)
{
	DECLARE_QSTATUS();
	
	// TODO
	// Account, SubAccount, Code and Descriptions may be null
	
	StringBuffer<WSTRING> buf(&status);
	CHECK_QSTATUS();
	
	Account* pAccount = info.getAccount();
	if (pAccount) {
		status = buf.append(L"[");
		CHECK_QSTATUS();
		status = buf.append(pAccount->getName());
		CHECK_QSTATUS();
		
		SubAccount* pSubAccount = info.getSubAccount();
		if (*pSubAccount->getName()) {
			status = buf.append(L'/');
			CHECK_QSTATUS();
			status = buf.append(pSubAccount->getName());
			CHECK_QSTATUS();
		}
		
		NormalFolder* pFolder = info.getFolder();
		if (pFolder) {
			string_ptr<WSTRING> wstrName;
			status = pFolder->getFullName(&wstrName);
			CHECK_QSTATUS();
			status = buf.append(L" - ");
			CHECK_QSTATUS();
			status = buf.append(wstrName.get());
			CHECK_QSTATUS();
		}
		
		status = buf.append(L"] ");
		CHECK_QSTATUS();
	}
	
	status = buf.append(info.getMessage());
	CHECK_QSTATUS();
	
	unsigned int nCode = info.getCode();
	if (nCode != 0) {
		WCHAR wszCode[32];
		swprintf(wszCode, L" (0x%08X)", info.getCode());
		status = buf.append(wszCode);
		CHECK_QSTATUS();
	}
	
	status = buf.append(L"\r\n");
	CHECK_QSTATUS();
	
	for (size_t n = 0; n < info.getDescriptionCount(); ++n) {
		const WCHAR* p = info.getDescription(n);
		if (p) {
			status = buf.append(L"  ");
			CHECK_QSTATUS();
			status = buf.append(p);
			CHECK_QSTATUS();
			status = buf.append(L"\r\n");
			CHECK_QSTATUS();
		}
	}
	
	status = pSyncDialog_->addError(buf.getCharArray());
	CHECK_QSTATUS();
	
	return QSTATUS_SUCCESS;
}

bool qm::SyncStatusWindow::isCanceled(unsigned int nId, bool bForce)
{
	if (nId == -1) {
		// TODO
	}
	else {
		Lock<CriticalSection> lock(cs_);
		ItemList::iterator it = getItem(nId);
		unsigned int nCanceledTime = (*it)->getCanceledTime();
		if (nCanceledTime == 0)
			return false;
		else if (!bForce)
			return true;
		else
			return ::GetTickCount() - nCanceledTime > 10*1000;
	}
}

LRESULT qm::SyncStatusWindow::onCreate(CREATESTRUCT* pCreateStruct)
{
	if (DefaultWindowHandler::onCreate(pCreateStruct) == -1)
		return -1;
	
	DECLARE_QSTATUS();
	
	ClientDeviceContext dc(getHandle(), &status);
	CHECK_QSTATUS_VALUE(-1);
	ObjectSelector<HFONT> selector(dc,
		reinterpret_cast<HFONT>(::GetStockObject(DEFAULT_GUI_FONT)));
	TEXTMETRIC tm;
	dc.getTextMetrics(&tm);
	nFontHeight_ = tm.tmHeight + tm.tmExternalLeading;
	
	SIZE size;
	dc.getTextExtent(wstrCancel_, wcslen(wstrCancel_), &size);
	nCancelWidth_ = size.cx;
	
	return 0;
}

LRESULT qm::SyncStatusWindow::onLButtonDown(UINT nFlags, const POINT& pt)
{
	RECT rect;
	getClientRect(&rect);
	
	int nHeight = rect.bottom - rect.top;
	int nPos = getScrollPos(SB_VERT);
	int nItemHeight = getItemHeight();
	int nCount = nHeight/nItemHeight + (nHeight%nItemHeight == 0 ? 0 : 1);
	
	ItemList::size_type n = nPos;
	while (n < static_cast<ItemList::size_type>(nPos + nCount) &&
		n < listItem_.size()) {
		rect.bottom = rect.top + nItemHeight;
		if (::PtInRect(&rect, pt)) {
			RECT rectButton;
			getCancelButtonRect(rect, &rectButton);
			if (::PtInRect(&rectButton, pt)) {
				listItem_[n]->cancel();
				validate();
			}
		}
		rect.top = rect.bottom;
		++n;
	}
	
	return 0;
}

LRESULT qm::SyncStatusWindow::onPaint()
{
	DECLARE_QSTATUS();
	
	PaintDeviceContext dc(getHandle(), &status);
	CHECK_QSTATUS_VALUE(0);
	
	ObjectSelector<HFONT> selector(dc,
		reinterpret_cast<HFONT>(::GetStockObject(DEFAULT_GUI_FONT)));
	
	RECT rect;
	getClientRect(&rect);
	int nBottom = rect.bottom;
	
	int nHeight = rect.bottom - rect.top;
	int nPos = getScrollPos(SB_VERT);
	int nItemHeight = getItemHeight();
	int nCount = nHeight/nItemHeight + (nHeight%nItemHeight == 0 ? 0 : 1);
	
	Lock<CriticalSection> lock(cs_);
	
	ItemList::size_type n = nPos;
	while (n < static_cast<ItemList::size_type>(nPos + nCount) &&
		n < listItem_.size()) {
		rect.bottom = rect.top + nItemHeight;
		paintItem(&dc, rect, listItem_[n]);
		rect.top = rect.bottom;
		++n;
	}
	
	rect.bottom = nBottom;
	dc.fillSolidRect(rect, ::GetSysColor(COLOR_BTNFACE));
	
	return 0;
}

LRESULT qm::SyncStatusWindow::onSize(UINT nFlags, int cx, int cy)
{
	updateScrollBar();
	invalidate();
	
	return DefaultWindowHandler::onSize(nFlags, cx, cy);
}

LRESULT qm::SyncStatusWindow::onVScroll(UINT nCode, UINT nPos, HWND hwnd)
{
	SCROLLINFO si = {
		sizeof(si),
		SIF_ALL
	};
	getScrollInfo(SB_VERT, &si);
	
	int nNewPos = si.nPos;
	switch (nCode) {
	case SB_TOP:
		nNewPos = 0;
		break;
	case SB_BOTTOM:
		nNewPos = si.nMax - si.nPage + 1;
		break;
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
	case SB_THUMBPOSITION:
	case SB_THUMBTRACK:
		nNewPos = nPos;
		break;
	case SB_ENDSCROLL:
	default:
		break;
	}
	if (nNewPos < 0)
		nNewPos = 0;
	else if (nNewPos > si.nMax - static_cast<int>(si.nPage) + 1)
		nNewPos = si.nMax - si.nPage + 1;
	
	if (nNewPos != si.nPos) {
		setScrollPos(SB_VERT, nNewPos);
		invalidate();
	}
	
	return DefaultWindowHandler::onVScroll(nCode, nPos, hwnd);
}

int qm::SyncStatusWindow::getItemHeight() const
{
	return nFontHeight_*2 + 8;
}

void qm::SyncStatusWindow::updateScrollBar()
{
	RECT rect;
	getClientRect(&rect);
	
	int nItemCount = 0;
	{
		Lock<CriticalSection> lock(cs_);
		nItemCount = listItem_.size();
	}
	
	int nItemHeight = getItemHeight();
	int nHeight = rect.bottom - rect.top;
	
	SCROLLINFO si = {
		sizeof(si),
		SIF_PAGE | SIF_RANGE,
		0,
		nItemCount,
		nHeight/nItemHeight + (nHeight%nItemHeight == 0 ? 0 : 1)
	};
	setScrollInfo(SB_VERT, si);
}

void qm::SyncStatusWindow::paintItem(DeviceContext* pdc,
	const RECT& rect, const Item* pItem)
{
	RECT rectClip = {
		rect.left,
		rect.top + nFontHeight_ + 5,
		rect.right,
		rect.bottom
	};
	
	if (pItem) {
		int nBarWidth = (rect.right - rect.left - 10)/2 - 5;
		
		RECT rectMain = {
			rect.left + 5,
			rect.top + 3,
			rect.left + 5 + nBarWidth,
			rect.top + 3 + nFontHeight_
		};
		paintProgress(pdc, rectMain, pItem->getProgress(false));
		
		RECT rectSub = {
			rect.right - 5 - nBarWidth,
			rect.top + 3,
			rect.right - 5,
			rect.top + 3 + nFontHeight_
		};
		paintProgress(pdc, rectSub, pItem->getProgress(true));
		
		RECT rectButton;
		getCancelButtonRect(rect, &rectButton);
		paintCancelButton(pdc, rectButton, pItem->isCanceled());
		
		rectClip.right = rectButton.left - 5;
		
		// TODO
		// Fill area not painted
		
	}
	else {
		RECT r = {
			rect.left,
			rect.top,
			rect.right,
			rect.top + nFontHeight_ + 5
		};
		pdc->fillSolidRect(r, ::GetSysColor(COLOR_BTNFACE));
	}
	
	const WCHAR* pwszMessage = 0;
	if (pItem)
		pwszMessage = pItem->getMessage();
	else
		pwszMessage = wstrFinished_;
	
	pdc->setTextColor(::GetSysColor(COLOR_BTNTEXT));
	pdc->setBkColor(::GetSysColor(COLOR_BTNFACE));
	pdc->extTextOut(rectClip.left + 5, rectClip.top,
		ETO_CLIPPED | ETO_OPAQUE, rectClip,
		pwszMessage, wcslen(pwszMessage), 0);
}

void qm::SyncStatusWindow::paintProgress(qs::DeviceContext* pdc,
	const RECT& rect, const Item::Progress& progress)
{
	int nMin = progress.nMin_;
	int nMax = progress.nMax_;
	int nPos = progress.nPos_;
	if (nPos < nMin)
		nPos = nMin;
	else if (nPos > nMax)
		nPos = nMax;
	
	RECT rectHighlight = {
		rect.left,
		rect.top,
		rect.left + (nMin == nMax ? 0 :
			(rect.right - rect.left)*(nPos - nMin)/(nMax - nMin)),
		rect.bottom
	};
	pdc->fillSolidRect(rectHighlight, ::GetSysColor(COLOR_ACTIVECAPTION));
	RECT rectUnHighlight = {
		rect.left + (nMin == nMax ? 0 :
			(rect.right - rect.left)*(nPos - nMin)/(nMax - nMin)),
		rect.top,
		rect.right,
		rect.bottom
	};
	pdc->fillSolidRect(rectUnHighlight, ::GetSysColor(COLOR_3DFACE));
	
	GdiObject<HPEN> penShadow(::CreatePen(PS_SOLID, 1,
		::GetSysColor(COLOR_BTNSHADOW)));
	ObjectSelector<HPEN> shadowPenSelector(*pdc, penShadow.get());
	POINT ptShadow[] = {
		{ rect.left,	rect.bottom	},
		{ rect.left,	rect.top	},
		{ rect.right,	rect.top	}
	};
	pdc->polyline(ptShadow, countof(ptShadow));
	
	GdiObject<HPEN> penHighlight(::CreatePen(PS_SOLID, 1,
		::GetSysColor(COLOR_BTNHIGHLIGHT)));
	ObjectSelector<HPEN> highlightPenSelector(*pdc, penHighlight.get());
	POINT ptHighlight[] = {
		{ rect.left,	rect.bottom		},
		{ rect.right,	rect.bottom		},
		{ rect.right,	rect.top - 1	}
	};
	pdc->polyline(ptHighlight, countof(ptHighlight));
	
	WCHAR wsz[128] = { L'\0' };
	if (nMin != 0 || nMax != 0)
		swprintf(wsz, L"%d/%d", nPos - nMin, nMax - nMin);
	
	SIZE size;
	pdc->getTextExtent(wsz, wcslen(wsz), &size);
	int nLeft = rect.left + ((rect.right - rect.left) - size.cx)/2;
	int nTop = rect.top;
	
	pdc->setBkMode(TRANSPARENT);
	pdc->setTextColor(::GetSysColor(COLOR_CAPTIONTEXT));
	pdc->extTextOut(nLeft, nTop, ETO_CLIPPED,
		rectHighlight, wsz, wcslen(wsz), 0);
	pdc->setTextColor(::GetSysColor(COLOR_BTNTEXT));
	pdc->extTextOut(nLeft, nTop, ETO_CLIPPED,
		rectUnHighlight, wsz, wcslen(wsz), 0);
}

void qm::SyncStatusWindow::paintCancelButton(
	qs::DeviceContext* pdc, const RECT& rect, bool bCanceled)
{
	pdc->setTextColor(::GetSysColor(
		bCanceled ? COLOR_GRAYTEXT : COLOR_BTNTEXT));
	pdc->setBkColor(::GetSysColor(COLOR_BTNFACE));
	pdc->extTextOut(rect.left + 5, rect.top,
		ETO_CLIPPED | ETO_OPAQUE, rect, L"Cancel", 6, 0);
	
	GdiObject<HPEN> penShadow(::CreatePen(PS_SOLID, 1,
		::GetSysColor(COLOR_BTNSHADOW)));
	ObjectSelector<HPEN> shadowPenSelector(*pdc, penShadow.get());
	POINT ptShadow[] = {
		{ rect.left,	rect.bottom	},
		{ rect.right,	rect.bottom	},
		{ rect.right,	rect.top	}
	};
	pdc->polyline(ptShadow, countof(ptShadow));
	
	GdiObject<HPEN> penHighlight(::CreatePen(PS_SOLID, 1,
		::GetSysColor(COLOR_BTNHIGHLIGHT)));
	ObjectSelector<HPEN> highlightPenSelector(*pdc, penHighlight.get());
	POINT ptHighlight[] = {
		{ rect.left,		rect.bottom	},
		{ rect.left,		rect.top	},
		{ rect.right + 1,	rect.top	}
	};
	pdc->polyline(ptHighlight, countof(ptHighlight));
}

void qm::SyncStatusWindow::getCancelButtonRect(
	const RECT& rect, RECT* pRectButton)
{
	pRectButton->right = rect.right - 5;
	pRectButton->bottom = rect.bottom - 3;
	pRectButton->left = pRectButton->right - nCancelWidth_ - 10;
	pRectButton->top = pRectButton->bottom - nFontHeight_;
}

SyncStatusWindow::ItemList::iterator qm::SyncStatusWindow::getItem(
	unsigned int nId)
{
	ItemList::iterator it = listItem_.begin();
	while (it != listItem_.end() &&
		(!*it || (*it)->getId() != nId))
		++it;
	return it;
}


/****************************************************************************
 *
 * SyncStatusWindow::Item
 *
 */

qm::SyncStatusWindow::Item::Item(unsigned int nId, qs::QSTATUS* pstatus) :
	nId_(nId),
	pAccount_(0),
	pSubAccount_(0),
	pFolder_(0),
	wstrOriginalMessage_(0),
	wstrMessage_(0),
	nCanceledTime_(0)
{
	main_.nMin_ = 0;
	main_.nMax_ = 0;
	main_.nPos_ = 0;
	sub_.nMin_ = 0;
	sub_.nMax_ = 0;
	sub_.nPos_ = 0;
}

qm::SyncStatusWindow::Item::~Item()
{
	freeWString(wstrOriginalMessage_);
	freeWString(wstrMessage_);
}

unsigned int qm::SyncStatusWindow::Item::getId() const
{
	return nId_;
}

const SyncStatusWindow::Item::Progress& qm::SyncStatusWindow::Item::getProgress(bool bSub) const
{
	return bSub ? sub_ : main_;
}

const WCHAR* qm::SyncStatusWindow::Item::getMessage() const
{
	return wstrMessage_ ? wstrMessage_ : L"";
}

bool qm::SyncStatusWindow::Item::isCanceled() const
{
	return nCanceledTime_ != 0;
}

unsigned int qm::SyncStatusWindow::Item::getCanceledTime() const
{
	return nCanceledTime_;
}

void qm::SyncStatusWindow::Item::setPos(bool bSub, unsigned int nPos)
{
	Progress& p = bSub ? sub_ : main_;
	p.nPos_ = nPos;
}

void qm::SyncStatusWindow::Item::setRange(
	bool bSub, unsigned int nMin, unsigned int nMax)
{
	Progress& p = bSub ? sub_ : main_;
	p.nMin_ = nMin;
	p.nMax_ = nMax;
}

QSTATUS qm::SyncStatusWindow::Item::setAccount(
	Account* pAccount, SubAccount* pSubAccount)
{
	pAccount_ = pAccount;
	pSubAccount_ = pSubAccount;
	return updateMessage();
}

QSTATUS qm::SyncStatusWindow::Item::setFolder(Folder* pFolder)
{
	pFolder_ = pFolder;
	return updateMessage();
}

QSTATUS qm::SyncStatusWindow::Item::setMessage(const WCHAR* pwszMessage)
{
	string_ptr<WSTRING> wstrMessage(allocWString(pwszMessage));
	if (!wstrMessage.get())
		return QSTATUS_OUTOFMEMORY;
	
	freeWString(wstrOriginalMessage_);
	wstrOriginalMessage_ = wstrMessage.release();
	
	return updateMessage();
}

void qm::SyncStatusWindow::Item::cancel()
{
	nCanceledTime_ = ::GetTickCount();
}

QSTATUS qm::SyncStatusWindow::Item::updateMessage()
{
	DECLARE_QSTATUS();
	
	StringBuffer<WSTRING> buf(&status);
	CHECK_QSTATUS();
	
	if (pAccount_) {
		status = buf.append(L'[');
		CHECK_QSTATUS();
		status = buf.append(pAccount_->getName());
		CHECK_QSTATUS();
		if (*pSubAccount_->getName()) {
			status = buf.append(L" (");
			CHECK_QSTATUS();
			status = buf.append(pSubAccount_->getName());
			CHECK_QSTATUS();
			status = buf.append(L')');
			CHECK_QSTATUS();
		}
		
		if (pFolder_) {
			status = buf.append(L" / ");
			CHECK_QSTATUS();
			string_ptr<WSTRING> wstrFolder;
			status = pFolder_->getFullName(&wstrFolder);
			CHECK_QSTATUS();
			status = buf.append(wstrFolder.get());
			CHECK_QSTATUS();
		}
		status = buf.append(L"] ");
		CHECK_QSTATUS();
	}
	
	if (wstrOriginalMessage_) {
		status = buf.append(wstrOriginalMessage_);
		CHECK_QSTATUS();
	}
	
	freeWString(wstrMessage_);
	wstrMessage_ = buf.getString();
	
	return QSTATUS_SUCCESS;
}


/****************************************************************************
 *
 * SyncDialogThread
 *
 */

qm::SyncDialogThread::SyncDialogThread(qs::Profile* pProfile, qs::QSTATUS* pstatus) :
	Thread(pstatus),
	pProfile_(pProfile),
	pDialog_(0),
	pEvent_(0)
{
	DECLARE_QSTATUS();
	
	status = newQsObject(true, false, &pEvent_);
	CHECK_QSTATUS_SET(pstatus);
}

qm::SyncDialogThread::~SyncDialogThread()
{
	delete pEvent_;
}

SyncDialog* qm::SyncDialogThread::getDialog()
{
	pEvent_->wait();
	return pDialog_;
}

void qm::SyncDialogThread::stop()
{
	pDialog_->postMessage(WM_CLOSE);
	join();
}

unsigned int qm::SyncDialogThread::run()
{
	DECLARE_QSTATUS();
	
	InitThread init(&status);
	CHECK_QSTATUS_VALUE(-1);
	
	std::auto_ptr<SyncDialog> pDialog;
	{
		struct Set
		{
			Set(Event* pEvent) : pEvent_(pEvent) {}
			~Set() { pEvent_->set(); }
			Event* pEvent_;
		} set(pEvent_);
		
		status = newQsObject(pProfile_, &pDialog);
		CHECK_QSTATUS_VALUE(-1);
		status = pDialog->create(0);
		CHECK_QSTATUS_VALUE(-1);
		pDialog_ = pDialog.get();
	}
	
	MSG msg;
	while (::GetMessage(&msg, 0, 0, 0)) {
		if (!::IsDialogMessage(pDialog_->getHandle(), &msg)) {
			::TranslateMessage(&msg);
			::DispatchMessage(&msg);
		}
	}
	
	return 0;
}
