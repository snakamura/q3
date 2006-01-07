/*
 * $Id$
 *
 * Copyright(C) 1998-2005 Satoshi Nakamura
 * All rights reserved.
 *
 */

#pragma warning(disable:4786)

#include <qmaccount.h>
#include <qmdocument.h>
#include <qmgoround.h>

#include <memory>

#include "dialogs.h"
#include "syncdialog.h"
#include "syncutil.h"
#include "../model/goround.h"
#include "../sync/syncmanager.h"

using namespace qm;
using namespace qs;


/****************************************************************************
 *
 * SyncUtil
 *
 */

bool qm::SyncUtil::syncFolder(SyncManager* pSyncManager,
							  Document* pDocument,
							  SyncDialogManager* pSyncDialogManager,
							  unsigned int nCallbackParam,
							  NormalFolder* pFolder,
							  unsigned int nFlags)
{
	Account::NormalFolderList listFolder(1, pFolder);
	return syncFolders(pSyncManager, pDocument,
		pSyncDialogManager, nCallbackParam, listFolder, nFlags);
}

bool qm::SyncUtil::syncFolders(SyncManager* pSyncManager,
							   Document* pDocument,
							   SyncDialogManager* pSyncDialogManager,
							   unsigned int nCallbackParam,
							   const Account::NormalFolderList& listFolder,
							   unsigned int nFlags)
{
	assert(pSyncManager);
	assert(pDocument);
	assert(pSyncDialogManager);
	assert(!listFolder.empty());
	
	std::auto_ptr<SyncData> pData(new SyncData(pSyncManager, pDocument,
		(nCallbackParam & SyncDialog::FLAG_NOTIFYNEWMESSAGE) != 0, nCallbackParam));
	
	Account* pAccount = listFolder.front()->getAccount();
	SubAccount* pSubAccount = pAccount->getCurrentSubAccount();
	
	setDialup(pData.get(), pSubAccount);
	
	for (Account::NormalFolderList::const_iterator it = listFolder.begin(); it != listFolder.end(); ++it) {
		NormalFolder* pFolder = *it;
		
		assert(pFolder->getAccount() == pAccount);
		assert(pFolder->isFlag(Folder::FLAG_SYNCABLE));
		
		pData->addFolder(pAccount, pSubAccount, pFolder,
			pSubAccount->getSyncFilterName(), nFlags);
	}
	
	SyncDialog* pSyncDialog = pSyncDialogManager->open();
	if (!pSyncDialog)
		return false;
	pData->setCallback(pSyncDialog->getSyncManagerCallback());
	
	return pSyncManager->sync(pData);
}

bool qm::SyncUtil::send(SyncManager* pSyncManager,
						Document* pDocument,
						SyncDialogManager* pSyncDialogManager,
						unsigned int nCallbackParam,
						Account* pAccount,
						SubAccount* pSubAccount,
						const WCHAR* pwszMessageId)
{
	assert(pSyncManager);
	assert(pDocument);
	assert(pSyncDialogManager);
	assert(pAccount);
	assert(pSubAccount);
	
	NormalFolder* pOutbox = static_cast<NormalFolder*>(
		pAccount->getFolderByBoxFlag(Folder::FLAG_OUTBOX));
	if (!pOutbox)
		return false;
	
	std::auto_ptr<SyncData> pData(new SyncData(pSyncManager, pDocument,
		(nCallbackParam & SyncDialog::FLAG_NOTIFYNEWMESSAGE) != 0, nCallbackParam));
	setDialup(pData.get(), pSubAccount);
	pData->addSend(pAccount, pSubAccount, SendSyncItem::CRBS_NONE, pwszMessageId);
	
	SyncDialog* pSyncDialog = pSyncDialogManager->open();
	if (!pSyncDialog)
		return false;
	pData->setCallback(pSyncDialog->getSyncManagerCallback());
	
	return pSyncManager->sync(pData);
}

bool qm::SyncUtil::sync(SyncManager* pSyncManager,
						Document* pDocument,
						SyncDialogManager* pSyncDialogManager,
						HWND hwnd,
						unsigned int nCallbackParam,
						Account* pAccount,
						bool bSend,
						bool bReceive,
						bool bSelectSyncFilter)
{
	assert(pSyncManager);
	assert(pDocument);
	assert(pSyncDialogManager);
	assert(pAccount);
	
	SubAccount* pSubAccount = pAccount->getCurrentSubAccount();
	
	std::auto_ptr<SyncData> pData(new SyncData(pSyncManager, pDocument,
		(nCallbackParam & SyncDialog::FLAG_NOTIFYNEWMESSAGE) != 0, nCallbackParam));
	
	setDialup(pData.get(), pSubAccount);
	
	if (bSend) {
		NormalFolder* pOutbox = static_cast<NormalFolder*>(
			pAccount->getFolderByBoxFlag(Folder::FLAG_OUTBOX));
		if (pOutbox)
			pData->addSend(pAccount, pSubAccount, SendSyncItem::CRBS_NONE, 0);
	}
	
	if (bReceive) {
		if (bSelectSyncFilter) {
			SelectSyncFilterDialog dialog(pSyncManager->getSyncFilterManager(),
				pSubAccount->getSyncFilterName());
			if (dialog.doModal(hwnd) != IDOK)
				return true;
			pData->addFolders(pAccount, pSubAccount, 0, dialog.getName());
		}
		else {
			pData->addFolders(pAccount, pSubAccount,
				0, pSubAccount->getSyncFilterName());
		}
	}
	
	if (pData->isEmpty())
		return true;
	
	SyncDialog* pSyncDialog = pSyncDialogManager->open();
	if (!pSyncDialog)
		return false;
	pData->setCallback(pSyncDialog->getSyncManagerCallback());
	
	return pSyncManager->sync(pData);
}

bool qm::SyncUtil::goRound(SyncManager* pSyncManager,
						   Document* pDocument,
						   SyncDialogManager* pSyncDialogManager,
						   unsigned int nCallbackParam,
						   const GoRoundCourse* pCourse)
{
	assert(pSyncManager);
	assert(pDocument);
	assert(pSyncDialogManager);
	
	std::auto_ptr<SyncData> pData(new SyncData(pSyncManager, pDocument,
		(nCallbackParam & SyncDialog::FLAG_NOTIFYNEWMESSAGE) != 0, nCallbackParam));
	if (pCourse) {
		const GoRoundDialup* pDialup = pCourse->getDialup();
		if (pDialup) {
			std::auto_ptr<SyncDialup> pSyncDialup(new SyncDialup(
				pDialup->getName(), pDialup->getFlags(),
				pDialup->getDialFrom(), pDialup->getDisconnectWait()));
			pData->setDialup(pSyncDialup);
		}
		
		bool bParallel = pCourse->getType() == GoRoundCourse::TYPE_PARALLEL;
		const GoRoundCourse::EntryList& l = pCourse->getEntries();
		for (GoRoundCourse::EntryList::const_iterator it = l.begin(); it != l.end(); ++it) {
			GoRoundEntry* pEntry = *it;
			Account* pAccount = pDocument->getAccount(pEntry->getAccount());
			if (pAccount) {
				SubAccount* pSubAccount = 0;
				if (pEntry->getSubAccount())
					pSubAccount = pAccount->getSubAccount(pEntry->getSubAccount());
				if (!pSubAccount)
					pSubAccount = pAccount->getCurrentSubAccount();
				
				const WCHAR* pwszFilter = pEntry->getFilter();
				if (!pwszFilter)
					pwszFilter = pSubAccount->getSyncFilterName();
				
				if (pEntry->isFlag(GoRoundEntry::FLAG_SEND)) {
					NormalFolder* pOutbox = static_cast<NormalFolder*>(
						pAccount->getFolderByBoxFlag(Folder::FLAG_OUTBOX));
					if (pOutbox)
						pData->addSend(pAccount, pSubAccount,
							static_cast<SendSyncItem::ConnectReceiveBeforeSend>(
								pEntry->getConnectReceiveBeforeSend()), 0);
				}
				if (pEntry->isFlag(GoRoundEntry::FLAG_RECEIVE)) {
					if (pEntry->isFlag(GoRoundEntry::FLAG_SELECTFOLDER)) {
						// TODO
					}
					else {
						pData->addFolders(pAccount, pSubAccount,
							pEntry->getFolderPattern(), pwszFilter);
					}
				}
			}
			if (bParallel)
				pData->newSlot();
		}
	}
	else {
		const Document::AccountList& listAccount = pDocument->getAccounts();
		for (Document::AccountList::const_iterator it = listAccount.begin(); it != listAccount.end(); ++it) {
			Account* pAccount = *it;
			SubAccount* pSubAccount = pAccount->getCurrentSubAccount();
			
			NormalFolder* pOutbox = static_cast<NormalFolder*>(
				pAccount->getFolderByBoxFlag(Folder::FLAG_OUTBOX));
			if (pOutbox)
				pData->addSend(pAccount, pSubAccount, SendSyncItem::CRBS_NONE, 0);
			
			pData->addFolders(pAccount, pSubAccount, 0, pSubAccount->getSyncFilterName());
		}
	}
	
	SyncDialog* pSyncDialog = pSyncDialogManager->open();
	if (!pSyncDialog)
		return false;
	pData->setCallback(pSyncDialog->getSyncManagerCallback());
	
	return pSyncManager->sync(pData);
}

void qm::SyncUtil::setDialup(SyncData* pSyncData,
							 const SubAccount* pSubAccount)
{
	assert(pSyncData);
	assert(pSubAccount);
	
	if (pSubAccount->getDialupType() != SubAccount::DIALUPTYPE_NEVER) {
		unsigned int nFlags = 0;
		if (pSubAccount->isDialupShowDialog())
			nFlags |= SyncDialup::FLAG_SHOWDIALOG;
		if (pSubAccount->getDialupType() == SubAccount::DIALUPTYPE_WHENEVERNOTCONNECTED)
			nFlags |= SyncDialup::FLAG_WHENEVERNOTCONNECTED;
		
		std::auto_ptr<SyncDialup> pDialup(new SyncDialup(pSubAccount->getDialupEntry(),
			nFlags, 0, pSubAccount->getDialupDisconnectWait()));
		pSyncData->setDialup(pDialup);
	}
}
