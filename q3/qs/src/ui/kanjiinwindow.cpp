/*
 * $Id$
 *
 * Copyright(C) 1998-2003 Satoshi Nakamura
 * All rights reserved.
 *
 */

#include <qswindow.h>
#include <qsconv.h>
#include <qsstring.h>
#include <qserror.h>
#include <qsnew.h>

#include <tchar.h>

using namespace qs;

#ifdef QS_KANJIIN

/****************************************************************************
 *
 * KanjiinWindowImpl
 *
 */

struct qs::KanjiinWindowImpl
{
	enum {
		ID_IME_OK			= 41260,
		ID_IME_CHAR			= 41263,
		ID_IME_GETMODE		= 41265,
		ID_IME_SETMODE		= 41266,
		ID_IME_ACTIVEHWND	= 41264,
	};
	
	bool imeChar(HWND hwnd, WCHAR c);
	void imeExecute(HWND hwnd);
	
	KanjiinWindow* pHandler_;
	TSTRING tstrCommand_;
};

bool qs::KanjiinWindowImpl::imeChar(HWND hwnd, WCHAR c)
{
	if (c <= L' ')
		return false;
	
	HWND hwndIme = ::FindWindow(_T("KANJIIN"), 0);
	bool bKanji = false;
	if (hwndIme)
		bKanji = !::SendMessage(hwndIme, WM_COMMAND, KanjiinWindowImpl::ID_IME_GETMODE, 0);
	
	if (bKanji && c != L' ') {
		::SendMessage(hwndIme, WM_COMMAND, MAKEWPARAM(ID_IME_ACTIVEHWND, c),
			reinterpret_cast<LPARAM>(hwnd));
		return true;
	}
	return false;
}

void qs::KanjiinWindowImpl::imeExecute(HWND hwnd)
{
	HWND hwndIme = ::FindWindow(_T("KANJIIN"), 0);
	if (hwndIme) {
		bool bKanji = ::SendMessage(hwndIme, WM_COMMAND, ID_IME_GETMODE, 0) != 0;
		::SendMessage(hwndIme, WM_COMMAND, ID_IME_SETMODE, bKanji ? 0 : 1);
	}
	else {
		PROCESS_INFORMATION pi;
		TCHAR szParam[20];
		wsprintf(szParam, _T("%d"), reinterpret_cast<int>(hwnd));
		if (::CreateProcess(tstrCommand_, szParam, 0, 0, FALSE, 0, 0, 0, 0, &pi)) {
			::CloseHandle(pi.hThread);
			::CloseHandle(pi.hProcess);
		}
	}
}


/****************************************************************************
 *
 * KanjiinWindow
 *
 */

qs::KanjiinWindow::KanjiinWindow(bool bDeleteThis, QSTATUS* pstatus) :
	WindowBase(bDeleteThis, pstatus),
	DefaultWindowHandler(pstatus),
	pImpl_(0)
{
	if (*pstatus != QSTATUS_SUCCESS)
		return;
	
	DECLARE_QSTATUS();
	
	status = newObject(&pImpl_);
	CHECK_QSTATUS_SET(pstatus);
	pImpl_->pHandler_ = this;
	
	string_ptr<WSTRING> wstrCommand;
	Registry reg(HKEY_CURRENT_USER, L"Software\\Gawaro", &status);
	CHECK_QSTATUS_SET(pstatus);
	if (reg) {
		status = reg.getValue(L"FepProgram", &wstrCommand);
		CHECK_QSTATUS_SET(pstatus);
	}
	if (!wstrCommand.get())
		wstrCommand.reset(allocWString(L"KANJIIN.EXE"));
#ifdef UNICODE
	pImpl_->tstrCommand_ = wstrCommand.release();
#else
	pImpl_->tstrCommand_ = wcs2tcs(wstrCommand.get());
#endif
	
	setWindowHandler(this, false);
}

qs::KanjiinWindow::~KanjiinWindow()
{
	freeTString(pImpl_->tstrCommand_);
	delete pImpl_;
	pImpl_ = 0;
}

LRESULT qs::KanjiinWindow::windowProc(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	BEGIN_MESSAGE_HANDLER()
		HANDLE_CHAR()
		HANDLE_COMMAND()
		HANDLE_SYSCHAR()
		HANDLE_SYSKEYDOWN()
	END_MESSAGE_HANDLER()
	return DefaultWindowHandler::windowProc(uMsg, wParam, lParam);
}

LRESULT qs::KanjiinWindow::onChar(UINT nChar, UINT nRepeat, UINT nFlags)
{
	if (pImpl_->imeChar(getHandle(), static_cast<WCHAR>(nChar)))
		return 1;
	return DefaultWindowHandler::onChar(nChar, nRepeat, nFlags);
}

LRESULT qs::KanjiinWindow::onCommand(UINT nCode, UINT nId, HWND hwnd)
{
	switch (nId) {
	case KanjiinWindowImpl::ID_IME_OK:
		return KanjiinWindowImpl::ID_IME_CHAR;
	case KanjiinWindowImpl::ID_IME_CHAR:
		sendMessage(WM_CHAR,
			reinterpret_cast<WPARAM>(hwnd),
			reinterpret_cast<LPARAM>(hwnd));
		return 0;
	}
	return DefaultWindowHandler::onCommand(nCode, nId, hwnd);
}

LRESULT qs::KanjiinWindow::onSysChar(UINT nKey, UINT nRepeat, UINT nFlags)
{
	if (static_cast<WCHAR>(nKey) == L' ' && nFlags & 0x20000000)
		return 0;
	return DefaultWindowHandler::onSysChar(nKey, nRepeat, nFlags);
}

LRESULT qs::KanjiinWindow::onSysKeyDown(UINT nKey, UINT nRepeat, UINT nFlags)
{
	if (static_cast<WCHAR>(nKey) == L' ' && nFlags & 0x20000000) {
		pImpl_->imeExecute(getHandle());
		return 0;
	}
	return DefaultWindowHandler::onSysKeyDown(nKey, nRepeat, nFlags);
}

#endif // QS_KANJIIN
