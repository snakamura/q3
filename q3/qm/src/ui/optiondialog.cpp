/*
 * $Id$
 *
 * Copyright(C) 1998-2005 Satoshi Nakamura
 * All rights reserved.
 *
 */

#include <qmdocument.h>
#include <qmeditwindow.h>
#include <qmfoldercombobox.h>
#include <qmfolderlistwindow.h>
#include <qmfolderwindow.h>
#include <qmjunk.h>
#include <qmlistwindow.h>
#include <qmmainwindow.h>
#include <qmmessagewindow.h>
#include <qmsecurity.h>
#include <qmtabwindow.h>

#include <qsdevicecontext.h>
#include <qsinit.h>
#include <qsras.h>
#include <qsuiutil.h>

#include <commdlg.h>
#include <tchar.h>

#include "addressbookwindow.h"
#include "editframewindow.h"
#include "messageframewindow.h"
#include "optiondialog.h"
#include "resourceinc.h"
#include "../sync/syncmanager.h"
#include "../uimodel/tabmodel.h"
#include "../util/util.h"

#pragma warning(disable:4786)

using namespace qm;
using namespace qs;


/****************************************************************************
 *
 * OptionDialog
 *
 */

qm::OptionDialog::OptionDialog(Document* pDocument,
							   GoRound* pGoRound,
							   FilterManager* pFilterManager,
							   ColorManager* pColorManager,
							   SyncFilterManager* pSyncFilterManager,
							   AutoPilotManager* pAutoPilotManager,
							   MainWindow* pMainWindow,
							   FolderWindow* pFolderWindow,
							   FolderComboBox* pFolderComboBox,
							   ListWindow* pListWindow,
							   FolderListWindow* pFolderListWindow,
							   MessageWindow* pPreviewWindow,
							   MessageFrameWindowManager* pMessageFrameWindowManager,
							   EditFrameWindowManager* pEditFrameWindowManager,
#ifdef QMTABWINDOW
							   TabWindow* pTabWindow,
#endif
							   AddressBookFrameWindowManager* pAddressBookFrameWindowManager,
							   Profile* pProfile,
							   Panel panel) :
	DefaultDialog(IDD_OPTION),
	pDocument_(pDocument),
	pGoRound_(pGoRound),
	pFilterManager_(pFilterManager),
	pColorManager_(pColorManager),
	pSyncFilterManager_(pSyncFilterManager),
	pAutoPilotManager_(pAutoPilotManager),
	pMainWindow_(pMainWindow),
	pFolderWindow_(pFolderWindow),
	pFolderComboBox_(pFolderComboBox),
	pListWindow_(pListWindow),
	pFolderListWindow_(pFolderListWindow),
	pPreviewWindow_(pPreviewWindow),
	pMessageFrameWindowManager_(pMessageFrameWindowManager),
	pEditFrameWindowManager_(pEditFrameWindowManager),
#ifdef QMTABWINDOW
	pTabWindow_(pTabWindow),
#endif
	pAddressBookFrameWindowManager_(pAddressBookFrameWindowManager),
	pProfile_(pProfile),
	panel_(panel),
	pCurrentPanel_(0),
	nEnd_(-1)
{
	listPanel_.resize(MAX_PANEL);
	
	if (panel_ == PANEL_NONE) {
		int nPanel = pProfile_->getInt(L"OptionDialog", L"Panel", 0);
		if (nPanel < 0 || MAX_PANEL <= nPanel)
			nPanel = 0;
		panel_ = static_cast<Panel>(nPanel);
	}
}

qm::OptionDialog::~OptionDialog()
{
	std::for_each(listPanel_.begin(), listPanel_.end(), qs::deleter<OptionDialogPanel>());
}

int qm::OptionDialog::doModal(HWND hwndParent)
{
	ModalHandler* pModalHandler = getModalHandler();
	ModalHandlerInvoker invoker(pModalHandler, hwndParent);
	
	if (!create(hwndParent))
		return -1;
	Window(hwndParent).enableWindow(false);
	showWindow();
	
	MSG msg;
	while (nEnd_ == -1 && ::GetMessage(&msg, 0, 0, 0)) {
		if (!processDialogMessage(msg)) {
			::TranslateMessage(&msg);
			::DispatchMessage(&msg);
		}
	}
	Window(hwndParent).enableWindow(true);
	destroyWindow();
	
	return nEnd_;
}

INT_PTR qm::OptionDialog::dialogProc(UINT uMsg,
									 WPARAM wParam,
									 LPARAM lParam)
{
	BEGIN_DIALOG_HANDLER()
		HANDLE_SIZE()
	END_DIALOG_HANDLER()
	return DefaultDialog::dialogProc(uMsg, wParam, lParam);
}

LRESULT qm::OptionDialog::onCommand(WORD nCode,
									WORD nId)
{
#ifdef _WIN32_WCE_PSPC
	BEGIN_COMMAND_HANDLER()
		HANDLE_COMMAND_ID_CODE(IDC_SELECTOR, CBN_SELCHANGE, onSelectorSelChange)
	END_COMMAND_HANDLER()
#endif
	return DefaultDialog::onCommand(nCode, nId);
}

LRESULT qm::OptionDialog::onDestroy()
{
	removeNotifyHandler(this);
	
	pProfile_->setInt(L"OptionDialog", L"Panel", panel_);
	
#ifndef _WIN32_WCE
	RECT rect;
	getWindowRect(&rect);
	pProfile_->setInt(L"OptionDialog", L"Width", rect.right - rect.left);
	pProfile_->setInt(L"OptionDialog", L"Height", rect.bottom - rect.top);
#endif
	
	return DefaultDialog::onDestroy();
}

LRESULT qm::OptionDialog::onInitDialog(HWND hwndFocus,
									   LPARAM lParam)
{
	HINSTANCE hInst = Application::getApplication().getResourceHandle();
	
	HWND hwndSelector = getDlgItem(IDC_SELECTOR);
	struct {
		Panel panel_;
		UINT nId_;
	} items[] = {
		{ PANEL_FOLDER,			IDS_PANEL_FOLDER			},
		{ PANEL_LIST,			IDS_PANEL_LIST				},
		{ PANEL_PREVIEW,		IDS_PANEL_PREVIEW			},
		{ PANEL_MESSAGE,		IDS_PANEL_MESSAGE			},
		{ PANEL_HEADER,			IDS_PANEL_HEADER			},
		{ PANEL_EDIT,			IDS_PANEL_EDIT				},
		{ PANEL_EDIT2,			IDS_PANEL_EDIT2				},
#ifdef QMTABWINDOW
		{ PANEL_TAB,			IDS_PANEL_TAB				},
#endif
		{ PANEL_ADDRESSBOOK,	IDS_PANEL_ADDRESSBOOK		},
		{ PANEL_RULES,			IDS_PANEL_RULES				},
		{ PANEL_COLORS,			IDS_PANEL_COLORS			},
		{ PANEL_GOROUND,		IDS_PANEL_GOROUND			},
		{ PANEL_SIGNATURES,		IDS_PANEL_SIGNATURES		},
		{ PANEL_FIXEDFORMTEXTS,	IDS_PANEL_FIXEDFORMTEXTS	},
		{ PANEL_FILTERS,		IDS_PANEL_FILTERS			},
		{ PANEL_SYNCFILTERS,	IDS_PANEL_SYNCFILTERS		},
		{ PANEL_AUTOPILOT,		IDS_PANEL_AUTOPILOT			},
		{ PANEL_CONFIRM,		IDS_PANEL_CONFIRM			},
		{ PANEL_MISC,			IDS_PANEL_MISC				},
		{ PANEL_MISC2,			IDS_PANEL_MISC2				},
#ifndef _WIN32_WCE
		{ PANEL_JUNK,			IDS_PANEL_JUNK				},
#endif
		{ PANEL_SECURITY,		IDS_PANEL_SECURITY			}
	};
	for (int n = 0; n < countof(items); ++n) {
		wstring_ptr wstrName(loadString(hInst, items[n].nId_));
		W2T(wstrName.get(), ptszName);
#ifndef _WIN32_WCE_PSPC
		TVINSERTSTRUCT tvis = {
			TVI_ROOT,
			TVI_LAST,
			{
				TVIF_TEXT | TVIF_PARAM,
				0,
				0,
				0,
				const_cast<LPTSTR>(ptszName),
				0,
				0,
				0,
				0,
				items[n].panel_
			}
		};
		TreeView_InsertItem(hwndSelector, &tvis);
#else
		int nItem = Window(hwndSelector).sendMessage(
			CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(ptszName));
		Window(hwndSelector).sendMessage(CB_SETITEMDATA, nItem, items[n].panel_);
#endif
	}
	
	setCurrentPanel(panel_, true);
	
#ifdef _WIN32_WCE
	RECT rectWorkArea;
	::SystemParametersInfo(SPI_GETWORKAREA, 0, &rectWorkArea, 0);
	int nWidth = rectWorkArea.right - rectWorkArea.left;
	int nHeight = rectWorkArea.bottom - rectWorkArea.top;
	setWindowPos(0, 0, 0, nWidth, nHeight, SWP_NOZORDER | SWP_NOACTIVATE);
#else
	int nWidth = pProfile_->getInt(L"OptionDialog", L"Width", 620);
	int nHeight = pProfile_->getInt(L"OptionDialog", L"Height", 450);
	setWindowPos(0, 0, 0, nWidth, nHeight,
		SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE);
#endif
	
	addNotifyHandler(this);
	
	init(false);
	
	return TRUE;
}

LRESULT qm::OptionDialog::onOk()
{
	OptionDialogContext context;
	for (PanelList::const_iterator it = listPanel_.begin(); it != listPanel_.end(); ++it) {
		OptionDialogPanel* pPanel = *it;
		if (pPanel) {
			if (!pPanel->save(&context)) {
				// TODO
			}
		}
	}
	
	unsigned int nFlags = context.getFlags();
	if (nFlags & OptionDialogContext::FLAG_RELOADFOLDER) {
		pFolderWindow_->reloadProfiles();
		pFolderComboBox_->reloadProfiles();
	}
	if (nFlags & OptionDialogContext::FLAG_RELOADLIST) {
		pListWindow_->reloadProfiles();
		pFolderListWindow_->reloadProfiles();
	}
	if (nFlags & OptionDialogContext::FLAG_RELOADMESSAGE)
		pMessageFrameWindowManager_->reloadProfiles();
	if (nFlags & OptionDialogContext::FLAG_RELOADPREVIEW)
		pPreviewWindow_->reloadProfiles();
	if (nFlags & OptionDialogContext::FLAG_RELOADEDIT)
		pEditFrameWindowManager_->reloadProfiles();
#ifdef QMTABWINDOW
	if (nFlags & OptionDialogContext::FLAG_RELOADTAB)
		pTabWindow_->reloadProfiles();
#endif
	if (nFlags & OptionDialogContext::FLAG_RELOADADDRESSBOOK) {
		pDocument_->getAddressBook()->reloadProfiles();
		pAddressBookFrameWindowManager_->reloadProfiles();
	}
	if (nFlags & OptionDialogContext::FLAG_RELOADSECURITY)
		pDocument_->getSecurity()->reload();
	
	if (nFlags & OptionDialogContext::FLAG_LAYOUTMAINWINDOW)
		pMainWindow_->layout();
	if (nFlags & OptionDialogContext::FLAG_LAYOUTMESSAGEWINDOW)
		pMessageFrameWindowManager_->layout();
	if (nFlags & OptionDialogContext::FLAG_LAYOUTEDITWINDOW)
		pEditFrameWindowManager_->layout();
	
	nEnd_ = IDOK;
	
	return 0;
}

LRESULT qm::OptionDialog::onCancel()
{
	nEnd_ = IDCANCEL;
	return 0;
}

LRESULT qm::OptionDialog::onNotify(NMHDR* pnmhdr,
								   bool* pbHandled)
{
#ifndef _WIN32_WCE_PSPC
	BEGIN_NOTIFY_HANDLER()
		HANDLE_NOTIFY(TVN_SELCHANGED, IDC_SELECTOR, onSelectorSelChanged)
	END_NOTIFY_HANDLER()
#endif
	return NotifyHandler::onNotify(pnmhdr, pbHandled);
}

LRESULT qm::OptionDialog::onSize(UINT nFlags,
								 int cx,
								 int cy)
{
	layout();
	return DefaultDialog::onSize(nFlags, cx, cy);
}

#ifndef _WIN32_WCE_PSPC
LRESULT qm::OptionDialog::onSelectorSelChanged(NMHDR* pnmhdr,
											   bool* pbHandled)
{
	NMTREEVIEW* pnmtv = reinterpret_cast<NMTREEVIEW*>(pnmhdr);
	
	if (pnmtv->itemNew.hItem && pnmtv->itemNew.state & TVIS_SELECTED) {
		Panel panel = static_cast<Panel>(pnmtv->itemNew.lParam);
		setCurrentPanel(panel, false);
	}
	
	return 1;
}
#else
LRESULT qm::OptionDialog::onSelectorSelChange()
{
	Window selector(getDlgItem(IDC_SELECTOR));
	int nItem = selector.sendMessage(CB_GETCURSEL);
	Panel panel = static_cast<Panel>(selector.sendMessage(CB_GETITEMDATA, nItem));
	setCurrentPanel(panel, false);
	
	return 0;
}
#endif

void qm::OptionDialog::layout()
{
	RECT rect;
	getClientRect(&rect);
	
	Window selector(getDlgItem(IDC_SELECTOR));
#ifndef _WIN32_WCE_PSPC
#ifndef _WIN32_WCE
	Window ok(getDlgItem(IDOK));
	Window cancel(getDlgItem(IDCANCEL));
#endif
	
	RECT rectSelector;
	selector.getWindowRect(&rectSelector);
	int nSelectorWidth = rectSelector.right - rectSelector.left;
	
#ifndef _WIN32_WCE
	RECT rectButton;
	ok.getWindowRect(&rectButton);
	int nButtonWidth = rectButton.right - rectButton.left;
	int nButtonHeight = rectButton.bottom - rectButton.top;
#else
	int nButtonHeight = -5;
#endif
	
	HDWP hdwp = beginDeferWindowPos(5);
	hdwp = selector.deferWindowPos(hdwp, 0, 5, 5, nSelectorWidth,
		rect.bottom - 15 - nButtonHeight, SWP_NOACTIVATE | SWP_NOZORDER);
	hdwp = Window(pCurrentPanel_->getWindow()).deferWindowPos(hdwp, 0,
		nSelectorWidth + 5, 0, rect.right - nSelectorWidth - 5,
		rect.bottom - 5 - nButtonHeight, SWP_NOACTIVATE | SWP_NOZORDER);
#ifndef _WIN32_WCE
	hdwp = ok.deferWindowPos(hdwp, 0, rect.right - (nButtonWidth + 5)*2 - 15,
		rect.bottom - nButtonHeight - 5, nButtonWidth, nButtonHeight,
		SWP_NOZORDER | SWP_NOSIZE | SWP_NOACTIVATE);
	hdwp = cancel.deferWindowPos(hdwp, 0, rect.right - (nButtonWidth + 5) - 15,
		rect.bottom - nButtonHeight - 5, nButtonWidth, nButtonHeight,
		SWP_NOZORDER | SWP_NOSIZE | SWP_NOACTIVATE);
	hdwp = Window(getDlgItem(IDC_SIZEGRIP)).deferWindowPos(hdwp, 0,
		rect.right - rect.left - 13, rect.bottom - rect.top - 12,
		13, 12, SWP_NOZORDER | SWP_NOACTIVATE);
#endif
	
	endDeferWindowPos(hdwp);
#else
	RECT rectSelector;
	selector.getWindowRect(&rectSelector);
	screenToClient(&rectSelector);
	
	RECT rectButton;
	Window(getDlgItem(IDOK)).getWindowRect(&rectButton);
	screenToClient(&rectButton);
	
	Window(pCurrentPanel_->getWindow()).setWindowPos(0,
		0, rectSelector.bottom, rect.right - rect.left,
		rectButton.top - rectSelector.bottom, SWP_NOACTIVATE | SWP_NOZORDER);
#endif
}

void qm::OptionDialog::setCurrentPanel(Panel panel,
									   bool bForce)
{
	if (!bForce && panel == panel_)
		return;
	
#define BEGIN_PANEL() \
	switch (panel) {

#define END_PANEL() \
	default: \
		assert(false); \
		break; \
	}

#define PANEL1(name, dialog, arg1) \
	case name: \
		{ \
			std::auto_ptr<dialog##Dialog> pDialog(new dialog##Dialog(arg1)); \
			pDialog->Dialog::create(getHandle()); \
			listPanel_[panel] = pDialog.release(); \
		} \
		break
	
#define PANEL2(name, dialog, arg1, arg2) \
	case name: \
		{ \
			std::auto_ptr<dialog##Dialog> pDialog(new dialog##Dialog(arg1, arg2)); \
			pDialog->Dialog::create(getHandle()); \
			listPanel_[panel] = pDialog.release(); \
		} \
		break
	
#define PANEL3(name, dialog, arg1, arg2, arg3) \
	case name: \
		{ \
			std::auto_ptr<dialog##Dialog> pDialog(new dialog##Dialog(arg1, arg2, arg3)); \
			pDialog->Dialog::create(getHandle()); \
			listPanel_[panel] = pDialog.release(); \
		} \
		break
	
#define PANEL4(name, dialog, arg1, arg2, arg3, arg4) \
	case name: \
		{ \
			std::auto_ptr<dialog##Dialog> pDialog(new dialog##Dialog(arg1, arg2, arg3, arg4)); \
			pDialog->Dialog::create(getHandle()); \
			listPanel_[panel] = pDialog.release(); \
		} \
		break
	
	if (!listPanel_[panel]) {
		BEGIN_PANEL()
			PANEL3(PANEL_FOLDER, OptionFolder, pFolderWindow_, pFolderComboBox_, pProfile_);
			PANEL3(PANEL_LIST, OptionList, pListWindow_, pFolderListWindow_, pProfile_);
			PANEL2(PANEL_PREVIEW, OptionPreview, pPreviewWindow_, pProfile_);
			PANEL2(PANEL_MESSAGE, OptionMessage, pMessageFrameWindowManager_, pProfile_);
			PANEL3(PANEL_HEADER, OptionHeader, pMessageFrameWindowManager_, pPreviewWindow_, pProfile_);
			PANEL2(PANEL_EDIT, OptionEdit, pEditFrameWindowManager_, pProfile_);
			PANEL2(PANEL_EDIT2, OptionEdit2, pEditFrameWindowManager_, pProfile_);
#ifdef QMTABWINDOW
			PANEL2(PANEL_TAB, OptionTab, pTabWindow_, pProfile_);
#endif
			PANEL3(PANEL_ADDRESSBOOK, OptionAddressBook, pDocument_->getAddressBook(), pAddressBookFrameWindowManager_, pProfile_);
			PANEL3(PANEL_RULES, RuleSets, pDocument_->getRuleManager(), pDocument_, pProfile_);
			PANEL3(PANEL_COLORS, ColorSets, pColorManager_, pDocument_, pProfile_);
			PANEL4(PANEL_GOROUND, GoRound, pGoRound_, pDocument_, pSyncFilterManager_, pProfile_);
			PANEL3(PANEL_SIGNATURES, Signatures, pDocument_->getSignatureManager(), pDocument_, pProfile_);
			PANEL2(PANEL_FIXEDFORMTEXTS, FixedFormTexts, pDocument_->getFixedFormTextManager(), pProfile_);
			PANEL1(PANEL_FILTERS, Filters, pFilterManager_);
			PANEL2(PANEL_SYNCFILTERS, SyncFilterSets, pSyncFilterManager_, pProfile_);
			PANEL3(PANEL_AUTOPILOT, AutoPilot, pAutoPilotManager_, pGoRound_, pProfile_);
			PANEL1(PANEL_CONFIRM, OptionConfirm, pProfile_);
			PANEL1(PANEL_MISC, OptionMisc, pProfile_);
			PANEL1(PANEL_MISC2, OptionMisc2, pProfile_);
#ifndef _WIN32_WCE
			PANEL1(PANEL_JUNK, OptionJunk, pDocument_->getJunkFilter());
#endif
			PANEL2(PANEL_SECURITY, OptionSecurity, pDocument_->getSecurity(), pProfile_);
		END_PANEL()
	}
	
	HWND hwndSelector = getDlgItem(IDC_SELECTOR);
#ifndef _WIN32_WCE_PSPC
	HTREEITEM hItem = TreeView_GetRoot(hwndSelector);
	while (hItem) {
		TVITEM item = {
			TVIF_HANDLE | TVIF_PARAM,
			hItem
		};
		TreeView_GetItem(hwndSelector, &item);
		if (item.lParam == panel) {
			TreeView_SelectItem(hwndSelector, hItem);
			break;
		}
		
		hItem = TreeView_GetNextSibling(hwndSelector, hItem);
	}
#else
	int nCount = Window(hwndSelector).sendMessage(CB_GETCOUNT);
	for (int n = 0; n < nCount; ++n) {
		Panel p = static_cast<Panel>(Window(hwndSelector).sendMessage(CB_GETITEMDATA, n));
		if (p == panel) {
			Window(hwndSelector).sendMessage(CB_SETCURSEL, n);
			break;
		}
	}
#endif
	
	if (pCurrentPanel_)
		Window(pCurrentPanel_->getWindow()).showWindow(SW_HIDE);
	panel_ = panel;
	pCurrentPanel_ = listPanel_[panel_];
	Window(pCurrentPanel_->getWindow()).showWindow();
	layout();
}

bool qm::OptionDialog::processDialogMessage(const MSG& msg)
{
	if (msg.message == WM_KEYDOWN) {
		int nKey = msg.wParam;
		if (nKey == VK_TAB) {
			bool bShift = ::GetKeyState(VK_SHIFT) < 0;
			processTab(bShift);
			return true;
		}
		else if (nKey == VK_RETURN) {
			int nDefId = 0;
			if (pCurrentPanel_)
				nDefId = Window(pCurrentPanel_->getWindow()).sendMessage(DM_GETDEFID);
			if (HIWORD(nDefId) == DC_HASDEFID && LOWORD(nDefId) != IDOK && LOWORD(nDefId) != IDCANCEL)
				Window(pCurrentPanel_->getWindow()).postMessage(WM_COMMAND, MAKEWPARAM(LOWORD(nDefId), 0), 0);
			else
				postMessage(WM_COMMAND, MAKEWPARAM(IDOK, 0), 0);
			return true;
		}
		else if (nKey == VK_ESCAPE) {
			postMessage(WM_COMMAND, MAKEWPARAM(IDCANCEL, 0), 0);
			return true;
		}
		else if (nKey == VK_LEFT || nKey == VK_RIGHT || nKey == VK_UP || nKey == VK_DOWN) {
			HWND hwnd = getFocus();
			if (hwnd && Window(hwnd).sendMessage(WM_GETDLGCODE) & DLGC_BUTTON)
				processTab(nKey == VK_LEFT || nKey == VK_UP);
		}
	}
	else if (msg.message == WM_SYSKEYDOWN) {
		int nKey = msg.wParam;
		if ((('A' <= nKey && nKey <= 'Z') || ('0' <= nKey && nKey <= '9')) &&
			::GetKeyState(VK_MENU) < 0) {
			processMnemonic(static_cast<char>(nKey));
			return true;
		}
	}
	
	return false;
}

void qm::OptionDialog::processTab(bool bShift)
{
	HWND hwnd = getFocus();
	if (!hwnd)
		return;
	
	UINT nId = Window(hwnd).getWindowLong(GWL_ID);
	if (nId == IDC_SELECTOR) {
		if (bShift) {
#ifndef _WIN32_WCE
			hwnd = getDlgItem(IDCANCEL);
#else
			HWND hwndChild = Window(pCurrentPanel_->getWindow()).getWindow(GW_CHILD);
			if (hwndChild)
				hwndChild = Window(hwndChild).getWindow(GW_HWNDLAST);
			while (hwndChild) {
				if (isTabStop(hwndChild)) {
					hwnd = hwndChild;
					break;
				}
				hwndChild = Window(hwndChild).getWindow(GW_HWNDPREV);
			}
#endif
		}
		else {
			HWND hwndChild = Window(pCurrentPanel_->getWindow()).getWindow(GW_CHILD);
			while (hwndChild) {
				if (isTabStop(hwndChild)) {
					hwnd = hwndChild;
					break;
				}
				hwndChild = Window(hwndChild).getWindow(GW_HWNDNEXT);
			}
		}
	}
#ifndef _WIN32_WCE
	else if (nId == IDOK) {
		if (bShift) {
			HWND hwndChild = Window(pCurrentPanel_->getWindow()).getWindow(GW_CHILD);
			if (hwndChild)
				hwndChild = Window(hwndChild).getWindow(GW_HWNDLAST);
			while (hwndChild) {
				if (isTabStop(hwndChild)) {
					hwnd = hwndChild;
					break;
				}
				hwndChild = Window(hwndChild).getWindow(GW_HWNDPREV);
			}
		}
		else {
			hwnd = getDlgItem(IDCANCEL);
		}
	}
	else if (nId == IDCANCEL) {
		hwnd = getDlgItem(bShift ? IDOK : IDC_SELECTOR);
	}
#endif
	else if (pCurrentPanel_ && Window(hwnd).getParent() == pCurrentPanel_->getWindow()) {
		HWND hwndSibling = Window(hwnd).getWindow(bShift ? GW_HWNDPREV : GW_HWNDNEXT);
		while (hwndSibling) {
			if (isTabStop(hwndSibling)) {
				hwnd = hwndSibling;
				break;
			}
			hwndSibling = Window(hwndSibling).getWindow(bShift ? GW_HWNDPREV : GW_HWNDNEXT);
		}
		if (!hwndSibling) {
#ifndef _WIN32_WCE
			hwnd = getDlgItem(bShift ? IDC_SELECTOR : IDOK);
#else
			hwnd = getDlgItem(IDC_SELECTOR);
#endif
		}
	}
	
	setFocus(hwnd);
}

bool qm::OptionDialog::isTabStop(HWND hwnd) const
{
	int nStyle = Window(hwnd).getStyle();
	return nStyle & WS_TABSTOP && nStyle & WS_VISIBLE && !(nStyle & WS_DISABLED);
}

void qm::OptionDialog::processMnemonic(char c)
{
	HWND hwnd = Window(pCurrentPanel_->getWindow()).getWindow(GW_CHILD);
	while (hwnd) {
		if (getMnemonic(hwnd) == c)
			break;
		hwnd = Window(hwnd).getWindow(GW_HWNDNEXT);
	}
	if (hwnd) {
		if (Window(hwnd).sendMessage(WM_GETDLGCODE) & DLGC_STATIC)
			hwnd = Window(hwnd).getWindow(GW_HWNDNEXT);
		if (hwnd) {
			int nCode = Window(hwnd).sendMessage(WM_GETDLGCODE);
			if (nCode & DLGC_DEFPUSHBUTTON || nCode & DLGC_UNDEFPUSHBUTTON)
				Window(pCurrentPanel_->getWindow()).postMessage(WM_COMMAND,
					MAKEWPARAM(Window(hwnd).getWindowLong(GWL_ID), BN_CLICKED),
					reinterpret_cast<LPARAM>(hwnd));
			else if (nCode & DLGC_BUTTON)
				Window(hwnd).sendMessage(BM_CLICK);
			else
				setFocus(hwnd);
		}
	}
}

void qm::OptionDialog::setFocus(HWND hwnd)
{
	Window wnd(hwnd);
	
	int nCode = wnd.sendMessage(WM_GETDLGCODE);
	if (nCode & DLGC_DEFPUSHBUTTON || nCode & DLGC_UNDEFPUSHBUTTON) {
		Window parent(wnd.getParent());
		parent.sendMessage(DM_SETDEFID, wnd.getWindowLong(GWL_ID));
		
		if (parent.getHandle() == getHandle())
			clearDefaultButton(pCurrentPanel_->getWindow());
		else
			clearDefaultButton(getHandle());
	}
	else {
		sendMessage(DM_SETDEFID, IDOK);
		clearDefaultButton(pCurrentPanel_->getWindow());
	}
	
	wnd.setFocus();
	
	if (nCode & DLGC_HASSETSEL)
		wnd.sendMessage(EM_SETSEL, 0, -1);
}

WCHAR qm::OptionDialog::getMnemonic(HWND hwnd)
{
	wstring_ptr wstrText(Window(hwnd).getWindowText());
	const WCHAR* p = wstrText.get();
	while (true) {
		p = wcschr(p, L'&');
		if (!p)
			return L'\0';
		if (*(p + 1) != L'&')
			return getMnemonic(*(p + 1));
		++p;
	}
	return L'\0';
}

WCHAR qm::OptionDialog::getMnemonic(WCHAR c)
{
	return (L'a' <= c && c <= L'z') ? c - L'a' + L'A' : c;
}

void qm::OptionDialog::clearDefaultButton(HWND hwnd)
{
	Window wnd(hwnd);
	int nDefId = wnd.sendMessage(DM_GETDEFID);
	if (HIWORD(nDefId) == DC_HASDEFID) {
		wnd.sendDlgItemMessage(LOWORD(nDefId), BM_SETSTYLE, BS_PUSHBUTTON, TRUE);
		wnd.sendMessage(DM_SETDEFID, IDOK);
	}
}


/****************************************************************************
 *
 * OptionDialogPanel
 *
 */

qm::OptionDialogPanel::~OptionDialogPanel()
{
}


/****************************************************************************
 *
 * OptionDialogContext
 *
 */

qm::OptionDialogContext::OptionDialogContext() :
	nFlags_(0)
{
}

qm::OptionDialogContext::~OptionDialogContext()
{
}

unsigned int qm::OptionDialogContext::getFlags() const
{
	return nFlags_;
}

void qm::OptionDialogContext::setFlags(unsigned int nFlags)
{
	setFlags(nFlags, nFlags);
}

void qm::OptionDialogContext::setFlags(unsigned int nFlags,
									   unsigned int nMask)
{
	nFlags_ = (nFlags_ & ~nMask) | (nFlags & nMask);
}


/****************************************************************************
 *
 * OptionDialogManager
 *
 */

qm::OptionDialogManager::OptionDialogManager(Document* pDocument,
											 GoRound* pGoRound,
											 FilterManager* pFilterManager,
											 ColorManager* pColorManager,
											 SyncManager* pSyncManager,
											 AutoPilotManager* pAutoPilotManager,
											 Profile* pProfile) :
	pDocument_(pDocument),
	pGoRound_(pGoRound),
	pFilterManager_(pFilterManager),
	pColorManager_(pColorManager),
	pSyncManager_(pSyncManager),
	pAutoPilotManager_(pAutoPilotManager),
	pProfile_(pProfile),
	pMainWindow_(0),
	pFolderWindow_(0),
	pFolderComboBox_(0),
	pListWindow_(0),
	pFolderListWindow_(0),
	pPreviewWindow_(0),
	pMessageFrameWindowManager_(0),
	pEditFrameWindowManager_(0),
#ifdef QMTABWINDOW
	pTabWindow_(0),
#endif
	pAddressBookFrameWindowManager_(0)
{
}

qm::OptionDialogManager::~OptionDialogManager()
{
}

void qm::OptionDialogManager::initUIs(MainWindow* pMainWindow,
									  FolderWindow* pFolderWindow,
									  FolderComboBox* pFolderComboBox,
									  ListWindow* pListWindow,
									  FolderListWindow* pFolderListWindow,
									  MessageWindow* pPreviewWindow,
									  MessageFrameWindowManager* pMessageFrameWindowManager,
									  EditFrameWindowManager* pEditFrameWindowManager,
#ifdef QMTABWINDOW
									  TabWindow* pTabWindow,
#endif
									  AddressBookFrameWindowManager* pAddressBookFrameWindowManager)
{
	pMainWindow_ = pMainWindow;
	pFolderWindow_ = pFolderWindow;
	pFolderComboBox_ = pFolderComboBox;
	pListWindow_ = pListWindow;
	pFolderListWindow_ = pFolderListWindow;
	pPreviewWindow_ = pPreviewWindow;
	pMessageFrameWindowManager_ = pMessageFrameWindowManager;
	pEditFrameWindowManager_ = pEditFrameWindowManager;
#ifdef QMTABWINDOW
	pTabWindow_ = pTabWindow;
#endif
	pAddressBookFrameWindowManager_ = pAddressBookFrameWindowManager;
}

int qm::OptionDialogManager::showDialog(HWND hwndParent,
										OptionDialog::Panel panel) const
{
	assert(pDocument_);
	assert(pGoRound_);
	assert(pFilterManager_);
	assert(pColorManager_);
	assert(pSyncManager_);
	assert(pAutoPilotManager_);
	assert(pProfile_);
	assert(pMainWindow_);
	assert(pFolderWindow_);
	assert(pFolderComboBox_);
	assert(pListWindow_);
	assert(pFolderListWindow_);
	assert(pPreviewWindow_);
	assert(pMessageFrameWindowManager_);
	assert(pEditFrameWindowManager_);
#ifdef QMTABWINDOW
	assert(pTabWindow_);
#endif
	assert(pAddressBookFrameWindowManager_);
	
	OptionDialog dialog(pDocument_, pGoRound_, pFilterManager_,
		pColorManager_, pSyncManager_->getSyncFilterManager(),
		pAutoPilotManager_, pMainWindow_, pFolderWindow_, pFolderComboBox_,
		pListWindow_, pFolderListWindow_, pPreviewWindow_,
		pMessageFrameWindowManager_, pEditFrameWindowManager_,
#ifdef QMTABWINDOW
		pTabWindow_,
#endif
		pAddressBookFrameWindowManager_, pProfile_, panel);
	return dialog.doModal(hwndParent);
}

bool qm::OptionDialogManager::canShowDialog() const
{
	return !pSyncManager_->isSyncing();
}


/****************************************************************************
 *
 * OptionAddressBookDialog
 *
 */

OptionAddressBookDialog::External qm::OptionAddressBookDialog::externals__[] = {
#ifndef _WIN32_WCE
	{ IDC_WAB,				L"WAB"				},
	{ IDC_OUTLOOK,			L"Outlook"			}
#else
	{ IDC_POCKETOUTLOOK,	L"PocketOutlook"	}
#endif
};

qm::OptionAddressBookDialog::OptionAddressBookDialog(AddressBook* pAddressBook,
													 AddressBookFrameWindowManager* pAddressBookFrameWindowManager,
													 Profile* pProfile) :
	DefaultDialog(IDD_OPTIONADDRESSBOOK),
	pAddressBook_(pAddressBook),
	pAddressBookFrameWindowManager_(pAddressBookFrameWindowManager),
	pProfile_(pProfile)
{
	qs::UIUtil::getLogFontFromProfile(pProfile_, L"AddressBookListWindow", false, &lf_);
}

qm::OptionAddressBookDialog::~OptionAddressBookDialog()
{
}

LRESULT qm::OptionAddressBookDialog::onCommand(WORD nCode,
											   WORD nId)
{
	BEGIN_COMMAND_HANDLER()
		HANDLE_COMMAND_ID(IDC_FONT, onFont)
	END_COMMAND_HANDLER()
	return DefaultDialog::onCommand(nCode, nId);
}

LRESULT qm::OptionAddressBookDialog::onInitDialog(HWND hwndFocus,
												  LPARAM lParam)
{
	wstring_ptr wstrExternals(pProfile_->getString(L"AddressBook",
		L"Externals", L"WAB Outlook PocketOutlook"));
	const WCHAR* p = wcstok(wstrExternals.get(), L" ");
	while (p) {
		for (int n = 0; n < countof(externals__); ++n) {
			if (wcscmp(p, externals__[n].pwszName_) == 0) {
				sendDlgItemMessage(externals__[n].nId_, BM_SETCHECK, BST_CHECKED);
				break;
			}
		}
		p = wcstok(0, L" ");
	}
	
	if (pProfile_->getInt(L"AddressBook", L"AddressOnly", 0))
		sendDlgItemMessage(IDC_ADDRESSONLY, BM_SETCHECK, BST_CHECKED);
	
	return FALSE;
}

bool qm::OptionAddressBookDialog::save(OptionDialogContext* pContext)
{
	StringBuffer<WSTRING> buf;
	for (int n = 0; n < countof(externals__); ++n) {
		if (sendDlgItemMessage(externals__[n].nId_, BM_GETCHECK) == BST_CHECKED) {
			if (buf.getLength() != 0)
				buf.append(L" ");
			buf.append(externals__[n].pwszName_);
		}
	}
	pProfile_->setString(L"AddressBook", L"Externals", buf.getCharArray());
	
	bool bAddressOnly = sendDlgItemMessage(IDC_ADDRESSONLY, BM_GETCHECK) == BST_CHECKED;
	pProfile_->setInt(L"AddressBook", L"AddressOnly", bAddressOnly);
	
	qs::UIUtil::setLogFontToProfile(pProfile_, L"AddressBookListWindow", lf_);
	
	pContext->setFlags(OptionDialogContext::FLAG_RELOADADDRESSBOOK);
	
	return true;
}

LRESULT qm::OptionAddressBookDialog::onFont()
{
	qs::UIUtil::browseFont(getParentPopup(), &lf_);
	return 0;
}


/****************************************************************************
 *
 * OptionConfirmDialog
 *
 */

DialogUtil::BoolProperty qm::OptionConfirmDialog::boolProperties__[] = {
	{ L"ConfirmDeleteMessage",			IDC_CONFIRMDELETEMESSAGE,			false	},
	{ L"ConfirmEmptyFolder",			IDC_CONFIRMEMPTYFOLDER,				true	},
	{ L"ConfirmEmptyTrash",				IDC_CONFIRMEMPTYTRASH,				true	}
};

qm::OptionConfirmDialog::OptionConfirmDialog(Profile* pProfile) :
	DefaultDialog(IDD_OPTIONCONFIRM),
	pProfile_(pProfile)
{
}

qm::OptionConfirmDialog::~OptionConfirmDialog()
{
}

LRESULT qm::OptionConfirmDialog::onInitDialog(HWND hwndFocus,
											  LPARAM lParam)
{
	DialogUtil::loadBoolProperties(this, pProfile_,
		L"Global", boolProperties__, countof(boolProperties__));
	
	return FALSE;
}

bool qm::OptionConfirmDialog::save(OptionDialogContext* pContext)
{
	DialogUtil::saveBoolProperties(this, pProfile_,
		L"Global", boolProperties__, countof(boolProperties__));
	
	return true;
}


/****************************************************************************
 *
 * OptionFolderDialog
 *
 */

DialogUtil::BoolProperty qm::OptionFolderDialog::windowBoolProperties__[] = {
	{ L"FolderShowAllCount",		IDC_FOLDERSHOWALL,		true	},
	{ L"FolderShowUnseenCount",		IDC_FOLDERSHOWUNSEEN,	true	},
	{ L"AccountShowAllCount",		IDC_ACCOUNTSHOWALL,		true	},
	{ L"AccountShowUnseenCount",	IDC_ACCOUNTSHOWUNSEEN,	true	},
	{ L"Vertical",					IDC_VERTICAL,			false	}
};

DialogUtil::BoolProperty qm::OptionFolderDialog::comboBoxBoolProperties__[] = {
	{ L"ShowAllCount",		IDC_SHOWALL,	true	},
	{ L"ShowUnseenCount",	IDC_SHOWUNSEEN,	true	}
};

qm::OptionFolderDialog::OptionFolderDialog(FolderWindow* pFolderWindow,
										   FolderComboBox* pFolderComboBox,
										   Profile* pProfile) :
	DefaultDialog(IDD_OPTIONFOLDER),
	pFolderWindow_(pFolderWindow),
	pFolderComboBox_(pFolderComboBox),
	pProfile_(pProfile)
{
	qs::UIUtil::getLogFontFromProfile(pProfile_, L"FolderWindow", false, &lfWindow_);
	qs::UIUtil::getLogFontFromProfile(pProfile_, L"FolderComboBox", false, &lfComboBox_);
}

qm::OptionFolderDialog::~OptionFolderDialog()
{
}

LRESULT qm::OptionFolderDialog::onCommand(WORD nCode,
										  WORD nId)
{
	BEGIN_COMMAND_HANDLER()
		HANDLE_COMMAND_ID(IDC_COMBOBOXFONT, onComboBoxFont)
		HANDLE_COMMAND_ID(IDC_WINDOWFONT, onWindowFont)
	END_COMMAND_HANDLER()
	return DefaultDialog::onCommand(nCode, nId);
}

LRESULT qm::OptionFolderDialog::onInitDialog(HWND hwndFocus,
											 LPARAM lParam)
{
	DialogUtil::loadBoolProperties(this, pProfile_,
		L"FolderWindow", windowBoolProperties__, countof(windowBoolProperties__));
	DialogUtil::loadBoolProperties(this, pProfile_,
		L"FolderComboBox", comboBoxBoolProperties__, countof(comboBoxBoolProperties__));
	
	return FALSE;
}

bool qm::OptionFolderDialog::save(OptionDialogContext* pContext)
{
	DialogUtil::saveBoolProperties(this, pProfile_,
		L"FolderWindow", windowBoolProperties__, countof(windowBoolProperties__));
	qs::UIUtil::setLogFontToProfile(pProfile_, L"FolderWindow", lfWindow_);
	
	DialogUtil::saveBoolProperties(this, pProfile_,
		L"FolderComboBox", comboBoxBoolProperties__, countof(comboBoxBoolProperties__));
	qs::UIUtil::setLogFontToProfile(pProfile_, L"FolderComboBox", lfComboBox_);
	
	pContext->setFlags(OptionDialogContext::FLAG_RELOADFOLDER);
	
	return true;
}

LRESULT qm::OptionFolderDialog::onComboBoxFont()
{
	qs::UIUtil::browseFont(getParentPopup(), &lfComboBox_);
	return 0;
}

LRESULT qm::OptionFolderDialog::onWindowFont()
{
	qs::UIUtil::browseFont(getParentPopup(), &lfWindow_);
	return 0;
}


/****************************************************************************
 *
 * OptionHeaderDialog
 *
 */

qm::OptionHeaderDialog::OptionHeaderDialog(MessageFrameWindowManager* pMessageFrameWindowManager,
										   MessageWindow* pPreviewWindow,
										   Profile* pProfile) :
	DefaultDialog(IDD_OPTIONHEADER),
	pMessageFrameWindowManager_(pMessageFrameWindowManager),
	pPreviewWindow_(pPreviewWindow),
	pProfile_(pProfile)
{
	qs::UIUtil::getLogFontFromProfile(pProfile_, L"HeaderWindow", false, &lf_);
}

qm::OptionHeaderDialog::~OptionHeaderDialog()
{
}

LRESULT qm::OptionHeaderDialog::onCommand(WORD nCode,
										  WORD nId)
{
	BEGIN_COMMAND_HANDLER()
		HANDLE_COMMAND_ID(IDC_FONT, onFont)
	END_COMMAND_HANDLER()
	return DefaultDialog::onCommand(nCode, nId);
}

LRESULT qm::OptionHeaderDialog::onInitDialog(HWND hwndFocus,
											 LPARAM lParam)
{
	return FALSE;
}

bool qm::OptionHeaderDialog::save(OptionDialogContext* pContext)
{
	qs::UIUtil::setLogFontToProfile(pProfile_, L"HeaderWindow", lf_);
	
	pContext->setFlags(OptionDialogContext::FLAG_RELOADMESSAGE |
		OptionDialogContext::FLAG_RELOADPREVIEW |
		OptionDialogContext::FLAG_LAYOUTMAINWINDOW |
		OptionDialogContext::FLAG_LAYOUTMESSAGEWINDOW);
	
	return true;
}

LRESULT qm::OptionHeaderDialog::onFont()
{
	qs::UIUtil::browseFont(getParentPopup(), &lf_);
	return 0;
}


#ifndef _WIN32_WCE
/****************************************************************************
 *
 * OptionJunkDialog
 *
 */

qm::OptionJunkDialog::OptionJunkDialog(JunkFilter* pJunkFilter) :
	DefaultDialog(IDD_OPTIONJUNK),
	pJunkFilter_(pJunkFilter)
{
}

qm::OptionJunkDialog::~OptionJunkDialog()
{
}

LRESULT qm::OptionJunkDialog::onInitDialog(HWND hwndFocus,
										   LPARAM lParam)
{
	if (pJunkFilter_) {
		unsigned int nFlags = pJunkFilter_->getFlags();
		if (nFlags & JunkFilter::FLAG_MANUALLEARN)
			sendDlgItemMessage(IDC_MANUALLEARN, BM_SETCHECK, BST_CHECKED);
		if (nFlags & JunkFilter::FLAG_AUTOLEARN)
			sendDlgItemMessage(IDC_AUTOLEARN, BM_SETCHECK, BST_CHECKED);
		
		float fThreshold = pJunkFilter_->getThresholdScore();
		WCHAR wszThreshold[32];
		swprintf(wszThreshold, L"%.2f", fThreshold);
		setDlgItemText(IDC_THRESHOLD, wszThreshold);
		
		unsigned int nMaxSize = pJunkFilter_->getMaxTextLength();
		setDlgItemInt(IDC_MAXSIZE, nMaxSize);
	}
	else {
		UINT nIds[] = {
			IDC_MANUALLEARN,
			IDC_AUTOLEARN,
			IDC_THRESHOLD,
			IDC_MAXSIZE
		};
		for (int n = 0; n < countof(nIds); ++n)
			Window(getDlgItem(nIds[n])).enableWindow(false);
	}
	
	return FALSE;
}

bool qm::OptionJunkDialog::save(OptionDialogContext* pContext)
{
	if (pJunkFilter_) {
		unsigned int nFlags = 0;
		if (sendDlgItemMessage(IDC_MANUALLEARN, BM_GETCHECK) == BST_CHECKED)
			nFlags |= JunkFilter::FLAG_MANUALLEARN;
		if (sendDlgItemMessage(IDC_AUTOLEARN, BM_GETCHECK) == BST_CHECKED)
			nFlags |= JunkFilter::FLAG_AUTOLEARN;
		pJunkFilter_->setFlags(nFlags, JunkFilter::FLAG_MANUALLEARN | JunkFilter::FLAG_AUTOLEARN);
		
		wstring_ptr wstrThreshold(getDlgItemText(IDC_THRESHOLD));
		WCHAR* pEnd = 0;
		double dThreshold = wcstod(wstrThreshold.get(), &pEnd);
		if (!*pEnd)
			pJunkFilter_->setThresholdScore(static_cast<float>(dThreshold));
		
		unsigned int nMaxSize = getDlgItemInt(IDC_MAXSIZE);
		pJunkFilter_->setMaxTextLength(nMaxSize);
	}
	
	return true;
}
#endif // _WIN32_WCE


/****************************************************************************
 *
 * OptionListDialog
 *
 */

DialogUtil::BoolProperty qm::OptionListDialog::boolProperties__[] = {
#ifdef _WIN32_WCE_PSPC
	{ L"SingleClickOpen",	IDC_SINGLECLICK,	true	}
#else
	{ L"SingleClickOpen",	IDC_SINGLECLICK,	false	}
#endif
};

qm::OptionListDialog::OptionListDialog(ListWindow* pListWindow,
									   FolderListWindow* pFolderListWindow,
									   Profile* pProfile) :
	DefaultDialog(IDD_OPTIONLIST),
	pListWindow_(pListWindow),
	pFolderListWindow_(pFolderListWindow),
	pProfile_(pProfile)
{
	qs::UIUtil::getLogFontFromProfile(pProfile_, L"ListWindow", false, &lf_);
}

qm::OptionListDialog::~OptionListDialog()
{
}

LRESULT qm::OptionListDialog::onCommand(WORD nCode,
										WORD nId)
{
	BEGIN_COMMAND_HANDLER()
		HANDLE_COMMAND_ID(IDC_FONT, onFont)
	END_COMMAND_HANDLER()
	return DefaultDialog::onCommand(nCode, nId);
}

LRESULT qm::OptionListDialog::onInitDialog(HWND hwndFocus,
										   LPARAM lParam)
{
	DialogUtil::loadBoolProperties(this, pProfile_,
		L"ListWindow", boolProperties__, countof(boolProperties__));
	
	return FALSE;
}

bool qm::OptionListDialog::save(OptionDialogContext* pContext)
{
	DialogUtil::saveBoolProperties(this, pProfile_,
		L"ListWindow", boolProperties__, countof(boolProperties__));
	qs::UIUtil::setLogFontToProfile(pProfile_, L"ListWindow", lf_);
	qs::UIUtil::setLogFontToProfile(pProfile_, L"FolderListWindow", lf_);
	
	pContext->setFlags(OptionDialogContext::FLAG_RELOADLIST);
	
	return true;
}

LRESULT qm::OptionListDialog::onFont()
{
	qs::UIUtil::browseFont(getParentPopup(), &lf_);
	return 0;
}


/****************************************************************************
 *
 * OptionMiscDialog
 *
 */

DialogUtil::BoolProperty qm::OptionMiscDialog::boolProperties__[] = {
	{ L"NextUnseenWhenScrollEnd",		IDC_SHOWNEXTUNSEENWHENSCROLLEND,	false	},
	{ L"EmptyTrashOnExit",				IDC_EMPTYTRASHONEXIT,				false	},
	{ L"SaveMessageViewModePerFolder",	IDC_SAVEMESSAGEVIEWMODEPERFOLDER,	true	},
	{ L"SaveOnDeactivate",				IDC_SAVEONDEACTIVATE,				false	},
#ifndef _WIN32_WCE
	{ L"ShowUnseenCountOnWelcome",		IDC_SHOWUNSEENCOUNTONWELCOME,		false	},
#endif
};

qm::OptionMiscDialog::OptionMiscDialog(Profile* pProfile) :
	DefaultDialog(IDD_OPTIONMISC),
	pProfile_(pProfile)
{
}

qm::OptionMiscDialog::~OptionMiscDialog()
{
}

LRESULT qm::OptionMiscDialog::onCommand(WORD nCode,
										WORD nId)
{
	BEGIN_COMMAND_HANDLER()
		HANDLE_COMMAND_ID(IDC_BROWSE, onBrowse)
	END_COMMAND_HANDLER()
	return DefaultDialog::onCommand(nCode, nId);
}

LRESULT qm::OptionMiscDialog::onInitDialog(HWND hwndFocus,
										   LPARAM lParam)
{
	DialogUtil::loadBoolProperties(this, pProfile_,
		L"Global", boolProperties__, countof(boolProperties__));
	
	wstring_ptr wstrTempFolder(pProfile_->getString(L"Global", L"TemporaryFolder", L""));
	setDlgItemText(IDC_TEMPORARYFOLDER, wstrTempFolder.get());
	
	wstring_ptr wstrEncodings(pProfile_->getString(L"Global",
		L"Encodings", L"iso-8859-1 iso-2022-jp shift_jis euc-jp utf-8"));
	setDlgItemText(IDC_ENCODING, wstrEncodings.get());
	
	const WCHAR* pwszLog[] = {
		L"NONE",
		L"FATAL",
		L"ERROR",
		L"WARN",
		L"INFO",
		L"DEBUG"
	};
	for (int n = 0; n < countof(pwszLog); ++n) {
		W2T(pwszLog[n], ptszLog);
		sendDlgItemMessage(IDC_LOG, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(ptszLog));
	}
	int nLog = 0;
	Init& init = Init::getInit();
	if (init.isLogEnabled())
		nLog = init.getLogLevel() + 1;
	sendDlgItemMessage(IDC_LOG, CB_SETCURSEL, nLog);
	
	return FALSE;
}

bool qm::OptionMiscDialog::save(OptionDialogContext* pContext)
{
	DialogUtil::saveBoolProperties(this, pProfile_,
		L"Global", boolProperties__, countof(boolProperties__));
	
	wstring_ptr wstrTempFolder(getDlgItemText(IDC_TEMPORARYFOLDER));
	pProfile_->setString(L"Global", L"TemporaryFolder", wstrTempFolder.get());
	
	wstring_ptr wstrEncodings(getDlgItemText(IDC_ENCODING));
	pProfile_->setString(L"Global", L"Encodings", wstrEncodings.get());
	
	int nLog = sendDlgItemMessage(IDC_LOG, CB_GETCURSEL);
	pProfile_->setInt(L"Global", L"Log", nLog - 1);
	
	Init& init = Init::getInit();
	bool bLogEnabled = init.isLogEnabled();
	Logger::Level logLevel = init.getLogLevel();
	if (nLog != 0)
		init.setLogLevel(static_cast<Logger::Level>(nLog - 1));
	init.setLogEnabled(nLog != 0);
	if (bLogEnabled != init.isLogEnabled() ||
		logLevel != init.getLogLevel())
		InitThread::getInitThread().resetLogger();
	
	return true;
}

LRESULT qm::OptionMiscDialog::onBrowse()
{
	HINSTANCE hInst = Application::getApplication().getResourceHandle();
	wstring_ptr wstrTitle(loadString(hInst, IDS_TEMPORARYFOLDER));
	wstring_ptr wstrPath(getDlgItemText(IDC_TEMPORARYFOLDER));
	wstrPath = qs::UIUtil::browseFolder(getParentPopup(), wstrTitle.get(), wstrPath.get());
	if (wstrPath.get())
		setDlgItemText(IDC_TEMPORARYFOLDER, wstrPath.get());
	
	return 0;
}


/****************************************************************************
 *
 * OptionMisc2Dialog
 *
 */

DialogUtil::BoolProperty qm::OptionMisc2Dialog::boolProperties__[] = {
	{ L"IncrementalSearch",	IDC_INCREMENTALSEARCH,	false	},
	{ L"Bcc",				IDC_BCC,				true	},
	{ L"NoBccForML",		IDC_NOBCCFORML,			false	},
	{ L"ForwardRfc822",		IDC_FORWARDRFC822,		false	},
	{ L"OpenAddressBook",	IDC_OPENADDRESSBOOK,	false	}
};

qm::OptionMisc2Dialog::OptionMisc2Dialog(Profile* pProfile) :
	DefaultDialog(IDD_OPTIONMISC2),
	pProfile_(pProfile)
{
}

qm::OptionMisc2Dialog::~OptionMisc2Dialog()
{
}

LRESULT qm::OptionMisc2Dialog::onInitDialog(HWND hwndFocus,
											LPARAM lParam)
{
	DialogUtil::loadBoolProperties(this, pProfile_,
		L"Global", boolProperties__, countof(boolProperties__));
	
	wstring_ptr wstrQuote(pProfile_->getString(L"Global", L"Quote", L"> "));
	setDlgItemText(IDC_QUOTE, wstrQuote.get());
	
	return FALSE;
}

bool qm::OptionMisc2Dialog::save(OptionDialogContext* pContext)
{
	DialogUtil::saveBoolProperties(this, pProfile_,
		L"Global", boolProperties__, countof(boolProperties__));
	
	wstring_ptr wstrQuote(getDlgItemText(IDC_QUOTE));
	pProfile_->setString(L"Global", L"Quote", wstrQuote.get());
	
	return true;
}


/****************************************************************************
 *
 * OptionSecurityDialog
 *
 */

DialogUtil::BoolProperty qm::OptionSecurityDialog::boolProperties__[] = {
	{ L"LoadSystemStore",	IDC_SYSTEMSTORE,		true	},
};

qm::OptionSecurityDialog::OptionSecurityDialog(Security* pSecurity,
											   Profile* pProfile) :
	DefaultDialog(IDD_OPTIONSECURITY),
	pSecurity_(pSecurity),
	pProfile_(pProfile)
{
}

qm::OptionSecurityDialog::~OptionSecurityDialog()
{
}

LRESULT qm::OptionSecurityDialog::onInitDialog(HWND hwndFocus,
											   LPARAM lParam)
{
	DialogUtil::loadBoolProperties(this, pProfile_,
		L"Security", boolProperties__, countof(boolProperties__));
	
#ifndef _WIN32_WCE
	bool bGPG = pProfile_->getInt(L"PGP", L"UseGPG", 1) != 0;
	sendDlgItemMessage(bGPG ? IDC_GNUPG : IDC_PGP, BM_SETCHECK, BST_CHECKED);
#endif
	
	if (!Security::isSSLEnabled() && !Security::isSMIMEEnabled())
		Window(getDlgItem(IDC_SYSTEMSTORE)).enableWindow(false);
#ifndef _WIN32_WCE
	if (!Security::isPGPEnabled()) {
		Window(getDlgItem(IDC_PGP)).enableWindow(false);
		Window(getDlgItem(IDC_GNUPG)).enableWindow(false);
	}
#endif
	
	wstring_ptr wstrExtensions(pProfile_->getString(L"Global",
		L"WarnExtensions", L"exe com pif bat scr htm html hta vbs js"));
	setDlgItemText(IDC_WARNEXTENSION, wstrExtensions.get());
	
	return FALSE;
}

bool qm::OptionSecurityDialog::save(OptionDialogContext* pContext)
{
	DialogUtil::saveBoolProperties(this, pProfile_,
		L"Security", boolProperties__, countof(boolProperties__));
	
#ifndef _WIN32_WCE
	bool bGPG = sendDlgItemMessage(IDC_GNUPG, BM_GETCHECK) == BST_CHECKED;
	pProfile_->setInt(L"PGP", L"UseGPG", bGPG);
#endif
	
	wstring_ptr wstrExtensions(getDlgItemText(IDC_WARNEXTENSION));
	pProfile_->setString(L"Global", L"WarnExtensions", wstrExtensions.get());
	
	pContext->setFlags(OptionDialogContext::FLAG_RELOADSECURITY);
	
	return true;
}


/****************************************************************************
 *
 * TextColorDialog
 *
 */

qm::TextColorDialog::TextColorDialog(const Data& data) :
	DefaultDialog(IDD_TEXTCOLOR),
	data_(data),
	hbrBackground_(0)
{
}

qm::TextColorDialog::~TextColorDialog()
{
}

const TextColorDialog::Data& qm::TextColorDialog::getData() const
{
	return data_;
}

INT_PTR qm::TextColorDialog::dialogProc(UINT uMsg,
										WPARAM wParam,
										LPARAM lParam)
{
	BEGIN_DIALOG_HANDLER()
		HANDLE_CTLCOLOREDIT()
		HANDLE_CTLCOLORSTATIC()
	END_DIALOG_HANDLER()
	return DefaultDialog::dialogProc(uMsg, wParam, lParam);
}

LRESULT qm::TextColorDialog::onCommand(WORD nCode,
									   WORD nId)
{
	BEGIN_COMMAND_HANDLER()
		HANDLE_COMMAND_ID_RANGE(IDC_CHOOSEFOREGROUND, IDC_CHOOSELINK, onChoose)
		HANDLE_COMMAND_ID_RANGE_CODE(IDC_SYSTEMCOLOR,
			IDC_CUSTOMCOLOR, BN_CLICKED, onColor)
	END_COMMAND_HANDLER()
	return DefaultDialog::onCommand(nCode, nId);
}

LRESULT qm::TextColorDialog::onDestroy()
{
	if (hbrBackground_)
		::DeleteObject(hbrBackground_);
	
	return DefaultDialog::onDestroy();
}

LRESULT qm::TextColorDialog::onInitDialog(HWND hwndFocus,
										  LPARAM lParam)
{
	sendDlgItemMessage(data_.bSystemColor_ ? IDC_SYSTEMCOLOR : IDC_CUSTOMCOLOR,
		BM_SETCHECK, BST_CHECKED);
	setDlgItemText(IDC_COLOR, L"Sample");
	setDlgItemText(IDC_QUOTE1, data_.wstrQuote_[0].get());
	setDlgItemText(IDC_QUOTE2, data_.wstrQuote_[1].get());
	setDlgItemText(IDC_LINK, L"http://example.com/");
	
	init(false);
	updateBackgroundBrush();
	updateState();
	
	return TRUE;
}

LRESULT qm::TextColorDialog::onCtlColorEdit(HDC hdc,
											HWND hwnd)
{
	return onCtlColorStatic(hdc, hwnd);
}

LRESULT qm::TextColorDialog::onCtlColorStatic(HDC hdc,
											  HWND hwnd)
{
	bool bSystemColor = sendDlgItemMessage(IDC_SYSTEMCOLOR, BM_GETCHECK) == BST_CHECKED;
	
	COLORREF crForeground;
	switch (Window(hwnd).getWindowLong(GWL_ID)) {
	case IDC_COLOR:
		crForeground = bSystemColor ? ::GetSysColor(COLOR_WINDOWTEXT) : data_.crForeground_;
		break;
	case IDC_QUOTE1:
		crForeground = data_.crQuote_[0];
		break;
	case IDC_QUOTE2:
		crForeground = data_.crQuote_[1];
		break;
	case IDC_LINK:
		crForeground = data_.crLink_;
		break;
	default:
		return DefaultDialog::onCtlColorStatic(hdc, hwnd);
	}
	
	COLORREF crBackground = bSystemColor ? ::GetSysColor(COLOR_WINDOW) : data_.crBackground_;
	
	DeviceContext dc(hdc);
	dc.setTextColor(crForeground);
	dc.setBkColor(crBackground);
	return reinterpret_cast<LRESULT>(hbrBackground_);
}

LRESULT qm::TextColorDialog::onOk()
{
	data_.bSystemColor_ = sendDlgItemMessage(IDC_SYSTEMCOLOR, BM_GETCHECK) == BST_CHECKED;
	
	return DefaultDialog::onOk();
}

LRESULT qm::TextColorDialog::onChoose(UINT nId)
{
	COLORREF* pcrs[] = {
		&data_.crForeground_,
		&data_.crBackground_,
		&data_.crQuote_[0],
		&data_.crQuote_[1],
		&data_.crLink_
	};
	COLORREF& cr = *pcrs[nId - IDC_CHOOSEFOREGROUND];
	
	COLORREF crCustom[16];
	CHOOSECOLOR cc = {
		sizeof(cc),
		getHandle(),
		0,
		cr,
		crCustom,
		CC_ANYCOLOR | CC_RGBINIT,
		0,
		0,
		0
	};
	if (::ChooseColor(&cc)) {
		cr = cc.rgbResult;
		
		if (nId == IDC_CHOOSEBACKGROUND)
			updateBackgroundBrush();
		
		invalidate();
	}
	
	return 0;
	
}

LRESULT qm::TextColorDialog::onColor(UINT nId)
{
	updateBackgroundBrush();
	updateState();
	return 0;
}

void qm::TextColorDialog::updateState()
{
	bool bEnable = sendDlgItemMessage(IDC_CUSTOMCOLOR, BM_GETCHECK) == BST_CHECKED;
	Window(getDlgItem(IDC_CHOOSEFOREGROUND)).enableWindow(bEnable);
	Window(getDlgItem(IDC_CHOOSEBACKGROUND)).enableWindow(bEnable);
}

void qm::TextColorDialog::updateBackgroundBrush()
{
	if (hbrBackground_)
		::DeleteObject(hbrBackground_);
	
	bool bSystemColor = sendDlgItemMessage(IDC_SYSTEMCOLOR, BM_GETCHECK) == BST_CHECKED;
	COLORREF crBackground = bSystemColor ? ::GetSysColor(COLOR_WINDOW) : data_.crBackground_;
	hbrBackground_ = ::CreateSolidBrush(crBackground);
	
	invalidate();
}


/****************************************************************************
 *
 * TextColorDialog::Data
 *
 */

qm::TextColorDialog::Data::Data(Profile* pProfile,
								const WCHAR* pwszSection)
{
	bSystemColor_ = pProfile->getInt(pwszSection, L"UseSystemColor", 1) != 0;
	
	struct
	{
		const WCHAR* pwszKey_;
		const WCHAR* pwszDefault_;
		COLORREF* pcr_;
	} colors[] = {
		{ L"ForegroundColor",	L"000000",	&crForeground_	},
		{ L"BackgroundColor",	L"ffffff",	&crBackground_	},
		{ L"QuoteColor1",		L"008000",	&crQuote_[0]	},
		{ L"QuoteColor2",		L"000080",	&crQuote_[1]	},
		{ L"LinkColor",			L"0000ff",	&crLink_		}
	};
	for (int n = 0; n < countof(colors); ++n) {
		wstring_ptr wstr(pProfile->getString(pwszSection,
			colors[n].pwszKey_, colors[n].pwszDefault_));
		Color color(wstr.get());
		if (color.getColor() != 0xffffffff)
			*colors[n].pcr_ = color.getColor();
	}
	
	struct
	{
		const WCHAR* pwszKey_;
		const WCHAR* pwszDefault_;
		wstring_ptr* pwstrValue_;
	} strings[] = {
		{ L"Quote1",	L">",	&wstrQuote_[0]	},
		{ L"Quote2",	L"#",	&wstrQuote_[1]	}
	};
	for (int n = 0; n < countof(strings); ++n)
		*strings[n].pwstrValue_ = pProfile->getString(pwszSection,
			strings[n].pwszKey_, strings[n].pwszDefault_);
}

qm::TextColorDialog::Data::Data(const Data& data) :
	bSystemColor_(data.bSystemColor_),
	crForeground_(data.crForeground_),
	crBackground_(data.crBackground_),
	crLink_(data.crLink_)
{
	for (int n = 0; n < countof(crQuote_); ++n) {
		wstrQuote_[n] = allocWString(data.wstrQuote_[n].get());
		crQuote_[n] = data.crQuote_[n];
	}
}

qm::TextColorDialog::Data::~Data()
{
}

TextColorDialog::Data& qm::TextColorDialog::Data::operator=(const Data& data)
{
	if (&data != this) {
		bSystemColor_= data.bSystemColor_;
		crForeground_ = data.crForeground_;
		crBackground_ = data.crBackground_;
		crLink_ = data.crLink_;
		for (int n = 0; n < countof(crQuote_); ++n) {
			wstrQuote_[n] = allocWString(data.wstrQuote_[n].get());
			crQuote_[n] = data.crQuote_[n];
		}
	}
	return *this;
}

void qm::TextColorDialog::Data::save(Profile* pProfile,
									 const WCHAR* pwszSection) const
{
	pProfile->setInt(pwszSection, L"UseSystemColor", bSystemColor_);
	
	struct
	{
		const WCHAR* pwszKey_;
		COLORREF cr_;
	} colors[] = {
		{ L"ForegroundColor",	crForeground_	},
		{ L"BackgroundColor",	crBackground_	},
		{ L"QuoteColor1",		crQuote_[0]		},
		{ L"QuoteColor2",		crQuote_[1]		},
		{ L"LinkColor",			crLink_			}
	};
	for (int n = 0; n < countof(colors); ++n) {
		wstring_ptr wstrColor(Color(colors[n].cr_).getString());
		pProfile->setString(pwszSection, colors[n].pwszKey_, wstrColor.get());
	}
	
	struct
	{
		const WCHAR* pwszKey_;
		const WCHAR* pwszValue_;
	} strings[] = {
		{ L"Quote1",	wstrQuote_[0].get()	},
		{ L"Quote2",	wstrQuote_[1].get()	}
	};
	for (int n = 0; n < countof(strings); ++n)
		pProfile->setString(pwszSection, strings[n].pwszKey_, strings[n].pwszValue_);
}


/****************************************************************************
 *
 * AbstractOptionTextDialog
 *
 */

DialogUtil::BoolProperty qm::AbstractOptionTextDialog::boolProperties__[] = {
	{ L"WordWrap",					IDC_WORDWRAP,					false	},
	{ L"ShowRuler",					IDC_SHOWRULER,					false	},
	{ L"ShowVerticalScrollBar",		IDC_SHOWVERTICALSCROLLBAR,		true	},
	{ L"ShowHorizontalScrollBar",	IDC_SHOWHORIZONTALSCROLLBAR,	false	}
};

DialogUtil::IntProperty qm::AbstractOptionTextDialog::intProperties__[] = {
	{ L"TabWidth",	IDC_TABWIDTH,	4	}
};

qm::AbstractOptionTextDialog::AbstractOptionTextDialog(UINT nId,
													   Profile* pProfile,
													   const WCHAR* pwszSection) :
	DefaultDialog(nId),
	pProfile_(pProfile),
	pwszSection_(pwszSection),
	color_(pProfile, pwszSection)
{
	qs::UIUtil::getLogFontFromProfile(pProfile_, pwszSection_, false, &lf_);
}

qm::AbstractOptionTextDialog::~AbstractOptionTextDialog()
{
}

LRESULT qm::AbstractOptionTextDialog::onCommand(WORD nCode,
												WORD nId)
{
	BEGIN_COMMAND_HANDLER()
		HANDLE_COMMAND_ID(IDC_COLORS, onColors)
		HANDLE_COMMAND_ID(IDC_FONT, onFont)
		HANDLE_COMMAND_ID_RANGE_CODE(IDC_WRAPWINDOWWIDTH,
			IDC_WRAPCOLUMN, BN_CLICKED, onWrapChange)
	END_COMMAND_HANDLER()
	return DefaultDialog::onCommand(nCode, nId);
}

LRESULT qm::AbstractOptionTextDialog::onInitDialog(HWND hwndFocus,
												   LPARAM lParam)
{
	unsigned int nCharInLine = pProfile_->getInt(pwszSection_, L"CharInLine", 0);
	if (nCharInLine == 0) {
		sendDlgItemMessage(IDC_WRAPWINDOWWIDTH, BM_SETCHECK, BST_CHECKED);
		setDlgItemInt(IDC_CHARINLINE, 80);
	}
	else {
		sendDlgItemMessage(IDC_WRAPCOLUMN, BM_SETCHECK, BST_CHECKED);
		setDlgItemInt(IDC_CHARINLINE, nCharInLine, false);
	}
	
	DialogUtil::loadBoolProperties(this, pProfile_,
		pwszSection_, boolProperties__, countof(boolProperties__));
	DialogUtil::loadIntProperties(this, pProfile_,
		pwszSection_, intProperties__, countof(intProperties__));
	
	updateState();
	
	return FALSE;
}

bool qm::AbstractOptionTextDialog::save(OptionDialogContext* pContext)
{
	unsigned int nCharInLine = 0;
	if (sendDlgItemMessage(IDC_WRAPCOLUMN, BM_GETCHECK) == BST_CHECKED)
		nCharInLine = getDlgItemInt(IDC_CHARINLINE);
	pProfile_->setInt(pwszSection_, L"CharInLine", nCharInLine);
	
	DialogUtil::saveBoolProperties(this, pProfile_,
		pwszSection_, boolProperties__, countof(boolProperties__));
	DialogUtil::saveIntProperties(this, pProfile_,
		pwszSection_, intProperties__, countof(intProperties__));
	
	qs::UIUtil::setLogFontToProfile(pProfile_, pwszSection_, lf_);
	color_.save(pProfile_, pwszSection_);
	
	return true;
}

void qm::AbstractOptionTextDialog::updateState()
{
	bool bEnable = sendDlgItemMessage(IDC_WRAPCOLUMN, BM_GETCHECK) == BST_CHECKED;
	Window(getDlgItem(IDC_CHARINLINE)).enableWindow(bEnable);
}

LRESULT qm::AbstractOptionTextDialog::onColors()
{
	TextColorDialog dialog(color_);
	if (dialog.doModal(getParentPopup()) == IDOK)
		color_ = dialog.getData();
	return 0;
}

LRESULT qm::AbstractOptionTextDialog::onFont()
{
	qs::UIUtil::browseFont(getParentPopup(), &lf_);
	return 0;
}

LRESULT qm::AbstractOptionTextDialog::onWrapChange(UINT nId)
{
	updateState();
	return 0;
}


/****************************************************************************
 *
 * OptionEditDialog
 *
 */

DialogUtil::BoolProperty qm::OptionEditDialog::boolProperties__[] = {
	{ L"ShowTab",				IDC_SHOWTAB,				true	},
	{ L"ShowNewLine",			IDC_SHOWNEWLINE,			true	},
#ifdef _WIN32_WCE
	{ L"HideHeaderIfNoFocus",	IDC_HIDEHEADERIFNOFOCUS,	true	}
#else
	{ L"HideHeaderIfNoFocus",	IDC_HIDEHEADERIFNOFOCUS,	false	}
#endif
};

qm::OptionEditDialog::OptionEditDialog(EditFrameWindowManager* pEditFrameWindowManager,
									   Profile* pProfile) :
	AbstractOptionTextDialog(IDD_OPTIONEDIT, pProfile, L"EditWindow"),
	pEditFrameWindowManager_(pEditFrameWindowManager),
	pProfile_(pProfile)
{
	qs::UIUtil::getLogFontFromProfile(pProfile_, L"HeaderEditWindow", false, &lfHeader_);
}

qm::OptionEditDialog::~OptionEditDialog()
{
}

LRESULT qm::OptionEditDialog::onCommand(WORD nCode,
										WORD nId)
{
	BEGIN_COMMAND_HANDLER()
		HANDLE_COMMAND_ID(IDC_HEADERFONT, onHeaderFont)
	END_COMMAND_HANDLER()
	return AbstractOptionTextDialog::onCommand(nCode, nId);
}

LRESULT qm::OptionEditDialog::onInitDialog(HWND hwndFocus,
										   LPARAM lParam)
{
	DialogUtil::loadBoolProperties(this, pProfile_,
		L"EditWindow", boolProperties__, countof(boolProperties__));
	
	return AbstractOptionTextDialog::onInitDialog(hwndFocus, lParam);
}

bool qm::OptionEditDialog::save(OptionDialogContext* pContext)
{
	if (!AbstractOptionTextDialog::save(pContext))
		return false;
	
	DialogUtil::saveBoolProperties(this, pProfile_,
		L"EditWindow", boolProperties__, countof(boolProperties__));
	qs::UIUtil::setLogFontToProfile(pProfile_, L"HeaderEditWindow", lfHeader_);
	
	pContext->setFlags(OptionDialogContext::FLAG_RELOADEDIT |
		OptionDialogContext::FLAG_LAYOUTEDITWINDOW);
	
	return true;
}

LRESULT qm::OptionEditDialog::onHeaderFont()
{
	qs::UIUtil::browseFont(getParentPopup(), &lfHeader_);
	return 0;
}


/****************************************************************************
 *
 * SecurityDialog
 *
 */

SecurityDialog::Item qm::SecurityDialog::items__[] = {
	{ IDC_SMIMEENCRYPT,			MESSAGESECURITY_SMIMEENCRYPT			},
	{ IDC_SMIMESIGN,			MESSAGESECURITY_SMIMESIGN				},
	{ IDC_SMIMEMULTIPARTSIGNED,	MESSAGESECURITY_SMIMEMULTIPARTSIGNED	},
	{ IDC_SMIMEENCRYPTFORSELF,	MESSAGESECURITY_SMIMEENCRYPTFORSELF		},
#ifndef _WIN32_WCE
	{ IDC_PGPENCRYPT,			MESSAGESECURITY_PGPENCRYPT				},
	{ IDC_PGPSIGN,				MESSAGESECURITY_PGPSIGN					},
	{ IDC_PGPMIME,				MESSAGESECURITY_PGPMIME					}
#endif
};

qm::SecurityDialog::SecurityDialog(unsigned int nMessageSecurity) :
	DefaultDialog(IDD_SECURITY),
	nMessageSecurity_(nMessageSecurity)
{
}

qm::SecurityDialog::~SecurityDialog()
{
}

unsigned int qm::SecurityDialog::getMessageSecurity() const
{
	return nMessageSecurity_;
}

LRESULT qm::SecurityDialog::onInitDialog(HWND hwndFocus,
										 LPARAM lParam)
{
	for (int n = 0; n < countof(items__); ++n) {
		if (nMessageSecurity_ & items__[n].security_)
			sendDlgItemMessage(items__[n].nId_, BM_SETCHECK, BST_CHECKED);
	}
	
	if (!Security::isSMIMEEnabled()) {
		for (UINT n = IDC_SMIMEENCRYPT; n <= IDC_SMIMEENCRYPTFORSELF; ++n)
			Window(getDlgItem(n)).enableWindow(false);
	}
	
#ifndef _WIN32_WCE
	if (!Security::isPGPEnabled()) {
		for (UINT n = IDC_PGPENCRYPT; n <= IDC_PGPMIME; ++n)
			Window(getDlgItem(n)).enableWindow(false);
	}
#endif
	
	init(false);
	
	return TRUE;
}

LRESULT qm::SecurityDialog::onOk()
{
	unsigned int nMessageSecurity = 0;
	for (int n = 0; n < countof(items__); ++n) {
		if (sendDlgItemMessage(items__[n].nId_, BM_GETCHECK) == BST_CHECKED)
			nMessageSecurity |= items__[n].security_;
	}
	nMessageSecurity_ = nMessageSecurity;
	
	return DefaultDialog::onOk();
}


/****************************************************************************
 *
 * OptionEdit2Dialog
 *
 */

DialogUtil::BoolProperty qm::OptionEdit2Dialog::globalBoolProperties__[] = {
	{ L"UseExternalEditor",			IDC_USEEXTERNALEDITOR,		false	},
	{ L"ExternalEditorAutoCreate",	IDC_AUTOCREATE,				true	}
};

DialogUtil::BoolProperty qm::OptionEdit2Dialog::editBoolProperties__[] = {
	{ L"AutoReform",			IDC_AUTOREFORM,			true	},
#ifdef QMZIP
	{ L"ArchiveAttachments",	IDC_ARCHIVEATTACHMENTS,	false	}
#endif
};

DialogUtil::IntProperty qm::OptionEdit2Dialog::intProperties__[] = {
	{ L"ReformLineLength",	IDC_COLUMN,	74	}
};

qm::OptionEdit2Dialog::OptionEdit2Dialog(EditFrameWindowManager* pEditFrameWindowManager,
										 Profile* pProfile) :
	DefaultDialog(IDD_OPTIONEDIT2),
	pEditFrameWindowManager_(pEditFrameWindowManager),
	pProfile_(pProfile),
	nMessageSecurity_(0)
{
	nMessageSecurity_ = pProfile_->getInt(L"Security",
		L"DefaultMessageSecurity", MESSAGESECURITY_PGPMIME);
}

qm::OptionEdit2Dialog::~OptionEdit2Dialog()
{
}

LRESULT qm::OptionEdit2Dialog::onCommand(WORD nCode,
										 WORD nId)
{
	BEGIN_COMMAND_HANDLER()
		HANDLE_COMMAND_ID(IDC_BROWSE, onBrowse)
		HANDLE_COMMAND_ID(IDC_SECURITY, onSecurity)
	END_COMMAND_HANDLER()
	return DefaultDialog::onCommand(nCode, nId);
}

LRESULT qm::OptionEdit2Dialog::onInitDialog(HWND hwndFocus,
											LPARAM lParam)
{
	DialogUtil::loadBoolProperties(this, pProfile_,
		L"Global", globalBoolProperties__, countof(globalBoolProperties__));
	DialogUtil::loadBoolProperties(this, pProfile_,
		L"EditWindow", editBoolProperties__, countof(editBoolProperties__));
	DialogUtil::loadIntProperties(this, pProfile_,
		L"EditWindow", intProperties__, countof(intProperties__));
	
	wstring_ptr wstrEditor(pProfile_->getString(L"Global", L"Editor", L"notepad.exe"));
	setDlgItemText(IDC_EDITOR, wstrEditor.get());
	
	return FALSE;
}

bool qm::OptionEdit2Dialog::save(OptionDialogContext* pContext)
{
	DialogUtil::saveBoolProperties(this, pProfile_,
		L"Global", globalBoolProperties__, countof(globalBoolProperties__));
	DialogUtil::saveBoolProperties(this, pProfile_,
		L"EditWindow", editBoolProperties__, countof(editBoolProperties__));
	DialogUtil::saveIntProperties(this, pProfile_,
		L"EditWindow", intProperties__, countof(intProperties__));
	
	wstring_ptr wstrEditor(getDlgItemText(IDC_EDITOR));
	pProfile_->setString(L"Global", L"Editor", wstrEditor.get());
	
	pProfile_->setInt(L"Security", L"DefaultMessageSecurity", nMessageSecurity_);
	
	return true;
}

LRESULT qm::OptionEdit2Dialog::onBrowse()
{
	HINSTANCE hInst = Application::getApplication().getResourceHandle();
	wstring_ptr wstrFilter(loadString(hInst, IDS_FILTER_EXECUTABLE));
	
	FileDialog dialog(true, wstrFilter.get(), 0, 0, 0,
		OFN_EXPLORER | OFN_HIDEREADONLY | OFN_LONGNAMES);
	if (dialog.doModal(getHandle()) == IDOK)
		setDlgItemText(IDC_EDITOR, dialog.getPath());
	
	return 0;
}

LRESULT qm::OptionEdit2Dialog::onSecurity()
{
	SecurityDialog dialog(nMessageSecurity_);
	if (dialog.doModal(getParentPopup()) == IDOK)
		nMessageSecurity_ = dialog.getMessageSecurity();
	return 0;
}


/****************************************************************************
 *
 * OptionMessageDialog
 *
 */

qm::OptionMessageDialog::OptionMessageDialog(MessageFrameWindowManager* pMessageFrameWindowManager,
											 Profile* pProfile) :
	AbstractOptionTextDialog(IDD_OPTIONMESSAGE, pProfile, L"MessageWindow"),
	pMessageFrameWindowManager_(pMessageFrameWindowManager),
	pProfile_(pProfile)
{
}

qm::OptionMessageDialog::~OptionMessageDialog()
{
}

LRESULT qm::OptionMessageDialog::onInitDialog(HWND hwndFocus,
											  LPARAM lParam)
{
	return AbstractOptionTextDialog::onInitDialog(hwndFocus, lParam);
}

bool qm::OptionMessageDialog::save(OptionDialogContext* pContext)
{
	if (!AbstractOptionTextDialog::save(pContext))
		return false;
	
	pContext->setFlags(OptionDialogContext::FLAG_RELOADMESSAGE);
	
	return true;
}


/****************************************************************************
 *
 * OptionPreviewDialog
 *
 */

DialogUtil::IntProperty qm::OptionPreviewDialog::intProperties__[] = {
	{ L"SeenWait",	IDC_SEENWAIT,	0	}
};

qm::OptionPreviewDialog::OptionPreviewDialog(MessageWindow* pPreviewWindow,
											 Profile* pProfile) :
	AbstractOptionTextDialog(IDD_OPTIONPREVIEW, pProfile, L"PreviewWindow"),
	pPreviewWindow_(pPreviewWindow),
	pProfile_(pProfile)
{
}

qm::OptionPreviewDialog::~OptionPreviewDialog()
{
}

LRESULT qm::OptionPreviewDialog::onInitDialog(HWND hwndFocus,
											  LPARAM lParam)
{
	DialogUtil::loadIntProperties(this, pProfile_,
		L"PreviewWindow", intProperties__, countof(intProperties__));
	
	return AbstractOptionTextDialog::onInitDialog(hwndFocus, lParam);
}

bool qm::OptionPreviewDialog::save(OptionDialogContext* pContext)
{
	if (!AbstractOptionTextDialog::save(pContext))
		return false;
	
	DialogUtil::saveIntProperties(this, pProfile_,
		L"PreviewWindow", intProperties__, countof(intProperties__));
	
	pContext->setFlags(OptionDialogContext::FLAG_RELOADPREVIEW);
	
	return true;
}


#ifdef QMTABWINDOW
/****************************************************************************
 *
 * OptionTabDialog
 *
 */

DialogUtil::BoolProperty qm::OptionTabDialog::boolProperties__[] = {
	{ L"Multiline",			IDC_MULTILINE,	false	},
	{ L"ShowAllCount",		IDC_SHOWALL,	true	},
	{ L"ShowUnseenCount",	IDC_SHOWUNSEEN,	true	}
};

qm::OptionTabDialog::OptionTabDialog(TabWindow* pTabWindow,
									 Profile* pProfile) :
	DefaultDialog(IDD_OPTIONTAB),
	pTabWindow_(pTabWindow),
	pProfile_(pProfile)
{
	qs::UIUtil::getLogFontFromProfile(pProfile_, L"TabWindow", false, &lf_);
}

qm::OptionTabDialog::~OptionTabDialog()
{
}

LRESULT qm::OptionTabDialog::onCommand(WORD nCode,
									   WORD nId)
{
	BEGIN_COMMAND_HANDLER()
		HANDLE_COMMAND_ID(IDC_FONT, onFont)
	END_COMMAND_HANDLER()
	return DefaultDialog::onCommand(nCode, nId);
}

LRESULT qm::OptionTabDialog::onInitDialog(HWND hwndFocus,
										  LPARAM lParam)
{
	DialogUtil::loadBoolProperties(this, pProfile_,
		L"TabWindow", boolProperties__, countof(boolProperties__));
	
	DefaultTabModel* pTabModel = static_cast<DefaultTabModel*>(pTabWindow_->getTabModel());
	unsigned int nReuse = pTabModel->getReuse();
	if (nReuse & DefaultTabModel::REUSE_OPEN)
		sendDlgItemMessage(IDC_REUSEOPEN, BM_SETCHECK, BST_CHECKED);
	if (nReuse & DefaultTabModel::REUSE_CHANGE)
		sendDlgItemMessage(IDC_REUSECHANGE, BM_SETCHECK, BST_CHECKED);
	
	return FALSE;
}

bool qm::OptionTabDialog::save(OptionDialogContext* pContext)
{
	DialogUtil::saveBoolProperties(this, pProfile_,
		L"TabWindow", boolProperties__, countof(boolProperties__));
	
	DefaultTabModel* pTabModel = static_cast<DefaultTabModel*>(pTabWindow_->getTabModel());
	unsigned int nReuse = DefaultTabModel::REUSE_NONE;
	if (sendDlgItemMessage(IDC_REUSEOPEN, BM_GETCHECK) == BST_CHECKED)
		nReuse |= DefaultTabModel::REUSE_OPEN;
	if (sendDlgItemMessage(IDC_REUSECHANGE, BM_GETCHECK) == BST_CHECKED)
		nReuse |= DefaultTabModel::REUSE_CHANGE;
	pTabModel->setReuse(nReuse);
	
	qs::UIUtil::setLogFontToProfile(pProfile_, L"TabWindow", lf_);
	
	pContext->setFlags(OptionDialogContext::FLAG_RELOADTAB);
	
	return true;
}

LRESULT qm::OptionTabDialog::onFont()
{
	qs::UIUtil::browseFont(getParentPopup(), &lf_);
	return 0;
}
#endif // QMTABWINDOW


/****************************************************************************
 *
 * ColorSetsDialog
 *
 */

qm::ColorSetsDialog::ColorSetsDialog(ColorManager* pColorManager,
									 AccountManager* pAccountManager,
									 Profile* pProfile) :
	RuleColorSetsDialog<ColorSet, ColorManager::ColorSetList, ColorManager, ColorsDialog>(
		pColorManager, pAccountManager, pProfile, IDS_COLORSETS,
		&ColorManager::getColorSets, &ColorManager::setColorSets)
{
}


/****************************************************************************
 *
 * ColorsDialog
 *
 */

qm::ColorsDialog::ColorsDialog(ColorSet* pColorSet,
							   AccountManager* pAccountManager,
							   Profile* pProfile) :
	RulesColorsDialog<ColorEntry, ColorSet::ColorList, ColorSet, ColorDialog>(
		pColorSet, pAccountManager, pProfile, IDS_COLORS, &ColorSet::getColors, &ColorSet::setColors)
{
}

const WCHAR* qm::ColorsDialog::getName()
{
	return L"ColorsDialog";
}


/****************************************************************************
 *
 * ColorDialog
 *
 */

qm::ColorDialog::ColorDialog(ColorEntry* pColor,
							 AccountManager* pAccountManager) :
	DefaultDialog(IDD_COLOR),
	pColor_(pColor)
{
}

qm::ColorDialog::~ColorDialog()
{
}

LRESULT qm::ColorDialog::onCommand(WORD nCode,
								   WORD nId)
{
	BEGIN_COMMAND_HANDLER()
		HANDLE_COMMAND_ID(IDC_EDIT, onEdit)
		HANDLE_COMMAND_ID(IDC_CHOOSE, onChoose)
		HANDLE_COMMAND_ID_CODE(IDC_COLOR, EN_CHANGE, onColorChange)
		HANDLE_COMMAND_ID_CODE(IDC_CONDITION, EN_CHANGE, onConditionChange)
	END_COMMAND_HANDLER()
	return DefaultDialog::onCommand(nCode, nId);
}

LRESULT qm::ColorDialog::onInitDialog(HWND hwndFocus,
									  LPARAM lParam)
{
	const Macro* pCondition = pColor_->getCondition();
	if (pCondition) {
		wstring_ptr wstrCondition(pCondition->getString());
		setDlgItemText(IDC_CONDITION, wstrCondition.get());
	}
	
	Color color(pColor_->getColor());
	wstring_ptr wstrColor(color.getString());
	setDlgItemText(IDC_COLOR, wstrColor.get());
	
	init(false);
	updateState();
	
	return TRUE;
}

LRESULT qm::ColorDialog::onOk()
{
	wstring_ptr wstrCondition(getDlgItemText(IDC_CONDITION));
	std::auto_ptr<Macro> pCondition(MacroParser().parse(wstrCondition.get()));
	if (!pCondition.get()) {
		// TODO MSG
		return 0;
	}
	
	wstring_ptr wstrColor(getDlgItemText(IDC_COLOR));
	Color color(wstrColor.get());
	if (color.getColor() == 0xffffffff) {
		// TODO MSG
		return 0;
	}
	
	pColor_->setCondition(pCondition);
	pColor_->setColor(color.getColor());
	
	return DefaultDialog::onOk();
}

LRESULT qm::ColorDialog::onEdit()
{
	wstring_ptr wstrCondition(getDlgItemText(IDC_CONDITION));
	ConditionDialog dialog(wstrCondition.get());
	if (dialog.doModal(getHandle()) == IDOK)
		setDlgItemText(IDC_CONDITION, dialog.getCondition());
	return 0;
}

LRESULT qm::ColorDialog::onChoose()
{
	wstring_ptr wstrColor(getDlgItemText(IDC_COLOR));
	Color color(wstrColor.get());
	COLORREF cr = color.getColor();
	if (cr == 0xffffffff)
		cr = RGB(0, 0, 0);
	
	COLORREF crCustom[16];
	CHOOSECOLOR cc = {
		sizeof(cc),
		getHandle(),
		0,
		cr,
		crCustom,
		CC_ANYCOLOR | CC_RGBINIT,
		0,
		0,
		0
	};
	if (::ChooseColor(&cc)) {
		Color color(cc.rgbResult);
		wstring_ptr wstrColor(color.getString());
		setDlgItemText(IDC_COLOR, wstrColor.get());
	}
	return 0;
}

LRESULT qm::ColorDialog::onConditionChange()
{
	updateState();
	return 0;
}

LRESULT qm::ColorDialog::onColorChange()
{
	updateState();
	return 0;
}

void qm::ColorDialog::updateState()
{
	Window(getDlgItem(IDOK)).enableWindow(
		Window(getDlgItem(IDC_CONDITION)).getWindowTextLength() != 0 &&
		Window(getDlgItem(IDC_COLOR)).getWindowTextLength() != 0);
}


/****************************************************************************
 *
 * RuleSetsDialog
 *
 */

qm::RuleSetsDialog::RuleSetsDialog(RuleManager* pRuleManager,
								   AccountManager* pAccountManager,
								   Profile* pProfile) :
	RuleColorSetsDialog<RuleSet, RuleManager::RuleSetList, RuleManager, RulesDialog>(
		pRuleManager, pAccountManager, pProfile, IDS_RULESETS,
		&RuleManager::getRuleSets, &RuleManager::setRuleSets)
{
}


/****************************************************************************
 *
 * RulesDialog
 *
 */

qm::RulesDialog::RulesDialog(RuleSet* pRuleSet,
							 AccountManager* pAccountManager,
							 Profile* pProfile) :
	RulesColorsDialog<Rule, RuleSet::RuleList, RuleSet, RuleDialog>(
		pRuleSet, pAccountManager, pProfile, IDS_RULES, &RuleSet::getRules, &RuleSet::setRules)
{
}

const WCHAR* qm::RulesDialog::getName()
{
	return L"RulesDialog";
}


/****************************************************************************
 *
 * RuleDialog
 *
 */

qm::RuleDialog::RuleDialog(Rule* pRule,
						   AccountManager* pAccountManager) :
	DefaultDialog(IDD_RULE),
	pRule_(pRule),
	pAccountManager_(pAccountManager),
	bInit_(false)
{
	RuleAction* pAction = pRule_->getAction();
	if (pAction &&
		(pAction->getType() == RuleAction::TYPE_MOVE ||
		pAction->getType() == RuleAction::TYPE_COPY)) {
		CopyRuleAction* pCopyAction = static_cast<CopyRuleAction*>(pAction);
		
		if (pCopyAction->getTemplate())
			wstrTemplate_ = allocWString(pCopyAction->getTemplate());
		
		const CopyRuleAction::ArgumentList& l = pCopyAction->getArguments();
		listArgument_.reserve(l.size());
		for (CopyRuleAction::ArgumentList::const_iterator it = l.begin(); it != l.end(); ++it)
			listArgument_.push_back(CopyRuleAction::ArgumentList::value_type(
				allocWString((*it).first).release(), allocWString((*it).second).release()));
	}
}

qm::RuleDialog::~RuleDialog()
{
	for (CopyRuleAction::ArgumentList::iterator it = listArgument_.begin(); it != listArgument_.end(); ++it) {
		freeWString((*it).first);
		freeWString((*it).second);
	}
}

LRESULT qm::RuleDialog::onCommand(WORD nCode,
								  WORD nId)
{
	BEGIN_COMMAND_HANDLER()
		HANDLE_COMMAND_ID(IDC_EDIT, onEdit)
		HANDLE_COMMAND_ID(IDC_TEMPLATE, onTemplate)
		HANDLE_COMMAND_ID_CODE(IDC_ACCOUNT, CBN_EDITCHANGE, onAccountEditChange)
		HANDLE_COMMAND_ID_CODE(IDC_ACCOUNT, CBN_SELCHANGE, onAccountSelChange)
		HANDLE_COMMAND_ID_CODE(IDC_ACTION, CBN_SELCHANGE, onActionSelChange)
		HANDLE_COMMAND_ID_CODE(IDC_CONDITION, EN_CHANGE, onConditionChange)
		HANDLE_COMMAND_ID_CODE(IDC_FOLDER, CBN_EDITCHANGE, onFolderEditChange)
		HANDLE_COMMAND_ID_CODE(IDC_FOLDER, CBN_SELCHANGE, onFolderSelChange)
		HANDLE_COMMAND_ID_CODE(IDC_MACRO, EN_CHANGE, onMacroChange)
	END_COMMAND_HANDLER()
	return DefaultDialog::onCommand(nCode, nId);
}

LRESULT qm::RuleDialog::onInitDialog(HWND hwndFocus,
									 LPARAM lParam)
{
	const Macro* pCondition = pRule_->getCondition();
	if (pCondition) {
		wstring_ptr wstrCondition(pCondition->getString());
		setDlgItemText(IDC_CONDITION, wstrCondition.get());
	}
	
	const WCHAR* pwszTypes[] = {
		L"None",
		L"Move",
		L"Copy",
		L"Delete",
		L"DeleteCache",
		L"Apply"
	};
	for (int n = 0; n < countof(pwszTypes); ++n) {
		W2T(pwszTypes[n], ptszType);
		sendDlgItemMessage(IDC_ACTION, CB_ADDSTRING,
			0, reinterpret_cast<LPARAM>(ptszType));
	}
	
	const AccountManager::AccountList& listAccount = pAccountManager_->getAccounts();
	for (AccountManager::AccountList::const_iterator it = listAccount.begin(); it != listAccount.end(); ++it) {
		Account* pAccount = *it;
		W2T(pAccount->getName(), ptszName);
		sendDlgItemMessage(IDC_ACCOUNT, CB_ADDSTRING,
			0, reinterpret_cast<LPARAM>(ptszName));
	}
	
	int nItem = 0;
	RuleAction* pAction = pRule_->getAction();
	if (pAction) {
		RuleAction::Type type = pAction->getType();
		switch (type) {
		case RuleAction::TYPE_MOVE:
		case RuleAction::TYPE_COPY:
			{
				nItem = type == RuleAction::TYPE_MOVE ? 1 : 2;
				
				CopyRuleAction* pCopy = static_cast<CopyRuleAction*>(pAction);
				const WCHAR* pwszAccount = pCopy->getAccount();
				if (pwszAccount)
					setDlgItemText(IDC_ACCOUNT, pwszAccount);
				setDlgItemText(IDC_FOLDER, pCopy->getFolder());
			}
			break;
		case RuleAction::TYPE_DELETE:
			{
				nItem = 3;
				
				bool bDirect = static_cast<DeleteRuleAction*>(pAction)->isDirect();
				sendDlgItemMessage(IDC_DIRECT, BM_SETCHECK,
					bDirect ? BST_CHECKED : BST_UNCHECKED);
			}
			break;
		case RuleAction::TYPE_DELETECACHE:
			nItem = 4;
			break;
		case RuleAction::TYPE_APPLY:
			{
				nItem = 5;
				
				wstring_ptr wstrMacro(static_cast<ApplyRuleAction*>(pAction)->getMacro()->getString());
				setDlgItemText(IDC_MACRO, wstrMacro.get());
			}
			break;
		default:
			assert(false);
			break;
		}
	}
	sendDlgItemMessage(IDC_ACTION, CB_SETCURSEL, nItem);
	
	unsigned int nUse = pRule_->getUse();
	sendDlgItemMessage(IDC_MANUAL, BM_SETCHECK, nUse & Rule::USE_MANUAL ? BST_CHECKED : BST_UNCHECKED);
	sendDlgItemMessage(IDC_AUTO, BM_SETCHECK, nUse & Rule::USE_AUTO ? BST_CHECKED : BST_UNCHECKED);
	
	bInit_ = true;
	
	init(false);
	updateState(true);
	
	return TRUE;
}

LRESULT qm::RuleDialog::onOk()
{
	wstring_ptr wstrCondition(getDlgItemText(IDC_CONDITION));
	std::auto_ptr<Macro> pCondition(MacroParser().parse(wstrCondition.get()));
	if (!pCondition.get()) {
		// TODO MSG
		return 0;
	}
	pRule_->setCondition(pCondition);
	
	std::auto_ptr<RuleAction> pAction;
	int nItem = sendDlgItemMessage(IDC_ACTION, CB_GETCURSEL);
	switch (nItem) {
	case 0:
		break;
	case 1:
	case 2:
		{
			wstring_ptr wstrAccount(getDlgItemText(IDC_ACCOUNT));
			if (!*wstrAccount.get())
				wstrAccount.reset(0);
			wstring_ptr wstrFolder(getDlgItemText(IDC_FOLDER));
			if (!*wstrFolder.get())
				return 0;
			
			std::auto_ptr<CopyRuleAction> pCopyAction(new CopyRuleAction(
				wstrAccount.get(), wstrFolder.get(), nItem == 1));
			if (wstrTemplate_.get()) {
				pCopyAction->setTemplate(wstrTemplate_.get());
				pCopyAction->setTemplateArguments(listArgument_);
			}
			
			pAction.reset(pCopyAction.release());
		}
		break;
	case 3:
		{
			bool bDirect = sendDlgItemMessage(IDC_DIRECT, BM_GETCHECK) == BST_CHECKED;
			pAction.reset(new DeleteRuleAction(bDirect));
		}
		break;
	case 4:
		pAction.reset(new DeleteCacheRuleAction());
		break;
	case 5:
		{
			wstring_ptr wstrMacro(getDlgItemText(IDC_MACRO));
			std::auto_ptr<Macro> pMacro(MacroParser().parse(wstrMacro.get()));
			if (!pMacro.get()) {
				// TODO MSG
				return 0;
			}
			pAction.reset(new ApplyRuleAction(pMacro));
		}
		break;
	default:
		assert(false);
		break;
	}
	pRule_->setAction(pAction);
	
	unsigned int nUse = 0;
	if (sendDlgItemMessage(IDC_MANUAL, BM_GETCHECK) == BST_CHECKED)
		nUse |= Rule::USE_MANUAL;
	if (sendDlgItemMessage(IDC_AUTO, BM_GETCHECK) == BST_CHECKED)
		nUse |= Rule::USE_AUTO;
	pRule_->setUse(nUse);
	
	return DefaultDialog::onOk();
}

LRESULT qm::RuleDialog::onEdit()
{
	wstring_ptr wstrCondition(getDlgItemText(IDC_CONDITION));
	ConditionDialog dialog(wstrCondition.get());
	if (dialog.doModal(getHandle()) == IDOK)
		setDlgItemText(IDC_CONDITION, dialog.getCondition());
	return 0;
}

LRESULT qm::RuleDialog::onTemplate()
{
	CopyRuleTemplateDialog dialog(wstrTemplate_.get(), &listArgument_);
	if (dialog.doModal(getHandle()) == IDOK) {
		const WCHAR* pwszTemplate = dialog.getName();
		if (pwszTemplate)
			wstrTemplate_ = allocWString(pwszTemplate);
		else
			wstrTemplate_.reset(0);
	}
	return 0;
}

LRESULT qm::RuleDialog::onActionSelChange()
{
	updateState(true);
	return 0;
}

LRESULT qm::RuleDialog::onAccountEditChange()
{
	updateState(true);
	return 0;
}

LRESULT qm::RuleDialog::onAccountSelChange()
{
	postMessage(WM_COMMAND, MAKEWPARAM(IDC_ACCOUNT, CBN_EDITCHANGE));
	return 0;
}

LRESULT qm::RuleDialog::onConditionChange()
{
	updateState(false);
	return 0;
}

LRESULT qm::RuleDialog::onFolderEditChange()
{
	updateState(false);
	return 0;
}

LRESULT qm::RuleDialog::onFolderSelChange()
{
	postMessage(WM_COMMAND, MAKEWPARAM(IDC_FOLDER, CBN_EDITCHANGE));
	return 0;
}

LRESULT qm::RuleDialog::onMacroChange()
{
	updateState(false);
	return 0;
}

void qm::RuleDialog::updateState(bool bUpdateFolder)
{
	if (!bInit_)
		return;
	
	struct {
		UINT nId_;
		bool b_;
	} items[] = {
		{ IDC_ACCOUNTLABEL,	false	},
		{ IDC_ACCOUNT,		false	},
		{ IDC_FOLDERLABEL,	false	},
		{ IDC_FOLDER,		false	},
		{ IDC_TEMPLATE,		false	},
		{ IDC_DIRECT,		false	},
		{ IDC_MACROLABEL,	false	},
		{ IDC_MACRO,		false	}
	};
	
	int nStart = 0;
	int nEnd = 0;
	bool bEnable = true;
	switch (sendDlgItemMessage(IDC_ACTION, CB_GETCURSEL)) {
	case 0:
		break;
	case 1:
	case 2:
		nStart = 0;
		nEnd = 5;
		bEnable = Window(getDlgItem(IDC_FOLDER)).getWindowTextLength() != 0;
		break;
	case 3:
		nStart = 5;
		nEnd = 6;
		break;
	case 4:
		break;
	case 5:
		nStart = 6;
		nEnd = 8;
		bEnable = Window(getDlgItem(IDC_MACRO)).getWindowTextLength() != 0;
		break;
	default:
		assert(false);
		break;
	}
	for (int n = nStart; n < nEnd; ++n)
		items[n].b_ = true;
	for (int n = 0; n < countof(items); ++n)
		Window(getDlgItem(items[n].nId_)).showWindow(items[n].b_ ? SW_SHOW : SW_HIDE);
	
	Window(getDlgItem(IDOK)).enableWindow(
		bEnable && Window(getDlgItem(IDC_CONDITION)).getWindowTextLength() != 0);
		
	
	if (bUpdateFolder) {
		Account* pAccount = 0;
		wstring_ptr wstrAccount(getDlgItemText(IDC_ACCOUNT));
		if (wstrAccount.get())
			pAccount = pAccountManager_->getAccount(wstrAccount.get());
		updateFolder(pAccount);
	}
}

void qm::RuleDialog::updateFolder(Account* pAccount)
{
	wstring_ptr wstrFolder(getDlgItemText(IDC_FOLDER));
	
	sendDlgItemMessage(IDC_FOLDER, CB_RESETCONTENT);
	
	if (pAccount) {
		Account::FolderList l(pAccount->getFolders());
		std::sort(l.begin(), l.end(), FolderLess());
		for (Account::FolderList::const_iterator it = l.begin(); it != l.end(); ++it) {
			Folder* pFolder = *it;
			
			wstring_ptr wstrName(pFolder->getFullName());
			W2T(wstrName.get(), ptszName);
			sendDlgItemMessage(IDC_FOLDER, CB_ADDSTRING,
				0, reinterpret_cast<LPARAM>(ptszName));
		}
	}
	
	setDlgItemText(IDC_FOLDER, wstrFolder.get());
}


/****************************************************************************
 *
 * CopyRuleTemplateDialog
 *
 */

qm::CopyRuleTemplateDialog::CopyRuleTemplateDialog(const WCHAR* pwszName,
												   CopyRuleAction::ArgumentList* pListArgument) :
	DefaultDialog(IDD_COPYRULETEMPLATE),
	pListArgument_(pListArgument)
{
	if (pwszName)
		wstrName_ = allocWString(pwszName);
}

qm::CopyRuleTemplateDialog::~CopyRuleTemplateDialog()
{
}

const WCHAR* qm::CopyRuleTemplateDialog::getName() const
{
	return wstrName_.get();
}

LRESULT qm::CopyRuleTemplateDialog::onCommand(WORD nCode,
											  WORD nId)
{
	BEGIN_COMMAND_HANDLER()
		HANDLE_COMMAND_ID(IDC_ADD, onAdd)
		HANDLE_COMMAND_ID(IDC_EDIT, onEdit)
		HANDLE_COMMAND_ID(IDC_REMOVE, onRemove)
	END_COMMAND_HANDLER()
	return DefaultDialog::onCommand(nCode, nId);
}

LRESULT qm::CopyRuleTemplateDialog::onDestroy()
{
	removeNotifyHandler(this);
	return DefaultDialog::onDestroy();
}

LRESULT qm::CopyRuleTemplateDialog::onInitDialog(HWND hwndFocus,
												 LPARAM lParam)
{
	if (wstrName_.get())
		setDlgItemText(IDC_NAME, wstrName_.get());
	
	HINSTANCE hInst = Application::getApplication().getResourceHandle();
	HWND hwndList = getDlgItem(IDC_ARGUMENT);
	
	ListView_SetExtendedListViewStyle(hwndList, LVS_EX_FULLROWSELECT);
	
	struct {
		UINT nId_;
		int nWidth_;
	} columns[] = {
		{ IDS_NAME,		100	},
		{ IDS_VALUE,	100	},
	};
	for (int n = 0; n < countof(columns); ++n) {
		wstring_ptr wstrColumn(loadString(hInst, columns[n].nId_));
		W2T(wstrColumn.get(), ptszColumn);
		
		LVCOLUMN column = {
			LVCF_FMT | LVCF_TEXT | LVCF_WIDTH | LVCF_SUBITEM,
			LVCFMT_LEFT,
			columns[n].nWidth_,
			const_cast<LPTSTR>(ptszColumn),
			0,
			n,
		};
		ListView_InsertColumn(hwndList, n, &column);
	}
	
	for (CopyRuleAction::ArgumentList::size_type n = 0; n < pListArgument_->size(); ++n) {
		W2T((*pListArgument_)[n].first, ptszName);
		
		LVITEM item = {
			LVIF_TEXT,
			n,
			0,
			0,
			0,
			const_cast<LPTSTR>(ptszName),
			0,
			0,
			0
		};
		int nItem = ListView_InsertItem(hwndList, &item);
		
		W2T((*pListArgument_)[n].second, ptszValue);
		ListView_SetItemText(hwndList, nItem, 1, const_cast<LPTSTR>(ptszValue));
	}
	
	init(false);
	updateState();
	
	addNotifyHandler(this);
	
	return TRUE;
}

LRESULT qm::CopyRuleTemplateDialog::onOk()
{
	wstring_ptr wstrName(getDlgItemText(IDC_NAME));
	if (*wstrName.get())
		wstrName_ = wstrName;
	else
		wstrName_.reset(0);
	
	HWND hwndList = getDlgItem(IDC_ARGUMENT);
	CopyRuleAction::ArgumentList listArgument;
	int nCount = ListView_GetItemCount(hwndList);
	listArgument.reserve(nCount);
	for (int n = 0; n < nCount; ++n) {
		TCHAR tsz[256];
		ListView_GetItemText(hwndList, n, 0, tsz, countof(tsz) - 1);
		wstring_ptr wstrName(tcs2wcs(tsz));
		ListView_GetItemText(hwndList, n, 1, tsz, countof(tsz) - 1);
		wstring_ptr wstrValue(tcs2wcs(tsz));
		listArgument.push_back(CopyRuleAction::ArgumentList::value_type(
			wstrName.release(), wstrValue.release()));
	}
	pListArgument_->swap(listArgument);
	for (CopyRuleAction::ArgumentList::iterator it = listArgument.begin(); it != listArgument.end(); ++it) {
		freeWString((*it).first);
		freeWString((*it).second);
	}
	
	return DefaultDialog::onOk();
}

LRESULT qm::CopyRuleTemplateDialog::onNotify(NMHDR* pnmhdr,
											 bool* pbHandled)
{
	BEGIN_NOTIFY_HANDLER()
		HANDLE_NOTIFY(LVN_ITEMCHANGED, IDC_ARGUMENT, onArgumentItemChanged);
	END_NOTIFY_HANDLER()
	return NotifyHandler::onNotify(pnmhdr, pbHandled);
}

LRESULT qm::CopyRuleTemplateDialog::onAdd()
{
	ArgumentDialog dialog(0, 0);
	if (dialog.doModal(getHandle()) == IDOK) {
		HWND hwndList = getDlgItem(IDC_ARGUMENT);
		int nCount = ListView_GetItemCount(hwndList);
		
		W2T(dialog.getName(), ptszName);
		LVITEM item = {
			LVIF_TEXT | LVIF_STATE,
			nCount,
			0,
			LVIS_SELECTED | LVIS_FOCUSED,
			LVIS_SELECTED | LVIS_FOCUSED,
			const_cast<LPTSTR>(ptszName),
			0,
			0,
			0
		};
		int nItem = ListView_InsertItem(hwndList, &item);
		
		W2T(dialog.getValue(), ptszValue);
		ListView_SetItemText(hwndList, nItem, 1, const_cast<LPTSTR>(ptszValue));
	}
	return 0;
}

LRESULT qm::CopyRuleTemplateDialog::onRemove()
{
	HWND hwndList = getDlgItem(IDC_ARGUMENT);
	int nItem = ListView_GetNextItem(hwndList, -1, LVNI_SELECTED);
	if (nItem != -1) {
		ListView_DeleteItem(hwndList, nItem);
		
		int nCount = ListView_GetItemCount(hwndList);
		if (nCount != 0) {
			if (nItem < nCount) {
				ListView_SetItemState(hwndList, nItem,
					LVIS_SELECTED | LVIS_FOCUSED, LVIS_SELECTED | LVIS_FOCUSED);
			}
			else {
				ListView_SetItemState(hwndList, nCount - 1,
					LVIS_SELECTED | LVIS_FOCUSED, LVIS_SELECTED | LVIS_FOCUSED);
			}
		}
	}
	return 0;
}

LRESULT qm::CopyRuleTemplateDialog::onEdit()
{
	HWND hwndList = getDlgItem(IDC_ARGUMENT);
	int nItem = ListView_GetNextItem(hwndList, -1, LVNI_SELECTED);
	if (nItem != -1) {
		TCHAR tsz[256];
		ListView_GetItemText(hwndList, nItem, 0, tsz, countof(tsz) - 1);
		wstring_ptr wstrName(tcs2wcs(tsz));
		ListView_GetItemText(hwndList, nItem, 1, tsz, countof(tsz) - 1);
		wstring_ptr wstrValue(tcs2wcs(tsz));
		
		ArgumentDialog dialog(wstrName.get(), wstrValue.get());
		if (dialog.doModal(getHandle()) == IDOK) {
			W2T(dialog.getName(), ptszName);
			ListView_SetItemText(hwndList, nItem, 0, const_cast<LPTSTR>(ptszName));
			W2T(dialog.getValue(), ptszValue);
			ListView_SetItemText(hwndList, nItem, 1, const_cast<LPTSTR>(ptszValue));
		}
	}
	return 0;
}

LRESULT qm::CopyRuleTemplateDialog::onArgumentItemChanged(NMHDR* pnmhdr,
														  bool* pbHandled)
{
	updateState();
	*pbHandled = true;
	return 0;
}

void qm::CopyRuleTemplateDialog::updateState()
{
	HWND hwndList = getDlgItem(IDC_ARGUMENT);
	int nItem = ListView_GetNextItem(hwndList, -1, LVNI_SELECTED);
	Window(getDlgItem(IDC_EDIT)).enableWindow(nItem != -1);
	Window(getDlgItem(IDC_REMOVE)).enableWindow(nItem != -1);
}


/****************************************************************************
 *
 * ArgumentDialog
 *
 */

qm::ArgumentDialog::ArgumentDialog(const WCHAR* pwszName,
								   const WCHAR* pwszValue) :
	DefaultDialog(IDD_ARGUMENT)
{
	if (pwszName)
		wstrName_ = allocWString(pwszName);
	if (pwszValue)
		wstrValue_ = allocWString(pwszValue);
}

qm::ArgumentDialog::~ArgumentDialog()
{
}

const WCHAR* qm::ArgumentDialog::getName() const
{
	return wstrName_.get();
}

const WCHAR* qm::ArgumentDialog::getValue() const
{
	return wstrValue_.get();
}

LRESULT qm::ArgumentDialog::onCommand(WORD nCode,
									  WORD nId)
{
	BEGIN_COMMAND_HANDLER()
		HANDLE_COMMAND_ID_CODE(IDC_NAME, EN_CHANGE, onNameChange)
	END_COMMAND_HANDLER()
	return DefaultDialog::onCommand(nCode, nId);
}

LRESULT qm::ArgumentDialog::onInitDialog(HWND hwndFocus,
										 LPARAM lParam)
{
	if (wstrName_.get())
		setDlgItemText(IDC_NAME, wstrName_.get());
	if (wstrValue_.get())
		setDlgItemText(IDC_VALUE, wstrValue_.get());
	
	init(false);
	updateState();
	
	return TRUE;
}

LRESULT qm::ArgumentDialog::onOk()
{
	wstrName_ = getDlgItemText(IDC_NAME);
	wstrValue_ = getDlgItemText(IDC_VALUE);
	
	return DefaultDialog::onOk();
}

LRESULT qm::ArgumentDialog::onNameChange()
{
	updateState();
	return 0;
}

void qm::ArgumentDialog::updateState()
{
	Window(getDlgItem(IDOK)).enableWindow(
		Window(getDlgItem(IDC_NAME)).getWindowTextLength() != 0);
}


/****************************************************************************
 *
 * AutoPilotDialog
 *
 */

qm::AutoPilotDialog::AutoPilotDialog(AutoPilotManager* pManager,
									 GoRound* pGoRound,
									 Profile* pProfile) :
	AbstractListDialog<AutoPilotEntry, AutoPilotManager::EntryList>(IDD_AUTOPILOT, IDC_ENTRIES, false),
	pManager_(pManager),
	pGoRound_(pGoRound),
	pProfile_(pProfile)
{
	const AutoPilotManager::EntryList& l = pManager->getEntries();
	AutoPilotManager::EntryList& list = getList();
	list.reserve(l.size());
	for (AutoPilotManager::EntryList::const_iterator it = l.begin(); it != l.end(); ++it)
		list.push_back(new AutoPilotEntry(**it));
}

qm::AutoPilotDialog::~AutoPilotDialog()
{
}

INT_PTR qm::AutoPilotDialog::dialogProc(UINT uMsg,
										WPARAM wParam,
										LPARAM lParam)
{
	BEGIN_DIALOG_HANDLER()
		HANDLE_SIZE()
	END_DIALOG_HANDLER()
	return AbstractListDialog<AutoPilotEntry, AutoPilotManager::EntryList>::dialogProc(uMsg, wParam, lParam);
}

LRESULT qm::AutoPilotDialog::onCommand(WORD nCode,
									   WORD nId)
{
	BEGIN_COMMAND_HANDLER()
		HANDLE_COMMAND_ID(IDC_BROWSE, onBrowse)
	END_COMMAND_HANDLER()
	return AbstractListDialog<AutoPilotEntry, AutoPilotManager::EntryList>::onCommand(nCode, nId);
}

LRESULT qm::AutoPilotDialog::onInitDialog(HWND hwndFocus,
										  LPARAM lParam)
{
	wstring_ptr wstrSound = pProfile_->getString(L"AutoPilot", L"Sound", L"");
	setDlgItemText(IDC_SOUND, wstrSound.get());
	
	if (pProfile_->getInt(L"AutoPilot", L"OnlyWhenConnected", 0))
		sendDlgItemMessage(IDC_ONLYWHENCONNECTED, BM_SETCHECK, BST_CHECKED);
	
	return AbstractListDialog<AutoPilotEntry, AutoPilotManager::EntryList>::onInitDialog(hwndFocus, lParam);
}

wstring_ptr qm::AutoPilotDialog::getLabel(const AutoPilotEntry* p) const
{
	return allocWString(p->getCourse());
}

std::auto_ptr<AutoPilotEntry> qm::AutoPilotDialog::create() const
{
	std::auto_ptr<AutoPilotEntry> pEntry(new AutoPilotEntry());
	AutoPilotEntryDialog dialog(pEntry.get(), pGoRound_);
	if (dialog.doModal(getParentPopup()) != IDOK)
		return std::auto_ptr<AutoPilotEntry>();
	return pEntry;
}

bool qm::AutoPilotDialog::edit(AutoPilotEntry* p) const
{
	AutoPilotEntryDialog dialog(p, pGoRound_);
	return dialog.doModal(getParentPopup()) == IDOK;
}

bool qm::AutoPilotDialog::save(OptionDialogContext* pContext)
{
	pManager_->setEntries(getList());
	if (!pManager_->save())
		return false;
	
	wstring_ptr wstrSound(getDlgItemText(IDC_SOUND));
	if (wstrSound.get())
		pProfile_->setString(L"AutoPilot", L"Sound", wstrSound.get());
	
	bool bConnected = sendDlgItemMessage(IDC_ONLYWHENCONNECTED, BM_GETCHECK) == BST_CHECKED;
	pProfile_->setInt(L"AutoPilot", L"OnlyWhenConnected", bConnected);
	
	return true;
}

LRESULT qm::AutoPilotDialog::onSize(UINT nFlags,
									int cx,
									int cy)
{
	layout();
	return DefaultDialog::onSize(nFlags, cx, cy);
}

LRESULT qm::AutoPilotDialog::onBrowse()
{
	HINSTANCE hInst = Application::getApplication().getResourceHandle();
	wstring_ptr wstrFilter(loadString(hInst, IDS_FILTER_SOUND));
	
	FileDialog dialog(true, wstrFilter.get(), 0, 0, 0,
		OFN_EXPLORER | OFN_HIDEREADONLY | OFN_LONGNAMES);
	if (dialog.doModal(getHandle()) == IDOK)
		setDlgItemText(IDC_SOUND, dialog.getPath());
	
	return 0;
}

void qm::AutoPilotDialog::layout()
{
#ifndef _WIN32_WCE_PSPC
	RECT rect;
	getClientRect(&rect);
	
	RECT rectSoundLabel;
	Window(getDlgItem(IDC_SOUNDLABEL)).getWindowRect(&rectSoundLabel);
	screenToClient(&rectSoundLabel);
	
	RECT rectSound;
	Window(getDlgItem(IDC_SOUND)).getWindowRect(&rectSound);
	screenToClient(&rectSound);
	int nSoundHeight = rectSound.bottom - rectSound.top;
	
	RECT rectBrowse;
	Window(getDlgItem(IDC_BROWSE)).getWindowRect(&rectBrowse);
	screenToClient(&rectBrowse);
	int nBrowseWidth = rectBrowse.right - rectBrowse.left;
	
	RECT rectConnected;
	Window(getDlgItem(IDC_ONLYWHENCONNECTED)).getWindowRect(&rectConnected);
	screenToClient(&rectConnected);
	int nConnectedHeight = rectConnected.bottom - rectConnected.top;
	
	HDWP hdwp = beginDeferWindowPos(10);
	
	hdwp = LayoutUtil::layout(this, IDC_ENTRIES, hdwp, 0,
		nSoundHeight + nConnectedHeight + 10);
	
#ifdef _WIN32_WCE
	int nLabelOffset = 2;
#else
	int nLabelOffset = 3;
#endif
	hdwp = Window(getDlgItem(IDC_SOUNDLABEL)).deferWindowPos(hdwp, 0,
		rectSoundLabel.left, rect.bottom - nSoundHeight - nConnectedHeight - 10 + nLabelOffset,
		0, 0, SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE);
	hdwp = Window(getDlgItem(IDC_SOUND)).deferWindowPos(hdwp, 0,
		rectSound.left, rect.bottom - nSoundHeight - nConnectedHeight - 10,
		rect.right - rectSound.left - nBrowseWidth - 10, nSoundHeight,
		SWP_NOZORDER | SWP_NOACTIVATE);
	hdwp = Window(getDlgItem(IDC_BROWSE)).deferWindowPos(hdwp, 0,
		rect.right - nBrowseWidth - 5, rect.bottom - nSoundHeight - nConnectedHeight - 10,
		0, 0, SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE);
	hdwp = Window(getDlgItem(IDC_ONLYWHENCONNECTED)).deferWindowPos(hdwp, 0,
		rectConnected.left, rect.bottom - nConnectedHeight - 5,
		0, 0, SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE);
	
	endDeferWindowPos(hdwp);
#endif
}


/****************************************************************************
 *
 * AutoPilotEntryDialog
 *
 */

qm::AutoPilotEntryDialog::AutoPilotEntryDialog(AutoPilotEntry* pEntry,
											   GoRound* pGoRound) :
	DefaultDialog(IDD_AUTOPILOTENTRY),
	pEntry_(pEntry),
	pGoRound_(pGoRound)
{
}

qm::AutoPilotEntryDialog::~AutoPilotEntryDialog()
{
}

LRESULT qm::AutoPilotEntryDialog::onCommand(WORD nCode,
											WORD nId)
{
	BEGIN_COMMAND_HANDLER()
		HANDLE_COMMAND_ID_CODE(IDC_COURSE, CBN_EDITCHANGE, onCourseEditChange)
		HANDLE_COMMAND_ID_CODE(IDC_COURSE, CBN_SELCHANGE, onCourseSelChange)
		HANDLE_COMMAND_ID_CODE(IDC_INTERVAL, EN_CHANGE, onIntervalChange)
	END_COMMAND_HANDLER()
	return DefaultDialog::onCommand(nCode, nId);
}

LRESULT qm::AutoPilotEntryDialog::onInitDialog(HWND hwndFocus,
											   LPARAM lParam)
{
	const GoRound::CourseList& listCourse = pGoRound_->getCourses();
	for (GoRound::CourseList::const_iterator it = listCourse.begin(); it != listCourse.end(); ++it) {
		W2T((*it)->getName(), ptszCourse);
		sendDlgItemMessage(IDC_COURSE, CB_ADDSTRING,
			0, reinterpret_cast<LPARAM>(ptszCourse));
	}
	setDlgItemText(IDC_COURSE, pEntry_->getCourse());
	setDlgItemInt(IDC_INTERVAL, pEntry_->getInterval());
	
	init(false);
	updateState();
	
	return TRUE;
}

LRESULT qm::AutoPilotEntryDialog::onOk()
{
	wstring_ptr wstrCourse(getDlgItemText(IDC_COURSE));
	if (!*wstrCourse.get())
		return 0;
	
	int nInterval = getDlgItemInt(IDC_INTERVAL);
	if (nInterval == 0)
		return 0;
	
	pEntry_->setCourse(wstrCourse.get());
	pEntry_->setInterval(nInterval);
	
	return DefaultDialog::onOk();
}

LRESULT qm::AutoPilotEntryDialog::onCourseEditChange()
{
	updateState();
	return 0;
}

LRESULT qm::AutoPilotEntryDialog::onCourseSelChange()
{
	postMessage(WM_COMMAND, MAKEWPARAM(IDC_COURSE, CBN_EDITCHANGE));
	return 0;
}

LRESULT qm::AutoPilotEntryDialog::onIntervalChange()
{
	updateState();
	return 0;
}

void qm::AutoPilotEntryDialog::updateState()
{
	Window(getDlgItem(IDOK)).enableWindow(
		Window(getDlgItem(IDC_COURSE)).getWindowTextLength() != 0 &&
		getDlgItemInt(IDC_INTERVAL) != 0);
}


/****************************************************************************
 *
 * FiltersDialog
 *
 */

qm::FiltersDialog::FiltersDialog(FilterManager* pManager) :
	AbstractListDialog<Filter, FilterManager::FilterList>(IDD_FILTERS, IDC_FILTERS, false),
	pManager_(pManager)
{
	const FilterManager::FilterList& l = pManager->getFilters();
	FilterManager::FilterList& list = getList();
	list.reserve(l.size());
	for (FilterManager::FilterList::const_iterator it = l.begin(); it != l.end(); ++it)
		list.push_back(new Filter(**it));
}

qm::FiltersDialog::~FiltersDialog()
{
}

INT_PTR qm::FiltersDialog::dialogProc(UINT uMsg,
									  WPARAM wParam,
									  LPARAM lParam)
{
	BEGIN_DIALOG_HANDLER()
		HANDLE_SIZE()
	END_DIALOG_HANDLER()
	return DefaultDialog::dialogProc(uMsg, wParam, lParam);
}

wstring_ptr qm::FiltersDialog::getLabel(const Filter* p) const
{
	return allocWString(p->getName());
}

std::auto_ptr<Filter> qm::FiltersDialog::create() const
{
	std::auto_ptr<Filter> pFilter(new Filter());
	FilterDialog dialog(pFilter.get());
	if (dialog.doModal(getParentPopup()) != IDOK)
		return std::auto_ptr<Filter>();
	return pFilter;
}

bool qm::FiltersDialog::edit(Filter* p) const
{
	FilterDialog dialog(p);
	return dialog.doModal(getParentPopup()) == IDOK;
}

bool qm::FiltersDialog::save(OptionDialogContext* pContext)
{
	pManager_->setFilters(getList());
	return pManager_->save();
}

LRESULT qm::FiltersDialog::onSize(UINT nFlags,
								  int cx,
								  int cy)
{
	layout();
	return DefaultDialog::onSize(nFlags, cx, cy);
}

void qm::FiltersDialog::layout()
{
#ifndef _WIN32_WCE_PSPC
	LayoutUtil::layout(this, IDC_FILTERS);
#endif
}


/****************************************************************************
 *
 * FilterDialog
 *
 */

qm::FilterDialog::FilterDialog(Filter* pFilter) :
	DefaultDialog(IDD_FILTER),
	pFilter_(pFilter)
{
}

qm::FilterDialog::~FilterDialog()
{
}

LRESULT qm::FilterDialog::onCommand(WORD nCode,
										   WORD nId)
{
	BEGIN_COMMAND_HANDLER()
		HANDLE_COMMAND_ID(IDC_EDIT, onEdit)
		HANDLE_COMMAND_ID_CODE(IDC_CONDITION, EN_CHANGE, onConditionChange)
		HANDLE_COMMAND_ID_CODE(IDC_NAME, EN_CHANGE, onNameChange)
	END_COMMAND_HANDLER()
	return DefaultDialog::onCommand(nCode, nId);
}

LRESULT qm::FilterDialog::onInitDialog(HWND hwndFocus,
											  LPARAM lParam)
{
	setDlgItemText(IDC_NAME, pFilter_->getName());
	
	const Macro* pCondition = pFilter_->getCondition();
	wstring_ptr wstrCondition(pCondition->getString());
	setDlgItemText(IDC_CONDITION, wstrCondition.get());
	
	init(false);
	
	return TRUE;
}

LRESULT qm::FilterDialog::onOk()
{
	wstring_ptr wstrCondition(getDlgItemText(IDC_CONDITION));
	std::auto_ptr<Macro> pCondition(MacroParser().parse(wstrCondition.get()));
	if (!pCondition.get()) {
		// TODO MSG
		return 0;
	}
	pFilter_->setCondition(pCondition);
	
	wstring_ptr wstrName(getDlgItemText(IDC_NAME));
	pFilter_->setName(wstrName.get());
	
	return DefaultDialog::onOk();
}

LRESULT qm::FilterDialog::onEdit()
{
	wstring_ptr wstrCondition(getDlgItemText(IDC_CONDITION));
	ConditionDialog dialog(wstrCondition.get());
	if (dialog.doModal(getHandle()) == IDOK)
		setDlgItemText(IDC_CONDITION, dialog.getCondition());
	return 0;
}

LRESULT qm::FilterDialog::onConditionChange()
{
	updateState();
	return 0;
}

LRESULT qm::FilterDialog::onNameChange()
{
	updateState();
	return 0;
}

void qm::FilterDialog::updateState()
{
	Window(getDlgItem(IDOK)).enableWindow(
		Window(getDlgItem(IDC_NAME)).getWindowTextLength() != 0 &&
		Window(getDlgItem(IDC_CONDITION)).getWindowTextLength() != 0);
}


/****************************************************************************
*
* FixedFormTextsDialog
*
*/

qm::FixedFormTextsDialog::FixedFormTextsDialog(FixedFormTextManager* pManager,
											   qs::Profile* pProfile) :
	AbstractListDialog<FixedFormText, FixedFormTextManager::TextList>(IDD_FIXEDFORMTEXTS, IDC_TEXTS, false),
	pManager_(pManager),
	pProfile_(pProfile)
{
	const FixedFormTextManager::TextList& l = pManager->getTexts();
	FixedFormTextManager::TextList& list = getList();
	list.reserve(l.size());
	for (FixedFormTextManager::TextList::const_iterator it = l.begin(); it != l.end(); ++it)
		list.push_back(new FixedFormText(**it));
}

qm::FixedFormTextsDialog::~FixedFormTextsDialog()
{
}

INT_PTR qm::FixedFormTextsDialog::dialogProc(UINT uMsg,
											 WPARAM wParam,
											 LPARAM lParam)
{
	BEGIN_DIALOG_HANDLER()
		HANDLE_SIZE()
	END_DIALOG_HANDLER()
	return DefaultDialog::dialogProc(uMsg, wParam, lParam);
}

wstring_ptr qm::FixedFormTextsDialog::getLabel(const FixedFormText* p) const
{
	return allocWString(p->getName());
}

std::auto_ptr<FixedFormText> qm::FixedFormTextsDialog::create() const
{
	std::auto_ptr<FixedFormText> pText(new FixedFormText());
	FixedFormTextDialog dialog(pText.get(), pProfile_);
	if (dialog.doModal(getParentPopup()) != IDOK)
		return std::auto_ptr<FixedFormText>();
	return pText;
}

bool qm::FixedFormTextsDialog::edit(FixedFormText* p) const
{
	FixedFormTextDialog dialog(p, pProfile_);
	return dialog.doModal(getParentPopup()) == IDOK;
}

bool qm::FixedFormTextsDialog::save(OptionDialogContext* pContext)
{
	pManager_->setTexts(getList());
	return pManager_->save();
}

LRESULT qm::FixedFormTextsDialog::onSize(UINT nFlags,
										 int cx,
										 int cy)
{
	layout();
	return DefaultDialog::onSize(nFlags, cx, cy);
}

void qm::FixedFormTextsDialog::layout()
{
#ifndef _WIN32_WCE_PSPC
	LayoutUtil::layout(this, IDC_TEXTS);
#endif
}


/****************************************************************************
 *
 * FixedFormTextDialog
 *
 */

qm::FixedFormTextDialog::FixedFormTextDialog(FixedFormText* pText,
											 qs::Profile* pProfile) :
	DefaultDialog(IDD_FIXEDFORMTEXT),
	pText_(pText),
	pProfile_(pProfile)
{
}

qm::FixedFormTextDialog::~FixedFormTextDialog()
{
}

INT_PTR qm::FixedFormTextDialog::dialogProc(UINT uMsg,
											WPARAM wParam,
											LPARAM lParam)
{
	BEGIN_DIALOG_HANDLER()
		HANDLE_SIZE()
	END_DIALOG_HANDLER()
	return DefaultDialog::dialogProc(uMsg, wParam, lParam);
}

LRESULT qm::FixedFormTextDialog::onCommand(WORD nCode,
										   WORD nId)
{
	BEGIN_COMMAND_HANDLER()
		HANDLE_COMMAND_ID_CODE(IDC_NAME, EN_CHANGE, onNameChange)
	END_COMMAND_HANDLER()
	return DefaultDialog::onCommand(nCode, nId);
}

LRESULT qm::FixedFormTextDialog::onDestroy()
{
#ifndef _WIN32_WCE
	RECT rect;
	getWindowRect(&rect);
	pProfile_->setInt(L"FixedFormTextDialog", L"Width", rect.right - rect.left);
	pProfile_->setInt(L"FixedFormTextDialog", L"Height", rect.bottom - rect.top);
#endif
	return DefaultDialog::onDestroy();
}

LRESULT qm::FixedFormTextDialog::onInitDialog(HWND hwndFocus,
											  LPARAM lParam)
{
	setDlgItemText(IDC_NAME, pText_->getName());
	
	wstring_ptr wstrText(Util::convertLFtoCRLF(pText_->getText()));
	setDlgItemText(IDC_TEXT, wstrText.get());
	
#ifdef _WIN32_WCE
	RECT rectWorkArea;
	::SystemParametersInfo(SPI_GETWORKAREA, 0, &rectWorkArea, 0);
	int nWidth = rectWorkArea.right - rectWorkArea.left;
	int nHeight = rectWorkArea.bottom - rectWorkArea.top;
	setWindowPos(0, 0, 0, nWidth, nHeight, SWP_NOZORDER | SWP_NOACTIVATE);
#else
	int nWidth = pProfile_->getInt(L"FixedFormTextDialog", L"Width", 620);
	int nHeight = pProfile_->getInt(L"FixedFormTextDialog", L"Height", 450);
	setWindowPos(0, 0, 0, nWidth, nHeight,
		SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE);
#endif
	
	init(false);
	
	return TRUE;
}

LRESULT qm::FixedFormTextDialog::onOk()
{
	wstring_ptr wstrName(getDlgItemText(IDC_NAME));
	
	wstring_ptr wstrText(getDlgItemText(IDC_TEXT));
	wstrText = Util::convertCRLFtoLF(wstrText.get());
	
	pText_->setName(wstrName.get());
	pText_->setText(wstrText.get());
	
	return DefaultDialog::onOk();
}

LRESULT qm::FixedFormTextDialog::onSize(UINT nFlags,
										int cx,
										int cy)
{
	layout();
	return DefaultDialog::onSize(nFlags, cx, cy);
}

LRESULT qm::FixedFormTextDialog::onNameChange()
{
	updateState();
	return 0;
}

void qm::FixedFormTextDialog::updateState()
{
	Window(getDlgItem(IDOK)).enableWindow(
		Window(getDlgItem(IDC_NAME)).getWindowTextLength() != 0);
}

void qm::FixedFormTextDialog::layout()
{
#ifndef _WIN32_WCE_PSPC
	RECT rect;
	getClientRect(&rect);
	
#ifndef _WIN32_WCE
	RECT rectButton;
	Window(getDlgItem(IDOK)).getWindowRect(&rectButton);
	int nButtonWidth = rectButton.right - rectButton.left;
	int nButtonHeight = rectButton.bottom - rectButton.top;
#else
	int nButtonHeight = -5;
#endif
	
	RECT rectName;
	Window(getDlgItem(IDC_NAME)).getWindowRect(&rectName);
	screenToClient(&rectName);
	
	RECT rectText;
	Window(getDlgItem(IDC_TEXT)).getWindowRect(&rectText);
	screenToClient(&rectText);
	
	HDWP hdwp = beginDeferWindowPos(5);
	hdwp = Window(getDlgItem(IDC_NAME)).deferWindowPos(hdwp, 0, 0, 0,
		rect.right - rectName.left - 5, rectName.bottom - rectName.top,
		SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE);
	hdwp = Window(getDlgItem(IDC_TEXT)).deferWindowPos(hdwp, 0, 0, 0,
		rect.right - rectText.left - 5, rect.bottom - rectText.top - nButtonHeight - 10,
		SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE);
	
#ifndef _WIN32_WCE
	hdwp = Window(getDlgItem(IDOK)).deferWindowPos(hdwp, 0,
		rect.right - (nButtonWidth + 5)*2 - 15, rect.bottom - nButtonHeight - 5,
		nButtonWidth, nButtonHeight, SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE);
	hdwp = Window(getDlgItem(IDCANCEL)).deferWindowPos(hdwp, 0,
		rect.right - (nButtonWidth + 5) - 15, rect.bottom - nButtonHeight - 5,
		nButtonWidth, nButtonHeight, SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE);
	hdwp = Window(getDlgItem(IDC_SIZEGRIP)).deferWindowPos(hdwp, 0,
		rect.right - rect.left - 13, rect.bottom - rect.top - 12,
		13, 12, SWP_NOZORDER | SWP_NOACTIVATE);
#endif
	
	endDeferWindowPos(hdwp);
#endif
}


/****************************************************************************
 *
 * GoRoundDialog
 *
 */

qm::GoRoundDialog::GoRoundDialog(GoRound* pGoRound,
								 AccountManager* pAccountManager,
								 SyncFilterManager* pSyncFilterManager,
								 Profile* pProfile) :
	AbstractListDialog<GoRoundCourse, GoRound::CourseList>(IDD_GOROUND, IDC_COURSE, false),
	pGoRound_(pGoRound),
	pAccountManager_(pAccountManager),
	pSyncFilterManager_(pSyncFilterManager),
	pProfile_(pProfile)
{
	const GoRound::CourseList& listCourse = pGoRound_->getCourses();
	GoRound::CourseList& list = getList();
	list.reserve(listCourse.size());
	for (GoRound::CourseList::const_iterator it = listCourse.begin(); it != listCourse.end(); ++it)
		list.push_back(new GoRoundCourse(**it));
}

qm::GoRoundDialog::~GoRoundDialog()
{
}

INT_PTR qm::GoRoundDialog::dialogProc(UINT uMsg,
									  WPARAM wParam,
									  LPARAM lParam)
{
	BEGIN_DIALOG_HANDLER()
		HANDLE_SIZE()
	END_DIALOG_HANDLER()
	return DefaultDialog::dialogProc(uMsg, wParam, lParam);
}

wstring_ptr qm::GoRoundDialog::getLabel(const GoRoundCourse* p) const
{
	return allocWString(p->getName());
}

std::auto_ptr<GoRoundCourse> qm::GoRoundDialog::create() const
{
	std::auto_ptr<GoRoundCourse> pCourse(new GoRoundCourse());
	GoRoundCourseDialog dialog(pCourse.get(), pAccountManager_, pSyncFilterManager_, pProfile_);
	if (dialog.doModal(getParentPopup()) != IDOK)
		return std::auto_ptr<GoRoundCourse>();
	return pCourse;
}

bool qm::GoRoundDialog::edit(GoRoundCourse* p) const
{
	GoRoundCourseDialog dialog(p, pAccountManager_, pSyncFilterManager_, pProfile_);
	return dialog.doModal(getParentPopup()) == IDOK;
}

bool qm::GoRoundDialog::save(OptionDialogContext* pContext)
{
	pGoRound_->setCourses(getList());
	return pGoRound_->save();
}

LRESULT qm::GoRoundDialog::onSize(UINT nFlags,
								  int cx,
								  int cy)
{
	layout();
	return DefaultDialog::onSize(nFlags, cx, cy);
}

void qm::GoRoundDialog::layout()
{
#ifndef _WIN32_WCE_PSPC
	LayoutUtil::layout(this, IDC_COURSE);
#endif
}


/****************************************************************************
 *
 * GoRoundCourseDialog
 *
 */

qm::GoRoundCourseDialog::GoRoundCourseDialog(GoRoundCourse* pCourse,
											 AccountManager* pAccountManager,
											 SyncFilterManager* pSyncFilterManager,
											 Profile* pProfile) :
	AbstractListDialog<GoRoundEntry, GoRoundCourse::EntryList>(IDD_GOROUNDCOURSE, IDC_ENTRY, true),
	pCourse_(pCourse),
	pAccountManager_(pAccountManager),
	pSyncFilterManager_(pSyncFilterManager),
	pProfile_(pProfile)
{
	const GoRoundCourse::EntryList& listEntry = pCourse_->getEntries();
	GoRoundCourse::EntryList& list = getList();
	list.reserve(listEntry.size());
	for (GoRoundCourse::EntryList::const_iterator it = listEntry.begin(); it != listEntry.end(); ++it)
		list.push_back(new GoRoundEntry(**it));
}

qm::GoRoundCourseDialog::~GoRoundCourseDialog()
{
}

INT_PTR qm::GoRoundCourseDialog::dialogProc(UINT uMsg,
											WPARAM wParam,
											LPARAM lParam)
{
	BEGIN_DIALOG_HANDLER()
		HANDLE_SIZE()
	END_DIALOG_HANDLER()
	return AbstractListDialog<GoRoundEntry, GoRoundCourse::EntryList>::dialogProc(uMsg, wParam, lParam);
}

LRESULT qm::GoRoundCourseDialog::onCommand(WORD nCode,
										   WORD nId)
{
	BEGIN_COMMAND_HANDLER()
		HANDLE_COMMAND_ID(IDC_DIALUP, onDialup)
		HANDLE_COMMAND_ID_CODE(IDC_NAME, EN_CHANGE, onNameChange)
	END_COMMAND_HANDLER()
	return AbstractListDialog<GoRoundEntry, GoRoundCourse::EntryList>::onCommand(nCode, nId);
}

LRESULT qm::GoRoundCourseDialog::onDestroy()
{
#ifndef _WIN32_WCE
	RECT rect;
	getWindowRect(&rect);
	pProfile_->setInt(L"GoRoundCourseDialog", L"Width", rect.right - rect.left);
	pProfile_->setInt(L"GoRoundCourseDialog", L"Height", rect.bottom - rect.top);
#endif
	
	return AbstractListDialog<GoRoundEntry, GoRoundCourse::EntryList>::onDestroy();
}

LRESULT qm::GoRoundCourseDialog::onInitDialog(HWND hwndFocus,
											  LPARAM lParam)
{
	setDlgItemText(IDC_NAME, pCourse_->getName());
	
	switch (pCourse_->getType()) {
	case GoRoundCourse::TYPE_SEQUENTIAL:
		sendDlgItemMessage(IDC_SEQUENTIAL, BM_SETCHECK, BST_CHECKED);
		break;
	case GoRoundCourse::TYPE_PARALLEL:
		sendDlgItemMessage(IDC_PARALLEL, BM_SETCHECK, BST_CHECKED);
		break;
	default:
		assert(false);
		break;
	}
	
	sendDlgItemMessage(IDC_CONFIRM, BM_SETCHECK,
		pCourse_->isFlag(GoRoundCourse::FLAG_CONFIRM) ? BST_CHECKED : BST_UNCHECKED);
	
#ifdef _WIN32_WCE
	RECT rectWorkArea;
	::SystemParametersInfo(SPI_GETWORKAREA, 0, &rectWorkArea, 0);
	int nWidth = rectWorkArea.right - rectWorkArea.left;
	int nHeight = rectWorkArea.bottom - rectWorkArea.top;
	setWindowPos(0, 0, 0, nWidth, nHeight, SWP_NOZORDER | SWP_NOACTIVATE);
#else
	int nWidth = pProfile_->getInt(L"GoRoundCourseDialog", L"Width", 620);
	int nHeight = pProfile_->getInt(L"GoRoundCourseDialog", L"Height", 450);
	setWindowPos(0, 0, 0, nWidth, nHeight,
		SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE);
#endif
	
	return AbstractListDialog<GoRoundEntry, GoRoundCourse::EntryList>::onInitDialog(hwndFocus, lParam);
}

LRESULT qm::GoRoundCourseDialog::onOk()
{
	wstring_ptr wstrName(getDlgItemText(IDC_NAME));
	if (!wstrName.get())
		return 0;
	pCourse_->setName(wstrName.get());
	
	pCourse_->setEntries(getList());
	
	pCourse_->setType(sendDlgItemMessage(IDC_SEQUENTIAL, BM_GETCHECK) == BST_CHECKED ?
		GoRoundCourse::TYPE_SEQUENTIAL : GoRoundCourse::TYPE_PARALLEL);
	pCourse_->setFlags(sendDlgItemMessage(IDC_CONFIRM, BM_GETCHECK) == BST_CHECKED ?
		GoRoundCourse::FLAG_CONFIRM : 0);
	
	return AbstractListDialog<GoRoundEntry, GoRoundCourse::EntryList>::onOk();
}

wstring_ptr qm::GoRoundCourseDialog::getLabel(const GoRoundEntry* p) const
{
	StringBuffer<WSTRING> buf(p->getAccount());
	if (p->getSubAccount()) {
		buf.append(L'/');
		buf.append(p->getSubAccount());
	}
	if (p->getFolder()) {
		buf.append(L" [");
		buf.append(p->getFolder());
		buf.append(L']');
	}
	return buf.getString();
}

std::auto_ptr<GoRoundEntry> qm::GoRoundCourseDialog::create() const
{
	std::auto_ptr<GoRoundEntry> pEntry(new GoRoundEntry());
	GoRoundEntryDialog dialog(pEntry.get(), pAccountManager_, pSyncFilterManager_);
	if (dialog.doModal(getHandle()) != IDOK)
		return std::auto_ptr<GoRoundEntry>();
	return pEntry;
}

bool qm::GoRoundCourseDialog::edit(GoRoundEntry* p) const
{
	GoRoundEntryDialog dialog(p, pAccountManager_, pSyncFilterManager_);
	return dialog.doModal(getHandle()) == IDOK;
}

void qm::GoRoundCourseDialog::updateState()
{
	AbstractListDialog<GoRoundEntry, GoRoundCourse::EntryList>::updateState();
	
	wstring_ptr wstrName(getDlgItemText(IDC_NAME));
	Window(getDlgItem(IDOK)).enableWindow(*wstrName.get() != L'\0');
}

LRESULT qm::GoRoundCourseDialog::onSize(UINT nFlags,
										int cx,
										int cy)
{
	layout();
	return AbstractListDialog<GoRoundEntry, GoRoundCourse::EntryList>::onSize(nFlags, cx, cy);
}

LRESULT qm::GoRoundCourseDialog::onDialup()
{
	if (pCourse_->getDialup()) {
		GoRoundDialupDialog dialog(pCourse_->getDialup(), false);
		if (dialog.doModal(getHandle()) == IDOK) {
			if (dialog.isNoDialup()) {
				std::auto_ptr<GoRoundDialup> pDialup;
				pCourse_->setDialup(pDialup);
			}
		}
	}
	else {
		std::auto_ptr<GoRoundDialup> pDialup(new GoRoundDialup());
		GoRoundDialupDialog dialog(pDialup.get(), true);
		if (dialog.doModal(getHandle()) == IDOK) {
			if (!dialog.isNoDialup())
				pCourse_->setDialup(pDialup);
		}
	}
	
	return 0;
}

LRESULT qm::GoRoundCourseDialog::onNameChange()
{
	updateState();
	return 0;
}

void qm::GoRoundCourseDialog::layout()
{
#ifndef _WIN32_WCE_PSPC
	RECT rect;
	getClientRect(&rect);
	
	RECT rectName;
	Window(getDlgItem(IDC_NAME)).getWindowRect(&rectName);
	screenToClient(&rectName);
	
	RECT rectType;
	Window(getDlgItem(IDC_TYPE)).getWindowRect(&rectType);
	screenToClient(&rectType);
	int nTypeHeight = rectType.bottom - rectType.top;
	
	RECT rectSequential;
	Window(getDlgItem(IDC_SEQUENTIAL)).getWindowRect(&rectSequential);
	screenToClient(&rectSequential);
	RECT rectParallel;
	Window(getDlgItem(IDC_PARALLEL)).getWindowRect(&rectParallel);
	screenToClient(&rectParallel);
	
	RECT rectButton;
	Window(getDlgItem(IDC_DIALUP)).getWindowRect(&rectButton);
	int nButtonWidth = rectButton.right - rectButton.left;
#ifndef _WIN32_WCE
	int nButtonHeight = rectButton.bottom - rectButton.top;
#else
	int nButtonHeight = -5;
#endif
	
	HDWP hdwp = beginDeferWindowPos(15);
	
	hdwp = Window(getDlgItem(IDC_NAME)).deferWindowPos(hdwp, 0, 0, 0,
		rect.right - rectName.left - 5, rectName.bottom - rectName.top,
		SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE);
	
	hdwp = LayoutUtil::layout(this, IDC_ENTRY, hdwp, rectName.bottom,
		nButtonHeight + nTypeHeight + 10);
	
	hdwp = Window(getDlgItem(IDC_TYPE)).deferWindowPos(hdwp, 0,
		rectType.left, rect.bottom - nButtonHeight - nTypeHeight - 10,
		0, 0, SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE);
	hdwp = Window(getDlgItem(IDC_SEQUENTIAL)).deferWindowPos(hdwp, 0,
		rectSequential.left, rect.bottom - nButtonHeight - nTypeHeight - 10 + (rectSequential.top - rectType.top),
		0, 0, SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE);
	hdwp = Window(getDlgItem(IDC_PARALLEL)).deferWindowPos(hdwp, 0,
		rectParallel.left, rect.bottom - nButtonHeight - nTypeHeight - 10 + (rectParallel.top - rectType.top),
		0, 0, SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE);
	hdwp = Window(getDlgItem(IDC_CONFIRM)).deferWindowPos(hdwp, 0,
		rectType.right + 5, rect.bottom - nButtonHeight - nTypeHeight,
		0, 0, SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE);
	hdwp = Window(getDlgItem(IDC_DIALUP)).deferWindowPos(hdwp, 0,
		rect.right - (nButtonWidth + 5), rect.bottom - nButtonHeight - nTypeHeight - 10,
		0, 0, SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE);
	
#ifndef _WIN32_WCE
	hdwp = Window(getDlgItem(IDOK)).deferWindowPos(hdwp, 0,
		rect.right - (nButtonWidth + 5)*2 - 15, rect.bottom - nButtonHeight - 5,
		nButtonWidth, nButtonHeight, SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE);
	hdwp = Window(getDlgItem(IDCANCEL)).deferWindowPos(hdwp, 0,
		rect.right - (nButtonWidth + 5) - 15, rect.bottom - nButtonHeight - 5,
		nButtonWidth, nButtonHeight, SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE);
	hdwp = Window(getDlgItem(IDC_SIZEGRIP)).deferWindowPos(hdwp, 0,
		rect.right - rect.left - 13, rect.bottom - rect.top - 12,
		13, 12, SWP_NOZORDER | SWP_NOACTIVATE);
#endif
	
	endDeferWindowPos(hdwp);
#endif
}


/****************************************************************************
 *
 * GoRoundEntryDialog
 *
 */

qm::GoRoundEntryDialog::GoRoundEntryDialog(GoRoundEntry* pEntry,
										   AccountManager* pAccountManager,
										   SyncFilterManager* pSyncFilterManager) :
	DefaultDialog(IDD_GOROUNDENTRY),
	pEntry_(pEntry),
	pAccountManager_(pAccountManager),
	pSyncFilterManager_(pSyncFilterManager)
{
}

qm::GoRoundEntryDialog::~GoRoundEntryDialog()
{
}

LRESULT qm::GoRoundEntryDialog::onCommand(WORD nCode,
										  WORD nId)
{
	BEGIN_COMMAND_HANDLER()
		HANDLE_COMMAND_ID(IDC_EDIT, onEdit)
		HANDLE_COMMAND_ID_CODE(IDC_ACCOUNT, CBN_EDITCHANGE, onAccountEditChange)
		HANDLE_COMMAND_ID_CODE(IDC_ACCOUNT, CBN_SELCHANGE, onAccountSelChange)
		HANDLE_COMMAND_ID_CODE(IDC_SELECTFOLDER, BN_CLICKED, onSelectFolderClicked)
	END_COMMAND_HANDLER()
	return DefaultDialog::onCommand(nCode, nId);
}

LRESULT qm::GoRoundEntryDialog::onInitDialog(HWND hwndFocus,
											 LPARAM lParam)
{
	HINSTANCE hInst = Application::getApplication().getResourceHandle();
	
	const AccountManager::AccountList& listAccount = pAccountManager_->getAccounts();
	for (AccountManager::AccountList::const_iterator it = listAccount.begin(); it != listAccount.end(); ++it) {
		Account* pAccount = *it;
		W2T(pAccount->getName(), ptszName);
		sendDlgItemMessage(IDC_ACCOUNT, CB_ADDSTRING,
			0, reinterpret_cast<LPARAM>(ptszName));
	}
	setDlgItemText(IDC_ACCOUNT, pEntry_->getAccount());
	
	const WCHAR* pwszSubAccount = pEntry_->getSubAccount();
	if (pwszSubAccount) {
		setDlgItemText(IDC_SUBACCOUNT, pwszSubAccount);
	}
	else {
		wstring_ptr wstrUnspecified(loadString(hInst, IDS_UNSPECIFIED));
		setDlgItemText(IDC_SUBACCOUNT, wstrUnspecified.get());
	}
	
	const WCHAR* pwszFolder = pEntry_->getFolder();
	if (pwszFolder) {
		setDlgItemText(IDC_FOLDER, pwszFolder);
	}
	else {
		wstring_ptr wstrAll(loadString(hInst, IDS_ALLFOLDER));
		setDlgItemText(IDC_FOLDER, wstrAll.get());
	}
	
	if (pEntry_->isFlag(GoRoundEntry::FLAG_SELECTFOLDER))
		sendDlgItemMessage(IDC_SELECTFOLDER, BM_SETCHECK, BST_CHECKED);
	
	const WCHAR* pwszFilter = pEntry_->getFilter();
	if (pwszFilter)
		setDlgItemText(IDC_SYNCFILTER, pwszFilter);
	sendDlgItemMessage(IDC_SYNCFILTER, CB_SETDROPPEDWIDTH, 150);
	
	if (pEntry_->isFlag(GoRoundEntry::FLAG_SEND) &&
		pEntry_->isFlag(GoRoundEntry::FLAG_RECEIVE))
		sendDlgItemMessage(IDC_SENDRECEIVE, BM_SETCHECK, BST_CHECKED);
	else if (pEntry_->isFlag(GoRoundEntry::FLAG_SEND))
		sendDlgItemMessage(IDC_SEND, BM_SETCHECK, BST_CHECKED);
	else if (pEntry_->isFlag(GoRoundEntry::FLAG_RECEIVE))
		sendDlgItemMessage(IDC_RECEIVE, BM_SETCHECK, BST_CHECKED);
	
	if (pEntry_->getConnectReceiveBeforeSend() == GoRoundEntry::CRBS_TRUE)
		sendDlgItemMessage(IDC_CONNECTRECEIVEBEFORESEND, BM_SETCHECK, BST_CHECKED);
	
	init(false);
	updateState();
	
	return TRUE;
}

LRESULT qm::GoRoundEntryDialog::onOk()
{
	HINSTANCE hInst = Application::getApplication().getResourceHandle();
	
	wstring_ptr wstrAccount(getDlgItemText(IDC_ACCOUNT));
	if (!wstrAccount.get())
		return 0;
	
	wstring_ptr wstrSubAccount(getDlgItemText(IDC_SUBACCOUNT));
	const WCHAR* pwszSubAccount = wstrSubAccount.get();
	wstring_ptr wstrUnspecified(loadString(hInst, IDS_UNSPECIFIED));
	wstring_ptr wstrDefault(loadString(hInst, IDS_DEFAULTSUBACCOUNT));
	if (wcscmp(pwszSubAccount, wstrUnspecified.get()) == 0)
		pwszSubAccount = 0;
	else if (wcscmp(pwszSubAccount, wstrDefault.get()) == 0)
		pwszSubAccount = L"";
	
	wstring_ptr wstrFolder(getDlgItemText(IDC_FOLDER));
	const WCHAR* pwszFolder = wstrFolder.get();
	wstring_ptr wstrAll(loadString(hInst, IDS_ALLFOLDER));
	if (wcscmp(pwszFolder, wstrAll.get()) == 0)
		pwszFolder = 0;
	std::auto_ptr<RegexPattern> pFolderPattern;
	if (pwszFolder) {
		pFolderPattern = RegexCompiler().compile(pwszFolder);
		if (!pFolderPattern.get())
			return 0;
	}
	
	unsigned int nFlags = 0;
	if (sendDlgItemMessage(IDC_SELECTFOLDER, BM_GETCHECK) == BST_CHECKED)
		nFlags |= GoRoundEntry::FLAG_SELECTFOLDER;
	if (sendDlgItemMessage(IDC_SENDRECEIVE, BM_GETCHECK) == BST_CHECKED)
		nFlags |= GoRoundEntry::FLAG_SEND | GoRoundEntry::FLAG_RECEIVE;
	else if (sendDlgItemMessage(IDC_RECEIVE, BM_GETCHECK) == BST_CHECKED)
		nFlags |= GoRoundEntry::FLAG_RECEIVE;
	else if (sendDlgItemMessage(IDC_SEND, BM_GETCHECK) == BST_CHECKED)
		nFlags |= GoRoundEntry::FLAG_SEND;
	
	wstring_ptr wstrFilter(getDlgItemText(IDC_SYNCFILTER));
	const WCHAR* pwszFilter = wstrFilter.get();
	if (!*pwszFilter)
		pwszFilter = 0;
	
	GoRoundEntry::ConnectReceiveBeforeSend crbs =
		sendDlgItemMessage(IDC_CONNECTRECEIVEBEFORESEND, BM_GETCHECK) == BST_CHECKED ?
		GoRoundEntry::CRBS_TRUE : GoRoundEntry::CRBS_NONE;
	
	pEntry_->setAccount(wstrAccount.get());
	pEntry_->setSubAccount(pwszSubAccount);
	pEntry_->setFolder(pwszFolder, pFolderPattern);
	pEntry_->setFlags(nFlags);
	pEntry_->setFilter(pwszFilter);
	pEntry_->setConnectReceiveBeforeSend(crbs);
	
	return DefaultDialog::onOk();
}

LRESULT qm::GoRoundEntryDialog::onEdit()
{
//	SyncFilterSetsDialog dialog(pSyncFilterManager_);
//	if (dialog.doModal(getHandle()) == IDOK)
//		updateFilter();
	return 0;
}

LRESULT qm::GoRoundEntryDialog::onAccountEditChange()
{
	updateState();
	return 0;
}

LRESULT qm::GoRoundEntryDialog::onAccountSelChange()
{
	postMessage(WM_COMMAND, MAKEWPARAM(IDC_ACCOUNT, CBN_EDITCHANGE));
	return 0;
}

LRESULT qm::GoRoundEntryDialog::onSelectFolderClicked()
{
	updateState();
	return 0;
}

void qm::GoRoundEntryDialog::updateState()
{
	Account* pAccount = 0;
	
	wstring_ptr wstrAccount(getDlgItemText(IDC_ACCOUNT));
	if (wstrAccount.get())
		pAccount = pAccountManager_->getAccount(wstrAccount.get());
	
	updateSubAccount(pAccount);
	updateFolder(pAccount);
	updateFilter();
	
	Window(getDlgItem(IDC_FOLDER)).enableWindow(
		sendDlgItemMessage(IDC_SELECTFOLDER, BM_GETCHECK) != BST_CHECKED);
	Window(getDlgItem(IDOK)).enableWindow(*wstrAccount.get() != L'\0');
}

void qm::GoRoundEntryDialog::updateSubAccount(Account* pAccount)
{
	HINSTANCE hInst = Application::getApplication().getResourceHandle();
	
	wstring_ptr wstrSubAccount(getDlgItemText(IDC_SUBACCOUNT));
	
	sendDlgItemMessage(IDC_SUBACCOUNT, CB_RESETCONTENT);
	
	wstring_ptr wstrUnspecified(loadString(hInst, IDS_UNSPECIFIED));
	W2T(wstrUnspecified.get(), ptszUnspecified);
	sendDlgItemMessage(IDC_SUBACCOUNT, CB_ADDSTRING,
		0, reinterpret_cast<LPARAM>(ptszUnspecified));
	
	if (pAccount) {
		Account::SubAccountList l(pAccount->getSubAccounts());
		std::sort(l.begin(), l.end(),
			binary_compose_f_gx_hy(
				string_equal<WCHAR>(),
				std::mem_fun(&SubAccount::getName),
				std::mem_fun(&SubAccount::getName)));
		for (Account::SubAccountList::const_iterator it = l.begin(); it != l.end(); ++it) {
			SubAccount* pSubAccount = *it;
			if (*pSubAccount->getName()) {
				W2T(pSubAccount->getName(), ptszName);
				sendDlgItemMessage(IDC_SUBACCOUNT, CB_ADDSTRING,
					0, reinterpret_cast<LPARAM>(ptszName));
			}
			else {
				wstring_ptr wstrDefault(loadString(hInst, IDS_DEFAULTSUBACCOUNT));
				W2T(wstrDefault.get(), ptszDefault);
				sendDlgItemMessage(IDC_SUBACCOUNT, CB_ADDSTRING,
					0, reinterpret_cast<LPARAM>(ptszDefault));
			}
		}
	}
	
	setDlgItemText(IDC_SUBACCOUNT, wstrSubAccount.get());
}

void qm::GoRoundEntryDialog::updateFolder(Account* pAccount)
{
	HINSTANCE hInst = Application::getApplication().getResourceHandle();
	
	wstring_ptr wstrFolder(getDlgItemText(IDC_FOLDER));
	
	sendDlgItemMessage(IDC_FOLDER, CB_RESETCONTENT);
	
	wstring_ptr wstrAll(loadString(hInst, IDS_ALLFOLDER));
	W2T(wstrAll.get(), ptszAll);
	sendDlgItemMessage(IDC_FOLDER, CB_ADDSTRING,
		0, reinterpret_cast<LPARAM>(ptszAll));
	
	if (pAccount) {
		Account::FolderList l(pAccount->getFolders());
		std::sort(l.begin(), l.end(), FolderLess());
		for (Account::FolderList::const_iterator it = l.begin(); it != l.end(); ++it) {
			Folder* pFolder = *it;
			
			wstring_ptr wstrName(pFolder->getFullName());
			W2T(wstrName.get(), ptszName);
			sendDlgItemMessage(IDC_FOLDER, CB_ADDSTRING,
				0, reinterpret_cast<LPARAM>(ptszName));
		}
	}
	
	setDlgItemText(IDC_FOLDER, wstrFolder.get());
}

void qm::GoRoundEntryDialog::updateFilter()
{
	HINSTANCE hInst = Application::getApplication().getResourceHandle();
	
	wstring_ptr wstrFilter(getDlgItemText(IDC_SYNCFILTER));
	
	sendDlgItemMessage(IDC_SYNCFILTER, CB_RESETCONTENT);
	
	sendDlgItemMessage(IDC_SYNCFILTER, CB_ADDSTRING,
		0, reinterpret_cast<LPARAM>(_T("")));
	const SyncFilterManager::FilterSetList& l = pSyncFilterManager_->getFilterSets();
	for (SyncFilterManager::FilterSetList::const_iterator it = l.begin(); it != l.end(); ++it) {
		SyncFilterSet* pSet = *it;
		W2T(pSet->getName(), ptszName);
		sendDlgItemMessage(IDC_SYNCFILTER, CB_ADDSTRING,
			0, reinterpret_cast<LPARAM>(ptszName));
	}
	
	setDlgItemText(IDC_SYNCFILTER, wstrFilter.get());
}


/****************************************************************************
 *
 * GoRoundDialupDialog
 *
 */

qm::GoRoundDialupDialog::GoRoundDialupDialog(GoRoundDialup* pDialup,
											 bool bNoDialup) :
	DefaultDialog(IDD_GOROUNDDIALUP),
	pDialup_(pDialup),
	bNoDialup_(bNoDialup)
{
}

qm::GoRoundDialupDialog::~GoRoundDialupDialog()
{
}

bool qm::GoRoundDialupDialog::isNoDialup() const
{
	return bNoDialup_;
}

LRESULT qm::GoRoundDialupDialog::onCommand(WORD nCode,
										   WORD nId)
{
	BEGIN_COMMAND_HANDLER()
		HANDLE_COMMAND_ID_RANGE(IDC_NEVER, IDC_CONNECT, onTypeSelect)
	END_COMMAND_HANDLER()
	return DefaultDialog::onCommand(nCode, nId);
}

LRESULT qm::GoRoundDialupDialog::onInitDialog(HWND hwndFocus,
											  LPARAM lParam)
{
	if (bNoDialup_)
		sendDlgItemMessage(IDC_NEVER, BM_SETCHECK, BST_CHECKED);
	else if (pDialup_->isFlag(GoRoundDialup::FLAG_WHENEVERNOTCONNECTED))
		sendDlgItemMessage(IDC_WHENEVERNOTCONNECTED, BM_SETCHECK, BST_CHECKED);
	else
		sendDlgItemMessage(IDC_CONNECT, BM_SETCHECK, BST_CHECKED);
	
	if (pDialup_->getName())
		setDlgItemText(IDC_ENTRY, pDialup_->getName());
	if (pDialup_->getDialFrom())
		setDlgItemText(IDC_DIALFROM, pDialup_->getDialFrom());
	sendDlgItemMessage(IDC_SHOWDIALOG, BM_SETCHECK,
		pDialup_->isFlag(GoRoundDialup::FLAG_SHOWDIALOG) ? BST_CHECKED : BST_UNCHECKED);
	setDlgItemInt(IDC_WAITBEFOREDISCONNECT, pDialup_->getDisconnectWait());
	
	RasConnection::EntryList listEntry;
	StringListFree<RasConnection::EntryList> free(listEntry);
	RasConnection::getEntries(&listEntry);
	for (RasConnection::EntryList::const_iterator it = listEntry.begin(); it != listEntry.end(); ++it) {
		W2T(*it, ptszEntry);
		sendDlgItemMessage(IDC_ENTRY, CB_ADDSTRING,
			0, reinterpret_cast<LPARAM>(ptszEntry));
	}
	
	init(false);
	updateState();
	
	return TRUE;
}

LRESULT qm::GoRoundDialupDialog::onOk()
{
	bNoDialup_ = sendDlgItemMessage(IDC_NEVER, BM_GETCHECK) == BST_CHECKED;
	if (!bNoDialup_) {
		wstring_ptr wstrEntry(getDlgItemText(IDC_ENTRY));
		pDialup_->setName(*wstrEntry.get() ? wstrEntry.get() : 0);
		
		wstring_ptr wstrDialFrom(getDlgItemText(IDC_DIALFROM));
		pDialup_->setDialFrom(*wstrDialFrom.get() ? wstrDialFrom.get() : 0);
		
		unsigned int nFlags = 0;
		if (sendDlgItemMessage(IDC_WHENEVERNOTCONNECTED, BM_GETCHECK) == BST_CHECKED)
			nFlags |= GoRoundDialup::FLAG_WHENEVERNOTCONNECTED;
		if (sendDlgItemMessage(IDC_SHOWDIALOG, BM_GETCHECK) == BST_CHECKED)
			nFlags |= GoRoundDialup::FLAG_SHOWDIALOG;
		pDialup_->setFlags(nFlags);
		
		pDialup_->setDisconnectWait(getDlgItemInt(IDC_WAITBEFOREDISCONNECT));
	}
	return DefaultDialog::onOk();
}

LRESULT qm::GoRoundDialupDialog::onTypeSelect(UINT nId)
{
	updateState();
	return 0;
}

void qm::GoRoundDialupDialog::updateState()
{
	bool bEnable = sendDlgItemMessage(IDC_NEVER, BM_GETCHECK) != BST_CHECKED;
	
	UINT nIds[] = {
		IDC_ENTRY,
		IDC_SHOWDIALOG,
		IDC_DIALFROM,
		IDC_WAITBEFOREDISCONNECT
	};
	for (int n = 0; n < countof(nIds); ++n)
		Window(getDlgItem(nIds[n])).enableWindow(bEnable);
}


/****************************************************************************
*
* SignaturesDialog
*
*/

qm::SignaturesDialog::SignaturesDialog(SignatureManager* pSignatureManager,
									   AccountManager* pAccountManager,
									   Profile* pProfile) :
	AbstractListDialog<Signature, SignatureManager::SignatureList>(IDD_SIGNATURES, IDC_SIGNATURES, false),
	pSignatureManager_(pSignatureManager),
	pAccountManager_(pAccountManager),
	pProfile_(pProfile)
{
	const SignatureManager::SignatureList& l = pSignatureManager_->getSignatures();
	SignatureManager::SignatureList& list = getList();
	list.reserve(l.size());
	for (SignatureManager::SignatureList::const_iterator it = l.begin(); it != l.end(); ++it)
		list.push_back(new Signature(**it));
}

qm::SignaturesDialog::~SignaturesDialog()
{
}

INT_PTR qm::SignaturesDialog::dialogProc(UINT uMsg,
										 WPARAM wParam,
										 LPARAM lParam)
{
	BEGIN_DIALOG_HANDLER()
		HANDLE_SIZE()
	END_DIALOG_HANDLER()
	return DefaultDialog::dialogProc(uMsg, wParam, lParam);
}

wstring_ptr qm::SignaturesDialog::getLabel(const Signature* p) const
{
	StringBuffer<WSTRING> buf(p->getName());
	if (p->getAccount()) {
		buf.append(L" [");
		buf.append(p->getAccount());
		buf.append(L"]");
	}
	if (p->isDefault())
		buf.append(L" *");
	return buf.getString();
}

std::auto_ptr<Signature> qm::SignaturesDialog::create() const
{
	std::auto_ptr<Signature> pSignature(new Signature());
	SignatureDialog dialog(pSignature.get(), pAccountManager_, pProfile_);
	if (dialog.doModal(getParentPopup()) != IDOK)
		return std::auto_ptr<Signature>();
	return pSignature;
}

bool qm::SignaturesDialog::edit(Signature* p) const
{
	SignatureDialog dialog(p, pAccountManager_, pProfile_);
	return dialog.doModal(getParentPopup()) == IDOK;
}

bool qm::SignaturesDialog::save(OptionDialogContext* pContext)
{
	pSignatureManager_->setSignatures(getList());
	return pSignatureManager_->save();
}

LRESULT qm::SignaturesDialog::onSize(UINT nFlags,
									 int cx,
									 int cy)
{
	layout();
	return DefaultDialog::onSize(nFlags, cx, cy);
}

void qm::SignaturesDialog::layout()
{
#ifndef _WIN32_WCE_PSPC
	LayoutUtil::layout(this, IDC_SIGNATURES);
#endif
}


/****************************************************************************
 *
 * SignatureDialog
 *
 */

qm::SignatureDialog::SignatureDialog(Signature* pSignature,
									 AccountManager* pAccountManager,
									 Profile* pProfile) :
	DefaultDialog(IDD_SIGNATURE),
	pSignature_(pSignature),
	pAccountManager_(pAccountManager),
	pProfile_(pProfile)
{
}

qm::SignatureDialog::~SignatureDialog()
{
}

INT_PTR qm::SignatureDialog::dialogProc(UINT uMsg,
										WPARAM wParam,
										LPARAM lParam)
{
	BEGIN_DIALOG_HANDLER()
		HANDLE_SIZE()
	END_DIALOG_HANDLER()
	return DefaultDialog::dialogProc(uMsg, wParam, lParam);
}

LRESULT qm::SignatureDialog::onCommand(WORD nCode,
									   WORD nId)
{
	BEGIN_COMMAND_HANDLER()
		HANDLE_COMMAND_ID_CODE(IDC_NAME, EN_CHANGE, onNameChange)
	END_COMMAND_HANDLER()
	return DefaultDialog::onCommand(nCode, nId);
}

LRESULT qm::SignatureDialog::onDestroy()
{
#ifndef _WIN32_WCE
	RECT rect;
	getWindowRect(&rect);
	pProfile_->setInt(L"SignatureDialog", L"Width", rect.right - rect.left);
	pProfile_->setInt(L"SignatureDialog", L"Height", rect.bottom - rect.top);
#endif
	return DefaultDialog::onDestroy();
}

LRESULT qm::SignatureDialog::onInitDialog(HWND hwndFocus,
										  LPARAM lParam)
{
	setDlgItemText(IDC_NAME, pSignature_->getName());
	
	const AccountManager::AccountList& l = pAccountManager_->getAccounts();
	for (AccountManager::AccountList::const_iterator it = l.begin(); it != l.end(); ++it) {
		W2T((*it)->getName(), ptszName);
		sendDlgItemMessage(IDC_ACCOUNT, CB_ADDSTRING,
			0, reinterpret_cast<LPARAM>(ptszName));
	}
	if (pSignature_->getAccount())
		setDlgItemText(IDC_ACCOUNT, pSignature_->getAccount());
	
	if (pSignature_->isDefault())
		sendDlgItemMessage(IDC_DEFAULT, BM_SETCHECK, BST_CHECKED);
	
	wstring_ptr wstrSignature(Util::convertLFtoCRLF(pSignature_->getSignature()));
	setDlgItemText(IDC_SIGNATURE, wstrSignature.get());
	
#ifdef _WIN32_WCE
	RECT rectWorkArea;
	::SystemParametersInfo(SPI_GETWORKAREA, 0, &rectWorkArea, 0);
	int nWidth = rectWorkArea.right - rectWorkArea.left;
	int nHeight = rectWorkArea.bottom - rectWorkArea.top;
	setWindowPos(0, 0, 0, nWidth, nHeight, SWP_NOZORDER | SWP_NOACTIVATE);
#else
	int nWidth = pProfile_->getInt(L"SignatureDialog", L"Width", 620);
	int nHeight = pProfile_->getInt(L"SignatureDialog", L"Height", 450);
	setWindowPos(0, 0, 0, nWidth, nHeight,
		SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE);
#endif
	
	init(false);
	updateState();
	
	return TRUE;
}

LRESULT qm::SignatureDialog::onOk()
{
	wstring_ptr wstrName(getDlgItemText(IDC_NAME));
	
	wstring_ptr wstrAccount(getDlgItemText(IDC_ACCOUNT));
	const WCHAR* pwszAccount = 0;
	std::auto_ptr<RegexPattern> pAccount;
	if (*wstrAccount.get()) {
		pAccount = RegexCompiler().compile(wstrAccount.get());
		if (!pAccount.get())
			return 0;
		pwszAccount = wstrAccount.get();
	}
	
	bool bDefault = sendDlgItemMessage(IDC_DEFAULT, BM_GETCHECK) == BST_CHECKED;
	
	wstring_ptr wstrSignature(getDlgItemText(IDC_SIGNATURE));
	wstrSignature = Util::convertCRLFtoLF(wstrSignature.get());
	
	pSignature_->setName(wstrName.get());
	pSignature_->setAccount(pwszAccount, pAccount);
	pSignature_->setDefault(bDefault);
	pSignature_->setSignature(wstrSignature.get());
	
	return DefaultDialog::onOk();
}

LRESULT qm::SignatureDialog::onSize(UINT nFlags,
									int cx,
									int cy)
{
	layout();
	return DefaultDialog::onSize(nFlags, cx, cy);
}

LRESULT qm::SignatureDialog::onNameChange()
{
	updateState();
	return 0;
}

void qm::SignatureDialog::updateState()
{
	Window(getDlgItem(IDOK)).enableWindow(
		Window(getDlgItem(IDC_NAME)).getWindowTextLength() != 0);
}

void qm::SignatureDialog::layout()
{
#ifndef _WIN32_WCE_PSPC
	RECT rect;
	getClientRect(&rect);
	
#ifndef _WIN32_WCE
	RECT rectButton;
	Window(getDlgItem(IDOK)).getWindowRect(&rectButton);
	int nButtonWidth = rectButton.right - rectButton.left;
	int nButtonHeight = rectButton.bottom - rectButton.top;
#else
	int nButtonHeight = -5;
#endif
	
	RECT rectName;
	Window(getDlgItem(IDC_NAME)).getWindowRect(&rectName);
	screenToClient(&rectName);
	
	RECT rectText;
	Window(getDlgItem(IDC_SIGNATURE)).getWindowRect(&rectText);
	screenToClient(&rectText);
	
	HDWP hdwp = beginDeferWindowPos(6);
	hdwp = Window(getDlgItem(IDC_NAME)).deferWindowPos(hdwp, 0, 0, 0,
		rect.right - rectName.left - 5, rectName.bottom - rectName.top,
		SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE);
	hdwp = Window(getDlgItem(IDC_ACCOUNT)).deferWindowPos(hdwp, 0, 0, 0,
		rect.right - rectName.left - 5, rectName.bottom - rectName.top,
		SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE);
	hdwp = Window(getDlgItem(IDC_SIGNATURE)).deferWindowPos(hdwp, 0, 0, 0,
		rect.right - rectText.left - 5, rect.bottom - rectText.top - nButtonHeight - 10,
		SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE);
	
#ifndef _WIN32_WCE
	hdwp = Window(getDlgItem(IDOK)).deferWindowPos(hdwp, 0,
		rect.right - (nButtonWidth + 5)*2 - 15, rect.bottom - nButtonHeight - 5,
		nButtonWidth, nButtonHeight, SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE);
	hdwp = Window(getDlgItem(IDCANCEL)).deferWindowPos(hdwp, 0,
		rect.right - (nButtonWidth + 5) - 15, rect.bottom - nButtonHeight - 5,
		nButtonWidth, nButtonHeight, SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE);
	
	hdwp = Window(getDlgItem(IDC_SIZEGRIP)).deferWindowPos(hdwp, 0,
		rect.right - rect.left - 13, rect.bottom - rect.top - 12,
		13, 12, SWP_NOZORDER | SWP_NOACTIVATE);
#endif
	
	endDeferWindowPos(hdwp);
#endif
}


/****************************************************************************
*
* SyncFilterSetsDialog
*
*/

qm::SyncFilterSetsDialog::SyncFilterSetsDialog(SyncFilterManager* pSyncFilterManager,
											   Profile* pProfile) :
	AbstractListDialog<SyncFilterSet, SyncFilterManager::FilterSetList>(IDD_SYNCFILTERSETS, IDC_FILTERSETS, false),
	pSyncFilterManager_(pSyncFilterManager),
	pProfile_(pProfile)
{
	const SyncFilterManager::FilterSetList& l = pSyncFilterManager->getFilterSets();
	SyncFilterManager::FilterSetList& list = getList();
	list.reserve(l.size());
	for (SyncFilterManager::FilterSetList::const_iterator it = l.begin(); it != l.end(); ++it)
		list.push_back(new SyncFilterSet(**it));
}

qm::SyncFilterSetsDialog::~SyncFilterSetsDialog()
{
}

INT_PTR qm::SyncFilterSetsDialog::dialogProc(UINT uMsg,
											 WPARAM wParam,
											 LPARAM lParam)
{
	BEGIN_DIALOG_HANDLER()
		HANDLE_SIZE()
	END_DIALOG_HANDLER()
	return DefaultDialog::dialogProc(uMsg, wParam, lParam);
}

wstring_ptr qm::SyncFilterSetsDialog::getLabel(const SyncFilterSet* p) const
{
	return allocWString(p->getName());
}

std::auto_ptr<SyncFilterSet> qm::SyncFilterSetsDialog::create() const
{
	std::auto_ptr<SyncFilterSet> pFilterSet(new SyncFilterSet());
	SyncFiltersDialog dialog(pFilterSet.get(), pProfile_);
	if (dialog.doModal(getParentPopup()) != IDOK)
		return std::auto_ptr<SyncFilterSet>();
	return pFilterSet;
}

bool qm::SyncFilterSetsDialog::edit(SyncFilterSet* p) const
{
	SyncFiltersDialog dialog(p, pProfile_);
	return dialog.doModal(getParentPopup()) == IDOK;
}

bool qm::SyncFilterSetsDialog::save(OptionDialogContext* pContext)
{
	pSyncFilterManager_->setFilterSets(getList());
	return pSyncFilterManager_->save();
}

LRESULT qm::SyncFilterSetsDialog::onSize(UINT nFlags,
										 int cx,
										 int cy)
{
	layout();
	return DefaultDialog::onSize(nFlags, cx, cy);
}

void qm::SyncFilterSetsDialog::layout()
{
#ifndef _WIN32_WCE_PSPC
	LayoutUtil::layout(this, IDC_FILTERSETS);
#endif
}


/****************************************************************************
*
* SyncFiltersDialog
*
*/

qm::SyncFiltersDialog::SyncFiltersDialog(SyncFilterSet* pSyncFilterSet,
										 Profile* pProfile) :
	AbstractListDialog<SyncFilter, SyncFilterSet::FilterList>(IDD_SYNCFILTERS, IDC_FILTERS, true),
	pSyncFilterSet_(pSyncFilterSet),
	pProfile_(pProfile)
{
	const SyncFilterSet::FilterList& l = pSyncFilterSet->getFilters();
	SyncFilterSet::FilterList& list = getList();
	list.reserve(l.size());
	for (SyncFilterSet::FilterList::const_iterator it = l.begin(); it != l.end(); ++it)
		list.push_back(new SyncFilter(**it));
}

qm::SyncFiltersDialog::~SyncFiltersDialog()
{
}

INT_PTR qm::SyncFiltersDialog::dialogProc(UINT uMsg,
										  WPARAM wParam,
										  LPARAM lParam)
{
	BEGIN_DIALOG_HANDLER()
		HANDLE_SIZE()
	END_DIALOG_HANDLER()
	return AbstractListDialog<SyncFilter, SyncFilterSet::FilterList>::dialogProc(uMsg, wParam, lParam);
}

LRESULT qm::SyncFiltersDialog::onCommand(WORD nCode,
										 WORD nId)
{
	BEGIN_COMMAND_HANDLER()
		HANDLE_COMMAND_ID_CODE(IDC_NAME, EN_CHANGE, onNameChange)
	END_COMMAND_HANDLER()
	return AbstractListDialog<SyncFilter, SyncFilterSet::FilterList>::onCommand(nCode, nId);
}

LRESULT qm::SyncFiltersDialog::onDestroy()
{
#ifndef _WIN32_WCE
	RECT rect;
	getWindowRect(&rect);
	pProfile_->setInt(L"SyncFiltersDialog", L"Width", rect.right - rect.left);
	pProfile_->setInt(L"SyncFiltersDialog", L"Height", rect.bottom - rect.top);
#endif
	
	return AbstractListDialog<SyncFilter, SyncFilterSet::FilterList>::onDestroy();
}

LRESULT qm::SyncFiltersDialog::onInitDialog(HWND hwndFocus,
											LPARAM lParam)
{
	setDlgItemText(IDC_NAME, pSyncFilterSet_->getName());
	
#ifdef _WIN32_WCE
	RECT rectWorkArea;
	::SystemParametersInfo(SPI_GETWORKAREA, 0, &rectWorkArea, 0);
	int nWidth = rectWorkArea.right - rectWorkArea.left;
	int nHeight = rectWorkArea.bottom - rectWorkArea.top;
	setWindowPos(0, 0, 0, nWidth, nHeight, SWP_NOZORDER | SWP_NOACTIVATE);
#else
	int nWidth = pProfile_->getInt(L"SyncFiltersDialog", L"Width", 620);
	int nHeight = pProfile_->getInt(L"SyncFiltersDialog", L"Height", 450);
	setWindowPos(0, 0, 0, nWidth, nHeight,
		SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE);
#endif
	
	return AbstractListDialog<SyncFilter, SyncFilterSet::FilterList>::onInitDialog(hwndFocus, lParam);
}

LRESULT qm::SyncFiltersDialog::onOk()
{
	wstring_ptr wstrName(getDlgItemText(IDC_NAME));
	
	pSyncFilterSet_->setName(wstrName.get());
	pSyncFilterSet_->setFilters(getList());
	
	return AbstractListDialog<SyncFilter, SyncFilterSet::FilterList>::onOk();
}

wstring_ptr qm::SyncFiltersDialog::getLabel(const SyncFilter* p) const
{
	StringBuffer<WSTRING> buf;
	
	const WCHAR* pwszFolder = p->getFolder();
	if (pwszFolder) {
		buf.append(L'[');
		buf.append(pwszFolder);
		buf.append(L"] ");
	}
	
	wstring_ptr wstrCondition(p->getCondition()->getString());
	buf.append(wstrCondition.get());
	
	return buf.getString();
}

std::auto_ptr<SyncFilter> qm::SyncFiltersDialog::create() const
{
	std::auto_ptr<SyncFilter> pFilter(new SyncFilter());
	SyncFilterDialog dialog(pFilter.get());
	if (dialog.doModal(getHandle()) != IDOK)
		return std::auto_ptr<SyncFilter>();
	return pFilter;
}

bool qm::SyncFiltersDialog::edit(SyncFilter* p) const
{
	SyncFilterDialog dialog(p);
	return dialog.doModal(getHandle()) == IDOK;
}

void qm::SyncFiltersDialog::updateState()
{
	AbstractListDialog<SyncFilter, SyncFilterSet::FilterList>::updateState();
	
	Window(getDlgItem(IDOK)).enableWindow(
		Window(getDlgItem(IDC_NAME)).getWindowTextLength() != 0);
}

LRESULT qm::SyncFiltersDialog::onSize(UINT nFlags,
									  int cx,
									  int cy)
{
	layout();
	return AbstractListDialog<SyncFilter, SyncFilterSet::FilterList>::onSize(nFlags, cx, cy);
}

LRESULT qm::SyncFiltersDialog::onNameChange()
{
	updateState();
	return 0;
}

void qm::SyncFiltersDialog::layout()
{
#ifndef _WIN32_WCE_PSPC
	RECT rect;
	getClientRect(&rect);
	
	RECT rectName;
	Window(getDlgItem(IDC_NAME)).getWindowRect(&rectName);
	screenToClient(&rectName);
	
#ifndef _WIN32_WCE
	RECT rectButton;
	Window(getDlgItem(IDOK)).getWindowRect(&rectButton);
	int nButtonWidth = rectButton.right - rectButton.left;
	int nButtonHeight = rectButton.bottom - rectButton.top;
#else
	int nButtonHeight = -5;
#endif
	
	HDWP hdwp = beginDeferWindowPos(10);
	
	hdwp = Window(getDlgItem(IDC_NAME)).deferWindowPos(hdwp, 0, 0, 0,
		rect.right - rectName.left - 5, rectName.bottom - rectName.top,
		SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE);
	
	hdwp = LayoutUtil::layout(this, IDC_FILTERS, hdwp,
		rectName.bottom, nButtonHeight + 5);
	
#ifndef _WIN32_WCE
	hdwp = Window(getDlgItem(IDOK)).deferWindowPos(hdwp, 0,
		rect.right - (nButtonWidth + 5)*2 - 15, rect.bottom - nButtonHeight - 5,
		nButtonWidth, nButtonHeight, SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE);
	hdwp = Window(getDlgItem(IDCANCEL)).deferWindowPos(hdwp, 0,
		rect.right - (nButtonWidth + 5) - 15, rect.bottom - nButtonHeight - 5,
		nButtonWidth, nButtonHeight, SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE);
	hdwp = Window(getDlgItem(IDC_SIZEGRIP)).deferWindowPos(hdwp, 0,
		rect.right - rect.left - 13, rect.bottom - rect.top - 12,
		13, 12, SWP_NOZORDER | SWP_NOACTIVATE);
#endif
	
	endDeferWindowPos(hdwp);
#endif
}


/****************************************************************************
 *
 * SyncFilterDialog
 *
 */

qm::SyncFilterDialog::SyncFilterDialog(SyncFilter* pSyncFilter) :
	DefaultDialog(IDD_SYNCFILTER),
	pSyncFilter_(pSyncFilter)
{
}

qm::SyncFilterDialog::~SyncFilterDialog()
{
}

LRESULT qm::SyncFilterDialog::onCommand(WORD nCode,
										WORD nId)
{
	BEGIN_COMMAND_HANDLER()
		HANDLE_COMMAND_ID(IDC_EDIT, onEdit)
		HANDLE_COMMAND_ID_CODE(IDC_ACTION, CBN_SELCHANGE, onActionSelChange)
	END_COMMAND_HANDLER()
	return DefaultDialog::onCommand(nCode, nId);
}

LRESULT qm::SyncFilterDialog::onInitDialog(HWND hwndFocus,
										   LPARAM lParam)
{
	wstring_ptr wstrCondition(pSyncFilter_->getCondition()->getString());
	setDlgItemText(IDC_CONDITION, wstrCondition.get());
	
	setDlgItemText(IDC_FOLDER, pSyncFilter_->getFolder());
	
	const TCHAR* ptszActions[] = {
		_T("Download (POP3)"),
		_T("Download (IMAP4)"),
		_T("Download (NNTP)"),
		_T("Delete (POP3, IMAP4)"),
		_T("Ignore (POP3, NNTP)")
	};
	for (int n = 0; n < countof(ptszActions); ++n)
		sendDlgItemMessage(IDC_ACTION, CB_ADDSTRING,
			0, reinterpret_cast<LPARAM>(ptszActions[n]));
	
	const TCHAR* ptszTypes[] = {
		_T("All"),
		_T("Text"),
		_T("Html"),
		_T("Header")
	};
	for (int n = 0; n < countof(ptszTypes); ++n)
		sendDlgItemMessage(IDC_TYPE, CB_ADDSTRING,
			0, reinterpret_cast<LPARAM>(ptszTypes[n]));
	
	int nAction = 0;
	int nMaxLine = 0;
	int nType = 0;
	
	const SyncFilter::ActionList& listAction = pSyncFilter_->getActions();
	if (!listAction.empty()) {
		const SyncFilterAction* pAction = listAction.front();
		const WCHAR* pwszAction = pAction->getName();
		if (wcscmp(pwszAction, L"download") == 0) {
			const WCHAR* pwszProtocol = L"pop3";
			if (pAction->getParam(L"line"))
				pwszProtocol = L"pop3";
			else if (pAction->getParam(L"type"))
				pwszProtocol = L"imap4";
			else
				pwszProtocol = L"nntp";
			
			if (wcscmp(pwszProtocol, L"imap4") == 0) {
				nAction = 1;
				const WCHAR* pwszType = pAction->getParam(L"type");
				if (pwszType) {
					const WCHAR* pwszTypes[] = {
						L"all",
						L"text",
						L"html",
						L"header"
					};
					for (int n = 0; n < countof(pwszTypes); ++n) {
						if (wcscmp(pwszType, pwszTypes[n]) == 0) {
							nType = n;
							break;
						}
					}
				}
			}
			else if (wcscmp(pwszProtocol, L"nntp") == 0) {
				nAction = 2;
			}
			else {
				const WCHAR* pwszLine = pAction->getParam(L"line");
				if (pwszLine) {
					WCHAR* pEnd = 0;
					long nLine = wcstol(pwszLine, &pEnd, 10);
					if (!*pEnd)
						nMaxLine = nLine;
				}
			}
		}
		else if (wcscmp(pwszAction, L"delete") == 0) {
			nAction = 3;
		}
		else if (wcscmp(pwszAction, L"ignore") == 0) {
			nAction = 4;
		}
	}
	
	sendDlgItemMessage(IDC_ACTION, CB_SETCURSEL, nAction);
	setDlgItemInt(IDC_MAXLINE, nMaxLine);
	sendDlgItemMessage(IDC_TYPE, CB_SETCURSEL, nType);
	
	init(false);
	updateState();
	
	return TRUE;
}

LRESULT qm::SyncFilterDialog::onOk()
{
	wstring_ptr wstrCondition(getDlgItemText(IDC_CONDITION));
	std::auto_ptr<Macro> pCondition(MacroParser().parse(wstrCondition.get()));
	if (!pCondition.get()) {
		// TODO MSG
		return 0;
	}
	
	wstring_ptr wstrFolder(getDlgItemText(IDC_FOLDER));
	const WCHAR* pwszFolder = 0;
	std::auto_ptr<RegexPattern> pFolder;
	if (*wstrFolder.get()) {
		pFolder = RegexCompiler().compile(wstrFolder.get());
		if (!pFolder.get()) {
			// TODO MSG
			return 0;
		}
		pwszFolder = wstrFolder.get();
	}
	
	int nAction = sendDlgItemMessage(IDC_ACTION, CB_GETCURSEL);
	if (nAction == CB_ERR) {
		// TODO MSG
		return 0;
	}
	
	const WCHAR* pwszName = 0;
	switch (nAction) {
	case 0:
	case 1:
	case 2:
		pwszName = L"download";
		break;
	case 3:
		pwszName = L"delete";
		break;
	case 4:
		pwszName = L"ignore";
		break;
	default:
		assert(false);
		return 0;
	}
	
	std::auto_ptr<SyncFilterAction> pAction(new SyncFilterAction(pwszName));
	
	switch (nAction) {
	case 0:
		{
			int nLine = getDlgItemInt(IDC_MAXLINE);
			wstring_ptr wstrLine(allocWString(32));
			swprintf(wstrLine.get(), L"%d", nLine);
			pAction->addParam(allocWString(L"line"), wstrLine);
		}
		break;
	case 1:
		{
			int nType = sendDlgItemMessage(IDC_TYPE, CB_GETCURSEL);
			if (nType == CB_ERR) {
				// TODO MSG
				return 0;
			}
			const WCHAR* pwszTypes[] = {
				L"all",
				L"text",
				L"html",
				L"header"
			};
			pAction->addParam(allocWString(L"type"), allocWString(pwszTypes[nType]));
		}
		break;
	default:
		break;
	}
	
	SyncFilter::ActionList listAction(1, pAction.get());
	pAction.release();
	
	const SyncFilter::ActionList& l = pSyncFilter_->getActions();
	listAction.reserve(l.size());
	for (SyncFilter::ActionList::size_type n = 1; n < l.size(); ++n)
		listAction.push_back(new SyncFilterAction(*l[n]));
	
	pSyncFilter_->setFolder(pwszFolder, pFolder);
	pSyncFilter_->setCondition(pCondition);
	pSyncFilter_->setActions(listAction);
	
	return DefaultDialog::onOk();
}

LRESULT qm::SyncFilterDialog::onEdit()
{
	wstring_ptr wstrCondition(getDlgItemText(IDC_CONDITION));
	ConditionDialog dialog(wstrCondition.get());
	if (dialog.doModal(getHandle()) == IDOK)
		setDlgItemText(IDC_CONDITION, dialog.getCondition());
	return 0;
}

LRESULT qm::SyncFilterDialog::onActionSelChange()
{
	updateState();
	return 0;
}

void qm::SyncFilterDialog::updateState()
{
	int nItem = sendDlgItemMessage(IDC_ACTION, CB_GETCURSEL);
	Window(getDlgItem(IDC_MAXLINELABEL)).showWindow(nItem == 0 ? SW_SHOW : SW_HIDE);
	Window(getDlgItem(IDC_MAXLINE)).showWindow(nItem == 0 ? SW_SHOW : SW_HIDE);
	Window(getDlgItem(IDC_TYPELABEL)).showWindow(nItem == 1 ? SW_SHOW : SW_HIDE);
	Window(getDlgItem(IDC_TYPE)).showWindow(nItem == 1 ? SW_SHOW : SW_HIDE);
}


/****************************************************************************
 *
 * LayoutUtil
 *
 */

void qm::LayoutUtil::layout(Window* pWindow,
							UINT nId)
{
	HDWP hdwp = Window::beginDeferWindowPos(6);
	hdwp = layout(pWindow, nId, hdwp, 0, 0);
	Window::endDeferWindowPos(hdwp);
}

HDWP qm::LayoutUtil::layout(qs::Window* pWindow,
							UINT nId,
							HDWP hdwp,
							int nTopMargin,
							int nBottomMargin)
{
	RECT rect;
	pWindow->getClientRect(&rect);
	
	RECT rectButton;
	Window(pWindow->getDlgItem(IDC_ADD)).getWindowRect(&rectButton);
	
	int nButtonWidth = rectButton.right - rectButton.left;
	int nButtonHeight = rectButton.bottom - rectButton.top;
	
	hdwp = Window(pWindow->getDlgItem(nId)).deferWindowPos(
		hdwp, 0, 5, nTopMargin + 5, rect.right - nButtonWidth - 15,
		rect.bottom - nTopMargin - nBottomMargin - 10,
		SWP_NOZORDER | SWP_NOACTIVATE);
	
	UINT nIds[] = {
		IDC_ADD,
		IDC_REMOVE,
		IDC_EDIT,
		IDC_UP,
		IDC_DOWN
	};
	int nTop = nTopMargin + 5;
	for (int n = 0; n < countof(nIds); ++n) {
		hdwp = Window(pWindow->getDlgItem(nIds[n])).deferWindowPos(hdwp,
			0, rect.right - nButtonWidth - 5, nTop, nButtonWidth,
			nButtonHeight, SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE);
		nTop += nButtonHeight + (n == 2 ? 5 : 2);
	}
	
	return hdwp;
}
