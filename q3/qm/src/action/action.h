/*
 * $Id$
 *
 * Copyright(C) 1998-2008 Satoshi Nakamura
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
#include <qsmenu.h>
#include <qsstream.h>

#include "attachmenthelper.h"
#include "messagecomposer.h"
#include "templateprocessor.h"
#include "../ui/messagewindow.h"
#include "../ui/optiondialog.h"
#include "../uimodel/messageviewmode.h"


namespace qm {

class AttachmentOpenAction;
class AttachmentSaveAction;
class ConfigViewsAction;
class DispatchAction;
class EditClearDeletedAction;
template<class Item> class EditCommandAction;
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
class FileHideAction;
class FileImportAction;
class FileLoadAction;
class FileOfflineAction;
class FileOpenAction;
class FilePrintAction;
class FileSalvageAction;
class FileSaveAction;
#ifndef _WIN32_WCE_PSPC
class FileShowAction;
#endif
class FileUninstallAction;
class FolderCreateAction;
class FolderDeleteAction;
class FolderEmptyAction;
class FolderEmptyTrashAction;
class FolderExpandAction;
class FolderPropertyAction;
class FolderRenameAction;
class FolderShowSizeAction;
class FolderSubscribeAction;
class FolderUpdateAction;
class HelpAboutAction;
class HelpCheckUpdateAction;
class HelpOpenURLAction;
class MessageApplyRuleAction;
class MessageApplyRuleBackgroundAction;
class MessageCertificateAction;
class MessageClearRecentsAction;
class MessageCombineAction;
class MessageCreateAction;
class MessageCreateFromClipboardAction;
class MessageCreateFromFileAction;
class MessageDeleteAttachmentAction;
class MessageDetachAction;
class MessageExpandDigestAction;
class MessageLabelAction;
class MessageMacroAction;
class MessageManageJunkAction;
class MessageMarkAction;
class MessageMoveAction;
class MessageOpenAction;
class MessageOpenAttachmentAction;
class MessageOpenFocusedAction;
class MessageOpenLinkAction;
class MessageOpenRecentAction;
class MessageOpenURLAction;
class MessagePropertyAction;
class MessageSearchAction;
class NoneAction;
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
class ToolInvokeActionAction;
class ToolOptionsAction;
class ToolPopupMenuAction;
class ToolScriptAction;
class ToolSubAccountAction;
class ToolSyncAction;
class ViewDropDownAction;
class ViewEncodingAction;
class ViewFilterAction;
class ViewFilterCustomAction;
class ViewFitAction;
class ViewFocusAction;
class ViewFocusItemAction;
class ViewFontGroupAction;
class ViewLockPreviewAction;
class ViewMessageModeAction;
class ViewNavigateFolderAction;
class ViewNavigateMessageAction;
class ViewOpenLinkAction;
class ViewRefreshAction;
class ViewSecurityAction;
class ViewScrollAction;
class ViewScrollMessageAction;
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
class ViewSortThreadAction;
class ViewTemplateAction;
class ViewZoomAction;
class ActionUtil;
class ActionParamUtil;
class FolderActionUtil;
class MessageActionUtil;
#ifdef QMTABWINDOW
class TabActionUtil;
#endif

class AccountSelectionModel;
class AddressBookFrameWindowManager;
class AttachmentSelectionModel;
class AutoPilot;
class AutoPilotManager;
class ColorManager;
class Document;
class EditFrameWindow;
class EditFrameWindowManager;
class EncodingModel;
class ExternalEditorManager;
class Filter;
class FilterManager;
class FindReplaceManager;
class FixedFormTextManager;
class FocusControllerBase;
class FolderComboBox;
class FolderImage;
class FolderListWindow;
class FolderModel;
class FolderSelectionModel;
class FolderWindow;
class GoRound;
class ListWindow;
class MainWindow;
class MessageEnumerator;
class MessageFrameWindow;
class MessageHolder;
class MessageModel;
class MessagePtr;
class MessageSelectionModel;
class MessageWindow;
class NormalFolder;
class PasswordManager;
class PreviewMessageModel;
class ProgressDialog;
class Recents;
class RuleManager;
class ScriptManager;
class SecurityModel;
class SignatureManager;
class SyncDialogManager;
class SyncFilterManager;
class SyncManager;
#ifdef QMTABWINDOW
class TabModel;
class TabWindow;
#endif
class TempFileCleaner;
class Template;
class UIManager;
class UndoManager;
class UpdateChecker;
class URIResolver;
class View;
class ViewModel;
class ViewModelHolder;
class ViewModelManager;


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
						 MessageFrameWindowManager* pMessageFrameWindowManager,
						 TempFileCleaner* pTempFileCleaner,
						 qs::Profile* pProfile,
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
						   FolderModelBase* pFolderModel,
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
	FolderModelBase* pFolderModel_;
	SyncDialogManager* pSyncDialogManager_;
	HWND hwnd_;
	qs::Profile* pProfile_;
};


/****************************************************************************
 *
 * EditCommandAction
 *
 */

template<class Item>
class EditCommandAction : public qs::AbstractAction
{
public:
	typedef void (Item::*PFN_DO)();
	typedef bool (Item::*PFN_CANDO)();

public:
	EditCommandAction(FocusController<Item>* pFocusController,
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
	FocusController<Item>* pFocusController_;
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
						  const URIResolver* pURIResolver,
						  TempFileCleaner* pTempFileCleaner,
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
	const URIResolver* pURIResolver_;
	TempFileCleaner* pTempFileCleaner_;
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
						 const URIResolver* pURIResolver,
						 TempFileCleaner* pTempFileCleaner,
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
	const URIResolver* pURIResolver_;
	TempFileCleaner* pTempFileCleaner_;
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
	enum Type {
		TYPE_NORMAL,
		TYPE_DIRECT,
		TYPE_JUNK
	};

public:
	EditDeleteMessageAction(MessageSelectionModel* pMessageSelectionModel,
							ViewModelHolder* pViewModelHolder,
							MessageModel* pMessageModel,
							Type type,
							bool bDontSelectNextIfDeletedFlag,
							UndoManager* pUndoManager,
							HWND hwnd,
							qs::Profile* pProfile);
	virtual ~EditDeleteMessageAction();

public:
	virtual void invoke(const qs::ActionEvent& event);
	virtual bool isEnabled(const qs::ActionEvent& event);

private:
	bool deleteMessages(const MessageHolderList& l,
						Folder* pFolder) const;
	bool confirm() const;

private:
	EditDeleteMessageAction(const EditDeleteMessageAction&);
	EditDeleteMessageAction& operator=(const EditDeleteMessageAction&);

private:
	MessageSelectionModel* pMessageSelectionModel_;
	ViewModelHolder* pViewModelHolder_;
	MessageModel* pMessageModel_;
	Type type_;
	bool bDontSelectNextIfDeletedFlag_;
	UndoManager* pUndoManager_;
	HWND hwnd_;
	qs::Profile* pProfile_;
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
	EditPasteMessageAction(AccountManager* pAccountManager,
						   const URIResolver* pURIResolver,
						   UndoManager* pUndoManager,
						   FolderModelBase* pFolderModel,
						   SyncManager* pSyncManager,
						   SyncDialogManager* pSyncDialogManager,
						   qs::Profile* pProfile,
						   HWND hwnd);
	virtual ~EditPasteMessageAction();

public:
	virtual void invoke(const qs::ActionEvent& event);
	virtual bool isEnabled(const qs::ActionEvent& event);

private:
	bool pasteMessages(NormalFolder* pFolder) const;

private:
	EditPasteMessageAction(const EditPasteMessageAction&);
	EditPasteMessageAction& operator=(const EditPasteMessageAction&);

private:
	AccountManager* pAccountManager_;
	const URIResolver* pURIResolver_;
	UndoManager* pUndoManager_;
	FolderModelBase* pFolderModel_;
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
						  const URIResolver* pURIResolver,
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
	const URIResolver* pURIResolver_;
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
	FileCheckAction(FolderModelBase* pFolderModel,
					HWND hwnd);
	virtual ~FileCheckAction();

public:
	virtual void invoke(const qs::ActionEvent& event);
	virtual bool isEnabled(const qs::ActionEvent& event);

private:
	bool check(Account* pAccount) const;

private:
	FileCheckAction(const FileCheckAction&);
	FileCheckAction& operator=(const FileCheckAction&);

private:
	FolderModelBase* pFolderModel_;
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
	FileCompactAction(FolderModelBase* pFolderModel,
					  HWND hwnd);
	virtual ~FileCompactAction();

public:
	virtual void invoke(const qs::ActionEvent& event);
	virtual bool isEnabled(const qs::ActionEvent& event);

private:
	bool compact(Account* pAccount) const;

private:
	FileCompactAction(const FileCompactAction&);
	FileCompactAction& operator=(const FileCompactAction&);

private:
	FolderModelBase* pFolderModel_;
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
	FileDumpAction(FolderModelBase* pFolderModel,
				   HWND hwnd);
	virtual ~FileDumpAction();

public:
	virtual void invoke(const qs::ActionEvent& event);
	virtual bool isEnabled(const qs::ActionEvent& event);

private:
	bool dump(Account* pAccount,
			  const WCHAR* pwszPath) const;
	bool dumpFolder(Folder* pFolder,
					const WCHAR* pwszPath,
					bool bCreateDirectoryOnly,
					ProgressDialog* pDialog) const;

private:
	static qs::wstring_ptr getDirectory(const WCHAR* pwszPath,
										Folder* pFolder);

private:
	FileDumpAction(const FileDumpAction&);
	FileDumpAction& operator=(const FileDumpAction&);

private:
	FolderModelBase* pFolderModel_;
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
					 const ActionInvoker* pActionInvoker,
					 qs::Profile* pProfile,
					 HWND hwnd);
	virtual ~FileExportAction();

public:
	virtual void invoke(const qs::ActionEvent& event);
	virtual bool isEnabled(const qs::ActionEvent& event);

private:
	bool exportMessages(MessageEnumerator* pEnum);
	bool writeMessage(qs::OutputStream* pStream,
					  MessageEnumerator* pEnum,
					  unsigned int nFlags);
	bool writeMessage(qs::OutputStream* pStream,
					  MessageEnumerator* pEnum,
					  const Template* pTemplate,
					  const WCHAR* pwszEncoding);

public:
	static bool writeMessage(qs::OutputStream* pStream,
							 MessageHolder* pmh,
							 unsigned int nFlags,
							 unsigned int nSecurityMode);
	static bool writeMessage(qs::OutputStream* pStream,
							 MessageHolder* pmh,
							 Message* pMessage,
							 unsigned int nFlags);

private:
	FileExportAction(const FileExportAction&);
	FileExportAction& operator=(const FileExportAction&);

private:
	MessageSelectionModel* pMessageSelectionModel_;
	EncodingModel* pEncodingModel_;
	SecurityModel* pSecurityModel_;
	Document* pDocument_;
	const ActionInvoker* pActionInvoker_;
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
	FileImportAction(FolderModelBase* pFolderModel,
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
	FolderModelBase* pFolderModel_;
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
	FileLoadAction(FolderModelBase* pFolderModel,
				   HWND hwnd);
	virtual ~FileLoadAction();

public:
	virtual void invoke(const qs::ActionEvent& event);
	virtual bool isEnabled(const qs::ActionEvent& event);

private:
	bool load(Account* pAccount,
			  const WCHAR* pwszPath) const;
	bool loadFolder(Account* pAccount,
					Folder* pFolder,
					const WCHAR* pwszPath,
					ProgressDialog* pDialog,
					int* pnPos) const;

private:
	static void getInfo(const WCHAR* pwszFileName,
						qs::wstring_ptr* pwstrName,
						Folder::Type* pType,
						unsigned int* pnFlags);

private:
	FileLoadAction(const FileLoadAction&);
	FileLoadAction& operator=(const FileLoadAction&);

private:
	FolderModelBase* pFolderModel_;
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
 * FileOpenAction
 *
 */

class FileOpenAction : public qs::AbstractAction
{
public:
	FileOpenAction(MessageFrameWindowManager* pMessageFrameWindowManager,
				   HWND hwnd);
	virtual ~FileOpenAction();

public:
	virtual void invoke(const qs::ActionEvent& event);

private:
	bool open(const WCHAR* pwszPath);

private:
	FileOpenAction(const FileOpenAction&);
	FileOpenAction& operator=(const FileOpenAction&);

private:
	MessageFrameWindowManager* pMessageFrameWindowManager_;
	HWND hwnd_;
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
					const ActionInvoker* pActionInvoker,
					HWND hwnd,
					qs::Profile* pProfile,
					TempFileCleaner* pTempFileCleaner);
	virtual ~FilePrintAction();

public:
	virtual void invoke(const qs::ActionEvent& event);
	virtual bool isEnabled(const qs::ActionEvent& event);

private:
	bool print(MessageEnumerator* pEnum,
			   const MessageHolderList& listSelected);

private:
	FilePrintAction(const FilePrintAction&);
	FilePrintAction& operator=(const FilePrintAction&);

private:
	Document* pDocument_;
	MessageSelectionModel* pMessageSelectionModel_;
	EncodingModel* pEncodingModel_;
	SecurityModel* pSecurityModel_;
	const ActionInvoker* pActionInvoker_;
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
	FileSalvageAction(FolderModelBase* pFolderModel,
					  HWND hwnd);
	virtual ~FileSalvageAction();

public:
	virtual void invoke(const qs::ActionEvent& event);
	virtual bool isEnabled(const qs::ActionEvent& event);

private:
	bool salvage(NormalFolder* pFolder) const;

private:
	FileSalvageAction(const FileSalvageAction&);
	FileSalvageAction& operator=(const FileSalvageAction&);

private:
	FolderModelBase* pFolderModel_;
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


#ifndef _WIN32_WCE_PSPC
/****************************************************************************
 *
 * FileShowAction
 *
 */

class FileShowAction : public qs::AbstractAction
{
public:
	FileShowAction(MainWindow* pMainWindow,
				   bool bShow);
	virtual ~FileShowAction();

public:
	virtual void invoke(const qs::ActionEvent& event);
	virtual bool isEnabled(const qs::ActionEvent& event);

private:
	FileShowAction(const FileShowAction&);
	FileShowAction& operator=(const FileShowAction&);

private:
	MainWindow* pMainWindow_;
	bool bShow_;
};
#endif


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
					   SyncManager* pSyncManager,
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
	SyncManager* pSyncManager_;
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
					   SyncManager* pSyncManager,
					   HWND hwnd);
	virtual ~FolderDeleteAction();

public:
	virtual void invoke(const qs::ActionEvent& event);
	virtual bool isEnabled(const qs::ActionEvent& event);

public:
	static bool deleteFolder(FolderModel* pFolderModel,
							 Folder* pFolder);

private:
	FolderDeleteAction(const FolderDeleteAction&);
	FolderDeleteAction& operator=(const FolderDeleteAction&);

private:
	FolderModel* pFolderModel_;
	FolderSelectionModel* pFolderSelectionModel_;
	SyncManager* pSyncManager_;
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
	FolderEmptyAction(AccountManager* pAccountManager,
					  FolderSelectionModel* pFolderSelectionModel,
					  UndoManager* pUndoManager,
					  HWND hwnd,
					  qs::Profile* pProfile);
	virtual ~FolderEmptyAction();

public:
	virtual void invoke(const qs::ActionEvent& event);
	virtual bool isEnabled(const qs::ActionEvent& event);

private:
	bool emptyFolders(const Account::FolderList& listFolder) const;
	void getFolderList(const qs::ActionEvent& event,
					   Account::FolderList* pList) const;

private:
	FolderEmptyAction(const FolderEmptyAction&);
	FolderEmptyAction& operator=(const FolderEmptyAction&);

private:
	AccountManager* pAccountManager_;
	FolderSelectionModel* pFolderSelectionModel_;
	UndoManager* pUndoManager_;
	HWND hwnd_;
	qs::Profile* pProfile_;
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
	qs::Profile* pProfile_;
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
						 SyncManager* pSyncManager,
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
	SyncManager* pSyncManager_;
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
					   SyncManager* pSyncManager,
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
	SyncManager* pSyncManager_;
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
 * FolderSubscribeAction
 *
 */

class FolderSubscribeAction : public qs::AbstractAction
{
public:
	FolderSubscribeAction(Document* pDocument,
						  PasswordManager* pPasswordManager,
						  FolderSelectionModel* pFolderSelectionModel,
						  SyncManager* pSyncManager,
						  HWND hwnd);
	virtual ~FolderSubscribeAction();

public:
	virtual void invoke(const qs::ActionEvent& event);
	virtual bool isEnabled(const qs::ActionEvent& event);
	virtual qs::wstring_ptr getText(const qs::ActionEvent& event);

public:
	static void subscribe(Document* pDocument,
						  Account* pAccount,
						  Folder* pFolder,
						  PasswordManager* pPasswordManager,
						  HWND hwnd,
						  void* pParam);

private:
	FolderSubscribeAction(const FolderSubscribeAction&);
	FolderSubscribeAction& operator=(const FolderSubscribeAction&);

private:
	Document* pDocument_;
	PasswordManager* pPasswordManager_;
	FolderSelectionModel* pFolderSelectionModel_;
	SyncManager* pSyncManager_;
	HWND hwnd_;
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
					   SyncManager* pSyncManager,
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
	SyncManager* pSyncManager_;
	HWND hwnd_;
};


/****************************************************************************
 *
 * HelpAboutAction
 *
 */

class HelpAboutAction : public qs::AbstractAction
{
public:
	explicit HelpAboutAction(HWND hwnd);
	virtual ~HelpAboutAction();

public:
	virtual void invoke(const qs::ActionEvent& event);

private:
	HelpAboutAction(const HelpAboutAction&);
	HelpAboutAction& operator=(const HelpAboutAction&);

private:
	HWND hwnd_;
};


/****************************************************************************
 *
 * HelpCheckUpdateAction
 *
 */

class HelpCheckUpdateAction : public qs::AbstractAction
{
public:
	HelpCheckUpdateAction(UpdateChecker* pUpdateChecker,
						  qs::Profile* pProfile,
						  HWND hwnd);
	virtual ~HelpCheckUpdateAction();

public:
	virtual void invoke(const qs::ActionEvent& event);

private:
	HelpCheckUpdateAction(const HelpCheckUpdateAction&);
	HelpCheckUpdateAction& operator=(const HelpCheckUpdateAction&);

private:
	UpdateChecker* pUpdateChecker_;
	qs::Profile* pProfile_;
	HWND hwnd_;
};


/****************************************************************************
 *
 * HelpOpenURLAction
 *
 */

class HelpOpenURLAction : public qs::AbstractAction
{
public:
	HelpOpenURLAction(qs::Profile* pProfile,
					  HWND hwnd);
	virtual ~HelpOpenURLAction();

public:
	virtual void invoke(const qs::ActionEvent& event);

private:
	HelpOpenURLAction(const HelpOpenURLAction&);
	HelpOpenURLAction& operator=(const HelpOpenURLAction&);

private:
	qs::Profile* pProfile_;
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
						   UndoManager* pUndoManager,
						   ViewModelManager* pViewModelManager,
						   bool bAll,
						   SecurityModel* pSecurityModel,
						   Document* pDocument,
						   const ActionInvoker* pActionInvoker,
						   HWND hwnd,
						   qs::Profile* pProfile);
	MessageApplyRuleAction(RuleManager* pRuleManager,
						   UndoManager* pUndoManager,
						   MessageSelectionModel* pMessageSelectionModel,
						   SecurityModel* pSecurityModel,
						   Document* pDocument,
						   const ActionInvoker* pActionInvoker,
						   HWND hwnd,
						   qs::Profile* pProfile);
	virtual ~MessageApplyRuleAction();

public:
	virtual void invoke(const qs::ActionEvent& event);
	virtual bool isEnabled(const qs::ActionEvent& event);

private:
	bool applyRule(Account** ppAccount) const;

private:
	MessageApplyRuleAction(const MessageApplyRuleAction&);
	MessageApplyRuleAction& operator=(const MessageApplyRuleAction&);

private:
	RuleManager* pRuleManager_;
	UndoManager* pUndoManager_;
	ViewModelManager* pViewModelManager_;
	MessageSelectionModel* pMessageSelectionModel_;
	bool bAll_;
	SecurityModel* pSecurityModel_;
	Document* pDocument_;
	const ActionInvoker* pActionInvoker_;
	HWND hwnd_;
	qs::Profile* pProfile_;
};


/****************************************************************************
 *
 * MessageApplyRuleBackgroundAction
 *
 */

class MessageApplyRuleBackgroundAction : public qs::AbstractAction
{
public:
	MessageApplyRuleBackgroundAction(SyncManager* pSyncManager,
									 Document* pDocument,
									 FolderModelBase* pFolderModel,
									 SyncDialogManager* pSyncDialogManager,
									 bool bAll,
									 HWND hwnd);
	virtual ~MessageApplyRuleBackgroundAction();

public:
	virtual void invoke(const qs::ActionEvent& event);
	virtual bool isEnabled(const qs::ActionEvent& event);

private:
	MessageApplyRuleBackgroundAction(const MessageApplyRuleBackgroundAction&);
	MessageApplyRuleBackgroundAction& operator=(const MessageApplyRuleBackgroundAction&);

private:
	SyncManager* pSyncManager_;
	Document* pDocument_;
	FolderModelBase* pFolderModel_;
	SyncDialogManager* pSyncDialogManager_;
	bool bAll_;
	HWND hwnd_;
};


/****************************************************************************
 *
 * MessageCertificateAction
 *
 */

class MessageCertificateAction : public qs::AbstractAction
{
public:
	explicit MessageCertificateAction(MessageWindow* pMessageWindow);
	virtual ~MessageCertificateAction();

public:
	virtual void invoke(const qs::ActionEvent& event);
	virtual bool isEnabled(const qs::ActionEvent& event);

private:
	MessageCertificateAction(const MessageCertificateAction&);
	MessageCertificateAction& operator=(const MessageCertificateAction&);

private:
	MessageWindow* pMessageWindow_;
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
						EditFrameWindowManager* pEditFrameWindowManager,
						ExternalEditorManager* pExternalEditorManager,
						const ActionInvoker* pActionInvoker,
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
									 AccountSelectionModel* pAccountSelectionModel,
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
								AccountSelectionModel* pAccountSelectionModel,
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
 * MessageLabelAction
 *
 */

class MessageLabelAction : public qs::AbstractAction
{
public:
	MessageLabelAction(MessageSelectionModel* pModel,
					   UndoManager* pUndoManager,
					   qs::Profile* pProfile,
					   HWND hwnd);
	virtual ~MessageLabelAction();

public:
	virtual void invoke(const qs::ActionEvent& event);
	virtual bool isEnabled(const qs::ActionEvent& event);

private:
	MessageLabelAction(const MessageLabelAction&);
	MessageLabelAction& operator=(const MessageLabelAction&);

private:
	MessageSelectionModel* pModel_;
	UndoManager* pUndoManager_;
	qs::Profile* pProfile_;
	HWND hwnd_;
};


/****************************************************************************
 *
 * MessageMacroAction
 *
 */

class MessageMacroAction : public qs::AbstractAction
{
public:
	MessageMacroAction(MessageSelectionModel* pMessageSelectionModel,
					   SecurityModel* pSecurityModel,
					   Document* pDocument,
					   const ActionInvoker* pActionInvoker,
					   qs::Profile* pProfile,
					   HWND hwnd);
	MessageMacroAction(FolderSelectionModel* pFolderSelectionModel,
					   SecurityModel* pSecurityModel,
					   Document* pDocument,
					   const ActionInvoker* pActionInvoker,
					   qs::Profile* pProfile,
					   HWND hwnd);
	virtual ~MessageMacroAction();

public:
	virtual void invoke(const qs::ActionEvent& event);
	virtual bool isEnabled(const qs::ActionEvent& event);

private:
	bool eval(const Macro* pMacro,
			  MessageEnumerator* pEnum,
			  const MessageHolderList& listSelected,
			  MacroVariableHolder* pGlobalVariable) const;

private:
	MessageMacroAction(const MessageMacroAction&);
	MessageMacroAction& operator=(const MessageMacroAction&);

private:
	MessageSelectionModel* pMessageSelectionModel_;
	FolderSelectionModel* pFolderSelectionModel_;
	SecurityModel* pSecurityModel_;
	Document* pDocument_;
	const ActionInvoker* pActionInvoker_;
	qs::Profile* pProfile_;
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
	MessageMoveAction(AccountManager* pAccountManager,
					  MessageSelectionModel* pMessageSelectionModel,
					  ViewModelHolder* pViewModelHolder,
					  MessageModel* pMessageModel,
					  bool bCopy,
					  bool bDontSelectNextIfDeletedFlag,
					  UndoManager* pUndoManager,
					  const FolderImage* pFolderImage,
					  qs::Profile* pProfile,
					  HWND hwnd);
	virtual ~MessageMoveAction();

public:
	virtual void invoke(const qs::ActionEvent& event);
	virtual bool isEnabled(const qs::ActionEvent& event);

private:
	bool moveMessages(const MessageHolderList& l,
					  Folder* pFolderFrom,
					  NormalFolder* pFolderTo,
					  bool bMove) const;

private:
	MessageMoveAction(const MessageMoveAction&);
	MessageMoveAction& operator=(const MessageMoveAction&);

private:
	AccountManager* pAccountManager_;
	MessageSelectionModel* pMessageSelectionModel_;
	ViewModelHolder* pViewModelHolder_;
	MessageModel* pMessageModel_;
	bool bCopy_;
	bool bDontSelectNextIfDeletedFlag_;
	UndoManager* pUndoManager_;
	const FolderImage* pFolderImage_;
	qs::Profile* pProfile_;
	HWND hwnd_;
};


/****************************************************************************
 *
 * MessageOpenAction
 *
 */

class MessageOpenAction : public qs::AbstractAction
{
public:
	MessageOpenAction(const URIResolver* pURIResolver,
					  ViewModelManager* pViewModelManager,
					  FolderModel* pFolderModel,
					  MessageFrameWindowManager* pMessageFrameWindowManager,
					  HWND hwnd);
	MessageOpenAction(const URIResolver* pURIResolver,
					  ViewModelManager* pViewModelManager,
					  ViewModelHolder* pViewModelHolder,
					  MessageModel* pMessageModel,
					  MessageFrameWindowManager* pMessageFrameWindowManager,
					  HWND hwnd);
	virtual ~MessageOpenAction();

public:
	virtual void invoke(const qs::ActionEvent& event);

private:
	MessageOpenAction(const MessageOpenAction&);
	MessageOpenAction& operator=(const MessageOpenAction&);

private:
	const URIResolver* pURIResolver_;
	ViewModelManager* pViewModelManager_;
	FolderModel* pFolderModel_;
	ViewModelHolder* pViewModelHolder_;
	MessageModel* pMessageModel_;
	MessageFrameWindowManager* pMessageFrameWindowManager_;
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
	MessageOpenAttachmentAction(const URIResolver* pURIResolver,
								SecurityModel* pSecurityModel,
								MessageFrameWindowManager* pMessageFrameWindowManager,
								TempFileCleaner* pTempFileCleaner,
								qs::Profile* pProfile,
								HWND hwnd);
	virtual ~MessageOpenAttachmentAction();

public:
	virtual void invoke(const qs::ActionEvent& event);
	virtual bool isEnabled(const qs::ActionEvent& event);

private:
	MessageOpenAttachmentAction(const MessageOpenAttachmentAction&);
	MessageOpenAttachmentAction& operator=(const MessageOpenAttachmentAction&);

private:
	const URIResolver* pURIResolver_;
	SecurityModel* pSecurityModel_;
	AttachmentHelper helper_;
	HWND hwnd_;
};


/****************************************************************************
 *
 * MessageOpenFocusedAction
 *
 */

class MessageOpenFocusedAction : public qs::AbstractAction
{
public:
	MessageOpenFocusedAction(ViewModelManager* pViewModelManager,
							 MessageFrameWindowManager* pMessageFrameWindowManager,
							 HWND hwnd);
	virtual ~MessageOpenFocusedAction();

public:
	virtual void invoke(const qs::ActionEvent& event);

private:
	MessageOpenFocusedAction(const MessageOpenFocusedAction&);
	MessageOpenFocusedAction& operator=(const MessageOpenFocusedAction&);

private:
	ViewModelManager* pViewModelManager_;
	MessageFrameWindowManager* pMessageFrameWindowManager_;
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
						  qs::Profile* pProfile,
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
	qs::Profile* pProfile_;
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
							const URIResolver* pURIResolver,
							ViewModelManager* pViewModelManager,
							FolderModel* pFolderModel,
							MainWindow* pMainWindow,
							MessageFrameWindowManager* pMessageFrameWindowManager,
							qs::Profile* pProfile,
							HWND hwnd);
	virtual ~MessageOpenRecentAction();

public:
	virtual void invoke(const qs::ActionEvent& event);
	virtual bool isEnabled(const qs::ActionEvent& event);

private:
	MessageOpenRecentAction(const MessageOpenRecentAction&);
	MessageOpenRecentAction& operator=(const MessageOpenRecentAction&);

private:
	Recents* pRecents_;
	const URIResolver* pURIResolver_;
	ViewModelManager* pViewModelManager_;
	FolderModel* pFolderModel_;
	MainWindow* pMainWindow_;
	MessageFrameWindowManager* pMessageFrameWindowManager_;
	qs::Profile* pProfile_;
	HWND hwnd_;
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
						 PasswordManager* pPasswordManager,
						 FolderModelBase* pFolderModel,
						 MessageSelectionModel* pMessageSelectionModel,
						 SecurityModel* pSecurityModel,
						 EditFrameWindowManager* pEditFrameWindowManager,
						 ExternalEditorManager* pExternalEditorManager,
						 const ActionInvoker* pActionInvoker,
						 HWND hwnd,
						 qs::Profile* pProfile,
						 bool bExternalEditor);
	virtual ~MessageOpenURLAction();

public:
	virtual void invoke(const qs::ActionEvent& event);

private:
	void openMailtoURL(const WCHAR* pwszURL,
					   const WCHAR* pwszAttachmentPath,
					   bool bExternalEditor) const;
	void openFeedURL(const WCHAR* pwszURL) const;

private:
	MessageOpenURLAction(const MessageOpenURLAction&);
	MessageOpenURLAction& operator=(const MessageOpenURLAction&);

private:
	TemplateProcessor processor_;
	Document* pDocument_;
	PasswordManager* pPasswordManager_;
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
						ActionInvoker* pActionInvoker,
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
	ActionInvoker* pActionInvoker_;
	HWND hwnd_;
	qs::Profile* pProfile_;
};


/****************************************************************************
 *
 * NoneAction
 *
 */

class NoneAction : public qs::AbstractAction
{
public:
	NoneAction();
	virtual ~NoneAction();

public:
	virtual void invoke(const qs::ActionEvent& event);
	virtual bool isEnabled(const qs::ActionEvent& event);

private:
	NoneAction(const NoneAction&);
	NoneAction& operator=(const NoneAction&);
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
	explicit TabSelectAction(TabModel* pTabModel);
	virtual ~TabSelectAction();

public:
	virtual void invoke(const qs::ActionEvent& event);
	virtual bool isEnabled(const qs::ActionEvent& event);

private:
	int getItem(const qs::ActionParam* pParam) const;

private:
	TabSelectAction(const TabSelectAction&);
	TabSelectAction& operator=(const TabSelectAction&);

private:
	TabModel* pTabModel_;
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
					  const FolderImage* pFolderImage,
					  OptionDialogManager* pOptionDialogManager,
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
	const FolderImage* pFolderImage_;
	OptionDialogManager* pOptionDialogManager_;
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
						 Security* pSecurity,
						 MessageSelectionModel* pMessageSelectionModel,
						 SecurityModel* pSecurityModel,
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
	Security* pSecurity_;
	MessageSelectionModel* pMessageSelectionModel_;
	SecurityModel* pSecurityModel_;
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
					  HWND hwnd);
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
};


/****************************************************************************
 *
 * ToolInvokeActionAction
 *
 */

class ToolInvokeActionAction : public qs::AbstractAction
{
private:
	typedef std::vector<qs::WSTRING> ActionList;

public:
	ToolInvokeActionAction(const ActionInvoker* pActionInvoker,
						   qs::Profile* pProfile,
						   HWND hwnd);
	virtual ~ToolInvokeActionAction();

public:
	virtual void invoke(const qs::ActionEvent& event);

private:
	static void parseActions(const WCHAR* pwszActions,
							 ActionList* pList);

private:
	ToolInvokeActionAction(const ToolInvokeActionAction&);
	ToolInvokeActionAction& operator=(const ToolInvokeActionAction&);

private:
	const ActionInvoker* pActionInvoker_;
	qs::Profile* pProfile_;
	HWND hwnd_;
};


/****************************************************************************
 *
 * ToolOptionsAction
 *
 */

class ToolOptionsAction : public qs::AbstractAction
{
public:
	ToolOptionsAction(OptionDialogManager* pOptionDialogManager,
					  AccountSelectionModel* pAccountSelectionModel,
					  HWND hwnd,
					  OptionDialog::Panel panel);
	virtual ~ToolOptionsAction();

public:
	virtual void invoke(const qs::ActionEvent& event);
	virtual bool isEnabled(const qs::ActionEvent& event);

private:
	ToolOptionsAction(const ToolOptionsAction&);
	ToolOptionsAction& operator=(const ToolOptionsAction&);

private:
	OptionDialogManager* pOptionDialogManager_;
	AccountSelectionModel* pAccountSelectionModel_;
	HWND hwnd_;
	OptionDialog::Panel panel_;
};


/****************************************************************************
 *
 * ToolPopupMenuAction
 *
 */

class ToolPopupMenuAction : public qs::AbstractAction
{
public:
	ToolPopupMenuAction(qs::MenuManager* pMenuManager,
						HWND hwnd);
	virtual ~ToolPopupMenuAction();

public:
	virtual void invoke(const qs::ActionEvent& event);

private:
	ToolPopupMenuAction(const ToolPopupMenuAction&);
	ToolPopupMenuAction& operator=(const ToolPopupMenuAction&);

private:
	qs::MenuManager* pMenuManager_;
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
	ToolScriptAction(ScriptManager* pScriptManager,
					 Document* pDocument,
					 qs::Profile* pProfile,
					 MainWindow* pMainWindow);
	ToolScriptAction(ScriptManager* pScriptManager,
					 Document* pDocument,
					 qs::Profile* pProfile,
					 EditFrameWindow* pEditFrameWindow);
	ToolScriptAction(ScriptManager* pScriptManager,
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
	ScriptManager* pScriptManager_;
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
						 AccountSelectionModel* pAccountSelectionModel,
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
	AccountSelectionModel* pAccountSelectionModel_;
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
	enum Type {
		TYPE_SENDRECEIVE,
		TYPE_SEND,
		TYPE_RECEIVE,
		TYPE_RECEIVEFOLDER
	};

public:
	ToolSyncAction(SyncManager* pSyncManager,
				   Document* pDocument,
				   FolderModelBase* pFolderModel,
				   SyncDialogManager* pSyncDialogManager,
				   Type type,
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
	FolderModelBase* pFolderModel_;
	SyncDialogManager* pSyncDialogManager_;
	Type type_;
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
	virtual ~ViewEncodingAction();

public:
	virtual void invoke(const qs::ActionEvent& event);
	virtual bool isEnabled(const qs::ActionEvent& event);
	virtual bool isChecked(const qs::ActionEvent& event);

private:
	ViewEncodingAction(const ViewEncodingAction&);
	ViewEncodingAction& operator=(const ViewEncodingAction&);

private:
	EncodingModel* pEncodingModel_;
};


/****************************************************************************
 *
 * ViewFilterAction
 *
 */

class ViewFilterAction : public qs::AbstractAction
{
public:
	explicit ViewFilterAction(ViewModelManager* pViewModelManager);
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
 * ViewFitAction
 *
 */

class ViewFitAction : public qs::AbstractAction
{
public:
	explicit ViewFitAction(MessageViewModeHolder* pMessageViewModeHolder);
	virtual ~ViewFitAction();

public:
	virtual void invoke(const qs::ActionEvent& event);
	virtual bool isEnabled(const qs::ActionEvent& event);
	virtual bool isChecked(const qs::ActionEvent& event);

private:
	static std::pair<MessageViewMode::Fit, bool> getParam(const qs::ActionParam* pParam);

private:
	ViewFitAction(const ViewFitAction&);
	ViewFitAction& operator=(const ViewFitAction&);

private:
	MessageViewModeHolder* pMessageViewModeHolder_;
};


/****************************************************************************
 *
 * ViewFocusAction
 *
 */

class ViewFocusAction : public qs::AbstractAction
{
public:
	enum Type {
		TYPE_VIEW,
		TYPE_NEXT,
		TYPE_PREV
	};

public:
	ViewFocusAction(View* pViews[],
					size_t nViewCount,
					Type type);
	ViewFocusAction(View* pViews[],
					size_t nViewCount,
					const WCHAR* pwszNames[]);
	virtual ~ViewFocusAction();

public:
	virtual void invoke(const qs::ActionEvent& event);

private:
	ViewFocusAction(const ViewFocusAction&);
	ViewFocusAction& operator=(const ViewFocusAction&);

private:
	typedef std::vector<View*> ViewList;
	typedef std::vector<const WCHAR*> NameList;

private:
	ViewList listView_;
	Type type_;
	NameList listName_;
};


/****************************************************************************
 *
 * ViewFocusItemAction
 *
 */

class ViewFocusItemAction : public qs::AbstractAction
{
public:
	enum Type {
		TYPE_ITEM,
		TYPE_NEXT,
		TYPE_PREV
	};

public:
	ViewFocusItemAction(FocusControllerBase* pFocusController,
						Type type);
	virtual ~ViewFocusItemAction();

public:
	virtual void invoke(const qs::ActionEvent& event);

private:
	ViewFocusItemAction(const ViewFocusItemAction&);
	ViewFocusItemAction& operator=(const ViewFocusItemAction&);

private:
	FocusControllerBase* pFocusController_;
	Type type_;
};


/****************************************************************************
 *
 * ViewFontGroupAction
 *
 */

class ViewFontGroupAction : public qs::AbstractAction
{
public:
	explicit ViewFontGroupAction(MessageWindow* pMessageWindow);
	virtual ~ViewFontGroupAction();

public:
	virtual void invoke(const qs::ActionEvent& event);
	virtual bool isChecked(const qs::ActionEvent& event);

private:
	ViewFontGroupAction(const ViewFontGroupAction&);
	ViewFontGroupAction& operator=(const ViewFontGroupAction&);

private:
	MessageWindow* pMessageWindow_;
};


/****************************************************************************
 *
 * ViewLockPreviewAction
 *
 */

class ViewLockPreviewAction : public qs::AbstractAction
{
public:
	ViewLockPreviewAction(PreviewMessageModel* pPreviewMessageModel,
						  MainWindow* pMainWindow);
	virtual ~ViewLockPreviewAction();

public:
	virtual void invoke(const qs::ActionEvent& event);
	virtual bool isEnabled(const qs::ActionEvent& event);

private:
	ViewLockPreviewAction(const ViewLockPreviewAction&);
	ViewLockPreviewAction& operator=(const ViewLockPreviewAction&);

private:
	PreviewMessageModel* pPreviewMessageModel_;
	MainWindow* pMainWindow_;
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
						  MessageViewMode::Mode exclusiveMode,
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
	MessageViewMode::Mode exclusiveMode_;
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
		TYPE_PREVACCOUNT,
		TYPE_SELECT
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
		TYPE_NEXT					= 0x01,
		TYPE_PREV					= 0x02,
		TYPE_NEXTUNSEEN				= 0x03,
		TYPE_NEXTPAGE				= 0x04,
		TYPE_PREVPAGE				= 0x05,
		TYPE_SELF					= 0x06,
		TYPE_TYPE_MASK				= 0x0f,
		
		TYPE_NEXTPAGEUNSEEN			= 0x10,
		TYPE_UNSEENINOTHERACCOUNT	= 0x20
	};

public:
	ViewNavigateMessageAction(ViewModelManager* pViewModelManager,
							  FolderModel* pFolderModel,
							  MainWindow* pMainWindow,
							  MessageWindow* pMessageWindow,
							  AccountManager* pAccountManager,
							  qs::Profile* pProfile,
							  Type type);
	ViewNavigateMessageAction(ViewModelManager* pViewModelManager,
							  ViewModelHolder* pViewModelHolder,
							  MessageWindow* pMessageWindow,
							  AccountManager* pAccountManager,
							  qs::Profile* pProfile,
							  Type type);
	virtual ~ViewNavigateMessageAction();

public:
	virtual void invoke(const qs::ActionEvent& event);
	virtual bool isEnabled(const qs::ActionEvent& event);

private:
	void init(qs::Profile* pProfile);
	bool isPreview() const;
	ViewModel* getViewModel() const;
	std::pair<ViewModel*, unsigned int> getNextUnseen(ViewModel* pViewModel,
													  unsigned int nIndex) const;
	Folder* getNextUnseenFolder(Account* pAccount,
								Folder* pFolderStart) const;
	Folder* getNextUnseenFolder(Account* pAccountStart) const;
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
	AccountManager* pAccountManager_;
	unsigned int nType_;
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
					  FolderModelBase* pFolderModel,
					  SecurityModel* pSecurityModel,
					  SyncDialogManager* pSyncDialogManager,
					  ActionInvoker* pActionInvoker,
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
	FolderModelBase* pFolderModel_;
	SecurityModel* pSecurityModel_;
	SyncDialogManager* pSyncDialogManager_;
	ActionInvoker* pActionInvoker_;
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
 * ViewScrollMessageAction
 *
 */

class ViewScrollMessageAction : public qs::AbstractAction
{
public:
	enum Scroll {
		SCROLL_PAGEUP			= 0x01,
		SCROLL_PAGEDOWN			= 0x02
	};

public:
	ViewScrollMessageAction(MessageWindow* pMessageWindow,
							Scroll scroll);
	virtual ~ViewScrollMessageAction();

public:
	virtual void invoke(const qs::ActionEvent& event);

private:
	ViewScrollMessageAction(const ViewScrollMessageAction&);
	ViewScrollMessageAction& operator=(const ViewScrollMessageAction&);

private:
	MessageWindow* pMessageWindow_;
	Scroll scroll_;
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
	virtual bool isEnabled(const qs::ActionEvent& event);

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
	explicit ViewSortAction(ViewModelManager* pViewModelManager);
	virtual ~ViewSortAction();

public:
	virtual void invoke(const qs::ActionEvent& event);
	virtual bool isEnabled(const qs::ActionEvent& event);
	virtual bool isChecked(const qs::ActionEvent& event);

private:
	unsigned int getColumn(const ViewModel* pViewModel,
						   const qs::ActionParam* pParam) const;

private:
	ViewSortAction(const ViewSortAction&);
	ViewSortAction& operator=(const ViewSortAction&);

private:
	ViewModelManager* pViewModelManager_;
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
 * ViewSortThreadAction
 *
 */

class ViewSortThreadAction : public qs::AbstractAction
{
public:
	enum Type {
		TYPE_FLAT,
		TYPE_THREAD,
		TYPE_FLOATTHREAD,
		
		TYPE_TOGGLETHREAD
	};

public:
	ViewSortThreadAction(ViewModelManager* pViewModelManager,
						 Type type);
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
	Type type_;
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
	virtual ~ViewTemplateAction();

public:
	virtual void invoke(const qs::ActionEvent& event);
	virtual bool isEnabled(const qs::ActionEvent& event);
	virtual bool isChecked(const qs::ActionEvent& event);

private:
	Account* getAccount() const;

private:
	ViewTemplateAction(const ViewTemplateAction&);
	ViewTemplateAction& operator=(const ViewTemplateAction&);

private:
	MessageWindow* pMessageWindow_;
};


/****************************************************************************
 *
 * ViewZoomAction
 *
 */

class ViewZoomAction : public qs::AbstractAction
{
private:
	enum Type {
		TYPE_ZOOM,
		TYPE_INCREMENT,
		TYPE_DECREMENT,
		TYPE_ERROR
	};

public:
	explicit ViewZoomAction(MessageViewModeHolder* pMessageViewModeHolder);
	virtual ~ViewZoomAction();

public:
	virtual void invoke(const qs::ActionEvent& event);
	virtual bool isEnabled(const qs::ActionEvent& event);
	virtual bool isChecked(const qs::ActionEvent& event);

private:
	static std::pair<Type, unsigned int> getParam(const qs::ActionParam* pParam);

private:
	ViewZoomAction(const ViewZoomAction&);
	ViewZoomAction& operator=(const ViewZoomAction&);

private:
	MessageViewModeHolder* pMessageViewModeHolder_;
};

}

#include "action.inl"

#endif // __ACTION_H__
