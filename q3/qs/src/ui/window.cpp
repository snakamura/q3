/*
 * $Id$
 *
 * Copyright(C) 1998-2004 Satoshi Nakamura
 * All rights reserved.
 *
 */

#include <qsaccelerator.h>
#include <qsaction.h>
#include <qsconv.h>
#include <qsinit.h>
#include <qsosutil.h>
#include <qsstl.h>
#include <qsstring.h>
#include <qsthread.h>
#include <qswindow.h>

#include <algorithm>
#include <hash_map>
#include <memory>
#include <vector>

#include <tchar.h>
#ifdef _WIN32_WCE_PSPC
#	include <aygshell.h>
#endif

#include "dialog.h"
#include "window.h"

#pragma warning(disable:4786)

using namespace qs;


/****************************************************************************
 *
 * Global functions
 *
 */

QSEXPORTPROC int qs::messageBox(HINSTANCE hInstResource,
								UINT nId)
{
	return messageBox(hInstResource, nId, MB_OK | MB_ICONINFORMATION, 0, 0, 0);
}

QSEXPORTPROC int qs::messageBox(HINSTANCE hInstResource,
								UINT nId,
								UINT nType)
{
	return messageBox(hInstResource, nId, nType, 0, 0, 0);
}

QSEXPORTPROC int qs::messageBox(HINSTANCE hInstResource,
								UINT nId,
								HWND hwnd)
{
	return messageBox(hInstResource, nId, MB_OK | MB_ICONINFORMATION, hwnd, 0, 0);
}

QSEXPORTPROC int qs::messageBox(HINSTANCE hInstResource,
								UINT nId,
								UINT nType,
								HWND hwnd)
{
	return messageBox(hInstResource, nId, nType, hwnd, 0, 0);
}

QSEXPORTPROC int qs::messageBox(HINSTANCE hInstResource,
								UINT nId,
								UINT nType,
								HWND hwnd,
								const WCHAR* pwszTitle,
								ModalHandler* pModalHandler)
{
	wstring_ptr wstrTitle(loadString(hInstResource, nId));
	return messageBox(wstrTitle.get(), nType, hwnd, pwszTitle, pModalHandler);
}

QSEXPORTPROC int qs::messageBox(const WCHAR* pwszMessage)
{
	return messageBox(pwszMessage, MB_OK | MB_ICONINFORMATION, 0, 0, 0);
}

QSEXPORTPROC int qs::messageBox(const WCHAR* pwszMessage, UINT nType)
{
	return messageBox(pwszMessage, nType, 0, 0, 0);
}

QSEXPORTPROC int qs::messageBox(const WCHAR* pwszMessage,
								HWND hwnd)
{
	return messageBox(pwszMessage, MB_OK | MB_ICONINFORMATION, hwnd, 0, 0);
}

QSEXPORTPROC int qs::messageBox(const WCHAR* pwszMessage,
								UINT nType,
								HWND hwnd)
{
	return messageBox(pwszMessage, nType, hwnd, 0, 0);
}

QSEXPORTPROC int qs::messageBox(const WCHAR* pwszMessage,
								UINT nType,
								HWND hwnd,
								const WCHAR* pwszTitle,
								ModalHandler* pModalHandler)
{
	if (!hwnd) {
		Window* pMainWindow = getMainWindow();
		if (pMainWindow)
			hwnd = pMainWindow->getHandle();
	}
	
	Cursor cursor(0);
	
	if (!pwszTitle)
		pwszTitle = getTitle();
	W2T(pwszTitle, ptszTitle);
	W2T(pwszMessage, ptszMessage);
	
	if (!pModalHandler)
		pModalHandler = getModalHandler();
	
	ModalHandlerInvoker invoker(pModalHandler, hwnd);
	return ::MessageBox(hwnd, ptszMessage, ptszTitle, nType);
}


/****************************************************************************
 *
 * Window
 *
 */

DWORD qs::Window::setStyle(DWORD dwStyle,
						   DWORD dwMask)
{
	DWORD dw = getStyle();
	dw &= ~dwMask;
	dw |= dwStyle & dwMask;
	return setWindowLong(GWL_STYLE, dw);
}

bool qs::Window::screenToClient(RECT* pRect) const
{
	assert(hwnd_);
	assert(pRect);
	
	POINT pt;
	pt.x = pRect->left;
	pt.y = pRect->top;
	if (!::ScreenToClient(hwnd_, &pt))
		return false;
	pRect->left = pt.x;
	pRect->top = pt.y;
	pt.x = pRect->right;
	pt.y = pRect->bottom;
	if (!::ScreenToClient(hwnd_, &pt))
		return false;
	pRect->right = pt.x;
	pRect->bottom = pt.y;
	
	return true;
}

bool qs::Window::clientToScreen(RECT* pRect) const
{
	assert(hwnd_);
	assert(pRect);
	
	POINT pt;
	pt.x = pRect->left;
	pt.y = pRect->top;
	if (!::ClientToScreen(hwnd_, &pt))
		return false;
	pRect->left = pt.x;
	pRect->top = pt.y;
	pt.x = pRect->right;
	pt.y = pRect->bottom;
	if (!::ClientToScreen(hwnd_, &pt))
		return false;
	pRect->right = pt.x;
	pRect->bottom = pt.y;
	
	return true;
}

bool qs::Window::centerWindow(HWND hwnd)
{
	assert(hwnd_);
	
	RECT rectParent;
	RECT rectShift = { 0, 0, 0, 0 };
	if (hwnd) {
		::GetWindowRect(hwnd, &rectParent);
	}
	else {
		Window wndParent(getParent());
		if (wndParent && !wndParent.isIconic()) {
			wndParent.getWindowRect(&rectParent);
		}
		else if (getMainWindow() && !getMainWindow()->isIconic()) {
			getMainWindow()->getWindowRect(&rectParent);
			if (rectParent.top < 0)
				rectParent.top = 0;
		}
		else {
			::SystemParametersInfo(SPI_GETWORKAREA, 0, &rectParent, 0);
		}
	}
	
	RECT rect;
	getWindowRect(&rect);
	
	return setWindowPos(0,
		QSMAX(0, (rectParent.left + rectParent.right - (rect.right - rect.left) - rectShift.left)/2),
		QSMAX(0, (rectParent.top + rectParent.bottom - (rect.bottom - rect.top) - rectShift.top)/2),
		0, 0, SWP_NOSIZE | SWP_NOZORDER);
}

HWND qs::Window::getParentFrame() const
{
	assert(hwnd_);
	
	HWND hwnd = hwnd_;
	HWND hwndParent = hwnd;
	while (hwndParent) {
		hwnd = hwndParent;
		hwndParent = ::GetParent(hwnd);
	}
	return hwnd;
}

HWND qs::Window::getParentPopup() const
{
	assert(hwnd_);
	
	HWND hwnd = hwnd_;
	do {
		if (!(::GetWindowLong(hwnd, GWL_STYLE) & WS_CHILD))
			return hwnd;
		hwnd = ::GetParent(hwnd);
	} while (hwnd);
	return 0;
}

HWND qs::Window::getActiveFrame()
{
	HWND hwnd = getActiveWindow();
	HWND hwndParent = hwnd;
	while (hwndParent) {
		hwnd = hwndParent;
		hwndParent = ::GetParent(hwnd);
	}
	return hwnd;
}

wstring_ptr qs::Window::getClassName() const
{
	assert(hwnd_);
	
	TCHAR szClassName[256];
	int nLen = ::GetClassName(hwnd_, szClassName, countof(szClassName));
	if (nLen == 0)
		return 0;
	return tcs2wcs(szClassName);
}

wstring_ptr qs::Window::getWindowText() const
{
	assert(hwnd_);
	
	int nLen = getWindowTextLength() + 1;
	tstring_ptr tstrText(allocTString(nLen));
	::GetWindowText(hwnd_, tstrText.get(), nLen);
	return tcs2wcs(tstrText.get());
}

bool qs::Window::setWindowText(const WCHAR* pwszText)
{
	assert(hwnd_);
	
	W2T(pwszText, ptszText);
	return ::SetWindowText(hwnd_, ptszText) != 0;
}

int qs::Window::getDlgItemInt(int nDlgItem,
							  bool* pbTranslated,
							  bool bSigned) const
{
	assert(hwnd_);
	
	BOOL b = FALSE;
	int n = ::GetDlgItemInt(hwnd_, nDlgItem, &b, bSigned);
	if (pbTranslated)
		*pbTranslated = b != 0;
	return n;
}

wstring_ptr qs::Window::getDlgItemText(int nDlgItem) const
{
	assert(hwnd_);
	
	int nLen = ::SendDlgItemMessage(hwnd_, nDlgItem, WM_GETTEXTLENGTH, 0, 0);
	tstring_ptr tstr(allocTString(nLen + 1));
	::GetDlgItemText(hwnd_, nDlgItem, tstr.get(), nLen + 1);
#ifdef UNICODE
	return tstr;
#else
	return tcs2wcs(tstr.get());
#endif
}

bool qs::Window::setDlgItemText(int nDlgItem,
								const WCHAR* pwszText)
{
	assert(hwnd_);
	
	W2T(pwszText, ptszText);
	return ::SetDlgItemText(hwnd_, nDlgItem, ptszText) != 0;
}

#if defined _WIN32_WCE && _WIN32_WCE >= 300 && defined _WIN32_WCE_PSPC
bool qs::Window::tapAndHold(const POINT& pt)
{
	assert(hwnd_);
	
	SHRGINFO rgi = { sizeof(rgi), hwnd_, { pt.x, pt.y }, SHRG_RETURNCMD };
	if (::SHRecognizeGesture(&rgi) != GN_CONTEXTMENU)
		return false;
	
	POINT ptScreen = pt;
	clientToScreen(&ptScreen);
	sendMessage(WM_CONTEXTMENU, reinterpret_cast<WPARAM>(hwnd_),
		MAKELPARAM(ptScreen.x, ptScreen.y));
	
	return true;
}
#endif


/****************************************************************************
 *
 * DefWindowProcHolder
 *
 */

qs::DefWindowProcHolder::~DefWindowProcHolder()
{
}


/****************************************************************************
 *
 * WindowBaseImpl
 *
 */

class qs::WindowBaseImpl
{
public:
	typedef std::vector<CommandHandler*> CommandHandlerList;
	typedef std::vector<NotifyHandler*> NotifyHandlerList;
	typedef std::vector<OwnerDrawHandler*> OwnerDrawHandlerList;
	typedef ControllerMap<WindowBase> WindowMap;

public:
	LRESULT notifyCommandHandlers(WORD wCode,
								  WORD wId) const;
	LRESULT notifyNotifyHandlers(NMHDR* pnmhdr,
								 bool* pbHandled) const;
	void notifyOwnerDrawHandlers(DRAWITEMSTRUCT* pDrawItem) const;
	void measureOwnerDrawHandlers(MEASUREITEMSTRUCT* pMeasureItem) const;
	LRESULT windowProc(UINT uMsg,
					   WPARAM wParam,
					   LPARAM lParam);
	void destroy();

public:
	static WindowMap* getWindowMap();

private:
	WindowBase* pThis_;
	bool bDeleteThis_;
	WindowHandler* pWindowHandler_;
	bool bDeleteHandler_;
	WNDPROC procSubclass_;
	WindowBase* pOrgWindowBase_;
	CommandHandlerList listCommandHandler_;
	NotifyHandlerList listNotifyHandler_;
	OwnerDrawHandlerList listOwnerDrawHandler_;
	InitThread* pInitThread_;

private:
	static WindowMap* pMap__;
	static class InitializerImpl : public Initializer
	{
	public:
		InitializerImpl();
		virtual ~InitializerImpl();
	
	public:
		virtual bool init();
		virtual void term();
		virtual bool initThread();
		virtual void termThread();
	} init__;

friend class InitializerImpl;
friend class WindowBase;
friend LRESULT CALLBACK windowProc(HWND,
								   UINT,
								   WPARAM,
								   LPARAM);
};

WindowBaseImpl::WindowMap* qs::WindowBaseImpl::pMap__;
WindowBaseImpl::InitializerImpl qs::WindowBaseImpl::init__;

LRESULT qs::WindowBaseImpl::notifyCommandHandlers(WORD wCode,
												  WORD wId) const
{
	for (CommandHandlerList::const_iterator it = listCommandHandler_.begin(); it != listCommandHandler_.end(); ++it) {
		LRESULT lResult = (*it)->onCommand(wCode, wId);
		if (lResult == 0)
			return lResult;
	}
	if (pOrgWindowBase_) {
		LRESULT lResult = pOrgWindowBase_->pImpl_->notifyCommandHandlers(wCode, wId);
		if (lResult == 0)
			return lResult;
	}
	return 1;
}

LRESULT qs::WindowBaseImpl::notifyNotifyHandlers(NMHDR* pnmhdr,
												 bool* pbHandled) const
{
	assert(pbHandled);
	
	for (NotifyHandlerList::const_iterator it = listNotifyHandler_.begin(); it != listNotifyHandler_.end(); ++it) {
		LRESULT lResult = (*it)->onNotify(pnmhdr, pbHandled);
		if (*pbHandled)
			return lResult;
	}
	if (pOrgWindowBase_) {
		LRESULT lResult = pOrgWindowBase_->pImpl_->notifyNotifyHandlers(
			pnmhdr, pbHandled);
		if (lResult == 0)
			return lResult;
	}
	return 1;
}

void qs::WindowBaseImpl::notifyOwnerDrawHandlers(DRAWITEMSTRUCT* pDrawItem) const
{
	for (OwnerDrawHandlerList::const_iterator it = listOwnerDrawHandler_.begin(); it != listOwnerDrawHandler_.end(); ++it)
		(*it)->onDrawItem(pDrawItem);
	if (pOrgWindowBase_)
		pOrgWindowBase_->pImpl_->notifyOwnerDrawHandlers(pDrawItem);
}

void qs::WindowBaseImpl::measureOwnerDrawHandlers(MEASUREITEMSTRUCT* pMeasureItem) const
{
	for (OwnerDrawHandlerList::const_iterator it = listOwnerDrawHandler_.begin(); it != listOwnerDrawHandler_.end(); ++it)
		(*it)->onMeasureItem(pMeasureItem);
	if (pOrgWindowBase_)
		pOrgWindowBase_->pImpl_->measureOwnerDrawHandlers(pMeasureItem);
}

LRESULT qs::WindowBaseImpl::windowProc(UINT uMsg,
									   WPARAM wParam,
									   LPARAM lParam)
{
	LRESULT lResult = 0;
	switch (uMsg) {
	case WM_COMMAND:
		{
			UINT nId = LOWORD(wParam);
			Action* pAction = 0;
			if ((ActionMap::ID_MIN <= nId && nId < ActionMap::ID_MAX) ||
				nId == IDOK || nId == IDCANCEL)
				pAction = pWindowHandler_->getAction(nId);
			if (pAction) {
				unsigned int nModifier = 0;
				
				if (HIWORD(wParam) != 1) {
					struct {
						int nKey_;
						ActionEvent::Modifier modifier_;
					} keys[] = {
						{ VK_SHIFT,		ActionEvent::MODIFIER_SHIFT	},
						{ VK_CONTROL,	ActionEvent::MODIFIER_CTRL	},
						{ VK_MENU,		ActionEvent::MODIFIER_ALT	}
					};
					for (int n = 0; n < countof(keys); ++n) {
						if (::GetKeyState(keys[n].nKey_) < 0)
							nModifier |= keys[n].modifier_;
					}
				}
				
				ActionEvent event(LOWORD(wParam), nModifier);
				if (pAction->isEnabled(event))
					pAction->invoke(event);
				return 0;
			}
			else {
				lResult = notifyCommandHandlers(HIWORD(wParam), LOWORD(wParam));
				if (lResult == 0)
					return lResult;
			}
		}
		break;
	
	case WM_NOTIFY:
		{
			bool bHandled = false;
			lResult = notifyNotifyHandlers(reinterpret_cast<NMHDR*>(lParam), &bHandled);
			if (bHandled)
				return lResult;
		}
		break;
	
#ifndef _WIN32_WCE
	case WM_NOTIFYFORMAT:
#ifdef UNICODE
		return NFR_UNICODE;
#else
		return NFR_ANSI;
#endif
#endif
	
	case WM_DRAWITEM:
		notifyOwnerDrawHandlers(reinterpret_cast<LPDRAWITEMSTRUCT>(lParam));
		break;
	
	case WM_MEASUREITEM:
		measureOwnerDrawHandlers(reinterpret_cast<LPMEASUREITEMSTRUCT>(lParam));
		break;
	
	default:
		break;
	}
	
	lResult = pWindowHandler_->windowProc(uMsg, wParam, lParam);
	
	switch (uMsg) {
#if !defined _WIN32_WCE || defined _WIN32_WCE_EMULATION
	case WM_NCDESTROY:
		destroy();
		break;
#endif
	
#if defined _WIN32_WCE && (_WIN32_WCE < 300 || !defined _WIN32_WCE_PSPC)
	case WM_LBUTTONDOWN:
		if (::GetKeyState(VK_MENU) < 0) {
			POINT pt = {
				static_cast<short>(LOWORD(lParam)),
				static_cast<short>(HIWORD(lParam))
			};
			pThis_->clientToScreen(&pt);
			return pThis_->sendMessage(WM_CONTEXTMENU,
				reinterpret_cast<WPARAM>(pThis_->getHandle()),
				MAKELPARAM(pt.x, pt.y));
		}
		break;
	
	case WM_KEYDOWN:
		if (wParam == VK_CONTROL && ::GetKeyState(VK_MENU) < 0)
			return pThis_->sendMessage(WM_CONTEXTMENU,
				reinterpret_cast<WPARAM>(pThis_->getHandle()),
				MAKELPARAM(-1, -1));
		break;
#endif
	}
	return lResult;
}

void qs::WindowBaseImpl::destroy()
{
	WindowMap* pMap = getWindowMap();
	
	if (procSubclass_)
		pThis_->setWindowLong(GWL_WNDPROC, reinterpret_cast<LONG>(procSubclass_));
	pMap->removeController(pThis_->getHandle());
	assert(listCommandHandler_.size() == 0);
	assert(listNotifyHandler_.size() == 0);
	assert(listOwnerDrawHandler_.size() == 0);
	if (bDeleteThis_)
		delete pThis_;
}

WindowBaseImpl::WindowMap* qs::WindowBaseImpl::getWindowMap()
{
	return pMap__;
}


/****************************************************************************
 *
 * WindowBaseImpl::InitializerImpl
 *
 */

qs::WindowBaseImpl::InitializerImpl::InitializerImpl()
{
}

qs::WindowBaseImpl::InitializerImpl::~InitializerImpl()
{
}

bool qs::WindowBaseImpl::InitializerImpl::init()
{
	WindowBaseImpl::pMap__ = new WindowMap();
	return true;
}

void qs::WindowBaseImpl::InitializerImpl::term()
{
	delete WindowBaseImpl::pMap__;
	WindowBaseImpl::pMap__ = 0;
}

bool qs::WindowBaseImpl::InitializerImpl::initThread()
{
	return WindowBaseImpl::pMap__->initThread();
}

void qs::WindowBaseImpl::InitializerImpl::termThread()
{
	WindowBaseImpl::pMap__->termThread();
}


/****************************************************************************
 *
 * WindowBase
 *
 */

qs::WindowBase::WindowBase(bool bDeleteThis) :
	Window(0)
{
	pImpl_ = new WindowBaseImpl();
	pImpl_->pThis_ = this;
	pImpl_->bDeleteThis_ = bDeleteThis;
	pImpl_->pWindowHandler_ = 0;
	pImpl_->bDeleteHandler_ = false;
	pImpl_->procSubclass_ = 0;
	pImpl_->pOrgWindowBase_ = 0;
	pImpl_->pInitThread_ = &InitThread::getInitThread();
}

qs::WindowBase::~WindowBase()
{
	if (pImpl_->bDeleteHandler_)
		delete pImpl_->pWindowHandler_;
	delete pImpl_;
	pImpl_ = 0;
}

void qs::WindowBase::setWindowHandler(WindowHandler* pWindowHandler,
									  bool bDeleteHandler)
{
	pImpl_->pWindowHandler_ = pWindowHandler;
	pImpl_->bDeleteHandler_ = bDeleteHandler;
	pWindowHandler->setWindowBase(this);
}

bool qs::WindowBase::create(const WCHAR* pwszClassName,
							const WCHAR* pwszTitle,
							DWORD dwStyle,
							const RECT& rect,
							HWND hwndParent,
							DWORD dwExStyle,
							const WCHAR* pwszSuperClass,
							UINT nId,
							void* pParam)
{
	return create(pwszClassName, pwszTitle, dwStyle, rect.left, rect.top,
		rect.right - rect.left, rect.bottom - rect.top, hwndParent,
		dwExStyle, pwszSuperClass, nId, pParam);
}

bool qs::WindowBase::create(const WCHAR* pwszClassName,
							const WCHAR* pwszTitle,
							DWORD dwStyle,
							int x,
							int y,
							int cx,
							int cy,
							HWND hwndParent,
							DWORD dwExStyle,
							const WCHAR* pwszSuperClass,
							UINT nId,
							void* pParam)
{
	assert(pwszClassName);
	
	if (getHandle())
		return false;
	
	W2T(pwszClassName, ptszClassName);
	W2T(pwszTitle, ptszTitle);
	
	wstring_ptr wstrSuperClass(pImpl_->pWindowHandler_->getSuperClass());
	assert(!wstrSuperClass.get() || !pwszSuperClass ||
		wcscmp(wstrSuperClass.get(), pwszSuperClass) == 0);
	if (wstrSuperClass.get())
		pwszSuperClass = wstrSuperClass.get();
	
	CREATESTRUCT cs = {
		pParam,
		getInstanceHandle(),
		reinterpret_cast<HMENU>(nId),
		hwndParent,
		cy,
		cx,
		y,
		x,
		dwStyle,
		ptszTitle,
		ptszClassName,
		dwExStyle
	};
	if (!pImpl_->pWindowHandler_->preCreateWindow(&cs))
		return false;
	
	dwStyle = cs.style;
	dwExStyle = cs.dwExStyle;
	x = cs.x;
	y = cs.y;
	cx = cs.cx;
	cy = cs.cy;
	hwndParent = cs.hwndParent;
	nId = reinterpret_cast<UINT>(cs.hMenu);
	ptszClassName = cs.lpszClass;
	ptszTitle = cs.lpszName;
	
	WNDCLASS wc;
	if (!::GetClassInfo(getInstanceHandle(), ptszClassName, &wc)) {
		pImpl_->pWindowHandler_->getWindowClass(
			pwszSuperClass, &wc, &pImpl_->procSubclass_);
		wc.lpszClassName = ptszClassName;
		if (!::RegisterClass(&wc))
			return false;
	}
	else if (pwszSuperClass) {
		W2T(pwszSuperClass, ptszSuperClass);
		if (!::GetClassInfo(getInstanceHandle(), ptszSuperClass, &wc))
			return false;
		pImpl_->pWindowHandler_->preSubclassWindow();
		pImpl_->procSubclass_ = wc.lpfnWndProc;
	}
	
	WindowBaseImpl::WindowMap* pMap = WindowBaseImpl::getWindowMap();
	pMap->setThis(this);
	HWND hwnd = ::CreateWindowEx(dwExStyle, ptszClassName, ptszTitle,
		dwStyle, x, y, cx, cy, hwndParent, reinterpret_cast<HMENU>(nId),
		getInstanceHandle(), pParam);
	if (!hwnd) {
		setHandle(0);
		return false;
	}
	assert(getHandle() == hwnd);
	
	return true;
}

bool qs::WindowBase::subclassWindow(HWND hwnd)
{
	pImpl_->pWindowHandler_->preSubclassWindow();
	
	pImpl_->procSubclass_ = reinterpret_cast<WNDPROC>(
		::SetWindowLong(hwnd, GWL_WNDPROC,
			reinterpret_cast<LONG>(&qs::windowProc)));
	if (!pImpl_->procSubclass_)
		return false;
	
	WindowBaseImpl::WindowMap* pMap = WindowBaseImpl::getWindowMap();
	WindowBase* pOrg = pMap->getController(hwnd);
	if (pOrg) {
		pMap->removeController(hwnd);
		pImpl_->pOrgWindowBase_ = pOrg;
	}
	pMap->setThis(this);
	Window(hwnd).sendMessage(WM_NULL);
	assert(getHandle() == hwnd);
	
	return true;
}

bool qs::WindowBase::unsubclassWindow()
{
	if (!pImpl_->procSubclass_)
		return false;
	
	if (::SetWindowLong(getHandle(), GWL_WNDPROC,
		reinterpret_cast<LONG>(pImpl_->procSubclass_)) == 0)
		return false;
	
	WindowBaseImpl::WindowMap* pMap = WindowBaseImpl::getWindowMap();
	pMap->removeController(getHandle());
	if (pImpl_->pOrgWindowBase_) {
		pMap->setController(getHandle(), pImpl_->pOrgWindowBase_);
//		pImpl_->pOrgWindowBase_ = 0;
	}
	setHandle(0);
	
	return true;
}

void qs::WindowBase::addCommandHandler(CommandHandler* pch)
{
	assert(pch);
	pImpl_->listCommandHandler_.push_back(pch);
}

void qs::WindowBase::removeCommandHandler(CommandHandler* pch)
{
	assert(pch);
	WindowBaseImpl::CommandHandlerList::iterator it =
		std::remove(pImpl_->listCommandHandler_.begin(),
			pImpl_->listCommandHandler_.end(), pch);
	assert(it != pImpl_->listCommandHandler_.end());
	pImpl_->listCommandHandler_.erase(it, pImpl_->listCommandHandler_.end());
}

void qs::WindowBase::addNotifyHandler(NotifyHandler* pnh)
{
	assert(pnh);
	pImpl_->listNotifyHandler_.push_back(pnh);
}

void qs::WindowBase::removeNotifyHandler(NotifyHandler* pnh)
{
	assert(pnh);
	WindowBaseImpl::NotifyHandlerList::iterator it =
		std::remove(pImpl_->listNotifyHandler_.begin(),
			pImpl_->listNotifyHandler_.end(), pnh);
	assert(it != pImpl_->listNotifyHandler_.end());
	pImpl_->listNotifyHandler_.erase(it, pImpl_->listNotifyHandler_.end());
}

void qs::WindowBase::addOwnerDrawHandler(OwnerDrawHandler* podh)
{
	assert(podh);
	pImpl_->listOwnerDrawHandler_.push_back(podh);
}

void qs::WindowBase::removeOwnerDrawHandler(OwnerDrawHandler* podh)
{
	assert(podh);
	WindowBaseImpl::OwnerDrawHandlerList::iterator it =
		std::remove(pImpl_->listOwnerDrawHandler_.begin(),
			pImpl_->listOwnerDrawHandler_.end(), podh);
	assert(it != pImpl_->listOwnerDrawHandler_.end());
	pImpl_->listOwnerDrawHandler_.erase(it, pImpl_->listOwnerDrawHandler_.end());
}

Accelerator* qs::WindowBase::getAccelerator() const
{
	if (pImpl_->pOrgWindowBase_)
		return pImpl_->pOrgWindowBase_->getAccelerator();
	else
		return pImpl_->pWindowHandler_->getAccelerator();
}

bool qs::WindowBase::preTranslateAccelerator(const MSG& msg)
{
	if (pImpl_->pOrgWindowBase_)
		return pImpl_->pOrgWindowBase_->preTranslateAccelerator(msg);
	else
		return pImpl_->pWindowHandler_->preTranslateAccelerator(msg);
}

bool qs::WindowBase::isFrame() const
{
	if (pImpl_->pOrgWindowBase_)
		return pImpl_->pOrgWindowBase_->isFrame();
	else
		return pImpl_->pWindowHandler_->isFrame();
}

InitThread* qs::WindowBase::getInitThread() const
{
	return pImpl_->pInitThread_;
}

LRESULT qs::WindowBase::defWindowProc(UINT uMsg,
									  WPARAM wParam,
									  LPARAM lParam)
{
	if (pImpl_->pOrgWindowBase_)
		return pImpl_->pOrgWindowBase_->pImpl_->pWindowHandler_->windowProc(uMsg, wParam, lParam);
	else if (pImpl_->procSubclass_)
		return ::CallWindowProc(pImpl_->procSubclass_, getHandle(), uMsg, wParam, lParam);
	else
		return ::DefWindowProc(getHandle(), uMsg, wParam, lParam);
}

bool qs::WindowBase::translateAccelerator(const MSG& msg)
{
	if (msg.message != WM_KEYDOWN &&
		msg.message != WM_KEYUP &&
		msg.message != WM_SYSKEYDOWN &&
		msg.message != WM_SYSKEYUP &&
		msg.message != WM_CHAR &&
		msg.message != WM_SYSCHAR)
		return false;
	
	bool bProcessed = false;
	
	WindowBaseImpl::WindowMap* pMap = WindowBaseImpl::getWindowMap();
	
	HWND hwnd = msg.hwnd;
	WindowBase* pWindowBase = pMap->getController(hwnd);
	if (pWindowBase) {
		if (pWindowBase->preTranslateAccelerator(msg))
			return true;
	}
	while (hwnd) {
		if (pWindowBase) {
			Accelerator* pAccel = pWindowBase->getAccelerator();
			if (pAccel) {
				HWND hwndFrame = hwnd;
				HWND hwndParent = hwndFrame;
				while (hwndParent) {
					pWindowBase = pMap->getController(hwndFrame);
					if (pWindowBase) {
						if (pWindowBase->isFrame())
							break;
					}
					hwndFrame = hwndParent;
					hwndParent = ::GetParent(hwndFrame);
				}
				assert(hwndFrame);
				bProcessed = pAccel->translateAccelerator(hwndFrame, msg);
				if (bProcessed)
					break;
			}
		}
		hwnd = ::GetParent(hwnd);
		if (hwnd)
			pWindowBase = pMap->getController(hwnd);
	}
	
	return bProcessed;
}

LRESULT CALLBACK qs::windowProc(HWND hwnd,
								UINT uMsg,
								WPARAM wParam,
								LPARAM lParam)
{
	WindowBaseImpl::WindowMap* pMap = WindowBaseImpl::getWindowMap();
	WindowBase* pThis = pMap->findController(hwnd);
	
	LRESULT lResult = 0;
	if (pThis)
		lResult = pThis->pImpl_->windowProc(uMsg, wParam, lParam);
	else
		lResult = ::DefWindowProc(hwnd, uMsg, wParam, lParam);
	
#if defined _WIN32_WCE && !defined _WIN32_WCE_EMULATION
	if (uMsg == WM_DESTROY)
		WindowDestroy::getWindowDestroy()->process(hwnd);
#endif
	
	return lResult;
}


/****************************************************************************
 *
 * ControllerMapBase
 *
 */

qs::ControllerMapBase::ControllerMapBase() :
	pThis_(0),
	pMap_(0)
{
	std::auto_ptr<ThreadLocal> pThis(new ThreadLocal());
	std::auto_ptr<ThreadLocal> pMap(new ThreadLocal());
	
	pThis_ = pThis;
	pMap_ = pMap;
}

qs::ControllerMapBase::~ControllerMapBase()
{
}

bool qs::ControllerMapBase::initThread()
{
	pMap_->set(new Map());
	return true;
}

void qs::ControllerMapBase::termThread()
{
	Map* pMap = static_cast<Map*>(pMap_->get());
	assert(pMap->empty());
	delete pMap;
}

void* qs::ControllerMapBase::getThis()
{
	return pThis_->get();
}

void qs::ControllerMapBase::setThis(void* pThis)
{
	pThis_->set(pThis);
}

void* qs::ControllerMapBase::getController(HWND hwnd)
{
	assert(hwnd);
	
	Map* pMap = static_cast<Map*>(pMap_->get());
	Map::iterator it = pMap->find(hwnd);
	if (it != pMap->end())
		return (*it).second;
	else
		return 0;
}

void qs::ControllerMapBase::setController(HWND hwnd,
										  void* pController)
{
	assert(hwnd);
	assert(pController);
	
	Map* pMap = static_cast<Map*>(pMap_->get());
	pMap->insert(Map::value_type(hwnd, pController));
}

void qs::ControllerMapBase::removeController(HWND hwnd)
{
	assert(hwnd);
	Map* pMap = static_cast<Map*>(pMap_->get());
	pMap->erase(hwnd);
}


#if defined _WIN32_WCE && !defined _WIN32_WCE_EMULATION
/****************************************************************************
 *
 * WindowDestroy
 *
 */

ThreadLocal* qs::WindowDestroy::pWindowDestroy__;
WindowDestroy::InitializerImpl qs::WindowDestroy::init__;

qs::WindowDestroy::WindowDestroy()
{
}

qs::WindowDestroy::~WindowDestroy()
{
}

void qs::WindowDestroy::process(HWND hwnd)
{
	WindowList listWindow;
	bool bMapped = false;
	listWindow.push_back(hwnd);
	for (WindowList::size_type n = 0; n < listWindow.size(); ++n) {
		HWND hwnd = ::GetWindow(listWindow[n], GW_CHILD);
		while (hwnd) {
			if (!bMapped)
				bMapped = isMapped(hwnd);
			listWindow.push_back(hwnd);
			hwnd = ::GetWindow(hwnd, GW_HWNDNEXT);
		}
	}
	if (bMapped) {
		listWindow[0] = 0;
		
		for (WindowList::iterator it = listWindow.begin() + 1; it != listWindow.end(); ++it) {
			if (!isMapped(*it))
				*it = 0;
		}
		
		listDestroy_.push_back(std::make_pair(hwnd, listWindow));
	}
	else {
		for (WindowList::const_iterator itW = listWindow.begin(); itW != listWindow.end(); ++itW)
			remove(*itW);
		
		HWND hwndParent = ::GetParent(hwnd);
		while (hwndParent) {
			remove(hwndParent);
			hwndParent = ::GetParent(hwndParent);
		}
		
		WindowList listDestroy;
		
		for (DestroyList::iterator itD = listDestroy_.begin(); itD != listDestroy_.end(); ) {
			WindowList& l = (*itD).second;
			
			WindowList::const_iterator it = std::find_if(
				l.begin(), l.end(),
				std::not1(
					std::bind2nd(
						binary_compose_f_gx_hy(
							std::equal_to<HWND>(),
							std::identity<HWND>(),
							std::identity<HWND>()),
						0)));
			if (it == l.end()) {
				listDestroy.push_back((*itD).first);
				itD = listDestroy_.erase(itD);
			}
			else {
				++itD;
			}
		}
		
		destroy(hwnd);
		
		for (WindowList::reverse_iterator it = listDestroy.rbegin(); it != listDestroy.rend(); ++it)
			destroy(*it);
	}
}

WindowDestroy* qs::WindowDestroy::getWindowDestroy()
{
	return static_cast<WindowDestroy*>(pWindowDestroy__->get());
}

bool qs::WindowDestroy::isMapped(HWND hwnd)
{
	WindowBaseImpl::WindowMap* pWindowMap = WindowBaseImpl::getWindowMap();
	WindowBase* pWindow = pWindowMap->getController(hwnd);
	if (pWindow) {
		return true;
	}
	else {
		DialogBaseImpl::DialogMap* pDialogMap = DialogBaseImpl::getDialogMap();
		DialogBase* pDialog = pDialogMap->getController(hwnd);
		if (pDialog)
			return true;
	}
	
	return false;
}

void qs::WindowDestroy::destroy(HWND hwnd)
{
	WindowBaseImpl::WindowMap* pWindowMap = WindowBaseImpl::getWindowMap();
	WindowBase* pWindow = pWindowMap->getController(hwnd);
	if (pWindow) {
		pWindow->pImpl_->destroy();
	}
	else {
		DialogBaseImpl::DialogMap* pDialogMap = DialogBaseImpl::getDialogMap();
		DialogBase* pDialog = pDialogMap->getController(hwnd);
		if (pDialog)
			pDialog->pImpl_->destroy();
	}
}

void qs::WindowDestroy::remove(HWND hwnd)
{
	for (DestroyList::iterator itD = listDestroy_.begin(); itD != listDestroy_.end(); ++itD) {
		WindowList& l = (*itD).second;
		for (WindowList::iterator itW = l.begin(); itW != l.end(); ++itW) {
			if (*itW == hwnd)
				*itW = 0;
		}
	}
}


/****************************************************************************
 *
 * WindowDestroy::InitializerImpl
 *
 */

qs::WindowDestroy::InitializerImpl::InitializerImpl()
{
}

qs::WindowDestroy::InitializerImpl::~InitializerImpl()
{
}

bool qs::WindowDestroy::InitializerImpl::init()
{
	WindowDestroy::pWindowDestroy__ = new ThreadLocal();
	return true;
}

void qs::WindowDestroy::InitializerImpl::term()
{
	delete WindowDestroy::pWindowDestroy__;
}

bool qs::WindowDestroy::InitializerImpl::initThread()
{
	pWindowDestroy__->set(new WindowDestroy());
	return true;
}

void qs::WindowDestroy::InitializerImpl::termThread()
{
	delete WindowDestroy::getWindowDestroy();
}
#endif


/****************************************************************************
 *
 * CommandUpdate
 *
 */

qs::CommandUpdate::~CommandUpdate()
{
}

void qs::CommandUpdate::setEnable()
{
	setEnable(true);
}

void qs::CommandUpdate::setCheck()
{
	setCheck(true);
}

void qs::CommandUpdate::setText(const WCHAR* pwszText)
{
	setText(pwszText, true);
}

void qs::CommandUpdate::setText(HINSTANCE hInstResource,
								UINT nId)
{
	setText(hInstResource, nId, true);
}

void qs::CommandUpdate::setText(HINSTANCE hInstResource,
								UINT nId,
								bool bWithoutAccel)
{
	wstring_ptr wstrText(loadString(hInstResource, nId));
	setText(wstrText.get(), bWithoutAccel);
}


/****************************************************************************
 *
 * CommandUpdateMenu
 *
 */

qs::CommandUpdateMenu::CommandUpdateMenu(HMENU hmenu,
										 UINT nId) :
	hmenu_(hmenu),
	nId_(nId)
{
}

qs::CommandUpdateMenu::~CommandUpdateMenu()
{
}

HMENU qs::CommandUpdateMenu::getMenu() const
{
	return hmenu_;
}

UINT qs::CommandUpdateMenu::getId() const
{
	return nId_;
}

void qs::CommandUpdateMenu::setEnable(bool bEnable)
{
	::EnableMenuItem(hmenu_, nId_,
		(bEnable ? MF_ENABLED : MF_GRAYED) | MF_BYCOMMAND);
}

void qs::CommandUpdateMenu::setCheck(bool bCheck)
{
	::CheckMenuItem(hmenu_, nId_,
		(bCheck ? MF_CHECKED : MF_UNCHECKED) | MF_BYCOMMAND);
}

void qs::CommandUpdateMenu::setText(const WCHAR* pwszText,
									bool bWithoutAccel)
{
	tstring_ptr tstrText(wcs2tcs(pwszText));
#ifndef _WIN32_WCE_PSPC
	if (bWithoutAccel && !wcschr(pwszText, L'\t')) {
		wstring_ptr wstrOld(getText());
		const WCHAR* p = wcsrchr(wstrOld.get(), L'\t');
		if (p) {
			tstring_ptr tstrOld(wcs2tcs(p));
			size_t nLen = _tcslen(tstrText.get()) + _tcslen(tstrOld.get()) + 1;
			tstrText = reallocTString(tstrText, nLen);
			_tcscat(tstrText.get(), tstrOld.get());
		}
	}
#endif
	MENUITEMINFO mii = {
		sizeof(mii),
		MIIM_TYPE,
		MFT_STRING,
		0,
		nId_,
		0,
		0,
		0,
		0,
		tstrText.get(),
		0
	};
	::SetMenuItemInfo(hmenu_, nId_, FALSE, &mii);
}

wstring_ptr qs::CommandUpdateMenu::getText() const
{
	TCHAR tszText[256];
	MENUITEMINFO mii = {
		sizeof(mii),
		MIIM_TYPE,
		MFT_STRING,
		0,
		nId_,
		0,
		0,
		0,
		0,
		tszText,
		countof(tszText) - 1
	};
	if (!::GetMenuItemInfo(hmenu_, nId_, false, &mii) || !mii.dwTypeData)
		return 0;
	return tcs2wcs(tszText);
}

void qs::CommandUpdateMenu::updateText()
{
	if (getId() != 0) {
		wstring_ptr wstrMenu(getText());
		WCHAR* p = wcsrchr(wstrMenu.get(), L'\t');
		if (p)
			*p = L'\0';
#ifndef _WIN32_WCE_PSPC
		WindowBaseImpl::WindowMap* pMap = WindowBaseImpl::getWindowMap();
		
		HWND hwnd = Window::getFocus();
		while (hwnd) {
			WindowBase* pWindowBase = pMap->getController(hwnd);
			if (pWindowBase) {
				Accelerator* pAccel = pWindowBase->getAccelerator();
				if (pAccel) {
					wstring_ptr wstrKey(pAccel->getKeyFromId(getId()));
					if (wstrKey.get()) {
						wstrMenu = concat(wstrMenu.get(), L"\t", wstrKey.get());
						break;
					}
				}
			}
			hwnd = ::GetParent(hwnd);
		}
#endif
		setText(wstrMenu.get(), false);
	}
}


/****************************************************************************
 *
 * CommandUpdateToolbar
 *
 */

qs::CommandUpdateToolbar::CommandUpdateToolbar(HWND hwnd,
											   UINT nId) :
	hwnd_(hwnd),
	nId_(nId)
{
}

qs::CommandUpdateToolbar::~CommandUpdateToolbar()
{
}

UINT qs::CommandUpdateToolbar::getId() const
{
	return nId_;
}

void qs::CommandUpdateToolbar::setEnable(bool bEnable)
{
	Window wnd(hwnd_);
	int nState = wnd.sendMessage(TB_GETSTATE, nId_);
	if (bEnable)
		nState |= TBSTATE_ENABLED;
	else
		nState &= ~TBSTATE_ENABLED;
	wnd.sendMessage(TB_SETSTATE, nId_, nState);
}

void qs::CommandUpdateToolbar::setCheck(bool bCheck)
{
	Window wnd(hwnd_);
	int nState = wnd.sendMessage(TB_GETSTATE, nId_);
	if (bCheck)
		nState |= TBSTATE_CHECKED;
	else
		nState &= ~TBSTATE_CHECKED;
	wnd.sendMessage(TB_SETSTATE, nId_, nState);
}

void qs::CommandUpdateToolbar::setText(const WCHAR* pwszText,
									   bool bWithoutAccel)
{
}

wstring_ptr qs::CommandUpdateToolbar::getText() const
{
	return 0;
}

void qs::CommandUpdateToolbar::updateText()
{
}


/****************************************************************************
 *
 * CommandHandler
 *
 */

qs::CommandHandler::~CommandHandler()
{
}


/****************************************************************************
 *
 * DefaultCommandHandler
 *
 */

qs::DefaultCommandHandler::DefaultCommandHandler()
{
}

qs::DefaultCommandHandler::~DefaultCommandHandler()
{
}

LRESULT qs::DefaultCommandHandler::onCommand(WORD nCode,
											 WORD nId)
{
	return 1;
}


/****************************************************************************
 *
 * NotifyHandler
 *
 */

qs::NotifyHandler::~NotifyHandler()
{
}


/****************************************************************************
 *
 * OwnerDrawListner
 *
 */

qs::OwnerDrawHandler::~OwnerDrawHandler()
{
}


/****************************************************************************
 *
 * ModalHandler
 *
 */

qs::ModalHandler::~ModalHandler()
{
}


/****************************************************************************
 *
 * ModalHandlerInvoker
 *
 */

qs::ModalHandlerInvoker::ModalHandlerInvoker(ModalHandler* pModalHandler,
											 HWND hwnd) :
	pModalHandler_(pModalHandler),
	hwnd_(hwnd)
{
	if (pModalHandler_)
		pModalHandler_->preModalDialog(hwnd_);
}

qs::ModalHandlerInvoker::~ModalHandlerInvoker()
{
	if (pModalHandler_)
		pModalHandler_->postModalDialog(hwnd_);
}


/****************************************************************************
 *
 * WindowHandler
 *
 */

qs::WindowHandler::~WindowHandler()
{
}


/****************************************************************************
 *
 * DefaultWindowHandlerBase
 *
 */

qs::DefaultWindowHandlerBase::~DefaultWindowHandlerBase()
{
}


/****************************************************************************
 *
 * DefaultWindowHandler
 *
 */

qs::DefaultWindowHandler::DefaultWindowHandler() :
	pWindowBase_(0)
{
}

qs::DefaultWindowHandler::~DefaultWindowHandler()
{
}

WindowBase* qs::DefaultWindowHandler::getWindowBase() const
{
	assert(pWindowBase_);
	return pWindowBase_;
}

void qs::DefaultWindowHandler::setWindowBase(WindowBase* pWindowBase)
{
	assert(!pWindowBase_);
	assert(pWindowBase);
	pWindowBase_ = pWindowBase;
}

wstring_ptr qs::DefaultWindowHandler::getSuperClass()
{
	return 0;
}

void qs::DefaultWindowHandler::getWindowClass(WNDCLASS* pwc)
{
	pwc->style = CS_DBLCLKS;
	pwc->lpfnWndProc = qs::windowProc;
	pwc->cbClsExtra = 0;
	pwc->cbWndExtra = 4;
	pwc->hInstance = getInstanceHandle();
	pwc->hIcon = 0;
#if defined _WIN32_WCE && _WIN32_WCE < 211
	pwc->hCursor = 0;
#else // _WIN32_WCE
	pwc->hCursor = ::LoadCursor(0, IDC_ARROW);
#endif // _WIN32_WCE
	pwc->hbrBackground = reinterpret_cast<HBRUSH>(COLOR_WINDOW + 1);
	pwc->lpszMenuName = 0;
	pwc->lpszClassName = 0;
}

bool qs::DefaultWindowHandler::getWindowClass(const WCHAR* pwszSuperClass,
											  WNDCLASS* pwc,
											  WNDPROC* pproc)
{
	if (pwszSuperClass) {
		assert(pproc);
		W2T(pwszSuperClass, ptszSuperClass);
		if (!::GetClassInfo(getInstanceHandle(), ptszSuperClass, pwc))
			return false;
		preSubclassWindow();
		*pproc = pwc->lpfnWndProc;
		pwc->lpfnWndProc = qs::windowProc;
	}
	else {
		getWindowClass(pwc);
	}
	return true;
}

bool qs::DefaultWindowHandler::preCreateWindow(CREATESTRUCT* pCreateStruct)
{
	return true;
}

bool qs::DefaultWindowHandler::preSubclassWindow()
{
	return true;
}

Action* qs::DefaultWindowHandler::getAction(UINT nId)
{
	return 0;
}

Accelerator* qs::DefaultWindowHandler::getAccelerator()
{
	return 0;
}

bool qs::DefaultWindowHandler::preTranslateAccelerator(const MSG& msg)
{
	return false;
}

bool qs::DefaultWindowHandler::isFrame() const
{
	return false;
}

LRESULT qs::DefaultWindowHandler::windowProc(UINT uMsg,
											 WPARAM wParam,
											 LPARAM lParam)
{
	assert(pWindowBase_);
	return pWindowBase_->defWindowProc(uMsg, wParam, lParam);
}

DefWindowProcHolder* qs::DefaultWindowHandler::getDefWindowProcHolder()
{
	assert(pWindowBase_);
	return pWindowBase_;
}


#if _WIN32_WCE >= 200

/****************************************************************************
 *
 * CommandBand
 *
 */

qs::CommandBand::CommandBand(bool bDeleteThis) :
	WindowBase(bDeleteThis)
{
	setWindowHandler(this, false);
}

qs::CommandBand::~CommandBand()
{
}

LRESULT qs::CommandBand::windowProc(UINT uMsg,
									WPARAM wParam,
									LPARAM lParam)
{
	BEGIN_MESSAGE_HANDLER()
		HANDLE_SIZE()
	END_MESSAGE_HANDLER()
	return DefaultWindowHandler::windowProc(uMsg, wParam, lParam);
}

LRESULT qs::CommandBand::onSize(UINT nFlags,
								int cx,
								int cy)
{
	HWND hwndParent = getWindowBase()->getParent();
	if (hwndParent) {
		RECT rect;
		::GetWindowRect(hwndParent, &rect);
		::SendMessage(hwndParent, WM_SIZE, SIZE_RESTORED,
			MAKELPARAM(rect.right - rect.left, rect.bottom - rect.top));
	}
	
	return DefaultWindowHandler::onSize(nFlags, cx, cy);
}

#endif // _WIN32_WCE >= 200


/****************************************************************************
 *
 * DisableRedraw
 *
 */

qs::DisableRedraw::DisableRedraw(HWND hwnd) :
	hwnd_(hwnd)
{
	::SendMessage(hwnd_, WM_SETREDRAW, FALSE, 0);
}

qs::DisableRedraw::~DisableRedraw()
{
	::SendMessage(hwnd_, WM_SETREDRAW, TRUE, 0);
}


/****************************************************************************
 *
 * Cursor
 *
 */

qs::Cursor::Cursor(HCURSOR hcursor) :
	hcursor_(0),
	bReset_(false)
{
	hcursor_ = ::SetCursor(hcursor);
}

qs::Cursor::~Cursor()
{
	if (!bReset_)
		::SetCursor(hcursor_);
}

void qs::Cursor::reset()
{
	::SetCursor(hcursor_);
	bReset_ = true;
}


/****************************************************************************
 *
 * WaitCursor
 *
 */

qs::WaitCursor::WaitCursor() :
	cursor_(::LoadCursor(0, IDC_WAIT))
{
}

qs::WaitCursor::~WaitCursor()
{
}

void qs::WaitCursor::reset()
{
	cursor_.reset();
}
