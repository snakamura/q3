/*
 * $Id$
 *
 * Copyright(C) 1998-2003 Satoshi Nakamura
 * All rights reserved.
 *
 */

#include <qsaction.h>
#include <qserror.h>
#include <qsnew.h>
#include <qswindow.h>

#ifdef _WIN32_WCE_PSPC
#	include <aygshell.h>
#endif

#include "framewindow.h"
#include "resourceinc.h"

using namespace qs;


/****************************************************************************
 *
 * FrameWindowImpl
 *
 */

struct qs::FrameWindowImpl
{
	QSTATUS updateCommand(CommandUpdate* pcu, bool bText);
	
	FrameWindow* pThis_;
	HINSTANCE hInstResource_;
	HWND hwndBands_;
#if defined _WIN32_WCE && _WIN32_WCE >= 300 && defined _WIN32_WCE_PSPC
	SHACTIVATEINFO shActivateInfo_;
#endif
};

QSTATUS qs::FrameWindowImpl::updateCommand(CommandUpdate* pcu, bool bText)
{
	DECLARE_QSTATUS();
	
	UINT nId = pcu->getId();
	
	Action* pAction = 0;
	status = pThis_->getAction(nId, &pAction);
	CHECK_QSTATUS();
	if (pAction) {
		ActionEvent event(nId, 0);
		
		bool bEnabled = false;
		status = pAction->isEnabled(event, &bEnabled);
		CHECK_QSTATUS();
		pcu->setEnable(bEnabled);
		
		bool bChecked = false;
		status = pAction->isChecked(event, &bChecked);
		CHECK_QSTATUS();
		pcu->setCheck(bChecked);
		
		if (bText) {
			string_ptr<WSTRING> wstrText;
			status = pAction->getText(event, &wstrText);
			CHECK_QSTATUS();
			if (wstrText.get())
				pcu->setText(wstrText.get(), true);
			
			Accelerator* pAccelerator = 0;
			status = pThis_->WindowBase::getAccelerator(&pAccelerator);
			CHECK_QSTATUS();
			if (pAccelerator)
				pcu->updateText();
		}
	}
	else {
		pcu->setEnable(false);
		pcu->setCheck(false);
	}
	
	return QSTATUS_SUCCESS;
}


/****************************************************************************
 *
 * FrameWindow
 *
 */

qs::FrameWindow::FrameWindow(HINSTANCE hInstResource,
	bool bDeleteThis, QSTATUS* pstatus) :
	WindowBase(bDeleteThis, pstatus),
	DefaultWindowHandler(pstatus),
	pImpl_(0)
{
	if (*pstatus != QSTATUS_SUCCESS)
		return;
	
	DECLARE_QSTATUS();
	
	status = newObject(&pImpl_);
	CHECK_QSTATUS_SET(pstatus);
	pImpl_->pThis_ = this;
	pImpl_->hInstResource_ = hInstResource;
	pImpl_->hwndBands_ = 0;
	
	setWindowHandler(this, false);
}

qs::FrameWindow::~FrameWindow()
{
	delete pImpl_;
	pImpl_ = 0;
}

HWND qs::FrameWindow::getToolbar() const
{
	assert(pImpl_);
	return pImpl_->hwndBands_;
}

int qs::FrameWindow::getToolbarHeight() const
{
	HWND hwndToolbar = getToolbar();
	
	int nBarHeight = 0;
#if !defined _WIN32_WCE || (_WIN32_WCE >= 300 && defined _WIN32_WCE_PSPC)
	RECT rectToolbar;
	::GetWindowRect(hwndToolbar, &rectToolbar);
	nBarHeight = rectToolbar.bottom - rectToolbar.top;
#elif _WIN32_WCE >= 200
	nBarHeight = CommandBands_Height(hwndToolbar);
#elif defined _WIN32_WCE
	nBarHeight = CommandBar_Height(hwndToolbar);
#endif // _WIN32_WCE
	
	return nBarHeight;
}

void qs::FrameWindow::adjustWindowSize(LPARAM lParam)
{
#if _WIN32_WCE >= 200
	RECT rect;
#ifdef _WIN32_WCE_PSPC
	SIPINFO si;
	si.cbSize = sizeof(si);
	si.dwImDataSize = 0;
	si.pvImData = 0;
	::SHSipInfo(SPI_GETSIPINFO, lParam, &si, 0);
	rect = si.rcVisibleDesktop;
#if _WIN32_WCE >= 300
	if ((si.fdwFlags & SIPF_ON) == 0) {
		int nToolbarHeight = getToolbarHeight();
		rect.bottom -= nToolbarHeight;
	}
#endif
#else // _WIN32_WCE_PSPC
	::SystemParametersInfo(SPI_GETWORKAREA, 0, &rect, 0);
#endif // _WIN32_WCE_PSPC
	setWindowPos(0, 0, rect.top, rect.right - rect.left,
		rect.bottom - rect.top, SWP_NOZORDER);
#endif
}

QSTATUS qs::FrameWindow::processIdle()
{
	DECLARE_QSTATUS();
	
#ifndef _WIN32_WCE_PSPC
	HWND hwnd = getToolbar();
#ifdef _WIN32_WCE
	UINT nId = 0;
	status = getBarId(1, &nId);
	CHECK_QSTATUS();
	hwnd = CommandBands_GetCommandBar(hwnd,
		::SendMessage(hwnd, RB_IDTOINDEX, nId, 0));
#endif
	Window wnd(hwnd);
	if (wnd.getHandle() && wnd.isVisible()) {
		int nCount = wnd.sendMessage(TB_BUTTONCOUNT);
		for (int n = 0; n < nCount; ++n) {
			TBBUTTON button;
			wnd.sendMessage(TB_GETBUTTON, n, reinterpret_cast<LPARAM>(&button));
			if ((button.fsStyle & TBSTYLE_SEP) == 0) {
				CommandUpdateToolbar cut(wnd.getHandle(), button.idCommand, &status);
				CHECK_QSTATUS();
				status = pImpl_->updateCommand(&cut, false);
				CHECK_QSTATUS();
			}
		}
	}
#endif
	
	return QSTATUS_SUCCESS;
}

QSTATUS qs::FrameWindow::save()
{
	DECLARE_QSTATUS();
	
#if defined _WIN32_WCE && (_WIN32_WCE < 300 || !defined _WIN32_WCE_PSPC)
	HWND hwndToolbar = getToolbar();
	if (hwndToolbar) {
		int nStored = 0;
		for (int n = 0; nStored < 2; ++n) {
			COMMANDBANDSRESTOREINFO cbri = { sizeof(cbri) };
			if (CommandBands_GetRestoreInformation(hwndToolbar, n, &cbri) && cbri.wID != -1) {
				status = setCommandBandsRestoreInfo(nStored, cbri);
				CHECK_QSTATUS();
				++nStored;
			}
		}
	}
#endif
	
	return QSTATUS_SUCCESS;
}

QSTATUS qs::FrameWindow::getToolbarButtons(Toolbar* pToolbar, bool* pbToolbar)
{
	assert(pToolbar);
	assert(pbToolbar);
	*pbToolbar = false;
	return QSTATUS_SUCCESS;
}

QSTATUS qs::FrameWindow::createToolbarButtons(void* pCreateParam, HWND hwndToolbar)
{
	return QSTATUS_SUCCESS;
}

QSTATUS qs::FrameWindow::getMenuHandle(void* pCreateParam, HMENU* phmenu)
{
	assert(phmenu);
	*phmenu = 0;
	return QSTATUS_SUCCESS;
}

QSTATUS qs::FrameWindow::getMenuId(UINT* pnId)
{
	assert(pnId);
	*pnId = 0;
	return QSTATUS_SUCCESS;
}

QSTATUS qs::FrameWindow::getIconId(UINT* pnId)
{
	assert(pnId);
	*pnId = 0;
	return QSTATUS_SUCCESS;
}

bool qs::FrameWindow::isFrame() const
{
	return true;
}

LRESULT qs::FrameWindow::windowProc(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	BEGIN_MESSAGE_HANDLER()
		HANDLE_ACTIVATE()
		HANDLE_CREATE()
		HANDLE_DESTROY()
		HANDLE_INITMENUPOPUP()
		HANDLE_SETTINGCHANGE()
	END_MESSAGE_HANDLER()
	return DefaultWindowHandler::windowProc(uMsg, wParam, lParam);
}

LRESULT qs::FrameWindow::onActivate(UINT nFlags, HWND hwnd, bool bMinimized)
{
#if defined _WIN32_WCE && _WIN32_WCE >= 300 && defined _WIN32_WCE_PSPC
	::SHHandleWMActivate(getHandle(), MAKEWPARAM(nFlags, bMinimized),
		reinterpret_cast<LPARAM>(hwnd), &pImpl_->shActivateInfo_, 0);
#endif
	return DefaultWindowHandler::onActivate(nFlags, hwnd, bMinimized);
}

LRESULT qs::FrameWindow::onCreate(CREATESTRUCT* pCreateStruct)
{
	if (DefaultWindowHandler::onCreate(pCreateStruct) == -1)
		return -1;
	
	DECLARE_QSTATUS();
	
	HMENU hmenu = 0;
	status = getMenuHandle(pCreateStruct->lpCreateParams, &hmenu);
	CHECK_QSTATUS_VALUE(-1);
	
	UINT nMenuId = 0;
	if (!hmenu) {
		status = getMenuId(&nMenuId);
		CHECK_QSTATUS_VALUE(-1);
	}
	
#ifndef _WIN32_WCE
	if (!hmenu)
		hmenu = ::LoadMenu(pImpl_->hInstResource_, MAKEINTRESOURCE(nMenuId));
	setMenu(hmenu);
#endif
	
	Toolbar toolbar = {
		0,
		0,
		0,
		0,
		0,
	};
	bool bToolbar = false;
	status = getToolbarButtons(&toolbar, &bToolbar);
	CHECK_QSTATUS_VALUE(-1);
	if (bToolbar) {
#if _WIN32_WCE >= 300 && defined _WIN32_WCE_PSPC
		SHMENUBARINFO mbi = {
			sizeof(mbi),
			getHandle(),
			0,
			hmenu ? IDR_EMPTY : nMenuId,
			hmenu ? getDllInstanceHandle() : pImpl_->hInstResource_,
			0,
			0
		};
		if (!::SHCreateMenuBar(&mbi))
			return -1;
		pImpl_->hwndBands_ = mbi.hwndMB;
		if (hmenu) {
			int nId = 1000;
			TCHAR tszText[256];
			for (int n = 0; ; ++n) {
				MENUITEMINFO mii = {
					sizeof(mii),
					MIIM_TYPE | MIIM_SUBMENU,
					0,
					0,
					0,
					0,
					0,
					0,
					0,
					tszText,
					countof(tszText) - 1,
				};
				if (!::GetMenuItemInfo(hmenu, n, TRUE, &mii))
					break;
				
#define TBSTYLE_NO_DROPDOWN_ARROW 0x0080
				TBBUTTON button = {
					I_IMAGENONE,
					nId++,
					TBSTATE_ENABLED,
					TBSTYLE_DROPDOWN | TBSTYLE_NO_DROPDOWN_ARROW | TBSTYLE_AUTOSIZE,
					{ 0, 0 },
					reinterpret_cast<DWORD_PTR>(mii.hSubMenu),
					reinterpret_cast<INT_PTR>(tszText)
				};
				::SendMessage(pImpl_->hwndBands_, TB_INSERTBUTTON, n,
					reinterpret_cast<LPARAM>(&button));
			}
			while (::RemoveMenu(hmenu, 0, MF_BYPOSITION))
				;
			::DestroyMenu(hmenu);
		}
		std::auto_ptr<MenuBarWindow> pMenuBarWindow;
		status = newQsObject(getHandle(), &pMenuBarWindow);
		CHECK_QSTATUS_VALUE(-1);
		status = pMenuBarWindow->subclassWindow(pImpl_->hwndBands_);
		CHECK_QSTATUS_VALUE(-1);
		pMenuBarWindow.release();
#elif _WIN32_WCE >= 200
		pImpl_->hwndBands_ = CommandBands_Create(getInstanceHandle(),
			getHandle(), toolbar.nId_, RBS_VARHEIGHT | RBS_BANDBORDERS, 0);
		if (!pImpl_->hwndBands_)
			return -1;
		
		COMMANDBANDSRESTOREINFO cbri[2];
		for (int n = 0; n < 2; ++n) {
			status = getCommandBandsRestoreInfo(n, &cbri[n]);
			CHECK_QSTATUS_VALUE(-1);
			if (cbri[n].cbSize != sizeof(cbri[n])) {
				cbri[n].cbSize = sizeof(cbri[n]);
				getBarId(n, &cbri[n].wID);
				cbri[n].fStyle = n == 0 ? RBBS_NOGRIPPER : 0;
				cbri[n].cxRestored = n == 0 ? 320 : 370;
				cbri[n].fMaximized = false;
			}
		}
		REBARBANDINFO rbbi[2];
		rbbi[0].cbSize = sizeof(REBARBANDINFO);
		rbbi[0].fMask = RBBIM_ID | RBBIM_STYLE| RBBIM_SIZE;
		rbbi[0].fStyle = cbri[0].fStyle;
		rbbi[0].wID = cbri[0].wID;
		rbbi[0].cx = cbri[0].cxRestored;
		rbbi[1].cbSize = sizeof(REBARBANDINFO);
		rbbi[1].fMask = RBBIM_ID | RBBIM_SIZE | RBBIM_STYLE;
		rbbi[1].wID = cbri[1].wID;
		rbbi[1].cx = cbri[1].cxRestored;
		rbbi[1].fStyle = cbri[1].fStyle;
		CommandBands_AddBands(pImpl_->hwndBands_, getInstanceHandle(),
			sizeof(rbbi)/sizeof(REBARBANDINFO), rbbi);
		HWND hCmdBarMenu =  CommandBands_GetCommandBar(pImpl_->hwndBands_, 0);
		if (hmenu)
			CommandBar_InsertMenubarEx(hCmdBarMenu,
				0, reinterpret_cast<LPTSTR>(hmenu), 0);
		else
			CommandBar_InsertMenubar(hCmdBarMenu,
				pImpl_->hInstResource_, nMenuId, 0);
		
		HWND hwndBarButton = CommandBands_GetCommandBar(pImpl_->hwndBands_, 1);
		if (toolbar.nBitmapCount_ != 0)
			CommandBar_AddBitmap(hwndBarButton, pImpl_->hInstResource_,
				toolbar.nBitmapId_, toolbar.nBitmapCount_, 0, 0);
		if (toolbar.nSize_ != 0)
			CommandBar_AddButtons(hwndBarButton, toolbar.nSize_, toolbar.ptbButton_);
		
		status = createToolbarButtons(pCreateStruct->lpCreateParams, hwndBarButton);
		CHECK_QSTATUS_VALUE(-1);
		
		for (n = 0; n < 2; ++n) {
			if (cbri[n].fMaximized)
				::SendMessage(pImpl_->hwndBands_, RB_MAXIMIZEBAND, n, 0);
		}
		
		CommandBands_AddAdornments(pImpl_->hwndBands_, pImpl_->hInstResource_, 0, 0);
		
		std::auto_ptr<CommandBand> apCommandBand;
		status = newQsObject(true, &apCommandBand);
		CHECK_QSTATUS_VALUE(-1);
		CommandBand* pCommandBand = apCommandBand.release();
		status = pCommandBand->subclassWindow(pImpl_->hwndBands_);
		CHECK_QSTATUS_VALUE(-1);
#elif defined _WIN32_WCE
		pImpl_->hwndBands_ = CommandBar_Create(
			pImpl_->hInstResource, getHandle(), ID_BAR);
		if (!pImpl_->hwndBands_)
			return -1;
		
		if (hmenu)
			CommandBar_InsertMenubar(pImpl_->hwndBands_,
				0, reinterpret_cast<LPTSTR>(hmenu), 0)
		else
			CommandBar_InsertMenubar(pImpl_->hwndBands_,
				pImpl_->hInstResource_, nMenuId, 0);
		
		status = createToolbarButtons(pCreateStruct->lpCreateParams, pImpl_->hwndBands_);
		CHECK_QSTATUS_VALUE(-1);
		
		CommandBar_AddAdornments(pImpl_->hwndBands_, 0, 0);
#else // _WIN32_WCE
		pImpl_->hwndBands_ = ::CreateToolbarEx(getHandle(),
			WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS |
			WS_CLIPCHILDREN | TBSTYLE_FLAT | CCS_TOP,
			toolbar.nId_, toolbar.nBitmapCount_, pImpl_->hInstResource_,
			toolbar.nBitmapId_, toolbar.ptbButton_, toolbar.nSize_,
			16, 16, 16, 16, sizeof(TBBUTTON));
		if (!pImpl_->hwndBands_)
			return -1;
		
		status = createToolbarButtons(pCreateStruct->lpCreateParams, pImpl_->hwndBands_);
		CHECK_QSTATUS_VALUE(-1);
#endif // _WIN32_WCE
	}
	
#ifdef _WIN32_WCE
	UINT nIconId = 0;
	status = getIconId(&nIconId);
	CHECK_QSTATUS_VALUE(-1);
	HICON hIcon = reinterpret_cast<HICON>(::LoadImage(pImpl_->hInstResource_,
		MAKEINTRESOURCE(nIconId), IMAGE_ICON, 16, 16, 0));
	if (hIcon)
		sendMessage(WM_SETICON,
			static_cast<WPARAM>(FALSE), reinterpret_cast<LPARAM>(hIcon));
#endif // _WIN32_WCE
	
#if defined _WIN32_WCE && _WIN32_WCE >= 300 && defined _WIN32_WCE_PSPC
	memset(&pImpl_->shActivateInfo_, 0, sizeof(pImpl_->shActivateInfo_));
	pImpl_->shActivateInfo_.cbSize = sizeof(pImpl_->shActivateInfo_);
#endif
	
	status = MessageLoop::getMessageLoop().addFrame(this);
	CHECK_QSTATUS_VALUE(-1);
	
	return 0;
}

LRESULT qs::FrameWindow::onDestroy()
{
	MessageLoop::getMessageLoop().removeFrame(this);
	return DefaultWindowHandler::onDestroy();
}

LRESULT qs::FrameWindow::onInitMenuPopup(HMENU hmenu, UINT nIndex, bool bSysMenu)
{
	DECLARE_QSTATUS();
	
	MENUITEMINFO mii = {
		sizeof(mii),
		MIIM_ID | MIIM_SUBMENU,
	};
	for (int nItem = 0; ::GetMenuItemInfo(hmenu, nItem, true, &mii); ++nItem) {
		if (!mii.hSubMenu && mii.wID < 0xf000 && mii.wID != 0) {
			CommandUpdateMenu cum(hmenu, mii.wID, &status);
			CHECK_QSTATUS_VALUE(0);
			status = pImpl_->updateCommand(&cum, true);
			CHECK_QSTATUS_VALUE(0);
		}
	}
	
	return DefaultWindowHandler::onInitMenuPopup(hmenu, nIndex, bSysMenu);
}

LRESULT qs::FrameWindow::onSettingChange(WPARAM wParam, LPARAM lParam)
{
#if defined _WIN32_WCE && _WIN32_WCE >= 300 && defined _WIN32_WCE_PSPC
	::SHHandleWMSettingChange(getHandle(),
		wParam, lParam, &pImpl_->shActivateInfo_);
#endif
	return DefaultWindowHandler::onSettingChange(wParam, lParam);
}


#if defined _WIN32_WCE && _WIN32_WCE >= 300 && defined _WIN32_WCE_PSPC

/****************************************************************************
 *
 * MenuBarWindow
 *
 */

qs::MenuBarWindow::MenuBarWindow(HWND hwndFrame, QSTATUS* pstatus) :
	WindowBase(true, pstatus),
	DefaultWindowHandler(pstatus),
	hwndFrame_(hwndFrame)
{
	setWindowHandler(this, false);
}

qs::MenuBarWindow::~MenuBarWindow()
{
}

LRESULT qs::MenuBarWindow::windowProc(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	BEGIN_MESSAGE_HANDLER()
		HANDLE_INITMENUPOPUP()
	END_MESSAGE_HANDLER()
	return DefaultWindowHandler::windowProc(uMsg, wParam, lParam);
}

LRESULT qs::MenuBarWindow::onInitMenuPopup(HMENU hmenu, UINT nIndex, bool bSysMenu)
{
	::SendMessage(hwndFrame_, WM_INITMENUPOPUP, reinterpret_cast<WPARAM>(hmenu),
		MAKELPARAM(static_cast<WORD>(nIndex), static_cast<WORD>(bSysMenu)));
	return 0;
}

#endif
