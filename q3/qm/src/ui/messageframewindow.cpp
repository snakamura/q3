/*
 * $Id$
 *
 * Copyright(C) 1998-2005 Satoshi Nakamura
 * All rights reserved.
 *
 */

#include <qmapplication.h>
#include <qmdocument.h>
#include <qmmessage.h>
#include <qmmessagewindow.h>
#include <qmsecurity.h>

#include <qsaccelerator.h>
#include <qsaction.h>
#include <qsuiutil.h>

#include <algorithm>

#ifdef _WIN32_WCE_PSPC
#	include <aygshell.h>
#endif

#include "actionid.h"
#include "keymap.h"
#include "menus.h"
#include "messageframewindow.h"
#include "messagewindow.h"
#include "resourceinc.h"
#include "statusbar.h"
#include "uimanager.h"
#include "uiutil.h"
#include "../action/action.h"
#include "../action/actionmacro.h"
#include "../action/findreplace.h"
#include "../uimodel/encodingmodel.h"
#include "../uimodel/messagemodel.h"
#include "../uimodel/messageselectionmodel.h"
#include "../uimodel/securitymodel.h"

#pragma warning(disable:4786)

using namespace qm;
using namespace qs;


/****************************************************************************
 *
 * MessageFrameWindowImpl
 *
 */

class qm::MessageFrameWindowImpl :
	public MessageWindowHandler,
	public FolderModelBase,
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
	void initActions();
	void layoutChildren();
	void layoutChildren(int cx,
						int cy);

public:
	virtual void messageChanged(const MessageWindowEvent& event);
	virtual void statusTextChanged(const MessageWindowStatusTextEvent& event);

public:
	virtual std::pair<Account*, Folder*> getCurrent() const;
	virtual std::pair<Account*, Folder*> getTemporary() const;

public:
	virtual void getSelectedMessages(AccountLock* pAccountLock,
									 Folder** ppFolder,
									 MessageHolderList* pList);
	virtual bool hasSelectedMessage();
	virtual MessagePtr getFocusedMessage();
	virtual bool hasFocusedMessage();
	virtual void selectAll();
	virtual bool canSelect();

public:
	MessageFrameWindow* pThis_;
	
	bool bShowToolbar_;
	bool bShowStatusBar_;
	
	MessageFrameWindowManager* pMessageFrameWindowManager_;
	Profile* pProfile_;
	Document* pDocument_;
	UIManager* pUIManager_;
	TempFileCleaner* pTempFileCleaner_;
	ViewModelManager* pViewModelManager_;
	std::auto_ptr<MessageMessageModel> pMessageModel_;
	MessageWindow* pMessageWindow_;
	MessageFrameWindowStatusBar* pStatusBar_;
	std::auto_ptr<Accelerator> pAccelerator_;
	std::auto_ptr<ActionMap> pActionMap_;
	std::auto_ptr<ActionInvoker> pActionInvoker_;
	std::auto_ptr<FindReplaceManager> pFindReplaceManager_;
	ExternalEditorManager* pExternalEditorManager_;
	EditFrameWindowManager* pEditFrameWindowManager_;
	std::auto_ptr<DefaultEncodingModel> pEncodingModel_;
	std::auto_ptr<DefaultSecurityModel> pSecurityModel_;
	MessageViewModeHolder* pMessageViewModeHolder_;
	std::auto_ptr<MoveMenu> pMoveMenu_;
	std::auto_ptr<AttachmentMenu> pAttachmentMenu_;
	std::auto_ptr<ViewTemplateMenu> pViewTemplateMenu_;
	std::auto_ptr<CreateTemplateMenu> pCreateTemplateMenu_;
	std::auto_ptr<CreateTemplateMenu> pCreateTemplateExternalMenu_;
	std::auto_ptr<EncodingMenu> pEncodingMenu_;
	std::auto_ptr<ScriptMenu> pScriptMenu_;
	ToolbarCookie* pToolbarCookie_;
	wstring_ptr wstrTitle_;
	bool bCreated_;
	int nInitialShow_;
	bool bLayouting_;
};

void qm::MessageFrameWindowImpl::initActions()
{
	pActionMap_.reset(new ActionMap());
	pActionInvoker_.reset(new ActionInvoker(pActionMap_.get()));
	pFindReplaceManager_.reset(new FindReplaceManager());
	
	ADD_ACTION6(AttachmentOpenAction,
		IDM_ATTACHMENT_OPEN,
		pMessageModel_.get(),
		pMessageWindow_->getAttachmentSelectionModel(),
		pSecurityModel_.get(),
		pProfile_,
		pTempFileCleaner_,
		pThis_->getHandle());
	ADD_ACTION6(AttachmentSaveAction,
		IDM_ATTACHMENT_SAVE,
		pMessageModel_.get(),
		pMessageWindow_->getAttachmentSelectionModel(),
		pSecurityModel_.get(),
		false,
		pProfile_,
		pThis_->getHandle());
	ADD_ACTION6(AttachmentSaveAction,
		IDM_ATTACHMENT_SAVEALL,
		pMessageModel_.get(),
		pMessageWindow_->getAttachmentSelectionModel(),
		pSecurityModel_.get(),
		true,
		pProfile_,
		pThis_->getHandle());
	ADD_ACTION3(EditCommandAction,
		IDM_EDIT_COPY,
		pMessageWindow_,
		&MessageWindowItem::copy,
		&MessageWindowItem::canCopy);
	ADD_ACTION8(EditDeleteMessageAction,
		IDM_EDIT_DELETE,
		this,
		pMessageModel_.get(),
		pMessageModel_.get(),
		EditDeleteMessageAction::TYPE_NORMAL,
		false,
		pDocument_->getUndoManager(),
		pThis_->getHandle(),
		pProfile_);
	ADD_ACTION8(EditDeleteMessageAction,
		IDM_EDIT_DELETEDIRECT,
		this,
		pMessageModel_.get(),
		pMessageModel_.get(),
		EditDeleteMessageAction::TYPE_DIRECT,
		false,
		pDocument_->getUndoManager(),
		pThis_->getHandle(),
		pProfile_);
	ADD_ACTION8(EditDeleteMessageAction,
		IDM_EDIT_DELETEJUNK,
		this,
		pMessageModel_.get(),
		pMessageModel_.get(),
		EditDeleteMessageAction::TYPE_JUNK,
		false,
		pDocument_->getUndoManager(),
		pThis_->getHandle(),
		pProfile_);
	ADD_ACTION3(EditFindAction,
		IDM_EDIT_FIND,
		pMessageWindow_,
		pProfile_,
		pFindReplaceManager_.get());
	ADD_ACTION3(EditFindAction,
		IDM_EDIT_FINDNEXT,
		pMessageWindow_,
		true,
		pFindReplaceManager_.get());
	ADD_ACTION3(EditFindAction,
		IDM_EDIT_FINDPREV,
		pMessageWindow_,
		false,
		pFindReplaceManager_.get());
	ADD_ACTION3(EditCommandAction,
		IDM_EDIT_SELECTALL,
		pMessageWindow_,
		&MessageWindowItem::selectAll,
		&MessageWindowItem::canSelectAll);
	ADD_ACTION3(EditUndoMessageAction,
		IDM_EDIT_UNDO,
		pDocument_->getUndoManager(),
		pDocument_,
		pThis_->getHandle());
	ADD_ACTION1(FileCloseAction,
		IDM_FILE_CLOSE,
		pThis_->getHandle());
	ADD_ACTION7(FilePrintAction,
		IDM_FILE_PRINT,
		pDocument_,
		this,
		pEncodingModel_.get(),
		pSecurityModel_.get(),
		pThis_->getHandle(),
		pProfile_,
		pTempFileCleaner_);
#if defined _WIN32_WCE && _WIN32_WCE >= 300 && defined _WIN32_WCE_PSPC
	ADD_ACTION1(FileCloseAction,
		IDOK,
		pThis_->getHandle());
#endif
	ADD_ACTION_RANGE11(MessageApplyTemplateAction,
		IDM_MESSAGE_APPLYTEMPLATE,
		IDM_MESSAGE_APPLYTEMPLATE + TemplateMenu::MAX_TEMPLATE,
		pCreateTemplateMenu_.get(),
		pDocument_,
		this,
		this,
		pEncodingModel_.get(),
		pSecurityModel_.get(),
		pEditFrameWindowManager_,
		pExternalEditorManager_,
		pThis_->getHandle(),
		pProfile_,
		false);
	ADD_ACTION_RANGE11(MessageApplyTemplateAction,
		IDM_MESSAGE_APPLYTEMPLATEEXTERNAL,
		IDM_MESSAGE_APPLYTEMPLATEEXTERNAL + TemplateMenu::MAX_TEMPLATE,
		pCreateTemplateExternalMenu_.get(),
		pDocument_,
		this,
		this,
		pEncodingModel_.get(),
		pSecurityModel_.get(),
		pEditFrameWindowManager_,
		pExternalEditorManager_,
		pThis_->getHandle(),
		pProfile_,
		true);
	
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
		ADD_ACTION11(MessageCreateAction,
			creates[n].nId_,
			pDocument_,
			this,
			this,
			pEncodingModel_.get(),
			pSecurityModel_.get(),
			creates[n].pwszName_,
			pEditFrameWindowManager_,
			pExternalEditorManager_,
			pThis_->getHandle(),
			pProfile_,
			false);
		ADD_ACTION11(MessageCreateAction,
			creates[n].nIdExternal_,
			pDocument_,
			this,
			this,
			pEncodingModel_.get(),
			pSecurityModel_.get(),
			creates[n].pwszName_,
			pEditFrameWindowManager_,
			pExternalEditorManager_,
			pThis_->getHandle(),
			pProfile_,
			true);
	}
	ADD_ACTION4(MessageDeleteAttachmentAction,
		IDM_MESSAGE_DELETEATTACHMENT,
		this,
		pSecurityModel_.get(),
		pDocument_->getUndoManager(),
		pThis_->getHandle());
	ADD_ACTION4(MessageDetachAction,
		IDM_MESSAGE_DETACH,
		pProfile_,
		this,
		pSecurityModel_.get(),
		pThis_->getHandle());
	ADD_ACTION_RANGE5(MessageOpenAttachmentAction,
		IDM_MESSAGE_ATTACHMENT,
		IDM_MESSAGE_ATTACHMENT + AttachmentMenu::MAX_ATTACHMENT,
		pSecurityModel_.get(),
		pProfile_,
		pAttachmentMenu_.get(),
		pTempFileCleaner_,
		pThis_->getHandle());
	
	struct {
		UINT nId_;
		unsigned int nFlags_;
		unsigned int nMask_;
	} marks[] = {
		{ IDM_MESSAGE_MARK,					MessageHolder::FLAG_MARKED,			MessageHolder::FLAG_MARKED			},
		{ IDM_MESSAGE_UNMARK,				0,									MessageHolder::FLAG_MARKED			},
		{ IDM_MESSAGE_MARKDELETED,			MessageHolder::FLAG_DELETED,		MessageHolder::FLAG_DELETED			},
		{ IDM_MESSAGE_UNMARKDELETED,		0,									MessageHolder::FLAG_DELETED			},
		{ IDM_MESSAGE_MARKDOWNLOAD,			MessageHolder::FLAG_DOWNLOAD,		MessageHolder::FLAG_DOWNLOAD		},
		{ IDM_MESSAGE_UNMARKDOWNLOAD,		0,									MessageHolder::FLAG_DOWNLOAD		},
		{ IDM_MESSAGE_MARKDOWNLOADTEXT,		MessageHolder::FLAG_DOWNLOADTEXT,	MessageHolder::FLAG_DOWNLOADTEXT	},
		{ IDM_MESSAGE_UNMARKDOWNLOADTEXT,	0,									MessageHolder::FLAG_DOWNLOADTEXT	},
		{ IDM_MESSAGE_MARKSEEN,				MessageHolder::FLAG_SEEN,			MessageHolder::FLAG_SEEN			},
		{ IDM_MESSAGE_UNMARKSEEN,			0,									MessageHolder::FLAG_SEEN			},
		{ IDM_MESSAGE_MARKUSER1,			MessageHolder::FLAG_USER1,			MessageHolder::FLAG_USER1			},
		{ IDM_MESSAGE_UNMARKUSER1,			0,									MessageHolder::FLAG_USER1			},
		{ IDM_MESSAGE_MARKUSER2,			MessageHolder::FLAG_USER2,			MessageHolder::FLAG_USER2			},
		{ IDM_MESSAGE_UNMARKUSER2,			0,									MessageHolder::FLAG_USER2			},
		{ IDM_MESSAGE_MARKUSER3,			MessageHolder::FLAG_USER3,			MessageHolder::FLAG_USER3			},
		{ IDM_MESSAGE_UNMARKUSER3,			0,									MessageHolder::FLAG_USER3			},
		{ IDM_MESSAGE_MARKUSER4,			MessageHolder::FLAG_USER4,			MessageHolder::FLAG_USER4			},
		{ IDM_MESSAGE_UNMARKUSER4,			0,									MessageHolder::FLAG_USER4			},
	};
	for (int n = 0; n < countof(marks); ++n) {
		ADD_ACTION5(MessageMarkAction,
			marks[n].nId_,
			this,
			marks[n].nFlags_,
			marks[n].nMask_,
			pDocument_->getUndoManager(),
			pThis_->getHandle());
	}
	
	ADD_ACTION_RANGE7(MessageMoveAction,
		IDM_MESSAGE_MOVE,
		IDM_MESSAGE_MOVE + MoveMenu::MAX_FOLDER,
		this,
		pMessageModel_.get(),
		pMessageModel_.get(),
		pMoveMenu_.get(),
		false,
		pDocument_->getUndoManager(),
		pThis_->getHandle());
	ADD_ACTION8(MessageMoveOtherAction,
		IDM_MESSAGE_MOVEOTHER,
		pDocument_,
		this,
		pMessageModel_.get(),
		pMessageModel_.get(),
		false,
		pDocument_->getUndoManager(),
		pProfile_,
		pThis_->getHandle());
	ADD_ACTION2(MessageOpenLinkAction,
		IDM_MESSAGE_OPENLINK,
		this,
		pThis_->getHandle());
	ADD_ACTION3(MessagePropertyAction,
		IDM_MESSAGE_PROPERTY,
		this,
		pDocument_->getUndoManager(),
		pThis_->getHandle());
	ADD_ACTION_RANGE4(ToolScriptAction,
		IDM_TOOL_SCRIPT,
		IDM_TOOL_SCRIPT + ScriptMenu::MAX_SCRIPT,
		pScriptMenu_.get(),
		pDocument_,
		pProfile_,
		pThis_);
	ADD_ACTION1(ViewEncodingAction,
		IDM_VIEW_ENCODINGAUTODETECT,
		pEncodingModel_.get());
	ADD_ACTION_RANGE2(ViewEncodingAction,
		IDM_VIEW_ENCODING,
		IDM_VIEW_ENCODING + EncodingMenu::MAX_ENCODING,
		pEncodingModel_.get(),
		pEncodingMenu_.get());
	ADD_ACTION3(ViewSecurityAction,
		IDM_VIEW_SMIMEMODE,
		pSecurityModel_.get(),
		SECURITYMODE_SMIME,
		Security::isSMIMEEnabled());
	ADD_ACTION3(ViewSecurityAction,
		IDM_VIEW_PGPMODE,
		pSecurityModel_.get(),
		SECURITYMODE_PGP,
		Security::isPGPEnabled());
	ADD_ACTION3(ViewMessageModeAction,
		IDM_VIEW_HTMLINTERNETZONEMODE,
		pMessageViewModeHolder_,
		MessageViewMode::MODE_INTERNETZONE,
		true);
	ADD_ACTION3(ViewMessageModeAction,
		IDM_VIEW_HTMLMODE,
		pMessageViewModeHolder_,
		MessageViewMode::MODE_HTML,
		true);
	ADD_ACTION3(ViewMessageModeAction,
		IDM_VIEW_HTMLONLINEMODE,
		pMessageViewModeHolder_,
		MessageViewMode::MODE_HTMLONLINE,
		true);
	
	struct {
		UINT nId_;
		ViewNavigateMessageAction::Type type_;
	} navigates[] = {
		{ IDM_VIEW_NEXTMESSAGE,			ViewNavigateMessageAction::TYPE_NEXT		},
		{ IDM_VIEW_PREVMESSAGE,			ViewNavigateMessageAction::TYPE_PREV		},
		{ IDM_VIEW_NEXTUNSEENMESSAGE,	ViewNavigateMessageAction::TYPE_NEXTUNSEEN	},
		{ IDM_VIEW_NEXTMESSAGEPAGE,		ViewNavigateMessageAction::TYPE_NEXTPAGE	},
		{ IDM_VIEW_PREVMESSAGEPAGE,		ViewNavigateMessageAction::TYPE_PREVPAGE	}
	};
	for (int n = 0; n < countof(navigates); ++n) {
		ADD_ACTION4(ViewNavigateMessageAction,
			navigates[n].nId_,
			pViewModelManager_,
			pMessageModel_.get(),
			pMessageWindow_,
			navigates[n].type_);
	}
	
	ADD_ACTION3(ViewMessageModeAction,
		IDM_VIEW_QUOTEMODE,
		pMessageViewModeHolder_,
		MessageViewMode::MODE_QUOTE,
		true);
	ADD_ACTION3(ViewMessageModeAction,
		IDM_VIEW_RAWMODE,
		pMessageViewModeHolder_,
		MessageViewMode::MODE_RAW,
		true);
	ADD_ACTION1(ViewOpenLinkAction,
		IDM_VIEW_OPENLINK,
		pMessageWindow_);
	ADD_ACTION3(ViewMessageModeAction,
		IDM_VIEW_SELECTMODE,
		pMessageViewModeHolder_,
		MessageViewMode::MODE_SELECT,
		true);
	ADD_ACTION1(ViewShowHeaderAction,
		IDM_VIEW_SHOWHEADER,
		pMessageWindow_);
	ADD_ACTION1(ViewShowStatusBarAction<MessageFrameWindow>,
		IDM_VIEW_SHOWSTATUSBAR,
		pThis_);
	ADD_ACTION1(ViewShowToolbarAction<MessageFrameWindow>,
		IDM_VIEW_SHOWTOOLBAR,
		pThis_);
	ADD_ACTION_RANGE2(ViewTemplateAction,
		IDM_VIEW_TEMPLATE,
		IDM_VIEW_TEMPLATE + TemplateMenu::MAX_TEMPLATE,
		pMessageWindow_,
		pViewTemplateMenu_.get());
	ADD_ACTION1(ViewTemplateAction,
		IDM_VIEW_TEMPLATENONE,
		pMessageWindow_);
}

void qm::MessageFrameWindowImpl::layoutChildren()
{
	RECT rect;
	pThis_->getClientRect(&rect);
	layoutChildren(rect.right - rect.left, rect.bottom - rect.top);
}

void qm::MessageFrameWindowImpl::layoutChildren(int cx,
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
#if _WIN32_WCE >= 300 && defined _WIN32_WCE_PSPC
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
	
#if _WIN32_WCE >= 300 && defined _WIN32_WCE_PSPC
	hdwp = wndToolbar.deferWindowPos(hdwp, 0, 0,
		cy - nToolbarHeight, cx, nToolbarHeight, SWP_NOZORDER);
#else
	hdwp = wndToolbar.deferWindowPos(hdwp, 0, 0, 0,
		cx, nToolbarHeight, SWP_NOMOVE | SWP_NOZORDER);
#endif
	wndToolbar.showWindow(bShowToolbar_ ? SW_SHOW : SW_HIDE);
	
	hdwp = pStatusBar_->deferWindowPos(hdwp, 0, 0,
		cy - nStatusBarHeight - nBottomBarHeight, cx,
		rectStatusBar.bottom - rectStatusBar.top, SWP_NOZORDER);
	pStatusBar_->showWindow(bShowStatusBar_ ? SW_SHOW : SW_HIDE);
	
	hdwp = pMessageWindow_->deferWindowPos(hdwp, 0, 0, nTopBarHeight, cx,
		cy - nStatusBarHeight - nTopBarHeight - nBottomBarHeight, SWP_NOZORDER);
	
	Window::endDeferWindowPos(hdwp);
	
#ifdef _WIN32_WCE_PSPC
	int nWidth[] = {
		cx - 40,
		cx - 20,
		-1
	};
#elif defined _WIN32_WCE
	int nWidth[] = {
		cx - 200,
		cx - 120,
		cx - 40,
		cx - 20,
		-1
	};
#else
	int nWidth[] = {
		cx - 238,
		cx - 158,
		cx - 78,
		cx - 54,
		cx - 30,
		-1
	};
#endif
	pStatusBar_->setParts(nWidth, countof(nWidth));
	
	bLayouting_ = false;
}

void qm::MessageFrameWindowImpl::messageChanged(const MessageWindowEvent& event)
{
	MessageHolder* pmh = event.getMessageHolder();
	if (pmh) {
		wstring_ptr wstrSubject(pmh->getSubject());
		wstring_ptr wstrTitle(concat(wstrSubject.get(), L" - ", wstrTitle_.get()));
		pThis_->setWindowText(wstrTitle.get());
		
		if (bShowStatusBar_) {
			pStatusBar_->setText(0, L"");
			pStatusBar_->updateMessageParts(pmh,
				event.getMessage(), event.getContentType());
		}
	}
	else {
		pThis_->postMessage(WM_CLOSE);
	}
}

void qm::MessageFrameWindowImpl::statusTextChanged(const MessageWindowStatusTextEvent& event)
{
	if (bShowStatusBar_)
		pStatusBar_->setText(0, event.getText());
}

std::pair<Account*, Folder*> qm::MessageFrameWindowImpl::getCurrent() const
{
	ViewModel* pViewModel = pMessageModel_->getViewModel();
	if (pViewModel)
		return std::make_pair(pMessageModel_->getCurrentAccount(), pViewModel->getFolder());
	else
		return std::pair<Account*, Folder*>(0, 0);
}

std::pair<Account*, Folder*> qm::MessageFrameWindowImpl::getTemporary() const
{
	return std::pair<Account*, Folder*>(0, 0);
}

void qm::MessageFrameWindowImpl::getSelectedMessages(AccountLock* pAccountLock,
													 Folder** ppFolder,
													 MessageHolderList* pList)
{
	assert(pAccountLock);
	
	if (ppFolder)
		*ppFolder = 0;
	
	MessagePtrLock mpl(pMessageModel_->getCurrentMessage());
	if (mpl) {
		pAccountLock->set(mpl->getAccount());
		if (ppFolder)
			*ppFolder = mpl->getFolder();
		if (pList)
			pList->push_back(mpl);
	}
}

bool qm::MessageFrameWindowImpl::hasSelectedMessage()
{
	return true;
}

MessagePtr qm::MessageFrameWindowImpl::getFocusedMessage()
{
	return pMessageModel_->getCurrentMessage();
}

bool qm::MessageFrameWindowImpl::hasFocusedMessage()
{
	return true;
}

void qm::MessageFrameWindowImpl::selectAll()
{
}

bool qm::MessageFrameWindowImpl::canSelect()
{
	return false;
}


/****************************************************************************
 *
 * MessageFrameWindow
 *
 */

qm::MessageFrameWindow::MessageFrameWindow(MessageFrameWindowManager* pMessageFrameWindowManager,
										   ViewModelManager* pViewModelManager,
										   Profile* pProfile) :
	FrameWindow(Application::getApplication().getResourceHandle(), true),
	pImpl_(0)
{
	assert(pProfile);
	
	pImpl_ = new MessageFrameWindowImpl();
	pImpl_->pThis_ = this;
	pImpl_->bShowToolbar_ = pProfile->getInt(L"MessageFrameWindow", L"ShowToolbar", 1) != 0;
	pImpl_->bShowStatusBar_ = pProfile->getInt(L"MessageFrameWindow", L"ShowStatusBar", 1) != 0;
	pImpl_->pMessageFrameWindowManager_ = pMessageFrameWindowManager;
	pImpl_->pProfile_ = pProfile;
	pImpl_->pViewModelManager_ = pViewModelManager;
	pImpl_->pMessageWindow_ = 0;
	pImpl_->pStatusBar_ = 0;
	pImpl_->pExternalEditorManager_ = 0;
	pImpl_->pToolbarCookie_ = 0;
	pImpl_->bCreated_ = false;
	pImpl_->nInitialShow_ = SW_SHOWNORMAL;
	pImpl_->bLayouting_ = false;
}

qm::MessageFrameWindow::~MessageFrameWindow()
{
	delete pImpl_;
}

MessageMessageModel* qm::MessageFrameWindow::getMessageModel() const
{
	return pImpl_->pMessageModel_.get();
}

const ActionInvoker* qm::MessageFrameWindow::getActionInvoker() const
{
	return pImpl_->pActionInvoker_.get();
}

void qm::MessageFrameWindow::initialShow()
{
	showWindow(pImpl_->nInitialShow_);
}

void qm::MessageFrameWindow::layout()
{
	pImpl_->layoutChildren();
	pImpl_->pMessageWindow_->layout();
}

void qm::MessageFrameWindow::reloadProfiles()
{
	pImpl_->pMessageWindow_->reloadProfiles();
}

bool qm::MessageFrameWindow::save()
{
	if (!pImpl_->pMessageWindow_->save())
		return false;
	
	Profile* pProfile = pImpl_->pProfile_;
	pProfile->setInt(L"MessageFrameWindow", L"ShowToolbar", pImpl_->bShowToolbar_);
	pProfile->setInt(L"MessageFrameWindow", L"ShowStatusBar", pImpl_->bShowStatusBar_);
	
	UIUtil::saveWindowPlacement(getHandle(), pProfile, L"MessageFrameWindow");
	
	pProfile->setInt(L"MainWindow", L"SecurityMode", pImpl_->pSecurityModel_->getSecurityMode());
	
	if (!FrameWindow::save())
		return false;
	
	return true;
}

bool qm::MessageFrameWindow::isShowToolbar() const
{
	return pImpl_->bShowToolbar_;
}

void qm::MessageFrameWindow::setShowToolbar(bool bShow)
{
	if (bShow != pImpl_->bShowToolbar_) {
		pImpl_->bShowToolbar_ = bShow;
		pImpl_->layoutChildren();
	}
}

bool qm::MessageFrameWindow::isShowStatusBar() const
{
	return pImpl_->bShowStatusBar_;
}

void qm::MessageFrameWindow::setShowStatusBar(bool bShow)
{
	if (bShow != pImpl_->bShowStatusBar_) {
		pImpl_->bShowStatusBar_ = bShow;
		pImpl_->layoutChildren();
	}
}

bool qm::MessageFrameWindow::getToolbarButtons(Toolbar* pToolbar)
{
	pToolbar->nId_ = MessageFrameWindowImpl::ID_TOOLBAR;
	return true;
}

bool qm::MessageFrameWindow::createToolbarButtons(void* pCreateParam,
												  HWND hwndToolbar)
{
	MessageFrameWindowCreateContext* pContext =
		static_cast<MessageFrameWindowCreateContext*>(pCreateParam);
	UIManager* pUIManager = pContext->pUIManager_;
	pImpl_->pToolbarCookie_ = pUIManager->getToolbarManager()->createButtons(
		L"messageframe", hwndToolbar, this);
	return pImpl_->pToolbarCookie_ != 0;
}

#ifdef _WIN32_WCE
UINT qm::MessageFrameWindow::getBarId(int n) const
{
	assert(n == 0 || n == 1);
	assert(pnId);
	static UINT nIds[] = {
		MessageFrameWindowImpl::ID_COMMANDBARMENU,
		MessageFrameWindowImpl::ID_COMMANDBARBUTTON
	};
	return nIds[n];
}

bool qm::MessageFrameWindow::getCommandBandsRestoreInfo(int n,
														COMMANDBANDSRESTOREINFO* pcbri) const
{
	WCHAR wszKey[32];
	swprintf(wszKey, L"CommandBandsRestoreInfo%d", n);
	size_t nSize = pImpl_->pProfile_->getBinary(L"MessageFrameWindow", wszKey,
		reinterpret_cast<unsigned char*>(pcbri), sizeof(*pcbri));
	if (nSize != sizeof(*pcbri))
		pcbri->cbSize = 0;
	return true;
}

bool qm::MessageFrameWindow::setCommandBandsRestoreInfo(int n,
														const COMMANDBANDSRESTOREINFO& cbri)
{
	WCHAR wszKey[32];
	swprintf(wszKey, L"CommandBandsRestoreInfo%d", n);
	pImpl_->pProfile_->setBinary(L"MessageFrameWindow", wszKey,
		reinterpret_cast<const unsigned char*>(&cbri), sizeof(cbri));
	return true;
}
#endif

HMENU qm::MessageFrameWindow::getMenuHandle(void* pCreateParam)
{
	MessageFrameWindowCreateContext* pContext =
		static_cast<MessageFrameWindowCreateContext*>(pCreateParam);
	UIManager* pUIManager = pContext->pUIManager_;
	return pUIManager->getMenuManager()->getMenu(L"messageframe", true, true);
}

UINT qm::MessageFrameWindow::getIconId()
{
	return IDI_MAINFRAME;
}

void qm::MessageFrameWindow::getWindowClass(WNDCLASS* pwc)
{
	FrameWindow::getWindowClass(pwc);
	pwc->hIcon = ::LoadIcon(Application::getApplication().getResourceHandle(),
		MAKEINTRESOURCE(IDI_MAINFRAME));
	pwc->hbrBackground = reinterpret_cast<HBRUSH>(COLOR_BTNFACE + 1);
}

bool qm::MessageFrameWindow::preCreateWindow(CREATESTRUCT* pCreateStruct)
{
	if (!FrameWindow::preCreateWindow(pCreateStruct))
		return false;
	
#if defined _WIN32_WCE && _WIN32_WCE >= 300 && defined _WIN32_WCE_PSPC
	RECT rect;
	qs::UIUtil::getWorkArea(&rect);
	pCreateStruct->x = rect.left;
	pCreateStruct->y = rect.top;
	pCreateStruct->cx = rect.right - rect.left;
	pCreateStruct->cy = rect.bottom - rect.top;
#elif !defined _WIN32_WCE
	pImpl_->nInitialShow_ = UIUtil::loadWindowPlacement(
		pImpl_->pProfile_, L"MessageFrameWindow", pCreateStruct);
#endif
	
	return true;
}

Action* qm::MessageFrameWindow::getAction(UINT nId)
{
	return pImpl_->pActionMap_->getAction(nId);
}

Accelerator* qm::MessageFrameWindow::getAccelerator()
{
	return pImpl_->pAccelerator_.get();
}

LRESULT qm::MessageFrameWindow::windowProc(UINT uMsg,
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

LRESULT qm::MessageFrameWindow::onActivate(UINT nFlags,
										   HWND hwnd,
										   bool bMinimized)
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
	
	MessageFrameWindowCreateContext* pContext =
		static_cast<MessageFrameWindowCreateContext*>(pCreateStruct->lpCreateParams);
	pImpl_->pDocument_ = pContext->pDocument_;
	pImpl_->pUIManager_ = pContext->pUIManager_;
	pImpl_->pEditFrameWindowManager_ = pContext->pEditFrameWindowManager_;
	pImpl_->pExternalEditorManager_ = pContext->pExternalEditorManager_;
	pImpl_->pTempFileCleaner_ = pContext->pTempFileCleaner_;
	
	pImpl_->pMessageModel_.reset(new MessageMessageModel());
	pImpl_->pEncodingModel_.reset(new DefaultEncodingModel());
	pImpl_->pSecurityModel_.reset(new DefaultSecurityModel(
		pImpl_->pProfile_->getInt(L"MessageWindow", L"SecurityMode", 0)));
	
	pImpl_->pMoveMenu_.reset(new MoveMenu());
	pImpl_->pAttachmentMenu_.reset(new AttachmentMenu(pImpl_->pSecurityModel_.get()));
	pImpl_->pViewTemplateMenu_.reset(new ViewTemplateMenu(
		pImpl_->pDocument_->getTemplateManager()));
	pImpl_->pCreateTemplateMenu_.reset(new CreateTemplateMenu(
		pImpl_->pDocument_->getTemplateManager(), false));
	pImpl_->pCreateTemplateExternalMenu_.reset(new CreateTemplateMenu(
		pImpl_->pDocument_->getTemplateManager(), true));
	pImpl_->pEncodingMenu_.reset(new EncodingMenu(pImpl_->pProfile_, IDM_VIEW_ENCODING));
	pImpl_->pScriptMenu_.reset(new ScriptMenu(pImpl_->pDocument_->getScriptManager()));
	
	CustomAcceleratorFactory acceleratorFactory;
	pImpl_->pAccelerator_ = pContext->pUIManager_->getKeyMap()->createAccelerator(
		&acceleratorFactory, L"MessageFrameWindow", mapKeyNameToId, countof(mapKeyNameToId));
	if (!pImpl_->pAccelerator_.get())
		return -1;
	
	DWORD dwStyle = WS_CHILD | WS_VISIBLE | WS_CLIPCHILDREN | WS_CLIPSIBLINGS;
	std::auto_ptr<MessageWindow> pMessageWindow(new MessageWindow(
		pImpl_->pMessageModel_.get(), pImpl_->pProfile_, L"MessageWindow"));
	pImpl_->pMessageViewModeHolder_ = pImpl_->pProfile_->getInt(L"Global", L"SaveMessageViewModePerFolder", 1) != 0 ?
		pImpl_->pMessageModel_.get() : pMessageWindow->getMessageViewModeHolder();
	MessageWindowCreateContext context = {
		pContext->pDocument_,
		pContext->pUIManager_,
		pImpl_->pMessageViewModeHolder_,
		pImpl_->pEncodingModel_.get(),
		pImpl_->pSecurityModel_.get()
	};
	if (!pMessageWindow->create(L"QmMessageWindow", 0, dwStyle, CW_USEDEFAULT,
		CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, getHandle(), 0, 0,
		MessageFrameWindowImpl::ID_MESSAGEWINDOW, &context))
		return -1;
	pImpl_->pMessageWindow_ = pMessageWindow.release();
	
	DWORD dwStatusBarStyle = dwStyle;
#if _WIN32_WCE >= 300 && defined _WIN32_WCE_PSPC
	dwStatusBarStyle |= CCS_NOPARENTALIGN;
#endif
	std::auto_ptr<MessageFrameWindowStatusBar> pStatusBar(new MessageFrameWindowStatusBar(
		pImpl_->pMessageModel_.get(), pImpl_->pMessageWindow_,
		pImpl_->pEncodingModel_.get(), 0, pImpl_->pEncodingMenu_.get(),
		pImpl_->pViewTemplateMenu_.get()));
	if (!pStatusBar->create(L"QmStatusBarWindow", 0, dwStatusBarStyle,
		CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, getHandle(),
		0, STATUSCLASSNAMEW, MessageFrameWindowImpl::ID_STATUSBAR, 0))
		return -1;
	pImpl_->pStatusBar_ = pStatusBar.release();
	
	pImpl_->layoutChildren();
	
	pImpl_->initActions();
	
	pImpl_->pMessageWindow_->addMessageWindowHandler(pImpl_);
	
	pImpl_->wstrTitle_ = getWindowText();
	
	pImpl_->bCreated_ = true;
	
	return 0;
}

LRESULT qm::MessageFrameWindow::onDestroy()
{
	pImpl_->pMessageWindow_->removeMessageWindowHandler(pImpl_);
	
	if (pImpl_->pToolbarCookie_)
		pImpl_->pUIManager_->getToolbarManager()->destroy(pImpl_->pToolbarCookie_);
	
	return FrameWindow::onDestroy();
}

LRESULT qm::MessageFrameWindow::onInitMenuPopup(HMENU hmenu,
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
		
		MessageModel* pModel = getMessageModel();
		Account* pAccount = pModel->getCurrentAccount();
		assert(pAccount);
		if (nIdLast == IDM_MESSAGE_MOVEOTHER) {
			pImpl_->pMoveMenu_->createMenu(hmenu, pAccount,
				::GetKeyState(VK_SHIFT) < 0, *pImpl_->pActionMap_);
		}
		else if (nIdFirst == IDM_MESSAGE_DETACH) {
			pImpl_->pAttachmentMenu_->createMenu(hmenu, pImpl_->getFocusedMessage());
		}
		else if (nIdFirst == IDM_VIEW_TEMPLATENONE) {
			pImpl_->pViewTemplateMenu_->createMenu(hmenu, pAccount);
		}
		else if (nIdFirst == IDM_MESSAGE_APPLYTEMPLATE ||
			nIdFirst == IDM_MESSAGE_APPLYTEMPLATENONE) {
			pImpl_->pCreateTemplateMenu_->createMenu(hmenu, pAccount);
		}
		else if (nIdFirst == IDM_MESSAGE_APPLYTEMPLATEEXTERNAL ||
			nIdFirst == IDM_MESSAGE_APPLYTEMPLATENONEEXTERNAL) {
			pImpl_->pCreateTemplateExternalMenu_->createMenu(hmenu, pAccount);
		}
		else if (nIdFirst == IDM_VIEW_ENCODINGAUTODETECT) {
			pImpl_->pEncodingMenu_->createMenu(hmenu);
		}
		else if (nIdFirst == IDM_TOOL_SCRIPTNONE ||
			nIdFirst == IDM_TOOL_SCRIPT) {
			pImpl_->pScriptMenu_->createMenu(hmenu);
		}
	}
	
	return FrameWindow::onInitMenuPopup(hmenu, nIndex, bSysMenu);
}

LRESULT qm::MessageFrameWindow::onSize(UINT nFlags,
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
 * MessageFrameWindowManager
 *
 */

qm::MessageFrameWindowManager::MessageFrameWindowManager(Document* pDocument,
														 UIManager* pUIManager,
														 TempFileCleaner* pTempFileCleaner,
														 Profile* pProfile,
														 ViewModelManager* pViewModelManager,
														 EditFrameWindowManager* pEditFrameWindowManager,
														 ExternalEditorManager* pExternalEditorManager) :
	pDocument_(pDocument),
	pUIManager_(pUIManager),
	pTempFileCleaner_(pTempFileCleaner),
	pProfile_(pProfile),
	pViewModelManager_(pViewModelManager),
	pEditFrameWindowManager_(pEditFrameWindowManager), 
	pExternalEditorManager_(pExternalEditorManager),
	pCachedFrame_(0)
{
	assert(pDocument);
	assert(pUIManager);
	assert(pProfile);
	assert(pViewModelManager);
	assert(pEditFrameWindowManager);
	assert(pExternalEditorManager);
}

qm::MessageFrameWindowManager::~MessageFrameWindowManager()
{
	if (pCachedFrame_)
		pCachedFrame_->destroyWindow();
	std::for_each(listFrame_.begin(), listFrame_.end(),
		std::mem_fun(&MessageFrameWindow::destroyWindow));
}

bool qm::MessageFrameWindowManager::open(ViewModel* pViewModel,
										 MessageHolder* pmh)
{
	assert(pViewModel);
	assert(pmh);
	
	MessageFrameWindow* pFrame = 0;
	if (pCachedFrame_) {
		assert(listFrame_.empty());
		pFrame = pCachedFrame_;
		pCachedFrame_ = 0;
	}
#if defined _WIN32_WCE && _WIN32_WCE >= 300 && defined _WIN32_WCE_PSPC
	else if (!listFrame_.empty()) {
		pFrame = listFrame_.front();
	}
	else {
		pFrame = create();
		if (!pFrame)
			return false;
	}
	if (listFrame_.empty())
		listFrame_.push_back(pFrame);
	assert(listFrame_.size() == 1);
#else
	else {
		pFrame = create();
		if (!pFrame)
			return false;
	}
	listFrame_.push_back(pFrame);
#endif
	
	MessageMessageModel* pMessageModel = pFrame->getMessageModel();
	pMessageModel->setViewModel(pViewModel);
	pMessageModel->setMessage(pmh);
	pFrame->showWindow(SW_SHOW);
	
	return true;
}

void qm::MessageFrameWindowManager::close(MessageFrameWindow* pMessageFrameWindow)
{
	assert(pMessageFrameWindow);
	assert(!pCachedFrame_ || pMessageFrameWindow == pCachedFrame_);
	
	if (pMessageFrameWindow == pCachedFrame_)
		return;
	
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

void qm::MessageFrameWindowManager::preModalDialog(HWND hwndParent)
{
	for (FrameList::iterator it = listFrame_.begin(); it != listFrame_.end(); ++it) {
		if ((*it)->getHandle() != hwndParent)
			(*it)->enableWindow(false);
	}
}

void qm::MessageFrameWindowManager::postModalDialog(HWND hwndParent)
{
	for (FrameList::iterator it = listFrame_.begin(); it != listFrame_.end(); ++it) {
		if ((*it)->getHandle() != hwndParent)
			(*it)->enableWindow(true);
	}
}

void qm::MessageFrameWindowManager::layout()
{
	for (FrameList::const_iterator it = listFrame_.begin(); it != listFrame_.end(); ++it)
		(*it)->layout();
}

void qm::MessageFrameWindowManager::reloadProfiles()
{
	if (pCachedFrame_) {
		pCachedFrame_->destroyWindow();
		pCachedFrame_ = 0;
	}
	
	for (FrameList::const_iterator it = listFrame_.begin(); it != listFrame_.end(); ++it)
		(*it)->reloadProfiles();
}

bool qm::MessageFrameWindowManager::save() const
{
	MessageFrameWindow* pFrame = 0;
	
	if (pCachedFrame_)
		pFrame = pCachedFrame_;
	else if (!listFrame_.empty())
		pFrame = listFrame_.back();
	
	if (pFrame) {
		if (!pFrame->save())
			return false;
	}
	
	return true;
}

MessageFrameWindow* qm::MessageFrameWindowManager::create()
{
	std::auto_ptr<MessageFrameWindow> pFrame(
		new MessageFrameWindow(this, pViewModelManager_, pProfile_));
#if defined _WIN32_WCE && _WIN32_WCE >= 300 && defined _WIN32_WCE_PSPC
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
		pUIManager_,
		pEditFrameWindowManager_,
		pExternalEditorManager_,
		pTempFileCleaner_
	};
	if (!pFrame->create(L"QmMessageFrameWindow", L"QMAIL", dwStyle,
		CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
		0, dwExStyle, 0, 0, &context))
		return 0;
	MessageFrameWindow* p = pFrame.release();
	p->initialShow();
	return p;
}


/****************************************************************************
 *
 * MessageFrameWindowStatusBar
 *
 */

qm::MessageFrameWindowStatusBar::MessageFrameWindowStatusBar(MessageModel* pMessageModel,
															 MessageWindow* pMessageWindow,
															 EncodingModel* pEncodingModel,
															 int nOffset,
															 EncodingMenu* pEncodingMenu,
															 ViewTemplateMenu* pViewTemplateMenu) :
	MessageStatusBar(pMessageWindow, pEncodingModel, nOffset, pEncodingMenu, pViewTemplateMenu),
	pMessageModel_(pMessageModel)
{
}

qm::MessageFrameWindowStatusBar::~MessageFrameWindowStatusBar()
{
}

Account* qm::MessageFrameWindowStatusBar::getAccount()
{
	return pMessageModel_->getCurrentAccount();
}
