/*
 * $Id$
 *
 * Copyright(C) 1998-2003 Satoshi Nakamura
 * All rights reserved.
 *
 */

#ifndef __ACTION_H__
#define __ACTION_H__

#include <qm.h>

#include <qsaction.h>
#include <qsstream.h>

#include "templateprocessor.h"
#include "../ui/messagecomposer.h"
#include "../ui/messagewindow.h"


namespace qm {

struct ActionParam;
class DispatchAction;
class EditClearDeletedAction;
class EditCommandAction;
class EditCopyMessageAction;
class EditCutMessageAction;
class EditDeleteMessageAction;
class EditFindAction;
class EditPasteMessageAction;
class EditSelectAllMessageAction;
class FileCloseAction;
class FileEmptyTrashAction;
class FileExitAction;
class FileExportAction;
class FileImportAction;
class FileOfflineAction;
class FilePrintAction;
class FileSaveAction;
class FolderCompactAction;
class FolderCreateAction;
class FolderDeleteAction;
class FolderPropertyAction;
class FolderUpdateAction;
class MessageApplyRuleAction;
class MessageApplyTemplateAction;
class MessageCreateAction;
class MessageCreateFromClipboardAction;
class MessageDetachAction;
class MessageMarkAction;
class MessageMoveAction;
class MessageMoveOtherAction;
class MessageOpenAttachmentAction;
class MessagePropertyAction;
class ToolAccountAction;
class ToolDialupAction;
class ToolGoRoundAction;
class ToolOptionsAction;
class ToolScriptAction;
class ToolSubAccountAction;
class ToolSyncAction;
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
class ViewScrollAction;
class ViewSelectModeAction;
template<class WindowX> class ViewShowControlAction;
	class ViewShowFolderAction;
	class ViewShowHeaderAction;
	class ViewShowHeaderColumnAction;
	class ViewShowPreviewAction;
	template<class WindowX> class ViewShowStatusBarAction;
	template<class WindowX> class ViewShowToolbarAction;
class ViewSortAction;
class ViewSortDirectionAction;
class ViewSortThreadAction;
class ViewTemplateAction;

class AttachmentMenu;
class Document;
class EditFrameWindow;
class EditFrameWindowManager;
class EncodingMenu;
class ExternalEditorManager;
class FilterManager;
class FindReplaceManager;
class FolderModel;
class FolderSelectionModel;
class GoRound;
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
class ScriptMenu;
class SyncDialogManager;
class SyncFilterManager;
class SyncManager;
class TempFileCleaner;
class Template;
class TemplateMenu;
class View;
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
 * DispatchAction
 *
 */

class DispatchAction : public qs::AbstractAction
{
public:
	DispatchAction(View* pViews[], qs::Action* pActions[],
		size_t nCount, qs::QSTATUS* pstatus);
	virtual ~DispatchAction();

public:
	virtual qs::QSTATUS invoke(const qs::ActionEvent& event);
	virtual qs::QSTATUS isEnabled(const qs::ActionEvent& event, bool* pbEnabled);
	virtual qs::QSTATUS isChecked(const qs::ActionEvent& event, bool* pbChecked);
	virtual qs::QSTATUS getText(const qs::ActionEvent& event, qs::WSTRING* pwstrText);

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
	EditClearDeletedAction(FolderModel* pFolderModel, qs::QSTATUS* pstatus);
	virtual ~EditClearDeletedAction();

public:
	virtual qs::QSTATUS invoke(const qs::ActionEvent& event);
	virtual qs::QSTATUS isEnabled(const qs::ActionEvent& event, bool* pbEnabled);

private:
	EditClearDeletedAction(const EditClearDeletedAction&);
	EditClearDeletedAction& operator=(const EditClearDeletedAction&);

private:
	FolderModel* pFolderModel_;
};


/****************************************************************************
 *
 * EditCommandAction
 *
 */

class EditCommandAction : public qs::AbstractAction
{
public:
	typedef qs::QSTATUS (MessageWindowItem::*PFN_DO)();
	typedef qs::QSTATUS (MessageWindowItem::*PFN_CANDO)(bool* pbCan);

public:
	EditCommandAction(MessageWindow* pMessageWindow,
		PFN_DO pfnDo, PFN_CANDO pfnCanDo, qs::QSTATUS* pstatus);
	virtual ~EditCommandAction();

public:
	virtual qs::QSTATUS invoke(const qs::ActionEvent& event);
	virtual qs::QSTATUS isEnabled(const qs::ActionEvent& event, bool* pbEnabled);

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
	EditCopyMessageAction(FolderModel* pFolderModel,
		MessageSelectionModel* pMessageSelectionModel, qs::QSTATUS* pstatus);
	virtual ~EditCopyMessageAction();

public:
	virtual qs::QSTATUS invoke(const qs::ActionEvent& event);
	virtual qs::QSTATUS isEnabled(const qs::ActionEvent& event, bool* pbEnabled);

private:
	EditCopyMessageAction(const EditCopyMessageAction&);
	EditCopyMessageAction& operator=(const EditCopyMessageAction&);

private:
	FolderModel* pFolderModel_;
	MessageSelectionModel* pMessageSelectionModel_;
};


/****************************************************************************
 *
 * EditCutMessageAction
 *
 */

class EditCutMessageAction : public qs::AbstractAction
{
public:
	EditCutMessageAction(FolderModel* pFolderModel,
		MessageSelectionModel* pMessageSelectionModel, qs::QSTATUS* pstatus);
	virtual ~EditCutMessageAction();

public:
	virtual qs::QSTATUS invoke(const qs::ActionEvent& event);
	virtual qs::QSTATUS isEnabled(const qs::ActionEvent& event, bool* pbEnabled);

private:
	EditCutMessageAction(const EditCutMessageAction&);
	EditCutMessageAction& operator=(const EditCutMessageAction&);

private:
	FolderModel* pFolderModel_;
	MessageSelectionModel* pMessageSelectionModel_;
};


/****************************************************************************
 *
 * EditDeleteMessageAction
 *
 */

class EditDeleteMessageAction : public qs::AbstractAction
{
public:
	EditDeleteMessageAction(MessageSelectionModel* pModel,
		bool bDirect, qs::QSTATUS* pstatus);
	virtual ~EditDeleteMessageAction();

public:
	virtual qs::QSTATUS invoke(const qs::ActionEvent& event);
	virtual qs::QSTATUS isEnabled(const qs::ActionEvent& event, bool* pbEnabled);

private:
	EditDeleteMessageAction(const EditDeleteMessageAction&);
	EditDeleteMessageAction& operator=(const EditDeleteMessageAction&);

private:
	MessageSelectionModel* pModel_;
	bool bDirect_;
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
	EditFindAction(MessageWindow* pMessageWindow, qs::Profile* pProfile,
		FindReplaceManager* pFindReplaceManager, qs::QSTATUS* pstatus);
	EditFindAction(MessageWindow* pMessageWindow, bool bNext,
		FindReplaceManager* pFindReplaceManager, qs::QSTATUS* pstatus);
	virtual ~EditFindAction();

public:
	virtual qs::QSTATUS invoke(const qs::ActionEvent& event);
	virtual qs::QSTATUS isEnabled(const qs::ActionEvent& event, bool* pbEnabled);

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
		FolderModel* pModel, qs::QSTATUS* pstatus);
	virtual ~EditPasteMessageAction();

public:
	virtual qs::QSTATUS invoke(const qs::ActionEvent& event);
	virtual qs::QSTATUS isEnabled(const qs::ActionEvent& event, bool* pbEnabled);

private:
	EditPasteMessageAction(const EditPasteMessageAction&);
	EditPasteMessageAction& operator=(const EditPasteMessageAction&);

private:
	Document* pDocument_;
	FolderModel* pModel_;
};


/****************************************************************************
 *
 * EditSelectAllMessageAction
 *
 */

class EditSelectAllMessageAction : public qs::AbstractAction
{
public:
	EditSelectAllMessageAction(MessageSelectionModel* pMessageSelectionModel,
		qs::QSTATUS* pstatus);
	virtual ~EditSelectAllMessageAction();

public:
	virtual qs::QSTATUS invoke(const qs::ActionEvent& event);
	virtual qs::QSTATUS isEnabled(const qs::ActionEvent& event, bool* pbEnabled);

private:
	EditSelectAllMessageAction(const EditSelectAllMessageAction&);
	EditSelectAllMessageAction& operator=(const EditSelectAllMessageAction&);

private:
	MessageSelectionModel* pMessageSelectionModel_;
};


/****************************************************************************
 *
 * FileCloseAction
 *
 */

class FileCloseAction : public qs::AbstractAction
{
public:
	FileCloseAction(HWND hwnd, qs::QSTATUS* pstatus);
	virtual ~FileCloseAction();

public:
	virtual qs::QSTATUS invoke(const qs::ActionEvent& event);

private:
	FileCloseAction(const FileCloseAction&);
	FileCloseAction& operator=(const FileCloseAction&);

private:
	HWND hwnd_;
};


/****************************************************************************
 *
 * FileEmptyTrashAction
 *
 */

class FileEmptyTrashAction : public qs::AbstractAction
{
public:
	FileEmptyTrashAction(FolderModel* pFolderModel, qs::QSTATUS* pstatus);
	virtual ~FileEmptyTrashAction();

public:
	virtual qs::QSTATUS invoke(const qs::ActionEvent& event);
	virtual qs::QSTATUS isEnabled(const qs::ActionEvent& event, bool* pbEnabled);

private:
	NormalFolder* getTrash() const;

private:
	FileEmptyTrashAction(const FileEmptyTrashAction&);
	FileEmptyTrashAction& operator=(const FileEmptyTrashAction&);

private:
	FolderModel* pFolderModel_;
};


/****************************************************************************
 *
 * FileExitAction
 *
 */

class FileExitAction : public qs::AbstractAction
{
public:
	FileExitAction(qs::Window* pWindow, Document* pDocument,
		SyncManager* pSyncManager, TempFileCleaner* pTempFileCleaner,
		EditFrameWindowManager* pEditFrameWindowManager, qs::QSTATUS* pstatus);
	virtual ~FileExitAction();

public:
	virtual qs::QSTATUS invoke(const qs::ActionEvent& event);

private:
	FileExitAction(const FileExitAction&);
	FileExitAction& operator=(const FileExitAction&);

private:
	qs::Window* pWindow_;
	Document* pDocument_;
	SyncManager* pSyncManager_;
	TempFileCleaner* pTempFileCleaner_;
	EditFrameWindowManager* pEditFrameWindowManager_;
};


/****************************************************************************
 *
 * FileExportAction
 *
 */

class FileExportAction : public qs::AbstractAction
{
public:
	FileExportAction(MessageSelectionModel* pModel, qs::QSTATUS* pstatus);
	virtual ~FileExportAction();

public:
	virtual qs::QSTATUS invoke(const qs::ActionEvent& event);
	virtual qs::QSTATUS isEnabled(const qs::ActionEvent& event, bool* pbEnabled);

private:
	static qs::QSTATUS writeMessage(qs::OutputStream* pStream,
		const MessagePtr& ptr, bool bAddFlags, const Template* pTemplate,
		const WCHAR* pwszEncoding, bool bWriteSeparator);

private:
	FileExportAction(const FileExportAction&);
	FileExportAction& operator=(const FileExportAction&);

private:
	MessageSelectionModel* pModel_;
};


/****************************************************************************
 *
 * FileImportAction
 *
 */

class FileImportAction : public qs::AbstractAction
{
public:
	FileImportAction(FolderModel* pFolderModel, qs::QSTATUS* pstatus);
	virtual ~FileImportAction();

public:
	virtual qs::QSTATUS invoke(const qs::ActionEvent& event);
	virtual qs::QSTATUS isEnabled(const qs::ActionEvent& event, bool* pbEnabled);

public:
	static qs::QSTATUS readMessage(NormalFolder* pFolder,
		qs::InputStream* pStream, bool bMultiple, unsigned int nFlags);

private:
	static qs::QSTATUS readLine(qs::InputStream* pStream, CHAR cPrev,
		qs::STRING* pstrLine, CHAR* pcNext, bool* pbNewLine);

private:
	FileImportAction(const FileImportAction&);
	FileImportAction& operator=(const FileImportAction&);

private:
	FolderModel* pFolderModel_;
};


/****************************************************************************
 *
 * FileOfflineAction
 *
 */

class FileOfflineAction : public qs::AbstractAction
{
public:
	FileOfflineAction(Document* pDocument, qs::QSTATUS* pstatus);
	virtual ~FileOfflineAction();

public:
	virtual qs::QSTATUS invoke(const qs::ActionEvent& event);
	virtual qs::QSTATUS isChecked(const qs::ActionEvent& event, bool* pbChecked);

private:
	FileOfflineAction(const FileOfflineAction&);
	FileOfflineAction& operator=(const FileOfflineAction&);

private:
	Document* pDocument_;
};


/****************************************************************************
 *
 * FilePrintAction
 *
 */

class FilePrintAction : public qs::AbstractAction
{
public:
	FilePrintAction(MessageSelectionModel* pModel, qs::QSTATUS* pstatus);
	virtual ~FilePrintAction();

public:
	virtual qs::QSTATUS invoke(const qs::ActionEvent& event);
	virtual qs::QSTATUS isEnabled(const qs::ActionEvent& event, bool* pbEnabled);

private:
	FilePrintAction(const FilePrintAction&);
	FilePrintAction& operator=(const FilePrintAction&);

private:
	MessageSelectionModel* pModel_;
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
		ViewModelManager* pViewModelManager, qs::QSTATUS* pstatus);
	virtual ~FileSaveAction();

public:
	virtual qs::QSTATUS invoke(const qs::ActionEvent& event);

private:
	FileSaveAction(const FileSaveAction&);
	FileSaveAction& operator=(const FileSaveAction&);

private:
	Document* pDocument_;
	ViewModelManager* pViewModelManager_;
};


/****************************************************************************
 *
 * FolderCompactAction
 *
 */

class FolderCompactAction : public qs::AbstractAction
{
public:
	FolderCompactAction(FolderModel* pFolderModel, qs::QSTATUS* pstatus);
	virtual ~FolderCompactAction();

public:
	virtual qs::QSTATUS invoke(const qs::ActionEvent& event);
	virtual qs::QSTATUS isEnabled(const qs::ActionEvent& event, bool* pbEnabled);

private:
	FolderCompactAction(const FolderCompactAction&);
	FolderCompactAction& operator=(const FolderCompactAction&);

private:
	FolderModel* pFolderModel_;
};


/****************************************************************************
 *
 * FolderCreateAction
 *
 */

class FolderCreateAction : public qs::AbstractAction
{
public:
	FolderCreateAction(FolderSelectionModel* pFolderSelectionModel, qs::QSTATUS* pstatus);
	virtual ~FolderCreateAction();

public:
	virtual qs::QSTATUS invoke(const qs::ActionEvent& event);
	virtual qs::QSTATUS isEnabled(const qs::ActionEvent& event, bool* pbEnabled);

private:
	FolderCreateAction(const FolderCreateAction&);
	FolderCreateAction& operator=(const FolderCreateAction&);

private:
	FolderSelectionModel* pFolderSelectionModel_;
};


/****************************************************************************
 *
 * FolderDeleteAction
 *
 */

class FolderDeleteAction : public qs::AbstractAction
{
public:
	FolderDeleteAction(FolderSelectionModel* pFolderSelectionModel, qs::QSTATUS* pstatus);
	virtual ~FolderDeleteAction();

public:
	virtual qs::QSTATUS invoke(const qs::ActionEvent& event);
	virtual qs::QSTATUS isEnabled(const qs::ActionEvent& event, bool* pbEnabled);

private:
	FolderDeleteAction(const FolderDeleteAction&);
	FolderDeleteAction& operator=(const FolderDeleteAction&);

private:
	FolderSelectionModel* pFolderSelectionModel_;
};


/****************************************************************************
 *
 * FolderPropertyAction
 *
 */

class FolderPropertyAction : public qs::AbstractAction
{
public:
	FolderPropertyAction(FolderSelectionModel* pModel,
		HWND hwnd, qs::QSTATUS* pstatus);
	virtual ~FolderPropertyAction();

public:
	virtual qs::QSTATUS invoke(const qs::ActionEvent& event);
	virtual qs::QSTATUS isEnabled(const qs::ActionEvent& event, bool* pbEnabled);

private:
	FolderPropertyAction(const FolderPropertyAction&);
	FolderPropertyAction& operator=(const FolderPropertyAction&);

private:
	FolderSelectionModel* pModel_;
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
	FolderUpdateAction(FolderModel* pFolderModel, qs::QSTATUS* pstatus);
	virtual ~FolderUpdateAction();

public:
	virtual qs::QSTATUS invoke(const qs::ActionEvent& event);
	virtual qs::QSTATUS isEnabled(const qs::ActionEvent& event, bool* pbEnabled);

private:
	FolderUpdateAction(const FolderUpdateAction&);
	FolderUpdateAction& operator=(const FolderUpdateAction&);

private:
	FolderModel* pFolderModel_;
};


/****************************************************************************
 *
 * MessageApplyRuleAction
 *
 */

class MessageApplyRuleAction : public qs::AbstractAction
{
public:
	MessageApplyRuleAction(Document* pDocument, FolderModel* pFolderModel,
		HWND hwnd, qs::Profile* pProfile, qs::QSTATUS* pstatus);
	virtual ~MessageApplyRuleAction();

public:
	virtual qs::QSTATUS invoke(const qs::ActionEvent& event);
	virtual qs::QSTATUS isEnabled(const qs::ActionEvent& event, bool* pbEnabled);

private:
	MessageApplyRuleAction(const MessageApplyRuleAction&);
	MessageApplyRuleAction& operator=(const MessageApplyRuleAction&);

private:
	Document* pDocument_;
	FolderModel* pFolderModel_;
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
		Document* pDocument, FolderModel* pFolderModel,
		MessageSelectionModel* pMessageSelectionModel,
		EditFrameWindowManager* pEditFrameWindowManager,
		ExternalEditorManager* pExternalEditorManager, HWND hwnd,
		qs::Profile* pProfile, bool bExternalEditor, qs::QSTATUS* pstatus);
	virtual ~MessageApplyTemplateAction();

public:
	virtual qs::QSTATUS invoke(const qs::ActionEvent& event);
	virtual qs::QSTATUS isEnabled(const qs::ActionEvent& event, bool* pbEnabled);

private:
	MessageApplyTemplateAction(const MessageApplyTemplateAction&);
	MessageApplyTemplateAction& operator=(const MessageApplyTemplateAction&);

private:
	TemplateProcessor processor_;
	TemplateMenu* pTemplateMenu_;
};


/****************************************************************************
 *
 * MessageCreateAction
 *
 */

class MessageCreateAction : public qs::AbstractAction
{
public:
	MessageCreateAction(Document* pDocument, FolderModel* pFolderModel,
		MessageSelectionModel* pMessageSelectionModel,
		const WCHAR* pwszTemplateName,
		EditFrameWindowManager* pEditFrameWindowManager,
		ExternalEditorManager* pExternalEditorManager, HWND hwnd,
		qs::Profile* pProfile, bool bExternalEditor, qs::QSTATUS* pstatus);
	virtual ~MessageCreateAction();

public:
	virtual qs::QSTATUS invoke(const qs::ActionEvent& event);
	virtual qs::QSTATUS isEnabled(const qs::ActionEvent& event, bool* pbEnabled);

private:
	MessageCreateAction(const MessageCreateAction&);
	MessageCreateAction& operator=(const MessageCreateAction&);

private:
	TemplateProcessor processor_;
	FolderModel* pFolderModel_;
	qs::WSTRING wstrTemplateName_;
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
		Document* pDocument, qs::Profile* pProfile, HWND hwnd,
		FolderModel* pFolderModel, qs::QSTATUS* pstatus);
	virtual ~MessageCreateFromClipboardAction();

public:
	virtual qs::QSTATUS invoke(const qs::ActionEvent& event);
	virtual qs::QSTATUS isEnabled(const qs::ActionEvent& event, bool* pbEnabled);

private:
	MessageCreateFromClipboardAction(const MessageCreateFromClipboardAction&);
	MessageCreateFromClipboardAction& operator=(const MessageCreateFromClipboardAction&);

private:
	MessageComposer composer_;
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
		MessageSelectionModel* pMessageSelectionModel, qs::QSTATUS* pstatus);
	virtual ~MessageDetachAction();

public:
	virtual qs::QSTATUS invoke(const qs::ActionEvent& event);
	virtual qs::QSTATUS isEnabled(const qs::ActionEvent& event, bool* pbEnabled);

private:
	MessageDetachAction(const MessageDetachAction&);
	MessageDetachAction& operator=(const MessageDetachAction&);

private:
	qs::Profile* pProfile_;
	MessageSelectionModel* pMessageSelectionModel_;
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
		unsigned int nFlags, unsigned int nMask, qs::QSTATUS* pstatus);
	virtual ~MessageMarkAction();

public:
	virtual qs::QSTATUS invoke(const qs::ActionEvent& event);
	virtual qs::QSTATUS isEnabled(const qs::ActionEvent& event, bool* pbEnabled);

private:
	MessageMarkAction(const MessageMarkAction&);
	MessageMarkAction& operator=(const MessageMarkAction&);

private:
	MessageSelectionModel* pModel_;
	unsigned int nFlags_;
	unsigned int nMask_;
};


/****************************************************************************
 *
 * MessageMoveAction
 *
 */

class MessageMoveAction : public qs::AbstractAction
{
public:
	MessageMoveAction(MessageSelectionModel* pModel,
		MoveMenu* pMoveMenu, qs::QSTATUS* pstatus);
	virtual ~MessageMoveAction();

public:
	virtual qs::QSTATUS invoke(const qs::ActionEvent& event);
	virtual qs::QSTATUS isEnabled(const qs::ActionEvent& event, bool* pbEnabled);

private:
	MessageMoveAction(const MessageMoveAction&);
	MessageMoveAction& operator=(const MessageMoveAction&);

private:
	MessageSelectionModel* pModel_;
	MoveMenu* pMoveMenu_;
};


/****************************************************************************
 *
 * MessageMoveOtherAction
 *
 */

class MessageMoveOtherAction : public qs::AbstractAction
{
public:
	MessageMoveOtherAction(MessageSelectionModel* pModel, qs::QSTATUS* pstatus);
	virtual ~MessageMoveOtherAction();

public:
	virtual qs::QSTATUS invoke(const qs::ActionEvent& event);
	virtual qs::QSTATUS isEnabled(const qs::ActionEvent& event, bool* pbEnabled);

private:
	MessageMoveOtherAction(const MessageMoveOtherAction&);
	MessageMoveOtherAction& operator=(const MessageMoveOtherAction&);

private:
	MessageSelectionModel* pModel_;
};


/****************************************************************************
 *
 * MessageOpenAttachmentAction
 *
 */

class MessageOpenAttachmentAction : public qs::AbstractAction
{
public:
	MessageOpenAttachmentAction(qs::Profile* pProfile,
		AttachmentMenu* pAttachmentMenu, TempFileCleaner* pTempFileCleaner,
		qs::QSTATUS* pstatus);
	virtual ~MessageOpenAttachmentAction();

public:
	virtual qs::QSTATUS invoke(const qs::ActionEvent& event);

private:
	qs::Profile* pProfile_;
	AttachmentMenu* pAttachmentMenu_;
	TempFileCleaner* pTempFileCleaner_;
};


/****************************************************************************
 *
 * MessagePropertyAction
 *
 */

class MessagePropertyAction : public qs::AbstractAction
{
public:
	MessagePropertyAction(MessageSelectionModel* pModel,
		HWND hwnd, qs::QSTATUS* pstatus);
	virtual ~MessagePropertyAction();

public:
	virtual qs::QSTATUS invoke(const qs::ActionEvent& event);
	virtual qs::QSTATUS isEnabled(const qs::ActionEvent& event, bool* pbEnabled);

private:
	MessagePropertyAction(const MessagePropertyAction&);
	MessagePropertyAction& operator=(const MessagePropertyAction&);

private:
	MessageSelectionModel* pModel_;
	HWND hwnd_;
};


/****************************************************************************
 *
 * ToolAccountAction
 *
 */

class ToolAccountAction : public qs::AbstractAction
{
public:
	ToolAccountAction(Document* pDocument, FolderModel* pFolderModel,
		SyncFilterManager* pSyncFilterManager, qs::QSTATUS* pstatus);
	virtual ~ToolAccountAction();

public:
	virtual qs::QSTATUS invoke(const qs::ActionEvent& event);
	virtual qs::QSTATUS isEnabled(const qs::ActionEvent& event, bool* pbEnabled);

private:
	ToolAccountAction(const ToolAccountAction&);
	ToolAccountAction& operator=(const ToolAccountAction&);

private:
	Document* pDocument_;
	FolderModel* pFolderModel_;
	SyncFilterManager* pSyncFilterManager_;
};


/****************************************************************************
 *
 * ToolDialupAction
 *
 */

class ToolDialupAction : public qs::AbstractAction
{
public:
	ToolDialupAction(SyncManager* pSyncManager, Document* pDocument,
		SyncDialogManager* pSyncDialogManager, HWND hwnd, qs::QSTATUS* pstatus);
	virtual ~ToolDialupAction();

public:
	virtual qs::QSTATUS invoke(const qs::ActionEvent& event);
	virtual qs::QSTATUS getText(const qs::ActionEvent& event, qs::WSTRING* pwstrText);

private:
	qs::QSTATUS isConnected(bool* pbConnected) const;

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
	ToolGoRoundAction(SyncManager* pSyncManager, Document* pDocument,
		GoRound* pGoRound, SyncDialogManager* pSyncDialogManager,
		HWND hwnd, qs::QSTATUS* pstatus);
	virtual ~ToolGoRoundAction();

public:
	virtual qs::QSTATUS invoke(const qs::ActionEvent& event);
	virtual qs::QSTATUS isEnabled(const qs::ActionEvent& event, bool* pbEnabled);

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
 * ToolOptionsAction
 *
 */

class ToolOptionsAction : public qs::AbstractAction
{
public:
	ToolOptionsAction(qs::Profile* pProfile, qs::QSTATUS* pstatus);
	virtual ~ToolOptionsAction();

public:
	virtual qs::QSTATUS invoke(const qs::ActionEvent& event);
	virtual qs::QSTATUS isEnabled(const qs::ActionEvent& event, bool* pbEnabled);

private:
	ToolOptionsAction(const ToolOptionsAction&);
	ToolOptionsAction& operator=(const ToolOptionsAction&);

private:
	qs::Profile* pProfile_;
};


/****************************************************************************
 *
 * ToolScriptAction
 *
 */

class ToolScriptAction : public qs::AbstractAction
{
public:
	ToolScriptAction(ScriptMenu* pScriptMenu, Document* pDocument,
		qs::Profile* pProfile, MainWindow* pMainWindow, qs::QSTATUS* pstatus);
	ToolScriptAction(ScriptMenu* pScriptMenu, Document* pDocument,
		qs::Profile* pProfile, EditFrameWindow* pEditFrameWindow,
		qs::QSTATUS* pstatus);
	ToolScriptAction(ScriptMenu* pScriptMenu, Document* pDocument,
		qs::Profile* pProfile, MessageFrameWindow* pMessageFrameWindow,
		qs::QSTATUS* pstatus);
	virtual ~ToolScriptAction();

public:
	virtual qs::QSTATUS invoke(const qs::ActionEvent& event);
	virtual qs::QSTATUS isEnabled(const qs::ActionEvent& event, bool* pbEnabled);

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
	ToolSubAccountAction(Document* pDocument,
		FolderModel* pFolderModel, qs::QSTATUS* pstatus);
	virtual ~ToolSubAccountAction();

public:
	virtual qs::QSTATUS invoke(const qs::ActionEvent& event);
	virtual qs::QSTATUS isEnabled(const qs::ActionEvent& event, bool* pbEnabled);
	virtual qs::QSTATUS isChecked(const qs::ActionEvent& event, bool* pbChecked);

private:
	ToolSubAccountAction(const ToolSubAccountAction&);
	ToolSubAccountAction& operator=(const ToolSubAccountAction&);

private:
	Document* pDocument_;
	FolderModel* pFolderModel_;
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
	ToolSyncAction(SyncManager* pSyncManager, Document* pDocument,
		FolderModel* pFolderModel, SyncDialogManager* pSyncDialogManager,
		unsigned int nSync, HWND hwnd, qs::QSTATUS* pstatus);
	virtual ~ToolSyncAction();

public:
	virtual qs::QSTATUS invoke(const qs::ActionEvent& event);
	virtual qs::QSTATUS isEnabled(const qs::ActionEvent& event, bool* pbEnabled);

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
 * ViewEncodingAction
 *
 */

class ViewEncodingAction : public qs::AbstractAction
{
public:
	ViewEncodingAction(MessageWindow* pMessageWindow, qs::QSTATUS* pstatus);
	ViewEncodingAction(MessageWindow* pMessageWindow,
		EncodingMenu* pEncodingMenu, qs::QSTATUS* pstatus);
	virtual ~ViewEncodingAction();

public:
	virtual qs::QSTATUS invoke(const qs::ActionEvent& event);
	virtual qs::QSTATUS isChecked(const qs::ActionEvent& event, bool* pbChecked);

private:
	ViewEncodingAction(const ViewEncodingAction&);
	ViewEncodingAction& operator=(const ViewEncodingAction&);

private:
	MessageWindow* pMessageWindow_;
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
		FilterManager* pFilterManager, qs::QSTATUS* pstatus);
	virtual ~ViewFilterAction();

public:
	virtual qs::QSTATUS invoke(const qs::ActionEvent& event);
	virtual qs::QSTATUS isEnabled(const qs::ActionEvent& event, bool* pbEnabled);
	virtual qs::QSTATUS isChecked(const qs::ActionEvent& event, bool* pbChecked);

private:
	ViewFilterAction(const ViewFilterAction&);
	ViewFilterAction& operator=(const ViewFilterAction&);

private:
	ViewModelManager* pViewModelManager_;
	FilterManager* pFilterManager_;
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
		const FilterManager* pFilterManager, qs::QSTATUS* pstatus);
	virtual ~ViewFilterCustomAction();

public:
	virtual qs::QSTATUS invoke(const qs::ActionEvent& event);
	virtual qs::QSTATUS isEnabled(const qs::ActionEvent& event, bool* pbEnabled);
	virtual qs::QSTATUS isChecked(const qs::ActionEvent& event, bool* pbChecked);

private:
	ViewFilterCustomAction(const ViewFilterCustomAction&);
	ViewFilterCustomAction& operator=(const ViewFilterCustomAction&);

private:
	ViewModelManager* pViewModelManager_;
	const FilterManager* pFilterManager_;
};


/****************************************************************************
 *
 * ViewFilterNoneAction
 *
 */

class ViewFilterNoneAction : public qs::AbstractAction
{
public:
	ViewFilterNoneAction(ViewModelManager* pViewModelManager,
		qs::QSTATUS* pstatus);
	virtual ~ViewFilterNoneAction();

public:
	virtual qs::QSTATUS invoke(const qs::ActionEvent& event);
	virtual qs::QSTATUS isEnabled(const qs::ActionEvent& event, bool* pbEnabled);
	virtual qs::QSTATUS isChecked(const qs::ActionEvent& event, bool* pbChecked);

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
	ViewFocusAction(View* pViews[], size_t nViewCount,
		bool bNext, qs::QSTATUS* pstatus);
	virtual ~ViewFocusAction();

public:
	virtual qs::QSTATUS invoke(const qs::ActionEvent& event);

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
	ViewLockPreviewAction(MessageModel* pMessageModel, qs::QSTATUS* pstatus);
	virtual ~ViewLockPreviewAction();

public:
	virtual qs::QSTATUS invoke(const qs::ActionEvent& event);

private:
	ViewLockPreviewAction(const ViewLockPreviewAction&);
	ViewLockPreviewAction& operator=(const ViewLockPreviewAction&);

private:
	MessageModel* pMessageModel_;
};


/****************************************************************************
 *
 * ViewMessageModeAction
 *
 */

class ViewMessageModeAction : public qs::AbstractAction
{
public:
	typedef bool (MessageWindow::*PFN_IS)() const;
	typedef qs::QSTATUS (MessageWindow::*PFN_SET)(bool);

public:
	ViewMessageModeAction(MessageWindow* pMessageWindow, PFN_IS pfnIs,
		PFN_SET pfnSet, bool bEnabled, qs::QSTATUS* pstatus);
	virtual ~ViewMessageModeAction();

public:
	virtual qs::QSTATUS invoke(const qs::ActionEvent& event);
	virtual qs::QSTATUS isEnabled(const qs::ActionEvent& event, bool* pbEnabled);
	virtual qs::QSTATUS isChecked(const qs::ActionEvent& event, bool* pbChecked);

private:
	ViewMessageModeAction(const ViewMessageModeAction&);
	ViewMessageModeAction& operator=(const ViewMessageModeAction&);

private:
	MessageWindow* pMessageWindow_;
	PFN_IS pfnIs_;
	PFN_SET pfnSet_;
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
	ViewNavigateFolderAction(Document* pDocument,
		FolderModel* pFolderModel, Type type, qs::QSTATUS* pstatus);
	virtual ~ViewNavigateFolderAction();

public:
	virtual qs::QSTATUS invoke(const qs::ActionEvent& event);
	virtual qs::QSTATUS isEnabled(const qs::ActionEvent& event, bool* pbEnabled);

private:
	ViewNavigateFolderAction(const ViewNavigateFolderAction&);
	ViewNavigateFolderAction& operator=(const ViewNavigateFolderAction&);

private:
	Document* pDocument_;
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
		MessageWindow* pMessageWindow, Type type, qs::QSTATUS* pstatus);
	ViewNavigateMessageAction(MessageWindow* pMessageWindow,
		Type type, qs::QSTATUS* pstatus);
	virtual ~ViewNavigateMessageAction();

public:
	virtual qs::QSTATUS invoke(const qs::ActionEvent& event);
	virtual qs::QSTATUS isEnabled(const qs::ActionEvent& event, bool* pbEnabled);

private:
	ViewNavigateMessageAction(const ViewNavigateMessageAction&);
	ViewNavigateMessageAction& operator=(const ViewNavigateMessageAction&);

private:
	ViewModelManager* pViewModelManager_;
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
	ViewOpenLinkAction(MessageWindow* pMessageWindow, qs::QSTATUS* pstatus);
	virtual ~ViewOpenLinkAction();

public:
	virtual qs::QSTATUS invoke(const qs::ActionEvent& event);

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
	ViewRefreshAction(SyncManager* pSyncManager, Document* pDocument,
		FolderModel* pFolderModel, SyncDialogManager* pSyncDialogManager,
		HWND hwnd, qs::QSTATUS* pstatus);
	virtual ~ViewRefreshAction();

public:
	virtual qs::QSTATUS invoke(const qs::ActionEvent& event);
	virtual qs::QSTATUS isEnabled(const qs::ActionEvent& event, bool* pbEnabled);

private:
	ViewRefreshAction(const ViewRefreshAction&);
	ViewRefreshAction& operator=(const ViewRefreshAction&);

private:
	SyncManager* pSyncManager_;
	Document* pDocument_;
	FolderModel* pFolderModel_;
	SyncDialogManager* pSyncDialogManager_;
	HWND hwnd_;
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
	ViewScrollAction(HWND hwnd, Scroll scroll, qs::QSTATUS* pstatus);
	virtual ~ViewScrollAction();

public:
	virtual qs::QSTATUS invoke(const qs::ActionEvent& event);

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
 * ViewSelectModeAction
 *
 */

class ViewSelectModeAction : public qs::AbstractAction
{
public:
	ViewSelectModeAction(MessageWindow* pMessageWindow, qs::QSTATUS* pstatus);
	virtual ~ViewSelectModeAction();

public:
	virtual qs::QSTATUS invoke(const qs::ActionEvent& event);
	virtual qs::QSTATUS isChecked(const qs::ActionEvent& event, bool* pbChecked);

private:
	ViewSelectModeAction(const ViewSelectModeAction&);
	ViewSelectModeAction& operator=(const ViewSelectModeAction&);

private:
	MessageWindow* pMessageWindow_;
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
	typedef qs::QSTATUS (WindowX::*PFN_SET)(bool);
	typedef bool (WindowX::*PFN_IS)() const;

public:
	ViewShowControlAction(WindowX* pWindow, PFN_SET pfnSet, PFN_IS pfnIs,
		UINT nShowId, UINT nHideId, qs::QSTATUS* pstatus);
	virtual ~ViewShowControlAction();

public:
	virtual qs::QSTATUS invoke(const qs::ActionEvent& event);
	virtual qs::QSTATUS getText(const qs::ActionEvent& event, qs::WSTRING* pwstrText);

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
	ViewShowFolderAction(MainWindow* pMainWindow, qs::QSTATUS* pstatus);
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
	ViewShowHeaderAction(MessageWindow* pMessageWindow, qs::QSTATUS* pstatus);
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
	ViewShowHeaderColumnAction(ListWindow* pListWindow, qs::QSTATUS* pstatus);
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
	ViewShowPreviewAction(MainWindow* pMainWindow, qs::QSTATUS* pstatus);
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
	ViewShowStatusBarAction(WindowX* pWindow, qs::QSTATUS* pstatus);
	virtual ~ViewShowStatusBarAction();

private:
	ViewShowStatusBarAction(const ViewShowStatusBarAction&);
	ViewShowStatusBarAction& operator=(const ViewShowStatusBarAction&);
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
	ViewShowToolbarAction(WindowX* pWindow, qs::QSTATUS* pstatus);
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
	ViewSortAction(ViewModelManager* pViewModelManager, qs::QSTATUS* pstatus);
	virtual ~ViewSortAction();

public:
	virtual qs::QSTATUS invoke(const qs::ActionEvent& event);
	virtual qs::QSTATUS isEnabled(const qs::ActionEvent& event, bool* pbEnabled);
	virtual qs::QSTATUS isChecked(const qs::ActionEvent& event, bool* pbChecked);

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
		bool bAscending, qs::QSTATUS* pstatus);
	virtual ~ViewSortDirectionAction();

public:
	virtual qs::QSTATUS invoke(const qs::ActionEvent& event);
	virtual qs::QSTATUS isEnabled(const qs::ActionEvent& event, bool* pbEnabled);
	virtual qs::QSTATUS isChecked(const qs::ActionEvent& event, bool* pbChecked);

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
	ViewSortThreadAction(ViewModelManager* pViewModelManager, qs::QSTATUS* pstatus);
	virtual ~ViewSortThreadAction();

public:
	virtual qs::QSTATUS invoke(const qs::ActionEvent& event);
	virtual qs::QSTATUS isEnabled(const qs::ActionEvent& event, bool* pbEnabled);
	virtual qs::QSTATUS isChecked(const qs::ActionEvent& event, bool* pbChecked);

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
	ViewTemplateAction(MessageWindow* pMessageWindow, qs::QSTATUS* pstatus);
	ViewTemplateAction(MessageWindow* pMessageWindow,
		TemplateMenu* pTemplateMenu, qs::QSTATUS* pstatus);
	virtual ~ViewTemplateAction();

public:
	virtual qs::QSTATUS invoke(const qs::ActionEvent& event);
	virtual qs::QSTATUS isChecked(const qs::ActionEvent& event, bool* pbChecked);

private:
	ViewTemplateAction(const ViewTemplateAction&);
	ViewTemplateAction& operator=(const ViewTemplateAction&);

private:
	MessageWindow* pMessageWindow_;
	TemplateMenu* pTemplateMenu_;
};

}

#include "action.inl"

#endif // __ACTION_H__
