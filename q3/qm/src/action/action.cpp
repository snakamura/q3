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
#include <qmgoround.h>
#include <qmlistwindow.h>
#include <qmmainwindow.h>
#include <qmmessage.h>
#include <qmmessageholder.h>
#include <qmmessagewindow.h>

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
#include "../ui/resource.h"
#include "../ui/syncdialog.h"
#include "../ui/syncutil.h"
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
		status = parser.getAttachments(&listAttachment);
		CHECK_QSTATUS();
		
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
	
	MessagePtrList listMessagePtr;
	status = STLWrapper<MessagePtrList>(listMessagePtr).push_back(
		pMessageModel_->getCurrentMessage());
	CHECK_QSTATUS();
	
	if (bAll_) {
		status = helper_.detach(listMessagePtr, 0);
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
		
		status = helper_.detach(listMessagePtr, &l);
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
	
	status = static_cast<NormalFolder*>(pFolder)->clearDeletedMessages();
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

qm::EditCopyMessageAction::EditCopyMessageAction(FolderModel* pFolderModel,
	MessageSelectionModel* pMessageSelectionModel, QSTATUS* pstatus) :
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
	
	Account* pAccount = pFolderModel_->getCurrentAccount();
	if (!pAccount)
		pAccount = pFolderModel_->getCurrentFolder()->getAccount();
	
	MessagePtrList l;
	status = pMessageSelectionModel_->getSelectedMessages(0, &l);
	CHECK_QSTATUS();
	
	if (!l.empty()) {
		MessageDataObject* p = 0;
		status = newQsObject(pAccount, l, MessageDataObject::FLAG_COPY, &p);
		CHECK_QSTATUS();
		p->AddRef();
		ComPtr<IDataObject> pDataObject(p);
		
#ifndef _WIN32_WCE
		HRESULT hr = ::OleSetClipboard(pDataObject.get());
		if (hr != S_OK) {
			// TODO
		}
#endif
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

qm::EditCutMessageAction::EditCutMessageAction(FolderModel* pFolderModel,
	MessageSelectionModel* pMessageSelectionModel, QSTATUS* pstatus) :
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
	
	Account* pAccount = pFolderModel_->getCurrentAccount();
	if (!pAccount)
		pAccount = pFolderModel_->getCurrentFolder()->getAccount();
	
	MessagePtrList l;
	status = pMessageSelectionModel_->getSelectedMessages(0, &l);
	CHECK_QSTATUS();
	
	if (!l.empty()) {
		MessageDataObject* p = 0;
		status = newQsObject(pAccount, l, MessageDataObject::FLAG_MOVE, &p);
		CHECK_QSTATUS();
		p->AddRef();
		ComPtr<IDataObject> pDataObject(p);
		
#ifndef _WIN32_WCE
		HRESULT hr = ::OleSetClipboard(pDataObject.get());
		if (hr != S_OK) {
			// TODO
		}
#endif
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
 * EditDeleteMessageAction
 *
 */

qm::EditDeleteMessageAction::EditDeleteMessageAction(
	MessageSelectionModel* pModel, bool bDirect, QSTATUS* pstatus) :
	pMessageSelectionModel_(pModel),
	pMessageModel_(0),
	bDirect_(bDirect)
{
	assert(pstatus);
	*pstatus = QSTATUS_SUCCESS;
}

qm::EditDeleteMessageAction::EditDeleteMessageAction(
	MessageModel* pModel, bool bDirect, QSTATUS* pstatus) :
	pMessageSelectionModel_(0),
	pMessageModel_(pModel),
	bDirect_(bDirect)
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
		Folder* pFolder = 0;
		MessagePtrList l;
		status = pMessageSelectionModel_->getSelectedMessages(&pFolder, &l);
		CHECK_QSTATUS();
		if (!l.empty()) {
			status = pFolder->removeMessages(l, bDirect_);
			CHECK_QSTATUS();
		}
	}
	else {
		ViewModel* pViewModel = pMessageModel_->getViewModel();
		
		Lock<ViewModel> lock(*pViewModel);
		
		MessageHolder* pmh = 0;
		
		MessagePtrLock mpl(pMessageModel_->getCurrentMessage());
		if (mpl) {
			unsigned int nIndex = pViewModel->getIndex(mpl);
			if (nIndex < pViewModel->getCount() - 1)
				pmh = pViewModel->getMessageHolder(nIndex + 1);
			
			Folder::MessageHolderList l;
			status = STLWrapper<Folder::MessageHolderList>(l).push_back(mpl);
			CHECK_QSTATUS();
			status = mpl->getFolder()->removeMessages(l, bDirect_);
			CHECK_QSTATUS();
		}
		
		status = pMessageModel_->setMessage(pmh);
		CHECK_QSTATUS();
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
		FindDialog dialog(pProfile_, &status);
		CHECK_QSTATUS();
		int nRet = 0;
		status = dialog.doModal(hwndFrame, 0, &nRet);
		CHECK_QSTATUS();
		if (nRet == IDOK) {
			status = pFindReplaceManager_->setData(dialog.getFind(),
				dialog.isMatchCase() ? FindReplaceData::FLAG_MATCHCASE : 0);
			CHECK_QSTATUS();
			
			status = pMessageWindow_->find(dialog.getFind(),
				dialog.isMatchCase(), dialog.isPrev(), &bFound);
			CHECK_QSTATUS();
		}
	}
	else {
		const FindReplaceData* pData = pFindReplaceManager_->getData();
		assert(pData);
		status = pMessageWindow_->find(pData->getFind(),
			(pData->getFlags() & FindReplaceData::FLAG_MATCHCASE) != 0,
			type_ == TYPE_PREV, &bFound);
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

qm::EditPasteMessageAction::EditPasteMessageAction(
	Document* pDocument, FolderModel* pModel, QSTATUS* pstatus) :
	pDocument_(pDocument),
	pModel_(pModel)
{
}

qm::EditPasteMessageAction::~EditPasteMessageAction()
{
}

QSTATUS qm::EditPasteMessageAction::invoke(const ActionEvent& event)
{
	DECLARE_QSTATUS();
	
#ifndef _WIN32_WCE
	Folder* pFolder = pModel_->getCurrentFolder();
	if (pFolder && pFolder->getType() == Folder::TYPE_NORMAL &&
		!pFolder->isFlag(Folder::FLAG_NOSELECT)) {
		ComPtr<IDataObject> pDataObject;
		HRESULT hr = ::OleGetClipboard(&pDataObject);
		if (hr == S_OK) {
			status = MessageDataObject::pasteMessages(pDataObject.get(),
				pDocument_, static_cast<NormalFolder*>(pFolder),
				MessageDataObject::FLAG_NONE, 0);
			CHECK_QSTATUS();
			::OleSetClipboard(0);
		}
	}
#endif
	
	return QSTATUS_SUCCESS;
}

QSTATUS qm::EditPasteMessageAction::isEnabled(
	const ActionEvent& event, bool* pbEnabled)
{
	assert(pbEnabled);
	
	*pbEnabled = false;
	
#ifndef _WIN32_WCE
	Folder* pFolder = pModel_->getCurrentFolder();
	if (pFolder && pFolder->getType() == Folder::TYPE_NORMAL &&
		!pFolder->isFlag(Folder::FLAG_NOSELECT)) {
		ComPtr<IDataObject> pDataObject;
		HRESULT hr = ::OleGetClipboard(&pDataObject);
		if (hr == S_OK)
			*pbEnabled = MessageDataObject::canPasteMessage(pDataObject.get());
	}
#endif
	
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
 * FileEmptyTrashAction
 *
 */

qm::FileEmptyTrashAction::FileEmptyTrashAction(
	FolderModel* pFolderModel, QSTATUS* pstatus) :
	pFolderModel_(pFolderModel)
{
}

qm::FileEmptyTrashAction::~FileEmptyTrashAction()
{
}

QSTATUS qm::FileEmptyTrashAction::invoke(const ActionEvent& event)
{
	DECLARE_QSTATUS();
	
	NormalFolder* pTrash = getTrash();
	if (pTrash) {
		// TODO
		// Sync folder if online and trash is syncable
		
		status = pTrash->removeAllMessages();
		CHECK_QSTATUS();
		
		if (!pTrash->isFlag(Folder::FLAG_LOCAL)) {
			status = pTrash->clearDeletedMessages();
			CHECK_QSTATUS();
		}
		
		status = pTrash->getAccount()->save();
		CHECK_QSTATUS();
	}
	
	return QSTATUS_SUCCESS;
}

QSTATUS qm::FileEmptyTrashAction::isEnabled(
	const ActionEvent& event, bool* pbEnabled)
{
	assert(pbEnabled);
	*pbEnabled = getTrash() != 0;
	return QSTATUS_SUCCESS;
}

NormalFolder* qm::FileEmptyTrashAction::getTrash() const
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
 * FileExitAction
 *
 */

qm::FileExitAction::FileExitAction(Window* pWindow, Document* pDocument,
	SyncManager* pSyncManager, TempFileCleaner* pTempFileCleaner,
	EditFrameWindowManager* pEditFrameWindowManager, QSTATUS* pstatus) :
	pWindow_(pWindow),
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

QSTATUS qm::FileExitAction::invoke(const ActionEvent& event)
{
	DECLARE_QSTATUS();
	
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
	
	pWindow_->destroyWindow();
	
	return QSTATUS_SUCCESS;
}


/****************************************************************************
 *
 * FileExportAction
 *
 */

qm::FileExportAction::FileExportAction(
	MessageSelectionModel* pModel, QSTATUS* pstatus) :
	pModel_(pModel)
{
}

qm::FileExportAction::~FileExportAction()
{
}

QSTATUS qm::FileExportAction::invoke(const ActionEvent& event)
{
	DECLARE_QSTATUS();
	
	MessagePtrList l;
	status = pModel_->getSelectedMessages(0, &l);
	CHECK_QSTATUS();
	
	if (!l.empty()) {
		ExportDialog dialog(l.size() == 1, &status);
		CHECK_QSTATUS();
		int nRet = 0;
		status = dialog.doModal(getMainWindow()->getHandle(), 0, &nRet);
		CHECK_QSTATUS();
		if (nRet == IDOK) {
			const Template* pTemplate = 0;
			const WCHAR* pwszEncoding = 0;
			const WCHAR* pwszTemplate = dialog.getTemplate();
			if (pwszTemplate) {
				// TODO
				// Get template and encoding
			}
			
			if (dialog.isFilePerMessage()) {
				const WCHAR* pwszPath = dialog.getPath();
				const WCHAR* pFileName = wcsrchr(pwszPath, L'\\');
				pFileName = pFileName ? pFileName + 1 : pwszPath;
				const WCHAR* pExt = wcsrchr(pFileName, L'.');
				if (!pExt)
					pExt = pFileName + wcslen(pFileName);
				
				MessagePtrList::size_type n = 0;
				while (n < l.size()) {
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
			}
			else {
				FileOutputStream fileStream(dialog.getPath(), &status);
				CHECK_QSTATUS();
				BufferedOutputStream stream(&fileStream, false, &status);
				CHECK_QSTATUS();
				
				if (l.size() == 1) {
					status = writeMessage(&stream, l.front(),
						dialog.isExportFlags(), pTemplate, pwszEncoding, false);
					CHECK_QSTATUS();
				}
				else {
					MessagePtrList::iterator it = l.begin();
					while (it != l.end()) {
						status = writeMessage(&stream, *it,
							dialog.isExportFlags(), pTemplate, pwszEncoding, true);
						CHECK_QSTATUS();
						++it;
					}
				}
				
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

qm::FileImportAction::FileImportAction(
	FolderModel* pFolderModel, QSTATUS* pstatus) :
	pFolderModel_(pFolderModel)
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
		status = dialog.doModal(getMainWindow()->getHandle(), 0, &nRet);
		CHECK_QSTATUS();
		if (nRet == IDOK) {
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
				
				status = readMessage(static_cast<NormalFolder*>(pFolder),
					&stream, dialog.isMultiple(), dialog.getFlags());
				CHECK_QSTATUS();
				
				if (!pEnd)
					break;
				pBegin = pEnd + 1;
				if (!*pBegin)
					break;
			}
		}
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
	InputStream* pStream, bool bMultiple, unsigned int nFlags)
{
	assert(pFolder);
	assert(pStream);
	
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

qm::FileOfflineAction::FileOfflineAction(Document* pDocument, QSTATUS* pstatus) :
	pDocument_(pDocument)
{
}

qm::FileOfflineAction::~FileOfflineAction()
{
}

QSTATUS qm::FileOfflineAction::invoke(const ActionEvent& event)
{
	return pDocument_->setOffline(!pDocument_->isOffline());
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

qm::FilePrintAction::FilePrintAction(
	MessageSelectionModel* pModel, QSTATUS* pstatus) :
	pModel_(pModel)
{
}

qm::FilePrintAction::~FilePrintAction()
{
}

QSTATUS qm::FilePrintAction::invoke(const ActionEvent& event)
{
	// TODO
	return QSTATUS_SUCCESS;
}

QSTATUS qm::FilePrintAction::isEnabled(const ActionEvent& event, bool* pbEnabled)
{
	assert(pbEnabled);
	// TODO
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
 * FolderCompactAction
 *
 */

qm::FolderCompactAction::FolderCompactAction(
	FolderModel* pFolderModel, QSTATUS* pstatus) :
	pFolderModel_(pFolderModel)
{
}

qm::FolderCompactAction::~FolderCompactAction()
{
}

QSTATUS qm::FolderCompactAction::invoke(const ActionEvent& event)
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

QSTATUS qm::FolderCompactAction::isEnabled(const ActionEvent& event, bool* pbEnabled)
{
	assert(pbEnabled);
	*pbEnabled = pFolderModel_->getCurrentAccount() ||
		pFolderModel_->getCurrentFolder();
	return QSTATUS_SUCCESS;
}


/****************************************************************************
 *
 * FolderCreateAction
 *
 */

qm::FolderCreateAction::FolderCreateAction(
	FolderSelectionModel* pFolderSelectionModel, QSTATUS* pstatus) :
	pFolderSelectionModel_(pFolderSelectionModel)
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
	bool bAllowRemote = pAccount->isSupport(Account::SUPPORT_REMOTEFOLDER);
	
	if (pFolder) {
		switch (pFolder->getType()) {
		case Folder::TYPE_NORMAL:
			if (pFolder->isFlag(Folder::FLAG_LOCAL)) {
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
	status = dialog.doModal(getMainWindow()->getHandle(), 0, &nRet);
	CHECK_QSTATUS();
	if (nRet == IDOK) {
		switch (dialog.getType()) {
		case CreateFolderDialog::TYPE_LOCALFOLDER:
			status = pAccount->createNormalFolder(
				dialog.getName(), pFolder, false);
			break;
		case CreateFolderDialog::TYPE_REMOTEFOLDER:
			status = pAccount->createNormalFolder(
				dialog.getName(), pFolder, true);
			break;
		case CreateFolderDialog::TYPE_QUERYFOLDER:
			status = pAccount->createQueryFolder(
				dialog.getName(), pFolder, dialog.getMacro());
			break;
		default:
			assert(false);
			break;
		}
		if (status != QSTATUS_SUCCESS)
			messageBox(Application::getApplication().getResourceHandle(),
				IDS_ERROR_CREATEFOLDER, MB_OK | MB_ICONERROR);
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

qm::FolderDeleteAction::FolderDeleteAction(
	FolderSelectionModel* pFolderSelectionModel, QSTATUS* pstatus) :
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
	// TODO
	return QSTATUS_SUCCESS;
}

QSTATUS qm::FolderDeleteAction::isEnabled(const ActionEvent& event, bool* pbEnabled)
{
	assert(pbEnabled);
	return pFolderSelectionModel_->hasSelectedFolder(pbEnabled);
}


/****************************************************************************
 *
 * FolderPropertyAction
 *
 */

qm::FolderPropertyAction::FolderPropertyAction(
	FolderSelectionModel* pModel, HWND hwnd, QSTATUS* pstatus) :
	pModel_(pModel),
	hwnd_(hwnd)
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
	
	HINSTANCE hInst = Application::getApplication().getResourceHandle();
	string_ptr<WSTRING> wstrTitle;
	status = loadString(hInst, IDS_PROPERTY, &wstrTitle);
	CHECK_QSTATUS();
	
	FolderPropertyPage page(listFolder, &status);
	CHECK_QSTATUS();
	PropertySheetBase sheet(hInst, wstrTitle.get(), false, &status);
	CHECK_QSTATUS();
	status = sheet.add(&page);
	CHECK_QSTATUS();
	
	int nRet = 0;
	status = sheet.doModal(hwnd_, 0, &nRet);
	CHECK_QSTATUS();
	if (nRet == IDOK) {
		Account::FolderList::const_iterator it = listFolder.begin();
		while (it != listFolder.end()) {
			(*it)->setFlags(page.getFlags(), page.getMask());
			++it;
		}
	}
	
	return QSTATUS_SUCCESS;
}

QSTATUS qm::FolderPropertyAction::isEnabled(const ActionEvent& event, bool* pbEnabled)
{
	assert(pbEnabled);
	return pModel_->hasSelectedFolder(pbEnabled);
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

qm::MessageApplyRuleAction::MessageApplyRuleAction(
	RuleManager* pRuleManager, FolderModel* pFolderModel,
	Document* pDocument, HWND hwnd, Profile* pProfile, QSTATUS* pstatus) :
	pRuleManager_(pRuleManager),
	pFolderModel_(pFolderModel),
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
	
	Folder* pFolder = pFolderModel_->getCurrentFolder();
	if (pFolder && pFolder->getType() == Folder::TYPE_NORMAL) {
		ProgressDialog dialog(IDS_APPLYMESSAGERULES, &status);
		CHECK_QSTATUS();
		RuleCallbackImpl callback(&dialog);
		
		struct Init
		{
			Init(ProgressDialog* pDialog, QSTATUS* pstatus) :
				pDialog_(pDialog)
				{ pDialog_->init(); }
			~Init() { pDialog_->term(); }
			ProgressDialog* pDialog_;
		} init(&dialog, &status);
		CHECK_QSTATUS();
		
		status = pRuleManager_->apply(static_cast<NormalFolder*>(pFolder),
			pDocument_, hwnd_, pProfile_, &callback);
		CHECK_QSTATUS();
	}
	
	return QSTATUS_SUCCESS;
}

QSTATUS qm::MessageApplyRuleAction::isEnabled(
	const ActionEvent& event, bool* pbEnabled)
{
	assert(pbEnabled);
	Folder* pFolder = pFolderModel_->getCurrentFolder();
	*pbEnabled = pFolder && pFolder->getType() == Folder::TYPE_NORMAL;
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
	
	MessagePtrList listMessagePtr;
	status = pMessageSelectionModel_->getSelectedMessages(0, &listMessagePtr);
	CHECK_QSTATUS();
	
	status = helper_.detach(listMessagePtr, 0);
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
	
	Folder* pFolder = 0;
	MessagePtrList l;
	status = pModel_->getSelectedMessages(&pFolder, &l);
	CHECK_QSTATUS();
	if (!l.empty()) {
		status = pFolder->setMessagesFlags(l, nFlags_, nMask_);
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

qm::MessageMoveAction::MessageMoveAction(
	MessageSelectionModel* pModel, MoveMenu* pMoveMenu, QSTATUS* pstatus) :
	pModel_(pModel),
	pMoveMenu_(pMoveMenu)
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
		Folder* pFolderFrom = 0;
		MessagePtrList l;
		status = pModel_->getSelectedMessages(&pFolderFrom, &l);
		CHECK_QSTATUS();
		if (!l.empty()) {
			bool bMove = (event.getModifier() & ActionEvent::MODIFIER_CTRL) == 0;
			status = pFolderFrom->copyMessages(l, pFolderTo, bMove);
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
	MessageSelectionModel* pModel, QSTATUS* pstatus) :
	pModel_(pModel)
{
}

qm::MessageMoveOtherAction::~MessageMoveOtherAction()
{
}

QSTATUS qm::MessageMoveOtherAction::invoke(const ActionEvent& event)
{
	// TODO
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
	
	Folder* pFolder = 0;
	MessagePtrList l;
	status = pModel_->getSelectedMessages(&pFolder, &l);
	CHECK_QSTATUS();
	
	HINSTANCE hInst = Application::getApplication().getResourceHandle();
	string_ptr<WSTRING> wstrTitle;
	status = loadString(hInst, IDS_PROPERTY, &wstrTitle);
	CHECK_QSTATUS();
	
	Lock<Folder> lock(*pFolder);
	
	Folder::MessageHolderList listMessageHolder;
	status = STLWrapper<Folder::MessageHolderList>(
		listMessageHolder).reserve(l.size());
	CHECK_QSTATUS();
	MessagePtrList::const_iterator it = l.begin();
	while (it != l.end()) {
		MessagePtrLock mpl(*it);
		if (mpl)
			listMessageHolder.push_back(mpl);
		++it;
	}
	
	MessagePropertyPage page(listMessageHolder, &status);
	CHECK_QSTATUS();
	PropertySheetBase sheet(hInst, wstrTitle.get(), false, &status);
	CHECK_QSTATUS();
	status = sheet.add(&page);
	CHECK_QSTATUS();
	
	int nRet = 0;
	status = sheet.doModal(hwnd_, 0, &nRet);
	CHECK_QSTATUS();
	if (nRet == IDOK) {
		status = pFolder->setMessagesFlags(listMessageHolder,
			page.getFlags(), page.getMask());
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
 * ToolAccountAction
 *
 */

qm::ToolAccountAction::ToolAccountAction(Document* pDocument,
	FolderModel* pFolderModel, SyncFilterManager* pSyncFilterManager,
	QSTATUS* pstatus) :
	pDocument_(pDocument),
	pFolderModel_(pFolderModel),
	pSyncFilterManager_(pSyncFilterManager)
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
	
	AccountDialog dialog(pDocument_, pAccount, pSyncFilterManager_, &status);
	CHECK_QSTATUS();
	int nRet = 0;
	status = dialog.doModal(getMainWindow()->getHandle(), 0, &nRet);
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
		status = newQsObject(static_cast<const WCHAR*>(0), 0,
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
	Document* pDocument, GoRound* pGoRound,
	SyncDialogManager* pSyncDialogManager, HWND hwnd, QSTATUS* pstatus) :
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

QSTATUS qm::ToolGoRoundAction::invoke(const ActionEvent& event)
{
	DECLARE_QSTATUS();
	
	std::auto_ptr<SyncData> pData;
	status = newQsObject(pSyncManager_, pDocument_,
		hwnd_, SyncDialog::FLAG_SHOWDIALOG, &pData);
	
	const GoRoundCourse* pCourse = 0;
	GoRoundCourseList* pCourseList = 0;
	status = pGoRound_->getCourseList(&pCourseList);
	CHECK_QSTATUS();
	if (pCourseList && pCourseList->getCount() > 0) {
		pCourse = pCourseList->getCourse(event.getId() - IDM_TOOL_GOROUND);
		assert(pCourse);
	}
	
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
	*pbEnabled = true;
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
	FolderModel* pFolderModel, QSTATUS* pstatus) :
	pDocument_(pDocument),
	pFolderModel_(pFolderModel)
{
}

qm::ToolSubAccountAction::~ToolSubAccountAction()
{
}

QSTATUS qm::ToolSubAccountAction::invoke(const ActionEvent& event)
{
	DECLARE_QSTATUS();
	
	Account* pAccount = pFolderModel_->getCurrentAccount();
	if (!pAccount) {
		Folder* pFolder = pFolderModel_->getCurrentFolder();
		if (pFolder)
			pAccount = pFolder->getAccount();
	}
	if (pAccount) {
		unsigned int nIndex = event.getId() - IDM_TOOL_SUBACCOUNT;
		const Account::SubAccountList& listSubAccount = pAccount->getSubAccounts();
		if (nIndex < listSubAccount.size()) {
			SubAccount* pSubAccount = listSubAccount[nIndex];
			const WCHAR* pwszName = pSubAccount->getName();
			
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
		unsigned int nIndex = event.getId() - IDM_TOOL_SUBACCOUNT;
		const Account::SubAccountList& l = pAccount->getSubAccounts();
		if (nIndex < l.size())
			*pbChecked = l[nIndex] == pAccount->getCurrentSubAccount();
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
		status = pData->addSend(pAccount, pSubAccount);
		CHECK_QSTATUS();
	}
	if (nSync_ & SYNC_RECEIVE) {
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
	FilterManager* pFilterManager, QSTATUS* pstatus) :
	pViewModelManager_(pViewModelManager),
	pFilterManager_(pFilterManager)
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
		unsigned int nIndex = event.getId() - IDM_VIEW_FILTER;
		const FilterManager::FilterList* pList = 0;
		status = pFilterManager_->getFilters(&pList);
		CHECK_QSTATUS();
		const Filter* pFilter = 0;
		if (nIndex < pList->size())
			pFilter = (*pList)[nIndex];
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
		unsigned int nIndex = event.getId() - IDM_VIEW_FILTER;
		const FilterManager::FilterList* pList = 0;
		status = pFilterManager_->getFilters(&pList);
		CHECK_QSTATUS();
		const Filter* pFilter = 0;
		if (nIndex < pList->size())
			pFilter = (*pList)[nIndex];
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
	ViewModelManager* pViewModelManager,
	const FilterManager* pFilterManager, QSTATUS* pstatus) :
	pViewModelManager_(pViewModelManager),
	pFilterManager_(pFilterManager)
{
}

qm::ViewFilterCustomAction::~ViewFilterCustomAction()
{
}

QSTATUS qm::ViewFilterCustomAction::invoke(const ActionEvent& event)
{
	// TODO
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
	// TODO
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
	MessageModel* pMessageModel, QSTATUS* pstatus) :
	pMessageModel_(pMessageModel)
{
}

qm::ViewLockPreviewAction::~ViewLockPreviewAction()
{
}

QSTATUS qm::ViewLockPreviewAction::invoke(const ActionEvent& event)
{
	DECLARE_QSTATUS();
	
	if (pMessageModel_->isConnectedToViewModel()) {
		status = pMessageModel_->disconnectFromViewModel();
		CHECK_QSTATUS();
		status = pMessageModel_->setMessage(0);
		CHECK_QSTATUS();
	}
	else {
		status = pMessageModel_->connectToViewModel();
		CHECK_QSTATUS();
		status = pMessageModel_->updateToViewModel();
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
			status = pFolderModel_->setCurrentFolder(pFolder, true);
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
				status = pFolderModel_->setCurrentAccount(*it, true);
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
	MessageWindow* pMessageWindow, Type type, QSTATUS* pstatus) :
	pViewModelManager_(pViewModelManager),
	pFolderModel_(pFolderModel),
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
	
	MessageModel* pMessageModel = pMessageWindow_->getMessageModel();
	
	if (bPreview && (type_ == TYPE_NEXTPAGE || type_ == TYPE_PREVPAGE)) {
		MessagePtrLock lock(pMessageModel->getCurrentMessage());
		if (!lock)
			type = TYPE_SELF;
	}
	
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
		pViewModel = pMessageModel->getViewModel();
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
	
	if (nIndex != static_cast<unsigned int>(-1)) {
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
				status = pFolderModel_->setCurrentFolder(
					pNewViewModel->getFolder(), false);
				CHECK_QSTATUS();
			}
			else {
				pMessageModel->setViewModel(pNewViewModel);
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
	
	unsigned int nCount = pViewModel->getCount();
	unsigned int nStart = nIndex;
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
				(*it)->getUnseenCount() != 0)
				pUnseenFolder = *it;
		}
		if (!pUnseenFolder) {
			for (it = listFolder.begin(); it != itThis && !pUnseenFolder; ++it) {
				if (!(*it)->isFlag(Folder::FLAG_TRASHBOX) &&
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
	SyncDialogManager* pSyncDialogManager, HWND hwnd, QSTATUS* pstatus) :
	pSyncManager_(pSyncManager),
	pDocument_(pDocument),
	pFolderModel_(pFolderModel),
	pSyncDialogManager_(pSyncDialogManager),
	hwnd_(hwnd)
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
		if (pFolder->getType() == Folder::TYPE_NORMAL) {
			if (pFolder->isFlag(Folder::FLAG_SYNCABLE)) {
				status = SyncUtil::syncFolder(pSyncManager_, pDocument_,
				pSyncDialogManager_, hwnd_, SyncDialog::FLAG_NONE,
				static_cast<NormalFolder*>(pFolder));
				CHECK_QSTATUS();
			}
		}
		else if (pFolder->getType() == Folder::TYPE_QUERY) {
			// TODO
		}
	}
	
	return QSTATUS_SUCCESS;
}

QSTATUS qm::ViewRefreshAction::isEnabled(const ActionEvent& event, bool* pbEnabled)
{
	assert(pbEnabled);
	Folder* pFolder = pFolderModel_->getCurrentFolder();
	*pbEnabled = pFolder != 0;
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

qm::ViewSortAction::ViewSortAction(
	ViewModelManager* pViewModelManager, QSTATUS* pstatus) :
	pViewModelManager_(pViewModelManager)
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
		unsigned int nSort = pViewModel->getSort();
		nSort &= ~ViewModel::SORT_INDEX_MASK;
		nSort &= ~ViewModel::SORT_DIRECTION_MASK;
		nSort |= event.getId() - IDM_VIEW_SORT;
		status = pViewModel->setSort(nSort);
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
		*pbChecked = event.getId() ==
			(pViewModel->getSort() & ViewModel::SORT_INDEX_MASK) + IDM_VIEW_SORT;
	
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
