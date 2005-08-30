/*
 * $Id$
 *
 * Copyright(C) 1998-2005 Satoshi Nakamura
 * All rights reserved.
 *
 */

#include <qsaction.h>
#include <qsmenu.h>
#include <qsuiutil.h>
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
	void updateCommand(CommandUpdate* pcu,
					   bool bText);
	
	FrameWindow* pThis_;
	HINSTANCE hInstResource_;
	HWND hwndBands_;
#if defined _WIN32_WCE && _WIN32_WCE >= 300 && defined _WIN32_WCE_PSPC
	SHACTIVATEINFO shActivateInfo_;
#endif
};

void qs::FrameWindowImpl::updateCommand(CommandUpdate* pcu,
										bool bText)
{
	UINT nId = pcu->getId();
	
	if ((ActionMap::ID_MIN <= nId && nId < ActionMap::ID_MAX) ||
		nId == IDOK || nId == IDCANCEL) {
		const ActionParam* pParam = pThis_->getActionParamInternal(nId);
		if (pParam)
			nId = pParam->getBaseId();
		
		Action* pAction = pThis_->getActionInternal(nId);
		if (pAction) {
			ActionEvent event(nId, 0, pParam);
			pcu->setEnable(pAction->isEnabled(event));
			pcu->setCheck(pAction->isChecked(event));
			if (bText) {
				wstring_ptr wstrText(pAction->getText(event));
				if (wstrText.get())
					pcu->setText(wstrText.get(), true);
				
				Accelerator* pAccelerator = pThis_->WindowBase::getAccelerator();
				if (pAccelerator)
					pcu->updateText();
			}
		}
		else {
			pcu->setEnable(false);
			pcu->setCheck(false);
		}
	}
}


/****************************************************************************
 *
 * FrameWindow
 *
 */

qs::FrameWindow::FrameWindow(HINSTANCE hInstResource,
							 bool bDeleteThis) :
	WindowBase(bDeleteThis),
	pImpl_(0)
{
	pImpl_ = new FrameWindowImpl();
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

void qs::FrameWindow::processIdle()
{
	Window wndBar(getToolbar());
	if (wndBar.isVisible()) {
#ifdef _WIN32_WCE
		Window wndToolbar(CommandBands_GetCommandBar(wndBar.getHandle(),
			wndBar.sendMessage(RB_IDTOINDEX, getBarId(1))));
#else
		REBARBANDINFO rbbi = {
			sizeof(rbbi),
			RBBIM_CHILD
		};
		wndBar.sendMessage(RB_GETBANDINFO, 0, reinterpret_cast<LPARAM>(&rbbi));
		Window wndToolbar(rbbi.hwndChild);
#endif
		int nCount = static_cast<int>(wndToolbar.sendMessage(TB_BUTTONCOUNT));
		for (int n = 0; n < nCount; ++n) {
			TBBUTTON button;
			wndToolbar.sendMessage(TB_GETBUTTON, n, reinterpret_cast<LPARAM>(&button));
			if ((button.fsStyle & TBSTYLE_SEP) == 0) {
				CommandUpdateToolbar cut(wndToolbar.getHandle(), button.idCommand);
				pImpl_->updateCommand(&cut, false);
			}
		}
	}
}

bool qs::FrameWindow::save()
{
#if _WIN32_WCE >= 300 && defined _WIN32_WCE_PSPC
	HWND hwndToolbar = getToolbar();
	if (hwndToolbar) {
		COMMANDBANDSRESTOREINFO cbri = { sizeof(cbri) };
		if (CommandBands_GetRestoreInformation(hwndToolbar, 0, &cbri) && cbri.wID != -1) {
			if (!setCommandBandsRestoreInfo(1, cbri))
				return false;
		}
	}
#elif defined _WIN32_WCE && (_WIN32_WCE < 300 || !defined _WIN32_WCE_PSPC)
	HWND hwndToolbar = getToolbar();
	if (hwndToolbar) {
		int nStored = 0;
		for (int n = 0; nStored < 2; ++n) {
			COMMANDBANDSRESTOREINFO cbri = { sizeof(cbri) };
			if (CommandBands_GetRestoreInformation(hwndToolbar, n, &cbri) && cbri.wID != -1) {
				if (!setCommandBandsRestoreInfo(nStored, cbri))
					return false;
				++nStored;
			}
		}
	}
#endif
	
	return true;
}

bool qs::FrameWindow::getToolbarButtons(Toolbar* pToolbar)
{
	assert(pToolbar);
	return false;
}

bool qs::FrameWindow::createToolbarButtons(void* pCreateParam,
										   HWND hwndToolbar)
{
	return true;
}

HMENU qs::FrameWindow::getMenuHandle(void* pCreateParam)
{
	return 0;
}

UINT qs::FrameWindow::getMenuId()
{
	return 0;
}

UINT qs::FrameWindow::getIconId()
{
	return 0;
}

DynamicMenuCreator* qs::FrameWindow::getDynamicMenuCreator(DWORD dwData)
{
	return 0;
}

bool qs::FrameWindow::isFrame() const
{
	return true;
}

LRESULT qs::FrameWindow::windowProc(UINT uMsg,
									WPARAM wParam,
									LPARAM lParam)
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

LRESULT qs::FrameWindow::onActivate(UINT nFlags,
									HWND hwnd,
									bool bMinimized)
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
	
	QTRY {
		HMENU hmenu = getMenuHandle(pCreateStruct->lpCreateParams);
		
		UINT nMenuId = 0;
		if (!hmenu)
			nMenuId = getMenuId();
		
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
		bool bToolbar = getToolbarButtons(&toolbar);
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
					::SendMessage(mbi.hwndMB, TB_INSERTBUTTON, n,
						reinterpret_cast<LPARAM>(&button));
				}
				while (::RemoveMenu(hmenu, 0, MF_BYPOSITION))
					;
				::DestroyMenu(hmenu);
			}
			std::auto_ptr<MenuBarWindow> pMenuBarWindow(
				new MenuBarWindow(getHandle()));
			if (!pMenuBarWindow->subclassWindow(mbi.hwndMB))
				return -1;
			pMenuBarWindow.release();
			
			pImpl_->hwndBands_ = CommandBands_Create(getInstanceHandle(), getHandle(),
				toolbar.nId_, RBS_VARHEIGHT | RBS_BANDBORDERS | CCS_NOPARENTALIGN, 0);
			if (!pImpl_->hwndBands_)
				return -1;
			
			COMMANDBANDSRESTOREINFO cbri;
			if (!getCommandBandsRestoreInfo(1, &cbri))
				return -1;
			if (cbri.cbSize != sizeof(cbri)) {
				cbri.cbSize = sizeof(cbri);
				cbri.wID = getBarId(1);
				cbri.fStyle = RBBS_NOGRIPPER;
				cbri.cxRestored = 1;
				cbri.fMaximized = TRUE;
			}
			REBARBANDINFO rbbi;
			rbbi.cbSize = sizeof(REBARBANDINFO);
			rbbi.fMask = RBBIM_ID | RBBIM_STYLE| RBBIM_SIZE;
			rbbi.fStyle = cbri.fStyle;
			rbbi.wID = cbri.wID;
			rbbi.cx = cbri.cxRestored;
#if _WIN32_WCE >= 421
			int nLogPixel = UIUtil::getLogPixel();
			if (nLogPixel != 96) {
				const int nDefaultBarHeight = 24;
				rbbi.fMask |= RBBIM_CHILDSIZE;
				rbbi.fStyle |= RBBS_VARIABLEHEIGHT;
				rbbi.cyChild = 16 + static_cast<int>((nDefaultBarHeight - 16)*(nLogPixel/96.0));
				rbbi.cyMaxChild = rbbi.cyChild;
				rbbi.cyIntegral = 1;
			}
#endif
			CommandBands_AddBands(pImpl_->hwndBands_, getInstanceHandle(), 1, &rbbi);
			
			HWND hwndBarButton = CommandBands_GetCommandBar(pImpl_->hwndBands_, 0);
			if (toolbar.nBitmapCount_ != 0)
				CommandBar_AddBitmap(hwndBarButton, pImpl_->hInstResource_,
					toolbar.nBitmapId_, toolbar.nBitmapCount_, 0, 0);
			if (toolbar.nSize_ != 0)
				CommandBar_AddButtons(hwndBarButton, toolbar.nSize_, toolbar.ptbButton_);
			
			if (!createToolbarButtons(pCreateStruct->lpCreateParams, hwndBarButton))
				return -1;
			
			if (cbri.fMaximized)
				::SendMessage(pImpl_->hwndBands_, RB_MAXIMIZEBAND, 0, 0);
			
			std::auto_ptr<CommandBand> pCommandBand(new CommandBand(true));
			if (!pCommandBand->subclassWindow(pImpl_->hwndBands_))
				return -1;
			pCommandBand.release();
#elif _WIN32_WCE >= 200
			pImpl_->hwndBands_ = CommandBands_Create(getInstanceHandle(),
				getHandle(), toolbar.nId_, RBS_VARHEIGHT | RBS_BANDBORDERS, 0);
			if (!pImpl_->hwndBands_)
				return -1;
			
			COMMANDBANDSRESTOREINFO cbri[2];
			for (int n = 0; n < 2; ++n) {
				if (!getCommandBandsRestoreInfo(n, &cbri[n]))
					return -1;
				if (cbri[n].cbSize != sizeof(cbri[n])) {
					cbri[n].cbSize = sizeof(cbri[n]);
					cbri[n].wID = getBarId(n);
					cbri[n].fStyle = n == 0 ? RBBS_NOGRIPPER : 0;
					cbri[n].cxRestored = n == 0 ? 320 : 370;
					cbri[n].fMaximized = FALSE;
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
			
			if (!createToolbarButtons(pCreateStruct->lpCreateParams, hwndBarButton))
				return -1;
			
			for (int n = 0; n < 2; ++n) {
				if (cbri[n].fMaximized)
					::SendMessage(pImpl_->hwndBands_, RB_MAXIMIZEBAND, n, 0);
			}
			
			CommandBands_AddAdornments(pImpl_->hwndBands_, pImpl_->hInstResource_, 0, 0);
			
			std::auto_ptr<CommandBand> pCommandBand(new CommandBand(true));
			if (!pCommandBand->subclassWindow(pImpl_->hwndBands_))
				return -1;
			pCommandBand.release();
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
			
			if (!createToolbarButtons(pCreateStruct->lpCreateParams, pImpl_->hwndBands_))
				return -1;
			
			CommandBar_AddAdornments(pImpl_->hwndBands_, 0, 0);
#else // _WIN32_WCE
			pImpl_->hwndBands_ = ::CreateWindowEx(WS_EX_TOOLWINDOW, REBARCLASSNAME,
				0, WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN |
				CCS_TOP | RBS_BANDBORDERS | RBS_AUTOSIZE | RBS_VARHEIGHT,
				0, 0, 0, 0, getHandle(), 0, pImpl_->hInstResource_, 0);
			if (!pImpl_->hwndBands_)
				return -1;
			
			HWND hwndToolbar = ::CreateToolbarEx(pImpl_->hwndBands_,
				WS_CHILD | WS_VISIBLE | CCS_NOPARENTALIGN | CCS_NORESIZE |
				TBSTYLE_FLAT | TBSTYLE_TOOLTIPS,
				toolbar.nId_, toolbar.nBitmapCount_, pImpl_->hInstResource_,
				toolbar.nBitmapId_, toolbar.ptbButton_, toolbar.nSize_,
				16, 16, 16, 16, sizeof(TBBUTTON));
			if (!hwndToolbar)
				return -1;
			if (!createToolbarButtons(pCreateStruct->lpCreateParams, hwndToolbar))
				return -1;
			::SendMessage(hwndToolbar, TB_AUTOSIZE, 0, 0);
			
			SIZE sizeToolbar;
			::SendMessage(hwndToolbar, TB_GETMAXSIZE, 0, reinterpret_cast<LPARAM>(&sizeToolbar));
			
			REBARBANDINFO rbbi = {
				sizeof(rbbi),
				RBBIM_STYLE | RBBIM_CHILD | RBBIM_CHILDSIZE,
				RBBS_VARIABLEHEIGHT,
				0,
				0,
				0,
				0,
				0,
				hwndToolbar,
				0,
				0,
				0,
				0,
				0,
				sizeToolbar.cy + 2,
				sizeToolbar.cy + 2,
				1
			};
			::SendMessage(pImpl_->hwndBands_, RB_INSERTBAND, -1, reinterpret_cast<LPARAM>(&rbbi));
#endif // _WIN32_WCE
		}
		
#ifdef _WIN32_WCE
		UINT nIconId = getIconId();
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
		
		MessageLoop::getMessageLoop().addFrame(this);
	}
	QCATCH_ALL() {
		return -1;
	}
	
	return 0;
}

LRESULT qs::FrameWindow::onDestroy()
{
	MessageLoop::getMessageLoop().removeFrame(this);
	return DefaultWindowHandler::onDestroy();
}

LRESULT qs::FrameWindow::onInitMenuPopup(HMENU hmenu,
										 UINT nIndex,
										 bool bSysMenu)
{
	if (!bSysMenu) {
		UINT nIndex = 0;
		while (true) {
			MENUITEMINFO mii = {
				sizeof(mii),
				MIIM_DATA
			};
			if (!::GetMenuItemInfo(hmenu, nIndex, TRUE, &mii))
				break;
			
			if (mii.dwItemData != 0) {
				DynamicMenuCreator* pMenuCreator = getDynamicMenuCreator(mii.dwItemData);
				if (pMenuCreator)
					nIndex = pMenuCreator->createMenu(hmenu, nIndex);
				else
					++nIndex;
			}
			else {
				++nIndex;
			}
		}
	}
	
	MENUITEMINFO mii = {
		sizeof(mii),
		MIIM_ID | MIIM_SUBMENU,
	};
	for (int nItem = 0; ::GetMenuItemInfo(hmenu, nItem, true, &mii); ++nItem) {
		if (!mii.hSubMenu && mii.wID < 0xf000 && mii.wID != 0) {
			CommandUpdateMenu cum(hmenu, mii.wID);
			pImpl_->updateCommand(&cum, true);
		}
	}
	
	return DefaultWindowHandler::onInitMenuPopup(hmenu, nIndex, bSysMenu);
}

LRESULT qs::FrameWindow::onSettingChange(WPARAM wParam,
										 LPARAM lParam)
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

qs::MenuBarWindow::MenuBarWindow(HWND hwndFrame) :
	WindowBase(true),
	hwndFrame_(hwndFrame)
{
	setWindowHandler(this, false);
}

qs::MenuBarWindow::~MenuBarWindow()
{
}

LRESULT qs::MenuBarWindow::windowProc(UINT uMsg,
									  WPARAM wParam,
									  LPARAM lParam)
{
	BEGIN_MESSAGE_HANDLER()
		HANDLE_INITMENUPOPUP()
	END_MESSAGE_HANDLER()
	return DefaultWindowHandler::windowProc(uMsg, wParam, lParam);
}

LRESULT qs::MenuBarWindow::onInitMenuPopup(HMENU hmenu,
										   UINT nIndex,
										   bool bSysMenu)
{
	::SendMessage(hwndFrame_, WM_INITMENUPOPUP, reinterpret_cast<WPARAM>(hmenu),
		MAKELPARAM(static_cast<WORD>(nIndex), static_cast<WORD>(bSysMenu)));
	return 0;
}

#endif
