/*
 * $Id$
 *
 * Copyright(C) 1998-2004 Satoshi Nakamura
 * All rights reserved.
 *
 */

#include <qmaccount.h>
#include <qmapplication.h>
#include <qmfolder.h>

#include <qsconv.h>
#include <qsinit.h>

#include <algorithm>

#include <tchar.h>

#include "dialogs.h"
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

qm::SyncDialogManager::SyncDialogManager(Profile* pProfile) :
	pProfile_(pProfile)
{
}

qm::SyncDialogManager::~SyncDialogManager()
{
	if (pThread_.get())
		pThread_->stop();
}

SyncDialog* qm::SyncDialogManager::open()
{
	if (!pThread_.get()) {
		std::auto_ptr<SyncDialogThread> pThread(new SyncDialogThread(pProfile_));
		if (!pThread->start())
			return 0;
		pThread_ = pThread;
	}
	return pThread_->getDialog();
}

bool qm::SyncDialogManager::save() const
{
	if (pThread_.get()) {
		SyncDialog* pSyncDialog = pThread_->getDialog();
		struct RunnableImpl : public Runnable
		{
			RunnableImpl(SyncDialog* pSyncDialog) :
				pSyncDialog_(pSyncDialog),
				bSave_(false)
			{
			}
			
			virtual void run()
			{
				bSave_ = pSyncDialog_->save();
			}
			
			SyncDialog* pSyncDialog_;
			bool bSave_;
		} runnable(pSyncDialog);
		pSyncDialog->getInitThread()->getSynchronizer()->syncExec(&runnable);
		return runnable.bSave_;
	}
	else {
		return true;
	}
}


/****************************************************************************
 *
 * SyncDialog
 *
 */

qm::SyncDialog::SyncDialog(Profile* pProfile) :
	Dialog(Application::getApplication().getResourceHandle(), IDD_SYNC, false),
	pProfile_(pProfile),
	pStatusWindow_(0),
	bShowError_(false),
	nCanceledTime_(0)
{
	addCommandHandler(this);
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

void qm::SyncDialog::setMessage(const WCHAR* pwszMessage)
{
	assert(pwszMessage);
	setDlgItemText(IDC_MESSAGE, pwszMessage);
}

unsigned int qm::SyncDialog::getCanceledTime() const
{
	return nCanceledTime_;
}

void qm::SyncDialog::resetCanceledTime()
{
	nCanceledTime_ = 0;
}

void qm::SyncDialog::addError(const WCHAR* pwszError)
{
	assert(pwszError);
	
	W2T(pwszError, ptszError);
	
	Lock<CriticalSection> lock(csError_);
	
	sendDlgItemMessage(IDC_ERROR, EM_SETSEL, -1, -1);
	sendDlgItemMessage(IDC_ERROR, EM_REPLACESEL, FALSE,
		reinterpret_cast<LPARAM>(ptszError));
	
	if (!bShowError_) {
		bShowError_ = true;
		layout();
		showWindow();
		setForegroundWindow();
	}
}

bool qm::SyncDialog::hasError() const
{
	return Window(getDlgItem(IDC_ERROR)).getWindowTextLength() != 0;
}

void qm::SyncDialog::enableCancel(bool bEnable)
{
	Window(getDlgItem(IDC_CANCEL)).enableWindow(bEnable);
	
	UINT nOldId = bEnable ? IDC_HIDE : IDC_CANCEL;
	UINT nNewId = bEnable ? IDC_CANCEL : IDC_HIDE;
	Window(getDlgItem(nNewId)).setFocus();
	sendDlgItemMessage(nOldId, BM_SETSTYLE, BS_PUSHBUTTON, TRUE);
	sendMessage(DM_SETDEFID, nNewId);
	sendDlgItemMessage(nNewId, BM_SETSTYLE, BS_DEFPUSHBUTTON, TRUE);
}

bool qm::SyncDialog::showDialupDialog(RASDIALPARAMS* prdp) const
{
	assert(prdp);
	
	struct RunnableImpl : public Runnable
	{
		RunnableImpl(HWND hwnd,
					 RASDIALPARAMS* prdp) :
			hwnd_(hwnd),
			prdp_(prdp),
			bCancel_(false)
		{
		}
		
		virtual void run()
		{
			T2W(prdp_->szEntryName, pwszEntryName);
			T2W(prdp_->szUserName, pwszUserName);
			T2W(prdp_->szPassword, pwszPassword);
			T2W(prdp_->szDomain, pwszDomain);
			
			DialupDialog dialog(pwszEntryName, pwszUserName, pwszPassword, pwszDomain);
			if (dialog.doModal(hwnd_, 0) == IDOK) {
				W2T(dialog.getUserName(), ptszUserName);
				_tcsncpy(prdp_->szUserName, ptszUserName, UNLEN);
				W2T(dialog.getPassword(), ptszPassword);
				_tcsncpy(prdp_->szPassword, ptszPassword, PWLEN);
				W2T(dialog.getDomain(), ptszDomain);
				_tcsncpy(prdp_->szDomain, ptszDomain, DNLEN);
			}
			else {
				bCancel_ = true;
			}
		}
		
		HWND hwnd_;
		RASDIALPARAMS* prdp_;
		bool bCancel_;
	} runnable(getHandle(), prdp);
	getInitThread()->getSynchronizer()->syncExec(&runnable);
	return !runnable.bCancel_;
}

wstring_ptr qm::SyncDialog::selectDialupEntry() const
{
	struct RunnableImpl : public Runnable
	{
		RunnableImpl(Profile* pProfile,
					 HWND hwnd) :
			pProfile_(pProfile),
			hwnd_(hwnd)
		{
		}
		
		virtual void run()
		{
			SelectDialupEntryDialog dialog(pProfile_);
			if (dialog.doModal(hwnd_, 0) == IDOK)
				wstrEntry_ = allocWString(dialog.getEntry());
		}
		
		Profile* pProfile_;
		HWND hwnd_;
		wstring_ptr wstrEntry_;
	} runnable(pProfile_, getHandle());
	getInitThread()->getSynchronizer()->syncExec(&runnable);
	return runnable.wstrEntry_;
}

void qm::SyncDialog::notifyNewMessage() const
{
	wstring_ptr wstrSound(pProfile_->getString(L"NewMailCheck", L"Sound", 0));
	if (*wstrSound.get()) {
		W2T(wstrSound.get(), ptszSound);
		sndPlaySound(ptszSound, SND_ASYNC);
	}
}

bool qm::SyncDialog::save() const
{
#ifndef _WIN32_WCE
	RECT rect;
	getWindowRect(&rect);
	pProfile_->setInt(L"SyncDialog", L"X", rect.left);
	pProfile_->setInt(L"SyncDialog", L"Y", rect.top);
	pProfile_->setInt(L"SyncDialog", L"Width", rect.right - rect.left);
	pProfile_->setInt(L"SyncDialog", L"Height", rect.bottom - rect.top);
#endif
	
	return true;
}

INT_PTR qm::SyncDialog::dialogProc(UINT uMsg,
								   WPARAM wParam,
								   LPARAM lParam)
{
	BEGIN_DIALOG_HANDLER()
		HANDLE_CLOSE()
		HANDLE_DESTROY()
		HANDLE_INITDIALOG()
		HANDLE_SIZE()
	END_DIALOG_HANDLER()
	return DefaultDialogHandler::dialogProc(uMsg, wParam, lParam);
}

LRESULT qm::SyncDialog::onCommand(WORD nCode,
								  WORD nId)
{
	BEGIN_COMMAND_HANDLER()
		HANDLE_COMMAND_ID(IDC_CANCEL, onCancel)
		HANDLE_COMMAND_ID(IDC_HIDE, onHide)
		HANDLE_COMMAND_ID(IDCANCEL, onEsc)
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
	pStatusWindow_->unsubclassWindow();
	removeCommandHandler(this);
	
	::PostQuitMessage(0);
	
	return 0;
}

LRESULT qm::SyncDialog::onInitDialog(HWND hwndFocus,
									 LPARAM lParam)
{
	pStatusWindow_ = new SyncStatusWindow(this);
	pStatusWindow_->create(L"QmSyncStatus", 0, WS_VISIBLE | WS_CHILD | WS_VSCROLL,
		0, 0, 0, 0, getHandle(), WS_EX_STATICEDGE, 0, IDC_SYNCSTATUS, 0);
	
#ifdef _WIN32_WCE
	int x = 0;
	int y = 0;
	RECT rectWorkArea;
	::SystemParametersInfo(SPI_GETWORKAREA, 0, &rectWorkArea, 0);
	int nWidth = rectWorkArea.right - rectWorkArea.left;
	int nHeight = rectWorkArea.bottom - rectWorkArea.top;
#else
	int x = pProfile_->getInt(L"SyncDialog", L"X", 0);
	int y = pProfile_->getInt(L"SyncDialog", L"Y", 0);
	int nWidth = pProfile_->getInt(L"SyncDialog", L"Width", 300);
	int nHeight = pProfile_->getInt(L"SyncDialog", L"Height", 200);
#endif
	setWindowPos(0, x, y, nWidth, nHeight, SWP_NOZORDER);
	
	return TRUE;
}

LRESULT qm::SyncDialog::onSize(UINT nFlags,
							   int cx,
							   int cy)
{
	layout(cx, cy);
	return DefaultDialogHandler::onSize(nFlags, cx, cy);
}

LRESULT qm::SyncDialog::onCancel()
{
	if (nCanceledTime_ == 0)
		nCanceledTime_ = ::GetTickCount();
	enableCancel(false);
	return 0;
}

LRESULT qm::SyncDialog::onEsc()
{
	return nCanceledTime_ == 0 ? onCancel() : onHide();
}

LRESULT qm::SyncDialog::onHide()
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

void qm::SyncDialog::layout(int cx,
							int cy)
{
	Window message(getDlgItem(IDC_MESSAGE));
	Window cancel(getDlgItem(IDC_CANCEL));
	Window hide(getDlgItem(IDC_HIDE));
	Window error(getDlgItem(IDC_ERROR));
	RECT rectMessage;
	message.getWindowRect(&rectMessage);
	int nMessageHeight = rectMessage.bottom - rectMessage.top;
	RECT rectButton;
	hide.getWindowRect(&rectButton);
	int nButtonWidth = rectButton.right - rectButton.left;
	int nButtonHeight = rectButton.bottom - rectButton.top;
	
#if defined _WIN32_WCE && _WIN32_WCE >= 300 && defined _WIN32_WCE_PSPC
	int nErrorHeight = bShowError_ ? (cy - nButtonHeight - 30)/2 : 0;
	error.setWindowPos(0, 5,
		cy - nErrorHeight - nButtonHeight - 10, cx - 10, nErrorHeight,
		SWP_NOZORDER | SWP_NOACTIVATE);
	error.showWindow(bShowError_);
	
	message.setWindowPos(0, 5, 5, cx - 10, nMessageHeight,
		SWP_NOZORDER | SWP_NOACTIVATE);
	pStatusWindow_->setWindowPos(0, 5, nMessageHeight + 10, cx - 10,
		cy - nErrorHeight - nButtonHeight - nMessageHeight - (bShowError_ ? 25 : 20),
		SWP_NOZORDER | SWP_NOACTIVATE);
	
	cancel.setWindowPos(0, cx - nButtonWidth*2 - 10, cy - nButtonHeight - 5,
		0, 0, SWP_NOZORDER | SWP_NOSIZE | SWP_NOACTIVATE);
	hide.setWindowPos(0, cx - nButtonWidth - 5, cy - nButtonHeight - 5,
		0, 0, SWP_NOZORDER | SWP_NOSIZE | SWP_NOACTIVATE);
#else
	int nErrorHeight = bShowError_ ? (cy - 25)/2 : 0;
	error.setWindowPos(0, 5,
		cy - nErrorHeight - 5, cx - nButtonWidth - 15, nErrorHeight,
		SWP_NOZORDER | SWP_NOACTIVATE);
	error.showWindow(bShowError_);
	
	message.setWindowPos(0, 5, 5, cx - nButtonWidth - 15,
		nMessageHeight, SWP_NOZORDER | SWP_NOACTIVATE);
	pStatusWindow_->setWindowPos(0, 5,
		nMessageHeight + 10, cx - nButtonWidth - 15,
		cy - nErrorHeight - nMessageHeight - (bShowError_ ? 20 : 15),
		SWP_NOZORDER | SWP_NOACTIVATE);
	cancel.setWindowPos(0, cx - nButtonWidth - 5, 5, 0, 0,
		SWP_NOZORDER | SWP_NOSIZE | SWP_NOACTIVATE);
	hide.setWindowPos(0, cx - nButtonWidth - 5, nButtonHeight + 8,
		0, 0, SWP_NOZORDER | SWP_NOSIZE | SWP_NOACTIVATE);
	
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

qm::SyncStatusWindow::SyncStatusWindow(SyncDialog* pSyncDialog) :
	WindowBase(true),
	pSyncDialog_(pSyncDialog),
	bNewMessage_(false),
	nFontHeight_(0),
	nCancelWidth_(0)
{
	HINSTANCE hInst = Application::getApplication().getResourceHandle();
	wstrFinished_ = loadString(hInst, IDS_SYNCMSG_FINISHED);
	wstrCancel_ = loadString(hInst, IDS_CANCEL);
	
	setWindowHandler(this, false);
}

qm::SyncStatusWindow::~SyncStatusWindow()
{
}

void qm::SyncStatusWindow::getWindowClass(WNDCLASS* pwc)
{
	DefaultWindowHandler::getWindowClass(pwc);
	pwc->hbrBackground = reinterpret_cast<HBRUSH>(COLOR_BTNFACE + 1);
}

LRESULT qm::SyncStatusWindow::windowProc(UINT uMsg,
										 WPARAM wParam,
										 LPARAM lParam)
{
	BEGIN_MESSAGE_HANDLER()
		HANDLE_CREATE()
		HANDLE_PAINT()
		HANDLE_SIZE()
		HANDLE_VSCROLL()
	END_MESSAGE_HANDLER()
	return DefaultWindowHandler::windowProc(uMsg, wParam, lParam);
}

void qm::SyncStatusWindow::start(unsigned int nParam)
{
	pSyncDialog_->resetCanceledTime();
	pSyncDialog_->setMessage(L"");
	class RunnableImpl : public Runnable
	{
	public:
		RunnableImpl(SyncDialog* pSyncDialog) :
			pSyncDialog_(pSyncDialog)
		{
		}
		
		virtual void run()
		{
			pSyncDialog_->enableCancel(true);
		}
	
	private:
		SyncDialog* pSyncDialog_;
	} runnable(pSyncDialog_);
	pSyncDialog_->getInitThread()->getSynchronizer()->syncExec(&runnable);
	
	if (nParam & SyncDialog::FLAG_SHOWDIALOG)
		pSyncDialog_->show();
}

void qm::SyncStatusWindow::end()
{
	bool bEmpty = false;
	bool bNewMessage = false;
	{
		Lock<CriticalSection> lock(cs_);
		bEmpty = listItem_.empty();
		if (bEmpty) {
			bNewMessage = bNewMessage_;
			bNewMessage_ = false;
		}
	}
	
	pSyncDialog_->setMessage(L"");
	class RunnableImpl : public Runnable
	{
	public:
		RunnableImpl(SyncDialog* pSyncDialog) :
			pSyncDialog_(pSyncDialog)
		{
		}
		
		virtual void run()
		{
			pSyncDialog_->enableCancel(false);
		}
	
	private:
		SyncDialog* pSyncDialog_;
	} runnable(pSyncDialog_);
	pSyncDialog_->getInitThread()->getSynchronizer()->syncExec(&runnable);
	
	if (bEmpty && !pSyncDialog_->hasError())
		pSyncDialog_->hide();
	
	if (bNewMessage)
		pSyncDialog_->notifyNewMessage();
}

void qm::SyncStatusWindow::startThread(unsigned int nId,
									   unsigned int nParam)
{
	std::auto_ptr<Item> pItem(new Item(nId, nParam));
	
	{
		Lock<CriticalSection> lock(cs_);
		listItem_.push_back(pItem.get());
		pItem.release();
	}
	
	invalidate(false);
	updateScrollBar();
}

void qm::SyncStatusWindow::endThread(unsigned int nId)
{
	{
		Lock<CriticalSection> lock(cs_);
		ItemList::iterator it = getItem(nId);
		delete *it;
		listItem_.erase(it);
	}
	
	invalidate(false);
	updateScrollBar();
	
	invalidate(false);
}

void qm::SyncStatusWindow::setPos(unsigned int nId,
								  bool bSub,
								  unsigned int nPos)
{
	Lock<CriticalSection> lock(cs_);
	ItemList::iterator it = getItem(nId);
	(*it)->setPos(bSub, nPos);
	
	if (!bSub) {
		(*it)->setRange(true, 0, 0);
		(*it)->setPos(true, 0);
	}
	
	invalidate(false);
}

void qm::SyncStatusWindow::setRange(unsigned int nId,
									bool bSub,
									unsigned int nMin,
									unsigned int nMax)
{
	Lock<CriticalSection> lock(cs_);
	ItemList::iterator it = getItem(nId);
	(*it)->setRange(bSub, nMin, nMax);
	invalidate(false);
}

void qm::SyncStatusWindow::setAccount(unsigned int nId,
									  Account* pAccount,
									  SubAccount* pSubAccount)
{
	Lock<CriticalSection> lock(cs_);
	ItemList::iterator it = getItem(nId);
	(*it)->setAccount(pAccount, pSubAccount);
	invalidate(false);
}

void qm::SyncStatusWindow::setFolder(unsigned int nId,
									 Folder* pFolder)
{
	Lock<CriticalSection> lock(cs_);
	ItemList::iterator it = getItem(nId);
	(*it)->setFolder(pFolder);
	invalidate(false);
}

void qm::SyncStatusWindow::setMessage(unsigned int nId,
									  const WCHAR* pwszMessage)
{
	if (nId == -1) {
		pSyncDialog_->setMessage(pwszMessage);
	}
	else {
		Lock<CriticalSection> lock(cs_);
		ItemList::iterator it = getItem(nId);
		(*it)->setMessage(pwszMessage);
		(*it)->setRange(true, 0, 0);
		(*it)->setPos(true, 0);
		invalidate(false);
	}
}

void qm::SyncStatusWindow::addError(unsigned int nId,
									const SessionErrorInfo& info)
{
	StringBuffer<WSTRING> buf;
	
	Account* pAccount = info.getAccount();
	if (pAccount) {
		buf.append(L"[");
		buf.append(pAccount->getName());
		
		SubAccount* pSubAccount = info.getSubAccount();
		if (*pSubAccount->getName()) {
			buf.append(L'/');
			buf.append(pSubAccount->getName());
		}
		
		NormalFolder* pFolder = info.getFolder();
		if (pFolder) {
			wstring_ptr wstrName(pFolder->getFullName());
			buf.append(L" - ");
			buf.append(wstrName.get());
		}
		
		buf.append(L"] ");
	}
	
	buf.append(info.getMessage());
	
	unsigned int nCode = info.getCode();
	if (nCode != 0) {
		WCHAR wszCode[32];
		swprintf(wszCode, L" (0x%08X)", info.getCode());
		buf.append(wszCode);
	}
	
	buf.append(L"\r\n");
	
	for (size_t n = 0; n < info.getDescriptionCount(); ++n) {
		const WCHAR* p = info.getDescription(n);
		if (p) {
			buf.append(L"  ");
			buf.append(p);
			buf.append(L"\r\n");
		}
	}
	
	pSyncDialog_->addError(buf.getCharArray());
}

bool qm::SyncStatusWindow::isCanceled(unsigned int nId,
									  bool bForce)
{
	unsigned int nCanceledTime = pSyncDialog_->getCanceledTime();
	if (nCanceledTime == 0)
		return false;
	else if (!bForce)
		return true;
	else
		return ::GetTickCount() - nCanceledTime > 10*1000;
}

wstring_ptr qm::SyncStatusWindow::selectDialupEntry()
{
	struct RunnableImpl : public Runnable
	{
		RunnableImpl(SyncDialog* pSyncDialog) :
			pSyncDialog_(pSyncDialog)
		{
		}
		
		virtual void run()
		{
			wstrEntry_ = pSyncDialog_->selectDialupEntry();
		}
		
		SyncDialog* pSyncDialog_;
		wstring_ptr wstrEntry_;
	} runnable(pSyncDialog_);
	pSyncDialog_->getInitThread()->getSynchronizer()->syncExec(&runnable);
	return runnable.wstrEntry_;
}

bool qm::SyncStatusWindow::showDialupDialog(RASDIALPARAMS* prdp)
{
	return pSyncDialog_->showDialupDialog(prdp);
}

void qm::SyncStatusWindow::notifyNewMessage(unsigned int nId)
{
	Lock<CriticalSection> lock(cs_);
	ItemList::iterator it = getItem(nId);
	if ((*it)->getParam() & SyncDialog::FLAG_NOTIFYNEWMESSAGE)
		bNewMessage_ = true;
}

LRESULT qm::SyncStatusWindow::onCreate(CREATESTRUCT* pCreateStruct)
{
	if (DefaultWindowHandler::onCreate(pCreateStruct) == -1)
		return -1;
	
	ClientDeviceContext dc(getHandle());
	ObjectSelector<HFONT> selector(dc,
		reinterpret_cast<HFONT>(::GetStockObject(DEFAULT_GUI_FONT)));
	TEXTMETRIC tm;
	dc.getTextMetrics(&tm);
	nFontHeight_ = tm.tmHeight + tm.tmExternalLeading;
	
	SIZE size;
	dc.getTextExtent(wstrCancel_.get(), wcslen(wstrCancel_.get()), &size);
	nCancelWidth_ = size.cx;
	
	return 0;
}

LRESULT qm::SyncStatusWindow::onPaint()
{
	PaintDeviceContext dc(getHandle());
	
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
	
	for (ItemList::size_type n = nPos; n < static_cast<ItemList::size_type>(nPos + nCount) && n < listItem_.size(); ++n) {
		rect.bottom = rect.top + nItemHeight;
		paintItem(&dc, rect, listItem_[n]);
		rect.top = rect.bottom;
	}
	
	rect.bottom = nBottom;
	dc.fillSolidRect(rect, ::GetSysColor(COLOR_BTNFACE));
	
	return 0;
}

LRESULT qm::SyncStatusWindow::onSize(UINT nFlags,
									 int cx,
									 int cy)
{
	updateScrollBar();
	invalidate();
	
	return DefaultWindowHandler::onSize(nFlags, cx, cy);
}

LRESULT qm::SyncStatusWindow::onVScroll(UINT nCode,
										UINT nPos,
										HWND hwnd)
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
	
	class RunnableImpl : public Runnable
	{
	public:
		RunnableImpl(Window* pWindow,
					 const SCROLLINFO& si) :
			pWindow_(pWindow),
			si_(si)
		{
		}
		
		virtual void run()
		{
			pWindow_->setScrollInfo(SB_VERT, si_);
		}
	
	private:
		Window* pWindow_;
		SCROLLINFO si_;
	} runnable(this, si);
	pSyncDialog_->getInitThread()->getSynchronizer()->syncExec(&runnable);
}

void qm::SyncStatusWindow::paintItem(DeviceContext* pdc,
									 const RECT& rect,
									 const Item* pItem)
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
		pwszMessage = wstrFinished_.get();
	
	pdc->setTextColor(::GetSysColor(COLOR_BTNTEXT));
	pdc->setBkColor(::GetSysColor(COLOR_BTNFACE));
	pdc->extTextOut(rectClip.left + 5, rectClip.top,
		ETO_CLIPPED | ETO_OPAQUE, rectClip,
		pwszMessage, wcslen(pwszMessage), 0);
}

void qm::SyncStatusWindow::paintProgress(qs::DeviceContext* pdc,
										 const RECT& rect,
										 const Item::Progress& progress)
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

SyncStatusWindow::ItemList::iterator qm::SyncStatusWindow::getItem(unsigned int nId)
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

qm::SyncStatusWindow::Item::Item(unsigned int nId,
								 unsigned int nParam) :
	nId_(nId),
	nParam_(nParam),
	pAccount_(0),
	pSubAccount_(0),
	pFolder_(0)
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
}

unsigned int qm::SyncStatusWindow::Item::getId() const
{
	return nId_;
}

unsigned int qm::SyncStatusWindow::Item::getParam() const
{
	return nParam_;
}

const SyncStatusWindow::Item::Progress& qm::SyncStatusWindow::Item::getProgress(bool bSub) const
{
	return bSub ? sub_ : main_;
}

const WCHAR* qm::SyncStatusWindow::Item::getMessage() const
{
	return wstrMessage_.get() ? wstrMessage_.get() : L"";
}

void qm::SyncStatusWindow::Item::setPos(bool bSub,
										unsigned int nPos)
{
	Progress& p = bSub ? sub_ : main_;
	p.nPos_ = nPos;
}

void qm::SyncStatusWindow::Item::setRange(bool bSub,
										  unsigned int nMin,
										  unsigned int nMax)
{
	Progress& p = bSub ? sub_ : main_;
	p.nMin_ = nMin;
	p.nMax_ = nMax;
}

void qm::SyncStatusWindow::Item::setAccount(Account* pAccount,
											SubAccount* pSubAccount)
{
	pAccount_ = pAccount;
	pSubAccount_ = pSubAccount;
	updateMessage();
}

void qm::SyncStatusWindow::Item::setFolder(Folder* pFolder)
{
	pFolder_ = pFolder;
	updateMessage();
}

void qm::SyncStatusWindow::Item::setMessage(const WCHAR* pwszMessage)
{
	wstrOriginalMessage_ = allocWString(pwszMessage);
	updateMessage();
}

void qm::SyncStatusWindow::Item::updateMessage()
{
	StringBuffer<WSTRING> buf;
	
	if (pAccount_) {
		buf.append(L'[');
		buf.append(pAccount_->getName());
		if (*pSubAccount_->getName()) {
			buf.append(L" (");
			buf.append(pSubAccount_->getName());
			buf.append(L')');
		}
		
		if (pFolder_) {
			buf.append(L" - ");
			wstring_ptr wstrFolder(pFolder_->getFullName());
			buf.append(wstrFolder.get());
		}
		buf.append(L"] ");
	}
	
	if (wstrOriginalMessage_.get())
		buf.append(wstrOriginalMessage_.get());
	
	wstrMessage_ = buf.getString();
}


/****************************************************************************
 *
 * SyncDialogThread
 *
 */

qm::SyncDialogThread::SyncDialogThread(qs::Profile* pProfile) :
	pProfile_(pProfile),
	pDialog_(0)
{
	pEvent_.reset(new Event(true, false));
}

qm::SyncDialogThread::~SyncDialogThread()
{
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

void qm::SyncDialogThread::run()
{
	InitThread init(InitThread::FLAG_SYNCHRONIZER);
	
	std::auto_ptr<SyncDialog> pDialog;
	{
		struct Set
		{
			Set(Event* pEvent) : pEvent_(pEvent) {}
			~Set() { pEvent_->set(); }
			Event* pEvent_;
		} set(pEvent_.get());
		
		pDialog.reset(new SyncDialog(pProfile_));
		Window wnd(Window::getForegroundWindow());
		pDialog->create(0);
		wnd.setForegroundWindow();
		pDialog_ = pDialog.get();
	}
	
	MSG msg;
	while (::GetMessage(&msg, 0, 0, 0)) {
		if (!::IsDialogMessage(pDialog_->getHandle(), &msg)) {
			::TranslateMessage(&msg);
			::DispatchMessage(&msg);
		}
	}
}
