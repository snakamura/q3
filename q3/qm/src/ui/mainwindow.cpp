/*
 * $Id$
 *
 * Copyright(C) 1998-2007 Satoshi Nakamura
 * All rights reserved.
 *
 */

#pragma warning(disable:4786)

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
#include <qmsecurity.h>
#include <qmtabwindow.h>

#include <qsaccelerator.h>
#include <qsconv.h>
#include <qsinit.h>
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
#include "actionitem.h"
#include "addressbookwindow.h"
#include "editframewindow.h"
#include "externaleditor.h"
#include "focus.h"
#include "foldercombobox.h"
#include "folderlistwindow.h"
#include "folderwindow.h"
#include "listwindow.h"
#include "mainwindow.h"
#include "menucreator.h"
#include "menucreatormacro.h"
#include "messageframewindow.h"
#include "messagewindow.h"
#include "optiondialog.h"
#include "recentswindow.h"
#include "resourceinc.h"
#include "statusbar.h"
#include "syncdialog.h"
#include "tabwindow.h"
#include "uimanager.h"
#include "uiutil.h"
#include "../action/action.h"
#include "../action/actionmacro.h"
#include "../action/findreplace.h"
#include "../main/updatechecker.h"
#include "../model/filter.h"
#include "../model/goround.h"
#include "../model/tempfilecleaner.h"
#include "../sync/autopilot.h"
#include "../sync/syncmanager.h"
#include "../sync/syncqueue.h"
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

using namespace qm;
using namespace qs;


/****************************************************************************
 *
 * MainWindowImpl
 *
 */

class qm::MainWindowImpl :
	public DefaultModalHandler,
	public SplitterWindowHandler,
	public FolderModelHandler,
	public FolderSelectionModel,
	public ViewModelHolder,
	public MessageWindowHandler,
	public DefaultDocumentHandler,
	public DefaultAccountManagerHandler,
#ifndef _WIN32_WCE_PSPC
	public ShellIconCallback,
#endif
#ifdef QMTABWINDOW
	public ViewModelManagerHandler,
	public DefaultTabModelHandler,
#endif
	public MenuCreatorListCallback
{
public:
	enum {
		ID_PRIMARYSPLITTERWINDOW	= 1001,
		ID_SECONDARYSPLITTERWINDOW	= 1002,
		ID_FOLDERWINDOW				= 1003,
		ID_FOLDERCOMBOBOX			= 1004,
		ID_TABWINDOW				= 1005,
		ID_LISTCONTAINERWINDOW		= 1006,
		ID_FOLDERLISTWINDOW			= 1007,
		ID_LISTWINDOW				= 1008,
		ID_MESSAGEWINDOW			= 1009,
		ID_TOOLBAR					= 1010,
		ID_STATUSBAR				= 1011,
		ID_COMMANDBARMENU			= 1012,
		ID_COMMANDBARBUTTON			= 1013,
		ID_SYNCNOTIFICATION			= 1014,
	};

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
	void initMenuCreators();
	void layoutChildren();
	void layoutChildren(int cx,
						int cy);
	void updateTitleBar();
	void updateStatusBar();
	void reloadProfiles(bool bInitialize);

protected:
	virtual void preModalDialog(HWND hwndParent,
								bool bFirst);
	virtual void postModalDialog(HWND hwndParent,
								 bool bLast);

public:
	virtual void sizeChanged(const SplitterWindowEvent& event);

public:
	virtual void accountSelected(const FolderModelEvent& event);
	virtual void folderSelected(const FolderModelEvent& event);

public:
	virtual std::pair<Account*, Folder*> getFocusedAccountOrFolder();
	virtual void getSelectedFolders(Account::FolderList* pList);
	virtual bool hasSelectedFolder();

public:
	virtual ViewModel* getViewModel() const;
	virtual void setViewModel(ViewModel* pViewModel);

public:
	virtual void messageChanged(const MessageWindowEvent& event);
	virtual void statusTextChanged(const MessageWindowStatusTextEvent& event);

public:
	virtual void offlineStatusChanged(const DocumentEvent& event);

public:
	virtual void accountListChanged(const AccountManagerEvent& event);

#ifndef _WIN32_WCE_PSPC
public:
	virtual void showRecentsMenu(bool bHotKey);
	virtual void show();
#endif

#ifdef QMTABWINDOW
public:
	virtual void viewModelSelected(const ViewModelManagerEvent& event);

public:
	virtual void currentChanged(const TabModelEvent& event);
#endif

public:
	virtual std::auto_ptr<MacroMenuCreator> createMacroMenuCreator();

public:
	MainWindow* pThis_;
	
	bool bShowToolbar_;
	bool bShowStatusBar_;
	bool bShowFolderComboBox_;
	bool bSaveOnDeactivate_;
#ifndef _WIN32_WCE_PSPC
	bool bHideWhenMinimized_;
#endif
	bool bImeControl_;
	
	Profile* pProfile_;
	Document* pDocument_;
	UIManager* pUIManager_;
	PasswordManager* pPasswordManager_;
	SyncManager* pSyncManager_;
	SyncDialogManager* pSyncDialogManager_;
	SyncQueue* pSyncQueue_;
	GoRound* pGoRound_;
	TempFileCleaner* pTempFileCleaner_;
	AutoPilot* pAutoPilot_;
	UpdateChecker* pUpdateChecker_;
	const FolderImage* pFolderImage_;
	std::auto_ptr<Accelerator> pAccelerator_;
	std::auto_ptr<SplitterHelper> pSplitterHelper_;
	SplitterWindow* pPrimarySplitterWindow_;
	SplitterWindow* pSecondarySplitterWindow_;
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
#ifndef _WIN32_WCE_PSPC
	std::auto_ptr<ShellIcon> pShellIcon_;
#endif
#ifdef QMRECENTSWINDOW
	std::auto_ptr<RecentsWindowManager> pRecentsWindowManager_;
#endif
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
	std::auto_ptr<OptionDialogManager> pOptionDialogManager_;
	std::auto_ptr<MessageWindowFontManager> pMessageWindowFontManager_;
	std::auto_ptr<MessageFrameWindowManager> pMessageFrameWindowManager_;
	std::auto_ptr<EditFrameWindowManager> pEditFrameWindowManager_;
	std::auto_ptr<AddressBookFrameWindowManager> pAddressBookFrameWindowManager_;
	std::auto_ptr<ActionMap> pActionMap_;
	std::auto_ptr<ActionInvoker> pActionInvoker_;
	std::auto_ptr<FindReplaceManager> pFindReplaceManager_;
	std::auto_ptr<ExternalEditorManager> pExternalEditorManager_;
	std::auto_ptr<DelayedFolderModelHandler> pDelayedFolderModelHandler_;
	std::auto_ptr<MenuCreatorList> pMenuCreatorList_;
	ToolbarCookie* pToolbarCookie_;
	bool bCreated_;
	int nInitialShow_;
	bool bLayouting_;
	bool bShowingModalDialog_;
	HWND hwndLastFocused_;
};

void qm::MainWindowImpl::initActions()
{
	ADD_ACTION0(NoneAction,
		IDM_NONE);
	
	View* pViews[] = {
		pFolderWindow_,
		pFolderComboBox_,
		pFolderListWindow_,
		pListWindow_,
		pMessageWindow_
	};
	const WCHAR* pwszViewNames[] = {
		L"Folder",
		L"FolderComboBox",
		L"FolderList",
		L"List",
		L"Preview"
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
	ADD_ACTION4(ToolOptionsAction,
		IDM_CONFIG_AUTOPILOT,
		pOptionDialogManager_.get(),
		this,
		pThis_->getHandle(),
		OptionDialog::PANEL_AUTOPILOT);
	ADD_ACTION4(ToolOptionsAction,
		IDM_CONFIG_COLORS,
		pOptionDialogManager_.get(),
		this,
		pThis_->getHandle(),
		OptionDialog::PANEL_COLORS);
	ADD_ACTION4(ToolOptionsAction,
		IDM_CONFIG_FILTERS,
		pOptionDialogManager_.get(),
		this,
		pThis_->getHandle(),
		OptionDialog::PANEL_FILTERS);
	ADD_ACTION4(ToolOptionsAction,
		IDM_CONFIG_GOROUND,
		pOptionDialogManager_.get(),
		this,
		pThis_->getHandle(),
		OptionDialog::PANEL_GOROUND);
	ADD_ACTION4(ToolOptionsAction,
		IDM_CONFIG_RULES,
		pOptionDialogManager_.get(),
		this,
		pThis_->getHandle(),
		OptionDialog::PANEL_RULES);
	ADD_ACTION4(ToolOptionsAction,
		IDM_CONFIG_SIGNATURES,
		pOptionDialogManager_.get(),
		this,
		pThis_->getHandle(),
		OptionDialog::PANEL_SIGNATURES);
	ADD_ACTION4(ToolOptionsAction,
		IDM_CONFIG_SYNCFILTERS,
		pOptionDialogManager_.get(),
		this,
		pThis_->getHandle(),
		OptionDialog::PANEL_SYNCFILTERS);
	ADD_ACTION4(ToolOptionsAction,
		IDM_CONFIG_TEXTS,
		pOptionDialogManager_.get(),
		this,
		pThis_->getHandle(),
		OptionDialog::PANEL_FIXEDFORMTEXTS);
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
		pDocument_, pMessageSelectionModel_.get(), pThis_->getHandle()));
	std::auto_ptr<EditCommandAction<MessageWindowItem> > pCopyAction(
		new EditCommandAction<MessageWindowItem>(pMessageWindow_->getFocusController(),
		&MessageWindowItem::copy, &MessageWindowItem::canCopy));
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
		pDocument_, pMessageSelectionModel_.get(), pThis_->getHandle()));
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
		EditDeleteMessageAction::Type type_;
	} deletes[] = {
		{ IDM_EDIT_DELETE,			EditDeleteMessageAction::TYPE_NORMAL	},
		{ IDM_EDIT_DELETEDIRECT,	EditDeleteMessageAction::TYPE_DIRECT	},
		{ IDM_EDIT_DELETEJUNK,		EditDeleteMessageAction::TYPE_JUNK		}
	};
	for (int n = 0; n < countof(deletes); ++n) {
		std::auto_ptr<EditDeleteMessageAction> pEditDeleteMessageAction1(
			new EditDeleteMessageAction(pMessageSelectionModel_.get(),
				this, 0, deletes[n].type_, false,
				pDocument_->getUndoManager(), pThis_->getHandle(), pProfile_));
		std::auto_ptr<EditDeleteMessageAction> pEditDeleteMessageAction2(
			new EditDeleteMessageAction(pMessageSelectionModel_.get(),
				this, pPreviewModel_.get(), deletes[n].type_, false,
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
	std::auto_ptr<EditCommandAction<MessageWindowItem> > pSelectAllAction(
		new EditCommandAction<MessageWindowItem>(pMessageWindow_->getFocusController(),
		&MessageWindowItem::selectAll, &MessageWindowItem::canSelectAll));
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
	ADD_ACTION7(FileExportAction,
		IDM_FILE_EXPORT,
		pMessageSelectionModel_.get(),
		pEncodingModel_.get(),
		pSecurityModel_.get(),
		pDocument_,
		pActionInvoker_.get(),
		pProfile_,
		pThis_->getHandle());
#ifndef _WIN32_WCE_PSPC
	ADD_ACTION2(FileShowAction,
		IDM_FILE_HIDE,
		pThis_,
		false);
#endif
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
	ADD_ACTION8(FilePrintAction,
		IDM_FILE_PRINT,
		pDocument_,
		pMessageSelectionModel_.get(),
		pEncodingModel_.get(),
		pSecurityModel_.get(),
		pActionInvoker_.get(),
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
#ifndef _WIN32_WCE_PSPC
	ADD_ACTION2(FileShowAction,
		IDM_FILE_SHOW,
		pThis_,
		true);
#endif
	ADD_ACTION0(FileUninstallAction,
		IDM_FILE_UNINSTALL);
	ADD_ACTION4(FolderCreateAction,
		IDM_FOLDER_CREATE,
		this,
		pSyncManager_,
		pThis_->getHandle(),
		pProfile_);
	ADD_ACTION4(FolderDeleteAction,
		IDM_FOLDER_DELETE,
		pFolderModel_.get(),
		this,
		pSyncManager_,
		pThis_->getHandle());
	ADD_ACTION5(FolderEmptyAction,
		IDM_FOLDER_EMPTY,
		pDocument_,
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
	ADD_ACTION4(FolderPropertyAction,
		IDM_FOLDER_PROPERTY,
		this,
		pSyncManager_,
		pThis_->getHandle(),
		pProfile_);
	ADD_ACTION3(FolderRenameAction,
		IDM_FOLDER_RENAME,
		this,
		pSyncManager_,
		pThis_->getHandle());
	ADD_ACTION1(FolderShowSizeAction,
		IDM_FOLDER_SHOWSIZE,
		pFolderListWindow_);
	ADD_ACTION5(FolderSubscribeAction,
		IDM_FOLDER_SUBSCRIBE,
		pDocument_,
		pPasswordManager_,
		this,
		pSyncManager_,
		pThis_->getHandle());
	ADD_ACTION3(FolderUpdateAction,
		IDM_FOLDER_UPDATE,
		pFolderModel_.get(),
		pSyncManager_,
		pThis_->getHandle());
	ADD_ACTION1(HelpAboutAction,
		IDM_HELP_ABOUT,
		pThis_->getHandle());
	ADD_ACTION2(HelpCheckUpdateAction,
		IDM_HELP_CHECKUPDATE,
		pUpdateChecker_,
		pThis_->getHandle());
	ADD_ACTION1(HelpOpenURLAction,
		IDM_HELP_OPENURL,
		pThis_->getHandle());
	ADD_ACTION9(MessageApplyRuleAction,
		IDM_MESSAGE_APPLYRULE,
		pDocument_->getRuleManager(),
		pDocument_->getUndoManager(),
		pViewModelManager_.get(),
		false,
		pSecurityModel_.get(),
		pDocument_,
		pActionInvoker_.get(),
		pThis_->getHandle(),
		pProfile_);
	ADD_ACTION9(MessageApplyRuleAction,
		IDM_MESSAGE_APPLYRULEALL,
		pDocument_->getRuleManager(),
		pDocument_->getUndoManager(),
		pViewModelManager_.get(),
		true,
		pSecurityModel_.get(),
		pDocument_,
		pActionInvoker_.get(),
		pThis_->getHandle(),
		pProfile_);
	ADD_ACTION8(MessageApplyRuleAction,
		IDM_MESSAGE_APPLYRULESELECTED,
		pDocument_->getRuleManager(),
		pDocument_->getUndoManager(),
		pMessageSelectionModel_.get(),
		pSecurityModel_.get(),
		pDocument_,
		pActionInvoker_.get(),
		pThis_->getHandle(),
		pProfile_);
	ADD_ACTION1(MessageCertificateAction,
		IDM_MESSAGE_CERTIFICATE,
		pMessageWindow_);
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
	ADD_ACTION11(MessageCreateAction,
		IDM_MESSAGE_CREATE,
		pDocument_,
		pFolderModel_.get(),
		pMessageSelectionModel_.get(),
		pEncodingModel_.get(),
		pSecurityModel_.get(),
		pEditFrameWindowManager_.get(),
		pExternalEditorManager_.get(),
		pActionInvoker_.get(),
		pThis_->getHandle(),
		pProfile_,
		false);
	ADD_ACTION11(MessageCreateAction,
		IDM_MESSAGE_CREATEEXTERNAL,
		pDocument_,
		pFolderModel_.get(),
		pMessageSelectionModel_.get(),
		pEncodingModel_.get(),
		pSecurityModel_.get(),
		pEditFrameWindowManager_.get(),
		pExternalEditorManager_.get(),
		pActionInvoker_.get(),
		pThis_->getHandle(),
		pProfile_,
		true);
	ADD_ACTION7(MessageCreateFromClipboardAction,
		IDM_MESSAGE_CREATEFROMCLIPBOARD,
		false,
		pDocument_,
		pPasswordManager_,
		pProfile_,
		pThis_->getHandle(),
		this,
		pSecurityModel_.get());
	ADD_ACTION7(MessageCreateFromFileAction,
		IDM_MESSAGE_CREATEFROMFILE,
		false,
		pDocument_,
		pPasswordManager_,
		pProfile_,
		pThis_->getHandle(),
		this,
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
		this,
		pSecurityModel_.get());
	ADD_ACTION7(MessageCreateFromFileAction,
		IDM_MESSAGE_DRAFTFROMFILE,
		true,
		pDocument_,
		pPasswordManager_,
		pProfile_,
		pThis_->getHandle(),
		this,
		pSecurityModel_.get());
	ADD_ACTION4(MessageLabelAction,
		IDM_MESSAGE_LABEL,
		pMessageSelectionModel_.get(),
		pDocument_->getUndoManager(),
		pProfile_,
		pThis_->getHandle());
	
	std::auto_ptr<MessageMacroAction> pMessageMacroAction1(
		new MessageMacroAction(pMessageSelectionModel_.get(),
			pSecurityModel_.get(), pDocument_, pActionInvoker_.get(),
			pProfile_, pThis_->getHandle()));
	std::auto_ptr<MessageMacroAction> pMessageMacroAction2(
		new MessageMacroAction(this, pSecurityModel_.get(), pDocument_,
			pActionInvoker_.get(), pProfile_, pThis_->getHandle()));
	Action* pMessageMacroActions[] = {
		pMessageMacroAction2.get(),
		pMessageMacroAction2.get(),
		pMessageMacroAction2.get(),
		pMessageMacroAction1.get(),
		pMessageMacroAction1.get()
	};
	ADD_ACTION3(DispatchAction,
		IDM_MESSAGE_MACRO,
		pViews,
		pMessageMacroActions,
		countof(pViews));
	pMessageMacroAction1.release();
	pMessageMacroAction2.release();
	
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
	
	struct {
		UINT nId_;
		bool bCopy_;
	} moves[] = {
		{ IDM_MESSAGE_COPY,	true	},
		{ IDM_MESSAGE_MOVE,	false	}
	};
	for (int n = 0; n < countof(moves); ++n) {
		std::auto_ptr<MessageMoveAction> pMessageMoveAction1(new MessageMoveAction(
			pDocument_, pMessageSelectionModel_.get(), this, 0, moves[n].bCopy_, false,
			pDocument_->getUndoManager(), pFolderImage_, pProfile_, pThis_->getHandle()));
		std::auto_ptr<MessageMoveAction> pMessageMoveAction2(new MessageMoveAction(
			pDocument_, pMessageSelectionModel_.get(), this, pPreviewModel_.get(),
			moves[n].bCopy_, false, pDocument_->getUndoManager(),
			pFolderImage_, pProfile_, pThis_->getHandle()));
		Action* pMessageMoveActions[] = {
			0,
			0,
			0,
			pMessageMoveAction1.get(),
			pMessageMoveAction2.get()
		};
		ADD_ACTION3(DispatchAction,
			moves[n].nId_,
			pViews,
			pMessageMoveActions,
			countof(pViews));
		pMessageMoveAction1.release();
		pMessageMoveAction2.release();
	}
	
	ADD_ACTION5(MessageOpenAction,
		IDM_MESSAGE_OPEN,
		pDocument_,
		pViewModelManager_.get(),
		pFolderModel_.get(),
		pMessageFrameWindowManager_.get(),
		pThis_->getHandle());
	ADD_ACTION5(MessageOpenAttachmentAction,
		IDM_MESSAGE_OPENATTACHMENT,
		pDocument_,
		pSecurityModel_.get(),
		pProfile_,
		pTempFileCleaner_,
		pThis_->getHandle());
	ADD_ACTION3(MessageOpenLinkAction,
		IDM_MESSAGE_OPENLINK,
		pMessageSelectionModel_.get(),
		pProfile_,
		pThis_->getHandle());
	ADD_ACTION8(MessageOpenRecentAction,
		IDM_MESSAGE_OPENRECENT,
		pDocument_->getRecents(),
		pDocument_,
		pViewModelManager_.get(),
		pFolderModel_.get(),
		pThis_,
		pMessageFrameWindowManager_.get(),
		pProfile_,
		pThis_->getHandle());
	ADD_ACTION11(MessageOpenURLAction,
		IDM_MESSAGE_OPENURL,
		pDocument_,
		pPasswordManager_,
		pFolderModel_.get(),
		pMessageSelectionModel_.get(),
		pSecurityModel_.get(),
		pEditFrameWindowManager_.get(),
		pExternalEditorManager_.get(),
		pActionInvoker_.get(),
		pThis_->getHandle(),
		pProfile_,
		false);
	ADD_ACTION3(MessagePropertyAction,
		IDM_MESSAGE_PROPERTY,
		pMessageSelectionModel_.get(),
		pDocument_->getUndoManager(),
		pThis_->getHandle());
	ADD_ACTION6(MessageSearchAction,
		IDM_MESSAGE_SEARCH,
		pFolderModel_.get(),
		pSecurityModel_.get(),
		pDocument_,
		pActionInvoker_.get(),
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
	ADD_ACTION1(TabSelectAction,
		IDM_TAB_SELECT,
		pTabModel_.get());
#endif
	ADD_ACTION8(ToolAccountAction,
		IDM_TOOL_ACCOUNT,
		pDocument_,
		this,
		pPasswordManager_,
		pSyncManager_,
		pFolderImage_,
		pOptionDialogManager_.get(),
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
	ADD_ACTION5(ToolGoRoundAction,
		IDM_TOOL_GOROUND,
		pSyncManager_,
		pDocument_,
		pGoRound_,
		pSyncDialogManager_,
		pThis_->getHandle());
	ADD_ACTION3(ToolInvokeActionAction,
		IDM_TOOL_INVOKEACTION,
		pActionInvoker_.get(),
		pProfile_,
		pThis_->getHandle());
	ADD_ACTION4(ToolOptionsAction,
		IDM_TOOL_OPTIONS,
		pOptionDialogManager_.get(),
		this,
		pThis_->getHandle(),
		OptionDialog::PANEL_NONE);
	ADD_ACTION2(ToolPopupMenuAction,
		IDM_TOOL_POPUPMENU,
		pUIManager_->getMenuManager(),
		pThis_->getHandle());
	ADD_ACTION4(ToolScriptAction,
		IDM_TOOL_SCRIPT,
		pDocument_->getScriptManager(),
		pDocument_,
		pProfile_,
		pThis_);
	ADD_ACTION4(ToolSubAccountAction,
		IDM_TOOL_SUBACCOUNT,
		pDocument_,
		this,
		pSyncManager_,
		pThis_->getHandle());
	
	struct {
		UINT nId_;
		ToolSyncAction::Type type_;
	} syncs[] = {
		{ IDM_TOOL_SYNC,			ToolSyncAction::TYPE_SENDRECEIVE	},
		{ IDM_TOOL_RECEIVE,			ToolSyncAction::TYPE_RECEIVE		},
		{ IDM_TOOL_SEND,			ToolSyncAction::TYPE_SEND			},
		{ IDM_TOOL_RECEIVEFOLDER,	ToolSyncAction::TYPE_RECEIVEFOLDER	}
	};
	for (int n = 0; n < countof(syncs); ++n) {
		ADD_ACTION6(ToolSyncAction,
			syncs[n].nId_,
			pSyncManager_,
			pDocument_,
			pFolderModel_.get(),
			pSyncDialogManager_,
			syncs[n].type_,
			pThis_->getHandle());
	}
	
	ADD_ACTION1(ViewFontGroupAction,
		IDM_VIEW_FONTGROUP,
		pMessageWindow_);
	ADD_ACTION2(ViewLockPreviewAction,
		IDM_VIEW_LOCKPREVIEW,
		pPreviewModel_.get(),
		pThis_);
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
		IDM_VIEW_ENCODING,
		pEncodingModel_.get());
	ADD_ACTION1(ViewFilterAction,
		IDM_VIEW_FILTER,
		pViewModelManager_.get());
	ADD_ACTION2(ViewFilterCustomAction,
		IDM_VIEW_FILTERCUSTOM,
		pViewModelManager_.get(),
		pThis_->getHandle());
	ADD_ACTION1(ViewFitAction,
		IDM_VIEW_FIT,
		pMessageViewModeHolder_);
	ADD_ACTION3(ViewFocusAction,
		IDM_VIEW_FOCUS,
		pViews,
		countof(pViews),
		pwszViewNames);
	ADD_ACTION3(ViewFocusAction,
		IDM_VIEW_FOCUSNEXT,
		pViews,
		countof(pViews),
		ViewFocusAction::TYPE_NEXT);
	ADD_ACTION3(ViewFocusAction,
		IDM_VIEW_FOCUSPREV,
		pViews,
		countof(pViews),
		ViewFocusAction::TYPE_PREV);
	ADD_ACTION2(ViewFocusItemAction,
		IDM_VIEW_FOCUSITEM,
		pMessageWindow_->getFocusController(),
		ViewFocusItemAction::TYPE_ITEM);
	ADD_ACTION4(ViewMessageModeAction,
		IDM_VIEW_HTMLINTERNETZONEMODE,
		pMessageViewModeHolder_,
		MessageViewMode::MODE_INTERNETZONE,
		MessageViewMode::MODE_NONE,
		true);
	ADD_ACTION4(ViewMessageModeAction,
		IDM_VIEW_HTMLMODE,
		pMessageViewModeHolder_,
		MessageViewMode::MODE_HTML,
		MessageViewMode::MODE_NONE,
		true);
	ADD_ACTION4(ViewMessageModeAction,
		IDM_VIEW_HTMLONLINEMODE,
		pMessageViewModeHolder_,
		MessageViewMode::MODE_HTMLONLINE,
		MessageViewMode::MODE_NONE,
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
		ADD_ACTION7(ViewNavigateMessageAction,
			navigateMessages[n].nId_,
			pViewModelManager_.get(),
			pFolderModel_.get(),
			pThis_,
			pMessageWindow_,
			pDocument_,
			pProfile_,
			navigateMessages[n].type_);
	}
	
	ADD_ACTION4(ViewMessageModeAction,
		IDM_VIEW_QUOTEMODE,
		pMessageViewModeHolder_,
		MessageViewMode::MODE_QUOTE,
		MessageViewMode::MODE_NONE,
		true);
	ADD_ACTION4(ViewMessageModeAction,
		IDM_VIEW_RAWMODE,
		pMessageViewModeHolder_,
		MessageViewMode::MODE_RAW,
		MessageViewMode::MODE_SOURCE,
		true);
	ADD_ACTION8(ViewRefreshAction,
		IDM_VIEW_REFRESH,
		pSyncManager_,
		pDocument_,
		pFolderModel_.get(),
		pSecurityModel_.get(),
		pSyncDialogManager_,
		pActionInvoker_.get(),
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
	
	struct {
		UINT nId_;
		ViewScrollMessageAction::Scroll scroll_;
	} scrollMessages[] = {
		{ IDM_VIEW_SCROLLMESSAGEPAGEUP,		ViewScrollMessageAction::SCROLL_PAGEUP		},
		{ IDM_VIEW_SCROLLMESSAGEPAGEDOWN,	ViewScrollMessageAction::SCROLL_PAGEDOWN	},
	};
	for (int n = 0; n < countof(scrollMessages); ++n) {
		ADD_ACTION2(ViewScrollMessageAction,
			scrollMessages[n].nId_,
			pMessageWindow_,
			scrollMessages[n].scroll_);
	}
	
	ADD_ACTION3(ViewSelectMessageAction,
		IDM_VIEW_SELECTMESSAGE,
		pViewModelManager_.get(),
		pFolderModel_.get(),
		pMessageSelectionModel_.get());
	ADD_ACTION4(ViewMessageModeAction,
		IDM_VIEW_SELECTMODE,
		pMessageViewModeHolder_,
		MessageViewMode::MODE_SELECT,
		MessageViewMode::MODE_NONE,
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
	ADD_ACTION1(ViewSortAction,
		IDM_VIEW_SORT,
		pViewModelManager_.get());
	ADD_ACTION2(ViewSortDirectionAction,
		IDM_VIEW_SORTASCENDING,
		pViewModelManager_.get(),
		true);
	ADD_ACTION2(ViewSortDirectionAction,
		IDM_VIEW_SORTDESCENDING,
		pViewModelManager_.get(),
		false);
	ADD_ACTION2(ViewSortThreadAction,
		IDM_VIEW_SORTFLAT,
		pViewModelManager_.get(),
		ViewSortThreadAction::TYPE_FLAT);
	ADD_ACTION2(ViewSortThreadAction,
		IDM_VIEW_SORTFLOATTHREAD,
		pViewModelManager_.get(),
		ViewSortThreadAction::TYPE_FLOATTHREAD);
	ADD_ACTION2(ViewSortThreadAction,
		IDM_VIEW_SORTTHREAD,
		pViewModelManager_.get(),
		ViewSortThreadAction::TYPE_THREAD);
	ADD_ACTION2(ViewSortThreadAction,
		IDM_VIEW_SORTTOGGLETHREAD,
		pViewModelManager_.get(),
		ViewSortThreadAction::TYPE_TOGGLETHREAD);
	ADD_ACTION4(ViewMessageModeAction,
		IDM_VIEW_SOURCEMODE,
		pMessageViewModeHolder_,
		MessageViewMode::MODE_SOURCE,
		MessageViewMode::MODE_RAW,
		true);
	ADD_ACTION1(ViewTemplateAction,
		IDM_VIEW_TEMPLATE,
		pMessageWindow_);
	ADD_ACTION1(ViewZoomAction,
		IDM_VIEW_ZOOM,
		pMessageViewModeHolder_);
}

void qm::MainWindowImpl::initMenuCreators()
{
	pMenuCreatorList_.reset(new MenuCreatorList(this));
	
	ADD_MENUCREATOR2(MoveMenuCreator,
		this,
		pMessageSelectionModel_.get());
	ADD_MENUCREATOR1(FilterMenuCreator,
		pViewModelManager_->getFilterManager());
	ADD_MENUCREATOR1(FontGroupMenuCreator,
		pMessageWindowFontManager_.get());
	ADD_MENUCREATOR1(SortMenuCreator,
		pViewModelManager_.get());
	ADD_MENUCREATOR2(AttachmentMenuCreator,
		pMessageSelectionModel_.get(),
		pSecurityModel_.get());
	ADD_MENUCREATOR2(ViewTemplateMenuCreator,
		pDocument_->getTemplateManager(),
		this);
	ADD_MENUCREATOR3(CreateTemplateMenuCreator,
		pDocument_->getTemplateManager(),
		this,
		false);
	ADD_MENUCREATOR3(CreateTemplateMenuCreator,
		pDocument_->getTemplateManager(),
		this,
		true);
	ADD_MENUCREATOR2(EncodingMenuCreator,
		pProfile_,
		true);
	ADD_MENUCREATOR1(SubAccountMenuCreator,
		this);
	ADD_MENUCREATOR1(GoRoundMenuCreator,
		pGoRound_);
	ADD_MENUCREATOR1(ScriptMenuCreator,
		pDocument_->getScriptManager());
	ADD_MENUCREATOR2(RecentsMenuCreator,
		pDocument_->getRecents(),
		pDocument_);
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
	
	int nFolderComboBoxHeight = 0;
	if (bShowFolderComboBox_) {
		RECT rectFolderComboBox;
		pFolderComboBox_->getWindowRect(&rectFolderComboBox);
		nFolderComboBoxHeight =
			rectFolderComboBox.bottom - rectFolderComboBox.top;
	}
	
	HDWP hdwp = Window::beginDeferWindowPos(5);
	
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
	
	hdwp = pSyncNotificationWindow_->deferWindowPos(hdwp, HWND_TOP,
		cx - SyncNotificationWindow::WIDTH, 0/*nTopBarHeight*/, 0, 0, SWP_NOSIZE);
	
	if (bShowFolderComboBox_) {
		hdwp = pFolderComboBox_->deferWindowPos(hdwp, 0, 0, nTopBarHeight,
			cx, /*nFolderComboBoxHeight*/200, SWP_NOZORDER);
		pFolderComboBox_->showWindow(SW_SHOW);
	}
	else {
		pFolderComboBox_->showWindow(SW_HIDE);
	}
	
	hdwp = pPrimarySplitterWindow_->deferWindowPos(hdwp, 0,
		0, nTopBarHeight + nFolderComboBoxHeight, cx,
		cy - nStatusBarHeight - nTopBarHeight - nFolderComboBoxHeight - nBottomBarHeight,
		SWP_NOZORDER);
	
	Window::endDeferWindowPos(hdwp);
	
	pSplitterHelper_->applyVisibility(SplitterHelper::COMPONENT_FOLDER);
	pSplitterHelper_->applyVisibility(SplitterHelper::COMPONENT_PREVIEW);
	pSplitterHelper_->applyLocation(SplitterHelper::SPLITTER_PRIMARY);
	pSplitterHelper_->applyLocation(SplitterHelper::SPLITTER_SECONDARY);
	
	if (bShowStatusBar_) {
		const double dBase = qs::UIUtil::getLogPixel()/96.0;
		if (pSplitterHelper_->isVisible(SplitterHelper::COMPONENT_PREVIEW)) {
#ifdef _WIN32_WCE_PSPC
			int nWidth[] = {
				cx - static_cast<int>(60*dBase),
				cx - static_cast<int>(40*dBase),
				cx - static_cast<int>(20*dBase),
				-1
			};
#elif defined _WIN32_WCE
			int nWidth[] = {
				cx - static_cast<int>(300*dBase),
				cx - static_cast<int>(280*dBase),
				cx - static_cast<int>(200*dBase),
				cx - static_cast<int>(120*dBase),
				cx - static_cast<int>(40*dBase),
				cx - static_cast<int>(20*dBase),
				-1
			};
#else
			int nWidth[] = {
				cx - static_cast<int>(342*dBase),
				cx - static_cast<int>(318*dBase),
				cx - static_cast<int>(238*dBase),
				cx - static_cast<int>(158*dBase),
				cx - static_cast<int>(78*dBase),
				cx - static_cast<int>(54*dBase),
				cx - static_cast<int>(30*dBase),
				-1
			};
#endif
			pStatusBar_->setParts(nWidth, countof(nWidth));
		}
		else {
#ifdef _WIN32_WCE_PSPC
			int nWidth[] = {
				cx - static_cast<int>(20*dBase),
				-1
			};
#elif defined _WIN32_WCE
			int nWidth[] = {
				cx - static_cast<int>(100*dBase),
				cx - static_cast<int>(80*dBase),
				-1
			};
#else
			int nWidth[] = {
				cx - static_cast<int>(134*dBase),
				cx - static_cast<int>(110*dBase),
				cx - static_cast<int>(30*dBase),
				-1
			};
#endif
			pStatusBar_->setParts(nWidth, countof(nWidth));
		}
	}
	
	bLayouting_ = false;
}

void qm::MainWindowImpl::updateTitleBar()
{
	const Application& app = Application::getApplication();
	wstring_ptr wstrTitle(app.getVersion(L' ', false));
	if (pDocument_->isOffline()) {
		wstring_ptr wstrOffline(loadString(app.getResourceHandle(), IDS_OFFLINE));
		wstrTitle = concat(wstrTitle.get(), L" ", wstrOffline.get());
	}
	pThis_->setWindowText(wstrTitle.get());
}

void qm::MainWindowImpl::updateStatusBar()
{
	assert(::GetCurrentThreadId() == ::GetWindowThreadProcessId(pThis_->getHandle(), 0));
	
	if (bShowStatusBar_)
		pStatusBar_->updateListParts(0);
}

void qm::MainWindowImpl::reloadProfiles(bool bInitialize)
{
	bSaveOnDeactivate_ = pProfile_->getInt(L"Global", L"SaveOnDeactivate") != 0;
#ifndef _WIN32_WCE_PSPC
	bHideWhenMinimized_ = pProfile_->getInt(L"Global", L"HideWhenMinimized") != 0;
#endif
}

void qm::MainWindowImpl::preModalDialog(HWND hwndParent,
										bool bFirst)
{
	if (bFirst) {
		pActionInvoker_->startPending();
		if (hwndParent != pThis_->getHandle())
			pThis_->enableWindow(false);
		bShowingModalDialog_ = true;
	}
}

void qm::MainWindowImpl::postModalDialog(HWND hwndParent,
										 bool bLast)
{
	if (bLast) {
		if (hwndParent != pThis_->getHandle())
			pThis_->enableWindow(true);
		bShowingModalDialog_ = false;
		pActionInvoker_->stopPending();
	}
}

void qm::MainWindowImpl::sizeChanged(const SplitterWindowEvent& event)
{
	if (bCreated_ && !bLayouting_) {
		SplitterWindow* pSplitterWindow = event.getSplitterWindow();
		assert(pSplitterWindow == pPrimarySplitterWindow_ ||
			pSplitterWindow == pSecondarySplitterWindow_);
		SplitterHelper::Splitter splitter = pSplitterWindow == pPrimarySplitterWindow_ ?
			SplitterHelper::SPLITTER_PRIMARY : SplitterHelper::SPLITTER_SECONDARY;
		pSplitterHelper_->saveLocation(splitter);
	}
}

void qm::MainWindowImpl::accountSelected(const FolderModelEvent& event)
{
	pFolderListModel_->setAccount(event.getAccount());
	pViewModelManager_->setCurrentAccount(event.getAccount());
}

void qm::MainWindowImpl::folderSelected(const FolderModelEvent& event)
{
	Folder* pFolder = event.getFolder();
	
	pFolderListModel_->setAccount(0);
	pViewModelManager_->setCurrentFolder(pFolder);
	
	if (!pDocument_->isOffline() &&
		pFolder->getType() == Folder::TYPE_NORMAL &&
		pFolder->isFlag(Folder::FLAG_SYNCABLE) &&
		pFolder->isFlag(Folder::FLAG_ACTIVESYNC))
		pSyncQueue_->pushFolder(static_cast<NormalFolder*>(pFolder), true);
	else if (pFolder->getType() == Folder::TYPE_QUERY &&
		pFolder->isFlag(Folder::FLAG_ACTIVESYNC))
		static_cast<QueryFolder*>(pFolder)->search(pDocument_, pActionInvoker_.get(),
			pThis_->getHandle(), pProfile_, pSecurityModel_->getSecurityMode());
}

std::pair<Account*, Folder*> qm::MainWindowImpl::getFocusedAccountOrFolder()
{
	if (pFolderListWindow_->isActive()) {
		Folder* pFolder = pFolderListModel_->getFocusedFolder();
		return std::make_pair(pFolder ? 0 : pFolderListModel_->getAccount(), pFolder);
	}
	else {
		std::pair<Account*, Folder*> p(pFolderModel_->getTemporary());
		if (p.first || p.second)
			return p;
		return pFolderModel_->getCurrent();
	}
}

void qm::MainWindowImpl::getSelectedFolders(Account::FolderList* pList)
{
	assert(pList);
	assert(pList->empty());
	
	if (pFolderListWindow_->isActive()) {
		pFolderListModel_->getSelectedFolders(pList);
	}
	else {
		std::pair<Account*, Folder*> p(pFolderModel_->getTemporary());
		if (p.first) {
		}
		else if (p.second) {
			pList->push_back(p.second);
		}
		else {
			Folder* pFolder = pFolderModel_->getCurrent().second;
			if (pFolder)
				pList->push_back(pFolder);
		}
	}
}

bool qm::MainWindowImpl::hasSelectedFolder()
{
	if (pFolderListWindow_->isActive())
		return pFolderListModel_->hasSelectedFolder();
	else
		return pFolderModel_->getTemporary().second ||
			pFolderModel_->getCurrent().second;
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
		if (pThis_->isShowPreviewWindow())
			pStatusBar_->updateMessageParts(event.getMessageHolder(), event.getMessage());
	}
}

void qm::MainWindowImpl::statusTextChanged(const MessageWindowStatusTextEvent& event)
{
	if (bShowStatusBar_)
		pStatusBar_->updateListParts(event.getText());
}

void qm::MainWindowImpl::offlineStatusChanged(const DocumentEvent& event)
{
	updateTitleBar();
}

void qm::MainWindowImpl::accountListChanged(const AccountManagerEvent& event)
{
	pFolderModel_->setCurrent(0, 0, false);
	pFolderListModel_->setAccount(0);
	pViewModelManager_->setCurrentAccount(0);
}

#ifndef _WIN32_WCE_PSPC
void qm::MainWindowImpl::showRecentsMenu(bool bHotKey)
{
	if (pThis_->isShowingModalDialog())
		return;
	
#ifdef QMRECENTSWINDOW
	if (pProfile_->getInt(L"RecentsWindow", L"Use") != 0) {
		pRecentsWindowManager_->showPopup(bHotKey);
		return;
	}
#endif
	
	HMENU hmenu = pUIManager_->getMenuManager()->getMenu(L"recents", false, false);
	if (hmenu) {
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
		::TrackPopupMenu(hmenu, nFlags, pt.x, pt.y, 0, pThis_->getHandle(), 0);
		pThis_->postMessage(WM_NULL);
	}
}

void qm::MainWindowImpl::show()
{
	if (!pThis_->isShowingModalDialog())
		pThis_->show();
}
#endif

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

std::auto_ptr<MacroMenuCreator> qm::MainWindowImpl::createMacroMenuCreator()
{
	return std::auto_ptr<MacroMenuCreator>(new MacroMenuCreator(
		pDocument_, pMessageSelectionModel_.get(), pSecurityModel_.get(),
		pProfile_, actionItems, countof(actionItems), pUIManager_->getActionParamMap()));
}


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
	
	if (ppFolder)
		*ppFolder = 0;
	
	if (pMainWindowImpl_->pListWindow_->isActive()) {
		ViewModel* pViewModel = pMainWindowImpl_->pViewModelManager_->getCurrentViewModel();
		if (pViewModel) {
			Lock<ViewModel> lock(*pViewModel);
			pAccountLock->set(pViewModel->getFolder()->getAccount());
			if (ppFolder)
				*ppFolder = pViewModel->getFolder();
			if (pList)
				pViewModel->getSelection(pList);
		}
	}
	else if (pMainWindowImpl_->pMessageWindow_->isActive() && !bListOnly_) {
		MessagePtrLock mpl(pMainWindowImpl_->pPreviewModel_->getCurrentMessage());
		if (mpl) {
			pAccountLock->set(mpl->getAccount());
			if (ppFolder)
				*ppFolder = mpl->getFolder();
			if (pList)
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
	pImpl_->bShowToolbar_ = pProfile->getInt(L"MainWindow", L"ShowToolbar") != 0;
	pImpl_->bShowStatusBar_ = pProfile->getInt(L"MainWindow", L"ShowStatusBar") != 0;
	pImpl_->bShowFolderComboBox_ = pProfile->getInt(L"MainWindow", L"ShowFolderComboBox") != 0;
	pImpl_->bSaveOnDeactivate_ = true;
#ifndef _WIN32_WCE_PSPC
	pImpl_->bHideWhenMinimized_ = false;
#endif
	pImpl_->bImeControl_ = pProfile->getInt(L"Global", L"ImeControl") != 0;
	pImpl_->pProfile_ = pProfile;
	pImpl_->pDocument_ = 0;
	pImpl_->pUIManager_ = 0;
	pImpl_->pPasswordManager_ = 0;
	pImpl_->pSyncManager_ = 0;
	pImpl_->pSyncDialogManager_ = 0;
	pImpl_->pSyncQueue_ = 0;
	pImpl_->pGoRound_ = 0;
	pImpl_->pTempFileCleaner_ = 0;
	pImpl_->pAutoPilot_ = 0;
	pImpl_->pUpdateChecker_ = 0;
	pImpl_->pPrimarySplitterWindow_ = 0;
	pImpl_->pSecondarySplitterWindow_ = 0;
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
	pImpl_->bShowingModalDialog_ = false;
	pImpl_->hwndLastFocused_ = 0;
	
	pImpl_->reloadProfiles(false);
}

qm::MainWindow::~MainWindow()
{
	delete pImpl_;
}

FolderModel* qm::MainWindow::getFolderModel() const
{
	return pImpl_->pFolderModel_.get();
}

SecurityModel* qm::MainWindow::getSecurityModel() const
{
	return pImpl_->pSecurityModel_.get();
}

const ActionInvoker* qm::MainWindow::getActionInvoker() const
{
	return pImpl_->pActionInvoker_.get();
}

bool qm::MainWindow::isShowingModalDialog() const
{
	return pImpl_->bShowingModalDialog_;
}

void qm::MainWindow::initialShow(bool bHidden)
{
	if (bHidden) {
#ifndef _WIN32_WCE_PSPC
		pImpl_->pShellIcon_->showHiddenIcon();
#else
		showWindow(SW_SHOWNOACTIVATE);
#endif
	}
	else {
		showWindow(pImpl_->nInitialShow_);
	}
}

void qm::MainWindow::layout()
{
	pImpl_->layoutChildren();
	pImpl_->pMessageWindow_->layout();
}

void qm::MainWindow::reloadProfiles()
{
	pImpl_->reloadProfiles(false);
}

bool qm::MainWindow::save(bool bForce)
{
	pImpl_->pMessageFrameWindowManager_->save();
	pImpl_->pFolderWindow_->save();
	pImpl_->pFolderListWindow_->save();
	pImpl_->pListWindow_->save();
	pImpl_->pMessageWindow_->save();
	pImpl_->pPreviewModel_->save();
	if (!pImpl_->pViewModelManager_->save(bForce))
		return false;
#ifdef QMTABWINDOW
	pImpl_->pTabWindow_->save();
	if (!pImpl_->pTabModel_->save(bForce))
		return false;
#endif
	
	Profile* pProfile = pImpl_->pProfile_;
	pProfile->setInt(L"MainWindow", L"ShowToolbar", pImpl_->bShowToolbar_);
	pProfile->setInt(L"MainWindow", L"ShowStatusBar", pImpl_->bShowStatusBar_);
	
	pImpl_->pSplitterHelper_->saveLocation(SplitterHelper::SPLITTER_PRIMARY);
	pImpl_->pSplitterHelper_->saveLocation(SplitterHelper::SPLITTER_SECONDARY);
	pImpl_->pSplitterHelper_->save();
	
	pProfile->setInt(L"MainWindow", L"ShowFolderComboBox", pImpl_->bShowFolderComboBox_);
	
	UIUtil::saveWindowPlacement(getHandle(), pProfile, L"MainWindow");
	
	pProfile->setInt(L"MainWindow", L"SecurityMode", pImpl_->pSecurityModel_->getSecurityMode());
	
	FrameWindow::save();
	
	return true;
}

#ifndef _WIN32_WCE_PSPC
void qm::MainWindow::show()
{
	if (!isHidden())
		return;
	
	setForegroundWindow();
	showWindow(isIconic() ? SW_RESTORE : SW_SHOW);
	
	pImpl_->pMessageFrameWindowManager_->showAll();
	pImpl_->pEditFrameWindowManager_->showAll();
	pImpl_->pAddressBookFrameWindowManager_->showAll();
	
	pImpl_->pShellIcon_->hideHiddenIcon();
}

void qm::MainWindow::hide()
{
	if (isHidden())
		return;
	
	pImpl_->pSyncDialogManager_->hide();
	pImpl_->pMessageFrameWindowManager_->hideAll();
	pImpl_->pEditFrameWindowManager_->hideAll();
	pImpl_->pAddressBookFrameWindowManager_->hideAll();
	
	showWindow(SW_HIDE);
	
	pImpl_->pShellIcon_->showHiddenIcon();
}

bool qm::MainWindow::isHidden() const
{
	return !isVisible();
}

void qm::MainWindow::activate()
{
	if (isHidden()) {
		show();
	}
	else {
		if (isIconic())
			showWindow(SW_RESTORE);
		setWindowPos(HWND_TOP, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
		setForegroundWindow();
	}
}
#endif // _WIN32_WCE_PSPC

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
	return pImpl_->pSplitterHelper_->isVisible(SplitterHelper::COMPONENT_FOLDER);
}

void qm::MainWindow::setShowFolderWindow(bool bShow)
{
	if (pImpl_->pSplitterHelper_->setVisible(SplitterHelper::COMPONENT_FOLDER, bShow)) {
		pImpl_->bShowFolderComboBox_ = !bShow;
		pImpl_->layoutChildren();
	}
}

bool qm::MainWindow::isShowPreviewWindow() const
{
	return pImpl_->pSplitterHelper_->isVisible(SplitterHelper::COMPONENT_PREVIEW);
}

void qm::MainWindow::setShowPreviewWindow(bool bShow)
{
	if (pImpl_->pSplitterHelper_->setVisible(SplitterHelper::COMPONENT_PREVIEW, bShow)) {
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
	pImpl_->pDocument_->getRecents()->removeSeens();
	
	if (pImpl_->pUpdateChecker_->isUpdated()) {
		HINSTANCE hInst = Application::getApplication().getResourceHandle();
		if (messageBox(hInst, IDS_CONFIRM_UPDATE, MB_YESNO, getHandle()) == IDYES)
			UIUtil::openURL(L"http://q3.snak.org/download/", getHandle());
		pImpl_->pUpdateChecker_->clearUpdated();
	}
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
	_snwprintf(wszKey, countof(wszKey), L"CommandBandsRestoreInfo%d", n);
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
	_snwprintf(wszKey, countof(wszKey), L"CommandBandsRestoreInfo%d", n);
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

const DynamicMenuItem* qm::MainWindow::getDynamicMenuItem(unsigned int nId) const
{
	return pImpl_->pUIManager_->getDynamicMenuMap()->getItem(nId);
}

DynamicMenuCreator* qm::MainWindow::getDynamicMenuCreator(const DynamicMenuItem* pItem)
{
	return pImpl_->pMenuCreatorList_->get(pItem);
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
	
#if defined _WIN32_WCE && _WIN32_WCE >= 0x300 && defined _WIN32_WCE_PSPC
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

std::auto_ptr<ActionParam> qm::MainWindow::getActionParam(UINT nId)
{
	return pImpl_->pUIManager_->getActionParamMap()->getActionParam(nId);
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
		HANDLE_INITMENUPOPUP()
#ifndef _WIN32_WCE
		HANDLE_QUERYENDSESSION()
#endif
		HANDLE_SIZE()
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
				Application::getApplication().save(false);
		}
	}
	else {
		HWND hwndFocus = pImpl_->hwndLastFocused_;
		if (!hwndFocus)
			hwndFocus = pImpl_->pListWindow_->getHandle();
		::SetFocus(hwndFocus);
		
		if (pImpl_->bImeControl_) {
#ifdef _WIN32_WCE_PSPC
			qs::UIUtil::setImeStatus(getHandle(), IME_CMODE_NOCONVERSION);
#else
			qs::UIUtil::setImeEnabled(getHandle(), false);
#endif
		}
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
	if (Application::getApplication().isShutdown())
		return 0;
	
#ifdef _WIN32_WCE_PSPC
	if ((pData->dwData & 0xffff0000) == 0)
		setForegroundWindow();
#endif
	
	UINT nId = static_cast<UINT>(pData->dwData & 0xffff);
	const WCHAR* pwszParam = static_cast<const WCHAR*>(pData->lpData);
	if (pwszParam) {
		typedef std::vector<const WCHAR*> ParamList;
		ParamList listParam;
		const WCHAR* p = pwszParam;
		size_t nSize = pData->cbData/sizeof(WCHAR);
		while (static_cast<size_t>(p - pwszParam) < nSize) {
			listParam.push_back(p);
			p += wcslen(p) + 1;
		}
		pImpl_->pActionInvoker_->invoke(nId, &listParam[0], listParam.size());
	}
	else {
		pImpl_->pActionInvoker_->invoke(nId, 0, 0);
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
	pImpl_->pSyncQueue_ = pContext->pSyncQueue_;
	pImpl_->pGoRound_ = pContext->pGoRound_;
	pImpl_->pTempFileCleaner_ = pContext->pTempFileCleaner_;
	pImpl_->pAutoPilot_ = pContext->pAutoPilot_;
	pImpl_->pUpdateChecker_ = pContext->pUpdateChecker_;
	pImpl_->pFolderImage_ = pContext->pFolderImage_;
	
	CustomAcceleratorFactory acceleratorFactory;
	pImpl_->pAccelerator_ = pImpl_->pUIManager_->getKeyMap()->createAccelerator(
		&acceleratorFactory, L"MainWindow");
	if (!pImpl_->pAccelerator_.get())
		return -1;
	
	pImpl_->pActionMap_.reset(new ActionMap());
	pImpl_->pActionInvoker_.reset(new ActionInvoker(pImpl_->pActionMap_.get()));
	pImpl_->pFindReplaceManager_.reset(new FindReplaceManager());
	
	pImpl_->pSplitterHelper_.reset(new SplitterHelper(pImpl_->pProfile_));
	
	pImpl_->pFolderModel_.reset(new DefaultFolderModel());
#ifdef QMTABWINDOW
	pImpl_->pTabModel_.reset(new DefaultTabModel(pImpl_->pDocument_, pImpl_->pProfile_,
		Application::getApplication().getProfilePath(FileNames::TABS_XML).get()));
#endif
	pImpl_->pFolderListModel_.reset(new FolderListModel());
	pImpl_->pEncodingModel_.reset(new DefaultEncodingModel());
	pImpl_->pSecurityModel_.reset(new DefaultSecurityModel(
		pImpl_->pProfile_->getInt(L"MainWindow", L"SecurityMode")));
	pImpl_->pViewModelManager_.reset(new ViewModelManager(pImpl_->pDocument_,
		pImpl_->pProfile_, pImpl_->pSecurityModel_.get()));
	pImpl_->pPreviewModel_.reset(new PreviewMessageModel(
		pImpl_->pViewModelManager_.get(), pImpl_->pProfile_,
		pImpl_->pSplitterHelper_->isVisible(SplitterHelper::COMPONENT_PREVIEW)));
	pImpl_->pOptionDialogManager_.reset(new OptionDialogManager(pImpl_->pDocument_,
		pImpl_->pGoRound_, pImpl_->pViewModelManager_->getFilterManager(),
		pImpl_->pViewModelManager_->getColorManager(), pImpl_->pSyncManager_,
		pImpl_->pAutoPilot_->getAutoPilotManager(), pImpl_->pUpdateChecker_, pImpl_->pProfile_));
	pImpl_->pEditFrameWindowManager_.reset(new EditFrameWindowManager(
		pImpl_->pDocument_, pImpl_->pUIManager_, pImpl_->pPasswordManager_,
		pImpl_->pSyncManager_, pImpl_->pSyncDialogManager_,
		pImpl_->pOptionDialogManager_.get(), pImpl_->pProfile_,
		pImpl_->pSecurityModel_.get()));
	pImpl_->pAddressBookFrameWindowManager_.reset(new AddressBookFrameWindowManager(
		pImpl_->pDocument_->getAddressBook(), pImpl_->pUIManager_, pImpl_->pProfile_));
	pImpl_->pExternalEditorManager_.reset(new ExternalEditorManager(
		pImpl_->pDocument_, pImpl_->pPasswordManager_, pImpl_->pProfile_,
		getHandle(), pImpl_->pTempFileCleaner_, pImpl_, pImpl_->pSecurityModel_.get()));
	pImpl_->pMessageWindowFontManager_.reset(new MessageWindowFontManager(
		Application::getApplication().getProfilePath(FileNames::FONTS_XML).get()));
	pImpl_->pMessageFrameWindowManager_.reset(new MessageFrameWindowManager(
		pImpl_->pDocument_, pImpl_->pUIManager_, pImpl_->pSyncManager_,
		pImpl_->pTempFileCleaner_, pImpl_->pFolderImage_, pImpl_->pProfile_,
		pImpl_->pViewModelManager_.get(), pImpl_->pEditFrameWindowManager_.get(),
		pImpl_->pExternalEditorManager_.get(), pImpl_->pMessageWindowFontManager_.get()));
	pImpl_->pMessageSelectionModel_.reset(
		new MainWindowImpl::MessageSelectionModelImpl(pImpl_, false));
	pImpl_->pListOnlyMessageSelectionModel_.reset(
		new MainWindowImpl::MessageSelectionModelImpl(pImpl_, true));
	pImpl_->pDelayedFolderModelHandler_.reset(new DelayedFolderModelHandler(pImpl_));
	
	DWORD dwStyle = WS_CHILD | WS_VISIBLE | WS_CLIPCHILDREN | WS_CLIPSIBLINGS;
#if defined _WIN32_WCE && _WIN32_WCE >= 0x300 && defined _WIN32_WCE_PSPC
	DWORD dwExStyle = 0;
#else
	DWORD dwExStyle = WS_EX_CLIENTEDGE;
#endif
	
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
	
	bool bVerticalPrimary = pImpl_->pSplitterHelper_->getType(
		SplitterHelper::SPLITTER_PRIMARY) == SplitterHelper::TYPE_VERTICAL;
	std::auto_ptr<SplitterWindow> pPrimarySplitterWindow(new SplitterWindow(
		bVerticalPrimary ? 1 : 2, bVerticalPrimary ? 2 : 1, true, pImpl_));
	if (!pPrimarySplitterWindow->create(L"QmPrimarySplitterWindow", 0, dwStyle,
		CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
		getHandle(), 0, 0, MainWindowImpl::ID_PRIMARYSPLITTERWINDOW, 0))
		return -1;
	pImpl_->pPrimarySplitterWindow_ = pPrimarySplitterWindow.release();
	
	bool bVerticalSecondary = pImpl_->pSplitterHelper_->getType(
		SplitterHelper::SPLITTER_SECONDARY) == SplitterHelper::TYPE_VERTICAL;
	std::auto_ptr<SplitterWindow> pSecondarySplitterWindow(new SplitterWindow(
		bVerticalSecondary ? 1 : 2, bVerticalSecondary ? 2 : 1, true, pImpl_));
	if (!pSecondarySplitterWindow->create(L"QmSecondarySplitterWindow", 0, dwStyle,
		CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
		getHandle(), 0, 0, MainWindowImpl::ID_SECONDARYSPLITTERWINDOW, 0))
		return -1;
	pImpl_->pSecondarySplitterWindow_ = pSecondarySplitterWindow.release();
	
	pImpl_->pSplitterHelper_->setWindows(
		pImpl_->pPrimarySplitterWindow_, pImpl_->pSecondarySplitterWindow_);
	
	SplitterWindow* pFolderSplitterWindow = pImpl_->pSplitterHelper_->getSplitterWindow(
		SplitterHelper::COMPONENT_FOLDER);
	std::auto_ptr<FolderWindow> pFolderWindow(new FolderWindow(
		pFolderSplitterWindow, pImpl_->pFolderModel_.get(), pImpl_->pProfile_));
	FolderWindowCreateContext folderWindowContext = {
		pContext->pDocument_,
		pContext->pDocument_->getUndoManager(),
		pContext->pUIManager_,
		pContext->pFolderImage_,
		pContext->pSyncManager_
	};
	if (!pFolderWindow->create(L"QmFolderWindow", 0, dwStyle,
		CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
		pFolderSplitterWindow->getHandle(), dwExStyle, 0,
		MainWindowImpl::ID_FOLDERWINDOW, &folderWindowContext))
		return -1;
	pImpl_->pFolderWindow_ = pFolderWindow.release();
	
	SplitterWindow* pListSplitterWindow = pImpl_->pSplitterHelper_->getSplitterWindow(
		SplitterHelper::COMPONENT_LIST);
#ifdef QMTABWINDOW
	std::auto_ptr<TabWindow> pTabWindow(new TabWindow(
		pImpl_->pTabModel_.get(), pImpl_->pProfile_));
	TabWindowCreateContext tabContext = {
		pContext->pDocument_,
		pContext->pUIManager_,
		pContext->pFolderImage_
	};
	if (!pTabWindow->create(L"QmTabWindow", 0, dwStyle,
		CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
		pListSplitterWindow->getHandle(), 0, 0,
		MainWindowImpl::ID_TABWINDOW, &tabContext))
		return -1;
	pImpl_->pTabWindow_ = pTabWindow.release();
	HWND hwndListContainerParent = pImpl_->pTabWindow_->getHandle();
#else
	HWND hwndListContainerParent = pListSplitterWindow->getHandle();
#endif
	
	std::auto_ptr<ListContainerWindow> pListContainerWindow(
		new ListContainerWindow(pImpl_->pFolderModel_.get()));
	if (!pListContainerWindow->create(L"QmListContainerWindow", 0, dwStyle,
		CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
		hwndListContainerParent, dwExStyle, 0,
		MainWindowImpl::ID_LISTCONTAINERWINDOW, 0))
		return -1;
	pImpl_->pListContainerWindow_ = pListContainerWindow.release();
#ifdef QMTABWINDOW
	pImpl_->pTabWindow_->setControl(pImpl_->pListContainerWindow_->getHandle());
#endif
	
	std::auto_ptr<FolderListWindow> pFolderListWindow(new FolderListWindow(
		pImpl_->pListContainerWindow_, pImpl_->pFolderListModel_.get(),
		pImpl_->pFolderModel_.get(), pImpl_->pProfile_));
	FolderListWindowCreateContext folderListContext = {
		pContext->pUIManager_,
		pContext->pFolderImage_
	};
	if (!pFolderListWindow->create(L"QmFolderListWindow", 0, dwStyle,
		CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
		pImpl_->pListContainerWindow_->getHandle(), 0, 0,
		MainWindowImpl::ID_FOLDERLISTWINDOW, &folderListContext))
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
	if (!pListWindow->create(L"QmListWindow", 0, dwStyle,
		CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
		pImpl_->pListContainerWindow_->getHandle(), 0, 0,
		MainWindowImpl::ID_LISTWINDOW, &listContext))
		return -1;
	pImpl_->pListWindow_ = pListWindow.release();
	pImpl_->pListContainerWindow_->setListWindow(pImpl_->pListWindow_);
	
	SplitterWindow* pPreviewSplitterWindow = pImpl_->pSplitterHelper_->getSplitterWindow(
		SplitterHelper::COMPONENT_PREVIEW);
	std::auto_ptr<MessageWindow> pMessageWindow(new MessageWindow(
		pImpl_->pPreviewModel_.get(), pImpl_->pProfile_, L"PreviewWindow"));
	pImpl_->pMessageViewModeHolder_ =
		pImpl_->pProfile_->getInt(L"Global", L"SaveMessageViewModePerFolder") != 0 ?
		pImpl_->pPreviewModel_.get() : pMessageWindow->getMessageViewModeHolder();
	MessageWindowCreateContext messageContext = {
		pContext->pDocument_,
		pContext->pUIManager_,
		pImpl_->pMessageViewModeHolder_,
		pImpl_->pEncodingModel_.get(),
		pImpl_->pSecurityModel_.get(),
		pImpl_->pActionInvoker_.get(),
		pImpl_->pMessageWindowFontManager_.get()
	};
	if (!pMessageWindow->create(L"QmMessageWindow", 0, dwStyle,
		CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
		pPreviewSplitterWindow->getHandle(), 0, 0,
		MainWindowImpl::ID_MESSAGEWINDOW, &messageContext))
		return -1;
	pImpl_->pMessageWindow_ = pMessageWindow.release();
	
#ifdef QMTABWINDOW
	pImpl_->pSplitterHelper_->addComponents(pImpl_->pFolderWindow_,
		pImpl_->pTabWindow_, pImpl_->pMessageWindow_);
#else
	pImpl_->pSplitterHelper_->addComponents(pImpl_->pFolderWindow_,
		pImpl_->pListContainerWindow_, pImpl_->pMessageWindow_);
#endif
	
	DWORD dwStatusBarStyle = dwStyle;
#if _WIN32_WCE >= 0x300 && defined _WIN32_WCE_PSPC
	dwStatusBarStyle |= CCS_NOPARENTALIGN;
#endif
	std::auto_ptr<MainWindowStatusBar> pStatusBar(new MainWindowStatusBar(
		pImpl_->pDocument_, pImpl_->pViewModelManager_.get(),
		pImpl_->pFolderModel_.get(), pImpl_->pSyncManager_,
		pImpl_->pMessageWindow_, pImpl_->pEncodingModel_.get(), 2,
		pImpl_->pUIManager_->getMenuManager()));
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
	pImpl_->updateTitleBar();
	
	pImpl_->pFolderModel_->addFolderModelHandler(pImpl_->pDelayedFolderModelHandler_.get());
	pImpl_->pDocument_->addDocumentHandler(pImpl_);
	pImpl_->pDocument_->addAccountManagerHandler(pImpl_);
#ifdef QMTABWINDOW
	pImpl_->pViewModelManager_->addViewModelManagerHandler(pImpl_);
	pImpl_->pTabModel_->addTabModelHandler(pImpl_);
#endif
	pImpl_->pMessageWindow_->addMessageWindowHandler(pImpl_);
	
	pImpl_->initActions();
	pImpl_->initMenuCreators();
	
#ifndef _WIN32_WCE_PSPC
	pImpl_->pShellIcon_.reset(new ShellIcon(pImpl_->pDocument_->getRecents(),
		pImpl_->pSyncManager_, pImpl_->pProfile_, getHandle(), pImpl_));
#endif
	
#ifdef QMRECENTSWINDOW
	pImpl_->pRecentsWindowManager_.reset(new RecentsWindowManager(
		pImpl_->pDocument_->getRecents(), pImpl_->pDocument_,
		pImpl_->pActionMap_.get(), pImpl_->pFolderImage_,
		pImpl_->pProfile_, getHandle()));
#endif
	
	pImpl_->pOptionDialogManager_->initUIs(this, pImpl_->pFolderWindow_,
		pImpl_->pFolderComboBox_, pImpl_->pListWindow_,
		pImpl_->pFolderListWindow_, pImpl_->pMessageWindow_,
		pImpl_->pMessageFrameWindowManager_.get(),
		pImpl_->pEditFrameWindowManager_.get(),
#ifdef QMTABWINDOW
		pImpl_->pTabWindow_,
#endif
#ifdef QMRECENTSWINDOW
		pImpl_->pRecentsWindowManager_.get(),
#endif
		pImpl_->pAddressBookFrameWindowManager_.get());
	
#if !defined _WIN32_WCE && _WIN32_WINNT >= 0x500
	UIUtil::setWindowAlpha(getHandle(), pImpl_->pProfile_, L"MainWindow");
#endif
	
	InitThread::getInitThread().addModalHandler(pImpl_);
	
	pImpl_->bCreated_ = true;
	
	return 0;
}

LRESULT qm::MainWindow::onDestroy()
{
	InitThread::getInitThread().removeModalHandler(pImpl_);
	
#ifdef QMRECENTSWINDOW
	pImpl_->pRecentsWindowManager_.reset(0);
#endif
	
#ifndef _WIN32_WCE_PSPC
	pImpl_->pShellIcon_.reset(0);
#endif
	
	pImpl_->pMessageWindow_->removeMessageWindowHandler(pImpl_);
	pImpl_->pFolderModel_->removeFolderModelHandler(
		pImpl_->pDelayedFolderModelHandler_.get());
	pImpl_->pDocument_->removeDocumentHandler(pImpl_);
	pImpl_->pDocument_->removeAccountManagerHandler(pImpl_);
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
		(nFlags == SIZE_RESTORED || nFlags == SIZE_MAXIMIZED)) {
		pImpl_->layoutChildren(cx, cy);
	}
#ifndef _WIN32_WCE_PSPC
	else if (nFlags == SIZE_MINIMIZED && pImpl_->bHideWhenMinimized_) {
		hide();
		return 1;
	}
#endif
	return FrameWindow::onSize(nFlags, cx, cy);
}


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
		HANDLE_LBUTTONUP()
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

LRESULT qm::SyncNotificationWindow::onLButtonUp(UINT nFlags,
												const POINT& pt)
{
	SyncDialog* pDialog = pSyncDialogManager_->open();
	if (pDialog)
		pDialog->show();
	return DefaultWindowHandler::onLButtonUp(nFlags, pt);
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
											 MenuManager* pMenuManager) :
	MessageStatusBar(pMessageWindow, pEncodingModel, nOffset, pMenuManager),
	pDocument_(pDocument),
	pViewModelManager_(pViewModelManager),
	pFolderModel_(pFolderModel),
	pSyncManager_(pSyncManager),
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
#ifndef _WIN32_WCE_PSPC
				wstring_ptr wstrTemplate(loadString(hInst, IDS_STATUS_VIEWMODELTEMPLATE));
#else
				wstring_ptr wstrTemplate(loadString(hInst, IDS_STATUS_VIEWMODELTEMPLATESHORT));
#endif
				WCHAR wsz[256];
				_snwprintf(wsz, countof(wsz), wstrTemplate.get(), nCount_, nUnseenCount_, nSelectedCount_);
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
				wstrFilter_ = loadString(hInst, IDS_STATUS_CUSTOM);
		}
		else {
			wstrFilter_ = loadString(hInst, IDS_STATUS_NONE);
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
		wstring_ptr wstrOnline(loadString(hInst, bOffline ? IDS_STATUS_OFFLINE : IDS_STATUS_ONLINE));
		setIconOrText(1, bOffline ? IDI_OFFLINE : IDI_ONLINE, wstrOnline.get());
		offline_ = offline;
	}
}

LRESULT qm::MainWindowStatusBar::windowProc(UINT uMsg,
											WPARAM wParam,
											LPARAM lParam)
{
	BEGIN_MESSAGE_HANDLER()
		HANDLE_LBUTTONUP()
	END_MESSAGE_HANDLER()
	return MessageStatusBar::windowProc(uMsg, wParam, lParam);
}

LRESULT qm::MainWindowStatusBar::onLButtonUp(UINT nFlags,
											 const POINT& pt)
{
	int nPart = getPart(pt);
	if (nPart == 1)
		FileOfflineAction::toggleOffline(pDocument_, pSyncManager_);
	
	return MessageStatusBar::onLButtonUp(nFlags, pt);
}

const WCHAR* qm::MainWindowStatusBar::getMenuName(int nPart)
{
	if (nPart == 2)
		return L"filter";
	else
		return MessageStatusBar::getMenuName(nPart);
}


#ifndef _WIN32_WCE_PSPC

/****************************************************************************
 *
 * ShellIcon
 *
 */

qm::ShellIcon::ShellIcon(Recents* pRecents,
						 SyncManager* pSyncManager,
						 Profile* pProfile,
						 HWND hwnd,
						 ShellIconCallback* pCallback) :
	WindowBase(false),
	pRecents_(pRecents),
	pSyncManager_(pSyncManager),
	pProfile_(pProfile),
	pCallback_(pCallback),
	hImageList_(0),
	nState_(0),
	nSync_(0),
	nIconIndex_(0)
{
	hImageList_ = UIUtil::createImageListFromFile(
		FileNames::NOTIFY_BMP, 16, CLR_DEFAULT);
	
	notifyIcon_.cbSize = sizeof(notifyIcon_);
	notifyIcon_.hWnd = hwnd;
	notifyIcon_.uID = ID_NOTIFYICON;
	notifyIcon_.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
	notifyIcon_.uCallbackMessage = WM_SHELLICON_NOTIFYICON;
	notifyIcon_.hIcon = 0;
	_tcscpy(notifyIcon_.szTip, _T("QMAIL"));
	
	setWindowHandler(this, false);
	
	subclassWindow(hwnd);
	
	pRecents_->addRecentsHandler(this);
	pSyncManager_->addSyncManagerHandler(this);
	
	UINT nHotKeyModifier = pProfile->getInt(L"Recents", L"HotKeyModifiers");
	UINT nHotKey = pProfile->getInt(L"Recents", L"HotKey");
	::RegisterHotKey(hwnd, HOTKEY_RECENTS, nHotKeyModifier, nHotKey);
}

qm::ShellIcon::~ShellIcon()
{
	::UnregisterHotKey(getHandle(), HOTKEY_RECENTS);
	
	pRecents_->removeRecentsHandler(this);
	pSyncManager_->removeSyncManagerHandler(this);
	unsubclassWindow();
	if (notifyIcon_.hIcon) {
		Shell_NotifyIcon(NIM_DELETE, &notifyIcon_);
		::DestroyIcon(notifyIcon_.hIcon);
	}
	ImageList_Destroy(hImageList_);
}

void qm::ShellIcon::showHiddenIcon()
{
	nState_ |= STATE_HIDDEN;
	updateIcon();
}

void qm::ShellIcon::hideHiddenIcon()
{
	nState_ &= ~STATE_HIDDEN;
	updateIcon();
}

LRESULT qm::ShellIcon::windowProc(UINT uMsg,
								  WPARAM wParam,
								  LPARAM lParam)
{
	BEGIN_MESSAGE_HANDLER()
		HANDLE_HOTKEY()
		HANDLE_TIMER()
		HANDLE_MESSAGE(WM_SHELLICON_NOTIFYICON, onNotifyIcon)
		HANDLE_MESSAGE(WM_SHELLICON_RECENTSCHANGED, onRecentsChanged)
		HANDLE_MESSAGE(WM_SHELLICON_SYNCSTATUSCHANGED, onSyncStatusChanged)
	END_MESSAGE_HANDLER()
	return DefaultWindowHandler::windowProc(uMsg, wParam, lParam);
}

LRESULT qm::ShellIcon::onHotKey(UINT nId,
								UINT nModifier,
								UINT nKey)
{
	if (nId == HOTKEY_RECENTS)
		pCallback_->showRecentsMenu(true);
	return 0;
}

LRESULT qm::ShellIcon::onTimer(UINT_PTR nId)
{
	if (nId == TIMER_ID) {
		if (nIconIndex_ != 0) {
			++nIconIndex_;
			if (nIconIndex_ >= 10)
				nIconIndex_ = 1;
			updateIcon();
		}
	}
	return 0;
}

LRESULT qm::ShellIcon::onNotifyIcon(WPARAM wParam,
									LPARAM lParam)
{
	if (wParam == ID_NOTIFYICON) {
#ifdef _WIN32_WCE
		if (lParam == WM_LBUTTONUP && ::GetAsyncKeyState(VK_MENU))
			lParam = WM_RBUTTONUP;
		bool bShow = lParam == WM_LBUTTONUP && nState_ & STATE_HIDDEN;
#else
		bool bShow = lParam == WM_LBUTTONUP;
#endif
		if (bShow)
			pCallback_->show();
		else if (lParam == WM_LBUTTONUP || lParam == WM_RBUTTONUP)
			pCallback_->showRecentsMenu(false);
	}
	return 0;
}

LRESULT qm::ShellIcon::onRecentsChanged(WPARAM wParam,
										LPARAM lParam)
{
	{
		Lock<Recents> lock(*pRecents_);
		if (pRecents_->getCount() != 0)
			nState_ |= STATE_RECENT;
		else
			nState_ &= ~STATE_RECENT;
	}
	updateIcon();
	
	return 0;
}

LRESULT qm::ShellIcon::onSyncStatusChanged(WPARAM wParam,
										   LPARAM lParam)
{
	bool bStart = wParam != 0;
	if (bStart) {
		if (nSync_++ == 0) {
			nIconIndex_ = 1;
			setTimer(TIMER_ID, TIMER_INTERVAL);
		}
	}
	else {
		if (--nSync_ == 0) {
			killTimer(TIMER_ID);
			nIconIndex_ = 0;
		}
	}
	updateIcon();
	
	return 0;
}

void qm::ShellIcon::recentsChanged(const RecentsEvent& event)
{
	postMessage(WM_SHELLICON_RECENTSCHANGED);
}

void qm::ShellIcon::statusChanged(const SyncManagerEvent& event)
{
	postMessage(WM_SHELLICON_SYNCSTATUSCHANGED,
		event.getStatus() == SyncManagerEvent::STATUS_START);
}

void qm::ShellIcon::updateIcon()
{
	int nIndex = -1;
	
	if (nState_ & STATE_RECENT)
		nIndex = nIconIndex_ + 10;
	else if (nState_ & STATE_HIDDEN)
		nIndex = nIconIndex_;
	
	if (nIndex != -1) {
		HICON hIconOld = notifyIcon_.hIcon;
		notifyIcon_.hIcon = ImageList_GetIcon(hImageList_, nIndex, ILD_NORMAL);
		Shell_NotifyIcon(hIconOld ? NIM_MODIFY : NIM_ADD, &notifyIcon_);
		if (hIconOld)
			::DestroyIcon(hIconOld);
	}
	else if (notifyIcon_.hIcon) {
		Shell_NotifyIcon(NIM_DELETE, &notifyIcon_);
		::DestroyIcon(notifyIcon_.hIcon);
		notifyIcon_.hIcon = 0;
	}
}


/****************************************************************************
 *
 * ShellIconCallback
 *
 */

qm::ShellIconCallback::~ShellIconCallback()
{
}

#endif // _WIN32_WCE_PSPC


/****************************************************************************
 *
 * SplitterHelper
 *
 */

qm::SplitterHelper::SplitterHelper(Profile* pProfile) :
	pProfile_(pProfile)
{
	assert(pProfile);
	
	wstring_ptr wstrPlacement(pProfile->getString(L"MainWindow", L"Placement"));
	
	const WCHAR* p = wstrPlacement.get();
	if (wcslen(p) == 7 && *p == L'(' && *(p + 4) == L')' &&
		checkType(*(p + 2)) && checkType(*(p + 5)) &&
		checkComponents(*(p + 1), *(p + 3), *(p + 6))) {
		types_[SPLITTER_PRIMARY] = getType(*(p + 5));
		types_[SPLITTER_SECONDARY] = getType(*(p + 2));
		placements_[COMPONENT_SECONDARY] = PLACEMENT_PRIMARY0;
		placements_[getComponent(*(p + 6))] = PLACEMENT_PRIMARY1;
		placements_[getComponent(*(p + 1))] = PLACEMENT_SECONDARY0;
		placements_[getComponent(*(p + 3))] = PLACEMENT_SECONDARY1;
	}
	else if (wcslen(p) == 7&& *(p + 2) == L'(' && *(p + 6) == L')' &&
		checkType(*(p + 1)) && checkType(*(p + 4)) &&
		checkComponents(*p, *(p + 3), *(p + 5))) {
		types_[SPLITTER_PRIMARY] = getType(*(p + 1));
		types_[SPLITTER_SECONDARY] = getType(*(p + 4));
		placements_[getComponent(*p)] = PLACEMENT_PRIMARY0;
		placements_[COMPONENT_SECONDARY] = PLACEMENT_PRIMARY1;
		placements_[getComponent(*(p + 3))] = PLACEMENT_SECONDARY0;
		placements_[getComponent(*(p + 5))] = PLACEMENT_SECONDARY1;
	}
	else {
		types_[SPLITTER_PRIMARY] = TYPE_HORIZONTAL;
		types_[SPLITTER_SECONDARY] = TYPE_VERTICAL;
		placements_[COMPONENT_SECONDARY] = PLACEMENT_PRIMARY1;
		placements_[COMPONENT_FOLDER] = PLACEMENT_PRIMARY0;
		placements_[COMPONENT_LIST] = PLACEMENT_SECONDARY0;
		placements_[COMPONENT_PREVIEW] = PLACEMENT_SECONDARY1;
	}
	
	pSplitterWindow_[SPLITTER_PRIMARY] = 0;
	pSplitterWindow_[SPLITTER_SECONDARY] = 0;
	nLocations_[SPLITTER_PRIMARY] = pProfile->getInt(L"MainWindow", L"PrimaryLocation");
	nLocations_[SPLITTER_SECONDARY] = pProfile->getInt(L"MainWindow", L"SecondaryLocation");
	bVisible_[COMPONENT_FOLDER] = pProfile->getInt(L"MainWindow", L"ShowFolderWindow") != 0;
	bVisible_[COMPONENT_LIST] = true;
	bVisible_[COMPONENT_PREVIEW] = pProfile->getInt(L"MainWindow", L"ShowPreviewWindow") != 0;
}

qm::SplitterHelper::~SplitterHelper()
{
}

SplitterHelper::Type qm::SplitterHelper::getType(Splitter splitter) const
{
	return types_[splitter];
}

SplitterHelper::Placement qm::SplitterHelper::getPlacement(Component component) const
{
	return placements_[component];
}

int qm::SplitterHelper::getLocation(Splitter splitter) const
{
	return nLocations_[splitter];
}

void qm::SplitterHelper::setWindows(SplitterWindow* pPrimarySplitterWindow,
									SplitterWindow* pSecondarySplitterWindow)
{
	assert(pSplitterWindow_[SPLITTER_PRIMARY] == 0 &&
		pSplitterWindow_[SPLITTER_SECONDARY] == 0);
	
	pSplitterWindow_[SPLITTER_PRIMARY] = pPrimarySplitterWindow;
	pSplitterWindow_[SPLITTER_SECONDARY] = pSecondarySplitterWindow;
}

SplitterWindow* qm::SplitterHelper::getSplitterWindow(Component component) const
{
	assert(pSplitterWindow_[SPLITTER_PRIMARY] != 0 &&
		pSplitterWindow_[SPLITTER_SECONDARY] != 0);
	
	return pSplitterWindow_[getSplitter(component)];
}

void qm::SplitterHelper::addComponents(Window* pFolderWindow,
									   Window* pListWindow,
									   Window* pPreviewWindow)
{
	assert(pSplitterWindow_[SPLITTER_PRIMARY] != 0 &&
		pSplitterWindow_[SPLITTER_SECONDARY] != 0);
	
	addComponent(COMPONENT_FOLDER, pFolderWindow);
	addComponent(COMPONENT_LIST, pListWindow);
	addComponent(COMPONENT_PREVIEW, pPreviewWindow);
	addComponent(COMPONENT_SECONDARY, pSplitterWindow_[SPLITTER_SECONDARY]);
}

void qm::SplitterHelper::applyLocation(Splitter splitter) const
{
	assert(pSplitterWindow_[SPLITTER_PRIMARY] != 0 &&
		pSplitterWindow_[SPLITTER_SECONDARY] != 0);
	
	SplitterWindow* pWindow = pSplitterWindow_[splitter];
	int nLocation = getLocation(splitter);
	if (getType(splitter) == TYPE_VERTICAL)
		pWindow->setRowHeight(0, nLocation);
	else
		pWindow->setColumnWidth(0, nLocation);
}

void qm::SplitterHelper::saveLocation(Splitter splitter)
{
	assert(pSplitterWindow_[SPLITTER_PRIMARY] != 0 &&
		pSplitterWindow_[SPLITTER_SECONDARY] != 0);
	
	SplitterWindow* pWindow = pSplitterWindow_[splitter];
	bool bVertical = getType(splitter) == TYPE_VERTICAL;
	if (pWindow->isShowPane(0, 0) &&
		pWindow->isShowPane(bVertical ? 0 : 1, bVertical ? 1 : 0))
		nLocations_[splitter] = bVertical ?
			pWindow->getRowHeight(0) : pWindow->getColumnWidth(0);
}

bool qm::SplitterHelper::isVisible(Component component) const
{
	return bVisible_[component];
}

bool qm::SplitterHelper::setVisible(Component component,
									bool bVisible)
{
	assert(pSplitterWindow_[SPLITTER_PRIMARY] != 0 &&
		pSplitterWindow_[SPLITTER_SECONDARY] != 0);
	
	if (bVisible == bVisible_[component])
		return false;
	
	saveLocation(getSplitter(component));
	bVisible_[component] = bVisible;
	
	return true;
}

void qm::SplitterHelper::applyVisibility(Component component) const
{
	assert(pSplitterWindow_[SPLITTER_PRIMARY] != 0 &&
		pSplitterWindow_[SPLITTER_SECONDARY] != 0);
	
	bool bVisible = bVisible_[component];
	SplitterWindow* pWindow = getSplitterWindow(component);
	std::pair<int, int> pane(getPane(component));
	Component opposite = getOppositeComponent(component);
	std::pair<int, int> oppositePane(getPane(opposite));
	if (bVisible) {
		pWindow->showPane(pane.first, pane.second, true);
		if (pWindow == pSplitterWindow_[SPLITTER_SECONDARY]) {
			std::pair<int, int> secondaryPane(getPane(COMPONENT_SECONDARY));
			if (!pSplitterWindow_[SPLITTER_PRIMARY]->isShowPane(
				secondaryPane.first, secondaryPane.second)) {
				pWindow->showPane(oppositePane.first, oppositePane.second, bVisible_[opposite]);
				pSplitterWindow_[SPLITTER_PRIMARY]->showPane(
					secondaryPane.first, secondaryPane.second, true);
			}
		}
	}
	else {
		if (pWindow->isShowPane(oppositePane.first, oppositePane.second)) {
			pWindow->showPane(pane.first, pane.second, false);
		}
		else {
			assert(pWindow == pSplitterWindow_[SPLITTER_SECONDARY]);
			std::pair<int, int> secondaryPane(getPane(COMPONENT_SECONDARY));
			pSplitterWindow_[SPLITTER_PRIMARY]->showPane(
				secondaryPane.first, secondaryPane.second, false);
		}
	}
}

void qm::SplitterHelper::save() const
{
	pProfile_->setInt(L"MainWindow", L"PrimaryLocation", nLocations_[SPLITTER_PRIMARY]);
	pProfile_->setInt(L"MainWindow", L"SecondaryLocation", nLocations_[SPLITTER_SECONDARY]);
	pProfile_->setInt(L"MainWindow", L"ShowFolderWindow", bVisible_[COMPONENT_FOLDER]);
	pProfile_->setInt(L"MainWindow", L"ShowPreviewWindow", bVisible_[COMPONENT_PREVIEW]);
}

SplitterHelper::Splitter qm::SplitterHelper::getSplitter(Component component) const
{
	Placement p = placements_[component];
	return p & PLACEMENT_PRIMARY ? SPLITTER_PRIMARY : SPLITTER_SECONDARY;
}

std::pair<int, int> qm::SplitterHelper::getPane(Component component) const
{
	Placement p = placements_[component];
	if (p & PLACEMENT_0)
		return std::make_pair(0, 0);
	else if (getType(getSplitter(component)) == TYPE_VERTICAL)
		return std::make_pair(0, 1);
	else
		return std::make_pair(1, 0);
}

void qm::SplitterHelper::addComponent(Component component,
									  Window* pWindow)
{
	assert(pWindow);
	
	SplitterWindow* pSplitterWindow = getSplitterWindow(component);
	std::pair<int, int> pane(getPane(component));
	pSplitterWindow->add(pane.first, pane.second, pWindow);
}

SplitterHelper::Component qm::SplitterHelper::getOppositeComponent(Component component) const
{
	Placement placement = placements_[component];
	Placement oppositePlacement = static_cast<Placement>(
		(placement & PLACEMENT_SPLITTER_MASK) |
		(placement & PLACEMENT_0 ? PLACEMENT_1 : PLACEMENT_0));
	for (int c = 0; c < MAX_COMPONENT; ++c) {
		if (placements_[c] == oppositePlacement)
			return static_cast<Component>(c);
	}
	assert(false);
	return component;
}

bool qm::SplitterHelper::checkType(WCHAR c)
{
	return getType(c) != MAX_TYPE;
}

bool qm::SplitterHelper::checkComponents(WCHAR c0,
										 WCHAR c1,
										 WCHAR c2)
{
	Component components[] = {
		getComponent(c0),
		getComponent(c1),
		getComponent(c2)
	};
	for (int n = 0; n < countof(components); ++n) {
		Component c = components[n];
		if (c == MAX_COMPONENT)
			return false;
		for (int m = 0; m < n; ++m) {
			if (c == components[m])
				return false;
		}
	}
	return true;
}

SplitterHelper::Type qm::SplitterHelper::getType(WCHAR c)
{
	switch (c) {
	case L'|':
		return TYPE_HORIZONTAL;
	case L'-':
		return TYPE_VERTICAL;
	default:
		return MAX_TYPE;
	}
}

SplitterHelper::Component qm::SplitterHelper::getComponent(WCHAR c)
{
	switch (c) {
	case L'F':
		return COMPONENT_FOLDER;
	case L'L':
		return COMPONENT_LIST;
	case L'P':
		return COMPONENT_PREVIEW;
	default:
		return MAX_COMPONENT;
	}
}
