/*
 * $Id$
 *
 * Copyright(C) 1998-2003 Satoshi Nakamura
 * All rights reserved.
 *
 */

#ifndef __QSWINDOW_INL__
#define __QSWINDOW_INL__

#include <qsassert.h>


/****************************************************************************
 *
 * Window
 *
 */

inline qs::Window::Window(HWND hwnd) :
	hwnd_(hwnd)
{
}

inline qs::Window::Window(const Window& window) :
	hwnd_(window.hwnd_)
{
}

inline qs::Window::~Window()
{
}

inline qs::Window::operator HWND() const
{
	return hwnd_;
}

inline qs::Window& qs::Window::operator=(HWND hwnd)
{
	hwnd_ = hwnd;
	return *this;
}

inline qs::Window& qs::Window::operator=(const Window& window)
{
	hwnd_ = window.hwnd_;
	return *this;
}

inline bool qs::Window::operator!() const
{
	return !hwnd_;
}

inline HWND qs::Window::getHandle() const
{
	return hwnd_;
}

inline bool qs::Window::updateWindow()
{
	assert(hwnd_);
	return ::UpdateWindow(hwnd_) != 0;
}

inline bool qs::Window::destroyWindow()
{
	assert(hwnd_);
	return ::DestroyWindow(hwnd_) != 0;
}

inline bool qs::Window::showWindow()
{
	return showWindow(SW_SHOW);
}

inline bool qs::Window::showWindow(int nShow)
{
	assert(hwnd_);
	return ::ShowWindow(hwnd_, nShow) != 0;
}

inline bool qs::Window::isVisible() const
{
	assert(hwnd_);
	return ::IsWindowVisible(hwnd_) != 0;
}

inline bool qs::Window::isIconic() const
{
#ifdef _WIN32_WCE
	return !isWindowEnabled();
#else
	assert(hwnd_);
	return ::IsIconic(hwnd_) != 0;
#endif
}

inline DWORD qs::Window::getStyle() const
{
	return getWindowLong(GWL_STYLE);
}

inline bool qs::Window::enableWindow()
{
	return enableWindow(true);
}

inline bool qs::Window::enableWindow(bool bEnable)
{
	assert(hwnd_);
	return ::EnableWindow(hwnd_, bEnable) != 0;
}

inline bool qs::Window::isWindowEnabled() const
{
	assert(hwnd_);
	return ::IsWindowEnabled(hwnd_) != 0;
}

inline HWND qs::Window::getWindow(UINT nCmd) const
{
	assert(hwnd_);
	return ::GetWindow(hwnd_, nCmd);
}

inline bool qs::Window::getClientRect(RECT* pRect) const
{
	assert(hwnd_);
	assert(pRect);
	return ::GetClientRect(hwnd_, pRect) != 0;
}

inline bool qs::Window::getWindowRect(RECT* pRect) const
{
	assert(hwnd_);
	assert(pRect);
	return ::GetWindowRect(hwnd_, pRect) != 0;
}

inline bool qs::Window::setWindowPos(HWND hwnd, int x, int y, int cx, int cy, UINT nFlags)
{
	assert(hwnd_);
	return ::SetWindowPos(hwnd_, hwnd, x, y, cx, cy, nFlags) != 0;
}

inline bool qs::Window::moveWindow(int x, int y, int cx, int cy)
{
	return moveWindow(x, y, cx, cy, true);
}

inline bool qs::Window::moveWindow(int x, int y, int cx, int cy, bool bRepaint)
{
	assert(hwnd_);
	return ::MoveWindow(hwnd_, x, y, cx, cy, bRepaint) != 0;
}


inline bool qs::Window::screenToClient(POINT* pPoint) const
{
	assert(hwnd_);
	assert(pPoint);
	return ::ScreenToClient(hwnd_, pPoint) != 0;
}

inline bool qs::Window::clientToScreen(POINT* pPoint) const
{
	assert(hwnd_);
	assert(pPoint);
	return ::ClientToScreen(hwnd_, pPoint) != 0;
}

inline bool qs::Window::centerWindow()
{
	return centerWindow(0);
}

inline HWND qs::Window::getParent() const
{
	assert(hwnd_);
	return ::GetParent(hwnd_);
}

inline HWND qs::Window::setParent(HWND hwndParent)
{
	assert(hwnd_);
	return ::SetParent(hwnd_, hwndParent);
}

#ifndef _WIN32_WCE
inline bool qs::Window::getWindowPlacement(WINDOWPLACEMENT* pwp) const
{
	assert(hwnd_);
	assert(pwp);
	pwp->length = sizeof(*pwp);
	return ::GetWindowPlacement(hwnd_, pwp) != 0;
}

inline bool qs::Window::setWindowPlacement(const WINDOWPLACEMENT& wp)
{
	assert(hwnd_);
	assert(wp.length == sizeof(wp));
	return ::SetWindowPlacement(hwnd_, &wp) != 0;
}

inline bool qs::Window::redrawWindow(const RECT* pRect, HRGN hrgn, UINT nFlags)
{
	assert(hwnd_);
	return ::RedrawWindow(hwnd_, pRect, hrgn, nFlags) != 0;
}
#endif

inline HWND qs::Window::getFocus()
{
	return ::GetFocus();
}

inline bool qs::Window::hasFocus() const
{
	assert(hwnd_);
	return ::GetFocus() == hwnd_;
}

inline HWND qs::Window::setFocus()
{
	assert(hwnd_);
	return ::SetFocus(hwnd_);
}

inline HWND qs::Window::getActiveWindow()
{
	return ::GetActiveWindow();
}

inline bool qs::Window::isActive() const
{
	assert(hwnd_);
	return ::GetActiveWindow() == hwnd_;
}

inline HWND qs::Window::setActiveWindow()
{
	assert(hwnd_);
	return ::SetActiveWindow(hwnd_);
}

inline HWND qs::Window::getForegroundWindow()
{
	return ::GetForegroundWindow();
}

inline bool qs::Window::setForegroundWindow()
{
	assert(hwnd_);
	return ::SetForegroundWindow(hwnd_) != 0;
}

inline bool qs::Window::invalidate()
{
	return invalidate(true);
}

inline bool qs::Window::invalidate(bool bErase)
{
	assert(hwnd_);
	return ::InvalidateRect(hwnd_, 0, bErase) != 0;
}

inline bool qs::Window::invalidateRect(const RECT& rect)
{
	return invalidateRect(rect, true);
}

inline bool qs::Window::invalidateRect(const RECT& rect, bool bErase)
{
	assert(hwnd_);
	return ::InvalidateRect(hwnd_, &rect, bErase) != 0;
}

inline bool qs::Window::validate()
{
	assert(hwnd_);
	return ::ValidateRect(hwnd_, 0) != 0;
}

inline bool qs::Window::validateRect(const RECT& rect)
{
	assert(hwnd_);
	return ::ValidateRect(hwnd_, &rect) != 0;
}

inline HDC qs::Window::beginPaint(PAINTSTRUCT* pps)
{
	assert(hwnd_);
	return ::BeginPaint(hwnd_, pps);
}

inline bool qs::Window::endPaint(const PAINTSTRUCT& ps)
{
	assert(hwnd_);
#ifdef _WIN32_WCE
	return ::EndPaint(hwnd_, const_cast<PAINTSTRUCT*>(&ps)) != 0;
#else
	return ::EndPaint(hwnd_, &ps) != 0;
#endif
}

inline HDC qs::Window::getDC() const
{
	assert(hwnd_);
	return ::GetDC(hwnd_);
}

inline int qs::Window::releaseDC(HDC hdc) const
{
	assert(hwnd_);
	return ::ReleaseDC(hwnd_, hdc);
}

inline HFONT qs::Window::getFont()
{
	return reinterpret_cast<HFONT>(sendMessage(WM_GETFONT));
}

inline void qs::Window::setFont(HFONT hfont)
{
	sendMessage(WM_SETFONT, reinterpret_cast<WPARAM>(hfont));
}

inline bool qs::Window::createCaret(int nWidth, int nHeight)
{
	return createCaret(nWidth, nHeight, 0);
}

inline bool qs::Window::createCaret(int nWidth, int nHeight, HBITMAP hBitmap)
{
	assert(hwnd_);
	return ::CreateCaret(hwnd_, hBitmap, nWidth, nHeight) != 0;
}

inline bool qs::Window::destroyCaret()
{
	return ::DestroyCaret() != 0;
}

inline bool qs::Window::showCaret()
{
	assert(hwnd_);
	return ::ShowCaret(hwnd_) != 0;
}

inline bool qs::Window::hideCaret()
{
	assert(hwnd_);
	return ::HideCaret(hwnd_) != 0;
}

inline bool qs::Window::setCaretPos(const POINT& pt)
{
	return ::SetCaretPos(pt.x, pt.y) != 0;
}

inline bool qs::Window::getCaretPos(POINT* pPoint)
{
	return ::GetCaretPos(pPoint) != 0;
}

inline bool qs::Window::getScrollInfo(int nBar, SCROLLINFO* psi) const
{
	assert(hwnd_);
	return ::GetScrollInfo(hwnd_, nBar, psi) != 0;
}

inline bool qs::Window::setScrollInfo(int nBar, const SCROLLINFO& si)
{
	return setScrollInfo(nBar, si, true);
}

inline bool qs::Window::setScrollInfo(int nBar, const SCROLLINFO& si, bool bRedraw)
{
	assert(hwnd_);
	return ::SetScrollInfo(hwnd_, nBar, &si, bRedraw) != 0;
}

inline int qs::Window::getScrollPos(int nBar) const
{
	SCROLLINFO si = { sizeof(si), SIF_POS };
	if (!getScrollInfo(nBar, &si))
		return 0;
	return si.nPos;
}

inline bool qs::Window::setScrollPos(int nBar, int nPos)
{
	return setScrollPos(nBar, nPos, true);
}

inline bool qs::Window::setScrollPos(int nBar, int nPos, bool bRedraw)
{
	assert(hwnd_);
	return ::SetScrollPos(hwnd_, nBar, nPos, bRedraw) != 0;
}

inline bool qs::Window::getScrollRange(int nBar, int* pnMinPos, int* pnMaxPos) const
{
	assert(pnMinPos);
	assert(pnMaxPos);
	
	SCROLLINFO si = { sizeof(si), SIF_RANGE };
	if (!getScrollInfo(nBar, &si))
		return false;
	*pnMinPos = si.nMin;
	*pnMaxPos = si.nMax;
	return true;
}

inline bool qs::Window::setScrollRange(int nBar, int nMinPos, int nMaxPos)
{
	return setScrollRange(nBar, nMinPos, nMaxPos, true);
}

inline bool qs::Window::setScrollRange(int nBar, int nMinPos, int nMaxPos, bool bRedraw)
{
	assert(hwnd_);
	return ::SetScrollRange(hwnd_, nBar, nMinPos, nMaxPos, bRedraw) != 0;
}

inline int qs::Window::scrollWindow(int x, int y)
{
	return scrollWindow(x, y, 0, 0, 0, 0, SW_INVALIDATE);
}

inline int qs::Window::scrollWindow(int x, int y, const RECT* prcScroll,
	const RECT* prcClip, HRGN hrgnUpdate, RECT* prcUpdate, UINT nFlags)
{
	assert(hwnd_);
	return ::ScrollWindowEx(hwnd_, x, y, prcScroll, prcClip,
		hrgnUpdate, prcUpdate, nFlags);
}

inline HWND qs::Window::getCapture()
{
	return ::GetCapture();
}

inline bool qs::Window::releaseCapture()
{
	return ::ReleaseCapture() != 0;
}

inline bool qs::Window::isCapture() const
{
	assert(hwnd_);
	return ::GetCapture() == hwnd_;
}

inline HWND qs::Window::setCapture()
{
	assert(hwnd_);
	return ::SetCapture(hwnd_);
}

inline int qs::Window::getWindowTextLength() const
{
	assert(hwnd_);
	return ::GetWindowTextLength(hwnd_);
}

inline long qs::Window::getWindowLong(int n) const
{
	assert(hwnd_);
	return ::GetWindowLong(hwnd_, n);
}

inline long qs::Window::setWindowLong(int n, long l)
{
	assert(hwnd_);
	return ::SetWindowLong(hwnd_, n, l);
}

inline UINT qs::Window::setTimer(UINT nId, UINT nTimeout)
{
	return setTimer(nId, nTimeout, 0);
}

inline UINT qs::Window::setTimer(UINT nId, UINT nTimeout, TIMERPROC proc)
{
	assert(hwnd_);
	return ::SetTimer(hwnd_, nId, nTimeout, proc);
}

inline bool qs::Window::killTimer(UINT nId)
{
	assert(hwnd_);
	return ::KillTimer(hwnd_, nId) != 0;
}

#ifndef _WIN32_WCE
inline HMENU qs::Window::getMenu() const
{
	assert(hwnd_);
	return ::GetMenu(hwnd_);
}

inline bool qs::Window::setMenu(HMENU hmenu)
{
	assert(hwnd_);
	return ::SetMenu(hwnd_, hmenu) != 0;
}

inline void qs::Window::dragAcceptFiles(bool bAccept)
{
	assert(hwnd_);
	::DragAcceptFiles(hwnd_, bAccept);
}
#endif // _WIN32_WCE

inline bool qs::Window::postMessage(UINT uMsg)
{
	return postMessage(uMsg, 0, 0);
}

inline bool qs::Window::postMessage(UINT uMsg, WPARAM wParam)
{
	return postMessage(uMsg, wParam, 0);
}

inline bool qs::Window::postMessage(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	assert(hwnd_);
	return ::PostMessage(hwnd_, uMsg, wParam, lParam) != 0;
}

inline LRESULT qs::Window::sendMessage(UINT uMsg)
{
	return sendMessage(uMsg, 0, 0);
}

inline LRESULT qs::Window::sendMessage(UINT uMsg, WPARAM wParam)
{
	return sendMessage(uMsg, wParam, 0);
}

inline LRESULT qs::Window::sendMessage(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	assert(hwnd_);
	return ::SendMessage(hwnd_, uMsg, wParam, lParam);
}

inline bool qs::Window::peekMessage(MSG* pMsg)
{
	return peekMessage(pMsg, 0, 0, PM_NOREMOVE);
}

inline bool qs::Window::peekMessage(MSG* pMsg,
	 UINT uMsgFilterMin, UINT uMsgFilterMax, UINT nRemoveMsg)
{
	assert(hwnd_);
	return ::PeekMessage(pMsg, hwnd_,
		uMsgFilterMin, uMsgFilterMax, nRemoveMsg) != 0;
}

inline HWND qs::Window::getDlgItem(int nDlgItem) const
{
	assert(hwnd_);
	return ::GetDlgItem(hwnd_, nDlgItem);
}

inline int qs::Window::getDlgItemInt(int nDlgItem) const
{
	return getDlgItemInt(nDlgItem, 0, true);
}

inline bool qs::Window::setDlgItemInt(int nDlgItem, int nValue)
{
	return setDlgItemInt(nDlgItem, nValue, true);
}

inline bool qs::Window::setDlgItemInt(int nDlgItem, int nValue, bool bSigned)
{
	assert(hwnd_);
	return ::SetDlgItemInt(hwnd_, nDlgItem, nValue, bSigned) != 0;
}

inline LRESULT qs::Window::sendDlgItemMessage(int nDlgItem, UINT uMsg)
{
	return sendDlgItemMessage(nDlgItem, uMsg, 0, 0);
}

inline LRESULT qs::Window::sendDlgItemMessage(
	int nDlgItem, UINT uMsg, WPARAM wParam)
{
	return sendDlgItemMessage(nDlgItem, uMsg, wParam, 0);
}

inline LRESULT qs::Window::sendDlgItemMessage(int nDlgItem,
	UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	assert(hwnd_);
	return ::SendDlgItemMessage(hwnd_, nDlgItem, uMsg, wParam, lParam);
}

inline bool qs::Window::isDialogMessage(MSG* pMsg)
{
	return ::IsDialogMessage(hwnd_, pMsg) != 0;
}

inline void qs::Window::setHandle(HWND hwnd)
{
	assert(!hwnd_ || !hwnd);
	hwnd_ = hwnd;
}


/****************************************************************************
 *
 * DefaultWindowHandlerBase
 *
 */

#define IMPLEMENT_DEFAULTPROC0(name, message, wparam, lparam) \
inline LRESULT qs::DefaultWindowHandlerBase::on##name() \
{ \
	return getDefWindowProcHolder()->defWindowProc(message, wparam, lparam); \
} \

#define IMPLEMENT_DEFAULTPROC1(name, argtype1, message, wparam, lparam) \
inline LRESULT qs::DefaultWindowHandlerBase::on##name(argtype1 arg1) \
{ \
	return getDefWindowProcHolder()->defWindowProc(message, wparam, lparam); \
} \

#define IMPLEMENT_DEFAULTPROC2(name, argtype1, argtype2, message, wparam, lparam) \
inline LRESULT qs::DefaultWindowHandlerBase::on##name(argtype1 arg1, argtype2 arg2) \
{ \
	return getDefWindowProcHolder()->defWindowProc(message, wparam, lparam); \
} \

#define IMPLEMENT_DEFAULTPROC3(name, argtype1, argtype2, argtype3, message, wparam, lparam) \
inline LRESULT qs::DefaultWindowHandlerBase::on##name(argtype1 arg1, argtype2 arg2, argtype3 arg3) \
{ \
	return getDefWindowProcHolder()->defWindowProc(message, wparam, lparam); \
} \

IMPLEMENT_DEFAULTPROC3(Activate, UINT, HWND, bool,
	WM_ACTIVATE, MAKEWPARAM(arg1, arg3), reinterpret_cast<LPARAM>(arg2))

#ifndef _WIN32_WCE
IMPLEMENT_DEFAULTPROC2(ActivateApp, bool, DWORD,
	WM_ACTIVATEAPP, arg1, arg2)
#endif

IMPLEMENT_DEFAULTPROC3(Char, UINT, UINT, UINT,
	WM_CHAR, arg1, MAKELPARAM(arg2, arg3))

IMPLEMENT_DEFAULTPROC0(Close,
	WM_CLOSE, 0, 0);

IMPLEMENT_DEFAULTPROC3(Command, UINT, UINT, HWND,
	WM_COMMAND, MAKEWPARAM(arg1, arg2), reinterpret_cast<LPARAM>(arg3))

IMPLEMENT_DEFAULTPROC2(ContextMenu, HWND, const POINT&,
	WM_CONTEXTMENU, reinterpret_cast<WPARAM>(arg1), MAKELPARAM(arg2.x, arg2.y))

IMPLEMENT_DEFAULTPROC0(Copy,
	WM_COPY, 0, 0)

IMPLEMENT_DEFAULTPROC2(CopyData, HWND, COPYDATASTRUCT*,
	WM_COPYDATA, reinterpret_cast<WPARAM>(arg1), reinterpret_cast<LPARAM>(arg2))

IMPLEMENT_DEFAULTPROC1(Create, CREATESTRUCT*,
	WM_CREATE, 0, reinterpret_cast<LPARAM>(arg1))

IMPLEMENT_DEFAULTPROC2(CtlColorStatic, HDC, HWND,
	WM_CTLCOLORSTATIC, reinterpret_cast<WPARAM>(arg1), reinterpret_cast<LPARAM>(arg2))

IMPLEMENT_DEFAULTPROC0(Cut,
	WM_CUT, 0, 0)

IMPLEMENT_DEFAULTPROC0(Destroy,
	WM_DESTROY, 0, 0)

IMPLEMENT_DEFAULTPROC1(EraseBkgnd, HDC,
	WM_ERASEBKGND, reinterpret_cast<WPARAM>(arg1), 0)

#ifndef _WIN32_WCE
IMPLEMENT_DEFAULTPROC1(DropFiles, HDROP,
	WM_DROPFILES, reinterpret_cast<WPARAM>(arg1), 0)
#endif

#ifndef _WIN32_WCE
IMPLEMENT_DEFAULTPROC2(EndSession, bool, int,
	WM_ENDSESSION, arg1, arg2);
#endif

IMPLEMENT_DEFAULTPROC3(HScroll, UINT, UINT, HWND,
	WM_HSCROLL, MAKEWPARAM(arg1, arg2), reinterpret_cast<LPARAM>(arg3))

IMPLEMENT_DEFAULTPROC3(ImeChar, UINT, UINT, UINT,
	WM_IME_CHAR, arg1, MAKELPARAM(arg2, arg3))

IMPLEMENT_DEFAULTPROC2(ImeComposition, UINT, UINT,
	WM_IME_COMPOSITION, arg1, arg2)

IMPLEMENT_DEFAULTPROC0(ImeEndComposition,
	WM_IME_ENDCOMPOSITION, 0, 0)

IMPLEMENT_DEFAULTPROC0(ImeStartComposition,
	WM_IME_STARTCOMPOSITION, 0, 0)

IMPLEMENT_DEFAULTPROC2(InitDialog, HWND, LPARAM,
	WM_INITDIALOG, reinterpret_cast<WPARAM>(arg1), arg2)

#ifndef _WIN32_WCE
IMPLEMENT_DEFAULTPROC1(InitMenu, HMENU,
	WM_INITMENU, reinterpret_cast<WPARAM>(arg1), 0);
#endif

IMPLEMENT_DEFAULTPROC3(InitMenuPopup, HMENU, UINT, bool,
	WM_INITMENUPOPUP, reinterpret_cast<WPARAM>(arg1),
	MAKELPARAM(static_cast<WORD>(arg2), static_cast<WORD>(arg3)));

IMPLEMENT_DEFAULTPROC3(KeyDown, UINT, UINT, UINT,
	WM_KEYDOWN, arg1, MAKELPARAM(arg2, arg3))

IMPLEMENT_DEFAULTPROC3(KeyUp, UINT, UINT, UINT,
	WM_KEYUP, arg1, MAKELPARAM(arg2, arg3))

IMPLEMENT_DEFAULTPROC1(KillFocus, HWND,
	WM_KILLFOCUS, reinterpret_cast<WPARAM>(arg1), 0)

IMPLEMENT_DEFAULTPROC2(LButtonDblClk, UINT, const POINT&,
	WM_LBUTTONDBLCLK, arg1, MAKELPARAM(arg2.x, arg2.y))

IMPLEMENT_DEFAULTPROC2(LButtonDown, UINT, const POINT&,
	WM_LBUTTONDOWN, arg1, MAKELPARAM(arg2.x, arg2.y))

IMPLEMENT_DEFAULTPROC2(LButtonUp, UINT, const POINT&,
	WM_LBUTTONUP, arg1, MAKELPARAM(arg2.x, arg2.y))

#ifndef _WIN32_WCE
IMPLEMENT_DEFAULTPROC3(MouseActivate, HWND, UINT, UINT,
	WM_MOUSEACTIVATE, reinterpret_cast<WPARAM>(arg1), MAKELPARAM(arg2, arg3))
#endif

IMPLEMENT_DEFAULTPROC2(MouseMove, UINT, const POINT&,
	WM_MOUSEMOVE, arg1, MAKELPARAM(arg2.x, arg2.y))

#if !defined _WIN32_WCE || _WIN32_WCE >= 211
IMPLEMENT_DEFAULTPROC3(MouseWheel, UINT, short, const POINT&,
	WM_MOUSEWHEEL, MAKEWPARAM(arg1, arg2), MAKELPARAM(arg3.x, arg3.y))
#endif

IMPLEMENT_DEFAULTPROC0(Paint,
	WM_PAINT, 0, 0);

IMPLEMENT_DEFAULTPROC0(Paste,
	WM_PASTE, 0, 0)

#ifndef _WIN32_WCE
IMPLEMENT_DEFAULTPROC1(QueryEndSession, int,
	WM_QUERYENDSESSION, 0, arg1);
#endif

IMPLEMENT_DEFAULTPROC2(RButtonDblClk, UINT, const POINT&,
	WM_RBUTTONDBLCLK, arg1, MAKELPARAM(arg2.x, arg2.y))

IMPLEMENT_DEFAULTPROC2(RButtonDown, UINT, const POINT&,
	WM_RBUTTONDOWN, arg1, MAKELPARAM(arg2.x, arg2.y))

IMPLEMENT_DEFAULTPROC2(RButtonUp, UINT, const POINT&,
	WM_RBUTTONUP, arg1, MAKELPARAM(arg2.x, arg2.y))

IMPLEMENT_DEFAULTPROC3(SetCursor, HWND, UINT, UINT,
	WM_SETCURSOR, reinterpret_cast<WPARAM>(arg1), MAKELPARAM(arg2, arg3))

IMPLEMENT_DEFAULTPROC1(SetFocus, HWND,
	WM_SETFOCUS, reinterpret_cast<WPARAM>(arg1), 0)

IMPLEMENT_DEFAULTPROC2(SettingChange, WPARAM, LPARAM,
	WM_SETTINGCHANGE, arg1, arg2)

IMPLEMENT_DEFAULTPROC3(Size, UINT, int, int,
	WM_SIZE, arg1, MAKELPARAM(arg2, arg3))

IMPLEMENT_DEFAULTPROC3(SysChar, UINT, UINT, UINT,
	WM_SYSCHAR, arg1, MAKELPARAM(arg2, arg3))

IMPLEMENT_DEFAULTPROC3(SysKeyDown, UINT, UINT, UINT,
	WM_SYSKEYDOWN, arg1, MAKELPARAM(arg2, arg3))

IMPLEMENT_DEFAULTPROC3(SysKeyUp, UINT, UINT, UINT,
	WM_SYSKEYUP, arg1, MAKELPARAM(arg2, arg3))

IMPLEMENT_DEFAULTPROC1(Timer, UINT,
	WM_TIMER, arg1, 0)

IMPLEMENT_DEFAULTPROC3(VScroll, UINT, UINT, HWND,
	WM_VSCROLL, MAKEWPARAM(arg1, arg2), reinterpret_cast<LPARAM>(arg3))


#endif // __QSWINDOW_INL__
