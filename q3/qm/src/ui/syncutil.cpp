/*
 * $Id$
 *
 * Copyright(C) 1998-2003 Satoshi Nakamura
 * All rights reserved.
 *
 */

#include <qmaccount.h>
#include <qmdocument.h>
#include <qmgoround.h>

#include <qserror.h>
#include <qsnew.h>

#include <memory>

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

QSTATUS qm::SyncUtil::syncFolder(SyncManager* pSyncManager,
	Document* pDocument, SyncDialogManager* pSyncDialogManager,
	HWND hwnd, unsigned int nCallbackParam, NormalFolder* pFolder)
{
	assert(pSyncManager);
	assert(pDocument);
	assert(pSyncDialogManager);
	assert(hwnd);
	assert(pFolder);
	assert(pFolder->isFlag(Folder::FLAG_SYNCABLE));
	
	DECLARE_QSTATUS();
	
	std::auto_ptr<SyncData> pData;
	status = newQsObject(pSyncManager, pDocument, hwnd, nCallbackParam, &pData);
	CHECK_QSTATUS();
	Account* pAccount = pFolder->getAccount();
	SubAccount* pSubAccount = pAccount->getCurrentSubAccount();
	status = pData->addFolder(pAccount, pSubAccount,
		pFolder, pSubAccount->getSyncFilterName());
	CHECK_QSTATUS();
	
	SyncDialog* pSyncDialog = 0;
	status = pSyncDialogManager->open(&pSyncDialog);
	CHECK_QSTATUS();
	pData->setCallback(pSyncDialog->getSyncManagerCallback());
	
	status = pSyncManager->sync(pData.get());
	CHECK_QSTATUS();
	pData.release();
	
	return QSTATUS_SUCCESS;
}

QSTATUS qm::SyncUtil::send(SyncManager* pSyncManager, Document* pDocument,
	SyncDialogManager* pSyncDialogManager, HWND hwnd,
	unsigned int nCallbackParam, Account* pAccount, SubAccount* pSubAccount)
{
	DECLARE_QSTATUS();
	
	std::auto_ptr<SyncData> pData;
	status = newQsObject(pSyncManager, pDocument, hwnd, nCallbackParam, &pData);
	CHECK_QSTATUS();
	status = pData->addSend(pAccount, pSubAccount);
	CHECK_QSTATUS();
	
	SyncDialog* pSyncDialog = 0;
	status = pSyncDialogManager->open(&pSyncDialog);
	CHECK_QSTATUS();
	pData->setCallback(pSyncDialog->getSyncManagerCallback());
	
	status = pSyncManager->sync(pData.get());
	CHECK_QSTATUS();
	pData.release();
	
	return QSTATUS_SUCCESS;
}

QSTATUS qm::SyncUtil::createGoRoundData(const GoRoundCourse* pCourse,
	Document* pDocument, SyncData* pData)
{
	assert(pData);
	
	DECLARE_QSTATUS();
	
	if (pCourse) {
		const GoRoundDialup* pDialup = pCourse->getDialup();
		if (pDialup) {
			std::auto_ptr<SyncDialup> pSyncDialup;
			status = newQsObject(pDialup->getName(),
				pDialup->getFlags(), pDialup->getDialFrom(),
				pDialup->getDisconnectWait(), &pSyncDialup);
			CHECK_QSTATUS();
			pData->setDialup(pSyncDialup.release());
		}
		
		bool bParallel = pCourse->getType() == GoRoundCourse::TYPE_PARALLEL;
		const GoRoundCourse::EntryList& l = pCourse->getEntries();
		GoRoundCourse::EntryList::const_iterator it = l.begin();
		while (it != l.end()) {
			GoRoundEntry* pEntry = *it;
			Account* pAccount = pDocument->getAccount(pEntry->getAccount());
			if (pAccount) {
				SubAccount* pSubAccount = 0;
				if (pEntry->getSubAccount())
					pSubAccount = pAccount->getSubAccount(pEntry->getSubAccount());
				if (!pSubAccount)
					pSubAccount = pAccount->getCurrentSubAccount();
				
				const WCHAR* pwszFilterName = pEntry->getFilterName();
				if (!pwszFilterName)
					pwszFilterName = pSubAccount->getSyncFilterName();
				
				if (pEntry->isFlag(GoRoundEntry::FLAG_SEND)) {
					status = pData->addSend(pAccount, pSubAccount);
					CHECK_QSTATUS();
				}
				if (pEntry->isFlag(GoRoundEntry::FLAG_RECEIVE)) {
					if (pEntry->isFlag(GoRoundEntry::FLAG_SELECTFOLDER)) {
						// TODO
					}
					else {
						status = pData->addFolders(pAccount, pSubAccount,
							pEntry->getFolderNamePattern(), pwszFilterName);
						CHECK_QSTATUS();
					}
				}
			}
			if (bParallel)
				pData->newSlot();
			++it;
		}
	}
	else {
		const Document::AccountList& listAccount = pDocument->getAccounts();
		Document::AccountList::const_iterator it = listAccount.begin();
		while (it != listAccount.end()) {
			Account* pAccount = *it;
			SubAccount* pSubAccount = pAccount->getCurrentSubAccount();
			status = pData->addSend(pAccount, pSubAccount);
			CHECK_QSTATUS();
			status = pData->addFolders(pAccount, pSubAccount, 0,
				pSubAccount->getSyncFilterName());
			CHECK_QSTATUS();
			++it;
		}
	}
	
	return QSTATUS_SUCCESS;
}
