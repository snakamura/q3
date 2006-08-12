/*
 * $Id$
 *
 * Copyright(C) 1998-2006 Satoshi Nakamura
 * All rights reserved.
 *
 */

#pragma warning(disable:4786)

#include <qmaccount.h>
#include <qmaddressbookwindow.h>
#include <qmapplication.h>
#include <qmdocument.h>
#include <qmfolder.h>
#include <qmfoldercombobox.h>
#include <qmfolderlistwindow.h>
#include <qmfolderwindow.h>
#include <qmgoround.h>
#include <qmlistwindow.h>
#include <qmmainwindow.h>
#include <qmmessage.h>
#include <qmmessageholder.h>
#include <qmmessagewindow.h>
#include <qmrecents.h>
#include <qmscript.h>
#include <qmsearch.h>
#include <qmtabwindow.h>
#include <qmtemplate.h>

#include <qsconv.h>
#include <qsfile.h>
#include <qsinit.h>
#include <qsstl.h>
#include <qsstream.h>
#include <qsuiutil.h>
#include <qswindow.h>

#include <algorithm>

#include <boost/bind.hpp>

#ifndef _WIN32_WCE
#	include <shlwapi.h>
#endif
#include <tchar.h>

#include "action.h"
#include "findreplace.h"
#include "../junk/junk.h"
#include "../main/updatechecker.h"
#include "../model/dataobject.h"
#include "../model/filter.h"
#include "../model/goround.h"
#include "../model/rule.h"
#include "../model/tempfilecleaner.h"
#include "../model/templatemanager.h"
#include "../model/undo.h"
#include "../model/uri.h"
#include "../script/scriptmanager.h"
#include "../sync/autopilot.h"
#include "../sync/syncmanager.h"
#include "../ui/accountdialog.h"
#include "../ui/addressbookdialog.h"
#include "../ui/addressbookwindow.h"
#include "../ui/dialogs.h"
#include "../ui/editframewindow.h"
#include "../ui/folderdialog.h"
#include "../ui/messageframewindow.h"
#include "../ui/optiondialog.h"
#include "../ui/propertypages.h"
#include "../ui/resourceinc.h"
#include "../ui/syncdialog.h"
#include "../ui/syncutil.h"
#include "../ui/uiutil.h"
#include "../uimodel/attachmentselectionmodel.h"
#include "../uimodel/encodingmodel.h"
#include "../uimodel/foldermodel.h"
#include "../uimodel/folderselectionmodel.h"
#include "../uimodel/messagemodel.h"
#include "../uimodel/messageselectionmodel.h"
#include "../uimodel/securitymodel.h"
#include "../uimodel/tabmodel.h"
#include "../uimodel/viewmodel.h"
#include "../util/util.h"

using namespace qm;
using namespace qs;


/****************************************************************************
 *
 * AttachmentOpenAction
 *
 */

qm::AttachmentOpenAction::AttachmentOpenAction(MessageModel* pMessageModel,
											   AttachmentSelectionModel* pAttachmentSelectionModel,
											   SecurityModel* pSecurityModel,
											   Profile* pProfile,
											   TempFileCleaner* pTempFileCleaner,
											   HWND hwnd) :
	pMessageModel_(pMessageModel),
	pAttachmentSelectionModel_(pAttachmentSelectionModel),
	pSecurityModel_(pSecurityModel),
	hwnd_(hwnd),
	helper_(pSecurityModel, pProfile, pTempFileCleaner, hwnd)
{
}

qm::AttachmentOpenAction::~AttachmentOpenAction()
{
}

void qm::AttachmentOpenAction::invoke(const ActionEvent& event)
{
	MessagePtrLock mpl(pMessageModel_->getCurrentMessage());
	if (!mpl)
		return;
	
	Message msg;
	if (!mpl->getMessage(Account::GETMESSAGEFLAG_ALL,
		0, pSecurityModel_->getSecurityMode(), &msg)) {
		ActionUtil::error(hwnd_, IDS_ERROR_EXECUTEATTACHMENT);
		return;
	}
	
	AttachmentParser parser(msg);
	AttachmentParser::AttachmentList listAttachment;
	AttachmentParser::AttachmentListFree freeAttachment(listAttachment);
	parser.getAttachments(false, &listAttachment);
	if (listAttachment.empty())
		return;
	
	AttachmentSelectionModel::NameList listName;
	StringListFree<AttachmentSelectionModel::NameList> freeName(listName);
	pAttachmentSelectionModel_->getSelectedAttachment(&listName);
	for (AttachmentSelectionModel::NameList::const_iterator itN = listName.begin(); itN != listName.end(); ++itN) {
		AttachmentParser::AttachmentList::const_iterator itA = std::find_if(
			listAttachment.begin(), listAttachment.end(),
			std::bind2nd(
				binary_compose_f_gx_hy(
					string_equal<WCHAR>(),
					std::select1st<AttachmentParser::AttachmentList::value_type>(),
					std::identity<const WCHAR*>()),
				*itN));
		if (itA != listAttachment.end()) {
			bool bExternalEditor = (event.getModifier() & ActionEvent::MODIFIER_SHIFT) != 0;
			if (helper_.open((*itA).second, *itN, bExternalEditor) == AttachmentParser::RESULT_FAIL) {
				ActionUtil::error(hwnd_, IDS_ERROR_EXECUTEATTACHMENT);
				return;
			}
		}
	}
}

bool qm::AttachmentOpenAction::isEnabled(const ActionEvent& event)
{
	return !pAttachmentSelectionModel_->isAttachmentDeleted() &&
		pAttachmentSelectionModel_->hasSelectedAttachment();
}


/****************************************************************************
 *
 * AttachmentSaveAction
 *
 */

qm::AttachmentSaveAction::AttachmentSaveAction(MessageModel* pMessageModel,
											   AttachmentSelectionModel* pAttachmentSelectionModel,
											   SecurityModel* pSecurityModel,
											   bool bAll,
											   Profile* pProfile,
											   HWND hwnd) :
	pMessageModel_(pMessageModel),
	pAttachmentSelectionModel_(pAttachmentSelectionModel),
	bAll_(bAll),
	helper_(pSecurityModel, pProfile, 0, hwnd),
	hwnd_(hwnd)
{
}

qm::AttachmentSaveAction::~AttachmentSaveAction()
{
}

void qm::AttachmentSaveAction::invoke(const ActionEvent& event)
{
	MessagePtrLock mpl(pMessageModel_->getCurrentMessage());
	if (!mpl)
		return;
	
	MessageHolderList listMessageHolder(1, mpl);
	
	if (bAll_) {
		if (helper_.detach(listMessageHolder, 0) == AttachmentParser::RESULT_FAIL) {
			ActionUtil::error(hwnd_, IDS_ERROR_DETACHATTACHMENT);
			return;
		}
	}
	else {
		AttachmentSelectionModel::NameList listName;
		StringListFree<AttachmentSelectionModel::NameList> freeName(listName);
		pAttachmentSelectionModel_->getSelectedAttachment(&listName);
		
		AttachmentHelper::NameList l(listName.begin(), listName.end());
		if (helper_.detach(listMessageHolder, &l) == AttachmentParser::RESULT_FAIL) {
			ActionUtil::error(hwnd_, IDS_ERROR_DETACHATTACHMENT);
			return;
		}
	}
}

bool qm::AttachmentSaveAction::isEnabled(const ActionEvent& event)
{
	if (pAttachmentSelectionModel_->isAttachmentDeleted())
		return false;
	return bAll_ ? pAttachmentSelectionModel_->hasAttachment() :
		pAttachmentSelectionModel_->hasSelectedAttachment();
}


/****************************************************************************
 *
 * ConfigViewsAction
 *
 */

qm::ConfigViewsAction::ConfigViewsAction(ViewModelManager* pViewModelManager,
										 HWND hwnd) :
	pViewModelManager_(pViewModelManager),
	hwnd_(hwnd)
{
}

qm::ConfigViewsAction::~ConfigViewsAction()
{
}

void qm::ConfigViewsAction::invoke(const ActionEvent& event)
{
	ViewModel* pViewModel = pViewModelManager_->getCurrentViewModel();
	if (!pViewModel)
		return;
	
	ViewsDialog dialog(pViewModelManager_, pViewModel);
	dialog.doModal(hwnd_);
}

bool qm::ConfigViewsAction::isEnabled(const ActionEvent& event)
{
	return pViewModelManager_->getCurrentViewModel() != 0;
}


/****************************************************************************
 *
 * DispatchAction
 *
 */

qm::DispatchAction::DispatchAction(View* pViews[],
								   Action* pActions[],
								   size_t nCount)
{
	listItem_.resize(nCount);
	for (size_t n = 0; n < nCount; ++n) {
		listItem_[n].pView_ = *(pViews + n);
		listItem_[n].pAction_ = *(pActions + n);
	}
}

qm::DispatchAction::~DispatchAction()
{
	std::sort(listItem_.begin(), listItem_.end(),
		binary_compose_f_gx_hy(
			std::less<Action*>(),
			mem_data_ref(&Item::pAction_),
			mem_data_ref(&Item::pAction_)));
	ItemList::iterator it = std::unique(
		listItem_.begin(), listItem_.end(),
		binary_compose_f_gx_hy(
			std::equal_to<Action*>(),
			mem_data_ref(&Item::pAction_),
			mem_data_ref(&Item::pAction_)));
	std::for_each(listItem_.begin(), it,
		unary_compose_f_gx(
			qs::deleter<Action>(),
			mem_data_ref(&Item::pAction_)));
}

void qm::DispatchAction::invoke(const ActionEvent& event)
{
	Action* pAction = getAction();
	if (pAction)
		pAction->invoke(event);
}

bool qm::DispatchAction::isEnabled(const ActionEvent& event)
{
	Action* pAction = getAction();
	return pAction ? pAction->isEnabled(event) : false;
}

bool qm::DispatchAction::isChecked(const ActionEvent& event)
{
	Action* pAction = getAction();
	return pAction ? pAction->isChecked(event) : false;
}

wstring_ptr qm::DispatchAction::getText(const ActionEvent& event)
{
	Action* pAction = getAction();
	return pAction ? pAction->getText(event) : 0;
}

Action* qm::DispatchAction::getAction() const
{
	for (ItemList::const_iterator it = listItem_.begin(); it != listItem_.end(); ++it) {
		if ((*it).pView_->isActive())
			return (*it).pAction_;
	}
	return 0;
}


/****************************************************************************
 *
 * EditClearDeletedAction
 *
 */

qm::EditClearDeletedAction::EditClearDeletedAction(SyncManager* pSyncManager,
												   Document* pDocument,
												   FolderModel* pFolderModel,
												   SyncDialogManager* pSyncDialogManager,
												   HWND hwnd,
												   Profile* pProfile) :
	pSyncManager_(pSyncManager),
	pDocument_(pDocument),
	pFolderModel_(pFolderModel),
	pSyncDialogManager_(pSyncDialogManager),
	hwnd_(hwnd),
	pProfile_(pProfile)
{
}

qm::EditClearDeletedAction::~EditClearDeletedAction()
{
}

void qm::EditClearDeletedAction::invoke(const ActionEvent& event)
{
	Folder* pFolder = FolderActionUtil::getFolder(pFolderModel_);
	if (!pFolder)
		return;
	
	Account::NormalFolderList l;
	switch (pFolder->getType()) {
	case Folder::TYPE_NORMAL:
		if (pFolder->isFlag(Folder::FLAG_NOSELECT) ||
			pFolder->isFlag(Folder::FLAG_LOCAL))
			return;
		l.push_back(static_cast<NormalFolder*>(pFolder));
		break;
	case Folder::TYPE_QUERY:
		{
			QueryFolder* pQueryFolder = static_cast<QueryFolder*>(pFolder);
			pFolder->getAccount()->getNormalFolders(pQueryFolder->getTargetFolder(),
				pQueryFolder->isRecursive(), &l);
		}
		break;
	default:
		assert(false);
		break;
	}
	
	if (!l.empty()) {
		if (!SyncUtil::syncFolders(pSyncManager_, pDocument_, pSyncDialogManager_,
			SyncData::TYPE_ACTIVE, l, ReceiveSyncItem::FLAG_EXPUNGE)) {
			ActionUtil::error(hwnd_, IDS_ERROR_CLEARDELETED);
			return;
		}
	}
}

bool qm::EditClearDeletedAction::isEnabled(const ActionEvent& event)
{
	Folder* pFolder = FolderActionUtil::getFolder(pFolderModel_);
	if (!pFolder)
		return false;
	switch (pFolder->getType()) {
	case Folder::TYPE_NORMAL:
		return !pFolder->isFlag(Folder::FLAG_NOSELECT) &&
			!pFolder->isFlag(Folder::FLAG_LOCAL);
	case Folder::TYPE_QUERY:
		return true;
	default:
		assert(false);
		return false;
	}
}


/****************************************************************************
 *
 * EditCommandAction
 *
 */

qm::EditCommandAction::EditCommandAction(MessageWindow* pMessageWindow,
										 PFN_DO pfnDo,
										 PFN_CANDO pfnCanDo) :
	pMessageWindow_(pMessageWindow),
	pfnDo_(pfnDo),
	pfnCanDo_(pfnCanDo)
{
}

qm::EditCommandAction::~EditCommandAction()
{
}

void qm::EditCommandAction::invoke(const ActionEvent& event)
{
	MessageWindowItem* pItem = pMessageWindow_->getFocusedItem();
	if (pItem)
		(pItem->*pfnDo_)();
}

bool qm::EditCommandAction::isEnabled(const ActionEvent& event)
{
	MessageWindowItem* pItem = pMessageWindow_->getFocusedItem();
	return pItem ? (pItem->*pfnCanDo_)() : false;
}


/****************************************************************************
 *
 * EditCopyMessageAction
 *
 */

qm::EditCopyMessageAction::EditCopyMessageAction(AccountManager* pAccountManager,
												 FolderModel* pFolderModel,
												 MessageSelectionModel* pMessageSelectionModel,
												 HWND hwnd) :
	pAccountManager_(pAccountManager),
	pFolderModel_(pFolderModel),
	pMessageSelectionModel_(pMessageSelectionModel),
	hwnd_(hwnd)
{
}

qm::EditCopyMessageAction::~EditCopyMessageAction()
{
}

void qm::EditCopyMessageAction::invoke(const ActionEvent& event)
{
	AccountLock lock;
	Folder* pFolder = 0;
	MessageHolderList l;
	pMessageSelectionModel_->getSelectedMessages(&lock, &pFolder, &l);
	
	if (!l.empty()) {
		MessageDataObject* p = new MessageDataObject(pAccountManager_,
			pFolder, l, MessageDataObject::FLAG_COPY);
		p->AddRef();
		ComPtr<IDataObject> pDataObject(p);
		if (!MessageDataObject::setClipboard(pDataObject.get())) {
			ActionUtil::error(hwnd_, IDS_ERROR_COPYMESSAGES);
			return;
		}
	}
}

bool qm::EditCopyMessageAction::isEnabled(const ActionEvent& event)
{
	return pMessageSelectionModel_->hasSelectedMessage();
}


/****************************************************************************
 *
 * EditCutMessageAction
 *
 */

qm::EditCutMessageAction::EditCutMessageAction(AccountManager* pAccountManager,
											   FolderModel* pFolderModel,
											   MessageSelectionModel* pMessageSelectionModel,
											   HWND hwnd) :
	pAccountManager_(pAccountManager),
	pFolderModel_(pFolderModel),
	pMessageSelectionModel_(pMessageSelectionModel),
	hwnd_(hwnd)
{
}

qm::EditCutMessageAction::~EditCutMessageAction()
{
}

void qm::EditCutMessageAction::invoke(const ActionEvent& event)
{
	AccountLock lock;
	Folder* pFolder = 0;
	MessageHolderList l;
	pMessageSelectionModel_->getSelectedMessages(&lock, &pFolder, &l);
	
	if (!l.empty()) {
		MessageDataObject* p = new MessageDataObject(pAccountManager_,
			pFolder, l, MessageDataObject::FLAG_MOVE);
		p->AddRef();
		ComPtr<IDataObject> pDataObject(p);
		if (!MessageDataObject::setClipboard(pDataObject.get())) {
			ActionUtil::error(hwnd_, IDS_ERROR_CUTMESSAGES);
			return;
		}
	}
}

bool qm::EditCutMessageAction::isEnabled(const ActionEvent& event)
{
	return pMessageSelectionModel_->hasSelectedMessage();
}


/****************************************************************************
 *
 * EditDeleteCacheAction
 *
 */

qm::EditDeleteCacheAction::EditDeleteCacheAction(MessageSelectionModel* pMessageSelectionModel,
												 HWND hwnd) :
	pMessageSelectionModel_(pMessageSelectionModel),
	hwnd_(hwnd)
{
}

qm::EditDeleteCacheAction::~EditDeleteCacheAction()
{
}

void qm::EditDeleteCacheAction::invoke(const ActionEvent& event)
{
	AccountLock lock;
	MessageHolderList l;
	pMessageSelectionModel_->getSelectedMessages(&lock, 0, &l);
	
	Account* pAccount = lock.get();
	if (!l.empty()) {
		if (!pAccount->deleteMessagesCache(l)) {
			ActionUtil::error(hwnd_, IDS_ERROR_DELETECACHE);
			return;
		}
	}
}

bool qm::EditDeleteCacheAction::isEnabled(const ActionEvent& event)
{
	return pMessageSelectionModel_->hasSelectedMessage();
}


/****************************************************************************
 *
 * EditDeleteMessageAction
 *
 */

qm::EditDeleteMessageAction::EditDeleteMessageAction(MessageSelectionModel* pMessageSelectionModel,
													 ViewModelHolder* pViewModelHolder,
													 MessageModel* pMessageModel,
													 Type type,
													 bool bDontSelectNextIfDeletedFlag,
													 UndoManager* pUndoManager,
													 HWND hwnd,
													 Profile* pProfile) :
	pMessageSelectionModel_(pMessageSelectionModel),
	pViewModelHolder_(pViewModelHolder),
	pMessageModel_(pMessageModel),
	type_(type),
	bDontSelectNextIfDeletedFlag_(bDontSelectNextIfDeletedFlag),
	pUndoManager_(pUndoManager),
	hwnd_(hwnd),
	pProfile_(pProfile)
{
}

qm::EditDeleteMessageAction::~EditDeleteMessageAction()
{
}

void qm::EditDeleteMessageAction::invoke(const ActionEvent& event)
{
	ViewModel* pViewModel = pViewModelHolder_->getViewModel();
	assert(pViewModel);
	Lock<ViewModel> lockViewModel(*pViewModel);
	
	AccountLock lock;
	Folder* pFolder = 0;
	MessageHolderList l;
	pMessageSelectionModel_->getSelectedMessages(&lock, &pFolder, &l);
	
	if (l.empty())
		return;
	
	if (!confirm())
		return;
	
	Account* pAccount = lock.get();
	
	bool bSelectNext = !bDontSelectNextIfDeletedFlag_ ||
		!pAccount->isSupport(Account::SUPPORT_DELETEDMESSAGE) ||
		pFolder->getType() != Folder::TYPE_NORMAL ||
		pFolder->isFlag(Folder::FLAG_LOCAL);
	if (bSelectNext) {
		unsigned int nIndex = l.size() == 1 ?
			pViewModel->getIndex(l.front()) : pViewModel->getFocused();
		if (nIndex < pViewModel->getCount() - 1)
			MessageActionUtil::select(pViewModel, nIndex + 1, pMessageModel_);
		else if (nIndex != 0)
			MessageActionUtil::select(pViewModel, nIndex - 1, pMessageModel_);
	}
	
	if (!deleteMessages(l, pFolder))
		ActionUtil::error(hwnd_, IDS_ERROR_DELETEMESSAGES);
}

bool qm::EditDeleteMessageAction::isEnabled(const ActionEvent& event)
{
	if (!pMessageSelectionModel_->hasSelectedMessage())
		return false;
	
	if (type_ == TYPE_JUNK) {
		AccountLock lock;
		pMessageSelectionModel_->getSelectedMessages(&lock, 0, 0);
		Account* pAccount = lock.get();
		if (!pAccount->getFolderByBoxFlag(Folder::FLAG_JUNKBOX))
			return false;
	}
	
	return true;
}

bool qm::EditDeleteMessageAction::deleteMessages(const MessageHolderList& l,
												 Folder* pFolder) const
{
	Account* pAccount = pFolder->getAccount();
	
	ProgressDialogMessageOperationCallback callback(hwnd_,
		IDS_PROGRESS_DELETE, IDS_PROGRESS_DELETE);
	UndoItemList undo;
	if (type_ != TYPE_JUNK) {
		unsigned int nRemoveFlags = Account::OPFLAG_ACTIVE |
			(type_ == TYPE_DIRECT ? Account::REMOVEFLAG_DIRECT : Account::REMOVEFLAG_NONE);
		if (!pAccount->removeMessages(l, pFolder, nRemoveFlags, &callback, &undo, 0))
			return false;
	}
	else {
		NormalFolder* pJunk = static_cast<NormalFolder*>(
			pAccount->getFolderByBoxFlag(Folder::FLAG_JUNKBOX));
		if (pJunk) {
			unsigned int nFlags = Account::OPFLAG_ACTIVE |
				Account::COPYFLAG_MOVE | Account::COPYFLAG_MANAGEJUNK;
			if (!pAccount->copyMessages(l, pFolder, pJunk, nFlags, &callback, &undo, 0))
				return false;
		}
	}
	pUndoManager_->pushUndoItem(undo.getUndoItem());
	
	return true;
}

bool qm::EditDeleteMessageAction::confirm() const
{
	if (pProfile_->getInt(L"Global", L"ConfirmDeleteMessage"))
		return messageBox(Application::getApplication().getResourceHandle(),
			IDS_CONFIRM_DELETEMESSAGE, MB_YESNO | MB_DEFBUTTON2 | MB_ICONQUESTION, hwnd_) == IDYES;
	else
		return true;
}


/****************************************************************************
 *
 * EditFindAction
 *
 */

qm::EditFindAction::EditFindAction(MessageWindow* pMessageWindow,
								   Profile* pProfile,
								   FindReplaceManager* pFindReplaceManager) :
	pMessageWindow_(pMessageWindow),
	pProfile_(pProfile),
	pFindReplaceManager_(pFindReplaceManager),
	type_(TYPE_NORMAL)
{
}

qm::EditFindAction::EditFindAction(MessageWindow* pMessageWindow,
								   bool bNext,
								   FindReplaceManager* pFindReplaceManager) :
	pMessageWindow_(pMessageWindow),
	pProfile_(0),
	pFindReplaceManager_(pFindReplaceManager),
	type_(bNext ? TYPE_NEXT : TYPE_PREV)
{
}

qm::EditFindAction::~EditFindAction()
{
}

void qm::EditFindAction::invoke(const ActionEvent& event)
{
	unsigned int nSupportedFlags = pMessageWindow_->getSupportedFindFlags();
	if (nSupportedFlags == -1)
		return;
	
	HWND hwndFrame = pMessageWindow_->getParentFrame();
	
	bool bFound = false;
	if (type_ == TYPE_NORMAL) {
		struct CallbackImpl : public FindDialog::Callback
		{
			CallbackImpl(MessageWindow* pMessageWindow,
						 bool& bFound) :
				pMessageWindow_(pMessageWindow),
				bFound_(bFound),
				bSearched_(false)
			{
				pMark_ = pMessageWindow_->mark();
			}
			
			virtual void statusChanged(const WCHAR* pwszFind,
									   bool bMatchCase,
									   bool bRegex)
			{
				if (bSearched_)
					pMessageWindow_->reset(*pMark_.get());
				
				unsigned int nFlags =
					(bMatchCase ? MessageWindow::FIND_MATCHCASE : 0) |
					(bRegex ? MessageWindow::FIND_REGEX : 0);
				bFound_ = pMessageWindow_->find(pwszFind, nFlags);
				
				bSearched_ = true;
			}
			
			MessageWindow* pMessageWindow_;
			bool& bFound_;
			std::auto_ptr<MessageWindow::Mark> pMark_;
			bool bSearched_;
		} callback(pMessageWindow_, bFound);
		
		bool bIncremental = nSupportedFlags & MessageWindow::FIND_INCREMENTAL &&
			pProfile_->getInt(L"Global", L"IncrementalSearch") != 0;
		bool bSupportRegex = (nSupportedFlags & MessageWindow::FIND_REGEX) != 0;
		FindDialog dialog(pProfile_, bSupportRegex, bIncremental ? &callback: 0);
		if (dialog.doModal(hwndFrame) != IDOK)
			return;
		
		pFindReplaceManager_->setData(dialog.getFind(),
			(dialog.isMatchCase() ? FindReplaceData::FLAG_MATCHCASE : 0) |
			(dialog.isRegex() ? FindReplaceData::FLAG_REGEX : 0));
		
		if (!bIncremental || !callback.bSearched_) {
			unsigned int nFlags =
				(dialog.isMatchCase() ? MessageWindow::FIND_MATCHCASE : 0) |
				(dialog.isRegex() ? MessageWindow::FIND_REGEX : 0) |
				(dialog.isPrev() ? MessageWindow::FIND_PREVIOUS : 0);
			bFound = pMessageWindow_->find(dialog.getFind(), nFlags);
		}
	}
	else {
		const FindReplaceData* pData = pFindReplaceManager_->getData();
		assert(pData);
		unsigned int nFlags =
			(pData->getFlags() & FindReplaceData::FLAG_MATCHCASE ? MessageWindow::FIND_MATCHCASE : 0) |
			(pData->getFlags() & FindReplaceData::FLAG_REGEX ? MessageWindow::FIND_REGEX : 0) |
			(type_ == TYPE_PREV ? MessageWindow::FIND_PREVIOUS : 0);
		bFound = pMessageWindow_->find(pData->getFind(), nFlags);
	}
	
	if (!bFound)
		ActionUtil::info(hwndFrame, IDS_MESSAGE_FINDNOTFOUND);
}

bool qm::EditFindAction::isEnabled(const ActionEvent& event)
{
	if (!pMessageWindow_->isActive())
		return false;
	else if (type_ == TYPE_NORMAL && pMessageWindow_->getSupportedFindFlags() == -1)
		return false;
	else if (type_ != TYPE_NORMAL && !pFindReplaceManager_->getData())
		return false;
	else
		return true;
}


/****************************************************************************
 *
 * EditPasteMessageAction
 *
 */

qm::EditPasteMessageAction::EditPasteMessageAction(Document* pDocument,
												   FolderModel* pFolderModel,
												   SyncManager* pSyncManager,
												   SyncDialogManager* pSyncDialogManager,
												   Profile* pProfile,
												   HWND hwnd) :
	pDocument_(pDocument),
	pFolderModel_(pFolderModel),
	pSyncManager_(pSyncManager),
	pSyncDialogManager_(pSyncDialogManager),
	pProfile_(pProfile),
	hwnd_(hwnd)
{
}

qm::EditPasteMessageAction::~EditPasteMessageAction()
{
}

void qm::EditPasteMessageAction::invoke(const ActionEvent& event)
{
	Folder* pFolder = FolderActionUtil::getFolder(pFolderModel_);
	if (!pFolder || pFolder->getType() != Folder::TYPE_NORMAL ||
		pFolder->isFlag(Folder::FLAG_NOSELECT))
		return;
	
	NormalFolder* pNormalFolder = static_cast<NormalFolder*>(pFolder);
	if (!pasteMessages(pNormalFolder)) {
		ActionUtil::error(hwnd_, IDS_ERROR_PASTEMESSAGES);
		return;
	}
	
#ifdef _WIN32_WCE
	Clipboard clipboard(0);
	clipboard.empty();
#else
	::OleSetClipboard(0);
#endif
}

bool qm::EditPasteMessageAction::isEnabled(const ActionEvent& event)
{
	Folder* pFolder = FolderActionUtil::getFolder(pFolderModel_);
	if (!pFolder || pFolder->getType() != Folder::TYPE_NORMAL ||
		pFolder->isFlag(Folder::FLAG_NOSELECT))
		return false;
	return MessageDataObject::queryClipboard();
}

bool qm::EditPasteMessageAction::pasteMessages(NormalFolder* pFolder) const
{
	ComPtr<IDataObject> pDataObject(MessageDataObject::getClipboard(pDocument_));
	
	MessageDataObject::Flag flag = MessageDataObject::getPasteFlag(
		pDataObject.get(), pDocument_, pFolder);
	UINT nId = flag == MessageDataObject::FLAG_MOVE ?
		IDS_PROGRESS_MOVEMESSAGE : IDS_PROGRESS_COPYMESSAGE;
	ProgressDialogMessageOperationCallback callback(hwnd_, nId, nId);
	return MessageDataObject::pasteMessages(pDataObject.get(), pDocument_,
		pFolder, flag, &callback, pDocument_->getUndoManager());
}


/****************************************************************************
 *
 * EditSelectAllMessageAction
 *
 */

qm::EditSelectAllMessageAction::EditSelectAllMessageAction(MessageSelectionModel* pMessageSelectionModel) :
	pMessageSelectionModel_(pMessageSelectionModel)
{
}

qm::EditSelectAllMessageAction::~EditSelectAllMessageAction()
{
}

void qm::EditSelectAllMessageAction::invoke(const ActionEvent& event)
{
	pMessageSelectionModel_->selectAll();
}

bool qm::EditSelectAllMessageAction::isEnabled(const ActionEvent& event)
{
	return pMessageSelectionModel_->canSelect();
}


/****************************************************************************
 *
 * EditUndoMessageAction
 *
 */

qm::EditUndoMessageAction::EditUndoMessageAction(UndoManager* pUndoManager,
												 AccountManager* pAccountManager,
												 HWND hwnd) :
	pUndoManager_(pUndoManager),
	pAccountManager_(pAccountManager),
	hwnd_(hwnd)
{
}

qm::EditUndoMessageAction::~EditUndoMessageAction()
{
}

void qm::EditUndoMessageAction::invoke(const ActionEvent& event)
{
	std::auto_ptr<UndoItem> pUndoItem(pUndoManager_->popUndoItem());
	if (!pUndoItem.get())
		return;
	
	std::auto_ptr<UndoExecutor> pExecutor(
		pUndoItem->getExecutor(UndoContext(pAccountManager_)));
	if (!pExecutor.get() || !pExecutor->execute()) {
		ActionUtil::error(hwnd_, IDS_ERROR_UNDO);
		return;
	}
}

bool qm::EditUndoMessageAction::isEnabled(const ActionEvent& event)
{
	return pUndoManager_->hasUndoItem();
}


/****************************************************************************
 *
 * FileCheckAction
 *
 */

qm::FileCheckAction::FileCheckAction(FolderModel* pFolderModel,
									 HWND hwnd) :
	pFolderModel_(pFolderModel),
	hwnd_(hwnd)
{
}

qm::FileCheckAction::~FileCheckAction()
{
}

void qm::FileCheckAction::invoke(const ActionEvent& event)
{
	Account* pAccount = FolderActionUtil::getAccount(pFolderModel_);
	if (!pAccount)
		return;
	
	if (!check(pAccount))
		ActionUtil::error(hwnd_, IDS_ERROR_CHECK);
}

bool qm::FileCheckAction::isEnabled(const ActionEvent& event)
{
	return FolderActionUtil::getAccount(pFolderModel_) != 0;
}

bool qm::FileCheckAction::check(Account* pAccount) const
{
	class AccountCheckCallbackImpl : public ProgressDialogMessageOperationCallbackBase<AccountCheckCallback>
	{
	public:
		AccountCheckCallbackImpl(HWND hwnd,
								 UINT nTitle,
								 UINT nMessage) :
			ProgressDialogMessageOperationCallbackBase<AccountCheckCallback>(hwnd, nTitle, nMessage)
		{
		}
	
	public:
		virtual Ignore isIgnoreError(MessageHolder* pmh)
		{
			HINSTANCE hInst = Application::getApplication().getResourceHandle();
			wstring_ptr wstrTemplate(loadString(hInst, IDS_CONFIRM_IGNORECHECKERROR));
			wstring_ptr wstrFolderName(pmh->getFolder()->getFullName());
			const size_t nLen = wcslen(wstrTemplate.get()) + wcslen(wstrFolderName.get()) + 100;
			wstring_ptr wstrMessage(allocWString(nLen));
			const MessageHolder::MessageBoxKey& boxKey = pmh->getMessageBoxKey();
			_snwprintf(wstrMessage.get(), nLen, wstrTemplate.get(), wstrFolderName.get(),
				pmh->getId(), boxKey.nOffset_, boxKey.nLength_);
			switch (messageBox(wstrMessage.get(), MB_YESNO | MB_ICONQUESTION | MB_DEFBUTTON2, getDialog()->getHandle())) {
			case IDYES:
				return ::GetKeyState(VK_SHIFT) < 0 ? IGNORE_ALL : IGNORE_TRUE;
			default:
				return IGNORE_FALSE;
			}
		}
	};
	
	AccountCheckCallbackImpl callback(hwnd_, IDS_PROGRESS_CHECK, IDS_PROGRESS_CHECK);
	return pAccount->check(&callback);
}


/****************************************************************************
 *
 * FileCloseAction
 *
 */

qm::FileCloseAction::FileCloseAction(HWND hwnd) :
	hwnd_(hwnd)
{
}

qm::FileCloseAction::~FileCloseAction()
{
}

void qm::FileCloseAction::invoke(const ActionEvent& event)
{
	Window(hwnd_).postMessage(WM_CLOSE);
}


/****************************************************************************
 *
 * FileCompactAction
 *
 */

qm::FileCompactAction::FileCompactAction(FolderModel* pFolderModel,
										 HWND hwnd) :
	pFolderModel_(pFolderModel),
	hwnd_(hwnd)
{
}

qm::FileCompactAction::~FileCompactAction()
{
}

void qm::FileCompactAction::invoke(const ActionEvent& event)
{
	Account* pAccount = FolderActionUtil::getAccount(pFolderModel_);
	if (!pAccount)
		return;
	
	if (!compact(pAccount))
		ActionUtil::error(hwnd_, IDS_ERROR_COMPACT);
}

bool qm::FileCompactAction::isEnabled(const ActionEvent& event)
{
	return FolderActionUtil::getAccount(pFolderModel_) != 0;
}

bool qm::FileCompactAction::compact(Account* pAccount) const
{
	ProgressDialogMessageOperationCallback callback(hwnd_,
		IDS_PROGRESS_COMPACT, IDS_PROGRESS_COMPACT);
	return pAccount->compact(&callback);
}


/****************************************************************************
 *
 * FileDumpAction
 *
 */

qm::FileDumpAction::FileDumpAction(FolderModel* pFolderModel,
								   HWND hwnd) :
								   pFolderModel_(pFolderModel),
								   hwnd_(hwnd)
{
}

qm::FileDumpAction::~FileDumpAction()
{
}

void qm::FileDumpAction::invoke(const ActionEvent& event)
{
	Account* pAccount = FolderActionUtil::getAccount(pFolderModel_);
	if (!pAccount)
		return;
	
	wstring_ptr wstrPath(qs::UIUtil::browseFolder(hwnd_, 0, 0));
	if (!wstrPath.get())
		return;
	
	if (!dump(pAccount, wstrPath.get()))
		ActionUtil::error(hwnd_, IDS_ERROR_DUMP);
}

bool qm::FileDumpAction::isEnabled(const ActionEvent& event)
{
	return FolderActionUtil::getAccount(pFolderModel_) != 0;
}

bool qm::FileDumpAction::dump(Account* pAccount,
							  const WCHAR* pwszPath) const
{
	Lock<Account> lock(*pAccount);
	
	const Account::FolderList& listFolder = pAccount->getFolders();
	
	unsigned int nCount = 0;
	for (Account::FolderList::const_iterator it = listFolder.begin(); it != listFolder.end(); ++it) {
		Folder* pFolder = *it;
		if (pFolder->getType() == Folder::TYPE_NORMAL) {
			if (!pFolder->loadMessageHolders())
				return false;
			nCount += pFolder->getCount();
		}
	}
	
	ProgressDialog dialog;
	ProgressDialogInit init(&dialog, hwnd_, IDS_PROGRESS_DUMP, IDS_PROGRESS_DUMP, 0, nCount, 0);
	
	for (Account::FolderList::const_iterator it = listFolder.begin(); it != listFolder.end(); ++it) {
		Folder* pFolder = *it;
		if (!dumpFolder(pFolder, pwszPath, pFolder->getType() != Folder::TYPE_NORMAL, &dialog))
			return false;
	}
	
	return true;
}

bool qm::FileDumpAction::dumpFolder(Folder* pFolder,
									const WCHAR* pwszPath,
									bool bCreateDirectoryOnly,
									ProgressDialog* pDialog) const
{
	wstring_ptr wstrDir(getDirectory(pwszPath, pFolder));
	if (!File::createDirectory(wstrDir.get()))
		return false;
	
	if (bCreateDirectoryOnly)
		return true;
	
	unsigned int nCount = pFolder->getCount();
	for (unsigned int n = 0; n < nCount; ++n) {
		MessageHolder* pmh = pFolder->getMessage(n);
		
		WCHAR wszName[32];
		_snwprintf(wszName, countof(wszName), L"%u", pmh->getId());
		
		wstring_ptr wstrPath(concat(wstrDir.get(), L"\\", wszName));
		FileOutputStream fileStream(wstrPath.get());
		if (!fileStream)
			return false;
		BufferedOutputStream stream(&fileStream, false);
		if (!FileExportAction::writeMessage(&stream, pmh,
			FileExportAction::FLAG_ADDFLAGS, SECURITYMODE_NONE))
			return false;
		
		if (pDialog->isCanceled())
			break;
		pDialog->step();
	}
	
	return true;
}

wstring_ptr qm::FileDumpAction::getDirectory(const WCHAR* pwszPath,
											 Folder* pFolder)
{
	assert(pwszPath);
	assert(pFolder);
	
	wstring_ptr wstrParentPath;
	if (pFolder->getParentFolder())
		wstrParentPath = getDirectory(pwszPath, pFolder->getParentFolder());
	else
		wstrParentPath = allocWString(pwszPath);
	
	WCHAR wsz[32];
	_snwprintf(wsz, countof(wsz), L"$%c%x",
		pFolder->getType() == Folder::TYPE_NORMAL ? L'n' : L'q', pFolder->getFlags());
	
	ConcatW c[] = {
		{ wstrParentPath.get(),	-1	},
		{ L"\\",				1	},
		{ pFolder->getName(),	-1	},
		{ wsz,					-1	}
	};
	return concat(c, countof(c));
}


/****************************************************************************
 *
 * FileExitAction
 *
 */

qm::FileExitAction::FileExitAction(HWND hwnd,
								   Document* pDocument,
								   SyncManager* pSyncManager,
								   SyncDialogManager* pSyncDialogManager,
								   TempFileCleaner* pTempFileCleaner,
								   EditFrameWindowManager* pEditFrameWindowManager,
								   AddressBookFrameWindowManager* pAddressBookFrameWindowManager,
								   FolderModel* pFolderModel,
								   Profile* pProfile) :
	hwnd_(hwnd),
	pDocument_(pDocument),
	pSyncManager_(pSyncManager),
	pSyncDialogManager_(pSyncDialogManager),
	pTempFileCleaner_(pTempFileCleaner),
	pEditFrameWindowManager_(pEditFrameWindowManager),
	pAddressBookFrameWindowManager_(pAddressBookFrameWindowManager),
	pFolderModel_(pFolderModel),
	pProfile_(pProfile)
{
}

qm::FileExitAction::~FileExitAction()
{
}

bool qm::FileExitAction::exit(bool bDestroy)
{
	if (pSyncManager_->isSyncing()) {
		ActionUtil::error(hwnd_, IDS_ERROR_SYNCHRONIZING);
		return false;
	}
	
	if (!pEditFrameWindowManager_->closeAll())
		return false;
	
	if (!pAddressBookFrameWindowManager_->closeAll())
		return false;
	
	bool bEmptyTrash = pProfile_->getInt(L"Global", L"EmptyTrashOnExit") != 0;
	if (bEmptyTrash)
		FolderEmptyTrashAction::emptyAllTrash(pDocument_, pSyncManager_,
			pSyncDialogManager_, pFolderModel_, hwnd_, pProfile_);
	
	{
		WaitCursor cursor;
		Application& app = Application::getApplication();
		bool bForce = false;
		do {
			if (app.save(bForce))
				break;
			assert(!bForce);
			
			int nId = messageBox(app.getResourceHandle(), IDS_CONFIRM_EXIT,
				MB_YESNOCANCEL | MB_ICONERROR | MB_DEFBUTTON3, hwnd_);
			switch (nId) {
			case IDYES:
				bForce = true;
				break;
			case IDNO:
				break;
			case IDCANCEL:
			default:
				return false;
			}
		} while (true);
		app.startShutdown();
		pDocument_->setOffline(true);
		pSyncManager_->dispose();
	}
	
	struct CallbackImpl : public TempFileCleanerCallback
	{
		virtual bool confirmDelete(const WCHAR* pwszPath)
		{
			HINSTANCE hInst = Application::getApplication().getResourceHandle();
			
			wstring_ptr wstr(loadString(hInst, IDS_CONFIRM_DELETETEMPFILE));
			wstring_ptr wstrMessage(concat(wstr.get(), pwszPath));
			
			int nMsg = messageBox(wstrMessage.get(),
				MB_YESNO | MB_DEFBUTTON2 | MB_ICONQUESTION);
			return nMsg == IDYES;
		}
	} callback;
	pTempFileCleaner_->clean(&callback);
	
	if (bDestroy)
		Window(hwnd_).destroyWindow();
	
	return true;
}

void qm::FileExitAction::invoke(const ActionEvent& event)
{
	exit(true);
}


/****************************************************************************
 *
 * FileExportAction
 *
 */

qm::FileExportAction::FileExportAction(MessageSelectionModel* pMessageSelectionModel,
									   EncodingModel* pEncodingModel,
									   SecurityModel* pSecurityModel,
									   Document* pDocument,
									   Profile* pProfile,
									   HWND hwnd) :
	pMessageSelectionModel_(pMessageSelectionModel),
	pEncodingModel_(pEncodingModel),
	pSecurityModel_(pSecurityModel),
	pDocument_(pDocument),
	pProfile_(pProfile),
	hwnd_(hwnd)
{
}

qm::FileExportAction::~FileExportAction()
{
}

void qm::FileExportAction::invoke(const ActionEvent& event)
{
	AccountLock lock;
	Folder* pFolder = 0;
	MessageHolderList l;
	pMessageSelectionModel_->getSelectedMessages(&lock, &pFolder, &l);
	
	if (!l.empty()) {
		if (!exportMessages(lock.get(), pFolder, l)) {
			ActionUtil::error(hwnd_, IDS_ERROR_EXPORT);
			return;
		}
	}
}

bool qm::FileExportAction::isEnabled(const ActionEvent& event)
{
	return pMessageSelectionModel_->hasSelectedMessage();
}

bool qm::FileExportAction::exportMessages(Account* pAccount,
										  Folder* pFolder,
										  const MessageHolderList& l)
{
	const TemplateManager* pTemplateManager = pDocument_->getTemplateManager();
	
	ExportDialog dialog(pAccount, pTemplateManager, pProfile_, l.size() == 1);
	if (dialog.doModal(hwnd_) == IDOK) {
		const Template* pTemplate = 0;
		const WCHAR* pwszEncoding = 0;
		const WCHAR* pwszTemplate = dialog.getTemplate();
		if (pwszTemplate) {
			pTemplate = pTemplateManager->getTemplate(pAccount, pFolder, pwszTemplate);
			if (!pTemplate)
				return false;
			pwszEncoding = dialog.getEncoding();
		}
		
		unsigned int nFlags = 0;
		if (dialog.isExportFlags())
			nFlags |= FLAG_ADDFLAGS;
		unsigned int nSecurityMode = pSecurityModel_->getSecurityMode();
		
		ProgressDialog progressDialog;
		ProgressDialogInit init(&progressDialog, hwnd_,
			IDS_PROGRESS_EXPORT, IDS_PROGRESS_EXPORT, 0, l.size(), 0);
		
		if (dialog.isFilePerMessage()) {
			const WCHAR* pwszPath = dialog.getPath();
			const WCHAR* pFileName = wcsrchr(pwszPath, L'\\');
			pFileName = pFileName ? pFileName + 1 : pwszPath;
			const WCHAR* pExt = wcsrchr(pFileName, L'.');
			if (!pExt)
				pExt = pFileName + wcslen(pFileName);
			
			MessageHolderList::size_type n = 0;
			while (n < l.size()) {
				if (progressDialog.isCanceled())
					break;
				progressDialog.setPos(n);
				
				WCHAR wszNumber[32];
				_snwprintf(wszNumber, countof(wszNumber), L"%d", n);
				ConcatW c[] = {
					{ pwszPath,		pExt - pwszPath	},
					{ wszNumber,	-1				},
					{ pExt,			-1				}
				};
				wstring_ptr wstrPath(concat(c, countof(c)));
				
				FileOutputStream fileStream(wstrPath.get());
				if (!fileStream)
					return false;
				BufferedOutputStream stream(&fileStream, false);
				if (pTemplate) {
					if (!writeMessage(&stream, pTemplate, pFolder, l[n], pwszEncoding))
						return false;
				}
				else {
					if (!writeMessage(&stream, l[n], nFlags, nSecurityMode))
						return false;
				}
				if (!stream.close())
					return false;
				
				++n;
			}
			progressDialog.setPos(n);
		}
		else {
			FileOutputStream fileStream(dialog.getPath());
			if (!fileStream)
				return false;
			BufferedOutputStream stream(&fileStream, false);
			
			int nPos = 0;
			if (l.size() == 1 && !pTemplate) {
				if (!writeMessage(&stream, l.front(), nFlags, nSecurityMode))
					return false;
				++nPos;
			}
			else {
				for (MessageHolderList::const_iterator it = l.begin(); it != l.end(); ++it) {
					if (progressDialog.isCanceled())
						break;
					progressDialog.setPos(nPos++);
					
					if (pTemplate) {
						if (!writeMessage(&stream, pTemplate, pFolder, *it, pwszEncoding))
							return false;
					}
					else {
						if (!writeMessage(&stream, *it, nFlags | FLAG_WRITESEPARATOR, nSecurityMode))
							return false;
					}
				}
			}
			progressDialog.setPos(nPos);
			
			if (!stream.close())
				return false;
		}
	}
	return true;
}

bool qm::FileExportAction::writeMessage(OutputStream* pStream,
										const Template* pTemplate,
										Folder* pFolder,
										MessageHolder* pmh,
										const WCHAR* pwszEncoding)
{
	Message msg;
	TemplateContext context(pmh, &msg, MessageHolderList(), pFolder,
		pmh->getAccount(), pDocument_, hwnd_, pEncodingModel_->getEncoding(),
		MacroContext::FLAG_UITHREAD | MacroContext::FLAG_UI,
		pSecurityModel_->getSecurityMode(), pProfile_, 0, TemplateContext::ArgumentList());
	
	wxstring_size_ptr wstrValue;
	if (pTemplate->getValue(context, &wstrValue) != Template::RESULT_SUCCESS)
		return false;
	
	std::auto_ptr<Converter> pConverter(ConverterFactory::getInstance(pwszEncoding));
	if (!pConverter.get())
		return false;
	
	size_t nLen = wstrValue.size();
	xstring_size_ptr strContent = pConverter->encode(wstrValue.get(), &nLen);
	if (!strContent.get())
		return false;
	
	if (pStream->write(reinterpret_cast<unsigned char*>(strContent.get()), strContent.size()) == -1)
		return false;
	
	return true;
}

bool qm::FileExportAction::writeMessage(OutputStream* pStream,
										MessageHolder* pmh,
										unsigned int nFlags,
										unsigned int nSecurityMode)
{
	assert(pStream);
	assert(pmh);
	
	Message msg;
	if (!pmh->getMessage(Account::GETMESSAGEFLAG_ALL, 0, nSecurityMode, &msg))
		return false;
	
	if (nFlags & FLAG_ADDFLAGS) {
		NumberParser flags(pmh->getFlags() & MessageHolder::FLAG_USER_MASK, NumberParser::FLAG_HEX);
		if (!msg.replaceField(L"X-QMAIL-Flags", flags))
			return false;
	}
	
	xstring_size_ptr strContent(msg.getContent());
	if (!strContent.get())
		return false;
	
	if (nFlags & FLAG_WRITESEPARATOR) {
		string_ptr strFrom;
		AddressListParser from;
		if (msg.getField(L"From", &from) == Part::FIELD_EXIST) {
			const AddressListParser::AddressList& l = from.getAddressList();
			if (!l.empty() && !l.front()->getGroup())
				strFrom = wcs2mbs(l.front()->getAddress().get());
		}
		const CHAR* pszFrom = strFrom.get();
		if (!pszFrom || !*pszFrom)
			pszFrom = "-";
		
		Time date;
		pmh->getDate(&date);
		string_ptr strDate(wcs2mbs(date.format(L"%W %M1 %D %h:%m:%s %Y4", Time::FORMAT_LOCAL).get()));
		
		if (pStream->write(reinterpret_cast<const unsigned char*>("From "), 5) == -1 ||
			pStream->write(reinterpret_cast<const unsigned char*>(pszFrom), strlen(pszFrom)) == -1 ||
			pStream->write(reinterpret_cast<const unsigned char*>(" "), 1) == -1 ||
			pStream->write(reinterpret_cast<const unsigned char*>(strDate.get()), strlen(strDate.get())) == -1 ||
			pStream->write(reinterpret_cast<const unsigned char*>("\r\n"), 2) == -1)
			return false;
		
		const CHAR* p = strContent.get();
		while (*p) {
			const CHAR* pCheck = p;
			while (*pCheck == '>')
				++pCheck;
			if (strncmp(pCheck, "From ", 5) == 0) {
				if (pStream->write(reinterpret_cast<unsigned char*>(">"), 1) == -1)
					return false;
			}
			
			const CHAR* pEnd = strstr(p, "\r\n");
			size_t nLen = pEnd ? pEnd - p + 2 : strlen(p);
			
			if (pStream->write(reinterpret_cast<const unsigned char*>(p), nLen) == -1)
				return false;
			
			p += nLen;
		}
		
		if (p - strContent.get() < 2 || *(p - 1) != '\n' || *(p - 2) != '\r') {
			if (pStream->write(reinterpret_cast<const unsigned char*>("\r\n"), 2) == -1)
				return false;
		}
		if (pStream->write(reinterpret_cast<const unsigned char*>("\r\n"), 2) == -1)
			return false;
	}
	else {
		if (pStream->write(reinterpret_cast<unsigned char*>(strContent.get()), strContent.size()) == -1)
			return false;
	}
	
	return true;
}


/****************************************************************************
 *
 * FileImportAction
 *
 */

qm::FileImportAction::FileImportAction(FolderModel* pFolderModel,
									   Document* pDocument,
									   SyncManager* pSyncManager,
									   SyncDialogManager* pSyncDialogManager,
									   Profile* pProfile,
									   HWND hwnd) :
	pFolderModel_(pFolderModel),
	pDocument_(pDocument),
	pSyncManager_(pSyncManager),
	pSyncDialogManager_(pSyncDialogManager),
	pProfile_(pProfile),
	hwnd_(hwnd)
{
}

qm::FileImportAction::~FileImportAction()
{
}

void qm::FileImportAction::invoke(const ActionEvent& event)
{
	Folder* pFolder = FolderActionUtil::getFolder(pFolderModel_);
	if (pFolder && pFolder->getType() == Folder::TYPE_NORMAL) {
		wstring_ptr wstrErrorPath;
		unsigned int nErrorLine = -1;
		if (!import(static_cast<NormalFolder*>(pFolder), &wstrErrorPath, &nErrorLine)) {
			HINSTANCE hInst = Application::getApplication().getResourceHandle();
			wstring_ptr wstrMessage(loadString(hInst, IDS_ERROR_IMPORT));
			if (wstrErrorPath.get()) {
				StringBuffer<WSTRING> buf(wstrMessage.get());
				buf.append(L'\n');
				buf.append(wstrErrorPath.get());
				if (nErrorLine != -1) {
					buf.append(L" (");
					WCHAR wszLine[32];
					_snwprintf(wszLine, countof(wszLine), L"%u", nErrorLine);
					buf.append(wszLine);
					buf.append(L")");
				}
				wstrMessage = buf.getString();
			}
			ActionUtil::error(hwnd_, wstrMessage.get());
			return;
		}
		if (!pFolder->getAccount()->save(false)) {
			ActionUtil::error(hwnd_, IDS_ERROR_SAVE);
			return;
		}
	}
}

bool qm::FileImportAction::isEnabled(const ActionEvent& event)
{
	Folder* pFolder = FolderActionUtil::getFolder(pFolderModel_);
	return pFolder && pFolder->getType() == Folder::TYPE_NORMAL;
}

bool qm::FileImportAction::import(NormalFolder* pFolder,
								  const PathList& listPath,
								  bool bMultipleMessagesInFile,
								  const WCHAR* pwszEncoding,
								  unsigned int nFlags,
								  HWND hwnd,
								  wstring_ptr* pwstrErrorPath,
								  unsigned int* pnErrorLine)
{
	size_t nCount = listPath.size();
	if (bMultipleMessagesInFile)
		nCount = 100;
	
	ProgressDialog progressDialog;
	ProgressDialogInit init(&progressDialog, hwnd,
		IDS_PROGRESS_IMPORT, IDS_PROGRESS_IMPORT, 0, nCount, 0);
	
	if (bMultipleMessagesInFile) {
		int nPos = 0;
		for (PathList::size_type n = 0; n < listPath.size(); ++n) {
			bool bCanceled = false;
			if (!readMultipleMessages(pFolder, listPath[n], pwszEncoding,
				nFlags, &progressDialog, &nPos, &bCanceled, pnErrorLine)) {
				if (pwstrErrorPath)
					*pwstrErrorPath = allocWString(listPath[n]);
				return false;
			}
			if (bCanceled)
				break;
		}
	}
	else {
		for (PathList::size_type n = 0; n < listPath.size(); ++n) {
			if (!readSingleMessage(pFolder, listPath[n], pwszEncoding, nFlags)) {
				if (pwstrErrorPath)
					*pwstrErrorPath = allocWString(listPath[n]);
				return false;
			}
			
			if (progressDialog.isCanceled())
				break;
			progressDialog.setPos(n);
		}
	}
	
	return true;
}

bool qm::FileImportAction::importShowDialog(NormalFolder* pFolder,
											const PathList& listPath,
											qs::Profile* pProfile,
											HWND hwnd,
											wstring_ptr* pwstrErrorPath,
											unsigned int* pnErrorLine)
{
	StringBuffer<WSTRING> bufPath;
	for (PathList::const_iterator it = listPath.begin(); it != listPath.end(); ++it) {
		if (bufPath.getLength() != 0)
			bufPath.append(L';');
		bufPath.append(*it);
	}
	
	ImportDialog dialog(bufPath.getCharArray(), pProfile);
	if (dialog.doModal(hwnd) == IDOK) {
		PathList listPath;
		StringListFree<PathList> free(listPath);
		
		const WCHAR* pwszPath = dialog.getPath();
		const WCHAR* pBegin = pwszPath;
		while (true) {
			const WCHAR* pEnd = wcschr(pBegin, L';');
			wstring_ptr wstrPath(allocWString(pBegin, pEnd ? pEnd - pBegin : -1));
			if (wcschr(wstrPath.get(), L'*') || wcschr(wstrPath.get(), L'?')) {
				wstring_ptr wstrDir;
				const WCHAR* pFileName = wcsrchr(wstrPath.get(), L'\\');
				if (pFileName)
					wstrDir = allocWString(wstrPath.get(), pFileName - wstrPath.get() + 1);
				else
					wstrDir = allocWString(L"");
				
				W2T(wstrPath.get(), ptszPath);
				WIN32_FIND_DATA fd;
				AutoFindHandle hFind(::FindFirstFile(ptszPath, &fd));
				if (hFind.get()) {
					do {
						if (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
							continue;
						
						T2W(fd.cFileName, ptszFileName);
						wstring_ptr wstrFilePath(concat(wstrDir.get(), ptszFileName));
						listPath.push_back(wstrFilePath.get());
						wstrFilePath.release();
						
					} while (::FindNextFile(hFind.get(), &fd));
				}
			}
			else {
				listPath.push_back(wstrPath.get());
				wstrPath.release();
			}
			
			if (!pEnd)
				break;
			pBegin = pEnd + 1;
			if (!*pBegin)
				break;
		}
		
		if (!import(pFolder, listPath, dialog.isMultiple(), dialog.getEncoding(),
			dialog.getFlags(), hwnd, pwstrErrorPath, pnErrorLine))
			return false;
	}
	
	return true;
}

bool qm::FileImportAction::readSingleMessage(NormalFolder* pFolder,
											 const WCHAR* pwszPath,
											 const WCHAR* pwszEncoding,
											 unsigned int nFlags)
{
	FileInputStream stream(pwszPath);
	if (!stream)
		return false;
	BufferedInputStream bufferedStream(&stream, false);
	
	xstring_size_ptr strContent;
	if (pwszEncoding) {
		InputStreamReader reader(&bufferedStream, false, pwszEncoding);
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
		if (!reader.close())
			return false;
		
		if (buf.getLength() == 0)
			return true;
		
		MessageCreator creator(MessageCreator::FLAG_RECOVER, SECURITYMODE_NONE);
		std::auto_ptr<Message> pMessage(creator.createMessage(
			0, buf.getCharArray(), buf.getLength()));
		if (!pMessage.get())
			return false;
		
		strContent = pMessage->getContent();
	}
	else {
		XStringBuffer<XSTRING> buf;
		
		unsigned char c = 0;
		bool bCR = false;
		while (true) {
			size_t nRead = stream.read(&c, 1);
			if (nRead == -1)
				return false;
			else if (nRead == 0)
				break;
			
			if (bCR) {
				if (!buf.append("\r\n"))
					return false;
				switch (c) {
				case '\r':
					break;
				case '\n':
					bCR = false;
					break;
				default:
					if (!buf.append(static_cast<CHAR>(c)))
						return false;
					bCR = false;
					break;
				}
			}
			else {
				switch (c) {
				case '\r':
					bCR = true;
					break;
				case '\n':
					if (!buf.append("\r\n"))
						return false;
					break;
				default:
					if (!buf.append(static_cast<CHAR>(c)))
						return false;
					break;
				}
			}
		}
		if (bCR) {
			if (!buf.append("\r\n"))
				return false;
		}
		
		strContent = buf.getXStringSize();
	}
	
	Account* pAccount = pFolder->getAccount();
	if (!pAccount->importMessage(pFolder, strContent.get(), strContent.size(), nFlags))
		return false;
	
	return true;
}

bool qm::FileImportAction::readMultipleMessages(NormalFolder* pFolder,
												const WCHAR* pwszPath,
												const WCHAR* pwszEncoding,
												unsigned int nFlags,
												ProgressDialog* pDialog,
												int* pnPos,
												bool* pbCanceled,
												unsigned int* pnErrorLine)
{
	assert(pFolder);
	assert(pwszPath);
	assert((pDialog && pnPos && pbCanceled) || (!pDialog && !pnPos && !pbCanceled));
	
	FileInputStream fileStream(pwszPath);
	if (!fileStream)
		return false;
	BufferedInputStream stream(&fileStream, false);
	
	XStringBuffer<XSTRING> buf;
	
	const CHAR* pszNewLine = pwszEncoding ? "\n" : "\r\n";
	CHAR cPrev = '\0';
	bool bNewLine = true;
	unsigned int nLine = 0;
	unsigned int nStartLine = 0;
	unsigned int& nErrorLine = pnErrorLine ? *pnErrorLine : nStartLine;
	while (bNewLine) {
		xstring_ptr strLine;
		CHAR cNext = '\0';
		if (!readLine(&stream, cPrev, &strLine, &cNext, &bNewLine))
			return false;
		cPrev = cNext;
		++nLine;
		
		if (!bNewLine || strncmp(strLine.get(), "From ", 5) == 0) {
			if (!bNewLine) {
				if (!buf.append(strLine.get()))
					return false;
			}
			if (buf.getLength() != 0) {
				if (pDialog) {
					if (pDialog->isCanceled()) {
						*pbCanceled = true;
						return true;
					}
					pDialog->setPos((*pnPos)++ % 100);
				}
				
				size_t nLen = buf.getLength();
				size_t nNewLineLen = strlen(pszNewLine);
				if (nLen >=  nNewLineLen*2 &&
					strncmp(buf.getCharArray() + nLen - nNewLineLen, pszNewLine, nNewLineLen) == 0 &&
					strncmp(buf.getCharArray() + nLen - nNewLineLen*2, pszNewLine, nNewLineLen) == 0)
					buf.remove(nLen - nNewLineLen, nLen);
				
				xstring_size_ptr strContent;
				if (pwszEncoding) {
					std::auto_ptr<Converter> pConverter(ConverterFactory::getInstance(pwszEncoding));
					if (!pConverter.get())
						return false;
					
					size_t nLen = buf.getLength();
					wxstring_size_ptr wstrMessage(pConverter->decode(buf.getCharArray(), &nLen));
					if (!wstrMessage.get())
						return false;
					
					MessageCreator creator(MessageCreator::FLAG_RECOVER, SECURITYMODE_NONE);
					std::auto_ptr<Message> pMessage(creator.createMessage(
						0, wstrMessage.get(), wstrMessage.size()));
					if (!pMessage.get())
						return false;
					
					strContent = pMessage->getContent();
				}
				else {
					strContent = buf.getXStringSize();
				}
				
				Account* pAccount = pFolder->getAccount();
				if (!pAccount->importMessage(pFolder, strContent.get(), strContent.size(), nFlags))
					return false;
				
				buf.remove();
				nErrorLine = nLine;
			}
		}
		else {
			const CHAR* p = strLine.get();
			if (*p == '>') {
				while (*p == '>')
					++p;
				if (strncmp(p, "From ", 5) == 0)
					p = strLine.get() + 1;
				else
					p = strLine.get();
			}
			
			if (!buf.append(p) || !buf.append(pszNewLine))
				return false;
		}
	}
	
	return true;
}

bool qm::FileImportAction::import(NormalFolder* pFolder,
								  wstring_ptr* pwstrErrorPath,
								  unsigned int* pnErrorLine)
{
	return importShowDialog(pFolder, PathList(), pProfile_, hwnd_, pwstrErrorPath, pnErrorLine);
}

bool qm::FileImportAction::readLine(InputStream* pStream,
									CHAR cPrev,
									xstring_ptr* pstrLine,
									CHAR* pcNext,
									bool* pbNewLine)
{
	assert(pStream);
	assert(pstrLine);
	assert(pcNext);
	assert(pbNewLine);
	
	pstrLine->reset(0);
	*pcNext = '\0';
	*pbNewLine = false;
	
	XStringBuffer<XSTRING> buf;
	
	unsigned char c = 0;
	bool bNewLine = false;
	while (!bNewLine) {
		if (cPrev != '\0') {
			c = cPrev;
			cPrev = '\0';
		}
		else {
			size_t nRead = pStream->read(&c, 1);
			if (nRead == -1)
				return false;
			else if (nRead == 0)
				break;
		}
		
		if (c == '\r') {
			bNewLine = true;
			size_t nRead = pStream->read(&c, 1);
			if (nRead == -1)
				return false;
			else if (nRead != 0 && c != '\n')
				*pcNext = c;
		}
		else if (c == '\n') {
			bNewLine = true;
		}
		else {
			if (!buf.append(static_cast<CHAR>(c)))
				return false;
		}
	}
	
	*pstrLine = buf.getXString();
	*pbNewLine = bNewLine;
	
	return true;
}


/****************************************************************************
 *
 * FileLoadAction
 *
 */

qm::FileLoadAction::FileLoadAction(FolderModel* pFolderModel,
								   HWND hwnd) :
								   pFolderModel_(pFolderModel),
								   hwnd_(hwnd)
{
}

qm::FileLoadAction::~FileLoadAction()
{
}

void qm::FileLoadAction::invoke(const ActionEvent& event)
{
	Account* pAccount = FolderActionUtil::getAccount(pFolderModel_);
	if (!pAccount)
		return;
	
	wstring_ptr wstrPath(qs::UIUtil::browseFolder(hwnd_, 0, 0));
	if (!wstrPath.get())
		return;
	
	if (!load(pAccount, wstrPath.get()))
		ActionUtil::error(hwnd_, IDS_ERROR_LOAD);
}

bool qm::FileLoadAction::isEnabled(const ActionEvent& event)
{
	return FolderActionUtil::getAccount(pFolderModel_) != 0;
}

bool qm::FileLoadAction::load(Account* pAccount,
							  const WCHAR* pwszPath) const
{
	Lock<Account> lock(*pAccount);
	
	ProgressDialog dialog;
	ProgressDialogInit init(&dialog, hwnd_, IDS_PROGRESS_LOAD, IDS_PROGRESS_LOAD, 0, 100, 0);
	
	int nPos = 0;
	return loadFolder(pAccount, 0, pwszPath, &dialog, &nPos);
}

bool qm::FileLoadAction::loadFolder(Account* pAccount,
									Folder* pFolder,
									const WCHAR* pwszPath,
									ProgressDialog* pDialog,
									int* pnPos) const
{
	wstring_ptr wstrFind(concat(pwszPath, L"\\*.*"));
	W2T(wstrFind.get(), ptszFind);
	WIN32_FIND_DATA fd;
	AutoFindHandle hFind(::FindFirstFile(ptszFind, &fd));
	if (hFind.get()) {
		do {
			if (_tcscmp(fd.cFileName, _T(".")) == 0 ||
				_tcscmp(fd.cFileName, _T("..")) == 0)
				continue;
			
			T2W(fd.cFileName, pwszFileName);
			wstring_ptr wstrPath(concat(pwszPath, L"\\", pwszFileName));
			if (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
				wstring_ptr wstrName;
				Folder::Type type = Folder::TYPE_NORMAL;
				unsigned int nFlags = Folder::FLAG_LOCAL;
				getInfo(pwszFileName, &wstrName, &type, &nFlags);
				
				const WCHAR* pwszFullName = wstrName.get();
				wstring_ptr wstrFullName;
				if (pFolder) {
					wstring_ptr wstrParentName(pFolder->getFullName());
					WCHAR wszSeparator[] = { pFolder->getSeparator(), L'\0' };
					wstrFullName = concat(wstrParentName.get(), wszSeparator, wstrName.get());
					pwszFullName = wstrFullName.get();
				}
				Folder* pChildFolder = pAccount->getFolder(pwszFullName);
				if (!pChildFolder) {
					switch (type) {
					case Folder::TYPE_NORMAL:
						{
							bool bRemote = (nFlags & Folder::FLAG_LOCAL) == 0;
							const WCHAR* pwszName = wstrName.get();
							wstring_ptr wstrRemoteName;
							if (nFlags & Folder::FLAG_NOSELECT) {
								WCHAR wsz[] = {
									pFolder ? pFolder->getSeparator() : L'/',
									L'\0'
								};
								wstrRemoteName = concat(pwszName, wsz);
								pwszName = wstrRemoteName.get();
							}
							pChildFolder = pAccount->createNormalFolder(
								pwszName, pFolder, bRemote, false);
							if (!pChildFolder)
								return false;
							pAccount->setFolderFlags(pChildFolder, nFlags,
								 Folder::FLAG_USER_MASK | Folder::FLAG_BOX_MASK);
						}
						break;
					case Folder::TYPE_QUERY:
						pChildFolder = pAccount->createQueryFolder(wstrName.get(),
							pFolder, L"macro", L"@False()", 0, false);
						if (!pChildFolder)
							return false;
						break;
					default:
						assert(false);
						return false;
					}
				}
				if (!loadFolder(pAccount, pChildFolder, wstrPath.get(), pDialog, pnPos))
					return false;
			}
			else if (pFolder && pFolder->getType() == Folder::TYPE_NORMAL) {
				bool bCanceled = false;
				if (!FileImportAction::readSingleMessage(
					static_cast<NormalFolder*>(pFolder), wstrPath.get(), 0, 0))
					return false;
				
				if (pDialog->isCanceled())
					break;
				pDialog->setPos((*pnPos)++);
			}
		} while (::FindNextFile(hFind.get(), &fd));
	}
	return true;
}

void qm::FileLoadAction::getInfo(const WCHAR* pwszFileName,
								 wstring_ptr* pwstrName,
								 Folder::Type* pType,
								 unsigned int* pnFlags)
{
	const WCHAR* p = wcsrchr(pwszFileName, L'$');
	if (p && p != pwszFileName) {
		*pwstrName = allocWString(pwszFileName, p - pwszFileName);
		++p;
		if (*p == L'n')
			*pType = Folder::TYPE_NORMAL;
		else if (*p == L'q')
			*pType = Folder::TYPE_QUERY;
		else
			p = 0;
		
		if (p) {
			++p;
			WCHAR* pEnd = 0;
			*pnFlags = wcstol(p, &pEnd, 16);
			if (*pEnd)
				p = 0;
		}
	}
	if (!p || p == pwszFileName) {
		*pwstrName = allocWString(pwszFileName);
		*pType = Folder::TYPE_NORMAL;
		*pnFlags = Folder::FLAG_LOCAL;
	}
}


/****************************************************************************
 *
 * FileOfflineAction
 *
 */

qm::FileOfflineAction::FileOfflineAction(Document* pDocument,
										 SyncManager* pSyncManager) :
	pDocument_(pDocument),
	pSyncManager_(pSyncManager)
{
}

qm::FileOfflineAction::~FileOfflineAction()
{
}

void qm::FileOfflineAction::invoke(const ActionEvent& event)
{
	toggleOffline(pDocument_, pSyncManager_);
}

bool qm::FileOfflineAction::isEnabled(const ActionEvent& event)
{
	return !pSyncManager_->isSyncing();
}

bool qm::FileOfflineAction::isChecked(const ActionEvent& event)
{
	return pDocument_->isOffline();
}

void qm::FileOfflineAction::toggleOffline(Document* pDocument,
										  SyncManager* pSyncManager)
{
	if (!pSyncManager->isSyncing())
		pDocument->setOffline(!pDocument->isOffline());
}


/****************************************************************************
 *
 * FilePrintAction
 *
 */

qm::FilePrintAction::FilePrintAction(Document* pDocument,
									 MessageSelectionModel* pMessageSelectionModel,
									 EncodingModel* pEncodingModel,
									 SecurityModel* pSecurityModel,
									 HWND hwnd,
									 Profile* pProfile,
									 TempFileCleaner* pTempFileCleaner) :
	pDocument_(pDocument),
	pMessageSelectionModel_(pMessageSelectionModel),
	pEncodingModel_(pEncodingModel),
	pSecurityModel_(pSecurityModel),
	hwnd_(hwnd),
	pProfile_(pProfile),
	pTempFileCleaner_(pTempFileCleaner)
{
}

qm::FilePrintAction::~FilePrintAction()
{
}

void qm::FilePrintAction::invoke(const ActionEvent& event)
{
	AccountLock lock;
	Folder* pFolder = 0;
	MessageHolderList l;
	pMessageSelectionModel_->getSelectedMessages(&lock, &pFolder, &l);
	
	Account* pAccount = lock.get();
	if (!l.empty()) {
		for (MessageHolderList::const_iterator it = l.begin(); it != l.end(); ++it) {
			if (!print(pAccount, pFolder, *it, l)) {
				ActionUtil::error(hwnd_, IDS_ERROR_PRINT);
				return;
			}
		}
	}
}

bool qm::FilePrintAction::isEnabled(const ActionEvent& event)
{
	return pMessageSelectionModel_->hasSelectedMessage();
}

bool qm::FilePrintAction::print(Account* pAccount,
								Folder* pFolder,
								MessageHolder* pmh,
								const MessageHolderList& listSelected)
{
	const Template* pTemplate = pDocument_->getTemplateManager()->getTemplate(
		pAccount, pFolder, L"print");
	if (!pTemplate)
		return false;
	
	Message msg;
	TemplateContext context(pmh, &msg, listSelected, pFolder, pAccount, pDocument_, hwnd_,
		pEncodingModel_->getEncoding(), MacroContext::FLAG_UITHREAD | MacroContext::FLAG_UI,
		pSecurityModel_->getSecurityMode(), pProfile_, 0, TemplateContext::ArgumentList());
	
	wxstring_size_ptr wstrValue;
	switch (pTemplate->getValue(context, &wstrValue)) {
	case Template::RESULT_SUCCESS:
		break;
	case Template::RESULT_ERROR:
		return false;
	case Template::RESULT_CANCEL:
		return true;
	default:
		assert(false);
		return false;
	}
	
	wstring_ptr wstrExtension(pProfile_->getString(L"Global", L"PrintExtension"));
	
	wstring_ptr wstrPath(UIUtil::writeTemporaryFile(wstrValue.get(),
		L"q3print", wstrExtension.get(), pTempFileCleaner_));
	if (!wstrPath.get())
		return false;
	
	W2T(wstrPath.get(), ptszPath);
	SHELLEXECUTEINFO sei = {
		sizeof(sei),
		0,
		hwnd_,
		_T("print"),
		ptszPath,
		0,
		0,
#ifdef _WIN32_WCE
		SW_SHOWNORMAL,
#else
		SW_SHOWDEFAULT,
#endif
	};
	if (!::ShellExecuteEx(&sei))
		return false;
	
	return true;
}


/****************************************************************************
 *
 * FileSalvageAction
 *
 */

qm::FileSalvageAction::FileSalvageAction(FolderModel* pFolderModel,
										 HWND hwnd) :
	pFolderModel_(pFolderModel),
	hwnd_(hwnd)
{
}

qm::FileSalvageAction::~FileSalvageAction()
{
}

void qm::FileSalvageAction::invoke(const ActionEvent& event)
{
	Folder* pFolder = FolderActionUtil::getFolder(pFolderModel_);
	if (!pFolder || pFolder->getType() != Folder::TYPE_NORMAL)
		return;
	
	if (!salvage(static_cast<NormalFolder*>(pFolder)))
		ActionUtil::error(hwnd_, IDS_ERROR_SALVAGE);
}

bool qm::FileSalvageAction::isEnabled(const ActionEvent& event)
{
	Folder* pFolder = FolderActionUtil::getFolder(pFolderModel_);
	return pFolder && pFolder->getType() == Folder::TYPE_NORMAL;
}

bool qm::FileSalvageAction::salvage(NormalFolder* pFolder) const
{
	Account* pAccount = pFolder->getAccount();
	ProgressDialogMessageOperationCallback callback(
		hwnd_, IDS_PROGRESS_SALVAGE, IDS_PROGRESS_SALVAGE);
	return pAccount->salvage(pFolder, &callback);
}


/****************************************************************************
 *
 * FileSaveAction
 *
 */

qm::FileSaveAction::FileSaveAction(Document* pDocument,
								   ViewModelManager* pViewModelManager,
								   HWND hwnd) :
	pDocument_(pDocument),
	pViewModelManager_(pViewModelManager),
	hwnd_(hwnd)
{
}

qm::FileSaveAction::~FileSaveAction()
{
}

void qm::FileSaveAction::invoke(const ActionEvent& event)
{
	WaitCursor cursor;
	if (!Application::getApplication().save(false)) {
		ActionUtil::error(hwnd_, IDS_ERROR_SAVE);
		return;
	}
}


#ifndef _WIN32_WCE_PSPC
/****************************************************************************
 *
 * FileShowAction
 *
 */

qm::FileShowAction::FileShowAction(MainWindow* pMainWindow,
								   bool bShow) :
	pMainWindow_(pMainWindow),
	bShow_(bShow)
{
}

qm::FileShowAction::~FileShowAction()
{
}

void qm::FileShowAction::invoke(const ActionEvent& event)
{
	if (bShow_)
		pMainWindow_->show();
	else
		pMainWindow_->hide();
}

bool qm::FileShowAction::isEnabled(const ActionEvent& event)
{
	if (bShow_)
		return pMainWindow_->isHidden();
	else
		return !pMainWindow_->isHidden();
}
#endif // _WIN32_WCE_PSPC


/****************************************************************************
 *
 * FileUninstallAction
 *
 */

qm::FileUninstallAction::FileUninstallAction()
{
}

qm::FileUninstallAction::~FileUninstallAction()
{
}

void qm::FileUninstallAction::invoke(const ActionEvent& event)
{
#ifndef _WIN32_WCE
	::SHDeleteKey(HKEY_CURRENT_USER, _T("Software\\sn\\q3"));
	::SHDeleteEmptyKey(HKEY_CURRENT_USER, _T("Software\\sn"));
#else
	::RegDeleteKey(HKEY_CURRENT_USER, _T("Software\\sn\\q3"));
	
	HKEY hKey = 0;
	if (::RegOpenKeyEx(HKEY_CURRENT_USER, _T("Software\\sn"), 0, 0, &hKey) == ERROR_SUCCESS) {
		bool bDelete = false;
		
		DWORD dwKeys = 0;
		DWORD dwValues = 0;
		if (::RegQueryInfoKey(hKey, 0, 0, 0, &dwKeys, 0, 0, &dwValues, 0, 0, 0, 0) == ERROR_SUCCESS)
			bDelete = dwKeys == 0 && dwValues == 0;
		
		::RegCloseKey(hKey);
		
		if (bDelete)
			::RegDeleteKey(HKEY_CURRENT_USER, _T("Software\\sn"));
	}
#endif
}


/****************************************************************************
 *
 * FolderCreateAction
 *
 */

qm::FolderCreateAction::FolderCreateAction(FolderSelectionModel* pFolderSelectionModel,
										   SyncManager* pSyncManager,
										   HWND hwnd,
										   Profile* pProfile) :
	pFolderSelectionModel_(pFolderSelectionModel),
	pSyncManager_(pSyncManager),
	hwnd_(hwnd),
	pProfile_(pProfile)
{
}

qm::FolderCreateAction::~FolderCreateAction()
{
}

void qm::FolderCreateAction::invoke(const ActionEvent& event)
{
	if (pSyncManager_->isSyncing()) {
		ActionUtil::error(hwnd_, IDS_ERROR_SYNCHRONIZING);
		return;
	}
	
	std::pair<Account*, Folder*> p(pFolderSelectionModel_->getFocusedAccountOrFolder());
	if (!p.first && !p.second)
		return;
	
	Folder* pFolder = p.second;
	Account* pAccount = p.first ? p.first : pFolder->getAccount();
	
	CreateFolderDialog::Type type = CreateFolderDialog::TYPE_LOCALFOLDER;
	bool bAllowRemote = pAccount->isSupport(Account::SUPPORT_REMOTEFOLDER) &&
		(!pFolder || !pFolder->isFlag(Folder::FLAG_NOINFERIORS));
	
	if (pFolder) {
		switch (pFolder->getType()) {
		case Folder::TYPE_NORMAL:
			if (pFolder->isFlag(Folder::FLAG_LOCAL) || !bAllowRemote) {
				type = CreateFolderDialog::TYPE_LOCALFOLDER;
				bAllowRemote = false;
			}
			else {
				type = CreateFolderDialog::TYPE_REMOTEFOLDER;
			}
			break;
		case Folder::TYPE_QUERY:
			type = CreateFolderDialog::TYPE_QUERYFOLDER;
			bAllowRemote = false;
			break;
		default:
			assert(false);
			break;
		}
	}
	else {
		type = bAllowRemote ? CreateFolderDialog::TYPE_REMOTEFOLDER :
			CreateFolderDialog::TYPE_LOCALFOLDER;
	}
	
	unsigned int nFlags = 0;
	if (bAllowRemote)
		nFlags |= CreateFolderDialog::FLAG_ALLOWREMOTE;
	
	CreateFolderDialog dialog(type, nFlags);
	if (dialog.doModal(hwnd_) != IDOK)
		return;
	
	NormalFolder* pNormalFolder = 0;
	QueryFolder* pQueryFolder = 0;
	switch (dialog.getType()) {
	case CreateFolderDialog::TYPE_LOCALFOLDER:
		pNormalFolder = pAccount->createNormalFolder(
			dialog.getName(), pFolder, false, false);
		break;
	case CreateFolderDialog::TYPE_REMOTEFOLDER:
		pNormalFolder = pAccount->createNormalFolder(
			dialog.getName(), pFolder, true, true);
		break;
	case CreateFolderDialog::TYPE_QUERYFOLDER:
		pQueryFolder = pAccount->createQueryFolder(dialog.getName(),
			pFolder, L"macro", L"@False()", 0, false);
		break;
	default:
		assert(false);
		break;
	}
	if (!pNormalFolder && !pQueryFolder) {
		ActionUtil::error(hwnd_, IDS_ERROR_CREATEFOLDER);
		return;
	}
	
	if (pQueryFolder) {
		Account::FolderList l(1, pQueryFolder);
		FolderPropertyAction::openProperty(l,
			FolderPropertyAction::OPEN_CONDITION, hwnd_, pProfile_);
	}
	
	if (!pAccount->save(false)) {
		ActionUtil::error(hwnd_, IDS_ERROR_SAVE);
		return;
	}
}

bool qm::FolderCreateAction::isEnabled(const ActionEvent& event)
{
	if (pSyncManager_->isSyncing())
		return false;
	
	std::pair<Account*, Folder*> p(pFolderSelectionModel_->getFocusedAccountOrFolder());
	return p.first || p.second;
}


/****************************************************************************
 *
 * FolderDeleteAction
 *
 */

qm::FolderDeleteAction::FolderDeleteAction(FolderModel* pFolderModel,
										   FolderSelectionModel* pFolderSelectionModel,
										   SyncManager* pSyncManager,
										   HWND hwnd) :
	pFolderModel_(pFolderModel),
	pFolderSelectionModel_(pFolderSelectionModel),
	pSyncManager_(pSyncManager),
	hwnd_(hwnd)
{
}

qm::FolderDeleteAction::~FolderDeleteAction()
{
}

void qm::FolderDeleteAction::invoke(const ActionEvent& event)
{
	if (pSyncManager_->isSyncing()) {
		ActionUtil::error(hwnd_, IDS_ERROR_SYNCHRONIZING);
		return;
	}
	
	Account::FolderList l;
	pFolderSelectionModel_->getSelectedFolders(&l);
	if (l.empty())
		return;
	
	HINSTANCE hInst = Application::getApplication().getResourceHandle();
	wstring_ptr wstrConfirm(loadString(hInst, IDS_CONFIRM_REMOVEFOLDER));
	wstring_ptr wstrName(Util::formatFolders(l, L", "));
	const size_t nLen = wcslen(wstrConfirm.get()) + wcslen(wstrName.get()) + 64;
	wstring_ptr wstrMessage(allocWString(nLen));
	_snwprintf(wstrMessage.get(), nLen, wstrConfirm.get(), wstrName.get());
	int nRet = messageBox(wstrMessage.get(), MB_YESNO | MB_DEFBUTTON2 | MB_ICONQUESTION, hwnd_);
	if (nRet != IDYES)
		return;
	
	Account* pAccount = l.front()->getAccount();
	
	if (l.size() == 1) {
		if (!deleteFolder(pFolderModel_, l[0])) {
			ActionUtil::error(hwnd_, IDS_ERROR_DELETEFOLDER);
			return;
		}
	}
	else {
		std::sort(l.begin(), l.end(), FolderLess());
		
		Folder* pPrevious = 0;
		for (Account::FolderList::iterator it = l.begin(); it != l.end(); ) {
			Folder* pFolder = *it;
			if (pPrevious && pPrevious->isAncestorOf(pFolder)) {
				it = l.erase(it);
			}
			else {
				pPrevious = pFolder;
				++it;
			}
		}
		
		for (Account::FolderList::const_iterator it = l.begin(); it != l.end(); ++it) {
			Folder* pFolder = *it;
			if (!deleteFolder(pFolderModel_, pFolder)) {
				ActionUtil::error(hwnd_, IDS_ERROR_DELETEFOLDER);
				return;
			}
		}
	}
	
	if (!pAccount->save(false)) {
		ActionUtil::error(hwnd_, IDS_ERROR_SAVE);
		return;
	}
}

bool qm::FolderDeleteAction::isEnabled(const ActionEvent& event)
{
	return !pSyncManager_->isSyncing() &&
		pFolderSelectionModel_->hasSelectedFolder();
}

bool qm::FolderDeleteAction::deleteFolder(FolderModel* pFolderModel,
										  Folder* pFolder)
{
	Account* pAccount = pFolder->getAccount();
	
	if (pFolderModel->getCurrent().second == pFolder) {
		Folder* pParent = pFolder->getParentFolder();
		Folder* pFolderNext = 0;
		
		Account::FolderList l(pAccount->getFolders());
		std::sort(l.begin(), l.end(), FolderLess());
		Account::FolderList::const_iterator it = std::find(l.begin(), l.end(), pFolder);
		for (++it; it != l.end() && !pFolderNext; ++it) {
			Folder* p = *it;
			if (p->getAccount() != pAccount)
				break;
			else if (p->getParentFolder() == pParent)
				pFolderNext = p;
		}
		if (!pFolderNext)
			pFolderNext = pParent;
		
		if (pFolderNext)
			pFolderModel->setCurrent(0, pFolderNext, false);
		else
			pFolderModel->setCurrent(pAccount, 0, false);
	}
	
	Folder* pTrash = pAccount->getFolderByBoxFlag(Folder::FLAG_TRASHBOX);
	if (pTrash &&
		pFolder != pTrash &&
		!pTrash->isAncestorOf(pFolder) &&
		!pFolder->isAncestorOf(pTrash) &&
		!pTrash->isFlag(Folder::FLAG_NOINFERIORS) &&
		(pFolder->isFlag(Folder::FLAG_LOCAL) || !pTrash->isFlag(Folder::FLAG_LOCAL))) {
		wstring_ptr wstrName;
		if (pAccount->getFolder(pTrash, pFolder->getName())) {
			const size_t nLen = wcslen(pFolder->getName()) + 32;
			wstrName = allocWString(pFolder->getName(), nLen);
			for (int n = 1; pAccount->getFolder(pTrash, wstrName.get()); ++n)
				_snwprintf(wstrName.get(), nLen, L"%s(%d)", pFolder->getName(), n);
		}
		return pAccount->moveFolder(pFolder, pTrash, wstrName.get());
	}
	else {
		return pAccount->removeFolder(pFolder);
	}
}


/****************************************************************************
 *
 * FolderEmptyAction
 *
 */

qm::FolderEmptyAction::FolderEmptyAction(AccountManager* pAccountManager,
										 FolderSelectionModel* pFolderSelectionModel,
										 UndoManager* pUndoManager,
										 HWND hwnd,
										 Profile* pProfile) :
	pAccountManager_(pAccountManager),
	pFolderSelectionModel_(pFolderSelectionModel),
	pUndoManager_(pUndoManager),
	hwnd_(hwnd),
	pProfile_(pProfile)
{
}

qm::FolderEmptyAction::~FolderEmptyAction()
{
}

void qm::FolderEmptyAction::invoke(const ActionEvent& event)
{
	Account::FolderList l;
	getFolderList(event, &l);
	if (l.empty())
		return;
	
	if (pProfile_->getInt(L"Global", L"ConfirmEmptyFolder")) {
		HINSTANCE hInst = Application::getApplication().getResourceHandle();
		wstring_ptr wstrConfirm(loadString(hInst, IDS_CONFIRM_EMPTYFOLDER));
		wstring_ptr wstrName(Util::formatFolders(l, L", "));
		const size_t nLen = wcslen(wstrConfirm.get()) + wcslen(wstrName.get()) + 64;
		wstring_ptr wstrMessage(allocWString(nLen));
		_snwprintf(wstrMessage.get(), nLen, wstrConfirm.get(), wstrName.get());
		if (messageBox(wstrMessage.get(), MB_YESNO | MB_DEFBUTTON2 | MB_ICONQUESTION, hwnd_) != IDYES)
			return;
	}
	
	if (!emptyFolders(l))
		ActionUtil::error(hwnd_, IDS_ERROR_EMPTYFOLDER);
}

bool qm::FolderEmptyAction::isEnabled(const ActionEvent& event)
{
	Account::FolderList l;
	getFolderList(event, &l);
	return l.size() > 1 || (l.size() == 1 && !l.front()->isFlag(Folder::FLAG_TRASHBOX));
}

bool qm::FolderEmptyAction::emptyFolders(const Account::FolderList& listFolder) const
{
	for (Account::FolderList::const_iterator it = listFolder.begin(); it != listFolder.end(); ++it) {
		Folder* pFolder = *it;
		
		if (!pFolder->isFlag(Folder::FLAG_TRASHBOX)) {
			Account* pAccount = pFolder->getAccount();
			Lock<Account> lock(*pAccount);
			
			MessageHolderList l(pFolder->getMessages());
			if (!l.empty()) {
				UndoItemList undo;
				if (!pAccount->removeMessages(l, pFolder, Account::OPFLAG_ACTIVE, 0, &undo, 0))
					return false;
				pUndoManager_->pushUndoItem(undo.getUndoItem());
			}
		}
	}
	return true;
}

void qm::FolderEmptyAction::getFolderList(const ActionEvent& event,
										  Account::FolderList* pList) const
{
	const WCHAR* pwszFolder = ActionParamUtil::getString(event.getParam(), 0);
	if (pwszFolder) {
		Account* pAccount = pFolderSelectionModel_->getAccount();
		if (pAccount) {
			Folder* pFolder = pAccountManager_->getFolder(pAccount, pwszFolder);
			if (pFolder)
				pList->push_back(pFolder);
		}
	}
	else {
		pFolderSelectionModel_->getSelectedFolders(pList);
	}
}


/****************************************************************************
 *
 * FolderEmptyTrashAction
 *
 */

qm::FolderEmptyTrashAction::FolderEmptyTrashAction(SyncManager* pSyncManager,
												   Document* pDocument,
												   FolderModel* pFolderModel,
												   SyncDialogManager* pSyncDialogManager,
												   HWND hwnd,
												   Profile* pProfile) :
	pSyncManager_(pSyncManager),
	pDocument_(pDocument),
	pFolderModel_(pFolderModel),
	pSyncDialogManager_(pSyncDialogManager),
	hwnd_(hwnd),
	pProfile_(pProfile)
{
}

qm::FolderEmptyTrashAction::~FolderEmptyTrashAction()
{
}

void qm::FolderEmptyTrashAction::invoke(const ActionEvent& event)
{
	if (pSyncManager_->isSyncing()) {
		ActionUtil::error(hwnd_, IDS_ERROR_SYNCHRONIZING);
		return;
	}
	
	Account* pAccount = getAccount();
	if (pAccount)
		emptyTrash(pAccount, pDocument_, pSyncManager_,
			pSyncDialogManager_, pFolderModel_, hwnd_,
			pProfile_->getInt(L"Global", L"ConfirmEmptyTrash") != 0);
}

bool qm::FolderEmptyTrashAction::isEnabled(const ActionEvent& event)
{
	return !pSyncManager_->isSyncing() && getTrash() != 0;
}

void qm::FolderEmptyTrashAction::emptyAllTrash(Document* pDocument,
											   SyncManager* pSyncManager,
											   SyncDialogManager* pSyncDialogManager,
											   FolderModel* pFolderModel,
											   HWND hwnd,
											   Profile* pProfile)
{
	const Document::AccountList& listAccount = pDocument->getAccounts();
	if (std::find_if(listAccount.begin(), listAccount.end(),
		&FolderEmptyTrashAction::hasTrash) == listAccount.end())
		return;
	
	if (pProfile->getInt(L"Global", L"ConfirmEmptyTrash")) {
		if (messageBox(Application::getApplication().getResourceHandle(),
			IDS_CONFIRM_EMPTYTRASH, MB_YESNO | MB_DEFBUTTON2 | MB_ICONQUESTION, hwnd) != IDYES)
			return;
	}
	
	for (Document::AccountList::const_iterator it = listAccount.begin(); it != listAccount.end(); ++it)
		emptyTrash(*it, pDocument, pSyncManager, pSyncDialogManager, pFolderModel, hwnd, false);
}

void qm::FolderEmptyTrashAction::emptyTrash(Account* pAccount,
											Document* pDocument,
											SyncManager* pSyncManager,
											SyncDialogManager* pSyncDialogManager,
											FolderModel* pFolderModel,
											HWND hwnd,
											bool bConfirm)
{
	NormalFolder* pTrash = getTrash(pAccount);
	if (!pTrash)
		return;
	
	if (bConfirm) {
		if (messageBox(Application::getApplication().getResourceHandle(),
			IDS_CONFIRM_EMPTYTRASH, MB_YESNO | MB_DEFBUTTON2 | MB_ICONQUESTION, hwnd) != IDYES)
			return;
	}
	
	Lock<Account> lock(*pAccount);
	
	const Account::FolderList& l = pAccount->getFolders();
	Account::FolderList listChildren;
	bool bSelected = false;
	for (Account::FolderList::const_iterator it = l.begin(); it != l.end(); ++it) {
		Folder* pFolder = *it;
		if (pFolder->getParentFolder() == pTrash) {
			listChildren.push_back(pFolder);
			if (pFolderModel->getCurrent().second == pFolder)
				bSelected = true;
		}
	}
	if (bSelected)
		pFolderModel->setCurrent(0, pTrash, false);
	for (Account::FolderList::const_iterator it = listChildren.begin(); it != listChildren.end(); ++it) {
		if (!pAccount->removeFolder(*it)) {
			ActionUtil::error(hwnd, IDS_ERROR_EMPTYTRASH);
			return;
		}
	}
	
	if (pTrash->isFlag(Folder::FLAG_LOCAL)) {
		MessageHolderList l(pTrash->getMessages());
		if (!l.empty()) {
			ProgressDialogMessageOperationCallback callback(
				hwnd, IDS_PROGRESS_EMPTYTRASH, IDS_PROGRESS_EMPTYTRASH);
			if (!pAccount->removeMessages(l, pTrash, Account::REMOVEFLAG_DIRECT, &callback, 0, 0)) {
				ActionUtil::error(hwnd, IDS_ERROR_EMPTYTRASH);
				return;
			}
			
			if (!pAccount->save(false)) {
				ActionUtil::error(hwnd, IDS_ERROR_SAVE);
				return;
			}
		}
	}
	else if (pTrash->isFlag(Folder::FLAG_SYNCABLE)) {
		if (!SyncUtil::syncFolder(pSyncManager, pDocument,
			pSyncDialogManager, SyncData::TYPE_ACTIVE, pTrash,
			ReceiveSyncItem::FLAG_EMPTY | ReceiveSyncItem::FLAG_EXPUNGE)) {
			ActionUtil::error(hwnd, IDS_ERROR_EMPTYTRASH);
			return;
		}
	}
}

bool qm::FolderEmptyTrashAction::hasTrash(Account* pAccount)
{
	assert(pAccount);
	
	NormalFolder* pTrash = getTrash(pAccount);
	if (!pTrash)
		return false;
	
	return pTrash->getCount() != 0;
}

Account* qm::FolderEmptyTrashAction::getAccount() const
{
	return FolderActionUtil::getAccount(pFolderModel_);
}

NormalFolder* qm::FolderEmptyTrashAction::getTrash() const
{
	Account* pAccount = getAccount();
	return pAccount ? getTrash(pAccount) : 0;
}

NormalFolder* qm::FolderEmptyTrashAction::getTrash(Account* pAccount)
{
	assert(pAccount);
	return static_cast<NormalFolder*>(
		pAccount->getFolderByBoxFlag(Folder::FLAG_TRASHBOX));
}


/****************************************************************************
 *
 * FolderExpandAction
 *
 */

qm::FolderExpandAction::FolderExpandAction(FolderWindow* pFolderWindow,
										   bool bExpand) :
	pFolderWindow_(pFolderWindow),
	bExpand_(bExpand)
{
}

qm::FolderExpandAction::~FolderExpandAction()
{
}

void qm::FolderExpandAction::invoke(const ActionEvent& event)
{
	pFolderWindow_->expand(bExpand_);
}


/****************************************************************************
 *
 * FolderPropertyAction
 *
 */

qm::FolderPropertyAction::FolderPropertyAction(FolderSelectionModel* pFolderSelectionModel,
											   SyncManager* pSyncManager,
											   HWND hwnd,
											   Profile* pProfile) :
	pFolderSelectionModel_(pFolderSelectionModel),
	pSyncManager_(pSyncManager),
	hwnd_(hwnd),
	pProfile_(pProfile)
{
}

qm::FolderPropertyAction::~FolderPropertyAction()
{
}

void qm::FolderPropertyAction::invoke(const ActionEvent& event)
{
	if (pSyncManager_->isSyncing()) {
		ActionUtil::error(hwnd_, IDS_ERROR_SYNCHRONIZING);
		return;
	}
	
	Account::FolderList l;
	pFolderSelectionModel_->getSelectedFolders(&l);
	openProperty(l, FolderPropertyAction::OPEN_PROPERTY, hwnd_, pProfile_);
}

bool qm::FolderPropertyAction::isEnabled(const ActionEvent& event)
{
	return !pSyncManager_->isSyncing() &&
		pFolderSelectionModel_->hasSelectedFolder();
}

void qm::FolderPropertyAction::openProperty(const Account::FolderList& listFolder,
											Open open,
											HWND hwnd,
											Profile* pProfile)
{
	if (listFolder.empty())
		return;
	
	HINSTANCE hInst = Application::getApplication().getResourceHandle();
	wstring_ptr wstrTitle(loadString(hInst, IDS_TITLE_PROPERTY));
	
	PropertySheetBase sheet(hInst, wstrTitle.get(), false);
	FolderPropertyPage pageProperty(listFolder);
	sheet.add(&pageProperty);
	
	QueryFolder* pQueryFolder = 0;
	std::auto_ptr<FolderConditionPage> pConditionPage;
	if (listFolder.size() == 1 &&
		listFolder.front()->getType() == Folder::TYPE_QUERY) {
		pQueryFolder = static_cast<QueryFolder*>(listFolder.front());
		pConditionPage.reset(new FolderConditionPage(pQueryFolder, pProfile));
		sheet.add(pConditionPage.get());
		if (open == OPEN_CONDITION)
			sheet.setStartPage(1);
	}
	
	std::auto_ptr<FolderParameterPage> pParameterPage;
	if (listFolder.size() == 1 &&
		listFolder.front()->getType() == Folder::TYPE_NORMAL) {
		Folder* pFolder = listFolder.front();
		Account* pAccount = pFolder->getAccount();
		std::pair<const WCHAR**, size_t> params(pAccount->getFolderParamNames(pFolder));
		if (params.second != 0) {
			pParameterPage.reset(new FolderParameterPage(
				pFolder, params.first, params.second));
			sheet.add(pParameterPage.get());
			if (open == OPEN_PARAMETER)
				sheet.setStartPage(1);
		}
	}
	
	sheet.doModal(hwnd);
	
	Account* pAccount = listFolder.front()->getAccount();
	if (!pAccount->save(false)) {
		ActionUtil::error(hwnd, IDS_ERROR_SAVE);
		return;
	}
}


/****************************************************************************
 *
 * FolderRenameAction
 *
 */

qm::FolderRenameAction::FolderRenameAction(FolderSelectionModel* pFolderSelectionModel,
										   SyncManager* pSyncManager,
										   HWND hwnd) :
	pFolderSelectionModel_(pFolderSelectionModel),
	pSyncManager_(pSyncManager),
	hwnd_(hwnd)
{
}

qm::FolderRenameAction::~FolderRenameAction()
{
}

void qm::FolderRenameAction::invoke(const ActionEvent& event)
{
	if (pSyncManager_->isSyncing()) {
		ActionUtil::error(hwnd_, IDS_ERROR_SYNCHRONIZING);
		return;
	}
	
	Folder* pFolder = pFolderSelectionModel_->getFocusedAccountOrFolder().second;
	if (!pFolder)
		return;
	
	RenameDialog dialog(pFolder->getName());
	if (dialog.doModal(hwnd_) != IDOK)
		return;
	
	const WCHAR* pwszName = dialog.getName();
	if (wcscmp(pFolder->getName(), pwszName) == 0)
		return;
	
	Account* pAccount = pFolder->getAccount();
	if (*pwszName == pFolder->getSeparator()) {
		Folder* pParent = 0;
		if (*(pwszName + 1) != L'\0') {
			pParent = pAccount->getFolder(pwszName + 1);
			if (!pParent) {
				ActionUtil::error(hwnd_, IDS_ERROR_MOVEFOLDER);
				return;
			}
		}
		if (!pAccount->moveFolder(pFolder, pParent, 0)) {
			ActionUtil::error(hwnd_, IDS_ERROR_MOVEFOLDER);
			return;
		}
	}
	else {
		if (!pAccount->renameFolder(pFolder, pwszName)) {
			ActionUtil::error(hwnd_, IDS_ERROR_RENAMEFOLDER);
			return;
		}
	}
	
	if (!pAccount->save(false)) {
		ActionUtil::error(hwnd_, IDS_ERROR_SAVE);
		return;
	}
}

bool qm::FolderRenameAction::isEnabled(const ActionEvent& event)
{
	return !pSyncManager_->isSyncing() &&
		pFolderSelectionModel_->getFocusedAccountOrFolder().second != 0;
}


/****************************************************************************
 *
 * FolderShowSizeAction
 *
 */

qm::FolderShowSizeAction::FolderShowSizeAction(FolderListWindow* pFolderListWindow) :
	pFolderListWindow_(pFolderListWindow)
{
}

qm::FolderShowSizeAction::~FolderShowSizeAction()
{
}

void qm::FolderShowSizeAction::invoke(const ActionEvent& event)
{
	pFolderListWindow_->showSize();
}

bool qm::FolderShowSizeAction::isEnabled(const ActionEvent& event)
{
	return pFolderListWindow_->isShow() &&
		!pFolderListWindow_->isSizeShown();
}


/****************************************************************************
 *
 * FolderSubscribeAction
 *
 */

qm::FolderSubscribeAction::FolderSubscribeAction(Document* pDocument,
												 PasswordManager* pPasswordManager,
												 FolderSelectionModel* pFolderSelectionModel,
												 SyncManager* pSyncManager,
												 HWND hwnd) :
	pDocument_(pDocument),
	pPasswordManager_(pPasswordManager),
	pFolderSelectionModel_(pFolderSelectionModel),
	pSyncManager_(pSyncManager),
	hwnd_(hwnd)
{
}

qm::FolderSubscribeAction::~FolderSubscribeAction()
{
}

void qm::FolderSubscribeAction::invoke(const ActionEvent& event)
{
	if (pSyncManager_->isSyncing()) {
		ActionUtil::error(hwnd_, IDS_ERROR_SYNCHRONIZING);
		return;
	}
	
	std::pair<Account*, Folder*> p(pFolderSelectionModel_->getFocusedAccountOrFolder());
	if (!p.first && !p.second)
		return;
	
	Account* pAccount = p.first ? p.first : p.second->getAccount();
	subscribe(pDocument_, pAccount, p.second, pPasswordManager_, hwnd_, 0);
	
	if (!pAccount->save(false)) {
		ActionUtil::error(hwnd_, IDS_ERROR_SAVE);
		return;
	}
}

bool qm::FolderSubscribeAction::isEnabled(const ActionEvent& event)
{
	if (pSyncManager_->isSyncing())
		return false;
	
	std::pair<Account*, Folder*> p(pFolderSelectionModel_->getFocusedAccountOrFolder());
	if (!p.first && !p.second)
		return false;
	
	Folder* pFolder = p.second;
	Account* pAccount = p.first ? p.first : pFolder->getAccount();
	std::auto_ptr<ReceiveSessionUI> pReceiveUI(
		ReceiveSessionFactory::getUI(pAccount->getType(Account::HOST_RECEIVE)));
	return pReceiveUI->canSubscribe(pAccount, pFolder);
}

wstring_ptr qm::FolderSubscribeAction::getText(const ActionEvent& event)
{
	wstring_ptr wstrText;
	
	std::pair<Account*, Folder*> p(pFolderSelectionModel_->getFocusedAccountOrFolder());
	if (p.first || p.second) {
		Account* pAccount = p.first ? p.first : p.second->getAccount();
		std::auto_ptr<ReceiveSessionUI> pReceiveUI(
			ReceiveSessionFactory::getUI(pAccount->getType(Account::HOST_RECEIVE)));
		wstrText = pReceiveUI->getSubscribeText();
	}
	
	if (!wstrText.get())
		wstrText = loadString(Application::getApplication().getResourceHandle(), IDS_ACTION_SUBSCRIBE);
	
	return wstrText;
}

void qm::FolderSubscribeAction::subscribe(Document* pDocument,
										  Account* pAccount,
										  Folder* pFolder,
										  PasswordManager* pPasswordManager,
										  HWND hwnd,
										  void* pParam)
{
	std::auto_ptr<ReceiveSessionUI> pReceiveUI(
		ReceiveSessionFactory::getUI(pAccount->getType(Account::HOST_RECEIVE)));
	DefaultPasswordCallback callback(pPasswordManager);
	pReceiveUI->subscribe(pDocument, pAccount, pFolder, &callback, hwnd, pParam);
}


/****************************************************************************
 *
 * FolderUpdateAction
 *
 */

qm::FolderUpdateAction::FolderUpdateAction(FolderModel* pFolderModel,
										   SyncManager* pSyncManager,
										   HWND hwnd) :
	pFolderModel_(pFolderModel),
	pSyncManager_(pSyncManager),
	hwnd_(hwnd)
{
}

qm::FolderUpdateAction::~FolderUpdateAction()
{
}

void qm::FolderUpdateAction::invoke(const ActionEvent& event)
{
	if (pSyncManager_->isSyncing()) {
		ActionUtil::error(hwnd_, IDS_ERROR_SYNCHRONIZING);
		return;
	}
	
	Account* pAccount = FolderActionUtil::getAccount(pFolderModel_);
	if (!pAccount)
		return;
	
	pFolderModel_->setCurrent(pAccount, 0, false);
	
	// TODO
	// Show progress dialog box?
	if (!pAccount->updateFolders()) {
		ActionUtil::error(hwnd_, IDS_ERROR_UPDATEFOLDER);
		return;
	}
	
	if (!pAccount->save(false)) {
		ActionUtil::error(hwnd_, IDS_ERROR_SAVE);
		return;
	}
}

bool qm::FolderUpdateAction::isEnabled(const ActionEvent& event)
{
	if (pSyncManager_->isSyncing())
		return false;
	
	Account* pAccount = FolderActionUtil::getAccount(pFolderModel_);
	return pAccount && pAccount->isSupport(Account::SUPPORT_REMOTEFOLDER);
}


/****************************************************************************
 *
 * HelpAboutAction
 *
 */

qm::HelpAboutAction::HelpAboutAction(HWND hwnd) :
	hwnd_(hwnd)
{
}

qm::HelpAboutAction::~HelpAboutAction()
{
}

void qm::HelpAboutAction::invoke(const qs::ActionEvent& event)
{
	AboutDialog dialog;
	dialog.doModal(hwnd_);
}


/****************************************************************************
 *
 * HelpCheckUpdateAction
 *
 */

qm::HelpCheckUpdateAction::HelpCheckUpdateAction(UpdateChecker* pUpdateChecker,
												 HWND hwnd) :
	pUpdateChecker_(pUpdateChecker),
	hwnd_(hwnd)
{
}

qm::HelpCheckUpdateAction::~HelpCheckUpdateAction()
{
}

void qm::HelpCheckUpdateAction::invoke(const qs::ActionEvent& event)
{
	HINSTANCE hInst = Application::getApplication().getResourceHandle();
	switch (pUpdateChecker_->checkUpdate()) {
	case UpdateChecker::UPDATE_UPDATED:
		if (messageBox(hInst, IDS_CONFIRM_UPDATE, MB_YESNO, hwnd_) == IDYES)
			UIUtil::openURL(L"http://q3.snak.org/download/", hwnd_);
		break;
	case UpdateChecker::UPDATE_LATEST:
		messageBox(hInst, IDS_MESSAGE_UPDATED, hwnd_);
		break;
	case UpdateChecker::UPDATE_ERROR:
		ActionUtil::error(hwnd_, IDS_ERROR_CHECKUPDATE);
		break;
	default:
		assert(false);
		break;
	}
}


/****************************************************************************
 *
 * HelpOpenURLAction
 *
 */

qm::HelpOpenURLAction::HelpOpenURLAction(HWND hwnd) :
	hwnd_(hwnd)
{
}

qm::HelpOpenURLAction::~HelpOpenURLAction()
{
}

void qm::HelpOpenURLAction::invoke(const qs::ActionEvent& event)
{
	const WCHAR* pwszURL = ActionParamUtil::getString(event.getParam(), 0);
	if (!pwszURL)
		return;
	
	UIUtil::openURL(pwszURL, hwnd_);
}


/****************************************************************************
 *
 * MessageApplyRuleAction
 *
 */

qm::MessageApplyRuleAction::MessageApplyRuleAction(RuleManager* pRuleManager,
												   UndoManager* pUndoManager,
												   ViewModelManager* pViewModelManager,
												   bool bAll,
												   SecurityModel* pSecurityModel,
												   Document* pDocument,
												   HWND hwnd,
												   Profile* pProfile) :
	pRuleManager_(pRuleManager),
	pUndoManager_(pUndoManager),
	pViewModelManager_(pViewModelManager),
	pMessageSelectionModel_(0),
	bAll_(bAll),
	pSecurityModel_(pSecurityModel),
	pDocument_(pDocument),
	hwnd_(hwnd),
	pProfile_(pProfile)
{
}

qm::MessageApplyRuleAction::MessageApplyRuleAction(RuleManager* pRuleManager,
												   UndoManager* pUndoManager,
												   MessageSelectionModel* pMessageSelectionModel,
												   SecurityModel* pSecurityModel,
												   Document* pDocument,
												   HWND hwnd,
												   Profile* pProfile) :
	pRuleManager_(pRuleManager),
	pUndoManager_(pUndoManager),
	pViewModelManager_(0),
	pMessageSelectionModel_(pMessageSelectionModel),
	bAll_(false),
	pSecurityModel_(pSecurityModel),
	pDocument_(pDocument),
	hwnd_(hwnd),
	pProfile_(pProfile)
{
}

qm::MessageApplyRuleAction::~MessageApplyRuleAction()
{
}

void qm::MessageApplyRuleAction::invoke(const ActionEvent& event)
{
	Account* pAccount = 0;
	if (!applyRule(&pAccount)) {
		ActionUtil::error(hwnd_, IDS_ERROR_APPLYRULE);
		return;
	}
	
	if (pAccount && !pAccount->save(false))
		ActionUtil::error(hwnd_, IDS_ERROR_SAVE);
}

bool qm::MessageApplyRuleAction::isEnabled(const ActionEvent& event)
{
	if (pViewModelManager_) {
		if (bAll_)
			return pViewModelManager_->getCurrentAccount() != 0;
		else
			return pViewModelManager_->getCurrentViewModel() != 0;
	}
	else {
		return pMessageSelectionModel_->hasSelectedMessage();
	}
}

bool qm::MessageApplyRuleAction::applyRule(Account** ppAccount) const
{
	assert(ppAccount);
	
	*ppAccount = 0;
	
	struct RuleCallbackImpl : public RuleCallback
	{
		RuleCallbackImpl(ProgressDialog* pProgressDialog) :
			pDialog_(pProgressDialog)
		{
		}
		
		virtual ~RuleCallbackImpl()
		{
		}
		
		virtual bool isCanceled()
		{
			return pDialog_->isCanceled();
		}
		
		virtual void checkingMessages(Folder* pFolder)
		{
			wstring_ptr wstrMessage(getMessage(IDS_MESSAGE_CHECKMESSAGES, pFolder));
			pDialog_->setMessage(wstrMessage.get());
		}
		
		virtual void applyingRule(Folder* pFolder)
		{
			wstring_ptr wstrMessage(getMessage(IDS_MESSAGE_APPLYRULE, pFolder));
			pDialog_->setMessage(wstrMessage.get());
		}
		
		virtual void setRange(size_t nMin,
							  size_t nMax)
		{
			pDialog_->setRange(nMin, nMax);
		}
		
		virtual void setPos(size_t nPos)
		{
			pDialog_->setPos(nPos);
		}
		
		wstring_ptr getMessage(UINT nId,
							   Folder* pFolder)
		{
			HINSTANCE hInst = Application::getApplication().getResourceHandle();
			wstring_ptr wstrMessage(loadString(hInst, nId));
			wstring_ptr wstrName(pFolder->getFullName());
			return concat(wstrMessage.get(), L" : ", wstrName.get());
		}
		
		ProgressDialog* pDialog_;
	};
	
	ProgressDialog dialog;
	RuleCallbackImpl callback(&dialog);
	
	Account* pAccount = 0;
	UndoItemList undo;
	if (pViewModelManager_) {
		if (bAll_) {
			pAccount = pViewModelManager_->getCurrentAccount();
			if (pAccount) {
				Lock<Account> lock(*pAccount);
				
				Account::FolderList l(pAccount->getFolders());
				std::sort(l.begin(), l.end(), FolderLess());
				
				ProgressDialogInit init(&dialog, hwnd_, IDS_PROGRESS_APPLYRULES);
				for (Account::FolderList::const_iterator it = l.begin(); it != l.end(); ++it) {
					Folder* pFolder = *it;
					if (pFolder->getType() == Folder::TYPE_NORMAL &&
						!pFolder->isHidden()) {
						if (!pRuleManager_->applyManual(pFolder, pDocument_, hwnd_, pProfile_,
							pSecurityModel_->getSecurityMode(), &undo, &callback))
							return false;
					}
				}
			}
		}
		else {
			ViewModel* pViewModel = pViewModelManager_->getCurrentViewModel();
			if (pViewModel) {
				Lock<ViewModel> lock(*pViewModel);
				
				Folder* pFolder = pViewModel->getFolder();
				
				unsigned int nCount = pViewModel->getCount();
				if (nCount != 0) {
					MessageHolderList l;
					l.resize(nCount);
					for (unsigned int n = 0; n < pViewModel->getCount(); ++n)
						l[n] = pViewModel->getMessageHolder(n);
					
					ProgressDialogInit init(&dialog, hwnd_, IDS_PROGRESS_APPLYRULES);
					if (!pRuleManager_->applyManual(pFolder, l, pDocument_, hwnd_, pProfile_,
						pSecurityModel_->getSecurityMode(), &undo, &callback))
						return false;
					pAccount = pFolder->getAccount();
				}
			}
		}
	}
	else {
		AccountLock lock;
		Folder* pFolder = 0;
		MessageHolderList l;
		pMessageSelectionModel_->getSelectedMessages(&lock, &pFolder, &l);
		if (!l.empty()) {
			ProgressDialogInit init(&dialog, hwnd_, IDS_PROGRESS_APPLYRULES);
			if (!pRuleManager_->applyManual(pFolder, l, pDocument_, hwnd_, pProfile_,
				pSecurityModel_->getSecurityMode(), &undo, &callback))
				return false;
			pAccount = lock.get();
		}
	}
	pUndoManager_->pushUndoItem(undo.getUndoItem());
	
	*ppAccount = pAccount;
	
	return true;
}


/****************************************************************************
 *
 * MessageCertificateAction
 *
 */

qm::MessageCertificateAction::MessageCertificateAction(MessageWindow* pMessageWindow) :
	pMessageWindow_(pMessageWindow)
{
}

qm::MessageCertificateAction::~MessageCertificateAction()
{
}

void qm::MessageCertificateAction::invoke(const ActionEvent& event)
{
	const WCHAR* pwszCertificate = pMessageWindow_->getCertificate();
	if (!pwszCertificate)
		return;
	
	CertificateDialog dialog(pwszCertificate);
	dialog.doModal(pMessageWindow_->getParentFrame());
}

bool qm::MessageCertificateAction::isEnabled(const ActionEvent& event)
{
	return pMessageWindow_->getCertificate() != 0;
}


/****************************************************************************
 *
 * MessageClearRecentsAction
 *
 */

qm::MessageClearRecentsAction::MessageClearRecentsAction(Recents* pRecents) :
	pRecents_(pRecents)
{
}

qm::MessageClearRecentsAction::~MessageClearRecentsAction()
{
}

void qm::MessageClearRecentsAction::invoke(const ActionEvent& event)
{
	pRecents_->clear();
}


/****************************************************************************
 *
 * MessageCombineAction
 *
 */

qm::MessageCombineAction::MessageCombineAction(MessageSelectionModel* pMessageSelectionModel,
											   SecurityModel* pSecurityModel,
											   UndoManager* pUndoManager,
											   HWND hwnd) :
	pMessageSelectionModel_(pMessageSelectionModel),
	pSecurityModel_(pSecurityModel),
	pUndoManager_(pUndoManager),
	hwnd_(hwnd)
{
}

qm::MessageCombineAction::~MessageCombineAction()
{
}

void qm::MessageCombineAction::invoke(const ActionEvent& event)
{
	AccountLock lock;
	MessageHolderList l;
	pMessageSelectionModel_->getSelectedMessages(&lock, 0, &l);
	
	Account* pAccount = lock.get();
	if (!l.empty()) {
		Message msg;
		if (!combine(l, &msg)) {
			ActionUtil::error(hwnd_, IDS_ERROR_COMBINE);
			return;
		}
		
		// TODO
		// Which folder should I put a new message?
		NormalFolder* pFolder = l.front()->getFolder();
		unsigned int nFlags = 0;
		UndoItemList undo;
		if (!pAccount->appendMessage(pFolder, msg, nFlags,
			0, Account::OPFLAG_NONE, &undo, 0)) {
			ActionUtil::error(hwnd_, IDS_ERROR_COMBINE);
			return;
		}
		pUndoManager_->pushUndoItem(undo.getUndoItem());
	}
}

bool qm::MessageCombineAction::isEnabled(const ActionEvent& event)
{
	return pMessageSelectionModel_->hasSelectedMessage();
}

bool qm::MessageCombineAction::combine(const MessageHolderList& l,
									   Message* pMessage)
{
	assert(pMessage);
	
	MessageHolderList listMessageHolder;
	listMessageHolder.resize(l.size());
	
	wstring_ptr wstrIdAll;
	unsigned int nTotal = 0;
	for (MessageHolderList::const_iterator it = l.begin(); it != l.end(); ++it) {
		MessageHolder* pmh = *it;
		
		Message msg;
		if (!pmh->getMessage(Account::GETMESSAGEFLAG_HEADER,
			L"Content-Type", pSecurityModel_->getSecurityMode(), &msg))
			return false;
		
		const ContentTypeParser* pContentType = msg.getContentType();
		if (!PartUtil::isContentType(pContentType, L"message", L"partial"))
			return false;
		
		wstring_ptr wstrId(pContentType->getParameter(L"id"));
		if (!wstrId.get())
			return false;
		else if (!wstrIdAll.get())
			wstrIdAll = wstrId;
		else if (wcscmp(wstrId.get(), wstrIdAll.get()) != 0)
			return false;
		
		if (nTotal == 0) {
			wstring_ptr wstrTotal(pContentType->getParameter(L"total"));
			if (wstrTotal.get()) {
				WCHAR* pEnd = 0;
				nTotal = wcstol(wstrTotal.get(), &pEnd, 10);
				if (*pEnd || nTotal != l.size())
					return false;
			}
		}
		
		wstring_ptr wstrNumber(pContentType->getParameter(L"number"));
		WCHAR* pEnd = 0;
		unsigned int nNumber = wcstol(wstrNumber.get(), &pEnd, 10);
		if (*pEnd || nNumber == 0 || nNumber > l.size())
			return false;
		if (listMessageHolder[nNumber - 1])
			return false;
		listMessageHolder[nNumber - 1] = *it;
	}
	if (nTotal == 0)
		return false;
	
	XStringBuffer<XSTRING> buf;
	
	Part::FieldList listField;
	Part::FieldListFree free(listField);
	
	for (MessageHolderList::const_iterator it = listMessageHolder.begin(); it != listMessageHolder.end(); ++it) {
		MessageHolder* pmh = *it;
		
		Message msg;
		if (!pmh->getMessage(Account::GETMESSAGEFLAG_ALL,
			0, pSecurityModel_->getSecurityMode(), &msg))
			return false;
		
		if (it == listMessageHolder.begin())
			msg.getFields(&listField);
		
		if (!buf.append(msg.getBody()))
			return false;
	}
	
	if (!pMessage->create(buf.getCharArray(), buf.getLength(), Message::FLAG_NONE))
		return false;
	buf.remove();
	
	for (Part::FieldList::const_iterator itF = listField.begin(); itF != listField.end(); ++itF) {
		if (!isSpecialField((*itF).first)) {
			if (!buf.append((*itF).second) || !buf.append("\r\n"))
				return false;
		}
	}
	
	free.free();
	pMessage->getFields(&listField);
	for (Part::FieldList::const_iterator itF = listField.begin(); itF != listField.end(); ++itF) {
		if (isSpecialField((*itF).first)) {
			if (!buf.append((*itF).second) || !buf.append("\r\n"))
				return false;
		}
	}
	
	if (!pMessage->setHeader(buf.getCharArray()))
		return false;
	
	return true;
}

bool qm::MessageCombineAction::isSpecialField(const CHAR* pszField)
{
	return strncmp(pszField, "content-", 8) == 0 ||
		strcmp(pszField, "subject") == 0 ||
		strcmp(pszField, "message-id") == 0 ||
		strcmp(pszField, "encrypted") == 0 ||
		strcmp(pszField, "mime-version") == 0;
}


/****************************************************************************
 *
 * MessageCreateAction
 *
 */

qm::MessageCreateAction::MessageCreateAction(Document* pDocument,
											 FolderModelBase* pFolderModel,
											 MessageSelectionModel* pMessageSelectionModel,
											 EncodingModel* pEncodingModel,
											 SecurityModel* pSecurityModel,
											 EditFrameWindowManager* pEditFrameWindowManager,
											 ExternalEditorManager* pExternalEditorManager,
											 HWND hwnd,
											 Profile* pProfile,
											 bool bExternalEditor) :
	processor_(pDocument, pFolderModel, pMessageSelectionModel,
		pEncodingModel, pSecurityModel, pEditFrameWindowManager,
		pExternalEditorManager, hwnd, pProfile, bExternalEditor,
		Application::getApplication().getTemporaryFolder()),
	pFolderModel_(pFolderModel),
	hwnd_(hwnd)
{
}

qm::MessageCreateAction::~MessageCreateAction()
{
}

void qm::MessageCreateAction::invoke(const ActionEvent& event)
{
	const WCHAR* pwszTemplate = ActionParamUtil::getString(event.getParam(), 0);
	if (!pwszTemplate)
		return;
	
	if (!processor_.process(pwszTemplate,
		(event.getModifier() & ActionEvent::MODIFIER_SHIFT) != 0)) {
		ActionUtil::error(hwnd_, IDS_ERROR_CREATEMESSAGE);
		return;
	}
}

bool qm::MessageCreateAction::isEnabled(const ActionEvent& event)
{
	if (!ActionParamUtil::getString(event.getParam(), 0))
		return false;
	
	std::pair<Account*, Folder*> p(pFolderModel_->getCurrent());
	return p.first || p.second;
}


/****************************************************************************
 *
 * MessageCreateFromClipboardAction
 *
 */

qm::MessageCreateFromClipboardAction::MessageCreateFromClipboardAction(bool bDraft,
																	   Document* pDocument,
																	   PasswordManager* pPasswordManager,
																	   Profile* pProfile,
																	   HWND hwnd,
																	   FolderModel* pFolderModel,
																	   SecurityModel* pSecurityModel) :
	composer_(bDraft, pDocument, pPasswordManager, pProfile, hwnd, pFolderModel, pSecurityModel),
	pDocument_(pDocument),
	pSecurityModel_(pSecurityModel),
	hwnd_(hwnd)
{
}

qm::MessageCreateFromClipboardAction::~MessageCreateFromClipboardAction()
{
}

void qm::MessageCreateFromClipboardAction::invoke(const ActionEvent& event)
{
	wstring_ptr wstrMessage(Clipboard::getText());
	if (!wstrMessage.get())
		return;
	
	if (!composer_.compose(wstrMessage.get(), -1, MESSAGESECURITY_NONE)) {
		ActionUtil::error(hwnd_, IDS_ERROR_CREATEMESSAGE);
		return;
	}
}

bool qm::MessageCreateFromClipboardAction::isEnabled(const ActionEvent& event)
{
	return Clipboard::isFormatAvailable(Clipboard::CF_QSTEXT);
}


/****************************************************************************
 *
 * MessageCreateFromFileAction
 *
 */

qm::MessageCreateFromFileAction::MessageCreateFromFileAction(bool bDraft,
															 Document* pDocument,
															 PasswordManager* pPasswordManager,
															 Profile* pProfile,
															 HWND hwnd,
															 FolderModel* pFolderModel,
															 SecurityModel* pSecurityModel) :
	composer_(bDraft, pDocument, pPasswordManager, pProfile, hwnd, pFolderModel, pSecurityModel),
	pDocument_(pDocument),
	pSecurityModel_(pSecurityModel),
	hwnd_(hwnd)
{
}

qm::MessageCreateFromFileAction::~MessageCreateFromFileAction()
{
}

void qm::MessageCreateFromFileAction::invoke(const ActionEvent& event)
{
	const WCHAR* pwszPath = ActionParamUtil::getString(event.getParam(), 0);
	if (!pwszPath)
		return;
	
	if (!composer_.compose(pwszPath, MESSAGESECURITY_NONE)) {
		ActionUtil::error(hwnd_, IDS_ERROR_CREATEMESSAGE);
		return;
	}
}


/****************************************************************************
 *
 * MessageDeleteAttachmentAction
 *
 */

qm::MessageDeleteAttachmentAction::MessageDeleteAttachmentAction(MessageSelectionModel* pMessageSelectionModel,
																 SecurityModel* pSecurityModel,
																 UndoManager* pUndoManager,
																 HWND hwnd) :
	pMessageSelectionModel_(pMessageSelectionModel),
	pSecurityModel_(pSecurityModel),
	pUndoManager_(pUndoManager),
	hwnd_(hwnd)
{
}

qm::MessageDeleteAttachmentAction::~MessageDeleteAttachmentAction()
{
}

void qm::MessageDeleteAttachmentAction::invoke(const ActionEvent& event)
{
	AccountLock lock;
	Folder* pFolder = 0;
	MessageHolderList l;
	pMessageSelectionModel_->getSelectedMessages(&lock, &pFolder, &l);
	
	if (l.empty())
		return;
	
	Account* pAccount = lock.get();
	if (!deleteAttachment(pAccount, pFolder, l)) {
		ActionUtil::error(hwnd_, IDS_ERROR_DELETEATTACHMENT);
		return;
	}
}

bool qm::MessageDeleteAttachmentAction::isEnabled(const ActionEvent& event)
{
	return pMessageSelectionModel_->hasSelectedMessage();
}

bool qm::MessageDeleteAttachmentAction::deleteAttachment(Account* pAccount,
														 Folder* pFolder,
														 const MessageHolderList& l) const
{
	UndoItemList undo;
	for (MessageHolderList::const_iterator it = l.begin(); it != l.end(); ++it) {
		if (!deleteAttachment(pAccount, pFolder, *it, &undo))
			return false;
	}
	pUndoManager_->pushUndoItem(undo.getUndoItem());
	
	return true;
}

bool qm::MessageDeleteAttachmentAction::deleteAttachment(Account* pAccount,
														 Folder* pFolder,
														 MessageHolder* pmh,
														 UndoItemList* pUndoItemList) const
{
	Message msg;
	if (!pmh->getMessage(Account::GETMESSAGEFLAG_ALL,
		0, pSecurityModel_->getSecurityMode(), &msg))
		return false;
	
	AttachmentParser::removeAttachments(&msg);
	AttachmentParser::setAttachmentDeleted(&msg);
	
	NormalFolder* pNormalFolder = pmh->getFolder();
	unsigned int nFlags = pmh->getFlags() & MessageHolder::FLAG_USER_MASK;
	wstring_ptr wstrLabel(pmh->getLabel());
	if (!pAccount->appendMessage(pNormalFolder, msg, nFlags,
		wstrLabel.get(), Account::OPFLAG_NONE, pUndoItemList, 0))
		return false;
	
	if (!pAccount->removeMessages(MessageHolderList(1, pmh),
		pFolder, Account::OPFLAG_ACTIVE, 0, pUndoItemList, 0))
		return false;
	
	return true;
}


/****************************************************************************
 *
 * MessageDetachAction
 *
 */

qm::MessageDetachAction::MessageDetachAction(Profile* pProfile,
											 MessageSelectionModel* pMessageSelectionModel,
											 SecurityModel* pSecurityModel,
											 HWND hwnd) :
	pMessageSelectionModel_(pMessageSelectionModel),
	helper_(pSecurityModel, pProfile, 0, hwnd),
	hwnd_(hwnd)
{
}

qm::MessageDetachAction::~MessageDetachAction()
{
}

void qm::MessageDetachAction::invoke(const ActionEvent& event)
{
	AccountLock lock;
	MessageHolderList l;
	pMessageSelectionModel_->getSelectedMessages(&lock, 0, &l);
	
	if (!l.empty()) {
		if (helper_.detach(l, 0) == AttachmentParser::RESULT_FAIL) {
			ActionUtil::error(hwnd_, IDS_ERROR_DETACHATTACHMENT);
			return;
		}
	}
}

bool qm::MessageDetachAction::isEnabled(const ActionEvent& event)
{
	return pMessageSelectionModel_->hasSelectedMessage();
}


/****************************************************************************
 *
 * MessageExpandDigestAction
 *
 */

qm::MessageExpandDigestAction::MessageExpandDigestAction(MessageSelectionModel* pMessageSelectionModel,
														 SecurityModel* pSecurityModel,
														 UndoManager* pUndoManager,
														 HWND hwnd) :
	pMessageSelectionModel_(pMessageSelectionModel),
	pSecurityModel_(pSecurityModel),
	pUndoManager_(pUndoManager),
	hwnd_(hwnd)
{
}

qm::MessageExpandDigestAction::~MessageExpandDigestAction()
{
}

void qm::MessageExpandDigestAction::invoke(const ActionEvent& event)
{
	AccountLock lock;
	MessageHolderList l;
	pMessageSelectionModel_->getSelectedMessages(&lock, 0, &l);
	
	if (l.empty())
		return;
	
	Account* pAccount = lock.get();
	if (!expandDigest(pAccount, l)) {
		ActionUtil::error(hwnd_, IDS_ERROR_EXPANDDIGEST);
		return;
	}
}

bool qm::MessageExpandDigestAction::isEnabled(const ActionEvent& event)
{
	return pMessageSelectionModel_->hasSelectedMessage();
}

bool qm::MessageExpandDigestAction::expandDigest(Account* pAccount,
												 const MessageHolderList& l)
{
	UndoItemList undo;
	for (MessageHolderList::const_iterator it = l.begin(); it != l.end(); ++it) {
		if (!expandDigest(pAccount, *it, &undo))
			return false;
	}
	pUndoManager_->pushUndoItem(undo.getUndoItem());
	
	return true;
}

bool qm::MessageExpandDigestAction::expandDigest(Account* pAccount,
												 MessageHolder* pmh,
												 UndoItemList* pUndoItemList)
{
	Message msg;
	if (!pmh->getMessage(Account::GETMESSAGEFLAG_ALL, 0, pSecurityModel_->getSecurityMode(), &msg))
		return false;
	
	PartUtil::MessageList l;
	struct Deleter
	{
		Deleter(PartUtil::MessageList& l) :
			l_(l)
		{
		}
		
		~Deleter()
		{
			std::for_each(l_.begin(), l_.end(), qs::deleter<Message>());
		}
		
		PartUtil::MessageList& l_;
	} deleter(l);
	if (!PartUtil(msg).getDigest(&l))
		return false;
	
	for (PartUtil::MessageList::const_iterator it = l.begin(); it != l.end(); ++it) {
		// TODO
		// Set flags and label?
		unsigned int nFlags = 0;
		if (!pAccount->appendMessage(pmh->getFolder(), **it,
			nFlags, 0, Account::OPFLAG_NONE, pUndoItemList, 0))
			return false;
	}
	
	return true;
}


/****************************************************************************
 *
 * MessageLabelAction
 *
 */

qm::MessageLabelAction::MessageLabelAction(MessageSelectionModel* pModel,
										   UndoManager* pUndoManager,
										   Profile* pProfile,
										   HWND hwnd) :
	pModel_(pModel),
	pUndoManager_(pUndoManager),
	pProfile_(pProfile),
	hwnd_(hwnd)
{
}

qm::MessageLabelAction::~MessageLabelAction()
{
}

void qm::MessageLabelAction::invoke(const ActionEvent& event)
{
	AccountLock lock;
	MessageHolderList l;
	pModel_->getSelectedMessages(&lock, 0, &l);
	
	if (l.empty())
		return;
	
	wstring_ptr wstrLabel;
	Util::LabelType type = Util::LABELTYPE_SET;
	
	const WCHAR* pwszLabel = ActionParamUtil::getString(event.getParam(), 0);
	if (pwszLabel) {
		if (*pwszLabel == L'=') {
			type = Util::LABELTYPE_SET;
			wstrLabel = allocWString(pwszLabel + 1);
		}
		else if (*pwszLabel == L'+') {
			type = Util::LABELTYPE_ADD;
			wstrLabel = allocWString(pwszLabel + 1);
		}
		else if (*pwszLabel == L'-') {
			type = Util::LABELTYPE_REMOVE;
			wstrLabel = allocWString(pwszLabel + 1);
		}
		else {
			type = Util::LABELTYPE_SET;
			wstrLabel = allocWString(pwszLabel);
		}
	}
	else {
		wstring_ptr wstrOldLabel;
		MessagePtrLock mpl(pModel_->getFocusedMessage());
		if (mpl)
			wstrOldLabel = mpl->getLabel();
		
		LabelDialog dialog(wstrOldLabel.get(), pProfile_);
		if (dialog.doModal(hwnd_) != IDOK)
			return;
		wstrLabel = allocWString(dialog.getLabel());
	}
	
	Account* pAccount = lock.get();
	UndoItemList undo;
	if (!Util::setMessagesLabel(pAccount, l, type, wstrLabel.get(), &undo)) {
		ActionUtil::error(hwnd_, IDS_ERROR_LABELMESSAGE);
		return;
	}
	pUndoManager_->pushUndoItem(undo.getUndoItem());
}

bool qm::MessageLabelAction::isEnabled(const ActionEvent& event)
{
	return pModel_->hasSelectedMessage();
}


/****************************************************************************
 *
 * MessageMacroAction
 *
 */

qm::MessageMacroAction::MessageMacroAction(MessageSelectionModel* pMessageSelectionModel,
										   SecurityModel* pSecurityModel,
										   Document* pDocument,
										   Profile* pProfile,
										   HWND hwnd) :
	pMessageSelectionModel_(pMessageSelectionModel),
	pSecurityModel_(pSecurityModel),
	pDocument_(pDocument),
	pProfile_(pProfile),
	hwnd_(hwnd)
{
}

qm::MessageMacroAction::~MessageMacroAction()
{
}

void qm::MessageMacroAction::invoke(const ActionEvent& event)
{
	AccountLock lock;
	Folder* pFolder = 0;
	MessageHolderList l;
	pMessageSelectionModel_->getSelectedMessages(&lock, &pFolder, &l);
	if (l.empty())
		return;
	
	const WCHAR* pwszMacro = ActionParamUtil::getString(event.getParam(), 0);
	wstring_ptr wstrMacro;
	if (!pwszMacro) {
		HINSTANCE hInst = Application::getApplication().getResourceHandle();
		wstring_ptr wstrTitle(loadString(hInst, IDS_EXECUTEMACRO));
		wstring_ptr wstrMessage(loadString(hInst, IDS_MACRO));
		wstring_ptr wstrPrevMacro(pProfile_->getString(L"Global", L"Macro"));
		
		MultiLineInputBoxDialog dialog(wstrTitle.get(), wstrMessage.get(),
			wstrPrevMacro.get(), false, pProfile_, L"MacroDialog");
		if (dialog.doModal(hwnd_) != IDOK)
			return;
		
		wstrMacro = allocWString(dialog.getValue());
		pwszMacro = wstrMacro.get();
		pProfile_->setString(L"Global", L"Macro", pwszMacro);
	}
	
	MacroParser parser;
	std::auto_ptr<Macro> pMacro(parser.parse(pwszMacro));
	if (!pMacro.get()) {
		ActionUtil::error(hwnd_, IDS_ERROR_INVALIDMACRO);
		return;
	}
	
	MacroVariableHolder globalVariable;
	for (MessageHolderList::const_iterator it = l.begin(); it != l.end(); ++it) {
		Message msg;
		MacroContext context(*it, &msg, lock.get(), l, pFolder, pDocument_, hwnd_, pProfile_, 0,
			MacroContext::FLAG_UI | MacroContext::FLAG_UITHREAD | MacroContext::FLAG_MODIFY,
			pSecurityModel_->getSecurityMode(), 0, &globalVariable);
		MacroValuePtr pValue(pMacro->value(&context));
		if (!pValue.get() && context.getReturnType() == MacroContext::RETURNTYPE_NONE) {
			ActionUtil::error(hwnd_, IDS_ERROR_EVALUATEMACRO);
			return;
		}
	}
}

bool qm::MessageMacroAction::isEnabled(const ActionEvent& event)
{
	return pMessageSelectionModel_->hasSelectedMessage();
}


/****************************************************************************
 *
 * MessageManageJunkAction
 *
 */

qm::MessageManageJunkAction::MessageManageJunkAction(MessageSelectionModel* pMessageSelectionModel,
													 JunkFilter* pJunkFilter,
													 JunkFilter::Operation operation,
													 HWND hwnd) :
	pMessageSelectionModel_(pMessageSelectionModel),
	pJunkFilter_(pJunkFilter),
	operation_(operation),
	hwnd_(hwnd)
{
}

qm::MessageManageJunkAction::~MessageManageJunkAction()
{
}

void qm::MessageManageJunkAction::invoke(const ActionEvent& event)
{
	AccountLock lock;
	MessageHolderList l;
	pMessageSelectionModel_->getSelectedMessages(&lock, 0, &l);
	
	ProgressDialog progressDialog;
	ProgressDialogInit init(&progressDialog, hwnd_,
		IDS_PROGRESS_PROCESS, IDS_PROGRESS_PROCESS, 0, l.size(), 0);
	
	for (MessageHolderList::size_type n = 0; n < l.size(); ++n) {
		JunkFilterUtil::manage(pJunkFilter_, l[n], operation_);
		if (progressDialog.isCanceled())
			break;
		progressDialog.setPos(n);
	}
}

bool qm::MessageManageJunkAction::isEnabled(const ActionEvent& event)
{
	return pJunkFilter_ != 0;
}


/****************************************************************************
 *
 * MessageMarkAction
 *
 */

qm::MessageMarkAction::MessageMarkAction(MessageSelectionModel* pModel,
										 unsigned int nFlags,
										 unsigned int nMask,
										 UndoManager* pUndoManager,
										 HWND hwnd) :
	pModel_(pModel),
	nFlags_(nFlags),
	nMask_(nMask),
	pUndoManager_(pUndoManager),
	hwnd_(hwnd)
{
}

qm::MessageMarkAction::~MessageMarkAction()
{
}

void qm::MessageMarkAction::invoke(const ActionEvent& event)
{
	AccountLock lock;
	MessageHolderList l;
	pModel_->getSelectedMessages(&lock, 0, &l);
	
	if (l.empty())
		return;
	
	Account* pAccount = lock.get();
	UndoItemList undo;
	if (!pAccount->setMessagesFlags(l, nFlags_, nMask_, &undo)) {
		ActionUtil::error(hwnd_, IDS_ERROR_MARKMESSAGE);
		return;
	}
	pUndoManager_->pushUndoItem(undo.getUndoItem());
}

bool qm::MessageMarkAction::isEnabled(const ActionEvent& event)
{
	return pModel_->hasSelectedMessage();
}


/****************************************************************************
 *
 * MessageMoveAction
 *
 */

qm::MessageMoveAction::MessageMoveAction(AccountManager* pAccountManager,
										 MessageSelectionModel* pMessageSelectionModel,
										 ViewModelHolder* pViewModelHolder,
										 MessageModel* pMessageModel,
										 bool bDontSelectNextIfDeletedFlag,
										 UndoManager* pUndoManager,
										 const FolderImage* pFolderImage,
										 qs::Profile* pProfile,
										 HWND hwnd) :
	pAccountManager_(pAccountManager),
	pMessageSelectionModel_(pMessageSelectionModel),
	pViewModelHolder_(pViewModelHolder),
	pMessageModel_(pMessageModel),
	bDontSelectNextIfDeletedFlag_(bDontSelectNextIfDeletedFlag),
	pUndoManager_(pUndoManager),
	pFolderImage_(pFolderImage),
	pProfile_(pProfile),
	hwnd_(hwnd)
{
}

qm::MessageMoveAction::~MessageMoveAction()
{
}

void qm::MessageMoveAction::invoke(const ActionEvent& event)
{
	ViewModel* pViewModel = pViewModelHolder_->getViewModel();
	assert(pViewModel);
	Lock<ViewModel> lockViewModel(*pViewModel);
	
	AccountLock lock;
	Folder* pFolderFrom = 0;
	MessageHolderList l;
	pMessageSelectionModel_->getSelectedMessages(&lock, &pFolderFrom, &l);
	assert(pFolderFrom == pViewModel->getFolder());
	if (l.empty())
		return;
	
	Account* pAccount = lock.get();
	
	NormalFolder* pFolderTo = 0;
	bool bMove = true;
	
	const WCHAR* pwszFolder = ActionParamUtil::getString(event.getParam(), 0);
	if (pwszFolder) {
		Folder* pFolder = pAccountManager_->getFolder(pAccount, pwszFolder);
		if (!pFolder || pFolder->getType() != Folder::TYPE_NORMAL)
			return;
		pFolderTo = static_cast<NormalFolder*>(pFolder);
		bMove = (event.getModifier() & ActionEvent::MODIFIER_CTRL) == 0;
	}
	else {
		MoveMessageDialog dialog(pAccountManager_, pAccount, pFolderImage_, pProfile_);
		if (dialog.doModal(hwnd_) != IDOK)
			return;
		
		pFolderTo = dialog.getFolder();
		bMove = !dialog.isCopy();
	}
	if (!pFolderTo)
		return;
	
	if (bMove) {
		bool bSelectNext = !bDontSelectNextIfDeletedFlag_ ||
			!pAccount->isSupport(Account::SUPPORT_DELETEDMESSAGE) ||
			pFolderFrom->getType() != Folder::TYPE_NORMAL ||
			pFolderFrom->isFlag(Folder::FLAG_LOCAL);
		if (bSelectNext) {
			unsigned int nIndex = l.size() == 1 ?
				pViewModel->getIndex(l.front()) : pViewModel->getFocused();
			if (nIndex < pViewModel->getCount() - 1)
				MessageActionUtil::select(pViewModel, nIndex + 1, pMessageModel_);
			else if (nIndex != 0)
				MessageActionUtil::select(pViewModel, nIndex - 1, pMessageModel_);
		}
	}
	
	if (!moveMessages(l, pFolderFrom, pFolderTo, bMove))
		ActionUtil::error(hwnd_, bMove ? IDS_ERROR_MOVEMESSAGE : IDS_ERROR_COPYMESSAGE);
}

bool qm::MessageMoveAction::isEnabled(const ActionEvent& event)
{
	return pMessageSelectionModel_->hasSelectedMessage();
}

bool qm::MessageMoveAction::moveMessages(const MessageHolderList& l,
										 Folder* pFolderFrom,
										 NormalFolder* pFolderTo,
										 bool bMove) const
{
	UINT nId = bMove ? IDS_PROGRESS_MOVEMESSAGE : IDS_PROGRESS_COPYMESSAGE;
	UndoItemList undo;
	ProgressDialogMessageOperationCallback callback(hwnd_, nId, nId);
	Account* pAccount = pFolderFrom->getAccount();
	unsigned int nFlags = Account::OPFLAG_ACTIVE | Account::COPYFLAG_MANAGEJUNK |
		(bMove ? Account::COPYFLAG_MOVE : Account::COPYFLAG_NONE);
	if (!pAccount->copyMessages(l, pFolderFrom, pFolderTo, nFlags, &callback, &undo, 0))
		return false;
	
	pUndoManager_->pushUndoItem(undo.getUndoItem());
	
	return true;
}


/****************************************************************************
 *
 * MessageOpenAttachmentAction
 *
 */

qm::MessageOpenAttachmentAction::MessageOpenAttachmentAction(AccountManager* pAccountManager,
															 SecurityModel* pSecurityModel,
															 Profile* pProfile,
															 TempFileCleaner* pTempFileCleaner,
															 HWND hwnd) :
	pAccountManager_(pAccountManager),
	pSecurityModel_(pSecurityModel),
	helper_(pSecurityModel, pProfile, pTempFileCleaner, hwnd),
	hwnd_(hwnd)
{
}

qm::MessageOpenAttachmentAction::~MessageOpenAttachmentAction()
{
}

void qm::MessageOpenAttachmentAction::invoke(const ActionEvent& event)
{
	const WCHAR* pwszURI = ActionParamUtil::getString(event.getParam(), 0);
	if (!pwszURI)
		return;
	
	std::auto_ptr<URI> pURI(URI::parse(pwszURI));
	if (!pURI.get())
		return;
	
	MessagePtrLock mpl(pAccountManager_->getMessage(*pURI.get()));
	if (!mpl)
		return;
	
	Message msg;
	if (!mpl->getMessage(Account::GETMESSAGEFLAG_ALL, 0, pSecurityModel_->getSecurityMode(), &msg)) {
		ActionUtil::error(hwnd_, IDS_ERROR_EXECUTEATTACHMENT);
		return;
	}
	
	const Part* pPart = pURI->getFragment().getPart(&msg);
	if (!pPart)
		return;
	const WCHAR* pwszName = pURI->getFragment().getName();
	
	bool bExternalEditor = (event.getModifier() & ActionEvent::MODIFIER_SHIFT) != 0;
	if (helper_.open(pPart, pwszName, bExternalEditor) == AttachmentParser::RESULT_FAIL) {
		ActionUtil::error(hwnd_, IDS_ERROR_EXECUTEATTACHMENT);
		return;
	}
}

bool qm::MessageOpenAttachmentAction::isEnabled(const ActionEvent& event)
{
	const WCHAR* pwszURI = ActionParamUtil::getString(event.getParam(), 0);
	if (!pwszURI)
		return false;
	return true;
}


/****************************************************************************
 *
 * MessageOpenLinkAction
 *
 */

qm::MessageOpenLinkAction::MessageOpenLinkAction(MessageSelectionModel* pMessageSelectionModel,
												 Profile* pProfile,
												 HWND hwnd) :
	pMessageSelectionModel_(pMessageSelectionModel),
	pProfile_(pProfile),
	hwnd_(hwnd)
{
}

qm::MessageOpenLinkAction::~MessageOpenLinkAction()
{
}

void qm::MessageOpenLinkAction::invoke(const ActionEvent& event)
{
	MessagePtrLock mpl(pMessageSelectionModel_->getFocusedMessage());
	if (mpl) {
		Account* pAccount = mpl->getAccount();
		if (!pAccount->isSupport(Account::SUPPORT_EXTERNALLINK))
			return;
		
		Message msg;
		if (!mpl->getMessage(Account::GETMESSAGEFLAG_HEADER,
			L"X-QMAIL-Link", SECURITYMODE_NONE, &msg))
			return;
		
		UnstructuredParser link;
		if (msg.getField(L"X-QMAIL-Link", &link) == Part::FIELD_EXIST)
			UIUtil::openURL(link.getValue(), pProfile_, hwnd_);
	}
}

bool qm::MessageOpenLinkAction::isEnabled(const ActionEvent& event)
{
	return pMessageSelectionModel_->hasFocusedMessage();
}


/****************************************************************************
 *
 * MessageOpenRecentAction
 *
 */

qm::MessageOpenRecentAction::MessageOpenRecentAction(Recents* pRecents,
													 AccountManager* pAccountManager,
													 ViewModelManager* pViewModelManager,
													 FolderModel* pFolderModel,
													 MainWindow* pMainWindow,
													 MessageFrameWindowManager* pMessageFrameWindowManager,
													 Profile* pProfile,
													 HWND hwnd) :
	pRecents_(pRecents),
	pAccountManager_(pAccountManager),
	pViewModelManager_(pViewModelManager),
	pFolderModel_(pFolderModel),
	pMainWindow_(pMainWindow),
	pMessageFrameWindowManager_(pMessageFrameWindowManager),
	pProfile_(pProfile),
	hwnd_(hwnd)
{
}

qm::MessageOpenRecentAction::~MessageOpenRecentAction()
{
}

void qm::MessageOpenRecentAction::invoke(const ActionEvent& event)
{
	const WCHAR* pwszURI = ActionParamUtil::getString(event.getParam(), 0);
	if (!pwszURI)
		return;
	
	std::auto_ptr<URI> pURI(URI::parse(pwszURI));
	if (!pURI.get())
		return;
	
	MessagePtrLock mpl(pAccountManager_->getMessage(*pURI.get()));
	if (mpl) {
		bool bOpenInPreview = pProfile_->getInt(L"Global", L"OpenRecentInPreview") != 0;
		if (event.getModifier() & ActionEvent::MODIFIER_SHIFT)
			bOpenInPreview = !bOpenInPreview;
		
		NormalFolder* pFolder = mpl->getFolder();
		ViewModel* pViewModel = pViewModelManager_->getViewModel(pFolder);
		if (bOpenInPreview) {
#ifndef _WIN32_WCE_PSPC
			if (pMainWindow_->isHidden())
				pMainWindow_->show();
#endif
			
			pFolderModel_->setCurrent(0, pFolder, false);
			
			Lock<ViewModel> lock(*pViewModel);
			unsigned int nIndex = pViewModel->getIndex(mpl);
			if (nIndex != -1)
				MessageActionUtil::select(pViewModel, nIndex, false);
		}
		else {
			if (!pMessageFrameWindowManager_->open(pViewModel, mpl))
				ActionUtil::error(hwnd_, IDS_ERROR_OPENMESSAGE);
		}
	}
	pRecents_->remove(pURI.get());
}

bool qm::MessageOpenRecentAction::isEnabled(const ActionEvent& event)
{
	const WCHAR* pwszURI = ActionParamUtil::getString(event.getParam(), 0);
	if (!pwszURI)
		return false;
	return true;
}


/****************************************************************************
 *
 * MessageOpenURLAction
 *
 */

qm::MessageOpenURLAction::MessageOpenURLAction(Document* pDocument,
											   PasswordManager* pPasswordManager,
											   FolderModelBase* pFolderModel,
											   MessageSelectionModel* pMessageSelectionModel,
											   SecurityModel* pSecurityModel,
											   EditFrameWindowManager* pEditFrameWindowManager,
											   ExternalEditorManager* pExternalEditorManager,
											   HWND hwnd,
											   Profile* pProfile,
											   bool bExternalEditor) :
	processor_(pDocument, pFolderModel, pMessageSelectionModel,
		0, pSecurityModel, pEditFrameWindowManager,
		pExternalEditorManager, hwnd, pProfile, bExternalEditor,
		Application::getApplication().getTemporaryFolder()),
	pDocument_(pDocument),
	pPasswordManager_(pPasswordManager),
	pFolderModel_(pFolderModel),
	pProfile_(pProfile),
	hwnd_(hwnd)
{
}

qm::MessageOpenURLAction::~MessageOpenURLAction()
{
}

void qm::MessageOpenURLAction::invoke(const ActionEvent& event)
{
	const WCHAR* pwszURL = ActionParamUtil::getString(event.getParam(), 0);
	if (!pwszURL)
		return;
	
	if (wcsncmp(pwszURL, L"feed:", 5) == 0)
		openFeedURL(pwszURL);
	else
		openMailtoURL(pwszURL, (event.getModifier() & ActionEvent::MODIFIER_SHIFT) != 0);
}

void qm::MessageOpenURLAction::openMailtoURL(const WCHAR* pwszURL,
											 bool bExternalEditor) const
{
	std::pair<Account*, bool> account(getAccount(L"mail", L"DefaultMailAccount"));
	if (!account.second)
		return;
	
	TemplateContext::Argument arg = {
		L"url",
		pwszURL
	};
	TemplateContext::ArgumentList listArgument(1, arg);
	if (!processor_.process(L"url", listArgument, bExternalEditor, account.first)) {
		ActionUtil::error(hwnd_, IDS_ERROR_OPENURL);
		return;
	}
}

void qm::MessageOpenURLAction::openFeedURL(const WCHAR* pwszURL) const
{
	std::pair<Account*, bool> account(getAccount(L"rss", L"DefaultRssAccount"));
	if (!account.second)
		return;
	
	wstring_ptr wstrURL;
	if (wcsncmp(pwszURL, L"feed://", 7) == 0)
		wstrURL = concat(L"http:", pwszURL + 5);
	else
		wstrURL = allocWString(pwszURL + 5);
	
	FolderSubscribeAction::subscribe(pDocument_, account.first,
		0, pPasswordManager_, hwnd_, wstrURL.get());
}

std::pair<Account*, bool> qm::MessageOpenURLAction::getAccount(const WCHAR* pwszClass,
															   const WCHAR* pwszDefaultKey) const
{
	std::pair<Account*, Folder*> p(FolderActionUtil::getCurrent(pFolderModel_));
	Account* pAccount = p.first ? p.first : p.second ? p.second->getAccount() : 0;
	if (!pAccount || wcscmp(pAccount->getClass(), pwszClass) != 0) {
		wstring_ptr wstrAccount(pProfile_->getString(L"Global", pwszDefaultKey));
		if (*wstrAccount.get()) {
			pAccount = pDocument_->getAccount(wstrAccount.get());
		}
		else {
			pAccount = 0;
			const Document::AccountList& l = pDocument_->getAccounts();
			for (Document::AccountList::const_iterator it = l.begin(); it != l.end() && !pAccount; ++it) {
				if (wcscmp((*it)->getClass(), pwszClass) == 0)
					pAccount = *it;
			}
		}
		if (!pAccount)
			return std::pair<Account*, bool>(0, false);
	}
	else {
		pAccount = 0;
	}
	return std::make_pair(pAccount, true);
}


/****************************************************************************
 *
 * MessagePropertyAction
 *
 */

qm::MessagePropertyAction::MessagePropertyAction(MessageSelectionModel* pMessageSelectionModel,
												 UndoManager* pUndoManager,
												 HWND hwnd) :
	pMessageSelectionModel_(pMessageSelectionModel),
	pUndoManager_(pUndoManager),
	hwnd_(hwnd)
{
}

qm::MessagePropertyAction::~MessagePropertyAction()
{
}

void qm::MessagePropertyAction::invoke(const ActionEvent& event)
{
	AccountLock lock;
	MessageHolderList l;
	pMessageSelectionModel_->getSelectedMessages(&lock, 0, &l);
	
	Account* pAccount = lock.get();
	
	HINSTANCE hInst = Application::getApplication().getResourceHandle();
	wstring_ptr wstrTitle(loadString(hInst, IDS_TITLE_PROPERTY));
	
	MessagePropertyPage page(l);
	PropertySheetBase sheet(hInst, wstrTitle.get(), false);
	sheet.add(&page);
	
	if (sheet.doModal(hwnd_) != IDOK)
		return;
	
	UndoItemList undo;
	if (!pAccount->setMessagesFlags(l, page.getFlags(), page.getMask(), &undo)) {
		ActionUtil::error(hwnd_, IDS_ERROR_SETFLAGS);
		return;
	}
	pUndoManager_->pushUndoItem(undo.getUndoItem());
}

bool qm::MessagePropertyAction::isEnabled(const ActionEvent& event)
{
	return pMessageSelectionModel_->hasSelectedMessage();
}


/****************************************************************************
 *
 * MessageSearchAction
 *
 */

qm::MessageSearchAction::MessageSearchAction(FolderModel* pFolderModel,
											 SecurityModel* pSecurityModel,
											 Document* pDocument,
											 HWND hwnd,
											 Profile* pProfile) :
	pFolderModel_(pFolderModel),
	pSecurityModel_(pSecurityModel),
	pDocument_(pDocument),
	hwnd_(hwnd),
	pProfile_(pProfile)
{
}

qm::MessageSearchAction::~MessageSearchAction()
{
}

void qm::MessageSearchAction::invoke(const ActionEvent& event)
{
	std::pair<Account*, Folder*> p(FolderActionUtil::getCurrent(pFolderModel_));
	Account* pAccount = p.first ? p.first : p.second ? p.second->getAccount() : 0;
	if (!pAccount)
		return;
	Folder* pFolder = p.second;
	
	QueryFolder* pSearch = static_cast<QueryFolder*>(
		pAccount->getFolderByBoxFlag(Folder::FLAG_SEARCHBOX));
	if (!pSearch)
		return;
	
	HINSTANCE hInst = Application::getApplication().getResourceHandle();
	wstring_ptr wstrTitle(loadString(hInst, IDS_TITLE_SEARCH));
	
	typedef std::vector<std::pair<SearchUI*, SearchPropertyPage*> > UIList;
	UIList listUI;
	struct Deleter
	{
		typedef std::vector<std::pair<SearchUI*, SearchPropertyPage*> > UIList;
		
		Deleter(UIList& l) :
			l_(l)
		{
		}
		
		~Deleter()
		{
			std::for_each(l_.begin(), l_.end(),
				unary_compose_fx_gx(
					qs::deleter<SearchUI>(),
					qs::deleter<SearchPropertyPage>()));
		}
		
		UIList& l_;
	} deleter(listUI);
	
	SearchDriverFactory::NameList listName;
	SearchDriverFactory::getNames(&listName);
	listUI.reserve(listName.size());
	for (SearchDriverFactory::NameList::iterator itN = listName.begin(); itN != listName.end(); ++itN) {
		std::auto_ptr<SearchUI> pUI(SearchDriverFactory::getUI(*itN, pAccount, pProfile_));
		if (pUI.get())
			listUI.push_back(UIList::value_type(pUI.release(), 0));
	}
	std::sort(listUI.begin(), listUI.end(),
		binary_compose_f_gx_hy(
			std::less<int>(),
			unary_compose_f_gx(
				std::mem_fun(&SearchUI::getIndex),
				std::select1st<UIList::value_type>()),
			unary_compose_f_gx(
				std::mem_fun(&SearchUI::getIndex),
				std::select1st<UIList::value_type>())));
	
	wstring_ptr wstrStartName(pProfile_->getString(L"Search", L"Page"));
	
	SearchPropertyData data(pAccount, pFolder, pProfile_);
	int nStartPage = 0;
	PropertySheetBase sheet(hInst, wstrTitle.get(), false);
	for (UIList::size_type n = 0; n < listUI.size(); ++n) {
		std::auto_ptr<SearchPropertyPage> pPage(
			listUI[n].first->createPropertyPage(&data));
		listUI[n].second = pPage.release();
		sheet.add(listUI[n].second);
		if (wcscmp(listUI[n].second->getDriver(), wstrStartName.get()) == 0)
			nStartPage = static_cast<int>(n);
	}
	sheet.setStartPage(nStartPage);
	
	if (sheet.doModal(hwnd_) != IDOK)
		return;
	
	UIList::size_type nPage = 0;
	while (nPage < listUI.size()) {
		if (listUI[nPage].second->getCondition())
			break;
		++nPage;
	}
	if (nPage == listUI.size())
		return;
	
	SearchPropertyPage* pPage = listUI[nPage].second;
	const WCHAR* pwszCondition = pPage->getCondition();
	if (*pwszCondition) {
		WaitCursor cursor;
		
		wstring_ptr wstrFolder;
		if (data.getFolder())
			wstrFolder = data.getFolder()->getFullName();
		
		if (data.isNewFolder()) {
			wstring_ptr wstrName(allocWString(data.getCondition()));
			std::replace(wstrName.get(), wstrName.get() + wcslen(wstrName.get()), L'/', L'_');
			if (pAccount->getFolder(0, wstrName.get())) {
				for (int n = 1; n < 1000; ++n) {
					WCHAR wsz[10];
					_snwprintf(wsz, countof(wsz), L" (%d)", n);
					wstrName = concat(data.getCondition(), wsz);
					if (!pAccount->getFolder(0, wstrName.get()))
						break;
				}
			}
			pSearch = pAccount->createQueryFolder(wstrName.get(), pSearch,
				pPage->getDriver(), pwszCondition, wstrFolder.get(), data.isRecursive());
			if (!pSearch)
				return;
		}
		else {
			pSearch->set(pPage->getDriver(), pwszCondition,
				wstrFolder.get(), data.isRecursive());
		}
		
		if (pFolder != pSearch)
			pFolderModel_->setCurrent(0, pSearch, false);
		
		if (pFolder == pSearch || !pSearch->isFlag(Folder::FLAG_ACTIVESYNC)) {
			if (!pSearch->search(pDocument_, hwnd_,
				pProfile_, pSecurityModel_->getSecurityMode())) {
				ActionUtil::error(hwnd_, IDS_ERROR_SEARCH);
				return;
			}
		}
	}
	pProfile_->setString(L"Search", L"Page", pPage->getDriver());
	data.save();
}

bool qm::MessageSearchAction::isEnabled(const ActionEvent& event)
{
	std::pair<Account*, Folder*> p(FolderActionUtil::getCurrent(pFolderModel_));
	return p.first || p.second;
}


#ifdef QMTABWINDOW

/****************************************************************************
 *
 * TabCloseAction
 *
 */

qm::TabCloseAction::TabCloseAction(TabModel* pTabModel) :
	pTabModel_(pTabModel)
{
}

qm::TabCloseAction::~TabCloseAction()
{
}

void qm::TabCloseAction::invoke(const ActionEvent& event)
{
	int nItem = TabActionUtil::getCurrent(pTabModel_);
	if (nItem != -1)
		pTabModel_->close(nItem);
}

bool qm::TabCloseAction::isEnabled(const ActionEvent& event)
{
	return pTabModel_->getCount() > 1;
}


/****************************************************************************
 *
 * TabCreateAction
 *
 */

qm::TabCreateAction::TabCreateAction(TabModel* pTabModel,
									 FolderSelectionModel* pFolderSelectionModel) :
	pTabModel_(pTabModel),
	pFolderSelectionModel_(pFolderSelectionModel)
{
}

qm::TabCreateAction::~TabCreateAction()
{
}

void qm::TabCreateAction::invoke(const ActionEvent& event)
{
	std::pair<Account*, Folder*> p(pFolderSelectionModel_->getFocusedAccountOrFolder());
	if (p.first)
		pTabModel_->open(p.first);
	else if (p.second)
		pTabModel_->open(p.second);
}

bool qm::TabCreateAction::isEnabled(const ActionEvent& event)
{
	std::pair<Account*, Folder*> p(pFolderSelectionModel_->getFocusedAccountOrFolder());
	return p.first || p.second;
}


/****************************************************************************
 *
 * TabEditTitleAction
 *
 */

qm::TabEditTitleAction::TabEditTitleAction(TabModel* pTabModel,
										   HWND hwnd) :
	pTabModel_(pTabModel),
	hwnd_(hwnd)
{
}

qm::TabEditTitleAction::~TabEditTitleAction()
{
}

void qm::TabEditTitleAction::invoke(const ActionEvent& event)
{
	int nItem = TabActionUtil::getCurrent(pTabModel_);
	if (nItem == -1)
		return;
	
	const TabItem* pItem = pTabModel_->getItem(nItem);
	
	TabTitleDialog dialog(pItem->getTitle());
	if (dialog.doModal(hwnd_) != IDOK)
		return;
	
	const WCHAR* pwszTitle = dialog.getTitle();
	pTabModel_->setTitle(nItem, *pwszTitle ? pwszTitle : 0);
}

bool qm::TabEditTitleAction::isEnabled(const ActionEvent& event)
{
	return TabActionUtil::getCurrent(pTabModel_) != -1;
}


/****************************************************************************
 *
 * TabLockAction
 *
 */

qm::TabLockAction::TabLockAction(TabModel* pTabModel) :
	pTabModel_(pTabModel)
{
}

qm::TabLockAction::~TabLockAction()
{
}

void qm::TabLockAction::invoke(const ActionEvent& event)
{
	int nItem = TabActionUtil::getCurrent(pTabModel_);
	if (nItem == -1)
		return;
	
	const TabItem* pItem = pTabModel_->getItem(nItem);
	pTabModel_->setLocked(nItem, !pItem->isLocked());
}

bool qm::TabLockAction::isEnabled(const ActionEvent& event)
{
	return TabActionUtil::getCurrent(pTabModel_) != -1;
}

bool qm::TabLockAction::isChecked(const ActionEvent& event)
{
	int nItem = TabActionUtil::getCurrent(pTabModel_);
	if (nItem == -1)
		return false;
	else
		return pTabModel_->getItem(nItem)->isLocked();
}


/****************************************************************************
 *
 * TabMoveAction
 *
 */

qm::TabMoveAction::TabMoveAction(TabModel* pTabModel,
								 bool bLeft) :
	pTabModel_(pTabModel),
	bLeft_(bLeft)
{
}

qm::TabMoveAction::~TabMoveAction()
{
}

void qm::TabMoveAction::invoke(const ActionEvent& event)
{
	int nItem = TabActionUtil::getCurrent(pTabModel_);
	if ((bLeft_ && nItem == 0) ||
		(!bLeft_ && nItem == pTabModel_->getCount() - 1))
		return;
	
	pTabModel_->moveItem(nItem, bLeft_ ? -1 : 1);
}

bool qm::TabMoveAction::isEnabled(const ActionEvent& event)
{
	int nItem = TabActionUtil::getCurrent(pTabModel_);
	return !((bLeft_ && nItem == 0) ||
		(!bLeft_ && nItem == pTabModel_->getCount() - 1));
}


/****************************************************************************
 *
 * TabNavigateAction
 *
 */

qm::TabNavigateAction::TabNavigateAction(TabModel* pTabModel,
										 bool bPrev) :
	pTabModel_(pTabModel),
	bPrev_(bPrev)
{
}

qm::TabNavigateAction::~TabNavigateAction()
{
}

void qm::TabNavigateAction::invoke(const ActionEvent& event)
{
	int nItem = pTabModel_->getCurrent();
	int nCount = pTabModel_->getCount();
	if (bPrev_) {
		if (nItem == 0)
			nItem = nCount - 1;
		else
			--nItem;
	}
	else {
		if (nItem == nCount - 1)
			nItem = 0;
		else
			++nItem;
	}
	pTabModel_->setCurrent(nItem);
}


/****************************************************************************
 *
 * TabSelectAction
 *
 */

qm::TabSelectAction::TabSelectAction(TabModel* pTabModel) :
	pTabModel_(pTabModel)
{
}

qm::TabSelectAction::~TabSelectAction()
{
}

void qm::TabSelectAction::invoke(const ActionEvent& event)
{
	int nItem = getItem(event.getParam());
	if (nItem < 0 || pTabModel_->getCount() <= nItem)
		return;
	pTabModel_->setCurrent(nItem);
}

bool qm::TabSelectAction::isEnabled(const ActionEvent& event)
{
	int nItem = getItem(event.getParam());
	return 0 <= nItem && nItem < pTabModel_->getCount();
}

int qm::TabSelectAction::getItem(const ActionParam* pParam) const
{
	std::pair<const WCHAR*, unsigned int> param(ActionParamUtil::getStringOrIndex(pParam, 0));
	if (param.second != -1)
		return param.second;
	
	// TODO
	// Treat param as folder path?
	
	return -1;
}

#endif // QMTABWINDOW


/****************************************************************************
 *
 * ToolAccountAction
 *
 */

qm::ToolAccountAction::ToolAccountAction(Document* pDocument,
										 FolderModel* pFolderModel,
										 PasswordManager* pPasswordManager,
										 SyncManager* pSyncManager,
										 const FolderImage* pFolderImage,
										 OptionDialogManager* pOptionDialogManager,
										 Profile* pProfile,
										 HWND hwnd) :
	pDocument_(pDocument),
	pFolderModel_(pFolderModel),
	pPasswordManager_(pPasswordManager),
	pSyncManager_(pSyncManager),
	pFolderImage_(pFolderImage),
	pOptionDialogManager_(pOptionDialogManager),
	pProfile_(pProfile),
	hwnd_(hwnd)
{
}

qm::ToolAccountAction::~ToolAccountAction()
{
}

void qm::ToolAccountAction::invoke(const ActionEvent& event)
{
	if (pSyncManager_->isSyncing()) {
		ActionUtil::error(hwnd_, IDS_ERROR_SYNCHRONIZING);
		return;
	}
	
	bool bOffline = pDocument_->isOffline();
	if (!bOffline)
		pDocument_->setOffline(true);
	
	Account* pAccount = FolderActionUtil::getAccount(pFolderModel_);
	AccountDialog dialog(pDocument_, pAccount, pPasswordManager_,
		pSyncManager_->getSyncFilterManager(), pDocument_->getSecurity(),
		pDocument_->getJunkFilter(), pFolderImage_, pOptionDialogManager_, pProfile_);
	dialog.doModal(hwnd_, 0);
	
	if (!Application::getApplication().save(false))
		ActionUtil::error(hwnd_, IDS_ERROR_SAVE);
	
	if (!bOffline)
		pDocument_->setOffline(false);
}

bool qm::ToolAccountAction::isEnabled(const ActionEvent& event)
{
	return !pSyncManager_->isSyncing();
}


/****************************************************************************
 *
 * ToolAddAddressAction
 *
 */

qm::ToolAddAddressAction::ToolAddAddressAction(AddressBook* pAddressBook,
											   MessageSelectionModel* pMessageSelectionModel,
											   HWND hwnd) :
	pAddressBook_(pAddressBook),
	pMessageSelectionModel_(pMessageSelectionModel),
	hwnd_(hwnd)
{
}

qm::ToolAddAddressAction::~ToolAddAddressAction()
{
}

void qm::ToolAddAddressAction::invoke(const ActionEvent& event)
{
	MessagePtrLock mpl(pMessageSelectionModel_->getFocusedMessage());
	if (!mpl)
		return;
	
	Message msg;
	if (!mpl->getMessage(Account::GETMESSAGEFLAG_HEADER, L"From", SECURITYMODE_NONE, &msg))
		return;
	AddressListParser from;
	if (msg.getField(L"From", &from) != Part::FIELD_EXIST || from.getAddressList().empty())
		return;
	AddressParser* pFrom = from.getAddressList().front();
	
	wstring_ptr wstrPath(Application::getApplication().getProfilePath(FileNames::ADDRESSBOOK_XML));
	std::auto_ptr<AddressBook> pAddressBook(new AddressBook(wstrPath.get(), 0, false));
	
	AddAddressDialog dialog(pAddressBook.get());
	if (dialog.doModal(hwnd_) != IDOK)
		return;
	
	wstring_ptr wstrAddress(pFrom->getAddress());
	const WCHAR* pwszPhrase = pFrom->getPhrase();
	if (!pwszPhrase)
		pwszPhrase = wstrAddress.get();
	
	switch (dialog.getType()) {
	case AddAddressDialog::TYPE_NEWENTRY:
		{
			std::auto_ptr<AddressBookEntry> pEntry(
				new AddressBookEntry(pwszPhrase, 0, false));
			std::auto_ptr<AddressBookAddress> pAddress(new AddressBookAddress(pEntry.get(),
				wstrAddress.get(), 0, AddressBookAddress::CategoryList(), 0, 0, false));
			pEntry->addAddress(pAddress);
			
			AddressBookEntryDialog dialog(pAddressBook.get(), pEntry.get());
			if (dialog.doModal(hwnd_) != IDOK)
				return;
			
			pAddressBook->addEntry(pEntry);
		}
		break;
	case AddAddressDialog::TYPE_NEWADDRESS:
		{
			AddressBookEntry* pEntry = dialog.getEntry();
			std::auto_ptr<AddressBookAddress> pAddress(new AddressBookAddress(pEntry,
				wstrAddress.get(), 0, AddressBookAddress::CategoryList(), 0, 0, false));
			
			AddressBookAddressDialog dialog(pAddressBook.get(), pAddress.get());
			if (dialog.doModal(hwnd_) != IDOK)
				return;
			
			pEntry->addAddress(pAddress);
		}
		break;
	default:
		assert(false);
		break;
	}
	
	if (!pAddressBook->save())
		ActionUtil::error(hwnd_, IDS_ERROR_SAVEADDRESSBOOK);
	
	pAddressBook_->reload();
}

bool qm::ToolAddAddressAction::isEnabled(const ActionEvent& event)
{
	return pMessageSelectionModel_->hasFocusedMessage();
}


/****************************************************************************
 *
 * ToolAddressBookAction
 *
 */

qm::ToolAddressBookAction::ToolAddressBookAction(AddressBookFrameWindowManager* pManager) :
	pManager_(pManager)
{
}

qm::ToolAddressBookAction::~ToolAddressBookAction()
{
}

void qm::ToolAddressBookAction::invoke(const ActionEvent& event)
{
	pManager_->open();
}


/****************************************************************************
 *
 * ToolAutoPilotAction
 *
 */

qm::ToolAutoPilotAction::ToolAutoPilotAction(AutoPilot* pAutoPilot) :
	pAutoPilot_(pAutoPilot)
{
}

qm::ToolAutoPilotAction::~ToolAutoPilotAction()
{
}

void qm::ToolAutoPilotAction::invoke(const ActionEvent& event)
{
	pAutoPilot_->setEnabled(!pAutoPilot_->isEnabled());
}

bool qm::ToolAutoPilotAction::isChecked(const ActionEvent& event)
{
	return pAutoPilot_->isEnabled();
}


/****************************************************************************
 *
 * ToolDialupAction
 *
 */

qm::ToolDialupAction::ToolDialupAction(SyncManager* pSyncManager,
									   Document* pDocument,
									   SyncDialogManager* pSyncDialogManager,
									   HWND hwnd) :
	pSyncManager_(pSyncManager),
	pDocument_(pDocument),
	pSyncDialogManager_(pSyncDialogManager),
	hwnd_(hwnd)
{
}

qm::ToolDialupAction::~ToolDialupAction()
{
}

void qm::ToolDialupAction::invoke(const ActionEvent& event)
{
	if (!isConnected()) {
		std::auto_ptr<SyncData> pData(new StaticSyncData(
			pDocument_, SyncData::TYPE_MANUAL, pSyncManager_));
		
		std::auto_ptr<SyncDialup> pDialup(new SyncDialup(0,
			SyncDialup::FLAG_SHOWDIALOG | SyncDialup::FLAG_NOTDISCONNECT, 0, 0));
		pData->setDialup(pDialup);
		
		SyncDialog* pSyncDialog = pSyncDialogManager_->open();
		if (!pSyncDialog) {
			ActionUtil::error(hwnd_, IDS_ERROR_DIALUPCONNECT);
			return;
		}
		pData->setCallback(pSyncDialog->getSyncManagerCallback());
		
		if (!pSyncManager_->sync(pData)) {
			ActionUtil::error(hwnd_, IDS_ERROR_DIALUPCONNECT);
			return;
		}
	}
	else {
		std::auto_ptr<RasConnection> pRasConnection(
			RasConnection::getActiveConnection(0));
		if (pRasConnection.get()) {
			RasConnection::Result result = pRasConnection->disconnect(0);
			if (result == RasConnection::RAS_FAIL) {
				ActionUtil::error(hwnd_, IDS_ERROR_DIALUPDISCONNECT);
				return;
			}
		}
	}
}

wstring_ptr qm::ToolDialupAction::getText(const ActionEvent& event)
{
	HINSTANCE hInst = Application::getApplication().getResourceHandle();
	UINT nId = isConnected() ? IDS_ACTION_DIALUPDISCONNECT : IDS_ACTION_DIALUPCONNECT;
	return loadString(hInst, nId);
}

bool qm::ToolDialupAction::isConnected() const
{
	return RasConnection::getActiveConnectionCount() != 0;
}


/****************************************************************************
 *
 * ToolGoRoundAction
 *
 */

qm::ToolGoRoundAction::ToolGoRoundAction(SyncManager* pSyncManager,
										 Document* pDocument,
										 GoRound* pGoRound,
										 SyncDialogManager* pSyncDialogManager,
										 HWND hwnd) :
	pSyncManager_(pSyncManager),
	pDocument_(pDocument),
	pGoRound_(pGoRound),
	pSyncDialogManager_(pSyncDialogManager),
	hwnd_(hwnd)
{
}

qm::ToolGoRoundAction::~ToolGoRoundAction()
{
}

void qm::ToolGoRoundAction::invoke(const ActionEvent& event)
{
	std::pair<const WCHAR*, unsigned int> param(ActionParamUtil::getStringOrIndex(event.getParam(), 0));
	
	const GoRoundCourse* pCourse = 0;
	if (param.first) {
		pCourse = pGoRound_->getCourse(param.first);
	}
	else if (param.second != -1) {
		const GoRound::CourseList& l = pGoRound_->getCourses();
		if (l.size() > param.second)
			pCourse = l[param.second];
	}
	
	if (!SyncUtil::goRound(pSyncManager_, pDocument_,
		pSyncDialogManager_, SyncData::TYPE_MANUAL, pCourse)) {
		ActionUtil::error(hwnd_, IDS_ERROR_GOROUND);
		return;
	}
}


/****************************************************************************
 *
 * ToolInvokeActionAction
 *
 */

qm::ToolInvokeActionAction::ToolInvokeActionAction(ActionInvoker* pActionInvoker,
												   Profile* pProfile,
												   HWND hwnd) :
	pActionInvoker_(pActionInvoker),
	pProfile_(pProfile),
	hwnd_(hwnd)
{
}

qm::ToolInvokeActionAction::~ToolInvokeActionAction()
{
}

void qm::ToolInvokeActionAction::invoke(const ActionEvent& event)
{
	const WCHAR* pwszActions = ActionParamUtil::getString(event.getParam(), 0);
	wstring_ptr wstrActions;
	if (!pwszActions) {
		HINSTANCE hInst = Application::getApplication().getResourceHandle();
		wstring_ptr wstrTitle(loadString(hInst, IDS_INVOKEACTION));
		wstring_ptr wstrMessage(loadString(hInst, IDS_ACTION));
		wstring_ptr wstrPrevActions(pProfile_->getString(L"Global", L"Action"));
		
		SingleLineInputBoxDialog dialog(wstrTitle.get(),
			wstrMessage.get(), wstrPrevActions.get(), false);
		if (dialog.doModal(hwnd_) != IDOK)
			return;
		
		wstrActions = allocWString(dialog.getValue());
		pProfile_->setString(L"Global", L"Action", wstrActions.get());
		pwszActions = wstrActions.get();
	}
	
	ActionList listAction;
	StringListFree<ActionList> free(listAction);
	parseActions(pwszActions, &listAction);
	
	struct CommandLineHandlerImpl : public CommandLineHandler
	{
		virtual ~CommandLineHandlerImpl()
		{
			std::for_each(listParam_.begin(), listParam_.end(), string_free<WSTRING>());
		}
		
		virtual bool process(const WCHAR* pwszOption)
		{
			wstring_ptr wstr(allocWString(pwszOption));
			if (!wstrAction_.get()) {
				wstrAction_ = wstr;
			}
			else {
				listParam_.push_back(wstr.get());
				wstr.release();
			}
			
			return true;
		}
		
		typedef std::vector<WSTRING> ParamList;
		
		wstring_ptr wstrAction_;
		ParamList listParam_;
	};
	
	for (ActionList::const_iterator it = listAction.begin(); it != listAction.end(); ++it) {
		CommandLineHandlerImpl handler;
		CommandLine commandLine(&handler);
		if (commandLine.parse(*it) && handler.wstrAction_.get())
			pActionInvoker_->invoke(handler.wstrAction_.get(),
				const_cast<const WCHAR**>(&handler.listParam_[0]), handler.listParam_.size());
	}
}

void qm::ToolInvokeActionAction::parseActions(const WCHAR* pwszActions,
											  ActionList* pList)
{
	assert(pwszActions);
	assert(pList);
	
	while (*pwszActions) {
		StringBuffer<WSTRING> buf;
		const WCHAR* p = pwszActions;
		while (*p) {
			if (*p == L'|') {
				if (*(p + 1) == L'|') {
					buf.append(*p);
					++p;
				}
				else {
					break;
				}
			}
			else {
				buf.append(*p);
			}
			++p;
		}
		if (buf.getLength() != 0) {
			wstring_ptr wstrAction(trim(buf.getCharArray()));
			pList->push_back(wstrAction.get());
			wstrAction.release();
		}
		
		if (!*p)
			break;
		
		pwszActions = p + 1;
	}
}


/****************************************************************************
 *
 * ToolOptionsAction
 *
 */

qm::ToolOptionsAction::ToolOptionsAction(OptionDialogManager* pOptionDialogManager,
										 AccountSelectionModel* pAccountSelectionModel,
										 HWND hwnd,
										 OptionDialog::Panel panel) :
	pOptionDialogManager_(pOptionDialogManager),
	pAccountSelectionModel_(pAccountSelectionModel),
	hwnd_(hwnd),
	panel_(panel)
{
}

qm::ToolOptionsAction::~ToolOptionsAction()
{
}

void qm::ToolOptionsAction::invoke(const ActionEvent& event)
{
	Account* pAccount = pAccountSelectionModel_->getAccount();
	pOptionDialogManager_->showDialog(hwnd_, pAccount, panel_);
	
	if (!Application::getApplication().save(false))
		ActionUtil::error(hwnd_, IDS_ERROR_SAVE);
}

bool qm::ToolOptionsAction::isEnabled(const ActionEvent& event)
{
	return pOptionDialogManager_->canShowDialog();
}


/****************************************************************************
 *
 * ToolScriptAction
 *
 */

qm::ToolScriptAction::ToolScriptAction(ScriptManager* pScriptManager,
									   Document* pDocument,
									   Profile* pProfile,
									   MainWindow* pMainWindow) :
	pScriptManager_(pScriptManager),
	pDocument_(pDocument),
	pProfile_(pProfile),
	pMainWindow_(pMainWindow),
	pEditFrameWindow_(0),
	pMessageFrameWindow_(0)
{
}

qm::ToolScriptAction::ToolScriptAction(ScriptManager* pScriptManager,
									   Document* pDocument,
									   Profile* pProfile,
									   EditFrameWindow* pEditFrameWindow) :
	pScriptManager_(pScriptManager),
	pDocument_(pDocument),
	pProfile_(pProfile),
	pMainWindow_(0),
	pEditFrameWindow_(pEditFrameWindow),
	pMessageFrameWindow_(0)
{
}

qm::ToolScriptAction::ToolScriptAction(ScriptManager* pScriptManager,
									   Document* pDocument,
									   Profile* pProfile,
									   MessageFrameWindow* pMessageFrameWindow) :
	pScriptManager_(pScriptManager),
	pDocument_(pDocument),
	pProfile_(pProfile),
	pMainWindow_(0),
	pEditFrameWindow_(0),
	pMessageFrameWindow_(pMessageFrameWindow)
{
}

qm::ToolScriptAction::~ToolScriptAction()
{
}

void qm::ToolScriptAction::invoke(const ActionEvent& event)
{
	const WCHAR* pwszName = ActionParamUtil::getString(event.getParam(), 0);
	if (!pwszName)
		return;
	
	ScriptManager::WindowInfo info;
	if (pMainWindow_) {
		info.type_ = ScriptManager::TYPE_MAIN;
		info.window_.pMainWindow_ = pMainWindow_;
	}
	else if (pEditFrameWindow_) {
		info.type_ = ScriptManager::TYPE_EDIT;
		info.window_.pEditFrameWindow_ = pEditFrameWindow_;
	}
	else if (pMessageFrameWindow_) {
		info.type_ = ScriptManager::TYPE_MESSAGE;
		info.window_.pMessageFrameWindow_ = pMessageFrameWindow_;
	}
	else {
		assert(false);
	}
	
	std::auto_ptr<Script> pScript(pScriptManager_->getScript(pwszName, pDocument_,
		pProfile_, InitThread::getInitThread().getModalHandler(), info));
	if (pScript.get())
		pScript->run(0, 0, 0);
}

bool qm::ToolScriptAction::isEnabled(const ActionEvent& event)
{
	if (!ActionParamUtil::getString(event.getParam(), 0))
		return false;
	
	// TODO
	// Check wether syncing or not
	return true;
}


/****************************************************************************
 *
 * ToolSubAccountAction
 *
 */

qm::ToolSubAccountAction::ToolSubAccountAction(AccountManager* pAccountManager,
											   FolderModel* pFolderModel,
											   SyncManager* pSyncManager,
											   HWND hwnd) :
	pAccountManager_(pAccountManager),
	pFolderModel_(pFolderModel),
	pSyncManager_(pSyncManager),
	hwnd_(hwnd)
{
}

qm::ToolSubAccountAction::~ToolSubAccountAction()
{
}

void qm::ToolSubAccountAction::invoke(const ActionEvent& event)
{
	if (pSyncManager_->isSyncing()) {
		ActionUtil::error(hwnd_, IDS_ERROR_SYNCHRONIZING);
		return;
	}
	
	const WCHAR* pwszName = ActionParamUtil::getString(event.getParam(), 0);
	if (!pwszName)
		return;
	
	const AccountManager::AccountList& listAccount = pAccountManager_->getAccounts();
	for (AccountManager::AccountList::const_iterator it = listAccount.begin(); it != listAccount.end(); ++it) {
		Account* pAccount = *it;
		SubAccount* pSubAccount = pAccount->getSubAccount(pwszName);
		if (pSubAccount)
			pAccount->setCurrentSubAccount(pSubAccount);
	}
}

bool qm::ToolSubAccountAction::isEnabled(const ActionEvent& event)
{
	const WCHAR* pwszName = ActionParamUtil::getString(event.getParam(), 0);
	if (!pwszName)
		return false;
	
	if (pSyncManager_->isSyncing())
		return false;
	
	std::pair<Account*, Folder*> p(pFolderModel_->getCurrent());
	return p.first || p.second;
}

bool qm::ToolSubAccountAction::isChecked(const ActionEvent& event)
{
	std::pair<Account*, Folder*> p(pFolderModel_->getCurrent());
	Account* pAccount = p.first ? p.first : p.second ? p.second->getAccount() : 0;
	if (!pAccount)
		return false;
	
	const WCHAR* pwszName = ActionParamUtil::getString(event.getParam(), 0);
	if (!pwszName)
		return false;
	
	SubAccount* pSubAccount = pAccount->getCurrentSubAccount();
	return wcscmp(pSubAccount->getName(), pwszName) == 0;
}


/****************************************************************************
 *
 * ToolSyncAction
 *
 */

qm::ToolSyncAction::ToolSyncAction(SyncManager* pSyncManager,
								   Document* pDocument,
								   FolderModel* pFolderModel,
								   SyncDialogManager* pSyncDialogManager,
								   Type type,
								   HWND hwnd) :
	pSyncManager_(pSyncManager),
	pDocument_(pDocument),
	pFolderModel_(pFolderModel),
	pSyncDialogManager_(pSyncDialogManager),
	type_(type),
	hwnd_(hwnd)
{
}

qm::ToolSyncAction::~ToolSyncAction()
{
}

void qm::ToolSyncAction::invoke(const ActionEvent& event)
{
	Account* pAccount = FolderActionUtil::getAccount(pFolderModel_);
	if (!pAccount)
		return;
	
	if (type_ != TYPE_RECEIVEFOLDER) {
		if (!SyncUtil::sync(pSyncManager_, pDocument_, pSyncDialogManager_,
			SyncData::TYPE_MANUAL, pAccount, type_ != TYPE_RECEIVE, type_ != TYPE_SEND,
			(event.getModifier() & ActionEvent::MODIFIER_SHIFT) != 0, hwnd_)) {
			ActionUtil::error(hwnd_, IDS_ERROR_SYNC);
			return;
		}
	}
	else {
		Folder* pFolder = pFolderModel_->getCurrent().second;
		if (!pFolder || pFolder->getType() != Folder::TYPE_NORMAL ||
			!pFolder->isFlag(Folder::FLAG_SYNCABLE))
			return;
		
		if (!SyncUtil::syncFolder(pSyncManager_, pDocument_, pSyncDialogManager_,
			SyncData::TYPE_MANUAL, static_cast<NormalFolder*>(pFolder), 0)) {
			ActionUtil::error(hwnd_, IDS_ERROR_SYNC);
			return;
		}
	}
}

bool qm::ToolSyncAction::isEnabled(const ActionEvent& event)
{
	if (!FolderActionUtil::getAccount(pFolderModel_))
		return false;
	
	if (type_ == TYPE_RECEIVEFOLDER) {
		Folder* pFolder = pFolderModel_->getCurrent().second;
		if (!pFolder || pFolder->getType() != Folder::TYPE_NORMAL ||
			!pFolder->isFlag(Folder::FLAG_SYNCABLE))
			return false;
	}
	
	return true;
}


/****************************************************************************
 *
 * ViewDropDownAction
 *
 */

qm::ViewDropDownAction::ViewDropDownAction(FolderComboBox* pFolderComboBox) :
	pFolderComboBox_(pFolderComboBox)
{
}

qm::ViewDropDownAction::~ViewDropDownAction()
{
}

void qm::ViewDropDownAction::invoke(const ActionEvent& event)
{
	pFolderComboBox_->sendMessage(CB_SHOWDROPDOWN,
		!pFolderComboBox_->sendMessage(CB_GETDROPPEDSTATE));
}


/****************************************************************************
 *
 * ViewEncodingAction
 *
 */

qm::ViewEncodingAction::ViewEncodingAction(EncodingModel* pEncodingModel) :
	pEncodingModel_(pEncodingModel)
{
}

qm::ViewEncodingAction::~ViewEncodingAction()
{
}

void qm::ViewEncodingAction::invoke(const ActionEvent& event)
{
	const WCHAR* pwszEncoding = ActionParamUtil::getString(event.getParam(), 0);
	if (!pwszEncoding)
		return;
	else if (*pwszEncoding)
		pEncodingModel_->setEncoding(pwszEncoding);
	else
		pEncodingModel_->setEncoding(0);
}

bool qm::ViewEncodingAction::isEnabled(const qs::ActionEvent& event)
{
	return ActionParamUtil::getString(event.getParam(), 0) != 0;
}

bool qm::ViewEncodingAction::isChecked(const ActionEvent& event)
{
	const WCHAR* pwszEncoding = ActionParamUtil::getString(event.getParam(), 0);
	if (!pwszEncoding)
		return false;
	
	const WCHAR* pwszCurrentEncoding = pEncodingModel_->getEncoding();
	if (*pwszEncoding)
		return pwszCurrentEncoding && wcscmp(pwszEncoding, pwszCurrentEncoding) == 0;
	else
		return !pwszCurrentEncoding;
}


/****************************************************************************
 *
 * ViewFilterAction
 *
 */

qm::ViewFilterAction::ViewFilterAction(ViewModelManager* pViewModelManager) :
	pViewModelManager_(pViewModelManager)
{
}

qm::ViewFilterAction::~ViewFilterAction()
{
}

void qm::ViewFilterAction::invoke(const ActionEvent& event)
{
	ViewModel* pViewModel = pViewModelManager_->getCurrentViewModel();
	if (pViewModel) {
		const WCHAR* pwszFilter = ActionParamUtil::getString(event.getParam(), 0);
		if (pwszFilter && *pwszFilter) {
			FilterManager* pFilterManager = pViewModelManager_->getFilterManager();
			const Filter* pFilter = pFilterManager->getFilter(pwszFilter);
			if (pFilter)
				pViewModel->setFilter(pFilter);
		}
		else {
			pViewModel->setFilter(0);
		}
	}
}

bool qm::ViewFilterAction::isEnabled(const ActionEvent& event)
{
	return pViewModelManager_->getCurrentViewModel() != 0;
}

bool qm::ViewFilterAction::isChecked(const ActionEvent& event)
{
	ViewModel* pViewModel = pViewModelManager_->getCurrentViewModel();
	if (pViewModel) {
		const WCHAR* pwszFilter = ActionParamUtil::getString(event.getParam(), 0);
		const Filter* pFilter = pViewModel->getFilter();
		if (pwszFilter && *pwszFilter)
			return pFilter && wcscmp(pFilter->getName(), pwszFilter) == 0;
		else
			return !pFilter;
	}
	else {
		return false;
	}
}


/****************************************************************************
 *
 * ViewFilterCustomAction
 *
 */

qm::ViewFilterCustomAction::ViewFilterCustomAction(ViewModelManager* pViewModelManager,
												   HWND hwnd) :
	pViewModelManager_(pViewModelManager),
	hwnd_(hwnd)
{
}

qm::ViewFilterCustomAction::~ViewFilterCustomAction()
{
}

void qm::ViewFilterCustomAction::invoke(const ActionEvent& event)
{
	ViewModel* pViewModel = pViewModelManager_->getCurrentViewModel();
	if (pViewModel) {
		const WCHAR* pwszCondition = L"";
		wstring_ptr wstrCondition;
		const Filter* pFilter = pViewModel->getFilter();
		if (pFilter && wcslen(pFilter->getName()) == 0) {
			wstrCondition = pFilter->getCondition()->getString();
			pwszCondition = wstrCondition.get();
		}
		CustomFilterDialog dialog(pwszCondition);
		if (dialog.doModal(hwnd_) == IDOK) {
			MacroParser parser;
			std::auto_ptr<Macro> pCondition(parser.parse(dialog.getCondition()));
			if (!pCondition.get()) {
				ActionUtil::error(hwnd_, IDS_ERROR_INVALIDMACRO);
				return;
			}
			std::auto_ptr<Filter> pFilter(new Filter(L"", pCondition));
			pViewModel->setFilter(pFilter.get());
		}
	}
}

bool qm::ViewFilterCustomAction::isEnabled(const ActionEvent& event)
{
	return pViewModelManager_->getCurrentViewModel() != 0;
}

bool qm::ViewFilterCustomAction::isChecked(const ActionEvent& event)
{
	ViewModel* pViewModel = pViewModelManager_->getCurrentViewModel();
	if (pViewModel) {
		const Filter* pFilter = pViewModel->getFilter();
		return pFilter && wcslen(pFilter->getName()) == 0;
	}
	else {
		return false;
	}
}


/****************************************************************************
 *
 * ViewFitAction
 *
 */

qm::ViewFitAction::ViewFitAction(MessageViewModeHolder* pMessageViewModeHolder) :
	pMessageViewModeHolder_(pMessageViewModeHolder)
{
}

qm::ViewFitAction::~ViewFitAction()
{
}

void qm::ViewFitAction::invoke(const ActionEvent& event)
{
	MessageViewMode* pMode = pMessageViewModeHolder_->getMessageViewMode();
	if (!pMode)
		return;
	
	std::pair<MessageViewMode::Fit, bool> fit(getParam(event.getParam()));
	if (!fit.second)
		return;
	
	pMode->setFit(fit.first);
}

bool qm::ViewFitAction::isEnabled(const ActionEvent& event)
{
	MessageViewMode* pMode = pMessageViewModeHolder_->getMessageViewMode();
	if (!pMode)
		return false;
	
	return getParam(event.getParam()).second;
}

bool qm::ViewFitAction::isChecked(const ActionEvent& event)
{
	MessageViewMode* pMode = pMessageViewModeHolder_->getMessageViewMode();
	if (!pMode)
		return false;
	
	std::pair<MessageViewMode::Fit, bool> fit(getParam(event.getParam()));
	if (!fit.second)
		return false;
	
	return pMode->getFit() == fit.first;
}

std::pair<MessageViewMode::Fit, bool> qm::ViewFitAction::getParam(const ActionParam* pParam)
{
	const WCHAR* pwszFit = ActionParamUtil::getString(pParam, 0);
	if (!pwszFit)
		return std::make_pair(MessageViewMode::FIT_NONE, false);
	
	MessageViewMode::Fit fit = MessageViewMode::FIT_NONE;
	if (wcscmp(pwszFit, L"none") == 0)
		return std::make_pair(MessageViewMode::FIT_NONE, true);
	else if (wcscmp(pwszFit, L"normal") == 0)
		return std::make_pair(MessageViewMode::FIT_NORMAL, true);
	else if (wcscmp(pwszFit, L"super") == 0)
		return std::make_pair(MessageViewMode::FIT_SUPER, true);
	else
		return std::make_pair(MessageViewMode::FIT_NONE, false);
}


/****************************************************************************
 *
 * ViewFocusAction
 *
 */

qm::ViewFocusAction::ViewFocusAction(View* pViews[],
									 size_t nViewCount,
									 bool bNext) :
	bNext_(bNext)
{
	listView_.resize(nViewCount);
	std::copy(pViews, pViews + nViewCount, listView_.begin());
}

qm::ViewFocusAction::~ViewFocusAction()
{
}

void qm::ViewFocusAction::invoke(const ActionEvent& event)
{
	int nViewCount = static_cast<int>(listView_.size());
	
	int nView = 0;
	for (nView = 0; nView < nViewCount; ++nView) {
		if (listView_[nView]->isActive())
			break;
	}
	if (nView == nViewCount)
		nView = 0;
	
	int n = 0;
	View* pView = 0;
	if (bNext_) {
		for (n = nView + 1; n < nViewCount && !pView; ++n) {
			if (listView_[n]->isShow())
				pView = listView_[n];
		}
		for (n = 0; n < nView && !pView; ++n) {
			if (listView_[n]->isShow())
				pView = listView_[n];
		}
	}
	else {
		for (n = nView - 1; n >= 0 && !pView; --n) {
			if (listView_[n]->isShow())
				pView = listView_[n];
		}
		for (n = nViewCount - 1; n > nView && !pView; --n) {
			if (listView_[n]->isShow())
				pView = listView_[n];
		}
	}
	if (pView)
		pView->setActive();
}


/****************************************************************************
 *
 * ViewLockPreviewAction
 *
 */

qm::ViewLockPreviewAction::ViewLockPreviewAction(PreviewMessageModel* pPreviewMessageModel,
												 MainWindow* pMainWindow) :
	pPreviewMessageModel_(pPreviewMessageModel),
	pMainWindow_(pMainWindow)
{
}

qm::ViewLockPreviewAction::~ViewLockPreviewAction()
{
}

void qm::ViewLockPreviewAction::invoke(const ActionEvent& event)
{
	if (pPreviewMessageModel_->isConnectedToViewModel()) {
		pPreviewMessageModel_->disconnectFromViewModel();
		pPreviewMessageModel_->setMessage(0);
	}
	else {
		pPreviewMessageModel_->connectToViewModel();
		pPreviewMessageModel_->updateToViewModel();
	}
}

bool qm::ViewLockPreviewAction::isEnabled(const ActionEvent& event)
{
	return pMainWindow_->isShowPreviewWindow();
}


/****************************************************************************
 *
 * ViewMessageModeAction
 *
 */

qm::ViewMessageModeAction::ViewMessageModeAction(MessageViewModeHolder* pMessageViewModeHolder,
												 MessageViewMode::Mode mode,
												 MessageViewMode::Mode exclusiveMode,
												 bool bEnabled) :
	pMessageViewModeHolder_(pMessageViewModeHolder),
	mode_(mode),
	exclusiveMode_(exclusiveMode),
	bEnabled_(bEnabled)
{
}

qm::ViewMessageModeAction::~ViewMessageModeAction()
{
}

void qm::ViewMessageModeAction::invoke(const ActionEvent& event)
{
	MessageViewMode* pMode = pMessageViewModeHolder_->getMessageViewMode();
	if (pMode) {
		if (pMode->getMode() & mode_)
			pMode->setMode(0, mode_);
		else
			pMode->setMode(mode_, mode_ | exclusiveMode_);
	}
}

bool qm::ViewMessageModeAction::isEnabled(const ActionEvent& event)
{
	return bEnabled_ && pMessageViewModeHolder_->getMessageViewMode();
}

bool qm::ViewMessageModeAction::isChecked(const ActionEvent& event)
{
	MessageViewMode* pMode = pMessageViewModeHolder_->getMessageViewMode();
	return pMode && pMode->getMode() & mode_;
}


/****************************************************************************
 *
 * ViewNavigateFolderAction
 *
 */

qm::ViewNavigateFolderAction::ViewNavigateFolderAction(AccountManager* pAccountManager,
													   FolderModel* pFolderModel,
													   Type type) :
	pAccountManager_(pAccountManager),
	pFolderModel_(pFolderModel),
	type_(type)
{
}

qm::ViewNavigateFolderAction::~ViewNavigateFolderAction()
{
}

void qm::ViewNavigateFolderAction::invoke(const ActionEvent& event)
{
	std::pair<Account*, Folder*> p(pFolderModel_->getCurrent());
	Account* pAccount = p.first ? p.first : p.second ? p.second->getAccount() : 0;
	if (!pAccount)
		return;
	Folder* pFolder = p.second;
	bool bFolderSelected = pFolder != 0;
	
	switch (type_) {
	case TYPE_NEXTFOLDER:
	case TYPE_PREVFOLDER:
		if (pFolder) {
			const Account::FolderList& l = pAccount->getFolders();
			Account::FolderList::const_iterator it = std::find(
				l.begin(), l.end(), pFolder);
			assert(it != l.end());
			switch (type_) {
			case TYPE_NEXTFOLDER:
				++it;
				break;
			case TYPE_PREVFOLDER:
				it = it != l.begin() ? it - 1 : l.end();
				break;
			default:
				assert(false);
				break;
			}
			pFolder = it != l.end() ? *it : 0;
		}
		if (!pFolder) {
			const AccountManager::AccountList& l = pAccountManager_->getAccounts();
			AccountManager::AccountList::const_iterator it =
				std::find(l.begin(), l.end(), pAccount);
			assert(it != l.end());
			switch (type_) {
			case TYPE_NEXTFOLDER:
				if (bFolderSelected)
					++it;
				while (it != l.end() && !pFolder) {
					if (!(*it)->getFolders().empty())
						pFolder = (*it)->getFolders().front();
				}
				break;
			case TYPE_PREVFOLDER:
				it = it != l.begin() ? it - 1 : l.end();
				if (it != l.end()) {
					while (!pFolder) {
						if (!(*it)->getFolders().empty())
							pFolder = (*it)->getFolders().back();
						if (it == l.begin())
							break;
						--it;
					}
				}
				break;
			default:
				assert(false);
				break;
			}
		}
		if (pFolder)
			pFolderModel_->setCurrent(0, pFolder, true);
		break;
	case TYPE_NEXTACCOUNT:
	case TYPE_PREVACCOUNT:
		{
			const AccountManager::AccountList& l = pAccountManager_->getAccounts();
			AccountManager::AccountList::const_iterator it =
				std::find(l.begin(), l.end(), pAccount);
			assert(it != l.end());
			switch (type_) {
			case TYPE_NEXTACCOUNT:
				++it;
				break;
			case TYPE_PREVACCOUNT:
				if (!pFolder)
					it = it != l.begin() ? it - 1 : l.end();
				break;
			default:
				assert(false);
				break;
			}
			if (it != l.end())
				pFolderModel_->setCurrent(*it, 0, true);
		}
		break;
	default:
		break;
	}
}

bool qm::ViewNavigateFolderAction::isEnabled(const ActionEvent& event)
{
	std::pair<Account*, Folder*> p(pFolderModel_->getCurrent());
	return p.first || p.second;
}


/****************************************************************************
 *
 * ViewNavigateMessageAction
 *
 */

qm::ViewNavigateMessageAction::ViewNavigateMessageAction(ViewModelManager* pViewModelManager,
														 FolderModel* pFolderModel,
														 MainWindow* pMainWindow,
														 MessageWindow* pMessageWindow,
														 AccountManager* pAccountManager,
														 Profile* pProfile,
														 Type type) :
	pViewModelManager_(pViewModelManager),
	pFolderModel_(pFolderModel),
	pViewModelHolder_(0),
	pMainWindow_(pMainWindow),
	pMessageWindow_(pMessageWindow),
	pAccountManager_(pAccountManager),
	nType_(type)
{
	assert(pViewModelManager);
	assert(pMessageWindow);
	
	init(pProfile);
}

qm::ViewNavigateMessageAction::ViewNavigateMessageAction(ViewModelManager* pViewModelManager,
														 ViewModelHolder* pViewModelHolder,
														 MessageWindow* pMessageWindow,
														 AccountManager* pAccountManager,
														 Profile* pProfile,
														 Type type) :
	pViewModelManager_(pViewModelManager),
	pFolderModel_(0),
	pViewModelHolder_(pViewModelHolder),
	pMainWindow_(0),
	pMessageWindow_(pMessageWindow),
	pAccountManager_(pAccountManager),
	nType_(type)
{
	assert(pViewModelManager);
	assert(pMessageWindow);
	
	init(pProfile);
}

qm::ViewNavigateMessageAction::~ViewNavigateMessageAction()
{
}

void qm::ViewNavigateMessageAction::invoke(const ActionEvent& event)
{
	Type type = static_cast<Type>(nType_ & TYPE_TYPE_MASK);
	bool bPreview = pFolderModel_ != 0;
	assert((bPreview && pFolderModel_ && pMainWindow_) ||
		(!bPreview && !pFolderModel_ && !pMainWindow_));
	
	MessageModel* pMessageModel = pMessageWindow_->getMessageModel();
	
	if (bPreview && (type == TYPE_NEXTPAGE || type == TYPE_PREVPAGE)) {
		MessagePtrLock mpl(pMessageModel->getCurrentMessage());
		if (!mpl)
			type = TYPE_SELF;
	}
	
	if (bPreview &&
		(!pMainWindow_->isShowPreviewWindow() ||
		 !static_cast<PreviewMessageModel*>(pMessageModel)->isConnectedToViewModel()) &&
		(type == TYPE_NEXTPAGE || type == TYPE_NEXTPAGE || type == TYPE_SELF))
		return;
	
	bool bScrolled = true;
	bool bDelay = true;
	switch (type) {
	case TYPE_NEXTPAGE:
		if (pMessageWindow_->scrollPage(false))
			return;
		if (bPreview) {
			MessagePtrLock mpl(pMessageModel->getCurrentMessage());
			assert(mpl);
			if (!mpl->isFlag(MessageHolder::FLAG_SEEN))
				mpl->getAccount()->setMessagesFlags(MessageHolderList(1, mpl),
					MessageHolder::FLAG_SEEN, MessageHolder::FLAG_SEEN, 0);
		}
		type = nType_ & TYPE_NEXTPAGEUNSEEN ? TYPE_NEXTUNSEEN : TYPE_NEXT;
		bDelay = false;
		break;
	case TYPE_PREVPAGE:
		if (pMessageWindow_->scrollPage(true))
			return;
		type = TYPE_PREV;
		bDelay = false;
		break;
	default:
		break;
	}
	
	ViewModel* pViewModel = 0;
	if (bPreview) {
		pViewModel = pViewModelManager_->getCurrentViewModel();
		if (!pViewModel)
			return;
	}
	else {
		pViewModel = pViewModelHolder_->getViewModel();
	}
	
	Lock<ViewModel> lock(*pViewModel);
	
	unsigned int nCount = pViewModel->getCount();
	unsigned int nIndex = -1;
	if (bPreview) {
		if (nCount != 0)
			nIndex = pViewModel->getFocused();
	}
	else {
		MessagePtrLock mpl(pMessageModel->getCurrentMessage());
		if (mpl)
			nIndex = pViewModel->getIndex(mpl);
	}
	
	if (nIndex != -1 || (bPreview && type == TYPE_NEXTUNSEEN)) {
		ViewModel* pNewViewModel = pViewModel;
		
		switch (type) {
		case TYPE_NEXT:
			if (nIndex == nCount - 1)
				nIndex = -1;
			else
				++nIndex;
			break;
		case TYPE_PREV:
			if (nIndex == 0)
				nIndex = -1;
			else
				--nIndex;
			break;
		case TYPE_NEXTUNSEEN:
			{
				std::pair<ViewModel*, unsigned int> unseen(
					getNextUnseen(pViewModel, nIndex + 1));
				pNewViewModel = unseen.first;
				nIndex = unseen.second;
			}
			break;
		case TYPE_SELF:
			break;
		default:
			assert(false);
			break;
		}
		
		Lock<ViewModel> lockNew(*pNewViewModel);
		
		if (pNewViewModel != pViewModel) {
			if (bPreview)
				pFolderModel_->setCurrent(0, pNewViewModel->getFolder(), false);
			else
				pViewModelHolder_->setViewModel(pNewViewModel);
			pViewModel = pNewViewModel;
		}
		
		if (!bPreview || type == TYPE_SELF) {
			MessageHolder* pmh = 0;
			if (nIndex != -1)
				pmh = pViewModel->getMessageHolder(nIndex);
			pMessageModel->setMessage(pmh);
		}
		
		if (nIndex != -1 && type != TYPE_SELF)
			MessageActionUtil::select(pViewModel, nIndex, bDelay);
	}
	else {
		pMessageModel->setMessage(0);
	}
}

bool qm::ViewNavigateMessageAction::isEnabled(const ActionEvent& event)
{
	// TODO
	return true;
}

void qm::ViewNavigateMessageAction::init(qs::Profile* pProfile)
{
	if (nType_ == TYPE_NEXTPAGE &&
		pProfile->getInt(L"Global", L"NextUnseenWhenScrollEnd"))
		nType_ |= TYPE_NEXTPAGEUNSEEN;
	else if (nType_ == TYPE_NEXTUNSEEN &&
		pProfile->getInt(L"Global", L"NextUnseenInOtherAccounts"))
		nType_ |= TYPE_UNSEENINOTHERACCOUNT;
}

std::pair<ViewModel*, unsigned int> qm::ViewNavigateMessageAction::getNextUnseen(ViewModel* pViewModel,
																				 unsigned int nIndex) const
{
	std::pair<ViewModel*, unsigned int> unseen(pViewModel, -1);
	
	Lock<ViewModel> lock(*pViewModel);
	
	unsigned int nStart = nIndex;
	bool bFound = false;
	if (nIndex != -1) {
		unsigned int nCount = pViewModel->getCount();
		for (; nIndex < nCount; ++nIndex) {
			MessageHolder* pmh = pViewModel->getMessageHolder(nIndex);
			if (!pmh->isSeen()) {
				bFound = true;
				break;
			}
		}
		if (!bFound) {
			for (nIndex = 0; nIndex < nStart; ++nIndex) {
				MessageHolder* pmh = pViewModel->getMessageHolder(nIndex);
				if (!pmh->isSeen()) {
					bFound = true;
					break;
				}
			}
		}
	}
	if (!bFound) {
		Folder* pFolder = pViewModel->getFolder();
		Account* pAccount = pFolder->getAccount();
		Folder* pUnseenFolder = getNextUnseenFolder(pAccount, pFolder);
		if (!pUnseenFolder && nType_ & TYPE_UNSEENINOTHERACCOUNT)
			pUnseenFolder = getNextUnseenFolder(pAccount);
		if (pUnseenFolder) {
			pViewModel = pViewModelManager_->getViewModel(pUnseenFolder);
			unseen = getNextUnseen(pViewModel, 0);
		}
	}
	else {
		unseen.second = nIndex;
	}
	
	return unseen;
}

Folder* qm::ViewNavigateMessageAction::getNextUnseenFolder(Account* pAccount,
														   Folder* pFolderStart) const
{
	assert(pAccount);
	
	Account::FolderList l(pAccount->getFolders());
	std::sort(l.begin(), l.end(), FolderLess());
	
	Account::FolderList::iterator itStart = l.end();
	if (pFolderStart) {
		itStart = std::find(l.begin(), l.end(), pFolderStart);
		assert(itStart != l.end());
		
		Account::FolderList::const_iterator it = std::find_if(itStart + 1, l.end(),
			boost::bind(&ViewNavigateMessageAction::isUnseenFolder, this, _1));
		if (it != l.end())
			return *it;
	}
	
	Account::FolderList::const_iterator it = std::find_if(l.begin(), itStart,
		boost::bind(&ViewNavigateMessageAction::isUnseenFolder, this, _1));
	return it != itStart ? *it : 0;
}

Folder* qm::ViewNavigateMessageAction::getNextUnseenFolder(Account* pAccountStart) const
{
	assert(pAccountStart);
	
	AccountManager::AccountList l(pAccountManager_->getAccounts());
	std::sort(l.begin(), l.end(),
		binary_compose_f_gx_hy(
			string_less<WCHAR>(),
			std::mem_fun(&Account::getName),
			std::mem_fun(&Account::getName)));
	
	AccountManager::AccountList::const_iterator itStart =
		std::find(l.begin(), l.end(), pAccountStart);
	assert(itStart != l.end());
	
	for (AccountManager::AccountList::const_iterator it = itStart + 1; it != l.end(); ++it) {
		Folder* pFolder = getNextUnseenFolder(*it, 0);
		if (pFolder)
			return pFolder;
	}
	for (AccountManager::AccountList::const_iterator it = l.begin(); it != itStart; ++it) {
		Folder* pFolder = getNextUnseenFolder(*it, 0);
		if (pFolder)
			return pFolder;
	}
	return 0;
}

bool qm::ViewNavigateMessageAction::isUnseenFolder(const Folder* pFolder) const
{
	const unsigned int nIgnoreFlags = Folder::FLAG_TRASHBOX |
		Folder::FLAG_JUNKBOX | Folder::FLAG_IGNOREUNSEEN;
	if (pFolder->getFlags() & nIgnoreFlags)
		return false;
	else if (pFolder->isHidden())
		return false;
	else
		return pFolder->getUnseenCount() != 0;
}


/****************************************************************************
 *
 * ViewOpenLinkAction
 *
 */

qm::ViewOpenLinkAction::ViewOpenLinkAction(MessageWindow* pMessageWindow) :
	pMessageWindow_(pMessageWindow)
{
}

qm::ViewOpenLinkAction::~ViewOpenLinkAction()
{
}

void qm::ViewOpenLinkAction::invoke(const ActionEvent& event)
{
	if (!pMessageWindow_->openLink())
		ActionUtil::error(pMessageWindow_->getParentFrame(), IDS_ERROR_OPENLINK);
}


/****************************************************************************
 *
 * ViewRefreshAction
 *
 */

qm::ViewRefreshAction::ViewRefreshAction(SyncManager* pSyncManager,
										 Document* pDocument,
										 FolderModel* pFolderModel,
										 SecurityModel* pSecurityModel,
										 SyncDialogManager* pSyncDialogManager,
										 HWND hwnd,
										 Profile* pProfile) :
	pSyncManager_(pSyncManager),
	pDocument_(pDocument),
	pFolderModel_(pFolderModel),
	pSecurityModel_(pSecurityModel),
	pSyncDialogManager_(pSyncDialogManager),
	hwnd_(hwnd),
	pProfile_(pProfile)
{
}

qm::ViewRefreshAction::~ViewRefreshAction()
{
}

void qm::ViewRefreshAction::invoke(const ActionEvent& event)
{
	Folder* pFolder = pFolderModel_->getCurrent().second;
	if (!pFolder)
		return;
	
	switch (pFolder->getType()) {
	case Folder::TYPE_NORMAL:
		if (pFolder->isFlag(Folder::FLAG_SYNCABLE)) {
			if (!SyncUtil::syncFolder(pSyncManager_, pDocument_, pSyncDialogManager_,
				SyncData::TYPE_ACTIVE, static_cast<NormalFolder*>(pFolder), 0)) {
				ActionUtil::error(hwnd_, IDS_ERROR_REFRESH);
				return;
			}
		}
		break;
	case Folder::TYPE_QUERY:
		if (!static_cast<QueryFolder*>(pFolder)->search(pDocument_,
			hwnd_, pProfile_, pSecurityModel_->getSecurityMode())) {
			ActionUtil::error(hwnd_, IDS_ERROR_REFRESH);
			return;
		}
		break;
	default:
		assert(false);
		break;
	}
}

bool qm::ViewRefreshAction::isEnabled(const ActionEvent& event)
{
	Folder* pFolder = pFolderModel_->getCurrent().second;
	return pFolder &&
		(pFolder->getType() == Folder::TYPE_QUERY ||
		pFolder->isFlag(Folder::FLAG_SYNCABLE));
}


/****************************************************************************
 *
 * ViewSecurityAction
 *
 */

qm::ViewSecurityAction::ViewSecurityAction(SecurityModel* pSecurityModel,
										   SecurityMode mode,
										   bool bEnabled) :
	pSecurityModel_(pSecurityModel),
	mode_(mode),
	bEnabled_(bEnabled)
{
}

qm::ViewSecurityAction::~ViewSecurityAction()
{
}

void qm::ViewSecurityAction::invoke(const ActionEvent& event)
{
	pSecurityModel_->setSecurityMode(mode_,
		(pSecurityModel_->getSecurityMode() & mode_) == 0);
}

bool qm::ViewSecurityAction::isEnabled(const ActionEvent& event)
{
	return bEnabled_;
}

bool qm::ViewSecurityAction::isChecked(const ActionEvent& event)
{
	return (pSecurityModel_->getSecurityMode() & mode_) != 0;
}


/****************************************************************************
 *
 * ViewScrollAction
 *
 */

qm::ViewScrollAction::ViewScrollAction(HWND hwnd,
									   Scroll scroll) :
	hwnd_(hwnd),
	nMsg_(scroll & SCROLL_VERTICAL_MASK ? WM_VSCROLL : WM_HSCROLL),
	nRequest_(0)
{
	struct Request {
		Scroll scroll_;
		int nRequest_;
	} requests[] = {
		{ SCROLL_LINEUP,	SB_LINEUP		},
		{ SCROLL_LINEDOWN,	SB_LINEDOWN		},
		{ SCROLL_PAGEUP,	SB_PAGEUP		},
		{ SCROLL_PAGEDOWN,	SB_PAGEDOWN		},
		{ SCROLL_TOP,		SB_TOP			},
		{ SCROLL_BOTTOM,	SB_BOTTOM		},
		{ SCROLL_LINELEFT,	SB_LINELEFT		},
		{ SCROLL_LINERIGHT,	SB_LINERIGHT	},
		{ SCROLL_PAGELEFT,	SB_PAGELEFT		},
		{ SCROLL_PAGERIGHT,	SB_PAGERIGHT	},
		{ SCROLL_LEFT,		SB_LEFT			},
		{ SCROLL_RIGHT,		SB_RIGHT		}
	};
	for (int n = 0; n < countof(requests); ++n) {
		if (scroll == requests[n].scroll_) {
			nRequest_ = requests[n].nRequest_;
			break;
		}
	}
}

qm::ViewScrollAction::~ViewScrollAction()
{
}

void qm::ViewScrollAction::invoke(const ActionEvent& event)
{
	Window(hwnd_).sendMessage(nMsg_, MAKEWPARAM(nRequest_, 0));
}


/****************************************************************************
 *
 * ViewSelectMessageAction
 *
 */

qm::ViewSelectMessageAction::ViewSelectMessageAction(ViewModelManager* pViewModelManager,
													 FolderModel* pFolderModel,
													 MessageSelectionModel* pMessageSelectionModel) :
													 pViewModelManager_(pViewModelManager),
													 pFolderModel_(pFolderModel),
													 pMessageSelectionModel_(pMessageSelectionModel)
{
}

qm::ViewSelectMessageAction::~ViewSelectMessageAction()
{
}

void qm::ViewSelectMessageAction::invoke(const ActionEvent& event)
{
	ViewModel* pViewModel = pViewModelManager_->getCurrentViewModel();
	if (!pViewModel)
		return;
	
	Lock<ViewModel> lock(*pViewModel);
	
	MessagePtr ptr(pMessageSelectionModel_->getFocusedMessage());
	MessagePtrLock mpl(ptr);
	if (mpl) {
		NormalFolder* pFolder = mpl->getFolder();
		if (pFolder != pViewModel->getFolder()) {
			pFolderModel_->setCurrent(0, pFolder, false);
			
			pViewModel = pViewModelManager_->getViewModel(pFolder);
			Lock<ViewModel> lock(*pViewModel);
			unsigned int nIndex = pViewModel->getIndex(mpl);
			if (nIndex != -1)
				MessageActionUtil::select(pViewModel, nIndex, false);
		}
	}
}

bool qm::ViewSelectMessageAction::isEnabled(const ActionEvent& event)
{
	return pViewModelManager_->getCurrentViewModel() &&
		pMessageSelectionModel_->hasFocusedMessage();
}


/****************************************************************************
 *
 * ViewShowFolderAction
 *
 */

qm::ViewShowFolderAction::ViewShowFolderAction(MainWindow* pMainWindow) :
	ViewShowControlAction<MainWindow>(pMainWindow,
		&qm::MainWindow::setShowFolderWindow,
		&qm::MainWindow::isShowFolderWindow,
		IDS_ACTION_SHOWFOLDER, IDS_ACTION_HIDEFOLDER)
{
}

qm::ViewShowFolderAction::~ViewShowFolderAction()
{
}


/****************************************************************************
 *
 * ViewShowHeaderAction
 *
 */

qm::ViewShowHeaderAction::ViewShowHeaderAction(MessageWindow* pMessageWindow) :
	ViewShowControlAction<MessageWindow>(pMessageWindow,
		&qm::MessageWindow::setShowHeaderWindow,
		&qm::MessageWindow::isShowHeaderWindow,
		IDS_ACTION_SHOWHEADER, IDS_ACTION_HIDEHEADER)
{
}

qm::ViewShowHeaderAction::~ViewShowHeaderAction()
{
}


/****************************************************************************
 *
 * ViewShowHeaderColumnAction
 *
 */

qm::ViewShowHeaderColumnAction::ViewShowHeaderColumnAction(ListWindow* pListWindow) :
	ViewShowControlAction<ListWindow>(pListWindow,
		&qm::ListWindow::setShowHeaderColumn,
		&qm::ListWindow::isShowHeaderColumn,
		IDS_ACTION_SHOWHEADERCOLUMN, IDS_ACTION_HIDEHEADERCOLUMN)
{
}

qm::ViewShowHeaderColumnAction::~ViewShowHeaderColumnAction()
{
}


/****************************************************************************
 *
 * ViewShowPreviewAction
 *
 */

qm::ViewShowPreviewAction::ViewShowPreviewAction(MainWindow* pMainWindow) :
	ViewShowControlAction<MainWindow>(pMainWindow,
		&qm::MainWindow::setShowPreviewWindow,
		&qm::MainWindow::isShowPreviewWindow,
		IDS_ACTION_SHOWPREVIEW, IDS_ACTION_HIDEPREVIEW)
{
}

qm::ViewShowPreviewAction::~ViewShowPreviewAction()
{
}


/****************************************************************************
 *
 * ViewShowSyncDialogAction
 *
 */

qm::ViewShowSyncDialogAction::ViewShowSyncDialogAction(SyncDialogManager* pManager) :
	pManager_(pManager)
{
}

qm::ViewShowSyncDialogAction::~ViewShowSyncDialogAction()
{
}

void qm::ViewShowSyncDialogAction::invoke(const ActionEvent& event)
{
	SyncDialog* pDialog = pManager_->open();
	if (pDialog)
		pDialog->show();
}


#ifdef QMTABWINDOW
/****************************************************************************
 *
 * ViewShowTabAction
 *
 */

qm::ViewShowTabAction::ViewShowTabAction(TabWindow* pTabWindow) :
	ViewShowControlAction<TabWindow>(pTabWindow, &qm::TabWindow::setShowTab,
		&qm::TabWindow::isShowTab, IDS_ACTION_SHOWTAB, IDS_ACTION_HIDETAB)
{
}

qm::ViewShowTabAction::~ViewShowTabAction()
{
}
#endif


/****************************************************************************
 *
 * ViewSortAction
 *
 */

qm::ViewSortAction::ViewSortAction(ViewModelManager* pViewModelManager) :
	pViewModelManager_(pViewModelManager)
{
}

qm::ViewSortAction::~ViewSortAction()
{
}

void qm::ViewSortAction::invoke(const ActionEvent& event)
{
	ViewModel* pViewModel = pViewModelManager_->getCurrentViewModel();
	if (pViewModel) {
		unsigned int nColumn = getColumn(pViewModel, event.getParam());
		if (nColumn == -1)
			return;
		pViewModel->setSort(nColumn, ViewModel::SORT_INDEX_MASK);
	}
}

bool qm::ViewSortAction::isEnabled(const ActionEvent& event)
{
	return pViewModelManager_->getCurrentViewModel() != 0;
}

bool qm::ViewSortAction::isChecked(const ActionEvent& event)
{
	ViewModel* pViewModel = pViewModelManager_->getCurrentViewModel();
	if (!pViewModel)
		return false;
	
	unsigned int nColumn = getColumn(pViewModel, event.getParam());
	if (nColumn == -1)
		return false;
	return nColumn == (pViewModel->getSort() & ViewModel::SORT_INDEX_MASK);
}

unsigned int qm::ViewSortAction::getColumn(const ViewModel* pViewModel,
										   const ActionParam* pParam) const
{
	unsigned int nColumn = ActionParamUtil::getIndex(pParam, 0);
	if (nColumn == -1 || nColumn >= pViewModel->getColumnCount())
		return -1;
	return nColumn;
}


/****************************************************************************
 *
 * ViewSortDirectionAction
 *
 */

qm::ViewSortDirectionAction::ViewSortDirectionAction(ViewModelManager* pViewModelManager,
													 bool bAscending) :
	pViewModelManager_(pViewModelManager),
	bAscending_(bAscending)
{
}

qm::ViewSortDirectionAction::~ViewSortDirectionAction()
{
}

void qm::ViewSortDirectionAction::invoke(const ActionEvent& event)
{
	ViewModel* pViewModel = pViewModelManager_->getCurrentViewModel();
	if (pViewModel) {
		unsigned int nSort = bAscending_ ?
			ViewModel::SORT_ASCENDING : ViewModel::SORT_DESCENDING;
		pViewModel->setSort(nSort, ViewModel::SORT_DIRECTION_MASK);
	}
}

bool qm::ViewSortDirectionAction::isEnabled(const ActionEvent& event)
{
	return pViewModelManager_->getCurrentViewModel() != 0;
}

bool qm::ViewSortDirectionAction::isChecked(const ActionEvent& event)
{
	ViewModel* pViewModel = pViewModelManager_->getCurrentViewModel();
	if (pViewModel) {
		unsigned int nSort = pViewModel->getSort();
		return (bAscending_ &&
			(nSort & ViewModel::SORT_DIRECTION_MASK) ==
				ViewModel::SORT_ASCENDING) ||
			(!bAscending_ &&
				(nSort & ViewModel::SORT_DIRECTION_MASK) ==
					ViewModel::SORT_DESCENDING);
	}
	else {
		return false;
	}
}


/****************************************************************************
 *
 * ViewSortThreadAction
 *
 */

qm::ViewSortThreadAction::ViewSortThreadAction(ViewModelManager* pViewModelManager,
											   Type type) :
	pViewModelManager_(pViewModelManager),
	type_(type)
{
}

qm::ViewSortThreadAction::~ViewSortThreadAction()
{
}

void qm::ViewSortThreadAction::invoke(const ActionEvent& event)
{
	ViewModel* pViewModel = pViewModelManager_->getCurrentViewModel();
	if (!pViewModel)
		return;
	
	unsigned int nSort = 0;
	switch (type_) {
	case TYPE_FLAT:
		nSort = ViewModel::SORT_NOTHREAD;
		break;
	case TYPE_THREAD:
		nSort = ViewModel::SORT_THREAD;
		break;
	case TYPE_FLOATTHREAD:
		nSort = ViewModel::SORT_THREAD | ViewModel::SORT_FLOATTHREAD;
		break;
	case TYPE_TOGGLETHREAD:
		nSort = pViewModel->getSort();
		if ((nSort & ViewModel::SORT_THREAD_MASK) == ViewModel::SORT_THREAD)
			nSort = (nSort & ~ViewModel::SORT_THREAD_MASK) | ViewModel::SORT_NOTHREAD;
		else
			nSort = (nSort & ~ViewModel::SORT_THREAD_MASK) | ViewModel::SORT_THREAD;
		break;
	default:
		assert(false);
		break;
	}
	pViewModel->setSort(nSort, ViewModel::SORT_THREAD_MASK | ViewModel::SORT_FLOATTHREAD);
}

bool qm::ViewSortThreadAction::isEnabled(const ActionEvent& event)
{
	return pViewModelManager_->getCurrentViewModel() != 0;
}

bool qm::ViewSortThreadAction::isChecked(const ActionEvent& event)
{
	ViewModel* pViewModel = pViewModelManager_->getCurrentViewModel();
	if (!pViewModel)
		return false;
	
	unsigned int nSort = pViewModel->getSort();
	switch (type_) {
	case TYPE_FLAT:
		return (nSort & ViewModel::SORT_THREAD_MASK) == ViewModel::SORT_NOTHREAD;
	case TYPE_THREAD:
		return (nSort & ViewModel::SORT_THREAD_MASK) == ViewModel::SORT_THREAD &&
			!(nSort & ViewModel::SORT_FLOATTHREAD);
	case TYPE_FLOATTHREAD:
		return (nSort & ViewModel::SORT_THREAD_MASK) == ViewModel::SORT_THREAD &&
			(nSort & ViewModel::SORT_FLOATTHREAD);
	case TYPE_TOGGLETHREAD:
		return (nSort & ViewModel::SORT_THREAD_MASK) == ViewModel::SORT_THREAD;
	default:
		assert(false);
		return false;
	}
}


/****************************************************************************
 *
 * ViewTemplateAction
 *
 */

qm::ViewTemplateAction::ViewTemplateAction(MessageWindow* pMessageWindow) :
	pMessageWindow_(pMessageWindow)
{
}

qm::ViewTemplateAction::~ViewTemplateAction()
{
}

void qm::ViewTemplateAction::invoke(const ActionEvent& event)
{
	const WCHAR* pwszTemplate = ActionParamUtil::getString(event.getParam(), 0);
	if (!pwszTemplate)
		return;
	else if (*pwszTemplate)
		pMessageWindow_->setTemplate(pwszTemplate);
	else
		pMessageWindow_->setTemplate(0);
}

bool qm::ViewTemplateAction::isEnabled(const ActionEvent& event)
{
	return ActionParamUtil::getString(event.getParam(), 0) != 0;
}

bool qm::ViewTemplateAction::isChecked(const ActionEvent& event)
{
	const WCHAR* pwszTemplate = ActionParamUtil::getString(event.getParam(), 0);
	if (!pwszTemplate)
		return false;
	
	const WCHAR* pwszCurrentTemplate = pMessageWindow_->getTemplate();
	if (*pwszTemplate)
		return pwszCurrentTemplate && wcscmp(pwszTemplate, pwszCurrentTemplate) == 0;
	else
		return !pwszCurrentTemplate;
}

Account* qm::ViewTemplateAction::getAccount() const
{
	MessageModel* pMessageModel = pMessageWindow_->getMessageModel();
	return pMessageModel->getCurrentAccount();
}


/****************************************************************************
 *
 * ViewZoomAction
 *
 */

qm::ViewZoomAction::ViewZoomAction(MessageViewModeHolder* pMessageViewModeHolder) :
	pMessageViewModeHolder_(pMessageViewModeHolder)
{
}

qm::ViewZoomAction::~ViewZoomAction()
{
}

void qm::ViewZoomAction::invoke(const ActionEvent& event)
{
	MessageViewMode* pMode = pMessageViewModeHolder_->getMessageViewMode();
	if (!pMode)
		return;
	
	unsigned int nZoom = -1;
	
	std::pair<Type, unsigned int> zoom(getParam(event.getParam()));
	switch (zoom.first) {
	case TYPE_ZOOM:
		nZoom = zoom.second;
		break;
	case TYPE_INCREMENT:
		nZoom = pMode->getZoom();
		if (nZoom == MessageViewMode::ZOOM_NONE || nZoom == MessageViewMode::ZOOM_MAX)
			return;
		++nZoom;
		break;
	case TYPE_DECREMENT:
		nZoom = pMode->getZoom();
		if (nZoom == MessageViewMode::ZOOM_NONE || nZoom == MessageViewMode::ZOOM_MIN)
			return;
		--nZoom;
		break;
	case TYPE_ERROR:
		return;
	default:
		assert(false);
		return;
	}
	
	pMode->setZoom(nZoom);
}

bool qm::ViewZoomAction::isEnabled(const ActionEvent& event)
{
	MessageViewMode* pMode = pMessageViewModeHolder_->getMessageViewMode();
	if (!pMode)
		return false;
	
	std::pair<Type, unsigned int> zoom(getParam(event.getParam()));
	switch (zoom.first) {
	case TYPE_ZOOM:
		return true;
	case TYPE_INCREMENT:
	case TYPE_DECREMENT:
		return pMode->getZoom() != MessageViewMode::ZOOM_NONE;
	case TYPE_ERROR:
		return false;
	default:
		assert(false);
		return false;
	}
}

bool qm::ViewZoomAction::isChecked(const ActionEvent& event)
{
	MessageViewMode* pMode = pMessageViewModeHolder_->getMessageViewMode();
	if (!pMode)
		return false;
	
	std::pair<Type, unsigned int> zoom(getParam(event.getParam()));
	if (zoom.first != TYPE_ZOOM)
		return false;
	
	return pMode->getZoom() == zoom.second;
}

std::pair<ViewZoomAction::Type, unsigned int> qm::ViewZoomAction::getParam(const ActionParam* pParam)
{
	const WCHAR* pwszZoom = ActionParamUtil::getString(pParam, 0);
	if (pwszZoom) {
		if (*pwszZoom == L'+') {
			return std::pair<ViewZoomAction::Type, unsigned int>(TYPE_INCREMENT, -1);
		}
		else if (*pwszZoom == L'-') {
			return std::pair<ViewZoomAction::Type, unsigned int>(TYPE_DECREMENT, -1);
		}
		else {
			WCHAR* pEnd = 0;
			unsigned int nZoom = static_cast<unsigned int>(wcstol(pwszZoom, &pEnd, 10));
			if (*pEnd || nZoom < MessageViewMode::ZOOM_MIN || MessageViewMode::ZOOM_MAX < nZoom)
				return std::pair<ViewZoomAction::Type, unsigned int>(TYPE_ERROR, -1);
			else
				return std::pair<ViewZoomAction::Type, unsigned int>(TYPE_ZOOM, nZoom);
		}
	}
	else {
		return std::pair<ViewZoomAction::Type, unsigned int>(TYPE_ZOOM, -1);
	}
}


/****************************************************************************
 *
 * ActionUtil
 *
 */

void qm::ActionUtil::info(HWND hwnd,
						  UINT nMessage)
{
	messageBox(Application::getApplication().getResourceHandle(),
		nMessage, MB_OK | MB_ICONINFORMATION, hwnd);
}

void qm::ActionUtil::error(HWND hwnd,
						   UINT nMessage)
{
	messageBox(Application::getApplication().getResourceHandle(),
		nMessage, MB_OK | MB_ICONERROR, hwnd);
}

void qm::ActionUtil::error(HWND hwnd,
						   const WCHAR* pwszMessage)
{
	messageBox(pwszMessage, MB_OK | MB_ICONERROR, hwnd);
}


/****************************************************************************
 *
 * ActionParamUtil
 *
 */

const WCHAR* qm::ActionParamUtil::getString(const qs::ActionParam* pParam,
											size_t n)
{
	if (!pParam || pParam->getCount() <= n)
		return 0;
	return pParam->getValue(n);
}

unsigned int qm::ActionParamUtil::getNumber(const qs::ActionParam* pParam,
											size_t n)
{
	const WCHAR* pwszParam = getString(pParam, n);
	if (!pwszParam)
		return -1;
	
	WCHAR* pEnd = 0;
	long nParam = wcstol(pwszParam, &pEnd, 10);
	if (*pEnd)
		return -1;
	
	return static_cast<unsigned int>(nParam);
}

unsigned int qm::ActionParamUtil::getIndex(const qs::ActionParam* pParam,
										   size_t n)
{
	const WCHAR* pwsz = getString(pParam, n);
	if (!pwsz || *pwsz != L'@')
		return -1;
	
	WCHAR* pEnd = 0;
	unsigned int nIndex = wcstol(pwsz + 1, &pEnd, 10);
	if (*pEnd)
		return -1;
	
	return nIndex;
}

std::pair<const WCHAR*, unsigned int> qm::ActionParamUtil::getStringOrIndex(const ActionParam* pParam,
																			size_t n)
{
	if (!pParam || pParam->getCount() <= n)
		return std::pair<const WCHAR*, unsigned int>(0, -1);
	
	const WCHAR* pwszParam = pParam->getValue(n);
	
	if (*pwszParam == L'@') {
		WCHAR* pEnd = 0;
		unsigned int nParam = wcstol(pwszParam + 1, &pEnd, 10);
		if (!*pEnd)
			return std::pair<const WCHAR*, unsigned int>(0, nParam);
	}
	
	return std::pair<const WCHAR*, unsigned int>(pwszParam, -1);
}


/****************************************************************************
 *
 * FolderActionUtil
 *
 */

std::pair<Account*, Folder*> qm::FolderActionUtil::getCurrent(const FolderModelBase* pModel)
{
	std::pair<Account*, Folder*> p(pModel->getTemporary());
	if (!p.first && !p.second)
		p = pModel->getCurrent();
	return p;
}

Account* qm::FolderActionUtil::getAccount(const FolderModelBase* pModel)
{
	std::pair<Account*, Folder*> p(getCurrent(pModel));
	return p.first ? p.first : p.second ? p.second->getAccount() : 0;
}

Folder* qm::FolderActionUtil::getFolder(const FolderModelBase* pModel)
{
	return getCurrent(pModel).second;
}


/****************************************************************************
 *
 * MessageActionUtil
 *
 */

void qm::MessageActionUtil::select(ViewModel* pViewModel,
								   unsigned int nIndex,
								   MessageModel* pMessageModel)
{
	assert(pViewModel);
	assert(pViewModel->isLocked());
	assert(nIndex < pViewModel->getCount());
	
	if (pMessageModel) {
		MessageHolder* pmh = pViewModel->getMessageHolder(nIndex);
		pMessageModel->setMessage(pmh);
	}
	
	select(pViewModel, nIndex, true);
}

void qm::MessageActionUtil::select(ViewModel* pViewModel,
								   unsigned int nIndex,
								   bool bDelay)
{
	assert(pViewModel);
	assert(pViewModel->isLocked());
	assert(nIndex < pViewModel->getCount());
	
	pViewModel->setFocused(nIndex, bDelay);
	pViewModel->setSelection(nIndex);
	pViewModel->setLastSelection(nIndex);
	pViewModel->payAttention(nIndex);
}


#ifdef QMTABWINDOW
/****************************************************************************
 *
 * TabActionUtil
 *
 */

int TabActionUtil::getCurrent(TabModel* pModel)
{
	int nItem = pModel->getTemporary();
	return nItem != -1 ? nItem : pModel->getCurrent();
}
#endif
