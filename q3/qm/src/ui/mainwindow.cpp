/*
 * $Id$
 *
 * Copyright(C) 1998-2005 Satoshi Nakamura
 * All rights reserved.
 *
 */

#include <qmapplication.h>
#include <qmdocument.h>
#include <qmfilenames.h>
#include <qmfoldercombobox.h>
#include <qmfolderlistwindow.h>
#include <qmfolderwindow.h>
#include <qmgoround.h>
#include <qmlistwindow.h>
#include <qmmainwindow.h>
#include <qmmessage.h>
#include <qmmessagewindow.h>
#include <qmrecents.h>
#include <qmsecurity.h>
#include <qmtabwindow.h>

#include <qsaccelerator.h>
#include <qsconv.h>
#include <qskeymap.h>
#include <qsprofile.h>
#include <qsstl.h>
#include <qstheme.h>
#include <qsuiutil.h>

#include <algorithm>
#include <memory>

#include <tchar.h>
#ifndef _WIN32_WCE
#	include <tmschema.h>
#endif
#ifdef _WIN32_WCE_PSPC
#	include <aygshell.h>
#endif

#include "actionid.h"
#include "addressbookwindow.h"
#include "editframewindow.h"
#include "externaleditor.h"
#include "foldercombobox.h"
#include "folderlistwindow.h"
#include "folderwindow.h"
#include "keymap.h"
#include "listwindow.h"
#include "mainwindow.h"
#include "menus.h"
#include "messageframewindow.h"
#include "messagewindow.h"
#include "resourceinc.h"
#include "statusbar.h"
#include "syncdialog.h"
#include "syncutil.h"
#include "tabwindow.h"
#include "uimanager.h"
#include "uiutil.h"
#include "../action/action.h"
#include "../action/actionmacro.h"
#include "../action/findreplace.h"
#include "../model/filter.h"
#include "../model/goround.h"
#include "../model/tempfilecleaner.h"
#include "../sync/autopilot.h"
#include "../sync/syncmanager.h"
#include "../uimodel/attachmentselectionmodel.h"
#include "../uimodel/encodingmodel.h"
#include "../uimodel/folderlistmodel.h"
#include "../uimodel/foldermodel.h"
#include "../uimodel/folderselectionmodel.h"
#include "../uimodel/messagemodel.h"
#include "../uimodel/messageselectionmodel.h"
#include "../uimodel/securitymodel.h"
#include "../uimodel/tabmodel.h"
#include "../uimodel/viewmodel.h"

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
	public ViewModelHolder,
	public MessageWindowHandler,
	public AccountManagerHandler,
#ifdef QMTABWINDOW
	public RecentsHandler,
	public ViewModelManagerHandler,
	public DefaultTabModelHandler
#else
	public RecentsHandler
#endif
{
public:
	enum {
		ID_FOLDERSPLITTERWINDOW	= 1001,
		ID_LISTSPLITTERWINDOW	= 1002,
		ID_FOLDERWINDOW			= 1003,
		ID_FOLDERCOMBOBOX		= 1004,
		ID_TABWINDOW			= 1005,
		ID_LISTCONTAINERWINDOW	= 1006,
		ID_FOLDERLISTWINDOW		= 1007,
		ID_LISTWINDOW			= 1008,
		ID_MESSAGEWINDOW		= 1009,
		ID_TOOLBAR				= 1010,
		ID_STATUSBAR			= 1011,
		ID_COMMANDBARMENU		= 1012,
		ID_COMMANDBARBUTTON		= 1013,
		ID_SYNCNOTIFICATION		= 1014,
		ID_NOTIFYICON			= 1015,
	};
	
	enum {
		WM_MAINWINDOW_NOTIFYICON		= WM_APP + 1001,
		WM_MAINWINDOW_RECENTSCHANGED	= WM_APP + 1002
	};
	
#ifndef _WIN32_WCE_PSPC
	enum {
		HOTKEY_RECENTS	= 0xC000
	};
#endif

public:
	class MessageSelectionModelImpl : public MessageSelectionModel
	{
	public:
		MessageSelectionModelImpl(MainWindowImpl* pMainWindowImpl,
								  bool bListOnly);
		virtual ~MessageSelectionModelImpl();
	
	public:
		virtual void getSelectedMessages(AccountLock* pAccountLock,
										 Folder** ppFolder,
										 MessageHolderList* pList);
		virtual bool hasSelectedMessage();
		virtual MessagePtr getFocusedMessage();
		virtual bool hasFocusedMessage();
		virtual void selectAll();
		virtual bool canSelect();
	
	private:
		MessageSelectionModelImpl(const MessageSelectionModelImpl&);
		MessageSelectionModelImpl& operator=(const MessageSelectionModelImpl&);
	
	private:
		MainWindowImpl* pMainWindowImpl_;
		bool bListOnly_;
	};

public:
	void initActions();
	void layoutChildren();
	void layoutChildren(int cx,
						int cy);
	void updateStatusBar();
#ifndef _WIN32_WCE_PSPC
	void showRecentsMenu();
#endif

public:
	virtual void preModalDialog(HWND hwndParent);
	virtual void postModalDialog(HWND hwndParent);

public:
	virtual void sizeChanged(const SplitterWindowEvent& event);

public:
	virtual void accountSelected(const FolderModelEvent& event);
	virtual void folderSelected(const FolderModelEvent& event);

public:
	virtual Account* getAccount();
	virtual void getSelectedFolders(Account::FolderList* pList);
	virtual bool hasSelectedFolder();
	virtual Folder* getFocusedFolder();
	virtual std::pair<Account*, Folder*> getTemporaryFocused();

public:
	virtual ViewModel* getViewModel() const;
	virtual void setViewModel(ViewModel* pViewModel);

public:
	virtual void messageChanged(const MessageWindowEvent& event);
	virtual void statusTextChanged(const MessageWindowStatusTextEvent& event);

public:
	virtual void accountListChanged(const AccountManagerEvent& event);

public:
	virtual void recentsChanged(const RecentsEvent& event);

#ifdef QMTABWINDOW
public:
	virtual void viewModelSelected(const ViewModelManagerEvent& event);

public:
	virtual void currentChanged(const TabModelEvent& event);
#endif

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
	bool bSaveOnDeactivate_;
	
	Profile* pProfile_;
	Document* pDocument_;
	UIManager* pUIManager_;
	PasswordManager* pPasswordManager_;
	SyncManager* pSyncManager_;
	SyncDialogManager* pSyncDialogManager_;
	GoRound* pGoRound_;
	TempFileCleaner* pTempFileCleaner_;
	AutoPilot* pAutoPilot_;
	std::auto_ptr<Accelerator> pAccelerator_;
	SplitterWindow* pFolderSplitterWindow_;
	SplitterWindow* pListSplitterWindow_;
	FolderWindow* pFolderWindow_;
	FolderComboBox* pFolderComboBox_;
#ifdef QMTABWINDOW
	TabWindow* pTabWindow_;
#endif
	ListContainerWindow* pListContainerWindow_;
	FolderListWindow* pFolderListWindow_;
	ListWindow* pListWindow_;
	MessageWindow* pMessageWindow_;
	MainWindowStatusBar* pStatusBar_;
	SyncNotificationWindow* pSyncNotificationWindow_;
	std::auto_ptr<FolderModel> pFolderModel_;
#ifdef QMTABWINDOW
	std::auto_ptr<DefaultTabModel> pTabModel_;
#endif
	std::auto_ptr<FolderListModel> pFolderListModel_;
	std::auto_ptr<ViewModelManager> pViewModelManager_;
	std::auto_ptr<PreviewMessageModel> pPreviewModel_;
	std::auto_ptr<MessageSelectionModelImpl> pMessageSelectionModel_;
	std::auto_ptr<MessageSelectionModelImpl> pListOnlyMessageSelectionModel_;
	std::auto_ptr<EncodingModel> pEncodingModel_;
	std::auto_ptr<DefaultSecurityModel> pSecurityModel_;
	MessageViewModeHolder* pMessageViewModeHolder_;
	std::auto_ptr<MessageFrameWindowManager> pMessageFrameWindowManager_;
	std::auto_ptr<EditFrameWindowManager> pEditFrameWindowManager_;
	std::auto_ptr<AddressBookFrameWindowManager> pAddressBookFrameWindowManager_;
	std::auto_ptr<ActionMap> pActionMap_;
	std::auto_ptr<ActionInvoker> pActionInvoker_;
	std::auto_ptr<FindReplaceManager> pFindReplaceManager_;
	std::auto_ptr<ExternalEditorManager> pExternalEditorManager_;
	std::auto_ptr<MoveMenu> pMoveMenu_;
	std::auto_ptr<FilterMenu> pFilterMenu_;
	std::auto_ptr<SortMenu> pSortMenu_;
	std::auto_ptr<AttachmentMenu> pAttachmentMenu_;
	std::auto_ptr<ViewTemplateMenu> pViewTemplateMenu_;
	std::auto_ptr<CreateTemplateMenu> pCreateTemplateMenu_;
	std::auto_ptr<CreateTemplateMenu> pCreateTemplateExternalMenu_;
	std::auto_ptr<EncodingMenu> pEncodingMenu_;
	std::auto_ptr<SubAccountMenu> pSubAccountMenu_;
	std::auto_ptr<GoRoundMenu> pGoRoundMenu_;
	std::auto_ptr<ScriptMenu> pScriptMenu_;
	std::auto_ptr<RecentsMenu> pRecentsMenu_;
	std::auto_ptr<DelayedFolderModelHandler> pDelayedFolderModelHandler_;
	ToolbarCookie* pToolbarCookie_;
	bool bCreated_;
	int nInitialShow_;
	bool bLayouting_;
	int nShowingModalDialog_;
	HWND hwndLastFocused_;
#ifndef _WIN32_WCE_PSPC
	NOTIFYICONDATA notifyIcon_;
	bool bNotifyIcon_;
#endif
};

void qm::MainWindowImpl::initActions()
{
	pActionMap_.reset(new ActionMap());
	pActionInvoker_.reset(new ActionInvoker(pActionMap_.get()));
	pFindReplaceManager_.reset(new FindReplaceManager());
	
	View* pViews[] = {
		pFolderWindow_,
		pFolderComboBox_,
		pFolderListWindow_,
		pListWindow_,
		pMessageWindow_
	};
	
	ADD_ACTION6(AttachmentOpenAction,
		IDM_ATTACHMENT_OPEN,
		pPreviewModel_.get(),
		pMessageWindow_->getAttachmentSelectionModel(),
		pSecurityModel_.get(),
		pProfile_,
		pTempFileCleaner_,
		pThis_->getHandle());
	ADD_ACTION6(AttachmentSaveAction,
		IDM_ATTACHMENT_SAVE,
		pPreviewModel_.get(),
		pMessageWindow_->getAttachmentSelectionModel(),
		pSecurityModel_.get(),
		false,
		pProfile_,
		pThis_->getHandle());
	ADD_ACTION6(AttachmentSaveAction,
		IDM_ATTACHMENT_SAVEALL,
		pPreviewModel_.get(),
		pMessageWindow_->getAttachmentSelectionModel(),
		pSecurityModel_.get(),
		true,
		pProfile_,
		pThis_->getHandle());
	ADD_ACTION3(ConfigAutoPilotAction,
		IDM_CONFIG_AUTOPILOT,
		pAutoPilot_->getAutoPilotManager(),
		pGoRound_,
		pThis_->getHandle());
	ADD_ACTION4(ConfigColorsAction,
		IDM_CONFIG_COLORS,
		pViewModelManager_->getColorManager(),
		pViewModelManager_.get(),
		pDocument_,
		pThis_->getHandle());
	ADD_ACTION2(ConfigFiltersAction,
		IDM_CONFIG_FILTERS,
		pViewModelManager_->getFilterManager(),
		pThis_->getHandle());
	ADD_ACTION4(ConfigGoRoundAction,
		IDM_CONFIG_GOROUND,
		pGoRound_,
		pDocument_,
		pSyncManager_->getSyncFilterManager(),
		pThis_->getHandle());
	ADD_ACTION3(ConfigRulesAction,
		IDM_CONFIG_RULES,
		pDocument_->getRuleManager(),
		pDocument_,
		pThis_->getHandle());
	ADD_ACTION3(ConfigSignaturesAction,
		IDM_CONFIG_SIGNATURES,
		pDocument_->getSignatureManager(),
		pDocument_,
		pThis_->getHandle());
	ADD_ACTION2(ConfigSyncFiltersAction,
		IDM_CONFIG_SYNCFILTERS,
		pSyncManager_->getSyncFilterManager(),
		pThis_->getHandle());
	ADD_ACTION2(ConfigTextsAction,
		IDM_CONFIG_TEXTS,
		pDocument_->getFixedFormTextManager(),
		pThis_->getHandle());
	ADD_ACTION2(ConfigViewsAction,
		IDM_CONFIG_VIEWS,
		pViewModelManager_.get(),
		pThis_->getHandle());
	ADD_ACTION6(EditClearDeletedAction,
		IDM_EDIT_CLEARDELETED,
		pSyncManager_,
		pDocument_,
		pFolderModel_.get(),
		pSyncDialogManager_,
		pThis_->getHandle(),
		pProfile_);
	
	std::auto_ptr<EditCopyMessageAction> pCopyMessageAction(new EditCopyMessageAction(
		pDocument_, pFolderModel_.get(), pMessageSelectionModel_.get(), pThis_->getHandle()));
	std::auto_ptr<EditCommandAction> pCopyAction(new EditCommandAction(
		pMessageWindow_, &MessageWindowItem::copy, &MessageWindowItem::canCopy));
	Action* pEditCopyActions[] = {
		0,
		0,
		0,
		pCopyMessageAction.get(),
		pCopyAction.get()
	};
	ADD_ACTION3(DispatchAction,
		IDM_EDIT_COPY,
		pViews,
		pEditCopyActions,
		countof(pViews));
	pCopyMessageAction.release();
	pCopyAction.release();
	
	std::auto_ptr<EditCutMessageAction> pCutMessageAction(new EditCutMessageAction(
		pDocument_, pFolderModel_.get(), pMessageSelectionModel_.get(), pThis_->getHandle()));
	Action* pEditCutActions[] = {
		0,
		0,
		0,
		pCutMessageAction.get(),
		0
	};
	ADD_ACTION3(DispatchAction,
		IDM_EDIT_CUT,
		pViews,
		pEditCutActions,
		countof(pViews));
	pCutMessageAction.release();
	
	ADD_ACTION2(EditDeleteCacheAction,
		IDM_EDIT_DELETECACHE,
		pMessageSelectionModel_.get(),
		pThis_->getHandle());
	
	struct {
		UINT nId_;
		bool bDirect_;
	} deletes[] = {
		{ IDM_EDIT_DELETE,			false	},
		{ IDM_EDIT_DELETEDIRECT,	true	}
	};
	for (int n = 0; n < countof(deletes); ++n) {
		std::auto_ptr<EditDeleteMessageAction> pEditDeleteMessageAction1(
			new EditDeleteMessageAction(pMessageSelectionModel_.get(),
				this, 0, deletes[n].bDirect_, true,
				pDocument_->getUndoManager(), pThis_->getHandle(), pProfile_));
		std::auto_ptr<EditDeleteMessageAction> pEditDeleteMessageAction2(
			new EditDeleteMessageAction(pMessageSelectionModel_.get(),
				this, pPreviewModel_.get(), deletes[n].bDirect_, false,
				pDocument_->getUndoManager(), pThis_->getHandle(), pProfile_));
		Action* pEditDeleteMessageActions[] = {
			0,
			0,
			0,
			pEditDeleteMessageAction1.get(),
			pEditDeleteMessageAction2.get()
		};
		ADD_ACTION3(DispatchAction,
			deletes[n].nId_,
			pViews,
			pEditDeleteMessageActions,
			countof(pViews));
		pEditDeleteMessageAction1.release();
		pEditDeleteMessageAction2.release();
	}
	
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
	
	std::auto_ptr<EditPasteMessageAction> pPasteMessageAction(
		new EditPasteMessageAction(pDocument_, pFolderModel_.get(),
			pSyncManager_, pSyncDialogManager_, pProfile_, pThis_->getHandle()));
	Action* pEditPasteActions[] = {
		pPasteMessageAction.get(),
		pPasteMessageAction.get(),
		0,
		pPasteMessageAction.get(),
		0
	};
	ADD_ACTION3(DispatchAction,
		IDM_EDIT_PASTE,
		pViews,
		pEditPasteActions,
		countof(pViews));
	pPasteMessageAction.release();
	
	std::auto_ptr<EditSelectAllMessageAction> pSelectAllMessageAction(
		new EditSelectAllMessageAction(pMessageSelectionModel_.get()));
	std::auto_ptr<EditCommandAction> pSelectAllAction(new EditCommandAction(
		pMessageWindow_, &MessageWindowItem::selectAll, &MessageWindowItem::canSelectAll));
	Action* pEditSelectAllActions[] = {
		0,
		0,
		0,
		pSelectAllMessageAction.get(),
		pSelectAllAction.get()
	};
	ADD_ACTION3(DispatchAction,
		IDM_EDIT_SELECTALL,
		pViews,
		pEditSelectAllActions,
		countof(pViews));
	pSelectAllMessageAction.release();
	pSelectAllAction.release();
	
	ADD_ACTION3(EditUndoMessageAction,
		IDM_EDIT_UNDO,
		pDocument_->getUndoManager(),
		pDocument_,
		pThis_->getHandle());
	ADD_ACTION2(FileCheckAction,
		IDM_FILE_CHECK,
		pFolderModel_.get(),
		pThis_->getHandle());
	ADD_ACTION2(FileCompactAction,
		IDM_FILE_COMPACT,
		pFolderModel_.get(),
		pThis_->getHandle());
	ADD_ACTION2(FileDumpAction,
		IDM_FILE_DUMP,
		pFolderModel_.get(),
		pThis_->getHandle());
	ADD_ACTION9(FileExitAction,
		IDM_FILE_EXIT,
		pThis_->getHandle(),
		pDocument_,
		pSyncManager_,
		pSyncDialogManager_,
		pTempFileCleaner_,
		pEditFrameWindowManager_.get(),
		pAddressBookFrameWindowManager_.get(),
		pFolderModel_.get(),
		pProfile_);
	ADD_ACTION6(FileExportAction,
		IDM_FILE_EXPORT,
		pMessageSelectionModel_.get(),
		pEncodingModel_.get(),
		pSecurityModel_.get(),
		pDocument_,
		pProfile_,
		pThis_->getHandle());
	ADD_ACTION6(FileImportAction,
		IDM_FILE_IMPORT,
		pFolderModel_.get(),
		pDocument_,
		pSyncManager_,
		pSyncDialogManager_,
		pProfile_,
		pThis_->getHandle());
	ADD_ACTION2(FileLoadAction,
		IDM_FILE_LOAD,
		pFolderModel_.get(),
		pThis_->getHandle());
	ADD_ACTION2(FileOfflineAction,
		IDM_FILE_OFFLINE,
		pDocument_,
		pSyncManager_);
	ADD_ACTION7(FilePrintAction,
		IDM_FILE_PRINT,
		pDocument_,
		pMessageSelectionModel_.get(),
		pEncodingModel_.get(),
		pSecurityModel_.get(),
		pThis_->getHandle(),
		pProfile_,
		pTempFileCleaner_);
	ADD_ACTION2(FileSalvageAction,
		IDM_FILE_SALVAGE,
		pFolderModel_.get(),
		pThis_->getHandle());
	ADD_ACTION3(FileSaveAction,
		IDM_FILE_SAVE,
		pDocument_,
		pViewModelManager_.get(),
		pThis_->getHandle());
	ADD_ACTION0(FileUninstallAction,
		IDM_FILE_UNINSTALL);
	ADD_ACTION3(FolderCreateAction,
		IDM_FOLDER_CREATE,
		this,
		pThis_->getHandle(),
		pProfile_);
	ADD_ACTION3(FolderDeleteAction,
		IDM_FOLDER_DELETE,
		pFolderModel_.get(),
		this,
		pThis_->getHandle());
	ADD_ACTION4(FolderEmptyAction,
		IDM_FOLDER_EMPTY,
		this,
		pDocument_->getUndoManager(),
		pThis_->getHandle(),
		pProfile_);
	ADD_ACTION6(FolderEmptyTrashAction,
		IDM_FOLDER_EMPTYTRASH,
		pSyncManager_,
		pDocument_,
		pFolderModel_.get(),
		pSyncDialogManager_,
		pThis_->getHandle(),
		pProfile_);
	ADD_ACTION2(FolderExpandAction,
		IDM_FOLDER_EXPAND,
		pFolderWindow_,
		true);
	ADD_ACTION2(FolderExpandAction,
		IDM_FOLDER_COLLAPSE,
		pFolderWindow_,
		false);
	ADD_ACTION3(FolderPropertyAction,
		IDM_FOLDER_PROPERTY,
		this,
		pThis_->getHandle(),
		pProfile_);
	ADD_ACTION2(FolderRenameAction,
		IDM_FOLDER_RENAME,
		this,
		pThis_->getHandle());
	ADD_ACTION2(FolderUpdateAction,
		IDM_FOLDER_UPDATE,
		pFolderModel_.get(),
		pThis_->getHandle());
	ADD_ACTION1(FolderShowSizeAction,
		IDM_FOLDER_SHOWSIZE,
		pFolderListWindow_);
	ADD_ACTION7(MessageApplyRuleAction,
		IDM_MESSAGE_APPLYRULE,
		pDocument_->getRuleManager(),
		pViewModelManager_.get(),
		false,
		pSecurityModel_.get(),
		pDocument_,
		pThis_->getHandle(),
		pProfile_);
	ADD_ACTION7(MessageApplyRuleAction,
		IDM_MESSAGE_APPLYRULEALL,
		pDocument_->getRuleManager(),
		pViewModelManager_.get(),
		true,
		pSecurityModel_.get(),
		pDocument_,
		pThis_->getHandle(),
		pProfile_);
	ADD_ACTION6(MessageApplyRuleAction,
		IDM_MESSAGE_APPLYRULESELECTED,
		pDocument_->getRuleManager(),
		pMessageSelectionModel_.get(),
		pSecurityModel_.get(),
		pDocument_,
		pThis_->getHandle(),
		pProfile_);
	ADD_ACTION_RANGE11(MessageApplyTemplateAction,
		IDM_MESSAGE_APPLYTEMPLATE,
		IDM_MESSAGE_APPLYTEMPLATE + TemplateMenu::MAX_TEMPLATE,
		pCreateTemplateMenu_.get(),
		pDocument_,
		pFolderModel_.get(),
		pMessageSelectionModel_.get(),
		pEncodingModel_.get(),
		pSecurityModel_.get(),
		pEditFrameWindowManager_.get(),
		pExternalEditorManager_.get(),
		pThis_->getHandle(),
		pProfile_,
		false);
	ADD_ACTION_RANGE11(MessageApplyTemplateAction,
		IDM_MESSAGE_APPLYTEMPLATEEXTERNAL,
		IDM_MESSAGE_APPLYTEMPLATEEXTERNAL + TemplateMenu::MAX_TEMPLATE,
		pCreateTemplateExternalMenu_.get(),
		pDocument_,
		pFolderModel_.get(),
		pMessageSelectionModel_.get(),
		pEncodingModel_.get(),
		pSecurityModel_.get(),
		pEditFrameWindowManager_.get(),
		pExternalEditorManager_.get(),
		pThis_->getHandle(),
		pProfile_,
		true);
	ADD_ACTION1(MessageClearRecentsAction,
		IDM_MESSAGE_CLEARRECENTS,
		pDocument_->getRecents());
	ADD_ACTION4(MessageCombineAction,
		IDM_MESSAGE_COMBINE,
		pMessageSelectionModel_.get(),
		pSecurityModel_.get(),
		pDocument_->getUndoManager(),
		pThis_->getHandle());
	ADD_ACTION4(MessageExpandDigestAction,
		IDM_MESSAGE_EXPANDDIGEST,
		pMessageSelectionModel_.get(),
		pSecurityModel_.get(),
		pDocument_->getUndoManager(),
		pThis_->getHandle());
	
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
			pFolderModel_.get(),
			pMessageSelectionModel_.get(),
			pEncodingModel_.get(),
			pSecurityModel_.get(),
			creates[n].pwszName_,
			pEditFrameWindowManager_.get(),
			pExternalEditorManager_.get(),
			pThis_->getHandle(),
			pProfile_,
			false);
		ADD_ACTION11(MessageCreateAction,
			creates[n].nIdExternal_,
			pDocument_,
			pFolderModel_.get(),
			pMessageSelectionModel_.get(),
			pEncodingModel_.get(),
			pSecurityModel_.get(),
			creates[n].pwszName_,
			pEditFrameWindowManager_.get(),
			pExternalEditorManager_.get(),
			pThis_->getHandle(),
			pProfile_,
			true);
	}
	
	ADD_ACTION7(MessageCreateFromClipboardAction,
		IDM_MESSAGE_CREATEFROMCLIPBOARD,
		false,
		pDocument_,
		pPasswordManager_,
		pProfile_,
		pThis_->getHandle(),
		pFolderModel_.get(),
		pSecurityModel_.get());
	ADD_ACTION7(MessageCreateFromFileAction,
		IDM_MESSAGE_CREATEFROMFILE,
		false,
		pDocument_,
		pPasswordManager_,
		pProfile_,
		pThis_->getHandle(),
		pFolderModel_.get(),
		pSecurityModel_.get());
	ADD_ACTION4(MessageDeleteAttachmentAction,
		IDM_MESSAGE_DELETEATTACHMENT,
		pMessageSelectionModel_.get(),
		pSecurityModel_.get(),
		pDocument_->getUndoManager(),
		pThis_->getHandle());
	ADD_ACTION4(MessageDetachAction,
		IDM_MESSAGE_DETACH,
		pProfile_,
		pMessageSelectionModel_.get(),
		pSecurityModel_.get(),
		pThis_->getHandle());
	ADD_ACTION7(MessageCreateFromClipboardAction,
		IDM_MESSAGE_DRAFTFROMCLIPBOARD,
		true,
		pDocument_,
		pPasswordManager_,
		pProfile_,
		pThis_->getHandle(),
		pFolderModel_.get(),
		pSecurityModel_.get());
	ADD_ACTION7(MessageCreateFromFileAction,
		IDM_MESSAGE_DRAFTFROMFILE,
		true,
		pDocument_,
		pPasswordManager_,
		pProfile_,
		pThis_->getHandle(),
		pFolderModel_.get(),
		pSecurityModel_.get());
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
			pMessageSelectionModel_.get(),
			marks[n].nFlags_,
			marks[n].nMask_,
			pDocument_->getUndoManager(),
			pThis_->getHandle());
	}
	
	struct {
		UINT nId_;
		JunkFilter::Operation operation_;
	} operations[] = {
		{ IDM_MESSAGE_ADDCLEAN,		JunkFilter::OPERATION_ADDCLEAN		},
		{ IDM_MESSAGE_REMOVECLEAN,	JunkFilter::OPERATION_REMOVECLEAN	},
		{ IDM_MESSAGE_ADDJUNK,		JunkFilter::OPERATION_ADDJUNK		},
		{ IDM_MESSAGE_REMOVEJUNK,	JunkFilter::OPERATION_REMOVEJUNK	}
	};
	for (int n = 0; n < countof(operations); ++n) {
		ADD_ACTION4(MessageManageJunkAction,
			operations[n].nId_,
			pMessageSelectionModel_.get(),
			pDocument_->getJunkFilter(),
			operations[n].operation_,
			pThis_->getHandle());
	}
	
	std::auto_ptr<MessageMoveAction> pMessageMoveAction1(new MessageMoveAction(
		pMessageSelectionModel_.get(), this, 0, pMoveMenu_.get(),
		true, pDocument_->getUndoManager(), pThis_->getHandle()));
	std::auto_ptr<MessageMoveAction> pMessageMoveAction2(new MessageMoveAction(
		pMessageSelectionModel_.get(), this, pPreviewModel_.get(), pMoveMenu_.get(),
		false, pDocument_->getUndoManager(), pThis_->getHandle()));
	Action* pMessageMoveActions[] = {
		0,
		0,
		0,
		pMessageMoveAction1.get(),
		pMessageMoveAction2.get()
	};
	ADD_ACTION_RANGE3(DispatchAction,
		IDM_MESSAGE_MOVE,
		IDM_MESSAGE_MOVE + MoveMenu::MAX_FOLDER,
		pViews,
		pMessageMoveActions,
		countof(pViews));
	pMessageMoveAction1.release();
	pMessageMoveAction2.release();
	
	std::auto_ptr<MessageMoveOtherAction> pMessageMoveOtherAction1(
		new MessageMoveOtherAction(pDocument_, pMessageSelectionModel_.get(),
			this, 0, true, pDocument_->getUndoManager(), pProfile_, pThis_->getHandle()));
	std::auto_ptr<MessageMoveOtherAction> pMessageMoveOtherAction2(
		new MessageMoveOtherAction(pDocument_, pMessageSelectionModel_.get(),
			this, pPreviewModel_.get(), false, pDocument_->getUndoManager(),
			pProfile_, pThis_->getHandle()));
	Action* pMessageMoveOtherActions[] = {
		0,
		0,
		0,
		pMessageMoveOtherAction1.get(),
		pMessageMoveOtherAction2.get()
	};
	ADD_ACTION3(DispatchAction,
		IDM_MESSAGE_MOVEOTHER,
		pViews,
		pMessageMoveOtherActions,
		countof(pViews));
	pMessageMoveOtherAction1.release();
	pMessageMoveOtherAction2.release();
	
	ADD_ACTION_RANGE5(MessageOpenRecentAction,
		IDM_MESSAGE_OPENRECENT,
		IDM_MESSAGE_OPENRECENT + RecentsMenu::MAX_RECENTS,
		pDocument_->getRecents(),
		pDocument_,
		pRecentsMenu_.get(),
		pViewModelManager_.get(),
		pMessageFrameWindowManager_.get());
	ADD_ACTION2(MessageOpenLinkAction,
		IDM_MESSAGE_OPENLINK,
		pMessageSelectionModel_.get(),
		pThis_->getHandle());
	ADD_ACTION9(MessageOpenURLAction,
		IDM_MESSAGE_OPENURL,
		pDocument_,
		pFolderModel_.get(),
		pMessageSelectionModel_.get(),
		pSecurityModel_.get(),
		pEditFrameWindowManager_.get(),
		pExternalEditorManager_.get(),
		pThis_->getHandle(),
		pProfile_,
		false);
	ADD_ACTION3(MessagePropertyAction,
		IDM_MESSAGE_PROPERTY,
		pMessageSelectionModel_.get(),
		pDocument_->getUndoManager(),
		pThis_->getHandle());
	ADD_ACTION5(MessageSearchAction,
		IDM_MESSAGE_SEARCH,
		pFolderModel_.get(),
		pSecurityModel_.get(),
		pDocument_,
		pThis_->getHandle(),
		pProfile_);
#ifdef QMTABWINDOW
	ADD_ACTION1(TabCloseAction,
		IDM_TAB_CLOSE,
		pTabModel_.get());
	ADD_ACTION2(TabCreateAction,
		IDM_TAB_CREATE,
		pTabModel_.get(),
		this);
	ADD_ACTION2(TabEditTitleAction,
		IDM_TAB_EDITTITLE,
		pTabModel_.get(),
		pThis_->getHandle());
	ADD_ACTION1(TabLockAction,
		IDM_TAB_LOCK,
		pTabModel_.get());
	ADD_ACTION2(TabMoveAction,
		IDM_TAB_MOVELEFT,
		pTabModel_.get(),
		true);
	ADD_ACTION2(TabMoveAction,
		IDM_TAB_MOVERIGHT,
		pTabModel_.get(),
		false);
	ADD_ACTION2(TabNavigateAction,
		IDM_TAB_NAVIGATENEXT,
		pTabModel_.get(),
		false);
	ADD_ACTION2(TabNavigateAction,
		IDM_TAB_NAVIGATEPREV,
		pTabModel_.get(),
		true);
	ADD_ACTION_RANGE2(TabSelectAction,
		IDM_TAB_SELECT,
		IDM_TAB_SELECT + 10,
		pTabModel_.get(),
		IDM_TAB_SELECT);
#endif
	ADD_ACTION6(ToolAccountAction,
		IDM_TOOL_ACCOUNT,
		pDocument_,
		pFolderModel_.get(),
		pPasswordManager_,
		pSyncManager_,
		pProfile_,
		pThis_->getHandle());
	ADD_ACTION3(ToolAddAddressAction,
		IDM_TOOL_ADDADDRESS,
		pDocument_->getAddressBook(),
		pMessageSelectionModel_.get(),
		pThis_->getHandle());
	ADD_ACTION1(ToolAddressBookAction,
		IDM_TOOL_ADDRESSBOOK,
		pAddressBookFrameWindowManager_.get());
	ADD_ACTION1(ToolAutoPilotAction,
		IDM_TOOL_AUTOPILOT,
		pAutoPilot_);
	ADD_ACTION4(ToolDialupAction,
		IDM_TOOL_DIALUP,
		pSyncManager_,
		pDocument_,
		pSyncDialogManager_,
		pThis_->getHandle());
	ADD_ACTION_RANGE6(ToolGoRoundAction,
		IDM_TOOL_GOROUND,
		IDM_TOOL_GOROUND + GoRoundMenu::MAX_COURSE,
		pSyncManager_,
		pDocument_,
		pGoRound_,
		pSyncDialogManager_,
		pThis_->getHandle(),
		pGoRoundMenu_.get());
	ADD_ACTION2(ToolOptionsAction,
		IDM_TOOL_OPTIONS,
		pProfile_,
		pThis_->getHandle());
	ADD_ACTION_RANGE4(ToolScriptAction,
		IDM_TOOL_SCRIPT,
		IDM_TOOL_SCRIPT + ScriptMenu::MAX_SCRIPT,
		pScriptMenu_.get(),
		pDocument_,
		pProfile_,
		pThis_);
	ADD_ACTION_RANGE5(ToolSubAccountAction,
		IDM_TOOL_SUBACCOUNT,
		IDM_TOOL_SUBACCOUNT + SubAccountMenu::MAX_SUBACCOUNT,
		pDocument_,
		pFolderModel_.get(),
		pSubAccountMenu_.get(),
		pSyncManager_,
		pThis_->getHandle());
	
	struct {
		UINT nId_;
		unsigned int nSync_;
	} syncs[] = {
		{ IDM_TOOL_SYNC,	ToolSyncAction::SYNC_SEND | ToolSyncAction::SYNC_RECEIVE	},
		{ IDM_TOOL_RECEIVE,	ToolSyncAction::SYNC_RECEIVE								},
		{ IDM_TOOL_SEND,	ToolSyncAction::SYNC_SEND									}
	};
	for (int n = 0; n < countof(syncs); ++n) {
		ADD_ACTION6(ToolSyncAction,
			syncs[n].nId_,
			pSyncManager_,
			pDocument_,
			pFolderModel_.get(),
			pSyncDialogManager_,
			syncs[n].nSync_,
			pThis_->getHandle());
	}
	
	ADD_ACTION1(ViewLockPreviewAction,
		IDM_VIEW_LOCKPREVIEW,
		pPreviewModel_.get());
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
	ADD_ACTION1(ViewDropDownAction,
		IDM_VIEW_DROPDOWN,
		pFolderComboBox_);
	ADD_ACTION1(ViewEncodingAction,
		IDM_VIEW_ENCODINGAUTODETECT,
		pEncodingModel_.get());
	ADD_ACTION_RANGE2(ViewEncodingAction,
		IDM_VIEW_ENCODING,
		IDM_VIEW_ENCODING + EncodingMenu::MAX_ENCODING,
		pEncodingModel_.get(),
		pEncodingMenu_.get());
	ADD_ACTION_RANGE2(ViewFilterAction,
		IDM_VIEW_FILTER,
		IDM_VIEW_FILTER + FilterMenu::MAX_FILTER,
		pViewModelManager_.get(),
		pFilterMenu_.get());
	ADD_ACTION2(ViewFilterCustomAction,
		IDM_VIEW_FILTERCUSTOM,
		pViewModelManager_.get(),
		pThis_->getHandle());
	ADD_ACTION1(ViewFilterNoneAction,
		IDM_VIEW_FILTERNONE,
		pViewModelManager_.get());
	ADD_ACTION3(ViewFocusAction,
		IDM_VIEW_FOCUSNEXT,
		pViews,
		countof(pViews),
		true);
	ADD_ACTION3(ViewFocusAction,
		IDM_VIEW_FOCUSPREV,
		pViews,
		countof(pViews),
		false);
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
		ViewNavigateFolderAction::Type type_;
	} navigateFolders[] = {
		{ IDM_VIEW_NEXTACCOUNT,		ViewNavigateFolderAction::TYPE_NEXTACCOUNT	},
		{ IDM_VIEW_PREVACCOUNT,		ViewNavigateFolderAction::TYPE_PREVACCOUNT	},
		{ IDM_VIEW_NEXTFOLDER,		ViewNavigateFolderAction::TYPE_NEXTFOLDER	},
		{ IDM_VIEW_PREVFOLDER,		ViewNavigateFolderAction::TYPE_PREVFOLDER	}
	};
	for (int n = 0; n < countof(navigateFolders); ++n) {
		ADD_ACTION3(ViewNavigateFolderAction,
			navigateFolders[n].nId_,
			pDocument_,
			pFolderModel_.get(),
			navigateFolders[n].type_);
	}
	
	ADD_ACTION1(ViewOpenLinkAction,
		IDM_VIEW_OPENLINK,
		pMessageWindow_);
	
	struct {
		UINT nId_;
		ViewNavigateMessageAction::Type type_;
	} navigateMessages[] = {
		{ IDM_VIEW_NEXTMESSAGE,			ViewNavigateMessageAction::TYPE_NEXT		},
		{ IDM_VIEW_PREVMESSAGE,			ViewNavigateMessageAction::TYPE_PREV		},
		{ IDM_VIEW_NEXTUNSEENMESSAGE,	ViewNavigateMessageAction::TYPE_NEXTUNSEEN	},
		{ IDM_VIEW_NEXTMESSAGEPAGE,		ViewNavigateMessageAction::TYPE_NEXTPAGE	},
		{ IDM_VIEW_PREVMESSAGEPAGE,		ViewNavigateMessageAction::TYPE_PREVPAGE	}
	};
	for (int n = 0; n < countof(navigateMessages); ++n) {
		ADD_ACTION5(ViewNavigateMessageAction,
			navigateMessages[n].nId_,
			pViewModelManager_.get(),
			pFolderModel_.get(),
			pThis_,
			pMessageWindow_,
			navigateMessages[n].type_);
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
	ADD_ACTION7(ViewRefreshAction,
		IDM_VIEW_REFRESH,
		pSyncManager_,
		pDocument_,
		pFolderModel_.get(),
		pSecurityModel_.get(),
		pSyncDialogManager_,
		pThis_->getHandle(),
		pProfile_);
	
	struct {
		UINT nId_;
		ViewScrollAction::Scroll scroll_;
	} scrolls[] = {
		{ IDM_VIEW_SCROLLLINEUP,		ViewScrollAction::SCROLL_LINEUP		},
		{ IDM_VIEW_SCROLLLINEDOWN,		ViewScrollAction::SCROLL_LINEDOWN	},
		{ IDM_VIEW_SCROLLPAGEUP,		ViewScrollAction::SCROLL_PAGEUP		},
		{ IDM_VIEW_SCROLLPAGEDOWN,		ViewScrollAction::SCROLL_PAGEDOWN	},
		{ IDM_VIEW_SCROLLTOP,			ViewScrollAction::SCROLL_TOP		},
		{ IDM_VIEW_SCROLLBOTTOM,		ViewScrollAction::SCROLL_BOTTOM		},
	};
	for (int n = 0; n < countof(scrolls); ++n) {
		ADD_ACTION2(ViewScrollAction,
			scrolls[n].nId_,
			pListWindow_->getHandle(),
			scrolls[n].scroll_);
	}
	
	ADD_ACTION3(ViewSelectMessageAction,
		IDM_VIEW_SELECTMESSAGE,
		pViewModelManager_.get(),
		pFolderModel_.get(),
		pMessageSelectionModel_.get());
	ADD_ACTION3(ViewMessageModeAction,
		IDM_VIEW_SELECTMODE,
		pMessageViewModeHolder_,
		MessageViewMode::MODE_SELECT,
		true);
	ADD_ACTION1(ViewShowFolderAction,
		IDM_VIEW_SHOWFOLDER,
		pThis_);
	ADD_ACTION1(ViewShowHeaderAction,
		IDM_VIEW_SHOWHEADER,
		pMessageWindow_);
	ADD_ACTION1(ViewShowHeaderColumnAction,
		IDM_VIEW_SHOWHEADERCOLUMN,
		pListWindow_);
	ADD_ACTION1(ViewShowPreviewAction,
		IDM_VIEW_SHOWPREVIEW,
		pThis_);
	ADD_ACTION1(ViewShowStatusBarAction<MainWindow>,
		IDM_VIEW_SHOWSTATUSBAR,
		pThis_);
	ADD_ACTION1(ViewShowSyncDialogAction,
		IDM_VIEW_SHOWSYNCDIALOG,
		pSyncDialogManager_);
#ifdef QMTABWINDOW
	ADD_ACTION1(ViewShowTabAction,
		IDM_VIEW_SHOWTAB,
		pTabWindow_);
#endif
	ADD_ACTION1(ViewShowToolbarAction<MainWindow>,
		IDM_VIEW_SHOWTOOLBAR,
		pThis_);
	ADD_ACTION_RANGE2(ViewSortAction,
		IDM_VIEW_SORT,
		IDM_VIEW_SORT + SortMenu::MAX_SORT,
		pViewModelManager_.get(),
		pSortMenu_.get());
	ADD_ACTION2(ViewSortDirectionAction,
		IDM_VIEW_SORTASCENDING,
		pViewModelManager_.get(),
		true);
	ADD_ACTION2(ViewSortDirectionAction,
		IDM_VIEW_SORTDESCENDING,
		pViewModelManager_.get(),
		false);
	ADD_ACTION1(ViewSortFloatThreadAction,
		IDM_VIEW_SORTFLOATTHREAD,
		pViewModelManager_.get());
	ADD_ACTION1(ViewSortThreadAction,
		IDM_VIEW_SORTTHREAD,
		pViewModelManager_.get());
	ADD_ACTION1(ViewTemplateAction,
		IDM_VIEW_TEMPLATENONE,
		pMessageWindow_);
	ADD_ACTION_RANGE2(ViewTemplateAction,
		IDM_VIEW_TEMPLATE,
		IDM_VIEW_TEMPLATE + TemplateMenu::MAX_TEMPLATE,
		pMessageWindow_,
		pViewTemplateMenu_.get());
}

void qm::MainWindowImpl::layoutChildren()
{
	RECT rect;
	pThis_->getClientRect(&rect);
	layoutChildren(rect.right - rect.left, rect.bottom - rect.top);
}

void qm::MainWindowImpl::layoutChildren(int cx,
										int cy)
{
	bLayouting_ = true;
	
	HWND hwndToolbar = pThis_->getToolbar();
	RECT rectToolbar;
	Window wndToolbar(hwndToolbar);
	wndToolbar.getWindowRect(&rectToolbar);
	int nToolbarHeight = rectToolbar.bottom - rectToolbar.top;
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
	
	int nFolderComboBoxHeight = 0;
	if (bShowFolderComboBox_) {
		RECT rectFolderComboBox;
		pFolderComboBox_->getWindowRect(&rectFolderComboBox);
		nFolderComboBoxHeight =
			rectFolderComboBox.bottom - rectFolderComboBox.top;
	}
	
	HDWP hdwp = Window::beginDeferWindowPos(5);
	
#if _WIN32_WCE >= 300 && defined _WIN32_WCE_PSPC
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
	
	hdwp = pSyncNotificationWindow_->deferWindowPos(hdwp, HWND_TOP,
		cx - SyncNotificationWindow::WIDTH, 0/*nTopBarHeight*/, 0, 0, SWP_NOSIZE);
	
	hdwp = pFolderSplitterWindow_->deferWindowPos(hdwp, 0,
		0, nTopBarHeight + nFolderComboBoxHeight, cx,
		cy - nStatusBarHeight - nTopBarHeight - nFolderComboBoxHeight - nBottomBarHeight,
		SWP_NOZORDER);
	pFolderSplitterWindow_->showPane(0, 0, bShowFolderWindow_);
	if (bVirticalFolderWindow_)
		pFolderSplitterWindow_->setRowHeight(0, nFolderWindowSize_);
	else
		pFolderSplitterWindow_->setColumnWidth(0, nFolderWindowSize_);
	
	if (bShowFolderComboBox_) {
		hdwp = pFolderComboBox_->deferWindowPos(hdwp, 0, 0, nTopBarHeight,
			cx, /*nFolderComboBoxHeight*/200, SWP_NOZORDER);
		pFolderComboBox_->showWindow(SW_SHOW);
	}
	else {
		pFolderComboBox_->showWindow(SW_HIDE);
	}
	
	Window::endDeferWindowPos(hdwp);
	
	pListSplitterWindow_->showPane(0, 1, bShowPreviewWindow_);
	pListSplitterWindow_->setRowHeight(0, nListWindowHeight_);
	
	if (bShowStatusBar_) {
		if (bShowPreviewWindow_) {
#ifdef _WIN32_WCE_PSPC
			int nWidth[] = {
				cx - 60,
				cx - 40,
				cx - 20,
				-1
			};
#elif defined _WIN32_WCE
			int nWidth[] = {
				cx - 300,
				cx - 280,
				cx - 200,
				cx - 120,
				cx - 40,
				cx - 20,
				-1
			};
#else
			int nWidth[] = {
				cx - 342,
				cx - 318,
				cx - 238,
				cx - 158,
				cx - 78,
				cx - 54,
				cx - 30,
				-1
			};
#endif
			pStatusBar_->setParts(nWidth, countof(nWidth));
		}
		else {
#ifdef _WIN32_WCE_PSPC
			int nWidth[] = {
				cx - 20,
				-1
			};
#elif defined _WIN32_WCE
			int nWidth[] = {
				cx - 100,
				cx - 80,
				-1,
			};
#else
			int nWidth[] = {
				cx - 134,
				cx - 110,
				cx - 30,
				-1
			};
#endif
			pStatusBar_->setParts(nWidth, countof(nWidth));
		}
	}
	
	bLayouting_ = false;
}

void qm::MainWindowImpl::updateStatusBar()
{
	assert(::GetCurrentThreadId() == ::GetWindowThreadProcessId(pThis_->getHandle(), 0));
	
	if (bShowStatusBar_)
		pStatusBar_->updateListParts(0);
}

#ifndef _WIN32_WCE_PSPC
void qm::MainWindowImpl::showRecentsMenu()
{
	AutoMenuHandle hmenu(::CreatePopupMenu());
	if (pRecentsMenu_->createMenu(hmenu.get())) {
		UINT nFlags = TPM_LEFTALIGN | TPM_TOPALIGN;
#ifndef _WIN32_WCE
		nFlags |= TPM_LEFTBUTTON | TPM_RIGHTBUTTON;
#endif
		POINT pt;
#ifdef _WIN32_WCE
		DWORD dwPos = ::GetMessagePos();
		pt.x = static_cast<int>(dwPos & 0x0000ffff);
		pt.y = static_cast<int>(dwPos & 0xffff0000) >> 16;
#else
		::GetCursorPos(&pt);
#endif
		pThis_->setForegroundWindow();
		::TrackPopupMenu(hmenu.get(), nFlags, pt.x, pt.y, 0, pThis_->getHandle(), 0);
		pThis_->postMessage(WM_NULL);
	}
}
#endif

void qm::MainWindowImpl::preModalDialog(HWND hwndParent)
{
	if (nShowingModalDialog_++ == 0) {
		if (hwndParent != pThis_->getHandle())
			pThis_->enableWindow(false);
		
		pMessageFrameWindowManager_->preModalDialog(hwndParent);
		pEditFrameWindowManager_->preModalDialog(hwndParent);
	}
}

void qm::MainWindowImpl::postModalDialog(HWND hwndParent)
{
	if (--nShowingModalDialog_ == 0) {
		if (hwndParent != pThis_->getHandle())
			pThis_->enableWindow(true);
		
		pMessageFrameWindowManager_->postModalDialog(hwndParent);
		pEditFrameWindowManager_->postModalDialog(hwndParent);
	}
}

void qm::MainWindowImpl::sizeChanged(const SplitterWindowEvent& event)
{
	if (bCreated_ && !bLayouting_) {
		SplitterWindow* pSplitterWindow = event.getSplitterWindow();
		if (pSplitterWindow == pFolderSplitterWindow_) {
			if (bVirticalFolderWindow_)
				nFolderWindowSize_ = pSplitterWindow->getRowHeight(0);
			else
				nFolderWindowSize_ = pSplitterWindow->getColumnWidth(0);
		}
		else if (pSplitterWindow == pListSplitterWindow_) {
			nListWindowHeight_ = pSplitterWindow->getRowHeight(0);
		}
		else {
			assert(false);
		}
	}
}

void qm::MainWindowImpl::accountSelected(const FolderModelEvent& event)
{
	pFolderListModel_->setAccount(event.getAccount());
	pViewModelManager_->setCurrentAccount(event.getAccount());
}

void qm::MainWindowImpl::folderSelected(const FolderModelEvent& event)
{
	assert(event.getFolder() == pFolderModel_->getCurrent().second);
	
	Folder* pFolder = event.getFolder();
	
	pFolderListModel_->setAccount(0);
	pViewModelManager_->setCurrentFolder(pFolder);
	
	if (!pDocument_->isOffline()) {
		if (pFolder->getType() == Folder::TYPE_NORMAL &&
			pFolder->isFlag(Folder::FLAG_SYNCABLE) &&
			pFolder->isFlag(Folder::FLAG_SYNCWHENOPEN)) {
			SyncUtil::syncFolder(pSyncManager_, pDocument_, pSyncDialogManager_,
				pThis_->getHandle(), SyncDialog::FLAG_NONE,
				static_cast<NormalFolder*>(pFolder), 0);
		}
	}
	
	if (pFolder->getType() == Folder::TYPE_QUERY &&
		pFolder->isFlag(Folder::FLAG_SYNCWHENOPEN))
		static_cast<QueryFolder*>(pFolder)->search(pDocument_,
			pThis_->getHandle(), pProfile_, pSecurityModel_->getSecurityMode());
}

Account* qm::MainWindowImpl::getAccount()
{
	if (pFolderListWindow_->isActive())
		return pFolderListModel_->getFocusedFolder() ?
			0 : pFolderListModel_->getAccount();
	else
		return pFolderModel_->getCurrent().first;
}

void qm::MainWindowImpl::getSelectedFolders(Account::FolderList* pList)
{
	assert(pList);
	
	if (pFolderListWindow_->isActive()) {
		pFolderListModel_->getSelectedFolders(pList);
	}
	else {
		Folder* pFolder = pFolderModel_->getCurrent().second;
		if (pFolder)
			pList->push_back(pFolder);
	}
}

bool qm::MainWindowImpl::hasSelectedFolder()
{
	if (pFolderListWindow_->isActive())
		return pFolderListModel_->hasSelectedFolder();
	else
		return pFolderModel_->getCurrent().second != 0;
}

Folder* qm::MainWindowImpl::getFocusedFolder()
{
	if (pFolderListWindow_->isActive())
		return pFolderListModel_->getFocusedFolder();
	else
		return pFolderModel_->getCurrent().second;
}

std::pair<Account*, Folder*> qm::MainWindowImpl::getTemporaryFocused()
{
	if (pFolderListWindow_->isActive())
		return std::pair<Account*, Folder*>(0, 0);
	else
		return pFolderModel_->getTemporary();
}

ViewModel* qm::MainWindowImpl::getViewModel() const
{
	return pViewModelManager_->getCurrentViewModel();
}

void qm::MainWindowImpl::setViewModel(ViewModel* pViewModel)
{
	assert(false);
}

void qm::MainWindowImpl::messageChanged(const MessageWindowEvent& event)
{
	if (bShowStatusBar_) {
		pStatusBar_->updateListParts(L"");
		if (bShowPreviewWindow_)
			pStatusBar_->updateMessageParts(event.getMessageHolder(),
				event.getMessage(), event.getContentType());
	}
}

void qm::MainWindowImpl::statusTextChanged(const MessageWindowStatusTextEvent& event)
{
	if (bShowStatusBar_)
		pStatusBar_->updateListParts(event.getText());
}

void qm::MainWindowImpl::accountListChanged(const AccountManagerEvent& event)
{
	pFolderModel_->setCurrent(0, 0, false);
	pFolderListModel_->setAccount(0);
	pViewModelManager_->setCurrentAccount(0);
}

void qm::MainWindowImpl::recentsChanged(const RecentsEvent& event)
{
#ifndef _WIN32_WCE_PSPC
	pThis_->postMessage(WM_MAINWINDOW_RECENTSCHANGED);
#endif
}

#ifdef QMTABWINDOW
void qm::MainWindowImpl::viewModelSelected(const ViewModelManagerEvent& event)
{
	ViewModel* pViewModel = event.getNewViewModel();
	if (pViewModel) {
		pTabModel_->setFolder(pViewModel->getFolder());
	}
	else {
		Account* pAccount = pViewModelManager_->getCurrentAccount();
		if (pAccount)
			pTabModel_->setAccount(pAccount);
	}
}

void qm::MainWindowImpl::currentChanged(const TabModelEvent& event)
{
	std::pair<Account*, Folder*> p;
	int nItem = pTabModel_->getCurrent();
	if (nItem != -1) {
		const TabItem* pItem = pTabModel_->getItem(nItem);
		p = pItem->get();
	}
	pFolderModel_->setCurrent(p.first, p.second, false);
}
#endif // QMTABWINDOW


/****************************************************************************
 *
 * MainWindowImpl::MessageSelectionModelImpl
 *
 */

qm::MainWindowImpl::MessageSelectionModelImpl::MessageSelectionModelImpl(MainWindowImpl* pMainWindowImpl,
																		 bool bListOnly) :
	pMainWindowImpl_(pMainWindowImpl),
	bListOnly_(bListOnly)
{
}

qm::MainWindowImpl::MessageSelectionModelImpl::~MessageSelectionModelImpl()
{
}

void qm::MainWindowImpl::MessageSelectionModelImpl::getSelectedMessages(AccountLock* pAccountLock,
																		Folder** ppFolder,
																		MessageHolderList* pList)
{
	assert(pAccountLock);
	assert(pList);
	
	if (ppFolder)
		*ppFolder = 0;
	
	if (pMainWindowImpl_->pListWindow_->isActive()) {
		ViewModel* pViewModel = pMainWindowImpl_->pViewModelManager_->getCurrentViewModel();
		if (pViewModel) {
			Lock<ViewModel> lock(*pViewModel);
			pAccountLock->set(pViewModel->getFolder()->getAccount());
			if (ppFolder)
				*ppFolder = pViewModel->getFolder();
			pViewModel->getSelection(pList);
		}
	}
	else if (pMainWindowImpl_->pMessageWindow_->isActive() && !bListOnly_) {
		MessagePtrLock mpl(pMainWindowImpl_->pPreviewModel_->getCurrentMessage());
		if (mpl) {
			pAccountLock->set(mpl->getAccount());
			if (ppFolder)
				*ppFolder = mpl->getFolder();
			pList->push_back(mpl);
		}
	}
}

bool qm::MainWindowImpl::MessageSelectionModelImpl::hasSelectedMessage()
{
	if (pMainWindowImpl_->pListWindow_->isActive()) {
		ViewModel* pViewModel = pMainWindowImpl_->pViewModelManager_->getCurrentViewModel();
		if (pViewModel) {
			Lock<ViewModel> lock(*pViewModel);
			return pViewModel->hasSelection();
		}
	}
	else if (pMainWindowImpl_->pMessageWindow_->isActive() && !bListOnly_) {
		MessagePtrLock mpl(pMainWindowImpl_->pPreviewModel_->getCurrentMessage());
		return mpl != 0;
	}
	return false;
}

MessagePtr qm::MainWindowImpl::MessageSelectionModelImpl::getFocusedMessage()
{
	if (pMainWindowImpl_->pListWindow_->isActive()) {
		ViewModel* pViewModel = pMainWindowImpl_->pViewModelManager_->getCurrentViewModel();
		if (pViewModel) {
			Lock<ViewModel> lock(*pViewModel);
			if (pViewModel->getCount() != 0) {
				unsigned int nItem = pViewModel->getFocused();
				return MessagePtr(pViewModel->getMessageHolder(nItem));
			}
		}
	}
	else if (pMainWindowImpl_->pMessageWindow_->isActive() && !bListOnly_) {
		return pMainWindowImpl_->pPreviewModel_->getCurrentMessage();
	}
	
	return MessagePtr();
}

bool qm::MainWindowImpl::MessageSelectionModelImpl::hasFocusedMessage()
{
	if (pMainWindowImpl_->pListWindow_->isActive()) {
		ViewModel* pViewModel = pMainWindowImpl_->pViewModelManager_->getCurrentViewModel();
		if (pViewModel) {
			Lock<ViewModel> lock(*pViewModel);
			return pViewModel->getCount() != 0;
		}
	}
	else if (pMainWindowImpl_->pMessageWindow_->isActive() && !bListOnly_) {
		MessagePtrLock mpl(pMainWindowImpl_->pPreviewModel_->getCurrentMessage());
		return mpl != 0;
	}
	return false;
}

void qm::MainWindowImpl::MessageSelectionModelImpl::selectAll()
{
	if (!pMainWindowImpl_->pListWindow_->isActive())
		return;
	
	ViewModel* pViewModel = pMainWindowImpl_->pViewModelManager_->getCurrentViewModel();
	if (!pViewModel)
		return;
	
	Lock<ViewModel> lock(*pViewModel);
	unsigned int nCount = pViewModel->getCount();
	if (nCount != 0)
		pViewModel->setSelection(0,  nCount - 1);
}

bool qm::MainWindowImpl::MessageSelectionModelImpl::canSelect()
{
	return pMainWindowImpl_->pListWindow_->isActive() &&
		pMainWindowImpl_->pViewModelManager_->getCurrentViewModel();
}


/****************************************************************************
 *
 * MainWindow
 *
 */

qm::MainWindow::MainWindow(Profile* pProfile) :
	FrameWindow(Application::getApplication().getResourceHandle(), true),
	pImpl_(0)
{
	assert(pProfile);
	
	pImpl_ = new MainWindowImpl();
	pImpl_->pThis_ = this;
	pImpl_->bShowToolbar_ = pProfile->getInt(L"MainWindow", L"ShowToolbar", 1) != 0;
	pImpl_->bShowStatusBar_ = pProfile->getInt(L"MainWindow", L"ShowStatusBar", 1) != 0;
	pImpl_->bShowFolderWindow_ = pProfile->getInt(L"MainWindow", L"ShowFolderWindow", 1) != 0;
	pImpl_->nFolderWindowSize_ = pProfile->getInt(L"MainWindow", L"FolderWindowSize", 100);
	pImpl_->bShowFolderComboBox_ = pProfile->getInt(L"MainWindow", L"ShowFolderComboBox", 0) != 0;
	pImpl_->bVirticalFolderWindow_ = pProfile->getInt(L"MainWindow", L"VirticalFolderWindow", 0) != 0;
	pImpl_->bShowPreviewWindow_ = pProfile->getInt(L"MainWindow", L"ShowPreviewWindow", 1) != 0;
	pImpl_->nListWindowHeight_ = pProfile->getInt(L"MainWindow", L"ListWindowHeight", 200);
	pImpl_->bSaveOnDeactivate_ = pProfile->getInt(L"MainWindow", L"SaveOnDeactivate", 0) != 0;
	pImpl_->pProfile_ = pProfile;
	pImpl_->pDocument_ = 0;
	pImpl_->pUIManager_ = 0;
	pImpl_->pPasswordManager_ = 0;
	pImpl_->pSyncManager_ = 0;
	pImpl_->pSyncDialogManager_ = 0;
	pImpl_->pGoRound_ = 0;
	pImpl_->pTempFileCleaner_ = 0;
	pImpl_->pAutoPilot_ = 0;
	pImpl_->pFolderSplitterWindow_ = 0;
	pImpl_->pListSplitterWindow_ = 0;
	pImpl_->pFolderWindow_ = 0;
	pImpl_->pFolderComboBox_ = 0;
#ifdef QMTABWINDOW
	pImpl_->pTabWindow_ = 0;
#endif
	pImpl_->pListContainerWindow_ = 0;
	pImpl_->pFolderListWindow_ = 0;
	pImpl_->pListWindow_ = 0;
	pImpl_->pMessageWindow_ = 0;
	pImpl_->pStatusBar_ = 0;
	pImpl_->pSyncNotificationWindow_ = 0;
	pImpl_->pMessageViewModeHolder_ = 0;
	pImpl_->bCreated_ = false;
	pImpl_->pToolbarCookie_ = 0;
	pImpl_->nInitialShow_ = SW_SHOWNORMAL;
	pImpl_->bLayouting_ = false;
	pImpl_->nShowingModalDialog_ = 0;
	pImpl_->hwndLastFocused_ = 0;
#ifndef _WIN32_WCE_PSPC
	pImpl_->bNotifyIcon_ = false;
#endif
	
	setModalHandler(pImpl_);
}

qm::MainWindow::~MainWindow()
{
	delete pImpl_;
}

FolderModel* qm::MainWindow::getFolderModel() const
{
	return pImpl_->pFolderModel_.get();
}

const ActionInvoker* qm::MainWindow::getActionInvoker() const
{
	return pImpl_->pActionInvoker_.get();
}

bool qm::MainWindow::isShowingModalDialog() const
{
	return pImpl_->nShowingModalDialog_ != 0;
}

void qm::MainWindow::initialShow()
{
	showWindow(pImpl_->nInitialShow_);
}

bool qm::MainWindow::save()
{
	if (!pImpl_->pMessageFrameWindowManager_->save() ||
		!pImpl_->pViewModelManager_->save() ||
		!pImpl_->pListWindow_->save() ||
		!pImpl_->pMessageWindow_->save() ||
		!pImpl_->pFolderWindow_->save() ||
		!pImpl_->pFolderListWindow_->save())
		return false;
#ifdef QMTABWINDOW
	if (!pImpl_->pTabModel_->save() ||
		!pImpl_->pTabWindow_->save())
		return false;
#endif
	
	Profile* pProfile = pImpl_->pProfile_;
	pProfile->setInt(L"MainWindow", L"ShowToolbar", pImpl_->bShowToolbar_);
	pProfile->setInt(L"MainWindow", L"ShowStatusBar", pImpl_->bShowStatusBar_);
	
	if (pImpl_->bShowFolderWindow_) {
		if (pImpl_->bVirticalFolderWindow_)
			pImpl_->nFolderWindowSize_ = pImpl_->pFolderSplitterWindow_->getRowHeight(0);
		else
			pImpl_->nFolderWindowSize_ = pImpl_->pFolderSplitterWindow_->getColumnWidth(0);
	}
	pProfile->setInt(L"MainWindow", L"FolderWindowSize", pImpl_->nFolderWindowSize_);
	pProfile->setInt(L"MainWindow", L"ShowFolderWindow", pImpl_->bShowFolderWindow_);
	pProfile->setInt(L"MainWindow", L"ShowFolderComboBox", pImpl_->bShowFolderComboBox_);
	
	if (pImpl_->bShowPreviewWindow_)
		pImpl_->nListWindowHeight_ = pImpl_->pListSplitterWindow_->getRowHeight(0);
	pProfile->setInt(L"MainWindow", L"ListWindowHeight", pImpl_->nListWindowHeight_);
	pProfile->setInt(L"MainWindow", L"ShowPreviewWindow", pImpl_->bShowPreviewWindow_);
	
	UIUtil::saveWindowPlacement(getHandle(), pProfile, L"MainWindow");
	
	pProfile->setInt(L"MainWindow", L"SecurityMode", pImpl_->pSecurityModel_->getSecurityMode());
	
	if (!FrameWindow::save())
		return false;
	
	return true;
}

bool qm::MainWindow::isShowToolbar() const
{
	return pImpl_->bShowToolbar_;
}

void qm::MainWindow::setShowToolbar(bool bShow)
{
	if (bShow != pImpl_->bShowToolbar_) {
		pImpl_->bShowToolbar_ = bShow;
		pImpl_->layoutChildren();
	}
}

bool qm::MainWindow::isShowStatusBar() const
{
	return pImpl_->bShowStatusBar_;
}

void qm::MainWindow::setShowStatusBar(bool bShow)
{
	if (bShow != pImpl_->bShowStatusBar_) {
		pImpl_->bShowStatusBar_ = bShow;
		pImpl_->layoutChildren();
	}
}

bool qm::MainWindow::isShowFolderWindow() const
{
	return pImpl_->bShowFolderWindow_;
}

void qm::MainWindow::setShowFolderWindow(bool bShow)
{
	if (bShow != pImpl_->bShowFolderWindow_) {
		if (!bShow) {
			if (pImpl_->bVirticalFolderWindow_)
				pImpl_->nFolderWindowSize_ =
					pImpl_->pFolderSplitterWindow_->getRowHeight(0);
			else
				pImpl_->nFolderWindowSize_ =
					pImpl_->pFolderSplitterWindow_->getColumnWidth(0);
		}
		pImpl_->bShowFolderWindow_ = bShow;
		pImpl_->bShowFolderComboBox_ = !bShow;
		pImpl_->layoutChildren();
	}
}

bool qm::MainWindow::isShowPreviewWindow() const
{
	return pImpl_->bShowPreviewWindow_;
}

void qm::MainWindow::setShowPreviewWindow(bool bShow)
{
	if (bShow != pImpl_->bShowPreviewWindow_) {
		if (!bShow)
			pImpl_->nListWindowHeight_ =
				pImpl_->pListSplitterWindow_->getRowHeight(0);
		pImpl_->bShowPreviewWindow_ = bShow;
		pImpl_->layoutChildren();
		
		if (bShow) {
			pImpl_->pPreviewModel_->connectToViewModel();
		}
		else {
			pImpl_->pPreviewModel_->disconnectFromViewModel();
			pImpl_->pPreviewModel_->setMessage(0);
		}
	}
}

void qm::MainWindow::processIdle()
{
	FrameWindow::processIdle();
	pImpl_->updateStatusBar();
}

bool qm::MainWindow::getToolbarButtons(Toolbar* pToolbar)
{
	pToolbar->nId_ = MainWindowImpl::ID_TOOLBAR;
	return true;
}

bool qm::MainWindow::createToolbarButtons(void* pCreateParam,
										  HWND hwndToolbar)
{
	MainWindowCreateContext* pContext =
		static_cast<MainWindowCreateContext*>(pCreateParam);
	UIManager* pUIManager = pContext->pUIManager_;
	pImpl_->pToolbarCookie_ = pUIManager->getToolbarManager()->createButtons(
		L"mainframe", hwndToolbar, this);
	return pImpl_->pToolbarCookie_ != 0;
}

#ifdef _WIN32_WCE
UINT qm::MainWindow::getBarId(int n) const
{
	assert(n == 0 || n == 1);
	
	static UINT nIds[] = {
		MainWindowImpl::ID_COMMANDBARMENU,
		MainWindowImpl::ID_COMMANDBARBUTTON
	};
	return nIds[n];
}

bool qm::MainWindow::getCommandBandsRestoreInfo(int n,
												COMMANDBANDSRESTOREINFO* pcbri) const
{
	WCHAR wszKey[32];
	swprintf(wszKey, L"CommandBandsRestoreInfo%d", n);
	size_t nSize = pImpl_->pProfile_->getBinary(L"MainWindow", wszKey,
		reinterpret_cast<unsigned char*>(pcbri), sizeof(*pcbri));
	if (nSize != sizeof(*pcbri))
		pcbri->cbSize = 0;
	return true;
}

bool qm::MainWindow::setCommandBandsRestoreInfo(int n,
												const COMMANDBANDSRESTOREINFO& cbri)
{
	WCHAR wszKey[32];
	swprintf(wszKey, L"CommandBandsRestoreInfo%d", n);
	pImpl_->pProfile_->setBinary(L"MainWindow", wszKey,
		reinterpret_cast<const unsigned char*>(&cbri), sizeof(cbri));
	return true;
}
#endif

HMENU qm::MainWindow::getMenuHandle(void* pCreateParam)
{
	MainWindowCreateContext* pContext =
		static_cast<MainWindowCreateContext*>(pCreateParam);
	UIManager* pUIManager = pContext->pUIManager_;
	return pUIManager->getMenuManager()->getMenu(L"mainframe", true, true);
}

UINT qm::MainWindow::getIconId()
{
	return IDI_MAINFRAME;
}

void qm::MainWindow::getWindowClass(WNDCLASS* pwc)
{
	HINSTANCE hInst = Application::getApplication().getResourceHandle();
	FrameWindow::getWindowClass(pwc);
	pwc->hIcon = ::LoadIcon(hInst, MAKEINTRESOURCE(IDI_MAINFRAME));
	pwc->hbrBackground = reinterpret_cast<HBRUSH>(COLOR_BTNFACE + 1);
}

bool qm::MainWindow::preCreateWindow(CREATESTRUCT* pCreateStruct)
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
		pImpl_->pProfile_, L"MainWindow", pCreateStruct);
#endif
	
	return true;
}

Action* qm::MainWindow::getAction(UINT nId)
{
	return pImpl_->pActionMap_->getAction(nId);
}

Accelerator* qm::MainWindow::getAccelerator()
{
	return pImpl_->pAccelerator_.get();
}

LRESULT qm::MainWindow::windowProc(UINT uMsg,
								   WPARAM wParam,
								   LPARAM lParam)
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
#ifndef _WIN32_WCE_PSPC
		HANDLE_HOTKEY()
#endif
		HANDLE_INITMENUPOPUP()
#ifndef _WIN32_WCE
		HANDLE_QUERYENDSESSION()
#endif
		HANDLE_SIZE()
#ifndef _WIN32_WCE_PSPC
		HANDLE_MESSAGE(MainWindowImpl::WM_MAINWINDOW_NOTIFYICON, onNotifyIcon)
		HANDLE_MESSAGE(MainWindowImpl::WM_MAINWINDOW_RECENTSCHANGED, onRecentsChanged)
#endif
	END_MESSAGE_HANDLER()
	return FrameWindow::windowProc(uMsg, wParam, lParam);
}

LRESULT qm::MainWindow::onActivate(UINT nFlags,
								   HWND hwnd,
								   bool bMinimized)
{
	FrameWindow::onActivate(nFlags, hwnd, bMinimized);
	
	// TODO
	// Handle other thread
	
	if (nFlags == WA_INACTIVE) {
		pImpl_->hwndLastFocused_ = ::GetFocus();
		
		if (pImpl_->bSaveOnDeactivate_) {
			bool bSave = true;
			if (hwnd) {
				DWORD dwId = 0;
				::GetWindowThreadProcessId(hwnd, &dwId);
				bSave = dwId != ::GetCurrentProcessId();
			}
			if (bSave)
				Application::getApplication().save();
		}
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
	pAction->exit(true);
	return 0;
}

LRESULT qm::MainWindow::onCopyData(HWND hwnd,
								   COPYDATASTRUCT* pData)
{
	if (pData->lpData) {
		Variant v(::SysAllocString(static_cast<WCHAR*>(pData->lpData)));
		if (v.bstrVal) {
			VARIANT* pvars[] = { &v };
			pImpl_->pActionInvoker_->invoke(pData->dwData, pvars, countof(pvars));
		}
	}
	else {
		pImpl_->pActionInvoker_->invoke(pData->dwData, 0, 0);
	}
	return 1;
}

LRESULT qm::MainWindow::onCreate(CREATESTRUCT* pCreateStruct)
{
	if (FrameWindow::onCreate(pCreateStruct) == -1)
		return -1;
	
	MainWindowCreateContext* pContext =
		static_cast<MainWindowCreateContext*>(pCreateStruct->lpCreateParams);
	pImpl_->pDocument_ = pContext->pDocument_;
	pImpl_->pUIManager_ = pContext->pUIManager_;
	pImpl_->pPasswordManager_ = pContext->pPasswordManager_;
	pImpl_->pSyncManager_ = pContext->pSyncManager_;
	pImpl_->pSyncDialogManager_ = pContext->pSyncDialogManager_;
	pImpl_->pGoRound_ = pContext->pGoRound_;
	pImpl_->pTempFileCleaner_ = pContext->pTempFileCleaner_;
	pImpl_->pAutoPilot_ = pContext->pAutoPilot_;
	
	CustomAcceleratorFactory acceleratorFactory;
	pImpl_->pAccelerator_ = pImpl_->pUIManager_->getKeyMap()->createAccelerator(
		&acceleratorFactory, L"MainWindow", mapKeyNameToId, countof(mapKeyNameToId));
	if (!pImpl_->pAccelerator_.get())
		return -1;
	
	pImpl_->pFolderModel_.reset(new DefaultFolderModel());
#ifdef QMTABWINDOW
	pImpl_->pTabModel_.reset(new DefaultTabModel(pImpl_->pDocument_, pImpl_->pProfile_,
		Application::getApplication().getProfilePath(FileNames::TABS_XML).get()));
#endif
	pImpl_->pFolderListModel_.reset(new FolderListModel());
	pImpl_->pEncodingModel_.reset(new DefaultEncodingModel());
	pImpl_->pSecurityModel_.reset(new DefaultSecurityModel(
		pImpl_->pProfile_->getInt(L"MainWindow", L"SecurityMode", 0)));
	pImpl_->pViewModelManager_.reset(new ViewModelManager(pImpl_->pDocument_,
		pImpl_->pProfile_, getHandle(), pImpl_->pSecurityModel_.get()));
	pImpl_->pPreviewModel_.reset(new PreviewMessageModel(
		pImpl_->pViewModelManager_.get(), pImpl_->bShowPreviewWindow_));
	pImpl_->pEditFrameWindowManager_.reset(new EditFrameWindowManager(
		pImpl_->pDocument_, pImpl_->pUIManager_, pImpl_->pPasswordManager_,
		pImpl_->pSyncManager_, pImpl_->pSyncDialogManager_,
		pImpl_->pProfile_, pImpl_->pSecurityModel_.get()));
	pImpl_->pAddressBookFrameWindowManager_.reset(new AddressBookFrameWindowManager(
		pImpl_->pDocument_->getAddressBook(), pImpl_->pUIManager_, pImpl_->pProfile_));
	pImpl_->pExternalEditorManager_.reset(new ExternalEditorManager(
		pImpl_->pDocument_, pImpl_->pPasswordManager_,
		pImpl_->pProfile_, getHandle(), pImpl_->pTempFileCleaner_,
		pImpl_->pFolderModel_.get(), pImpl_->pSecurityModel_.get()));
	pImpl_->pMessageFrameWindowManager_.reset(new MessageFrameWindowManager(
		pImpl_->pDocument_, pImpl_->pUIManager_, pImpl_->pTempFileCleaner_,
		pImpl_->pProfile_, pImpl_->pViewModelManager_.get(),
		pImpl_->pEditFrameWindowManager_.get(), pImpl_->pExternalEditorManager_.get()));
	
	pImpl_->pMoveMenu_.reset(new MoveMenu());
	pImpl_->pFilterMenu_.reset(new FilterMenu(
		pImpl_->pViewModelManager_->getFilterManager()));
	pImpl_->pSortMenu_.reset(new SortMenu(pImpl_->pViewModelManager_.get()));
	pImpl_->pAttachmentMenu_.reset(new AttachmentMenu(pImpl_->pSecurityModel_.get()));
	pImpl_->pViewTemplateMenu_.reset(new ViewTemplateMenu(
		pImpl_->pDocument_->getTemplateManager()));
	pImpl_->pCreateTemplateMenu_.reset(new CreateTemplateMenu(
		pImpl_->pDocument_->getTemplateManager(), false));
	pImpl_->pCreateTemplateExternalMenu_.reset(new CreateTemplateMenu(
		pImpl_->pDocument_->getTemplateManager(), true));
	pImpl_->pEncodingMenu_.reset(new EncodingMenu(pImpl_->pProfile_));
	pImpl_->pSubAccountMenu_.reset(new SubAccountMenu(pImpl_->pFolderModel_.get()));
	pImpl_->pGoRoundMenu_.reset(new GoRoundMenu(pImpl_->pGoRound_));
	pImpl_->pScriptMenu_.reset(new ScriptMenu(pImpl_->pDocument_->getScriptManager()));
	pImpl_->pRecentsMenu_.reset(new RecentsMenu(
		pImpl_->pDocument_->getRecents(), pImpl_->pDocument_));
	
	bool bVirticalFolderWindow = pImpl_->bVirticalFolderWindow_;
	DWORD dwStyle = WS_CHILD | WS_VISIBLE | WS_CLIPCHILDREN | WS_CLIPSIBLINGS;
#if defined _WIN32_WCE && _WIN32_WCE >= 300 && defined _WIN32_WCE_PSPC
	DWORD dwExStyle = 0;
#else
	DWORD dwExStyle = WS_EX_CLIENTEDGE;
#endif
	
	std::auto_ptr<SplitterWindow> pFolderSplitterWindow(new SplitterWindow(
		bVirticalFolderWindow ? 1 : 2, bVirticalFolderWindow ? 2 : 1, true, pImpl_));
	if (!pFolderSplitterWindow->create(L"QmFolderSplitterWindow",
		0, dwStyle, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
		CW_USEDEFAULT, getHandle(), 0, 0,
		MainWindowImpl::ID_FOLDERSPLITTERWINDOW, 0))
		return -1;
	pImpl_->pFolderSplitterWindow_ = pFolderSplitterWindow.release();
	
	std::auto_ptr<FolderWindow> pFolderWindow(new FolderWindow(
		pImpl_->pFolderSplitterWindow_, pImpl_->pFolderModel_.get(), pImpl_->pProfile_));
	FolderWindowCreateContext folderWindowContext = {
		pContext->pDocument_,
		pContext->pUIManager_
	};
	if (!pFolderWindow->create(L"QmFolderWindow",
		0, dwStyle, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
		CW_USEDEFAULT, pImpl_->pFolderSplitterWindow_->getHandle(),
		dwExStyle, 0, MainWindowImpl::ID_FOLDERWINDOW, &folderWindowContext))
		return -1;
	pImpl_->pFolderWindow_ = pFolderWindow.release();
	
	std::auto_ptr<FolderComboBox> pFolderComboBox(new FolderComboBox(
		this, pImpl_->pFolderModel_.get(), pImpl_->pProfile_));
	FolderComboBoxCreateContext folderComboBoxContext = {
		pContext->pDocument_,
		pContext->pUIManager_
	};
	if (!pFolderComboBox->create(L"QmFolderComboBox",
		0, dwStyle, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
		CW_USEDEFAULT, getHandle(), dwExStyle, 0,
		MainWindowImpl::ID_FOLDERCOMBOBOX, &folderComboBoxContext))
		return -1;
	pImpl_->pFolderComboBox_ = pFolderComboBox.release();
	
	std::auto_ptr<SplitterWindow> pListSplitterWindow(
		new SplitterWindow(1, 2, true, pImpl_));
	if (!pListSplitterWindow->create(L"QmListSplitterWindow",
		0, dwStyle, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
		CW_USEDEFAULT, getHandle(), 0, 0,
		MainWindowImpl::ID_LISTSPLITTERWINDOW, 0))
		return -1;
	pImpl_->pListSplitterWindow_ = pListSplitterWindow.release();
	
#ifdef QMTABWINDOW
	std::auto_ptr<TabWindow> pTabWindow(new TabWindow(
		pImpl_->pTabModel_.get(), pImpl_->pProfile_));
	TabWindowCreateContext tabContext = {
		pContext->pDocument_,
		pContext->pUIManager_,
	};
	if (!pTabWindow->create(L"QmTabWindow", 0, dwStyle,
		CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
		pImpl_->pListSplitterWindow_->getHandle(), 0, 0,
		MainWindowImpl::ID_TABWINDOW, &tabContext))
		return -1;
	pImpl_->pTabWindow_ = pTabWindow.release();
	HWND hwndListContainerParent = pImpl_->pTabWindow_->getHandle();
#else
	HWND hwndListContainerParent = pImpl_->pListSplitterWindow_->getHandle();
#endif
	
	std::auto_ptr<ListContainerWindow> pListContainerWindow(
		new ListContainerWindow(pImpl_->pFolderModel_.get()));
	if (!pListContainerWindow->create(L"QmListContainerWindow", 0,
		dwStyle, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
		CW_USEDEFAULT, hwndListContainerParent, dwExStyle,
		0, MainWindowImpl::ID_LISTCONTAINERWINDOW, 0))
		return -1;
	pImpl_->pListContainerWindow_ = pListContainerWindow.release();
#ifdef QMTABWINDOW
	pImpl_->pTabWindow_->setControl(pImpl_->pListContainerWindow_->getHandle());
#endif
	
	std::auto_ptr<FolderListWindow> pFolderListWindow(new FolderListWindow(
		pImpl_->pListContainerWindow_, pImpl_->pFolderListModel_.get(),
		pImpl_->pFolderModel_.get(), pImpl_->pProfile_));
	FolderListWindowCreateContext folderListContext = {
		pContext->pUIManager_
	};
	if (!pFolderListWindow->create(L"QmFolderListWindow",
		0, dwStyle, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
		CW_USEDEFAULT, pImpl_->pListContainerWindow_->getHandle(),
		0, 0, MainWindowImpl::ID_FOLDERLISTWINDOW, &folderListContext))
		return -1;
	pImpl_->pFolderListWindow_ = pFolderListWindow.release();
	pImpl_->pListContainerWindow_->setFolderListWindow(pImpl_->pFolderListWindow_);
	
	std::auto_ptr<ListWindow> pListWindow(new ListWindow(
		pImpl_->pViewModelManager_.get(), pImpl_->pProfile_,
		pImpl_->pMessageFrameWindowManager_.get()));
	ListWindowCreateContext listContext = {
		pContext->pDocument_,
		pContext->pUIManager_,
		pImpl_->pSyncManager_,
		pImpl_->pSyncDialogManager_
	};
	if (!pListWindow->create(L"QmListWindow",
		0, dwStyle, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
		CW_USEDEFAULT, pImpl_->pListContainerWindow_->getHandle(),
		0, 0, MainWindowImpl::ID_LISTWINDOW, &listContext))
		return -1;
	pImpl_->pListWindow_ = pListWindow.release();
	pImpl_->pListContainerWindow_->setListWindow(pImpl_->pListWindow_);
	
	std::auto_ptr<MessageWindow> pMessageWindow(new MessageWindow(
		pImpl_->pPreviewModel_.get(), pImpl_->pProfile_, L"PreviewWindow"));
	pImpl_->pMessageViewModeHolder_ = pImpl_->pProfile_->getInt(L"Global", L"SaveMessageViewModePerFolder", 1) != 0 ?
		pImpl_->pPreviewModel_.get() : pMessageWindow->getMessageViewModeHolder();
	MessageWindowCreateContext messageContext = {
		pContext->pDocument_,
		pContext->pUIManager_,
		pImpl_->pMessageViewModeHolder_,
		pImpl_->pEncodingModel_.get(),
		pImpl_->pSecurityModel_.get(),
	};
	if (!pMessageWindow->create(L"QmMessageWindow",
		0, dwStyle, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
		CW_USEDEFAULT, pImpl_->pFolderSplitterWindow_->getHandle(),
		0, 0, MainWindowImpl::ID_MESSAGEWINDOW, &messageContext))
		return -1;
	pImpl_->pMessageWindow_ = pMessageWindow.release();
	
#ifdef QMTABWINDOW
	pImpl_->pListSplitterWindow_->add(0, 0, pImpl_->pTabWindow_);
#else
	pImpl_->pListSplitterWindow_->add(0, 0, pImpl_->pListContainerWindow_);
#endif
	pImpl_->pListSplitterWindow_->add(0, 1, pImpl_->pMessageWindow_);
	
	pImpl_->pFolderSplitterWindow_->add(0, 0, pImpl_->pFolderWindow_);
	pImpl_->pFolderSplitterWindow_->add(bVirticalFolderWindow ? 0 : 1,
		bVirticalFolderWindow ? 1 : 0, pImpl_->pListSplitterWindow_);
	
	DWORD dwStatusBarStyle = dwStyle;
#if _WIN32_WCE >= 300 && defined _WIN32_WCE_PSPC
	dwStatusBarStyle |= CCS_NOPARENTALIGN;
#endif
	std::auto_ptr<MainWindowStatusBar> pStatusBar(new MainWindowStatusBar(
		pImpl_->pDocument_, pImpl_->pViewModelManager_.get(),
		pImpl_->pFolderModel_.get(), pImpl_->pSyncManager_,
		pImpl_->pMessageWindow_, pImpl_->pEncodingModel_.get(), 2,
		pImpl_->pEncodingMenu_.get(), pImpl_->pViewTemplateMenu_.get(),
		pImpl_->pFilterMenu_.get()));
	if (!pStatusBar->create(L"QmStatusBarWindow", 0, dwStatusBarStyle,
		CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, getHandle(),
		0, STATUSCLASSNAMEW, MainWindowImpl::ID_STATUSBAR, 0))
		return -1;
	pImpl_->pStatusBar_ = pStatusBar.release();
	
	std::auto_ptr<SyncNotificationWindow> pSyncNotificationWindow(
		new SyncNotificationWindow(pImpl_->pSyncManager_, pImpl_->pSyncDialogManager_));
	if (!pSyncNotificationWindow->create(L"QmSyncNotificationWindow",
		0, dwStyle & ~WS_VISIBLE, 0, 0, SyncNotificationWindow::WIDTH,
		SyncNotificationWindow::HEIGHT, getHandle(), 0,
		0, MainWindowImpl::ID_SYNCNOTIFICATION, 0))
		return -1;
	pImpl_->pSyncNotificationWindow_ = pSyncNotificationWindow.release();
	
	pImpl_->layoutChildren();
	
	if (bVirticalFolderWindow)
		pImpl_->pFolderSplitterWindow_->setRowHeight(
			0, pImpl_->nFolderWindowSize_);
	else
		pImpl_->pFolderSplitterWindow_->setColumnWidth(
			0, pImpl_->nFolderWindowSize_);
	
	pImpl_->pListSplitterWindow_->setRowHeight(
		0, pImpl_->nListWindowHeight_);
	
	pImpl_->pMessageSelectionModel_.reset(
		new MainWindowImpl::MessageSelectionModelImpl(pImpl_, false));
	pImpl_->pListOnlyMessageSelectionModel_.reset(
		new MainWindowImpl::MessageSelectionModelImpl(pImpl_, true));
	
	pImpl_->pDelayedFolderModelHandler_.reset(new DelayedFolderModelHandler(pImpl_));
	pImpl_->pFolderModel_->addFolderModelHandler(pImpl_->pDelayedFolderModelHandler_.get());
	pImpl_->pDocument_->addAccountManagerHandler(pImpl_);
	pImpl_->pDocument_->getRecents()->addRecentsHandler(pImpl_);
#ifdef QMTABWINDOW
	pImpl_->pViewModelManager_->addViewModelManagerHandler(pImpl_);
	pImpl_->pTabModel_->addTabModelHandler(pImpl_);
#endif
	pImpl_->pMessageWindow_->addMessageWindowHandler(pImpl_);
	
	pImpl_->initActions();
	
#ifndef _WIN32_WCE_PSPC
	HINSTANCE hInst = Application::getApplication().getResourceHandle();
	pImpl_->notifyIcon_.cbSize = sizeof(pImpl_->notifyIcon_);
	pImpl_->notifyIcon_.hWnd = getHandle();
	pImpl_->notifyIcon_.uID = MainWindowImpl::ID_NOTIFYICON;
	pImpl_->notifyIcon_.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
	pImpl_->notifyIcon_.uCallbackMessage = MainWindowImpl::WM_MAINWINDOW_NOTIFYICON;
	pImpl_->notifyIcon_.hIcon = reinterpret_cast<HICON>(::LoadImage(hInst,
		MAKEINTRESOURCE(IDI_MAINFRAME), IMAGE_ICON, 16, 16, 0));
	_tcscpy(pImpl_->notifyIcon_.szTip, _T("QMAIL"));
	
	UINT nHotKeyModifier = pImpl_->pProfile_->getInt(
		L"Recents", L"HotKeyModifiers", MOD_ALT | MOD_SHIFT);
	UINT nHotKey = pImpl_->pProfile_->getInt(L"Recents", L"HotKey", 'A');
	::RegisterHotKey(getHandle(), MainWindowImpl::HOTKEY_RECENTS, nHotKeyModifier, nHotKey);
#endif
	
	pImpl_->bCreated_ = true;
	
	return 0;
}

LRESULT qm::MainWindow::onDestroy()
{
#ifndef _WIN32_WCE_PSPC
	::UnregisterHotKey(getHandle(), MainWindowImpl::HOTKEY_RECENTS);
	
	if (pImpl_->bNotifyIcon_)
		Shell_NotifyIcon(NIM_DELETE, &pImpl_->notifyIcon_);
#endif
	
	pImpl_->pMessageWindow_->removeMessageWindowHandler(pImpl_);
	pImpl_->pFolderModel_->removeFolderModelHandler(
		pImpl_->pDelayedFolderModelHandler_.get());
	pImpl_->pDocument_->removeAccountManagerHandler(pImpl_);
	pImpl_->pDocument_->getRecents()->removeRecentsHandler(pImpl_);
#ifdef QMTABWINDOW
	pImpl_->pViewModelManager_->removeViewModelManagerHandler(pImpl_);
	pImpl_->pTabModel_->removeTabModelHandler(pImpl_);
#endif
	
	if (pImpl_->pToolbarCookie_)
		pImpl_->pUIManager_->getToolbarManager()->destroy(pImpl_->pToolbarCookie_);
	
	::PostQuitMessage(0);
	
	return FrameWindow::onDestroy();
}

#ifndef _WIN32_WCE
LRESULT qm::MainWindow::onEndSession(bool bEnd,
									 int nOption)
{
	if (bEnd)
		Application::getApplication().uninitialize();
	return 0;
}
#endif

#ifndef _WIN32_WCE_PSPC
LRESULT qm::MainWindow::onHotKey(UINT nId,
								 UINT nModifier,
								 UINT nKey)
{
	if (nId == MainWindowImpl::HOTKEY_RECENTS)
		pImpl_->showRecentsMenu();
	return 0;
}
#endif

LRESULT qm::MainWindow::onInitMenuPopup(HMENU hmenu,
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
		
		std::pair<Account*, Folder*> p(pImpl_->pFolderModel_->getCurrent());
		Account* pAccount = p.first ? p.first : p.second ? p.second->getAccount() : 0;
		if (nIdLast == IDM_MESSAGE_MOVEOTHER) {
			if (pAccount)
				pImpl_->pMoveMenu_->createMenu(hmenu, pAccount,
					::GetKeyState(VK_SHIFT) < 0, *pImpl_->pActionMap_);
			else
				::EnableMenuItem(hmenu, nIndex, MF_GRAYED | MF_BYPOSITION);
		}
		else if (nIdLast == IDM_VIEW_SORTFLOATTHREAD) {
			pImpl_->pSortMenu_->createMenu(hmenu);
		}
		else if (nIdFirst == IDM_VIEW_FILTERNONE) {
			pImpl_->pFilterMenu_->createMenu(hmenu);
		}
		else if (nIdFirst == IDM_TOOL_GOROUND) {
			pImpl_->pGoRoundMenu_->createMenu(hmenu);
		}
		else if (nIdFirst == IDM_TOOL_SUBACCOUNT) {
			pImpl_->pSubAccountMenu_->createMenu(hmenu);
		}
		else if (nIdFirst == IDM_MESSAGE_DETACH) {
			pImpl_->pAttachmentMenu_->createMenu(hmenu,
				pImpl_->pMessageSelectionModel_->getFocusedMessage());
		}
		else if (nIdFirst == IDM_VIEW_TEMPLATENONE) {
			if (pAccount)
				pImpl_->pViewTemplateMenu_->createMenu(hmenu, pAccount);
			else
				::EnableMenuItem(hmenu, nIndex, MF_GRAYED | MF_BYPOSITION);
		}
		else if (nIdFirst == IDM_MESSAGE_APPLYTEMPLATE ||
			nIdFirst == IDM_MESSAGE_APPLYTEMPLATENONE) {
			if (pAccount)
				pImpl_->pCreateTemplateMenu_->createMenu(hmenu, pAccount);
			else
				::EnableMenuItem(hmenu, nIndex, MF_GRAYED | MF_BYPOSITION);
		}
		else if (nIdFirst == IDM_MESSAGE_APPLYTEMPLATEEXTERNAL ||
			nIdFirst == IDM_MESSAGE_APPLYTEMPLATENONEEXTERNAL) {
			if (pAccount)
				pImpl_->pCreateTemplateExternalMenu_->createMenu(hmenu, pAccount);
			else
				::EnableMenuItem(hmenu, nIndex, MF_GRAYED | MF_BYPOSITION);
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

#ifndef _WIN32_WCE
LRESULT qm::MainWindow::onQueryEndSession(int nOption)
{
	FileExitAction* pAction = static_cast<FileExitAction*>(
		pImpl_->pActionMap_->getAction(IDM_FILE_EXIT));
	assert(pAction);
	return pAction->exit(false);
}
#endif

LRESULT qm::MainWindow::onSize(UINT nFlags,
							   int cx,
							   int cy)
{
	if (pImpl_->bCreated_ &&
		!pImpl_->bLayouting_ &&
		(nFlags == SIZE_RESTORED || nFlags == SIZE_MAXIMIZED))
		pImpl_->layoutChildren(cx, cy);
	return FrameWindow::onSize(nFlags, cx, cy);
}

#ifndef _WIN32_WCE_PSPC
LRESULT qm::MainWindow::onNotifyIcon(WPARAM wParam,
									 LPARAM lParam)
{
	if (wParam == MainWindowImpl::ID_NOTIFYICON) {
		if (lParam == WM_LBUTTONDOWN || lParam == WM_RBUTTONDOWN)
			pImpl_->showRecentsMenu();
	}
	return 0;
}

LRESULT qm::MainWindow::onRecentsChanged(WPARAM wParam,
										 LPARAM lParam)
{
	Recents* pRecents = pImpl_->pDocument_->getRecents();
	
	Lock<Recents> lock(*pRecents);
	
	unsigned int nCount = pRecents->getCount();
	if (nCount != 0 && !pImpl_->bNotifyIcon_) {
		Shell_NotifyIcon(NIM_ADD, &pImpl_->notifyIcon_);
		pImpl_->bNotifyIcon_ = true;
	}
	else if (nCount == 0 && pImpl_->bNotifyIcon_) {
		Shell_NotifyIcon(NIM_DELETE, &pImpl_->notifyIcon_);
		pImpl_->bNotifyIcon_ = false;
	}
	
	return 0;
}
#endif


/****************************************************************************
 *
 * ListContainerWindow
 *
 */

qm::ListContainerWindow::ListContainerWindow(FolderModel* pFolderModel) :
	WindowBase(true),
	pFolderModel_(pFolderModel),
	pFolderListWindow_(0),
	pListWindow_(0)
{
	pDelayedFolderModelHandler_.reset(new DelayedFolderModelHandler(this));
	pFolderModel_->addFolderModelHandler(pDelayedFolderModelHandler_.get());
	setWindowHandler(this, false);
}

qm::ListContainerWindow::~ListContainerWindow()
{
	pFolderModel_->removeFolderModelHandler(pDelayedFolderModelHandler_.get());
}

void qm::ListContainerWindow::setFolderListWindow(FolderListWindow* pFolderListWindow)
{
	pFolderListWindow_ = pFolderListWindow;
	pFolderListWindow_->showWindow(SW_HIDE);
}

void qm::ListContainerWindow::setListWindow(ListWindow* pListWindow)
{
	pListWindow_ = pListWindow;
}

LRESULT qm::ListContainerWindow::windowProc(UINT uMsg,
											WPARAM wParam,
											LPARAM lParam)
{
	BEGIN_MESSAGE_HANDLER()
		HANDLE_CREATE()
		HANDLE_DESTROY()
#ifndef _WIN32_WCE
		HANDLE_NCPAINT()
#endif
		HANDLE_SIZE()
#ifndef _WIN32_WCE
		HANDLE_THEMECHANGED()
#endif
	END_MESSAGE_HANDLER()
	return DefaultWindowHandler::windowProc(uMsg, wParam, lParam);
}

LRESULT qm::ListContainerWindow::onCreate(CREATESTRUCT* pCreateStruct)
{
	if (DefaultWindowHandler::onCreate(pCreateStruct) == -1)
		return -1;
	
#ifndef _WIN32_WCE
	pTheme_.reset(new Theme(getHandle(), L"ListView"));
#endif
	
	return 0;
}

LRESULT qm::ListContainerWindow::onDestroy()
{
#ifndef _WIN32_WCE
	pTheme_.reset(0);
#endif
	return DefaultWindowHandler::onDestroy();
}

#ifndef _WIN32_WCE
LRESULT qm::ListContainerWindow::onNcPaint(HRGN hrgn)
{
	DefaultWindowHandler::onNcPaint(hrgn);
	
	if (getWindowLong(GWL_EXSTYLE) & WS_EX_CLIENTEDGE && pTheme_->isActive())
		qs::UIUtil::drawThemeBorder(pTheme_.get(), getHandle(), LVP_LISTDETAIL, 0, ::GetSysColor(COLOR_WINDOW));
	
	return 0;
}
#endif

LRESULT qm::ListContainerWindow::onSize(UINT nFlags,
										int cx,
										int cy)
{
	if (pFolderListWindow_)
		pFolderListWindow_->setWindowPos(0, 0, 0, cx, cy,
			SWP_NOZORDER | SWP_NOMOVE | SWP_NOACTIVATE);
	if (pListWindow_)
		pListWindow_->setWindowPos(0, 0, 0, cx, cy,
			SWP_NOZORDER | SWP_NOMOVE | SWP_NOACTIVATE);
	
	return DefaultWindowHandler::onSize(nFlags, cx, cy);
}

#ifndef _WIN32_WCE
LRESULT qm::ListContainerWindow::onThemeChanged()
{
	pTheme_.reset(new Theme(getHandle(), L"ListView"));
	return 0;
}
#endif

void qm::ListContainerWindow::accountSelected(const FolderModelEvent& event)
{
	assert(pFolderListWindow_);
	assert(pListWindow_);
	
	bool bActive = pListWindow_->isActive();
	
	pFolderListWindow_->showWindow(SW_SHOW);
	pListWindow_->showWindow(SW_HIDE);
	
	if (bActive)
		pFolderListWindow_->setActive();
}

void qm::ListContainerWindow::folderSelected(const FolderModelEvent& event)
{
	assert(pFolderListWindow_);
	assert(pListWindow_);
	
	bool bActive = pFolderListWindow_->isActive();
	
	pListWindow_->showWindow(SW_SHOW);
	pFolderListWindow_->showWindow(SW_HIDE);
	
	if (bActive)
		pListWindow_->setActive();
}


/****************************************************************************
 *
 * SyncNotificationWindow
 *
 */

qm::SyncNotificationWindow::SyncNotificationWindow(SyncManager* pSyncManager,
												   SyncDialogManager* pSyncDialogManager) :
	WindowBase(true),
	pSyncManager_(pSyncManager),
	pSyncDialogManager_(pSyncDialogManager),
	hbm_(0)
{
	setWindowHandler(this, false);
}

qm::SyncNotificationWindow::~SyncNotificationWindow()
{
}

void qm::SyncNotificationWindow::getWindowClass(WNDCLASS* pwc)
{
	DefaultWindowHandler::getWindowClass(pwc);
	pwc->hbrBackground = reinterpret_cast<HBRUSH>(::GetStockObject(BLACK_BRUSH));
}

LRESULT qm::SyncNotificationWindow::windowProc(UINT uMsg,
											   WPARAM wParam,
											   LPARAM lParam)
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
	
	hbm_ = static_cast<HBITMAP>(::LoadImage(
		Application::getApplication().getResourceHandle(),
		MAKEINTRESOURCE(IDB_SYNCNOTIFICATION), IMAGE_BITMAP, 0, 0, 0));
	
	pSyncManager_->addSyncManagerHandler(this);
	
	return 0;
}

LRESULT qm::SyncNotificationWindow::onDestroy()
{
	::DeleteObject(hbm_);
	pSyncManager_->removeSyncManagerHandler(this);
	return DefaultWindowHandler::onDestroy();
}

LRESULT qm::SyncNotificationWindow::onLButtonDown(UINT nFlags,
												  const POINT& pt)
{
	SyncDialog* pDialog = pSyncDialogManager_->open();
	if (pDialog)
		pDialog->show();
	return DefaultWindowHandler::onLButtonDown(nFlags, pt);
}

LRESULT qm::SyncNotificationWindow::onPaint()
{
	PaintDeviceContext dc(getHandle());
	
	CompatibleDeviceContext dcMem(dc.getHandle());
	ObjectSelector<HBITMAP> selector(dcMem, hbm_);
	dc.bitBlt(0, 0, WIDTH, HEIGHT, dcMem.getHandle(), 0, 0, SRCCOPY);
	
	return 0;
}

LRESULT qm::SyncNotificationWindow::onStatusChanged(WPARAM wParam,
													LPARAM lParam)
{
	if (pSyncManager_->isSyncing())
		showWindow(SW_SHOW);
	else
		showWindow(SW_HIDE);
	return 0;
}

void qm::SyncNotificationWindow::statusChanged(const SyncManagerEvent& event)
{
	postMessage(WM_SYNCNOTIFICATION_STATUSCHANGED);
}


/****************************************************************************
 *
 * MainWindowStatusBar
 *
 */

qm::MainWindowStatusBar::MainWindowStatusBar(Document* pDocument,
											 ViewModelManager* pViewModelManager,
											 FolderModel* pFolderModel,
											 SyncManager* pSyncManager,
											 MessageWindow* pMessageWindow,
											 EncodingModel* pEncodingModel,
											 int nOffset,
											 EncodingMenu* pEncodingMenu,
											 ViewTemplateMenu* pViewTemplateMenu,
											 FilterMenu* pFilterMenu) :
	MessageStatusBar(pMessageWindow, pEncodingModel, nOffset, pEncodingMenu, pViewTemplateMenu),
	pDocument_(pDocument),
	pViewModelManager_(pViewModelManager),
	pFolderModel_(pFolderModel),
	pSyncManager_(pSyncManager),
	pFilterMenu_(pFilterMenu),
	nCount_(-1),
	nUnseenCount_(-1),
	nSelectedCount_(-1),
	offline_(OFFLINE_NONE)
{
}

qm::MainWindowStatusBar::~MainWindowStatusBar()
{
}

void qm::MainWindowStatusBar::updateListParts(const WCHAR* pwszText)
{
	HINSTANCE hInst = Application::getApplication().getResourceHandle();
	
	if (pwszText) {
		if (*pwszText) {
			if (!wstrText_.get() || wcscmp(wstrText_.get(), pwszText) != 0) {
				nCount_ = -1;
				nUnseenCount_ = -1;
				nSelectedCount_ = -1;
				wstrText_ = allocWString(pwszText);
				setText(0, wstrText_.get());
			}
		}
		else {
			wstrText_.reset(0);
		}
	}
	
	ViewModel* pViewModel = pViewModelManager_->getCurrentViewModel();
	if (pViewModel) {
		Lock<ViewModel> lock(*pViewModel);
		
		if (!wstrText_.get()) {
			unsigned int nCount = nCount_;
			unsigned int nUnseenCount = nUnseenCount_;
			unsigned int nSelectedCount = nSelectedCount_;
			nCount_ = pViewModel->getCount();
			nUnseenCount_ = pViewModel->getUnseenCount();
			nSelectedCount_ = pViewModel->getSelectedCount();
			if (nCount != nCount_ ||
				nUnseenCount != nUnseenCount_ ||
				nSelectedCount != nSelectedCount_) {
				wstring_ptr wstrTemplate(loadString(hInst, IDS_VIEWMODELSTATUSTEMPLATE));
				WCHAR wsz[256];
				swprintf(wsz, wstrTemplate.get(), nCount_, nUnseenCount_, nSelectedCount_);
				setText(0, wsz);
			}
		}
		
#ifndef _WIN32_WCE_PSPC
		wstring_ptr wstrFilter(wstrFilter_);
		const Filter* pFilter = pViewModel->getFilter();
		if (pFilter) {
			const WCHAR* pwszName = pFilter->getName();
			if (*pwszName)
				wstrFilter_ = allocWString(pwszName);
			else
				wstrFilter_ = loadString(hInst, IDS_CUSTOM);
		}
		else {
			wstrFilter_ = loadString(hInst, IDS_NONE);
		}
		if (!wstrFilter.get() || wcscmp(wstrFilter.get(), wstrFilter_.get()) != 0)
			setText(2, wstrFilter_.get());
#endif
	}
	else {
		nCount_ = -1;
		nUnseenCount_ = -1;
		nSelectedCount_ = -1;
		wstrFilter_.reset(0);
		
		setText(0, L"");
		setText(2, L"");
	}
	
	Offline offline = pDocument_->isOffline() ? OFFLINE_OFFLINE : OFFLINE_ONLINE;
	if (offline != offline_) {
		bool bOffline = offline == OFFLINE_OFFLINE;
		wstring_ptr wstrOnline(loadString(hInst, bOffline ? IDS_OFFLINE : IDS_ONLINE));
		setIconOrText(1, bOffline ? IDI_OFFLINE : IDI_ONLINE, wstrOnline.get());
		offline_ = offline;
	}
}

LRESULT qm::MainWindowStatusBar::windowProc(UINT uMsg,
											WPARAM wParam,
											LPARAM lParam)
{
	BEGIN_MESSAGE_HANDLER()
		HANDLE_LBUTTONDOWN()
	END_MESSAGE_HANDLER()
	return MessageStatusBar::windowProc(uMsg, wParam, lParam);
}

LRESULT qm::MainWindowStatusBar::onLButtonDown(UINT nFlags,
											   const POINT& pt)
{
	int nPart = getPart(pt);
	if (nPart == 1)
		FileOfflineAction::toggleOffline(pDocument_, pSyncManager_);
	
	return MessageStatusBar::onLButtonDown(nFlags, pt);
}

HMENU qm::MainWindowStatusBar::getMenu(int nPart)
{
	HMENU hmenu = 0;
	if (nPart == 2) {
		HINSTANCE hInst = Application::getApplication().getResourceHandle();
		hmenu = ::LoadMenu(hInst, MAKEINTRESOURCE(IDR_VIEWFILTER));
		pFilterMenu_->createMenu(::GetSubMenu(hmenu, 0));
	}
	else {
		hmenu = MessageStatusBar::getMenu(nPart);
	}
	return hmenu;
}

Account* qm::MainWindowStatusBar::getAccount()
{
	std::pair<Account*, Folder*> p(pFolderModel_->getCurrent());
	return p.first ? p.first : p.second ? p.second->getAccount() : 0;
}
