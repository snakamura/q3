/*
 * $Id$
 *
 * Copyright(C) 1998-2006 Satoshi Nakamura
 * All rights reserved.
 *
 */

#ifndef __EDITACTION_H__
#define __EDITACTION_H__

#include <qsaction.h>
#include <qsprofile.h>
#include <qstextwindow.h>

#include "messagecomposer.h"
#include "../model/editmessage.h"
#include "../ui/editwindow.h"


namespace qm {

class EditAttachmentEditAddAction;
class EditAttachmentEditDeleteAction;
class EditEditCommandAction;
class EditEditDeleteAction;
class EditEditFindAction;
class EditEditMoveCaretAction;
class EditEditPasteWithQuoteAction;
class EditEditReplaceAction;
class EditFileInsertAction;
class EditFileOpenAction;
class EditFileSaveAction;
class EditFileSendAction;
class EditFocusItemAction;
#ifdef QMZIP
class EditToolArchiveAttachmentAction;
#endif
class EditToolAttachmentAction;
class EditToolEncodingAction;
class EditToolFlagAction;
class EditToolInsertSignatureAction;
class EditToolInsertTextAction;
class EditToolHeaderEditAction;
class EditToolMessageSecurityAction;
class EditToolReformAction;
class EditToolReformAllAction;
class EditToolSelectAddressAction;

class AddressBook;
class AttachmentSelectionModel;
class Document;
class EditFrameWindow;
class EditMessageHolder;
class EditWindow;
class FindReplaceManager;
class SecurityModel;


/****************************************************************************
 *
 * EditAttachmentEditAddAction
 *
 */

class EditAttachmentEditAddAction : public qs::AbstractAction
{
public:
	EditAttachmentEditAddAction(EditMessageHolder* pEditMessageHolder,
								HWND hwnd);
	virtual ~EditAttachmentEditAddAction();

public:
	virtual void invoke(const qs::ActionEvent& event);

private:
	EditAttachmentEditAddAction(const EditAttachmentEditAddAction&);
	EditAttachmentEditAddAction& operator=(const EditAttachmentEditAddAction&);

private:
	EditMessageHolder* pEditMessageHolder_;
	HWND hwnd_;
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
								   AttachmentSelectionModel* pAttachmentSelectionModel);
	virtual ~EditAttachmentEditDeleteAction();

public:
	virtual void invoke(const qs::ActionEvent& event);
	virtual bool isEnabled(const qs::ActionEvent& event);

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
	typedef void (EditWindowItem::*PFN_DO)();
	typedef bool (EditWindowItem::*PFN_CANDO)();

public:
	EditEditCommandAction(EditWindow* pEditWindow,
						  PFN_DO pfnDo,
						  PFN_CANDO pfnCanDo);
	virtual ~EditEditCommandAction();

public:
	virtual void invoke(const qs::ActionEvent& event);
	virtual bool isEnabled(const qs::ActionEvent& event);

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
 * EditEditDeleteAction
 *
 */

class EditEditDeleteAction : public qs::AbstractAction
{
public:
	EditEditDeleteAction(qs::TextWindow* pTextWindow,
						 qs::TextWindow::DeleteTextFlag flag);
	virtual ~EditEditDeleteAction();

public:
	virtual void invoke(const qs::ActionEvent& event);
	virtual bool isEnabled(const qs::ActionEvent& event);

private:
	EditEditDeleteAction(const EditEditDeleteAction&);
	EditEditDeleteAction& operator=(const EditEditDeleteAction&);

private:
	qs::TextWindow* pTextWindow_;
	qs::TextWindow::DeleteTextFlag flag_;
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
	EditEditFindAction(qs::TextWindow* pTextWindow,
					   qs::Profile* pProfile,
					   FindReplaceManager* pFindReplaceManager);
	EditEditFindAction(qs::TextWindow* pTextWindow,
					   bool bNext,
					   FindReplaceManager* pFindReplaceManager);
	virtual ~EditEditFindAction();

public:
	virtual void invoke(const qs::ActionEvent& event);
	virtual bool isEnabled(const qs::ActionEvent& event);

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
							qs::TextWindow::MoveCaret moveCaret);
	virtual ~EditEditMoveCaretAction();

public:
	virtual void invoke(const qs::ActionEvent& event);
	virtual bool isEnabled(const qs::ActionEvent& event);

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
	EditEditPasteWithQuoteAction(Document* pDocument,
								 EditMessageHolder* pEditMessageHolder,
								 qs::TextWindow* pTextWindow,
								 SecurityModel* pSecurityModel,
								 qs::Profile* pProfile,
								 HWND hwnd);
	virtual ~EditEditPasteWithQuoteAction();

public:
	virtual void invoke(const qs::ActionEvent& event);
	virtual bool isEnabled(const qs::ActionEvent& event);

private:
	EditEditPasteWithQuoteAction(const EditEditPasteWithQuoteAction&);
	EditEditPasteWithQuoteAction& operator=(const EditEditPasteWithQuoteAction&);

private:
	Document* pDocument_;
	EditMessageHolder* pEditMessageHolder_;
	qs::TextWindow* pTextWindow_;
	SecurityModel* pSecurityModel_;
	qs::Profile* pProfile_;
	HWND hwnd_;
};


/****************************************************************************
 *
 * EditEditReplaceAction
 *
 */

class EditEditReplaceAction : public qs::AbstractAction
{
public:
	EditEditReplaceAction(qs::TextWindow* pTextWindow,
						  qs::Profile* pProfile,
						  FindReplaceManager* pFindReplaceManager);
	virtual ~EditEditReplaceAction();

public:
	virtual void invoke(const qs::ActionEvent& event);
	virtual bool isEnabled(const qs::ActionEvent& event);

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
	explicit EditFileInsertAction(qs::TextWindow* pTextWindow);
	virtual ~EditFileInsertAction();

public:
	virtual void invoke(const qs::ActionEvent& event);
	virtual bool isEnabled(const qs::ActionEvent& event);

private:
	bool insertText(const WCHAR* pwszPath);

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
					   HWND hwnd);
	virtual ~EditFileOpenAction();

public:
	virtual void invoke(const qs::ActionEvent& event);

private:
	bool open(const WCHAR* pwszPath);

private:
	EditFileOpenAction(const EditFileOpenAction&);
	EditFileOpenAction& operator=(const EditFileOpenAction&);

private:
	EditMessageHolder* pEditMessageHolder_;
	HWND hwnd_;
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
					   HWND hwnd);
	virtual ~EditFileSaveAction();

public:
	virtual void invoke(const qs::ActionEvent& event);

private:
	bool save(const WCHAR* pwszPath);

private:
	EditFileSaveAction(const EditFileSaveAction&);
	EditFileSaveAction& operator=(const EditFileSaveAction&);

private:
	EditMessageHolder* pEditMessageHolder_;
	HWND hwnd_;
};


/****************************************************************************
 *
 * EditFileSendAction
 *
 */

class EditFileSendAction : public qs::AbstractAction
{
public:
	enum Type {
		TYPE_SEND,
		TYPE_DRAFT,
		TYPE_DRAFTCLOSE
	};

public:
	EditFileSendAction(Type type,
					   Document* pDocument,
					   PasswordManager* pPasswordManager,
					   EditMessageHolder* pEditMessageHolder,
					   EditFrameWindow* pEditFrameWindow,
					   qs::Profile* pProfile,
					   SecurityModel* pSecurityModel);
	EditFileSendAction(Document* pDocument,
					   PasswordManager* pPasswordManager,
					   EditMessageHolder* pEditMessageHolder,
					   EditFrameWindow* pEditFrameWindow,
					   qs::Profile* pProfile,
					   SyncManager* pSyncManager,
					   SyncDialogManager* pSyncDialogManager,
					   SecurityModel* pSecurityModel);
	virtual ~EditFileSendAction();

public:
	virtual void invoke(const qs::ActionEvent& event);

private:
	EditFileSendAction(const EditFileSendAction&);
	EditFileSendAction& operator=(const EditFileSendAction&);

private:
	MessageComposer composer_;
	Type type_;
	EditMessageHolder* pEditMessageHolder_;
	EditFrameWindow* pEditFrameWindow_;
	Document* pDocument_;
	SyncManager* pSyncManager_;
	SyncDialogManager* pSyncDialogManager_;
	SecurityModel* pSecurityModel_;
};


/****************************************************************************
 *
 * EditFocusItemAction
 *
 */

class EditFocusItemAction : public qs::AbstractAction
{
public:
	EditFocusItemAction(EditWindow* pEditWindow);
	virtual ~EditFocusItemAction();

public:
	virtual void invoke(const qs::ActionEvent& event);

private:
	EditFocusItemAction(const EditFocusItemAction&);
	EditFocusItemAction& operator=(const EditFocusItemAction&);

private:
	EditWindow* pEditWindow_;
};


#ifdef QMZIP
/****************************************************************************
 *
 * EditToolArchiveAttachmentAction
 *
 */

class EditToolArchiveAttachmentAction : public qs::AbstractAction
{
public:
	explicit EditToolArchiveAttachmentAction(EditMessageHolder* pEditMessageHolder);
	virtual ~EditToolArchiveAttachmentAction();

public:
	virtual void invoke(const qs::ActionEvent& event);
	virtual bool isChecked(const qs::ActionEvent& event);

private:
	EditToolArchiveAttachmentAction(const EditToolArchiveAttachmentAction&);
	EditToolArchiveAttachmentAction& operator=(const EditToolArchiveAttachmentAction&);

private:
	EditMessageHolder* pEditMessageHolder_;
};
#endif


/****************************************************************************
 *
 * EditToolAttachmentAction
 *
 */

class EditToolAttachmentAction : public qs::AbstractAction
{
public:
	EditToolAttachmentAction(EditMessageHolder* pEditMessageHolder,
							 HWND hwnd);
	virtual ~EditToolAttachmentAction();

public:
	virtual void invoke(const qs::ActionEvent& event);

private:
	EditToolAttachmentAction(const EditToolAttachmentAction&);
	EditToolAttachmentAction& operator=(const EditToolAttachmentAction&);

private:
	EditMessageHolder* pEditMessageHolder_;
	HWND hwnd_;
};


/****************************************************************************
 *
 * EditToolEncodingAction
 *
 */

class EditToolEncodingAction : public qs::AbstractAction
{
public:
	explicit EditToolEncodingAction(EditMessageHolder* pEditMessageHolder);
	virtual ~EditToolEncodingAction();

public:
	virtual void invoke(const qs::ActionEvent& event);
	virtual bool isEnabled(const qs::ActionEvent& event);
	virtual bool isChecked(const qs::ActionEvent& event);

private:
	EditToolEncodingAction(const EditToolEncodingAction&);
	EditToolEncodingAction& operator=(const EditToolEncodingAction&);

private:
	EditMessageHolder* pEditMessageHolder_;
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
					   PFN_IS pfnIs,
					   PFN_SET pfnSet,
					   bool bEnabled);
	virtual ~EditToolFlagAction();

public:
	virtual void invoke(const qs::ActionEvent& event);
	virtual bool isEnabled(const qs::ActionEvent& event);
	virtual bool isChecked(const qs::ActionEvent& event);

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
								  qs::TextWindow* pTextWindow);
	virtual ~EditToolInsertSignatureAction();

public:
	virtual void invoke(const qs::ActionEvent& event);
	virtual bool isEnabled(const qs::ActionEvent& event);

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
	EditToolInsertTextAction(FixedFormTextManager* pFixedFormTextManager,
							 qs::TextWindow* pTextWindow);
	virtual ~EditToolInsertTextAction();

public:
	virtual void invoke(const qs::ActionEvent& event);
	virtual bool isEnabled(const qs::ActionEvent& event);

private:
	EditToolInsertTextAction(const EditToolInsertTextAction&);
	EditToolInsertTextAction& operator=(const EditToolInsertTextAction&);

private:
	FixedFormTextManager* pFixedFormTextManager_;
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
	explicit EditToolHeaderEditAction(EditWindow* pEditWindow);
	virtual ~EditToolHeaderEditAction();

public:
	virtual void invoke(const qs::ActionEvent& event);
	virtual bool isChecked(const qs::ActionEvent& event);

private:
	EditToolHeaderEditAction(const EditToolHeaderEditAction&);
	EditToolHeaderEditAction& operator=(const EditToolHeaderEditAction&);

private:
	EditWindow* pEditWindow_;
};


/****************************************************************************
 *
 * EditToolMessageSecurityAction
 *
 */

class EditToolMessageSecurityAction : public qs::AbstractAction
{
public:
	EditToolMessageSecurityAction(EditMessageHolder* pEditMessageHolder,
								  MessageSecurity security,
								  bool bEnabled);
	virtual ~EditToolMessageSecurityAction();

public:
	virtual void invoke(const qs::ActionEvent& event);
	virtual bool isEnabled(const qs::ActionEvent& event);
	virtual bool isChecked(const qs::ActionEvent& event);

private:
	EditToolMessageSecurityAction(const EditToolFlagAction&);
	EditToolMessageSecurityAction& operator=(const EditToolFlagAction&);

private:
	EditMessageHolder* pEditMessageHolder_;
	MessageSecurity security_;
	bool bEnabled_;
};


/****************************************************************************
 *
 * EditToolReformAction
 *
 */

class EditToolReformAction : public qs::AbstractAction
{
public:
	explicit EditToolReformAction(qs::TextWindow* pTextWindow);
	virtual ~EditToolReformAction();

public:
	virtual void invoke(const qs::ActionEvent& event);
	virtual bool isEnabled(const qs::ActionEvent& event);

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
							qs::Profile* pProfile);
	virtual ~EditToolReformAllAction();

public:
	virtual void invoke(const qs::ActionEvent& event);
	virtual bool isEnabled(const qs::ActionEvent& event);

private:
	EditToolReformAllAction(const EditToolReformAllAction&);
	EditToolReformAllAction& operator=(const EditToolReformAllAction&);

private:
	qs::TextWindow* pTextWindow_;
	qs::Profile* pProfile_;
};


/****************************************************************************
 *
 * EditToolSelectAddressAction
 *
 */

class EditToolSelectAddressAction : public qs::AbstractAction
{
public:
	EditToolSelectAddressAction(EditMessageHolder* pEditMessageHolder,
								EditWindow* pEditWindow,
								AddressBook* pAddressBook,
								qs::Profile* pProfile);
	virtual ~EditToolSelectAddressAction();

public:
	virtual void invoke(const qs::ActionEvent& event);
	virtual bool isEnabled(const qs::ActionEvent& event);

private:
	EditToolSelectAddressAction(const EditToolSelectAddressAction&);
	EditToolSelectAddressAction& operator=(const EditToolSelectAddressAction&);

private:
	EditMessageHolder* pEditMessageHolder_;
	EditWindow* pEditWindow_;
	AddressBook* pAddressBook_;
	qs::Profile* pProfile_;
};

}

#endif // __EDITACTION_H__
