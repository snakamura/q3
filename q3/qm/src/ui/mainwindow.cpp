/*
 * $Id$
 *
 * Copyright(C) 1998-2003 Satoshi Nakamura
 * All rights reserved.
 *
 */

#include <qmapplication.h>
#include <qmdocument.h>
#include <qmfoldercombobox.h>
#include <qmfolderlistwindow.h>
#include <qmfolderwindow.h>
#include <qmgoround.h>
#include <qmlistwindow.h>
#include <qmmainwindow.h>
#include <qmmessage.h>
#include <qmmessagewindow.h>
#include <qmsecurity.h>

#include <qsaccelerator.h>
#include <qsconv.h>
#include <qserror.h>
#include <qskeymap.h>
#include <qsnew.h>
#include <qsprofile.h>
#include <qsstl.h>

#include <algorithm>
#include <memory>

#ifdef _WIN32_WCE_PSPC
#	include <aygshell.h>
#endif

#include "attachmentselectionmodel.h"
#include "editframewindow.h"
#include "externaleditor.h"
#include "foldercombobox.h"
#include "folderlistmodel.h"
#include "folderlistwindow.h"
#include "foldermodel.h"
#include "folderselectionmodel.h"
#include "folderwindow.h"
#include "keymap.h"
#include "listwindow.h"
#include "mainwindow.h"
#include "menus.h"
#include "messageframewindow.h"
#include "messagemodel.h"
#include "messageselectionmodel.h"
#include "messagewindow.h"
#include "resourceinc.h"
#include "statusbar.h"
#include "syncdialog.h"
#include "syncutil.h"
#include "uiutil.h"
#include "viewmodel.h"
#include "../action/action.h"
#include "../action/findreplace.h"
#include "../model/filter.h"
#include "../model/goround.h"
#include "../model/tempfilecleaner.h"
#include "../sync/syncmanager.h"

#pragma warning(disable:4786)

using namespace qm;
using namespace qs;


/****************************************************************************
 *
 * MainWindowImpl
 *
 */

class qm::MainWindowImpl :
	public ModalHandler,
	public SplitterWindowHandler,
	public FolderModelHandler,
	public FolderSelectionModel,
	public MessageWindowHandler,
	public ViewModelManagerHandler,
	public DefaultViewModelHandler,
	public DefaultDocumentHandler
{
public:
	enum {
		ID_FOLDERSPLITTERWINDOW	= 1001,
		ID_LISTSPLITTERWINDOW	= 1002,
		ID_FOLDERWINDOW			= 1003,
		ID_FOLDERCOMBOBOX		= 1004,
		ID_LISTCONTAINERWINDOW	= 1005,
		ID_FOLDERLISTWINDOW		= 1006,
		ID_LISTWINDOW			= 1007,
		ID_MESSAGEWINDOW		= 1008,
		ID_TOOLBAR				= 1009,
		ID_STATUSBAR			= 1010,
		ID_COMMANDBARMENU		= 1011,
		ID_COMMANDBARBUTTON		= 1012,
		ID_SYNCNOTIFICATION		= 1013
	};
	
	enum {
		WM_MAINWINDOW_ITEMADDED		= WM_APP + 1101,
		WM_MAINWINDOW_ITEMREMOVED	= WM_APP + 1102,
		WM_MAINWINDOW_ITEMCHANGED	= WM_APP + 1103
	};

public:
	class MessageSelectionModelImpl : public MessageSelectionModel
	{
	public:
		MessageSelectionModelImpl(MainWindowImpl* pMainWindowImpl,
			bool bListOnly, QSTATUS* pstatus);
		virtual ~MessageSelectionModelImpl();
	
	public:
		virtual QSTATUS getSelectedMessages(AccountLock* pAccountLock,
			Folder** ppFolder, MessageHolderList* pList);
		virtual QSTATUS hasSelectedMessage(bool* pbHas);
		virtual QSTATUS getFocusedMessage(MessagePtr* pptr);
		virtual QSTATUS hasFocusedMessage(bool* pbHas);
		virtual QSTATUS selectAll();
		virtual QSTATUS canSelect(bool* pbCan);
	
	private:
		MessageSelectionModelImpl(const MessageSelectionModelImpl&);
		MessageSelectionModelImpl& operator=(const MessageSelectionModelImpl&);
	
	private:
		MainWindowImpl* pMainWindowImpl_;
		bool bListOnly_;
	};

public:
	QSTATUS initActions();
	QSTATUS layoutChildren();
	QSTATUS layoutChildren(int cx, int cy);
	QSTATUS updateStatusBar();

public:
	virtual qs::QSTATUS preModalDialog(HWND hwndParent);
	virtual qs::QSTATUS postModalDialog(HWND hwndParent);

public:
	virtual QSTATUS sizeChanged(const SplitterWindowEvent& event);

public:
	virtual qs::QSTATUS accountSelected(const FolderModelEvent& event);
	virtual qs::QSTATUS folderSelected(const FolderModelEvent& event);

public:
	virtual Account* getAccount();
	virtual qs::QSTATUS getSelectedFolders(Account::FolderList* pList);
	virtual qs::QSTATUS hasSelectedFolder(bool* pbHas);
	virtual Folder* getFocusedFolder();

public:
	virtual QSTATUS messageChanged(const MessageWindowEvent& event);

public:
	virtual qs::QSTATUS viewModelSelected(
		const ViewModelManagerEvent& event);

public:
	virtual qs::QSTATUS itemAdded(const ViewModelEvent& event);
	virtual qs::QSTATUS itemRemoved(const ViewModelEvent& event);
	virtual qs::QSTATUS itemChanged(const ViewModelEvent& event);
	virtual qs::QSTATUS itemStateChanged(const ViewModelEvent& event);
	virtual qs::QSTATUS updated(const ViewModelEvent& event);

public:
	virtual qs::QSTATUS offlineStatusChanged(const DocumentEvent& event);
	virtual qs::QSTATUS accountListChanged(const AccountListChangedEvent& event);

public:
	MainWindow* pThis_;
	
	bool bShowToolbar_;
	bool bShowStatusBar_;
	bool bShowFolderWindow_;
	int nFolderWindowSize_;
	bool bShowFolderComboBox_;
	bool bVirticalFolderWindow_;
	bool bShowPreviewWindow_;
	int nListWindowHeight_;
	
	Profile* pProfile_;
	Document* pDocument_;
	SyncManager* pSyncManager_;
	SyncDialogManager* pSyncDialogManager_;
	GoRound* pGoRound_;
	TempFileCleaner* pTempFileCleaner_;
	Accelerator* pAccelerator_;
	SplitterWindow* pFolderSplitterWindow_;
	SplitterWindow* pListSplitterWindow_;
	FolderWindow* pFolderWindow_;
	FolderComboBox* pFolderComboBox_;
	ListContainerWindow* pListContainerWindow_;
	FolderListWindow* pFolderListWindow_;
	ListWindow* pListWindow_;
	MessageWindow* pMessageWindow_;
	StatusBar* pStatusBar_;
	SyncNotificationWindow* pSyncNotificationWindow_;
	FolderModel* pFolderModel_;
	FolderListModel* pFolderListModel_;
	ViewModelManager* pViewModelManager_;
	PreviewMessageModel* pPreviewModel_;
	MessageSelectionModelImpl* pMessageSelectionModel_;
	MessageSelectionModelImpl* pListOnlyMessageSelectionModel_;
	MessageFrameWindowManager* pMessageFrameWindowManager_;
	EditFrameWindowManager* pEditFrameWindowManager_;
	ActionMap* pActionMap_;
	ActionInvoker* pActionInvoker_;
	FindReplaceManager* pFindReplaceManager_;
	ExternalEditorManager* pExternalEditorManager_;
	MoveMenu* pMoveMenu_;
	FilterMenu* pFilterMenu_;
	SortMenu* pSortMenu_;
	AttachmentMenu* pAttachmentMenu_;
	ViewTemplateMenu* pViewTemplateMenu_;
	CreateTemplateMenu* pCreateTemplateMenu_;
	CreateTemplateMenu* pCreateTemplateExternalMenu_;
	EncodingMenu* pEncodingMenu_;
	SubAccountMenu* pSubAccountMenu_;
	GoRoundMenu* pGoRoundMenu_;
	ScriptMenu* pScriptMenu_;
	DelayedFolderModelHandler* pDelayedFolderModelHandler_;
	bool bCreated_;
	int nInitialShow_;
	bool bLayouting_;
	int nShowingModalDialog_;
	
	HWND hwndLastFocused_;
};

QSTATUS qm::MainWindowImpl::initActions()
{
	DECLARE_QSTATUS();
	
	status = newQsObject(&pActionMap_);
	CHECK_QSTATUS();
	status = newQsObject(pActionMap_, &pActionInvoker_);
	CHECK_QSTATUS();
	status = newQsObject(&pFindReplaceManager_);
	CHECK_QSTATUS();
	
	View* pViews[] = {
		pFolderWindow_,
		pFolderComboBox_,
		pFolderListWindow_,
		pListWindow_,
		pMessageWindow_
	};
	
	status = InitAction5<AttachmentOpenAction, MessageModel*,
		AttachmentSelectionModel*, Profile*, TempFileCleaner*, HWND>(
		pActionMap_, IDM_ATTACHMENT_OPEN, pPreviewModel_,
		pMessageWindow_->getAttachmentSelectionModel(), pProfile_,
		pTempFileCleaner_, pThis_->getHandle());
	CHECK_QSTATUS();
	status = InitAction5<AttachmentSaveAction, MessageModel*,
		AttachmentSelectionModel*, bool, Profile*, HWND>(
		pActionMap_, IDM_ATTACHMENT_SAVE, pPreviewModel_,
		pMessageWindow_->getAttachmentSelectionModel(),
		false, pProfile_, pThis_->getHandle());
	CHECK_QSTATUS();
	status = InitAction5<AttachmentSaveAction, MessageModel*,
		AttachmentSelectionModel*, bool, Profile*, HWND>(
		pActionMap_, IDM_ATTACHMENT_SAVEALL, pPreviewModel_,
		pMessageWindow_->getAttachmentSelectionModel(),
		true, pProfile_, pThis_->getHandle());
	CHECK_QSTATUS();
	status = InitAction1<EditClearDeletedAction, FolderModel*>(
		pActionMap_, IDM_EDIT_CLEARDELETED, pFolderModel_);
	CHECK_QSTATUS();
	std::auto_ptr<EditCopyMessageAction> pCopyMessageAction;
	status = newQsObject(pFolderModel_,
		pMessageSelectionModel_, &pCopyMessageAction);
	CHECK_QSTATUS();
	std::auto_ptr<EditCommandAction> pCopyAction;
	status = newQsObject(pMessageWindow_, &MessageWindowItem::copy,
		&MessageWindowItem::canCopy, &pCopyAction);
	CHECK_QSTATUS();
	Action* pEditCopyActions[] = {
		0,
		0,
		0,
		pCopyMessageAction.get(),
		pCopyAction.get()
	};
	status = InitAction3<DispatchAction, View**, Action**, size_t>(
		pActionMap_, IDM_EDIT_COPY, pViews, pEditCopyActions, countof(pViews));
	CHECK_QSTATUS();
	pCopyMessageAction.release();
	pCopyAction.release();
	std::auto_ptr<EditCutMessageAction> pCutMessageAction;
	status = newQsObject(pFolderModel_,
		pMessageSelectionModel_, &pCutMessageAction);
	CHECK_QSTATUS();
	Action* pEditCutActions[] = {
		0,
		0,
		0,
		pCutMessageAction.get(),
		0
	};
	status = InitAction3<DispatchAction, View**, Action**, size_t>(
		pActionMap_, IDM_EDIT_CUT, pViews, pEditCutActions, countof(pViews));
	CHECK_QSTATUS();
	pCutMessageAction.release();
	status = InitAction1<EditDeleteCacheAction, MessageSelectionModel*>(
		pActionMap_, IDM_EDIT_DELETECACHE, pMessageSelectionModel_);
	CHECK_QSTATUS();
	status = InitAction3<EditDeleteMessageAction, MessageSelectionModel*, bool, HWND>(
		pActionMap_, IDM_EDIT_DELETE, pMessageSelectionModel_, false, pThis_->getHandle());
	CHECK_QSTATUS();
	status = InitAction3<EditDeleteMessageAction, MessageSelectionModel*, bool, HWND>(
		pActionMap_, IDM_EDIT_DELETEDIRECT, pMessageSelectionModel_, true, pThis_->getHandle());
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
	std::auto_ptr<EditPasteMessageAction> pPasteMessageAction;
	status = newQsObject(pDocument_, pFolderModel_, pThis_->getHandle(), &pPasteMessageAction);
	CHECK_QSTATUS();
	Action* pEditPasteActions[] = {
		pPasteMessageAction.get(),
		pPasteMessageAction.get(),
		0,
		pPasteMessageAction.get(),
		0
	};
	status = InitAction3<DispatchAction, View**, Action**, size_t>(
		pActionMap_, IDM_EDIT_PASTE, pViews, pEditPasteActions, countof(pViews));
	CHECK_QSTATUS();
	pPasteMessageAction.release();
	std::auto_ptr<EditSelectAllMessageAction> pSelectAllMessageAction;
	status = newQsObject(pMessageSelectionModel_, &pSelectAllMessageAction);
	CHECK_QSTATUS();
	std::auto_ptr<EditCommandAction> pSelectAllAction;
	status = newQsObject(pMessageWindow_, &MessageWindowItem::selectAll,
		&MessageWindowItem::canSelectAll, &pSelectAllAction);
	CHECK_QSTATUS();
	Action* pEditSelectAllActions[] = {
		0,
		0,
		0,
		pSelectAllMessageAction.get(),
		pSelectAllAction.get()
	};
	status = InitAction3<DispatchAction, View**, Action**, size_t>(
		pActionMap_, IDM_EDIT_SELECTALL, pViews, pEditSelectAllActions, countof(pViews));
	CHECK_QSTATUS();
	pSelectAllMessageAction.release();
	pSelectAllAction.release();
	status = InitAction1<FileCompactAction, FolderModel*>(
		pActionMap_, IDM_FILE_COMPACT, pFolderModel_);
	CHECK_QSTATUS();
	status = InitAction2<FileEmptyTrashAction, FolderModel*, HWND>(
		pActionMap_, IDM_FILE_EMPTYTRASH, pFolderModel_, pThis_->getHandle());
	CHECK_QSTATUS();
	status = InitAction5<FileExitAction, HWND, Document*,
		SyncManager*, TempFileCleaner*, EditFrameWindowManager*>(
		pActionMap_, IDM_FILE_EXIT, pThis_->getHandle(), pDocument_,
		pSyncManager_, pTempFileCleaner_, pEditFrameWindowManager_);
	CHECK_QSTATUS();
	status = InitAction2<FileExportAction, MessageSelectionModel*, HWND>(
		pActionMap_, IDM_FILE_EXPORT, pMessageSelectionModel_, pThis_->getHandle());
	CHECK_QSTATUS();
	status = InitAction2<FileImportAction, FolderModel*, HWND>(
		pActionMap_, IDM_FILE_IMPORT, pFolderModel_, pThis_->getHandle());
	CHECK_QSTATUS();
	status = InitAction2<FileOfflineAction, Document*, SyncManager*>(
		pActionMap_, IDM_FILE_OFFLINE, pDocument_, pSyncManager_);
	CHECK_QSTATUS();
	status = InitAction1<FilePrintAction, MessageSelectionModel*>(
		pActionMap_, IDM_FILE_PRINT, pMessageSelectionModel_);
	CHECK_QSTATUS();
	status = InitAction1<FileSalvageAction, FolderModel*>(
		pActionMap_, IDM_FILE_SALVAGE, pFolderModel_);
	CHECK_QSTATUS();
	status = InitAction2<FileSaveAction, Document*, ViewModelManager*>(
		pActionMap_, IDM_FILE_SAVE, pDocument_, pViewModelManager_);
	CHECK_QSTATUS();
	status = InitAction2<FolderCreateAction, FolderSelectionModel*, HWND>(
		pActionMap_, IDM_FOLDER_CREATE, this, pThis_->getHandle());
	CHECK_QSTATUS();
	status = InitAction2<FolderDeleteAction, FolderModel*, FolderSelectionModel*>(
		pActionMap_, IDM_FOLDER_DELETE, pFolderModel_, this);
	CHECK_QSTATUS();
	status = InitAction1<FolderEmptyAction, FolderSelectionModel*>(
		pActionMap_, IDM_FOLDER_EMPTY, this);
	CHECK_QSTATUS();
	status = InitAction2<FolderPropertyAction, FolderSelectionModel*, HWND>(
		pActionMap_, IDM_FOLDER_PROPERTY, this, pThis_->getHandle());
	CHECK_QSTATUS();
	status = InitAction2<FolderRenameAction, FolderSelectionModel*, HWND>(
		pActionMap_, IDM_FOLDER_RENAME, this, pThis_->getHandle());
	CHECK_QSTATUS();
	status = InitAction1<FolderUpdateAction, FolderModel*>(
		pActionMap_, IDM_FOLDER_UPDATE, pFolderModel_);
	CHECK_QSTATUS();
	status = InitAction1<FolderShowSizeAction, FolderListWindow*>(
		pActionMap_, IDM_FOLDER_SHOWSIZE, pFolderListWindow_);
	CHECK_QSTATUS();
	status = InitAction5<MessageApplyRuleAction, RuleManager*,
		FolderModel*, Document*, HWND, Profile*>(
		pActionMap_, IDM_MESSAGE_APPLYRULE, pDocument_->getRuleManager(),
		pFolderModel_, pDocument_, pThis_->getHandle(), pProfile_);
	CHECK_QSTATUS();
	status = InitAction5<MessageApplyRuleAction, RuleManager*,
		MessageSelectionModel*, Document*, HWND, Profile*>(
		pActionMap_, IDM_MESSAGE_APPLYRULESELECTED, pDocument_->getRuleManager(),
		pMessageSelectionModel_, pDocument_, pThis_->getHandle(), pProfile_);
	CHECK_QSTATUS();
	status = InitActionRange9<MessageApplyTemplateAction, TemplateMenu*,
		Document*, FolderModelBase*, MessageSelectionModel*,
		EditFrameWindowManager*, ExternalEditorManager*, HWND, Profile*, bool>(
		pActionMap_, IDM_MESSAGE_APPLYTEMPLATE,
		IDM_MESSAGE_APPLYTEMPLATE + TemplateMenu::MAX_TEMPLATE,
		pCreateTemplateMenu_, pDocument_, pFolderModel_, pMessageSelectionModel_,
		pEditFrameWindowManager_, pExternalEditorManager_,
		pThis_->getHandle(), pProfile_, false);
	CHECK_QSTATUS();
	status = InitActionRange9<MessageApplyTemplateAction, TemplateMenu*,
		Document*, FolderModelBase*, MessageSelectionModel*,
		EditFrameWindowManager*, ExternalEditorManager*, HWND, Profile*, bool>(
		pActionMap_, IDM_MESSAGE_APPLYTEMPLATEEXTERNAL,
		IDM_MESSAGE_APPLYTEMPLATEEXTERNAL + TemplateMenu::MAX_TEMPLATE,
		pCreateTemplateExternalMenu_, pDocument_, pFolderModel_,
		pMessageSelectionModel_, pEditFrameWindowManager_,
		pExternalEditorManager_, pThis_->getHandle(), pProfile_, true);
	CHECK_QSTATUS();
	status = InitAction2<MessageCombineAction, MessageSelectionModel*, HWND>(
		pActionMap_, IDM_MESSAGE_COMBINE, pMessageSelectionModel_, pThis_->getHandle());
	CHECK_QSTATUS();
	status = InitAction1<MessageExpandDigestAction, MessageSelectionModel*>(
		pActionMap_, IDM_MESSAGE_EXPANDDIGEST, pMessageSelectionModel_);
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
		status = InitAction9<MessageCreateAction, Document*,
			FolderModelBase*, MessageSelectionModel*, const WCHAR*,
			EditFrameWindowManager*, ExternalEditorManager*, HWND, Profile*, bool>(
			pActionMap_, creates[n].nId_, pDocument_, pFolderModel_,
			pMessageSelectionModel_, creates[n].pwszName_,
			pEditFrameWindowManager_, pExternalEditorManager_,
			pThis_->getHandle(), pProfile_, false);
		CHECK_QSTATUS();
		status = InitAction9<MessageCreateAction, Document*,
			FolderModelBase*, MessageSelectionModel*, const WCHAR*,
			EditFrameWindowManager*, ExternalEditorManager*, HWND, Profile*, bool>(
			pActionMap_, creates[n].nIdExternal_, pDocument_, pFolderModel_,
			pMessageSelectionModel_, creates[n].pwszName_,
			pEditFrameWindowManager_, pExternalEditorManager_,
			pThis_->getHandle(), pProfile_, true);
		CHECK_QSTATUS();
	}
	
	status = InitAction5<MessageCreateFromClipboardAction,
		bool, Document*, Profile*, HWND, FolderModel*>(
		pActionMap_, IDM_MESSAGE_CREATEFROMCLIPBOARD, false,
		pDocument_, pProfile_, pThis_->getHandle(), pFolderModel_);
	CHECK_QSTATUS();
	status = InitAction1<MessageDeleteAttachmentAction, MessageSelectionModel*>(
		pActionMap_, IDM_MESSAGE_DELETEATTACHMENT, pMessageSelectionModel_);
	CHECK_QSTATUS();
	status = InitAction3<MessageDetachAction,
		Profile*, MessageSelectionModel*, HWND>(
		pActionMap_, IDM_MESSAGE_DETACH, pProfile_,
		pMessageSelectionModel_, pThis_->getHandle());
	CHECK_QSTATUS();
	status = InitAction5<MessageCreateFromClipboardAction,
		bool, Document*, Profile*, HWND, FolderModel*>(
		pActionMap_, IDM_MESSAGE_DRAFTFROMCLIPBOARD, true,
		pDocument_, pProfile_, pThis_->getHandle(), pFolderModel_);
	CHECK_QSTATUS();
	status = InitActionRange4<MessageOpenAttachmentAction,
		Profile*, AttachmentMenu*, TempFileCleaner*, HWND>(
		pActionMap_, IDM_MESSAGE_ATTACHMENT,
		IDM_MESSAGE_ATTACHMENT + AttachmentMenu::MAX_ATTACHMENT,
		pProfile_, pAttachmentMenu_, pTempFileCleaner_, pThis_->getHandle());
	CHECK_QSTATUS();
	status = InitAction3<MessageMarkAction, MessageSelectionModel*,
		unsigned int, unsigned int>(
		pActionMap_, IDM_MESSAGE_MARK, pMessageSelectionModel_,
		MessageHolder::FLAG_MARKED, MessageHolder::FLAG_MARKED);
	CHECK_QSTATUS();
	status = InitAction3<MessageMarkAction, MessageSelectionModel*,
		unsigned int, unsigned int>(
		pActionMap_, IDM_MESSAGE_MARKDELETED, pMessageSelectionModel_,
		MessageHolder::FLAG_DELETED, MessageHolder::FLAG_DELETED);
	CHECK_QSTATUS();
	status = InitAction3<MessageMarkAction, MessageSelectionModel*,
		unsigned int, unsigned int>(
		pActionMap_, IDM_MESSAGE_MARKDOWNLOAD, pMessageSelectionModel_,
		MessageHolder::FLAG_DOWNLOAD, MessageHolder::FLAG_DOWNLOAD);
	CHECK_QSTATUS();
	status = InitAction3<MessageMarkAction, MessageSelectionModel*,
		unsigned int, unsigned int>(
		pActionMap_, IDM_MESSAGE_MARKDOWNLOADTEXT, pMessageSelectionModel_,
		MessageHolder::FLAG_DOWNLOADTEXT, MessageHolder::FLAG_DOWNLOADTEXT);
	CHECK_QSTATUS();
	status = InitAction3<MessageMarkAction, MessageSelectionModel*,
		unsigned int, unsigned int>(
		pActionMap_, IDM_MESSAGE_MARKSEEN, pMessageSelectionModel_,
		MessageHolder::FLAG_SEEN, MessageHolder::FLAG_SEEN);
	CHECK_QSTATUS();
	status = InitAction3<MessageMarkAction, MessageSelectionModel*,
		unsigned int, unsigned int>(
		pActionMap_, IDM_MESSAGE_UNMARK, pMessageSelectionModel_,
		0, MessageHolder::FLAG_MARKED);
	CHECK_QSTATUS();
	status = InitAction3<MessageMarkAction, MessageSelectionModel*,
		unsigned int, unsigned int>(
		pActionMap_, IDM_MESSAGE_UNMARKDELETED, pMessageSelectionModel_,
		0, MessageHolder::FLAG_DELETED);
	CHECK_QSTATUS();
	status = InitAction3<MessageMarkAction, MessageSelectionModel*,
		unsigned int, unsigned int>(
		pActionMap_, IDM_MESSAGE_UNMARKDOWNLOAD, pMessageSelectionModel_,
		0, MessageHolder::FLAG_DOWNLOAD);
	CHECK_QSTATUS();
	status = InitAction3<MessageMarkAction, MessageSelectionModel*,
		unsigned int, unsigned int>(
		pActionMap_, IDM_MESSAGE_UNMARKDOWNLOADTEXT, pMessageSelectionModel_,
		0, MessageHolder::FLAG_DOWNLOADTEXT);
	CHECK_QSTATUS();
	status = InitAction3<MessageMarkAction, MessageSelectionModel*,
		unsigned int, unsigned int>(
		pActionMap_, IDM_MESSAGE_UNMARKSEEN, pMessageSelectionModel_,
		0, MessageHolder::FLAG_SEEN);
	CHECK_QSTATUS();
	status = InitActionRange3<MessageMoveAction,
		MessageSelectionModel*, MoveMenu*, HWND>(
		pActionMap_, IDM_MESSAGE_MOVE, IDM_MESSAGE_MOVE + MoveMenu::MAX_FOLDER,
		pMessageSelectionModel_, pMoveMenu_, pThis_->getHandle());
	CHECK_QSTATUS();
	status = InitAction4<MessageMoveOtherAction, Document*,
		MessageSelectionModel*, Profile*, HWND>(
		pActionMap_, IDM_MESSAGE_MOVEOTHER, pDocument_,
		pMessageSelectionModel_, pProfile_, pThis_->getHandle());
	CHECK_QSTATUS();
	status = InitAction8<MessageOpenURLAction, Document*,
		FolderModelBase*, MessageSelectionModel*, EditFrameWindowManager*,
		ExternalEditorManager*, HWND, Profile*, bool>(
		pActionMap_, IDM_MESSAGE_OPENURL, pDocument_, pFolderModel_,
		pMessageSelectionModel_, pEditFrameWindowManager_,
		pExternalEditorManager_, pThis_->getHandle(), pProfile_, false);
	CHECK_QSTATUS();
	status = InitAction2<MessagePropertyAction,
		MessageSelectionModel*, HWND>(
		pActionMap_, IDM_MESSAGE_PROPERTY,
		pMessageSelectionModel_, pThis_->getHandle());
	CHECK_QSTATUS();
	status = InitAction4<MessageSearchAction,
		FolderModel*, Document*, HWND, Profile*>(
		pActionMap_, IDM_MESSAGE_SEARCH, pFolderModel_,
		pDocument_, pThis_->getHandle(), pProfile_);
	CHECK_QSTATUS();
	status = InitAction5<ToolAccountAction, Document*,
		FolderModel*, SyncFilterManager*, Profile*, HWND>(
		pActionMap_, IDM_TOOL_ACCOUNT, pDocument_, pFolderModel_,
		pSyncManager_->getSyncFilterManager(), pProfile_, pThis_->getHandle());
	CHECK_QSTATUS();
	status = InitAction1<ToolCheckNewMailAction, Document*>(
		pActionMap_, IDM_TOOL_CHECKNEWMAIL, pDocument_);
	CHECK_QSTATUS();
	status = InitAction4<ToolDialupAction, SyncManager*,
		Document*, SyncDialogManager*, HWND>(
		pActionMap_, IDM_TOOL_DIALUP, pSyncManager_,
		pDocument_, pSyncDialogManager_, pThis_->getHandle());
	CHECK_QSTATUS();
	status = InitActionRange6<ToolGoRoundAction, SyncManager*,
		Document*, GoRound*, SyncDialogManager*, HWND, GoRoundMenu*>(
		pActionMap_, IDM_TOOL_GOROUND, IDM_TOOL_GOROUND + GoRoundMenu::MAX_COURSE,
		pSyncManager_, pDocument_, pGoRound_, pSyncDialogManager_,
		pThis_->getHandle(), pGoRoundMenu_);
	CHECK_QSTATUS();
	status = InitAction1<ToolOptionsAction, Profile*>(
		pActionMap_, IDM_TOOL_OPTIONS, pProfile_);
	CHECK_QSTATUS();
	status = InitActionRange4<ToolScriptAction,
		ScriptMenu*, Document*, Profile*, MainWindow*>(
		pActionMap_, IDM_TOOL_SCRIPT, IDM_TOOL_SCRIPT + ScriptMenu::MAX_SCRIPT,
		pScriptMenu_, pDocument_, pProfile_, pThis_);
	CHECK_QSTATUS();
	status = InitActionRange3<ToolSubAccountAction,
		Document*, FolderModel*, SubAccountMenu*>(
		pActionMap_, IDM_TOOL_SUBACCOUNT,
		IDM_TOOL_SUBACCOUNT + SubAccountMenu::MAX_SUBACCOUNT,
		pDocument_, pFolderModel_, pSubAccountMenu_);
	CHECK_QSTATUS();
	status = InitAction6<ToolSyncAction, SyncManager*, Document*,
		FolderModel*, SyncDialogManager*, unsigned int, HWND>(
		pActionMap_, IDM_TOOL_SYNC, pSyncManager_, pDocument_,
		pFolderModel_, pSyncDialogManager_,
		ToolSyncAction::SYNC_SEND | ToolSyncAction::SYNC_RECEIVE,
		pThis_->getHandle());
	CHECK_QSTATUS();
	status = InitAction6<ToolSyncAction, SyncManager*, Document*,
		FolderModel*, SyncDialogManager*, unsigned int, HWND>(
		pActionMap_, IDM_TOOL_RECEIVE, pSyncManager_, pDocument_,
		pFolderModel_, pSyncDialogManager_,
		ToolSyncAction::SYNC_RECEIVE, pThis_->getHandle());
	CHECK_QSTATUS();
	status = InitAction6<ToolSyncAction, SyncManager*, Document*,
		FolderModel*, SyncDialogManager*, unsigned int, HWND>(
		pActionMap_, IDM_TOOL_SEND, pSyncManager_, pDocument_,
		pFolderModel_, pSyncDialogManager_,
		ToolSyncAction::SYNC_SEND, pThis_->getHandle());
	CHECK_QSTATUS();
	status = InitAction1<ViewLockPreviewAction, PreviewMessageModel*>(
		pActionMap_, IDM_VIEW_LOCKPREVIEW, pPreviewModel_);
	CHECK_QSTATUS();
	status = InitAction4<ViewMessageModeAction, MessageWindow*,
		ViewMessageModeAction::PFN_IS, ViewMessageModeAction::PFN_SET, bool>(
		pActionMap_, IDM_VIEW_DECRYPTVERIFYMODE, pMessageWindow_,
		&MessageWindow::isDecryptVerifyMode, &MessageWindow::setDecryptVerifyMode,
		Security::isEnabled());
	CHECK_QSTATUS();
	status = InitAction1<ViewEncodingAction, MessageWindow*>(
		pActionMap_, IDM_VIEW_ENCODINGAUTODETECT, pMessageWindow_);
	CHECK_QSTATUS();
	status = InitActionRange2<ViewEncodingAction, MessageWindow*, EncodingMenu*>(
		pActionMap_, IDM_VIEW_ENCODING, IDM_VIEW_ENCODING + EncodingMenu::MAX_ENCODING,
		pMessageWindow_, pEncodingMenu_);
	CHECK_QSTATUS();
	status = InitActionRange2<ViewFilterAction, ViewModelManager*, FilterMenu*>(
		pActionMap_, IDM_VIEW_FILTER, IDM_VIEW_FILTER + FilterMenu::MAX_FILTER,
		pViewModelManager_, pFilterMenu_);
	CHECK_QSTATUS();
	status = InitAction2<ViewFilterCustomAction, ViewModelManager*, HWND>(
		pActionMap_, IDM_VIEW_FILTERCUSTOM, pViewModelManager_, pThis_->getHandle());
	CHECK_QSTATUS();
	status = InitAction1<ViewFilterNoneAction, ViewModelManager*>(
		pActionMap_, IDM_VIEW_FILTERNONE, pViewModelManager_);
	CHECK_QSTATUS();
	status = InitAction3<ViewFocusAction, View**, size_t, bool>(
		pActionMap_, IDM_VIEW_FOCUSNEXT, pViews, countof(pViews), true);
	CHECK_QSTATUS();
	status = InitAction3<ViewFocusAction, View**, size_t, bool>(
		pActionMap_, IDM_VIEW_FOCUSPREV, pViews, countof(pViews), false);
	CHECK_QSTATUS();
	status = InitAction4<ViewMessageModeAction, MessageWindow*,
		ViewMessageModeAction::PFN_IS, ViewMessageModeAction::PFN_SET, bool>(
		pActionMap_, IDM_VIEW_HTMLMODE, pMessageWindow_,
		&MessageWindow::isHtmlMode, &MessageWindow::setHtmlMode, true);
	CHECK_QSTATUS();
	status = InitAction4<ViewMessageModeAction, MessageWindow*,
		ViewMessageModeAction::PFN_IS, ViewMessageModeAction::PFN_SET, bool>(
		pActionMap_, IDM_VIEW_HTMLONLINEMODE, pMessageWindow_,
		&MessageWindow::isHtmlOnlineMode, &MessageWindow::setHtmlOnlineMode, true);
	CHECK_QSTATUS();
	status = InitAction3<ViewNavigateFolderAction,
		Document*, FolderModel*, ViewNavigateFolderAction::Type>(
		pActionMap_, IDM_VIEW_NEXTACCOUNT, pDocument_, pFolderModel_,
		ViewNavigateFolderAction::TYPE_NEXTACCOUNT);
	CHECK_QSTATUS();
	status = InitAction1<ViewOpenLinkAction, MessageWindow*>(
		pActionMap_, IDM_VIEW_OPENLINK, pMessageWindow_);
	CHECK_QSTATUS();
	status = InitAction3<ViewNavigateFolderAction,
		Document*, FolderModel*, ViewNavigateFolderAction::Type>(
		pActionMap_, IDM_VIEW_PREVACCOUNT, pDocument_, pFolderModel_,
		ViewNavigateFolderAction::TYPE_PREVACCOUNT);
	CHECK_QSTATUS();
	status = InitAction3<ViewNavigateFolderAction,
		Document*, FolderModel*, ViewNavigateFolderAction::Type>(
		pActionMap_, IDM_VIEW_NEXTFOLDER, pDocument_, pFolderModel_,
		ViewNavigateFolderAction::TYPE_NEXTFOLDER);
	CHECK_QSTATUS();
	status = InitAction3<ViewNavigateFolderAction,
		Document*, FolderModel*, ViewNavigateFolderAction::Type>(
		pActionMap_, IDM_VIEW_PREVFOLDER, pDocument_, pFolderModel_,
		ViewNavigateFolderAction::TYPE_PREVFOLDER);
	CHECK_QSTATUS();
	status = InitAction5<ViewNavigateMessageAction, ViewModelManager*,
		FolderModel*, MainWindow*, MessageWindow*, ViewNavigateMessageAction::Type>(
		pActionMap_, IDM_VIEW_NEXTMESSAGE, pViewModelManager_, pFolderModel_,
		pThis_, pMessageWindow_, ViewNavigateMessageAction::TYPE_NEXT);
	CHECK_QSTATUS();
	status = InitAction5<ViewNavigateMessageAction, ViewModelManager*,
		FolderModel*, MainWindow*, MessageWindow*, ViewNavigateMessageAction::Type>(
		pActionMap_, IDM_VIEW_PREVMESSAGE, pViewModelManager_, pFolderModel_,
		pThis_, pMessageWindow_, ViewNavigateMessageAction::TYPE_PREV);
	CHECK_QSTATUS();
	status = InitAction5<ViewNavigateMessageAction, ViewModelManager*,
		FolderModel*, MainWindow*, MessageWindow*, ViewNavigateMessageAction::Type>(
		pActionMap_, IDM_VIEW_NEXTUNSEENMESSAGE, pViewModelManager_, pFolderModel_,
		pThis_, pMessageWindow_, ViewNavigateMessageAction::TYPE_NEXTUNSEEN);
	CHECK_QSTATUS();
	status = InitAction5<ViewNavigateMessageAction, ViewModelManager*,
		FolderModel*, MainWindow*, MessageWindow*, ViewNavigateMessageAction::Type>(
		pActionMap_, IDM_VIEW_NEXTMESSAGEPAGE, pViewModelManager_, pFolderModel_,
		pThis_, pMessageWindow_, ViewNavigateMessageAction::TYPE_NEXTPAGE);
	CHECK_QSTATUS();
	status = InitAction5<ViewNavigateMessageAction, ViewModelManager*,
		FolderModel*, MainWindow*, MessageWindow*, ViewNavigateMessageAction::Type>(
		pActionMap_, IDM_VIEW_PREVMESSAGEPAGE, pViewModelManager_, pFolderModel_,
		pThis_, pMessageWindow_, ViewNavigateMessageAction::TYPE_PREVPAGE);
	CHECK_QSTATUS();
	status = InitAction4<ViewMessageModeAction, MessageWindow*,
		ViewMessageModeAction::PFN_IS, ViewMessageModeAction::PFN_SET, bool>(
		pActionMap_, IDM_VIEW_RAWMODE, pMessageWindow_,
		&MessageWindow::isRawMode, &MessageWindow::setRawMode, true);
	CHECK_QSTATUS();
	status = InitAction6<ViewRefreshAction, SyncManager*,
		Document*, FolderModel*, SyncDialogManager*, HWND, Profile*>(
		pActionMap_, IDM_VIEW_REFRESH, pSyncManager_, pDocument_,
		pFolderModel_, pSyncDialogManager_, pThis_->getHandle(), pProfile_);
	CHECK_QSTATUS();
	status = InitAction2<ViewScrollAction, HWND, ViewScrollAction::Scroll>(
		pActionMap_, IDM_VIEW_SCROLLLINEUP,
		pListWindow_->getHandle(), ViewScrollAction::SCROLL_LINEUP);
	CHECK_QSTATUS();
	status = InitAction2<ViewScrollAction, HWND, ViewScrollAction::Scroll>(
		pActionMap_, IDM_VIEW_SCROLLLINEDOWN,
		pListWindow_->getHandle(), ViewScrollAction::SCROLL_LINEDOWN);
	CHECK_QSTATUS();
	status = InitAction2<ViewScrollAction, HWND, ViewScrollAction::Scroll>(
		pActionMap_, IDM_VIEW_SCROLLPAGEUP,
		pListWindow_->getHandle(), ViewScrollAction::SCROLL_PAGEUP);
	CHECK_QSTATUS();
	status = InitAction2<ViewScrollAction, HWND, ViewScrollAction::Scroll>(
		pActionMap_, IDM_VIEW_SCROLLPAGEDOWN,
		pListWindow_->getHandle(), ViewScrollAction::SCROLL_PAGEDOWN);
	CHECK_QSTATUS();
	status = InitAction2<ViewScrollAction, HWND, ViewScrollAction::Scroll>(
		pActionMap_, IDM_VIEW_SCROLLTOP,
		pListWindow_->getHandle(), ViewScrollAction::SCROLL_TOP);
	CHECK_QSTATUS();
	status = InitAction2<ViewScrollAction, HWND, ViewScrollAction::Scroll>(
		pActionMap_, IDM_VIEW_SCROLLBOTTOM,
		pListWindow_->getHandle(), ViewScrollAction::SCROLL_BOTTOM);
	CHECK_QSTATUS();
	status = InitAction1<ViewSelectModeAction, MessageWindow*>(
		pActionMap_, IDM_VIEW_SELECTMODE, pMessageWindow_);
	CHECK_QSTATUS();
	status = InitAction1<ViewShowFolderAction, MainWindow*>(
		pActionMap_, IDM_VIEW_SHOWFOLDER, pThis_);
	CHECK_QSTATUS();
	status = InitAction1<ViewShowHeaderAction, MessageWindow*>(
		pActionMap_, IDM_VIEW_SHOWHEADER, pMessageWindow_);
	CHECK_QSTATUS();
	status = InitAction1<ViewShowHeaderColumnAction, ListWindow*>(
		pActionMap_, IDM_VIEW_SHOWHEADERCOLUMN, pListWindow_);
	CHECK_QSTATUS();
	status = InitAction1<ViewShowPreviewAction, MainWindow*>(
		pActionMap_, IDM_VIEW_SHOWPREVIEW, pThis_);
	CHECK_QSTATUS();
	status = InitAction1<ViewShowStatusBarAction<MainWindow>, MainWindow*>(
		pActionMap_, IDM_VIEW_SHOWSTATUSBAR, pThis_);
	CHECK_QSTATUS();
	status = InitAction1<ViewShowSyncDialogAction, SyncDialogManager*>(
		pActionMap_, IDM_VIEW_SHOWSYNCDIALOG, pSyncDialogManager_);
	CHECK_QSTATUS();
	status = InitAction1<ViewShowToolbarAction<MainWindow>, MainWindow*>(
		pActionMap_, IDM_VIEW_SHOWTOOLBAR, pThis_);
	CHECK_QSTATUS();
	status = InitActionRange2<ViewSortAction, ViewModelManager*, SortMenu*>(
		pActionMap_, IDM_VIEW_SORT, IDM_VIEW_SORT + SortMenu::MAX_SORT,
		pViewModelManager_, pSortMenu_);
	CHECK_QSTATUS();
	status = InitAction2<ViewSortDirectionAction, ViewModelManager*, bool>(
		pActionMap_, IDM_VIEW_SORTASCENDING, pViewModelManager_, true);
	CHECK_QSTATUS();
	status = InitAction2<ViewSortDirectionAction, ViewModelManager*, bool>(
		pActionMap_, IDM_VIEW_SORTDESCENDING, pViewModelManager_, false);
	CHECK_QSTATUS();
	status = InitAction1<ViewSortThreadAction, ViewModelManager*>(
		pActionMap_, IDM_VIEW_SORTTHREAD, pViewModelManager_);
	CHECK_QSTATUS();
	status = InitAction1<ViewTemplateAction, MessageWindow*>(
		pActionMap_, IDM_VIEW_TEMPLATENONE, pMessageWindow_);
	CHECK_QSTATUS();
	status = InitActionRange2<ViewTemplateAction,
		MessageWindow*, TemplateMenu*>(
		pActionMap_, IDM_VIEW_TEMPLATE,
		IDM_VIEW_TEMPLATE + TemplateMenu::MAX_TEMPLATE,
		pMessageWindow_, pViewTemplateMenu_);
	CHECK_QSTATUS();
	
	return QSTATUS_SUCCESS;
}

QSTATUS qm::MainWindowImpl::layoutChildren()
{
	RECT rect;
	pThis_->getClientRect(&rect);
	return layoutChildren(rect.right - rect.left, rect.bottom - rect.top);
}

QSTATUS qm::MainWindowImpl::layoutChildren(int cx, int cy)
{
	DECLARE_QSTATUS();
	
	bLayouting_ = true;
	
#if defined _WIN32_WCE && _WIN32_WCE >= 300 && defined _WIN32_WCE_PSPC
	int nToolbarHeight = 0;
#else
	HWND hwndToolbar = pThis_->getToolbar();
	CHECK_QSTATUS();
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
	
	int nFolderComboBoxHeight = 0;
	if (bShowFolderComboBox_) {
		RECT rectFolderComboBox;
		pFolderComboBox_->getWindowRect(&rectFolderComboBox);
		nFolderComboBoxHeight =
			rectFolderComboBox.bottom - rectFolderComboBox.top;
	}
	
#if !defined _WIN32_WCE || _WIN32_WCE < 300 || !defined _WIN32_WCE_PSPC
	wndToolbar.setWindowPos(0, 0, 0, cx,
		rectToolbar.bottom - rectToolbar.top, SWP_NOMOVE | SWP_NOZORDER);
	wndToolbar.showWindow(bShowToolbar_ ? SW_SHOW : SW_HIDE);
#endif
	
	pStatusBar_->setWindowPos(0, 0, cy - nStatusBarHeight,
		cx, rectStatusBar.bottom - rectStatusBar.top, SWP_NOZORDER);
	pStatusBar_->showWindow(bShowStatusBar_ ? SW_SHOW : SW_HIDE);
	
	pSyncNotificationWindow_->setWindowPos(HWND_TOP,
		cx - SyncNotificationWindow::WIDTH, nToolbarHeight, 0, 0, SWP_NOSIZE);
	
	pFolderSplitterWindow_->setWindowPos(0,
		0, nToolbarHeight + nFolderComboBoxHeight, cx,
		cy - nStatusBarHeight - nToolbarHeight - nFolderComboBoxHeight,
		SWP_NOZORDER);
	pFolderSplitterWindow_->showPane(0, 0, bShowFolderWindow_);
	if (bVirticalFolderWindow_)
		pFolderSplitterWindow_->setRowHeight(0, nFolderWindowSize_);
	else
		pFolderSplitterWindow_->setColumnWidth(0, nFolderWindowSize_);
	
	if (bShowFolderComboBox_) {
		pFolderComboBox_->setWindowPos(0, 0, nToolbarHeight, cx,
			/*nFolderComboBoxHeight*/200, SWP_NOZORDER);
		pFolderComboBox_->showWindow(SW_SHOW);
	}
	else {
		pFolderComboBox_->showWindow(SW_HIDE);
	}
	
	pListSplitterWindow_->showPane(0, 1, bShowPreviewWindow_);
	pListSplitterWindow_->setRowHeight(0, nListWindowHeight_);
	
	int nWidth[] = {
		cx - 350,
		cx - 310,
		cx - 230,
		cx - 150,
		cx - 70,
		cx - 50,
		cx - 30,
		-1
	};
	status = pStatusBar_->setParts(nWidth, countof(nWidth));
	CHECK_QSTATUS();
	
	bLayouting_ = false;
	
	return QSTATUS_SUCCESS;
}

QSTATUS qm::MainWindowImpl::updateStatusBar()
{
	assert(::GetCurrentThreadId() == ::GetWindowThreadProcessId(pThis_->getHandle(), 0));
	
	DECLARE_QSTATUS();
	
	if (bShowStatusBar_) {
		ViewModel* pViewModel = pViewModelManager_->getCurrentViewModel();
		
		if (pViewModel) {
			Lock<ViewModel> lock(*pViewModel);
			
			HINSTANCE hInst = Application::getApplication().getResourceHandle();
			string_ptr<WSTRING> wstrTemplate;
			status = loadString(hInst, IDS_VIEWMODELSTATUSTEMPLATE, &wstrTemplate);
			CHECK_QSTATUS();
			WCHAR wsz[256];
			swprintf(wsz, wstrTemplate.get(), pViewModel->getCount(),
				pViewModel->getUnseenCount(), pViewModel->getSelectedCount());
			status = pStatusBar_->setText(0, wsz);
			CHECK_QSTATUS();
			
			UINT nOnlineId = pDocument_->isOffline() ? IDS_OFFLINE : IDS_ONLINE;
			string_ptr<WSTRING> wstrOnline;
			status = loadString(hInst, nOnlineId, &wstrOnline);
			CHECK_QSTATUS();
			status = pStatusBar_->setText(1, wstrOnline.get());
			CHECK_QSTATUS();
			
			const WCHAR* pwszFilterName = 0;
			UINT nFilterId = 0;
			const Filter* pFilter = pViewModel->getFilter();
			if (pFilter) {
				pwszFilterName = pFilter->getName();
				if (!*pwszFilterName)
					nFilterId = IDS_CUSTOM;
			}
			else {
				nFilterId = IDS_NONE;
			}
			string_ptr<WSTRING> wstrFilterName;
			if (nFilterId != 0) {
				status = loadString(hInst, nFilterId, &wstrFilterName);
				CHECK_QSTATUS();
				pwszFilterName = wstrFilterName.get();
			}
			status = pStatusBar_->setText(2, pwszFilterName);
			CHECK_QSTATUS();
		}
		else {
			for (int n = 0; n < 2; ++n) {
				status = pStatusBar_->setText(n, L"");
				CHECK_QSTATUS();
			}
		}
	}
	
	return QSTATUS_SUCCESS;
}

QSTATUS qm::MainWindowImpl::preModalDialog(HWND hwndParent)
{
	DECLARE_QSTATUS();
	
	if (nShowingModalDialog_++ == 0) {
		if (hwndParent != pThis_->getHandle())
			pThis_->enableWindow(false);
		
		status = pMessageFrameWindowManager_->preModalDialog(hwndParent);
		CHECK_QSTATUS();
		status = pEditFrameWindowManager_->preModalDialog(hwndParent);
		CHECK_QSTATUS();
	}
	
	return QSTATUS_SUCCESS;
}

QSTATUS qm::MainWindowImpl::postModalDialog(HWND hwndParent)
{
	DECLARE_QSTATUS();
	
	if (--nShowingModalDialog_ == 0) {
		if (hwndParent != pThis_->getHandle())
			pThis_->enableWindow(true);
		
		status = pMessageFrameWindowManager_->postModalDialog(hwndParent);
		CHECK_QSTATUS();
		status = pEditFrameWindowManager_->postModalDialog(hwndParent);
		CHECK_QSTATUS();
	}
	
	return QSTATUS_SUCCESS;
}

QSTATUS qm::MainWindowImpl::sizeChanged(const SplitterWindowEvent& event)
{
	DECLARE_QSTATUS();
	
	if (bCreated_ && !bLayouting_) {
		SplitterWindow* pSplitterWindow = event.getSplitterWindow();
		if (pSplitterWindow == pFolderSplitterWindow_) {
			if (bVirticalFolderWindow_) {
				status = pSplitterWindow->getRowHeight(0, &nFolderWindowSize_);
				CHECK_QSTATUS();
			}
			else {
				status = pSplitterWindow->getColumnWidth(0, &nFolderWindowSize_);
				CHECK_QSTATUS();
			}
		}
		else if (pSplitterWindow == pListSplitterWindow_) {
			status = pSplitterWindow->getRowHeight(0, &nListWindowHeight_);
			CHECK_QSTATUS();
		}
		else {
			assert(false);
		}
	}
	
	return QSTATUS_SUCCESS;
}

QSTATUS qm::MainWindowImpl::accountSelected(const FolderModelEvent& event)
{
	DECLARE_QSTATUS();
	
	status = pFolderListModel_->setAccount(event.getAccount());
	CHECK_QSTATUS();
	status = pViewModelManager_->setCurrentAccount(event.getAccount());
	CHECK_QSTATUS();
	
	return QSTATUS_SUCCESS;
}

QSTATUS qm::MainWindowImpl::folderSelected(const FolderModelEvent& event)
{
	DECLARE_QSTATUS();
	
	status = pFolderListModel_->setAccount(0);
	CHECK_QSTATUS();
	status = pViewModelManager_->setCurrentFolder(event.getFolder());
	CHECK_QSTATUS();
	
	Folder* pFolder = pFolderModel_->getCurrentFolder();
	if (!pDocument_->isOffline()) {
		if (pFolder->getType() == Folder::TYPE_NORMAL &&
			pFolder->isFlag(Folder::FLAG_SYNCABLE) &&
			pFolder->isFlag(Folder::FLAG_SYNCWHENOPEN)) {
			status = SyncUtil::syncFolder(pSyncManager_,
				pDocument_, pSyncDialogManager_, pThis_->getHandle(),
				SyncDialog::FLAG_NONE, static_cast<NormalFolder*>(pFolder));
			CHECK_QSTATUS();
		}
	}
	
	if (pFolder->getType() == Folder::TYPE_QUERY) {
		status = static_cast<QueryFolder*>(pFolder)->search(
			pDocument_, pThis_->getHandle(), pProfile_);
//		CHECK_QSTATUS();
	}
	
	return QSTATUS_SUCCESS;
}

Account* qm::MainWindowImpl::getAccount()
{
	DECLARE_QSTATUS();
	
	if (pFolderListWindow_->isActive())
		return pFolderListModel_->getFocusedFolder() ?
			0 : pFolderListModel_->getAccount();
	else
		return pFolderModel_->getCurrentAccount();
}

QSTATUS qm::MainWindowImpl::getSelectedFolders(Account::FolderList* pList)
{
	assert(pList);
	
	DECLARE_QSTATUS();
	
	if (pFolderListWindow_->isActive()) {
		status = pFolderListModel_->getSelectedFolders(pList);
		CHECK_QSTATUS();
	}
	else {
		Folder* pFolder = pFolderModel_->getCurrentFolder();
		if (pFolder) {
			status = STLWrapper<Account::FolderList>(
				*pList).push_back(pFolder);
			CHECK_QSTATUS();
		}
	}
	
	return QSTATUS_SUCCESS;
}

QSTATUS qm::MainWindowImpl::hasSelectedFolder(bool* pbHas)
{
	assert(pbHas);
	
	if (pFolderListWindow_->isActive())
		*pbHas = pFolderListModel_->hasSelectedFolder();
	else
		*pbHas = pFolderModel_->getCurrentFolder() != 0;
	
	return QSTATUS_SUCCESS;
}

Folder* qm::MainWindowImpl::getFocusedFolder()
{
	if (pFolderListWindow_->isActive())
		return pFolderListModel_->getFocusedFolder();
	else
		return pFolderModel_->getCurrentFolder();
}

QSTATUS qm::MainWindowImpl::messageChanged(const MessageWindowEvent& event)
{
	DECLARE_QSTATUS();
	
	if (bShowStatusBar_) {
		status = UIUtil::updateStatusBar(pMessageWindow_, pStatusBar_,
			2, event.getMessageHolder(), event.getMessage());
		CHECK_QSTATUS();
	}
	
	return QSTATUS_SUCCESS;
}

QSTATUS qm::MainWindowImpl::viewModelSelected(
	const ViewModelManagerEvent& event)
{
	DECLARE_QSTATUS();
	
	ViewModel* pOldViewModel = event.getOldViewModel();
	if (pOldViewModel) {
		status = pOldViewModel->removeViewModelHandler(this);
		CHECK_QSTATUS();
	}
	
	ViewModel* pNewViewModel = event.getNewViewModel();
	if (pNewViewModel) {
		status = pNewViewModel->addViewModelHandler(this);
		CHECK_QSTATUS();
	}
	
	status = updateStatusBar();
	CHECK_QSTATUS();
	
	return QSTATUS_SUCCESS;
}

QSTATUS qm::MainWindowImpl::itemAdded(const ViewModelEvent& event)
{
	pThis_->postMessage(WM_MAINWINDOW_ITEMADDED);
	return QSTATUS_SUCCESS;
}

QSTATUS qm::MainWindowImpl::itemRemoved(const ViewModelEvent& event)
{
	pThis_->postMessage(WM_MAINWINDOW_ITEMREMOVED);
	return QSTATUS_SUCCESS;
}

QSTATUS qm::MainWindowImpl::itemChanged(const ViewModelEvent& event)
{
	pThis_->postMessage(WM_MAINWINDOW_ITEMCHANGED);
	return QSTATUS_SUCCESS;
}

QSTATUS qm::MainWindowImpl::itemStateChanged(const ViewModelEvent& event)
{
	return updateStatusBar();
}

QSTATUS qm::MainWindowImpl::updated(const ViewModelEvent& event)
{
	return updateStatusBar();
}

QSTATUS qm::MainWindowImpl::offlineStatusChanged(const DocumentEvent& event)
{
	return updateStatusBar();
}

QSTATUS qm::MainWindowImpl::accountListChanged(
	const AccountListChangedEvent& event)
{
	DECLARE_QSTATUS();
	
	status = pFolderModel_->setCurrent(0, 0, false);
	CHECK_QSTATUS();
	status = pFolderListModel_->setAccount(0);
	CHECK_QSTATUS();
	status = pViewModelManager_->setCurrentAccount(0);
	CHECK_QSTATUS();
	
	return QSTATUS_SUCCESS;
}


/****************************************************************************
 *
 * MainWindowImpl::MessageSelectionModelImpl
 *
 */

qm::MainWindowImpl::MessageSelectionModelImpl::MessageSelectionModelImpl(
	MainWindowImpl* pMainWindowImpl, bool bListOnly, QSTATUS* pstatus) :
	pMainWindowImpl_(pMainWindowImpl),
	bListOnly_(bListOnly)
{
	assert(pstatus);
	*pstatus = QSTATUS_SUCCESS;
}

qm::MainWindowImpl::MessageSelectionModelImpl::~MessageSelectionModelImpl()
{
}

QSTATUS qm::MainWindowImpl::MessageSelectionModelImpl::getSelectedMessages(
	AccountLock* pAccountLock, Folder** ppFolder, MessageHolderList* pList)
{
	assert(pAccountLock);
	assert(pList);
	
	DECLARE_QSTATUS();
	
	if (ppFolder)
		*ppFolder = 0;
	
	if (pMainWindowImpl_->pListWindow_->isActive()) {
		ViewModel* pViewModel = pMainWindowImpl_->pViewModelManager_->getCurrentViewModel();
		if (pViewModel) {
			Lock<ViewModel> lock(*pViewModel);
			pAccountLock->set(pViewModel->getFolder()->getAccount());
			if (ppFolder)
				*ppFolder = pViewModel->getFolder();
			status = pViewModel->getSelection(pList);
			CHECK_QSTATUS();
		}
	}
	else if (pMainWindowImpl_->pMessageWindow_->isActive() && !bListOnly_) {
		MessagePtrLock mpl(pMainWindowImpl_->pPreviewModel_->getCurrentMessage());
		if (mpl) {
			pAccountLock->set(mpl->getAccount());
			if (ppFolder)
				*ppFolder = mpl->getFolder();
			status = STLWrapper<MessageHolderList>(*pList).push_back(mpl);
			CHECK_QSTATUS();
		}
	}
	
	return QSTATUS_SUCCESS;
}

QSTATUS qm::MainWindowImpl::MessageSelectionModelImpl::hasSelectedMessage(bool* pbHas)
{
	assert(pbHas);
	
	*pbHas = false;
	
	if (pMainWindowImpl_->pListWindow_->isActive()) {
		ViewModel* pViewModel = pMainWindowImpl_->pViewModelManager_->getCurrentViewModel();
		if (pViewModel) {
			Lock<ViewModel> lock(*pViewModel);
			*pbHas = pViewModel->hasSelection();
		}
	}
	else if (pMainWindowImpl_->pMessageWindow_->isActive() && !bListOnly_) {
		MessagePtrLock mpl(pMainWindowImpl_->pPreviewModel_->getCurrentMessage());
		*pbHas = mpl != 0;
	}
	
	return QSTATUS_SUCCESS;
}

QSTATUS qm::MainWindowImpl::MessageSelectionModelImpl::getFocusedMessage(MessagePtr* pptr)
{
	assert(pptr);
	
	DECLARE_QSTATUS();
	
	if (pMainWindowImpl_->pListWindow_->isActive()) {
		ViewModel* pViewModel = pMainWindowImpl_->pViewModelManager_->getCurrentViewModel();
		if (pViewModel) {
			Lock<ViewModel> lock(*pViewModel);
			if (pViewModel->getCount() != 0) {
				unsigned int nItem = pViewModel->getFocused();
				*pptr = MessagePtr(pViewModel->getMessageHolder(nItem));
			}
		}
	}
	else if (pMainWindowImpl_->pMessageWindow_->isActive() && !bListOnly_) {
		*pptr = pMainWindowImpl_->pPreviewModel_->getCurrentMessage();
	}
	
	return QSTATUS_SUCCESS;
}

QSTATUS qm::MainWindowImpl::MessageSelectionModelImpl::hasFocusedMessage(bool* pbHas)
{
	assert(pbHas);
	
	DECLARE_QSTATUS();
	
	*pbHas = false;
	
	if (pMainWindowImpl_->pListWindow_->isActive()) {
		ViewModel* pViewModel = pMainWindowImpl_->pViewModelManager_->getCurrentViewModel();
		if (pViewModel) {
			Lock<ViewModel> lock(*pViewModel);
			*pbHas = pViewModel->getCount() != 0;
		}
	}
	else if (pMainWindowImpl_->pMessageWindow_->isActive() && !bListOnly_) {
		MessagePtrLock mpl(pMainWindowImpl_->pPreviewModel_->getCurrentMessage());
		*pbHas = mpl != 0;
	}
	
	return QSTATUS_SUCCESS;
}

QSTATUS qm::MainWindowImpl::MessageSelectionModelImpl::selectAll()
{
	DECLARE_QSTATUS();
	
	if (!pMainWindowImpl_->pListWindow_->isActive())
		return QSTATUS_FAIL;
	
	ViewModel* pViewModel = pMainWindowImpl_->pViewModelManager_->getCurrentViewModel();
	if (!pViewModel)
		return QSTATUS_FAIL;
	
	Lock<ViewModel> lock(*pViewModel);
	unsigned int nCount = pViewModel->getCount();
	if (nCount != 0) {
		status = pViewModel->setSelection(0,  nCount - 1);
		CHECK_QSTATUS();
	}
	
	return QSTATUS_SUCCESS;
}

QSTATUS qm::MainWindowImpl::MessageSelectionModelImpl::canSelect(bool* pbCan)
{
	assert(pbCan);
	*pbCan = pMainWindowImpl_->pListWindow_->isActive() &&
		pMainWindowImpl_->pViewModelManager_->getCurrentViewModel();
	return QSTATUS_SUCCESS;
}


/****************************************************************************
 *
 * MainWindow
 *
 */

qm::MainWindow::MainWindow(Profile* pProfile, QSTATUS* pstatus) :
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
		L"MainWindow", L"ShowToolbar", 1, &nShowToolbar);
	CHECK_QSTATUS_SET(pstatus);
	int nShowStatusBar = 0;
	status = pProfile->getInt(
		L"MainWindow", L"ShowStatusBar", 1, &nShowStatusBar);
	CHECK_QSTATUS_SET(pstatus);
	int nShowFolderWindow = 0;
	status = pProfile->getInt(
		L"MainWindow", L"ShowFolderWindow", 1, &nShowFolderWindow);
	CHECK_QSTATUS_SET(pstatus);
	int nFolderWindowSize = 0;
	status = pProfile->getInt(
		L"MainWindow", L"FolderWindowSize", 100, &nFolderWindowSize);
	CHECK_QSTATUS_SET(pstatus);
	int nShowFolderComboBox = 0;
	status = pProfile->getInt(
		L"MainWindow", L"ShowFolderComboBox", 0, &nShowFolderComboBox);
	CHECK_QSTATUS_SET(pstatus);
	int nVirticalFolderWindow = 0;
	status = pProfile->getInt(
		L"MainWindow", L"VirticalFolderWindow", 0, &nVirticalFolderWindow);
	int nShowPreviewWindow = 0;
	status = pProfile->getInt(
		L"MainWindow", L"ShowPreviewWindow", 1, &nShowPreviewWindow);
	CHECK_QSTATUS_SET(pstatus);
	int nListWindowHeight = 0;
	status = pProfile->getInt(
		L"MainWindow", L"ListWindowHeight", 200, &nListWindowHeight);
	CHECK_QSTATUS_SET(pstatus);
	
	status = newObject(&pImpl_);
	CHECK_QSTATUS_SET(pstatus);
	pImpl_->pThis_ = this;
	pImpl_->bShowToolbar_ = nShowToolbar != 0;
	pImpl_->bShowStatusBar_ = nShowStatusBar != 0;
	pImpl_->bShowFolderWindow_ = nShowFolderWindow != 0;
	pImpl_->nFolderWindowSize_ = nFolderWindowSize;
	pImpl_->bShowFolderComboBox_ = nShowFolderComboBox != 0;
	pImpl_->bVirticalFolderWindow_ = nVirticalFolderWindow != 0;
	pImpl_->bShowPreviewWindow_ = nShowPreviewWindow != 0;
	pImpl_->nListWindowHeight_ = nListWindowHeight;
	pImpl_->pProfile_ = pProfile;
	pImpl_->pDocument_ = 0;
	pImpl_->pSyncManager_ = 0;
	pImpl_->pSyncDialogManager_ = 0;
	pImpl_->pGoRound_ = 0;
	pImpl_->pTempFileCleaner_ = 0;
	pImpl_->pAccelerator_ = 0;
	pImpl_->pFolderSplitterWindow_ = 0;
	pImpl_->pListSplitterWindow_ = 0;
	pImpl_->pFolderWindow_ = 0;
	pImpl_->pFolderComboBox_ = 0;
	pImpl_->pListContainerWindow_ = 0;
	pImpl_->pFolderListWindow_ = 0;
	pImpl_->pListWindow_ = 0;
	pImpl_->pMessageWindow_ = 0;
	pImpl_->pStatusBar_ = 0;
	pImpl_->pSyncNotificationWindow_ = 0;
	pImpl_->pFolderModel_ = 0;
	pImpl_->pFolderListModel_ = 0;
	pImpl_->pViewModelManager_ = 0;
	pImpl_->pPreviewModel_ = 0;
	pImpl_->pMessageSelectionModel_ = 0;
	pImpl_->pListOnlyMessageSelectionModel_ = 0;
	pImpl_->pMessageFrameWindowManager_ = 0;
	pImpl_->pEditFrameWindowManager_ = 0;
	pImpl_->pActionMap_ = 0;
	pImpl_->pActionInvoker_ = 0;
	pImpl_->pFindReplaceManager_ = 0;
	pImpl_->pExternalEditorManager_ = 0;
	pImpl_->pMoveMenu_ = 0;
	pImpl_->pFilterMenu_ = 0;
	pImpl_->pSortMenu_ = 0;
	pImpl_->pAttachmentMenu_ = 0;
	pImpl_->pViewTemplateMenu_ = 0;
	pImpl_->pCreateTemplateMenu_ = 0;
	pImpl_->pCreateTemplateExternalMenu_ = 0;
	pImpl_->pEncodingMenu_ = 0;
	pImpl_->pSubAccountMenu_ = 0;
	pImpl_->pGoRoundMenu_ = 0;
	pImpl_->pScriptMenu_ = 0;
	pImpl_->pDelayedFolderModelHandler_ = 0;
	pImpl_->bCreated_ = false;
	pImpl_->nInitialShow_ = SW_SHOWNORMAL;
	pImpl_->bLayouting_ = false;
	pImpl_->nShowingModalDialog_ = 0;
	pImpl_->hwndLastFocused_ = 0;
	
	setModalHandler(pImpl_);
}

qm::MainWindow::~MainWindow()
{
	if (pImpl_) {
		delete pImpl_->pAccelerator_;
		delete pImpl_->pMessageFrameWindowManager_;
		delete pImpl_->pEditFrameWindowManager_;
		delete pImpl_->pPreviewModel_;
		delete pImpl_->pMessageSelectionModel_;
		delete pImpl_->pListOnlyMessageSelectionModel_;
		delete pImpl_->pViewModelManager_;
		delete pImpl_->pFolderListModel_;
		delete pImpl_->pFolderModel_;
		delete pImpl_->pActionMap_;
		delete pImpl_->pActionInvoker_;
		delete pImpl_->pFindReplaceManager_;
		delete pImpl_->pExternalEditorManager_;
		delete pImpl_->pMoveMenu_;
		delete pImpl_->pFilterMenu_;
		delete pImpl_->pSortMenu_;
		delete pImpl_->pAttachmentMenu_;
		delete pImpl_->pViewTemplateMenu_;
		delete pImpl_->pCreateTemplateMenu_;
		delete pImpl_->pCreateTemplateExternalMenu_;
		delete pImpl_->pEncodingMenu_;
		delete pImpl_->pSubAccountMenu_;
		delete pImpl_->pGoRoundMenu_;
		delete pImpl_->pScriptMenu_;
		delete pImpl_->pDelayedFolderModelHandler_;
		delete pImpl_;
		pImpl_ = 0;
	}
}

FolderModel* qm::MainWindow::getFolderModel() const
{
	return pImpl_->pFolderModel_;
}

const ActionInvoker* qm::MainWindow::getActionInvoker() const
{
	return pImpl_->pActionInvoker_;
}

bool qm::MainWindow::isShowingModalDialog() const
{
	return pImpl_->nShowingModalDialog_ != 0;
}

void qm::MainWindow::initialShow()
{
	showWindow(pImpl_->nInitialShow_);
}

QSTATUS qm::MainWindow::save() const
{
	DECLARE_QSTATUS();
	
	status = pImpl_->pMessageFrameWindowManager_->save();
	CHECK_QSTATUS();
	status = pImpl_->pViewModelManager_->save();
	CHECK_QSTATUS();
	status = pImpl_->pListWindow_->save();
	CHECK_QSTATUS();
	status = pImpl_->pMessageWindow_->save();
	CHECK_QSTATUS();
	status = pImpl_->pFolderListWindow_->save();
	CHECK_QSTATUS();
	
	Profile* pProfile = pImpl_->pProfile_;
	
	status = pProfile->setInt(L"MainWindow",
		L"ShowToolbar", pImpl_->bShowToolbar_);
	CHECK_QSTATUS();
	status = pProfile->setInt(L"MainWindow",
		L"ShowStatusBar", pImpl_->bShowStatusBar_);
	CHECK_QSTATUS();
	
	if (pImpl_->bShowFolderWindow_) {
		if (pImpl_->bVirticalFolderWindow_) {
			status = pImpl_->pFolderSplitterWindow_->getRowHeight(
				0, &pImpl_->nFolderWindowSize_);
			CHECK_QSTATUS();
		}
		else {
			status = pImpl_->pFolderSplitterWindow_->getColumnWidth(
				0, &pImpl_->nFolderWindowSize_);
			CHECK_QSTATUS();
		}
	}
	status = pProfile->setInt(L"MainWindow",
		L"FolderWindowSize", pImpl_->nFolderWindowSize_);
	CHECK_QSTATUS();
	status = pProfile->setInt(L"MainWindow",
		L"ShowFolderWindow", pImpl_->bShowFolderWindow_);
	CHECK_QSTATUS();
	status = pProfile->setInt(L"MainWindow",
		L"ShowFolderComboBox", pImpl_->bShowFolderComboBox_);
	CHECK_QSTATUS();
	
	if (pImpl_->bShowPreviewWindow_) {
		status = pImpl_->pListSplitterWindow_->getRowHeight(
			0, &pImpl_->nListWindowHeight_);
		CHECK_QSTATUS();
	}
	status = pProfile->setInt(L"MainWindow",
		L"ListWindowHeight", pImpl_->nListWindowHeight_);
	CHECK_QSTATUS();
	status = pProfile->setInt(L"MainWindow",
		L"ShowPreviewWindow", pImpl_->bShowPreviewWindow_);
	CHECK_QSTATUS();
	
	status = UIUtil::saveWindowPlacement(getHandle(), pProfile, L"MainWindow");
	CHECK_QSTATUS();
	
	return QSTATUS_SUCCESS;
}

bool qm::MainWindow::isShowToolbar() const
{
	return pImpl_->bShowToolbar_;
}

QSTATUS qm::MainWindow::setShowToolbar(bool bShow)
{
	DECLARE_QSTATUS();
	
	if (bShow != pImpl_->bShowToolbar_) {
		pImpl_->bShowToolbar_ = bShow;
		status = pImpl_->layoutChildren();
	}
	
	return status;
}

bool qm::MainWindow::isShowStatusBar() const
{
	return pImpl_->bShowStatusBar_;
}

QSTATUS qm::MainWindow::setShowStatusBar(bool bShow)
{
	DECLARE_QSTATUS();
	
	if (bShow != pImpl_->bShowStatusBar_) {
		pImpl_->bShowStatusBar_ = bShow;
		status = pImpl_->layoutChildren();
	}
	
	return status;
}

bool qm::MainWindow::isShowFolderWindow() const
{
	return pImpl_->bShowFolderWindow_;
}

QSTATUS qm::MainWindow::setShowFolderWindow(bool bShow)
{
	DECLARE_QSTATUS();
	if (bShow != pImpl_->bShowFolderWindow_) {
		if (!bShow) {
			if (pImpl_->bVirticalFolderWindow_)
				pImpl_->pFolderSplitterWindow_->getRowHeight(
					0, &pImpl_->nFolderWindowSize_);
			else
				pImpl_->pFolderSplitterWindow_->getColumnWidth(
					0, &pImpl_->nFolderWindowSize_);
		}
		pImpl_->bShowFolderWindow_ = bShow;
		pImpl_->bShowFolderComboBox_ = !bShow;
		status = pImpl_->layoutChildren();
	}
	return status;
}

bool qm::MainWindow::isShowPreviewWindow() const
{
	return pImpl_->bShowPreviewWindow_;
}

QSTATUS qm::MainWindow::setShowPreviewWindow(bool bShow)
{
	DECLARE_QSTATUS();
	
	if (bShow != pImpl_->bShowPreviewWindow_) {
		if (!bShow)
			pImpl_->pListSplitterWindow_->getRowHeight(
				0, &pImpl_->nListWindowHeight_);
		pImpl_->bShowPreviewWindow_ = bShow;
		status = pImpl_->layoutChildren();
		
		if (bShow) {
			status = pImpl_->pPreviewModel_->connectToViewModel();
			CHECK_QSTATUS();
		}
		else {
			status = pImpl_->pPreviewModel_->disconnectFromViewModel();
			CHECK_QSTATUS();
		}
	}
	
	return status;
}

QSTATUS qm::MainWindow::getToolbarButtons(Toolbar* pToolbar, bool* pbToolbar)
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
	pToolbar->nId_ = MainWindowImpl::ID_TOOLBAR;
	pToolbar->nBitmapId_ = IDB_TOOLBAR;
	
	*pbToolbar = true;
	
	return QSTATUS_SUCCESS;
}

#if defined _WIN32_WCE && (_WIN32_WCE < 300 || !defined _WIN32_WCE_PSPC)
QSTATUS qm::MainWindow::getBarId(int n, UINT* pnId) const
{
	assert(n == 0 || n == 1);
	assert(pnId);
	
	static UINT nIds[] = {
		MainWindowImpl::ID_COMMANDBARMENU,
		MainWindowImpl::ID_COMMANDBARBUTTON
	};
	*pnId = nIds[n];
	
	return QSTATUS_SUCCESS;
}

QSTATUS qm::MainWindow::getCommandBandsRestoreInfo(int n,
	COMMANDBANDSRESTOREINFO* pcbri) const
{
	DECLARE_QSTATUS();
	
	WCHAR wszKey[32];
	swprintf(wszKey, L"CommandBandsRestoreInfo%d", n);
	int nSize = sizeof(*pcbri);
	status = pImpl_->pProfile_->getBinary(L"MainWindow", wszKey,
		reinterpret_cast<unsigned char*>(pcbri), &nSize);
	if (nSize != sizeof(*pcbri))
		pcbri->cbSize = 0;
	return status;
}

QSTATUS qm::MainWindow::setCommandBandsRestoreInfo(int n,
	const COMMANDBANDSRESTOREINFO& cbri)
{
	WCHAR wszKey[32];
	swprintf(wszKey, L"CommandBandsRestoreInfo%d", n);
	return pImpl_->pProfile_->setBinary(L"MainWindow", wszKey,
		reinterpret_cast<const unsigned char*>(&cbri), sizeof(cbri));
}
#endif

QSTATUS qm::MainWindow::getMenuHandle(void* pCreateParam, HMENU* phmenu)
{
	assert(phmenu);
	
	DECLARE_QSTATUS();
	
	MainWindowCreateContext* pContext =
		static_cast<MainWindowCreateContext*>(pCreateParam);
	
	status = pContext->pMenuManager_->getMenu(
		L"mainframe", true, true, phmenu);
	CHECK_QSTATUS();
	
	return QSTATUS_SUCCESS;
}

QSTATUS qm::MainWindow::getIconId(UINT* pnId)
{
	assert(pnId);
	*pnId = IDI_MAINFRAME;
	return QSTATUS_SUCCESS;
}

QSTATUS qm::MainWindow::getWindowClass(WNDCLASS* pwc)
{
	DECLARE_QSTATUS();
	
	status = DefaultWindowHandler::getWindowClass(pwc);
	CHECK_QSTATUS();
	
	pwc->hIcon = ::LoadIcon(Application::getApplication().getResourceHandle(),
		MAKEINTRESOURCE(IDI_MAINFRAME));
	pwc->hbrBackground = reinterpret_cast<HBRUSH>(COLOR_BTNFACE + 1);
	
	return QSTATUS_SUCCESS;
}

QSTATUS qm::MainWindow::preCreateWindow(CREATESTRUCT* pCreateStruct)
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
	status = UIUtil::loadWindowPlacement(pImpl_->pProfile_,
		L"MainWindow", pCreateStruct, &pImpl_->nInitialShow_);
	CHECK_QSTATUS();
#endif
	
	return QSTATUS_SUCCESS;
}

QSTATUS qm::MainWindow::getAction(UINT nId, Action** ppAction)
{
	assert(ppAction);
	*ppAction = pImpl_->pActionMap_->getAction(nId);
	return QSTATUS_SUCCESS;
}

QSTATUS qm::MainWindow::getAccelerator(Accelerator** ppAccelerator)
{
	assert(ppAccelerator);
	*ppAccelerator = pImpl_->pAccelerator_;
	return QSTATUS_SUCCESS;
}

LRESULT qm::MainWindow::windowProc(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	BEGIN_MESSAGE_HANDLER()
		HANDLE_ACTIVATE()
		HANDLE_CLOSE()
		HANDLE_COPYDATA()
		HANDLE_CREATE()
		HANDLE_DESTROY()
#ifndef _WIN32_WCE
		HANDLE_ENDSESSION()
#endif
		HANDLE_INITMENUPOPUP()
#ifndef _WIN32_WCE
		HANDLE_QUERYENDSESSION()
#endif
		HANDLE_SIZE()
		HANDLE_MESSAGE(MainWindowImpl::WM_MAINWINDOW_ITEMADDED, onItemAdded)
		HANDLE_MESSAGE(MainWindowImpl::WM_MAINWINDOW_ITEMREMOVED, onItemRemoved)
		HANDLE_MESSAGE(MainWindowImpl::WM_MAINWINDOW_ITEMCHANGED, onItemChanged)
	END_MESSAGE_HANDLER()
	return FrameWindow::windowProc(uMsg, wParam, lParam);
}

LRESULT qm::MainWindow::onActivate(UINT nFlags, HWND hwnd, bool bMinimized)
{
	FrameWindow::onActivate(nFlags, hwnd, bMinimized);
	
	// TODO
	// Handle other thread
	
	if (nFlags == WA_INACTIVE) {
		pImpl_->hwndLastFocused_ = ::GetFocus();
	}
	else {
		HWND hwndFocus = pImpl_->hwndLastFocused_;
		if (!hwndFocus)
			hwndFocus = pImpl_->pListWindow_->getHandle();
		::SetFocus(hwndFocus);
		
		HIMC hImc = ::ImmGetContext(getHandle());
		::ImmSetOpenStatus(hImc, FALSE);
		::ImmReleaseContext(getHandle(), hImc);
	}
	
	return 0;
}

LRESULT qm::MainWindow::onClose()
{
	FileExitAction* pAction = static_cast<FileExitAction*>(
		pImpl_->pActionMap_->getAction(IDM_FILE_EXIT));
	assert(pAction);
	pAction->exit(true, 0);
	return 0;
}

LRESULT qm::MainWindow::onCopyData(HWND hwnd, COPYDATASTRUCT* pData)
{
	Variant v(::SysAllocString(static_cast<WCHAR*>(pData->lpData)));
	if (v.bstrVal) {
		VARIANT* pvars[] = { &v };
		pImpl_->pActionInvoker_->invoke(pData->dwData, pvars, countof(pvars));
	}
	return 1;
}

LRESULT qm::MainWindow::onCreate(CREATESTRUCT* pCreateStruct)
{
	if (FrameWindow::onCreate(pCreateStruct) == -1)
		return -1;
	
	DECLARE_QSTATUS();
	
	MainWindowCreateContext* pContext =
		static_cast<MainWindowCreateContext*>(pCreateStruct->lpCreateParams);
	pImpl_->pDocument_ = pContext->pDocument_;
	pImpl_->pSyncManager_ = pContext->pSyncManager_;
	pImpl_->pSyncDialogManager_ = pContext->pSyncDialogManager_;
	pImpl_->pGoRound_ = pContext->pGoRound_;
	pImpl_->pTempFileCleaner_ = pContext->pTempFileCleaner_;
	
	status = pContext->pKeyMap_->createAccelerator(
		CustomAcceleratorFactory(), L"MainWindow",
		mapKeyNameToId, countof(mapKeyNameToId), &pImpl_->pAccelerator_);
	CHECK_QSTATUS_VALUE(-1);
	
	std::auto_ptr<DefaultFolderModel> pFolderModel;
	status = newQsObject(&pFolderModel);
	CHECK_QSTATUS_VALUE(-1);
	pImpl_->pFolderModel_ = pFolderModel.release();
	
	status = newQsObject(&pImpl_->pFolderListModel_);
	CHECK_QSTATUS_VALUE(-1);
	
	status = newQsObject(pImpl_->pProfile_, pImpl_->pDocument_,
		getHandle(), pImpl_->pFolderModel_, &pImpl_->pViewModelManager_);
	CHECK_QSTATUS_VALUE(-1);
	
	status = newQsObject(pImpl_->pViewModelManager_,
		pImpl_->bShowPreviewWindow_, &pImpl_->pPreviewModel_);
	CHECK_QSTATUS();
	
	status = newQsObject(pImpl_->pDocument_, pImpl_->pSyncManager_,
		pImpl_->pSyncDialogManager_, pContext->pKeyMap_,
		pImpl_->pProfile_, pContext->pMenuManager_,
		&pImpl_->pEditFrameWindowManager_);
	CHECK_QSTATUS_VALUE(-1);
	
	status = newQsObject(pImpl_->pDocument_, pImpl_->pProfile_,
		getHandle(), pImpl_->pFolderModel_, &pImpl_->pExternalEditorManager_);
	CHECK_QSTATUS_VALUE(-1);
	
	status = newQsObject(pImpl_->pDocument_, pImpl_->pTempFileCleaner_,
		pContext->pMenuManager_, pContext->pKeyMap_, pImpl_->pProfile_,
		pImpl_->pViewModelManager_, pImpl_->pEditFrameWindowManager_,
		pImpl_->pExternalEditorManager_, &pImpl_->pMessageFrameWindowManager_);
	CHECK_QSTATUS_VALUE(-1);
	
	bool bVirticalFolderWindow = pImpl_->bVirticalFolderWindow_;
	DWORD dwStyle = WS_CHILD | WS_VISIBLE | WS_CLIPCHILDREN | WS_CLIPSIBLINGS;
#if defined _WIN32_WCE && _WIN32_WCE >= 300 && defined _WIN32_WCE_PSPC
	DWORD dwExStyle = 0;
#else
	DWORD dwExStyle = WS_EX_CLIENTEDGE;
#endif
	
	std::auto_ptr<SplitterWindow> pFolderSplitterWindow;
	status = newQsObject(bVirticalFolderWindow ? 1 : 2,
		bVirticalFolderWindow ? 2 : 1, true, pImpl_, &pFolderSplitterWindow);
	CHECK_QSTATUS_VALUE(-1);
	status = pFolderSplitterWindow->create(L"QmFolderSplitterWindow",
		0, dwStyle, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
		CW_USEDEFAULT, getHandle(), 0, 0,
		MainWindowImpl::ID_FOLDERSPLITTERWINDOW, 0);
	CHECK_QSTATUS_VALUE(-1);
	pImpl_->pFolderSplitterWindow_ = pFolderSplitterWindow.release();
	
	std::auto_ptr<FolderWindow> pFolderWindow;
	status = newQsObject(pImpl_->pFolderSplitterWindow_,
		pImpl_->pFolderModel_, pImpl_->pProfile_, &pFolderWindow);
	CHECK_QSTATUS_VALUE(-1);
	FolderWindowCreateContext folderWindowContext = {
		pContext->pDocument_,
		pContext->pMenuManager_,
		pContext->pKeyMap_
	};
	status = pFolderWindow->create(L"QmFolderWindow",
		0, dwStyle, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
		CW_USEDEFAULT, pImpl_->pFolderSplitterWindow_->getHandle(),
		dwExStyle, 0, MainWindowImpl::ID_FOLDERWINDOW, &folderWindowContext);
	CHECK_QSTATUS_VALUE(-1);
	pImpl_->pFolderWindow_ = pFolderWindow.release();
	
	std::auto_ptr<FolderComboBox> pFolderComboBox;
	status = newQsObject(this, pImpl_->pFolderModel_,
		pImpl_->pProfile_, &pFolderComboBox);
	CHECK_QSTATUS_VALUE(-1);
	FolderComboBoxCreateContext folderComboBoxContext = {
		pContext->pDocument_,
		pContext->pMenuManager_,
		pContext->pKeyMap_
	};
	status = pFolderComboBox->create(L"QmFolderComboBox",
		0, dwStyle, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
		CW_USEDEFAULT, getHandle(), dwExStyle, 0,
		MainWindowImpl::ID_FOLDERCOMBOBOX, &folderComboBoxContext);
	CHECK_QSTATUS_VALUE(-1);
	pImpl_->pFolderComboBox_ = pFolderComboBox.release();
	
	std::auto_ptr<SplitterWindow> pListSplitterWindow;
	status = newQsObject(1, 2, true, pImpl_, &pListSplitterWindow);
	CHECK_QSTATUS_VALUE(-1);
	status = pListSplitterWindow->create(L"QmListSplitterWindow",
		0, dwStyle, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
		CW_USEDEFAULT, getHandle(), 0, 0,
		MainWindowImpl::ID_LISTSPLITTERWINDOW, 0);
	CHECK_QSTATUS_VALUE(-1);
	pImpl_->pListSplitterWindow_ = pListSplitterWindow.release();
	
	std::auto_ptr<ListContainerWindow> pListContainerWindow;
	status = newQsObject(pImpl_->pFolderModel_, &pListContainerWindow);
	CHECK_QSTATUS_VALUE(-1);
	status = pListContainerWindow->create(L"QmListContainerWindow", 0,
		dwStyle, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
		pImpl_->pListSplitterWindow_->getHandle(), dwExStyle, 0,
		MainWindowImpl::ID_LISTCONTAINERWINDOW, 0);
	CHECK_QSTATUS_VALUE(-1);
	pImpl_->pListContainerWindow_ = pListContainerWindow.release();
	
	std::auto_ptr<FolderListWindow> pFolderListWindow;
	status = newQsObject(pImpl_->pListContainerWindow_,
		pImpl_->pFolderListModel_, pImpl_->pFolderModel_,
		pImpl_->pProfile_, &pFolderListWindow);
	CHECK_QSTATUS_VALUE(-1);
	FolderListWindowCreateContext folderListContext = {
		pContext->pDocument_,
		pContext->pMenuManager_,
		pContext->pKeyMap_
	};
	status = pFolderListWindow->create(L"QmFolderListWindow",
		0, dwStyle, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
		CW_USEDEFAULT, pImpl_->pListContainerWindow_->getHandle(),
		0, 0, MainWindowImpl::ID_FOLDERLISTWINDOW, &folderListContext);
	CHECK_QSTATUS_VALUE(-1);
	pImpl_->pFolderListWindow_ = pFolderListWindow.release();
	pImpl_->pListContainerWindow_->setFolderListWindow(
		pImpl_->pFolderListWindow_);
	
	std::auto_ptr<ListWindow> pListWindow;
	status = newQsObject(pImpl_->pViewModelManager_, pImpl_->pProfile_,
		pImpl_->pMessageFrameWindowManager_, &pListWindow);
	CHECK_QSTATUS_VALUE(-1);
	ListWindowCreateContext listContext = {
		pContext->pDocument_,
		pContext->pMenuManager_,
		pContext->pKeyMap_
	};
	status = pListWindow->create(L"QmListWindow",
		0, dwStyle, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
		CW_USEDEFAULT, pImpl_->pListContainerWindow_->getHandle(),
		0, 0, MainWindowImpl::ID_LISTWINDOW, &listContext);
	CHECK_QSTATUS_VALUE(-1);
	pImpl_->pListWindow_ = pListWindow.release();
	pImpl_->pListContainerWindow_->setListWindow(pImpl_->pListWindow_);
	
	std::auto_ptr<MessageWindow> pMessageWindow;
	status = newQsObject(pImpl_->pPreviewModel_, pImpl_->pProfile_,
		L"PreviewWindow", &pMessageWindow);
	CHECK_QSTATUS_VALUE(-1);
	MessageWindowCreateContext messageContext = {
		pContext->pDocument_,
		pContext->pMenuManager_,
		pContext->pKeyMap_
	};
	status = pMessageWindow->create(L"QmMessageWindow",
		0, dwStyle, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
		CW_USEDEFAULT, pImpl_->pFolderSplitterWindow_->getHandle(),
		0, 0, MainWindowImpl::ID_MESSAGEWINDOW, &messageContext);
	CHECK_QSTATUS_VALUE(-1);
	pImpl_->pMessageWindow_ = pMessageWindow.release();
	
	status = pImpl_->pListSplitterWindow_->add(
		0, 0, pImpl_->pListContainerWindow_);
	CHECK_QSTATUS_VALUE(-1);
	status = pImpl_->pListSplitterWindow_->add(
		0, 1, pImpl_->pMessageWindow_);
	CHECK_QSTATUS_VALUE(-1);
	
	status = pImpl_->pFolderSplitterWindow_->add(
		0, 0, pImpl_->pFolderWindow_);
	CHECK_QSTATUS_VALUE(-1);
	status = pImpl_->pFolderSplitterWindow_->add(
		bVirticalFolderWindow ? 0 : 1, bVirticalFolderWindow ? 1 : 0,
		pImpl_->pListSplitterWindow_);
	CHECK_QSTATUS_VALUE(-1);
	
	std::auto_ptr<StatusBar> pStatusBar;
	status = newQsObject(&pStatusBar);
	CHECK_QSTATUS_VALUE(-1);
	status = pStatusBar->create(L"QmStatusBarWindow",
		0, dwStyle, CW_USEDEFAULT, CW_USEDEFAULT,
		CW_USEDEFAULT, CW_USEDEFAULT, getHandle(), 0,
		STATUSCLASSNAMEW, MainWindowImpl::ID_STATUSBAR, 0);
	CHECK_QSTATUS_VALUE(-1);
	pImpl_->pStatusBar_ = pStatusBar.release();
	
	std::auto_ptr<SyncNotificationWindow> pSyncNotificationWindow;
	status = newQsObject(pImpl_->pSyncManager_,
		pImpl_->pSyncDialogManager_, &pSyncNotificationWindow);
	CHECK_QSTATUS_VALUE(-1);
	status = pSyncNotificationWindow->create(L"QmSyncNotificationWindow",
		0, dwStyle & ~WS_VISIBLE, 0, 0, SyncNotificationWindow::WIDTH,
		SyncNotificationWindow::HEIGHT, getHandle(), 0,
		0, MainWindowImpl::ID_SYNCNOTIFICATION, 0);
	CHECK_QSTATUS_VALUE(-1);
	pImpl_->pSyncNotificationWindow_ = pSyncNotificationWindow.release();
	
	status = pImpl_->layoutChildren();
	CHECK_QSTATUS_VALUE(-1);
	
	if (bVirticalFolderWindow)
		pImpl_->pFolderSplitterWindow_->setRowHeight(
			0, pImpl_->nFolderWindowSize_);
	else
		pImpl_->pFolderSplitterWindow_->setColumnWidth(
			0, pImpl_->nFolderWindowSize_);
	
	pImpl_->pListSplitterWindow_->setRowHeight(
		0, pImpl_->nListWindowHeight_);
	
	status = newQsObject(pImpl_, false, &pImpl_->pMessageSelectionModel_);
	CHECK_QSTATUS_VALUE(-1);
	status = newQsObject(pImpl_, true, &pImpl_->pListOnlyMessageSelectionModel_);
	CHECK_QSTATUS_VALUE(-1);
	
	status = newQsObject(&pImpl_->pMoveMenu_);
	CHECK_QSTATUS_VALUE(-1);
	
	status = newQsObject(pImpl_->pViewModelManager_->getFilterManager(),
		&pImpl_->pFilterMenu_);
	CHECK_QSTATUS_VALUE(-1);
	
	status = newQsObject(pImpl_->pViewModelManager_, &pImpl_->pSortMenu_);
	CHECK_QSTATUS_VALUE(-1);
	
	status = newQsObject(&pImpl_->pAttachmentMenu_);
	CHECK_QSTATUS_VALUE(-1);
	
	status = newQsObject(pImpl_->pDocument_->getTemplateManager(),
		&pImpl_->pViewTemplateMenu_);
	CHECK_QSTATUS_VALUE(-1);
	
	status = newQsObject(pImpl_->pDocument_->getTemplateManager(),
		false, &pImpl_->pCreateTemplateMenu_);
	CHECK_QSTATUS_VALUE(-1);
	
	status = newQsObject(pImpl_->pDocument_->getTemplateManager(),
		true, &pImpl_->pCreateTemplateExternalMenu_);
	CHECK_QSTATUS_VALUE(-1);
	
	status = newQsObject(pImpl_->pProfile_, &pImpl_->pEncodingMenu_);
	CHECK_QSTATUS();
	
	status = newQsObject(pImpl_->pFolderModel_, &pImpl_->pSubAccountMenu_);
	CHECK_QSTATUS();
	
	status = newQsObject(pImpl_->pGoRound_, &pImpl_->pGoRoundMenu_);
	CHECK_QSTATUS();
	
	status = newQsObject(pImpl_->pDocument_->getScriptManager(),
		&pImpl_->pScriptMenu_);
	CHECK_QSTATUS_VALUE(-1);
	
	status = newQsObject(pImpl_, &pImpl_->pDelayedFolderModelHandler_);
	CHECK_QSTATUS_VALUE(-1);
	status = pImpl_->pFolderModel_->addFolderModelHandler(
		pImpl_->pDelayedFolderModelHandler_);
	CHECK_QSTATUS_VALUE(-1);
	
	status = pImpl_->pDocument_->addDocumentHandler(pImpl_);
	CHECK_QSTATUS_VALUE(-1);
	status = pImpl_->pViewModelManager_->addViewModelManagerHandler(pImpl_);
	CHECK_QSTATUS_VALUE(-1);
	status = pImpl_->pMessageWindow_->addMessageWindowHandler(pImpl_);
	CHECK_QSTATUS_VALUE(-1);
	
	status = pImpl_->initActions();
	CHECK_QSTATUS_VALUE(-1);
	
	pImpl_->bCreated_ = true;
	
	return 0;
}

LRESULT qm::MainWindow::onDestroy()
{
	pImpl_->pMessageWindow_->removeMessageWindowHandler(pImpl_);
	pImpl_->pViewModelManager_->removeViewModelManagerHandler(pImpl_);
	pImpl_->pFolderModel_->removeFolderModelHandler(
		pImpl_->pDelayedFolderModelHandler_);
	pImpl_->pDocument_->removeDocumentHandler(pImpl_);
	
	::PostQuitMessage(0);
	
	return FrameWindow::onDestroy();
}

#ifndef _WIN32_WCE
LRESULT qm::MainWindow::onEndSession(bool bEnd, int nOption)
{
	if (bEnd)
		Application::getApplication().uninitialize();
	return 0;
}
#endif

LRESULT qm::MainWindow::onInitMenuPopup(HMENU hmenu, UINT nIndex, bool bSysMenu)
{
	DECLARE_QSTATUS();
	
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
		
		Account* pAccount = pImpl_->pFolderModel_->getCurrentAccount();
		if (!pAccount) {
			Folder* pFolder = pImpl_->pFolderModel_->getCurrentFolder();
			if (pFolder)
				pAccount = pFolder->getAccount();
		}
		if (nIdLast == IDM_MESSAGE_MOVEOTHER) {
			if (pAccount) {
				status = pImpl_->pMoveMenu_->createMenu(hmenu, pAccount,
					::GetKeyState(VK_SHIFT) < 0, *pImpl_->pActionMap_);
				// TODO
			}
			else {
				::EnableMenuItem(hmenu, nIndex, MF_GRAYED | MF_BYPOSITION);
			}
		}
		else if (nIdLast == IDM_VIEW_SORTTHREAD) {
			status = pImpl_->pSortMenu_->createMenu(hmenu);
			// TODO
		}
		else if (nIdFirst == IDM_VIEW_FILTERNONE) {
			status = pImpl_->pFilterMenu_->createMenu(hmenu);
			// TODO
		}
		else if (nIdFirst == IDM_TOOL_GOROUND) {
			status = pImpl_->pGoRoundMenu_->createMenu(hmenu);
			// TODO
		}
		else if (nIdFirst == IDM_TOOL_SUBACCOUNT) {
			status = pImpl_->pSubAccountMenu_->createMenu(hmenu);
			// TODO
		}
		else if (nIdFirst == IDM_MESSAGE_DETACH) {
			AccountLock lock;
			MessageHolderList l;
			status = pImpl_->pMessageSelectionModel_->getSelectedMessages(&lock, 0, &l);
			if (status == QSTATUS_SUCCESS) {
				status = pImpl_->pAttachmentMenu_->createMenu(hmenu, l);
				// TODO
			}
		}
		else if (nIdFirst == IDM_VIEW_TEMPLATENONE) {
			if (pAccount) {
				status = pImpl_->pViewTemplateMenu_->createMenu(hmenu, pAccount);
			}
			else {
				::EnableMenuItem(hmenu, nIndex, MF_GRAYED | MF_BYPOSITION);
			}
		}
		else if (nIdFirst == IDM_MESSAGE_APPLYTEMPLATE ||
			nIdFirst == IDM_MESSAGE_APPLYTEMPLATENONE) {
			if (pAccount) {
				status = pImpl_->pCreateTemplateMenu_->createMenu(hmenu, pAccount);
			}
			else {
				::EnableMenuItem(hmenu, nIndex, MF_GRAYED | MF_BYPOSITION);
			}
		}
		else if (nIdFirst == IDM_MESSAGE_APPLYTEMPLATEEXTERNAL ||
			nIdFirst == IDM_MESSAGE_APPLYTEMPLATENONEEXTERNAL) {
			if (pAccount) {
				status = pImpl_->pCreateTemplateExternalMenu_->createMenu(hmenu, pAccount);
			}
			else {
				::EnableMenuItem(hmenu, nIndex, MF_GRAYED | MF_BYPOSITION);
			}
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

#ifndef _WIN32_WCE
LRESULT qm::MainWindow::onQueryEndSession(int nOption)
{
	FileExitAction* pAction = static_cast<FileExitAction*>(
		pImpl_->pActionMap_->getAction(IDM_FILE_EXIT));
	assert(pAction);
	bool bCanceled = false;
	pAction->exit(false, &bCanceled);
	return !bCanceled;
}
#endif

LRESULT qm::MainWindow::onSize(UINT nFlags, int cx, int cy)
{
	if (pImpl_->bCreated_ &&
		!pImpl_->bLayouting_ &&
		(nFlags == SIZE_RESTORED || nFlags == SIZE_MAXIMIZED))
		pImpl_->layoutChildren(cx, cy);
	return FrameWindow::onSize(nFlags, cx, cy);
}

LRESULT qm::MainWindow::onItemAdded(WPARAM wParam, LPARAM lParam)
{
	MSG msg;
	while (::PeekMessage(&msg, getHandle(),
		MainWindowImpl::WM_MAINWINDOW_ITEMADDED,
		MainWindowImpl::WM_MAINWINDOW_ITEMADDED, PM_REMOVE));
	pImpl_->updateStatusBar();
	return 0;
}

LRESULT qm::MainWindow::onItemRemoved(WPARAM wParam, LPARAM lParam)
{
	MSG msg;
	while (::PeekMessage(&msg, getHandle(),
		MainWindowImpl::WM_MAINWINDOW_ITEMREMOVED,
		MainWindowImpl::WM_MAINWINDOW_ITEMREMOVED, PM_REMOVE));
	pImpl_->updateStatusBar();
	return 0;
}

LRESULT qm::MainWindow::onItemChanged(WPARAM wParam, LPARAM lParam)
{
	MSG msg;
	while (::PeekMessage(&msg, getHandle(),
		MainWindowImpl::WM_MAINWINDOW_ITEMCHANGED,
		MainWindowImpl::WM_MAINWINDOW_ITEMCHANGED, PM_REMOVE));
	pImpl_->updateStatusBar();
	return 0;
}


/****************************************************************************
 *
 * ListContainerWindow
 *
 */

qm::ListContainerWindow::ListContainerWindow(
	FolderModel* pFolderModel, QSTATUS* pstatus) :
	WindowBase(true, pstatus),
	DefaultWindowHandler(pstatus),
	pFolderModel_(pFolderModel),
	pFolderListWindow_(0),
	pListWindow_(0),
	pDelayedFolderModelHandler_(0)
{
	DECLARE_QSTATUS();
	
	status = newQsObject(this, &pDelayedFolderModelHandler_);
	CHECK_QSTATUS_SET(pstatus);
	
	status = pFolderModel_->addFolderModelHandler(pDelayedFolderModelHandler_);
	CHECK_QSTATUS_SET(pstatus);
	
	setWindowHandler(this, false);
}

qm::ListContainerWindow::~ListContainerWindow()
{
	pFolderModel_->removeFolderModelHandler(pDelayedFolderModelHandler_);
	delete pDelayedFolderModelHandler_;
}

void qm::ListContainerWindow::setFolderListWindow(
	FolderListWindow* pFolderListWindow)
{
	pFolderListWindow_ = pFolderListWindow;
	pFolderListWindow_->showWindow(SW_HIDE);
}

void qm::ListContainerWindow::setListWindow(ListWindow* pListWindow)
{
	pListWindow_ = pListWindow;
}

LRESULT qm::ListContainerWindow::windowProc(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	BEGIN_MESSAGE_HANDLER()
		HANDLE_SIZE()
	END_MESSAGE_HANDLER()
	return DefaultWindowHandler::windowProc(uMsg, wParam, lParam);
}

LRESULT qm::ListContainerWindow::onSize(UINT nFlags, int cx, int cy)
{
	if (pFolderListWindow_)
		pFolderListWindow_->setWindowPos(0, 0, 0, cx, cy,
			SWP_NOZORDER | SWP_NOMOVE | SWP_NOACTIVATE);
	if (pListWindow_)
		pListWindow_->setWindowPos(0, 0, 0, cx, cy,
			SWP_NOZORDER | SWP_NOMOVE | SWP_NOACTIVATE);
	return DefaultWindowHandler::onSize(nFlags, cx, cy);
}

QSTATUS qm::ListContainerWindow::accountSelected(const FolderModelEvent& event)
{
	assert(pFolderListWindow_);
	assert(pListWindow_);
	
	bool bActive = pListWindow_->isActive();
	
	pFolderListWindow_->showWindow(SW_SHOW);
	pListWindow_->showWindow(SW_HIDE);
	
	if (bActive)
		pFolderListWindow_->setActive();
	
	return QSTATUS_SUCCESS;
}

QSTATUS qm::ListContainerWindow::folderSelected(const FolderModelEvent& event)
{
	assert(pFolderListWindow_);
	assert(pListWindow_);
	
	bool bActive = pFolderListWindow_->isActive();
	
	pListWindow_->showWindow(SW_SHOW);
	pFolderListWindow_->showWindow(SW_HIDE);
	
	if (bActive)
		pListWindow_->setActive();
	
	return QSTATUS_SUCCESS;
}


/****************************************************************************
 *
 * SyncNotificationWindow
 *
 */

qm::SyncNotificationWindow::SyncNotificationWindow(SyncManager* pSyncManager,
	SyncDialogManager* pSyncDialogManager, QSTATUS* pstatus) :
	WindowBase(true, pstatus),
	DefaultWindowHandler(pstatus),
	pSyncManager_(pSyncManager),
	pSyncDialogManager_(pSyncDialogManager),
	hbm_(0)
{
	setWindowHandler(this, false);
}

qm::SyncNotificationWindow::~SyncNotificationWindow()
{
}

QSTATUS qm::SyncNotificationWindow::getWindowClass(WNDCLASS* pwc)
{
	DECLARE_QSTATUS();
	
	status = DefaultWindowHandler::getWindowClass(pwc);
	CHECK_QSTATUS();
	
	pwc->hbrBackground = reinterpret_cast<HBRUSH>(::GetStockObject(BLACK_BRUSH));
	
	return QSTATUS_SUCCESS;
}

LRESULT qm::SyncNotificationWindow::windowProc(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	BEGIN_MESSAGE_HANDLER()
		HANDLE_CREATE()
		HANDLE_DESTROY()
		HANDLE_LBUTTONDOWN()
		HANDLE_PAINT()
		HANDLE_MESSAGE(WM_SYNCNOTIFICATION_STATUSCHANGED, onStatusChanged)
	END_MESSAGE_HANDLER()
	return DefaultWindowHandler::windowProc(uMsg, wParam, lParam);
}

LRESULT qm::SyncNotificationWindow::onCreate(CREATESTRUCT* pCreateStruct)
{
	if (DefaultWindowHandler::onCreate(pCreateStruct) == -1)
		return -1;
	
	DECLARE_QSTATUS();
	
	hbm_ = static_cast<HBITMAP>(::LoadImage(
		Application::getApplication().getResourceHandle(),
		MAKEINTRESOURCE(IDB_SYNCNOTIFICATION), IMAGE_BITMAP, 0, 0, 0));
	
	status = pSyncManager_->addSyncManagerHandler(this);
	CHECK_QSTATUS_VALUE(-1);
	
	return 0;
}

LRESULT qm::SyncNotificationWindow::onDestroy()
{
	::DeleteObject(hbm_);
	pSyncManager_->removeSyncManagerHandler(this);
	return DefaultWindowHandler::onDestroy();
}

LRESULT qm::SyncNotificationWindow::onLButtonDown(UINT nFlags, const POINT& pt)
{
	DECLARE_QSTATUS();
	
	SyncDialog* pDialog = 0;
	status = pSyncDialogManager_->open(&pDialog);
	if (status == QSTATUS_SUCCESS)
		pDialog->show();
	
	return DefaultWindowHandler::onLButtonDown(nFlags, pt);
}

LRESULT qm::SyncNotificationWindow::onPaint()
{
	DECLARE_QSTATUS();
	
	PaintDeviceContext dc(getHandle(), &status);
	CHECK_QSTATUS_VALUE(0);
	
	CompatibleDeviceContext dcMem(dc.getHandle(), &status);
	CHECK_QSTATUS_VALUE(0);
	ObjectSelector<HBITMAP> selector(dcMem, hbm_);
	
	dc.bitBlt(0, 0, WIDTH, HEIGHT, dcMem.getHandle(), 0, 0, SRCCOPY);
	
	return 0;
}

LRESULT qm::SyncNotificationWindow::onStatusChanged(WPARAM wParam, LPARAM lParam)
{
	if (pSyncManager_->isSyncing())
		showWindow(SW_SHOW);
	else
		showWindow(SW_HIDE);
	return 0;
}

QSTATUS qm::SyncNotificationWindow::statusChanged(const SyncManagerEvent& event)
{
	postMessage(WM_SYNCNOTIFICATION_STATUSCHANGED);
	return QSTATUS_SUCCESS;
}
