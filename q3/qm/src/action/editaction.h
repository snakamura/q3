/*
 * $Id$
 *
 * Copyright(C) 1998-2003 Satoshi Nakamura
 * All rights reserved.
 *
 */

#ifndef __EDITACTION_H__
#define __EDITACTION_H__

#include <qsaction.h>
#include <qsprofile.h>
#include <qstextwindow.h>

#include "../ui/editwindow.h"
#include "../ui/messagecomposer.h"


namespace qm {

class EditAttachmentEditAddAction;
class EditAttachmentEditDeleteAction;
class EditEditCommandAction;
class EditEditFindAction;
class EditEditMoveCaretAction;
class EditEditPasteWithQuoteAction;
class EditEditReplaceAction;
class EditFileInsertAction;
class EditFileOpenAction;
class EditFileSaveAction;
class EditFileSendAction;
class EditFocusItemAction;
class EditToolAddressBookAction;
class EditToolAttachmentAction;
class EditToolFlagAction;
class EditToolInsertSignatureAction;
class EditToolInsertTextAction;
class EditToolHeaderEditAction;
class EditToolReformAction;
class EditToolReformAllAction;

class AddressBook;
class AttachmentSelectionModel;
class Document;
class EditFrameWindow;
class EditMessageHolder;
class EditWindow;
class FindReplaceManager;


/****************************************************************************
 *
 * EditAttachmentEditAddAction
 *
 */

class EditAttachmentEditAddAction : public qs::AbstractAction
{
public:
	EditAttachmentEditAddAction(EditMessageHolder* pEditMessageHolder,
		HWND hwndFrame, qs::QSTATUS* pstatus);
	virtual ~EditAttachmentEditAddAction();

public:
	virtual qs::QSTATUS invoke(const qs::ActionEvent& event);

private:
	EditAttachmentEditAddAction(const EditAttachmentEditAddAction&);
	EditAttachmentEditAddAction& operator=(const EditAttachmentEditAddAction&);

private:
	EditMessageHolder* pEditMessageHolder_;
	HWND hwndFrame_;
};


/****************************************************************************
 *
 * EditAttachmentEditDeleteAction
 *
 */

class EditAttachmentEditDeleteAction : public qs::AbstractAction
{
public:
	EditAttachmentEditDeleteAction(EditMessageHolder* pEditMessageHolder,
		AttachmentSelectionModel* pAttachmentSelectionModel, qs::QSTATUS* pstatus);
	virtual ~EditAttachmentEditDeleteAction();

public:
	virtual qs::QSTATUS invoke(const qs::ActionEvent& event);
	virtual qs::QSTATUS isEnabled(const qs::ActionEvent& event, bool* pbEnabled);

private:
	EditAttachmentEditDeleteAction(const EditAttachmentEditDeleteAction&);
	EditAttachmentEditDeleteAction& operator=(const EditAttachmentEditDeleteAction&);

private:
	EditMessageHolder* pEditMessageHolder_;
	AttachmentSelectionModel* pAttachmentSelectionModel_;
};


/****************************************************************************
 *
 * EditEditCommandAction
 *
 */

class EditEditCommandAction : public qs::AbstractAction
{
public:
	typedef qs::QSTATUS (EditWindowItem::*PFN_DO)();
	typedef qs::QSTATUS (EditWindowItem::*PFN_CANDO)(bool*);

public:
	EditEditCommandAction(EditWindow* pEditWindow,
		PFN_DO pfnDo, PFN_CANDO pfnCanDo, qs::QSTATUS* pstatus);
	virtual ~EditEditCommandAction();

public:
	virtual qs::QSTATUS invoke(const qs::ActionEvent& event);
	virtual qs::QSTATUS isEnabled(const qs::ActionEvent& event, bool* pbEnabled);

private:
	EditEditCommandAction(const EditEditCommandAction&);
	EditEditCommandAction& operator=(const EditEditCommandAction&);

private:
	EditWindow* pEditWindow_;
	PFN_DO pfnDo_;
	PFN_CANDO pfnCanDo_;
};


/****************************************************************************
 *
 * EditEditFindAction
 *
 */

class EditEditFindAction : public qs::AbstractAction
{
public:
	enum Type {
		TYPE_NORMAL,
		TYPE_PREV,
		TYPE_NEXT
	};

public:
	EditEditFindAction(qs::TextWindow* pTextWindow, qs::Profile* pProfile,
		FindReplaceManager* pFindReplaceManager, qs::QSTATUS* pstatus);
	EditEditFindAction(qs::TextWindow* pTextWindow, bool bNext,
		FindReplaceManager* pFindReplaceManager, qs::QSTATUS* pstatus);
	virtual ~EditEditFindAction();

public:
	virtual qs::QSTATUS invoke(const qs::ActionEvent& event);
	virtual qs::QSTATUS isEnabled(const qs::ActionEvent& event, bool* pbEnabled);

private:
	EditEditFindAction(const EditEditFindAction&);
	EditEditFindAction& operator=(const EditEditFindAction&);

private:
	qs::TextWindow* pTextWindow_;
	qs::Profile* pProfile_;
	FindReplaceManager* pFindReplaceManager_;
	Type type_;
};


/****************************************************************************
 *
 * EditEditMoveCaretAction
 *
 */

class EditEditMoveCaretAction : public qs::AbstractAction
{
public:
	EditEditMoveCaretAction(qs::TextWindow* pTextWindow,
		qs::TextWindow::MoveCaret moveCaret, qs::QSTATUS* pstatus);
	virtual ~EditEditMoveCaretAction();

public:
	virtual qs::QSTATUS invoke(const qs::ActionEvent& event);
	virtual qs::QSTATUS isEnabled(const qs::ActionEvent& event, bool* pbEnabled);

private:
	EditEditMoveCaretAction(const EditEditMoveCaretAction&);
	EditEditMoveCaretAction& operator=(const EditEditMoveCaretAction&);

private:
	qs::TextWindow* pTextWindow_;
	qs::TextWindow::MoveCaret moveCaret_;
};


/****************************************************************************
 *
 * EditEditPasteWithQuoteAction
 *
 */

class EditEditPasteWithQuoteAction : public qs::AbstractAction
{
public:
	EditEditPasteWithQuoteAction(qs::TextWindow* pTextWindow,
		qs::Profile* pProfile, qs::QSTATUS* pstatus);
	virtual ~EditEditPasteWithQuoteAction();

public:
	virtual qs::QSTATUS invoke(const qs::ActionEvent& event);
	virtual qs::QSTATUS isEnabled(const qs::ActionEvent& event, bool* pbEnabled);

private:
	EditEditPasteWithQuoteAction(const EditEditPasteWithQuoteAction&);
	EditEditPasteWithQuoteAction& operator=(const EditEditPasteWithQuoteAction&);

private:
	qs::TextWindow* pTextWindow_;
	qs::Profile* pProfile_;
};


/****************************************************************************
 *
 * EditEditReplaceAction
 *
 */

class EditEditReplaceAction : public qs::AbstractAction
{
public:
	EditEditReplaceAction(qs::TextWindow* pTextWindow, qs::Profile* pProfile,
		FindReplaceManager* pFindReplaceManager, qs::QSTATUS* pstatus);
	virtual ~EditEditReplaceAction();

public:
	virtual qs::QSTATUS invoke(const qs::ActionEvent& event);
	virtual qs::QSTATUS isEnabled(const qs::ActionEvent& event, bool* pbEnabled);

private:
	EditEditReplaceAction(const EditEditReplaceAction&);
	EditEditReplaceAction& operator=(const EditEditReplaceAction&);

private:
	qs::TextWindow* pTextWindow_;
	qs::Profile* pProfile_;
	FindReplaceManager* pFindReplaceManager_;
};


/****************************************************************************
 *
 * EditFileInsertAction
 *
 */

class EditFileInsertAction : public qs::AbstractAction
{
public:
	EditFileInsertAction(qs::TextWindow* pTextWindow, qs::QSTATUS* pstatus);
	virtual ~EditFileInsertAction();

public:
	virtual qs::QSTATUS invoke(const qs::ActionEvent& event);
	virtual qs::QSTATUS isEnabled(const qs::ActionEvent& event, bool* pbEnabled);

private:
	EditFileInsertAction(const EditFileInsertAction&);
	EditFileInsertAction& operator=(const EditFileInsertAction&);

private:
	qs::TextWindow* pTextWindow_;
};


/****************************************************************************
 *
 * EditFileOpenAction
 *
 */

class EditFileOpenAction : public qs::AbstractAction
{
public:
	EditFileOpenAction(EditMessageHolder* pEditMessageHolder,
		HWND hwndFrame, qs::QSTATUS* pstatus);
	virtual ~EditFileOpenAction();

public:
	virtual qs::QSTATUS invoke(const qs::ActionEvent& event);

private:
	EditFileOpenAction(const EditFileOpenAction&);
	EditFileOpenAction& operator=(const EditFileOpenAction&);

private:
	EditMessageHolder* pEditMessageHolder_;
	HWND hwndFrame_;
};


/****************************************************************************
 *
 * EditFileSaveAction
 *
 */

class EditFileSaveAction : public qs::AbstractAction
{
public:
	EditFileSaveAction(EditMessageHolder* pEditMessageHolder,
		HWND hwndFrame, qs::QSTATUS* pstatus);
	virtual ~EditFileSaveAction();

public:
	virtual qs::QSTATUS invoke(const qs::ActionEvent& event);

private:
	EditFileSaveAction(const EditFileSaveAction&);
	EditFileSaveAction& operator=(const EditFileSaveAction&);

private:
	EditMessageHolder* pEditMessageHolder_;
	HWND hwndFrame_;
};


/****************************************************************************
 *
 * EditFileSendAction
 *
 */

class EditFileSendAction : public qs::AbstractAction
{
public:
	EditFileSendAction(bool bDraft, Document* pDocument,
		EditMessageHolder* pEditMessageHolder, EditFrameWindow* pEditFrameWindow,
		qs::Profile* pProfile, qs::QSTATUS* pstatus);
	EditFileSendAction(Document* pDocument,
		EditMessageHolder* pEditMessageHolder, EditFrameWindow* pEditFrameWindow,
		qs::Profile* pProfile, SyncManager* pSyncManager,
		SyncDialogManager* pSyncDialogManager, qs::QSTATUS* pstatus);
	virtual ~EditFileSendAction();

public:
	virtual qs::QSTATUS invoke(const qs::ActionEvent& event);

private:
	EditFileSendAction(const EditFileSendAction&);
	EditFileSendAction& operator=(const EditFileSendAction&);

private:
	MessageComposer composer_;
	EditMessageHolder* pEditMessageHolder_;
	EditFrameWindow* pEditFrameWindow_;
	Document* pDocument_;
	SyncManager* pSyncManager_;
	SyncDialogManager* pSyncDialogManager_;
};


/****************************************************************************
 *
 * EditFocusItemAction
 *
 */

class EditFocusItemAction : public qs::AbstractAction
{
public:
	EditFocusItemAction(EditWindow* pEditWindow, qs::QSTATUS* pstatus);
	virtual ~EditFocusItemAction();

public:
	virtual qs::QSTATUS invoke(const qs::ActionEvent& event);

private:
	EditFocusItemAction(const EditFocusItemAction&);
	EditFocusItemAction& operator=(const EditFocusItemAction&);

private:
	EditWindow* pEditWindow_;
};


/****************************************************************************
 *
 * EditToolAddressBookAction
 *
 */

class EditToolAddressBookAction : public qs::AbstractAction
{
public:
	EditToolAddressBookAction(EditMessageHolder* pEditMessageHolder,
		EditWindow* pEditWindow, AddressBook* pAddressBook,
		qs::Profile* pProfile, qs::QSTATUS* pstatus);
	virtual ~EditToolAddressBookAction();

public:
	virtual qs::QSTATUS invoke(const qs::ActionEvent& event);
	virtual qs::QSTATUS isEnabled(const qs::ActionEvent& event, bool* pbEnabled);

private:
	EditToolAddressBookAction(const EditToolAddressBookAction&);
	EditToolAddressBookAction& operator=(const EditToolAddressBookAction&);

private:
	EditMessageHolder* pEditMessageHolder_;
	EditWindow* pEditWindow_;
	AddressBook* pAddressBook_;
	qs::Profile* pProfile_;
};


/****************************************************************************
 *
 * EditToolAttachmentAction
 *
 */

class EditToolAttachmentAction : public qs::AbstractAction
{
public:
	EditToolAttachmentAction(EditMessageHolder* pEditMessageHolder,
		HWND hwndFrame, qs::QSTATUS* pstatus);
	virtual ~EditToolAttachmentAction();

public:
	virtual qs::QSTATUS invoke(const qs::ActionEvent& event);

private:
	EditToolAttachmentAction(const EditToolAttachmentAction&);
	EditToolAttachmentAction& operator=(const EditToolAttachmentAction&);

private:
	EditMessageHolder* pEditMessageHolder_;
	HWND hwndFrame_;
};


/****************************************************************************
 *
 * EditToolFlagAction
 *
 */

class EditToolFlagAction : public qs::AbstractAction
{
public:
	typedef bool (EditMessage::*PFN_IS)() const;
	typedef void (EditMessage::*PFN_SET)(bool);

public:
	EditToolFlagAction(EditMessageHolder* pEditMessageHolder,
		PFN_IS pfnIs, PFN_SET pfnSet, bool bEnabled, qs::QSTATUS* pstatus);
	virtual ~EditToolFlagAction();

public:
	virtual qs::QSTATUS invoke(const qs::ActionEvent& event);
	virtual qs::QSTATUS isEnabled(const qs::ActionEvent& event, bool* pbEnabled);
	virtual qs::QSTATUS isChecked(const qs::ActionEvent& event, bool* pbChecked);

private:
	EditToolFlagAction(const EditToolFlagAction&);
	EditToolFlagAction& operator=(const EditToolFlagAction&);

private:
	EditMessageHolder* pEditMessageHolder_;
	PFN_IS pfnIs_;
	PFN_SET pfnSet_;
	bool bEnabled_;
};


/****************************************************************************
 *
 * EditToolInsertSignatureAction
 *
 */

class EditToolInsertSignatureAction : public qs::AbstractAction
{
public:
	EditToolInsertSignatureAction(EditMessageHolder* pEditMessageHolder,
		qs::TextWindow* pTextWindow, qs::QSTATUS* pstatus);
	virtual ~EditToolInsertSignatureAction();

public:
	virtual qs::QSTATUS invoke(const qs::ActionEvent& event);
	virtual qs::QSTATUS isEnabled(const qs::ActionEvent& event, bool* pbEnabled);

private:
	EditToolInsertSignatureAction(const EditToolInsertSignatureAction&);
	EditToolInsertSignatureAction& operator=(const EditToolInsertSignatureAction&);

private:
	EditMessageHolder* pEditMessageHolder_;
	qs::TextWindow* pTextWindow_;
};


/****************************************************************************
 *
 * EditToolInsertTextAction
 *
 */

class EditToolInsertTextAction : public qs::AbstractAction
{
public:
	EditToolInsertTextAction(qs::TextWindow* pTextWindow, qs::QSTATUS* pstatus);
	virtual ~EditToolInsertTextAction();

public:
	virtual qs::QSTATUS invoke(const qs::ActionEvent& event);
	virtual qs::QSTATUS isEnabled(const qs::ActionEvent& event, bool* pbEnabled);

private:
	EditToolInsertTextAction(const EditToolInsertTextAction&);
	EditToolInsertTextAction& operator=(const EditToolInsertTextAction&);

private:
	qs::TextWindow* pTextWindow_;
};


/****************************************************************************
 *
 * EditToolHeaderEditAction
 *
 */

class EditToolHeaderEditAction : public qs::AbstractAction
{
public:
	EditToolHeaderEditAction(EditWindow* pEditWindow, qs::QSTATUS* pstatus);
	virtual ~EditToolHeaderEditAction();

public:
	virtual qs::QSTATUS invoke(const qs::ActionEvent& event);
	virtual qs::QSTATUS isChecked(const qs::ActionEvent& event, bool* pbChecked);

private:
	EditToolHeaderEditAction(const EditToolHeaderEditAction&);
	EditToolHeaderEditAction& operator=(const EditToolHeaderEditAction&);

private:
	EditWindow* pEditWindow_;
};


/****************************************************************************
 *
 * EditToolReformAction
 *
 */

class EditToolReformAction : public qs::AbstractAction
{
public:
	EditToolReformAction(qs::TextWindow* pTextWindow, qs::QSTATUS* pstatus);
	virtual ~EditToolReformAction();

public:
	virtual qs::QSTATUS invoke(const qs::ActionEvent& event);
	virtual qs::QSTATUS isEnabled(const qs::ActionEvent& event, bool* pbEnabled);

private:
	EditToolReformAction(const EditToolReformAction&);
	EditToolReformAction& operator=(const EditToolReformAction&);

private:
	qs::TextWindow* pTextWindow_;
};


/****************************************************************************
 *
 * EditToolReformAllAction
 *
 */

class EditToolReformAllAction : public qs::AbstractAction
{
public:
	EditToolReformAllAction(qs::TextWindow* pTextWindow,
		qs::Profile* pProfile, qs::QSTATUS* pstatus);
	virtual ~EditToolReformAllAction();

public:
	virtual qs::QSTATUS invoke(const qs::ActionEvent& event);
	virtual qs::QSTATUS isEnabled(const qs::ActionEvent& event, bool* pbEnabled);

private:
	EditToolReformAllAction(const EditToolReformAllAction&);
	EditToolReformAllAction& operator=(const EditToolReformAllAction&);

private:
	qs::TextWindow* pTextWindow_;
	qs::Profile* pProfile_;
};

}

#endif // __EDITACTION_H__
