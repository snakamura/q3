/*
 * $Id: messageframewindow.cpp,v 1.7 2003/05/25 07:24:24 snakamura Exp $
 *
 * Copyright(C) 1998-2003 Satoshi Nakamura
 * All rights reserved.
 *
 */

#include <qmapplication.h>
#include <qmdocument.h>
#include <qmmessagewindow.h>

#include <qsaccelerator.h>
#include <qsaction.h>
#include <qsnew.h>

#include <algorithm>

#ifdef _WIN32_WCE_PSPC
#	include <aygshell.h>
#endif

#include "keymap.h"
#include "menus.h"
#include "messageframewindow.h"
#include "messagemodel.h"
#include "messageselectionmodel.h"
#include "messagewindow.h"
#include "resourceinc.h"
#include "statusbar.h"
#include "../action/action.h"
#include "../action/findreplace.h"
#include "../model/security.h"

#pragma warning(disable:4786)

using namespace qm;
using namespace qs;


/****************************************************************************
 *
 * MessageFrameWindowImpl
 *
 */

class qm::MessageFrameWindowImpl :
	public MessageModelHandler,
	public FolderModel,
	public MessageSelectionModel
{
public:
	enum {
		ID_MESSAGEWINDOW		= 1001,
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
	virtual QSTATUS messageChanged(const MessageModelEvent& event);

public:
	virtual Account* getCurrentAccount() const;
	virtual QSTATUS setCurrentAccount(Account* pAccount);
	virtual Folder* getCurrentFolder() const;
	virtual QSTATUS setCurrentFolder(Folder* pFolder);
	virtual QSTATUS addFolderModelHandler(FolderModelHandler* pHandler);
	virtual QSTATUS removeFolderModelHandler(FolderModelHandler* pHandler);

public:
	virtual QSTATUS getSelectedMessages(
		Folder** ppFolder, MessagePtrList* pList);
	virtual QSTATUS hasSelectedMessage(bool* pbHas);
	virtual QSTATUS getFocusedMessage(MessagePtr* pptr);
	virtual QSTATUS hasFocusedMessage(bool* pbHas);
	virtual QSTATUS selectAll();
	virtual QSTATUS canSelect(bool* pbCan);

public:
	MessageFrameWindow* pThis_;
	
	bool bShowToolbar_;
	bool bShowStatusBar_;
	
	MessageFrameWindowManager* pMessageFrameWindowManager_;
	Profile* pProfile_;
	Document* pDocument_;
	TempFileCleaner* pTempFileCleaner_;
	ViewModelManager* pViewModelManager_;
	MessageWindow* pMessageWindow_;
	StatusBar* pStatusBar_;
	Accelerator* pAccelerator_;
	ActionMap* pActionMap_;
	ActionInvoker* pActionInvoker_;
	FindReplaceManager* pFindReplaceManager_;
	EditFrameWindowManager* pEditFrameWindowManager_;
	MoveMenu* pMoveMenu_;
	AttachmentMenu* pAttachmentMenu_;
	ViewTemplateMenu* pViewTemplateMenu_;
	CreateTemplateMenu* pCreateTemplateMenu_;
	CreateTemplateMenu* pCreateTemplateExternalMenu_;
	EncodingMenu* pEncodingMenu_;
	ScriptMenu* pScriptMenu_;
	bool bCreated_;
	bool bMaximize_;
	bool bLayouting_;
};

QSTATUS qm::MessageFrameWindowImpl::initActions()
{
	DECLARE_QSTATUS();
	
	status = newQsObject(&pActionMap_);
	CHECK_QSTATUS();
	status = newQsObject(pActionMap_, &pActionInvoker_);
	CHECK_QSTATUS();
	status = newQsObject(&pFindReplaceManager_);
	CHECK_QSTATUS();
	
	status = InitAction3<EditCommandAction, MessageWindow*,
		EditCommandAction::PFN_DO, EditCommandAction::PFN_CANDO>(
		pActionMap_, IDM_EDIT_COPY, pMessageWindow_,
		&MessageWindowItem::copy, &MessageWindowItem::canCopy);
	CHECK_QSTATUS();
	status = InitAction2<EditDeleteMessageAction, MessageSelectionModel*, bool>(
		pActionMap_, IDM_EDIT_DELETE, this, false);
	CHECK_QSTATUS();
	status = InitAction2<EditDeleteMessageAction, MessageSelectionModel*, bool>(
		pActionMap_, IDM_EDIT_DELETEDIRECT, this, true);
	CHECK_QSTATUS();
	status = InitAction3<EditFindAction, MessageWindow*, Profile*, FindReplaceManager*>(
		pActionMap_, IDM_EDIT_FIND, pMessageWindow_, pProfile_, pFindReplaceManager_);
	CHECK_QSTATUS();
	status = InitAction3<EditFindAction, MessageWindow*, bool, FindReplaceManager*>(
		pActionMap_, IDM_EDIT_FINDNEXT, pMessageWindow_, true, pFindReplaceManager_);
	CHECK_QSTATUS();
	status = InitAction3<EditFindAction, MessageWindow*, bool, FindReplaceManager*>(
		pActionMap_, IDM_EDIT_FINDPREV, pMessageWindow_, false, pFindReplaceManager_);
	CHECK_QSTATUS();
	status = InitAction3<EditCommandAction, MessageWindow*,
		EditCommandAction::PFN_DO, EditCommandAction::PFN_CANDO>(
		pActionMap_, IDM_EDIT_SELECTALL, pMessageWindow_,
		&MessageWindowItem::selectAll, &MessageWindowItem::canSelectAll);
	CHECK_QSTATUS();
	status = InitAction1<FileCloseAction, HWND>(
		pActionMap_, IDM_FILE_CLOSE, pThis_->getHandle());
	CHECK_QSTATUS();
#if defined _WIN32_WCE && _WIN32_WCE >= 300 && defined _WIN32_WCE_PSPC
	status = InitAction1<FileCloseAction, HWND>(
		pActionMap_, IDOK, pThis_->getHandle());
	CHECK_QSTATUS();
#endif
	status = InitActionRange8<MessageApplyTemplateAction, TemplateMenu*,
		Document*, FolderModel*, MessageSelectionModel*,
		EditFrameWindowManager*, HWND, Profile*, bool>(
		pActionMap_, IDM_MESSAGE_APPLYTEMPLATE, IDM_MESSAGE_APPLYTEMPLATE + 100,
		pCreateTemplateMenu_, pDocument_, this, this,
		pEditFrameWindowManager_, pThis_->getHandle(), pProfile_, false);
	CHECK_QSTATUS();
	status = InitActionRange8<MessageApplyTemplateAction, TemplateMenu*,
		Document*, FolderModel*, MessageSelectionModel*,
		EditFrameWindowManager*, HWND, Profile*, bool>(
		pActionMap_, IDM_MESSAGE_APPLYTEMPLATEEXTERNAL, IDM_MESSAGE_APPLYTEMPLATEEXTERNAL + 100,
		pCreateTemplateExternalMenu_, pDocument_, this, this,
		pEditFrameWindowManager_, pThis_->getHandle(), pProfile_, true);
	CHECK_QSTATUS();
	
	struct {
		UINT nId_;
		UINT nIdExternal_;
		const WCHAR* pwszName_;
	} creates[] = {
		{ IDM_MESSAGE_EDIT,		IDM_MESSAGE_EDITEXTERNAL,		L"edit"			},
		{ IDM_MESSAGE_FORWARD,	IDM_MESSAGE_FORWARDEXTERNAL,	L"forward"		},
		{ IDM_MESSAGE_NEW,		IDM_MESSAGE_NEWEXTERNAL,		L"new"			},
		{ IDM_MESSAGE_REPLY,	IDM_MESSAGE_REPLYEXTERNAL,		L"reply"		},
		{ IDM_MESSAGE_REPLYALL,	IDM_MESSAGE_REPLYALLEXTERNAL,	L"reply_all"	},
	};
	for (int n = 0; n < countof(creates); ++n) {
		status = InitAction8<MessageCreateAction, Document*,
			FolderModel*, MessageSelectionModel*, const WCHAR*,
			EditFrameWindowManager*, HWND, Profile*, bool>(
			pActionMap_, creates[n].nId_, pDocument_, this,
			this, creates[n].pwszName_, pEditFrameWindowManager_,
			pThis_->getHandle(), pProfile_, false);
		CHECK_QSTATUS();
		status = InitAction8<MessageCreateAction, Document*,
			FolderModel*, MessageSelectionModel*, const WCHAR*,
			EditFrameWindowManager*, HWND, Profile*, bool>(
			pActionMap_, creates[n].nIdExternal_, pDocument_, this,
			this, creates[n].pwszName_, pEditFrameWindowManager_,
			pThis_->getHandle(), pProfile_, true);
		CHECK_QSTATUS();
	}
	status = InitAction2<MessageDetachAction, Profile*, MessageSelectionModel*>(
		pActionMap_, IDM_MESSAGE_DETACH, pProfile_, this);
	CHECK_QSTATUS();
	status = InitActionRange3<MessageExecuteAttachmentAction,
		Profile*, AttachmentMenu*, TempFileCleaner*>(
		pActionMap_, IDM_MESSAGE_ATTACHMENT, IDM_MESSAGE_ATTACHMENT + 100,
		pProfile_, pAttachmentMenu_, pTempFileCleaner_);
	CHECK_QSTATUS();
	status = InitAction3<MessageMarkAction, MessageSelectionModel*,
		unsigned int, unsigned int>(
		pActionMap_, IDM_MESSAGE_MARK, this,
		MessageHolder::FLAG_MARKED, MessageHolder::FLAG_MARKED);
	CHECK_QSTATUS();
	status = InitAction3<MessageMarkAction, MessageSelectionModel*,
		unsigned int, unsigned int>(
		pActionMap_, IDM_MESSAGE_MARKDOWNLOAD, this,
		MessageHolder::FLAG_DOWNLOAD, MessageHolder::FLAG_DOWNLOAD);
	CHECK_QSTATUS();
	status = InitAction3<MessageMarkAction, MessageSelectionModel*,
		unsigned int, unsigned int>(
		pActionMap_, IDM_MESSAGE_MARKSEEN, this,
		MessageHolder::FLAG_SEEN, MessageHolder::FLAG_SEEN);
	CHECK_QSTATUS();
	status = InitAction3<MessageMarkAction, MessageSelectionModel*,
		unsigned int, unsigned int>(
		pActionMap_, IDM_MESSAGE_UNMARK, this,
		0, MessageHolder::FLAG_MARKED);
	CHECK_QSTATUS();
	status = InitAction3<MessageMarkAction, MessageSelectionModel*,
		unsigned int, unsigned int>(
		pActionMap_, IDM_MESSAGE_MARKUNSEEN, this,
		0, MessageHolder::FLAG_SEEN);
	CHECK_QSTATUS();
	status = InitActionRange2<MessageMoveAction, MessageSelectionModel*, MoveMenu*>(
		pActionMap_, IDM_MESSAGE_MOVE, IDM_MESSAGE_MOVE + 900, this, pMoveMenu_);
	CHECK_QSTATUS();
	status = InitAction1<MessageMoveOtherAction, MessageSelectionModel*>(
		pActionMap_, IDM_MESSAGE_MOVEOTHER, this);
	CHECK_QSTATUS();
	status = InitAction2<MessagePropertyAction, MessageSelectionModel*, HWND>(
		pActionMap_, IDM_MESSAGE_PROPERTY, this, pThis_->getHandle());
	CHECK_QSTATUS();
	status = InitActionRange4<ToolScriptAction,
		ScriptMenu*, Document*, Profile*, MessageFrameWindow*>(
		pActionMap_, IDM_TOOL_SCRIPT, IDM_TOOL_SCRIPT + 100,
		pScriptMenu_, pDocument_, pProfile_, pThis_);
	CHECK_QSTATUS();
	status = InitAction1<ViewEncodingAction, MessageWindow*>(
		pActionMap_, IDM_VIEW_ENCODINGAUTODETECT, pMessageWindow_);
	CHECK_QSTATUS();
	status = InitActionRange2<ViewEncodingAction, MessageWindow*, EncodingMenu*>(
		pActionMap_, IDM_VIEW_ENCODING, IDM_VIEW_ENCODING + 100,
		pMessageWindow_, pEncodingMenu_);
	CHECK_QSTATUS();
	status = InitAction4<ViewMessageModeAction, MessageWindow*,
		ViewMessageModeAction::PFN_IS, ViewMessageModeAction::PFN_SET, bool>(
		pActionMap_, IDM_VIEW_DECRYPTVERIFYMODE, pMessageWindow_,
		&MessageWindow::isDecryptVerifyMode, &MessageWindow::setDecryptVerifyMode,
		Security::isEnabled());
	CHECK_QSTATUS();
	status = InitAction4<ViewMessageModeAction, MessageWindow*,
		ViewMessageModeAction::PFN_IS, ViewMessageModeAction::PFN_SET, bool>(
		pActionMap_, IDM_VIEW_HTMLMODE, pMessageWindow_,
		&MessageWindow::isHtmlMode, &MessageWindow::setHtmlMode, true);
	CHECK_QSTATUS();
	status = InitAction4<ViewNavigateMessageAction, ViewModelManager*,
		MessageModel*, MessageWindow*, ViewNavigateMessageAction::Type>(
		pActionMap_, IDM_VIEW_NEXTMESSAGE, pViewModelManager_,
		pMessageWindow_->getMessageModel(), pMessageWindow_,
		ViewNavigateMessageAction::TYPE_NEXT);
	CHECK_QSTATUS();
	status = InitAction4<ViewNavigateMessageAction, ViewModelManager*,
		MessageModel*, MessageWindow*, ViewNavigateMessageAction::Type>(
		pActionMap_, IDM_VIEW_PREVMESSAGE, pViewModelManager_,
		pMessageWindow_->getMessageModel(), pMessageWindow_,
		ViewNavigateMessageAction::TYPE_PREV);
	CHECK_QSTATUS();
	status = InitAction4<ViewNavigateMessageAction, ViewModelManager*,
		MessageModel*, MessageWindow*, ViewNavigateMessageAction::Type>(
		pActionMap_, IDM_VIEW_NEXTUNSEENMESSAGE, pViewModelManager_,
		pMessageWindow_->getMessageModel(), pMessageWindow_,
		ViewNavigateMessageAction::TYPE_NEXTUNSEEN);
	CHECK_QSTATUS();
	status = InitAction4<ViewNavigateMessageAction, ViewModelManager*,
		MessageModel*, MessageWindow*, ViewNavigateMessageAction::Type>(
		pActionMap_, IDM_VIEW_NEXTMESSAGEPAGE, pViewModelManager_,
		pMessageWindow_->getMessageModel(), pMessageWindow_,
		ViewNavigateMessageAction::TYPE_NEXTPAGE);
	CHECK_QSTATUS();
	status = InitAction4<ViewNavigateMessageAction, ViewModelManager*,
		MessageModel*, MessageWindow*, ViewNavigateMessageAction::Type>(
		pActionMap_, IDM_VIEW_PREVMESSAGEPAGE, pViewModelManager_,
		pMessageWindow_->getMessageModel(), pMessageWindow_,
		ViewNavigateMessageAction::TYPE_PREVPAGE);
	CHECK_QSTATUS();
	status = InitAction4<ViewMessageModeAction, MessageWindow*,
		ViewMessageModeAction::PFN_IS, ViewMessageModeAction::PFN_SET, bool>(
		pActionMap_, IDM_VIEW_RAWMODE, pMessageWindow_,
		&MessageWindow::isRawMode, &MessageWindow::setRawMode, true);
	CHECK_QSTATUS();
	status = InitAction1<ViewSelectModeAction, MessageWindow*>(
		pActionMap_, IDM_VIEW_SELECTMODE, pMessageWindow_);
	CHECK_QSTATUS();
	status = InitAction1<ViewShowHeaderAction, MessageWindow*>(
		pActionMap_, IDM_VIEW_SHOWHEADER, pMessageWindow_);
	CHECK_QSTATUS();
	status = InitAction1<ViewShowStatusBarAction<MessageFrameWindow>, MessageFrameWindow*>(
		pActionMap_, IDM_VIEW_SHOWSTATUSBAR, pThis_);
	CHECK_QSTATUS();
	status = InitAction1<ViewShowToolbarAction<MessageFrameWindow>, MessageFrameWindow*>(
		pActionMap_, IDM_VIEW_SHOWTOOLBAR, pThis_);
	CHECK_QSTATUS();
	status = InitActionRange2<ViewTemplateAction, MessageWindow*, TemplateMenu*>(
		pActionMap_, IDM_VIEW_TEMPLATE, IDM_VIEW_TEMPLATE + 100,
		pMessageWindow_, pViewTemplateMenu_);
	CHECK_QSTATUS();
	status = InitAction1<ViewTemplateAction, MessageWindow*>(
		pActionMap_, IDM_VIEW_TEMPLATENONE, pMessageWindow_);
	CHECK_QSTATUS();
	
	return QSTATUS_SUCCESS;
}

QSTATUS qm::MessageFrameWindowImpl::layoutChildren()
{
	RECT rect;
	pThis_->getClientRect(&rect);
	return layoutChildren(rect.right - rect.left, rect.bottom - rect.top);
}

QSTATUS qm::MessageFrameWindowImpl::layoutChildren(int cx, int cy)
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
	
	pMessageWindow_->setWindowPos(0, 0, nToolbarHeight, cx,
		cy - nStatusBarHeight - nToolbarHeight, SWP_NOZORDER);
	
	bLayouting_ = false;
	
	return QSTATUS_SUCCESS;
}

QSTATUS qm::MessageFrameWindowImpl::messageChanged(const MessageModelEvent& event)
{
	if (!event.getMessageHolder())
		pThis_->postMessage(WM_CLOSE);
	
	return QSTATUS_SUCCESS;
}

Account* qm::MessageFrameWindowImpl::getCurrentAccount() const
{
	MessageModel* pMessageModel = pThis_->getMessageModel();
	ViewModel* pViewModel = pMessageModel->getViewModel();
	return pViewModel ? 0 : pMessageModel->getCurrentAccount();
}

QSTATUS qm::MessageFrameWindowImpl::setCurrentAccount(Account* pAccount)
{
	assert(false);
	return QSTATUS_FAIL;
}

Folder* qm::MessageFrameWindowImpl::getCurrentFolder() const
{
	MessageModel* pMessageModel = pThis_->getMessageModel();
	ViewModel* pViewModel = pMessageModel->getViewModel();
	return pViewModel ? pViewModel->getFolder() : 0;
}

QSTATUS qm::MessageFrameWindowImpl::setCurrentFolder(Folder* pFolder)
{
	assert(false);
	return QSTATUS_FAIL;
}

QSTATUS qm::MessageFrameWindowImpl::addFolderModelHandler(FolderModelHandler* pHandler)
{
	assert(false);
	return QSTATUS_FAIL;
}

QSTATUS qm::MessageFrameWindowImpl::removeFolderModelHandler(FolderModelHandler* pHandler)
{
	assert(false);
	return QSTATUS_FAIL;
}

QSTATUS qm::MessageFrameWindowImpl::getSelectedMessages(
	Folder** ppFolder, MessagePtrList* pList)
{
	assert(pList);
	
	DECLARE_QSTATUS();
	
	MessagePtr ptr(pThis_->getMessageModel()->getCurrentMessage());
	Folder* pFolder = ptr.getFolder();
	if (pFolder) {
		status = STLWrapper<MessagePtrList>(*pList).push_back(ptr);
		CHECK_QSTATUS();
	}
	
	if (ppFolder)
		*ppFolder = pFolder;
	
	return QSTATUS_SUCCESS;
}

QSTATUS qm::MessageFrameWindowImpl::hasSelectedMessage(bool* pbHas)
{
	assert(pbHas);
	*pbHas = true;
	return QSTATUS_SUCCESS;
}

QSTATUS qm::MessageFrameWindowImpl::getFocusedMessage(MessagePtr* pptr)
{
	assert(pptr);
	*pptr = pThis_->getMessageModel()->getCurrentMessage();
	return QSTATUS_SUCCESS;
}

QSTATUS qm::MessageFrameWindowImpl::hasFocusedMessage(bool* pbHas)
{
	assert(pbHas);
	*pbHas = true;
	return QSTATUS_SUCCESS;
}

QSTATUS qm::MessageFrameWindowImpl::selectAll()
{
	return QSTATUS_FAIL;
}

QSTATUS qm::MessageFrameWindowImpl::canSelect(bool* pbCan)
{
	*pbCan = false;
	return QSTATUS_SUCCESS;
}


/****************************************************************************
 *
 * MessageFrameWindow
 *
 */

qm::MessageFrameWindow::MessageFrameWindow(
	MessageFrameWindowManager* pMessageFrameWindowManager, Profile* pProfile,
	ViewModelManager* pViewModelManager, QSTATUS* pstatus) :
	FrameWindow(Application::getApplication().getResourceHandle(), true, pstatus),
	pImpl_(0)
{
	assert(pProfile);
	assert(pstatus);
	
	DECLARE_QSTATUS();
	
	if (*pstatus != QSTATUS_SUCCESS)
		return;
	
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
	pImpl_->pMessageFrameWindowManager_ = pMessageFrameWindowManager;
	pImpl_->pProfile_ = pProfile;
	pImpl_->pViewModelManager_ = pViewModelManager;
	pImpl_->pMessageWindow_ = 0;
	pImpl_->pStatusBar_ = 0;
	pImpl_->pAccelerator_ = 0;
	pImpl_->pActionMap_ = 0;
	pImpl_->pActionInvoker_ = 0;
	pImpl_->pFindReplaceManager_ = 0;
	pImpl_->pMoveMenu_ = 0;
	pImpl_->pAttachmentMenu_ = 0;
	pImpl_->pViewTemplateMenu_ = 0;
	pImpl_->pCreateTemplateMenu_ = 0;
	pImpl_->pCreateTemplateExternalMenu_ = 0;
	pImpl_->pEncodingMenu_ = 0;
	pImpl_->pScriptMenu_ = 0;
	pImpl_->bCreated_ = false;
	pImpl_->bMaximize_ = false;
	pImpl_->bLayouting_ = false;
}

qm::MessageFrameWindow::~MessageFrameWindow()
{
	if (pImpl_) {
		delete pImpl_->pAccelerator_;
		delete pImpl_->pActionMap_;
		delete pImpl_->pActionInvoker_;
		delete pImpl_->pFindReplaceManager_;
		delete pImpl_->pMoveMenu_;
		delete pImpl_->pAttachmentMenu_;
		delete pImpl_->pViewTemplateMenu_;
		delete pImpl_->pCreateTemplateMenu_;
		delete pImpl_->pCreateTemplateExternalMenu_;
		delete pImpl_->pEncodingMenu_;
		delete pImpl_->pScriptMenu_;
		delete pImpl_;
		pImpl_ = 0;
	}
}

MessageModel* qm::MessageFrameWindow::getMessageModel() const
{
	return pImpl_->pMessageWindow_->getMessageModel();
}

bool qm::MessageFrameWindow::isShowToolbar() const
{
	return pImpl_->bShowToolbar_;
}

QSTATUS qm::MessageFrameWindow::setShowToolbar(bool bShow)
{
	DECLARE_QSTATUS();
	
	if (bShow != pImpl_->bShowToolbar_) {
		pImpl_->bShowToolbar_ = bShow;
		status = pImpl_->layoutChildren();
	}
	
	return status;
}

bool qm::MessageFrameWindow::isShowStatusBar() const
{
	return pImpl_->bShowStatusBar_;
}

QSTATUS qm::MessageFrameWindow::setShowStatusBar(bool bShow)
{
	DECLARE_QSTATUS();
	
	if (bShow != pImpl_->bShowStatusBar_) {
		pImpl_->bShowStatusBar_ = bShow;
		status = pImpl_->layoutChildren();
	}
	
	return status;
}

const ActionInvoker* qm::MessageFrameWindow::getActionInvoker() const
{
	return pImpl_->pActionInvoker_;
}

QSTATUS qm::MessageFrameWindow::getToolbarButtons(Toolbar* pToolbar, bool* pbToolbar)
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
	pToolbar->nId_ = MessageFrameWindowImpl::ID_TOOLBAR;
	pToolbar->nBitmapId_ = IDB_TOOLBAR;
	
	*pbToolbar = true;
	
	return QSTATUS_SUCCESS;
}

#if defined _WIN32_WCE && (_WIN32_WCE < 300 || !defined _WIN32_WCE_PSPC)
QSTATUS qm::MessageFrameWindow::getBarId(int n, UINT* pnId) const
{
	assert(n == 0 || n == 1);
	assert(pnId);
	static UINT nIds[] = {
		MessageFrameWindowImpl::ID_COMMANDBARMENU,
		MessageFrameWindowImpl::ID_COMMANDBARBUTTON
	};
	*pnId = nIds[n];
	return QSTATUS_SUCCESS;
}

QSTATUS qm::MessageFrameWindow::getCommandBandsRestoreInfo(
	int n, COMMANDBANDSRESTOREINFO* pcbri) const
{
	DECLARE_QSTATUS();
	
	WCHAR wszKey[32];
	swprintf(wszKey, L"CommandBandsRestoreInfo%d", n);
	int nSize = sizeof(*pcbri);
	status = pImpl_->pProfile_->getBinary(L"MessageFrameWindow", wszKey,
		reinterpret_cast<unsigned char*>(pcbri), &nSize);
	if (nSize != sizeof(*pcbri))
		pcbri->cbSize = 0;
	return status;
}

QSTATUS qm::MessageFrameWindow::setCommandBandsRestoreInfo(
	int n, const COMMANDBANDSRESTOREINFO& cbri)
{
	WCHAR wszKey[32];
	swprintf(wszKey, L"CommandBandsRestoreInfo%d", n);
	return pImpl_->pProfile_->setBinary(L"MessageFrameWindow", wszKey,
		reinterpret_cast<const unsigned char*>(&cbri), sizeof(cbri));
}
#endif

QSTATUS qm::MessageFrameWindow::getMenuHandle(void* pCreateParam, HMENU* phmenu)
{
	assert(phmenu);
	
	DECLARE_QSTATUS();
	
	MessageFrameWindowCreateContext* pContext =
		static_cast<MessageFrameWindowCreateContext*>(pCreateParam);
	
	status = pContext->pMenuManager_->getMenu(
		L"messageframe", true, true, phmenu);
	CHECK_QSTATUS();
	
	return QSTATUS_SUCCESS;
}

QSTATUS qm::MessageFrameWindow::getIconId(UINT* pnId)
{
	assert(pnId);
	*pnId = IDI_MAINFRAME;
	return QSTATUS_SUCCESS;
}

QSTATUS qm::MessageFrameWindow::getWindowClass(WNDCLASS* pwc)
{
	DECLARE_QSTATUS();
	
	status = DefaultWindowHandler::getWindowClass(pwc);
	CHECK_QSTATUS();
	
	pwc->hIcon = ::LoadIcon(Application::getApplication().getResourceHandle(),
		MAKEINTRESOURCE(IDI_MAINFRAME));
	pwc->hbrBackground = reinterpret_cast<HBRUSH>(COLOR_BTNFACE + 1);
	
	return QSTATUS_SUCCESS;
}

QSTATUS qm::MessageFrameWindow::preCreateWindow(CREATESTRUCT* pCreateStruct)
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
	status = pImpl_->pProfile_->getBinary(L"MessageFrameWindow", L"WindowPlacement",
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

QSTATUS qm::MessageFrameWindow::getAction(UINT nId, Action** ppAction)
{
	assert(ppAction);
	*ppAction = pImpl_->pActionMap_->getAction(nId);
	return QSTATUS_SUCCESS;
}

QSTATUS qm::MessageFrameWindow::getAccelerator(Accelerator** ppAccelerator)
{
	assert(ppAccelerator);
	*ppAccelerator = pImpl_->pAccelerator_;
	return QSTATUS_SUCCESS;
}

LRESULT qm::MessageFrameWindow::windowProc(UINT uMsg, WPARAM wParam, LPARAM lParam)
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

LRESULT qm::MessageFrameWindow::onActivate(UINT nFlags, HWND hwnd, bool bMinimized)
{
	FrameWindow::onActivate(nFlags, hwnd, bMinimized);
	
	if (nFlags != WA_INACTIVE) {
		pImpl_->pMessageWindow_->setActive();
		
		HIMC hImc = ::ImmGetContext(getHandle());
		::ImmSetOpenStatus(hImc, FALSE);
		::ImmReleaseContext(getHandle(), hImc);
	}
	
	return 0;
}

LRESULT qm::MessageFrameWindow::onClose()
{
	pImpl_->pMessageFrameWindowManager_->close(this);
	return 0;
}

LRESULT qm::MessageFrameWindow::onCreate(CREATESTRUCT* pCreateStruct)
{
	if (FrameWindow::onCreate(pCreateStruct) == -1)
		return -1;
	
	DECLARE_QSTATUS();
	
	MessageFrameWindowCreateContext* pContext =
		static_cast<MessageFrameWindowCreateContext*>(pCreateStruct->lpCreateParams);
	pImpl_->pDocument_ = pContext->pDocument_;
	pImpl_->pEditFrameWindowManager_ = pContext->pEditFrameWindowManager_;
	pImpl_->pTempFileCleaner_ = pContext->pTempFileCleaner_;
	
	status = pContext->pKeyMap_->createAccelerator(
		CustomAcceleratorFactory(), L"MessageFrameWindow",
		mapKeyNameToId, countof(mapKeyNameToId), &pImpl_->pAccelerator_);
	CHECK_QSTATUS_VALUE(-1);
	
	DWORD dwStyle = WS_CHILD | WS_VISIBLE | WS_CLIPCHILDREN | WS_CLIPSIBLINGS;
	std::auto_ptr<MessageWindow> pMessageWindow;
	status = newQsObject(false, false, static_cast<ViewModelManager*>(0),
		pImpl_->pProfile_, &pMessageWindow);
	CHECK_QSTATUS_VALUE(-1);
	MessageWindowCreateContext context = {
		pContext->pDocument_,
		pContext->pMenuManager_,
		pContext->pKeyMap_
	};
	status = pMessageWindow->create(L"QmMessageWindow",
		0, dwStyle, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
		CW_USEDEFAULT, getHandle(), 0, 0,
		MessageFrameWindowImpl::ID_MESSAGEWINDOW, &context);
	CHECK_QSTATUS_VALUE(-1);
	pImpl_->pMessageWindow_ = pMessageWindow.release();
	
	std::auto_ptr<StatusBar> pStatusBar;
	status = newQsObject(&pStatusBar);
	CHECK_QSTATUS_VALUE(-1);
	status = pStatusBar->create(L"QmStatusBarWindow",
		0, dwStyle, CW_USEDEFAULT, CW_USEDEFAULT,
		CW_USEDEFAULT, CW_USEDEFAULT, getHandle(), 0,
		STATUSCLASSNAMEW, MessageFrameWindowImpl::ID_STATUSBAR, 0);
	CHECK_QSTATUS_VALUE(-1);
	pImpl_->pStatusBar_ = pStatusBar.release();
	
	status = pImpl_->layoutChildren();
	CHECK_QSTATUS_VALUE(-1);
	
	status = newQsObject(&pImpl_->pMoveMenu_);
	CHECK_QSTATUS();
	
	status = newQsObject(&pImpl_->pAttachmentMenu_);
	CHECK_QSTATUS();
	
	status = newQsObject(pImpl_->pDocument_->getTemplateManager(),
		&pImpl_->pViewTemplateMenu_);
	CHECK_QSTATUS();
	
	status = newQsObject(pImpl_->pDocument_->getTemplateManager(),
		false, &pImpl_->pCreateTemplateMenu_);
	CHECK_QSTATUS();
	
	status = newQsObject(pImpl_->pDocument_->getTemplateManager(),
		true, &pImpl_->pCreateTemplateExternalMenu_);
	CHECK_QSTATUS();
	
	status = newQsObject(pImpl_->pProfile_, &pImpl_->pEncodingMenu_);
	CHECK_QSTATUS();
	
	status = newQsObject(pImpl_->pDocument_->getScriptManager(),
		&pImpl_->pScriptMenu_);
	CHECK_QSTATUS_VALUE(-1);
	
	status = pImpl_->initActions();
	CHECK_QSTATUS();
	
	status = getMessageModel()->addMessageModelHandler(pImpl_);
	CHECK_QSTATUS();
	
	pImpl_->bCreated_ = true;
	
	if (pImpl_->bMaximize_)
		showWindow(SW_MAXIMIZE);
	
	return 0;
}

LRESULT qm::MessageFrameWindow::onDestroy()
{
	Profile* pProfile = pImpl_->pProfile_;
	
	getMessageModel()->removeMessageModelHandler(pImpl_);
	
	pProfile->setInt(L"MessageFrameWindow", L"ShowToolbar", pImpl_->bShowToolbar_);
	pProfile->setInt(L"MessageFrameWindow", L"ShowStatusBar", pImpl_->bShowStatusBar_);
	
#ifndef _WIN32_WCE
	WINDOWPLACEMENT wp;
	getWindowPlacement(&wp);
	pProfile->setBinary(L"MessageFrameWindow", L"WindowPlacement",
		reinterpret_cast<const unsigned char*>(&wp), sizeof(wp));
#endif
	
	return FrameWindow::onDestroy();
}

LRESULT qm::MessageFrameWindow::onInitMenuPopup(HMENU hmenu, UINT nIndex, bool bSysMenu)
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
		
		if (nIdLast == IDM_MESSAGE_MOVEOTHER) {
			MessageModel* pModel = getMessageModel();
			Account* pAccount = pModel->getCurrentAccount();
			assert(pAccount);
			status = pImpl_->pMoveMenu_->createMenu(
				hmenu, pAccount, *pImpl_->pActionMap_);
			// TODO
		}
		else if (nIdFirst == IDM_MESSAGE_DETACH) {
			MessagePtrList l;
			status = pImpl_->getSelectedMessages(0, &l);
			if (status == QSTATUS_SUCCESS) {
				status = pImpl_->pAttachmentMenu_->createMenu(hmenu, l);
				// TODO
			}
		}
		else if (nIdFirst == IDM_VIEW_TEMPLATENONE) {
			status = pImpl_->pViewTemplateMenu_->createMenu(hmenu);
		}
		else if (nIdFirst == IDM_MESSAGE_APPLYTEMPLATE ||
			nIdFirst == IDM_MESSAGE_APPLYTEMPLATENONE) {
			status = pImpl_->pCreateTemplateMenu_->createMenu(hmenu);
		}
		else if (nIdFirst == IDM_MESSAGE_APPLYTEMPLATEEXTERNAL ||
			nIdFirst == IDM_MESSAGE_APPLYTEMPLATENONEEXTERNAL) {
			status = pImpl_->pCreateTemplateExternalMenu_->createMenu(hmenu);
		}
		else if (nIdFirst == IDM_VIEW_ENCODINGAUTODETECT) {
			status = pImpl_->pEncodingMenu_->createMenu(hmenu);
		}
		else if (nIdFirst == IDM_TOOL_SCRIPTNONE ||
			nIdFirst == IDM_TOOL_SCRIPT) {
			status = pImpl_->pScriptMenu_->createMenu(hmenu);
		}
	}
	
	return FrameWindow::onInitMenuPopup(hmenu, nIndex, bSysMenu);
}

LRESULT qm::MessageFrameWindow::onSize(UINT nFlags, int cx, int cy)
{
	if (pImpl_->bCreated_ &&
		!pImpl_->bLayouting_ &&
		(nFlags == SIZE_RESTORED || nFlags == SIZE_MAXIMIZED))
		pImpl_->layoutChildren(cx, cy);
	return FrameWindow::onSize(nFlags, cx, cy);
}


/****************************************************************************
 *
 * MessageFrameWindowManager
 *
 */

qm::MessageFrameWindowManager::MessageFrameWindowManager(
	Document* pDocument, TempFileCleaner* pTempFileCleaner,
	MenuManager* pMenuManager, KeyMap* pKeyMap,
	Profile* pProfile, ViewModelManager* pViewModelManager,
	EditFrameWindowManager* pEditFrameWindowManager, QSTATUS* pstatus) :
	pDocument_(pDocument),
	pTempFileCleaner_(pTempFileCleaner),
	pMenuManager_(pMenuManager),
	pKeyMap_(pKeyMap),
	pProfile_(pProfile),
	pViewModelManager_(pViewModelManager),
	pEditFrameWindowManager_(pEditFrameWindowManager), 
	pCachedFrame_(0)
{
	assert(pstatus);
	assert(pDocument);
	assert(pKeyMap);
	assert(pProfile);
	assert(pViewModelManager);
	*pstatus = QSTATUS_SUCCESS;
}

qm::MessageFrameWindowManager::~MessageFrameWindowManager()
{
	if (pCachedFrame_)
		pCachedFrame_->destroyWindow();
	std::for_each(listFrame_.begin(), listFrame_.end(),
		std::mem_fun(&MessageFrameWindow::destroyWindow));
}

QSTATUS qm::MessageFrameWindowManager::open(
	ViewModel* pViewModel, MessageHolder* pmh)
{
	assert(pViewModel);
	assert(pmh);
	
	DECLARE_QSTATUS();
	
	MessageFrameWindow* pFrame = 0;
	
	status = STLWrapper<FrameList>(listFrame_).reserve(listFrame_.size() + 1);
	CHECK_QSTATUS();
	
	if (pCachedFrame_) {
		assert(listFrame_.empty());
		pFrame = pCachedFrame_;
		pCachedFrame_ = 0;
	}
#if defined _WIN32_WCE && _WIN32_WCE >= 300 && _WIN32_WCE_PSPC
	else if (!listFrame_.empty()) {
		pFrame = listFrame_.front();
	}
	else {
		status = create(&pFrame);
		CHECK_QSTATUS();
	}
	if (listFrame_.empty())
		listFrame_.push_back(pFrame);
	assert(listFrame_.size() == 1);
#else
	else {
		status = create(&pFrame);
		CHECK_QSTATUS();
	}
	listFrame_.push_back(pFrame);
#endif
	
	MessageModel* pMessageModel = pFrame->getMessageModel();
	pMessageModel->setViewModel(pViewModel);
	status = pMessageModel->setMessage(pmh);
	CHECK_QSTATUS();
	pFrame->showWindow(SW_SHOW);
	
	return QSTATUS_SUCCESS;
}

void qm::MessageFrameWindowManager::close(
	MessageFrameWindow* pMessageFrameWindow)
{
	assert(!pCachedFrame_);
	
	if (listFrame_.size() == 1) {
		pMessageFrameWindow->showWindow(SW_HIDE);
		pCachedFrame_ = pMessageFrameWindow;
	}
	else {
		pMessageFrameWindow->destroyWindow();
	}
	
	FrameList::iterator it = std::remove(listFrame_.begin(),
		listFrame_.end(), pMessageFrameWindow);
	listFrame_.erase(it, listFrame_.end());
}

QSTATUS qm::MessageFrameWindowManager::preModalDialog(HWND hwndParent)
{
	FrameList::iterator it = listFrame_.begin();
	while (it != listFrame_.end()) {
		if ((*it)->getHandle() != hwndParent)
			(*it)->enableWindow(false);
		++it;
	}
	
	return QSTATUS_SUCCESS;
}

QSTATUS qm::MessageFrameWindowManager::postModalDialog(HWND hwndParent)
{
	FrameList::iterator it = listFrame_.begin();
	while (it != listFrame_.end()) {
		if ((*it)->getHandle() != hwndParent)
			(*it)->enableWindow(true);
		++it;
	}
	
	return QSTATUS_SUCCESS;
}

QSTATUS qm::MessageFrameWindowManager::create(MessageFrameWindow** ppFrame)
{
	assert(ppFrame);
	
	DECLARE_QSTATUS();
	
	std::auto_ptr<MessageFrameWindow> pFrame;
	status = newQsObject(this, pProfile_, pViewModelManager_, &pFrame);
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
	MessageFrameWindowCreateContext context = {
		pDocument_,
		pEditFrameWindowManager_,
		pTempFileCleaner_,
		pMenuManager_,
		pKeyMap_
	};
	status = pFrame->create(L"QmMessageFrameWindow", L"QMAIL", dwStyle,
		CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
		0, dwExStyle, 0, 0, &context);
	CHECK_QSTATUS();
	
	*ppFrame = pFrame.release();
	
	return QSTATUS_SUCCESS;
}
