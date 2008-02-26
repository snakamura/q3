/*
 * $Id$
 *
 * Copyright(C) 1998-2008 Satoshi Nakamura
 *
 */

#pragma warning(disable:4786)

#include <qmaccount.h>
#include <qmfolder.h>
#include <qmpassword.h>

#include <qsconv.h>
#include <qsinit.h>

#include <algorithm>

#include <tchar.h>

#include "dialogs.h"
#include "resourceinc.h"
#include "syncdialog.h"
#include "uiutil.h"
#include "../main/main.h"

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

qm::SyncDialogManager::SyncDialogManager(Profile* pProfile,
										 PasswordManager* pPasswordManager) :
	pProfile_(pProfile),
	pPasswordManager_(pPasswordManager)
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
		std::auto_ptr<SyncDialogThread> pThread(
			new SyncDialogThread(pProfile_, pPasswordManager_));
		if (!pThread->start())
			return 0;
		pThread_ = pThread;
	}
	return pThread_->getDialog();
}

void qm::SyncDialogManager::hide()
{
	if (pThread_.get()) {
		SyncDialog* pSyncDialog = pThread_->getDialog();
		pSyncDialog->hide();
	}
}

void qm::SyncDialogManager::save() const
{
	if (pThread_.get()) {
		SyncDialog* pSyncDialog = pThread_->getDialog();
		struct RunnableImpl : public Runnable
		{
			RunnableImpl(SyncDialog* pSyncDialog) :
				pSyncDialog_(pSyncDialog)
			{
			}
			
			virtual void run()
			{
				pSyncDialog_->save();
			}
			
			SyncDialog* pSyncDialog_;
		} runnable(pSyncDialog);
		pSyncDialog->getInitThread()->getSynchronizer()->syncExec(&runnable);
	}
}

ErrorCallback* qm::SyncDialogManager::getErrorCallback()
{
	if (!pErrorCallback_.get())
		pErrorCallback_.reset(new ErrorCallbackImpl(this));
	return pErrorCallback_.get();
}


/****************************************************************************
 *
 * SyncDialogManager::ErrorCallbackImpl
 *
 */

qm::SyncDialogManager::ErrorCallbackImpl::ErrorCallbackImpl(SyncDialogManager* pManager) :
	pManager_(pManager)
{
}

qm::SyncDialogManager::ErrorCallbackImpl::~ErrorCallbackImpl()
{
}

void qm::SyncDialogManager::ErrorCallbackImpl::addError(const SessionErrorInfo& info)
{
	// This method may be called from sync threads,
	// but in that case, SyncDialog should have been created.
	SyncDialog* pSyncDialog = pManager_->open();
	if (pSyncDialog)
		pSyncDialog->addError(info);
}


/****************************************************************************
 *
 * SyncDialog
 *
 */

qm::SyncDialog::SyncDialog(Profile* pProfile,
						   PasswordManager* pPasswordManager) :
	Dialog(getResourceHandle(), IDD_SYNC, IDD_SYNC, false),
	pProfile_(pProfile),
	pPasswordManager_(pPasswordManager),
	pStatusWindow_(0),
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
	if (::GetCurrentThreadId() != ::GetWindowThreadProcessId(getHandle(), 0)) {
		struct RunnableImpl : public Runnable
		{
			RunnableImpl(SyncDialog* pSyncDialog) :
				pSyncDialog_(pSyncDialog)
			{
			}
			
			virtual void run()
			{
				pSyncDialog_->show();
			}
			
			SyncDialog* pSyncDialog_;
		} runnable(this);
		getInitThread()->getSynchronizer()->syncExec(&runnable);
	}
	else {
		if (!isVisible()) {
			layout();
			showWindow(SW_SHOWNA);
		}
		setForegroundWindow();
	}
}

void qm::SyncDialog::hide()
{
	if (::GetCurrentThreadId() != ::GetWindowThreadProcessId(getHandle(), 0)) {
		struct RunnableImpl : public Runnable
		{
			RunnableImpl(SyncDialog* pSyncDialog) :
				pSyncDialog_(pSyncDialog)
			{
			}
			
			virtual void run()
			{
				pSyncDialog_->hide();
			}
			
			SyncDialog* pSyncDialog_;
		} runnable(this);
		getInitThread()->getSynchronizer()->syncExec(&runnable);
	}
	else {
		if (!isVisible())
			return;
		
		setDlgItemText(IDC_ERROR, L"");
		
		showWindow(SW_HIDE);
		if (Window::getForegroundWindow() == getHandle())
			getMainWindow()->setForegroundWindow();
	}
}

void qm::SyncDialog::setMessage(const WCHAR* pwszMessage)
{
	assert(::GetCurrentThreadId() == ::GetWindowThreadProcessId(getHandle(), 0));
	
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

void qm::SyncDialog::addError(const SessionErrorInfo& info)
{
	struct RunnableImpl : public Runnable
	{
		RunnableImpl(SyncDialog* pSyncDialog,
					 wstring_ptr wstrError) :
			pSyncDialog_(pSyncDialog),
			wstrError_(wstrError)
		{
		}
		
		virtual void run()
		{
			pSyncDialog_->addError(wstrError_.get());
		}
		
		SyncDialog* pSyncDialog_;
		wstring_ptr wstrError_;
	};
	std::auto_ptr<Runnable> pRunnable(new RunnableImpl(this, formatError(info)));
	getInitThread()->getSynchronizer()->asyncExec(pRunnable);
}

void qm::SyncDialog::addError(const WCHAR* pwszError)
{
	assert(pwszError);
	assert(::GetCurrentThreadId() == ::GetWindowThreadProcessId(getHandle(), 0));
	
	bool bHasError = hasError();
	
	sendDlgItemMessage(IDC_ERROR, EM_SETSEL, -1, -1);
	W2T(pwszError, ptszError);
	sendDlgItemMessage(IDC_ERROR, EM_REPLACESEL,
		FALSE, reinterpret_cast<LPARAM>(ptszError));
	
	if (!bHasError) {
		layout();
		if (!isVisible()) {
			showWindow(SW_SHOWNA);
			setWindowPos(HWND_BOTTOM, 0, 0, 0, 0,
				SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
		}
#ifndef _WIN32_WCE
		flashWindow(false);
#endif
	}
}

bool qm::SyncDialog::hasError() const
{
	assert(::GetCurrentThreadId() == ::GetWindowThreadProcessId(getHandle(), 0));
	return Window(getDlgItem(IDC_ERROR)).getWindowTextLength() != 0;
}

void qm::SyncDialog::enableCancel(bool bEnable)
{
	assert(::GetCurrentThreadId() == ::GetWindowThreadProcessId(getHandle(), 0));
	
	if (bEnable)
		Window(getDlgItem(IDC_CANCEL)).enableWindow(bEnable);
	
	UINT nOldId = bEnable ? IDC_HIDE : IDC_CANCEL;
	UINT nNewId = bEnable ? IDC_CANCEL : IDC_HIDE;
	if (isActive())
		Window(getDlgItem(nNewId)).setFocus();
	
	if (!bEnable)
		Window(getDlgItem(IDC_CANCEL)).enableWindow(bEnable);
	
	sendDlgItemMessage(nOldId, BM_SETSTYLE, BS_PUSHBUTTON, TRUE);
	sendMessage(DM_SETDEFID, nNewId);
	sendDlgItemMessage(nNewId, BM_SETSTYLE, BS_DEFPUSHBUTTON, TRUE);
}

PasswordState qm::SyncDialog::getPassword(SubAccount* pSubAccount,
										  Account::Host host,
										  wstring_ptr* pwstrPassword)
{
	assert(::GetCurrentThreadId() == ::GetWindowThreadProcessId(getHandle(), 0));
	
	PasswordState state = PASSWORDSTATE_NONE;
	Account* pAccount = pSubAccount->getAccount();
	AccountPasswordCondition condition(pAccount, pSubAccount, host);
	*pwstrPassword = pPasswordManager_->getPassword(condition, false, 0);
	if (pwstrPassword->get()) {
		state = PASSWORDSTATE_ONETIME;
	}
	else {
		show();
		*pwstrPassword = pPasswordManager_->getPassword(condition, false, &state);
		if (!pwstrPassword->get())
			state = PASSWORDSTATE_NONE;
	}
	return state;
}

void qm::SyncDialog::setPassword(SubAccount* pSubAccount,
								 Account::Host host,
								 const WCHAR* pwszPassword,
								 bool bPermanent)
{
	assert(::GetCurrentThreadId() == ::GetWindowThreadProcessId(getHandle(), 0));
	
	Account* pAccount = pSubAccount->getAccount();
	AccountPasswordCondition condition(pAccount, pSubAccount, host);
	pPasswordManager_->setPassword(condition, pwszPassword, bPermanent);
}

bool qm::SyncDialog::showDialupDialog(RASDIALPARAMS* prdp)
{
	assert(::GetCurrentThreadId() == ::GetWindowThreadProcessId(getHandle(), 0));
	assert(prdp);
	
	show();
	
	T2W(prdp->szEntryName, pwszEntryName);
	T2W(prdp->szUserName, pwszUserName);
	T2W(prdp->szPassword, pwszPassword);
	T2W(prdp->szDomain, pwszDomain);
	
	DialupDialog dialog(pwszEntryName, pwszUserName, pwszPassword, pwszDomain);
	if (dialog.doModal(getHandle()) != IDOK)
		return false;
	
	W2T(dialog.getUserName(), ptszUserName);
	_tcsncpy(prdp->szUserName, ptszUserName, UNLEN);
	W2T(dialog.getPassword(), ptszPassword);
	_tcsncpy(prdp->szPassword, ptszPassword, PWLEN);
	W2T(dialog.getDomain(), ptszDomain);
	_tcsncpy(prdp->szDomain, ptszDomain, DNLEN);
	
	return true;
}

wstring_ptr qm::SyncDialog::selectDialupEntry()
{
	assert(::GetCurrentThreadId() == ::GetWindowThreadProcessId(getHandle(), 0));
	
	show();
	
	SelectDialupEntryDialog dialog(pProfile_);
	if (dialog.doModal(getHandle()) != IDOK)
		return 0;
	return allocWString(dialog.getEntry());
}

void qm::SyncDialog::notifyNewMessage() const
{
	assert(::GetCurrentThreadId() == ::GetWindowThreadProcessId(getHandle(), 0));
	
	wstring_ptr wstrSound(pProfile_->getString(L"Sync", L"Sound"));
	if (*wstrSound.get()) {
		W2T(wstrSound.get(), ptszSound);
		sndPlaySound(ptszSound, SND_ASYNC);
	}
	
	wstring_ptr wstrCommand(pProfile_->getString(L"Sync", L"Command"));
	if (*wstrCommand.get())
		Process::shellExecute(wstrCommand.get(), getHandle());
}

bool qm::SyncDialog::isShowDialog(SyncData::Type type) const
{
	switch (pProfile_->getInt(L"SyncDialog", L"Show")) {
	case SHOW_ALWAYS:
		return true;
	case SHOW_NEVER:
		return false;
	case SHOW_MANUAL:
	default:
		return type == SyncData::TYPE_MANUAL;
	}
}

void qm::SyncDialog::save() const
{
	assert(::GetCurrentThreadId() == ::GetWindowThreadProcessId(getHandle(), 0));
	
#ifndef _WIN32_WCE
	RECT rect;
	getWindowRect(&rect);
	pProfile_->setInt(L"SyncDialog", L"Left", rect.left);
	pProfile_->setInt(L"SyncDialog", L"Top", rect.top);
	pProfile_->setInt(L"SyncDialog", L"Width", rect.right - rect.left);
	pProfile_->setInt(L"SyncDialog", L"Height", rect.bottom - rect.top);
#endif
}

INT_PTR qm::SyncDialog::dialogProc(UINT uMsg,
								   WPARAM wParam,
								   LPARAM lParam)
{
	BEGIN_DIALOG_HANDLER()
		HANDLE_ACTIVATE()
		HANDLE_CLOSE()
		HANDLE_DESTROY()
		HANDLE_INITDIALOG()
		HANDLE_SIZE()
		HANDLE_SYSCOMMAND()
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
	return CommandHandler::onCommand(nCode, nId);
}

LRESULT qm::SyncDialog::onActivate(UINT nFlags,
								   HWND hwnd,
								   bool bMinimized)
{
	if (nFlags == WA_ACTIVE) {
		LRESULT nId = sendMessage(DM_GETDEFID);
		if (HIWORD(nId) == DC_HASDEFID) {
			Window wnd(getDlgItem(LOWORD(nId)));
			if (!wnd.hasFocus())
				wnd.setFocus();
		}
	}
	
	return DefaultDialogHandler::onActivate(nFlags, hwnd, bMinimized);
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
	int nLeft = 0;
	int nTop = 0;
	RECT rectWorkArea;
	::SystemParametersInfo(SPI_GETWORKAREA, 0, &rectWorkArea, 0);
	int nWidth = rectWorkArea.right - rectWorkArea.left;
	int nHeight = rectWorkArea.bottom - rectWorkArea.top;
#else
	int nLeft = pProfile_->getInt(L"SyncDialog", L"Left");
	int nTop = pProfile_->getInt(L"SyncDialog", L"Top");
	int nWidth = pProfile_->getInt(L"SyncDialog", L"Width");
	int nHeight = pProfile_->getInt(L"SyncDialog", L"Height");
#endif
	setWindowPos(0, nLeft, nTop, nWidth, nHeight, SWP_NOZORDER);
	
#if !defined _WIN32_WCE && _WIN32_WINNT >= 0x500
	UIUtil::setWindowAlpha(getHandle(), pProfile_, L"SyncDialog");
#endif
	
	return TRUE;
}

LRESULT qm::SyncDialog::onSize(UINT nFlags,
							   int cx,
							   int cy)
{
	layout(cx, cy);
	return DefaultDialogHandler::onSize(nFlags, cx, cy);
}

LRESULT qm::SyncDialog::onSysCommand(UINT nId,
									 LPARAM lParam)
{
	if (nId == SC_CLOSE)
		return 0;
	return DefaultDialogHandler::onSysCommand(nId, lParam);
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
	bool bShowError = hasError();
	
	HDWP hdwp = Window::beginDeferWindowPos(6);
	
#if defined _WIN32_WCE && _WIN32_WCE >= 0x300 && defined _WIN32_WCE_PSPC
	int nErrorHeight = bShowError ? (cy - nButtonHeight - 30)/2 : 0;
	hdwp = error.deferWindowPos(hdwp, 0, 5,
		cy - nErrorHeight - nButtonHeight - 10, cx - 10, nErrorHeight,
		SWP_NOZORDER | SWP_NOACTIVATE);
	error.showWindow(bShowError);
	
	hdwp = message.deferWindowPos(hdwp, 0, 5, 5, cx - 10,
		nMessageHeight, SWP_NOZORDER | SWP_NOACTIVATE);
	hdwp = pStatusWindow_->deferWindowPos(hdwp, 0, 5, nMessageHeight + 10, cx - 10,
		cy - nErrorHeight - nButtonHeight - nMessageHeight - (bShowError ? 25 : 20),
		SWP_NOZORDER | SWP_NOACTIVATE);
	
	hdwp = cancel.deferWindowPos(hdwp, 0, cx - nButtonWidth*2 - 10,
		cy - nButtonHeight - 5, 0, 0, SWP_NOZORDER | SWP_NOSIZE | SWP_NOACTIVATE);
	hdwp = hide.deferWindowPos(hdwp, 0, cx - nButtonWidth - 5,
		cy - nButtonHeight - 5, 0, 0, SWP_NOZORDER | SWP_NOSIZE | SWP_NOACTIVATE);
#else
	int nErrorHeight = bShowError ? (cy - 25)/2 : 0;
	hdwp = error.deferWindowPos(hdwp, 0, 5,
		cy - nErrorHeight - 5, cx - nButtonWidth - 15, nErrorHeight,
		SWP_NOZORDER | SWP_NOACTIVATE);
	error.showWindow(bShowError);
	
	hdwp = message.deferWindowPos(hdwp, 0, 5, 5, cx - nButtonWidth - 15,
		nMessageHeight, SWP_NOZORDER | SWP_NOACTIVATE);
	hdwp = pStatusWindow_->deferWindowPos(hdwp, 0, 5,
		nMessageHeight + 10, cx - nButtonWidth - 15,
		cy - nErrorHeight - nMessageHeight - (bShowError ? 20 : 15),
		SWP_NOZORDER | SWP_NOACTIVATE);
	hdwp = cancel.deferWindowPos(hdwp, 0, cx - nButtonWidth - 5,
		5, 0, 0, SWP_NOZORDER | SWP_NOSIZE | SWP_NOACTIVATE);
	hdwp = hide.deferWindowPos(hdwp, 0, cx - nButtonWidth - 5, nButtonHeight + 8,
		0, 0, SWP_NOZORDER | SWP_NOSIZE | SWP_NOACTIVATE);
	
#ifndef _WIN32_WCE
	Window sizeGrip(getDlgItem(IDC_SIZEGRIP));
	RECT rectSizeGrip;
	sizeGrip.getWindowRect(&rectSizeGrip);
	hdwp = sizeGrip.deferWindowPos(hdwp, 0,
		cx - (rectSizeGrip.right - rectSizeGrip.left),
		cy - (rectSizeGrip.bottom - rectSizeGrip.top),
		0, 0, SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE);
#endif
#endif
	
	Window::endDeferWindowPos(hdwp);
}

wstring_ptr qm::SyncDialog::formatError(const SessionErrorInfo& info)
{
	StringBuffer<WSTRING> buf;
	
	Account* pAccount = info.getAccount();
	if (pAccount) {
		buf.append(L"[");
		buf.append(pAccount->getName());
		
		SubAccount* pSubAccount = info.getSubAccount();
		if (pSubAccount && *pSubAccount->getName()) {
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
		_snwprintf(wszCode, countof(wszCode), L" (0x%08X)", info.getCode());
		buf.append(wszCode);
	}
	
	buf.append(L"\r\n");
	
	for (size_t n = 0; n < info.getDescriptionCount(); ++n) {
		const WCHAR* p = info.getDescription(n);
		if (p) {
			while (p && *p) {
				const WCHAR* pNewLine = wcschr(p, L'\n');
				if (pNewLine) {
					buf.append(L"  ");
					buf.append(p, pNewLine - p);
					buf.append(L"\r\n");
					p = pNewLine + 1;
				}
				else {
					buf.append(L"  ");
					buf.append(p);
					buf.append(L"\r\n");
					p = 0;
				}
			}
		}
	}
	
	return buf.getString();
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
	nFontHeight_(0)
{
	wstrFinished_ = loadString(getResourceHandle(), IDS_MESSAGE_FINISHED);
	
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

void qm::SyncStatusWindow::start(SyncData::Type type)
{
	bool bShow = pSyncDialog_->isShowDialog(type);
	
	struct RunnableImpl : public Runnable
	{
		RunnableImpl(SyncDialog* pSyncDialog,
					 bool bShow) :
			pSyncDialog_(pSyncDialog),
			bShow_(bShow)
		{
		}
		
		virtual void run()
		{
			pSyncDialog_->resetCanceledTime();
			pSyncDialog_->setMessage(L"");
			
			if (bShow_)
				pSyncDialog_->show();
			
			pSyncDialog_->enableCancel(true);
		}
		
		SyncDialog* pSyncDialog_;
		bool bShow_;
	} runnable(pSyncDialog_, bShow);
	pSyncDialog_->getInitThread()->getSynchronizer()->syncExec(&runnable);
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
	
	struct RunnableImpl : public Runnable
	{
		RunnableImpl(SyncDialog* pSyncDialog,
					 bool bEmpty,
					 bool bNewMessage) :
			pSyncDialog_(pSyncDialog),
			bEmpty_(bEmpty),
			bNewMessage_(bNewMessage)
		{
		}
		
		virtual void run()
		{
			if (bEmpty_) {
				pSyncDialog_->setMessage(L"");
				pSyncDialog_->enableCancel(false);
				
				if (!pSyncDialog_->hasError())
					pSyncDialog_->hide();
			}
			
			if (bNewMessage_)
				pSyncDialog_->notifyNewMessage();
		}
		
		SyncDialog* pSyncDialog_;
		bool bEmpty_;
		bool bNewMessage_;
	} runnable(pSyncDialog_, bEmpty, bNewMessage);
	pSyncDialog_->getInitThread()->getSynchronizer()->syncExec(&runnable);
}

void qm::SyncStatusWindow::startThread(unsigned int nId,
									   SyncData::Type type)
{
	std::auto_ptr<Item> pItem(new Item(nId));
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
}

void qm::SyncStatusWindow::setPos(unsigned int nId,
								  bool bSub,
								  size_t nPos)
{
	{
		Lock<CriticalSection> lock(cs_);
		ItemList::iterator it = getItem(nId);
		(*it)->setPos(bSub, nPos);
		
		if (!bSub) {
			(*it)->setRange(true, 0, 0);
			(*it)->setPos(true, 0);
		}
	}
	invalidate(false);
}

void qm::SyncStatusWindow::setRange(unsigned int nId,
									bool bSub,
									size_t nMin,
									size_t nMax)
{
	{
		Lock<CriticalSection> lock(cs_);
		ItemList::iterator it = getItem(nId);
		(*it)->setRange(bSub, nMin, nMax);
	}
	invalidate(false);
}

void qm::SyncStatusWindow::setAccount(unsigned int nId,
									  Account* pAccount,
									  SubAccount* pSubAccount)
{
	{
		Lock<CriticalSection> lock(cs_);
		ItemList::iterator it = getItem(nId);
		(*it)->setAccount(pAccount, pSubAccount);
	}
	invalidate(false);
}

void qm::SyncStatusWindow::setFolder(unsigned int nId,
									 Folder* pFolder)
{
	{
		Lock<CriticalSection> lock(cs_);
		ItemList::iterator it = getItem(nId);
		(*it)->setFolder(pFolder);
	}
	invalidate(false);
}

void qm::SyncStatusWindow::setMessage(unsigned int nId,
									  const WCHAR* pwszMessage)
{
	if (nId == -1) {
		struct RunnableImpl : public Runnable
		{
			RunnableImpl(SyncDialog* pSyncDialog,
						 const WCHAR* pwszMessage) :
				pSyncDialog_(pSyncDialog),
				pwszMessage_(pwszMessage)
			{
			}
			
			virtual void run()
			{
				pSyncDialog_->setMessage(pwszMessage_);
			}
			
			SyncDialog* pSyncDialog_;
			const WCHAR* pwszMessage_;
		} runnable(pSyncDialog_, pwszMessage);
		pSyncDialog_->getInitThread()->getSynchronizer()->syncExec(&runnable);
	}
	else {
		{
			Lock<CriticalSection> lock(cs_);
			ItemList::iterator it = getItem(nId);
			(*it)->setMessage(pwszMessage);
			(*it)->setRange(true, 0, 0);
			(*it)->setPos(true, 0);
		}
		invalidate(false);
	}
}

void qm::SyncStatusWindow::addError(unsigned int nId,
									const SessionErrorInfo& info)
{
	SubAccount* pSubAccount = info.getSubAccount();
	if (pSubAccount && pSubAccount->getPropertyInt(L"Misc", L"IgnoreError"))
		return;
	
	pSyncDialog_->addError(info);
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
		return ::GetTickCount() - nCanceledTime > 5*1000;
}

PasswordState qm::SyncStatusWindow::getPassword(SubAccount* pSubAccount,
												Account::Host host,
												wstring_ptr* pwstrPassword)
{
	struct RunnableImpl : public Runnable
	{
		RunnableImpl(SyncDialog* pSyncDialog,
					 SubAccount* pSubAccount,
					 Account::Host host,
					 wstring_ptr* pwstrPassword) :
			pSyncDialog_(pSyncDialog),
			pSubAccount_(pSubAccount),
			host_(host),
			pwstrPassword_(pwstrPassword),
			state_(PASSWORDSTATE_NONE)
		{
		}
		
		virtual void run()
		{
			state_ = pSyncDialog_->getPassword(pSubAccount_, host_, pwstrPassword_);
		}
		
		SyncDialog* pSyncDialog_;
		SubAccount* pSubAccount_;
		Account::Host host_;
		wstring_ptr* pwstrPassword_;
		PasswordState state_;
	} runnable(pSyncDialog_, pSubAccount, host, pwstrPassword);
	pSyncDialog_->getInitThread()->getSynchronizer()->syncExec(&runnable);
	return runnable.state_;
}

void qm::SyncStatusWindow::setPassword(SubAccount* pSubAccount,
									   Account::Host host,
									   const WCHAR* pwszPassword,
									   bool bPermanent)
{
	struct RunnableImpl : public Runnable
	{
		RunnableImpl(SyncDialog* pSyncDialog,
					 SubAccount* pSubAccount,
					 Account::Host host,
					 const WCHAR* pwszPassword,
					 bool bPermanent) :
			pSyncDialog_(pSyncDialog),
			pSubAccount_(pSubAccount),
			host_(host),
			pwszPassword_(pwszPassword),
			bPermanent_(bPermanent)
		{
		}
		
		virtual void run()
		{
			pSyncDialog_->setPassword(pSubAccount_, host_, pwszPassword_, bPermanent_);
		}
		
		SyncDialog* pSyncDialog_;
		SubAccount* pSubAccount_;
		Account::Host host_;
		const WCHAR* pwszPassword_;
		bool bPermanent_;
	} runnable(pSyncDialog_, pSubAccount, host, pwszPassword, bPermanent);
	pSyncDialog_->getInitThread()->getSynchronizer()->syncExec(&runnable);
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
	struct RunnableImpl : public Runnable
	{
		RunnableImpl(SyncDialog* pSyncDialog,
					 RASDIALPARAMS* prdp) :
			pSyncDialog_(pSyncDialog),
			prdp_(prdp),
			b_(false)
		{
		}
		
		virtual void run()
		{
			b_ = pSyncDialog_->showDialupDialog(prdp_);
		}
		
		SyncDialog* pSyncDialog_;
		RASDIALPARAMS* prdp_;
		bool b_;
	} runnable(pSyncDialog_, prdp);
	pSyncDialog_->getInitThread()->getSynchronizer()->syncExec(&runnable);
	return runnable.b_;
}

void qm::SyncStatusWindow::notifyNewMessage(unsigned int nId)
{
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
		nItemCount = static_cast<int>(listItem_.size());
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
	
	struct RunnableImpl : public Runnable
	{
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
		ETO_CLIPPED | ETO_OPAQUE, rectClip, pwszMessage,
		static_cast<UINT>(wcslen(pwszMessage)), 0);
}

void qm::SyncStatusWindow::paintProgress(qs::DeviceContext* pdc,
										 const RECT& rect,
										 const Item::Progress& progress)
{
	size_t nMin = progress.nMin_;
	size_t nMax = progress.nMax_;
	size_t nPos = progress.nPos_;
	if (nPos < nMin)
		nPos = nMin;
	else if (nPos > nMax)
		nPos = nMax;
	
	double dPercent = nMax == nMin ? 0 : static_cast<double>(nPos - nMin)/(nMax - nMin);
	RECT rectHighlight = {
		rect.left,
		rect.top,
		rect.left + static_cast<long>((rect.right - rect.left)*dPercent),
		rect.bottom
	};
	pdc->fillSolidRect(rectHighlight, ::GetSysColor(COLOR_ACTIVECAPTION));
	RECT rectUnHighlight = {
		rectHighlight.right,
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
		_snwprintf(wsz, countof(wsz), L"%u/%u", static_cast<unsigned int>(nPos - nMin),
			static_cast<unsigned int>(nMax - nMin));
	
	SIZE size;
	pdc->getTextExtent(wsz, static_cast<int>(wcslen(wsz)), &size);
	int nLeft = rect.left + ((rect.right - rect.left) - size.cx)/2;
	int nTop = rect.top;
	
	pdc->setBkMode(TRANSPARENT);
	pdc->setTextColor(::GetSysColor(COLOR_CAPTIONTEXT));
	pdc->extTextOut(nLeft, nTop, ETO_CLIPPED, rectHighlight,
		wsz, static_cast<UINT>(wcslen(wsz)), 0);
	pdc->setTextColor(::GetSysColor(COLOR_BTNTEXT));
	pdc->extTextOut(nLeft, nTop, ETO_CLIPPED, rectUnHighlight,
		wsz, static_cast<UINT>(wcslen(wsz)), 0);
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

qm::SyncStatusWindow::Item::Item(unsigned int nId) :
	nId_(nId),
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

const SyncStatusWindow::Item::Progress& qm::SyncStatusWindow::Item::getProgress(bool bSub) const
{
	return bSub ? sub_ : main_;
}

const WCHAR* qm::SyncStatusWindow::Item::getMessage() const
{
	return wstrMessage_.get() ? wstrMessage_.get() : L"";
}

void qm::SyncStatusWindow::Item::setPos(bool bSub,
										size_t nPos)
{
	Progress& p = bSub ? sub_ : main_;
	p.nPos_ = nPos;
}

void qm::SyncStatusWindow::Item::setRange(bool bSub,
										  size_t nMin,
										  size_t nMax)
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

qm::SyncDialogThread::SyncDialogThread(qs::Profile* pProfile,
									   PasswordManager* pPasswordManager) :
	pProfile_(pProfile),
	pPasswordManager_(pPasswordManager),
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
		
		pDialog.reset(new SyncDialog(pProfile_, pPasswordManager_));
		Window wnd(Window::getForegroundWindow());
		pDialog->create(0);
		if (wnd)
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
