/*
 * $Id$
 *
 * Copyright(C) 1998-2008 Satoshi Nakamura
 *
 */

#pragma warning(disable:4786)

#include <qmaccount.h>
#include <qmdocument.h>
#include <qmeditwindow.h>
#include <qmfolder.h>
#include <qmmacro.h>
#include <qmmessage.h>
#include <qmsecurity.h>
#include <qmtemplate.h>

#include <qsstl.h>
#include <qsstream.h>
#include <qstextutil.h>

#include <algorithm>
#include <functional>

#include <boost/bind.hpp>

#include <commdlg.h>

#include "actionutil.h"
#include "editaction.h"
#include "findreplace.h"
#include "../main/main.h"
#include "../model/dataobject.h"
#include "../model/editmessage.h"
#include "../model/fixedformtext.h"
#include "../model/messagecontext.h"
#include "../model/templatemanager.h"
#include "../model/uri.h"
#include "../ui/addressbookdialog.h"
#include "../ui/dialogs.h"
#include "../ui/editwindow.h"
#include "../ui/focus.h"
#include "../ui/syncutil.h"
#include "../ui/uiutil.h"
#include "../uimodel/attachmentselectionmodel.h"
#include "../uimodel/securitymodel.h"

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
	
	wstring_ptr wstrFilter(loadString(getResourceHandle(), IDS_FILTER_ATTACHMENT));
	FileDialog dialog(true, wstrFilter.get(), 0, 0, 0,
		OFN_EXPLORER | OFN_HIDEREADONLY | OFN_LONGNAMES | OFN_ALLOWMULTISELECT);
	
	if (dialog.doModal(hwnd_) != IDOK)
		return;
	
	const WCHAR* pwszPath = dialog.getPath();
	const WCHAR* p = pwszPath;
	while (*p) {
		pEditMessage->addAttachment(p);
		p += wcslen(p) + 1;
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
	return !pAttachmentSelectionModel_->isAttachmentDeleted() &&
		pAttachmentSelectionModel_->hasSelectedAttachment();
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

bool qm::EditEditDeleteAction::isEnabled(const ActionEvent& event)
{
	return pTextWindow_->hasFocus();
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
		struct CallbackImpl : public FindDialog::Callback
		{
			CallbackImpl(TextWindow* pTextWindow,
						 bool& bFound) :
				pTextWindow_(pTextWindow),
				bFound_(bFound),
				nLine_(-1),
				nChar_(-1),
				bSearched_(false)
			{
				pTextWindow_->getFindPosition(false, &nLine_, &nChar_);
			}
			
			virtual void statusChanged(const WCHAR* pwszFind,
									   bool bMatchCase,
									   bool bRegex)
			{
				if (bSearched_)
					pTextWindow_->moveCaret(TextWindow::MOVECARET_POS, nLine_, nChar_,
						TextWindow::SELECT_CLEAR, TextWindow::MOVECARETFLAG_NONE);
				
				unsigned int nFlags =
					(bMatchCase ? TextWindow::FIND_MATCHCASE : 0) |
					(bRegex ? TextWindow::FIND_REGEX : 0);
				bFound_ = pTextWindow_->find(pwszFind, nFlags);
				
				bSearched_ = true;
			}
			
			TextWindow* pTextWindow_;
			bool& bFound_;
			size_t nLine_;
			size_t nChar_;
			bool bSearched_;
		} callback(pTextWindow_, bFound);
		
		bool bIncremental = pProfile_->getInt(L"Global", L"IncrementalSearch") != 0;
		FindDialog dialog(pProfile_, true, bIncremental ? &callback : 0);
		if (dialog.doModal(hwnd) != IDOK)
			return;
		
		pFindReplaceManager_->setData(dialog.getFind(),
			(dialog.isMatchCase() ? FindReplaceData::FLAG_MATCHCASE : 0) |
			(dialog.isRegex() ? FindReplaceData::FLAG_REGEX : 0));
		
		if (!bIncremental || !callback.bSearched_) {
			unsigned int nFlags =
				(dialog.isMatchCase() ? TextWindow::FIND_MATCHCASE : 0) |
				(dialog.isRegex() ? TextWindow::FIND_REGEX : 0) |
				(dialog.isPrev() ? TextWindow::FIND_PREVIOUS : 0);
			bFound = pTextWindow_->find(dialog.getFind(), nFlags);
		}
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
		ActionUtil::info(hwnd, IDS_MESSAGE_FINDNOTFOUND);
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
	const WCHAR* pwszParam = ActionParamUtil::getString(event.getParam(), 0);
	if (pwszParam && _wcsicmp(pwszParam, L"true") == 0)
		select = TextWindow::SELECT_SELECT;
	
	pTextWindow_->moveCaret(moveCaret_, 0, 0, select, TextWindow::MOVECARETFLAG_SCROLL);
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
															   const ActionInvoker* pActionInvoker,
															   Profile* pProfile,
															   HWND hwnd) :
	pDocument_(pDocument),
	pEditMessageHolder_(pEditMessageHolder),
	pTextWindow_(pTextWindow),
	pSecurityModel_(pSecurityModel),
	pActionInvoker_(pActionInvoker),
	pProfile_(pProfile),
	hwnd_(hwnd)
{
}

qm::EditEditPasteWithQuoteAction::~EditEditPasteWithQuoteAction()
{
}

void qm::EditEditPasteWithQuoteAction::invoke(const ActionEvent& event)
{
	std::auto_ptr<MessageContext> pContext(UIUtil::getMessageFromClipboard(
		pTextWindow_->getHandle(), pDocument_->getURIResolver()));
	
	wxstring_size_ptr wstrMessage;
	wstring_ptr wstrMessageId;
	
	if (pContext.get()) {
		unsigned int nSecurityMode = pSecurityModel_->getSecurityMode();
		Message msg;
		Message* pMessage = &msg;
		MessagePtr ptr(pContext->getMessagePtr());
		if (!ptr)
			pMessage = pContext->getMessage(Account::GMF_ALL, 0, nSecurityMode);
		
		if (pMessage) {
			MessagePtrLock mpl(ptr);
			NormalFolder* pFolder = mpl ? mpl->getFolder() : 0;
			Account* pAccount = pFolder ? pFolder->getAccount() : 0;
			const TemplateManager* pManager = pDocument_->getTemplateManager();
			const Template* pTemplate = pManager->getTemplate(pAccount, pFolder, L"quote");
			if (pTemplate) {
				TemplateContext context(mpl, pMessage, MessageHolderList(),
					pFolder, pAccount, pDocument_, pActionInvoker_, hwnd_, 0,
					MacroContext::FLAG_UITHREAD | MacroContext::FLAG_UI,
					nSecurityMode, pProfile_, 0, TemplateContext::ArgumentList());
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
			
			if (mpl &&
				pMessage->getFlag() == Message::FLAG_EMPTY ||
				pMessage->getFlag() == Message::FLAG_TEMPORARY) {
				if (!mpl->getMessage(Account::GMF_HEADER, L"Message-Id",
					pSecurityModel_->getSecurityMode(), pMessage)) {
					ActionUtil::error(pTextWindow_->getParentFrame(), IDS_ERROR_PASTE);
					return;
				}
			}
			MessageIdParser messageId;
			if (pMessage->getField(L"Message-Id", &messageId) == Part::FIELD_EXIST)
				wstrMessageId = allocWString(messageId.getMessageId());
		}
	}
	if (wstrMessage.get()) {
		if (!pTextWindow_->insertText(wstrMessage.get(), wstrMessage.size())) {
			ActionUtil::error(pTextWindow_->getParentFrame(), IDS_ERROR_PASTE);
			return;
		}
	}
	else {
		wstring_ptr wstrText(Clipboard::getText(pTextWindow_->getHandle()));
		if (!wstrText.get())
			return;
		
		wstring_ptr wstrQuote(pProfile_->getString(L"Global", L"Quote"));
		
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
		pTextWindow_->moveCaret(TextWindow::MOVECARET_DOCSTART, 0, 0,
			TextWindow::SELECT_CLEAR, TextWindow::MOVECARETFLAG_NONE);
		
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
			ActionUtil::info(hwnd, IDS_MESSAGE_FINDNOTFOUND);
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
	
	wstring_ptr wstrFilter(loadString(getResourceHandle(), IDS_FILTER_INSERT));
	FileDialog dialog(true, wstrFilter.get(), 0, 0, 0,
		OFN_EXPLORER | OFN_HIDEREADONLY | OFN_LONGNAMES);
	if (dialog.doModal(hwnd) != IDOK)
		return;
	
	if (!insertText(dialog.getPath()))
		ActionUtil::error(pTextWindow_->getParentFrame(), IDS_ERROR_INSERTFILE);
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
	
	XStringBuffer<WXSTRING> buf;
	const size_t nBufferSize = 8192;
	while (true) {
		XStringBufferLock<WXSTRING> lock(&buf, nBufferSize);
		if (!lock.get())
			return false;
		
		size_t nRead = reader.read(lock.get(), nBufferSize);
		if (nRead == -1)
			return false;
		else if (nRead == 0)
			break;
		
		lock.unlock(nRead);
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
	const WCHAR* pwszPath = ActionParamUtil::getString(event.getParam(), 0);
	wstring_ptr wstrPath;
	if (!pwszPath) {
		wstring_ptr wstrFilter(loadString(getResourceHandle(), IDS_FILTER_OPEN));
		FileDialog dialog(true, wstrFilter.get(), 0, 0, 0,
			OFN_EXPLORER | OFN_HIDEREADONLY | OFN_LONGNAMES);
		if (dialog.doModal(hwnd_) != IDOK)
			return;
		wstrPath = allocWString(dialog.getPath());
		pwszPath = wstrPath.get();
	}
	
	if (!open(pwszPath))
		ActionUtil::error(hwnd_, IDS_ERROR_OPENFILE);
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
	const WCHAR* pwszPath = ActionParamUtil::getString(event.getParam(), 0);
	wstring_ptr wstrPath;
	if (!pwszPath) {
		wstring_ptr wstrFilter(loadString(getResourceHandle(), IDS_FILTER_SAVE));
		FileDialog dialog(false, wstrFilter.get(), 0, 0, 0,
			OFN_EXPLORER | OFN_HIDEREADONLY | OFN_LONGNAMES | OFN_OVERWRITEPROMPT);
		if (dialog.doModal(hwnd_) != IDOK)
			return;
		wstrPath = allocWString(dialog.getPath());
		pwszPath = wstrPath.get();
	}
	
	if (!save(pwszPath))
		ActionUtil::error(hwnd_, IDS_ERROR_SAVEFILE);
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
	
#ifdef QMZIP
	if (pEditMessage->isArchiveAttachments()) {
		EditMessage::AttachmentList l;
		EditMessage::AttachmentListFree free(l);
		pEditMessage->getAttachments(&l);
		EditMessage::AttachmentList::const_iterator it = std::find_if(
			l.begin(), l.end(),
			boost::bind(std::logical_and<bool>(),
				boost::bind(&EditMessage::Attachment::bNew_, _1),
				boost::bind(&MessageComposer::isAttachmentArchiving, &composer_,
					boost::bind(&EditMessage::Attachment::wstrName_, _1))));
		if (it != l.end()) {
			const WCHAR* pwszArchive = pEditMessage->getArchiveName();
			wstring_ptr wstrArchive;
			if (!pwszArchive) {
				wstrArchive = allocWString((*it).wstrName_);
				WCHAR* pFileName = wcsrchr(wstrArchive.get(), L'\\');
				if (pFileName)
					*pFileName++ = L'\0';
				else
					pFileName = wstrArchive.get();
				WCHAR* pExt = wcsrchr(pFileName, L'.');
				if (pExt)
					*pExt = L'\0';
				wstrArchive = concat(*pFileName ? pFileName : L"attachment", L".zip");
				pwszArchive = wstrArchive.get();
			}
			ArchiveDialog dialog(pwszArchive);
			if (dialog.doModal(pEditFrameWindow_->getHandle()) != IDOK)
				return;
			pEditMessage->setArchiveName(dialog.getFileName());
		}
	}
#endif
	
	std::auto_ptr<Message> pMessage(pEditMessage->getMessage(type_ == TYPE_SEND));
	if (!pMessage.get()) {
		ActionUtil::error(pEditFrameWindow_->getHandle(), IDS_ERROR_SEND);
		return;
	}
	
	Account* pAccount = pEditMessage->getAccount();
	SubAccount* pSubAccount = pEditMessage->getSubAccount();
	
	unsigned int nMessageSecurity = pEditMessage->getMessageSecurity();
	MessagePtr ptr;
	if (!composer_.compose(pMessage.get(), nMessageSecurity,
		pEditMessage->getAccount(), pEditMessage->getSubAccount(), &ptr)) {
		ActionUtil::error(pEditFrameWindow_->getHandle(), IDS_ERROR_SEND);
		return;
	}
	
	if (pSyncManager_) {
		const WCHAR* pwszMessageId = 0;
		MessageIdParser messageId;
		if (pMessage->getField(L"Message-Id", &messageId) == Part::FIELD_EXIST)
			pwszMessageId = messageId.getMessageId();
		if (!SyncUtil::send(pSyncManager_, pDocument_, pSyncDialogManager_,
			SyncData::TYPE_ACTIVE, pAccount, pSubAccount, pwszMessageId)) {
			ActionUtil::error(pEditFrameWindow_->getHandle(), IDS_ERROR_SEND);
			return;
		}
	}
	
	const WCHAR* pwszURI = pEditMessage->getPreviousURI();
	if (pwszURI) {
		std::auto_ptr<MessageHolderURI> pURI(URIFactory::parseMessageHolderURI(pwszURI));
		if (pURI.get()) {
			MessagePtrLock mpl(pURI->resolveMessagePtr(pDocument_->getURIResolver()));
			if (mpl) {
				Account* p = mpl->getAccount();
				if (!p->removeMessages(MessageHolderList(1, mpl),
					0, Account::OPFLAG_ACTIVE, 0, 0, 0)) {
					// TODO
				}
			}
		}
	}
	
	if (type_ != TYPE_SEND) {
		MessagePtrLock mpl(ptr);
		if (mpl) {
			wstring_ptr wstrURI(MessageHolderURI(mpl).toString());
			pEditMessage->setPreviousURI(wstrURI.get());
		}
		pEditMessage->removeField(L"X-QMAIL-DraftMacro");
	}
	if (type_ == TYPE_SEND || type_ == TYPE_DRAFTCLOSE)
		pEditFrameWindow_->close();
}


/****************************************************************************
 *
 * EditToolApplyTemplateAction
 *
 */

qm::EditToolApplyTemplateAction::EditToolApplyTemplateAction(EditMessageHolder* pEditMessageHolder,
															 const ActionInvoker* pActionInvoker,
															 Profile* pProfile,
															 HWND hwnd) :
	pEditMessageHolder_(pEditMessageHolder),
	pActionInvoker_(pActionInvoker),
	pProfile_(pProfile),
	hwnd_(hwnd)
{
}

qm::EditToolApplyTemplateAction::~EditToolApplyTemplateAction()
{
}

void qm::EditToolApplyTemplateAction::invoke(const ActionEvent& event)
{
	const WCHAR* pwszTemplate = ActionParamUtil::getString(event.getParam(), 0);
	if (!pwszTemplate || !*pwszTemplate)
		return;
	
	TemplateContext::ArgumentList listArg;
	TemplateActionUtil::ArgList l;
	StringListFree<TemplateActionUtil::ArgList> free(l);
	TemplateActionUtil::parseArgs(event.getParam(), 1, &listArg, &l);
	
	EditMessage* pEditMessage = pEditMessageHolder_->getEditMessage();
	Document* pDocument = pEditMessage->getDocument();
	const Template* pTemplate = pDocument->getTemplateManager()->getTemplate(
		pEditMessage->getAccount(), 0, pwszTemplate);
	if (!pTemplate) {
		ActionUtil::error(hwnd_, IDS_ERROR_APPLYTEMPLATE);
		return;
	}
	
	std::auto_ptr<Message> pMessage(pEditMessage->getMessage(false));
	if (!pMessage.get()) {
		ActionUtil::error(hwnd_, IDS_ERROR_APPLYTEMPLATE);
		return;
	}
	
	TemplateContext context(0, pMessage.get(), MessageHolderList(), 0,
		pEditMessage->getAccount(), pDocument, pActionInvoker_, hwnd_,
		0, MacroContext::FLAG_UITHREAD | MacroContext::FLAG_UI,
		SECURITYMODE_NONE, pProfile_, 0, listArg);
	wstring_ptr wstr;
	Template::Result r = pTemplate->getValue(context, &wstr);
	switch (r) {
	case Template::RESULT_SUCCESS:
		break;
	case Template::RESULT_ERROR:
		ActionUtil::error(hwnd_, IDS_ERROR_APPLYTEMPLATE);
		return;
	case Template::RESULT_CANCEL:
		return;
	}
	
	std::auto_ptr<Message> p(MessageCreator().createMessage(wstr.get(), -1));
	if (!p.get()) {
		ActionUtil::error(hwnd_, IDS_ERROR_APPLYTEMPLATE);
		return;
	}
	
	if (!pEditMessage->setMessage(p)) {
		ActionUtil::error(hwnd_, IDS_ERROR_APPLYTEMPLATE);
		return;
	}
}

bool qm::EditToolApplyTemplateAction::isEnabled(const ActionEvent& event)
{
	const WCHAR* pwszTemplate = ActionParamUtil::getString(event.getParam(), 0);
	return pwszTemplate && *pwszTemplate;
}


#ifdef QMZIP
/****************************************************************************
 *
 * EditToolArchiveAttachmentAction
 *
 */

qm::EditToolArchiveAttachmentAction::EditToolArchiveAttachmentAction(EditMessageHolder* pEditMessageHolder) :
	pEditMessageHolder_(pEditMessageHolder)
{
}

qm::EditToolArchiveAttachmentAction::~EditToolArchiveAttachmentAction()
{
}

void qm::EditToolArchiveAttachmentAction::invoke(const ActionEvent& event)
{
	EditMessage* pEditMessage = pEditMessageHolder_->getEditMessage();
	pEditMessage->setArchiveAttachments(!pEditMessage->isArchiveAttachments());
}

bool qm::EditToolArchiveAttachmentAction::isChecked(const qs::ActionEvent& event)
{
	EditMessage* pEditMessage = pEditMessageHolder_->getEditMessage();
	return pEditMessage->isArchiveAttachments();
}
#endif


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
 * EditToolEncodingAction
 *
 */

qm::EditToolEncodingAction::EditToolEncodingAction(EditMessageHolder* pEditMessageHolder) :
	pEditMessageHolder_(pEditMessageHolder)
{
}

qm::EditToolEncodingAction::~EditToolEncodingAction()
{
}

void qm::EditToolEncodingAction::invoke(const ActionEvent& event)
{
	const WCHAR* pwszEncoding = ActionParamUtil::getString(event.getParam(), 0);
	if (!pwszEncoding)
		return;
	
	EditMessage* pEditMessage = pEditMessageHolder_->getEditMessage();
	if (*pwszEncoding)
		pEditMessage->setEncoding(pwszEncoding);
	else
		pEditMessage->setEncoding(0);
}

bool qm::EditToolEncodingAction::isEnabled(const ActionEvent& event)
{
	return ActionParamUtil::getString(event.getParam(), 0) != 0;
}

bool qm::EditToolEncodingAction::isChecked(const ActionEvent& event)
{
	const WCHAR* pwszEncoding = ActionParamUtil::getString(event.getParam(), 0);
	if (!pwszEncoding)
		return false;
	
	EditMessage* pEditMessage = pEditMessageHolder_->getEditMessage();
	const WCHAR* pwszCurrentEncoding = pEditMessage->getEncoding();
	if (*pwszEncoding)
		return pwszCurrentEncoding && wcscmp(pwszEncoding, pwszCurrentEncoding) == 0;
	else
		return !pwszCurrentEncoding;
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
 * EditToolInsertMacroAction
 *
 */

qm::EditToolInsertMacroAction::EditToolInsertMacroAction(EditMessageHolder* pEditMessageHolder,
														 TextWindow* pTextWindow,
														 const ActionInvoker* pActionInvoker,
														 Profile* pProfile,
														 HWND hwnd) :
	pEditMessageHolder_(pEditMessageHolder),
	pTextWindow_(pTextWindow),
	pActionInvoker_(pActionInvoker),
	pProfile_(pProfile),
	hwnd_(hwnd)
{
}

qm::EditToolInsertMacroAction::~EditToolInsertMacroAction()
{
}

void qm::EditToolInsertMacroAction::invoke(const ActionEvent& event)
{
	std::auto_ptr<Macro> pMacro(MacroActionUtil::getMacro(
		event.getParam(), 0, pProfile_, hwnd_));
	if (!pMacro.get())
		return;
	
	EditMessage* pEditMessage = pEditMessageHolder_->getEditMessage();
	std::auto_ptr<Message> pMessage(pEditMessage->getMessage(false));
	if (!pMessage.get()) {
		ActionUtil::error(hwnd_, IDS_ERROR_INSERTMACRO);
		return;
	}
	
	MacroVariableHolder globalVariable;
	MacroContext context(0, pMessage.get(), pEditMessage->getAccount(),
		MessageHolderList(), 0, pEditMessage->getDocument(), pActionInvoker_,
		hwnd_, pProfile_, 0, MacroContext::FLAG_UI | MacroContext::FLAG_UITHREAD,
		SECURITYMODE_NONE, 0, &globalVariable);
	MacroValuePtr pValue(pMacro->value(&context));
	if (pValue.get()) {
		if (!pTextWindow_->insertText(pValue->string().get(), -1)) {
			ActionUtil::error(hwnd_, IDS_ERROR_INSERTMACRO);
			return;
		}
	}
	else {
		if (context.getReturnType() == MacroContext::RETURNTYPE_NONE)
			ActionUtil::error(hwnd_, IDS_ERROR_EVALUATEMACRO);
	}
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
			ActionUtil::error(pTextWindow_->getParentFrame(), IDS_ERROR_INSERTSIGNATURE);
			return;
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

qm::EditToolInsertTextAction::EditToolInsertTextAction(FixedFormTextManager* pFixedFormTextManager,
													   TextWindow* pTextWindow) :
	pFixedFormTextManager_(pFixedFormTextManager),
	pTextWindow_(pTextWindow)
{
}

qm::EditToolInsertTextAction::~EditToolInsertTextAction()
{
}

void qm::EditToolInsertTextAction::invoke(const ActionEvent& event)
{
	std::pair<const WCHAR*, unsigned int> param(ActionParamUtil::getStringOrIndex(event.getParam(), 0));
	
	const FixedFormText* pText = 0;
	if (param.first) {
		pText = pFixedFormTextManager_->getText(param.first);
	}
	else if (param.second != -1) {
		const FixedFormTextManager::TextList& l = pFixedFormTextManager_->getTexts();
		if (l.size() > param.second)
			pText = l[param.second];
	}
	if (!pText)
		return;
	
	if (!pTextWindow_->insertText(pText->getText(), -1)) {
		ActionUtil::error(pTextWindow_->getParentFrame(), IDS_ERROR_INSERTSIGNATURE);
		return;
	}
}

bool qm::EditToolInsertTextAction::isEnabled(const ActionEvent& event)
{
	std::pair<const WCHAR*, unsigned int> param(ActionParamUtil::getStringOrIndex(event.getParam(), 0));
	if (!param.first &&
		(param.second == -1 || param.second >= pFixedFormTextManager_->getTexts().size()))
		return false;
	
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
 * EditToolMessageSecurityAction
 *
 */

qm::EditToolMessageSecurityAction::EditToolMessageSecurityAction(EditMessageHolder* pEditMessageHolder,
																 MessageSecurity security,
																 bool bEnabled) :
	pEditMessageHolder_(pEditMessageHolder),
	security_(security),
	bEnabled_(bEnabled)
{
}

qm::EditToolMessageSecurityAction::~EditToolMessageSecurityAction()
{
}

void qm::EditToolMessageSecurityAction::invoke(const ActionEvent& event)
{
	EditMessage* pEditMessage = pEditMessageHolder_->getEditMessage();
	pEditMessage->setMessageSecurity(security_,
		(pEditMessage->getMessageSecurity() & security_) == 0);
}

bool qm::EditToolMessageSecurityAction::isEnabled(const ActionEvent& event)
{
	return bEnabled_;
}

bool qm::EditToolMessageSecurityAction::isChecked(const ActionEvent& event)
{
	EditMessage* pEditMessage = pEditMessageHolder_->getEditMessage();
	return (pEditMessage->getMessageSecurity() & security_) != 0;
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
		ActionUtil::error(pTextWindow_->getParentFrame(), IDS_ERROR_REFORM);
		return;
	}
	
	int nLineLen = pProfile_->getInt(L"EditWindow", L"ReformLineLength");
	int nTabWidth = pProfile_->getInt(L"EditWindow", L"TabWidth");
	wxstring_ptr wstrReformedText(TextUtil::fold(wstrText.get(), -1, nLineLen, 0, 0, nTabWidth));
	if (!pTextModel->setText(wstrReformedText.get(), -1)) {
		ActionUtil::error(pTextWindow_->getParentFrame(), IDS_ERROR_REFORM);
		return;
	}
}

bool qm::EditToolReformAllAction::isEnabled(const ActionEvent& event)
{
	return pTextWindow_->hasFocus();
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
