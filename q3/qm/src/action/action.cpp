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
#include <qmsearch.h>
#include <qmtabwindow.h>
#include <qmtemplate.h>

#include <qmscript.h>

#include <qsconv.h>
#include <qsfile.h>
#include <qsstl.h>
#include <qsstream.h>
#include <qsuiutil.h>
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
#include "../model/uri.h"
#include "../script/scriptmanager.h"
#include "../sync/autopilot.h"
#include "../sync/syncmanager.h"
#include "../ui/dialogs.h"
#include "../ui/editframewindow.h"
#include "../ui/menus.h"
#include "../ui/messageframewindow.h"
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
	return pAttachmentSelectionModel_->hasSelectedAttachment();
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
	return bAll_ ? pAttachmentSelectionModel_->hasAttachment() :
		pAttachmentSelectionModel_->hasSelectedAttachment();
}


/****************************************************************************
 *
 * ConfigAutoPilotAction
 *
 */

qm::ConfigAutoPilotAction::ConfigAutoPilotAction(AutoPilotManager* pAutoPilotManager,
												 GoRound* pGoRound,
												 HWND hwnd) :
	pAutoPilotManager_(pAutoPilotManager),
	pGoRound_(pGoRound),
	hwnd_(hwnd)
{
}

qm::ConfigAutoPilotAction::~ConfigAutoPilotAction()
{
}

void qm::ConfigAutoPilotAction::invoke(const ActionEvent& event)
{
	AutoPilotDialog dialog(pAutoPilotManager_, pGoRound_);
	dialog.doModal(hwnd_);
}


/****************************************************************************
 *
 * ConfigColorsAction
 *
 */

qm::ConfigColorsAction::ConfigColorsAction(ColorManager* pColorManager,
										   ViewModelManager* pViewModelManager,
										   Document* pDocument,
										   HWND hwnd) :
	pColorManager_(pColorManager),
	pViewModelManager_(pViewModelManager),
	pDocument_(pDocument),
	hwnd_(hwnd)
{
}

qm::ConfigColorsAction::~ConfigColorsAction()
{
}

void qm::ConfigColorsAction::invoke(const ActionEvent& event)
{
	ColorSetsDialog dialog(pColorManager_, pDocument_);
	if (dialog.doModal(hwnd_) == IDOK)
		pViewModelManager_->invalidateColors();
}


/****************************************************************************
 *
 * ConfigFiltersAction
 *
 */

qm::ConfigFiltersAction::ConfigFiltersAction(FilterManager* pFilterManager,
											 HWND hwnd) :
	pFilterManager_(pFilterManager),
	hwnd_(hwnd)
{
}

qm::ConfigFiltersAction::~ConfigFiltersAction()
{
}

void qm::ConfigFiltersAction::invoke(const ActionEvent& event)
{
	FiltersDialog dialog(pFilterManager_);
	dialog.doModal(hwnd_);
}


/****************************************************************************
 *
 * ConfigGoRoundAction
 *
 */

qm::ConfigGoRoundAction::ConfigGoRoundAction(GoRound* pGoRound,
											 Document* pDocument,
											 SyncFilterManager* pSyncFilterManager,
											 HWND hwnd) :
	pGoRound_(pGoRound),
	pDocument_(pDocument),
	pSyncFilterManager_(pSyncFilterManager),
	hwnd_(hwnd)
{
}

qm::ConfigGoRoundAction::~ConfigGoRoundAction()
{
}

void qm::ConfigGoRoundAction::invoke(const ActionEvent& event)
{
	GoRoundDialog dialog(pGoRound_, pDocument_, pSyncFilterManager_);
	dialog.doModal(hwnd_);
}


/****************************************************************************
 *
 * ConfigRulesAction
 *
 */

qm::ConfigRulesAction::ConfigRulesAction(RuleManager* pRuleManager,
										 Document* pDocument,
										 HWND hwnd) :
	pRuleManager_(pRuleManager),
	pDocument_(pDocument),
	hwnd_(hwnd)
{
}

qm::ConfigRulesAction::~ConfigRulesAction()
{
}

void qm::ConfigRulesAction::invoke(const ActionEvent& event)
{
	RuleSetsDialog dialog(pRuleManager_, pDocument_);
	dialog.doModal(hwnd_);
}


/****************************************************************************
 *
 * ConfigSignaturesAction
 *
 */

qm::ConfigSignaturesAction::ConfigSignaturesAction(SignatureManager* pSignatureManager,
												   Document* pDocument,
												   HWND hwnd) :
	pSignatureManager_(pSignatureManager),
	pDocument_(pDocument),
	hwnd_(hwnd)
{
}

qm::ConfigSignaturesAction::~ConfigSignaturesAction()
{
}

void qm::ConfigSignaturesAction::invoke(const ActionEvent& event)
{
	SignaturesDialog dialog(pSignatureManager_, pDocument_);
	dialog.doModal(hwnd_);
}


/****************************************************************************
 *
 * ConfigSyncFiltersAction
 *
 */

qm::ConfigSyncFiltersAction::ConfigSyncFiltersAction(SyncFilterManager* pManager,
													 HWND hwnd) :
	pManager_(pManager),
	hwnd_(hwnd)
{
}

qm::ConfigSyncFiltersAction::~ConfigSyncFiltersAction()
{
}

void qm::ConfigSyncFiltersAction::invoke(const ActionEvent& event)
{
	SyncFilterSetsDialog dialog(pManager_);
	dialog.doModal(hwnd_);
}


/****************************************************************************
 *
 * ConfigTextsAction
 *
 */

qm::ConfigTextsAction::ConfigTextsAction(FixedFormTextManager* pManager,
										 HWND hwnd) :
	pManager_(pManager),
	hwnd_(hwnd)
{
}

qm::ConfigTextsAction::~ConfigTextsAction()
{
}

void qm::ConfigTextsAction::invoke(const ActionEvent& event)
{
	FixedFormTextsDialog dialog(pManager_);
	dialog.doModal(hwnd_);
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
			hwnd_, SyncDialog::FLAG_NONE, l, ReceiveSyncItem::FLAG_EXPUNGE)) {
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

qm::EditCopyMessageAction::EditCopyMessageAction(Document* pDocument,
												 FolderModel* pFolderModel,
												 MessageSelectionModel* pMessageSelectionModel,
												 HWND hwnd) :
	pDocument_(pDocument),
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
		MessageDataObject* p = new MessageDataObject(pDocument_,
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

qm::EditCutMessageAction::EditCutMessageAction(Document* pDocument,
											   FolderModel* pFolderModel,
											   MessageSelectionModel* pMessageSelectionModel,
											   HWND hwnd) :
	pDocument_(pDocument),
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
		MessageDataObject* p = new MessageDataObject(pDocument_,
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
													 bool bDirect,
													 HWND hwnd,
													 Profile* pProfile) :
	pMessageSelectionModel_(pMessageSelectionModel),
	pMessageModel_(0),
	bDirect_(bDirect),
	hwnd_(hwnd),
	bConfirm_(false)
{
	bConfirm_ = pProfile->getInt(L"Global", L"ConfirmDeleteMessage", 0) != 0;
}

qm::EditDeleteMessageAction::EditDeleteMessageAction(MessageModel* pMessageModel,
													 ViewModelHolder* pViewModelHolder,
													 bool bDirect,
													 HWND hwnd,
													 Profile* pProfile) :
	pMessageSelectionModel_(0),
	pMessageModel_(pMessageModel),
	pViewModelHolder_(pViewModelHolder),
	bDirect_(bDirect),
	hwnd_(hwnd),
	bConfirm_(false)
{
	bConfirm_ = pProfile->getInt(L"Global", L"ConfirmDeleteMessage", 0) != 0;
}

qm::EditDeleteMessageAction::~EditDeleteMessageAction()
{
}

void qm::EditDeleteMessageAction::invoke(const ActionEvent& event)
{
	if (pMessageSelectionModel_) {
		AccountLock lock;
		Folder* pFolder = 0;
		MessageHolderList l;
		pMessageSelectionModel_->getSelectedMessages(&lock, &pFolder, &l);
		
		Account* pAccount = lock.get();
		if (l.empty())
			return;
		
		if (!confirm())
			return;
		
		ProgressDialogMessageOperationCallback callback(
			hwnd_, IDS_DELETE, IDS_DELETE);
		if (!pAccount->removeMessages(l, pFolder, bDirect_, &callback)) {
			ActionUtil::error(hwnd_, IDS_ERROR_DELETEMESSAGES);
			return;
		}
	}
	else {
		ViewModel* pViewModel = pViewModelHolder_->getViewModel();
		
		Lock<ViewModel> lock(*pViewModel);
		
		MessagePtrLock mpl(pMessageModel_->getCurrentMessage());
		if (!mpl)
			return;
		
		if (!confirm())
			return;
		
		unsigned int nIndex = pViewModel->getIndex(mpl);
		if (nIndex < pViewModel->getCount() - 1) {
			MessageHolder* pmh = pViewModel->getMessageHolder(nIndex + 1);
			pMessageModel_->setMessage(pmh);
		}
		
		Account* pAccount = mpl->getFolder()->getAccount();
		MessageHolderList l(1, mpl);
		if (!pAccount->removeMessages(l, pViewModel->getFolder(), bDirect_, 0)) {
			ActionUtil::error(hwnd_, IDS_ERROR_DELETEMESSAGES);
			return;
		}
	}
}

bool qm::EditDeleteMessageAction::isEnabled(const ActionEvent& event)
{
	if (pMessageSelectionModel_) {
		return pMessageSelectionModel_->hasSelectedMessage();
	}
	else {
		MessagePtrLock mpl(pMessageModel_->getCurrentMessage());
		return mpl != 0;
	}
}

bool qm::EditDeleteMessageAction::confirm() const
{
	if (bConfirm_)
		return messageBox(Application::getApplication().getResourceHandle(),
			IDS_CONFIRMDELETEMESSAGE, MB_YESNO | MB_DEFBUTTON2 | MB_ICONQUESTION, hwnd_) == IDYES;
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
	HWND hwndFrame = pMessageWindow_->getParentFrame();
	
	bool bFound = false;
	if (type_ == TYPE_NORMAL) {
		bool bSupportRegex = (pMessageWindow_->getSupportedFindFlags() & MessageWindow::FIND_REGEX) != 0;
		FindDialog dialog(pProfile_, bSupportRegex);
		if (dialog.doModal(hwndFrame) != IDOK)
			return;
		
		pFindReplaceManager_->setData(dialog.getFind(),
			(dialog.isMatchCase() ? FindReplaceData::FLAG_MATCHCASE : 0) |
			(dialog.isRegex() ? FindReplaceData::FLAG_REGEX : 0));
		
		unsigned int nFlags =
			(dialog.isMatchCase() ? MessageWindow::FIND_MATCHCASE : 0) |
			(dialog.isRegex() ? MessageWindow::FIND_REGEX : 0) |
			(dialog.isPrev() ? MessageWindow::FIND_PREVIOUS : 0);
		bFound = pMessageWindow_->find(dialog.getFind(), nFlags);
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
		ActionUtil::info(hwndFrame, IDS_FINDNOTFOUND);
}

bool qm::EditFindAction::isEnabled(const ActionEvent& event)
{
	if (!pMessageWindow_->isActive())
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
	if (pFolder && pFolder->getType() == Folder::TYPE_NORMAL &&
		!pFolder->isFlag(Folder::FLAG_NOSELECT)) {
		ComPtr<IDataObject> pDataObject(MessageDataObject::getClipboard(pDocument_));
		
		NormalFolder* pNormalFolder = static_cast<NormalFolder*>(pFolder);
		MessageDataObject::Flag flag = MessageDataObject::getPasteFlag(
			pDataObject.get(), pDocument_, pNormalFolder);
		UINT nId = flag == MessageDataObject::FLAG_MOVE ?
			IDS_MOVEMESSAGE : IDS_COPYMESSAGE;
		ProgressDialogMessageOperationCallback callback(hwnd_, nId, nId);
		if (!MessageDataObject::pasteMessages(pDataObject.get(),
			pDocument_, pNormalFolder, flag, &callback)) {
			ActionUtil::error(hwnd_, IDS_ERROR_PASTEMESSAGES);
			return;
		}
		
		if (!pDocument_->isOffline() &&
			!pFolder->isFlag(Folder::FLAG_LOCAL) &&
			pFolder->isFlag(Folder::FLAG_SYNCABLE) &&
			pFolder->isFlag(Folder::FLAG_SYNCWHENOPEN)) {
			SyncUtil::syncFolder(pSyncManager_, pDocument_, pSyncDialogManager_,
				hwnd_, SyncDialog::FLAG_NONE, pNormalFolder, 0);
		}
#ifdef _WIN32_WCE
		Clipboard clipboard(0);
		clipboard.empty();
#else
		::OleSetClipboard(0);
#endif
	}
}

bool qm::EditPasteMessageAction::isEnabled(const ActionEvent& event)
{
	Folder* pFolder = FolderActionUtil::getFolder(pFolderModel_);
	if (!pFolder || pFolder->getType() != Folder::TYPE_NORMAL ||
		pFolder->isFlag(Folder::FLAG_NOSELECT))
		return false;
	return MessageDataObject::queryClipboard();
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
			wstring_ptr wstrTemplate(loadString(hInst, IDS_IGNORECHECKERROR));
			wstring_ptr wstrFolderName(pmh->getFolder()->getFullName());
			wstring_ptr wstrMessage(allocWString(
				wcslen(wstrTemplate.get()) + wcslen(wstrFolderName.get()) + 100));
			const MessageHolder::MessageBoxKey& boxKey = pmh->getMessageBoxKey();
			swprintf(wstrMessage.get(), wstrTemplate.get(), wstrFolderName.get(),
				pmh->getId(), boxKey.nOffset_, boxKey.nLength_);
			switch (messageBox(wstrMessage.get(), MB_YESNO | MB_ICONQUESTION | MB_DEFBUTTON2, getDialog()->getHandle())) {
			case IDYES:
				return ::GetKeyState(VK_SHIFT) < 0 ? IGNORE_ALL : IGNORE_TRUE;
			default:
				return IGNORE_FALSE;
			}
		}
	};
	
	AccountCheckCallbackImpl callback(hwnd_, IDS_CHECK, IDS_CHECK);
	if (!pAccount->check(&callback)) {
		ActionUtil::error(hwnd_, IDS_ERROR_CHECK);
		return;
	}
}

bool qm::FileCheckAction::isEnabled(const ActionEvent& event)
{
	return FolderActionUtil::getAccount(pFolderModel_) != 0;
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
	
	ProgressDialogMessageOperationCallback callback(
		hwnd_, IDS_COMPACT, IDS_COMPACT);
	if (!pAccount->compact(&callback)) {
		ActionUtil::error(hwnd_, IDS_ERROR_COMPACT);
		return;
	}
}

bool qm::FileCompactAction::isEnabled(const ActionEvent& event)
{
	return FolderActionUtil::getAccount(pFolderModel_) != 0;
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
	
	Lock<Account> lock(*pAccount);
	
	const Account::FolderList& listFolder = pAccount->getFolders();
	
	unsigned int nCount = 0;
	for (Account::FolderList::const_iterator it = listFolder.begin(); it != listFolder.end(); ++it) {
		Folder* pFolder = *it;
		if (pFolder->getType() == Folder::TYPE_NORMAL) {
			if (!pFolder->loadMessageHolders())
				return;
			nCount += pFolder->getCount();
		}
	}
	
	ProgressDialog dialog(IDS_DUMP);
	ProgressDialogInit init(&dialog, hwnd_, IDS_DUMP, IDS_DUMP, 0, nCount, 0);
	
	for (Account::FolderList::const_iterator it = listFolder.begin(); it != listFolder.end(); ++it) {
		Folder* pFolder = *it;
		if (!dumpFolder(wstrPath.get(), pFolder, pFolder->getType() != Folder::TYPE_NORMAL, &dialog)) {
			ActionUtil::error(hwnd_, IDS_ERROR_DUMP);
			return;
		}
	}
}

bool qm::FileDumpAction::isEnabled(const ActionEvent& event)
{
	return FolderActionUtil::getAccount(pFolderModel_) != 0;
}

bool qm::FileDumpAction::dumpFolder(const WCHAR* pwszPath,
									Folder* pFolder,
									bool bCreateDirectoryOnly,
									ProgressDialog* pDialog)
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
		swprintf(wszName, L"%u", pmh->getId());
		
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
	swprintf(wsz, L"$%c%x", pFolder->getType() == Folder::TYPE_NORMAL ? L'n' : L'q', pFolder->getFlags());
	
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
								   FolderModel* pFolderModel,
								   Profile* pProfile) :
	hwnd_(hwnd),
	pDocument_(pDocument),
	pSyncManager_(pSyncManager),
	pSyncDialogManager_(pSyncDialogManager),
	pTempFileCleaner_(pTempFileCleaner),
	pEditFrameWindowManager_(pEditFrameWindowManager),
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
		ActionUtil::error(hwnd_, IDS_SYNCHRONIZING);
		return false;
	}
	
	if (!pEditFrameWindowManager_->closeAll())
		return false;
	
	bool bEmptyTrash = pProfile_->getInt(L"Global", L"EmptyTrashOnExit", 0) != 0;
	if (bEmptyTrash)
		FolderEmptyTrashAction::emptyAllTrash(pDocument_, pSyncManager_,
			pSyncDialogManager_, pFolderModel_, hwnd_, pProfile_);
	
	{
		WaitCursor cursor;
		Application& app = Application::getApplication();
		if (!app.save()) {
			ActionUtil::error(hwnd_, IDS_ERROR_SAVE);
			return false;
		}
		app.startShutdown();
		pDocument_->setOffline(true);
		pSyncManager_->dispose();
	}
	
	struct CallbackImpl : public TempFileCleanerCallback
	{
		virtual bool confirmDelete(const WCHAR* pwszPath)
		{
			HINSTANCE hInst = Application::getApplication().getResourceHandle();
			
			wstring_ptr wstr(loadString(hInst, IDS_CONFIRMDELETETEMPFILE));
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
		if (!export(lock.get(), pFolder, l)) {
			ActionUtil::error(hwnd_, IDS_ERROR_EXPORT);
			return;
		}
	}
}

bool qm::FileExportAction::isEnabled(const ActionEvent& event)
{
	return pMessageSelectionModel_->hasSelectedMessage();
}

bool qm::FileExportAction::export(Account* pAccount,
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
		
		ProgressDialog progressDialog(IDS_EXPORT);
		ProgressDialogInit init(&progressDialog, hwnd_,
			IDS_EXPORT, IDS_EXPORT, 0, l.size(), 0);
		
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
				swprintf(wszNumber, L"%d", n);
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
					if (!writeMessage(&stream, pTemplate, l[n], pwszEncoding))
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
						if (!writeMessage(&stream, pTemplate, *it, pwszEncoding))
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
										MessageHolder* pmh,
										const WCHAR* pwszEncoding)
{
	Message msg;
	TemplateContext context(pmh, &msg, MessageHolderList(),
		pmh->getFolder()->getAccount(), pDocument_, hwnd_,
		pEncodingModel_->getEncoding(), pSecurityModel_->getSecurityMode(),
		pProfile_, 0, TemplateContext::ArgumentList());
	
	wstring_ptr wstrValue;
	if (pTemplate->getValue(context, &wstrValue) != Template::RESULT_SUCCESS)
		return false;
	
	std::auto_ptr<Converter> pConverter(ConverterFactory::getInstance(pwszEncoding));
	if (!pConverter.get())
		return false;
	
	size_t nLen = wcslen(wstrValue.get());
	xstring_size_ptr strContent = pConverter->encode(wstrValue.get(), &nLen);
	
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
	if (!pmh->getMessage(Account::GETMESSAGEFLAG_ALL | Account::GETMESSAGEFLAG_NOFALLBACK,
		0, nSecurityMode, &msg))
		return false;
	
	if (nFlags & FLAG_ADDFLAGS) {
		NumberParser flags(pmh->getFlags() & MessageHolder::FLAG_USER_MASK, NumberParser::FLAG_HEX);
		if (!msg.replaceField(L"X-QMAIL-Flags", flags))
			return false;
	}
	
	xstring_ptr strContent(msg.getContent());
	if (!strContent.get())
		return false;
	
	if (nFlags & FLAG_WRITESEPARATOR) {
		if (pStream->write(reinterpret_cast<const unsigned char*>("From \r\n"), 7) == -1)
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
	}
	else {
		if (pStream->write(reinterpret_cast<unsigned char*>(strContent.get()),
			strlen(strContent.get())) == -1)
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
		if (!import(static_cast<NormalFolder*>(pFolder))) {
			ActionUtil::error(hwnd_, IDS_ERROR_IMPORT);
			return;
		}
		if (!pDocument_->isOffline() &&
			!pFolder->isFlag(Folder::FLAG_LOCAL) &&
			pFolder->isFlag(Folder::FLAG_SYNCABLE) &&
			pFolder->isFlag(Folder::FLAG_SYNCWHENOPEN)) {
			SyncUtil::syncFolder(pSyncManager_, pDocument_, pSyncDialogManager_,
				hwnd_, SyncDialog::FLAG_NONE, static_cast<NormalFolder*>(pFolder), 0);
		}
		else {
			if (!pFolder->getAccount()->save()) {
				ActionUtil::error(hwnd_, IDS_ERROR_SAVE);
				return;
			}
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
								  HWND hwnd)
{
	unsigned int nCount = listPath.size();
	if (bMultipleMessagesInFile)
		nCount = 100;
	
	ProgressDialog progressDialog(IDS_IMPORT);
	ProgressDialogInit init(&progressDialog, hwnd,
		IDS_IMPORT, IDS_IMPORT, 0, nCount, 0);
	
	if (bMultipleMessagesInFile) {
		int nPos = 0;
		for (PathList::size_type n = 0; n < listPath.size(); ++n) {
			bool bCanceled = false;
			if (!readMultipleMessages(pFolder, listPath[n], pwszEncoding,
				nFlags, &progressDialog, &nPos, &bCanceled))
				return false;
			if (bCanceled)
				break;
		}
	}
	else {
		for (PathList::size_type n = 0; n < listPath.size(); ++n) {
			if (!readSingleMessage(pFolder, listPath[n], pwszEncoding, nFlags))
				return false;
			
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
											HWND hwnd)
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
		
		if (!import(pFolder, listPath, dialog.isMultiple(),
			dialog.getEncoding(), dialog.getFlags(), hwnd))
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
	
	xstring_ptr strContent;
	if (pwszEncoding) {
		InputStreamReader reader(&bufferedStream, false, pwszEncoding);
		if (!reader)
			return false;
		
		XStringBuffer<WXSTRING> buf;
		while (true) {
			XStringBufferLock<WXSTRING> lock(&buf, 1024);
			
			size_t nRead = reader.read(lock.get(), 1024);
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
		
		strContent = buf.getXString();
	}
	
	Account* pAccount = pFolder->getAccount();
	if (!pAccount->importMessage(pFolder, strContent.get(), nFlags))
		return false;
	
	return true;
}

bool qm::FileImportAction::readMultipleMessages(NormalFolder* pFolder,
												const WCHAR* pwszPath,
												const WCHAR* pwszEncoding,
												unsigned int nFlags,
												ProgressDialog* pDialog,
												int* pnPos,
												bool* pbCanceled)
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
	while (bNewLine) {
		xstring_ptr strLine;
		CHAR cNext = '\0';
		if (!readLine(&stream, cPrev, &strLine, &cNext, &bNewLine))
			return false;
		cPrev = cNext;
		
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
				
				xstring_ptr strContent;
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
					strContent = buf.getXString();
				}
				
				Account* pAccount = pFolder->getAccount();
				if (!pAccount->importMessage(pFolder, strContent.get(), nFlags))
					return false;
				
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
			
			if (!buf.append(p) || !buf.append(pszNewLine))
				return false;
		}
	}
	
	return true;
}

bool qm::FileImportAction::import(NormalFolder* pFolder)
{
	return importShowDialog(pFolder, PathList(), pProfile_, hwnd_);
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
	
	ProgressDialog dialog(IDS_LOAD);
	ProgressDialogInit init(&dialog, hwnd_, IDS_LOAD, IDS_LOAD, 0, 100, 0);
	
	int nPos = 0;
	if (!loadFolder(pAccount, 0, wstrPath.get(), &dialog, &nPos)) {
		ActionUtil::error(hwnd_, IDS_ERROR_LOAD);
		return;
	}
}

bool qm::FileLoadAction::isEnabled(const ActionEvent& event)
{
	return FolderActionUtil::getAccount(pFolderModel_) != 0;
}

bool qm::FileLoadAction::loadFolder(Account* pAccount,
									Folder* pFolder,
									const WCHAR* pwszPath,
									ProgressDialog* pDialog,
									int* pnPos)
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
	pDocument_->setOffline(!pDocument_->isOffline());
}

bool qm::FileOfflineAction::isEnabled(const ActionEvent& event)
{
	return !pSyncManager_->isSyncing();
}

bool qm::FileOfflineAction::isChecked(const ActionEvent& event)
{
	return pDocument_->isOffline();
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
	TemplateContext context(pmh, &msg, listSelected, pAccount, pDocument_, hwnd_,
		pEncodingModel_->getEncoding(), pSecurityModel_->getSecurityMode(),
		pProfile_, 0, TemplateContext::ArgumentList());
	
	wstring_ptr wstrValue;
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
	
	wstring_ptr wstrExtension(pProfile_->getString(
		L"Global", L"PrintExtension", L"html"));
	
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
	
	Account* pAccount = pFolder->getAccount();
	ProgressDialogMessageOperationCallback callback(
		hwnd_, IDS_SALVAGE, IDS_SALVAGE);
	if (!pAccount->salvage(static_cast<NormalFolder*>(pFolder), &callback)) {
		ActionUtil::error(hwnd_, IDS_ERROR_SALVAGE);
		return;
	}
}

bool qm::FileSalvageAction::isEnabled(const ActionEvent& event)
{
	Folder* pFolder = FolderActionUtil::getFolder(pFolderModel_);
	return pFolder && pFolder->getType() == Folder::TYPE_NORMAL;
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
	if (!Application::getApplication().save()) {
		ActionUtil::error(hwnd_, IDS_ERROR_SAVE);
		return;
	}
}


/****************************************************************************
 *
 * FolderCreateAction
 *
 */

qm::FolderCreateAction::FolderCreateAction(FolderSelectionModel* pFolderSelectionModel,
										   HWND hwnd,
										   Profile* pProfile) :
	pFolderSelectionModel_(pFolderSelectionModel),
	hwnd_(hwnd),
	pProfile_(pProfile)
{
}

qm::FolderCreateAction::~FolderCreateAction()
{
}

void qm::FolderCreateAction::invoke(const ActionEvent& event)
{
	std::pair<Account*, Folder*> p(FolderActionUtil::getFocused(pFolderSelectionModel_));
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
	if (pAccount->isSupport(Account::SUPPORT_LOCALFOLDERSYNC))
		nFlags |= CreateFolderDialog::FLAG_ALLOWLOCALSYNC;
	
	CreateFolderDialog dialog(type, nFlags);
	if (dialog.doModal(hwnd_) == IDOK) {
		NormalFolder* pNormalFolder = 0;
		QueryFolder* pQueryFolder = 0;
		switch (dialog.getType()) {
		case CreateFolderDialog::TYPE_LOCALFOLDER:
			pNormalFolder = pAccount->createNormalFolder(
				dialog.getName(), pFolder, false, dialog.isSyncable());
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
		else {
			std::pair<const WCHAR**, size_t> params(pAccount->getFolderParamNames());
			if (params.second) {
				Account::FolderList l(1, pNormalFolder);
				FolderPropertyAction::openProperty(l,
					FolderPropertyAction::OPEN_PARAMETER, hwnd_, pProfile_);
			}
		}
	}
}

bool qm::FolderCreateAction::isEnabled(const ActionEvent& event)
{
	std::pair<Account*, Folder*> p(FolderActionUtil::getFocused(pFolderSelectionModel_));
	return p.first || p.second;
}


/****************************************************************************
 *
 * FolderDeleteAction
 *
 */

qm::FolderDeleteAction::FolderDeleteAction(FolderModel* pFolderModel,
										   FolderSelectionModel* pFolderSelectionModel,
										   HWND hwnd) :
	pFolderModel_(pFolderModel),
	pFolderSelectionModel_(pFolderSelectionModel),
	hwnd_(hwnd)
{
}

qm::FolderDeleteAction::~FolderDeleteAction()
{
}

void qm::FolderDeleteAction::invoke(const ActionEvent& event)
{
	Account::FolderList l;
	FolderActionUtil::getSelected(pFolderSelectionModel_, &l);
	if (l.empty())
		return;
	
	HINSTANCE hInst = Application::getApplication().getResourceHandle();
	wstring_ptr wstrConfirm(loadString(hInst, IDS_CONFIRMREMOVEFOLDER));
	wstring_ptr wstrName(Util::formatFolders(l, L", "));
	wstring_ptr wstrMessage(allocWString(wcslen(wstrConfirm.get()) + wcslen(wstrName.get()) + 64));
	swprintf(wstrMessage.get(), wstrConfirm.get(), wstrName.get());
	int nRet = messageBox(wstrMessage.get(), MB_YESNO | MB_DEFBUTTON2 | MB_ICONQUESTION, hwnd_);
	if (nRet != IDYES)
		return;
	
	if (l.size() == 1) {
		if (!deleteFolder(l[0])) {
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
			if (!deleteFolder(pFolder)) {
				ActionUtil::error(hwnd_, IDS_ERROR_DELETEFOLDER);
				return;
			}
		}
	}
}

bool qm::FolderDeleteAction::isEnabled(const ActionEvent& event)
{
	return FolderActionUtil::hasSelected(pFolderSelectionModel_);
}

bool qm::FolderDeleteAction::deleteFolder(Folder* pFolder) const
{
	Account* pAccount = pFolder->getAccount();
	
	Folder* pTrash = pAccount->getFolderByFlag(Folder::FLAG_TRASHBOX);
	if (pTrash &&
		pFolder != pTrash &&
		!pTrash->isAncestorOf(pFolder) &&
		!pFolder->isAncestorOf(pTrash) &&
		!pTrash->isFlag(Folder::FLAG_NOINFERIORS)) {
		return pAccount->moveFolder(pFolder, pTrash);
	}
	else {
		if (pFolderModel_->getCurrent().second == pFolder) {
			Folder* pParent = pFolder->getParentFolder();
			if (pParent)
				pFolderModel_->setCurrent(0, pParent, false);
			else
				pFolderModel_->setCurrent(pAccount, 0, false);
		}
		return pAccount->removeFolder(pFolder);
	}
}


/****************************************************************************
 *
 * FolderEmptyAction
 *
 */

qm::FolderEmptyAction::FolderEmptyAction(FolderSelectionModel* pFolderSelectionModel,
										 HWND hwnd,
										 Profile* pProfile) :
	pFolderSelectionModel_(pFolderSelectionModel),
	hwnd_(hwnd),
	bConfirm_(true)
{
	bConfirm_ = pProfile->getInt(L"Global", L"ConfirmEmptyFolder", 1) != 0;
}

qm::FolderEmptyAction::~FolderEmptyAction()
{
}

void qm::FolderEmptyAction::invoke(const ActionEvent& event)
{
	Account::FolderList l;
	FolderActionUtil::getSelected(pFolderSelectionModel_, &l);
	if (l.empty())
		return;
	
	if (bConfirm_) {
		HINSTANCE hInst = Application::getApplication().getResourceHandle();
		wstring_ptr wstrConfirm(loadString(hInst, IDS_CONFIRMEMPTYFOLDER));
		wstring_ptr wstrName(Util::formatFolders(l, L", "));
		wstring_ptr wstrMessage(allocWString(wcslen(wstrConfirm.get()) + wcslen(wstrName.get()) + 64));
		swprintf(wstrMessage.get(), wstrConfirm.get(), wstrName.get());
		if (messageBox(wstrMessage.get(), MB_YESNO | MB_DEFBUTTON2 | MB_ICONQUESTION, hwnd_) != IDYES)
			return;
	}
	
	for (Account::FolderList::const_iterator it = l.begin(); it != l.end(); ++it) {
		Folder* pFolder = *it;
		
		if (!pFolder->isFlag(Folder::FLAG_TRASHBOX)) {
			Account* pAccount = pFolder->getAccount();
			Lock<Account> lock(*pAccount);
			
			MessageHolderList l(pFolder->getMessages());
			if (!l.empty()) {
				if (!pAccount->removeMessages(l, pFolder, false, 0)) {
					ActionUtil::error(hwnd_, IDS_ERROR_EMPTYFOLDER);
					return;
				}
			}
		}
	}
}

bool qm::FolderEmptyAction::isEnabled(const ActionEvent& event)
{
	Account::FolderList l;
	FolderActionUtil::getSelected(pFolderSelectionModel_, &l);
	return l.size() > 1 || (l.size() == 1 && !l.front()->isFlag(Folder::FLAG_TRASHBOX));
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
	bConfirm_(true)
{
	bConfirm_ = pProfile->getInt(L"Global", L"ConfirmEmptyTrash", 1) != 0;
}

qm::FolderEmptyTrashAction::~FolderEmptyTrashAction()
{
}

void qm::FolderEmptyTrashAction::invoke(const ActionEvent& event)
{
	Account* pAccount = getAccount();
	if (pAccount)
		emptyTrash(pAccount, pDocument_, pSyncManager_,
			pSyncDialogManager_, pFolderModel_, hwnd_, bConfirm_);
}

bool qm::FolderEmptyTrashAction::isEnabled(const ActionEvent& event)
{
	return getTrash() != 0;
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
	
	bool bConfirm = pProfile->getInt(L"Global", L"ConfirmEmptyTrash", 1) != 0;
	if (bConfirm) {
		if (messageBox(Application::getApplication().getResourceHandle(),
			IDS_CONFIRMEMPTYTRASH, MB_YESNO | MB_DEFBUTTON2 | MB_ICONQUESTION, hwnd) != IDYES)
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
			IDS_CONFIRMEMPTYTRASH, MB_YESNO | MB_DEFBUTTON2 | MB_ICONQUESTION, hwnd) != IDYES)
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
				hwnd, IDS_EMPTYTRASH, IDS_EMPTYTRASH);
			if (!pAccount->removeMessages(l, pTrash, false, &callback)) {
				ActionUtil::error(hwnd, IDS_ERROR_EMPTYTRASH);
				return;
			}
			
			if (!pAccount->save()) {
				ActionUtil::error(hwnd, IDS_ERROR_SAVE);
				return;
			}
		}
	}
	else if (pTrash->isFlag(Folder::FLAG_SYNCABLE)) {
		if (!SyncUtil::syncFolder(pSyncManager, pDocument,
			pSyncDialogManager, hwnd, SyncDialog::FLAG_NONE, pTrash,
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
	
	Folder* pTrash = pAccount->getFolderByFlag(Folder::FLAG_TRASHBOX);
	if (pTrash && pTrash->getType() == Folder::TYPE_NORMAL)
		return static_cast<NormalFolder*>(pTrash);
	else
		return 0;
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
											   HWND hwnd,
											   Profile* pProfile) :
	pFolderSelectionModel_(pFolderSelectionModel),
	hwnd_(hwnd),
	pProfile_(pProfile)
{
}

qm::FolderPropertyAction::~FolderPropertyAction()
{
}

void qm::FolderPropertyAction::invoke(const ActionEvent& event)
{
	Account::FolderList l;
	FolderActionUtil::getSelected(pFolderSelectionModel_, &l);
	openProperty(l, FolderPropertyAction::OPEN_PROPERTY, hwnd_, pProfile_);
}

bool qm::FolderPropertyAction::isEnabled(const ActionEvent& event)
{
	return FolderActionUtil::hasSelected(pFolderSelectionModel_);
}

void qm::FolderPropertyAction::openProperty(const Account::FolderList& listFolder,
											Open open,
											HWND hwnd,
											Profile* pProfile)
{
	HINSTANCE hInst = Application::getApplication().getResourceHandle();
	wstring_ptr wstrTitle(loadString(hInst, IDS_PROPERTY));
	
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
		Account* pAccount = listFolder.front()->getAccount();
		std::pair<const WCHAR**, size_t> params(pAccount->getFolderParamNames());
		if (params.second != 0) {
			pParameterPage.reset(new FolderParameterPage(
				listFolder.front(), params.first, params.second));
			sheet.add(pParameterPage.get());
			if (open == OPEN_PARAMETER)
				sheet.setStartPage(1);
		}
	}
	
	sheet.doModal(hwnd);
}


/****************************************************************************
 *
 * FolderRenameAction
 *
 */

qm::FolderRenameAction::FolderRenameAction(FolderSelectionModel* pFolderSelectionModel,
										   HWND hwnd) :
	pFolderSelectionModel_(pFolderSelectionModel),
	hwnd_(hwnd)
{
}

qm::FolderRenameAction::~FolderRenameAction()
{
}

void qm::FolderRenameAction::invoke(const ActionEvent& event)
{
	Folder* pFolder = FolderActionUtil::getFocused(pFolderSelectionModel_).second;
	if (pFolder) {
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
			if (!pAccount->moveFolder(pFolder, pParent)) {
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
	}
}

bool qm::FolderRenameAction::isEnabled(const ActionEvent& event)
{
	return FolderActionUtil::getFocused(pFolderSelectionModel_).second != 0;
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
 * FolderUpdateAction
 *
 */

qm::FolderUpdateAction::FolderUpdateAction(FolderModel* pFolderModel,
										   HWND hwnd) :
	pFolderModel_(pFolderModel),
	hwnd_(hwnd)
{
}

qm::FolderUpdateAction::~FolderUpdateAction()
{
}

void qm::FolderUpdateAction::invoke(const ActionEvent& event)
{
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
}

bool qm::FolderUpdateAction::isEnabled(const ActionEvent& event)
{
	Account* pAccount = FolderActionUtil::getAccount(pFolderModel_);
	return pAccount && pAccount->isSupport(Account::SUPPORT_REMOTEFOLDER);
}


/****************************************************************************
 *
 * MessageApplyRuleAction
 *
 */

qm::MessageApplyRuleAction::MessageApplyRuleAction(RuleManager* pRuleManager,
												   FolderModel* pFolderModel,
												   bool bAll,
												   SecurityModel* pSecurityModel,
												   Document* pDocument,
												   HWND hwnd,
												   Profile* pProfile) :
	pRuleManager_(pRuleManager),
	pFolderModel_(pFolderModel),
	pMessageSelectionModel_(0),
	bAll_(bAll),
	pSecurityModel_(pSecurityModel),
	pDocument_(pDocument),
	hwnd_(hwnd),
	pProfile_(pProfile)
{
}

qm::MessageApplyRuleAction::MessageApplyRuleAction(RuleManager* pRuleManager,
												   MessageSelectionModel* pMessageSelectionModel,
												   SecurityModel* pSecurityModel,
												   Document* pDocument,
												   HWND hwnd,
												   Profile* pProfile) :
	pRuleManager_(pRuleManager),
	pFolderModel_(0),
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
			wstring_ptr wstrMessage(getMessage(IDS_APPLYRULE_CHECKINGMESSAGES, pFolder));
			pDialog_->setMessage(wstrMessage.get());
		}
		
		virtual void applyingRule(Folder* pFolder)
		{
			wstring_ptr wstrMessage(getMessage(IDS_APPLYRULE_APPLYINGRULE, pFolder));
			pDialog_->setMessage(wstrMessage.get());
		}
		
		virtual void setRange(unsigned int nMin,
							  unsigned int nMax)
		{
			pDialog_->setRange(nMin, nMax);
		}
		
		virtual void setPos(unsigned int nPos)
		{
			pDialog_->setPos(nPos);
		}
		
		wstring_ptr getMessage(UINT nId,
							   Folder* pFolder) {
			HINSTANCE hInst = Application::getApplication().getResourceHandle();
			wstring_ptr wstrMessage(loadString(hInst, nId));
			wstring_ptr wstrName(pFolder->getFullName());
			return concat(wstrMessage.get(), L" : ", wstrName.get());
		}
		
		ProgressDialog* pDialog_;
	};
	
	ProgressDialog dialog(IDS_APPLYMESSAGERULES);
	RuleCallbackImpl callback(&dialog);
	
	if (pFolderModel_) {
		if (bAll_) {
			Account* pAccount = FolderActionUtil::getAccount(pFolderModel_);
			if (!pAccount)
				return;
			Account::FolderList l(pAccount->getFolders());
			std::sort(l.begin(), l.end(), FolderLess());
			
			ProgressDialogInit init(&dialog, hwnd_);
			for (Account::FolderList::const_iterator it = l.begin(); it != l.end(); ++it) {
				Folder* pFolder = *it;
				if (pFolder->getType() == Folder::TYPE_NORMAL &&
					!pFolder->isFlag(Folder::FLAG_TRASHBOX) &&
					!pFolder->isHidden()) {
					if (!pRuleManager_->apply(pFolder, 0, pDocument_, hwnd_, pProfile_,
						pSecurityModel_->getSecurityMode(), &callback)) {
						ActionUtil::error(hwnd_, IDS_ERROR_APPLYRULE);
						return;
					}
				}
			}
		}
		else {
			Folder* pFolder = FolderActionUtil::getFolder(pFolderModel_);
			if (pFolder) {
				ProgressDialogInit init(&dialog, hwnd_);
				if (!pRuleManager_->apply(pFolder, 0, pDocument_, hwnd_, pProfile_,
					pSecurityModel_->getSecurityMode(), &callback)) {
					ActionUtil::error(hwnd_, IDS_ERROR_APPLYRULE);
					return;
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
			ProgressDialogInit init(&dialog, hwnd_);
			if (!pRuleManager_->apply(pFolder, &l, pDocument_, hwnd_, pProfile_,
				pSecurityModel_->getSecurityMode(), &callback)) {
				ActionUtil::error(hwnd_, IDS_ERROR_APPLYRULE);
				return;
			}
		}
	}
}

bool qm::MessageApplyRuleAction::isEnabled(const ActionEvent& event)
{
	if (pFolderModel_)
		return FolderActionUtil::getAccount(pFolderModel_) != 0;
	else
		return pMessageSelectionModel_->hasSelectedMessage();
}


/****************************************************************************
 *
 * MessageApplyTemplateAction
 *
 */

qm::MessageApplyTemplateAction::MessageApplyTemplateAction(TemplateMenu* pTemplateMenu,
														   Document* pDocument,
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
		pExternalEditorManager, hwnd, pProfile, bExternalEditor),
	pTemplateMenu_(pTemplateMenu),
	pFolderModel_(pFolderModel),
	hwnd_(hwnd)
{
}

qm::MessageApplyTemplateAction::~MessageApplyTemplateAction()
{
}

void qm::MessageApplyTemplateAction::invoke(const ActionEvent& event)
{
	std::pair<Account*, Folder*> p(pFolderModel_->getCurrent());
	Account* pAccount = p.first ? p.first : p.second ? p.second->getAccount() : 0;
	if (!pAccount)
		return;
	
	const WCHAR* pwszTemplate = pTemplateMenu_->getTemplate(event.getId(), pAccount);
	if (!pwszTemplate)
		return;
	
	bool bExternalEditor = (event.getModifier() & ActionEvent::MODIFIER_SHIFT) != 0;
	if (!processor_.process(pwszTemplate, bExternalEditor)) {
		ActionUtil::error(hwnd_, IDS_ERROR_APPLYTEMPLATE);
		return;
	}
}

bool qm::MessageApplyTemplateAction::isEnabled(const ActionEvent& event)
{
	std::pair<Account*, Folder*> p(pFolderModel_->getCurrent());
	return p.first || p.second;
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
											   HWND hwnd) :
	pMessageSelectionModel_(pMessageSelectionModel),
	pSecurityModel_(pSecurityModel),
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
		NormalFolder* pFolder = l.front()->getFolder();
		unsigned int nFlags = 0;
		if (!pAccount->appendMessage(pFolder, msg, nFlags, 0)) {
			ActionUtil::error(hwnd_, IDS_ERROR_COMBINE);
			return;
		}
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
											 const WCHAR* pwszTemplateName,
											 EditFrameWindowManager* pEditFrameWindowManager,
											 ExternalEditorManager* pExternalEditorManager,
											 HWND hwnd,
											 Profile* pProfile,
											 bool bExternalEditor) :
	processor_(pDocument, pFolderModel, pMessageSelectionModel,
		pEncodingModel, pSecurityModel, pEditFrameWindowManager,
		pExternalEditorManager, hwnd, pProfile, bExternalEditor),
	pFolderModel_(pFolderModel),
	hwnd_(hwnd)
{
	wstrTemplateName_ = allocWString(pwszTemplateName);
}

qm::MessageCreateAction::~MessageCreateAction()
{
}

void qm::MessageCreateAction::invoke(const ActionEvent& event)
{
	if (!processor_.process(wstrTemplateName_.get(),
		(event.getModifier() & ActionEvent::MODIFIER_SHIFT) != 0)) {
		ActionUtil::error(hwnd_, IDS_ERROR_CREATEMESSAGE);
		return;
	}
}

bool qm::MessageCreateAction::isEnabled(const ActionEvent& event)
{
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
	if (wstrMessage.get()) {
		MessageCreator creator(MessageCreator::FLAG_ADDCONTENTTYPE |
			MessageCreator::FLAG_EXPANDALIAS |
			MessageCreator::FLAG_EXTRACTATTACHMENT |
			MessageCreator::FLAG_ENCODETEXT,
			pSecurityModel_->getSecurityMode());
		std::auto_ptr<Message> pMessage(creator.createMessage(pDocument_, wstrMessage.get(), -1));
		
		unsigned int nFlags = 0;
		// TODO
		// Set flags
		if (!composer_.compose(0, 0, pMessage.get(), nFlags, 0)) {
			ActionUtil::error(hwnd_, IDS_ERROR_CREATEMESSAGE);
			return;
		}
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
	if (!event.getParam())
		return;
	ActionParam* pParam = static_cast<ActionParam*>(event.getParam());
	if (pParam->nArgs_ == 0)
		return;
	Variant v;
	if (::VariantChangeType(&v, pParam->ppvarArgs_[0], 0, VT_BSTR) != S_OK)
		return;
	
	const WCHAR* pwszPath = v.bstrVal;
	unsigned int nFlags = 0;
	// TODO
	if (!composer_.compose(0, 0, pwszPath, nFlags)) {
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
																 HWND hwnd) :
	pMessageSelectionModel_(pMessageSelectionModel),
	pSecurityModel_(pSecurityModel),
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
	
	if (!l.empty()) {
		Account* pAccount = lock.get();
		if (!deleteAttachment(pAccount, pFolder, l)) {
			ActionUtil::error(hwnd_, IDS_ERROR_DELETEATTACHMENT);
			return;
		}
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
	for (MessageHolderList::const_iterator it = l.begin(); it != l.end(); ++it) {
		if (!deleteAttachment(pAccount, pFolder, *it))
			return false;
	}
	return true;
}

bool qm::MessageDeleteAttachmentAction::deleteAttachment(Account* pAccount,
														 Folder* pFolder,
														 MessageHolder* pmh) const
{
	Message msg;
	if (!pmh->getMessage(Account::GETMESSAGEFLAG_ALL,
		0, pSecurityModel_->getSecurityMode(), &msg))
		return false;
	
	AttachmentParser::removeAttachments(&msg);
	AttachmentParser::setAttachmentDeleted(&msg);
	
	NormalFolder* pNormalFolder = pmh->getFolder();
	if (!pAccount->appendMessage(pNormalFolder, msg,
		pmh->getFlags() & MessageHolder::FLAG_USER_MASK, 0))
		return false;
	
	if (!pAccount->removeMessages(MessageHolderList(1, pmh), pFolder, false, 0))
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
														 HWND hwnd) :
	pMessageSelectionModel_(pMessageSelectionModel),
	pSecurityModel_(pSecurityModel),
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
	
	if (!l.empty()) {
		Account* pAccount = lock.get();
		if (!expandDigest(pAccount, l)) {
			ActionUtil::error(hwnd_, IDS_ERROR_EXPANDDIGEST);
			return;
		}
	}
}

bool qm::MessageExpandDigestAction::isEnabled(const ActionEvent& event)
{
	return pMessageSelectionModel_->hasSelectedMessage();
}

bool qm::MessageExpandDigestAction::expandDigest(Account* pAccount,
												 const MessageHolderList& l)
{
	for (MessageHolderList::const_iterator it = l.begin(); it != l.end(); ++it) {
		if (!expandDigest(pAccount, *it))
			return false;
	}
	return true;
}

bool qm::MessageExpandDigestAction::expandDigest(Account* pAccount,
												 MessageHolder* pmh)
{
	Message msg;
	if (!pmh->getMessage(Account::GETMESSAGEFLAG_ALL,
		0, pSecurityModel_->getSecurityMode(), &msg))
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
		// Set flags?
		unsigned int nFlags = 0;
		if (!pAccount->appendMessage(pmh->getFolder(), **it, nFlags, 0))
			return false;
	}
	
	return true;
}


/****************************************************************************
 *
 * MessageMarkAction
 *
 */

qm::MessageMarkAction::MessageMarkAction(MessageSelectionModel* pModel,
										 unsigned int nFlags,
										 unsigned int nMask,
										 HWND hwnd) :
	pModel_(pModel),
	nFlags_(nFlags),
	nMask_(nMask),
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
	
	if (!l.empty()) {
		Account* pAccount = lock.get();
		if (!pAccount->setMessagesFlags(l, nFlags_, nMask_)) {
			ActionUtil::error(hwnd_, IDS_ERROR_MARKMESSAGE);
			return;
		}
	}
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

qm::MessageMoveAction::MessageMoveAction(MessageSelectionModel* pMessageSelectionModel,
										 MoveMenu* pMoveMenu,
										 HWND hwnd) :
	pMessageSelectionModel_(pMessageSelectionModel),
	pMoveMenu_(pMoveMenu),
	hwnd_(hwnd)
{
}

qm::MessageMoveAction::~MessageMoveAction()
{
}

void qm::MessageMoveAction::invoke(const ActionEvent& event)
{
	NormalFolder* pFolderTo = pMoveMenu_->getFolder(event.getId());
	if (pFolderTo) {
		AccountLock lock;
		Folder* pFolderFrom = 0;
		MessageHolderList l;
		pMessageSelectionModel_->getSelectedMessages(&lock, &pFolderFrom, &l);
		
		if (!l.empty()) {
			Account* pAccount = lock.get();
			bool bMove = (event.getModifier() & ActionEvent::MODIFIER_CTRL) == 0;
			UINT nId = bMove ? IDS_MOVEMESSAGE : IDS_COPYMESSAGE;
			ProgressDialogMessageOperationCallback callback(hwnd_, nId, nId);
			if (!pAccount->copyMessages(l, pFolderFrom, pFolderTo, bMove, &callback)) {
				ActionUtil::error(hwnd_, bMove ? IDS_ERROR_MOVEMESSAGE : IDS_ERROR_COPYMESSAGE);
				return;
			}
		}
	}
}

bool qm::MessageMoveAction::isEnabled(const ActionEvent& event)
{
	return pMessageSelectionModel_->hasSelectedMessage();
}


/****************************************************************************
 *
 * MessageMoveOtherAction
 *
 */

qm::MessageMoveOtherAction::MessageMoveOtherAction(Document* pDocument,
												   MessageSelectionModel* pMessageSelectionModel,
												   Profile* pProfile,
												   HWND hwnd) :
	pDocument_(pDocument),
	pMessageSelectionModel_(pMessageSelectionModel),
	pProfile_(pProfile),
	hwnd_(hwnd)
{
}

qm::MessageMoveOtherAction::~MessageMoveOtherAction()
{
}

void qm::MessageMoveOtherAction::invoke(const ActionEvent& event)
{
	AccountLock lock;
	Folder* pFolderFrom = 0;
	MessageHolderList l;
	pMessageSelectionModel_->getSelectedMessages(&lock, &pFolderFrom, &l);
	if (l.empty())
		return;
	
	Account* pAccount = lock.get();
	
	MoveMessageDialog dialog(pDocument_, pAccount, pProfile_);
	if (dialog.doModal(hwnd_) != IDOK)
		return;
	
	NormalFolder* pFolderTo = dialog.getFolder();
	if (!pFolderTo)
		return;
	
	bool bMove = !dialog.isCopy();
	
	UINT nId = bMove ? IDS_MOVEMESSAGE : IDS_COPYMESSAGE;
	ProgressDialogMessageOperationCallback callback(hwnd_, nId, nId);
	if (!pAccount->copyMessages(l, pFolderFrom, pFolderTo, bMove, &callback)) {
		ActionUtil::error(hwnd_, bMove ? IDS_ERROR_MOVEMESSAGE : IDS_ERROR_COPYMESSAGE);
		return;
	}
}

bool qm::MessageMoveOtherAction::isEnabled(const ActionEvent& event)
{
	return pMessageSelectionModel_->hasSelectedMessage();
}


/****************************************************************************
 *
 * MessageOpenAttachmentAction
 *
 */

qm::MessageOpenAttachmentAction::MessageOpenAttachmentAction(SecurityModel* pSecurityModel,
															 Profile* pProfile,
															 AttachmentMenu* pAttachmentMenu,
															 TempFileCleaner* pTempFileCleaner,
															 HWND hwnd) :
	pAttachmentMenu_(pAttachmentMenu),
	helper_(pSecurityModel, pProfile, pTempFileCleaner, hwnd),
	hwnd_(hwnd)
{
}

qm::MessageOpenAttachmentAction::~MessageOpenAttachmentAction()
{
}

void qm::MessageOpenAttachmentAction::invoke(const ActionEvent& event)
{
	Message msg;
	wstring_ptr wstrName;
	const Part* pPart = 0;
	if (!pAttachmentMenu_->getPart(event.getId(), &msg, &wstrName, &pPart)) {
		ActionUtil::error(hwnd_, IDS_ERROR_EXECUTEATTACHMENT);
		return;
	}
	
	bool bExternalEditor = (event.getModifier() & ActionEvent::MODIFIER_SHIFT) != 0;
	if (helper_.open(pPart, wstrName.get(), bExternalEditor) == AttachmentParser::RESULT_FAIL) {
		ActionUtil::error(hwnd_, IDS_ERROR_EXECUTEATTACHMENT);
		return;
	}
}


/****************************************************************************
 *
 * MessageOpenLinkAction
 *
 */

qm::MessageOpenLinkAction::MessageOpenLinkAction(MessageSelectionModel* pMessageSelectionModel,
												 HWND hwnd) :
	pMessageSelectionModel_(pMessageSelectionModel),
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
		Account* pAccount = mpl->getFolder()->getAccount();
		if (!pAccount->isSupport(Account::SUPPORT_EXTERNALLINK))
			return;
		
		Message msg;
		if (!mpl->getMessage(Account::GETMESSAGEFLAG_HEADER,
			L"X-QMAIL-Link", SECURITYMODE_NONE, &msg))
			return;
		
		UnstructuredParser link;
		if (msg.getField(L"X-QMAIL-Link", &link) == Part::FIELD_EXIST)
			UIUtil::openURL(hwnd_, link.getValue());
	}
}

bool qm::MessageOpenLinkAction::isEnabled(const qs::ActionEvent& event)
{
	return pMessageSelectionModel_->hasFocusedMessage();
}


/****************************************************************************
 *
 * MessageOpenRecent
 *
 */

qm::MessageOpenRecentAction::MessageOpenRecentAction(RecentsMenu* pRecentsMenu,
													 Document* pDocument,
													 ViewModelManager* pViewModelManager,
													 MessageFrameWindowManager* pMessageFrameWindowManager) :
	pRecentsMenu_(pRecentsMenu),
	pDocument_(pDocument),
	pViewModelManager_(pViewModelManager),
	pMessageFrameWindowManager_(pMessageFrameWindowManager)
{
}

qm::MessageOpenRecentAction::~MessageOpenRecentAction()
{
}

void qm::MessageOpenRecentAction::invoke(const ActionEvent& event)
{
	const WCHAR* pwszURI = pRecentsMenu_->getURI(event.getId());
	if (pwszURI) {
		std::auto_ptr<URI> pURI(URI::parse(pwszURI));
		if (pURI.get()) {
			MessagePtrLock mpl(pDocument_->getMessage(*pURI.get()));
			if (mpl) {
				ViewModel* pViewModel = pViewModelManager_->getViewModel(mpl->getFolder());
				if (!pMessageFrameWindowManager_->open(pViewModel, mpl)) {
					// TODO MSG
				}
			}
		}
		pDocument_->getRecents()->remove(pwszURI);
	}
}


/****************************************************************************
 *
 * MessageOpenURLAction
 *
 */

qm::MessageOpenURLAction::MessageOpenURLAction(Document* pDocument,
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
		pExternalEditorManager, hwnd, pProfile, bExternalEditor),
	pDocument_(pDocument),
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
	if (!event.getParam())
		return;
	
	ActionParam* pParam = static_cast<ActionParam*>(event.getParam());
	if (pParam->nArgs_ == 0)
		return;
	
	Variant v;
	if (::VariantChangeType(&v, pParam->ppvarArgs_[0], 0, VT_BSTR) != S_OK)
		return;
	
	std::pair<Account*, Folder*> p(pFolderModel_->getCurrent());
	Account* pAccount = p.first ? p.first : p.second ? p.second->getAccount() : 0;
	if (!pAccount || wcscmp(pAccount->getClass(), L"mail") != 0) {
		wstring_ptr wstrAccount(pProfile_->getString(L"Global", L"DefaultMailAccount", L""));
		if (*wstrAccount.get()) {
			pAccount = pDocument_->getAccount(wstrAccount.get());
		}
		else {
			pAccount = 0;
			const Document::AccountList& l = pDocument_->getAccounts();
			for (Document::AccountList::const_iterator it = l.begin(); it != l.end() && !pAccount; ++it) {
				if (wcscmp((*it)->getClass(), L"mail") == 0)
					pAccount = *it;
			}
		}
		if (!pAccount)
			return;
	}
	else {
		pAccount = 0;
	}
	
	TemplateContext::Argument arg = {
		L"url",
		v.bstrVal
	};
	TemplateContext::ArgumentList listArgument(1, arg);
	bool bExternalEditor = (event.getModifier() & ActionEvent::MODIFIER_SHIFT) != 0;
	if (!processor_.process(L"url", listArgument, bExternalEditor, pAccount)) {
		ActionUtil::error(hwnd_, IDS_ERROR_OPENURL);
		return;
	}
}


/****************************************************************************
 *
 * MessagePropertyAction
 *
 */

qm::MessagePropertyAction::MessagePropertyAction(MessageSelectionModel* pMessageSelectionModel,
												 HWND hwnd) :
	pMessageSelectionModel_(pMessageSelectionModel),
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
	wstring_ptr wstrTitle(loadString(hInst, IDS_PROPERTY));
	
	MessagePropertyPage page(l);
	PropertySheetBase sheet(hInst, wstrTitle.get(), false);
	sheet.add(&page);
	
	if (sheet.doModal(hwnd_) == IDOK) {
		if (!pAccount->setMessagesFlags(l, page.getFlags(), page.getMask())) {
			ActionUtil::error(hwnd_, IDS_ERROR_SETFLAGS);
			return;
		}
	}
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
	
	Folder* pSearchFolder = pAccount->getFolderByFlag(Folder::FLAG_SEARCHBOX);
	if (!pSearchFolder || pSearchFolder->getType() != Folder::TYPE_QUERY)
		return;
	QueryFolder* pSearch = static_cast<QueryFolder*>(pSearchFolder);
	
	HINSTANCE hInst = Application::getApplication().getResourceHandle();
	wstring_ptr wstrTitle(loadString(hInst, IDS_SEARCH));
	
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
	
	wstring_ptr wstrStartName(pProfile_->getString(L"Search", L"Page", 0));
	
	int nStartPage = 0;
	PropertySheetBase sheet(hInst, wstrTitle.get(), false);
	for (UIList::size_type n = 0; n < listUI.size(); ++n) {
		std::auto_ptr<SearchPropertyPage> pPage(
			listUI[n].first->createPropertyPage(pFolder == 0));
		listUI[n].second = pPage.release();
		sheet.add(listUI[n].second);
		if (wcscmp(listUI[n].second->getDriver(), wstrStartName.get()) == 0)
			nStartPage = n;
	}
	sheet.setStartPage(nStartPage);
	
	if (sheet.doModal(hwnd_) == IDOK) {
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
				
				wstring_ptr wstrFolder;
				if (!pPage->isAllFolder())
					wstrFolder = pFolder->getFullName();
				
				pSearch->set(pPage->getDriver(), pwszCondition,
					wstrFolder.get(), pPage->isRecursive());
				
				if (pFolder != pSearch)
					pFolderModel_->setCurrent(0, pSearch, false);
				
				if (pFolder == pSearch || !pSearch->isFlag(Folder::FLAG_SYNCWHENOPEN)) {
					if (!pSearch->search(pDocument_, hwnd_,
						pProfile_, pSecurityModel_->getSecurityMode())) {
						ActionUtil::error(hwnd_, IDS_ERROR_SEARCH);
						return;
					}
				}
			}
			pProfile_->setString(L"Search", L"Page", pPage->getDriver());
		}
	}
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
	std::pair<Account*, Folder*> p(FolderActionUtil::getFocused(pFolderSelectionModel_));
	if (p.first)
		pTabModel_->open(p.first);
	else if (p.second)
		pTabModel_->open(p.second);
}

bool qm::TabCreateAction::isEnabled(const ActionEvent& event)
{
	std::pair<Account*, Folder*> p(FolderActionUtil::getFocused(pFolderSelectionModel_));
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

qm::TabSelectAction::TabSelectAction(TabModel* pTabModel,
									 unsigned int nBaseId) :
	pTabModel_(pTabModel),
	nBaseId_(nBaseId)
{
}

qm::TabSelectAction::~TabSelectAction()
{
}

void qm::TabSelectAction::invoke(const qs::ActionEvent& event)
{
	int nItem = getItem(event.getId());
	if (nItem < 0 || pTabModel_->getCount() <= nItem)
		return;
	pTabModel_->setCurrent(nItem);
}

bool qm::TabSelectAction::isEnabled(const qs::ActionEvent& event)
{
	int nItem = getItem(event.getId());
	return 0 <= nItem && nItem < pTabModel_->getCount();
}

int qm::TabSelectAction::getItem(unsigned int nId) const
{
	return nId - nBaseId_;
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
										 Profile* pProfile,
										 HWND hwnd) :
	pDocument_(pDocument),
	pFolderModel_(pFolderModel),
	pPasswordManager_(pPasswordManager),
	pSyncManager_(pSyncManager),
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
		ActionUtil::error(hwnd_, IDS_SYNCHRONIZING);
		return;
	}
	
	bool bOffline = pDocument_->isOffline();
	if (!bOffline)
		pDocument_->setOffline(true);
	
	Account* pAccount = FolderActionUtil::getAccount(pFolderModel_);
	AccountDialog dialog(pDocument_, pAccount, pPasswordManager_,
		pSyncManager_->getSyncFilterManager(), pProfile_);
	dialog.doModal(hwnd_, 0);
	
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
	AddressListParser from(0);
	if (msg.getField(L"From", &from) != Part::FIELD_EXIST || from.getAddressList().empty())
		return;
	AddressParser* pFrom = from.getAddressList().front();
	
	AddAddressDialog dialog(pAddressBook_);
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
			
			AddressBookEntryDialog dialog(pAddressBook_, pEntry.get());
			if (dialog.doModal(hwnd_) != IDOK)
				return;
			
			pAddressBook_->addEntry(pEntry);
		}
		break;
	case AddAddressDialog::TYPE_NEWADDRESS:
		{
			AddressBookEntry* pEntry = dialog.getEntry();
			std::auto_ptr<AddressBookAddress> pAddress(new AddressBookAddress(pEntry,
				wstrAddress.get(), 0, AddressBookAddress::CategoryList(), 0, 0, false));
			
			AddressBookAddressDialog dialog(pAddressBook_, pAddress.get());
			if (dialog.doModal(hwnd_) != IDOK)
				return;
			
			pEntry->addAddress(pAddress);
		}
		break;
	default:
		assert(false);
		break;
	}
	
	if (!pAddressBook_->save()) {
		// TODO
	}
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

qm::ToolAddressBookAction::ToolAddressBookAction(AddressBook* pAddressBook) :
	pAddressBook_(pAddressBook)
{
}

qm::ToolAddressBookAction::~ToolAddressBookAction()
{
}

void qm::ToolAddressBookAction::invoke(const ActionEvent& event)
{
	// TODO
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
		std::auto_ptr<SyncData> pData(new SyncData(pSyncManager_,
			pDocument_, hwnd_, false, SyncDialog::FLAG_SHOWDIALOG));
		
		std::auto_ptr<SyncDialup> pDialup(new SyncDialup(
			static_cast<const WCHAR*>(0),
			SyncDialup::FLAG_SHOWDIALOG | SyncDialup::FLAG_NOTDISCONNECT,
			static_cast<const WCHAR*>(0), 0));
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
			RasConnection::Result result = pRasConnection->disconnect(false);
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
	UINT nId = isConnected() ? IDS_DIALUPDISCONNECT : IDS_DIALUPCONNECT;
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
										 HWND hwnd,
										 GoRoundMenu* pGoRoundMenu) :
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

void qm::ToolGoRoundAction::invoke(const ActionEvent& event)
{
	const GoRoundCourse* pCourse = 0;
	if (event.getParam()) {
		ActionParam* pParam = static_cast<ActionParam*>(event.getParam());
		if (pParam->nArgs_ > 0) {
			Variant v;
			if (::VariantChangeType(&v, pParam->ppvarArgs_[0], 0, VT_BSTR) != S_OK)
				return;
			
			pCourse = pGoRound_->getCourse(v.bstrVal);
		}
	}
	else {
		pCourse = pGoRoundMenu_->getCourse(event.getId());
	}
	
	if (!SyncUtil::goRound(pSyncManager_, pDocument_,
		pSyncDialogManager_, hwnd_, SyncDialog::FLAG_SHOWDIALOG, pCourse)) {
		ActionUtil::error(hwnd_, IDS_ERROR_GOROUND);
		return;
	}
}


/****************************************************************************
 *
 * ToolOptionsAction
 *
 */

qm::ToolOptionsAction::ToolOptionsAction(Profile* pProfile,
										 HWND hwnd) :
	pProfile_(pProfile),
	hwnd_(hwnd)
{
}

qm::ToolOptionsAction::~ToolOptionsAction()
{
}

void qm::ToolOptionsAction::invoke(const ActionEvent& event)
{
	// TODO
}

bool qm::ToolOptionsAction::isEnabled(const ActionEvent& event)
{
	// TODO
	// Check wether syncing or not
	return false;
}


/****************************************************************************
 *
 * ToolScriptAction
 *
 */

qm::ToolScriptAction::ToolScriptAction(ScriptMenu* pScriptMenu,
									   Document* pDocument,
									   Profile* pProfile,
									   MainWindow* pMainWindow) :
	pScriptMenu_(pScriptMenu),
	pDocument_(pDocument),
	pProfile_(pProfile),
	pMainWindow_(pMainWindow),
	pEditFrameWindow_(0),
	pMessageFrameWindow_(0)
{
}

qm::ToolScriptAction::ToolScriptAction(ScriptMenu* pScriptMenu,
									   Document* pDocument,
									   Profile* pProfile,
									   EditFrameWindow* pEditFrameWindow) :
	pScriptMenu_(pScriptMenu),
	pDocument_(pDocument),
	pProfile_(pProfile),
	pMainWindow_(0),
	pEditFrameWindow_(pEditFrameWindow),
	pMessageFrameWindow_(0)
{
}

qm::ToolScriptAction::ToolScriptAction(ScriptMenu* pScriptMenu,
									   Document* pDocument,
									   Profile* pProfile,
									   MessageFrameWindow* pMessageFrameWindow) :
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

void qm::ToolScriptAction::invoke(const ActionEvent& event)
{
	const WCHAR* pwszName = pScriptMenu_->getScript(event.getId());
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
	
	ScriptManager* pScriptManager = pScriptMenu_->getScriptManager();
	std::auto_ptr<Script> pScript(pScriptManager->getScript(
		pwszName, pDocument_, pProfile_, getModalHandler(), info));
	if (pScript.get())
		pScript->run(0, 0, 0);
}

bool qm::ToolScriptAction::isEnabled(const ActionEvent& event)
{
	// TODO
	// Check wether syncing or not
	return true;
}


/****************************************************************************
 *
 * ToolSubAccountAction
 *
 */

qm::ToolSubAccountAction::ToolSubAccountAction(Document* pDocument,
											   FolderModel* pFolderModel,
											   SubAccountMenu* pSubAccountMenu,
											   SyncManager* pSyncManager,
											   HWND hwnd) :
	pDocument_(pDocument),
	pFolderModel_(pFolderModel),
	pSubAccountMenu_(pSubAccountMenu),
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
		ActionUtil::error(hwnd_, IDS_SYNCHRONIZING);
		return;
	}
	
	const WCHAR* pwszName = pSubAccountMenu_->getName(event.getId());
	if (!pwszName)
		return;
	
	const Document::AccountList& listAccount = pDocument_->getAccounts();
	for (Document::AccountList::const_iterator it = listAccount.begin(); it != listAccount.end(); ++it) {
		Account* pAccount = *it;
		SubAccount* pSubAccount = pAccount->getSubAccount(pwszName);
		if (pSubAccount)
			pAccount->setCurrentSubAccount(pSubAccount);
	}
}

bool qm::ToolSubAccountAction::isEnabled(const ActionEvent& event)
{
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
	
	const WCHAR* pwszName = pSubAccountMenu_->getName(event.getId());
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
								   unsigned int nSync,
								   HWND hwnd) :
	pSyncManager_(pSyncManager),
	pDocument_(pDocument),
	pFolderModel_(pFolderModel),
	pSyncDialogManager_(pSyncDialogManager),
	nSync_(nSync),
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
	
	if (!SyncUtil::sync(pSyncManager_, pDocument_, pSyncDialogManager_,
		hwnd_, SyncDialog::FLAG_SHOWDIALOG, pAccount,
		(nSync_ & SYNC_SEND) != 0, (nSync_ & SYNC_RECEIVE) != 0,
		(event.getModifier() & ActionEvent::MODIFIER_SHIFT) != 0)) {
		ActionUtil::error(hwnd_, IDS_ERROR_SYNC);
		return;
	}
}

bool qm::ToolSyncAction::isEnabled(const ActionEvent& event)
{
	return FolderActionUtil::getAccount(pFolderModel_) != 0;
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
	pEncodingModel_(pEncodingModel),
	pEncodingMenu_(0)
{
}

qm::ViewEncodingAction::ViewEncodingAction(EncodingModel* pEncodingModel,
										   EncodingMenu* pEncodingMenu) :
	pEncodingModel_(pEncodingModel),
	pEncodingMenu_(pEncodingMenu)
{
}

qm::ViewEncodingAction::~ViewEncodingAction()
{
}

void qm::ViewEncodingAction::invoke(const ActionEvent& event)
{
	const WCHAR* pwszEncoding = 0;
	if (pEncodingMenu_)
		pwszEncoding = pEncodingMenu_->getEncoding(event.getId());
	pEncodingModel_->setEncoding(pwszEncoding);
}

bool qm::ViewEncodingAction::isChecked(const ActionEvent& event)
{
	if (pEncodingMenu_) {
		const WCHAR* pwszEncoding = pEncodingModel_->getEncoding();
		return pwszEncoding &&
			wcscmp(pwszEncoding, pEncodingMenu_->getEncoding(event.getId())) == 0;
	}
	else {
		return !pEncodingModel_->getEncoding();
	}
}


/****************************************************************************
 *
 * ViewFilterAction
 *
 */

qm::ViewFilterAction::ViewFilterAction(ViewModelManager* pViewModelManager,
									   FilterMenu* pFilterMenu) :
	pViewModelManager_(pViewModelManager),
	pFilterMenu_(pFilterMenu)
{
}

qm::ViewFilterAction::~ViewFilterAction()
{
}

void qm::ViewFilterAction::invoke(const ActionEvent& event)
{
	ViewModel* pViewModel = pViewModelManager_->getCurrentViewModel();
	if (pViewModel) {
		const Filter* pFilter = pFilterMenu_->getFilter(event.getId());
		pViewModel->setFilter(pFilter);
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
		const Filter* pFilter = pViewModel->getFilter();
		return pFilter &&
			wcscmp(pFilter->getName(),
				pFilterMenu_->getFilter(event.getId())->getName()) == 0;
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
			MacroParser parser(MacroParser::TYPE_FILTER);
			std::auto_ptr<Macro> pCondition(parser.parse(dialog.getCondition()));
			if (!pCondition.get()) {
				// TODO
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
 * ViewFilterNoneAction
 *
 */

qm::ViewFilterNoneAction::ViewFilterNoneAction(ViewModelManager* pViewModelManager) :
	pViewModelManager_(pViewModelManager)
{
}

qm::ViewFilterNoneAction::~ViewFilterNoneAction()
{
}

void qm::ViewFilterNoneAction::invoke(const ActionEvent& event)
{
	ViewModel* pViewModel = pViewModelManager_->getCurrentViewModel();
	if (pViewModel)
		pViewModel->setFilter(0);
}

bool qm::ViewFilterNoneAction::isEnabled(const ActionEvent& event)
{
	return pViewModelManager_->getCurrentViewModel() != 0;
}

bool qm::ViewFilterNoneAction::isChecked(const ActionEvent& event)
{
	ViewModel* pViewModel = pViewModelManager_->getCurrentViewModel();
	if (pViewModel)
		return !pViewModel->getFilter();
	else
		return false;
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
	int nViewCount = listView_.size();
	
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

qm::ViewLockPreviewAction::ViewLockPreviewAction(PreviewMessageModel* pPreviewMessageModel) :
	pPreviewMessageModel_(pPreviewMessageModel)
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


/****************************************************************************
 *
 * ViewMessageModeAction
 *
 */

qm::ViewMessageModeAction::ViewMessageModeAction(MessageViewModeHolder* pMessageViewModeHolder,
												 MessageViewMode::Mode mode,
												 bool bEnabled) :
	pMessageViewModeHolder_(pMessageViewModeHolder),
	mode_(mode),
	bEnabled_(bEnabled)
{
}

qm::ViewMessageModeAction::~ViewMessageModeAction()
{
}

void qm::ViewMessageModeAction::invoke(const ActionEvent& event)
{
	MessageViewMode* pMode = pMessageViewModeHolder_->getMessageViewMode();
	if (pMode)
		pMode->setMode(mode_, !pMode->isMode(mode_));
}

bool qm::ViewMessageModeAction::isEnabled(const ActionEvent& event)
{
	return bEnabled_ && pMessageViewModeHolder_->getMessageViewMode();
}

bool qm::ViewMessageModeAction::isChecked(const ActionEvent& event)
{
	MessageViewMode* pMode = pMessageViewModeHolder_->getMessageViewMode();
	return pMode && pMode->isMode(mode_);
}


/****************************************************************************
 *
 * ViewNavigateFolderAction
 *
 */

qm::ViewNavigateFolderAction::ViewNavigateFolderAction(Document* pDocument,
													   FolderModel* pFolderModel,
													   Type type) :
	pDocument_(pDocument),
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
		if (pFolder)
			pFolderModel_->setCurrent(0, pFolder, true);
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
														 Type type) :
	pViewModelManager_(pViewModelManager),
	pFolderModel_(pFolderModel),
	pViewModelHolder_(0),
	pMainWindow_(pMainWindow),
	pMessageWindow_(pMessageWindow),
	type_(type)
{
	assert(pViewModelManager);
	assert(pMessageWindow);
}

qm::ViewNavigateMessageAction::ViewNavigateMessageAction(ViewModelManager* pViewModelManager,
														 ViewModelHolder* pViewModelHolder,
														 MessageWindow* pMessageWindow,
														 Type type) :
	pViewModelManager_(pViewModelManager),
	pFolderModel_(0),
	pViewModelHolder_(pViewModelHolder),
	pMainWindow_(0),
	pMessageWindow_(pMessageWindow),
	type_(type)
{
	assert(pViewModelManager);
	assert(pMessageWindow);
}

qm::ViewNavigateMessageAction::~ViewNavigateMessageAction()
{
}

void qm::ViewNavigateMessageAction::invoke(const ActionEvent& event)
{
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
	
	if (bPreview &&
		(!pMainWindow_->isShowPreviewWindow() ||
		 !static_cast<PreviewMessageModel*>(pMessageModel)->isConnectedToViewModel()) &&
		(type == TYPE_NEXTPAGE || type == TYPE_NEXTPAGE || type == TYPE_SELF))
		return;
	
	bool bScrolled = true;
	switch (type) {
	case TYPE_NEXTPAGE:
		if (pMessageWindow_->scrollPage(false))
			return;
		type = TYPE_NEXT;
		break;
	case TYPE_PREVPAGE:
		if (pMessageWindow_->scrollPage(true))
			return;
		type = TYPE_PREV;
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
		
		if (nIndex != -1 && type != TYPE_SELF) {
			pViewModel->setFocused(nIndex);
			pViewModel->setSelection(nIndex);
			pViewModel->setLastSelection(nIndex);
			pViewModel->payAttention(nIndex);
		}
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
			if (!pmh->isFlag(MessageHolder::FLAG_SEEN)) {
				bFound = true;
				break;
			}
		}
		if (!bFound) {
			for (nIndex = 0; nIndex < nStart; ++nIndex) {
				MessageHolder* pmh = pViewModel->getMessageHolder(nIndex);
				if (!pmh->isFlag(MessageHolder::FLAG_SEEN)) {
					bFound = true;
					break;
				}
			}
		}
	}
	if (!bFound) {
		Folder* pFolder = pViewModel->getFolder();
		Account* pAccount = pFolder->getAccount();
		const Account::FolderList& l = pAccount->getFolders();
		Account::FolderList listFolder(l);
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
			pViewModel = pViewModelManager_->getViewModel(pUnseenFolder);
			unseen = getNextUnseen(pViewModel, 0);
		}
	}
	else {
		unseen.second = nIndex;
	}
	
	return unseen;
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
				hwnd_, SyncDialog::FLAG_NONE, static_cast<NormalFolder*>(pFolder), 0)) {
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
			if (nIndex != -1) {
				pViewModel->setFocused(nIndex);
				pViewModel->setSelection(nIndex);
				pViewModel->setLastSelection(nIndex);
				pViewModel->payAttention(nIndex);
			}
		}
	}
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
		IDS_SHOWFOLDER, IDS_HIDEFOLDER)
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
		IDS_SHOWHEADER, IDS_HIDEHEADER)
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
		IDS_SHOWHEADERCOLUMN, IDS_HIDEHEADERCOLUMN)
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
		IDS_SHOWPREVIEW, IDS_HIDEPREVIEW)
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
		&qm::TabWindow::isShowTab, IDS_SHOWTAB, IDS_HIDETAB)
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

qm::ViewSortAction::ViewSortAction(ViewModelManager* pViewModelManager,
								   SortMenu* pSortMenu) :
	pViewModelManager_(pViewModelManager),
	pSortMenu_(pSortMenu)
{
}

qm::ViewSortAction::~ViewSortAction()
{
}

void qm::ViewSortAction::invoke(const ActionEvent& event)
{
	ViewModel* pViewModel = pViewModelManager_->getCurrentViewModel();
	if (pViewModel) {
		unsigned int nColumn = pSortMenu_->getSort(event.getId());
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
	if (pViewModel)
		return (pSortMenu_->getSort(event.getId()) & ViewModel::SORT_INDEX_MASK) ==
			(pViewModel->getSort() & ViewModel::SORT_INDEX_MASK);
	else
		return false;
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
 * ViewSortFloatThreadAction
 *
 */

qm::ViewSortFloatThreadAction::ViewSortFloatThreadAction(ViewModelManager* pViewModelManager) :
	pViewModelManager_(pViewModelManager)
{
}

qm::ViewSortFloatThreadAction::~ViewSortFloatThreadAction()
{
}

void qm::ViewSortFloatThreadAction::invoke(const ActionEvent& event)
{
	ViewModel* pViewModel = pViewModelManager_->getCurrentViewModel();
	if (pViewModel) {
		bool bFloat = (pViewModel->getSort() & ViewModel::SORT_FLOATTHREAD) != 0;
		unsigned int nSort = bFloat ? 0 : ViewModel::SORT_FLOATTHREAD;
		pViewModel->setSort(nSort, ViewModel::SORT_FLOATTHREAD);
	}
}

bool qm::ViewSortFloatThreadAction::isEnabled(const ActionEvent& event)
{
	return pViewModelManager_->getCurrentViewModel() != 0;
}

bool qm::ViewSortFloatThreadAction::isChecked(const ActionEvent& event)
{
	ViewModel* pViewModel = pViewModelManager_->getCurrentViewModel();
	return pViewModel && pViewModel->getSort() & ViewModel::SORT_FLOATTHREAD;
}


/****************************************************************************
 *
 * ViewSortThreadAction
 *
 */

qm::ViewSortThreadAction::ViewSortThreadAction(ViewModelManager* pViewModelManager) :
	pViewModelManager_(pViewModelManager)
{
}

qm::ViewSortThreadAction::~ViewSortThreadAction()
{
}

void qm::ViewSortThreadAction::invoke(const ActionEvent& event)
{
	ViewModel* pViewModel = pViewModelManager_->getCurrentViewModel();
	if (pViewModel) {
		bool bThread = (pViewModel->getSort() & ViewModel::SORT_THREAD_MASK) == ViewModel::SORT_THREAD;
		unsigned int nSort = bThread ? ViewModel::SORT_NOTHREAD : ViewModel::SORT_THREAD;
		pViewModel->setSort(nSort, ViewModel::SORT_THREAD_MASK);
	}
}

bool qm::ViewSortThreadAction::isEnabled(const ActionEvent& event)
{
	return pViewModelManager_->getCurrentViewModel() != 0;
}

bool qm::ViewSortThreadAction::isChecked(const ActionEvent& event)
{
	ViewModel* pViewModel = pViewModelManager_->getCurrentViewModel();
	return pViewModel &&
		(pViewModel->getSort() & ViewModel::SORT_THREAD_MASK) ==
			ViewModel::SORT_THREAD;
}


/****************************************************************************
 *
 * ViewTemplateAction
 *
 */

qm::ViewTemplateAction::ViewTemplateAction(MessageWindow* pMessageWindow) :
	pMessageWindow_(pMessageWindow),
	pTemplateMenu_(0)
{
}

qm::ViewTemplateAction::ViewTemplateAction(MessageWindow* pMessageWindow,
										   TemplateMenu* pTemplateMenu) :
	pMessageWindow_(pMessageWindow),
	pTemplateMenu_(pTemplateMenu)
{
}

qm::ViewTemplateAction::~ViewTemplateAction()
{
}

void qm::ViewTemplateAction::invoke(const ActionEvent& event)
{
	Account* pAccount = getAccount();
	if (!pAccount)
		return;
	
	const WCHAR* pwszTemplate = 0;
	if (pTemplateMenu_)
		pwszTemplate = pTemplateMenu_->getTemplate(event.getId(), pAccount);
	
	pMessageWindow_->setTemplate(pwszTemplate);
}

bool qm::ViewTemplateAction::isChecked(const ActionEvent& event)
{
	if (pTemplateMenu_) {
		const WCHAR* pwszTemplate = pMessageWindow_->getTemplate();
		if (!pwszTemplate)
			return false;
		
		Account* pAccount = getAccount();
		if (!pAccount)
			return false;
		return wcscmp(pwszTemplate, pTemplateMenu_->getTemplate(event.getId(), pAccount)) == 0;
	}
	else {
		return !pMessageWindow_->getTemplate();
	}
}

Account* qm::ViewTemplateAction::getAccount() const
{
	MessageModel* pMessageModel = pMessageWindow_->getMessageModel();
	return pMessageModel->getCurrentAccount();
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


/****************************************************************************
 *
 * FolderActionUtil
 *
 */

std::pair<Account*, Folder*> qm::FolderActionUtil::getFocused(FolderSelectionModel* pModel)
{
	std::pair<Account*, Folder*> p(pModel->getTemporaryFocused());
	if (!p.first && !p.second) {
		p.second = pModel->getFocusedFolder();
		p.first = pModel->getAccount();
	}
	return p;
}

void qm::FolderActionUtil::getSelected(FolderSelectionModel* pModel,
									   Account::FolderList* pListFolder)
{
	std::pair<Account*, Folder*> p(pModel->getTemporaryFocused());
	if (p.first)
		;
	else if (p.second)
		pListFolder->push_back(p.second);
	else
		pModel->getSelectedFolders(pListFolder);
}

bool qm::FolderActionUtil::hasSelected(FolderSelectionModel* pModel)
{
	std::pair<Account*, Folder*> p(pModel->getTemporaryFocused());
	if (p.first)
		return false;
	else if (p.second)
		return true;
	else
		return pModel->hasSelectedFolder();
}

std::pair<Account*, Folder*> qm::FolderActionUtil::getCurrent(FolderModel* pModel)
{
	std::pair<Account*, Folder*> p(pModel->getTemporary());
	if (!p.first && !p.second)
		p = pModel->getCurrent();
	return p;
}

Account* qm::FolderActionUtil::getAccount(FolderModel* pModel)
{
	std::pair<Account*, Folder*> p(getCurrent(pModel));
	return p.first ? p.first : p.second ? p.second->getAccount() : 0;
}

Folder* qm::FolderActionUtil::getFolder(FolderModel* pModel)
{
	return getCurrent(pModel).second;
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
