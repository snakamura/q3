/*
 * $Id: editframewindow.cpp,v 1.13 2003/05/31 08:04:51 snakamura Exp $
 *
 * Copyright(C) 1998-2003 Satoshi Nakamura
 * All rights reserved.
 *
 */

#include <qmapplication.h>
#include <qmdocument.h>
#include <qmeditwindow.h>

#include <qsaccelerator.h>
#include <qsaction.h>
#include <qsnew.h>

#include <algorithm>

#ifdef _WIN32_WCE_PSPC
#	include <aygshell.h>
#endif

#include "editframewindow.h"
#include "editwindow.h"
#include "keymap.h"
#include "menus.h"
#include "resourceinc.h"
#include "statusbar.h"
#include "../action/action.h"
#include "../action/editaction.h"
#include "../action/findreplace.h"
#include "../model/editmessage.h"
#include "../model/security.h"

#pragma warning(disable:4786)

using namespace qm;
using namespace qs;


/****************************************************************************
 *
 * EditFrameWindowImpl
 *
 */

class qm::EditFrameWindowImpl
{
public:
	enum {
		ID_EDITWINDOW			= 1001,
		ID_TOOLBAR				= 1002,
		ID_STATUSBAR			= 1003,
		ID_COMMANDBARMENU		= 1004,
		ID_COMMANDBARBUTTON		= 1005
	};

public:
	QSTATUS initActions();
	QSTATUS layoutChildren();
	QSTATUS layoutChildren(int cx, int cy);

public:
	EditFrameWindow* pThis_;
	
	bool bShowToolbar_;
	bool bShowStatusBar_;
	
	EditFrameWindowManager* pManager_;
	Profile* pProfile_;
	Document* pDocument_;
	EditWindow* pEditWindow_;
	StatusBar* pStatusBar_;
	Accelerator* pAccelerator_;
	ActionMap* pActionMap_;
	ActionInvoker* pActionInvoker_;
	FindReplaceManager* pFindReplaceManager_;
	ScriptMenu* pScriptMenu_;
	bool bIme_;
	bool bCreated_;
	bool bMaximize_;
	bool bLayouting_;
};

QSTATUS qm::EditFrameWindowImpl::initActions()
{
	DECLARE_QSTATUS();
	
	status = newQsObject(&pActionMap_);
	CHECK_QSTATUS();
	status = newQsObject(pActionMap_, &pActionInvoker_);
	CHECK_QSTATUS();
	status = newQsObject(&pFindReplaceManager_);
	CHECK_QSTATUS();
	
	status = InitAction3<EditEditCommandAction, EditWindow*,
		EditEditCommandAction::PFN_DO, EditEditCommandAction::PFN_CANDO>(
		pActionMap_, IDM_EDIT_COPY, pEditWindow_,
		&EditWindowItem::copy, &EditWindowItem::canCopy);
	CHECK_QSTATUS();
	status = InitAction3<EditEditCommandAction, EditWindow*,
		EditEditCommandAction::PFN_DO, EditEditCommandAction::PFN_CANDO>(
		pActionMap_, IDM_EDIT_CUT, pEditWindow_,
		&EditWindowItem::cut, &EditWindowItem::canCut);
	CHECK_QSTATUS();
	status = InitAction3<EditEditFindAction, TextWindow*,
		Profile*, FindReplaceManager*>(
		pActionMap_, IDM_EDIT_FIND, pEditWindow_->getTextWindow(),
		pProfile_, pFindReplaceManager_);
	CHECK_QSTATUS();
	status = InitAction3<EditEditFindAction, TextWindow*,
		bool, FindReplaceManager*>(
		pActionMap_, IDM_EDIT_FINDNEXT, pEditWindow_->getTextWindow(),
		true, pFindReplaceManager_);
	CHECK_QSTATUS();
	status = InitAction3<EditEditFindAction, TextWindow*,
		bool, FindReplaceManager*>(
		pActionMap_, IDM_EDIT_FINDPREV, pEditWindow_->getTextWindow(),
		false, pFindReplaceManager_);
	CHECK_QSTATUS();
	status = InitAction3<EditEditCommandAction, EditWindow*,
		EditEditCommandAction::PFN_DO, EditEditCommandAction::PFN_CANDO>(
		pActionMap_, IDM_EDIT_PASTE, pEditWindow_,
		&EditWindowItem::paste, &EditWindowItem::canPaste);
	CHECK_QSTATUS();
	status = InitAction2<EditEditPasteWithQuoteAction, TextWindow*, Profile*>(
		pActionMap_, IDM_EDIT_PASTEWITHQUOTE, pEditWindow_->getTextWindow(), pProfile_);
	CHECK_QSTATUS();
	status = InitAction3<EditEditCommandAction, EditWindow*,
		EditEditCommandAction::PFN_DO, EditEditCommandAction::PFN_CANDO>(
		pActionMap_, IDM_EDIT_REDO, pEditWindow_,
		&EditWindowItem::redo, &EditWindowItem::canRedo);
	CHECK_QSTATUS();
	status = InitAction3<EditEditReplaceAction, TextWindow*,
		Profile*, FindReplaceManager*>(
		pActionMap_, IDM_EDIT_REPLACE, pEditWindow_->getTextWindow(),
		pProfile_, pFindReplaceManager_);
	CHECK_QSTATUS();
	status = InitAction3<EditEditCommandAction, EditWindow*,
		EditEditCommandAction::PFN_DO, EditEditCommandAction::PFN_CANDO>(
		pActionMap_, IDM_EDIT_SELECTALL, pEditWindow_,
		&EditWindowItem::selectAll, &EditWindowItem::canSelectAll);
	CHECK_QSTATUS();
	status = InitAction3<EditEditCommandAction, EditWindow*,
		EditEditCommandAction::PFN_DO, EditEditCommandAction::PFN_CANDO>(
		pActionMap_, IDM_EDIT_UNDO, pEditWindow_,
		&EditWindowItem::undo, &EditWindowItem::canUndo);
	CHECK_QSTATUS();
	status = InitAction1<ViewShowStatusBarAction<EditFrameWindow>, EditFrameWindow*>(
		pActionMap_, IDM_VIEW_SHOWSTATUSBAR, pThis_);
	CHECK_QSTATUS();
	status = InitAction1<ViewShowToolbarAction<EditFrameWindow>, EditFrameWindow*>(
		pActionMap_, IDM_VIEW_SHOWTOOLBAR, pThis_);
	CHECK_QSTATUS();
	
	struct {
		UINT nId_;
		TextWindow::MoveCaret moveCaret_;
	} moveCaretActions[] = {
		{ IDM_EDIT_MOVECHARLEFT,	TextWindow::MOVECARET_CHARLEFT	},
		{ IDM_EDIT_MOVECHARRIGHT,	TextWindow::MOVECARET_CHARRIGHT	},
		{ IDM_EDIT_MOVELINESTART,	TextWindow::MOVECARET_LINESTART	},
		{ IDM_EDIT_MOVELINEEND,		TextWindow::MOVECARET_LINEEND	},
		{ IDM_EDIT_MOVELINEUP,		TextWindow::MOVECARET_LINEUP	},
		{ IDM_EDIT_MOVELINEDOWN,	TextWindow::MOVECARET_LINEDOWN	},
		{ IDM_EDIT_MOVEPAGEUP,		TextWindow::MOVECARET_PAGEUP	},
		{ IDM_EDIT_MOVEPAGEDOWN,	TextWindow::MOVECARET_PAGEDOWN	},
		{ IDM_EDIT_MOVEDOCSTART,	TextWindow::MOVECARET_DOCSTART	},
		{ IDM_EDIT_MOVEDOCEND,		TextWindow::MOVECARET_DOCEND	}
	};
	for (int n = 0; n < countof(moveCaretActions); ++n) {
		status = InitAction2<EditEditMoveCaretAction, TextWindow*, TextWindow::MoveCaret>(
			pActionMap_, moveCaretActions[n].nId_, pEditWindow_->getTextWindow(),
			moveCaretActions[n].moveCaret_);
		CHECK_QSTATUS();
	}
	
	status = InitAction1<FileCloseAction, HWND>(
		pActionMap_, IDM_FILE_CLOSE, pThis_->getHandle());
	CHECK_QSTATUS();
#if defined _WIN32_WCE && _WIN32_WCE >= 300 && defined _WIN32_WCE_PSPC
	status = InitAction1<FileCloseAction, HWND>(
		pActionMap_, IDOK, pThis_->getHandle());
	CHECK_QSTATUS();
#endif
	status = InitAction5<EditFileSendAction, unsigned int,
		Document*, EditMessageHolder*, EditFrameWindow*, Profile*>(
		pActionMap_, IDM_FILE_DRAFT, EditFileSendAction::FLAG_DRAFT,
		pDocument_, pEditWindow_->getEditMessageHolder(), pThis_, pProfile_);
	CHECK_QSTATUS();
	status = InitAction1<EditFileInsertAction, TextWindow*>(
		pActionMap_, IDM_FILE_INSERT, pEditWindow_->getTextWindow());
	CHECK_QSTATUS();
	status = InitAction2<EditFileOpenAction, EditMessageHolder*, HWND>(
		pActionMap_, IDM_FILE_OPEN, pEditWindow_->getEditMessageHolder(),
		pThis_->getHandle());
	CHECK_QSTATUS();
	status = InitAction2<EditFileSaveAction, EditMessageHolder*, HWND>(
		pActionMap_, IDM_FILE_SAVE, pEditWindow_->getEditMessageHolder(),
		pThis_->getHandle());
	CHECK_QSTATUS();
	status = InitAction5<EditFileSendAction, unsigned int,
		Document*, EditMessageHolder*, EditFrameWindow*, Profile*>(
		pActionMap_, IDM_FILE_SEND, 0, pDocument_,
		pEditWindow_->getEditMessageHolder(), pThis_, pProfile_);
	CHECK_QSTATUS();
	status = InitAction5<EditFileSendAction, unsigned int,
		Document*, EditMessageHolder*, EditFrameWindow*, Profile*>(
		pActionMap_, IDM_FILE_SENDNOW, EditFileSendAction::FLAG_NOW,
		pDocument_, pEditWindow_->getEditMessageHolder(), pThis_, pProfile_);
	CHECK_QSTATUS();
	status = InitActionRange1<EditFocusItemAction, EditWindow*>(
		pActionMap_, IDM_FOCUS_HEADEREDITITEM,
		IDM_FOCUS_HEADEREDITITEM + 10, pEditWindow_);
	CHECK_QSTATUS();
	status = InitAction4<EditToolAddressBookAction,
		EditMessageHolder*, HWND, AddressBook*, Profile*>(
		pActionMap_, IDM_TOOL_ADDRESSBOOK, pEditWindow_->getEditMessageHolder(),
		pThis_->getHandle(), pDocument_->getAddressBook(), pProfile_);
	CHECK_QSTATUS();
	status = InitAction2<EditToolAttachmentAction, EditMessageHolder*, HWND>(
		pActionMap_, IDM_TOOL_ATTACHMENT, pEditWindow_->getEditMessageHolder(),
		pThis_->getHandle());
	CHECK_QSTATUS();
	status = InitAction4<EditToolFlagAction, EditMessageHolder*,
		EditToolFlagAction::PFN_IS, EditToolFlagAction::PFN_SET, bool>(
		pActionMap_, IDM_TOOL_ENCRYPT, pEditWindow_->getEditMessageHolder(),
		&EditMessage::isEncrypt, &EditMessage::setEncrypt, Security::isEnabled());
	CHECK_QSTATUS();
	status = InitAction2<EditToolInsertSignatureAction,
		EditMessageHolder*, TextWindow*>(
		pActionMap_, IDM_TOOL_INSERTSIGNATURE,
		pEditWindow_->getEditMessageHolder(), pEditWindow_->getTextWindow());
	CHECK_QSTATUS();
	status = InitAction1<EditToolInsertTextAction, TextWindow*>(
		pActionMap_, IDM_TOOL_INSERTTEXT, pEditWindow_->getTextWindow());
	CHECK_QSTATUS();
	status = InitAction1<EditToolReformAction, TextWindow*>(
		pActionMap_, IDM_TOOL_REFORM, pEditWindow_->getTextWindow());
	CHECK_QSTATUS();
	status = InitAction2<EditToolReformAllAction, TextWindow*, Profile*>(
		pActionMap_, IDM_TOOL_REFORMALL, pEditWindow_->getTextWindow(), pProfile_);
	CHECK_QSTATUS();
	status = InitAction4<EditToolFlagAction, EditMessageHolder*,
		EditToolFlagAction::PFN_IS, EditToolFlagAction::PFN_SET, bool>(
		pActionMap_, IDM_TOOL_REFORMAUTO, pEditWindow_->getEditMessageHolder(),
		&EditMessage::isAutoReform, &EditMessage::setAutoReform, true);
	CHECK_QSTATUS();
	status = InitAction4<EditToolFlagAction, EditMessageHolder*,
		EditToolFlagAction::PFN_IS, EditToolFlagAction::PFN_SET, bool>(
		pActionMap_, IDM_TOOL_SIGN, pEditWindow_->getEditMessageHolder(),
		&EditMessage::isSign, &EditMessage::setSign, Security::isEnabled());
	CHECK_QSTATUS();
	status = InitActionRange4<ToolScriptAction,
		ScriptMenu*, Document*, Profile*, EditFrameWindow*>(
		pActionMap_, IDM_TOOL_SCRIPT, IDM_TOOL_SCRIPT + 100,
		pScriptMenu_, pDocument_, pProfile_, pThis_);
	CHECK_QSTATUS();
	
	return QSTATUS_SUCCESS;
}

QSTATUS qm::EditFrameWindowImpl::layoutChildren()
{
	RECT rect;
	pThis_->getClientRect(&rect);
	return layoutChildren(rect.right - rect.left, rect.bottom - rect.top);
}

QSTATUS qm::EditFrameWindowImpl::layoutChildren(int cx, int cy)
{
	DECLARE_QSTATUS();
	
	bLayouting_ = true;
	
#if defined _WIN32_WCE && _WIN32_WCE >= 300 && defined _WIN32_WCE_PSPC
	int nToolbarHeight = 0;
#else
	HWND hwndToolbar = pThis_->getToolbar();
	RECT rectToolbar;
	Window wndToolbar(hwndToolbar);
	wndToolbar.getWindowRect(&rectToolbar);
	int nToolbarHeight = bShowToolbar_ ?
		rectToolbar.bottom - rectToolbar.top : 0;
#endif
	
	RECT rectStatusBar;
	pStatusBar_->getWindowRect(&rectStatusBar);
	int nStatusBarHeight = bShowStatusBar_ ?
		rectStatusBar.bottom - rectStatusBar.top : 0;
	
#if !defined _WIN32_WCE || _WIN32_WCE < 300 || !defined _WIN32_WCE_PSPC
	wndToolbar.setWindowPos(0, 0, 0, cx,
		rectToolbar.bottom - rectToolbar.top, SWP_NOMOVE | SWP_NOZORDER);
	wndToolbar.showWindow(bShowToolbar_ ? SW_SHOW : SW_HIDE);
#endif
	
	pStatusBar_->setWindowPos(0, 0, cy - nStatusBarHeight,
		cx, rectStatusBar.bottom - rectStatusBar.top, SWP_NOZORDER);
	pStatusBar_->showWindow(bShowStatusBar_ ? SW_SHOW : SW_HIDE);
	
	pEditWindow_->setWindowPos(0, 0, nToolbarHeight, cx,
		cy - nStatusBarHeight - nToolbarHeight, SWP_NOZORDER);
	
	bLayouting_ = false;
	
	return QSTATUS_SUCCESS;
}


/****************************************************************************
 *
 * EditFrameWindow
 *
 */

qm::EditFrameWindow::EditFrameWindow(EditFrameWindowManager* pManager,
	Profile* pProfile, QSTATUS* pstatus) :
	FrameWindow(Application::getApplication().getResourceHandle(), true, pstatus),
	pImpl_(0)
{
	if (*pstatus != QSTATUS_SUCCESS)
		return;
	
	DECLARE_QSTATUS();
	
	int nShowToolbar = 0;
	status = pProfile->getInt(
		L"MessageFrameWindow", L"ShowToolbar", 1, &nShowToolbar);
	CHECK_QSTATUS_SET(pstatus);
	int nShowStatusBar = 0;
	status = pProfile->getInt(
		L"MessageFrameWindow", L"ShowStatusBar", 1, &nShowStatusBar);
	CHECK_QSTATUS_SET(pstatus);
	
	status = newObject(&pImpl_);
	CHECK_QSTATUS_SET(pstatus);
	pImpl_->pThis_ = this;
	pImpl_->bShowToolbar_ = nShowToolbar != 0;
	pImpl_->bShowStatusBar_ = nShowStatusBar != 0;
	pImpl_->pManager_ = pManager;
	pImpl_->pProfile_ = pProfile;
	pImpl_->pDocument_ = 0;
	pImpl_->pEditWindow_ = 0;
	pImpl_->pStatusBar_ = 0;
	pImpl_->pAccelerator_ = 0;
	pImpl_->pActionMap_ = 0;
	pImpl_->pActionInvoker_ = 0;
	pImpl_->pFindReplaceManager_ = 0;
	pImpl_->pScriptMenu_ = 0;
	pImpl_->bIme_ = false;
	pImpl_->bCreated_ = false;
	pImpl_->bMaximize_ = false;
	pImpl_->bLayouting_ = false;
}

qm::EditFrameWindow::~EditFrameWindow()
{
	if (pImpl_) {
		delete pImpl_->pAccelerator_;
		delete pImpl_->pActionMap_;
		delete pImpl_->pActionInvoker_;
		delete pImpl_->pFindReplaceManager_;
		delete pImpl_->pScriptMenu_;
		delete pImpl_;
		pImpl_ = 0;
	}
}

EditWindow* qm::EditFrameWindow::getEditWindow() const
{
	return pImpl_->pEditWindow_;
}

void qm::EditFrameWindow::close()
{
	pImpl_->pManager_->close(this);
}

bool qm::EditFrameWindow::isShowToolbar() const
{
	return pImpl_->bShowToolbar_;
}

QSTATUS qm::EditFrameWindow::setShowToolbar(bool bShow)
{
	DECLARE_QSTATUS();
	
	if (bShow != pImpl_->bShowToolbar_) {
		pImpl_->bShowToolbar_ = bShow;
		status = pImpl_->layoutChildren();
	}
	
	return status;
}

bool qm::EditFrameWindow::isShowStatusBar() const
{
	return pImpl_->bShowStatusBar_;
}

QSTATUS qm::EditFrameWindow::setShowStatusBar(bool bShow)
{
	DECLARE_QSTATUS();
	
	if (bShow != pImpl_->bShowStatusBar_) {
		pImpl_->bShowStatusBar_ = bShow;
		status = pImpl_->layoutChildren();
	}
	
	return status;
}

const ActionInvoker* qm::EditFrameWindow::getActionInvoker() const
{
	return pImpl_->pActionInvoker_;
}

QSTATUS qm::EditFrameWindow::getToolbarButtons(
	Toolbar* pToolbar, bool* pbToolbar)
{
	assert(pToolbar);
	assert(pbToolbar);
	static TBBUTTON	tbButton[] = {
		{ 0,	IDM_MESSAGE_NEW,				TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0 },
		{ 1,	IDM_MESSAGE_REPLY,				TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0 },
		{ 2,	IDM_MESSAGE_REPLYALL,			TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0 },
		{ 3,	IDM_MESSAGE_FORWARD,			TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0 },
		{ 0,	0,								TBSTATE_ENABLED, TBSTYLE_SEP,	 0, 0 },
		{ 4,	IDM_EDIT_DELETE,				TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0 },
		{ 0,	0,								TBSTATE_ENABLED, TBSTYLE_SEP,	 0, 0 },
		{ 5,	IDM_MESSAGE_SEARCH,				TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0 },
		{ 0,	0,								TBSTATE_ENABLED, TBSTYLE_SEP,	 0, 0 },
		{ 6,	IDM_TOOL_SYNC,					TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0 },
		{ 7,	IDM_TOOL_GOROUND,				TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0 },
		{ 8,	IDM_TOOL_CANCEL,				TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0 },
	};
	pToolbar->ptbButton_ = tbButton;
	pToolbar->nSize_ = sizeof(tbButton)/sizeof(tbButton[0]);
	pToolbar->nId_ = EditFrameWindowImpl::ID_TOOLBAR;
	pToolbar->nBitmapId_ = IDB_TOOLBAR;
	
	*pbToolbar = true;
	
	return QSTATUS_SUCCESS;
}

#if defined _WIN32_WCE && (_WIN32_WCE < 300 || !defined _WIN32_WCE_PSPC)
QSTATUS qm::EditFrameWindow::getBarId(int n, UINT* pnId) const
{
	assert(n == 0 || n == 1);
	assert(pnId);
	static UINT nIds[] = {
		EditFrameWindowImpl::ID_COMMANDBARMENU,
		EditFrameWindowImpl::ID_COMMANDBARBUTTON
	};
	*pnId = nIds[n];
	return QSTATUS_SUCCESS;
}

QSTATUS qm::EditFrameWindow::getCommandBandsRestoreInfo(
	int n, COMMANDBANDSRESTOREINFO* pcbri) const
{
	DECLARE_QSTATUS();
	
	WCHAR wszKey[32];
	swprintf(wszKey, L"CommandBandsRestoreInfo%d", n);
	int nSize = sizeof(*pcbri);
	status = pImpl_->pProfile_->getBinary(L"EditFrameWindow", wszKey,
		reinterpret_cast<unsigned char*>(pcbri), &nSize);
	if (nSize != sizeof(*pcbri))
		pcbri->cbSize = 0;
	return status;
}

QSTATUS qm::EditFrameWindow::setCommandBandsRestoreInfo(
	int n, const COMMANDBANDSRESTOREINFO& cbri)
{
	WCHAR wszKey[32];
	swprintf(wszKey, L"CommandBandsRestoreInfo%d", n);
	return pImpl_->pProfile_->setBinary(L"EditFrameWindow", wszKey,
		reinterpret_cast<const unsigned char*>(&cbri), sizeof(cbri));
}
#endif

QSTATUS qm::EditFrameWindow::getMenuHandle(void* pCreateParam, HMENU* phmenu)
{
	assert(phmenu);
	
	DECLARE_QSTATUS();
	
	EditFrameWindowCreateContext* pContext =
		static_cast<EditFrameWindowCreateContext*>(pCreateParam);
	
	status = pContext->pMenuManager_->getMenu(
		L"editframe", true, true, phmenu);
	CHECK_QSTATUS();
	
	return QSTATUS_SUCCESS;
}

QSTATUS qm::EditFrameWindow::getIconId(UINT* pnId)
{
	assert(pnId);
	*pnId = IDI_MAINFRAME;
	return QSTATUS_SUCCESS;
}

QSTATUS qm::EditFrameWindow::getWindowClass(WNDCLASS* pwc)
{
	DECLARE_QSTATUS();
	
	status = DefaultWindowHandler::getWindowClass(pwc);
	CHECK_QSTATUS();
	
	pwc->hIcon = ::LoadIcon(Application::getApplication().getResourceHandle(),
		MAKEINTRESOURCE(IDI_MAINFRAME));
	pwc->hbrBackground = reinterpret_cast<HBRUSH>(COLOR_BTNFACE + 1);
	
	return QSTATUS_SUCCESS;
}

QSTATUS qm::EditFrameWindow::preCreateWindow(CREATESTRUCT* pCreateStruct)
{
	DECLARE_QSTATUS();
	
#if defined _WIN32_WCE && _WIN32_WCE >= 300 && defined _WIN32_WCE_PSPC
#define MENU_HEIGHT 26
	SIPINFO si = { sizeof(si) };
	::SHSipInfo(SPI_GETSIPINFO, 0, &si, 0);
	pCreateStruct->cx = si.rcVisibleDesktop.right - si.rcVisibleDesktop.left;
	pCreateStruct->cy = si.rcVisibleDesktop.bottom -
		si.rcVisibleDesktop.top - (si.fdwFlags & SIPF_ON ? 0 : MENU_HEIGHT);
#elif !defined _WIN32_WCE
	WINDOWPLACEMENT wp;
	int nSize = sizeof(wp);
	status = pImpl_->pProfile_->getBinary(L"EditFrameWindow", L"WindowPlacement",
		reinterpret_cast<unsigned char*>(&wp), &nSize);
	CHECK_QSTATUS();
	if (nSize == sizeof(wp)) {
		RECT rect;
		::SystemParametersInfo(SPI_GETWORKAREA, 0, &rect, 0);
		pCreateStruct->x = wp.rcNormalPosition.left + rect.left;
		pCreateStruct->y = wp.rcNormalPosition.top + rect.top;
		pCreateStruct->cx = wp.rcNormalPosition.right - wp.rcNormalPosition.left;
		pCreateStruct->cy = wp.rcNormalPosition.bottom - wp.rcNormalPosition.top;
		switch (wp.showCmd) {
		case SW_MAXIMIZE:
//		case SW_SHOWMAXIMIZED:
			pCreateStruct->style |= WS_MAXIMIZE;
			pImpl_->bMaximize_ = true;
			break;
		case SW_MINIMIZE:
		case SW_SHOWMINIMIZED:
			pCreateStruct->style |= WS_MINIMIZE;
			break;
		}
	}
#endif
	
	return QSTATUS_SUCCESS;
}

QSTATUS qm::EditFrameWindow::getAction(UINT nId, Action** ppAction)
{
	assert(ppAction);
	*ppAction = pImpl_->pActionMap_->getAction(nId);
	return QSTATUS_SUCCESS;
}

QSTATUS qm::EditFrameWindow::getAccelerator(Accelerator** ppAccelerator)
{
	assert(ppAccelerator);
	*ppAccelerator = pImpl_->pAccelerator_;
	return QSTATUS_SUCCESS;
}

LRESULT qm::EditFrameWindow::windowProc(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	BEGIN_MESSAGE_HANDLER()
		HANDLE_ACTIVATE()
		HANDLE_CLOSE()
		HANDLE_CREATE()
		HANDLE_DESTROY()
		HANDLE_INITMENUPOPUP()
		HANDLE_SIZE()
	END_MESSAGE_HANDLER()
	return FrameWindow::windowProc(uMsg, wParam, lParam);
}

LRESULT qm::EditFrameWindow::onActivate(UINT nFlags, HWND hwnd, bool bMinimized)
{
	FrameWindow::onActivate(nFlags, hwnd, bMinimized);
	
	HIMC hImc = ::ImmGetContext(getHandle());
	
	if (nFlags == WA_INACTIVE) {
		pImpl_->pEditWindow_->saveFocusedItem();
		pImpl_->bIme_ = ::ImmGetOpenStatus(hImc) != 0;
	}
	else {
		pImpl_->pEditWindow_->restoreFocusedItem();
		::ImmSetOpenStatus(hImc, pImpl_->bIme_);
	}
	
	::ImmReleaseContext(getHandle(), hImc);
	
	return 0;
}

LRESULT qm::EditFrameWindow::onClose()
{
	DECLARE_QSTATUS();
	
	HINSTANCE hInst = Application::getApplication().getResourceHandle();
	int nMsg = 0;
	status = messageBox(hInst, IDS_CONFIRMCLOSEEDITFRAME,
		MB_YESNOCANCEL | MB_DEFBUTTON1 | MB_ICONQUESTION,
		getHandle(), 0, 0, &nMsg);
	if (status == QSTATUS_SUCCESS) {
		bool bCancel = false;
		switch (nMsg) {
		case IDYES:
			{
				Action* pAction = pImpl_->pActionMap_->getAction(IDM_FILE_SEND);
				assert(pAction);
				status = pAction->invoke(ActionEvent(IDM_FILE_SEND, 0));
				bCancel = status != QSTATUS_SUCCESS;
			}
			break;
		case IDNO:
			break;
		default:
			bCancel = true;
			break;
		}
		if (!bCancel)
			pImpl_->pManager_->close(this);
	}
	
	return 0;
}

LRESULT qm::EditFrameWindow::onCreate(CREATESTRUCT* pCreateStruct)
{
	if (FrameWindow::onCreate(pCreateStruct) == -1)
		return -1;
	
	DECLARE_QSTATUS();
	
	EditFrameWindowCreateContext* pContext =
		static_cast<EditFrameWindowCreateContext*>(pCreateStruct->lpCreateParams);
	pImpl_->pDocument_ = pContext->pDocument_;
	
	status = pContext->pKeyMap_->createAccelerator(
		CustomAcceleratorFactory(), L"EditFrameWindow",
		mapKeyNameToId, countof(mapKeyNameToId), &pImpl_->pAccelerator_);
	CHECK_QSTATUS_VALUE(-1);
	
	DWORD dwStyle = WS_CHILD | WS_VISIBLE | WS_CLIPCHILDREN | WS_CLIPSIBLINGS;
	std::auto_ptr<EditWindow> pEditWindow;
	status = newQsObject(pImpl_->pProfile_, &pEditWindow);
	CHECK_QSTATUS_VALUE(-1);
	EditWindowCreateContext context = {
		pContext->pMenuManager_,
		pContext->pKeyMap_
	};
	status = pEditWindow->create(L"QmEditWindow", 0, dwStyle,
		CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
		getHandle(), 0, 0, EditFrameWindowImpl::ID_EDITWINDOW, &context);
	CHECK_QSTATUS_VALUE(-1);
	pImpl_->pEditWindow_ = pEditWindow.release();
	
	std::auto_ptr<StatusBar> pStatusBar;
	status = newQsObject(&pStatusBar);
	CHECK_QSTATUS_VALUE(-1);
	status = pStatusBar->create(L"QmStatusBarWindow",
		0, dwStyle, CW_USEDEFAULT, CW_USEDEFAULT,
		CW_USEDEFAULT, CW_USEDEFAULT, getHandle(), 0,
		STATUSCLASSNAMEW, EditFrameWindowImpl::ID_STATUSBAR, 0);
	CHECK_QSTATUS_VALUE(-1);
	pImpl_->pStatusBar_ = pStatusBar.release();
	
	status = newQsObject(pImpl_->pDocument_->getScriptManager(),
		&pImpl_->pScriptMenu_);
	CHECK_QSTATUS();
	
	status = pImpl_->layoutChildren();
	CHECK_QSTATUS_VALUE(-1);
	
	status = pImpl_->initActions();
	CHECK_QSTATUS();
	
	pImpl_->bCreated_ = true;
	
	if (pImpl_->bMaximize_)
		showWindow(SW_MAXIMIZE);
	
	return 0;
}

LRESULT qm::EditFrameWindow::onDestroy()
{
	Profile* pProfile = pImpl_->pProfile_;
	
	pProfile->setInt(L"MessageFrameWindow", L"ShowToolbar", pImpl_->bShowToolbar_);
	pProfile->setInt(L"MessageFrameWindow", L"ShowStatusBar", pImpl_->bShowStatusBar_);
	
#ifndef _WIN32_WCE
	WINDOWPLACEMENT wp;
	getWindowPlacement(&wp);
	pProfile->setBinary(L"EditFrameWindow", L"WindowPlacement",
		reinterpret_cast<const unsigned char*>(&wp), sizeof(wp));
#endif
	
	return FrameWindow::onDestroy();
}

LRESULT qm::EditFrameWindow::onInitMenuPopup(HMENU hmenu, UINT nIndex, bool bSysMenu)
{
	DECLARE_QSTATUS();
	
	if (!bSysMenu) {
		UINT nIdFirst = 0;
		UINT nIdLast = 0;
		MENUITEMINFO mii = { sizeof(mii), MIIM_TYPE | MIIM_ID };
		for (UINT n = 0; ; ++n) {
			if (!::GetMenuItemInfo(hmenu, n, TRUE, &mii))
				break;
			if (nIdFirst == 0)
				nIdFirst = mii.wID;
			nIdLast = mii.wID;
		}
		
		if (nIdFirst == IDM_TOOL_SCRIPTNONE ||
			nIdFirst == IDM_TOOL_SCRIPT) {
			status = pImpl_->pScriptMenu_->createMenu(hmenu);
		}
	}
	
	return FrameWindow::onInitMenuPopup(hmenu, nIndex, bSysMenu);
}

LRESULT qm::EditFrameWindow::onSize(UINT nFlags, int cx, int cy)
{
	if (pImpl_->bCreated_ &&
		!pImpl_->bLayouting_ &&
		(nFlags == SIZE_RESTORED || nFlags == SIZE_MAXIMIZED))
		pImpl_->layoutChildren(cx, cy);
	return FrameWindow::onSize(nFlags, cx, cy);
}


/****************************************************************************
 *
 * EditFrameWindowManager
 *
 */

qm::EditFrameWindowManager::EditFrameWindowManager(Document* pDocument,
	KeyMap* pKeyMap, Profile* pProfile,
	qs::MenuManager* pMenuManager, QSTATUS* pstatus) :
	pDocument_(pDocument),
	pKeyMap_(pKeyMap),
	pProfile_(pProfile),
	pMenuManager_(pMenuManager)
{
}

qm::EditFrameWindowManager::~EditFrameWindowManager()
{
	FrameList::iterator it = listFrame_.begin();
	while (it != listFrame_.end()) {
		EditMessageHolder* pHolder = (*it)->getEditWindow()->getEditMessageHolder();
		delete pHolder->getEditMessage();
		(*it)->destroyWindow();
		++it;
	}
}

QSTATUS qm::EditFrameWindowManager::open(EditMessage* pEditMessage)
{
	DECLARE_QSTATUS();
	
	std::auto_ptr<EditFrameWindow> pFrame;
	status = newQsObject(this, pProfile_, &pFrame);
	CHECK_QSTATUS();
#if defined _WIN32_WCE && _WIN32_WCE >= 300 && _WIN32_WCE_PSPC
	DWORD dwStyle = WS_CLIPCHILDREN;
	DWORD dwExStyle = WS_EX_CAPTIONOKBTN;
#elif defined _WIN32_WCE
	DWORD dwStyle = WS_CLIPCHILDREN;
	DWORD dwExStyle = 0;
#else
	DWORD dwStyle = WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN;
	DWORD dwExStyle = WS_EX_WINDOWEDGE;
#endif
	EditFrameWindowCreateContext context = {
		pDocument_,
		pMenuManager_,
		pKeyMap_
	};
	status = pFrame->create(L"QmEditFrameWindow", L"QMAIL", dwStyle,
		CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
		0, dwExStyle, 0, 0, &context);
	CHECK_QSTATUS();
	EditFrameWindow* p = pFrame.release();
	
	EditMessageHolder* pHolder = p->getEditWindow()->getEditMessageHolder();
	status = pHolder->setEditMessage(pEditMessage);
	CHECK_QSTATUS();
	
	status = STLWrapper<FrameList>(listFrame_).push_back(p);
	CHECK_QSTATUS();
	
	p->showWindow(SW_SHOW);
	
	return QSTATUS_SUCCESS;
}

void qm::EditFrameWindowManager::close(EditFrameWindow* pEditFrameWindow)
{
	FrameList::iterator it = std::find(listFrame_.begin(),
		listFrame_.end(), pEditFrameWindow);
	if (it != listFrame_.end())
		listFrame_.erase(it);
	
	EditMessageHolder* pHolder = (*it)->getEditWindow()->getEditMessageHolder();
	pHolder->releaseEditMessage();
	pEditFrameWindow->destroyWindow();
}


QSTATUS qm::EditFrameWindowManager::preModalDialog(HWND hwndParent)
{
	FrameList::iterator it = listFrame_.begin();
	while (it != listFrame_.end()) {
		if ((*it)->getHandle() != hwndParent)
			(*it)->enableWindow(false);
		++it;
	}
	
	return QSTATUS_SUCCESS;
}

QSTATUS qm::EditFrameWindowManager::postModalDialog(HWND hwndParent)
{
	FrameList::iterator it = listFrame_.begin();
	while (it != listFrame_.end()) {
		if ((*it)->getHandle() != hwndParent)
			(*it)->enableWindow(true);
		++it;
	}
	
	return QSTATUS_SUCCESS;
}
