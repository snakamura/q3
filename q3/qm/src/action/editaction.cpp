/*
 * $Id$
 *
 * Copyright(C) 1998-2004 Satoshi Nakamura
 * All rights reserved.
 *
 */

#include <qmaccount.h>
#include <qmapplication.h>
#include <qmdocument.h>
#include <qmeditwindow.h>
#include <qmfolder.h>
#include <qmmacro.h>
#include <qmmessage.h>
#include <qmsecurity.h>

#include <qsstl.h>
#include <qsstream.h>
#include <qstextutil.h>

#include <algorithm>

#include <commdlg.h>

#include "action.h"
#include "editaction.h"
#include "findreplace.h"
#include "../model/dataobject.h"
#include "../model/editmessage.h"
#include "../model/fixedformtext.h"
#include "../model/templatemanager.h"
#include "../model/uri.h"
#include "../ui/actionid.h"
#include "../ui/dialogs.h"
#include "../ui/editwindow.h"
#include "../ui/menus.h"
#include "../ui/syncutil.h"
#include "../ui/uiutil.h"
#include "../uimodel/attachmentselectionmodel.h"
#include "../uimodel/securitymodel.h"

#pragma warning(disable:4786)

using namespace qm;
using namespace qs;


/****************************************************************************
 *
 * EditAttachmentEditAddAction
 *
 */

qm::EditAttachmentEditAddAction::EditAttachmentEditAddAction(EditMessageHolder* pEditMessageHolder,
															 HWND hwnd) :
	pEditMessageHolder_(pEditMessageHolder),
	hwnd_(hwnd)
{
}

qm::EditAttachmentEditAddAction::~EditAttachmentEditAddAction()
{
}

void qm::EditAttachmentEditAddAction::invoke(const ActionEvent& event)
{
	EditMessage* pEditMessage = pEditMessageHolder_->getEditMessage();
	
	wstring_ptr wstrFilter(loadString(
		Application::getApplication().getResourceHandle(), IDS_FILTER_ATTACHMENT));
	
	FileDialog dialog(true, wstrFilter.get(), 0, 0, 0,
		OFN_EXPLORER | OFN_HIDEREADONLY | OFN_LONGNAMES | OFN_ALLOWMULTISELECT);
	
	if (dialog.doModal(hwnd_) == IDOK) {
		const WCHAR* pwszPath = dialog.getPath();
		const WCHAR* p = pwszPath;
		while (*p) {
			pEditMessage->addAttachment(p);
			p += wcslen(p) + 1;
		}
	}
}


/****************************************************************************
 *
 * EditAttachmentEditDeleteAction
 *
 */

qm::EditAttachmentEditDeleteAction::EditAttachmentEditDeleteAction(EditMessageHolder* pEditMessageHolder,
																   AttachmentSelectionModel* pAttachmentSelectionModel) :
	pEditMessageHolder_(pEditMessageHolder),
	pAttachmentSelectionModel_(pAttachmentSelectionModel)
{
}

qm::EditAttachmentEditDeleteAction::~EditAttachmentEditDeleteAction()
{
}

void qm::EditAttachmentEditDeleteAction::invoke(const ActionEvent& event)
{
	EditMessage* pEditMessage = pEditMessageHolder_->getEditMessage();
	
	AttachmentSelectionModel::NameList l;
	StringListFree<AttachmentSelectionModel::NameList> free(l);
	pAttachmentSelectionModel_->getSelectedAttachment(&l);
	
	for (AttachmentSelectionModel::NameList::const_iterator it = l.begin(); it != l.end(); ++it)
		pEditMessage->removeAttachment(*it);
}

bool qm::EditAttachmentEditDeleteAction::isEnabled(const ActionEvent& event)
{
	return pAttachmentSelectionModel_->hasSelectedAttachment();
}


/****************************************************************************
 *
 * EditEditCommandAction
 *
 */

qm::EditEditCommandAction::EditEditCommandAction(EditWindow* pEditWindow,
												 PFN_DO pfnDo,
												 PFN_CANDO pfnCanDo) :
	pEditWindow_(pEditWindow),
	pfnDo_(pfnDo),
	pfnCanDo_(pfnCanDo)
{
}

qm::EditEditCommandAction::~EditEditCommandAction()
{
}

void qm::EditEditCommandAction::invoke(const ActionEvent& event)
{
	EditWindowItem* pItem = pEditWindow_->getFocusedItem();
	if (pItem)
		(pItem->*pfnDo_)();
}

bool qm::EditEditCommandAction::isEnabled(const ActionEvent& event)
{
	EditWindowItem* pItem = pEditWindow_->getFocusedItem();
	if (pItem)
		return (pItem->*pfnCanDo_)();
	else
		return false;
}


/****************************************************************************
 *
 * EditEditDeleteAction
 *
 */

qm::EditEditDeleteAction::EditEditDeleteAction(TextWindow* pTextWindow,
											   TextWindow::DeleteTextFlag flag) :
	pTextWindow_(pTextWindow),
	flag_(flag)
{
}

qm::EditEditDeleteAction::~EditEditDeleteAction()
{
}

void qm::EditEditDeleteAction::invoke(const qs::ActionEvent& event)
{
	pTextWindow_->deleteText(flag_);
}


/****************************************************************************
 *
 * EditEditFindAction
 *
 */

qm::EditEditFindAction::EditEditFindAction(TextWindow* pTextWindow,
										   Profile* pProfile,
										   FindReplaceManager* pFindReplaceManager) :
	pTextWindow_(pTextWindow),
	pProfile_(pProfile),
	pFindReplaceManager_(pFindReplaceManager),
	type_(TYPE_NORMAL)
{
}

qm::EditEditFindAction::EditEditFindAction(TextWindow* pTextWindow,
										   bool bNext,
										   FindReplaceManager* pFindReplaceManager) :
	pTextWindow_(pTextWindow),
	pProfile_(0),
	pFindReplaceManager_(pFindReplaceManager),
	type_(bNext ? TYPE_NEXT : TYPE_PREV)
{
}

qm::EditEditFindAction::~EditEditFindAction()
{
}

void qm::EditEditFindAction::invoke(const ActionEvent& event)
{
	HWND hwnd = pTextWindow_->getParentFrame();
	
	bool bFound = false;
	if (type_ == TYPE_NORMAL) {
		FindDialog dialog(pProfile_, true);
		if (dialog.doModal(hwnd) != IDOK)
			return;
		
		pFindReplaceManager_->setData(dialog.getFind(),
			(dialog.isMatchCase() ? FindReplaceData::FLAG_MATCHCASE : 0) |
			(dialog.isRegex() ? FindReplaceData::FLAG_REGEX : 0));
		
		unsigned int nFlags = 0;
		if (dialog.isMatchCase())
			nFlags |= TextWindow::FIND_MATCHCASE;
		if (dialog.isRegex())
			nFlags |= TextWindow::FIND_REGEX;
		if (dialog.isPrev())
			nFlags |= TextWindow::FIND_PREVIOUS;
		
		bFound = pTextWindow_->find(dialog.getFind(), nFlags);
	}
	else {
		const FindReplaceData* pData = pFindReplaceManager_->getData();
		if (!pData)
			return;
		
		unsigned int nFlags = 0;
		if (pData->getFlags() & FindReplaceData::FLAG_MATCHCASE)
			nFlags |= TextWindow::FIND_MATCHCASE;
		if (pData->getFlags() & FindReplaceData::FLAG_REGEX)
			nFlags |= TextWindow::FIND_REGEX;
		if (type_ == TYPE_PREV)
			nFlags |= TextWindow::FIND_PREVIOUS;
		
		const WCHAR* pwszReplace = pData->getReplace();
		if (pwszReplace)
			bFound = pTextWindow_->replace(pData->getFind(), pwszReplace, nFlags);
		else
			bFound = pTextWindow_->find(pData->getFind(), nFlags);
	}
	
	if (!bFound)
		ActionUtil::info(hwnd, IDS_FINDNOTFOUND);
}

bool qm::EditEditFindAction::isEnabled(const ActionEvent& event)
{
	if (type_ == TYPE_NORMAL)
		return pTextWindow_->hasFocus();
	else
		return pTextWindow_->hasFocus() && pFindReplaceManager_->getData();
}


/****************************************************************************
 *
 * EditEditMoveCaretAction
 *
 */

qm::EditEditMoveCaretAction::EditEditMoveCaretAction(TextWindow* pTextWindow,
													 TextWindow::MoveCaret moveCaret) :
	pTextWindow_(pTextWindow),
	moveCaret_(moveCaret)
{
}

qm::EditEditMoveCaretAction::~EditEditMoveCaretAction()
{
}

void qm::EditEditMoveCaretAction::invoke(const ActionEvent& event)
{
	TextWindow::Select select = TextWindow::SELECT_CLEAR;
	if (event.getParam()) {
		ActionParam* pParam = static_cast<ActionParam*>(event.getParam());
		if (pParam->nArgs_ > 0) {
			Variant v;
			if (::VariantChangeType(&v, pParam->ppvarArgs_[0], 0, VT_BOOL) == S_OK &&
				v.boolVal == VARIANT_TRUE)
				select = TextWindow::SELECT_SELECT;
		}
	}
	
	pTextWindow_->moveCaret(moveCaret_, 0, 0, false, select, true);
}

bool qm::EditEditMoveCaretAction::isEnabled(const ActionEvent& event)
{
	return pTextWindow_->hasFocus();
}


/****************************************************************************
 *
 * EditEditPasteWithQuoteAction
 *
 */

qm::EditEditPasteWithQuoteAction::EditEditPasteWithQuoteAction(Document* pDocument,
															   EditMessageHolder* pEditMessageHolder,
															   TextWindow* pTextWindow,
															   SecurityModel* pSecurityModel,
															   Profile* pProfile,
															   HWND hwnd) :
	pDocument_(pDocument),
	pEditMessageHolder_(pEditMessageHolder),
	pTextWindow_(pTextWindow),
	pSecurityModel_(pSecurityModel),
	pProfile_(pProfile),
	hwnd_(hwnd)
{
}

qm::EditEditPasteWithQuoteAction::~EditEditPasteWithQuoteAction()
{
}

void qm::EditEditPasteWithQuoteAction::invoke(const ActionEvent& event)
{
	MessagePtr ptr(UIUtil::getMessageFromClipboard(pTextWindow_->getHandle(), pDocument_));
	
	wstring_ptr wstrMessage;
	wstring_ptr wstrMessageId;
	
	MessagePtrLock mpl(ptr);
	if (mpl) {
		Message msg;
		
		NormalFolder* pFolder = mpl->getFolder();
		Account* pAccount = pFolder->getAccount();
		const TemplateManager* pManager = pDocument_->getTemplateManager();
		const Template* pTemplate = pManager->getTemplate(pAccount, pFolder, L"quote");
		if (pTemplate) {
			TemplateContext context(mpl, &msg, MessageHolderList(), pAccount,
				pDocument_, hwnd_, 0, pSecurityModel_->getSecurityMode(),
				pProfile_, 0, TemplateContext::ArgumentList());
			switch (pTemplate->getValue(context, &wstrMessage)) {
			case Template::RESULT_SUCCESS:
				break;
			case Template::RESULT_ERROR:
				ActionUtil::error(pTextWindow_->getParentFrame(), IDS_ERROR_PASTE);
				return;
			case Template::RESULT_CANCEL:
				return;
			}
		}
		
		if (msg.getFlag() == Message::FLAG_EMPTY ||
			msg.getFlag() == Message::FLAG_TEMPORARY) {
			if (!mpl->getMessage(Account::GETMESSAGEFLAG_HEADER,
				L"Message-Id", pSecurityModel_->getSecurityMode(), &msg)) {
				ActionUtil::error(pTextWindow_->getParentFrame(), IDS_ERROR_PASTE);
				return;
			}
		}
		MessageIdParser messageId;
		if (msg.getField(L"Message-Id", &messageId) == Part::FIELD_EXIST)
			wstrMessageId = allocWString(messageId.getMessageId());
	}
	if (wstrMessage.get()) {
		if (!pTextWindow_->insertText(wstrMessage.get(), -1)) {
			ActionUtil::error(pTextWindow_->getParentFrame(), IDS_ERROR_PASTE);
			return;
		}
	}
	else {
		wstring_ptr wstrText(Clipboard::getText(pTextWindow_->getHandle()));
		if (!wstrText.get())
			return;
		
		wstring_ptr wstrQuote(pProfile_->getString(L"EditWindow", L"PasteQuote", L"> "));
		
		XStringBuffer<WSTRING> buf;
		bool bNewLine = true;
		for (const WCHAR* p = wstrText.get(); *p; ++p) {
			if (bNewLine) {
				if (!buf.append(wstrQuote.get()))
					return;
			}
			bNewLine = *p == L'\n';
			if (!buf.append(*p))
				return;
		}
		
		if (!pTextWindow_->insertText(buf.getCharArray(), buf.getLength())) {
			ActionUtil::error(pTextWindow_->getParentFrame(), IDS_ERROR_PASTE);
			return;
		}
	}
	
	if (wstrMessageId.get()) {
		EditMessage* pEditMessage = pEditMessageHolder_->getEditMessage();
		
		bool bFound = false;
		wstring_ptr wstrInReplyTo = pEditMessage->getField(
			L"In-Reply-To", EditMessage::FIELDTYPE_REFERENCES);
		if (wstrInReplyTo.get()) {
			Part part;
			if (MessageCreator::setField(&part, L"In-Reply-To",
				wstrInReplyTo.get(), MessageCreator::FIELDTYPE_REFERENCES)) {
				ReferencesParser inReplyTo;
				if (part.getField(L"In-Reply-To", &inReplyTo) == Part::FIELD_EXIST) {
					const ReferencesParser::ReferenceList& l = inReplyTo.getReferences();
					for (ReferencesParser::ReferenceList::const_iterator it = l.begin(); it != l.end() && !bFound; ++it) {
						if ((*it).second == ReferencesParser::T_MSGID &&
							wcscmp((*it).first, wstrMessageId.get()) == 0)
							bFound = true;
					}
				}
			}
		}
		if (!bFound) {
			StringBuffer<WSTRING> buf;
			if (wstrInReplyTo.get() && *wstrInReplyTo.get()) {
				buf.append(wstrInReplyTo.get());
				buf.append(L' ');
			}
			buf.append(L'<');
			buf.append(wstrMessageId.get());
			buf.append(L'>');
			
			pEditMessage->setField(L"In-Reply-To",
				buf.getCharArray(), EditMessage::FIELDTYPE_REFERENCES);
		}
	}
}

bool qm::EditEditPasteWithQuoteAction::isEnabled(const ActionEvent& event)
{
	if (pTextWindow_->hasFocus())
		return Clipboard::isFormatAvailable(Clipboard::CF_QSTEXT) ||
			Clipboard::isFormatAvailable(MessageDataObject::nFormats__[MessageDataObject::FORMAT_MESSAGEHOLDERLIST]);
	else
		return false;
}


/****************************************************************************
 *
 * EditEditReplaceAction
 *
 */

qm::EditEditReplaceAction::EditEditReplaceAction(TextWindow* pTextWindow,
												 Profile* pProfile,
												 FindReplaceManager* pFindReplaceManager) :
	pTextWindow_(pTextWindow),
	pProfile_(pProfile),
	pFindReplaceManager_(pFindReplaceManager)
{
}

qm::EditEditReplaceAction::~EditEditReplaceAction()
{
}

void qm::EditEditReplaceAction::invoke(const ActionEvent& event)
{
	HWND hwnd = pTextWindow_->getParentFrame();
	
	ReplaceDialog dialog(pProfile_);
	if (dialog.doModal(hwnd) != IDOK)
		return;
	
	pFindReplaceManager_->setData(dialog.getFind(), dialog.getReplace(),
		dialog.isMatchCase() ? FindReplaceData::FLAG_MATCHCASE : 0);
	
	ReplaceDialog::Type type = dialog.getType();
	unsigned int nFlags = 0;
	if (dialog.isMatchCase())
		nFlags |= TextWindow::FIND_MATCHCASE;
	if (dialog.isRegex())
		nFlags |= TextWindow::FIND_REGEX;
	if (type == ReplaceDialog::TYPE_PREV)
		nFlags |= TextWindow::FIND_PREVIOUS;
	
	if (type == ReplaceDialog::TYPE_ALL) {
		pTextWindow_->moveCaret(TextWindow::MOVECARET_DOCSTART,
			0, 0, false, TextWindow::SELECT_CLEAR, false);
		
		while (true) {
			bool bFound = pTextWindow_->replace(
				dialog.getFind(), dialog.getReplace(), nFlags);
			if (!bFound)
				break;
		}
	}
	else {
		bool bFound = pTextWindow_->replace(
			dialog.getFind(), dialog.getReplace(), nFlags);
		if (!bFound)
			ActionUtil::info(hwnd, IDS_FINDNOTFOUND);
	}
}

bool qm::EditEditReplaceAction::isEnabled(const ActionEvent& event)
{
	return pTextWindow_->hasFocus();
}


/****************************************************************************
 *
 * EditFileInsertAction
 *
 */

qm::EditFileInsertAction::EditFileInsertAction(TextWindow* pTextWindow) :
	pTextWindow_(pTextWindow)
{
}

qm::EditFileInsertAction::~EditFileInsertAction()
{
}

void qm::EditFileInsertAction::invoke(const ActionEvent& event)
{
	HWND hwnd = pTextWindow_->getParentFrame();
	
	wstring_ptr wstrFilter(loadString(
		Application::getApplication().getResourceHandle(), IDS_FILTER_INSERT));
	
	FileDialog dialog(true, wstrFilter.get(), 0, 0, 0,
		OFN_EXPLORER | OFN_HIDEREADONLY | OFN_LONGNAMES);
	if (dialog.doModal(hwnd) == IDOK) {
		if (!insertText(dialog.getPath()))
			ActionUtil::error(pTextWindow_->getParentFrame(), IDS_ERROR_INSERTFILE);
	}
}

bool qm::EditFileInsertAction::isEnabled(const ActionEvent& event)
{
	return pTextWindow_->hasFocus();
}

bool qm::EditFileInsertAction::insertText(const WCHAR* pwszPath)
{
	FileInputStream stream(pwszPath);
	if (!stream)
		return false;
	BufferedInputStream bufferedStream(&stream, false);
	InputStreamReader reader(&bufferedStream, false, 0);
	if (!reader)
		return false;
	
	// TODO
	// use malloc based buffer
	StringBuffer<WSTRING> buf;
	WCHAR wsz[1024];
	while (true) {
		size_t nLen = reader.read(wsz, countof(wsz));
		if (nLen == -1)
			return false;
		else if (nLen == 0)
			break;
		else
			buf.append(wsz, nLen);
	}
	
	if (!pTextWindow_->insertText(buf.getCharArray(), buf.getLength()))
		return false;
	
	return true;
}


/****************************************************************************
 *
 * EditFileOpenAction
 *
 */

qm::EditFileOpenAction::EditFileOpenAction(EditMessageHolder* pEditMessageHolder,
										   HWND hwnd) :
	pEditMessageHolder_(pEditMessageHolder),
	hwnd_(hwnd)
{
}

qm::EditFileOpenAction::~EditFileOpenAction()
{
}

void qm::EditFileOpenAction::invoke(const ActionEvent& event)
{
	wstring_ptr wstrFilter(loadString(
		Application::getApplication().getResourceHandle(), IDS_FILTER_OPEN));
	
	FileDialog dialog(true, wstrFilter.get(), 0, 0, 0,
		OFN_EXPLORER | OFN_HIDEREADONLY | OFN_LONGNAMES);
	if (dialog.doModal(hwnd_) == IDOK) {
		if (!open(dialog.getPath()))
			ActionUtil::error(hwnd_, IDS_ERROR_OPENFILE);
	}
}

bool qm::EditFileOpenAction::open(const WCHAR* pwszPath)
{
	FileInputStream stream(pwszPath);
	if (!stream)
		return false;
	BufferedInputStream bufferedStream(&stream, false);
	
	StringBuffer<STRING> buf;
	unsigned char sz[1024];
	while (true) {
		size_t nLen = bufferedStream.read(sz, countof(sz));
		if (nLen == -1)
			return false;
		else if (nLen == 0)
			break;
		else
			buf.append(reinterpret_cast<CHAR*>(sz), nLen);
	}
	
	std::auto_ptr<Message> pMessage(new Message());
	if (!pMessage->create(buf.getCharArray(), buf.getLength(), Message::FLAG_NONE))
		return false;
	
	EditMessage* pEditMessage = pEditMessageHolder_->getEditMessage();
	if (!pEditMessage->setMessage(pMessage))
		return false;
	
	return true;
}


/****************************************************************************
 *
 * EditFileSaveAction
 *
 */

qm::EditFileSaveAction::EditFileSaveAction(EditMessageHolder* pEditMessageHolder,
										   HWND hwnd) :
	pEditMessageHolder_(pEditMessageHolder),
	hwnd_(hwnd)
{
}

qm::EditFileSaveAction::~EditFileSaveAction()
{
}

void qm::EditFileSaveAction::invoke(const ActionEvent& event)
{
	wstring_ptr wstrFilter(loadString(
		Application::getApplication().getResourceHandle(), IDS_FILTER_SAVE));
	
	FileDialog dialog(true, wstrFilter.get(), 0, 0, 0,
		OFN_EXPLORER | OFN_HIDEREADONLY | OFN_LONGNAMES | OFN_OVERWRITEPROMPT);
	if (dialog.doModal(hwnd_) == IDOK) {
		if (!save(dialog.getPath()))
			ActionUtil::error(hwnd_, IDS_ERROR_SAVEFILE);
	}
}

bool qm::EditFileSaveAction::save(const WCHAR* pwszPath)
{
	EditMessage* pEditMessage = pEditMessageHolder_->getEditMessage();
	std::auto_ptr<Message> pMessage(pEditMessage->getMessage(false));
	if (!pMessage.get())
		return false;
	
	xstring_size_ptr strMessage(pMessage->getContent());
	if (!strMessage.get())
		return false;
	
	FileOutputStream stream(pwszPath);
	if (!stream)
		return false;
	BufferedOutputStream bufferedStream(&stream, false);
	if (bufferedStream.write(reinterpret_cast<unsigned char*>(strMessage.get()), strMessage.size()) == -1)
		return false;
	if (!bufferedStream.close())
		return false;
	
	return true;
}


/****************************************************************************
 *
 * EditFileSendAction
 *
 */

qm::EditFileSendAction::EditFileSendAction(Type type,
										   Document* pDocument,
										   PasswordManager* pPasswordManager,
										   EditMessageHolder* pEditMessageHolder,
										   EditFrameWindow* pEditFrameWindow,
										   Profile* pProfile,
										   SecurityModel* pSecurityModel) :
	composer_(type != TYPE_SEND, pDocument, pPasswordManager,
		pProfile, pEditFrameWindow->getHandle(), 0, pSecurityModel),
	type_(type),
	pEditMessageHolder_(pEditMessageHolder),
	pEditFrameWindow_(pEditFrameWindow),
	pDocument_(pDocument),
	pSyncManager_(0),
	pSyncDialogManager_(0)
{
}

qm::EditFileSendAction::EditFileSendAction(Document* pDocument,
										   PasswordManager* pPasswordManager,
										   EditMessageHolder* pEditMessageHolder,
										   EditFrameWindow* pEditFrameWindow,
										   Profile* pProfile,
										   SyncManager* pSyncManager,
										   SyncDialogManager* pSyncDialogManager,
										   SecurityModel* pSecurityModel) :
	composer_(false, pDocument, pPasswordManager, pProfile,
		pEditFrameWindow->getHandle(), 0, pSecurityModel),
	type_(TYPE_SEND),
	pEditMessageHolder_(pEditMessageHolder),
	pEditFrameWindow_(pEditFrameWindow),
	pDocument_(pDocument),
	pSyncManager_(pSyncManager),
	pSyncDialogManager_(pSyncDialogManager)
{
}

qm::EditFileSendAction::~EditFileSendAction()
{
}

void qm::EditFileSendAction::invoke(const ActionEvent& event)
{
	EditMessage* pEditMessage = pEditMessageHolder_->getEditMessage();
	std::auto_ptr<Message> pMessage(pEditMessage->getMessage(type_ == TYPE_SEND));
	if (!pMessage.get()) {
		ActionUtil::error(pEditFrameWindow_->getHandle(), IDS_ERROR_SEND);
		return;
	}
	
	Account* pAccount = pEditMessage->getAccount();
	SubAccount* pSubAccount = pEditMessage->getSubAccount();
	
	unsigned int nFlags = pEditMessage->getSecure();
	MessagePtr ptr;
	if (!composer_.compose(pEditMessage->getAccount(),
		pEditMessage->getSubAccount(), pMessage.get(), nFlags, &ptr)) {
		ActionUtil::error(pEditFrameWindow_->getHandle(), IDS_ERROR_SEND);
		return;
	}
	
	if (pSyncManager_) {
		const WCHAR* pwszMessageId = 0;
		MessageIdParser messageId;
		if (pMessage->getField(L"Message-Id", &messageId) == Part::FIELD_EXIST)
			pwszMessageId = messageId.getMessageId();
		if (!SyncUtil::send(pSyncManager_, pDocument_, pSyncDialogManager_,
			pEditFrameWindow_->getHandle(), 0, pAccount, pSubAccount, pwszMessageId)) {
			ActionUtil::error(pEditFrameWindow_->getHandle(), IDS_ERROR_SEND);
			return;
		}
	}
	
	const WCHAR* pwszURI = pEditMessage->getPreviousURI();
	if (pwszURI) {
		std::auto_ptr<URI> pURI(URI::parse(pwszURI));
		if (pURI.get()) {
			MessagePtrLock mpl(pDocument_->getMessage(*pURI));
			if (mpl) {
				Account* p = mpl->getFolder()->getAccount();
				if (!p->removeMessages(MessageHolderList(1, mpl), 0, false, 0)) {
					// TODO
				}
			}
		}
	}
	
	if (type_ != TYPE_SEND) {
		MessagePtrLock mpl(ptr);
		if (mpl) {
			wstring_ptr wstrURI(URI(mpl).toString());
			pEditMessage->setPreviousURI(wstrURI.get());
		}
		pEditMessage->removeField(L"X-QMAIL-DraftMacro");
	}
	if (type_ == TYPE_SEND || type_ == TYPE_DRAFTCLOSE)
		pEditFrameWindow_->close();
}


/****************************************************************************
 *
 * EditFocusItemAction
 *
 */

qm::EditFocusItemAction::EditFocusItemAction(EditWindow* pEditWindow) :
	pEditWindow_(pEditWindow)
{
}

qm::EditFocusItemAction::~EditFocusItemAction()
{
}

void qm::EditFocusItemAction::invoke(const ActionEvent& event)
{
	EditWindowItem* pItem = pEditWindow_->getItemByNumber(
		event.getId() - IDM_FOCUS_HEADEREDITITEM);
	if (pItem)
		pItem->setFocus();
}


/****************************************************************************
 *
 * EditToolAttachmentAction
 *
 */

qm::EditToolAttachmentAction::EditToolAttachmentAction(EditMessageHolder* pEditMessageHolder,
													   HWND hwnd) :
	pEditMessageHolder_(pEditMessageHolder),
	hwnd_(hwnd)
{
}

qm::EditToolAttachmentAction::~EditToolAttachmentAction()
{
}

void qm::EditToolAttachmentAction::invoke(const ActionEvent& event)
{
	EditMessage* pEditMessage = pEditMessageHolder_->getEditMessage();
	
	EditMessage::AttachmentList l;
	EditMessage::AttachmentListFree free(l);
	pEditMessage->getAttachments(&l);
	
	AttachmentDialog dialog(l);
	if (dialog.doModal(hwnd_) == IDOK)
		pEditMessage->setAttachments(l);
}


/****************************************************************************
 *
 * EditToolFlagAction
 *
 */

qm::EditToolFlagAction::EditToolFlagAction(EditMessageHolder* pEditMessageHolder,
										   PFN_IS pfnIs,
										   PFN_SET pfnSet,
										   bool bEnabled) :
	pEditMessageHolder_(pEditMessageHolder),
	pfnIs_(pfnIs),
	pfnSet_(pfnSet),
	bEnabled_(bEnabled)
{
}

qm::EditToolFlagAction::~EditToolFlagAction()
{
}

void qm::EditToolFlagAction::invoke(const ActionEvent& event)
{
	EditMessage* pEditMessage = pEditMessageHolder_->getEditMessage();
	(pEditMessage->*pfnSet_)(!(pEditMessage->*pfnIs_)());
}

bool qm::EditToolFlagAction::isEnabled(const ActionEvent& event)
{
	return bEnabled_;
}

bool qm::EditToolFlagAction::isChecked(const ActionEvent& event)
{
	EditMessage* pEditMessage = pEditMessageHolder_->getEditMessage();
	return (pEditMessage->*pfnIs_)();
}


/****************************************************************************
 *
 * EditToolInsertSignatureAction
 *
 */

qm::EditToolInsertSignatureAction::EditToolInsertSignatureAction(EditMessageHolder* pEditMessageHolder,
																 TextWindow* pTextWindow) :
	pEditMessageHolder_(pEditMessageHolder),
	pTextWindow_(pTextWindow)
{
}

qm::EditToolInsertSignatureAction::~EditToolInsertSignatureAction()
{
}

void qm::EditToolInsertSignatureAction::invoke(const ActionEvent& event)
{
	EditMessage* pEditMessage = pEditMessageHolder_->getEditMessage();
	wstring_ptr wstrSignature(pEditMessage->getSignatureText());
	if (wstrSignature.get()) {
		if (!pTextWindow_->insertText(wstrSignature.get(), -1)) {
			// TODO MSG
		}
		pEditMessage->setSignature(0);
	}
}

bool qm::EditToolInsertSignatureAction::isEnabled(const ActionEvent& event)
{
	return pTextWindow_->hasFocus() &&
		pEditMessageHolder_->getEditMessage()->getSignature();
}


/****************************************************************************
 *
 * EditToolInsertTextAction
 *
 */

qm::EditToolInsertTextAction::EditToolInsertTextAction(InsertTextMenu* pInsertTextMenu,
													   TextWindow* pTextWindow) :
	pInsertTextMenu_(pInsertTextMenu),
	pTextWindow_(pTextWindow)
{
}

qm::EditToolInsertTextAction::~EditToolInsertTextAction()
{
}

void qm::EditToolInsertTextAction::invoke(const ActionEvent& event)
{
	const FixedFormText* pText = pInsertTextMenu_->getText(event.getId());
	if (pText) {
		if (!pTextWindow_->insertText(pText->getText(), -1)) {
			// TODO MSG
		}
	}
}

bool qm::EditToolInsertTextAction::isEnabled(const ActionEvent& event)
{
	return pTextWindow_->hasFocus();
}


/****************************************************************************
 *
 * EditToolHeaderEditAction
 *
 */

qm::EditToolHeaderEditAction::EditToolHeaderEditAction(EditWindow* pEditWindow) :
	pEditWindow_(pEditWindow)
{
}

qm::EditToolHeaderEditAction::~EditToolHeaderEditAction()
{
}

void qm::EditToolHeaderEditAction::invoke(const ActionEvent& event)
{
	pEditWindow_->setHeaderEdit(!pEditWindow_->isHeaderEdit());
}

bool qm::EditToolHeaderEditAction::isChecked(const ActionEvent& event)
{
	return pEditWindow_->isHeaderEdit();
}


/****************************************************************************
 *
 * EditToolReformAction
 *
 */

qm::EditToolReformAction::EditToolReformAction(TextWindow* pTextWindow) :
	pTextWindow_(pTextWindow)
{
}

qm::EditToolReformAction::~EditToolReformAction()
{
}

void qm::EditToolReformAction::invoke(const ActionEvent& event)
{
	pTextWindow_->reform();
}

bool qm::EditToolReformAction::isEnabled(const ActionEvent& event)
{
	return pTextWindow_->hasFocus();
}


/****************************************************************************
 *
 * EditToolReformAllAction
 *
 */

qm::EditToolReformAllAction::EditToolReformAllAction(TextWindow* pTextWindow,
													 Profile* pProfile) :
	pTextWindow_(pTextWindow),
	pProfile_(pProfile)
{
}

qm::EditToolReformAllAction::~EditToolReformAllAction()
{
}

void qm::EditToolReformAllAction::invoke(const ActionEvent& event)
{
	EditableTextModel* pTextModel =
		static_cast<EditableTextModel*>(pTextWindow_->getTextModel());
	wxstring_ptr wstrText(pTextModel->getText());
	if (!wstrText.get()) {
		// TODO MSG
	}
	
	// TODO
	// Get line length and tab width
	wxstring_ptr wstrReformedText(TextUtil::fold(wstrText.get(), -1, 74, 0, 0, 4));
	if (!pTextModel->setText(wstrReformedText.get(), -1)) {
		// TODO MSG
	}
}

bool qm::EditToolReformAllAction::isEnabled(const ActionEvent& event)
{
	return pTextWindow_->hasFocus();
}


/****************************************************************************
 *
 * EditToolSecureAction
 *
 */

qm::EditToolSecureAction::EditToolSecureAction(EditMessageHolder* pEditMessageHolder,
											   EditMessage::Secure secure,
											   bool bEnabled) :
	pEditMessageHolder_(pEditMessageHolder),
	secure_(secure),
	bEnabled_(bEnabled)
{
}

qm::EditToolSecureAction::~EditToolSecureAction()
{
}

void qm::EditToolSecureAction::invoke(const ActionEvent& event)
{
	EditMessage* pEditMessage = pEditMessageHolder_->getEditMessage();
	pEditMessage->setSecure(secure_,
		(pEditMessage->getSecure() & secure_) == 0);
}

bool qm::EditToolSecureAction::isEnabled(const ActionEvent& event)
{
	return bEnabled_;
}

bool qm::EditToolSecureAction::isChecked(const ActionEvent& event)
{
	EditMessage* pEditMessage = pEditMessageHolder_->getEditMessage();
	return (pEditMessage->getSecure() & secure_) != 0;
}


/****************************************************************************
 *
 * EditToolSelectAddressAction
 *
 */

qm::EditToolSelectAddressAction::EditToolSelectAddressAction(EditMessageHolder* pEditMessageHolder,
															 EditWindow* pEditWindow,
															 AddressBook* pAddressBook,
															 Profile* pProfile) :
	pEditMessageHolder_(pEditMessageHolder),
	pEditWindow_(pEditWindow),
	pAddressBook_(pAddressBook),
	pProfile_(pProfile)
{
}

qm::EditToolSelectAddressAction::~EditToolSelectAddressAction()
{
}

void qm::EditToolSelectAddressAction::invoke(const ActionEvent& event)
{
	EditMessage* pEditMessage = pEditMessageHolder_->getEditMessage();
	pEditMessage->update();
	
	const WCHAR* pwszFields[] = {
		L"To",
		L"Cc",
		L"Bcc"
	};
	wstring_ptr wstrAddresses[countof(pwszFields)];
	for (int n = 0; n < countof(pwszFields); ++n)
		wstrAddresses[n] = pEditMessage->getField(
			pwszFields[n], EditMessage::FIELDTYPE_ADDRESSLIST);
	const WCHAR* pwszAddresses[] = {
		wstrAddresses[0].get() ? wstrAddresses[0].get() : L"",
		wstrAddresses[1].get() ? wstrAddresses[1].get() : L"",
		wstrAddresses[2].get() ? wstrAddresses[2].get() : L"",
	};
	SelectAddressDialog dialog(pAddressBook_, pProfile_, pwszAddresses);
	if (dialog.doModal(pEditWindow_->getParentFrame()) == IDOK) {
		struct Type
		{
			SelectAddressDialog::Type dialogType_;
			const WCHAR* pwszField_;
		} types[] = {
			{ SelectAddressDialog::TYPE_TO,		L"To"	},
			{ SelectAddressDialog::TYPE_CC,		L"Cc"	},
			{ SelectAddressDialog::TYPE_BCC,	L"Bcc"	}
		};
		for (int n = 0; n < countof(types); ++n) {
			StringBuffer<WSTRING> buf;
			const SelectAddressDialog::AddressList& l =
				dialog.getAddresses(types[n].dialogType_);
			for (SelectAddressDialog::AddressList::const_iterator it = l.begin(); it != l.end(); ++it) {
				if (buf.getLength() != 0)
					buf.append(L", ");
				buf.append(*it);
			}
			
			pEditMessage->setField(types[n].pwszField_,
				buf.getCharArray(), EditMessage::FIELDTYPE_ADDRESSLIST);
		}
	}
}

bool qm::EditToolSelectAddressAction::isEnabled(const ActionEvent& event)
{
	return !pEditWindow_->isHeaderEdit();
}
