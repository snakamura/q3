/*
 * $Id$
 *
 * Copyright(C) 1998-2003 Satoshi Nakamura
 * All rights reserved.
 *
 */

#include <qmaccount.h>

#include <qserror.h>
#include <qsnew.h>

#include <memory>

#include "syncdialog.h"
#include "syncutil.h"
#include "../sync/syncmanager.h"

using namespace qm;
using namespace qs;


/****************************************************************************
 *
 * SyncUtil
 *
 */

QSTATUS qm::SyncUtil::syncFolder(SyncManager* pSyncManager, Document* pDocument,
	SyncDialogManager* pSyncDialogManager, HWND hwnd, NormalFolder* pFolder)
{
	assert(pSyncManager);
	assert(pDocument);
	assert(pSyncDialogManager);
	assert(hwnd);
	assert(pFolder);
	assert(pFolder->isFlag(Folder::FLAG_SYNCABLE));
	
	DECLARE_QSTATUS();
	
	std::auto_ptr<SyncData> pData;
	status = newQsObject(pSyncManager, pDocument, hwnd, &pData);
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
