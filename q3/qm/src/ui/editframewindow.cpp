/*
 * $Id$
 *
 * Copyright(C) 1998-2004 Satoshi Nakamura
 * All rights reserved.
 *
 */

#include <qmapplication.h>
#include <qmdocument.h>
#include <qmeditwindow.h>
#include <qmsecurity.h>

#include <qsaccelerator.h>
#include <qsaction.h>

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
#include "uimanager.h"
#include "uiutil.h"
#include "../action/action.h"
#include "../action/actionmacro.h"
#include "../action/editaction.h"
#include "../action/findreplace.h"
#include "../model/editmessage.h"

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
	void initActions();
	void layoutChildren();
	void layoutChildren(int cx,
						int cy);

public:
	EditFrameWindow* pThis_;
	
	bool bShowToolbar_;
	bool bShowStatusBar_;
	
	EditFrameWindowManager* pManager_;
	Profile* pProfile_;
	Document* pDocument_;
	UIManager* pUIManager_;
	SyncManager* pSyncManager_;
	SyncDialogManager* pSyncDialogManager_;
	EditWindow* pEditWindow_;
	StatusBar* pStatusBar_;
	SecurityModel* pSecurityModel_;
	std::auto_ptr<Accelerator> pAccelerator_;
	std::auto_ptr<ActionMap> pActionMap_;
	std::auto_ptr<ActionInvoker> pActionInvoker_;
	std::auto_ptr<FindReplaceManager> pFindReplaceManager_;
	std::auto_ptr<InsertTextMenu> pInsertTextMenu_;
	std::auto_ptr<ScriptMenu> pScriptMenu_;
	ToolbarCookie* pToolbarCookie_;
	bool bIme_;
	bool bCreated_;
	int nInitialShow_;
	bool bLayouting_;
};

void qm::EditFrameWindowImpl::initActions()
{
	pActionMap_.reset(new ActionMap());
	pActionInvoker_.reset(new ActionInvoker(pActionMap_.get()));
	pFindReplaceManager_.reset(new FindReplaceManager());
	
	ADD_ACTION2(ConfigTextsAction,
		IDM_CONFIG_TEXTS,
		pDocument_->getFixedFormTextManager(),
		pThis_->getHandle());
	ADD_ACTION2(EditAttachmentEditAddAction,
		IDM_ATTACHMENTEDIT_ADD,
		pEditWindow_->getEditMessageHolder(),
		pThis_->getHandle());
	ADD_ACTION2(EditAttachmentEditDeleteAction,
		IDM_ATTACHMENTEDIT_DELETE,
		pEditWindow_->getEditMessageHolder(),
		pEditWindow_->getAttachmentSelectionModel());
	ADD_ACTION3(EditEditCommandAction,
		IDM_EDIT_COPY,
		pEditWindow_,
		&EditWindowItem::copy,
		&EditWindowItem::canCopy);
	ADD_ACTION3(EditEditCommandAction,
		IDM_EDIT_CUT,
		pEditWindow_,
		&EditWindowItem::cut,
		&EditWindowItem::canCut);
	
	struct {
		UINT nId_;
		TextWindow::DeleteTextFlag flag_;
	} deletes[] = {
		{ IDM_EDIT_DELETECHAR,			TextWindow::DELETETEXTFLAG_DELETECHAR			},
		{ IDM_EDIT_DELETEWORD,			TextWindow::DELETETEXTFLAG_DELETEWORD			},
		{ IDM_EDIT_DELETEBACKWARDCHAR,	TextWindow::DELETETEXTFLAG_DELETEBACKWARDCHAR	},
		{ IDM_EDIT_DELETEBACKWARDWORD,	TextWindow::DELETETEXTFLAG_DELETEBACKWARDWORD	}
	};
	for (int n = 0; n < countof(deletes); ++n) {
		ADD_ACTION2(EditEditDeleteAction,
			deletes[n].nId_,
			pEditWindow_->getTextWindow(),
			deletes[n].flag_);
	}
	
	ADD_ACTION3(EditEditFindAction,
		IDM_EDIT_FIND,
		pEditWindow_->getTextWindow(),
		pProfile_,
		pFindReplaceManager_.get());
	ADD_ACTION3(EditEditFindAction,
		IDM_EDIT_FINDNEXT,
		pEditWindow_->getTextWindow(),
		true,
		pFindReplaceManager_.get());
	ADD_ACTION3(EditEditFindAction,
		IDM_EDIT_FINDPREV,
		pEditWindow_->getTextWindow(),
		false,
		pFindReplaceManager_.get());
	ADD_ACTION3(EditEditCommandAction,
		IDM_EDIT_PASTE,
		pEditWindow_,
		&EditWindowItem::paste,
		&EditWindowItem::canPaste);
	ADD_ACTION3(EditEditPasteWithQuoteAction,
		IDM_EDIT_PASTEWITHQUOTE,
		pDocument_,
		pEditWindow_->getTextWindow(),
		pProfile_);
	ADD_ACTION3(EditEditCommandAction,
		IDM_EDIT_REDO,
		pEditWindow_,
		&EditWindowItem::redo,
		&EditWindowItem::canRedo);
	ADD_ACTION3(EditEditReplaceAction,
		IDM_EDIT_REPLACE,
		pEditWindow_->getTextWindow(),
		pProfile_,
		pFindReplaceManager_.get());
	ADD_ACTION3(EditEditCommandAction,
		IDM_EDIT_SELECTALL,
		pEditWindow_,
		&EditWindowItem::selectAll,
		&EditWindowItem::canSelectAll);
	ADD_ACTION3(EditEditCommandAction,
		IDM_EDIT_UNDO,
		pEditWindow_,
		&EditWindowItem::undo,
		&EditWindowItem::canUndo);
	ADD_ACTION1(ViewShowStatusBarAction<EditFrameWindow>,
		IDM_VIEW_SHOWSTATUSBAR,
		pThis_);
	ADD_ACTION1(ViewShowToolbarAction<EditFrameWindow>,
		IDM_VIEW_SHOWTOOLBAR,
		pThis_);
	
	struct {
		UINT nId_;
		TextWindow::MoveCaret moveCaret_;
	} moveCarets[] = {
		{ IDM_EDIT_MOVECHARLEFT,	TextWindow::MOVECARET_CHARLEFT	},
		{ IDM_EDIT_MOVECHARRIGHT,	TextWindow::MOVECARET_CHARRIGHT	},
		{ IDM_EDIT_MOVELINESTART,	TextWindow::MOVECARET_LINESTART	},
		{ IDM_EDIT_MOVELINEEND,		TextWindow::MOVECARET_LINEEND	},
		{ IDM_EDIT_MOVELINEUP,		TextWindow::MOVECARET_LINEUP	},
		{ IDM_EDIT_MOVELINEDOWN,	TextWindow::MOVECARET_LINEDOWN	},
		{ IDM_EDIT_MOVEPAGEUP,		TextWindow::MOVECARET_PAGEUP	},
		{ IDM_EDIT_MOVEPAGEDOWN,	TextWindow::MOVECARET_PAGEDOWN	},
		{ IDM_EDIT_MOVEDOCSTART,	TextWindow::MOVECARET_DOCSTART	},
		{ IDM_EDIT_MOVEDOCEND,		TextWindow::MOVECARET_DOCEND	},
	};
	for (int n = 0; n < countof(moveCarets); ++n) {
		ADD_ACTION2(EditEditMoveCaretAction,
			moveCarets[n].nId_,
			pEditWindow_->getTextWindow(),
			moveCarets[n].moveCaret_);
	}
	
	ADD_ACTION1(FileCloseAction,
		IDM_FILE_CLOSE,
		pThis_->getHandle());
#if defined _WIN32_WCE && _WIN32_WCE >= 300 && defined _WIN32_WCE_PSPC
	ADD_ACTION1(FileCloseAction,
		IDOK,
		pThis_->getHandle());
#endif
	ADD_ACTION6(EditFileSendAction,
		IDM_FILE_DRAFT,
		true,
		pDocument_,
		pEditWindow_->getEditMessageHolder(),
		pThis_,
		pProfile_,
		pSecurityModel_);
	ADD_ACTION1(EditFileInsertAction,
		IDM_FILE_INSERT,
		pEditWindow_->getTextWindow());
	ADD_ACTION2(EditFileOpenAction,
		IDM_FILE_OPEN,
		pEditWindow_->getEditMessageHolder(),
		pThis_->getHandle());
	ADD_ACTION2(EditFileSaveAction,
		IDM_FILE_SAVE,
		pEditWindow_->getEditMessageHolder(),
		pThis_->getHandle());
	ADD_ACTION6(EditFileSendAction,
		IDM_FILE_SEND,
		false,
		pDocument_,
		pEditWindow_->getEditMessageHolder(),
		pThis_,
		pProfile_,
		pSecurityModel_);
	ADD_ACTION7(EditFileSendAction,
		IDM_FILE_SENDNOW,
		pDocument_,
		pEditWindow_->getEditMessageHolder(),
		pThis_,
		pProfile_,
		pSyncManager_,
		pSyncDialogManager_,
		pSecurityModel_);
	ADD_ACTION_RANGE1(EditFocusItemAction,
		IDM_FOCUS_HEADEREDITITEM,
		IDM_FOCUS_HEADEREDITITEM + 10,
		pEditWindow_);
	ADD_ACTION4(EditToolAddressBookAction,
		IDM_TOOL_ADDRESSBOOK,
		pEditWindow_->getEditMessageHolder(),
		pEditWindow_,
		pDocument_->getAddressBook(),
		pProfile_);
	ADD_ACTION2(EditToolAttachmentAction,
		IDM_TOOL_ATTACHMENT,
		pEditWindow_->getEditMessageHolder(),
		pThis_->getHandle());
	ADD_ACTION4(EditToolFlagAction,
		IDM_TOOL_ENCRYPT,
		pEditWindow_->getEditMessageHolder(),
		&EditMessage::isEncrypt,
		&EditMessage::setEncrypt,
		Security::isEnabled());
	ADD_ACTION2(EditToolInsertSignatureAction,
		IDM_TOOL_INSERTSIGNATURE,
		pEditWindow_->getEditMessageHolder(),
		pEditWindow_->getTextWindow());
	ADD_ACTION_RANGE2(EditToolInsertTextAction,
		IDM_TOOL_INSERTTEXT,
		IDM_TOOL_INSERTTEXT + 100,
		pInsertTextMenu_.get(),
		pEditWindow_->getTextWindow());
	ADD_ACTION1(EditToolHeaderEditAction,
		IDM_TOOL_HEADEREDIT,
		pEditWindow_);
	ADD_ACTION1(EditToolReformAction,
		IDM_TOOL_REFORM,
		pEditWindow_->getTextWindow());
	ADD_ACTION2(EditToolReformAllAction,
		IDM_TOOL_REFORMALL,
		pEditWindow_->getTextWindow(),
		pProfile_);
	ADD_ACTION4(EditToolFlagAction,
		IDM_TOOL_REFORMAUTO,
		pEditWindow_->getEditMessageHolder(),
		&EditMessage::isAutoReform,
		&EditMessage::setAutoReform,
		true);
	ADD_ACTION4(EditToolFlagAction,
		IDM_TOOL_SIGN,
		pEditWindow_->getEditMessageHolder(),
		&EditMessage::isSign,
		&EditMessage::setSign,
		Security::isEnabled());
	ADD_ACTION_RANGE4(ToolScriptAction,
		IDM_TOOL_SCRIPT,
		IDM_TOOL_SCRIPT + 100,
		pScriptMenu_.get(),
		pDocument_,
		pProfile_,
		pThis_);
}

void qm::EditFrameWindowImpl::layoutChildren()
{
	RECT rect;
	pThis_->getClientRect(&rect);
	layoutChildren(rect.right - rect.left, rect.bottom - rect.top);
}

void qm::EditFrameWindowImpl::layoutChildren(int cx,
											 int cy)
{
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
	
	int nWidth[] = {
		cx - 110,
		cx - 80,
		cx - 50,
		-1
	};
	pStatusBar_->setParts(nWidth, countof(nWidth));
	
	bLayouting_ = false;
}


/****************************************************************************
 *
 * EditFrameWindow
 *
 */

qm::EditFrameWindow::EditFrameWindow(EditFrameWindowManager* pManager,
									 Profile* pProfile) :
	FrameWindow(Application::getApplication().getResourceHandle(), true),
	pImpl_(0)
{
	pImpl_ = new EditFrameWindowImpl();
	pImpl_->pThis_ = this;
	pImpl_->bShowToolbar_ = pProfile->getInt(L"MessageFrameWindow", L"ShowToolbar", 1) != 0;
	pImpl_->bShowStatusBar_ = pProfile->getInt(L"MessageFrameWindow", L"ShowStatusBar", 1) != 0;
	pImpl_->pManager_ = pManager;
	pImpl_->pProfile_ = pProfile;
	pImpl_->pDocument_ = 0;
	pImpl_->pEditWindow_ = 0;
	pImpl_->pStatusBar_ = 0;
	pImpl_->pSecurityModel_ = 0;
	pImpl_->pToolbarCookie_ = 0;
	pImpl_->bIme_ = false;
	pImpl_->bCreated_ = false;
	pImpl_->nInitialShow_ = SW_SHOWNORMAL;
	pImpl_->bLayouting_ = false;
}

qm::EditFrameWindow::~EditFrameWindow()
{
	delete pImpl_;
}

EditWindow* qm::EditFrameWindow::getEditWindow() const
{
	return pImpl_->pEditWindow_;
}

const ActionInvoker* qm::EditFrameWindow::getActionInvoker() const
{
	return pImpl_->pActionInvoker_.get();
}

void qm::EditFrameWindow::initialShow()
{
	showWindow(pImpl_->nInitialShow_);
}

void qm::EditFrameWindow::close()
{
	pImpl_->pManager_->close(this);
}

bool qm::EditFrameWindow::tryClose()
{
	bool bCancel = true;
	
	HINSTANCE hInst = Application::getApplication().getResourceHandle();
	int nMsg = messageBox(hInst, IDS_CONFIRMCLOSEEDITFRAME,
		MB_YESNOCANCEL | MB_DEFBUTTON1 | MB_ICONQUESTION, getHandle(), 0, 0);
	switch (nMsg) {
	case IDYES:
		{
			Action* pAction = pImpl_->pActionMap_->getAction(IDM_FILE_SEND);
			assert(pAction);
			pAction->invoke(ActionEvent(IDM_FILE_SEND, 0));
			return true;
		}
		break;
	case IDNO:
		bCancel = false;
		break;
	default:
		break;
	}
	if (!bCancel)
		pImpl_->pManager_->close(this);
	
	return !bCancel;
}

bool qm::EditFrameWindow::isShowToolbar() const
{
	return pImpl_->bShowToolbar_;
}

void qm::EditFrameWindow::setShowToolbar(bool bShow)
{
	if (bShow != pImpl_->bShowToolbar_) {
		pImpl_->bShowToolbar_ = bShow;
		pImpl_->layoutChildren();
	}
}

bool qm::EditFrameWindow::isShowStatusBar() const
{
	return pImpl_->bShowStatusBar_;
}

void qm::EditFrameWindow::setShowStatusBar(bool bShow)
{
	if (bShow != pImpl_->bShowStatusBar_) {
		pImpl_->bShowStatusBar_ = bShow;
		pImpl_->layoutChildren();
	}
}

bool qm::EditFrameWindow::getToolbarButtons(Toolbar* pToolbar)
{
	pToolbar->nId_ = EditFrameWindowImpl::ID_TOOLBAR;
	return true;
}

bool qm::EditFrameWindow::createToolbarButtons(void* pCreateParam,
											   HWND hwndToolbar)
{
	EditFrameWindowCreateContext* pContext =
		static_cast<EditFrameWindowCreateContext*>(pCreateParam);
	UIManager* pUIManager = pContext->pUIManager_;
	pImpl_->pToolbarCookie_ = pUIManager->getToolbarManager()->createButtons(
		L"editframe", hwndToolbar, this);
	return pImpl_->pToolbarCookie_ != 0;
}

#if defined _WIN32_WCE && (_WIN32_WCE < 300 || !defined _WIN32_WCE_PSPC)
UINT qm::EditFrameWindow::getBarId(int n) const
{
	assert(n == 0 || n == 1);
	assert(pnId);
	static UINT nIds[] = {
		EditFrameWindowImpl::ID_COMMANDBARMENU,
		EditFrameWindowImpl::ID_COMMANDBARBUTTON
	};
	return nIds[n];
}

bool qm::EditFrameWindow::getCommandBandsRestoreInfo(int n,
													 COMMANDBANDSRESTOREINFO* pcbri) const
{
	WCHAR wszKey[32];
	swprintf(wszKey, L"CommandBandsRestoreInfo%d", n);
	size_t nSize = pImpl_->pProfile_->getBinary(L"EditFrameWindow", wszKey,
		reinterpret_cast<unsigned char*>(pcbri), sizeof(*pcbri));
	if (nSize != sizeof(*pcbri))
		pcbri->cbSize = 0;
	return true;
}

bool qm::EditFrameWindow::setCommandBandsRestoreInfo(int n,
													 const COMMANDBANDSRESTOREINFO& cbri)
{
	WCHAR wszKey[32];
	swprintf(wszKey, L"CommandBandsRestoreInfo%d", n);
	pImpl_->pProfile_->setBinary(L"EditFrameWindow", wszKey,
		reinterpret_cast<const unsigned char*>(&cbri), sizeof(cbri));
	return true;
}
#endif

HMENU qm::EditFrameWindow::getMenuHandle(void* pCreateParam)
{
	EditFrameWindowCreateContext* pContext =
		static_cast<EditFrameWindowCreateContext*>(pCreateParam);
	UIManager* pUIManager = pContext->pUIManager_;
	return pUIManager->getMenuManager()->getMenu(L"editframe", true, true);
}

UINT qm::EditFrameWindow::getIconId()
{
	return IDI_MAINFRAME;
}

void qm::EditFrameWindow::getWindowClass(WNDCLASS* pwc)
{
	FrameWindow::getWindowClass(pwc);
	pwc->hIcon = ::LoadIcon(Application::getApplication().getResourceHandle(),
		MAKEINTRESOURCE(IDI_MAINFRAME));
	pwc->hbrBackground = reinterpret_cast<HBRUSH>(COLOR_BTNFACE + 1);
}

bool qm::EditFrameWindow::preCreateWindow(CREATESTRUCT* pCreateStruct)
{
#if defined _WIN32_WCE && _WIN32_WCE >= 300 && defined _WIN32_WCE_PSPC
#define MENU_HEIGHT 26
	SIPINFO si = { sizeof(si) };
	::SHSipInfo(SPI_GETSIPINFO, 0, &si, 0);
	pCreateStruct->cx = si.rcVisibleDesktop.right - si.rcVisibleDesktop.left;
	pCreateStruct->cy = si.rcVisibleDesktop.bottom -
		si.rcVisibleDesktop.top - (si.fdwFlags & SIPF_ON ? 0 : MENU_HEIGHT);
#elif !defined _WIN32_WCE
	pImpl_->nInitialShow_ = UIUtil::loadWindowPlacement(
		pImpl_->pProfile_, L"EditFrameWindow", pCreateStruct);
#endif
	return true;
}

Action* qm::EditFrameWindow::getAction(UINT nId)
{
	return pImpl_->pActionMap_->getAction(nId);
}

Accelerator* qm::EditFrameWindow::getAccelerator()
{
	return pImpl_->pAccelerator_.get();
}

LRESULT qm::EditFrameWindow::windowProc(UINT uMsg,
										WPARAM wParam,
										LPARAM lParam)
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

LRESULT qm::EditFrameWindow::onActivate(UINT nFlags,
										HWND hwnd,
										bool bMinimized)
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
	tryClose();
	
	return 0;
}

LRESULT qm::EditFrameWindow::onCreate(CREATESTRUCT* pCreateStruct)
{
	if (FrameWindow::onCreate(pCreateStruct) == -1)
		return -1;
	
	EditFrameWindowCreateContext* pContext =
		static_cast<EditFrameWindowCreateContext*>(pCreateStruct->lpCreateParams);
	pImpl_->pDocument_ = pContext->pDocument_;
	pImpl_->pUIManager_ = pContext->pUIManager_;
	pImpl_->pSyncManager_ = pContext->pSyncManager_;
	pImpl_->pSyncDialogManager_ = pContext->pSyncDialogManager_;
	pImpl_->pSecurityModel_ = pContext->pSecurityModel_;
	
	CustomAcceleratorFactory acceleratorFactory;
	pImpl_->pAccelerator_ = pContext->pUIManager_->getKeyMap()->createAccelerator(
		&acceleratorFactory, L"EditFrameWindow", mapKeyNameToId, countof(mapKeyNameToId));
	if (!pImpl_->pAccelerator_.get())
		return -1;
	
	DWORD dwStyle = WS_CHILD | WS_VISIBLE | WS_CLIPCHILDREN | WS_CLIPSIBLINGS;
	std::auto_ptr<EditWindow> pEditWindow(new EditWindow(pImpl_->pProfile_));
	EditWindowCreateContext context = {
		pContext->pUIManager_,
		pContext->pwszClass_
	};
	if (!pEditWindow->create(L"QmEditWindow", 0, dwStyle,
		CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
		getHandle(), 0, 0, EditFrameWindowImpl::ID_EDITWINDOW, &context))
		return -1;
	pImpl_->pEditWindow_ = pEditWindow.release();
	
	std::auto_ptr<StatusBar> pStatusBar(new StatusBar());
	if (!pStatusBar->create(L"QmStatusBarWindow",
		0, dwStyle, CW_USEDEFAULT, CW_USEDEFAULT,
		CW_USEDEFAULT, CW_USEDEFAULT, getHandle(), 0,
		STATUSCLASSNAMEW, EditFrameWindowImpl::ID_STATUSBAR, 0))
		return -1;
	pImpl_->pStatusBar_ = pStatusBar.release();
	
	pImpl_->pInsertTextMenu_.reset(new InsertTextMenu(
		pImpl_->pDocument_->getFixedFormTextManager()));
	pImpl_->pScriptMenu_.reset(new ScriptMenu(
		pImpl_->pDocument_->getScriptManager()));
	pImpl_->layoutChildren();
	pImpl_->initActions();
	pImpl_->bCreated_ = true;
	
	return 0;
}

LRESULT qm::EditFrameWindow::onDestroy()
{
	Profile* pProfile = pImpl_->pProfile_;
	
	pProfile->setInt(L"MessageFrameWindow",
		L"ShowToolbar", pImpl_->bShowToolbar_);
	pProfile->setInt(L"MessageFrameWindow",
		L"ShowStatusBar", pImpl_->bShowStatusBar_);
	
	if (pImpl_->pToolbarCookie_)
		pImpl_->pUIManager_->getToolbarManager()->destroy(pImpl_->pToolbarCookie_);
	
	UIUtil::saveWindowPlacement(getHandle(), pProfile, L"EditFrameWindow");
	
	FrameWindow::save();
	
	return FrameWindow::onDestroy();
}

LRESULT qm::EditFrameWindow::onInitMenuPopup(HMENU hmenu,
											 UINT nIndex,
											 bool bSysMenu)
{
	if (!bSysMenu) {
		UINT nIdFirst = 0;
		UINT nIdLast = 0;
		MENUITEMINFO mii = { sizeof(mii), MIIM_ID };
		for (UINT n = 0; ; ++n) {
			if (!::GetMenuItemInfo(hmenu, n, TRUE, &mii))
				break;
			if (nIdFirst == 0)
				nIdFirst = mii.wID;
			nIdLast = mii.wID;
		}
		
		if (nIdLast == IDM_CONFIG_TEXTS)
			pImpl_->pInsertTextMenu_->createMenu(hmenu);
		else if (nIdFirst == IDM_TOOL_SCRIPTNONE ||
			nIdFirst == IDM_TOOL_SCRIPT)
			pImpl_->pScriptMenu_->createMenu(hmenu);
	}
	
	return FrameWindow::onInitMenuPopup(hmenu, nIndex, bSysMenu);
}

LRESULT qm::EditFrameWindow::onSize(UINT nFlags,
									int cx,
									int cy)
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
												   UIManager* pUIManager,
												   SyncManager* pSyncManager,
												   SyncDialogManager* pSyncDialogManager,
												   Profile* pProfile,
												   SecurityModel* pSecurityModel) :
	pDocument_(pDocument),
	pUIManager_(pUIManager),
	pSyncManager_(pSyncManager),
	pSyncDialogManager_(pSyncDialogManager),
	pProfile_(pProfile),
	pSecurityModel_(pSecurityModel)
{
}

qm::EditFrameWindowManager::~EditFrameWindowManager()
{
	for (FrameList::iterator it = listFrame_.begin(); it != listFrame_.end(); ++it) {
		EditMessageHolder* pHolder = (*it)->getEditWindow()->getEditMessageHolder();
		delete pHolder->getEditMessage();
		(*it)->destroyWindow();
	}
}

bool qm::EditFrameWindowManager::open(std::auto_ptr<EditMessage> pEditMessage)
{
	std::auto_ptr<EditFrameWindow> pFrame(new EditFrameWindow(this, pProfile_));
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
		pUIManager_,
		pSyncManager_,
		pSyncDialogManager_,
		pSecurityModel_,
		pEditMessage->getAccount()->getClass()
	};
	if (!pFrame->create(L"QmEditFrameWindow", L"QMAIL", dwStyle,
		CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
		0, dwExStyle, 0, 0, &context))
		return false;
	EditFrameWindow* p = pFrame.release();
	
	EditMessageHolder* pHolder = p->getEditWindow()->getEditMessageHolder();
	if (!pHolder->setEditMessage(pEditMessage.get()))
		return false;
	pEditMessage.release();
	
	listFrame_.push_back(p);
	
	p->initialShow();
	
	return true;
}

void qm::EditFrameWindowManager::close(EditFrameWindow* pEditFrameWindow)
{
	FrameList::iterator it = std::find(listFrame_.begin(),
		listFrame_.end(), pEditFrameWindow);
	if (it != listFrame_.end())
		listFrame_.erase(it);
	
	EditWindow* pEditWindow = pEditFrameWindow->getEditWindow();
	EditMessageHolder* pHolder = pEditWindow->getEditMessageHolder();
	pHolder->releaseEditMessage();
	pEditFrameWindow->destroyWindow();
}

bool qm::EditFrameWindowManager::closeAll()
{
	while (!listFrame_.empty()) {
		EditFrameWindow* pWindow = listFrame_.back();
		if (!pWindow->tryClose())
			break;
	}
	
	return listFrame_.empty();
}

void qm::EditFrameWindowManager::preModalDialog(HWND hwndParent)
{
	for (FrameList::iterator it = listFrame_.begin(); it != listFrame_.end(); ++it) {
		if ((*it)->getHandle() != hwndParent)
			(*it)->enableWindow(false);
	}
}

void qm::EditFrameWindowManager::postModalDialog(HWND hwndParent)
{
	for (FrameList::iterator it = listFrame_.begin(); it != listFrame_.end(); ++it) {
		if ((*it)->getHandle() != hwndParent)
			(*it)->enableWindow(true);
	}
}
