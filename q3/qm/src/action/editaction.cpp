/*
 * $Id$
 *
 * Copyright(C) 1998-2003 Satoshi Nakamura
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

#include <qsnew.h>
#include <qsstl.h>
#include <qsstream.h>
#include <qstextutil.h>

#include <algorithm>

#include <commdlg.h>

#include "action.h"
#include "editaction.h"
#include "findreplace.h"
#include "../model/editmessage.h"
#include "../model/fixedformtext.h"
#include "../ui/attachmentselectionmodel.h"
#include "../ui/dialogs.h"
#include "../ui/editwindow.h"
#include "../ui/resourceinc.h"
#include "../ui/syncutil.h"

using namespace qm;
using namespace qs;


/****************************************************************************
 *
 * EditAttachmentEditAddAction
 *
 */

qm::EditAttachmentEditAddAction::EditAttachmentEditAddAction(
	EditMessageHolder* pEditMessageHolder, HWND hwndFrame, QSTATUS* pstatus) :
	pEditMessageHolder_(pEditMessageHolder),
	hwndFrame_(hwndFrame)
{
}

qm::EditAttachmentEditAddAction::~EditAttachmentEditAddAction()
{
}

QSTATUS qm::EditAttachmentEditAddAction::invoke(const ActionEvent& event)
{
	DECLARE_QSTATUS();
	
	EditMessage* pEditMessage = pEditMessageHolder_->getEditMessage();
	
	string_ptr<WSTRING> wstrFilter;
	status = loadString(Application::getApplication().getResourceHandle(),
		IDS_FILTER_ATTACHMENT, &wstrFilter);
	CHECK_QSTATUS();
	
	FileDialog dialog(true, wstrFilter.get(), 0, 0, 0,
		OFN_EXPLORER | OFN_HIDEREADONLY | OFN_LONGNAMES | OFN_ALLOWMULTISELECT,
		&status);
	CHECK_QSTATUS_VALUE(0);
	
	int nRet = IDCANCEL;
	status = dialog.doModal(hwndFrame_, 0, &nRet);
	CHECK_QSTATUS_VALUE(0);
	if (nRet == IDOK) {
		const WCHAR* pwszPath = dialog.getPath();
		const WCHAR* p = pwszPath;
		while (*p) {
			status = pEditMessage->addAttachment(p);
			CHECK_QSTATUS();
			p += wcslen(p) + 1;
		}
	}
	
	return QSTATUS_SUCCESS;
}


/****************************************************************************
 *
 * EditAttachmentEditDeleteAction
 *
 */

qm::EditAttachmentEditDeleteAction::EditAttachmentEditDeleteAction(
	EditMessageHolder* pEditMessageHolder,
	AttachmentSelectionModel* pAttachmentSelectionModel, QSTATUS* pstatus) :
	pEditMessageHolder_(pEditMessageHolder),
	pAttachmentSelectionModel_(pAttachmentSelectionModel)
{
}

qm::EditAttachmentEditDeleteAction::~EditAttachmentEditDeleteAction()
{
}

QSTATUS qm::EditAttachmentEditDeleteAction::invoke(const ActionEvent& event)
{
	DECLARE_QSTATUS();
	
	EditMessage* pEditMessage = pEditMessageHolder_->getEditMessage();
	
	AttachmentSelectionModel::NameList l;
	StringListFree<AttachmentSelectionModel::NameList> free(l);
	status = pAttachmentSelectionModel_->getSelectedAttachment(&l);
	CHECK_QSTATUS();
	
	AttachmentSelectionModel::NameList::const_iterator it = l.begin();
	while (it != l.end()) {
		status = pEditMessage->removeAttachment(*it);
		CHECK_QSTATUS();
		++it;
	}
	
	return QSTATUS_SUCCESS;
}

QSTATUS qm::EditAttachmentEditDeleteAction::isEnabled(
	const ActionEvent& event, bool* pbEnabled)
{
	assert(pbEnabled);
	return pAttachmentSelectionModel_->hasSelectedAttachment(pbEnabled);
}


/****************************************************************************
 *
 * EditEditCommandAction
 *
 */

qm::EditEditCommandAction::EditEditCommandAction(EditWindow* pEditWindow,
	PFN_DO pfnDo, PFN_CANDO pfnCanDo, QSTATUS* pstatus) :
	pEditWindow_(pEditWindow),
	pfnDo_(pfnDo),
	pfnCanDo_(pfnCanDo)
{
	*pstatus = QSTATUS_SUCCESS;
}

qm::EditEditCommandAction::~EditEditCommandAction()
{
}

QSTATUS qm::EditEditCommandAction::invoke(const ActionEvent& event)
{
	DECLARE_QSTATUS();
	
	EditWindowItem* pItem = pEditWindow_->getFocusedItem();
	if (pItem) {
		status = (pItem->*pfnDo_)();
		CHECK_QSTATUS();
	}
	
	return QSTATUS_SUCCESS;
}

QSTATUS qm::EditEditCommandAction::isEnabled(
	const ActionEvent& event, bool* pbEnabled)
{
	DECLARE_QSTATUS();
	
	EditWindowItem* pItem = pEditWindow_->getFocusedItem();
	if (pItem) {
		status = (pItem->*pfnCanDo_)(pbEnabled);
		CHECK_QSTATUS();
	}
	else {
		*pbEnabled = false;
	}
	
	return QSTATUS_SUCCESS;
}


/****************************************************************************
 *
 * EditEditFindAction
 *
 */

qm::EditEditFindAction::EditEditFindAction(TextWindow* pTextWindow,
	Profile* pProfile, FindReplaceManager* pFindReplaceManager, QSTATUS* pstatus) :
	pTextWindow_(pTextWindow),
	pProfile_(pProfile),
	pFindReplaceManager_(pFindReplaceManager),
	type_(TYPE_NORMAL)
{
}

qm::EditEditFindAction::EditEditFindAction(TextWindow* pTextWindow,
	bool bNext, FindReplaceManager* pFindReplaceManager, QSTATUS* pstatus) :
	pTextWindow_(pTextWindow),
	pProfile_(0),
	pFindReplaceManager_(pFindReplaceManager),
	type_(bNext ? TYPE_NEXT : TYPE_PREV)
{
}

qm::EditEditFindAction::~EditEditFindAction()
{
}

QSTATUS qm::EditEditFindAction::invoke(const ActionEvent& event)
{
	DECLARE_QSTATUS();
	
	HWND hwndFrame = pTextWindow_->getParentFrame();
	
	bool bFound = false;
	if (type_ == TYPE_NORMAL) {
		FindDialog dialog(pProfile_, &status);
		CHECK_QSTATUS();
		int nRet = 0;
		status = dialog.doModal(hwndFrame, 0, &nRet);
		CHECK_QSTATUS();
		if (nRet != IDOK)
			return QSTATUS_SUCCESS;
		
		status = pFindReplaceManager_->setData(dialog.getFind(),
			dialog.isMatchCase() ? FindReplaceData::FLAG_MATCHCASE : 0);
		CHECK_QSTATUS();
		
		unsigned int nFlags = 0;
		if (dialog.isMatchCase())
			nFlags |= TextWindow::FIND_MATCHCASE;
		if (dialog.isPrev())
			nFlags |= TextWindow::FIND_PREVIOUS;
		
		status = pTextWindow_->find(dialog.getFind(), nFlags, &bFound);
		CHECK_QSTATUS();
	}
	else {
		const FindReplaceData* pData = pFindReplaceManager_->getData();
		assert(pData);
		
		unsigned int nFlags = 0;
		if (pData->getFlags() & FindReplaceData::FLAG_MATCHCASE)
			nFlags |= TextWindow::FIND_MATCHCASE;
		if (type_ == TYPE_PREV)
			nFlags |= TextWindow::FIND_PREVIOUS;
		
		status = pTextWindow_->find(pData->getFind(), nFlags, &bFound);
		CHECK_QSTATUS();
		
		if (bFound) {
			const WCHAR* pwszReplace = pData->getReplace();
			if (pwszReplace) {
				status = pTextWindow_->insertText(pwszReplace, -1);
				CHECK_QSTATUS();
			}
		}
	}
	
	if (!bFound)
		messageBox(Application::getApplication().getResourceHandle(),
			IDS_FINDNOTFOUND, hwndFrame);
	
	return QSTATUS_SUCCESS;
}

QSTATUS qm::EditEditFindAction::isEnabled(
	const ActionEvent& event, bool* pbEnabled)
{
	assert(pbEnabled);
	if (type_ == TYPE_NORMAL)
		*pbEnabled = pTextWindow_->hasFocus();
	else
		*pbEnabled = pTextWindow_->hasFocus() &&
			pFindReplaceManager_->getData();
	return QSTATUS_SUCCESS;
}


/****************************************************************************
 *
 * EditEditMoveCaretAction
 *
 */

qm::EditEditMoveCaretAction::EditEditMoveCaretAction(TextWindow* pTextWindow,
	TextWindow::MoveCaret moveCaret, QSTATUS* pstatus) :
	pTextWindow_(pTextWindow),
	moveCaret_(moveCaret)
{
}

qm::EditEditMoveCaretAction::~EditEditMoveCaretAction()
{
}

QSTATUS qm::EditEditMoveCaretAction::invoke(const ActionEvent& event)
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
	
	return pTextWindow_->moveCaret(moveCaret_, 0, 0, false, select, true);
}

QSTATUS qm::EditEditMoveCaretAction::isEnabled(
	const ActionEvent& event, bool* pbEnabled)
{
	assert(pbEnabled);
	*pbEnabled = pTextWindow_->hasFocus();
	return QSTATUS_SUCCESS;
}


/****************************************************************************
 *
 * EditEditPasteWithQuoteAction
 *
 */

qm::EditEditPasteWithQuoteAction::EditEditPasteWithQuoteAction(
	TextWindow* pTextWindow, Profile* pProfile, QSTATUS* pstatus) :
	pTextWindow_(pTextWindow),
	pProfile_(pProfile)
{
}

qm::EditEditPasteWithQuoteAction::~EditEditPasteWithQuoteAction()
{
}

QSTATUS qm::EditEditPasteWithQuoteAction::invoke(const ActionEvent& event)
{
	DECLARE_QSTATUS();
	
	string_ptr<WSTRING> wstrText;
	status = Clipboard::getText(pTextWindow_->getHandle(), &wstrText);
	CHECK_QSTATUS();
	
	string_ptr<WSTRING> wstrQuote;
	status = pProfile_->getString(L"EditWindow", L"PasteQuote", L"> ", &wstrQuote);
	CHECK_QSTATUS();
	
	if (wstrText.get()) {
		StringBuffer<WSTRING> buf(&status);
		CHECK_QSTATUS();
		bool bNewLine = true;
		for (const WCHAR* p = wstrText.get(); *p; ++p) {
			if (bNewLine) {
				status = buf.append(wstrQuote.get());
				CHECK_QSTATUS();
			}
			bNewLine = *p == L'\n';
			status = buf.append(*p);
			CHECK_QSTATUS();
		}
		
		status = pTextWindow_->insertText(
			buf.getCharArray(), buf.getLength());
		CHECK_QSTATUS();
	}
	
	return QSTATUS_SUCCESS;
}

QSTATUS qm::EditEditPasteWithQuoteAction::isEnabled(
	const ActionEvent& event, bool* pbEnabled)
{
	assert(pbEnabled);
	
	DECLARE_QSTATUS();
	
	if (pTextWindow_->hasFocus()) {
		status = Clipboard::isFormatAvailable(Clipboard::CF_QSTEXT, pbEnabled);
		CHECK_QSTATUS();
	}
	else {
		*pbEnabled = false;
	}
	
	return QSTATUS_SUCCESS;
}


/****************************************************************************
 *
 * EditEditReplaceAction
 *
 */

qm::EditEditReplaceAction::EditEditReplaceAction(TextWindow* pTextWindow,
	Profile* pProfile, FindReplaceManager* pFindReplaceManager, QSTATUS* pstatus) :
	pTextWindow_(pTextWindow),
	pProfile_(pProfile),
	pFindReplaceManager_(pFindReplaceManager)
{
}

qm::EditEditReplaceAction::~EditEditReplaceAction()
{
}

QSTATUS qm::EditEditReplaceAction::invoke(const ActionEvent& event)
{
	DECLARE_QSTATUS();
	
	HWND hwndFrame = pTextWindow_->getParentFrame();
	
	ReplaceDialog dialog(pProfile_, &status);
	CHECK_QSTATUS();
	int nRet = 0;
	status = dialog.doModal(hwndFrame, 0, &nRet);
	CHECK_QSTATUS();
	if (nRet != IDOK)
		return QSTATUS_SUCCESS;
	
	status = pFindReplaceManager_->setData(
		dialog.getFind(), dialog.getReplace(),
		dialog.isMatchCase() ? FindReplaceData::FLAG_MATCHCASE : 0);
	CHECK_QSTATUS();
	
	ReplaceDialog::Type type = dialog.getType();
	unsigned int nFlags = 0;
	if (dialog.isMatchCase())
		nFlags |= TextWindow::FIND_MATCHCASE;
	if (type == ReplaceDialog::TYPE_PREV)
		nFlags |= TextWindow::FIND_PREVIOUS;
	
	if (type == ReplaceDialog::TYPE_ALL) {
		status = pTextWindow_->moveCaret(TextWindow::MOVECARET_DOCSTART,
			0, 0, false, TextWindow::SELECT_CLEAR, false);
		CHECK_QSTATUS();
		
		while (true) {
			bool bFound = false;
			status = pTextWindow_->find(dialog.getFind(), nFlags, &bFound);
			CHECK_QSTATUS();
			if (!bFound)
				break;
			status = pTextWindow_->insertText(dialog.getReplace(), -1);
			CHECK_QSTATUS();
		}
	}
	else {
		bool bFound = false;
		status = pTextWindow_->find(dialog.getFind(), nFlags, &bFound);
		CHECK_QSTATUS();
		if (bFound) {
			status = pTextWindow_->insertText(dialog.getReplace(), -1);
			CHECK_QSTATUS();
		}
		else {
			messageBox(Application::getApplication().getResourceHandle(),
				IDS_FINDNOTFOUND, hwndFrame);
		}
	}
	
	return QSTATUS_SUCCESS;
}

QSTATUS qm::EditEditReplaceAction::isEnabled(
	const ActionEvent& event, bool* pbEnabled)
{
	assert(pbEnabled);
	*pbEnabled = pTextWindow_->hasFocus();
	return QSTATUS_SUCCESS;
}


/****************************************************************************
 *
 * EditFileInsertAction
 *
 */

qm::EditFileInsertAction::EditFileInsertAction(
	TextWindow* pTextWindow, QSTATUS* pstatus) :
	pTextWindow_(pTextWindow)
{
}

qm::EditFileInsertAction::~EditFileInsertAction()
{
}

QSTATUS qm::EditFileInsertAction::invoke(const ActionEvent& event)
{
	DECLARE_QSTATUS();
	
	string_ptr<WSTRING> wstrFilter;
	status = loadString(Application::getApplication().getResourceHandle(),
		IDS_FILTER_INSERT, &wstrFilter);
	CHECK_QSTATUS();
	
	FileDialog dialog(true, wstrFilter.get(), 0, 0, 0,
		OFN_EXPLORER | OFN_HIDEREADONLY | OFN_LONGNAMES, &status);
	CHECK_QSTATUS();
	
	int nRet = IDCANCEL;
	status = dialog.doModal(pTextWindow_->getParentFrame(), 0, &nRet);
	CHECK_QSTATUS();
	if (nRet == IDOK) {
		FileInputStream stream(dialog.getPath(), &status);
		CHECK_QSTATUS();
		BufferedInputStream bufferedStream(&stream, false, &status);
		CHECK_QSTATUS();
		InputStreamReader reader(&bufferedStream, false, 0, &status);
		CHECK_QSTATUS();
		
		StringBuffer<WSTRING> buf(&status);
		CHECK_QSTATUS();
		WCHAR wsz[1024];
		while (true) {
			size_t nLen = 0;
			status = reader.read(wsz, countof(wsz), &nLen);
			CHECK_QSTATUS();
			if (nLen == -1)
				break;
			status = buf.append(wsz, nLen);
			CHECK_QSTATUS();
		}
		
		status = pTextWindow_->insertText(buf.getCharArray(), buf.getLength());
		CHECK_QSTATUS();
	}
	
	return QSTATUS_SUCCESS;
}

QSTATUS qm::EditFileInsertAction::isEnabled(
	const ActionEvent& event, bool* pbEnabled)
{
	assert(pbEnabled);
	*pbEnabled = pTextWindow_->hasFocus();
	return QSTATUS_SUCCESS;
}


/****************************************************************************
 *
 * EditFileOpenAction
 *
 */

qm::EditFileOpenAction::EditFileOpenAction(
	EditMessageHolder* pEditMessageHolder,
	HWND hwndFrame, QSTATUS* pstatus) :
	pEditMessageHolder_(pEditMessageHolder),
	hwndFrame_(hwndFrame)
{
}

qm::EditFileOpenAction::~EditFileOpenAction()
{
}

QSTATUS qm::EditFileOpenAction::invoke(const ActionEvent& event)
{
	DECLARE_QSTATUS();
	
	string_ptr<WSTRING> wstrFilter;
	status = loadString(Application::getApplication().getResourceHandle(),
		IDS_FILTER_OPEN, &wstrFilter);
	CHECK_QSTATUS();
	
	FileDialog dialog(true, wstrFilter.get(), 0, 0, 0,
		OFN_EXPLORER | OFN_HIDEREADONLY | OFN_LONGNAMES, &status);
	CHECK_QSTATUS();
	
	int nRet = IDCANCEL;
	status = dialog.doModal(hwndFrame_, 0, &nRet);
	CHECK_QSTATUS();
	if (nRet == IDOK) {
		FileInputStream stream(dialog.getPath(), &status);
		CHECK_QSTATUS();
		BufferedInputStream bufferedStream(&stream, false, &status);
		CHECK_QSTATUS();
		
		StringBuffer<STRING> buf(&status);
		CHECK_QSTATUS();
		unsigned char sz[1024];
		while (true) {
			size_t nLen = 0;
			status = bufferedStream.read(sz, countof(sz), &nLen);
			CHECK_QSTATUS();
			if (nLen == -1)
				break;
			status = buf.append(reinterpret_cast<CHAR*>(sz), nLen);
			CHECK_QSTATUS();
		}
		
		std::auto_ptr<Message> pMessage;
		status = newQsObject(buf.getCharArray(), buf.getLength(),
			Message::FLAG_NONE, &pMessage);
		CHECK_QSTATUS();
		
		EditMessage* pEditMessage = pEditMessageHolder_->getEditMessage();
		status = pEditMessage->setMessage(pMessage.get());
		CHECK_QSTATUS();
		pMessage.release();
	}
	
	return QSTATUS_SUCCESS;
}


/****************************************************************************
 *
 * EditFileSaveAction
 *
 */

qm::EditFileSaveAction::EditFileSaveAction(
	EditMessageHolder* pEditMessageHolder, HWND hwndFrame, QSTATUS* pstatus) :
	pEditMessageHolder_(pEditMessageHolder),
	hwndFrame_(hwndFrame)
{
}

qm::EditFileSaveAction::~EditFileSaveAction()
{
}

QSTATUS qm::EditFileSaveAction::invoke(const ActionEvent& event)
{
	DECLARE_QSTATUS();
	
	string_ptr<WSTRING> wstrFilter;
	status = loadString(Application::getApplication().getResourceHandle(),
		IDS_FILTER_SAVE, &wstrFilter);
	CHECK_QSTATUS();
	
	FileDialog dialog(true, wstrFilter.get(), 0, 0, 0,
		OFN_EXPLORER | OFN_HIDEREADONLY | OFN_LONGNAMES | OFN_OVERWRITEPROMPT,
		&status);
	CHECK_QSTATUS();
	
	int nRet = IDCANCEL;
	status = dialog.doModal(hwndFrame_, 0, &nRet);
	CHECK_QSTATUS();
	if (nRet == IDOK) {
		EditMessage* pEditMessage = pEditMessageHolder_->getEditMessage();
		Message* pMessage = 0;
		status = pEditMessage->getMessage(&pMessage);
		CHECK_QSTATUS();
		
		string_ptr<STRING> strMessage;
		status = pMessage->getContent(&strMessage);
		CHECK_QSTATUS();
		
		FileOutputStream stream(dialog.getPath(), &status);
		CHECK_QSTATUS();
		BufferedOutputStream bufferedStream(&stream, false, &status);
		CHECK_QSTATUS();
		status = bufferedStream.write(
			reinterpret_cast<unsigned char*>(strMessage.get()),
			strlen(strMessage.get()));
		CHECK_QSTATUS();
		status = bufferedStream.close();
		CHECK_QSTATUS();
	}
	
	return QSTATUS_SUCCESS;
}


/****************************************************************************
 *
 * EditFileSendAction
 *
 */

qm::EditFileSendAction::EditFileSendAction(bool bDraft,
	Document* pDocument, EditMessageHolder* pEditMessageHolder,
	EditFrameWindow* pEditFrameWindow, Profile* pProfile, QSTATUS* pstatus) :
	composer_(bDraft, pDocument, pProfile, pEditFrameWindow->getHandle(), 0),
	pEditMessageHolder_(pEditMessageHolder),
	pEditFrameWindow_(pEditFrameWindow),
	pDocument_(0),
	pSyncManager_(0),
	pSyncDialogManager_(0)
{
}

qm::EditFileSendAction::EditFileSendAction(Document* pDocument,
	EditMessageHolder* pEditMessageHolder, EditFrameWindow* pEditFrameWindow,
	qs::Profile* pProfile, SyncManager* pSyncManager,
	SyncDialogManager* pSyncDialogManager, qs::QSTATUS* pstatus) :
	composer_(false, pDocument, pProfile, pEditFrameWindow->getHandle(), 0),
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

QSTATUS qm::EditFileSendAction::invoke(const ActionEvent& event)
{
	DECLARE_QSTATUS();
	
	EditMessage* pEditMessage = pEditMessageHolder_->getEditMessage();
	Message* pMessage = 0;
	status = pEditMessage->getMessage(&pMessage);
	CHECK_QSTATUS();
	
	Account* pAccount = pEditMessage->getAccount();
	SubAccount* pSubAccount = pEditMessage->getSubAccount();
	
	unsigned int nFlags = (pEditMessage->isSign() ? MessageComposer::FLAG_SIGN : 0) |
		(pEditMessage->isEncrypt() ? MessageComposer::FLAG_ENCRYPT : 0);
	status = composer_.compose(pEditMessage->getAccount(),
		pEditMessage->getSubAccount(), pMessage, nFlags);
	CHECK_QSTATUS();
	
	if (pSyncManager_) {
		status = SyncUtil::send(pSyncManager_, pDocument_, pSyncDialogManager_,
			getMainWindow()->getHandle(), 0, pAccount, pSubAccount);
		CHECK_QSTATUS();
	}
	
	pEditFrameWindow_->close();
	
	return QSTATUS_SUCCESS;
}


/****************************************************************************
 *
 * EditFocusItemAction
 *
 */

qm::EditFocusItemAction::EditFocusItemAction(
	EditWindow* pEditWindow, QSTATUS* pstatus) :
	pEditWindow_(pEditWindow)
{
	*pstatus = QSTATUS_SUCCESS;
}

qm::EditFocusItemAction::~EditFocusItemAction()
{
}

QSTATUS qm::EditFocusItemAction::invoke(const ActionEvent& event)
{
	EditWindowItem* pItem = pEditWindow_->getItemByNumber(
		event.getId() - IDM_FOCUS_HEADEREDITITEM);
	return pItem ? pItem->setFocus() : QSTATUS_SUCCESS;
}


/****************************************************************************
 *
 * EditToolAddressBookAction
 *
 */

qm::EditToolAddressBookAction::EditToolAddressBookAction(
	EditMessageHolder* pEditMessageHolder, EditWindow* pEditWindow,
	AddressBook* pAddressBook, Profile* pProfile, QSTATUS* pstatus) :
	pEditMessageHolder_(pEditMessageHolder),
	pEditWindow_(pEditWindow),
	pAddressBook_(pAddressBook),
	pProfile_(pProfile)
{
}

qm::EditToolAddressBookAction::~EditToolAddressBookAction()
{
}

QSTATUS qm::EditToolAddressBookAction::invoke(const ActionEvent& event)
{
	DECLARE_QSTATUS();
	
	EditMessage* pEditMessage = pEditMessageHolder_->getEditMessage();
	status = pEditMessage->update();
	CHECK_QSTATUS();
	
	const WCHAR* pwszFields[] = {
		L"To",
		L"Cc",
		L"Bcc"
	};
	WSTRING wstrAddresses[countof(pwszFields)];
	for (int n = 0; n < countof(pwszFields); ++n) {
		status = pEditMessage->getField(pwszFields[n],
			EditMessage::FIELDTYPE_ADDRESSLIST, &wstrAddresses[n]);
		CHECK_QSTATUS();
	}
	const WCHAR* pwszAddresses[] = {
		wstrAddresses[0] ? wstrAddresses[0] : L"",
		wstrAddresses[1] ? wstrAddresses[1] : L"",
		wstrAddresses[2] ? wstrAddresses[2] : L"",
	};
	AddressBookDialog dialog(pAddressBook_, pProfile_, pwszAddresses, &status);
	CHECK_QSTATUS();
	int nRet = 0;
	status = dialog.doModal(pEditWindow_->getParentFrame(), 0, &nRet);
	if (nRet == IDOK) {
		struct Type
		{
			AddressBookDialog::Type dialogType_;
			const WCHAR* pwszField_;
		} types[] = {
			{ AddressBookDialog::TYPE_TO,	L"To"	},
			{ AddressBookDialog::TYPE_CC,	L"Cc"	},
			{ AddressBookDialog::TYPE_BCC,	L"Bcc"	}
		};
		for (int n = 0; n < countof(types); ++n) {
			StringBuffer<WSTRING> buf(&status);
			const AddressBookDialog::AddressList& l =
				dialog.getAddresses(types[n].dialogType_);
			AddressBookDialog::AddressList::const_iterator it = l.begin();
			while (it != l.end()) {
				if (buf.getLength() != 0) {
					status = buf.append(L", ");
					CHECK_QSTATUS();
				}
				status = buf.append(*it);
				CHECK_QSTATUS();
				++it;
			}
			
			status = pEditMessage->setField(types[n].pwszField_,
				buf.getCharArray(), EditMessage::FIELDTYPE_ADDRESSLIST);
			CHECK_QSTATUS();
		}
	}
	
	return QSTATUS_SUCCESS;
}

QSTATUS qm::EditToolAddressBookAction::isEnabled(
	const ActionEvent& event, bool* pbEnabled)
{
	assert(pbEnabled);
	*pbEnabled = !pEditWindow_->isHeaderEdit();
	return QSTATUS_SUCCESS;
}


/****************************************************************************
 *
 * EditToolAttachmentAction
 *
 */

qm::EditToolAttachmentAction::EditToolAttachmentAction(
	EditMessageHolder* pEditMessageHolder, HWND hwndFrame, QSTATUS* pstatus) :
	pEditMessageHolder_(pEditMessageHolder),
	hwndFrame_(hwndFrame)
{
}

qm::EditToolAttachmentAction::~EditToolAttachmentAction()
{
}

QSTATUS qm::EditToolAttachmentAction::invoke(const ActionEvent& event)
{
	DECLARE_QSTATUS();
	
	EditMessage* pEditMessage = pEditMessageHolder_->getEditMessage();
	
	EditMessage::AttachmentList l;
	EditMessage::AttachmentListFree free(l);
	status = pEditMessage->getAttachments(&l);
	CHECK_QSTATUS();
	
	AttachmentDialog dialog(l, &status);
	CHECK_QSTATUS();
	int nRet = 0;
	status = dialog.doModal(hwndFrame_, 0, &nRet);
	CHECK_QSTATUS();
	if (nRet == IDOK) {
		status = pEditMessage->setAttachments(l);
		CHECK_QSTATUS();
	}
	
	return QSTATUS_SUCCESS;
}


/****************************************************************************
 *
 * EditToolFlagAction
 *
 */

qm::EditToolFlagAction::EditToolFlagAction(EditMessageHolder* pEditMessageHolder,
	PFN_IS pfnIs, PFN_SET pfnSet, bool bEnabled, QSTATUS* pstatus) :
	pEditMessageHolder_(pEditMessageHolder),
	pfnIs_(pfnIs),
	pfnSet_(pfnSet),
	bEnabled_(bEnabled)
{
}

qm::EditToolFlagAction::~EditToolFlagAction()
{
}

QSTATUS qm::EditToolFlagAction::invoke(const ActionEvent& event)
{
	EditMessage* pEditMessage = pEditMessageHolder_->getEditMessage();
	(pEditMessage->*pfnSet_)(!(pEditMessage->*pfnIs_)());
	return QSTATUS_SUCCESS;
}

QSTATUS qm::EditToolFlagAction::isEnabled(
	const ActionEvent& event, bool* pbEnabled)
{
	assert(pbEnabled);
	*pbEnabled = bEnabled_;
	return QSTATUS_SUCCESS;
}

QSTATUS qm::EditToolFlagAction::isChecked(
	const ActionEvent& event, bool* pbChecked)
{
	assert(pbChecked);
	EditMessage* pEditMessage = pEditMessageHolder_->getEditMessage();
	*pbChecked = (pEditMessage->*pfnIs_)();
	return QSTATUS_SUCCESS;
}


/****************************************************************************
 *
 * EditToolInsertSignatureAction
 *
 */

qm::EditToolInsertSignatureAction::EditToolInsertSignatureAction(
	EditMessageHolder* pEditMessageHolder,
	TextWindow* pTextWindow, QSTATUS* pstatus) :
	pEditMessageHolder_(pEditMessageHolder),
	pTextWindow_(pTextWindow)
{
}

qm::EditToolInsertSignatureAction::~EditToolInsertSignatureAction()
{
}

QSTATUS qm::EditToolInsertSignatureAction::invoke(const ActionEvent& event)
{
	DECLARE_QSTATUS();
	
	EditMessage* pEditMessage = pEditMessageHolder_->getEditMessage();
	string_ptr<WSTRING> wstrSignature;
	status = pEditMessage->getSignatureText(&wstrSignature);
	CHECK_QSTATUS();
	if (wstrSignature.get()) {
		status = pTextWindow_->insertText(wstrSignature.get(), -1);
		CHECK_QSTATUS();
		status = pEditMessage->setSignature(0);
		CHECK_QSTATUS();
	}
	
	return QSTATUS_SUCCESS;
}

QSTATUS qm::EditToolInsertSignatureAction::isEnabled(
	const ActionEvent& event, bool* pbEnabled)
{
	assert(pbEnabled);
	*pbEnabled = pTextWindow_->hasFocus() &&
		pEditMessageHolder_->getEditMessage()->getSignature();
	return QSTATUS_SUCCESS;
}


/****************************************************************************
 *
 * EditToolInsertTextAction
 *
 */

qm::EditToolInsertTextAction::EditToolInsertTextAction(
	TextWindow* pTextWindow, QSTATUS* pstatus) :
	pTextWindow_(pTextWindow)
{
}

qm::EditToolInsertTextAction::~EditToolInsertTextAction()
{
}

QSTATUS qm::EditToolInsertTextAction::invoke(const ActionEvent& event)
{
	DECLARE_QSTATUS();
	
	InsertTextDialog dialog(&status);
	CHECK_QSTATUS();
	int nRet = 0;
	status = dialog.doModal(pTextWindow_->getParentFrame(), 0, &nRet);
	if (nRet == IDOK) {
		const FixedFormText* pText = dialog.getText();
		assert(pText);
		status = pTextWindow_->insertText(pText->getText(), -1);
		CHECK_QSTATUS();
	}
	
	return QSTATUS_SUCCESS;
}

QSTATUS qm::EditToolInsertTextAction::isEnabled(
	const ActionEvent& event, bool* pbEnabled)
{
	assert(pbEnabled);
	*pbEnabled = pTextWindow_->hasFocus();
	return QSTATUS_SUCCESS;
}


/****************************************************************************
 *
 * EditToolHeaderEditAction
 *
 */

qm::EditToolHeaderEditAction::EditToolHeaderEditAction(
	EditWindow* pEditWindow, QSTATUS* pstatus) :
	pEditWindow_(pEditWindow)
{
}

qm::EditToolHeaderEditAction::~EditToolHeaderEditAction()
{
}

QSTATUS qm::EditToolHeaderEditAction::invoke(const ActionEvent& event)
{
	return pEditWindow_->setHeaderEdit(!pEditWindow_->isHeaderEdit());
}

QSTATUS qm::EditToolHeaderEditAction::isChecked(
	const ActionEvent& event, bool* pbChecked)
{
	assert(pbChecked);
	*pbChecked = pEditWindow_->isHeaderEdit();
	return QSTATUS_SUCCESS;
}


/****************************************************************************
 *
 * EditToolReformAction
 *
 */

qm::EditToolReformAction::EditToolReformAction(
	TextWindow* pTextWindow, QSTATUS* pstatus) :
	pTextWindow_(pTextWindow)
{
}

qm::EditToolReformAction::~EditToolReformAction()
{
}

QSTATUS qm::EditToolReformAction::invoke(const ActionEvent& event)
{
	return pTextWindow_->reform();
}

QSTATUS qm::EditToolReformAction::isEnabled(
	const ActionEvent& event, bool* pbEnabled)
{
	assert(pbEnabled);
	*pbEnabled = pTextWindow_->hasFocus();
	return QSTATUS_SUCCESS;
}


/****************************************************************************
 *
 * EditToolReformAllAction
 *
 */

qm::EditToolReformAllAction::EditToolReformAllAction(
	TextWindow* pTextWindow, Profile* pProfile, QSTATUS* pstatus) :
	pTextWindow_(pTextWindow),
	pProfile_(pProfile)
{
}

qm::EditToolReformAllAction::~EditToolReformAllAction()
{
}

QSTATUS qm::EditToolReformAllAction::invoke(const ActionEvent& event)
{
	DECLARE_QSTATUS();
	
	EditableTextModel* pTextModel =
		static_cast<EditableTextModel*>(pTextWindow_->getTextModel());
	string_ptr<WSTRING> wstrText;
	status = pTextModel->getText(&wstrText);
	CHECK_QSTATUS();
	
	// TODO
	// Get line length and tab width
	string_ptr<WSTRING> wstrReformedText;
	status = TextUtil::fold(wstrText.get(), -1, 74, 0, 0, 4, &wstrReformedText);
	CHECK_QSTATUS();
	status = pTextModel->setText(wstrReformedText.get(), -1);
	CHECK_QSTATUS();
	
	return QSTATUS_SUCCESS;
}

QSTATUS qm::EditToolReformAllAction::isEnabled(
	const ActionEvent& event, bool* pbEnabled)
{
	assert(pbEnabled);
	*pbEnabled = pTextWindow_->hasFocus();
	return QSTATUS_SUCCESS;
}
