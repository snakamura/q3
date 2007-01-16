/*
 * $Id$
 *
 * Copyright(C) 1998-2007 Satoshi Nakamura
 * All rights reserved.
 *
 */

#pragma warning(disable:4786)

#include <qmaction.h>
#include <qmapplication.h>
#include <qmdocument.h>
#include <qmeditwindow.h>
#include <qmsecurity.h>

#include <qsaccelerator.h>
#include <qsaction.h>
#include <qsuiutil.h>

#include <algorithm>

#ifdef _WIN32_WCE_PSPC
#	include <aygshell.h>
#endif

#include "actionid.h"
#include "actionitem.h"
#include "dialogs.h"
#include "editframewindow.h"
#include "editwindow.h"
#include "focus.h"
#include "menucreator.h"
#include "optiondialog.h"
#include "statusbar.h"
#include "uimanager.h"
#include "uiutil.h"
#include "../action/action.h"
#include "../action/actionmacro.h"
#include "../action/editaction.h"
#include "../action/findreplace.h"
#include "../model/editmessage.h"
#include "../uimodel/folderselectionmodel.h"

using namespace qm;
using namespace qs;


/****************************************************************************
 *
 * EditFrameWindowImpl
 *
 */

class qm::EditFrameWindowImpl : public AccountSelectionModel
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
	typedef std::vector<MenuCreator*> MenuCreatorList;

public:
	void initActions();
	void layoutChildren();
	void layoutChildren(int cx,
						int cy);

public:
	virtual Account* getAccount();

public:
	EditFrameWindow* pThis_;
	
	bool bShowToolbar_;
	bool bShowStatusBar_;
	
	EditFrameWindowManager* pManager_;
	Profile* pProfile_;
	Document* pDocument_;
	UIManager* pUIManager_;
	PasswordManager* pPasswordManager_;
	SyncManager* pSyncManager_;
	SyncDialogManager* pSyncDialogManager_;
	OptionDialogManager* pOptionDialogManager_;
	EditWindow* pEditWindow_;
	StatusBar* pStatusBar_;
	SecurityModel* pSecurityModel_;
	std::auto_ptr<Accelerator> pAccelerator_;
	std::auto_ptr<ActionMap> pActionMap_;
	std::auto_ptr<ActionInvoker> pActionInvoker_;
	std::auto_ptr<FindReplaceManager> pFindReplaceManager_;
	MenuCreatorList listMenuCreator_;
	std::auto_ptr<MacroMenuCreator> pMacroMenuCreator_;
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
	
	ADD_ACTION0(NoneAction,
		IDM_NONE);
	ADD_ACTION4(ToolOptionsAction,
		IDM_CONFIG_TEXTS,
		pOptionDialogManager_,
		this,
		pThis_->getHandle(),
		OptionDialog::PANEL_FIXEDFORMTEXTS);
	ADD_ACTION2(EditAttachmentEditAddAction,
		IDM_ATTACHMENTEDIT_ADD,
		pEditWindow_->getEditMessageHolder(),
		pThis_->getHandle());
	ADD_ACTION2(EditAttachmentEditDeleteAction,
		IDM_ATTACHMENTEDIT_DELETE,
		pEditWindow_->getEditMessageHolder(),
		pEditWindow_->getAttachmentSelectionModel());
	ADD_ACTION3(EditCommandAction<EditWindowItem>,
		IDM_EDIT_COPY,
		pEditWindow_->getFocusController(),
		&EditWindowItem::copy,
		&EditWindowItem::canCopy);
	ADD_ACTION3(EditCommandAction<EditWindowItem>,
		IDM_EDIT_CUT,
		pEditWindow_->getFocusController(),
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
	ADD_ACTION3(EditCommandAction<EditWindowItem>,
		IDM_EDIT_PASTE,
		pEditWindow_->getFocusController(),
		&EditWindowItem::paste,
		&EditWindowItem::canPaste);
	ADD_ACTION6(EditEditPasteWithQuoteAction,
		IDM_EDIT_PASTEWITHQUOTE,
		pDocument_,
		pEditWindow_->getEditMessageHolder(),
		pEditWindow_->getTextWindow(),
		pSecurityModel_,
		pProfile_,
		pThis_->getHandle());
	ADD_ACTION3(EditCommandAction<EditWindowItem>,
		IDM_EDIT_REDO,
		pEditWindow_->getFocusController(),
		&EditWindowItem::redo,
		&EditWindowItem::canRedo);
	ADD_ACTION3(EditEditReplaceAction,
		IDM_EDIT_REPLACE,
		pEditWindow_->getTextWindow(),
		pProfile_,
		pFindReplaceManager_.get());
	ADD_ACTION3(EditCommandAction<EditWindowItem>,
		IDM_EDIT_SELECTALL,
		pEditWindow_->getFocusController(),
		&EditWindowItem::selectAll,
		&EditWindowItem::canSelectAll);
	ADD_ACTION3(EditCommandAction<EditWindowItem>,
		IDM_EDIT_UNDO,
		pEditWindow_->getFocusController(),
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
#if defined _WIN32_WCE && _WIN32_WCE >= 0x300 && defined _WIN32_WCE_PSPC
	ADD_ACTION1(FileCloseAction,
		IDOK,
		pThis_->getHandle());
#endif
	ADD_ACTION7(EditFileSendAction,
		IDM_FILE_DRAFT,
		EditFileSendAction::TYPE_DRAFT,
		pDocument_,
		pPasswordManager_,
		pEditWindow_->getEditMessageHolder(),
		pThis_,
		pProfile_,
		pSecurityModel_);
	ADD_ACTION7(EditFileSendAction,
		IDM_FILE_DRAFTCLOSE,
		EditFileSendAction::TYPE_DRAFTCLOSE,
		pDocument_,
		pPasswordManager_,
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
	ADD_ACTION7(EditFileSendAction,
		IDM_FILE_SEND,
		EditFileSendAction::TYPE_SEND,
		pDocument_,
		pPasswordManager_,
		pEditWindow_->getEditMessageHolder(),
		pThis_,
		pProfile_,
		pSecurityModel_);
	ADD_ACTION8(EditFileSendAction,
		IDM_FILE_SENDNOW,
		pDocument_,
		pPasswordManager_,
		pEditWindow_->getEditMessageHolder(),
		pThis_,
		pProfile_,
		pSyncManager_,
		pSyncDialogManager_,
		pSecurityModel_);
	ADD_ACTION2(ViewFocusItemAction,
		IDM_VIEW_FOCUSEDITITEM,
		pEditWindow_->getFocusController(),
		ViewFocusItemAction::TYPE_ITEM);
	ADD_ACTION2(ViewFocusItemAction,
		IDM_VIEW_FOCUSNEXTEDITITEM,
		pEditWindow_->getFocusController(),
		ViewFocusItemAction::TYPE_NEXT);
	ADD_ACTION2(ViewFocusItemAction,
		IDM_VIEW_FOCUSPREVEDITITEM,
		pEditWindow_->getFocusController(),
		ViewFocusItemAction::TYPE_PREV);
#ifdef QMZIP
	ADD_ACTION1(EditToolArchiveAttachmentAction,
		IDM_TOOL_ARCHIVEATTACHMENT,
		pEditWindow_->getEditMessageHolder());
#endif
	ADD_ACTION2(EditToolAttachmentAction,
		IDM_TOOL_ATTACHMENT,
		pEditWindow_->getEditMessageHolder(),
		pThis_->getHandle());
	ADD_ACTION1(EditToolEncodingAction,
		IDM_TOOL_ENCODING,
		pEditWindow_->getEditMessageHolder());
	ADD_ACTION1(EditToolHeaderEditAction,
		IDM_TOOL_HEADEREDIT,
		pEditWindow_);
	ADD_ACTION2(EditToolInsertSignatureAction,
		IDM_TOOL_INSERTSIGNATURE,
		pEditWindow_->getEditMessageHolder(),
		pEditWindow_->getTextWindow());
	ADD_ACTION2(EditToolInsertTextAction,
		IDM_TOOL_INSERTTEXT,
		pDocument_->getFixedFormTextManager(),
		pEditWindow_->getTextWindow());
	ADD_ACTION3(ToolInvokeActionAction,
		IDM_TOOL_INVOKEACTION,
		pActionInvoker_.get(),
		pProfile_,
		pThis_->getHandle());
	ADD_ACTION3(EditToolMessageSecurityAction,
		IDM_TOOL_PGPENCRYPT,
		pEditWindow_->getEditMessageHolder(),
		MESSAGESECURITY_PGPENCRYPT,
		Security::isPGPEnabled());
	ADD_ACTION3(EditToolMessageSecurityAction,
		IDM_TOOL_PGPMIME,
		pEditWindow_->getEditMessageHolder(),
		MESSAGESECURITY_PGPMIME,
		Security::isPGPEnabled());
	ADD_ACTION3(EditToolMessageSecurityAction,
		IDM_TOOL_PGPSIGN,
		pEditWindow_->getEditMessageHolder(),
		MESSAGESECURITY_PGPSIGN,
		Security::isPGPEnabled());
	ADD_ACTION2(ToolPopupMenuAction,
		IDM_TOOL_POPUPMENU,
		pUIManager_->getMenuManager(),
		pThis_->getHandle());
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
	ADD_ACTION4(EditToolSelectAddressAction,
		IDM_TOOL_SELECTADDRESS,
		pEditWindow_->getEditMessageHolder(),
		pEditWindow_,
		pDocument_->getAddressBook(),
		pProfile_);
	ADD_ACTION3(EditToolMessageSecurityAction,
		IDM_TOOL_SMIMEENCRYPT,
		pEditWindow_->getEditMessageHolder(),
		MESSAGESECURITY_SMIMEENCRYPT,
		Security::isSMIMEEnabled());
	ADD_ACTION3(EditToolMessageSecurityAction,
		IDM_TOOL_SMIMESIGN,
		pEditWindow_->getEditMessageHolder(),
		MESSAGESECURITY_SMIMESIGN,
		Security::isSMIMEEnabled());
	ADD_ACTION4(ToolScriptAction,
		IDM_TOOL_SCRIPT,
		pDocument_->getScriptManager(),
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
	
	HWND hwndToolbar = pThis_->getToolbar();
	RECT rectToolbar;
	Window wndToolbar(hwndToolbar);
	wndToolbar.getWindowRect(&rectToolbar);
#ifndef _WIN32_WCE
	pThis_->screenToClient(&rectToolbar);
	int nToolbarHeight = rectToolbar.bottom;
#else
	int nToolbarHeight = rectToolbar.bottom - rectToolbar.top;
#endif
#if _WIN32_WCE >= 0x300 && defined _WIN32_WCE_PSPC
	int nTopBarHeight = 0;
	int nBottomBarHeight = bShowToolbar_ ? nToolbarHeight : 0;
#else
	int nTopBarHeight = bShowToolbar_ ? nToolbarHeight : 0;
	int nBottomBarHeight = 0;
#endif
	
	RECT rectStatusBar;
	pStatusBar_->getWindowRect(&rectStatusBar);
	int nStatusBarHeight = bShowStatusBar_ ?
		rectStatusBar.bottom - rectStatusBar.top : 0;
	
	HDWP hdwp = Window::beginDeferWindowPos(3);
	
#if _WIN32_WCE >= 0x300 && defined _WIN32_WCE_PSPC
	hdwp = wndToolbar.deferWindowPos(hdwp, 0, 0,
		cy - nToolbarHeight, cx, nToolbarHeight, SWP_NOZORDER);
#else
	hdwp = wndToolbar.deferWindowPos(hdwp, 0, 0, 0, cx,
		nToolbarHeight, SWP_NOMOVE | SWP_NOZORDER);
#endif
	wndToolbar.showWindow(bShowToolbar_ ? SW_SHOW : SW_HIDE);
	
	hdwp = pStatusBar_->deferWindowPos(hdwp, 0, 0,
		cy - nStatusBarHeight - nBottomBarHeight, cx,
		rectStatusBar.bottom - rectStatusBar.top, SWP_NOZORDER);
	pStatusBar_->showWindow(bShowStatusBar_ ? SW_SHOW : SW_HIDE);
	
	hdwp = pEditWindow_->deferWindowPos(hdwp, 0, 0, nTopBarHeight, cx,
		cy - nStatusBarHeight - nTopBarHeight - nBottomBarHeight, SWP_NOZORDER);
	
	Window::endDeferWindowPos(hdwp);
	
	const double dBase = qs::UIUtil::getLogPixel()/96.0;
	int nWidth[] = {
		cx - static_cast<int>(110*dBase),
		cx - static_cast<int>(80*dBase),
		cx - static_cast<int>(50*dBase),
		-1
	};
	pStatusBar_->setParts(nWidth, countof(nWidth));
	
	bLayouting_ = false;
}

Account* qm::EditFrameWindowImpl::getAccount()
{
	EditMessage* pEditMessage = pEditWindow_->getEditMessageHolder()->getEditMessage();
	return pEditMessage ? pEditMessage->getAccount() : 0;
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
	pImpl_->bShowToolbar_ = pProfile->getInt(L"EditFrameWindow", L"ShowToolbar") != 0;
	pImpl_->bShowStatusBar_ = pProfile->getInt(L"EditFrameWindow", L"ShowStatusBar") != 0;
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
	
	ConfirmSendDialog dialog;
	switch (dialog.doModal(getHandle())) {
	case ConfirmSendDialog::ID_SEND:
		{
			Action* pAction = pImpl_->pActionMap_->getAction(IDM_FILE_SEND);
			assert(pAction);
			pAction->invoke(ActionEvent(IDM_FILE_SEND, 0));
			return true;
		}
		break;
	case ConfirmSendDialog::ID_SAVE:
		{
			Action* pAction = pImpl_->pActionMap_->getAction(IDM_FILE_DRAFTCLOSE);
			assert(pAction);
			pAction->invoke(ActionEvent(IDM_FILE_DRAFTCLOSE, 0));
			return true;
		}
		break;
	case ConfirmSendDialog::ID_DISCARD:
		bCancel = false;
		break;
	default:
		break;
	}
	if (!bCancel)
		pImpl_->pManager_->close(this);
	
	return !bCancel;
}

void qm::EditFrameWindow::layout()
{
	pImpl_->layoutChildren();
	pImpl_->pEditWindow_->layout();
}

void qm::EditFrameWindow::reloadProfiles()
{
	pImpl_->pEditWindow_->reloadProfiles();
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

#ifdef _WIN32_WCE
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
	_snwprintf(wszKey, countof(wszKey), L"CommandBandsRestoreInfo%d", n);
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
	_snwprintf(wszKey, countof(wszKey), L"CommandBandsRestoreInfo%d", n);
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
	return IDI_EDIT;
}

const DynamicMenuItem* qm::EditFrameWindow::getDynamicMenuItem(unsigned int nId) const
{
	return pImpl_->pUIManager_->getDynamicMenuMap()->getItem(nId);
}

DynamicMenuCreator* qm::EditFrameWindow::getDynamicMenuCreator(const DynamicMenuItem* pItem)
{
	if (pItem->getParam()) {
		if (!pImpl_->pMacroMenuCreator_.get())
			pImpl_->pMacroMenuCreator_.reset(new MacroMenuCreator(
				pImpl_->pDocument_, pImpl_, pImpl_->pSecurityModel_,
				pImpl_->pProfile_, actionItems, countof(actionItems),
				pImpl_->pUIManager_->getActionParamMap()));
		return pImpl_->pMacroMenuCreator_.get();
	}
	else {
		EditFrameWindowImpl::MenuCreatorList::const_iterator it = std::find_if(
			pImpl_->listMenuCreator_.begin(), pImpl_->listMenuCreator_.end(),
			std::bind2nd(
				binary_compose_f_gx_hy(
					string_equal<WCHAR>(),
					std::mem_fun(&MenuCreator::getName),
					std::identity<const WCHAR*>()),
				pItem->getName()));
		return it != pImpl_->listMenuCreator_.end() ? *it : 0;
	}
}

void qm::EditFrameWindow::getWindowClass(WNDCLASS* pwc)
{
	FrameWindow::getWindowClass(pwc);
	pwc->hIcon = ::LoadIcon(Application::getApplication().getResourceHandle(),
		MAKEINTRESOURCE(IDI_EDIT));
	pwc->hbrBackground = reinterpret_cast<HBRUSH>(COLOR_BTNFACE + 1);
}

bool qm::EditFrameWindow::preCreateWindow(CREATESTRUCT* pCreateStruct)
{
#if defined _WIN32_WCE && _WIN32_WCE >= 0x300 && defined _WIN32_WCE_PSPC
	RECT rect;
	qs::UIUtil::getWorkArea(&rect);
	pCreateStruct->x = rect.left;
	pCreateStruct->y = rect.top;
	pCreateStruct->cx = rect.right - rect.left;
	pCreateStruct->cy = rect.bottom - rect.top;
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

const ActionParam* qm::EditFrameWindow::getActionParam(UINT nId)
{
	return pImpl_->pUIManager_->getActionParamMap()->getActionParam(nId);
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
	
	if (nFlags == WA_INACTIVE)
		pImpl_->pEditWindow_->saveFocusedItem();
	else
		pImpl_->pEditWindow_->restoreFocusedItem();
	
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
	pImpl_->pPasswordManager_ = pContext->pPasswordManager_;
	pImpl_->pSyncManager_ = pContext->pSyncManager_;
	pImpl_->pSyncDialogManager_ = pContext->pSyncDialogManager_;
	pImpl_->pOptionDialogManager_ = pContext->pOptionDialogManager_;
	pImpl_->pSecurityModel_ = pContext->pSecurityModel_;
	
	CustomAcceleratorFactory acceleratorFactory;
	pImpl_->pAccelerator_ = pContext->pUIManager_->getKeyMap()->createAccelerator(
		&acceleratorFactory, L"EditFrameWindow");
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
	
	DWORD dwStatusBarStyle = dwStyle;
#if _WIN32_WCE >= 0x300 && defined _WIN32_WCE_PSPC
	dwStatusBarStyle |= CCS_NOPARENTALIGN;
#endif
	std::auto_ptr<StatusBar> pStatusBar(new StatusBar());
	if (!pStatusBar->create(L"QmStatusBarWindow", 0, dwStatusBarStyle,
		CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, getHandle(),
		0, STATUSCLASSNAMEW, EditFrameWindowImpl::ID_STATUSBAR, 0))
		return -1;
	pImpl_->pStatusBar_ = pStatusBar.release();
	
	pImpl_->listMenuCreator_.push_back(
		new InsertTextMenuCreator(pImpl_->pDocument_->getFixedFormTextManager(),
			pImpl_->pUIManager_->getActionParamMap()));
	pImpl_->listMenuCreator_.push_back(
		new EncodingMenuCreator(pImpl_->pProfile_, false,
			pImpl_->pUIManager_->getActionParamMap()));
	pImpl_->listMenuCreator_.push_back(
		new ScriptMenuCreator(pImpl_->pDocument_->getScriptManager(),
			pImpl_->pUIManager_->getActionParamMap()));
	
	pImpl_->layoutChildren();
	
	pImpl_->initActions();
	
#if !defined _WIN32_WCE && _WIN32_WINNT >= 0x500
	UIUtil::setWindowAlpha(getHandle(), pImpl_->pProfile_, L"EditFrameWindow");
#endif
	
	pImpl_->bCreated_ = true;
	
	return 0;
}

LRESULT qm::EditFrameWindow::onDestroy()
{
	Profile* pProfile = pImpl_->pProfile_;
	
	pProfile->setInt(L"EditFrameWindow", L"ShowToolbar", pImpl_->bShowToolbar_);
	pProfile->setInt(L"EditFrameWindow", L"ShowStatusBar", pImpl_->bShowStatusBar_);
	
	if (pImpl_->pToolbarCookie_)
		pImpl_->pUIManager_->getToolbarManager()->destroy(pImpl_->pToolbarCookie_);
	
	std::for_each(pImpl_->listMenuCreator_.begin(),
		pImpl_->listMenuCreator_.end(), qs::deleter<DynamicMenuCreator>());
	
	UIUtil::saveWindowPlacement(getHandle(), pProfile, L"EditFrameWindow");
	FrameWindow::save();
	
	return FrameWindow::onDestroy();
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
												   PasswordManager* pPasswordManager,
												   SyncManager* pSyncManager,
												   SyncDialogManager* pSyncDialogManager,
												   OptionDialogManager* pOptionDialogManager,
												   Profile* pProfile,
												   SecurityModel* pSecurityModel) :
	pDocument_(pDocument),
	pUIManager_(pUIManager),
	pPasswordManager_(pPasswordManager),
	pSyncManager_(pSyncManager),
	pSyncDialogManager_(pSyncDialogManager),
	pOptionDialogManager_(pOptionDialogManager),
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
#if defined _WIN32_WCE && _WIN32_WCE >= 0x300 && defined _WIN32_WCE_PSPC
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
		pPasswordManager_,
		pSyncManager_,
		pSyncDialogManager_,
		pOptionDialogManager_,
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

void qm::EditFrameWindowManager::showAll()
{
	for (FrameList::const_iterator it = listFrame_.begin(); it != listFrame_.end(); ++it)
		(*it)->showWindow();
}

void qm::EditFrameWindowManager::hideAll()
{
	for (FrameList::const_iterator it = listFrame_.begin(); it != listFrame_.end(); ++it)
		(*it)->showWindow(SW_HIDE);
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

void qm::EditFrameWindowManager::layout()
{
	for (FrameList::iterator it = listFrame_.begin(); it != listFrame_.end(); ++it)
		(*it)->layout();
}

void qm::EditFrameWindowManager::reloadProfiles()
{
	for (FrameList::iterator it = listFrame_.begin(); it != listFrame_.end(); ++it)
		(*it)->reloadProfiles();
}
