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
#include <qmfolder.h>
#include <qmfolderlistwindow.h>
#include <qmgoround.h>
#include <qmlistwindow.h>
#include <qmmainwindow.h>
#include <qmmessage.h>
#include <qmmessageholder.h>
#include <qmmessagewindow.h>
#include <qmsearch.h>
#include <qmtemplate.h>

#include <qmscript.h>

#include <qsconv.h>
#include <qsnew.h>
#include <qsstl.h>
#include <qsstream.h>
#include <qswindow.h>

#include <algorithm>

#include <tchar.h>

#include "action.h"
#include "findreplace.h"
#include "../model/dataobject.h"
#include "../model/filter.h"
#include "../model/goround.h"
#include "../model/rule.h"
#include "../model/tempfilecleaner.h"
#include "../model/templatemanager.h"
#include "../script/scriptmanager.h"
#include "../sync/syncmanager.h"
#include "../ui/attachmentselectionmodel.h"
#include "../ui/dialogs.h"
#include "../ui/editframewindow.h"
#include "../ui/foldermodel.h"
#include "../ui/folderselectionmodel.h"
#include "../ui/menus.h"
#include "../ui/messagemodel.h"
#include "../ui/messageselectionmodel.h"
#include "../ui/propertypages.h"
#include "../ui/resourceinc.h"
#include "../ui/syncdialog.h"
#include "../ui/syncutil.h"
#include "../ui/uiutil.h"
#include "../ui/viewmodel.h"

#pragma warning(disable:4786)

using namespace qm;
using namespace qmscript;
using namespace qs;


/****************************************************************************
 *
 * AttachmentOpenAction
 *
 */

qm::AttachmentOpenAction::AttachmentOpenAction(MessageModel* pMessageModel,
	AttachmentSelectionModel* pAttachmentSelectionModel, Profile* pProfile,
	TempFileCleaner* pTempFileCleaner, HWND hwnd,  QSTATUS* pstatus) :
	pMessageModel_(pMessageModel),
	pAttachmentSelectionModel_(pAttachmentSelectionModel),
	helper_(pProfile, pTempFileCleaner, hwnd)
{
}

qm::AttachmentOpenAction::~AttachmentOpenAction()
{
}

QSTATUS qm::AttachmentOpenAction::invoke(const ActionEvent& event)
{
	DECLARE_QSTATUS();
	
	MessagePtrLock mpl(pMessageModel_->getCurrentMessage());
	if (mpl) {
		Message msg(&status);
		CHECK_QSTATUS();
		status = mpl->getMessage(Account::GETMESSAGEFLAG_ALL, 0, &msg);
		CHECK_QSTATUS();
		
		AttachmentParser parser(msg);
		AttachmentParser::AttachmentList listAttachment;
		AttachmentParser::AttachmentListFree freeAttachment(listAttachment);
		status = parser.getAttachments(false, &listAttachment);
		CHECK_QSTATUS();
		if (listAttachment.empty())
			return QSTATUS_SUCCESS;
		
		AttachmentSelectionModel::NameList listName;
		StringListFree<AttachmentSelectionModel::NameList> freeName(listName);
		status = pAttachmentSelectionModel_->getSelectedAttachment(&listName);
		CHECK_QSTATUS();
		AttachmentSelectionModel::NameList::const_iterator itN = listName.begin();
		while (itN != listName.end()) {
			AttachmentParser::AttachmentList::const_iterator itA = std::find_if(
				listAttachment.begin(), listAttachment.end(),
				std::bind2nd(
					binary_compose_f_gx_hy(
						string_equal<WCHAR>(),
						std::select1st<AttachmentParser::AttachmentList::value_type>(),
						std::identity<const WCHAR*>()),
					*itN));
			if (itA != listAttachment.end()) {
				status = helper_.open((*itA).second, *itN,
					(event.getModifier() & ActionEvent::MODIFIER_SHIFT) != 0);
				CHECK_QSTATUS();
			}
			++itN;
		}
	}
	
	return QSTATUS_SUCCESS;
}

QSTATUS qm::AttachmentOpenAction::isEnabled(
	const ActionEvent& event, bool* pbEnabled)
{
	return pAttachmentSelectionModel_->hasSelectedAttachment(pbEnabled);
}


/****************************************************************************
 *
 * AttachmentSaveAction
 *
 */

qm::AttachmentSaveAction::AttachmentSaveAction(MessageModel* pMessageModel,
	AttachmentSelectionModel* pAttachmentSelectionModel,
	bool bAll, Profile* pProfile, HWND hwnd, QSTATUS* pstatus) :
	pMessageModel_(pMessageModel),
	pAttachmentSelectionModel_(pAttachmentSelectionModel),
	bAll_(bAll),
	helper_(pProfile, 0, hwnd)
{
}

qm::AttachmentSaveAction::~AttachmentSaveAction()
{
}

QSTATUS qm::AttachmentSaveAction::invoke(const ActionEvent& event)
{
	DECLARE_QSTATUS();
	
	MessagePtrLock mpl(pMessageModel_->getCurrentMessage());
	if (!mpl)
		return QSTATUS_SUCCESS;
	
	MessageHolderList listMessageHolder;
	status = STLWrapper<MessageHolderList>(listMessageHolder).push_back(mpl);
	CHECK_QSTATUS();
	
	if (bAll_) {
		status = helper_.detach(listMessageHolder, 0);
		CHECK_QSTATUS();
	}
	else {
		AttachmentSelectionModel::NameList listName;
		StringListFree<AttachmentSelectionModel::NameList> freeName(listName);
		status = pAttachmentSelectionModel_->getSelectedAttachment(&listName);
		CHECK_QSTATUS();
		
		AttachmentHelper::NameList l;
		status = STLWrapper<AttachmentHelper::NameList>(l).resize(listName.size());
		CHECK_QSTATUS();
		std::copy(listName.begin(), listName.end(), l.begin());
		
		status = helper_.detach(listMessageHolder, &l);
		CHECK_QSTATUS();
	}
	
	return QSTATUS_SUCCESS;
}

QSTATUS qm::AttachmentSaveAction::isEnabled(
	const ActionEvent& event, bool* pbEnabled)
{
	return bAll_ ? pAttachmentSelectionModel_->hasAttachment(pbEnabled) :
		pAttachmentSelectionModel_->hasSelectedAttachment(pbEnabled);
}


/****************************************************************************
 *
 * DispatchAction
 *
 */

qm::DispatchAction::DispatchAction(View* pViews[],
	Action* pActions[], size_t nCount, QSTATUS* pstatus)
{
	DECLARE_QSTATUS();
	
	status = STLWrapper<ItemList>(listItem_).resize(nCount);
	CHECK_QSTATUS_SET(pstatus);
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
			deleter<Action>(),
			mem_data_ref(&Item::pAction_)));
}

QSTATUS qm::DispatchAction::invoke(const ActionEvent& event)
{
	Action* pAction = getAction();
	return pAction ? pAction->invoke(event) : QSTATUS_SUCCESS;
}

QSTATUS qm::DispatchAction::isEnabled(const ActionEvent& event, bool* pbEnabled)
{
	Action* pAction = getAction();
	if (pAction) {
		return pAction->isEnabled(event, pbEnabled);
	}
	else {
		*pbEnabled = false;
		return QSTATUS_SUCCESS;
	}
}

QSTATUS qm::DispatchAction::isChecked(const ActionEvent& event, bool* pbChecked)
{
	Action* pAction = getAction();
	if (pAction) {
		return pAction->isChecked(event, pbChecked);
	}
	else {
		*pbChecked = false;
		return QSTATUS_SUCCESS;
	}
}

QSTATUS qm::DispatchAction::getText(const ActionEvent& event, WSTRING* pwstrText)
{
	Action* pAction = getAction();
	if (pAction) {
		return pAction->getText(event, pwstrText);
	}
	else {
		*pwstrText = 0;
		return QSTATUS_SUCCESS;
	}
}

Action* qm::DispatchAction::getAction() const
{
	ItemList::const_iterator it = listItem_.begin();
	while (it != listItem_.end()) {
		if ((*it).pView_->isActive())
			return (*it).pAction_;
		++it;
	}
	return 0;
}


/****************************************************************************
 *
 * EditClearDeletedAction
 *
 */

qm::EditClearDeletedAction::EditClearDeletedAction(
	FolderModel* pFolderModel, QSTATUS* pstatus) :
	pFolderModel_(pFolderModel)
{
}

qm::EditClearDeletedAction::~EditClearDeletedAction()
{
}

QSTATUS qm::EditClearDeletedAction::invoke(const ActionEvent& event)
{
	DECLARE_QSTATUS();
	
	Folder* pFolder = pFolderModel_->getCurrentFolder();
	if (!pFolder ||
		pFolder->getType() != Folder::TYPE_NORMAL ||
		pFolder->isFlag(Folder::FLAG_NOSELECT) ||
		pFolder->isFlag(Folder::FLAG_LOCAL))
		return QSTATUS_FAIL;
	
	Account* pAccount = pFolder->getAccount();
	status = pAccount->clearDeletedMessages(
		static_cast<NormalFolder*>(pFolder));
	CHECK_QSTATUS();
	
	return QSTATUS_SUCCESS;
}

QSTATUS qm::EditClearDeletedAction::isEnabled(
	const ActionEvent& event, bool* pbEnabled)
{
	assert(pbEnabled);
	
	Folder* pFolder = pFolderModel_->getCurrentFolder();
	*pbEnabled = pFolder &&
		pFolder->getType() == Folder::TYPE_NORMAL &&
		!pFolder->isFlag(Folder::FLAG_NOSELECT) &&
		!pFolder->isFlag(Folder::FLAG_LOCAL);
	
	return QSTATUS_SUCCESS;
}


/****************************************************************************
 *
 * EditCommandAction
 *
 */

qm::EditCommandAction::EditCommandAction(MessageWindow* pMessageWindow,
	PFN_DO pfnDo, PFN_CANDO pfnCanDo, QSTATUS* pstatus) :
	pMessageWindow_(pMessageWindow),
	pfnDo_(pfnDo),
	pfnCanDo_(pfnCanDo)
{
}

qm::EditCommandAction::~EditCommandAction()
{
}

QSTATUS qm::EditCommandAction::invoke(const ActionEvent& event)
{
	DECLARE_QSTATUS();
	
	MessageWindowItem* pItem = pMessageWindow_->getFocusedItem();
	if (pItem) {
		status = (pItem->*pfnDo_)();
		CHECK_QSTATUS();
	}
	
	return QSTATUS_SUCCESS;
}

QSTATUS qm::EditCommandAction::isEnabled(const ActionEvent& event, bool* pbEnabled)
{
	assert(pbEnabled);
	
	DECLARE_QSTATUS();
	
	MessageWindowItem* pItem = pMessageWindow_->getFocusedItem();
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
 * EditCopyMessageAction
 *
 */

qm::EditCopyMessageAction::EditCopyMessageAction(
	Document* pDocument, FolderModel* pFolderModel,
	MessageSelectionModel* pMessageSelectionModel, QSTATUS* pstatus) :
	pDocument_(pDocument),
	pFolderModel_(pFolderModel),
	pMessageSelectionModel_(pMessageSelectionModel)
{
}

qm::EditCopyMessageAction::~EditCopyMessageAction()
{
}

QSTATUS qm::EditCopyMessageAction::invoke(const ActionEvent& event)
{
	DECLARE_QSTATUS();
	
	AccountLock lock;
	MessageHolderList l;
	status = pMessageSelectionModel_->getSelectedMessages(&lock, 0, &l);
	CHECK_QSTATUS();
	
	Account* pAccount = lock.get();
	if (!l.empty()) {
		MessageDataObject* p = 0;
		status = newQsObject(pDocument_, pAccount,
			l, MessageDataObject::FLAG_COPY, &p);
		CHECK_QSTATUS();
		p->AddRef();
		ComPtr<IDataObject> pDataObject(p);
		status = MessageDataObject::setClipboard(pDataObject.get());
		CHECK_QSTATUS();
	}
	
	return QSTATUS_SUCCESS;
}

QSTATUS qm::EditCopyMessageAction::isEnabled(
	const ActionEvent& event, bool* pbEnabled)
{
	assert(pbEnabled);
	return pMessageSelectionModel_->hasSelectedMessage(pbEnabled);
}


/****************************************************************************
 *
 * EditCutMessageAction
 *
 */

qm::EditCutMessageAction::EditCutMessageAction(
	Document* pDocument, FolderModel* pFolderModel,
	MessageSelectionModel* pMessageSelectionModel, QSTATUS* pstatus) :
	pDocument_(pDocument),
	pFolderModel_(pFolderModel),
	pMessageSelectionModel_(pMessageSelectionModel)
{
}

qm::EditCutMessageAction::~EditCutMessageAction()
{
}

QSTATUS qm::EditCutMessageAction::invoke(const ActionEvent& event)
{
	DECLARE_QSTATUS();
	
	AccountLock lock;
	MessageHolderList l;
	status = pMessageSelectionModel_->getSelectedMessages(&lock, 0, &l);
	CHECK_QSTATUS();
	
	Account* pAccount = lock.get();
	if (!l.empty()) {
		MessageDataObject* p = 0;
		status = newQsObject(pDocument_, pAccount, l, MessageDataObject::FLAG_MOVE, &p);
		CHECK_QSTATUS();
		p->AddRef();
		ComPtr<IDataObject> pDataObject(p);
		status = MessageDataObject::setClipboard(pDataObject.get());
		CHECK_QSTATUS();
	}
	
	return QSTATUS_SUCCESS;
}

QSTATUS qm::EditCutMessageAction::isEnabled(
	const ActionEvent& event, bool* pbEnabled)
{
	assert(pbEnabled);
	return pMessageSelectionModel_->hasSelectedMessage(pbEnabled);
}


/****************************************************************************
 *
 * EditDeleteCacheAction
 *
 */

qm::EditDeleteCacheAction::EditDeleteCacheAction(
	MessageSelectionModel* pModel, QSTATUS* pstatus) :
	pModel_(pModel)
{
	assert(pstatus);
	*pstatus = QSTATUS_SUCCESS;
}

qm::EditDeleteCacheAction::~EditDeleteCacheAction()
{
}

QSTATUS qm::EditDeleteCacheAction::invoke(const ActionEvent& event)
{
	DECLARE_QSTATUS();
	
	AccountLock lock;
	MessageHolderList l;
	status = pModel_->getSelectedMessages(&lock, 0, &l);
	CHECK_QSTATUS();
	
	if (!l.empty()) {
		Account* pAccount = lock.get();
		status = pAccount->deleteMessagesCache(l);
		CHECK_QSTATUS();
	}
	
	return QSTATUS_SUCCESS;
}

QSTATUS qm::EditDeleteCacheAction::isEnabled(
	const ActionEvent& event, bool* pbEnabled)
{
	assert(pbEnabled);
	return pModel_->hasSelectedMessage(pbEnabled);
}


/****************************************************************************
 *
 * EditDeleteMessageAction
 *
 */

qm::EditDeleteMessageAction::EditDeleteMessageAction(
	MessageSelectionModel* pModel, bool bDirect,
	HWND hwndFrame, QSTATUS* pstatus) :
	pMessageSelectionModel_(pModel),
	pMessageModel_(0),
	bDirect_(bDirect),
	hwndFrame_(hwndFrame)
{
	assert(pstatus);
	*pstatus = QSTATUS_SUCCESS;
}

qm::EditDeleteMessageAction::EditDeleteMessageAction(
	MessageModel* pModel, ViewModelHolder* pViewModelHolder,
	bool bDirect, QSTATUS* pstatus) :
	pMessageSelectionModel_(0),
	pMessageModel_(pModel),
	pViewModelHolder_(pViewModelHolder),
	bDirect_(bDirect),
	hwndFrame_(0)
{
	assert(pstatus);
	*pstatus = QSTATUS_SUCCESS;
}

qm::EditDeleteMessageAction::~EditDeleteMessageAction()
{
}

QSTATUS qm::EditDeleteMessageAction::invoke(const ActionEvent& event)
{
	DECLARE_QSTATUS();
	
	if (pMessageSelectionModel_) {
		AccountLock lock;
		MessageHolderList l;
		status = pMessageSelectionModel_->getSelectedMessages(&lock, 0, &l);
		CHECK_QSTATUS();
		
		Account* pAccount = lock.get();
		if (!l.empty()) {
			ProgressDialogMessageOperationCallback callback(
				hwndFrame_, IDS_DELETE, IDS_DELETE);
			status = pAccount->removeMessages(l, bDirect_, &callback);
			CHECK_QSTATUS();
		}
	}
	else {
		ViewModel* pViewModel = pViewModelHolder_->getViewModel();
		
		Lock<ViewModel> lock(*pViewModel);
		
		MessagePtrLock mpl(pMessageModel_->getCurrentMessage());
		if (mpl) {
			unsigned int nIndex = pViewModel->getIndex(mpl);
			if (nIndex < pViewModel->getCount() - 1) {
				MessageHolder* pmh = pViewModel->getMessageHolder(nIndex + 1);
				status = pMessageModel_->setMessage(pmh);
				CHECK_QSTATUS();
			}
			
			Account* pAccount = mpl->getFolder()->getAccount();
			MessageHolderList l;
			status = STLWrapper<MessageHolderList>(l).push_back(mpl);
			CHECK_QSTATUS();
			status = pAccount->removeMessages(l, bDirect_, 0);
		}
	}
	
	return QSTATUS_SUCCESS;
}

QSTATUS qm::EditDeleteMessageAction::isEnabled(
	const ActionEvent& event, bool* pbEnabled)
{
	assert(pbEnabled);
	
	DECLARE_QSTATUS();
	
	if (pMessageSelectionModel_) {
		status = pMessageSelectionModel_->hasSelectedMessage(pbEnabled);
		CHECK_QSTATUS();
	}
	else {
		MessagePtrLock mpl(pMessageModel_->getCurrentMessage());
		*pbEnabled = mpl != 0;
	}
	
	return QSTATUS_SUCCESS;
}


/****************************************************************************
 *
 * EditFindAction
 *
 */

qm::EditFindAction::EditFindAction(MessageWindow* pMessageWindow,
	Profile* pProfile, FindReplaceManager* pFindReplaceManager, QSTATUS* pstatus) :
	pMessageWindow_(pMessageWindow),
	pProfile_(pProfile),
	pFindReplaceManager_(pFindReplaceManager),
	type_(TYPE_NORMAL)
{
}

qm::EditFindAction::EditFindAction(MessageWindow* pMessageWindow, bool bNext,
	FindReplaceManager* pFindReplaceManager, QSTATUS* pstatus) :
	pMessageWindow_(pMessageWindow),
	pProfile_(0),
	pFindReplaceManager_(pFindReplaceManager),
	type_(bNext ? TYPE_NEXT : TYPE_PREV)
{
}

qm::EditFindAction::~EditFindAction()
{
}

QSTATUS qm::EditFindAction::invoke(const ActionEvent& event)
{
	DECLARE_QSTATUS();
	
	HWND hwndFrame = pMessageWindow_->getParentFrame();
	
	bool bFound = false;
	if (type_ == TYPE_NORMAL) {
		bool bSupportRegex = (pMessageWindow_->getSupportedFindFlags() & MessageWindow::FIND_REGEX) != 0;
		FindDialog dialog(pProfile_, bSupportRegex, &status);
		CHECK_QSTATUS();
		int nRet = 0;
		status = dialog.doModal(hwndFrame, 0, &nRet);
		CHECK_QSTATUS();
		if (nRet != IDOK)
			return QSTATUS_SUCCESS;
		
		status = pFindReplaceManager_->setData(dialog.getFind(),
			(dialog.isMatchCase() ? FindReplaceData::FLAG_MATCHCASE : 0) |
			(dialog.isRegex() ? FindReplaceData::FLAG_REGEX : 0));
		CHECK_QSTATUS();
		
		unsigned int nFlags =
			(dialog.isMatchCase() ? MessageWindow::FIND_MATCHCASE : 0) |
			(dialog.isRegex() ? MessageWindow::FIND_REGEX : 0) |
			(dialog.isPrev() ? MessageWindow::FIND_PREVIOUS : 0);
		status = pMessageWindow_->find(dialog.getFind(), nFlags, &bFound);
		CHECK_QSTATUS();
	}
	else {
		const FindReplaceData* pData = pFindReplaceManager_->getData();
		assert(pData);
		unsigned int nFlags =
			(pData->getFlags() & FindReplaceData::FLAG_MATCHCASE ? MessageWindow::FIND_MATCHCASE : 0) |
			(pData->getFlags() & FindReplaceData::FLAG_REGEX ? MessageWindow::FIND_REGEX : 0) |
			(type_ == TYPE_PREV ? MessageWindow::FIND_PREVIOUS : 0);
		status = pMessageWindow_->find(pData->getFind(), nFlags, &bFound);
		CHECK_QSTATUS();
	}
	
	if (!bFound)
		messageBox(Application::getApplication().getResourceHandle(),
			IDS_FINDNOTFOUND, hwndFrame);
	
	return QSTATUS_SUCCESS;
}

QSTATUS qm::EditFindAction::isEnabled(const ActionEvent& event, bool* pbEnabled)
{
	assert(pbEnabled);
	if (type_ == TYPE_NORMAL)
		*pbEnabled = pMessageWindow_->isActive();
	else
		*pbEnabled = pMessageWindow_->isActive() &&
			pFindReplaceManager_->getData();
	return QSTATUS_SUCCESS;
}


/****************************************************************************
 *
 * EditPasteMessageAction
 *
 */

qm::EditPasteMessageAction::EditPasteMessageAction(Document* pDocument,
	FolderModel* pModel, HWND hwndFrame, QSTATUS* pstatus) :
	pDocument_(pDocument),
	pModel_(pModel),
	hwndFrame_(hwndFrame)
{
}

qm::EditPasteMessageAction::~EditPasteMessageAction()
{
}

QSTATUS qm::EditPasteMessageAction::invoke(const ActionEvent& event)
{
	DECLARE_QSTATUS();
	
	Folder* pFolder = pModel_->getCurrentFolder();
	if (pFolder && pFolder->getType() == Folder::TYPE_NORMAL &&
		!pFolder->isFlag(Folder::FLAG_NOSELECT)) {
		ComPtr<IDataObject> pDataObject;
		status = MessageDataObject::getClipboard(pDocument_, &pDataObject);
		CHECK_QSTATUS();
		
		NormalFolder* pNormalFolder = static_cast<NormalFolder*>(pFolder);
		MessageDataObject::Flag flag = MessageDataObject::getPasteFlag(
			pDataObject.get(), pDocument_, pNormalFolder);
		UINT nId = flag == MessageDataObject::FLAG_MOVE ?
			IDS_MOVEMESSAGE : IDS_COPYMESSAGE;
		ProgressDialogMessageOperationCallback callback(hwndFrame_, nId, nId);
		status = MessageDataObject::pasteMessages(pDataObject.get(),
			pDocument_, pNormalFolder, flag, &callback);
		CHECK_QSTATUS();
#ifdef _WIN32_WCE
		Clipboard clipboard(0, &status);
		CHECK_QSTATUS();
		status = clipboard.empty();
		CHECK_QSTATUS();
#else
		::OleSetClipboard(0);
#endif
	}
	
	return QSTATUS_SUCCESS;
}

QSTATUS qm::EditPasteMessageAction::isEnabled(
	const ActionEvent& event, bool* pbEnabled)
{
	assert(pbEnabled);
	
	DECLARE_QSTATUS();
	
	*pbEnabled = false;
	
	Folder* pFolder = pModel_->getCurrentFolder();
	if (pFolder && pFolder->getType() == Folder::TYPE_NORMAL &&
		!pFolder->isFlag(Folder::FLAG_NOSELECT)) {
		status = MessageDataObject::queryClipboard(pbEnabled);
		CHECK_QSTATUS();
	}
	
	return QSTATUS_SUCCESS;
}


/****************************************************************************
 *
 * EditSelectAllMessageAction
 *
 */

qm::EditSelectAllMessageAction::EditSelectAllMessageAction(
	MessageSelectionModel* pMessageSelectionModel, QSTATUS* pstatus) :
	pMessageSelectionModel_(pMessageSelectionModel)
{
}

qm::EditSelectAllMessageAction::~EditSelectAllMessageAction()
{
}

QSTATUS qm::EditSelectAllMessageAction::invoke(const ActionEvent& event)
{
	return pMessageSelectionModel_->selectAll();
}

QSTATUS qm::EditSelectAllMessageAction::isEnabled(
	const ActionEvent& event, bool* pbEnabled)
{
	assert(pbEnabled);
	return pMessageSelectionModel_->canSelect(pbEnabled);
}


/****************************************************************************
 *
 * FileCloseAction
 *
 */

qm::FileCloseAction::FileCloseAction(HWND hwnd, QSTATUS* pstatus) :
	hwnd_(hwnd)
{
	assert(pstatus);
	*pstatus = QSTATUS_SUCCESS;
}

qm::FileCloseAction::~FileCloseAction()
{
}

QSTATUS qm::FileCloseAction::invoke(const ActionEvent& event)
{
	Window(hwnd_).postMessage(WM_CLOSE);
	return QSTATUS_SUCCESS;
}


/****************************************************************************
 *
 * FileCompactAction
 *
 */

qm::FileCompactAction::FileCompactAction(
	FolderModel* pFolderModel, QSTATUS* pstatus) :
	pFolderModel_(pFolderModel)
{
}

qm::FileCompactAction::~FileCompactAction()
{
}

QSTATUS qm::FileCompactAction::invoke(const ActionEvent& event)
{
	DECLARE_QSTATUS();
	
	Account* pAccount = pFolderModel_->getCurrentAccount();
	if (!pAccount) {
		Folder* pFolder = pFolderModel_->getCurrentFolder();
		if (pFolder)
			pAccount = pFolder->getAccount();
	}
	if (pAccount) {
		status = pAccount->compact();
		CHECK_QSTATUS();
	}
	
	return QSTATUS_SUCCESS;
}

QSTATUS qm::FileCompactAction::isEnabled(const ActionEvent& event, bool* pbEnabled)
{
	assert(pbEnabled);
	*pbEnabled = pFolderModel_->getCurrentAccount() ||
		pFolderModel_->getCurrentFolder();
	return QSTATUS_SUCCESS;
}


/****************************************************************************
 *
 * FileExitAction
 *
 */

qm::FileExitAction::FileExitAction(HWND hwnd, Document* pDocument,
	SyncManager* pSyncManager, TempFileCleaner* pTempFileCleaner,
	EditFrameWindowManager* pEditFrameWindowManager, QSTATUS* pstatus) :
	hwnd_(hwnd),
	pDocument_(pDocument),
	pSyncManager_(pSyncManager),
	pTempFileCleaner_(pTempFileCleaner),
	pEditFrameWindowManager_(pEditFrameWindowManager)
{
	assert(pstatus);
	*pstatus = QSTATUS_SUCCESS;
}

qm::FileExitAction::~FileExitAction()
{
}

QSTATUS qm::FileExitAction::exit(bool bDestroy, bool* pbCanceled)
{
	DECLARE_QSTATUS();
	
	if (pbCanceled)
		*pbCanceled = true;
	
	if (pSyncManager_->isSyncing()) {
		// TODO
		// Show message
		return QSTATUS_SUCCESS;
	}
	
	bool bClosed = false;
	status = pEditFrameWindowManager_->closeAll(&bClosed);
	CHECK_QSTATUS();
	if (!bClosed)
		return QSTATUS_SUCCESS;
	
	{
		WaitCursor cursor;
		status = Application::getApplication().save();
		CHECK_QSTATUS();
		pDocument_->setOffline(true);
		pSyncManager_->dispose();
	}
	
	struct CallbackImpl : public TempFileCleanerCallback
	{
		virtual bool confirmDelete(const WCHAR* pwszPath)
		{
			DECLARE_QSTATUS();
			
			HINSTANCE hInst = Application::getApplication().getResourceHandle();
			
			string_ptr<WSTRING> wstr;
			status = loadString(hInst, IDS_CONFIRMDELETETEMPFILE, &wstr);
			CHECK_QSTATUS_VALUE(false);
			string_ptr<WSTRING> wstrMessage(concat(wstr.get(), pwszPath));
			if (!wstrMessage.get())
				return false;
			
			int nMsg = 0;
			status = messageBox(wstrMessage.get(),
				MB_YESNO | MB_DEFBUTTON2 | MB_ICONQUESTION, &nMsg);
			CHECK_QSTATUS_VALUE(false);
			return nMsg == IDYES;
		}
	} callback;
	pTempFileCleaner_->clean(&callback);
	
	if (bDestroy)
		Window(hwnd_).destroyWindow();
	
	if (pbCanceled)
		*pbCanceled = false;
	
	return QSTATUS_SUCCESS;
}

QSTATUS qm::FileExitAction::invoke(const ActionEvent& event)
{
	return exit(true, 0);
}


/****************************************************************************
 *
 * FileExportAction
 *
 */

qm::FileExportAction::FileExportAction(MessageSelectionModel* pModel,
	HWND hwndFrame, QSTATUS* pstatus) :
	pModel_(pModel),
	hwndFrame_(hwndFrame)
{
}

qm::FileExportAction::~FileExportAction()
{
}

QSTATUS qm::FileExportAction::invoke(const ActionEvent& event)
{
	DECLARE_QSTATUS();
	
	AccountLock lock;
	MessageHolderList l;
	status = pModel_->getSelectedMessages(&lock, 0, &l);
	CHECK_QSTATUS();
	
	Account* pAccount = lock.get();
	if (!l.empty()) {
		ExportDialog dialog(l.size() == 1, &status);
		CHECK_QSTATUS();
		int nRet = 0;
		status = dialog.doModal(hwndFrame_, 0, &nRet);
		CHECK_QSTATUS();
		if (nRet == IDOK) {
			const Template* pTemplate = 0;
			const WCHAR* pwszEncoding = 0;
			const WCHAR* pwszTemplate = dialog.getTemplate();
			if (pwszTemplate) {
				// TODO
				// Get template and encoding
			}
			
			ProgressDialog progressDialog(IDS_EXPORT, &status);
			CHECK_QSTATUS();
			ProgressDialogInit init(&progressDialog, hwndFrame_,
				IDS_EXPORT, IDS_EXPORT, 0, l.size(), 0, &status);
			CHECK_QSTATUS();
			
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
					status = progressDialog.setPos(n);
					CHECK_QSTATUS();
					
					WCHAR wszNumber[32];
					swprintf(wszNumber, L"%d", n);
					ConcatW c[] = {
						{ pwszPath,		pExt - pwszPath	},
						{ wszNumber,	-1				},
						{ pExt,			-1				}
					};
					string_ptr<WSTRING> wstrPath(concat(c, countof(c)));
					if (!wstrPath.get())
						return QSTATUS_OUTOFMEMORY;
					
					FileOutputStream fileStream(wstrPath.get(), &status);
					CHECK_QSTATUS();
					BufferedOutputStream stream(&fileStream, false, &status);
					CHECK_QSTATUS();
					status = writeMessage(&stream, l[n],
						dialog.isExportFlags(), pTemplate, pwszEncoding, false);
					CHECK_QSTATUS();
					status = stream.close();
					CHECK_QSTATUS();
					
					++n;
				}
				status = progressDialog.setPos(n);
				CHECK_QSTATUS();
			}
			else {
				FileOutputStream fileStream(dialog.getPath(), &status);
				CHECK_QSTATUS();
				BufferedOutputStream stream(&fileStream, false, &status);
				CHECK_QSTATUS();
				
				int nPos = 0;
				if (l.size() == 1) {
					status = writeMessage(&stream, l.front(),
						dialog.isExportFlags(), pTemplate, pwszEncoding, false);
					CHECK_QSTATUS();
					++nPos;
				}
				else {
					MessageHolderList::iterator it = l.begin();
					while (it != l.end()) {
						if (progressDialog.isCanceled())
							break;
						status = progressDialog.setPos(nPos++);
						CHECK_QSTATUS();
						
						status = writeMessage(&stream, *it,
							dialog.isExportFlags(), pTemplate, pwszEncoding, true);
						CHECK_QSTATUS();
						++it;
					}
				}
				status = progressDialog.setPos(nPos);
				CHECK_QSTATUS();
				
				status = stream.close();
				CHECK_QSTATUS();
			}
		}
	}
	
	return QSTATUS_SUCCESS;
}

QSTATUS qm::FileExportAction::isEnabled(
	const ActionEvent& event, bool* pbEnabled)
{
	assert(pbEnabled);
	return pModel_->hasSelectedMessage(pbEnabled);
}

QSTATUS qm::FileExportAction::writeMessage(OutputStream* pStream,
	const MessagePtr& ptr, bool bAddFlags, const Template* pTemplate,
	const WCHAR* pwszEncoding, bool bWriteSeparator)
{
	assert(pStream);
	assert((pTemplate && pwszEncoding) || (!pTemplate && !pwszEncoding));
	
	DECLARE_QSTATUS();
	
	MessagePtrLock mpl(ptr);
	if (mpl) {
		Message msg(&status);
		CHECK_QSTATUS();
		status = mpl->getMessage(
			Account::GETMESSAGEFLAG_ALL, 0, &msg);
		CHECK_QSTATUS();
		
		if (bAddFlags) {
			NumberParser flags(
				mpl->getFlags() & MessageHolder::FLAG_USER_MASK,
				NumberParser::FLAG_HEX, &status);
			status = msg.replaceField(L"X-QMAIL-Flags", flags);
			CHECK_QSTATUS();
		}
		
		string_ptr<STRING> strContent;
		if (pTemplate) {
			// TODO
			// Process template
		}
		else {
			status = msg.getContent(&strContent);
			CHECK_QSTATUS();
		}
		
		if (bWriteSeparator) {
			status = pStream->write(
				reinterpret_cast<const unsigned char*>("From \r\n"), 7);
			CHECK_QSTATUS();
			
			const CHAR* p = strContent.get();
			while (p) {
				const CHAR* pCheck = p;
				while (*pCheck == '>')
					++pCheck;
				if (strncmp(pCheck, "From ", 5) == 0) {
					status = pStream->write(
						reinterpret_cast<unsigned char*>(">"), 1);
					CHECK_QSTATUS();
				}
				
				const CHAR* pEnd = strstr(p, "\r\n");
				size_t nLen = pEnd ? pEnd - p + 2 : strlen(p);
				
				status = pStream->write(
					reinterpret_cast<const unsigned char*>(p), nLen);
				CHECK_QSTATUS();
				
				p = pEnd ? pEnd + 2 : 0;
			}
		}
		else {
			status = pStream->write(
				reinterpret_cast<unsigned char*>(strContent.get()),
				strlen(strContent.get()));
			CHECK_QSTATUS();
		}
	}
	
	return QSTATUS_SUCCESS;
}


/****************************************************************************
 *
 * FileImportAction
 *
 */

qm::FileImportAction::FileImportAction(FolderModel* pFolderModel,
	HWND hwndFrame, QSTATUS* pstatus) :
	pFolderModel_(pFolderModel),
	hwndFrame_(hwndFrame)
{
	assert(pstatus);
	*pstatus = QSTATUS_SUCCESS;
}

qm::FileImportAction::~FileImportAction()
{
}

QSTATUS qm::FileImportAction::invoke(const ActionEvent& event)
{
	DECLARE_QSTATUS();
	
	Folder* pFolder = pFolderModel_->getCurrentFolder();
	if (pFolder && pFolder->getType() == Folder::TYPE_NORMAL) {
		ImportDialog dialog(&status);
		CHECK_QSTATUS();
		int nRet = 0;
		status = dialog.doModal(hwndFrame_, 0, &nRet);
		CHECK_QSTATUS();
		if (nRet == IDOK) {
			ProgressDialog progressDialog(IDS_IMPORT, &status);
			CHECK_QSTATUS();
			ProgressDialogInit init(&progressDialog, hwndFrame_,
				IDS_IMPORT, IDS_IMPORT, 0, 100, 0, &status);
			CHECK_QSTATUS();
			int nPos = 0;
			
			const WCHAR* pwszPath = dialog.getPath();
			const WCHAR* pBegin = pwszPath;
			while (true) {
				const WCHAR* pEnd = wcschr(pBegin, L';');
				string_ptr<WSTRING> wstrPath(allocWString(
					pBegin, pEnd ? pEnd - pBegin : -1));
				if (!wstrPath.get())
					return QSTATUS_OUTOFMEMORY;
				
				FileInputStream fileStream(wstrPath.get(), &status);
				CHECK_QSTATUS();
				BufferedInputStream stream(&fileStream, false, &status);
				CHECK_QSTATUS();
				
				bool bCanceled = false;
				status = readMessage(static_cast<NormalFolder*>(pFolder),
					&stream, dialog.isMultiple(), dialog.getFlags(),
					&progressDialog, &nPos, &bCanceled);
				CHECK_QSTATUS();
				if (bCanceled)
					break;
				
				if (!pEnd)
					break;
				pBegin = pEnd + 1;
				if (!*pBegin)
					break;
			}
		}
		
		status = pFolder->getAccount()->save();
		CHECK_QSTATUS();
	}
	
	return QSTATUS_SUCCESS;
}

QSTATUS qm::FileImportAction::isEnabled(const ActionEvent& event, bool* pbEnabled)
{
	assert(pbEnabled);
	Folder* pFolder = pFolderModel_->getCurrentFolder();
	*pbEnabled = pFolder && pFolder->getType() == Folder::TYPE_NORMAL;
	return QSTATUS_SUCCESS;
}

QSTATUS qm::FileImportAction::readMessage(NormalFolder* pFolder,
	InputStream* pStream, bool bMultiple, unsigned int nFlags,
	ProgressDialog* pDialog, int* pnPos, bool* pbCanceled)
{
	assert(pFolder);
	assert(pStream);
	assert((pDialog && pnPos && pbCanceled) ||
		(!pDialog && !pnPos && !pbCanceled));
	
	DECLARE_QSTATUS();
	
	if (bMultiple) {
		StringBuffer<STRING> buf(&status);
		CHECK_QSTATUS();
		
		CHAR cPrev = '\0';
		bool bNewLine = true;
		while (bNewLine) {
			string_ptr<STRING> strLine;
			CHAR cNext = '\0';
			status = readLine(pStream, cPrev, &strLine, &cNext, &bNewLine);
			CHECK_QSTATUS();
			cPrev = cNext;
			
			if (!bNewLine || strncmp(strLine.get(), "From ", 5) == 0) {
				if (buf.getLength() != 0) {
					if (pDialog) {
						if (pDialog->isCanceled()) {
							*pbCanceled = true;
							return QSTATUS_SUCCESS;
						}
						status = pDialog->setPos((*pnPos)++ % 100);
						CHECK_QSTATUS();
					}
					
					status = pFolder->getAccount()->importMessage(pFolder,
						buf.getCharArray(), nFlags);
					CHECK_QSTATUS();
					buf.remove();
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
				
				status = buf.append(p);
				CHECK_QSTATUS();
				status = buf.append("\r\n");
				CHECK_QSTATUS();
			}
		}
	}
	else {
		StringBuffer<STRING> buf(&status);
		CHECK_QSTATUS();
		
		unsigned char c = 0;
		size_t nRead = 0;
		bool bCR = false;
		while (true) {
			status = pStream->read(&c, 1, &nRead);
			CHECK_QSTATUS();
			if (nRead == static_cast<size_t>(-1))
				break;
			
			if (bCR) {
				status = buf.append("\r\n");
				CHECK_QSTATUS();
				switch (c) {
				case '\r':
					break;
				case '\n':
					bCR = false;
					break;
				default:
					status = buf.append(static_cast<CHAR>(c));
					CHECK_QSTATUS();
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
					status = buf.append("\r\n");
					CHECK_QSTATUS();
					break;
				default:
					status = buf.append(static_cast<CHAR>(c));
					CHECK_QSTATUS();
					break;
				}
			}
		}
		if (bCR) {
			status = buf.append("\r\n");
			CHECK_QSTATUS();
		}
		
		if (pDialog) {
			if (pDialog->isCanceled()) {
				*pbCanceled = true;
				return QSTATUS_SUCCESS;
			}
			status = pDialog->setPos((*pnPos)++ % 100);
			CHECK_QSTATUS();
		}
		
		status = pFolder->getAccount()->importMessage(
			pFolder, buf.getCharArray(), nFlags);
		CHECK_QSTATUS();
	}
	
	return QSTATUS_SUCCESS;
}

QSTATUS qm::FileImportAction::readLine(InputStream* pStream,
	CHAR cPrev, STRING* pstrLine, CHAR* pcNext, bool* pbNewLine)
{
	assert(pStream);
	assert(pstrLine);
	assert(pcNext);
	assert(pbNewLine);
	
	DECLARE_QSTATUS();
	
	*pstrLine = 0;
	*pcNext = '\0';
	*pbNewLine = false;
	
	StringBuffer<STRING> buf(&status);
	CHECK_QSTATUS();
	
	unsigned char c = 0;
	size_t nRead = 0;
	bool bNewLine = false;
	while (!bNewLine) {
		if (cPrev != '\0') {
			c = cPrev;
			cPrev = '\0';
		}
		else {
			status = pStream->read(&c, 1, &nRead);
			CHECK_QSTATUS();
			if (nRead == static_cast<size_t>(-1))
				break;
		}
		
		if (c == '\r') {
			bNewLine = true;
			status = pStream->read(&c, 1, &nRead);
			CHECK_QSTATUS();
			if (nRead != static_cast<size_t>(-1) && c != '\n')
				*pcNext = c;
		}
		else if (c == '\n') {
			bNewLine = true;
		}
		else {
			status = buf.append(static_cast<CHAR>(c));
			CHECK_QSTATUS();
		}
	}
	
	*pstrLine = buf.getString();
	*pbNewLine = bNewLine;
	
	return QSTATUS_SUCCESS;
}


/****************************************************************************
 *
 * FileOfflineAction
 *
 */

qm::FileOfflineAction::FileOfflineAction(Document* pDocument,
	SyncManager* pSyncManager, QSTATUS* pstatus) :
	pDocument_(pDocument),
	pSyncManager_(pSyncManager)
{
}

qm::FileOfflineAction::~FileOfflineAction()
{
}

QSTATUS qm::FileOfflineAction::invoke(const ActionEvent& event)
{
	return pDocument_->setOffline(!pDocument_->isOffline());
}

QSTATUS qm::FileOfflineAction::isEnabled(const ActionEvent& event, bool* pbEnabled)
{
	assert(pbEnabled);
	*pbEnabled = !pSyncManager_->isSyncing();
	return QSTATUS_SUCCESS;
}

QSTATUS qm::FileOfflineAction::isChecked(const ActionEvent& event, bool* pbChecked)
{
	assert(pbChecked);
	*pbChecked = pDocument_->isOffline();
	return QSTATUS_SUCCESS;
}


/****************************************************************************
 *
 * FilePrintAction
 *
 */

qm::FilePrintAction::FilePrintAction(Document* pDocument,
	MessageSelectionModel* pModel, HWND hwnd, Profile* pProfile,
	TempFileCleaner* pTempFileCleaner, QSTATUS* pstatus) :
	pDocument_(pDocument),
	pModel_(pModel),
	hwnd_(hwnd),
	pProfile_(pProfile),
	pTempFileCleaner_(pTempFileCleaner)
{
}

qm::FilePrintAction::~FilePrintAction()
{
}

QSTATUS qm::FilePrintAction::invoke(const ActionEvent& event)
{
	DECLARE_QSTATUS();
	
	AccountLock lock;
	Folder* pFolder = 0;
	MessageHolderList l;
	status = pModel_->getSelectedMessages(&lock, &pFolder, &l);
	CHECK_QSTATUS();
	
	Account* pAccount = lock.get();
	if (!l.empty()) {
		MessageHolderList::const_iterator it = l.begin();
		while (it != l.end()) {
			MessageHolder* pmh = *it;
			const Template* pTemplate = 0;
			status = pDocument_->getTemplateManager()->getTemplate(
				pAccount, pFolder, L"print", &pTemplate);
			CHECK_QSTATUS();
			
			Message msg(&status);
			CHECK_QSTATUS();
			TemplateContext context(pmh, &msg, pAccount, pDocument_,
				hwnd_, pProfile_, 0, TemplateContext::ArgumentList(), &status);
			CHECK_QSTATUS();
			
			string_ptr<WSTRING> wstrValue;
			status = pTemplate->getValue(context, &wstrValue);
			CHECK_QSTATUS();
			
			string_ptr<WSTRING> wstrExtension;
			status = pProfile_->getString(L"Global",
				L"PrintExtension", L"html", &wstrExtension);
			CHECK_QSTATUS();
			
			string_ptr<WSTRING> wstrPath;
			status = UIUtil::writeTemporaryFile(wstrValue.get(), L"q3print",
				wstrExtension.get(), pTempFileCleaner_, &wstrPath);
			CHECK_QSTATUS();
			
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
			if (!::ShellExecuteEx(&sei)) {
				// TODO
			}
			
			++it;
		}
	}
	
	return QSTATUS_SUCCESS;
}

QSTATUS qm::FilePrintAction::isEnabled(const ActionEvent& event, bool* pbEnabled)
{
	assert(pbEnabled);
	return pModel_->hasSelectedMessage(pbEnabled);
}


/****************************************************************************
 *
 * FileSalvageAction
 *
 */

qm::FileSalvageAction::FileSalvageAction(
	FolderModel* pFolderModel, QSTATUS* pstatus) :
	pFolderModel_(pFolderModel)
{
}

qm::FileSalvageAction::~FileSalvageAction()
{
}

QSTATUS qm::FileSalvageAction::invoke(const qs::ActionEvent& event)
{
	DECLARE_QSTATUS();
	
	Folder* pFolder = pFolderModel_->getCurrentFolder();
	if (!pFolder || pFolder->getType() != Folder::TYPE_NORMAL)
		return QSTATUS_SUCCESS;
	
	Account* pAccount = pFolder->getAccount();
	status = pAccount->salvage(static_cast<NormalFolder*>(pFolder));
	CHECK_QSTATUS();
	
	return QSTATUS_SUCCESS;
}

QSTATUS qm::FileSalvageAction::isEnabled(const qs::ActionEvent& event, bool* pbEnabled)
{
	assert(pbEnabled);
	Folder* pFolder = pFolderModel_->getCurrentFolder();
	*pbEnabled = pFolder && pFolder->getType() == Folder::TYPE_NORMAL;
	return QSTATUS_SUCCESS;
}


/****************************************************************************
 *
 * FileSaveAction
 *
 */

qm::FileSaveAction::FileSaveAction(Document* pDocument,
	ViewModelManager* pViewModelManager, QSTATUS* pstatus) :
	pDocument_(pDocument),
	pViewModelManager_(pViewModelManager)
{
}

qm::FileSaveAction::~FileSaveAction()
{
}

QSTATUS qm::FileSaveAction::invoke(const ActionEvent& event)
{
	DECLARE_QSTATUS();
	
	WaitCursor cursor;
	
	status = Application::getApplication().save();
	CHECK_QSTATUS();
	
	return QSTATUS_SUCCESS;
}


/****************************************************************************
 *
 * FolderCreateAction
 *
 */

qm::FolderCreateAction::FolderCreateAction(
	FolderSelectionModel* pFolderSelectionModel,
	HWND hwndFrame, Profile* pProfile, QSTATUS* pstatus) :
	pFolderSelectionModel_(pFolderSelectionModel),
	hwndFrame_(hwndFrame),
	pProfile_(pProfile)
{
	assert(pstatus);
	*pstatus = QSTATUS_SUCCESS;
}

qm::FolderCreateAction::~FolderCreateAction()
{
}

QSTATUS qm::FolderCreateAction::invoke(const ActionEvent& event)
{
	DECLARE_QSTATUS();
	
	Folder* pFolder = pFolderSelectionModel_->getFocusedFolder();
	Account* pAccount = pFolder ? pFolder->getAccount() :
		pFolderSelectionModel_->getAccount();
	
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
	
	CreateFolderDialog dialog(type, bAllowRemote, &status);
	CHECK_QSTATUS();
	int nRet = 0;
	status = dialog.doModal(hwndFrame_, 0, &nRet);
	CHECK_QSTATUS();
	if (nRet == IDOK) {
		NormalFolder* pNormalFolder = 0;
		QueryFolder* pQueryFolder = 0;
		switch (dialog.getType()) {
		case CreateFolderDialog::TYPE_LOCALFOLDER:
			status = pAccount->createNormalFolder(dialog.getName(),
				pFolder, false, &pNormalFolder);
			break;
		case CreateFolderDialog::TYPE_REMOTEFOLDER:
			status = pAccount->createNormalFolder(dialog.getName(),
				pFolder, true, &pNormalFolder);
			break;
		case CreateFolderDialog::TYPE_QUERYFOLDER:
			status = pAccount->createQueryFolder(dialog.getName(),
				pFolder, L"macro", L"@False()", 0, false, &pQueryFolder);
			break;
		default:
			assert(false);
			break;
		}
		if (status == QSTATUS_SUCCESS) {
			if (pQueryFolder) {
				Account::FolderList l;
				status = STLWrapper<Account::FolderList>(l).push_back(pQueryFolder);
				CHECK_QSTATUS();
				status = FolderPropertyAction::openProperty(
					l, true, hwndFrame_, pProfile_);
				CHECK_QSTATUS();
			}
		}
		else {
			messageBox(Application::getApplication().getResourceHandle(),
				IDS_ERROR_CREATEFOLDER, MB_OK | MB_ICONERROR);
		}
	}
	
	return QSTATUS_SUCCESS;
}

QSTATUS qm::FolderCreateAction::isEnabled(const ActionEvent& event, bool* pbEnabled)
{
	assert(pbEnabled);
	*pbEnabled = pFolderSelectionModel_->getAccount() ||
		pFolderSelectionModel_->getFocusedFolder();
	return QSTATUS_SUCCESS;
}


/****************************************************************************
 *
 * FolderDeleteAction
 *
 */

qm::FolderDeleteAction::FolderDeleteAction(FolderModel* pFolderModel,
	FolderSelectionModel* pFolderSelectionModel, QSTATUS* pstatus) :
	pFolderModel_(pFolderModel),
	pFolderSelectionModel_(pFolderSelectionModel)
{
	assert(pstatus);
	*pstatus = QSTATUS_SUCCESS;
}

qm::FolderDeleteAction::~FolderDeleteAction()
{
}

QSTATUS qm::FolderDeleteAction::invoke(const ActionEvent& event)
{
	DECLARE_QSTATUS();
	
	Account::FolderList l;
	status = pFolderSelectionModel_->getSelectedFolders(&l);
	CHECK_QSTATUS();
	
	int nRet = 0;
	status = messageBox(Application::getApplication().getResourceHandle(),
		IDS_CONFIRMREMOVEFOLDER, MB_YESNO | MB_DEFBUTTON2 | MB_ICONQUESTION, &nRet);
	CHECK_QSTATUS();
	if (nRet != IDYES)
		return QSTATUS_SUCCESS;
	
	if (l.size() == 1) {
		Folder* pFolder = l[0];
		Account* pAccount = pFolder->getAccount();
		if (pFolderModel_->getCurrentFolder() == pFolder) {
			Folder* pParent = pFolder->getParentFolder();
			if (pParent) {
				status = pFolderModel_->setCurrent(0, pParent, false);
				CHECK_QSTATUS();
			}
			else {
				status = pFolderModel_->setCurrent(pAccount, 0, false);
				CHECK_QSTATUS();
			}
		}
		status = pAccount->removeFolder(pFolder);
		CHECK_QSTATUS();
	}
	else {
		std::sort(l.begin(), l.end(), std::not2(FolderLess()));
		
		Account::FolderList::const_iterator it = l.begin();
		while (it != l.end()) {
			Folder* pFolder = *it;
			Account* pAccount = pFolder->getAccount();
			assert(pFolderModel_->getCurrentAccount() == pAccount);
			status = pAccount->removeFolder(pFolder);
			CHECK_QSTATUS();
			++it;
		}
	}
	
	return QSTATUS_SUCCESS;
}

QSTATUS qm::FolderDeleteAction::isEnabled(const ActionEvent& event, bool* pbEnabled)
{
	assert(pbEnabled);
	return pFolderSelectionModel_->hasSelectedFolder(pbEnabled);
}


/****************************************************************************
 *
 * FolderEmptyAction
 *
 */

qm::FolderEmptyAction::FolderEmptyAction(FolderSelectionModel* pModel, QSTATUS* pstatus) :
	pModel_(pModel)
{
	assert(pstatus);
	*pstatus = QSTATUS_SUCCESS;
}

qm::FolderEmptyAction::~FolderEmptyAction()
{
}

QSTATUS qm::FolderEmptyAction::invoke(const ActionEvent& event)
{
	DECLARE_QSTATUS();
	
	Account::FolderList l;
	status = pModel_->getSelectedFolders(&l);
	CHECK_QSTATUS();
	
	Account::FolderList::const_iterator it = l.begin();
	while (it != l.end()) {
		Folder* pFolder = *it;
		
		if (!pFolder->isFlag(Folder::FLAG_TRASHBOX)) {
			Account* pAccount = pFolder->getAccount();
			Lock<Account> lock(*pAccount);
			
			const MessageHolderList* pList = 0;
			status = pFolder->getMessages(&pList);
			CHECK_QSTATUS();
			if (!pList->empty()) {
				MessageHolderList l;
				status = STLWrapper<MessageHolderList>(l).resize(pList->size());
				CHECK_QSTATUS();
				std::copy(pList->begin(), pList->end(), l.begin());
				status = pAccount->removeMessages(l, false, 0);
				CHECK_QSTATUS();
			}
		}
		
		++it;
	}
	
	return QSTATUS_SUCCESS;
}

QSTATUS qm::FolderEmptyAction::isEnabled(const ActionEvent& event, bool* pbEnabled)
{
	assert(pbEnabled);
	
	DECLARE_QSTATUS();
	
	Account::FolderList l;
	status = pModel_->getSelectedFolders(&l);
	CHECK_QSTATUS();
	
	*pbEnabled = l.size() > 1 ||
		(l.size() == 1 && !l.front()->isFlag(Folder::FLAG_TRASHBOX));
	
	return QSTATUS_SUCCESS;
}


/****************************************************************************
 *
 * FolderEmptyTrashAction
 *
 */

qm::FolderEmptyTrashAction::FolderEmptyTrashAction(
	FolderModel* pFolderModel, HWND hwndFrame, QSTATUS* pstatus) :
	pFolderModel_(pFolderModel),
	hwndFrame_(hwndFrame)
{
}

qm::FolderEmptyTrashAction::~FolderEmptyTrashAction()
{
}

QSTATUS qm::FolderEmptyTrashAction::invoke(const ActionEvent& event)
{
	DECLARE_QSTATUS();
	
	NormalFolder* pTrash = getTrash();
	if (pTrash) {
		Account* pAccount = pTrash->getAccount();
		Lock<Account> lock(*pAccount);
		
		// TODO
		// Sync folder if online and trash is syncable
		
		const MessageHolderList* pList = 0;
		status = pTrash->getMessages(&pList);
		CHECK_QSTATUS();
		if (!pList->empty()) {
			MessageHolderList l;
			status = STLWrapper<MessageHolderList>(l).resize(pList->size());
			CHECK_QSTATUS();
			std::copy(pList->begin(), pList->end(), l.begin());
			
			ProgressDialogMessageOperationCallback callback(
				hwndFrame_, IDS_EMPTYTRASH, IDS_EMPTYTRASH);
			status = pAccount->removeMessages(l, false, &callback);
			CHECK_QSTATUS();
			
			if (!pTrash->isFlag(Folder::FLAG_LOCAL)) {
				status = pAccount->clearDeletedMessages(pTrash);
				CHECK_QSTATUS();
			}
			
			status = pAccount->save();
			CHECK_QSTATUS();
		}
	}
	
	return QSTATUS_SUCCESS;
}

QSTATUS qm::FolderEmptyTrashAction::isEnabled(
	const ActionEvent& event, bool* pbEnabled)
{
	assert(pbEnabled);
	*pbEnabled = getTrash() != 0;
	return QSTATUS_SUCCESS;
}

NormalFolder* qm::FolderEmptyTrashAction::getTrash() const
{
	Account* pAccount = pFolderModel_->getCurrentAccount();
	if (!pAccount) {
		Folder* pFolder = pFolderModel_->getCurrentFolder();
		if (pFolder)
			pAccount = pFolder->getAccount();
	}
	if (pAccount) {
		Folder* pTrash = pAccount->getFolderByFlag(Folder::FLAG_TRASHBOX);
		if (pTrash && pTrash->getType() == Folder::TYPE_NORMAL)
			return static_cast<NormalFolder*>(pTrash);
	}
	return 0;
}


/****************************************************************************
 *
 * FolderPropertyAction
 *
 */

qm::FolderPropertyAction::FolderPropertyAction(FolderSelectionModel* pModel,
	HWND hwnd, Profile* pProfile, QSTATUS* pstatus) :
	pModel_(pModel),
	hwnd_(hwnd),
	pProfile_(pProfile)
{
}

qm::FolderPropertyAction::~FolderPropertyAction()
{
}

QSTATUS qm::FolderPropertyAction::invoke(const ActionEvent& event)
{
	DECLARE_QSTATUS();
	
	Account::FolderList listFolder;
	status = pModel_->getSelectedFolders(&listFolder);
	CHECK_QSTATUS();
	status = openProperty(listFolder, false, hwnd_, pProfile_);
	CHECK_QSTATUS();
	
	return QSTATUS_SUCCESS;
}

QSTATUS qm::FolderPropertyAction::isEnabled(const ActionEvent& event, bool* pbEnabled)
{
	assert(pbEnabled);
	return pModel_->hasSelectedFolder(pbEnabled);
}

QSTATUS qm::FolderPropertyAction::openProperty(
	const Account::FolderList& listFolder,
	bool bOpenCondition, HWND hwnd, Profile* pProfile)
{
	DECLARE_QSTATUS();
	
	HINSTANCE hInst = Application::getApplication().getResourceHandle();
	string_ptr<WSTRING> wstrTitle;
	status = loadString(hInst, IDS_PROPERTY, &wstrTitle);
	CHECK_QSTATUS();
	
	PropertySheetBase sheet(hInst, wstrTitle.get(), false, &status);
	CHECK_QSTATUS();
	FolderPropertyPage pageProperty(listFolder, &status);
	CHECK_QSTATUS();
	status = sheet.add(&pageProperty);
	CHECK_QSTATUS();
	
	QueryFolder* pQueryFolder = 0;
	std::auto_ptr<FolderConditionPage> pConditionPage;
	if (listFolder.size() == 1 &&
		listFolder.front()->getType() == Folder::TYPE_QUERY) {
		pQueryFolder = static_cast<QueryFolder*>(listFolder.front());
		status = newQsObject(pQueryFolder, pProfile, &pConditionPage);
		CHECK_QSTATUS();
		status = sheet.add(pConditionPage.get());
		CHECK_QSTATUS();
		if (bOpenCondition)
			sheet.setStartPage(1);
	}
	
	int nRet = 0;
	status = sheet.doModal(hwnd, 0, &nRet);
	CHECK_QSTATUS();
	if (nRet == IDOK) {
		Account::FolderList::const_iterator it = listFolder.begin();
		while (it != listFolder.end()) {
			Folder* pFolder = *it;
			
			unsigned int nFlags = pageProperty.getFlags();
			unsigned int nMask = pageProperty.getMask();
			if (!pFolder->isFlag(Folder::FLAG_SYNCABLE))
				nMask &= ~(Folder::FLAG_SYNCWHENOPEN | Folder::FLAG_CACHEWHENREAD);
			if (pFolder->isFlag(Folder::FLAG_NOSELECT))
				nMask &= ~(Folder::FLAG_INBOX | Folder::FLAG_OUTBOX |
					Folder::FLAG_SENTBOX | Folder::FLAG_DRAFTBOX | Folder::FLAG_TRASHBOX);
			
			pFolder->setFlags(nFlags, nMask);
			
			++it;
		}
		
		if (pQueryFolder) {
			status = pQueryFolder->set(pConditionPage->getDriver(),
				pConditionPage->getCondition(),
				pConditionPage->getTargetFolder(),
				pConditionPage->isRecursive());
			CHECK_QSTATUS();
		}
	}
	
	return QSTATUS_SUCCESS;
}


/****************************************************************************
 *
 * FolderRenameAction
 *
 */

qm::FolderRenameAction::FolderRenameAction(
	FolderSelectionModel* pFolderSelectionModel,
	HWND hwnd, qs::QSTATUS* pstatus) :
	pFolderSelectionModel_(pFolderSelectionModel),
	hwnd_(hwnd)
{
}

qm::FolderRenameAction::~FolderRenameAction()
{
}

QSTATUS qm::FolderRenameAction::invoke(const ActionEvent& event)
{
	DECLARE_QSTATUS();
	
	Folder* pFolder = pFolderSelectionModel_->getFocusedFolder();
	if (pFolder) {
		RenameDialog dialog(pFolder->getName(), &status);
		CHECK_QSTATUS();
		int nRet = 0;
		status = dialog.doModal(hwnd_, 0, &nRet);
		CHECK_QSTATUS();
		if (nRet == IDOK) {
			const WCHAR* pwszName = dialog.getName();
			if (wcscmp(pFolder->getName(), pwszName) != 0) {
				Account* pAccount = pFolder->getAccount();
				status = pAccount->renameFolder(pFolder, pwszName);
				CHECK_QSTATUS();
			}
		}
	}
	
	return QSTATUS_SUCCESS;
}

QSTATUS qm::FolderRenameAction::isEnabled(const ActionEvent& event, bool* pbEnabled)
{
	assert(pbEnabled);
	*pbEnabled = pFolderSelectionModel_->getFocusedFolder() != 0;
	return QSTATUS_SUCCESS;
}


/****************************************************************************
 *
 * FolderShowSizeAction
 *
 */

qm::FolderShowSizeAction::FolderShowSizeAction(
	FolderListWindow* pFolderListWindow, QSTATUS* pstatus) :
	pFolderListWindow_(pFolderListWindow)
{
}

qm::FolderShowSizeAction::~FolderShowSizeAction()
{
}

QSTATUS qm::FolderShowSizeAction::invoke(const ActionEvent& event)
{
	return pFolderListWindow_->showSize();
}

QSTATUS qm::FolderShowSizeAction::isEnabled(
	const ActionEvent& event, bool* pbEnabled)
{
	*pbEnabled = pFolderListWindow_->isShow() &&
		!pFolderListWindow_->isSizeShown();
	return QSTATUS_SUCCESS;
}


/****************************************************************************
 *
 * FolderUpdateAction
 *
 */

qm::FolderUpdateAction::FolderUpdateAction(
	FolderModel* pFolderModel, QSTATUS* pstatus) :
	pFolderModel_(pFolderModel)
{
	assert(pstatus);
	*pstatus = QSTATUS_SUCCESS;
}

qm::FolderUpdateAction::~FolderUpdateAction()
{
}

QSTATUS qm::FolderUpdateAction::invoke(const ActionEvent& event)
{
	DECLARE_QSTATUS();
	
	Account* pAccount = pFolderModel_->getCurrentAccount();
	if (!pAccount) {
		Folder* pFolder = pFolderModel_->getCurrentFolder();
		if (pFolder)
			pAccount = pFolder->getAccount();
	}
	if (!pAccount)
		return QSTATUS_FAIL;
	
	status = pFolderModel_->setCurrent(pAccount, 0, false);
	CHECK_QSTATUS();
	
	// TODO
	// Show progress dialog box?
	status = pAccount->updateFolders();
	CHECK_QSTATUS();
	
	// TODO
	// Error handling
	
	return QSTATUS_SUCCESS;
}

QSTATUS qm::FolderUpdateAction::isEnabled(const ActionEvent& event, bool* pbEnabled)
{
	assert(pbEnabled);
	
	Account* pAccount = pFolderModel_->getCurrentAccount();
	if (!pAccount) {
		Folder* pFolder = pFolderModel_->getCurrentFolder();
		if (pFolder)
			pAccount = pFolder->getAccount();
	}
	*pbEnabled = pAccount && pAccount->isSupport(Account::SUPPORT_REMOTEFOLDER);
	
	return QSTATUS_SUCCESS;
}


/****************************************************************************
 *
 * MessageApplyRuleAction
 *
 */

qm::MessageApplyRuleAction::MessageApplyRuleAction(RuleManager* pRuleManager,
	FolderModel* pFolderModel, Document* pDocument, HWND hwnd,
	Profile* pProfile, QSTATUS* pstatus) :
	pRuleManager_(pRuleManager),
	pFolderModel_(pFolderModel),
	pMessageSelectionModel_(0),
	pDocument_(pDocument),
	hwnd_(hwnd),
	pProfile_(pProfile)
{
}

qm::MessageApplyRuleAction::MessageApplyRuleAction(RuleManager* pRuleManager,
	MessageSelectionModel* pMessageSelectionModel,
	Document* pDocument, HWND hwnd, Profile* pProfile, QSTATUS* pstatus) :
	pRuleManager_(pRuleManager),
	pFolderModel_(0),
	pMessageSelectionModel_(pMessageSelectionModel),
	pDocument_(pDocument),
	hwnd_(hwnd),
	pProfile_(pProfile)
{
}

qm::MessageApplyRuleAction::~MessageApplyRuleAction()
{
}

QSTATUS qm::MessageApplyRuleAction::invoke(const ActionEvent& event)
{
	DECLARE_QSTATUS();
	
	struct RuleCallbackImpl : public RuleCallback
	{
		RuleCallbackImpl(ProgressDialog* pProgressDialog) :
			pDialog_(pProgressDialog) {}
		virtual ~RuleCallbackImpl() {}
		
		virtual bool isCanceled()
			{ return pDialog_->isCanceled(); }
		virtual QSTATUS checkingMessages()
			{ return pDialog_->setMessage(IDS_APPLYRULE_CHECKINGMESSAGES); }
		virtual QSTATUS applyingRule()
			{ return pDialog_->setMessage(IDS_APPLYRULE_APPLYINGRULE); }
		virtual QSTATUS setRange(unsigned int nMin, unsigned int nMax)
			{ return pDialog_->setRange(nMin, nMax); }
		virtual QSTATUS setPos(unsigned int nPos)
			{ return pDialog_->setPos(nPos); }
		
		ProgressDialog* pDialog_;
	};
	
	ProgressDialog dialog(IDS_APPLYMESSAGERULES, &status);
	CHECK_QSTATUS();
	RuleCallbackImpl callback(&dialog);
	
	if (pFolderModel_) {
		Folder* pFolder = pFolderModel_->getCurrentFolder();
		if (pFolder) {
			ProgressDialogInit init(&dialog, hwnd_, &status);
			CHECK_QSTATUS();
			status = pRuleManager_->apply(pFolder,
				0, pDocument_, hwnd_, pProfile_, &callback);
			CHECK_QSTATUS();
		}
	}
	else {
		AccountLock lock;
		Folder* pFolder = 0;
		MessageHolderList l;
		status = pMessageSelectionModel_->getSelectedMessages(&lock, &pFolder, &l);
		CHECK_QSTATUS();
		if (!l.empty()) {
			ProgressDialogInit init(&dialog, hwnd_, &status);
			CHECK_QSTATUS();
			status = pRuleManager_->apply(pFolder, &l,
				pDocument_, hwnd_, pProfile_, &callback);
			CHECK_QSTATUS();
		}
	}
	
	return QSTATUS_SUCCESS;
}

QSTATUS qm::MessageApplyRuleAction::isEnabled(
	const ActionEvent& event, bool* pbEnabled)
{
	assert(pbEnabled);
	
	DECLARE_QSTATUS();
	
	if (pFolderModel_) {
		*pbEnabled = pFolderModel_->getCurrentFolder() != 0;
	}
	else {
		status = pMessageSelectionModel_->hasSelectedMessage(pbEnabled);
		CHECK_QSTATUS();
	}
	
	return QSTATUS_SUCCESS;
}


/****************************************************************************
 *
 * MessageApplyTemplateAction
 *
 */

qm::MessageApplyTemplateAction::MessageApplyTemplateAction(
	TemplateMenu* pTemplateMenu, Document* pDocument,
	FolderModelBase* pFolderModel, MessageSelectionModel* pMessageSelectionModel,
	EditFrameWindowManager* pEditFrameWindowManager,
	ExternalEditorManager* pExternalEditorManager,
	HWND hwnd, Profile* pProfile, bool bExternalEditor, QSTATUS* pstatus) :
	processor_(pDocument, pFolderModel, pMessageSelectionModel,
		pEditFrameWindowManager, pExternalEditorManager, hwnd,
		pProfile, bExternalEditor),
	pTemplateMenu_(pTemplateMenu)
{
}

qm::MessageApplyTemplateAction::~MessageApplyTemplateAction()
{
}

QSTATUS qm::MessageApplyTemplateAction::invoke(const ActionEvent& event)
{
	DECLARE_QSTATUS();
	
	const WCHAR* pwszTemplate = pTemplateMenu_->getTemplate(event.getId());
	assert(pwszTemplate);
	status = processor_.process(pwszTemplate,
		(event.getModifier() & ActionEvent::MODIFIER_SHIFT) != 0);
	CHECK_QSTATUS();
	
	return QSTATUS_SUCCESS;
}

QSTATUS qm::MessageApplyTemplateAction::isEnabled(
	const ActionEvent& event, bool* pbEnabled)
{
	assert(pbEnabled);
	// TODO
	*pbEnabled = true;
	return QSTATUS_SUCCESS;
}


/****************************************************************************
 *
 * MessageCombineAction
 *
 */

qm::MessageCombineAction::MessageCombineAction(
	MessageSelectionModel* pMessageSelectionModel, HWND hwnd, QSTATUS* pstatus) :
	pMessageSelectionModel_(pMessageSelectionModel),
	hwnd_(hwnd)
{
}

qm::MessageCombineAction::~MessageCombineAction()
{
}

QSTATUS qm::MessageCombineAction::invoke(const ActionEvent& event)
{
	DECLARE_QSTATUS();
	
	AccountLock lock;
	MessageHolderList l;
	status = pMessageSelectionModel_->getSelectedMessages(&lock, 0, &l);
	CHECK_QSTATUS();
	
	Account* pAccount = lock.get();
	if (!l.empty()) {
		Message msg(&status);
		CHECK_QSTATUS();
		status = combine(l, &msg);
		CHECK_QSTATUS();
		
		// TODO
		NormalFolder* pFolder = l.front()->getFolder();
		unsigned int nFlags = 0;
		status = pAccount->appendMessage(pFolder, msg, nFlags);
		CHECK_QSTATUS();
	}
	
	return QSTATUS_SUCCESS;
}

QSTATUS qm::MessageCombineAction::isEnabled(const ActionEvent& event, bool* pbEnabled)
{
	assert(pbEnabled);
	return pMessageSelectionModel_->hasSelectedMessage(pbEnabled);
}

QSTATUS qm::MessageCombineAction::combine(
	const MessageHolderList& l, Message* pMessage)
{
	assert(pMessage);
	
	DECLARE_QSTATUS();
	
	MessageHolderList listMessageHolder;
	status = STLWrapper<MessageHolderList>(listMessageHolder).resize(l.size());
	CHECK_QSTATUS();
	
	string_ptr<WSTRING> wstrIdAll;
	unsigned int nTotal = 0;
	MessageHolderList::const_iterator it = l.begin();
	while (it != l.end()) {
		MessageHolder* pmh = *it;
		
		Message msg(&status);
		CHECK_QSTATUS();
		status = pmh->getMessage(Account::GETMESSAGEFLAG_HEADER, L"Content-Type", &msg);
		CHECK_QSTATUS();
		
		const ContentTypeParser* pContentType = msg.getContentType();
		if (!PartUtil::isContentType(pContentType, L"message", L"partial"))
			return QSTATUS_FAIL;
		
		string_ptr<WSTRING> wstrId;
		status = pContentType->getParameter(L"id", &wstrId);
		CHECK_QSTATUS();
		if (!wstrId.get())
			return QSTATUS_FAIL;
		else if (!wstrIdAll.get())
			wstrIdAll.reset(wstrId.release());
		else if (wcscmp(wstrId.get(), wstrIdAll.get()) != 0)
			return QSTATUS_FAIL;
		
		if (nTotal == 0) {
			string_ptr<WSTRING> wstrTotal;
			status = pContentType->getParameter(L"total", &wstrTotal);
			CHECK_QSTATUS();
			if (wstrTotal.get()) {
				WCHAR* pEnd = 0;
				nTotal = wcstol(wstrTotal.get(), &pEnd, 10);
				if (*pEnd || nTotal != l.size())
					return QSTATUS_FAIL;
			}
		}
		
		string_ptr<WSTRING> wstrNumber;
		status = pContentType->getParameter(L"number", &wstrNumber);
		CHECK_QSTATUS();
		WCHAR* pEnd = 0;
		unsigned int nNumber = wcstol(wstrNumber.get(), &pEnd, 10);
		if (*pEnd || nNumber == 0 || nNumber > l.size())
			return QSTATUS_FAIL;
		listMessageHolder[nNumber - 1] = *it;
		// TODO
		// Check duplicated number
		
		++it;
	}
	if (nTotal == 0)
		return QSTATUS_FAIL;
	
	StringBuffer<STRING> buf(&status);
	CHECK_QSTATUS();
	
	Part::FieldList listField;
	struct Deleter
	{
		Deleter(Part::FieldList& l) :
			l_(l)
		{
		}
		
		~Deleter()
		{
			clear();
		}
		
		void clear()
		{
			Part::FieldList::iterator it = l_.begin();
			while (it != l_.end()) {
				freeString((*it).first);
				freeString((*it).second);
				++it;
			}
			l_.clear();
		}
		
		Part::FieldList& l_;
	} deleter(listField);
	
	it = listMessageHolder.begin();
	while (it != listMessageHolder.end()) {
//		MessagePtrLock mpl(*it);
//		if (!mpl)
//			return QSTATUS_FAIL;
		MessageHolder* pmh = *it;
		
		Message msg(&status);
		CHECK_QSTATUS();
		status = pmh->getMessage(Account::GETMESSAGEFLAG_ALL, 0, &msg);
		CHECK_QSTATUS();
		
		if (it == listMessageHolder.begin()) {
			status = msg.getFields(&listField);
			CHECK_QSTATUS();
		}
		
		status = buf.append(msg.getBody());
		CHECK_QSTATUS();
		
		++it;
	}
	
	status = pMessage->create(buf.getCharArray(),
		buf.getLength(), Message::FLAG_NONE);
	CHECK_QSTATUS();
	buf.remove();
	
	Part::FieldList::const_iterator itF = listField.begin();
	while (itF != listField.end()) {
		if (!isSpecialField((*itF).first)) {
			status = buf.append((*itF).second);
			CHECK_QSTATUS();
			status = buf.append("\r\n");
			CHECK_QSTATUS();
		}
		++itF;
	}
	
	deleter.clear();
	status = pMessage->getFields(&listField);
	CHECK_QSTATUS();
	itF = listField.begin();
	while (itF != listField.end()) {
		if (isSpecialField((*itF).first)) {
			status = buf.append((*itF).second);
			CHECK_QSTATUS();
			status = buf.append("\r\n");
			CHECK_QSTATUS();
		}
		++itF;
	}
	
	status = pMessage->setHeader(buf.getCharArray());
	CHECK_QSTATUS();
	
	return QSTATUS_SUCCESS;
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
	FolderModelBase* pFolderModel, MessageSelectionModel* pMessageSelectionModel,
	const WCHAR* pwszTemplateName, EditFrameWindowManager* pEditFrameWindowManager,
	ExternalEditorManager* pExternalEditorManager, HWND hwnd,
	Profile* pProfile, bool bExternalEditor, QSTATUS* pstatus) :
	processor_(pDocument, pFolderModel, pMessageSelectionModel,
		pEditFrameWindowManager, pExternalEditorManager,
		hwnd, pProfile, bExternalEditor),
	pFolderModel_(pFolderModel),
	wstrTemplateName_(0)
{
	wstrTemplateName_ = allocWString(pwszTemplateName);
	if (!wstrTemplateName_) {
		*pstatus = QSTATUS_OUTOFMEMORY;
		return;
	}
}

qm::MessageCreateAction::~MessageCreateAction()
{
	freeWString(wstrTemplateName_);
}

QSTATUS qm::MessageCreateAction::invoke(const ActionEvent& event)
{
	DECLARE_QSTATUS();
	
	status = processor_.process(wstrTemplateName_,
		(event.getModifier() & ActionEvent::MODIFIER_SHIFT) != 0);
	CHECK_QSTATUS();
	
	return QSTATUS_SUCCESS;
}

QSTATUS qm::MessageCreateAction::isEnabled(
	const ActionEvent& event, bool* pbEnabled)
{
	assert(pbEnabled);
	*pbEnabled = pFolderModel_->getCurrentAccount() ||
		pFolderModel_->getCurrentFolder();
	return QSTATUS_SUCCESS;
}


/****************************************************************************
 *
 * MessageCreateFromClipboardAction
 *
 */

qm::MessageCreateFromClipboardAction::MessageCreateFromClipboardAction(
	bool bDraft, Document* pDocument, Profile* pProfile,
	HWND hwnd, FolderModel* pFolderModel, QSTATUS* pstatus) :
	composer_(bDraft, pDocument, pProfile, hwnd, pFolderModel)
{
}

qm::MessageCreateFromClipboardAction::~MessageCreateFromClipboardAction()
{
}

QSTATUS qm::MessageCreateFromClipboardAction::invoke(const ActionEvent& event)
{
	DECLARE_QSTATUS();
	
	string_ptr<WSTRING> wstrMessage;
	status = Clipboard::getText(&wstrMessage);
	CHECK_QSTATUS();
	
	Message* p = 0;
	MessageCreator creator;
	status = creator.createMessage(wstrMessage.get(), -1, &p);
	CHECK_QSTATUS();
	std::auto_ptr<Message> pMessage(p);
	
	unsigned int nFlags = 0;
	// TODO
	// Set flags
	status = composer_.compose(0, 0, pMessage.get(), nFlags);
	CHECK_QSTATUS();
	
	return QSTATUS_SUCCESS;
}

QSTATUS qm::MessageCreateFromClipboardAction::isEnabled(
	const ActionEvent& event, bool* pbEnabled)
{
	return Clipboard::isFormatAvailable(Clipboard::CF_QSTEXT, pbEnabled);
}


/****************************************************************************
 *
 * MessageDeleteAttachmentAction
 *
 */

qm::MessageDeleteAttachmentAction::MessageDeleteAttachmentAction(
	MessageSelectionModel* pMessageSelectionModel, qs::QSTATUS* pstatus) :
	pMessageSelectionModel_(pMessageSelectionModel)
{
}

qm::MessageDeleteAttachmentAction::~MessageDeleteAttachmentAction()
{
}

QSTATUS qm::MessageDeleteAttachmentAction::invoke(const ActionEvent& event)
{
	DECLARE_QSTATUS();
	
	AccountLock lock;
	MessageHolderList l;
	status = pMessageSelectionModel_->getSelectedMessages(&lock, 0, &l);
	CHECK_QSTATUS();
	
	Account* pAccount = lock.get();
	MessageHolderList::const_iterator it = l.begin();
	while (it != l.end()) {
		MessageHolder* pmh = *it;
		
		Message msg(&status);
		CHECK_QSTATUS();
		status = pmh->getMessage(Account::GETMESSAGEFLAG_ALL, 0, &msg);
		CHECK_QSTATUS();
		status = AttachmentParser::removeAttachments(&msg);
		CHECK_QSTATUS();
		status = AttachmentParser::setAttachmentDeleted(&msg);
		CHECK_QSTATUS();
		
		NormalFolder* pFolder = pmh->getFolder();
		status = pAccount->appendMessage(pFolder, msg,
			pmh->getFlags() & MessageHolder::FLAG_USER_MASK);
		CHECK_QSTATUS();
		
		MessageHolderList listRemove;
		status = STLWrapper<MessageHolderList>(listRemove).push_back(pmh);
		CHECK_QSTATUS();
		status = pAccount->removeMessages(l, false, 0);
		CHECK_QSTATUS();
		
		++it;
	}
	
	return QSTATUS_SUCCESS;
}

QSTATUS qm::MessageDeleteAttachmentAction::isEnabled(
	const ActionEvent& event, bool* pbEnabled)
{
	assert(pbEnabled);
	return pMessageSelectionModel_->hasSelectedMessage(pbEnabled);
}


/****************************************************************************
 *
 * MessageDetachAction
 *
 */

qm::MessageDetachAction::MessageDetachAction(Profile* pProfile,
	MessageSelectionModel* pMessageSelectionModel, HWND hwnd, QSTATUS* pstatus) :
	pMessageSelectionModel_(pMessageSelectionModel),
	helper_(pProfile, 0, hwnd)
{
}

qm::MessageDetachAction::~MessageDetachAction()
{
}

QSTATUS qm::MessageDetachAction::invoke(const ActionEvent& event)
{
	DECLARE_QSTATUS();
	
	AccountLock lock;
	MessageHolderList l;
	status = pMessageSelectionModel_->getSelectedMessages(&lock, 0, &l);
	CHECK_QSTATUS();
	
	status = helper_.detach(l, 0);
	CHECK_QSTATUS();
	
	return QSTATUS_SUCCESS;
}

QSTATUS qm::MessageDetachAction::isEnabled(const ActionEvent& event, bool* pbEnabled)
{
	assert(pbEnabled);
	return pMessageSelectionModel_->hasSelectedMessage(pbEnabled);
}


/****************************************************************************
 *
 * MessageExpandDigestAction
 *
 */

qm::MessageExpandDigestAction::MessageExpandDigestAction(
	MessageSelectionModel* pMessageSelectionModel, QSTATUS* pstatus) :
	pMessageSelectionModel_(pMessageSelectionModel)
{
}

qm::MessageExpandDigestAction::~MessageExpandDigestAction()
{
}

QSTATUS qm::MessageExpandDigestAction::invoke(const ActionEvent& event)
{
	DECLARE_QSTATUS();
	
	AccountLock lock;
	MessageHolderList l;
	status = pMessageSelectionModel_->getSelectedMessages(&lock, 0, &l);
	CHECK_QSTATUS();
	
	Account* pAccount = lock.get();
	MessageHolderList::const_iterator itM = l.begin();
	while (itM != l.end()) {
		MessageHolder* pmh = *itM;
		
		Message msg(&status);
		CHECK_QSTATUS();
		status = pmh->getMessage(Account::GETMESSAGEFLAG_ALL, 0, &msg);
		CHECK_QSTATUS();
		
		PartUtil::MessageList listMessage;
		struct Deleter
		{
			Deleter(PartUtil::MessageList& l) :
				l_(l)
			{
			}
			
			~Deleter()
			{
				std::for_each(l_.begin(), l_.end(), deleter<Message>());
			}
			
			PartUtil::MessageList& l_;
		} deleter(listMessage);
		status = PartUtil(msg).getDigest(&listMessage);
		CHECK_QSTATUS();
		PartUtil::MessageList::const_iterator itE = listMessage.begin();
		while (itE != listMessage.end()) {
			// TODO
			// Set flags?
			unsigned int nFlags = 0;
			status = pAccount->appendMessage(pmh->getFolder(), **itE, nFlags);
			CHECK_QSTATUS();
			++itE;
		}
		
		++itM;
	}
	
	return QSTATUS_SUCCESS;
}

QSTATUS qm::MessageExpandDigestAction::isEnabled(const ActionEvent& event, bool* pbEnabled)
{
	assert(pbEnabled);
	return pMessageSelectionModel_->hasSelectedMessage(pbEnabled);
}


/****************************************************************************
 *
 * MessageMarkAction
 *
 */

qm::MessageMarkAction::MessageMarkAction(MessageSelectionModel* pModel,
	unsigned int nFlags, unsigned int nMask, QSTATUS* pstatus) :
	pModel_(pModel),
	nFlags_(nFlags),
	nMask_(nMask)
{
}

qm::MessageMarkAction::~MessageMarkAction()
{
}

QSTATUS qm::MessageMarkAction::invoke(const ActionEvent& event)
{
	DECLARE_QSTATUS();
	
	AccountLock lock;
	MessageHolderList l;
	status = pModel_->getSelectedMessages(&lock, 0, &l);
	CHECK_QSTATUS();
	
	Account* pAccount = lock.get();
	if (!l.empty()) {
		status = pAccount->setMessagesFlags(l, nFlags_, nMask_);
		CHECK_QSTATUS();
	}
	
	return QSTATUS_SUCCESS;
}

QSTATUS qm::MessageMarkAction::isEnabled(const ActionEvent& event, bool* pbEnabled)
{
	assert(pbEnabled);
	return pModel_->hasSelectedMessage(pbEnabled);
}


/****************************************************************************
 *
 * MessageMoveAction
 *
 */

qm::MessageMoveAction::MessageMoveAction(MessageSelectionModel* pModel,
	MoveMenu* pMoveMenu, HWND hwndFrame, QSTATUS* pstatus) :
	pModel_(pModel),
	pMoveMenu_(pMoveMenu),
	hwndFrame_(hwndFrame)
{
}

qm::MessageMoveAction::~MessageMoveAction()
{
}

QSTATUS qm::MessageMoveAction::invoke(const ActionEvent& event)
{
	DECLARE_QSTATUS();
	
	NormalFolder* pFolderTo = pMoveMenu_->getFolder(event.getId());
	if (pFolderTo) {
		AccountLock lock;
		MessageHolderList l;
		status = pModel_->getSelectedMessages(&lock, 0, &l);
		CHECK_QSTATUS();
		
		Account* pAccount = lock.get();
		if (!l.empty()) {
			bool bMove = (event.getModifier() & ActionEvent::MODIFIER_CTRL) == 0;
			UINT nId = bMove ? IDS_MOVEMESSAGE : IDS_COPYMESSAGE;
			ProgressDialogMessageOperationCallback callback(hwndFrame_, nId, nId);
			status = pAccount->copyMessages(l, pFolderTo, bMove, &callback);
			CHECK_QSTATUS();
		}
	}
	
	return QSTATUS_SUCCESS;
}

QSTATUS qm::MessageMoveAction::isEnabled(const ActionEvent& event, bool* pbEnabled)
{
	assert(pbEnabled);
	return pModel_->hasSelectedMessage(pbEnabled);
}


/****************************************************************************
 *
 * MessageMoveOtherAction
 *
 */

qm::MessageMoveOtherAction::MessageMoveOtherAction(
	Document* pDocument, MessageSelectionModel* pModel,
	Profile* pProfile, HWND hwndFrame, QSTATUS* pstatus) :
	pDocument_(pDocument),
	pModel_(pModel),
	pProfile_(pProfile),
	hwndFrame_(hwndFrame)
{
}

qm::MessageMoveOtherAction::~MessageMoveOtherAction()
{
}

QSTATUS qm::MessageMoveOtherAction::invoke(const ActionEvent& event)
{
	DECLARE_QSTATUS();
	
	MoveMessageDialog dialog(pDocument_, pProfile_, &status);
	CHECK_QSTATUS();
	int nRet = 0;
	status = dialog.doModal(hwndFrame_, 0, &nRet);
	CHECK_QSTATUS();
	if (nRet == IDOK) {
		NormalFolder* pFolderTo = dialog.getFolder();
		if (pFolderTo) {
			AccountLock lock;
			MessageHolderList l;
			status = pModel_->getSelectedMessages(&lock, 0, &l);
			CHECK_QSTATUS();
			
			Account* pAccount = lock.get();
			if (!l.empty()) {
				bool bMove = !dialog.isCopy();
				
				UINT nId = bMove ? IDS_MOVEMESSAGE : IDS_COPYMESSAGE;
				ProgressDialogMessageOperationCallback callback(
					hwndFrame_, nId, nId);
				status = pAccount->copyMessages(l, pFolderTo, bMove, &callback);
				CHECK_QSTATUS();
			}
		}
	}
	
	return QSTATUS_SUCCESS;
}

QSTATUS qm::MessageMoveOtherAction::isEnabled(
	const ActionEvent& event, bool* pbEnabled)
{
	assert(pbEnabled);
	return pModel_->hasSelectedMessage(pbEnabled);
}


/****************************************************************************
 *
 * MessageOpenAttachmentAction
 *
 */

qm::MessageOpenAttachmentAction::MessageOpenAttachmentAction(
	Profile* pProfile, AttachmentMenu* pAttachmentMenu,
	TempFileCleaner* pTempFileCleaner, HWND hwnd, QSTATUS* pstatus) :
	pAttachmentMenu_(pAttachmentMenu),
	helper_(pProfile, pTempFileCleaner, hwnd)
{
}

qm::MessageOpenAttachmentAction::~MessageOpenAttachmentAction()
{
}

QSTATUS qm::MessageOpenAttachmentAction::invoke(const ActionEvent& event)
{
	DECLARE_QSTATUS();
	
	Message msg(&status);
	CHECK_QSTATUS();
	string_ptr<WSTRING> wstrName;
	const Part* pPart = 0;
	status = pAttachmentMenu_->getPart(event.getId(), &msg, &wstrName, &pPart);
	CHECK_QSTATUS();
	
	status = helper_.open(pPart, wstrName.get(),
		(event.getModifier() & ActionEvent::MODIFIER_SHIFT) != 0);
	CHECK_QSTATUS();
	
	return QSTATUS_SUCCESS;
}


/****************************************************************************
 *
 * MessageOpenURLAction
 *
 */

qm::MessageOpenURLAction::MessageOpenURLAction(Document* pDocument,
	FolderModelBase* pFolderModel, MessageSelectionModel* pMessageSelectionModel,
	EditFrameWindowManager* pEditFrameWindowManager,
	ExternalEditorManager* pExternalEditorManager,
	HWND hwnd, Profile* pProfile, bool bExternalEditor, QSTATUS* pstatus) :
	processor_(pDocument, pFolderModel, pMessageSelectionModel,
		pEditFrameWindowManager, pExternalEditorManager, hwnd,
		pProfile, bExternalEditor)
{
}

qm::MessageOpenURLAction::~MessageOpenURLAction()
{
}

QSTATUS qm::MessageOpenURLAction::invoke(const ActionEvent& event)
{
	DECLARE_QSTATUS();
	
	if (event.getParam()) {
		ActionParam* pParam = static_cast<ActionParam*>(event.getParam());
		if (pParam->nArgs_ > 0) {
			Variant v;
			if (::VariantChangeType(&v, pParam->ppvarArgs_[0], 0, VT_BSTR) == S_OK) {
				TemplateContext::ArgumentList listArgument;
				TemplateContext::Argument arg = {
					L"url",
					v.bstrVal
				};
				status = STLWrapper<TemplateContext::ArgumentList>(
					listArgument).push_back(arg);
				CHECK_QSTATUS();
				status = processor_.process(L"url", listArgument,
					(event.getModifier() & ActionEvent::MODIFIER_SHIFT) != 0);
				CHECK_QSTATUS();
			}
		}
	}
	
	return QSTATUS_SUCCESS;
}

QSTATUS qm::MessageOpenURLAction::isEnabled(const ActionEvent& event, bool* pbEnabled)
{
	assert(pbEnabled);
	// TODO
	*pbEnabled = true;
	return QSTATUS_SUCCESS;
}


/****************************************************************************
 *
 * MessagePropertyAction
 *
 */

qm::MessagePropertyAction::MessagePropertyAction(
	MessageSelectionModel* pModel, HWND hwnd, QSTATUS* pstatus) :
	pModel_(pModel),
	hwnd_(hwnd)
{
}

qm::MessagePropertyAction::~MessagePropertyAction()
{
}

QSTATUS qm::MessagePropertyAction::invoke(const ActionEvent& event)
{
	DECLARE_QSTATUS();
	
	AccountLock lock;
	MessageHolderList l;
	status = pModel_->getSelectedMessages(&lock, 0, &l);
	CHECK_QSTATUS();
	
	Account* pAccount = lock.get();
	
	HINSTANCE hInst = Application::getApplication().getResourceHandle();
	string_ptr<WSTRING> wstrTitle;
	status = loadString(hInst, IDS_PROPERTY, &wstrTitle);
	CHECK_QSTATUS();
	
	MessagePropertyPage page(l, &status);
	CHECK_QSTATUS();
	PropertySheetBase sheet(hInst, wstrTitle.get(), false, &status);
	CHECK_QSTATUS();
	status = sheet.add(&page);
	CHECK_QSTATUS();
	
	int nRet = 0;
	status = sheet.doModal(hwnd_, 0, &nRet);
	CHECK_QSTATUS();
	if (nRet == IDOK) {
		status = pAccount->setMessagesFlags(l, page.getFlags(), page.getMask());
		CHECK_QSTATUS();
	}
	
	return QSTATUS_SUCCESS;
}

QSTATUS qm::MessagePropertyAction::isEnabled(
	const ActionEvent& event, bool* pbEnabled)
{
	assert(pbEnabled);
	return pModel_->hasSelectedMessage(pbEnabled);
}


/****************************************************************************
 *
 * MessageSearchAction
 *
 */

qm::MessageSearchAction::MessageSearchAction(FolderModel* pFolderModel,
	Document* pDocument, HWND hwnd, Profile* pProfile, QSTATUS* pstatus) :
	pFolderModel_(pFolderModel),
	pDocument_(pDocument),
	hwnd_(hwnd),
	pProfile_(pProfile)
{
}

qm::MessageSearchAction::~MessageSearchAction()
{
}

QSTATUS qm::MessageSearchAction::invoke(const ActionEvent& event)
{
	DECLARE_QSTATUS();
	
	Folder* pFolder = pFolderModel_->getCurrentFolder();
	Account* pAccount = pFolder ? pFolder->getAccount() :
		pFolderModel_->getCurrentAccount();
	
	Folder* pSearchFolder = pAccount->getFolderByFlag(Folder::FLAG_SEARCHBOX);
	if (!pSearchFolder || pSearchFolder->getType() != Folder::TYPE_QUERY)
		return QSTATUS_SUCCESS;
	QueryFolder* pSearch = static_cast<QueryFolder*>(pSearchFolder);
	
	HINSTANCE hInst = Application::getApplication().getResourceHandle();
	string_ptr<WSTRING> wstrTitle;
	status = loadString(hInst, IDS_SEARCH, &wstrTitle);
	CHECK_QSTATUS();
	
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
			UIList::iterator it = l_.begin();
			while (it != l_.end()) {
				delete (*it).first;
				delete (*it).second;
				++it;
			}
		}
		
		UIList& l_;
	} deleter(listUI);
	
	SearchDriverFactory::NameList listName;
	status = SearchDriverFactory::getNames(&listName);
	CHECK_QSTATUS();
	status = STLWrapper<UIList>(listUI).reserve(listName.size());
	CHECK_QSTATUS();
	SearchDriverFactory::NameList::iterator itN = listName.begin();
	while (itN != listName.end()) {
		SearchUI* pUI = 0;
		status = SearchDriverFactory::getUI(*itN, pAccount, pProfile_, &pUI);
		CHECK_QSTATUS();
		if (pUI)
			listUI.push_back(UIList::value_type(pUI, 0));
		++itN;
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
	
	string_ptr<WSTRING> wstrStartName;
	status = pProfile_->getString(L"Search", L"Page", 0, &wstrStartName);
	CHECK_QSTATUS();
	
	int nStartPage = 0;
	PropertySheetBase sheet(hInst, wstrTitle.get(), false, &status);
	CHECK_QSTATUS_VALUE(0);
	for (UIList::size_type n = 0; n < listUI.size(); ++n) {
		SearchPropertyPage* pPage = 0;
		status = listUI[n].first->createPropertyPage(pFolder == 0, &pPage);
		CHECK_QSTATUS();
		listUI[n].second = pPage;
		status = sheet.add(pPage);
		CHECK_QSTATUS();
		if (wcscmp(pPage->getDriver(), wstrStartName.get()) == 0)
			nStartPage = n;
	}
	status = sheet.setStartPage(nStartPage);
	CHECK_QSTATUS();
	
	int nRet = 0;
	status = sheet.doModal(hwnd_, 0, &nRet);
	CHECK_QSTATUS_VALUE(0);
	if (nRet == IDOK) {
		UIList::size_type nPage = 0;
		while (nPage < listUI.size()) {
			if (listUI[nPage].second->getCondition())
				break;
			++nPage;
		}
		
		if (nPage != listUI.size()) {
			SearchPropertyPage* pPage = listUI[nPage].second;
			const WCHAR* pwszCondition = pPage->getCondition();
			if (*pwszCondition) {
				WaitCursor cursor;
				
				string_ptr<WSTRING> wstrFolder;
				if (!pPage->isAllFolder()) {
					status = pFolder->getFullName(&wstrFolder);
					CHECK_QSTATUS();
				}
				
				status = pSearch->set(pPage->getDriver(), pwszCondition,
					wstrFolder.get(), pPage->isRecursive());
				CHECK_QSTATUS();
				if (pFolder == pSearch) {
					status = pSearch->search(pDocument_, hwnd_, pProfile_);
					CHECK_QSTATUS();
				}
				else {
					status = pFolderModel_->setCurrent(0, pSearch, false);
					CHECK_QSTATUS();
				}
			}
			status = pProfile_->setString(L"Search", L"Page", pPage->getDriver());
			CHECK_QSTATUS();
		}
	}
	
	return QSTATUS_SUCCESS;
}

QSTATUS qm::MessageSearchAction::isEnabled(const ActionEvent& event, bool* pbEnabled)
{
	assert(pbEnabled);
	*pbEnabled = pFolderModel_->getCurrentFolder() ||
		pFolderModel_->getCurrentAccount();
	return QSTATUS_SUCCESS;
}


/****************************************************************************
 *
 * ToolAccountAction
 *
 */

qm::ToolAccountAction::ToolAccountAction(Document* pDocument,
	FolderModel* pFolderModel, SyncFilterManager* pSyncFilterManager,
	Profile* pProfile, HWND hwndFrame, QSTATUS* pstatus) :
	pDocument_(pDocument),
	pFolderModel_(pFolderModel),
	pSyncFilterManager_(pSyncFilterManager),
	pProfile_(pProfile),
	hwndFrame_(hwndFrame)
{
}

qm::ToolAccountAction::~ToolAccountAction()
{
}

QSTATUS qm::ToolAccountAction::invoke(const ActionEvent& event)
{
	DECLARE_QSTATUS();
	
	// TODO
	// Make offline if it's online now
	
	Account* pAccount = pFolderModel_->getCurrentAccount();
	if (!pAccount) {
		Folder* pFolder = pFolderModel_->getCurrentFolder();
		if (pFolder)
			pAccount = pFolder->getAccount();
	}
	
	AccountDialog dialog(pDocument_, pAccount,
		pSyncFilterManager_, pProfile_, &status);
	CHECK_QSTATUS();
	int nRet = 0;
	status = dialog.doModal(hwndFrame_, 0, &nRet);
	CHECK_QSTATUS();
	
	return QSTATUS_SUCCESS;
}

QSTATUS qm::ToolAccountAction::isEnabled(
	const ActionEvent& event, bool* pbEnabled)
{
	assert(pbEnabled);
	// TODO
	// Check if syncing or not
	*pbEnabled = true;
	return QSTATUS_SUCCESS;
}


/****************************************************************************
 *
 * ToolCheckNewMailAction
 *
 */

qm::ToolCheckNewMailAction::ToolCheckNewMailAction(
	Document* pDocument, QSTATUS* pstatus) :
	pDocument_(pDocument)
{
}

qm::ToolCheckNewMailAction::~ToolCheckNewMailAction()
{
}

QSTATUS qm::ToolCheckNewMailAction::invoke(const ActionEvent& event)
{
	pDocument_->setCheckNewMail(!pDocument_->isCheckNewMail());
	return QSTATUS_SUCCESS;
}

QSTATUS qm::ToolCheckNewMailAction::isChecked(
	const ActionEvent& event, bool* pbChecked)
{
	assert(pbChecked);
	*pbChecked = pDocument_->isCheckNewMail();
	return QSTATUS_SUCCESS;
}


/****************************************************************************
 *
 * ToolDialupAction
 *
 */

qm::ToolDialupAction::ToolDialupAction(SyncManager* pSyncManager,
	Document* pDocument, SyncDialogManager* pSyncDialogManager,
	HWND hwnd, QSTATUS* pstatus) :
	pSyncManager_(pSyncManager),
	pDocument_(pDocument),
	pSyncDialogManager_(pSyncDialogManager),
	hwnd_(hwnd)
{
}

qm::ToolDialupAction::~ToolDialupAction()
{
}

QSTATUS qm::ToolDialupAction::invoke(const ActionEvent& event)
{
	DECLARE_QSTATUS();
	
	bool bConnected = false;
	status = isConnected(&bConnected);
	CHECK_QSTATUS();
	if (!bConnected) {
		std::auto_ptr<SyncData> pData;
		status = newQsObject(pSyncManager_, pDocument_,
			hwnd_, SyncDialog::FLAG_SHOWDIALOG, &pData);
		CHECK_QSTATUS();
		
		std::auto_ptr<SyncDialup> pDialup;
		status = newQsObject(static_cast<const WCHAR*>(0),
			SyncDialup::FLAG_SHOWDIALOG | SyncDialup::FLAG_NOTDISCONNECT,
			static_cast<const WCHAR*>(0), 0, &pDialup);
		CHECK_QSTATUS();
		pData->setDialup(pDialup.release());
		
		SyncDialog* pSyncDialog = 0;
		status = pSyncDialogManager_->open(&pSyncDialog);
		CHECK_QSTATUS();
		pData->setCallback(pSyncDialog->getSyncManagerCallback());
		
		status = pSyncManager_->sync(pData.get());
		CHECK_QSTATUS();
		pData.release();
	}
	else {
		RasConnection* p = 0;
		status = RasConnection::getActiveConnection(0, &p);
		CHECK_QSTATUS();
		std::auto_ptr<RasConnection> pRasConnection(p);
		RasConnection::Result result;
		status = pRasConnection->disconnect(false, &result);
		CHECK_QSTATUS();
	}
	
	return QSTATUS_SUCCESS;
}

QSTATUS qm::ToolDialupAction::getText(
	const ActionEvent& event, WSTRING* pwstrText)
{
	DECLARE_QSTATUS();
	
	bool bConnected = false;
	status = isConnected(&bConnected);
	CHECK_QSTATUS();
	
	HINSTANCE hInst = Application::getApplication().getResourceHandle();
	UINT nId = bConnected ? IDS_DIALUPDISCONNECT : IDS_DIALUPCONNECT;
	status = loadString(hInst, nId, pwstrText);
	CHECK_QSTATUS();
	
	return QSTATUS_SUCCESS;
}

QSTATUS qm::ToolDialupAction::isConnected(bool* pbConnected) const
{
	assert(pbConnected);
	
	DECLARE_QSTATUS();
	
	int nCount = 0;
	status = RasConnection::getActiveConnectionCount(&nCount);
	CHECK_QSTATUS();
	
	*pbConnected = nCount != 0;
	
	return QSTATUS_SUCCESS;
}


/****************************************************************************
 *
 * ToolGoRoundAction
 *
 */

qm::ToolGoRoundAction::ToolGoRoundAction(SyncManager* pSyncManager,
	Document* pDocument, GoRound* pGoRound, SyncDialogManager* pSyncDialogManager,
	HWND hwnd, GoRoundMenu* pGoRoundMenu, QSTATUS* pstatus) :
	pSyncManager_(pSyncManager),
	pDocument_(pDocument),
	pGoRound_(pGoRound),
	pSyncDialogManager_(pSyncDialogManager),
	hwnd_(hwnd),
	pGoRoundMenu_(pGoRoundMenu)
{
}

qm::ToolGoRoundAction::~ToolGoRoundAction()
{
}

QSTATUS qm::ToolGoRoundAction::invoke(const ActionEvent& event)
{
	DECLARE_QSTATUS();
	
	const GoRoundCourse* pCourse = 0;
	if (event.getParam()) {
		ActionParam* pParam = static_cast<ActionParam*>(event.getParam());
		if (pParam->nArgs_ > 0) {
			Variant v;
			if (::VariantChangeType(&v, pParam->ppvarArgs_[0], 0, VT_BSTR) != S_OK)
				return QSTATUS_SUCCESS;
			
			GoRoundCourseList* pCourseList = 0;
			status = pGoRound_->getCourseList(&pCourseList);
			CHECK_QSTATUS();
			pCourse = pCourseList->getCourse(v.bstrVal);
		}
	}
	else {
		status = pGoRoundMenu_->getCourse(event.getId(), &pCourse);
		CHECK_QSTATUS();
	}
	
	std::auto_ptr<SyncData> pData;
	status = newQsObject(pSyncManager_, pDocument_,
		hwnd_, SyncDialog::FLAG_SHOWDIALOG, &pData);
	CHECK_QSTATUS();
	status = SyncUtil::createGoRoundData(pCourse, pDocument_, pData.get());
	CHECK_QSTATUS();
	
	if (!pData->isEmpty()) {
		SyncDialog* pSyncDialog = 0;
		status = pSyncDialogManager_->open(&pSyncDialog);
		CHECK_QSTATUS();
		pData->setCallback(pSyncDialog->getSyncManagerCallback());
		
		status = pSyncManager_->sync(pData.get());
		CHECK_QSTATUS();
		pData.release();
	}
	
	return QSTATUS_SUCCESS;
}

QSTATUS qm::ToolGoRoundAction::isEnabled(
	const ActionEvent& event, bool* pbEnabled)
{
	assert(pbEnabled);
	*pbEnabled = true;
	return QSTATUS_SUCCESS;
}


/****************************************************************************
 *
 * ToolOptionsAction
 *
 */

qm::ToolOptionsAction::ToolOptionsAction(Profile* pProfile, QSTATUS* pstatus) :
	pProfile_(pProfile)
{
}

qm::ToolOptionsAction::~ToolOptionsAction()
{
}

QSTATUS qm::ToolOptionsAction::invoke(const ActionEvent& event)
{
	DECLARE_QSTATUS();
	
	// TODO
	
	return QSTATUS_SUCCESS;
}

QSTATUS qm::ToolOptionsAction::isEnabled(
	const ActionEvent& event, bool* pbEnabled)
{
	// TODO
	// Check wether syncing or not
	assert(pbEnabled);
//	*pbEnabled = true;
	*pbEnabled = false;
	return QSTATUS_SUCCESS;
}


/****************************************************************************
 *
 * ToolScriptAction
 *
 */

qm::ToolScriptAction::ToolScriptAction(ScriptMenu* pScriptMenu,
	Document* pDocument, Profile* pProfile,
	MainWindow* pMainWindow, QSTATUS* pstatus) :
	pScriptMenu_(pScriptMenu),
	pDocument_(pDocument),
	pProfile_(pProfile),
	pMainWindow_(pMainWindow),
	pEditFrameWindow_(0),
	pMessageFrameWindow_(0)
{
}

qm::ToolScriptAction::ToolScriptAction(ScriptMenu* pScriptMenu,
	Document* pDocument, Profile* pProfile,
	EditFrameWindow* pEditFrameWindow, QSTATUS* pstatus) :
	pScriptMenu_(pScriptMenu),
	pDocument_(pDocument),
	pProfile_(pProfile),
	pMainWindow_(0),
	pEditFrameWindow_(pEditFrameWindow),
	pMessageFrameWindow_(0)
{
}

qm::ToolScriptAction::ToolScriptAction(ScriptMenu* pScriptMenu,
	Document* pDocument, Profile* pProfile,
	MessageFrameWindow* pMessageFrameWindow, QSTATUS* pstatus) :
	pScriptMenu_(pScriptMenu),
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

QSTATUS qm::ToolScriptAction::invoke(const ActionEvent& event)
{
	DECLARE_QSTATUS();
	
	const WCHAR* pwszName = pScriptMenu_->getScript(event.getId());
	if (!pwszName)
		return QSTATUS_SUCCESS;
	
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
	
	ScriptManager* pScriptManager = pScriptMenu_->getScriptManager();
	Script* p = 0;
	status = pScriptManager->getScript(pwszName, pDocument_,
		pProfile_, getModalHandler(), info, &p);
	CHECK_QSTATUS();
	if (p) {
		std::auto_ptr<Script> pScript(p);
		status = pScript->run(0, 0, 0);
		CHECK_QSTATUS();
	}
	
	return QSTATUS_SUCCESS;
}

QSTATUS qm::ToolScriptAction::isEnabled(
	const ActionEvent& event, bool* pbEnabled)
{
	// TODO
	// Check wether syncing or not
	assert(pbEnabled);
	*pbEnabled = true;
	return QSTATUS_SUCCESS;
}


/****************************************************************************
 *
 * ToolSubAccountAction
 *
 */

qm::ToolSubAccountAction::ToolSubAccountAction(Document* pDocument,
	FolderModel* pFolderModel, SubAccountMenu* pSubAccountMenu, QSTATUS* pstatus) :
	pDocument_(pDocument),
	pFolderModel_(pFolderModel),
	pSubAccountMenu_(pSubAccountMenu)
{
}

qm::ToolSubAccountAction::~ToolSubAccountAction()
{
}

QSTATUS qm::ToolSubAccountAction::invoke(const ActionEvent& event)
{
	DECLARE_QSTATUS();
	
	const WCHAR* pwszName = pSubAccountMenu_->getName(event.getId());
	if (pwszName) {
		const Document::AccountList& listAccount = pDocument_->getAccounts();
		Document::AccountList::const_iterator it = listAccount.begin();
		while (it != listAccount.end()) {
			Account* pAccount = *it;
			SubAccount* pSubAccount = pAccount->getSubAccount(pwszName);
			if (pSubAccount)
				pAccount->setCurrentSubAccount(pSubAccount);
			++it;
		}
	}
	
	return QSTATUS_SUCCESS;
}

QSTATUS qm::ToolSubAccountAction::isEnabled(
	const ActionEvent& event, bool* pbEnabled)
{
	assert(pbEnabled);
	*pbEnabled = pFolderModel_->getCurrentAccount() ||
		pFolderModel_->getCurrentFolder();
	return QSTATUS_SUCCESS;
}

QSTATUS qm::ToolSubAccountAction::isChecked(
	const ActionEvent& event, bool* pbChecked)
{
	assert(pbChecked);
	
	Account* pAccount = pFolderModel_->getCurrentAccount();
	if (!pAccount) {
		Folder* pFolder = pFolderModel_->getCurrentFolder();
		if (pFolder)
			pAccount = pFolder->getAccount();
	}
	if (pAccount) {
		const WCHAR* pwszName = pSubAccountMenu_->getName(event.getId());
		SubAccount* pSubAccount = pAccount->getCurrentSubAccount();
		*pbChecked = wcscmp(pSubAccount->getName(), pwszName) == 0;
	}
	
	return QSTATUS_SUCCESS;
}


/****************************************************************************
 *
 * ToolSyncAction
 *
 */

qm::ToolSyncAction::ToolSyncAction(SyncManager* pSyncManager,
	Document* pDocument, FolderModel* pFolderModel,
	SyncDialogManager* pSyncDialogManager, unsigned int nSync,
	HWND hwnd, QSTATUS* pstatus) :
	pSyncManager_(pSyncManager),
	pDocument_(pDocument),
	pFolderModel_(pFolderModel),
	pSyncDialogManager_(pSyncDialogManager),
	nSync_(nSync),
	hwnd_(hwnd)
{
	assert(pstatus);
	*pstatus = QSTATUS_SUCCESS;
}

qm::ToolSyncAction::~ToolSyncAction()
{
}

QSTATUS qm::ToolSyncAction::invoke(const ActionEvent& event)
{
	DECLARE_QSTATUS();
	
	Account* pAccount = pFolderModel_->getCurrentAccount();
	if (!pAccount) {
		Folder* pFolder = pFolderModel_->getCurrentFolder();
		assert(pFolder);
		pAccount = pFolder->getAccount();
	}
	assert(pAccount);
	
	SubAccount* pSubAccount = pAccount->getCurrentSubAccount();
	
	std::auto_ptr<SyncData> pData;
	status = newQsObject(pSyncManager_, pDocument_,
		hwnd_, SyncDialog::FLAG_SHOWDIALOG, &pData);
	CHECK_QSTATUS();
	
	if (pSubAccount->getDialupType() != SubAccount::DIALUPTYPE_NEVER) {
		unsigned int nFlags = 0;
		if (pSubAccount->isDialupShowDialog())
			nFlags |= SyncDialup::FLAG_SHOWDIALOG;
		if (pSubAccount->getDialupType() == SubAccount::DIALUPTYPE_WHENEVERNOTCONNECTED)
			nFlags |= SyncDialup::FLAG_WHENEVERNOTCONNECTED;
		
		std::auto_ptr<SyncDialup> pDialup;
		status = newQsObject(pSubAccount->getDialupEntry(),
			nFlags, static_cast<const WCHAR*>(0),
			pSubAccount->getDialupDisconnectWait(), &pDialup);
		CHECK_QSTATUS();
		pData->setDialup(pDialup.release());
	}
	
	if (nSync_ & SYNC_SEND) {
		status = pData->addSend(pAccount, pSubAccount, SyncItem::CRBS_NONE);
		CHECK_QSTATUS();
	}
	if (nSync_ & SYNC_RECEIVE) {
		if (event.getModifier() & ActionEvent::MODIFIER_SHIFT) {
			SelectSyncFilterDialog dialog(pSyncManager_->getSyncFilterManager(),
				pAccount, pSubAccount->getSyncFilterName(), &status);
			CHECK_QSTATUS();
			int nRet = 0;
			status = dialog.doModal(hwnd_, 0, &nRet);
			CHECK_QSTATUS();
			if (nRet != IDOK)
				return QSTATUS_SUCCESS;
			status = pData->addFolders(pAccount, pSubAccount, 0, dialog.getName());
			CHECK_QSTATUS();
		}
		else {
			status = pData->addFolders(pAccount, pSubAccount,
				0, pSubAccount->getSyncFilterName());
			CHECK_QSTATUS();
		}
	}
	
	if (!pData->isEmpty()) {
		SyncDialog* pSyncDialog = 0;
		status = pSyncDialogManager_->open(&pSyncDialog);
		CHECK_QSTATUS();
		pData->setCallback(pSyncDialog->getSyncManagerCallback());
		
		status = pSyncManager_->sync(pData.get());
		CHECK_QSTATUS();
		pData.release();
	}
	
	return QSTATUS_SUCCESS;
}

QSTATUS qm::ToolSyncAction::isEnabled(const ActionEvent& event, bool* pbEnabled)
{
	assert(pbEnabled);
	*pbEnabled = pFolderModel_->getCurrentAccount() ||
		pFolderModel_->getCurrentFolder();
	return QSTATUS_SUCCESS;
}


/****************************************************************************
 *
 * ViewEncodingAction
 *
 */

qm::ViewEncodingAction::ViewEncodingAction(
	MessageWindow* pMessageWindow, QSTATUS* pstatus) :
	pMessageWindow_(pMessageWindow),
	pEncodingMenu_(0)
{
}

qm::ViewEncodingAction::ViewEncodingAction(MessageWindow* pMessageWindow,
	EncodingMenu* pEncodingMenu, QSTATUS* pstatus) :
	pMessageWindow_(pMessageWindow),
	pEncodingMenu_(pEncodingMenu)
{
}

qm::ViewEncodingAction::~ViewEncodingAction()
{
}

QSTATUS qm::ViewEncodingAction::invoke(const ActionEvent& event)
{
	const WCHAR* pwszEncoding = 0;
	if (pEncodingMenu_)
		pwszEncoding = pEncodingMenu_->getEncoding(event.getId());
	return pMessageWindow_->setEncoding(pwszEncoding);
}

QSTATUS qm::ViewEncodingAction::isChecked(const ActionEvent& event, bool* pbChecked)
{
	assert(pbChecked);
	
	if (pEncodingMenu_) {
		const WCHAR* pwszEncoding = pMessageWindow_->getEncoding();
		*pbChecked = pwszEncoding &&
			wcscmp(pwszEncoding, pEncodingMenu_->getEncoding(event.getId())) == 0;
	}
	else {
		*pbChecked = !pMessageWindow_->getEncoding();
	}
	
	return QSTATUS_SUCCESS;
}


/****************************************************************************
 *
 * ViewFilterAction
 *
 */

qm::ViewFilterAction::ViewFilterAction(ViewModelManager* pViewModelManager,
	FilterMenu* pFilterMenu, QSTATUS* pstatus) :
	pViewModelManager_(pViewModelManager),
	pFilterMenu_(pFilterMenu)
{
}

qm::ViewFilterAction::~ViewFilterAction()
{
}

QSTATUS qm::ViewFilterAction::invoke(const ActionEvent& event)
{
	DECLARE_QSTATUS();
	
	ViewModel* pViewModel = pViewModelManager_->getCurrentViewModel();
	if (pViewModel) {
		const Filter* pFilter = 0;
		status = pFilterMenu_->getFilter(event.getId(), &pFilter);
		CHECK_QSTATUS();
		status = pViewModel->setFilter(pFilter);
		CHECK_QSTATUS();
	}
	
	return QSTATUS_SUCCESS;
}

QSTATUS qm::ViewFilterAction::isEnabled(
	const ActionEvent& event, bool* pbEnabled)
{
	assert(pbEnabled);
	
	ViewModel* pViewModel = pViewModelManager_->getCurrentViewModel();
	*pbEnabled = pViewModel != 0;
	
	return QSTATUS_SUCCESS;
}

QSTATUS qm::ViewFilterAction::isChecked(
	const ActionEvent& event, bool* pbChecked)
{
	assert(pbChecked);
	
	DECLARE_QSTATUS();
	
	*pbChecked = false;
	
	ViewModel* pViewModel = pViewModelManager_->getCurrentViewModel();
	if (pViewModel) {
		const Filter* pFilter = 0;
		status = pFilterMenu_->getFilter(event.getId(), &pFilter);
		CHECK_QSTATUS();
		*pbChecked = pViewModel->getFilter() == pFilter;
	}
	
	return QSTATUS_SUCCESS;
}


/****************************************************************************
 *
 * ViewFilterCustomAction
 *
 */

qm::ViewFilterCustomAction::ViewFilterCustomAction(
	ViewModelManager* pViewModelManager, HWND hwndFrame, QSTATUS* pstatus) :
	pViewModelManager_(pViewModelManager),
	hwndFrame_(hwndFrame),
	pFilter_(0)
{
}

qm::ViewFilterCustomAction::~ViewFilterCustomAction()
{
	delete pFilter_;
}

QSTATUS qm::ViewFilterCustomAction::invoke(const ActionEvent& event)
{
	DECLARE_QSTATUS();
	
	ViewModel* pViewModel = pViewModelManager_->getCurrentViewModel();
	if (pViewModel) {
		string_ptr<WSTRING> wstrMacro;
		if (pFilter_) {
			status = pFilter_->getMacro()->getString(&wstrMacro);
			CHECK_QSTATUS();
		}
		CustomFilterDialog dialog(wstrMacro.get(), &status);
		CHECK_QSTATUS();
		int nRet = 0;
		status = dialog.doModal(hwndFrame_, 0, &nRet);
		CHECK_QSTATUS();
		if (nRet == IDOK) {
			std::auto_ptr<Filter> pFilter;
			status = newQsObject(L"", dialog.getMacro(), &pFilter);
			CHECK_QSTATUS();
			delete pFilter_;
			pFilter_ = pFilter.release();
			status = pViewModel->setFilter(pFilter_);
			CHECK_QSTATUS();
		}
	}
	
	return QSTATUS_SUCCESS;
}

QSTATUS qm::ViewFilterCustomAction::isEnabled(
	const ActionEvent& event, bool* pbEnabled)
{
	assert(pbEnabled);
	
	ViewModel* pViewModel = pViewModelManager_->getCurrentViewModel();
	*pbEnabled = pViewModel != 0;
	
	return QSTATUS_SUCCESS;
}

QSTATUS qm::ViewFilterCustomAction::isChecked(
	const ActionEvent& event, bool* pbChecked)
{
	assert(pbChecked);
	
	*pbChecked = false;
	
	ViewModel* pViewModel = pViewModelManager_->getCurrentViewModel();
	if (pViewModel && pFilter_)
		*pbChecked = pViewModel->getFilter() == pFilter_;
	
	return QSTATUS_SUCCESS;
}


/****************************************************************************
 *
 * ViewFilterNoneAction
 *
 */

qm::ViewFilterNoneAction::ViewFilterNoneAction(
	ViewModelManager* pViewModelManager, QSTATUS* pstatus) :
	pViewModelManager_(pViewModelManager)
{
}

qm::ViewFilterNoneAction::~ViewFilterNoneAction()
{
}

QSTATUS qm::ViewFilterNoneAction::invoke(const ActionEvent& event)
{
	DECLARE_QSTATUS();
	
	ViewModel* pViewModel = pViewModelManager_->getCurrentViewModel();
	if (pViewModel) {
		status = pViewModel->setFilter(0);
		CHECK_QSTATUS();
	}
	
	return QSTATUS_SUCCESS;
}

QSTATUS qm::ViewFilterNoneAction::isEnabled(
	const ActionEvent& event, bool* pbEnabled)
{
	assert(pbEnabled);
	
	ViewModel* pViewModel = pViewModelManager_->getCurrentViewModel();
	*pbEnabled = pViewModel != 0;
	
	return QSTATUS_SUCCESS;
}

QSTATUS qm::ViewFilterNoneAction::isChecked(
	const ActionEvent& event, bool* pbChecked)
{
	assert(pbChecked);
	
	*pbChecked = false;
	
	ViewModel* pViewModel = pViewModelManager_->getCurrentViewModel();
	if (pViewModel)
		*pbChecked = !pViewModel->getFilter();
	
	return QSTATUS_SUCCESS;
}


/****************************************************************************
 *
 * ViewFocusAction
 *
 */

qm::ViewFocusAction::ViewFocusAction(View* pViews[],
	size_t nViewCount, bool bNext, QSTATUS* pstatus) :
	bNext_(bNext)
{
	DECLARE_QSTATUS();
	
	status = STLWrapper<ViewList>(listView_).resize(nViewCount);
	CHECK_QSTATUS_SET(pstatus);
	std::copy(pViews, pViews + nViewCount, listView_.begin());
}

qm::ViewFocusAction::~ViewFocusAction()
{
}

QSTATUS qm::ViewFocusAction::invoke(const ActionEvent& event)
{
	DECLARE_QSTATUS();
	
	int nViewCount = listView_.size();
	
	int nView = 0;
	for (nView = 0; nView < nViewCount; ++nView) {
		if (listView_[nView]->isActive())
			break;
	}
	if (nView == nViewCount)
		return QSTATUS_SUCCESS;
	
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
	if (pView) {
		status = pView->setActive();
		CHECK_QSTATUS();
	}
	
	return QSTATUS_SUCCESS;
}


/****************************************************************************
 *
 * ViewLockPreviewAction
 *
 */

qm::ViewLockPreviewAction::ViewLockPreviewAction(
	PreviewMessageModel* pPreviewModel, QSTATUS* pstatus) :
	pPreviewModel_(pPreviewModel)
{
}

qm::ViewLockPreviewAction::~ViewLockPreviewAction()
{
}

QSTATUS qm::ViewLockPreviewAction::invoke(const ActionEvent& event)
{
	DECLARE_QSTATUS();
	
	if (pPreviewModel_->isConnectedToViewModel()) {
		status = pPreviewModel_->disconnectFromViewModel();
		CHECK_QSTATUS();
		status = pPreviewModel_->setMessage(0);
		CHECK_QSTATUS();
	}
	else {
		status = pPreviewModel_->connectToViewModel();
		CHECK_QSTATUS();
		status = pPreviewModel_->updateToViewModel();
		CHECK_QSTATUS();
	}
	
	return QSTATUS_SUCCESS;
}


/****************************************************************************
 *
 * ViewMessageModeAction
 *
 */

qm::ViewMessageModeAction::ViewMessageModeAction(MessageWindow* pMessageWindow,
	PFN_IS pfnIs, PFN_SET pfnSet, bool bEnabled, QSTATUS* pstatus) :
	pMessageWindow_(pMessageWindow),
	pfnIs_(pfnIs),
	pfnSet_(pfnSet),
	bEnabled_(bEnabled)
{
}

qm::ViewMessageModeAction::~ViewMessageModeAction()
{
}

QSTATUS qm::ViewMessageModeAction::invoke(const ActionEvent& event)
{
	return (pMessageWindow_->*pfnSet_)(!(pMessageWindow_->*pfnIs_)());
}

QSTATUS qm::ViewMessageModeAction::isEnabled(
	const ActionEvent& event, bool* pbEnabled)
{
	assert(pbEnabled);
	*pbEnabled = bEnabled_;
	return QSTATUS_SUCCESS;
}

QSTATUS qm::ViewMessageModeAction::isChecked(
	const ActionEvent& event, bool* pbChecked)
{
	assert(pbChecked);
	*pbChecked = (pMessageWindow_->*pfnIs_)();
	return QSTATUS_SUCCESS;
}


/****************************************************************************
 *
 * ViewNavigateFolderAction
 *
 */

qm::ViewNavigateFolderAction::ViewNavigateFolderAction(Document* pDocument,
	FolderModel* pFolderModel, Type type, QSTATUS* pstatus) :
	pDocument_(pDocument),
	pFolderModel_(pFolderModel),
	type_(type)
{
}

qm::ViewNavigateFolderAction::~ViewNavigateFolderAction()
{
}

QSTATUS qm::ViewNavigateFolderAction::invoke(const ActionEvent& event)
{
	DECLARE_QSTATUS();
	
	Account* pAccount = pFolderModel_->getCurrentAccount();
	Folder* pFolder = pFolderModel_->getCurrentFolder();
	if (!pAccount) {
		if (!pFolder)
			return QSTATUS_SUCCESS;
		pAccount = pFolder->getAccount();
	}
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
			const Document::AccountList& l = pDocument_->getAccounts();
			Document::AccountList::const_iterator it = std::find(
				l.begin(), l.end(), pAccount);
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
		if (pFolder) {
			status = pFolderModel_->setCurrent(0, pFolder, true);
			CHECK_QSTATUS();
		}
		break;
	case TYPE_NEXTACCOUNT:
	case TYPE_PREVACCOUNT:
		{
			const Document::AccountList& l = pDocument_->getAccounts();
			Document::AccountList::const_iterator it = std::find(
				l.begin(), l.end(), pAccount);
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
			if (it != l.end()) {
				status = pFolderModel_->setCurrent(*it, 0, true);
				CHECK_QSTATUS();
			}
		}
		break;
	default:
		break;
	}
	
	return QSTATUS_SUCCESS;
}

QSTATUS qm::ViewNavigateFolderAction::isEnabled(
	const ActionEvent& event, bool* pbEnabled)
{
	assert(pbEnabled);
	*pbEnabled = pFolderModel_->getCurrentAccount() ||
		pFolderModel_->getCurrentFolder();
	return QSTATUS_SUCCESS;
}


/****************************************************************************
 *
 * ViewNavigateMessageAction
 *
 */

qm::ViewNavigateMessageAction::ViewNavigateMessageAction(
	ViewModelManager* pViewModelManager, FolderModel* pFolderModel,
	MainWindow* pMainWindow, MessageWindow* pMessageWindow,
	Type type, QSTATUS* pstatus) :
	pViewModelManager_(pViewModelManager),
	pFolderModel_(pFolderModel),
	pViewModelHolder_(0),
	pMainWindow_(pMainWindow),
	pMessageWindow_(pMessageWindow),
	type_(type)
{
	assert(pViewModelManager);
	assert(pMessageWindow);
	assert(pstatus);
	*pstatus = QSTATUS_SUCCESS;
}

qm::ViewNavigateMessageAction::ViewNavigateMessageAction(
	ViewModelManager* pViewModelManager, ViewModelHolder* pViewModelHolder,
	MessageWindow* pMessageWindow, Type type, QSTATUS* pstatus) :
	pViewModelManager_(pViewModelManager),
	pFolderModel_(0),
	pViewModelHolder_(pViewModelHolder),
	pMainWindow_(0),
	pMessageWindow_(pMessageWindow),
	type_(type)
{
	assert(pViewModelManager);
	assert(pMessageWindow);
	assert(pstatus);
	*pstatus = QSTATUS_SUCCESS;
}

qm::ViewNavigateMessageAction::~ViewNavigateMessageAction()
{
}

QSTATUS qm::ViewNavigateMessageAction::invoke(const ActionEvent& event)
{
	DECLARE_QSTATUS();
	
	Type type = type_;
	bool bPreview = pFolderModel_ != 0;
	assert((bPreview && pFolderModel_ && pMainWindow_) ||
		(!bPreview && !pFolderModel_ && !pMainWindow_));
	
	MessageModel* pMessageModel = pMessageWindow_->getMessageModel();
	
	if (bPreview && (type == TYPE_NEXTPAGE || type == TYPE_PREVPAGE)) {
		MessagePtrLock lock(pMessageModel->getCurrentMessage());
		if (!lock)
			type = TYPE_SELF;
	}
	
	if (bPreview && !pMainWindow_->isShowPreviewWindow() &&
		(type == TYPE_NEXTPAGE || type == TYPE_NEXTPAGE || type == TYPE_SELF))
		return QSTATUS_SUCCESS;
	
	bool bScrolled = true;
	switch (type) {
	case TYPE_NEXTPAGE:
		status = pMessageWindow_->scrollPage(false, &bScrolled);
		CHECK_QSTATUS();
		if (bScrolled)
			return QSTATUS_SUCCESS;
		type = TYPE_NEXT;
		break;
	case TYPE_PREVPAGE:
		status = pMessageWindow_->scrollPage(true, &bScrolled);
		CHECK_QSTATUS();
		if (bScrolled)
			return QSTATUS_SUCCESS;
		type = TYPE_PREV;
		break;
	default:
		break;
	}
	
	ViewModel* pViewModel = 0;
	if (bPreview) {
		pViewModel = pViewModelManager_->getCurrentViewModel();
		if (!pViewModel)
			return QSTATUS_SUCCESS;
	}
	else {
		pViewModel = pViewModelHolder_->getViewModel();
	}
	
	Lock<ViewModel> lock(*pViewModel);
	
	unsigned int nCount = pViewModel->getCount();
	unsigned int nIndex = static_cast<unsigned int>(-1);
	if (bPreview) {
		if (nCount != 0)
			nIndex = pViewModel->getFocused();
	}
	else {
		MessagePtrLock mpl(pMessageModel->getCurrentMessage());
		if (mpl)
			nIndex = pViewModel->getIndex(mpl);
	}
	
	if (nIndex != static_cast<unsigned int>(-1) ||
		(bPreview && type == TYPE_NEXTUNSEEN)) {
		ViewModel* pNewViewModel = pViewModel;
		
		switch (type) {
		case TYPE_NEXT:
			if (nIndex == nCount - 1)
				nIndex = static_cast<unsigned int>(-1);
			else
				++nIndex;
			break;
		case TYPE_PREV:
			if (nIndex == 0)
				nIndex = static_cast<unsigned int>(-1);
			else
				--nIndex;
			break;
		case TYPE_NEXTUNSEEN:
			status = getNextUnseen(pViewModel, nIndex,
				false, &pNewViewModel, &nIndex);
			CHECK_QSTATUS();
			break;
		case TYPE_SELF:
			break;
		default:
			assert(false);
			break;
		}
		
		Lock<ViewModel> lockNew(*pNewViewModel);
		
		if (pNewViewModel != pViewModel) {
			if (bPreview) {
				status = pFolderModel_->setCurrent(0, 
					pNewViewModel->getFolder(), false);
				CHECK_QSTATUS();
			}
			else {
				status = pViewModelHolder_->setViewModel(pNewViewModel);
				CHECK_QSTATUS();
			}
			pViewModel = pNewViewModel;
		}
		
		if (!bPreview || type == TYPE_SELF) {
			MessageHolder* pmh = 0;
			if (nIndex != static_cast<unsigned int>(-1))
				pmh = pViewModel->getMessageHolder(nIndex);
			status = pMessageModel->setMessage(pmh);
			CHECK_QSTATUS();
		}
		
		if (nIndex != static_cast<unsigned int>(-1) && type != TYPE_SELF) {
			status = pViewModel->setFocused(nIndex);
			CHECK_QSTATUS();
			status = pViewModel->setSelection(nIndex);
			CHECK_QSTATUS();
			pViewModel->setLastSelection(nIndex);
			status = pViewModel->payAttention(nIndex);
			CHECK_QSTATUS();
		}
	}
	else {
		status = pMessageModel->setMessage(0);
		CHECK_QSTATUS();
	}
	
	return QSTATUS_SUCCESS;
}

QSTATUS qm::ViewNavigateMessageAction::isEnabled(
	const ActionEvent& event, bool* pbEnabled)
{
	assert(pbEnabled);
	// TODO
	*pbEnabled = true;
	return QSTATUS_SUCCESS;
}

QSTATUS qm::ViewNavigateMessageAction::getNextUnseen(
	ViewModel* pViewModel, unsigned int nIndex, bool bIncludeSelf,
	ViewModel** ppViewModel, unsigned int* pnIndex) const
{
	assert(ppViewModel);
	assert(pnIndex);
	
	DECLARE_QSTATUS();
	
	*ppViewModel = pViewModel;
	*pnIndex = -1;
	
	Lock<ViewModel> lock(*pViewModel);
	
	unsigned int nStart = nIndex;
	if (nIndex != -1) {
		unsigned int nCount = pViewModel->getCount();
		for (bIncludeSelf ? 0 : ++nIndex; nIndex < nCount; ++nIndex) {
			MessageHolder* pmh = pViewModel->getMessageHolder(nIndex);
			if (!pmh->isFlag(MessageHolder::FLAG_SEEN))
				break;
		}
		if (nIndex == nCount) {
			for (nIndex = 0; nIndex < nStart; ++nIndex) {
				MessageHolder* pmh = pViewModel->getMessageHolder(nIndex);
				if (!pmh->isFlag(MessageHolder::FLAG_SEEN))
					break;
			}
		}
	}
	if (!bIncludeSelf && nIndex == nStart) {
		Folder* pFolder = pViewModel->getFolder();
		Account* pAccount = pFolder->getAccount();
		const Account::FolderList& l = pAccount->getFolders();
		Account::FolderList listFolder;
		status = STLWrapper<Account::FolderList>(listFolder).resize(l.size());
		CHECK_QSTATUS();
		std::copy(l.begin(), l.end(), listFolder.begin());
		std::sort(listFolder.begin(), listFolder.end(), FolderLess());
		
		Account::FolderList::const_iterator itThis = std::find(
			listFolder.begin(), listFolder.end(), pFolder);
		assert(itThis != listFolder.end());
		
		Folder* pUnseenFolder = 0;
		Account::FolderList::const_iterator it = itThis;
		for (++it; it != listFolder.end() && !pUnseenFolder; ++it) {
			if (!(*it)->isFlag(Folder::FLAG_TRASHBOX) &&
				!(*it)->isFlag(Folder::FLAG_IGNOREUNSEEN) &&
				!(*it)->isHidden() &&
				(*it)->getUnseenCount() != 0)
				pUnseenFolder = *it;
		}
		if (!pUnseenFolder) {
			for (it = listFolder.begin(); it != itThis && !pUnseenFolder; ++it) {
				if (!(*it)->isFlag(Folder::FLAG_TRASHBOX) &&
					!(*it)->isFlag(Folder::FLAG_IGNOREUNSEEN) &&
					!(*it)->isHidden() &&
					(*it)->getUnseenCount() != 0)
					pUnseenFolder = *it;
			}
		}
		
		if (pUnseenFolder) {
			status = pViewModelManager_->getViewModel(
				pUnseenFolder, &pViewModel);
			CHECK_QSTATUS();
			status = getNextUnseen(pViewModel, pViewModel->getFocused(),
				true, ppViewModel, pnIndex);
			CHECK_QSTATUS();
		}
	}
	else {
		*pnIndex = nIndex;
	}
	
	return QSTATUS_SUCCESS;
}


/****************************************************************************
 *
 * ViewOpenLinkAction
 *
 */

qm::ViewOpenLinkAction::ViewOpenLinkAction(
	MessageWindow* pMessageWindow, QSTATUS* pstatus) :
	pMessageWindow_(pMessageWindow)
{
}

qm::ViewOpenLinkAction::~ViewOpenLinkAction()
{
}

QSTATUS qm::ViewOpenLinkAction::invoke(const ActionEvent& event)
{
	return pMessageWindow_->openLink();
}


/****************************************************************************
 *
 * ViewRefreshAction
 *
 */

qm::ViewRefreshAction::ViewRefreshAction(SyncManager* pSyncManager,
	Document* pDocument, FolderModel* pFolderModel,
	SyncDialogManager* pSyncDialogManager, HWND hwnd,
	Profile* pProfile, QSTATUS* pstatus) :
	pSyncManager_(pSyncManager),
	pDocument_(pDocument),
	pFolderModel_(pFolderModel),
	pSyncDialogManager_(pSyncDialogManager),
	hwnd_(hwnd),
	pProfile_(pProfile)
{
	assert(pstatus);
	*pstatus = QSTATUS_SUCCESS;
}

qm::ViewRefreshAction::~ViewRefreshAction()
{
}

QSTATUS qm::ViewRefreshAction::invoke(const ActionEvent& event)
{
	DECLARE_QSTATUS();
	
	Folder* pFolder = pFolderModel_->getCurrentFolder();
	if (pFolder) {
		switch (pFolder->getType()) {
		case Folder::TYPE_NORMAL:
			if (pFolder->isFlag(Folder::FLAG_SYNCABLE)) {
				status = SyncUtil::syncFolder(pSyncManager_, pDocument_,
				pSyncDialogManager_, hwnd_, SyncDialog::FLAG_NONE,
				static_cast<NormalFolder*>(pFolder));
				CHECK_QSTATUS();
			}
			break;
		case Folder::TYPE_QUERY:
			status = static_cast<QueryFolder*>(pFolder)->search(
				pDocument_, hwnd_, pProfile_);
			CHECK_QSTATUS();
			break;
		default:
			assert(false);
			break;
		}
	}
	
	return QSTATUS_SUCCESS;
}

QSTATUS qm::ViewRefreshAction::isEnabled(const ActionEvent& event, bool* pbEnabled)
{
	assert(pbEnabled);
	Folder* pFolder = pFolderModel_->getCurrentFolder();
	*pbEnabled = pFolder &&
		(pFolder->getType() == Folder::TYPE_QUERY ||
		pFolder->isFlag(Folder::FLAG_SYNCABLE));
	return QSTATUS_SUCCESS;
}


/****************************************************************************
 *
 * ViewScrollAction
 *
 */

qm::ViewScrollAction::ViewScrollAction(HWND hwnd,
	Scroll scroll, QSTATUS* pstatus) :
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
	if (n == countof(requests))
		*pstatus = QSTATUS_FAIL;
}

qm::ViewScrollAction::~ViewScrollAction()
{
}

QSTATUS qm::ViewScrollAction::invoke(const ActionEvent& event)
{
	Window(hwnd_).sendMessage(nMsg_, MAKEWPARAM(nRequest_, 0));
	return QSTATUS_SUCCESS;
}


/****************************************************************************
 *
 * ViewSelectModeAction
 *
 */

qm::ViewSelectModeAction::ViewSelectModeAction(
	MessageWindow* pMessageWindow, QSTATUS* pstatus) :
	pMessageWindow_(pMessageWindow)
{
}

qm::ViewSelectModeAction::~ViewSelectModeAction()
{
}

QSTATUS qm::ViewSelectModeAction::invoke(const ActionEvent& event)
{
	return pMessageWindow_->setSelectMode(!pMessageWindow_->isSelectMode());
}

QSTATUS qm::ViewSelectModeAction::isChecked(
	const ActionEvent& event, bool* pbChecked)
{
	assert(pbChecked);
	*pbChecked = pMessageWindow_->isSelectMode();
	return QSTATUS_SUCCESS;
}


/****************************************************************************
 *
 * ViewShowFolderAction
 *
 */

qm::ViewShowFolderAction::ViewShowFolderAction(
	MainWindow* pMainWindow, QSTATUS* pstatus) :
	ViewShowControlAction<MainWindow>(pMainWindow,
		&qm::MainWindow::setShowFolderWindow,
		&qm::MainWindow::isShowFolderWindow,
		IDS_SHOWFOLDER, IDS_HIDEFOLDER, pstatus)
{
	assert(pstatus);
	*pstatus = QSTATUS_SUCCESS;
}

qm::ViewShowFolderAction::~ViewShowFolderAction()
{
}


/****************************************************************************
 *
 * ViewShowHeaderAction
 *
 */

qm::ViewShowHeaderAction::ViewShowHeaderAction(
	MessageWindow* pMessageWindow, QSTATUS* pstatus) :
	ViewShowControlAction<MessageWindow>(pMessageWindow,
		&qm::MessageWindow::setShowHeaderWindow,
		&qm::MessageWindow::isShowHeaderWindow,
		IDS_SHOWHEADER, IDS_HIDEHEADER, pstatus)
{
	assert(pstatus);
	*pstatus = QSTATUS_SUCCESS;
}

qm::ViewShowHeaderAction::~ViewShowHeaderAction()
{
}


/****************************************************************************
 *
 * ViewShowHeaderColumnAction
 *
 */

qm::ViewShowHeaderColumnAction::ViewShowHeaderColumnAction(
	ListWindow* pListWindow, QSTATUS* pstatus) :
	ViewShowControlAction<ListWindow>(pListWindow,
		&qm::ListWindow::setShowHeaderColumn,
		&qm::ListWindow::isShowHeaderColumn,
		IDS_SHOWHEADERCOLUMN, IDS_HIDEHEADERCOLUMN, pstatus)
{
	assert(pstatus);
	*pstatus = QSTATUS_SUCCESS;
}

qm::ViewShowHeaderColumnAction::~ViewShowHeaderColumnAction()
{
}


/****************************************************************************
 *
 * ViewShowPreviewAction
 *
 */

qm::ViewShowPreviewAction::ViewShowPreviewAction(
	MainWindow* pMainWindow, QSTATUS* pstatus) :
	ViewShowControlAction<MainWindow>(pMainWindow,
		&qm::MainWindow::setShowPreviewWindow,
		&qm::MainWindow::isShowPreviewWindow,
		IDS_SHOWPREVIEW, IDS_HIDEPREVIEW, pstatus)
{
	assert(pstatus);
	*pstatus = QSTATUS_SUCCESS;
}

qm::ViewShowPreviewAction::~ViewShowPreviewAction()
{
}


/****************************************************************************
 *
 * ViewShowSyncDialogAction
 *
 */

qm::ViewShowSyncDialogAction::ViewShowSyncDialogAction(
	SyncDialogManager* pManager, QSTATUS* pstatus) :
	pManager_(pManager)
{
}

qm::ViewShowSyncDialogAction::~ViewShowSyncDialogAction()
{
}

QSTATUS qm::ViewShowSyncDialogAction::invoke(const ActionEvent& event)
{
	DECLARE_QSTATUS();
	
	SyncDialog* pDialog = 0;
	status = pManager_->open(&pDialog);
	CHECK_QSTATUS();
	pDialog->show();
	
	return QSTATUS_SUCCESS;
}


/****************************************************************************
 *
 * ViewSortAction
 *
 */

qm::ViewSortAction::ViewSortAction(ViewModelManager* pViewModelManager,
	SortMenu* pSortMenu, QSTATUS* pstatus) :
	pViewModelManager_(pViewModelManager),
	pSortMenu_(pSortMenu)
{
	assert(pstatus);
	*pstatus = QSTATUS_SUCCESS;
}

qm::ViewSortAction::~ViewSortAction()
{
}

QSTATUS qm::ViewSortAction::invoke(const ActionEvent& event)
{
	DECLARE_QSTATUS();
	
	ViewModel* pViewModel = pViewModelManager_->getCurrentViewModel();
	if (pViewModel) {
		status = pViewModel->setSort(pSortMenu_->getSort(event.getId()));
		CHECK_QSTATUS();
	}
	
	return QSTATUS_SUCCESS;
}

QSTATUS qm::ViewSortAction::isEnabled(const ActionEvent& event, bool* pbEnabled)
{
	assert(pbEnabled);
	
	ViewModel* pViewModel = pViewModelManager_->getCurrentViewModel();
	*pbEnabled = pViewModel != 0;
	
	return QSTATUS_SUCCESS;
}

QSTATUS qm::ViewSortAction::isChecked(const ActionEvent& event, bool* pbChecked)
{
	assert(pbChecked);
	
	ViewModel* pViewModel = pViewModelManager_->getCurrentViewModel();
	if (pViewModel)
		*pbChecked = (pSortMenu_->getSort(event.getId()) & ViewModel::SORT_INDEX_MASK) ==
			(pViewModel->getSort() & ViewModel::SORT_INDEX_MASK);
	
	return QSTATUS_SUCCESS;
}


/****************************************************************************
 *
 * ViewSortDirectionAction
 *
 */

qm::ViewSortDirectionAction::ViewSortDirectionAction(
	ViewModelManager* pViewModelManager, bool bAscending, QSTATUS* pstatus) :
	pViewModelManager_(pViewModelManager),
	bAscending_(bAscending)
{
	assert(pstatus);
	*pstatus = QSTATUS_SUCCESS;
}

qm::ViewSortDirectionAction::~ViewSortDirectionAction()
{
}

QSTATUS qm::ViewSortDirectionAction::invoke(const ActionEvent& event)
{
	DECLARE_QSTATUS();
	
	ViewModel* pViewModel = pViewModelManager_->getCurrentViewModel();
	if (pViewModel) {
		unsigned int nSort = pViewModel->getSort();
		nSort &= ~ViewModel::SORT_DIRECTION_MASK;
		nSort |= bAscending_ ? ViewModel::SORT_ASCENDING : ViewModel::SORT_DESCENDING;
		status = pViewModel->setSort(nSort);
		CHECK_QSTATUS();
	}
	
	return QSTATUS_SUCCESS;
}

QSTATUS qm::ViewSortDirectionAction::isEnabled(const ActionEvent& event, bool* pbEnabled)
{
	assert(pbEnabled);
	
	ViewModel* pViewModel = pViewModelManager_->getCurrentViewModel();
	*pbEnabled = pViewModel != 0;
	
	return QSTATUS_SUCCESS;
}

QSTATUS qm::ViewSortDirectionAction::isChecked(const ActionEvent& event, bool* pbChecked)
{
	assert(pbChecked);
	
	ViewModel* pViewModel = pViewModelManager_->getCurrentViewModel();
	if (pViewModel) {
		unsigned int nSort = pViewModel->getSort();
		*pbChecked =
			(bAscending_ &&
				(nSort & ViewModel::SORT_DIRECTION_MASK) ==
					ViewModel::SORT_ASCENDING) ||
			(!bAscending_ &&
				(nSort & ViewModel::SORT_DIRECTION_MASK) ==
					ViewModel::SORT_DESCENDING);
	}
	else {
		*pbChecked = false;
	}
	return QSTATUS_SUCCESS;
}


/****************************************************************************
 *
 * ViewSortThreadAction
 *
 */

qm::ViewSortThreadAction::ViewSortThreadAction(
	ViewModelManager* pViewModelManager, QSTATUS* pstatus) :
	pViewModelManager_(pViewModelManager)
{
	assert(pstatus);
	*pstatus = QSTATUS_SUCCESS;
}

qm::ViewSortThreadAction::~ViewSortThreadAction()
{
}

QSTATUS qm::ViewSortThreadAction::invoke(const ActionEvent& event)
{
	DECLARE_QSTATUS();
	
	ViewModel* pViewModel = pViewModelManager_->getCurrentViewModel();
	if (pViewModel) {
		unsigned int nSort = pViewModel->getSort();
		bool bThread = (nSort & ViewModel::SORT_THREAD_MASK) == ViewModel::SORT_THREAD;
		nSort &= ~ViewModel::SORT_THREAD_MASK;
		nSort |= bThread ? ViewModel::SORT_NOTHREAD : ViewModel::SORT_THREAD;
		status = pViewModel->setSort(nSort);
		CHECK_QSTATUS();
	}
	
	return QSTATUS_SUCCESS;
}

QSTATUS qm::ViewSortThreadAction::isEnabled(const ActionEvent& event, bool* pbEnabled)
{
	assert(pbEnabled);
	
	ViewModel* pViewModel = pViewModelManager_->getCurrentViewModel();
	*pbEnabled = pViewModel != 0;
	
	return QSTATUS_SUCCESS;
}

QSTATUS qm::ViewSortThreadAction::isChecked(const ActionEvent& event, bool* pbChecked)
{
	assert(pbChecked);
	
	ViewModel* pViewModel = pViewModelManager_->getCurrentViewModel();
	*pbChecked = pViewModel &&
		(pViewModel->getSort() & ViewModel::SORT_THREAD_MASK) ==
			ViewModel::SORT_THREAD;
	
	return QSTATUS_SUCCESS;
}


/****************************************************************************
 *
 * ViewTemplateAction
 *
 */

qm::ViewTemplateAction::ViewTemplateAction(
	MessageWindow* pMessageWindow, QSTATUS* pstatus) :
	pMessageWindow_(pMessageWindow),
	pTemplateMenu_(0)
{
}

qm::ViewTemplateAction::ViewTemplateAction(MessageWindow* pMessageWindow,
	TemplateMenu* pTemplateMenu, QSTATUS* pstatus) :
	pMessageWindow_(pMessageWindow),
	pTemplateMenu_(pTemplateMenu)
{
}

qm::ViewTemplateAction::~ViewTemplateAction()
{
}

QSTATUS qm::ViewTemplateAction::invoke(const ActionEvent& event)
{
	const WCHAR* pwszTemplate = 0;
	if (pTemplateMenu_)
		pwszTemplate = pTemplateMenu_->getTemplate(event.getId());
	return pMessageWindow_->setTemplate(pwszTemplate);
}

QSTATUS qm::ViewTemplateAction::isChecked(
	const ActionEvent& event, bool* pbChecked)
{
	assert(pbChecked);
	
	if (pTemplateMenu_) {
		const WCHAR* pwszTemplate = pMessageWindow_->getTemplate();
		*pbChecked = pwszTemplate &&
			wcscmp(pwszTemplate, pTemplateMenu_->getTemplate(event.getId())) == 0;
	}
	else {
		*pbChecked = !pMessageWindow_->getTemplate();
	}
	return QSTATUS_SUCCESS;
}
