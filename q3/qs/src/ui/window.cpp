/*
 * $Id: window.cpp,v 1.3 2003/05/21 15:59:41 snakamura Exp $
 *
 * Copyright(C) 1998-2003 Satoshi Nakamura
 * All rights reserved.
 *
 */

#include <qswindow.h>
#include <qsconv.h>
#include <qsstring.h>
#include <qserror.h>
#include <qsthread.h>
#include <qsinit.h>
#include <qsstl.h>
#include <qsosutil.h>
#include <qsnew.h>
#include <qsaccelerator.h>
#include <qsaction.h>

#include <algorithm>
#include <memory>
#include <vector>
#include <hash_map>

#include <tchar.h>
#ifdef _WIN32_WCE_PSPC
#	include <aygshell.h>
#endif

#include "window.h"

#pragma warning(disable:4786)

using namespace qs;


/****************************************************************************
 *
 * Global functions
 *
 */

QSEXPORTPROC QSTATUS qs::messageBox(HINSTANCE hInstResource, UINT nId)
{
	return messageBox(hInstResource, nId, MB_OK | MB_ICONINFORMATION, 0, 0, 0, 0);
}

QSEXPORTPROC QSTATUS qs::messageBox(HINSTANCE hInstResource, UINT nId, int* pnRet)
{
	return messageBox(hInstResource, nId, MB_OK | MB_ICONINFORMATION, 0, 0, 0, pnRet);
}

QSEXPORTPROC QSTATUS qs::messageBox(HINSTANCE hInstResource, UINT nId, UINT nType)
{
	return messageBox(hInstResource, nId, nType, 0, 0, 0, 0);
}

QSEXPORTPROC QSTATUS qs::messageBox(
	HINSTANCE hInstResource, UINT nId, UINT nType, int* pnRet)
{
	return messageBox(hInstResource, nId, nType, 0, 0, 0, pnRet);
}

QSEXPORTPROC QSTATUS qs::messageBox(HINSTANCE hInstResource, UINT nId, HWND hwnd)
{
	return messageBox(hInstResource, nId, MB_OK | MB_ICONINFORMATION, hwnd, 0, 0, 0);
}

QSEXPORTPROC QSTATUS qs::messageBox(
	HINSTANCE hInstResource, UINT nId, HWND hwnd, int* pnRet)
{
	return messageBox(hInstResource, nId, MB_OK | MB_ICONINFORMATION, hwnd, 0, 0, pnRet);
}

QSEXPORTPROC QSTATUS qs::messageBox(HINSTANCE hInstResource, UINT nId, UINT nType,
	HWND hwnd, const WCHAR* pwszTitle, ModalHandler* pModalHandler, int* pnRet)
{
	DECLARE_QSTATUS();
	string_ptr<WSTRING> wstrTitle;
	status = loadString(hInstResource, nId, &wstrTitle);
	CHECK_QSTATUS();
	return messageBox(wstrTitle.get(), nType, hwnd, pwszTitle, pModalHandler, pnRet);
}

QSEXPORTPROC QSTATUS qs::messageBox(const WCHAR* pwszMessage)
{
	return messageBox(pwszMessage, MB_OK | MB_ICONINFORMATION, 0, 0, 0, 0);
}

QSEXPORTPROC QSTATUS qs::messageBox(const WCHAR* pwszMessage, int* pnRet)
{
	return messageBox(pwszMessage, MB_OK | MB_ICONINFORMATION, 0, 0, 0, pnRet);
}

QSEXPORTPROC QSTATUS qs::messageBox(const WCHAR* pwszMessage, UINT nType, int* pnRet)
{
	return messageBox(pwszMessage, nType, 0, 0, 0, pnRet);
}

QSEXPORTPROC QSTATUS qs::messageBox(const WCHAR* pwszMessage, UINT nType,
	HWND hwnd, const WCHAR* pwszTitle, ModalHandler* pModalHandler, int* pnRet)
{
	DECLARE_QSTATUS();
	
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
	
	ModalHandlerInvoker invoker(pModalHandler, hwnd, &status);
	CHECK_QSTATUS();
	int nRet = ::MessageBox(hwnd, ptszMessage, ptszTitle, nType);
	if (pnRet)
		*pnRet = nRet;
	
	return QSTATUS_SUCCESS;
}


/****************************************************************************
 *
 * Window
 *
 */

DWORD qs::Window::setStyle(DWORD dwStyle, DWORD dwMask)
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

WSTRING qs::Window::getClassName() const
{
	assert(hwnd_);
	TCHAR szClassName[256];
	int nLen = ::GetClassName(hwnd_, szClassName, countof(szClassName));
	if (nLen == 0)
		return 0;
	return tcs2wcs(szClassName);
}

WSTRING qs::Window::getWindowText() const
{
	assert(hwnd_);
	int nLen = getWindowTextLength() + 1;
	string_ptr<TSTRING> tstrText(allocTString(nLen));
	if (!tstrText.get())
		return 0;
	::GetWindowText(hwnd_, tstrText.get(), nLen);
	return tcs2wcs(tstrText.get());
}

bool qs::Window::setWindowText(const WCHAR* pwszText)
{
	assert(hwnd_);
#ifdef UNICODE
	return ::SetWindowText(hwnd_, pwszText) != 0;
#else
	string_ptr<STRING> str(wcs2mbs(pwszText));
	if (!str.get())
		return false;
	return ::SetWindowText(hwnd_, str.get()) != 0;
#endif
}

int qs::Window::getDlgItemInt(int nDlgItem,
	bool* pbTranslated, bool bSigned) const
{
	assert(hwnd_);
	BOOL b = FALSE;
	int n = ::GetDlgItemInt(hwnd_, nDlgItem, &b, bSigned);
	if (pbTranslated)
		*pbTranslated = b != 0;
	return n;
}

WSTRING qs::Window::getDlgItemText(int nDlgItem) const
{
	assert(hwnd_);
	int nLen = ::SendDlgItemMessage(hwnd_, nDlgItem, WM_GETTEXTLENGTH, 0, 0);
	string_ptr<TSTRING> tstr(allocTString(nLen + 1));
	if (!tstr.get())
		return 0;
	::GetDlgItemText(hwnd_, nDlgItem, tstr.get(), nLen + 1);
#ifdef UNICODE
	return tstr.release();
#else
	return tcs2wcs(tstr.get());
#endif
}

bool qs::Window::setDlgItemText(int nDlgItem, const WCHAR* pwszText)
{
	assert(hwnd_);
#ifdef UNICODE
	return ::SetDlgItemText(hwnd_, nDlgItem, pwszText) != 0;
#else
	string_ptr<STRING> str(wcs2tcs(pwszText));
	if (!str.get())
		return false;
	return ::SetDlgItemText(hwnd_, nDlgItem, str.get()) != 0;
#endif
}

#if defined _WIN32_WCE && _WIN32_WCE >= 300 && defined _WIN32_WCE_PSPC
bool qs::Window::tapAndHold(const POINT& pt)
{
	assert(hwnd_);
	SHRGINFO rgi = { sizeof(rgi), hwnd_, { pt.x, pt.y }, SHRG_RETURNCMD };
	if (::SHRecognizeGesture(&rgi) != GN_CONTEXTMENU)
		return false;
	sendMessage(WM_CONTEXTMENU,
		reinterpret_cast<WPARAM>(hwnd_), MAKELPARAM(pt.x, pt.y));
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
	LRESULT notifyCommandHandlers(WORD wCode, WORD wId) const;
	void updateCommandHandlers(CommandUpdate* pcu) const;
	LRESULT notifyNotifyHandlers(NMHDR* pnmhdr, bool* pbHandled) const;
	void notifyOwnerDrawHandlers(DRAWITEMSTRUCT* pDrawItem) const;
	void measureOwnerDrawHandlers(MEASUREITEMSTRUCT* pMeasureItem) const;
	LRESULT windowProc(UINT uMsg, WPARAM wParam, LPARAM lParam);

public:
	static QSTATUS getWindowMap(WindowMap** ppMap);

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

private:
	static WindowMap* pMap__;
	static class InitializerImpl : public Initializer
	{
	public:
		InitializerImpl();
		virtual ~InitializerImpl();
	
	public:
		virtual QSTATUS init();
		virtual QSTATUS term();
		virtual QSTATUS initThread();
		virtual QSTATUS termThread();
	} init__;

friend class InitializerImpl;
friend class WindowBase;
friend LRESULT CALLBACK windowProc(HWND, UINT, WPARAM, LPARAM);
};

WindowBaseImpl::WindowMap* qs::WindowBaseImpl::pMap__;
WindowBaseImpl::InitializerImpl qs::WindowBaseImpl::init__;

LRESULT qs::WindowBaseImpl::notifyCommandHandlers(
	WORD wCode, WORD wId) const
{
	CommandHandlerList::const_iterator it = listCommandHandler_.begin();
	while (it != listCommandHandler_.end()) {
		LRESULT lResult = (*it)->onCommand(wCode, wId);
		if (lResult == 0)
			return lResult;
		++it;
	}
	if (pOrgWindowBase_) {
		LRESULT lResult = pOrgWindowBase_->pImpl_->notifyCommandHandlers(wCode, wId);
		if (lResult == 0)
			return lResult;
	}
	return 1;
}

void qs::WindowBaseImpl::updateCommandHandlers(CommandUpdate* pcu) const
{
	CommandHandlerList::const_iterator it = listCommandHandler_.begin();
	while (it != listCommandHandler_.end()) {
		(*it)->updateCommand(pcu);
		++it;
	}
	if (pOrgWindowBase_)
		pOrgWindowBase_->pImpl_->updateCommandHandlers(pcu);
}

LRESULT qs::WindowBaseImpl::notifyNotifyHandlers(
	NMHDR* pnmhdr, bool* pbHandled) const
{
	assert(pbHandled);
	
	NotifyHandlerList::const_iterator it = listNotifyHandler_.begin();
	while (it != listNotifyHandler_.end()) {
		LRESULT lResult = (*it)->onNotify(pnmhdr, pbHandled);
		if (*pbHandled)
			return lResult;
		++it;
	}
	if (pOrgWindowBase_) {
		LRESULT lResult = pOrgWindowBase_->pImpl_->notifyNotifyHandlers(
			pnmhdr, pbHandled);
		if (lResult == 0)
			return lResult;
	}
	return 1;
}

void qs::WindowBaseImpl::notifyOwnerDrawHandlers(
	DRAWITEMSTRUCT* pDrawItem) const
{
	OwnerDrawHandlerList::const_iterator it = listOwnerDrawHandler_.begin();
	while (it != listOwnerDrawHandler_.end()) {
		(*it)->onDrawItem(pDrawItem);
		++it;
	}
	if (pOrgWindowBase_)
		pOrgWindowBase_->pImpl_->notifyOwnerDrawHandlers(pDrawItem);
}

void qs::WindowBaseImpl::measureOwnerDrawHandlers(
	MEASUREITEMSTRUCT* pMeasureItem) const
{
	OwnerDrawHandlerList::const_iterator it = listOwnerDrawHandler_.begin();
	while (it != listOwnerDrawHandler_.end()) {
		(*it)->onMeasureItem(pMeasureItem);
		++it;
	}
	if (pOrgWindowBase_)
		pOrgWindowBase_->pImpl_->measureOwnerDrawHandlers(pMeasureItem);
}

LRESULT qs::WindowBaseImpl::windowProc(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	DECLARE_QSTATUS();
	
	LRESULT lResult = 0;
	switch (uMsg) {
	case WM_COMMAND:
		{
			Action* pAction = 0;
			status = pWindowHandler_->getAction(LOWORD(wParam), &pAction);
			CHECK_QSTATUS_VALUE(0);
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
				bool bEnabled = false;
				status = pAction->isEnabled(event, &bEnabled);
				CHECK_QSTATUS();
				if (bEnabled) {
					status = pAction->invoke(event);
					CHECK_QSTATUS_VALUE(0);
				}
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
	case WM_INITMENUPOPUP:
		if (pWindowHandler_->isFrame()) {
			HMENU hmenu = reinterpret_cast<HMENU>(wParam);
			MENUITEMINFO mii = {
				sizeof(mii),
				MIIM_ID | MIIM_SUBMENU,
			};
			for (int nItem = 0; ::GetMenuItemInfo(hmenu, nItem, true, &mii); ++nItem) {
				// If this menu is neither popup menu nor system menu,
				// update state using CommandUpdate.
				if (!mii.hSubMenu && mii.wID < 0xf000 && mii.wID != 0) {
					CommandUpdateMenu cum(hmenu, mii.wID, &status);
					if (status == QSTATUS_SUCCESS) {
						Action* pAction = 0;
						status = pWindowHandler_->getAction(mii.wID, &pAction);
						CHECK_QSTATUS_VALUE(0);
						if (pAction) {
							ActionEvent event(mii.wID, 0);
							bool bEnabled = false;
							status = pAction->isEnabled(event, &bEnabled);
							CHECK_QSTATUS_VALUE(0);
							cum.setEnable(bEnabled);
							bool bChecked = false;
							status = pAction->isChecked(event, &bChecked);
							CHECK_QSTATUS_VALUE(0);
							cum.setCheck(bChecked);
							string_ptr<WSTRING> wstrText;
							status = pAction->getText(event, &wstrText);
							CHECK_QSTATUS_VALUE(0);
							if (wstrText.get())
								cum.setText(wstrText.get(), true);
							Accelerator* pAccelerator = 0;
							status = pWindowHandler_->getAccelerator(&pAccelerator);
							if (status == QSTATUS_SUCCESS && pAccelerator)
								cum.updateText();
						}
						else {
							cum.setEnable(false);
							cum.setCheck(false);
							updateCommandHandlers(&cum);
						}
					}
				}
			}
		}
		break;
	
#if defined _WIN32_WCE && !defined _WIN32_WCE_EMULATION
	case WM_DESTROY:
#elif defined _WIN32_WCE_EMULATION
	case 0x82:		// W_NCDESTROY is not defined winuser.h
#else
	case WM_NCDESTROY:
#endif // _WIN32_WCE
		{
			WindowBaseImpl::WindowMap* pMap = 0;
			status = WindowBaseImpl::getWindowMap(&pMap);
			CHECK_QSTATUS_VALUE(0);
			
			if (procSubclass_)
				pThis_->setWindowLong(GWL_WNDPROC, reinterpret_cast<LONG>(procSubclass_));
			pMap->removeController(pThis_->getHandle());
			assert(listCommandHandler_.size() == 0);
			assert(listNotifyHandler_.size() == 0);
			assert(listOwnerDrawHandler_.size() == 0);
			if (bDeleteThis_)
				delete pThis_;
		}
		break;
	
#if defined _WIN32_WCE && (_WIN32_WCE < 300 || !defined _WIN32_WCE_PSPC)
	case WM_LBUTTONDOWN:
		if (::GetKeyState(VK_MENU) < 0)
			return pThis_->sendMessage(WM_CONTEXTMENU,
				reinterpret_cast<WPARAM>(pThis_->getHandle()), lParam);
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

QSTATUS qs::WindowBaseImpl::getWindowMap(WindowMap** ppMap)
{
	assert(ppMap);
	*ppMap = pMap__;
	return QSTATUS_SUCCESS;
}

qs::WindowBaseImpl::InitializerImpl::InitializerImpl()
{
}

qs::WindowBaseImpl::InitializerImpl::~InitializerImpl()
{
}

QSTATUS qs::WindowBaseImpl::InitializerImpl::init()
{
	DECLARE_QSTATUS();
	
	status = newQsObject(&WindowBaseImpl::pMap__);
	CHECK_QSTATUS();
	
	return QSTATUS_SUCCESS;
}

QSTATUS qs::WindowBaseImpl::InitializerImpl::term()
{
	delete WindowBaseImpl::pMap__;
	WindowBaseImpl::pMap__ = 0;
	
	return QSTATUS_SUCCESS;
}

QSTATUS qs::WindowBaseImpl::InitializerImpl::initThread()
{
	return WindowBaseImpl::pMap__->initThread();
}

QSTATUS qs::WindowBaseImpl::InitializerImpl::termThread()
{
	return WindowBaseImpl::pMap__->termThread();
}


/****************************************************************************
 *
 * WindowBase
 *
 */

qs::WindowBase::WindowBase(bool bDeleteThis, QSTATUS* pstatus) :
	Window(0)
{
	assert(pstatus);
	
	*pstatus = QSTATUS_SUCCESS;
	
	DECLARE_QSTATUS();
	
	status = newObject(&pImpl_);
	CHECK_QSTATUS_SET(pstatus);
	assert(pImpl_);
	pImpl_->pThis_ = this;
	pImpl_->bDeleteThis_ = bDeleteThis;
	pImpl_->pWindowHandler_ = 0;
	pImpl_->bDeleteHandler_ = false;
	pImpl_->procSubclass_ = 0;
	pImpl_->pOrgWindowBase_ = 0;
}

qs::WindowBase::~WindowBase()
{
	if (pImpl_->bDeleteHandler_)
		delete pImpl_->pWindowHandler_;
	delete pImpl_;
	pImpl_ = 0;
}

QSTATUS qs::WindowBase::setWindowHandler(
	WindowHandler* pWindowHandler, bool bDeleteHandler)
{
	DECLARE_QSTATUS();
	
	pImpl_->pWindowHandler_ = pWindowHandler;
	pImpl_->bDeleteHandler_ = bDeleteHandler;
	
	status = pWindowHandler->setWindowBase(this);
	CHECK_QSTATUS();
	
	return QSTATUS_SUCCESS;
}

QSTATUS qs::WindowBase::create(const WCHAR* pwszClassName,
	const WCHAR* pwszTitle, DWORD dwStyle, const RECT& rect, HWND hwndParent,
	DWORD dwExStyle, const WCHAR* pwszSuperClass, UINT nId, void* pParam)
{
	return create(pwszClassName, pwszTitle, dwStyle, rect.left, rect.top,
		rect.right - rect.left, rect.bottom - rect.top, hwndParent,
		dwExStyle, pwszSuperClass, nId, pParam);
}

QSTATUS qs::WindowBase::create(const WCHAR* pwszClassName,
	const WCHAR* pwszTitle, DWORD dwStyle, int x, int y, int cx, int cy,
	HWND hwndParent, DWORD dwExStyle, const WCHAR* pwszSuperClass,
	UINT nId, void* pParam)
{
	assert(pwszClassName);
	
	DECLARE_QSTATUS();
	
	if (getHandle())
		return QSTATUS_FAIL;
	
	W2T(pwszClassName, ptszClassName);
	W2T(pwszTitle, ptszTitle);
	
	string_ptr<WSTRING> wstrSuperClass;
	status = pImpl_->pWindowHandler_->getSuperClass(&wstrSuperClass);
	CHECK_QSTATUS();
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
	status = pImpl_->pWindowHandler_->preCreateWindow(&cs);
	CHECK_QSTATUS();
	
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
		QSTATUS status = pImpl_->pWindowHandler_->getWindowClass(
			pwszSuperClass, &wc, &pImpl_->procSubclass_);
		CHECK_QSTATUS();
		wc.lpszClassName = ptszClassName;
		if (!::RegisterClass(&wc))
			return QSTATUS_FAIL;
	}
	else if (pwszSuperClass) {
		W2T(pwszSuperClass, ptszSuperClass);
		if (!::GetClassInfo(getInstanceHandle(), ptszSuperClass, &wc))
			return QSTATUS_FAIL;
		pImpl_->pWindowHandler_->preSubclassWindow();
		pImpl_->procSubclass_ = wc.lpfnWndProc;
	}
	
	WindowBaseImpl::WindowMap* pMap = 0;
	status = WindowBaseImpl::getWindowMap(&pMap);
	CHECK_QSTATUS();
	status = pMap->setThis(this);
	CHECK_QSTATUS();
	HWND hwnd = ::CreateWindowEx(dwExStyle, ptszClassName, ptszTitle,
		dwStyle, x, y, cx, cy, hwndParent, reinterpret_cast<HMENU>(nId),
		getInstanceHandle(), pParam);
	if (!hwnd) {
		setHandle(0);
		return QSTATUS_FAIL;
	}
	assert(getHandle() == hwnd);
	
	return QSTATUS_SUCCESS;
}

QSTATUS qs::WindowBase::subclassWindow(HWND hwnd)
{
	DECLARE_QSTATUS();
	
	pImpl_->procSubclass_ = reinterpret_cast<WNDPROC>(
		::SetWindowLong(hwnd, GWL_WNDPROC,
			reinterpret_cast<LONG>(&qs::windowProc)));
	if (!pImpl_->procSubclass_)
		return QSTATUS_FAIL;
	
	WindowBaseImpl::WindowMap* pMap = 0;
	status = WindowBaseImpl::getWindowMap(&pMap);
	CHECK_QSTATUS();
	WindowBase* pOrg = 0;
	status = pMap->getController(hwnd, &pOrg);
	CHECK_QSTATUS();
	if (pOrg) {
		status = pMap->removeController(hwnd);
		CHECK_QSTATUS();
		pImpl_->pOrgWindowBase_ = pOrg;
	}
	pMap->setThis(this);
	Window(hwnd).sendMessage(WM_NULL);
	assert(getHandle() == hwnd);
	
	return QSTATUS_SUCCESS;
}

QSTATUS qs::WindowBase::unsubclassWindow()
{
	DECLARE_QSTATUS();
	
	if (!pImpl_->procSubclass_)
		return QSTATUS_FAIL;
	
	if (::SetWindowLong(getHandle(), GWL_WNDPROC,
		reinterpret_cast<LONG>(pImpl_->procSubclass_)) == 0)
		return QSTATUS_FAIL;
	
	WindowBaseImpl::WindowMap* pMap = 0;
	status = WindowBaseImpl::getWindowMap(&pMap);
	CHECK_QSTATUS();
	status = pMap->removeController(getHandle());
	CHECK_QSTATUS();
	if (pImpl_->pOrgWindowBase_) {
		status = pMap->setController(getHandle(), pImpl_->pOrgWindowBase_);
		CHECK_QSTATUS();
//		pImpl_->pOrgWindowBase_ = 0;
	}
	setHandle(0);
	
	return QSTATUS_SUCCESS;
}

QSTATUS qs::WindowBase::addCommandHandler(CommandHandler* pch)
{
	assert(pImpl_);
	return STLWrapper<WindowBaseImpl::CommandHandlerList>
		(pImpl_->listCommandHandler_).push_back(pch);
}

QSTATUS qs::WindowBase::removeCommandHandler(CommandHandler* pch)
{
	assert(pImpl_);
	WindowBaseImpl::CommandHandlerList::iterator it =
		std::remove(pImpl_->listCommandHandler_.begin(),
			pImpl_->listCommandHandler_.end(), pch);
	assert(it != pImpl_->listCommandHandler_.end());
	pImpl_->listCommandHandler_.erase(it, pImpl_->listCommandHandler_.end());
	return QSTATUS_SUCCESS;
}

QSTATUS qs::WindowBase::addNotifyHandler(NotifyHandler* pnh)
{
	assert(pImpl_);
	return STLWrapper<WindowBaseImpl::NotifyHandlerList>
		(pImpl_->listNotifyHandler_).push_back(pnh);
}

QSTATUS qs::WindowBase::removeNotifyHandler(NotifyHandler* pnh)
{
	assert(pImpl_);
	WindowBaseImpl::NotifyHandlerList::iterator it =
		std::remove(pImpl_->listNotifyHandler_.begin(),
			pImpl_->listNotifyHandler_.end(), pnh);
	assert(it != pImpl_->listNotifyHandler_.end());
	pImpl_->listNotifyHandler_.erase(it, pImpl_->listNotifyHandler_.end());
	return QSTATUS_SUCCESS;
}

QSTATUS qs::WindowBase::addOwnerDrawHandler(OwnerDrawHandler* podh)
{
	assert(pImpl_);
	return STLWrapper<WindowBaseImpl::OwnerDrawHandlerList>
		(pImpl_->listOwnerDrawHandler_).push_back(podh);
}

QSTATUS qs::WindowBase::removeOwnerDrawHandler(OwnerDrawHandler* podh)
{
	assert(pImpl_);
	WindowBaseImpl::OwnerDrawHandlerList::iterator it =
		std::remove(pImpl_->listOwnerDrawHandler_.begin(),
			pImpl_->listOwnerDrawHandler_.end(), podh);
	assert(it != pImpl_->listOwnerDrawHandler_.end());
	pImpl_->listOwnerDrawHandler_.erase(it, pImpl_->listOwnerDrawHandler_.end());
	return QSTATUS_SUCCESS;
}

QSTATUS qs::WindowBase::getAccelerator(Accelerator** ppAccelerator)
{
	if (pImpl_->pOrgWindowBase_)
		return pImpl_->pOrgWindowBase_->getAccelerator(ppAccelerator);
	else
		return pImpl_->pWindowHandler_->getAccelerator(ppAccelerator);
}

QSTATUS qs::WindowBase::preTranslateAccelerator(const MSG& msg, bool* pbProcessed)
{
	if (pImpl_->pOrgWindowBase_)
		return pImpl_->pOrgWindowBase_->preTranslateAccelerator(msg, pbProcessed);
	else
		return pImpl_->pWindowHandler_->preTranslateAccelerator(msg, pbProcessed);
}

bool qs::WindowBase::isFrame() const
{
	if (pImpl_->pOrgWindowBase_)
		return pImpl_->pOrgWindowBase_->isFrame();
	else
		return pImpl_->pWindowHandler_->isFrame();
}

LRESULT qs::WindowBase::defWindowProc(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	if (pImpl_->pOrgWindowBase_)
		return pImpl_->pOrgWindowBase_->pImpl_->pWindowHandler_->windowProc(uMsg, wParam, lParam);
	else if (pImpl_->procSubclass_)
		return ::CallWindowProc(pImpl_->procSubclass_, getHandle(), uMsg, wParam, lParam);
	else
		return ::DefWindowProc(getHandle(), uMsg, wParam, lParam);
}

QSTATUS qs::WindowBase::translateAccelerator(const MSG& msg, bool* pbProcessed)
{
	assert(pbProcessed);
	
	*pbProcessed = false;
	if (msg.message != WM_KEYDOWN &&
		msg.message != WM_KEYUP &&
		msg.message != WM_SYSKEYDOWN &&
		msg.message != WM_SYSKEYUP &&
		msg.message != WM_CHAR &&
		msg.message != WM_SYSCHAR)
		return QSTATUS_SUCCESS;
	
	DECLARE_QSTATUS();
	
	WindowBaseImpl::WindowMap* pMap = 0;
	status = WindowBaseImpl::getWindowMap(&pMap);
	CHECK_QSTATUS();
	
	HWND hwnd = msg.hwnd;
	WindowBase* pWindowBase = 0;
	status = pMap->getController(hwnd, &pWindowBase);
	CHECK_QSTATUS();
	if (pWindowBase) {
		status = pWindowBase->preTranslateAccelerator(msg, pbProcessed);
		CHECK_QSTATUS();
		if (*pbProcessed)
			return QSTATUS_SUCCESS;
	}
	while (hwnd) {
		if (pWindowBase) {
			Accelerator* pAccel = 0;
			status = pWindowBase->getAccelerator(&pAccel);
			CHECK_QSTATUS();
			if (pAccel) {
				HWND hwndFrame = hwnd;
				HWND hwndParent = hwndFrame;
				while (hwndParent) {
					status = pMap->getController(hwndFrame, &pWindowBase);
					CHECK_QSTATUS();
					if (pWindowBase) {
						if (pWindowBase->isFrame())
							break;
					}
					hwndFrame = hwndParent;
					hwndParent = ::GetParent(hwndFrame);
				}
				assert(hwndFrame);
				status = pAccel->translateAccelerator(hwndFrame, msg, pbProcessed);
				CHECK_QSTATUS();
				if (*pbProcessed)
					break;
			}
		}
		hwnd = ::GetParent(hwnd);
		if (hwnd) {
			status = pMap->getController(hwnd, &pWindowBase);
			CHECK_QSTATUS();
		}
	}
	return QSTATUS_SUCCESS;
}

LRESULT CALLBACK qs::windowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	DECLARE_QSTATUS();
	
	WindowBaseImpl::WindowMap* pMap = 0;
	status = WindowBaseImpl::getWindowMap(&pMap);
	CHECK_QSTATUS_VALUE(0);
	
	WindowBase* pThis = 0;
	status = pMap->findController(hwnd, &pThis);
	CHECK_QSTATUS_VALUE(0);
	
	return pThis->pImpl_->windowProc(uMsg, wParam, lParam);
}


/****************************************************************************
 *
 * ControllerMapBase
 *
 */

qs::ControllerMapBase::ControllerMapBase(QSTATUS* pstatus) :
	pThis_(0), pMap_(0)
{
	assert(pstatus);
	
	DECLARE_QSTATUS();
	
	std::auto_ptr<ThreadLocal> pThis;
	status = newQsObject(&pThis);
	CHECK_QSTATUS_SET(pstatus);
	
	std::auto_ptr<ThreadLocal> pMap;
	status = newQsObject(&pMap);
	CHECK_QSTATUS_SET(pstatus);
	
	pThis_ = pThis.release();
	pMap_ = pMap.release();
}

qs::ControllerMapBase::~ControllerMapBase()
{
	delete pMap_;
	pMap_ = 0;
	delete pThis_;
	pThis_ = 0;
}

QSTATUS qs::ControllerMapBase::initThread()
{
	DECLARE_QSTATUS();
	
	std::auto_ptr<Map> pMap;
	status = newObject(&pMap);
	CHECK_QSTATUS();
	status = pMap_->set(pMap.get());
	CHECK_QSTATUS();
	pMap.release();
	
	return QSTATUS_SUCCESS;
}

QSTATUS qs::ControllerMapBase::termThread()
{
	void* pMap = 0;
	pMap_->get(&pMap);
	assert(static_cast<Map*>(pMap)->empty());
	delete static_cast<Map*>(pMap);
	
	return QSTATUS_SUCCESS;
}

QSTATUS qs::ControllerMapBase::getThis(void** ppThis)
{
	assert(ppThis);
	return pThis_->get(ppThis);
}

QSTATUS qs::ControllerMapBase::setThis(void* pThis)
{
	return pThis_->set(pThis);
}

QSTATUS qs::ControllerMapBase::getController(HWND hwnd, void** ppController)
{
	assert(hwnd);
	assert(ppController);
	
	DECLARE_QSTATUS();
	
	*ppController = 0;
	
	void* pValue = 0;
	status = pMap_->get(&pValue);
	CHECK_QSTATUS();
	
	Map* pMap = static_cast<Map*>(pValue);
	assert(pMap);
	Map::iterator it = pMap->find(hwnd);
	if (it != pMap->end())
		*ppController = (*it).second;
	
	return QSTATUS_SUCCESS;
}

QSTATUS qs::ControllerMapBase::setController(HWND hwnd, void* pController)
{
	assert(hwnd);
	assert(pController);
	
	DECLARE_QSTATUS();
	
	void* pValue = 0;
	status = pMap_->get(&pValue);
	CHECK_QSTATUS();
	
	Map* pMap = static_cast<Map*>(pValue);
	assert(pMap);
	std::pair<Map::iterator, bool> ret;
	status = STLWrapper<Map>(*pMap).insert(Map::value_type(hwnd, pController), &ret);
	CHECK_QSTATUS();
	
	return ret.second ? QSTATUS_SUCCESS : QSTATUS_FAIL;
}

QSTATUS qs::ControllerMapBase::removeController(HWND hwnd)
{
	assert(hwnd);
	
	DECLARE_QSTATUS();
	
	void* pValue = 0;
	status = pMap_->get(&pValue);
	CHECK_QSTATUS();
	
	Map* pMap = static_cast<Map*>(pValue);
	assert(pMap);
	pMap->erase(hwnd);
	
	return QSTATUS_SUCCESS;
}


/****************************************************************************
 *
 * CommandUpdate
 *
 */

qs::CommandUpdate::~CommandUpdate()
{
}

QSTATUS qs::CommandUpdate::setEnable()
{
	return setEnable(true);
}

QSTATUS qs::CommandUpdate::setCheck()
{
	return setCheck(true);
}

QSTATUS qs::CommandUpdate::setText(const WCHAR* pwszText)
{
	return setText(pwszText, true);
}

QSTATUS qs::CommandUpdate::setText(HINSTANCE hInstResource, UINT nId)
{
	return setText(hInstResource, nId, true);
}

QSTATUS qs::CommandUpdate::setText(HINSTANCE hInstResource,
	UINT nId, bool bWithoutAccel)
{
	DECLARE_QSTATUS();
	
	string_ptr<WSTRING> wstrText;
	status = loadString(hInstResource, nId, &wstrText);
	CHECK_QSTATUS();
	return setText(wstrText.get(), bWithoutAccel);
}


/****************************************************************************
 *
 * CommandUpdateMenu
 *
 */

qs::CommandUpdateMenu::CommandUpdateMenu(HMENU hmenu, UINT nId, QSTATUS* pstatus) :
	hmenu_(hmenu),
	nId_(nId)
{
	assert(pstatus);
	*pstatus = QSTATUS_SUCCESS;
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

QSTATUS qs::CommandUpdateMenu::setEnable(bool bEnable)
{
	::EnableMenuItem(hmenu_, nId_,
		(bEnable ? MF_ENABLED : MF_GRAYED) | MF_BYCOMMAND);
	return QSTATUS_SUCCESS;
}

QSTATUS qs::CommandUpdateMenu::setCheck(bool bCheck)
{
	::CheckMenuItem(hmenu_, nId_,
		(bCheck ? MF_CHECKED : MF_UNCHECKED) | MF_BYCOMMAND);
	return QSTATUS_SUCCESS;
}

QSTATUS qs::CommandUpdateMenu::setText(const WCHAR* pwszText, bool bWithoutAccel)
{
	DECLARE_QSTATUS();
	
	string_ptr<TSTRING> tstrText(wcs2tcs(pwszText));
	if (!tstrText.get())
		return QSTATUS_OUTOFMEMORY;
#ifndef _WIN32_WCE_PSPC
	if (bWithoutAccel && !wcschr(pwszText, L'\t')) {
		string_ptr<WSTRING> wstrOld;
		status = getText(&wstrOld);
		CHECK_QSTATUS();
		const WCHAR* p = wcsrchr(wstrOld.get(), L'\t');
		if (p) {
			string_ptr<TSTRING> tstrOld(wcs2tcs(p));
			if (!tstrOld.get())
				return QSTATUS_OUTOFMEMORY;
			size_t nLen = _tcslen(tstrText.get()) + _tcslen(tstrOld.get()) + 1;
			tstrText.reset(reallocTString(tstrText.release(), nLen));
			if (!tstrText.get())
				return QSTATUS_OUTOFMEMORY;
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
	
	return QSTATUS_SUCCESS;
}

QSTATUS qs::CommandUpdateMenu::getText(WSTRING* pwstrText) const
{
	assert(pwstrText);
	
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
		return QSTATUS_FAIL;
	string_ptr<WSTRING> wstr(tcs2wcs(tszText));
	if (!wstr.get())
		return QSTATUS_OUTOFMEMORY;
	*pwstrText = wstr.release();
	return QSTATUS_SUCCESS;
}

QSTATUS qs::CommandUpdateMenu::updateText()
{
	DECLARE_QSTATUS();
	
	if (getId() != 0) {
		string_ptr<WSTRING> wstrMenu;
		QSTATUS status = getText(&wstrMenu);
		CHECK_QSTATUS();
		WCHAR* p = wcsrchr(wstrMenu.get(), L'\t');
		if (p)
			*p = L'\0';
#ifndef _WIN32_WCE_PSPC
		WindowBaseImpl::WindowMap* pMap = 0;
		status = WindowBaseImpl::getWindowMap(&pMap);
		CHECK_QSTATUS();
		
		HWND hwnd = Window::getFocus();
		while (hwnd) {
			WindowBase* pWindowBase = 0;
			status = pMap->getController(hwnd, &pWindowBase);
			CHECK_QSTATUS();
			
			if (pWindowBase) {
				Accelerator* pAccel = 0;
				status = pWindowBase->getAccelerator(&pAccel);
				CHECK_QSTATUS();
				
				if (pAccel) {
					string_ptr<WSTRING> wstrKey;
					status = pAccel->getKeyFromId(getId(), &wstrKey);
					CHECK_QSTATUS();
					if (wstrKey.get()) {
						WSTRING wstr = concat(wstrMenu.get(), L"\t", wstrKey.get());
						if (!wstr)
							return QSTATUS_OUTOFMEMORY;
						wstrMenu.reset(wstr);
						break;
					}
				}
			}
			hwnd = ::GetParent(hwnd);
		}
#endif
		status = setText(wstrMenu.get(), false);
		CHECK_QSTATUS();
	}
	return QSTATUS_SUCCESS;
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

qs::DefaultCommandHandler::DefaultCommandHandler(QSTATUS* pstatus)
{
}

qs::DefaultCommandHandler::~DefaultCommandHandler()
{
}

LRESULT qs::DefaultCommandHandler::onCommand(WORD nCode, WORD nId)
{
	return 1;
}

void qs::DefaultCommandHandler::updateCommand(CommandUpdate* pcu)
{
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

qs::ModalHandlerInvoker::ModalHandlerInvoker(
	ModalHandler* pModalHandler, HWND hwnd, QSTATUS* pstatus) :
	pModalHandler_(pModalHandler), hwnd_(hwnd)
{
	assert(pstatus);
	*pstatus = QSTATUS_SUCCESS;
	if (pModalHandler_)
		*pstatus = pModalHandler_->preModalDialog(hwnd_);
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

qs::DefaultWindowHandler::DefaultWindowHandler(QSTATUS* pstatus) :
	pWindowBase_(0)
{
	assert(pstatus);
	*pstatus = QSTATUS_SUCCESS;
}

qs::DefaultWindowHandler::~DefaultWindowHandler()
{
}

WindowBase* qs::DefaultWindowHandler::getWindowBase() const
{
	assert(pWindowBase_);
	return pWindowBase_;
}

QSTATUS qs::DefaultWindowHandler::setWindowBase(WindowBase* pWindowBase)
{
	assert(!pWindowBase_);
	assert(pWindowBase);
	pWindowBase_ = pWindowBase;
	return QSTATUS_SUCCESS;
}

QSTATUS qs::DefaultWindowHandler::getSuperClass(WSTRING* pwstrSuperClass)
{
	assert(pwstrSuperClass);
	*pwstrSuperClass = 0;
	return QSTATUS_SUCCESS;
}

QSTATUS qs::DefaultWindowHandler::getWindowClass(WNDCLASS* pwc)
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
	
	return QSTATUS_SUCCESS;
}

QSTATUS qs::DefaultWindowHandler::getWindowClass(
	const WCHAR* pwszSuperClass, WNDCLASS* pwc, WNDPROC* pproc)
{
	QSTATUS status = QSTATUS_SUCCESS;
	if (pwszSuperClass) {
		assert(pproc);
		W2T(pwszSuperClass, ptszSuperClass);
		if (!::GetClassInfo(getInstanceHandle(), ptszSuperClass, pwc))
			return QSTATUS_FAIL;
		preSubclassWindow();
		*pproc = pwc->lpfnWndProc;
		pwc->lpfnWndProc = qs::windowProc;
	}
	else {
		status = getWindowClass(pwc);
	}
	return status;
}

QSTATUS qs::DefaultWindowHandler::preCreateWindow(CREATESTRUCT* pCreateStruct)
{
	return QSTATUS_SUCCESS;
}

QSTATUS qs::DefaultWindowHandler::preSubclassWindow()
{
	return QSTATUS_SUCCESS;
}

QSTATUS qs::DefaultWindowHandler::getAction(UINT nId, Action** ppAction)
{
	assert(ppAction);
	*ppAction = 0;
	return QSTATUS_SUCCESS;
}

QSTATUS qs::DefaultWindowHandler::getAccelerator(Accelerator** ppAccelerator)
{
	assert(ppAccelerator);
	*ppAccelerator = 0;
	return QSTATUS_SUCCESS;
}

QSTATUS qs::DefaultWindowHandler::preTranslateAccelerator(
	const MSG& msg, bool* pbProcessed)
{
	assert(pbProcessed);
	*pbProcessed = false;
	return QSTATUS_SUCCESS;
}

bool qs::DefaultWindowHandler::isFrame() const
{
	return false;
}

LRESULT qs::DefaultWindowHandler::windowProc(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	assert(pWindowBase_);
	return pWindowBase_->defWindowProc(uMsg, wParam, lParam);
}

DefWindowProcHolder* qs::DefaultWindowHandler::getDefWindowProcHolder()
{
	assert(pWindowBase_);
	return pWindowBase_;
}


#if _WIN32_WCE >= 200 && (_WIN32_WCE < 300 || !defined _WIN32_WCE_PSPC)

/****************************************************************************
 *
 * CommandBand
 *
 */

qs::CommandBand::CommandBand(bool bDeleteThis, QSTATUS* pstatus) :
	WindowBase(bDeleteThis, pstatus),
	DefaultWindowHandler(pstatus)
{
	assert(pstatus);
	
	if (*pstatus != QSTATUS_SUCCESS)
		return;
	
	*pstatus = QSTATUS_SUCCESS;
	
	setWindowHandler(this, false);
}

qs::CommandBand::~CommandBand()
{
}

LRESULT qs::CommandBand::windowProc(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	BEGIN_MESSAGE_HANDLER()
		HANDLE_SIZE()
	END_MESSAGE_HANDLER()
	return DefaultWindowHandler::windowProc(uMsg, wParam, lParam);
}

LRESULT qs::CommandBand::onSize(UINT nFlags, int cx, int cy)
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

#endif // _WIN32_WCE >= 200 && (_WIN32_WCE < 300 || !defined _WIN32_WCE_PSPC)


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

QSTATUS qs::Cursor::reset()
{
	::SetCursor(hcursor_);
	bReset_ = true;
	return QSTATUS_SUCCESS;
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

QSTATUS qs::WaitCursor::reset()
{
	return cursor_.reset();
}
