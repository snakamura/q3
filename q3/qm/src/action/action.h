/*
 * $Id$
 *
 * Copyright(C) 1998-2005 Satoshi Nakamura
 * All rights reserved.
 *
 */

#ifndef __ACTION_H__
#define __ACTION_H__

#include <qm.h>
#include <qmaccount.h>
#include <qmmessagewindow.h>
#include <qmsecurity.h>
#include <qmjunk.h>

#include <qsaction.h>
#include <qsstream.h>

#include "attachmenthelper.h"
#include "templateprocessor.h"
#include "../ui/messagewindow.h"
#include "../uimodel/messagecomposer.h"
#include "../uimodel/messageviewmode.h"


namespace qm {

struct ActionParam;
class AttachmentOpenAction;
class AttachmentSaveAction;
class ConfigAutoPilotAction;
class ConfigColorsAction;
class ConfigFiltersAction;
class ConfigGoRoundAction;
class ConfigRulesAction;
class ConfigSignaturesAction;
class ConfigSyncFiltersAction;
class ConfigTextsAction;
class ConfigViewsAction;
class DispatchAction;
class EditClearDeletedAction;
class EditCommandAction;
class EditCopyMessageAction;
class EditCutMessageAction;
class EditDeleteCacheAction;
class EditDeleteMessageAction;
class EditFindAction;
class EditPasteMessageAction;
class EditSelectAllMessageAction;
class EditUndoMessageAction;
class FileCheckAction;
class FileCloseAction;
class FileCompactAction;
class FileDumpAction;
class FileExitAction;
class FileExportAction;
class FileImportAction;
class FileLoadAction;
class FileOfflineAction;
class FilePrintAction;
class FileSalvageAction;
class FileSaveAction;
class FileUninstallAction;
class FolderCreateAction;
class FolderDeleteAction;
class FolderEmptyAction;
class FolderEmptyTrashAction;
class FolderExpandAction;
class FolderPropertyAction;
class FolderRenameAction;
class FolderShowSizeAction;
class FolderUpdateAction;
class MessageApplyRuleAction;
class MessageApplyTemplateAction;
class MessageClearRecentsAction;
class MessageCombineAction;
class MessageCreateAction;
class MessageCreateFromClipboardAction;
class MessageCreateFromFileAction;
class MessageDeleteAttachmentAction;
class MessageDetachAction;
class MessageExpandDigestAction;
class MessageManageJunkAction;
class MessageMarkAction;
class MessageMoveAction;
class MessageMoveOtherAction;
class MessageOpenAttachmentAction;
class MessageOpenLinkAction;
class MessageOpenRecentAction;
class MessageOpenURLAction;
class MessagePropertyAction;
class MessageSearchAction;
#ifdef QMTABWINDOW
class TabCloseAction;
class TabCreateAction;
class TabEditTitleAction;
class TabLockAction;
class TabMoveAction;
class TabNavigateAction;
class TabSelectAction;
#endif
class ToolAccountAction;
class ToolAddAddressAction;
class ToolAddressBookAction;
class ToolAutoPilotAction;
class ToolDialupAction;
class ToolGoRoundAction;
class ToolOptionsAction;
class ToolScriptAction;
class ToolSubAccountAction;
class ToolSyncAction;
class ViewDropDownAction;
class ViewEncodingAction;
class ViewFilterAction;
class ViewFilterCustomAction;
class ViewFilterNoneAction;
class ViewFocusAction;
class ViewLockPreviewAction;
class ViewMessageModeAction;
class ViewNavigateFolderAction;
class ViewNavigateMessageAction;
class ViewOpenLinkAction;
class ViewRefreshAction;
class ViewSecurityAction;
class ViewScrollAction;
class ViewSelectMessageAction;
template<class WindowX> class ViewShowControlAction;
	class ViewShowFolderAction;
	class ViewShowHeaderAction;
	class ViewShowHeaderColumnAction;
	class ViewShowPreviewAction;
	template<class WindowX> class ViewShowStatusBarAction;
#ifdef QMTABWINDOW
	class ViewShowTabAction;
#endif
	template<class WindowX> class ViewShowToolbarAction;
class ViewShowSyncDialogAction;
class ViewSortAction;
class ViewSortDirectionAction;
class ViewSortFloatThreadAction;
class ViewSortThreadAction;
class ViewTemplateAction;
class ActionUtil;
class FolderActionUtil;
class MessageActionUtil;
#ifdef QMTABWINDOW
class TabActionUtil;
#endif

class AddressBookFrameWindowManager;
class AttachmentMenu;
class AttachmentSelectionModel;
class AutoPilot;
class AutoPilotManager;
class ColorManager;
class Document;
class EditFrameWindow;
class EditFrameWindowManager;
class EncodingMenu;
class EncodingModel;
class ExternalEditorManager;
class Filter;
class FilterManager;
class FilterMenu;
class FindReplaceManager;
class FixedFormTextManager;
class FolderComboBox;
class FolderListWindow;
class FolderModel;
class FolderSelectionModel;
class FolderWindow;
class GoRound;
class GoRoundMenu;
class ListWindow;
class MainWindow;
class MessageFrameWindow;
class MessageHolder;
class MessageModel;
class MessagePtr;
class MessageSelectionModel;
class MessageWindow;
class MoveMenu;
class NormalFolder;
class PasswordManager;
class PreviewMessageModel;
class ProgressDialog;
class Recents;
class RecentsMenu;
class RuleManager;
class ScriptMenu;
class SecurityModel;
class SignatureManager;
class SortMenu;
class SubAccountMenu;
class SyncDialogManager;
class SyncFilterManager;
class SyncManager;
#ifdef QMTABWINDOW
class TabModel;
class TabWindow;
#endif
class TempFileCleaner;
class Template;
class TemplateMenu;
class UIManager;
class UndoManager;
class View;
class ViewModel;
class ViewModelHolder;
class ViewModelManager;


/****************************************************************************
 *
 * ActionParam
 *
 */

struct ActionParam
{
	VARIANT** ppvarArgs_;
	size_t nArgs_;
};


/****************************************************************************
 *
 * AttachmentOpenAction
 *
 */

class AttachmentOpenAction : public qs::AbstractAction
{
public:
	AttachmentOpenAction(MessageModel* pMessageModel,
						 AttachmentSelectionModel* pAttachmentSelectionModel,
						 SecurityModel* pSecurityModel,
						 qs::Profile* pProfile,
						 TempFileCleaner* pTempFileCleaner,
						 HWND hwnd);
	virtual ~AttachmentOpenAction();

public:
	virtual void invoke(const qs::ActionEvent& event);
	virtual bool isEnabled(const qs::ActionEvent& event);

private:
	AttachmentOpenAction(const AttachmentOpenAction&);
	AttachmentOpenAction& operator=(const AttachmentOpenAction&);

private:
	MessageModel* pMessageModel_;
	AttachmentSelectionModel* pAttachmentSelectionModel_;
	SecurityModel* pSecurityModel_;
	HWND hwnd_;
	AttachmentHelper helper_;
};


/****************************************************************************
 *
 * AttachmentSaveAction
 *
 */

class AttachmentSaveAction : public qs::AbstractAction
{
public:
	AttachmentSaveAction(MessageModel* pMessageModel,
						 AttachmentSelectionModel* pAttachmentSelectionModel,
						 SecurityModel* pSecurityModel,
						 bool bAll,
						 qs::Profile* pProfile,
						 HWND hwnd);
	virtual ~AttachmentSaveAction();

public:
	virtual void invoke(const qs::ActionEvent& event);
	virtual bool isEnabled(const qs::ActionEvent& event);

private:
	AttachmentSaveAction(const AttachmentSaveAction&);
	AttachmentSaveAction& operator=(const AttachmentSaveAction&);

private:
	MessageModel* pMessageModel_;
	AttachmentSelectionModel* pAttachmentSelectionModel_;
	bool bAll_;
	AttachmentHelper helper_;
	HWND hwnd_;
};


/****************************************************************************
 *
 * ConfigAutoPilotAction
 *
 */

class ConfigAutoPilotAction : public qs::AbstractAction
{
public:
	ConfigAutoPilotAction(AutoPilotManager* pAutoPilotManager,
						  GoRound* pGoRound,
						  HWND hwnd);
	virtual ~ConfigAutoPilotAction();

public:
	virtual void invoke(const qs::ActionEvent& event);

private:
	ConfigAutoPilotAction(const ConfigAutoPilotAction&);
	ConfigAutoPilotAction& operator=(const ConfigAutoPilotAction&);

private:
	AutoPilotManager* pAutoPilotManager_;
	GoRound* pGoRound_;
	HWND hwnd_;
};


/****************************************************************************
 *
 * ConfigColorsAction
 *
 */

class ConfigColorsAction : public qs::AbstractAction
{
public:
	ConfigColorsAction(ColorManager* pColorManager,
					   ViewModelManager* pViewModelManager,
					   AccountManager* pAccountManager,
					   HWND hwnd);
	virtual ~ConfigColorsAction();

public:
	virtual void invoke(const qs::ActionEvent& event);

private:
	ConfigColorsAction(const ConfigColorsAction&);
	ConfigColorsAction& operator=(const ConfigColorsAction&);

private:
	ColorManager* pColorManager_;
	ViewModelManager* pViewModelManager_;
	AccountManager* pAccountManager_;
	HWND hwnd_;
};


/****************************************************************************
 *
 * ConfigFiltersAction
 *
 */

class ConfigFiltersAction : public qs::AbstractAction
{
public:
	ConfigFiltersAction(FilterManager* pFilterManager,
						HWND hwnd);
	virtual ~ConfigFiltersAction();

public:
	virtual void invoke(const qs::ActionEvent& event);

private:
	ConfigFiltersAction(const ConfigFiltersAction&);
	ConfigFiltersAction& operator=(const ConfigFiltersAction&);

private:
	FilterManager* pFilterManager_;
	HWND hwnd_;
};


/****************************************************************************
 *
 * ConfigGoRoundAction
 *
 */

class ConfigGoRoundAction : public qs::AbstractAction
{
public:
	ConfigGoRoundAction(GoRound* pGoRound,
						AccountManager* pAccountManager,
						SyncFilterManager* pSyncFilterManager,
						HWND hwnd);
	virtual ~ConfigGoRoundAction();

public:
	virtual void invoke(const qs::ActionEvent& event);

private:
	ConfigGoRoundAction(const ConfigGoRoundAction&);
	ConfigGoRoundAction& operator=(const ConfigGoRoundAction&);

private:
	GoRound* pGoRound_;
	AccountManager* pAccountManager_;
	SyncFilterManager* pSyncFilterManager_;
	HWND hwnd_;
};


/****************************************************************************
 *
 * ConfigRulesAction
 *
 */

class ConfigRulesAction : public qs::AbstractAction
{
public:
	ConfigRulesAction(RuleManager* pRuleManager,
					  AccountManager* pAccountManager,
					  HWND hwnd);
	virtual ~ConfigRulesAction();

public:
	virtual void invoke(const qs::ActionEvent& event);

private:
	ConfigRulesAction(const ConfigRulesAction&);
	ConfigRulesAction& operator=(const ConfigRulesAction&);

private:
	RuleManager* pRuleManager_;
	AccountManager* pAccountManager_;
	HWND hwnd_;
};


/****************************************************************************
 *
 * ConfigSignaturesAction
 *
 */

class ConfigSignaturesAction : public qs::AbstractAction
{
public:
	ConfigSignaturesAction(SignatureManager* pSignatureManager,
						   AccountManager* pAccountManager,
						   HWND hwnd);
	virtual ~ConfigSignaturesAction();

public:
	virtual void invoke(const qs::ActionEvent& event);

private:
	ConfigSignaturesAction(const ConfigSignaturesAction&);
	ConfigSignaturesAction& operator=(const ConfigSignaturesAction&);

private:
	SignatureManager* pSignatureManager_;
	AccountManager* pAccountManager_;
	HWND hwnd_;
};


/****************************************************************************
 *
 * ConfigSyncFiltersAction
 *
 */

class ConfigSyncFiltersAction : public qs::AbstractAction
{
public:
	ConfigSyncFiltersAction(SyncFilterManager* pManager,
							HWND hwnd);
	virtual ~ConfigSyncFiltersAction();

public:
	virtual void invoke(const qs::ActionEvent& event);

private:
	ConfigSyncFiltersAction(const ConfigSyncFiltersAction&);
	ConfigSyncFiltersAction& operator=(const ConfigSyncFiltersAction&);

private:
	SyncFilterManager* pManager_;
	HWND hwnd_;
};


/****************************************************************************
 *
 * ConfigTextsAction
 *
 */

class ConfigTextsAction : public qs::AbstractAction
{
public:
	ConfigTextsAction(FixedFormTextManager* pManager,
					  HWND hwnd);
	virtual ~ConfigTextsAction();

public:
	virtual void invoke(const qs::ActionEvent& event);

private:
	ConfigTextsAction(const ConfigTextsAction&);
	ConfigTextsAction& operator=(const ConfigTextsAction&);

private:
	FixedFormTextManager* pManager_;
	HWND hwnd_;
};


/****************************************************************************
 *
 * ConfigViewsAction
 *
 */

class ConfigViewsAction : public qs::AbstractAction
{
public:
	ConfigViewsAction(ViewModelManager* pViewModelManager,
					  HWND hwnd);
	virtual ~ConfigViewsAction();

public:
	virtual void invoke(const qs::ActionEvent& event);
	virtual bool isEnabled(const qs::ActionEvent& event);

private:
	ConfigViewsAction(const ConfigViewsAction&);
	ConfigViewsAction& operator=(const ConfigViewsAction&);

private:
	ViewModelManager* pViewModelManager_;
	HWND hwnd_;
};


/****************************************************************************
 *
 * DispatchAction
 *
 */

class DispatchAction : public qs::AbstractAction
{
public:
	DispatchAction(View* pViews[],
				   qs::Action* pActions[],
				   size_t nCount);
	virtual ~DispatchAction();

public:
	virtual void invoke(const qs::ActionEvent& event);
	virtual bool isEnabled(const qs::ActionEvent& event);
	virtual bool isChecked(const qs::ActionEvent& event);
	virtual qs::wstring_ptr getText(const qs::ActionEvent& event);

private:
	qs::Action* getAction() const;

private:
	DispatchAction(const DispatchAction&);
	DispatchAction& operator=(const DispatchAction&);

private:
	struct Item
	{
		View* pView_;
		qs::Action* pAction_;
	};

private:
	typedef std::vector<Item> ItemList;

private:
	ItemList listItem_;
};


/****************************************************************************
 *
 * EditClearDeletedAction
 *
 */

class EditClearDeletedAction : public qs::AbstractAction
{
public:
	EditClearDeletedAction(SyncManager* pSyncManager,
						   Document* pDocument,
						   FolderModel* pFolderModel,
						   SyncDialogManager* pSyncDialogManager,
						   HWND hwnd,
						   qs::Profile* pProfile);
	virtual ~EditClearDeletedAction();

public:
	virtual void invoke(const qs::ActionEvent& event);
	virtual bool isEnabled(const qs::ActionEvent& event);

private:
	EditClearDeletedAction(const EditClearDeletedAction&);
	EditClearDeletedAction& operator=(const EditClearDeletedAction&);

private:
	SyncManager* pSyncManager_;
	Document* pDocument_;
	FolderModel* pFolderModel_;
	SyncDialogManager* pSyncDialogManager_;
	HWND hwnd_;
	qs::Profile* pProfile_;
};


/****************************************************************************
 *
 * EditCommandAction
 *
 */

class EditCommandAction : public qs::AbstractAction
{
public:
	typedef void (MessageWindowItem::*PFN_DO)();
	typedef bool (MessageWindowItem::*PFN_CANDO)();

public:
	EditCommandAction(MessageWindow* pMessageWindow,
					  PFN_DO pfnDo,
					  PFN_CANDO pfnCanDo);
	virtual ~EditCommandAction();

public:
	virtual void invoke(const qs::ActionEvent& event);
	virtual bool isEnabled(const qs::ActionEvent& event);

private:
	EditCommandAction(const EditCommandAction&);
	EditCommandAction& operator=(const EditCommandAction&);

private:
	MessageWindow* pMessageWindow_;
	PFN_DO pfnDo_;
	PFN_CANDO pfnCanDo_;
};


/****************************************************************************
 *
 * EditCopyMessageAction
 *
 */

class EditCopyMessageAction : public qs::AbstractAction
{
public:
	EditCopyMessageAction(AccountManager* pAccountManager,
						  FolderModel* pFolderModel,
						  MessageSelectionModel* pMessageSelectionModel,
						  HWND hwnd);
	virtual ~EditCopyMessageAction();

public:
	virtual void invoke(const qs::ActionEvent& event);
	virtual bool isEnabled(const qs::ActionEvent& event);

private:
	EditCopyMessageAction(const EditCopyMessageAction&);
	EditCopyMessageAction& operator=(const EditCopyMessageAction&);

private:
	AccountManager* pAccountManager_;
	FolderModel* pFolderModel_;
	MessageSelectionModel* pMessageSelectionModel_;
	HWND hwnd_;
};


/****************************************************************************
 *
 * EditCutMessageAction
 *
 */

class EditCutMessageAction : public qs::AbstractAction
{
public:
	EditCutMessageAction(AccountManager* pAccountManager,
						 FolderModel* pFolderModel,
						 MessageSelectionModel* pMessageSelectionModel,
						 HWND hwnd);
	virtual ~EditCutMessageAction();

public:
	virtual void invoke(const qs::ActionEvent& event);
	virtual bool isEnabled(const qs::ActionEvent& event);

private:
	EditCutMessageAction(const EditCutMessageAction&);
	EditCutMessageAction& operator=(const EditCutMessageAction&);

private:
	AccountManager* pAccountManager_;
	FolderModel* pFolderModel_;
	MessageSelectionModel* pMessageSelectionModel_;
	HWND hwnd_;
};


/****************************************************************************
 *
 * EditDeleteCacheAction
 *
 */

class EditDeleteCacheAction : public qs::AbstractAction
{
public:
	EditDeleteCacheAction(MessageSelectionModel* pMessageSelectionModel,
						  HWND hwnd);
	virtual ~EditDeleteCacheAction();

public:
	virtual void invoke(const qs::ActionEvent& event);
	virtual bool isEnabled(const qs::ActionEvent& event);

private:
	EditDeleteCacheAction(const EditDeleteCacheAction&);
	EditDeleteCacheAction& operator=(const EditDeleteCacheAction&);

private:
	MessageSelectionModel* pMessageSelectionModel_;
	HWND hwnd_;
};


/****************************************************************************
 *
 * EditDeleteMessageAction
 *
 */

class EditDeleteMessageAction : public qs::AbstractAction
{
public:
	EditDeleteMessageAction(MessageSelectionModel* pMessageSelectionModel,
							ViewModelHolder* pViewModelHolder,
							MessageModel* pMessageModel,
							bool bDirect,
							bool bDontSelectNextIfDeletedFlag,
							UndoManager* pUndoManager,
							HWND hwnd,
							qs::Profile* pProfile);
	virtual ~EditDeleteMessageAction();

public:
	virtual void invoke(const qs::ActionEvent& event);
	virtual bool isEnabled(const qs::ActionEvent& event);

private:
	bool confirm() const;

private:
	EditDeleteMessageAction(const EditDeleteMessageAction&);
	EditDeleteMessageAction& operator=(const EditDeleteMessageAction&);

private:
	MessageSelectionModel* pMessageSelectionModel_;
	ViewModelHolder* pViewModelHolder_;
	MessageModel* pMessageModel_;
	bool bDirect_;
	bool bDontSelectNextIfDeletedFlag_;
	UndoManager* pUndoManager_;
	HWND hwnd_;
	bool bConfirm_;
};


/****************************************************************************
 *
 * EditFindAction
 *
 */

class EditFindAction : public qs::AbstractAction
{
public:
	enum Type {
		TYPE_NORMAL,
		TYPE_PREV,
		TYPE_NEXT
	};

public:
	EditFindAction(MessageWindow* pMessageWindow,
				   qs::Profile* pProfile,
				   FindReplaceManager* pFindReplaceManager);
	EditFindAction(MessageWindow* pMessageWindow,
				   bool bNext,
				   FindReplaceManager* pFindReplaceManager);
	virtual ~EditFindAction();

public:
	virtual void invoke(const qs::ActionEvent& event);
	virtual bool isEnabled(const qs::ActionEvent& event);

private:
	EditFindAction(const EditFindAction&);
	EditFindAction& operator=(const EditFindAction&);

private:
	MessageWindow* pMessageWindow_;
	qs::Profile* pProfile_;
	FindReplaceManager* pFindReplaceManager_;
	Type type_;
};


/****************************************************************************
 *
 * EditPasteMessageAction
 *
 */

class EditPasteMessageAction : public qs::AbstractAction
{
public:
	EditPasteMessageAction(Document* pDocument,
						   FolderModel* pFolderModel,
						   SyncManager* pSyncManager,
						   SyncDialogManager* pSyncDialogManager,
						   qs::Profile* pProfile,
						   HWND hwnd);
	virtual ~EditPasteMessageAction();

public:
	virtual void invoke(const qs::ActionEvent& event);
	virtual bool isEnabled(const qs::ActionEvent& event);

private:
	EditPasteMessageAction(const EditPasteMessageAction&);
	EditPasteMessageAction& operator=(const EditPasteMessageAction&);

private:
	Document* pDocument_;
	FolderModel* pFolderModel_;
	SyncManager* pSyncManager_;
	SyncDialogManager* pSyncDialogManager_;
	qs::Profile* pProfile_;
	HWND hwnd_;
};


/****************************************************************************
 *
 * EditSelectAllMessageAction
 *
 */

class EditSelectAllMessageAction : public qs::AbstractAction
{
public:
	explicit EditSelectAllMessageAction(MessageSelectionModel* pMessageSelectionModel);
	virtual ~EditSelectAllMessageAction();

public:
	virtual void invoke(const qs::ActionEvent& event);
	virtual bool isEnabled(const qs::ActionEvent& event);

private:
	EditSelectAllMessageAction(const EditSelectAllMessageAction&);
	EditSelectAllMessageAction& operator=(const EditSelectAllMessageAction&);

private:
	MessageSelectionModel* pMessageSelectionModel_;
};


/****************************************************************************
 *
 * EditUndoMessageAction
 *
 */

class EditUndoMessageAction : public qs::AbstractAction
{
public:
	EditUndoMessageAction(UndoManager* pUndoManager,
						  AccountManager* pAccountManager,
						  HWND hwnd);
	virtual ~EditUndoMessageAction();

public:
	virtual void invoke(const qs::ActionEvent& event);
	virtual bool isEnabled(const qs::ActionEvent& event);

private:
	EditUndoMessageAction(const EditUndoMessageAction&);
	EditUndoMessageAction& operator=(const EditUndoMessageAction&);

private:
	UndoManager* pUndoManager_;
	AccountManager* pAccountManager_;
	HWND hwnd_;
};


/****************************************************************************
 *
 * FileCheckAction
 *
 */

class FileCheckAction : public qs::AbstractAction
{
public:
	FileCheckAction(FolderModel* pFolderModel,
					HWND hwnd);
	virtual ~FileCheckAction();

public:
	virtual void invoke(const qs::ActionEvent& event);
	virtual bool isEnabled(const qs::ActionEvent& event);

private:
	FileCheckAction(const FileCheckAction&);
	FileCheckAction& operator=(const FileCheckAction&);

private:
	FolderModel* pFolderModel_;
	HWND hwnd_;
};


/****************************************************************************
 *
 * FileCloseAction
 *
 */

class FileCloseAction : public qs::AbstractAction
{
public:
	explicit FileCloseAction(HWND hwnd);
	virtual ~FileCloseAction();

public:
	virtual void invoke(const qs::ActionEvent& event);

private:
	FileCloseAction(const FileCloseAction&);
	FileCloseAction& operator=(const FileCloseAction&);

private:
	HWND hwnd_;
};


/****************************************************************************
 *
 * FileCompactAction
 *
 */

class FileCompactAction : public qs::AbstractAction
{
public:
	FileCompactAction(FolderModel* pFolderModel,
					  HWND hwnd);
	virtual ~FileCompactAction();

public:
	virtual void invoke(const qs::ActionEvent& event);
	virtual bool isEnabled(const qs::ActionEvent& event);

private:
	FileCompactAction(const FileCompactAction&);
	FileCompactAction& operator=(const FileCompactAction&);

private:
	FolderModel* pFolderModel_;
	HWND hwnd_;
};


/****************************************************************************
 *
 * FileDumpAction
 *
 */

class FileDumpAction : public qs::AbstractAction
{
public:
	FileDumpAction(FolderModel* pFolderModel,
				   HWND hwnd);
	virtual ~FileDumpAction();

public:
	virtual void invoke(const qs::ActionEvent& event);
	virtual bool isEnabled(const qs::ActionEvent& event);

private:
	bool dumpFolder(const WCHAR* pwszPath,
					Folder* pFolder,
					bool bCreateDirectoryOnly,
					ProgressDialog* pDialog);

private:
	static qs::wstring_ptr getDirectory(const WCHAR* pwszPath,
										Folder* pFolder);

private:
	FileDumpAction(const FileDumpAction&);
	FileDumpAction& operator=(const FileDumpAction&);

private:
	FolderModel* pFolderModel_;
	HWND hwnd_;
};


/****************************************************************************
 *
 * FileExitAction
 *
 */

class FileExitAction : public qs::AbstractAction
{
public:
	FileExitAction(HWND hwnd,
				   Document* pDocument,
				   SyncManager* pSyncManager,
				   SyncDialogManager* pSyncDialogManager,
				   TempFileCleaner* pTempFileCleaner,
				   EditFrameWindowManager* pEditFrameWindowManager,
				   AddressBookFrameWindowManager* pAddressBookFrameWindowManager,
				   FolderModel* pFolderModel,
				   qs::Profile* pProfile);
	virtual ~FileExitAction();

public:
	bool exit(bool bDestroy);

public:
	virtual void invoke(const qs::ActionEvent& event);

private:
	FileExitAction(const FileExitAction&);
	FileExitAction& operator=(const FileExitAction&);

private:
	HWND hwnd_;
	Document* pDocument_;
	SyncManager* pSyncManager_;
	SyncDialogManager* pSyncDialogManager_;
	TempFileCleaner* pTempFileCleaner_;
	EditFrameWindowManager* pEditFrameWindowManager_;
	AddressBookFrameWindowManager* pAddressBookFrameWindowManager_;
	FolderModel* pFolderModel_;
	qs::Profile* pProfile_;
};


/****************************************************************************
 *
 * FileExportAction
 *
 */

class FileExportAction : public qs::AbstractAction
{
public:
	enum Flag {
		FLAG_ADDFLAGS		= 0x01,
		FLAG_WRITESEPARATOR	= 0x02
	};

public:
	FileExportAction(MessageSelectionModel* pMessageSelectionModel,
					 EncodingModel* pEncodingModel,
					 SecurityModel* pSecurityModel,
					 Document* pDocument,
					 qs::Profile* pProfile,
					 HWND hwnd);
	virtual ~FileExportAction();

public:
	virtual void invoke(const qs::ActionEvent& event);
	virtual bool isEnabled(const qs::ActionEvent& event);

private:
	bool export(Account* pAccount,
				Folder* pFolder,
				const MessageHolderList& l);
	bool writeMessage(qs::OutputStream* pStream,
					  const Template* pTemplate,
					  MessageHolder* pmh,
					  const WCHAR* pwszEncoding);

public:
	static bool writeMessage(qs::OutputStream* pStream,
							 MessageHolder* pmh,
							 unsigned int nFlags,
							 unsigned int nSecurityMode);

private:
	FileExportAction(const FileExportAction&);
	FileExportAction& operator=(const FileExportAction&);

private:
	MessageSelectionModel* pMessageSelectionModel_;
	EncodingModel* pEncodingModel_;
	SecurityModel* pSecurityModel_;
	Document* pDocument_;
	qs::Profile* pProfile_;
	HWND hwnd_;
};


/****************************************************************************
 *
 * FileImportAction
 *
 */

class FileImportAction : public qs::AbstractAction
{
public:
	typedef std::vector<qs::WSTRING> PathList;

public:
	FileImportAction(FolderModel* pFolderModel,
					 Document* pDocument,
					 SyncManager* pSyncManager,
					 SyncDialogManager* pSyncDialogManager,
					 qs::Profile* pProfile,
					 HWND hwnd);
	virtual ~FileImportAction();

public:
	virtual void invoke(const qs::ActionEvent& event);
	virtual bool isEnabled(const qs::ActionEvent& event);

public:
	static bool import(NormalFolder* pFolder,
					   const PathList& listPath,
					   bool bMultipleMessagesInFile,
					   const WCHAR* pwszEncoding,
					   unsigned int nFlags,
					   HWND hwnd,
					   qs::wstring_ptr* pwstrErrorPath,
					   unsigned int* pnErrorLine);
	static bool importShowDialog(NormalFolder* pFolder,
								 const PathList& listPath,
								 qs::Profile* pProfile,
								 HWND hwnd,
								 qs::wstring_ptr* pwstrErrorPath,
								 unsigned int* pnErrorLine);
	static bool readSingleMessage(NormalFolder* pFolder,
								  const WCHAR* pwszPath,
								  const WCHAR* pwszEncoding,
								  unsigned int nFlags);
	static bool readMultipleMessages(NormalFolder* pFolder,
									 const WCHAR* pwszPath,
									 const WCHAR* pwszEncoding,
									 unsigned int nFlags,
									 ProgressDialog* pDialog,
									 int* pnPos,
									 bool* pbCanceled,
									 unsigned int* pnErrorLine);

private:
	bool import(NormalFolder* pFolder,
				qs::wstring_ptr* pwstrErrorPath,
				unsigned int* pnErrorLine);

private:
	static bool readLine(qs::InputStream* pStream,
						 CHAR cPrev,
						 qs::xstring_ptr* pstrLine,
						 CHAR* pcNext,
						 bool* pbNewLine);

private:
	FileImportAction(const FileImportAction&);
	FileImportAction& operator=(const FileImportAction&);

private:
	FolderModel* pFolderModel_;
	Document* pDocument_;
	SyncManager* pSyncManager_;
	SyncDialogManager* pSyncDialogManager_;
	qs::Profile* pProfile_;
	HWND hwnd_;
};


/****************************************************************************
 *
 * FileLoadAction
 *
 */

class FileLoadAction : public qs::AbstractAction
{
public:
	FileLoadAction(FolderModel* pFolderModel,
				   HWND hwnd);
	virtual ~FileLoadAction();

public:
	virtual void invoke(const qs::ActionEvent& event);
	virtual bool isEnabled(const qs::ActionEvent& event);

private:
	bool loadFolder(Account* pAccount,
					Folder* pFolder,
					const WCHAR* pwszPath,
					ProgressDialog* pDialog,
					int* pnPos);

private:
	static void getInfo(const WCHAR* pwszFileName,
						qs::wstring_ptr* pwstrName,
						Folder::Type* pType,
						unsigned int* pnFlags);

private:
	FileLoadAction(const FileLoadAction&);
	FileLoadAction& operator=(const FileLoadAction&);

private:
	FolderModel* pFolderModel_;
	HWND hwnd_;
};


/****************************************************************************
 *
 * FileOfflineAction
 *
 */

class FileOfflineAction : public qs::AbstractAction
{
public:
	FileOfflineAction(Document* pDocument,
					  SyncManager* pSyncManager);
	virtual ~FileOfflineAction();

public:
	virtual void invoke(const qs::ActionEvent& event);
	virtual bool isEnabled(const qs::ActionEvent& event);
	virtual bool isChecked(const qs::ActionEvent& event);

public:
	static void toggleOffline(Document* pDocument,
							  SyncManager* pSyncManager);

private:
	FileOfflineAction(const FileOfflineAction&);
	FileOfflineAction& operator=(const FileOfflineAction&);

private:
	Document* pDocument_;
	SyncManager* pSyncManager_;
};


/****************************************************************************
 *
 * FilePrintAction
 *
 */

class FilePrintAction : public qs::AbstractAction
{
public:
	FilePrintAction(Document* pDocument,
					MessageSelectionModel* pMessageSelectionModel,
					EncodingModel* pEncodingModel,
					SecurityModel* pSecurityModel,
					HWND hwnd,
					qs::Profile* pProfile,
					TempFileCleaner* pTempFileCleaner);
	virtual ~FilePrintAction();

public:
	virtual void invoke(const qs::ActionEvent& event);
	virtual bool isEnabled(const qs::ActionEvent& event);

private:
	bool print(Account* pAccount,
			   Folder* pFolder,
			   MessageHolder* pmh,
			   const MessageHolderList& listSelected);

private:
	FilePrintAction(const FilePrintAction&);
	FilePrintAction& operator=(const FilePrintAction&);

private:
	Document* pDocument_;
	MessageSelectionModel* pMessageSelectionModel_;
	EncodingModel* pEncodingModel_;
	SecurityModel* pSecurityModel_;
	HWND hwnd_;
	qs::Profile* pProfile_;
	TempFileCleaner* pTempFileCleaner_;
};


/****************************************************************************
 *
 * FileSalvageAction
 *
 */

class FileSalvageAction : public qs::AbstractAction
{
public:
	FileSalvageAction(FolderModel* pFolderModel,
					  HWND hwnd);
	virtual ~FileSalvageAction();

public:
	virtual void invoke(const qs::ActionEvent& event);
	virtual bool isEnabled(const qs::ActionEvent& event);

private:
	FileSalvageAction(const FileSalvageAction&);
	FileSalvageAction& operator=(const FileSalvageAction&);

private:
	FolderModel* pFolderModel_;
	HWND hwnd_;
};


/****************************************************************************
 *
 * FileSaveAction
 *
 */

class FileSaveAction : public qs::AbstractAction
{
public:
	FileSaveAction(Document* pDocument,
				   ViewModelManager* pViewModelManager,
				   HWND hwnd);
	virtual ~FileSaveAction();

public:
	virtual void invoke(const qs::ActionEvent& event);

private:
	FileSaveAction(const FileSaveAction&);
	FileSaveAction& operator=(const FileSaveAction&);

private:
	Document* pDocument_;
	ViewModelManager* pViewModelManager_;
	HWND hwnd_;
};


/****************************************************************************
 *
 * FileUninstallAction
 *
 */

class FileUninstallAction : public qs::AbstractAction
{
public:
	FileUninstallAction();
	virtual ~FileUninstallAction();

public:
	virtual void invoke(const qs::ActionEvent& event);

private:
	FileUninstallAction(const FileUninstallAction&);
	FileUninstallAction& operator=(const FileUninstallAction&);
};


/****************************************************************************
 *
 * FolderCreateAction
 *
 */

class FolderCreateAction : public qs::AbstractAction
{
public:
	FolderCreateAction(FolderSelectionModel* pFolderSelectionModel,
					   HWND hwnd,
					   qs::Profile* pProfile);
	virtual ~FolderCreateAction();

public:
	virtual void invoke(const qs::ActionEvent& event);
	virtual bool isEnabled(const qs::ActionEvent& event);

private:
	FolderCreateAction(const FolderCreateAction&);
	FolderCreateAction& operator=(const FolderCreateAction&);

private:
	FolderSelectionModel* pFolderSelectionModel_;
	HWND hwnd_;
	qs::Profile* pProfile_;
};


/****************************************************************************
 *
 * FolderDeleteAction
 *
 */

class FolderDeleteAction : public qs::AbstractAction
{
public:
	FolderDeleteAction(FolderModel* pFolderModel,
					   FolderSelectionModel* pFolderSelectionModel,
					   HWND hwnd);
	virtual ~FolderDeleteAction();

public:
	virtual void invoke(const qs::ActionEvent& event);
	virtual bool isEnabled(const qs::ActionEvent& event);

private:
	bool deleteFolder(Folder* pFolder) const;

private:
	FolderDeleteAction(const FolderDeleteAction&);
	FolderDeleteAction& operator=(const FolderDeleteAction&);

private:
	FolderModel* pFolderModel_;
	FolderSelectionModel* pFolderSelectionModel_;
	HWND hwnd_;
};


/****************************************************************************
 *
 * FolderEmptyAction
 *
 */

class FolderEmptyAction : public qs::AbstractAction
{
public:
	FolderEmptyAction(FolderSelectionModel* pFolderSelectionModel,
					  UndoManager* pUndoManager,
					  HWND hwnd,
					  qs::Profile* pProfile);
	virtual ~FolderEmptyAction();

public:
	virtual void invoke(const qs::ActionEvent& event);
	virtual bool isEnabled(const qs::ActionEvent& event);

private:
	FolderEmptyAction(const FolderEmptyAction&);
	FolderEmptyAction& operator=(const FolderEmptyAction&);

private:
	FolderSelectionModel* pFolderSelectionModel_;
	UndoManager* pUndoManager_;
	HWND hwnd_;
	bool bConfirm_;
};


/****************************************************************************
 *
 * FolderEmptyTrashAction
 *
 */

class FolderEmptyTrashAction : public qs::AbstractAction
{
public:
	FolderEmptyTrashAction(SyncManager* pSyncManager,
						   Document* pDocument,
						   FolderModel* pFolderModel,
						   SyncDialogManager* pSyncDialogManager,
						   HWND hwnd,
						   qs::Profile* pProfile);
	virtual ~FolderEmptyTrashAction();

public:
	virtual void invoke(const qs::ActionEvent& event);
	virtual bool isEnabled(const qs::ActionEvent& event);

public:
	static void emptyAllTrash(Document* pDocument,
							  SyncManager* pSyncManager,
							  SyncDialogManager* pSyncDialogManager,
							  FolderModel* pFolderModel,
							  HWND hwnd,
							  qs::Profile* pProfile);
	static void emptyTrash(Account* pAccount,
						   Document* pDocument,
						   SyncManager* pSyncManager,
						   SyncDialogManager* pSyncDialogManager,
						   FolderModel* pFolderModel,
						   HWND hwnd,
						   bool bConfirm);
	static bool hasTrash(Account* pAccount);

private:
	Account* getAccount() const;
	NormalFolder* getTrash() const;

private:
	static NormalFolder* getTrash(Account* pAccount);

private:
	FolderEmptyTrashAction(const FolderEmptyTrashAction&);
	FolderEmptyTrashAction& operator=(const FolderEmptyTrashAction&);

private:
	SyncManager* pSyncManager_;
	Document* pDocument_;
	FolderModel* pFolderModel_;
	SyncDialogManager* pSyncDialogManager_;
	HWND hwnd_;
	bool bConfirm_;
};


/****************************************************************************
 *
 * FolderExpandAction
 *
 */

class FolderExpandAction : public qs::AbstractAction
{
public:
	FolderExpandAction(FolderWindow* pFolderWindow,
					   bool bExpand);
	virtual ~FolderExpandAction();

public:
	virtual void invoke(const qs::ActionEvent& event);

private:
	FolderExpandAction(const FolderExpandAction&);
	FolderExpandAction& operator=(const FolderExpandAction&);

private:
	FolderWindow* pFolderWindow_;
	bool bExpand_;
};


/****************************************************************************
 *
 * FolderPropertyAction
 *
 */

class FolderPropertyAction : public qs::AbstractAction
{
public:
	enum Open {
		OPEN_PROPERTY,
		OPEN_CONDITION,
		OPEN_PARAMETER
	};

public:
	FolderPropertyAction(FolderSelectionModel* pFolderSelectionModel,
						 HWND hwnd,
						 qs::Profile* pProfile);
	virtual ~FolderPropertyAction();

public:
	virtual void invoke(const qs::ActionEvent& event);
	virtual bool isEnabled(const qs::ActionEvent& event);

public:
	static void openProperty(const Account::FolderList& listFolder,
							 Open open,
							 HWND hwnd,
							 qs::Profile* pProfile);

private:
	FolderPropertyAction(const FolderPropertyAction&);
	FolderPropertyAction& operator=(const FolderPropertyAction&);

private:
	FolderSelectionModel* pFolderSelectionModel_;
	HWND hwnd_;
	qs::Profile* pProfile_;
};


/****************************************************************************
 *
 * FolderRenameAction
 *
 */

class FolderRenameAction : public qs::AbstractAction
{
public:
	FolderRenameAction(FolderSelectionModel* pFolderSelectionModel,
					   HWND hwnd);
	virtual ~FolderRenameAction();

public:
	virtual void invoke(const qs::ActionEvent& event);
	virtual bool isEnabled(const qs::ActionEvent& event);

private:
	FolderRenameAction(const FolderRenameAction&);
	FolderRenameAction& operator=(const FolderRenameAction&);

private:
	FolderSelectionModel* pFolderSelectionModel_;
	HWND hwnd_;
};


/****************************************************************************
 *
 * FolderShowSizeAction
 *
 */

class FolderShowSizeAction : public qs::AbstractAction
{
public:
	explicit FolderShowSizeAction(FolderListWindow* pFolderListWindow);
	virtual ~FolderShowSizeAction();

public:
	virtual void invoke(const qs::ActionEvent& event);
	virtual bool isEnabled(const qs::ActionEvent& event);

private:
	FolderShowSizeAction(const FolderShowSizeAction&);
	FolderShowSizeAction& operator=(const FolderShowSizeAction&);

private:
	FolderListWindow* pFolderListWindow_;
};


/****************************************************************************
 *
 * FolderUpdateAction
 *
 */

class FolderUpdateAction : public qs::AbstractAction
{
public:
	FolderUpdateAction(FolderModel* pFolderModel,
					   HWND hwnd);
	virtual ~FolderUpdateAction();

public:
	virtual void invoke(const qs::ActionEvent& event);
	virtual bool isEnabled(const qs::ActionEvent& event);

private:
	FolderUpdateAction(const FolderUpdateAction&);
	FolderUpdateAction& operator=(const FolderUpdateAction&);

private:
	FolderModel* pFolderModel_;
	HWND hwnd_;
};


/****************************************************************************
 *
 * MessageApplyRuleAction
 *
 */

class MessageApplyRuleAction : public qs::AbstractAction
{
public:
	MessageApplyRuleAction(RuleManager* pRuleManager,
						   FolderModel* pFolderModel,
						   bool bAll,
						   SecurityModel* pSecurityModel,
						   Document* pDocument,
						   HWND hwnd,
						   qs::Profile* pProfile);
	MessageApplyRuleAction(RuleManager* pRuleManager,
						   MessageSelectionModel* pMessageSelectionModel,
						   SecurityModel* pSecurityModel,
						   Document* pDocument,
						   HWND hwnd,
						   qs::Profile* pProfile);
	virtual ~MessageApplyRuleAction();

public:
	virtual void invoke(const qs::ActionEvent& event);
	virtual bool isEnabled(const qs::ActionEvent& event);

private:
	MessageApplyRuleAction(const MessageApplyRuleAction&);
	MessageApplyRuleAction& operator=(const MessageApplyRuleAction&);

private:
	RuleManager* pRuleManager_;
	FolderModel* pFolderModel_;
	MessageSelectionModel* pMessageSelectionModel_;
	bool bAll_;
	SecurityModel* pSecurityModel_;
	Document* pDocument_;
	HWND hwnd_;
	qs::Profile* pProfile_;
};


/****************************************************************************
 *
 * MessageApplyTemplateAction
 *
 */

class MessageApplyTemplateAction : public qs::AbstractAction
{
public:
	MessageApplyTemplateAction(TemplateMenu* pTemplateMenu,
							   Document* pDocument,
							   FolderModelBase* pFolderModel,
							   MessageSelectionModel* pMessageSelectionModel,
							   EncodingModel* pEncodingModel,
							   SecurityModel* pSecurityModel,
							   EditFrameWindowManager* pEditFrameWindowManager,
							   ExternalEditorManager* pExternalEditorManager,
							   HWND hwnd,
							   qs::Profile* pProfile,
							   bool bExternalEditor);
	virtual ~MessageApplyTemplateAction();

public:
	virtual void invoke(const qs::ActionEvent& event);
	virtual bool isEnabled(const qs::ActionEvent& event);

private:
	MessageApplyTemplateAction(const MessageApplyTemplateAction&);
	MessageApplyTemplateAction& operator=(const MessageApplyTemplateAction&);

private:
	TemplateProcessor processor_;
	TemplateMenu* pTemplateMenu_;
	FolderModelBase* pFolderModel_;
	HWND hwnd_;
};


/****************************************************************************
 *
 * MessageClearRecentsAction
 *
 */

class MessageClearRecentsAction : public qs::AbstractAction
{
public:
	MessageClearRecentsAction(Recents* pRecents);
	virtual ~MessageClearRecentsAction();

public:
	virtual void invoke(const qs::ActionEvent& event);

private:
	MessageClearRecentsAction(const MessageClearRecentsAction&);
	MessageClearRecentsAction& operator=(const MessageClearRecentsAction&);

private:
	Recents* pRecents_;
};


/****************************************************************************
 *
 * MessageCombineAction
 *
 */

class MessageCombineAction : public qs::AbstractAction
{
public:
	MessageCombineAction(MessageSelectionModel* pMessageSelectionModel,
						 SecurityModel* pSecurityModel,
						 UndoManager* pUndoManager,
						 HWND hwnd);
	virtual ~MessageCombineAction();

public:
	virtual void invoke(const qs::ActionEvent& event);
	virtual bool isEnabled(const qs::ActionEvent& event);

private:
	bool combine(const MessageHolderList& l,
				 Message* pMessage);

private:
	static bool isSpecialField(const CHAR* pszField);

private:
	MessageCombineAction(const MessageCombineAction&);
	MessageCombineAction& operator=(const MessageCombineAction&);

private:
	MessageSelectionModel* pMessageSelectionModel_;
	SecurityModel* pSecurityModel_;
	UndoManager* pUndoManager_;
	HWND hwnd_;
};


/****************************************************************************
 *
 * MessageCreateAction
 *
 */

class MessageCreateAction : public qs::AbstractAction
{
public:
	MessageCreateAction(Document* pDocument,
						FolderModelBase* pFolderModel,
						MessageSelectionModel* pMessageSelectionModel,
						EncodingModel* pEncodingModel,
						SecurityModel* pSecurityModel,
						const WCHAR* pwszTemplateName,
						EditFrameWindowManager* pEditFrameWindowManager,
						ExternalEditorManager* pExternalEditorManager,
						HWND hwnd,
						qs::Profile* pProfile,
						bool bExternalEditor);
	virtual ~MessageCreateAction();

public:
	virtual void invoke(const qs::ActionEvent& event);
	virtual bool isEnabled(const qs::ActionEvent& event);

private:
	MessageCreateAction(const MessageCreateAction&);
	MessageCreateAction& operator=(const MessageCreateAction&);

private:
	TemplateProcessor processor_;
	FolderModelBase* pFolderModel_;
	qs::wstring_ptr wstrTemplateName_;
	HWND hwnd_;
};


/****************************************************************************
 *
 * MessageCreateFromClipboardAction
 *
 */

class MessageCreateFromClipboardAction : public qs::AbstractAction
{
public:
	MessageCreateFromClipboardAction(bool bDraft,
									 Document* pDocument,
									 PasswordManager* pPasswordManager,
									 qs::Profile* pProfile,
									 HWND hwnd,
									 FolderModel* pFolderModel,
									 SecurityModel* pSecurityModel);
	virtual ~MessageCreateFromClipboardAction();

public:
	virtual void invoke(const qs::ActionEvent& event);
	virtual bool isEnabled(const qs::ActionEvent& event);

private:
	MessageCreateFromClipboardAction(const MessageCreateFromClipboardAction&);
	MessageCreateFromClipboardAction& operator=(const MessageCreateFromClipboardAction&);

private:
	MessageComposer composer_;
	Document* pDocument_;
	SecurityModel* pSecurityModel_;
	HWND hwnd_;
};


/****************************************************************************
 *
 * MessageCreateFromFileAction
 *
 */

class MessageCreateFromFileAction : public qs::AbstractAction
{
public:
	MessageCreateFromFileAction(bool bDraft,
								Document* pDocument,
								PasswordManager* pPasswordManager,
								qs::Profile* pProfile,
								HWND hwnd,
								FolderModel* pFolderModel,
								SecurityModel* pSecurityModel);
	virtual ~MessageCreateFromFileAction();

public:
	virtual void invoke(const qs::ActionEvent& event);

private:
	MessageCreateFromFileAction(const MessageCreateFromFileAction&);
	MessageCreateFromFileAction& operator=(const MessageCreateFromFileAction&);

private:
	MessageComposer composer_;
	Document* pDocument_;
	SecurityModel* pSecurityModel_;
	HWND hwnd_;
};


/****************************************************************************
 *
 * MessageDeleteAttachmentAction
 *
 */

class MessageDeleteAttachmentAction : public qs::AbstractAction
{
public:
	MessageDeleteAttachmentAction(MessageSelectionModel* pMessageSelectionModel,
								  SecurityModel* pSecurityModel,
								  UndoManager* pUndoManager,
								  HWND hwnd);
	virtual ~MessageDeleteAttachmentAction();

public:
	virtual void invoke(const qs::ActionEvent& event);
	virtual bool isEnabled(const qs::ActionEvent& event);

private:
	bool deleteAttachment(Account* pAccount,
						  Folder* pFolder,
						  const MessageHolderList& l) const;
	bool deleteAttachment(Account* pAccount,
						  Folder* pFolder,
						  MessageHolder* pmh,
						  UndoItemList* pUndoItemList) const;

private:
	MessageDeleteAttachmentAction(const MessageDeleteAttachmentAction&);
	MessageDeleteAttachmentAction& operator=(const MessageDeleteAttachmentAction&);

private:
	MessageSelectionModel* pMessageSelectionModel_;
	SecurityModel* pSecurityModel_;
	UndoManager* pUndoManager_;
	HWND hwnd_;
};


/****************************************************************************
 *
 * MessageDetachAction
 *
 */

class MessageDetachAction : public qs::AbstractAction
{
public:
	MessageDetachAction(qs::Profile* pProfile,
						MessageSelectionModel* pMessageSelectionModel,
						SecurityModel* pSecurityModel,
						HWND hwnd);
	virtual ~MessageDetachAction();

public:
	virtual void invoke(const qs::ActionEvent& event);
	virtual bool isEnabled(const qs::ActionEvent& event);

private:
	MessageDetachAction(const MessageDetachAction&);
	MessageDetachAction& operator=(const MessageDetachAction&);

private:
	MessageSelectionModel* pMessageSelectionModel_;
	AttachmentHelper helper_;
	HWND hwnd_;
};


/****************************************************************************
 *
 * MessageExpandDigestAction
 *
 */

class MessageExpandDigestAction : public qs::AbstractAction
{
public:
	MessageExpandDigestAction(MessageSelectionModel* pMessageSelectionModel,
							  SecurityModel* pSecurityModel,
							  UndoManager* pUndoManager,
							  HWND hwnd);
	virtual ~MessageExpandDigestAction();

public:
	virtual void invoke(const qs::ActionEvent& event);
	virtual bool isEnabled(const qs::ActionEvent& event);

private:
	bool expandDigest(Account* pAccount,
					  const MessageHolderList& l);
	bool expandDigest(Account* pAccount,
					  MessageHolder* pmh,
					  UndoItemList* pUndoItemList);

private:
	MessageExpandDigestAction(const MessageExpandDigestAction&);
	MessageExpandDigestAction& operator=(const MessageExpandDigestAction&);

private:
	MessageSelectionModel* pMessageSelectionModel_;
	SecurityModel* pSecurityModel_;
	UndoManager* pUndoManager_;
	HWND hwnd_;
};


/****************************************************************************
 *
 * MessageManageJunkAction
 *
 */

class MessageManageJunkAction : public qs::AbstractAction
{
public:
	MessageManageJunkAction(MessageSelectionModel* pMessageSelectionModel,
							JunkFilter* pJunkFilter,
							JunkFilter::Operation operation,
							HWND hwnd);
	virtual ~MessageManageJunkAction();

public:
	virtual void invoke(const qs::ActionEvent& event);
	virtual bool isEnabled(const qs::ActionEvent& event);

private:
	MessageManageJunkAction(const MessageManageJunkAction&);
	MessageManageJunkAction& operator=(const MessageManageJunkAction&);

private:
	MessageSelectionModel* pMessageSelectionModel_;
	JunkFilter* pJunkFilter_;
	JunkFilter::Operation operation_;
	HWND hwnd_;
};


/****************************************************************************
 *
 * MessageMarkAction
 *
 */

class MessageMarkAction : public qs::AbstractAction
{
public:
	MessageMarkAction(MessageSelectionModel* pModel,
					  unsigned int nFlags,
					  unsigned int nMask,
					  UndoManager* pUndoManager,
					  HWND hwnd);
	virtual ~MessageMarkAction();

public:
	virtual void invoke(const qs::ActionEvent& event);
	virtual bool isEnabled(const qs::ActionEvent& event);

private:
	MessageMarkAction(const MessageMarkAction&);
	MessageMarkAction& operator=(const MessageMarkAction&);

private:
	MessageSelectionModel* pModel_;
	unsigned int nFlags_;
	unsigned int nMask_;
	UndoManager* pUndoManager_;
	HWND hwnd_;
};


/****************************************************************************
 *
 * MessageMoveAction
 *
 */

class MessageMoveAction : public qs::AbstractAction
{
public:
	MessageMoveAction(MessageSelectionModel* pMessageSelectionModel,
					  ViewModelHolder* pViewModelHolder,
					  MessageModel* pMessageModel,
					  MoveMenu* pMoveMenu,
					  bool bDontSelectNextIfDeletedFlag,
					  UndoManager* pUndoManager,
					  HWND hwnd);
	virtual ~MessageMoveAction();

public:
	virtual void invoke(const qs::ActionEvent& event);
	virtual bool isEnabled(const qs::ActionEvent& event);

private:
	MessageMoveAction(const MessageMoveAction&);
	MessageMoveAction& operator=(const MessageMoveAction&);

private:
	MessageSelectionModel* pMessageSelectionModel_;
	ViewModelHolder* pViewModelHolder_;
	MessageModel* pMessageModel_;
	MoveMenu* pMoveMenu_;
	bool bDontSelectNextIfDeletedFlag_;
	UndoManager* pUndoManager_;
	HWND hwnd_;
};


/****************************************************************************
 *
 * MessageMoveOtherAction
 *
 */

class MessageMoveOtherAction : public qs::AbstractAction
{
public:
	MessageMoveOtherAction(AccountManager* pAccountManager,
						   MessageSelectionModel* pMessageSelectionModel,
						   ViewModelHolder* pViewModelHolder,
						   MessageModel* pMessageModel,
						   bool bDontSelectNextIfDeletedFlag,
						   UndoManager* pUndoManager,
						   qs::Profile* pProfile,
						   HWND hwnd);
	virtual ~MessageMoveOtherAction();

public:
	virtual void invoke(const qs::ActionEvent& event);
	virtual bool isEnabled(const qs::ActionEvent& event);

private:
	MessageMoveOtherAction(const MessageMoveOtherAction&);
	MessageMoveOtherAction& operator=(const MessageMoveOtherAction&);

private:
	AccountManager* pAccountManager_;
	MessageSelectionModel* pMessageSelectionModel_;
	ViewModelHolder* pViewModelHolder_;
	MessageModel* pMessageModel_;
	bool bDontSelectNextIfDeletedFlag_;
	UndoManager* pUndoManager_;
	qs::Profile* pProfile_;
	HWND hwnd_;
};


/****************************************************************************
 *
 * MessageOpenAttachmentAction
 *
 */

class MessageOpenAttachmentAction : public qs::AbstractAction
{
public:
	MessageOpenAttachmentAction(SecurityModel* pSecurityModel,
								qs::Profile* pProfile,
								AttachmentMenu* pAttachmentMenu,
								TempFileCleaner* pTempFileCleaner,
								HWND hwnd);
	virtual ~MessageOpenAttachmentAction();

public:
	virtual void invoke(const qs::ActionEvent& event);

private:
	MessageOpenAttachmentAction(const MessageOpenAttachmentAction&);
	MessageOpenAttachmentAction& operator=(const MessageOpenAttachmentAction&);

private:
	AttachmentMenu* pAttachmentMenu_;
	AttachmentHelper helper_;
	HWND hwnd_;
};


/****************************************************************************
 *
 * MessageOpenLinkAction
 *
 */

class MessageOpenLinkAction : public qs::AbstractAction
{
public:
	MessageOpenLinkAction(MessageSelectionModel* pMessageSelectionModel,
						  HWND hwnd);
	virtual ~MessageOpenLinkAction();

public:
	virtual void invoke(const qs::ActionEvent& event);
	virtual bool isEnabled(const qs::ActionEvent& event);

private:
	MessageOpenLinkAction(const MessageOpenLinkAction&);
	MessageOpenLinkAction& operator=(const MessageOpenLinkAction&);

private:
	MessageSelectionModel* pMessageSelectionModel_;
	HWND hwnd_;
};


/****************************************************************************
 *
 * MessageOpenRecentAction
 *
 */

class MessageOpenRecentAction : public qs::AbstractAction
{
public:
	MessageOpenRecentAction(Recents* pRecents,
							AccountManager* pAccountManager,
							RecentsMenu* pRecentsMenu,
							ViewModelManager* pViewModelManager,
							MessageFrameWindowManager* pMessageFrameWindowManager);
	virtual ~MessageOpenRecentAction();

public:
	virtual void invoke(const qs::ActionEvent& event);

private:
	MessageOpenRecentAction(const MessageOpenRecentAction&);
	MessageOpenRecentAction& operator=(const MessageOpenRecentAction&);

private:
	Recents* pRecents_;
	AccountManager* pAccountManager_;
	RecentsMenu* pRecentsMenu_;
	ViewModelManager* pViewModelManager_;
	MessageFrameWindowManager* pMessageFrameWindowManager_;
};


/****************************************************************************
 *
 * MessageOpenURLAction
 *
 */

class MessageOpenURLAction : public qs::AbstractAction
{
public:
	MessageOpenURLAction(Document* pDocument,
						 FolderModelBase* pFolderModel,
						 MessageSelectionModel* pMessageSelectionModel,
						 SecurityModel* pSecurityModel,
						 EditFrameWindowManager* pEditFrameWindowManager,
						 ExternalEditorManager* pExternalEditorManager,
						 HWND hwnd,
						 qs::Profile* pProfile,
						 bool bExternalEditor);
	virtual ~MessageOpenURLAction();

public:
	virtual void invoke(const qs::ActionEvent& event);

private:
	MessageOpenURLAction(const MessageOpenURLAction&);
	MessageOpenURLAction& operator=(const MessageOpenURLAction&);

private:
	TemplateProcessor processor_;
	Document* pDocument_;
	FolderModelBase* pFolderModel_;
	qs::Profile* pProfile_;
	HWND hwnd_;
};


/****************************************************************************
 *
 * MessagePropertyAction
 *
 */

class MessagePropertyAction : public qs::AbstractAction
{
public:
	MessagePropertyAction(MessageSelectionModel* pMessageSelectionModel,
						  UndoManager* pUndoManager,
						  HWND hwnd);
	virtual ~MessagePropertyAction();

public:
	virtual void invoke(const qs::ActionEvent& event);
	virtual bool isEnabled(const qs::ActionEvent& event);

private:
	MessagePropertyAction(const MessagePropertyAction&);
	MessagePropertyAction& operator=(const MessagePropertyAction&);

private:
	MessageSelectionModel* pMessageSelectionModel_;
	UndoManager* pUndoManager_;
	HWND hwnd_;
};


/****************************************************************************
 *
 * MessageSearchAction
 *
 */

class MessageSearchAction : public qs::AbstractAction
{
public:
	MessageSearchAction(FolderModel* pFolderModel,
						SecurityModel* pSecurityModel,
						Document* pDocument,
						HWND hwnd,
						qs::Profile* pProfile);
	virtual ~MessageSearchAction();

public:
	virtual void invoke(const qs::ActionEvent& event);
	virtual bool isEnabled(const qs::ActionEvent& event);

private:
	MessageSearchAction(const MessageSearchAction&);
	MessageSearchAction& operator=(const MessageSearchAction&);

private:
	FolderModel* pFolderModel_;
	SecurityModel* pSecurityModel_;
	Document* pDocument_;
	HWND hwnd_;
	qs::Profile* pProfile_;
};


#ifdef QMTABWINDOW
/****************************************************************************
 *
 * TabCloseAction
 *
 */

class TabCloseAction : public qs::AbstractAction
{
public:
	explicit TabCloseAction(TabModel* pTabModel);
	virtual ~TabCloseAction();

public:
	virtual void invoke(const qs::ActionEvent& event);
	virtual bool isEnabled(const qs::ActionEvent& event);

private:
	TabCloseAction(const TabCloseAction&);
	TabCloseAction& operator=(const TabCloseAction&);

private:
	TabModel* pTabModel_;
};


/****************************************************************************
 *
 * TabCreateAction
 *
 */

class TabCreateAction : public qs::AbstractAction
{
public:
	TabCreateAction(TabModel* pTabModel,
					FolderSelectionModel* pFolderSelectionModel);
	virtual ~TabCreateAction();

public:
	virtual void invoke(const qs::ActionEvent& event);
	virtual bool isEnabled(const qs::ActionEvent& event);

private:
	TabCreateAction(const TabCreateAction&);
	TabCreateAction& operator=(const TabCreateAction&);

private:
	TabModel* pTabModel_;
	FolderSelectionModel* pFolderSelectionModel_;
};


/****************************************************************************
 *
 * TabEditTitleAction
 *
 */

class TabEditTitleAction : public qs::AbstractAction
{
public:
	TabEditTitleAction(TabModel* pTabModel,
					   HWND hwnd);
	virtual ~TabEditTitleAction();

public:
	virtual void invoke(const qs::ActionEvent& event);
	virtual bool isEnabled(const qs::ActionEvent& event);

private:
	TabEditTitleAction(const TabEditTitleAction&);
	TabEditTitleAction& operator=(const TabEditTitleAction&);

private:
	TabModel* pTabModel_;
	HWND hwnd_;
};


/****************************************************************************
 *
 * TabLockAction
 *
 */

class TabLockAction : public qs::AbstractAction
{
public:
	explicit TabLockAction(TabModel* pTabModel);
	virtual ~TabLockAction();

public:
	virtual void invoke(const qs::ActionEvent& event);
	virtual bool isEnabled(const qs::ActionEvent& event);
	virtual bool isChecked(const qs::ActionEvent& event);

private:
	TabLockAction(const TabLockAction&);
	TabLockAction& operator=(const TabLockAction&);

private:
	TabModel* pTabModel_;
};


/****************************************************************************
 *
 * TabMoveAction
 *
 */

class TabMoveAction : public qs::AbstractAction
{
public:
	TabMoveAction(TabModel* pTabModel,
				  bool bLeft);
	virtual ~TabMoveAction();

public:
	virtual void invoke(const qs::ActionEvent& event);
	virtual bool isEnabled(const qs::ActionEvent& event);

private:
	TabMoveAction(const TabMoveAction&);
	TabMoveAction& operator=(const TabMoveAction&);

private:
	TabModel* pTabModel_;
	bool bLeft_;
};


/****************************************************************************
 *
 * TabNavigateAction
 *
 */

class TabNavigateAction : public qs::AbstractAction
{
public:
	TabNavigateAction(TabModel* pTabModel,
					  bool bPrev);
	virtual ~TabNavigateAction();

public:
	virtual void invoke(const qs::ActionEvent& event);

private:
	TabNavigateAction(const TabNavigateAction&);
	TabNavigateAction& operator=(const TabNavigateAction&);

private:
	TabModel* pTabModel_;
	bool bPrev_;
};


/****************************************************************************
 *
 * TabSelectAction
 *
 */

class TabSelectAction : public qs::AbstractAction
{
public:
	TabSelectAction(TabModel* pTabModel,
					unsigned int nBaseId);
	virtual ~TabSelectAction();

public:
	virtual void invoke(const qs::ActionEvent& event);
	virtual bool isEnabled(const qs::ActionEvent& event);

private:
	int getItem(unsigned int nId) const;

private:
	TabSelectAction(const TabSelectAction&);
	TabSelectAction& operator=(const TabSelectAction&);

private:
	TabModel* pTabModel_;
	unsigned int nBaseId_;
};
#endif // QMTABWINDOW


/****************************************************************************
 *
 * ToolAccountAction
 *
 */

class ToolAccountAction : public qs::AbstractAction
{
public:
	ToolAccountAction(Document* pDocument,
					  FolderModel* pFolderModel,
					  PasswordManager* pPasswordManager,
					  SyncManager* pSyncManager,
					  qs::Profile* pProfile,
					  HWND hwnd);
	virtual ~ToolAccountAction();

public:
	virtual void invoke(const qs::ActionEvent& event);
	virtual bool isEnabled(const qs::ActionEvent& event);

private:
	ToolAccountAction(const ToolAccountAction&);
	ToolAccountAction& operator=(const ToolAccountAction&);

private:
	Document* pDocument_;
	FolderModel* pFolderModel_;
	PasswordManager* pPasswordManager_;
	SyncManager* pSyncManager_;
	qs::Profile* pProfile_;
	HWND hwnd_;
};


/****************************************************************************
 *
 * ToolAddAddressAction
 *
 */

class ToolAddAddressAction : public qs::AbstractAction
{
public:
	ToolAddAddressAction(AddressBook* pAddressBook,
						 MessageSelectionModel* pMessageSelectionModel,
						 HWND hwnd);
	virtual ~ToolAddAddressAction();

public:
	virtual void invoke(const qs::ActionEvent& event);
	virtual bool isEnabled(const qs::ActionEvent& event);

private:
	ToolAddAddressAction(const ToolAddAddressAction&);
	ToolAddAddressAction& operator=(const ToolAddAddressAction&);

private:
	AddressBook* pAddressBook_;
	MessageSelectionModel* pMessageSelectionModel_;
	HWND hwnd_;
};


/****************************************************************************
 *
 * ToolAddressBookAction
 *
 */

class ToolAddressBookAction : public qs::AbstractAction
{
public:
	explicit ToolAddressBookAction(AddressBookFrameWindowManager* pManager);
	virtual ~ToolAddressBookAction();

public:
	virtual void invoke(const qs::ActionEvent& event);

private:
	ToolAddressBookAction(const ToolAddressBookAction&);
	ToolAddressBookAction& operator=(const ToolAddressBookAction&);

private:
	AddressBookFrameWindowManager* pManager_;
};


/****************************************************************************
 *
 * ToolAutoPilotAction
 *
 */

class ToolAutoPilotAction : public qs::AbstractAction
{
public:
	explicit ToolAutoPilotAction(AutoPilot* pAutoPilot);
	virtual ~ToolAutoPilotAction();

public:
	virtual void invoke(const qs::ActionEvent& event);
	virtual bool isChecked(const qs::ActionEvent& event);

private:
	ToolAutoPilotAction(const ToolAutoPilotAction&);
	ToolAutoPilotAction& operator=(const ToolAutoPilotAction&);

private:
	AutoPilot* pAutoPilot_;
};


/****************************************************************************
 *
 * ToolDialupAction
 *
 */

class ToolDialupAction : public qs::AbstractAction
{
public:
	ToolDialupAction(SyncManager* pSyncManager,
					 Document* pDocument,
					 SyncDialogManager* pSyncDialogManager,
					 HWND hwnd);
	virtual ~ToolDialupAction();

public:
	virtual void invoke(const qs::ActionEvent& event);
	virtual qs::wstring_ptr getText(const qs::ActionEvent& event);

private:
	bool isConnected() const;

private:
	ToolDialupAction(const ToolDialupAction&);
	ToolDialupAction& operator=(const ToolDialupAction&);

private:
	SyncManager* pSyncManager_;
	Document* pDocument_;
	SyncDialogManager* pSyncDialogManager_;
	HWND hwnd_;
};


/****************************************************************************
 *
 * ToolGoRoundAction
 *
 */

class ToolGoRoundAction : public qs::AbstractAction
{
public:
	ToolGoRoundAction(SyncManager* pSyncManager,
					  Document* pDocument,
					  GoRound* pGoRound,
					  SyncDialogManager* pSyncDialogManager,
					  HWND hwnd,
					  GoRoundMenu* pGoRoundMenu);
	virtual ~ToolGoRoundAction();

public:
	virtual void invoke(const qs::ActionEvent& event);

private:
	ToolGoRoundAction(const ToolGoRoundAction&);
	ToolGoRoundAction& operator=(const ToolGoRoundAction&);

private:
	SyncManager* pSyncManager_;
	Document* pDocument_;
	GoRound* pGoRound_;
	SyncDialogManager* pSyncDialogManager_;
	HWND hwnd_;
	GoRoundMenu* pGoRoundMenu_;
};


/****************************************************************************
 *
 * ToolOptionsAction
 *
 */

class ToolOptionsAction : public qs::AbstractAction
{
public:
	ToolOptionsAction(qs::Profile* pProfile,
					  HWND hwnd);
	virtual ~ToolOptionsAction();

public:
	virtual void invoke(const qs::ActionEvent& event);
	virtual bool isEnabled(const qs::ActionEvent& event);

private:
	ToolOptionsAction(const ToolOptionsAction&);
	ToolOptionsAction& operator=(const ToolOptionsAction&);

private:
	qs::Profile* pProfile_;
	HWND hwnd_;
};


/****************************************************************************
 *
 * ToolScriptAction
 *
 */

class ToolScriptAction : public qs::AbstractAction
{
public:
	ToolScriptAction(ScriptMenu* pScriptMenu,
					 Document* pDocument,
					 qs::Profile* pProfile,
					 MainWindow* pMainWindow);
	ToolScriptAction(ScriptMenu* pScriptMenu,
					 Document* pDocument,
					 qs::Profile* pProfile,
					 EditFrameWindow* pEditFrameWindow);
	ToolScriptAction(ScriptMenu* pScriptMenu,
					 Document* pDocument,
					 qs::Profile* pProfile,
					 MessageFrameWindow* pMessageFrameWindow);
	virtual ~ToolScriptAction();

public:
	virtual void invoke(const qs::ActionEvent& event);
	virtual bool isEnabled(const qs::ActionEvent& event);

private:
	ToolScriptAction(const ToolScriptAction&);
	ToolScriptAction& operator=(const ToolScriptAction&);

private:
	ScriptMenu* pScriptMenu_;
	Document* pDocument_;
	qs::Profile* pProfile_;
	MainWindow* pMainWindow_;
	EditFrameWindow* pEditFrameWindow_;
	MessageFrameWindow* pMessageFrameWindow_;
};


/****************************************************************************
 *
 * ToolSubAccountAction
 *
 */

class ToolSubAccountAction : public qs::AbstractAction
{
public:
	ToolSubAccountAction(AccountManager* pAccountManager,
						 FolderModel* pFolderModel,
						 SubAccountMenu* pSubAccountMenu,
						 SyncManager* pSyncManager,
						 HWND hwnd);
	virtual ~ToolSubAccountAction();

public:
	virtual void invoke(const qs::ActionEvent& event);
	virtual bool isEnabled(const qs::ActionEvent& event);
	virtual bool isChecked(const qs::ActionEvent& event);

private:
	ToolSubAccountAction(const ToolSubAccountAction&);
	ToolSubAccountAction& operator=(const ToolSubAccountAction&);

private:
	AccountManager* pAccountManager_;
	FolderModel* pFolderModel_;
	SubAccountMenu* pSubAccountMenu_;
	SyncManager* pSyncManager_;
	HWND hwnd_;
};


/****************************************************************************
 *
 * ToolSyncAction
 *
 */

class ToolSyncAction : public qs::AbstractAction
{
public:
	enum Sync {
		SYNC_SEND		= 0x01,
		SYNC_RECEIVE	= 0x02
	};

public:
	ToolSyncAction(SyncManager* pSyncManager,
				   Document* pDocument,
				   FolderModel* pFolderModel,
				   SyncDialogManager* pSyncDialogManager,
				   unsigned int nSync,
				   HWND hwnd);
	virtual ~ToolSyncAction();

public:
	virtual void invoke(const qs::ActionEvent& event);
	virtual bool isEnabled(const qs::ActionEvent& event);

private:
	ToolSyncAction(const ToolSyncAction&);
	ToolSyncAction& operator=(const ToolSyncAction&);

private:
	SyncManager* pSyncManager_;
	Document* pDocument_;
	FolderModel* pFolderModel_;
	SyncDialogManager* pSyncDialogManager_;
	unsigned int nSync_;
	HWND hwnd_;
};


/****************************************************************************
 *
 * ViewDropDownAction
 *
 */

class ViewDropDownAction : public qs::AbstractAction
{
public:
	explicit ViewDropDownAction(FolderComboBox* pFolderComboBox);
	virtual ~ViewDropDownAction();

public:
	virtual void invoke(const qs::ActionEvent& event);

private:
	ViewDropDownAction(const ViewDropDownAction&);
	ViewDropDownAction& operator=(const ViewDropDownAction&);

private:
	FolderComboBox* pFolderComboBox_;
};


/****************************************************************************
 *
 * ViewEncodingAction
 *
 */

class ViewEncodingAction : public qs::AbstractAction
{
public:
	explicit ViewEncodingAction(EncodingModel* pEncodingModel);
	ViewEncodingAction(EncodingModel* pEncodingModel,
					   EncodingMenu* pEncodingMenu);
	virtual ~ViewEncodingAction();

public:
	virtual void invoke(const qs::ActionEvent& event);
	virtual bool isChecked(const qs::ActionEvent& event);

private:
	ViewEncodingAction(const ViewEncodingAction&);
	ViewEncodingAction& operator=(const ViewEncodingAction&);

private:
	EncodingModel* pEncodingModel_;
	EncodingMenu* pEncodingMenu_;
};


/****************************************************************************
 *
 * ViewFilterAction
 *
 */

class ViewFilterAction : public qs::AbstractAction
{
public:
	ViewFilterAction(ViewModelManager* pViewModelManager,
					 FilterMenu* pFilterMenu);
	virtual ~ViewFilterAction();

public:
	virtual void invoke(const qs::ActionEvent& event);
	virtual bool isEnabled(const qs::ActionEvent& event);
	virtual bool isChecked(const qs::ActionEvent& event);

private:
	ViewFilterAction(const ViewFilterAction&);
	ViewFilterAction& operator=(const ViewFilterAction&);

private:
	ViewModelManager* pViewModelManager_;
	FilterMenu* pFilterMenu_;
};


/****************************************************************************
 *
 * ViewFilterCustomAction
 *
 */

class ViewFilterCustomAction : public qs::AbstractAction
{
public:
	ViewFilterCustomAction(ViewModelManager* pViewModelManager,
						   HWND hwnd);
	virtual ~ViewFilterCustomAction();

public:
	virtual void invoke(const qs::ActionEvent& event);
	virtual bool isEnabled(const qs::ActionEvent& event);
	virtual bool isChecked(const qs::ActionEvent& event);

private:
	ViewFilterCustomAction(const ViewFilterCustomAction&);
	ViewFilterCustomAction& operator=(const ViewFilterCustomAction&);

private:
	ViewModelManager* pViewModelManager_;
	HWND hwnd_;
};


/****************************************************************************
 *
 * ViewFilterNoneAction
 *
 */

class ViewFilterNoneAction : public qs::AbstractAction
{
public:
	explicit ViewFilterNoneAction(ViewModelManager* pViewModelManager);
	virtual ~ViewFilterNoneAction();

public:
	virtual void invoke(const qs::ActionEvent& event);
	virtual bool isEnabled(const qs::ActionEvent& event);
	virtual bool isChecked(const qs::ActionEvent& event);

private:
	ViewFilterNoneAction(const ViewFilterNoneAction&);
	ViewFilterNoneAction& operator=(const ViewFilterNoneAction&);

private:
	ViewModelManager* pViewModelManager_;
};


/****************************************************************************
 *
 * ViewFocusAction
 *
 */

class ViewFocusAction : public qs::AbstractAction
{
public:
	ViewFocusAction(View* pViews[],
					size_t nViewCount,
					bool bNext);
	virtual ~ViewFocusAction();

public:
	virtual void invoke(const qs::ActionEvent& event);

private:
	ViewFocusAction(const ViewFocusAction&);
	ViewFocusAction& operator=(const ViewFocusAction&);

private:
	typedef std::vector<View*> ViewList;

private:
	ViewList listView_;
	bool bNext_;
};


/****************************************************************************
 *
 * ViewLockPreviewAction
 *
 */

class ViewLockPreviewAction : public qs::AbstractAction
{
public:
	explicit ViewLockPreviewAction(PreviewMessageModel* pPreviewMessageModel);
	virtual ~ViewLockPreviewAction();

public:
	virtual void invoke(const qs::ActionEvent& event);

private:
	ViewLockPreviewAction(const ViewLockPreviewAction&);
	ViewLockPreviewAction& operator=(const ViewLockPreviewAction&);

private:
	PreviewMessageModel* pPreviewMessageModel_;
};


/****************************************************************************
 *
 * ViewMessageModeAction
 *
 */

class ViewMessageModeAction : public qs::AbstractAction
{
public:
	ViewMessageModeAction(MessageViewModeHolder* pMessageViewModeHolder,
						  MessageViewMode::Mode mode,
						  bool bEnabled);
	virtual ~ViewMessageModeAction();

public:
	virtual void invoke(const qs::ActionEvent& event);
	virtual bool isEnabled(const qs::ActionEvent& event);
	virtual bool isChecked(const qs::ActionEvent& event);

private:
	ViewMessageModeAction(const ViewMessageModeAction&);
	ViewMessageModeAction& operator=(const ViewMessageModeAction&);

private:
	MessageViewModeHolder* pMessageViewModeHolder_;
	MessageViewMode::Mode mode_;
	bool bEnabled_;
};


/****************************************************************************
 *
 * ViewNavigateFolderAction
 *
 */

class ViewNavigateFolderAction : public qs::AbstractAction
{
public:
	enum Type {
		TYPE_NEXTFOLDER,
		TYPE_PREVFOLDER,
		TYPE_NEXTACCOUNT,
		TYPE_PREVACCOUNT
	};

public:
	ViewNavigateFolderAction(AccountManager* pAccountManager,
							 FolderModel* pFolderModel,
							 Type type);
	virtual ~ViewNavigateFolderAction();

public:
	virtual void invoke(const qs::ActionEvent& event);
	virtual bool isEnabled(const qs::ActionEvent& event);

private:
	ViewNavigateFolderAction(const ViewNavigateFolderAction&);
	ViewNavigateFolderAction& operator=(const ViewNavigateFolderAction&);

private:
	AccountManager* pAccountManager_;
	FolderModel* pFolderModel_;
	Type type_;
};


/****************************************************************************
 *
 * ViewNavigateMessageAction
 *
 */

class ViewNavigateMessageAction : public qs::AbstractAction
{
public:
	enum Type {
		TYPE_NEXT,
		TYPE_PREV,
		TYPE_NEXTUNSEEN,
		TYPE_NEXTPAGE,
		TYPE_PREVPAGE,
		TYPE_SELF
	};

public:
	ViewNavigateMessageAction(ViewModelManager* pViewModelManager,
							  FolderModel* pFolderModel,
							  MainWindow* pMainWindow,
							  MessageWindow* pMessageWindow,
							  Type type);
	ViewNavigateMessageAction(ViewModelManager* pViewModelManager,
							  ViewModelHolder* pViewModelHolder,
							  MessageWindow* pMessageWindow,
							  Type type);
	virtual ~ViewNavigateMessageAction();

public:
	virtual void invoke(const qs::ActionEvent& event);
	virtual bool isEnabled(const qs::ActionEvent& event);

private:
	std::pair<ViewModel*, unsigned int> getNextUnseen(ViewModel* pViewModel,
													  unsigned int nIndex) const;
	bool isUnseenFolder(const Folder* pFolder) const;

private:
	ViewNavigateMessageAction(const ViewNavigateMessageAction&);
	ViewNavigateMessageAction& operator=(const ViewNavigateMessageAction&);

private:
	ViewModelManager* pViewModelManager_;
	FolderModel* pFolderModel_;
	ViewModelHolder* pViewModelHolder_;
	MainWindow* pMainWindow_;
	MessageWindow* pMessageWindow_;
	Type type_;
};


/****************************************************************************
 *
 * ViewOpenLinkAction
 *
 */

class ViewOpenLinkAction : public qs::AbstractAction
{
public:
	explicit ViewOpenLinkAction(MessageWindow* pMessageWindow);
	virtual ~ViewOpenLinkAction();

public:
	virtual void invoke(const qs::ActionEvent& event);

private:
	ViewOpenLinkAction(const ViewOpenLinkAction&);
	ViewOpenLinkAction& operator=(const ViewOpenLinkAction&);

private:
	MessageWindow* pMessageWindow_;
};


/****************************************************************************
 *
 * ViewRefreshAction
 *
 */

class ViewRefreshAction : public qs::AbstractAction
{
public:
	ViewRefreshAction(SyncManager* pSyncManager,
					  Document* pDocument,
					  FolderModel* pFolderModel,
					  SecurityModel* pSecurityModel,
					  SyncDialogManager* pSyncDialogManager,
					  HWND hwnd,
					  qs::Profile* pProfile);
	virtual ~ViewRefreshAction();

public:
	virtual void invoke(const qs::ActionEvent& event);
	virtual bool isEnabled(const qs::ActionEvent& event);

private:
	ViewRefreshAction(const ViewRefreshAction&);
	ViewRefreshAction& operator=(const ViewRefreshAction&);

private:
	SyncManager* pSyncManager_;
	Document* pDocument_;
	FolderModel* pFolderModel_;
	SecurityModel* pSecurityModel_;
	SyncDialogManager* pSyncDialogManager_;
	HWND hwnd_;
	qs::Profile* pProfile_;
};


/****************************************************************************
 *
 * ViewSecurityAction
 *
 */

class ViewSecurityAction : public qs::AbstractAction
{
public:
	ViewSecurityAction(SecurityModel* pSecurityModel,
					   SecurityMode mode,
					   bool bEnabled);
	virtual ~ViewSecurityAction();

public:
	virtual void invoke(const qs::ActionEvent& event);
	virtual bool isEnabled(const qs::ActionEvent& event);
	virtual bool isChecked(const qs::ActionEvent& event);

private:
	ViewSecurityAction(const ViewSecurityAction&);
	ViewSecurityAction& operator=(const ViewSecurityAction&);

private:
	SecurityModel* pSecurityModel_;
	SecurityMode mode_;
	bool bEnabled_;
};

/****************************************************************************
 *
 * ViewScrollAction
 *
 */

class ViewScrollAction : public qs::AbstractAction
{
public:
	enum Scroll {
		SCROLL_LINEUP			= 0x01,
		SCROLL_LINEDOWN			= 0x02,
		SCROLL_PAGEUP			= 0x03,
		SCROLL_PAGEDOWN			= 0x04,
		SCROLL_TOP				= 0x05,
		SCROLL_BOTTOM			= 0x06,
		SCROLL_VERTICAL_MASK	= 0x0f,
		
		SCROLL_LINELEFT			= 0x10,
		SCROLL_LINERIGHT		= 0x20,
		SCROLL_PAGELEFT			= 0x30,
		SCROLL_PAGERIGHT		= 0x40,
		SCROLL_LEFT				= 0x50,
		SCROLL_RIGHT			= 0x60,
		SCROLL_HORIZONTAL_MASK	= 0xf0,
	};

public:
	ViewScrollAction(HWND hwnd,
					 Scroll scroll);
	virtual ~ViewScrollAction();

public:
	virtual void invoke(const qs::ActionEvent& event);

private:
	ViewScrollAction(const ViewScrollAction&);
	ViewScrollAction& operator=(const ViewScrollAction&);

private:
	HWND hwnd_;
	UINT nMsg_;
	int nRequest_;
};


/****************************************************************************
 *
 * ViewSelectMessageAction
 *
 */

class ViewSelectMessageAction : public qs::AbstractAction
{
public:
	ViewSelectMessageAction(ViewModelManager* pViewModelManager,
							FolderModel* pFolderModel,
							MessageSelectionModel* pMessageSelectionModel);
	virtual ~ViewSelectMessageAction();

public:
	virtual void invoke(const qs::ActionEvent& event);

private:
	ViewSelectMessageAction(const ViewSelectMessageAction&);
	ViewSelectMessageAction& operator=(const ViewSelectMessageAction&);

private:
	ViewModelManager* pViewModelManager_;
	FolderModel* pFolderModel_;
	MessageSelectionModel* pMessageSelectionModel_;
};


/****************************************************************************
 *
 * ViewShowControlAction
 *
 */

template<class WindowX>
class ViewShowControlAction : public qs::AbstractAction
{
public:
	typedef void (WindowX::*PFN_SET)(bool);
	typedef bool (WindowX::*PFN_IS)() const;

public:
	ViewShowControlAction(WindowX* pWindow,
						  PFN_SET pfnSet,
						  PFN_IS pfnIs,
						  UINT nShowId,
						  UINT nHideId);
	virtual ~ViewShowControlAction();

public:
	virtual void invoke(const qs::ActionEvent& event);
	virtual qs::wstring_ptr getText(const qs::ActionEvent& event);

private:
	ViewShowControlAction(const ViewShowControlAction&);
	ViewShowControlAction& operator=(const ViewShowControlAction&);

private:
	WindowX* pWindow_;
	PFN_SET pfnSet_;
	PFN_IS pfnIs_;
	UINT nShowId_;
	UINT nHideId_;
};


/****************************************************************************
 *
 * ViewShowFolderAction
 *
 */

class ViewShowFolderAction : public ViewShowControlAction<MainWindow>
{
public:
	explicit ViewShowFolderAction(MainWindow* pMainWindow);
	virtual ~ViewShowFolderAction();

private:
	ViewShowFolderAction(const ViewShowFolderAction&);
	ViewShowFolderAction& operator=(const ViewShowFolderAction&);
};


/****************************************************************************
 *
 * ViewShowHeaderAction
 *
 */

class ViewShowHeaderAction : public ViewShowControlAction<MessageWindow>
{
public:
	explicit ViewShowHeaderAction(MessageWindow* pMessageWindow);
	virtual ~ViewShowHeaderAction();

private:
	ViewShowHeaderAction(const ViewShowHeaderAction&);
	ViewShowHeaderAction& operator=(const ViewShowHeaderAction&);
};


/****************************************************************************
 *
 * ViewShowHeaderColumnAction
 *
 */

class ViewShowHeaderColumnAction : public ViewShowControlAction<ListWindow>
{
public:
	explicit ViewShowHeaderColumnAction(ListWindow* pListWindow);
	virtual ~ViewShowHeaderColumnAction();

private:
	ViewShowHeaderColumnAction(const ViewShowHeaderColumnAction&);
	ViewShowHeaderColumnAction& operator=(const ViewShowHeaderColumnAction&);
};


/****************************************************************************
 *
 * ViewShowPreviewAction
 *
 */

class ViewShowPreviewAction : public ViewShowControlAction<MainWindow>
{
public:
	explicit ViewShowPreviewAction(MainWindow* pMainWindow);
	virtual ~ViewShowPreviewAction();

private:
	ViewShowPreviewAction(const ViewShowPreviewAction&);
	ViewShowPreviewAction& operator=(const ViewShowPreviewAction&);
};


/****************************************************************************
 *
 * ViewShowStatusBarAction
 *
 */

template<class WindowX>
class ViewShowStatusBarAction : public ViewShowControlAction<WindowX>
{
public:
	explicit ViewShowStatusBarAction(WindowX* pWindow);
	virtual ~ViewShowStatusBarAction();

private:
	ViewShowStatusBarAction(const ViewShowStatusBarAction&);
	ViewShowStatusBarAction& operator=(const ViewShowStatusBarAction&);
};


#ifdef QMTABWINDOW
/****************************************************************************
 *
 * ViewShowTabAction
 *
 */

class ViewShowTabAction : public ViewShowControlAction<TabWindow>
{
public:
	explicit ViewShowTabAction(TabWindow* pTabWindow);
	virtual ~ViewShowTabAction();

private:
	ViewShowTabAction(const ViewShowTabAction&);
	ViewShowTabAction& operator=(const ViewShowTabAction&);
};
#endif


/****************************************************************************
 *
 * ViewShowSyncDialogAction
 *
 */

class ViewShowSyncDialogAction : public qs::AbstractAction
{
public:
	explicit ViewShowSyncDialogAction(SyncDialogManager* pManager);
	virtual ~ViewShowSyncDialogAction();

public:
	virtual void invoke(const qs::ActionEvent& event);

private:
	ViewShowSyncDialogAction(const ViewShowSyncDialogAction&);
	ViewShowSyncDialogAction& operator=(const ViewShowSyncDialogAction&);

private:
	SyncDialogManager* pManager_;
};


/****************************************************************************
 *
 * ViewShowToolbarAction
 *
 */

template<class WindowX>
class ViewShowToolbarAction : public ViewShowControlAction<WindowX>
{
public:
	explicit ViewShowToolbarAction(WindowX* pWindow);
	virtual ~ViewShowToolbarAction();

private:
	ViewShowToolbarAction(const ViewShowToolbarAction&);
	ViewShowToolbarAction& operator=(const ViewShowToolbarAction&);
};


/****************************************************************************
 *
 * ViewSortAction
 *
 */

class ViewSortAction : public qs::AbstractAction
{
public:
	ViewSortAction(ViewModelManager* pViewModelManager,
				   SortMenu* pSortMenu);
	virtual ~ViewSortAction();

public:
	virtual void invoke(const qs::ActionEvent& event);
	virtual bool isEnabled(const qs::ActionEvent& event);
	virtual bool isChecked(const qs::ActionEvent& event);

private:
	ViewSortAction(const ViewSortAction&);
	ViewSortAction& operator=(const ViewSortAction&);

private:
	ViewModelManager* pViewModelManager_;
	SortMenu* pSortMenu_;
};


/****************************************************************************
 *
 * ViewSortDirectionAction
 *
 */

class ViewSortDirectionAction : public qs::AbstractAction
{
public:
	ViewSortDirectionAction(ViewModelManager* pViewModelManager,
							bool bAscending);
	virtual ~ViewSortDirectionAction();

public:
	virtual void invoke(const qs::ActionEvent& event);
	virtual bool isEnabled(const qs::ActionEvent& event);
	virtual bool isChecked(const qs::ActionEvent& event);

private:
	ViewSortDirectionAction(const ViewSortDirectionAction&);
	ViewSortDirectionAction& operator=(const ViewSortDirectionAction&);

private:
	ViewModelManager* pViewModelManager_;
	bool bAscending_;
};


/****************************************************************************
 *
 * ViewSortFloatThreadAction
 *
 */

class ViewSortFloatThreadAction : public qs::AbstractAction
{
public:
	explicit ViewSortFloatThreadAction(ViewModelManager* pViewModelManager);
	virtual ~ViewSortFloatThreadAction();

public:
	virtual void invoke(const qs::ActionEvent& event);
	virtual bool isEnabled(const qs::ActionEvent& event);
	virtual bool isChecked(const qs::ActionEvent& event);

private:
	ViewSortFloatThreadAction(const ViewSortFloatThreadAction&);
	ViewSortFloatThreadAction& operator=(const ViewSortFloatThreadAction&);

private:
	ViewModelManager* pViewModelManager_;
};


/****************************************************************************
 *
 * ViewSortThreadAction
 *
 */

class ViewSortThreadAction : public qs::AbstractAction
{
public:
	explicit ViewSortThreadAction(ViewModelManager* pViewModelManager);
	virtual ~ViewSortThreadAction();

public:
	virtual void invoke(const qs::ActionEvent& event);
	virtual bool isEnabled(const qs::ActionEvent& event);
	virtual bool isChecked(const qs::ActionEvent& event);

private:
	ViewSortThreadAction(const ViewSortThreadAction&);
	ViewSortThreadAction& operator=(const ViewSortThreadAction&);

private:
	ViewModelManager* pViewModelManager_;
};


/****************************************************************************
 *
 * ViewTemplateAction
 *
 */

class ViewTemplateAction : public qs::AbstractAction
{
public:
	explicit ViewTemplateAction(MessageWindow* pMessageWindow);
	ViewTemplateAction(MessageWindow* pMessageWindow,
					   TemplateMenu* pTemplateMenu);
	virtual ~ViewTemplateAction();

public:
	virtual void invoke(const qs::ActionEvent& event);
	virtual bool isChecked(const qs::ActionEvent& event);

private:
	Account* getAccount() const;

private:
	ViewTemplateAction(const ViewTemplateAction&);
	ViewTemplateAction& operator=(const ViewTemplateAction&);

private:
	MessageWindow* pMessageWindow_;
	TemplateMenu* pTemplateMenu_;
};


/****************************************************************************
 *
 * ActionUtil
 *
 */

class ActionUtil
{
public:
	static void info(HWND hwnd,
					 UINT nMessage);
	static void error(HWND hwnd,
					  UINT nMessage);
	static void error(HWND hwnd,
					  const WCHAR* pwszMessage);
};


/****************************************************************************
 *
 * FolderActionUtil
 *
 */

class FolderActionUtil
{
public:
	static std::pair<Account*, Folder*> getFocused(FolderSelectionModel* pModel);
	static void getSelected(FolderSelectionModel* pModel,
							Account::FolderList* pListFolder);
	static bool hasSelected(FolderSelectionModel* pModel);
	static std::pair<Account*, Folder*> getCurrent(FolderModel* pModel);
	static Account* getAccount(FolderModel* pModel);
	static Folder* getFolder(FolderModel* pModel);
};


/****************************************************************************
 *
 * MessageActionUtil
 *
 */

class MessageActionUtil
{
public:
	static void select(ViewModel* pViewModel,
					   unsigned int nIndex,
					   MessageModel* pMessageModel);
};


#ifdef QMTABWINDOW
/****************************************************************************
 *
 * TabActionUtil
 *
 */

class TabActionUtil
{
public:
	static int getCurrent(TabModel* pModel);
};
#endif

}

#include "action.inl"

#endif // __ACTION_H__
